#include "supply.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "current-sense.h"
#include "pc-comms.h"
#include "task.h"
#include "calibration.h"

osThreadId_t replayTaskHandle;
const osThreadAttr_t replayTask_attributes = {
        .name = "replayTask",
        .stack_size = 128 * 10,
        .priority = (osPriority_t) osPriorityNormal6,
};

static bool paused = false;
static bool initialized = false;
static bool virtCap_replay_ended = false;
static QueueHandle_t xProfileDataQueue;
static supplyOperation *replay_operation;

static float virtCapCapacitanceuF = 220.0f;
static float virtCapVoltage = 0.0f;
static float virtOutHighThreshholdV = 2.6f;
static float virtOutLowThreshholdV = 2.0f;
static float virtCapOutputVoltage = 1.8f;
static float virtCapMaxVoltage = 3.0f;
static bool virtCapDacOutput = false;
static float virtCapInputCurrentmA = 0.5f;
static bool virtCapOutputOn = false;

static float virtCapDebuggingCompensation = 0.0f;

// https://www.ti.com/lit/ds/symlink/bq25570.pdf
// figure 4 VIN = 2V VSTOR = 3V
// Efficiency changes with VIN VSTOR and the current. We simplify to one trace.
const static float virtCapInputEfficiencyTable[][9] = {
    {.64, .80, .84, .86, .86, .86, .86, .87, .88}, // 0.01-0.1 mA
    {.885, .90, .91, .92, .92, .92, .925, .925, .925}, // 0.1-1 mA
    {.925, .925, .925, .925, .925, .93, .93, .925, .91}, // 1-10mA
    {.905, .905, .89, .88, .85, .85, .85, .85, .85} // 10-100mA
};

// https://www.ti.com/lit/ds/symlink/bq25570.pdf
// figure 11 VOUT = 1.8V VSTOR = 3.6V
// Efficiency changes with VOUT VSTOR and the current. We simplify to one trace.
const static float virtCapOutputEfficiencyTable[][9] = {
     {.82, .85, .855, .86, .865, .865, .87, .87, .875}, // 0.01-0.1 mA
     {.875, .875, .875, .88, .88, .88, .88, .88, .89}, // 0.1-1 mA
     {.90, .90, .89, .87, .87, .87, .87, .88, .88}, // 1-10mA
     {.89, .88, .875, .88, .89, .885, .885, .89, .89} // 10-100mA
};


// In our testing we found the quiescent current to be about 2-3uA in circuit
// together with caps
// Datasheet specifies 488 nA typical.
static float virtCapQuiesCurrentmA = 0.003f;
static bool virtCapIdealMode = false;

static bool virtCap_replay_mode = false;

static Supply_ProfileDataRespond currentReplayParams = {0};
static int msgRequestCounter = 0;

TIM_HandleTypeDef htim5;
#define disableDeltaTimer() (TIM5->CR1 &= ~TIM_CR1_CEN)
#define enableDeltaTimer() (TIM5->CR1 |= TIM_CR1_CEN)

extern void setVirtCapDac(float valueV);

void replay_supply_detach() {
  setVirtCapDac(0);
  disableEmulatorTimer();
  vTaskSuspend(replayTaskHandle);

  if(virtCap_replay_mode){
    virtCap_replay_mode = false;
    disableDeltaTimer();
  }
}

/**
 * Used for timer 5 (replay trace) initialization
 */
void deltaTimer(void) {
    __HAL_RCC_TIM5_CLK_ENABLE();
    htim5.Instance = TIM5;
    htim5.Init.Period = UINT32_MAX - 1; // ,max
    htim5.Init.Prescaler = 72 -1; // 1MHz -> us ticks
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK) {
        /* Initialization Error */
        Error_Handler();
    }
}

float compensateCurrent(const float table[][9], float currentmA) {
  if(currentmA < 0){
    return 0;
  }
  if (currentmA < 0.1) {
    uint16_t index = currentmA * 100.0f;
    if(index < 10){
      return table[0][index];
    }
  } else if (currentmA < 1) {
    uint16_t index = currentmA * 10.0f;
    if(index < 10){
      return table[1][index];
    }
  } else if (currentmA < 10) {
    uint16_t index = currentmA * 1.0f;
    if(index < 10){
      return table[2][index];
    }
  } else {
    uint16_t index = currentmA * 0.1f;
    if(index < 10){
      return table[3][index];
    }
  }
  return 0;
}

void replay_supply_period() {
  while (true) {
    if (paused) {
      osDelay(100);
      continue;
    }

    float dutConsumedCurrentmA = 0;
    uint32_t dtuS = TIM2->CNT;

    if(virtCapOutputOn){
      dutConsumedCurrentmA = getDutCurrent();

      if (!virtCapIdealMode) {
        dutConsumedCurrentmA *= compensateCurrent(virtCapOutputEfficiencyTable,
                                                  dutConsumedCurrentmA);
      }
    }


    if (virtCap_replay_mode) {
      size_t messages = uxQueueMessagesWaiting(xProfileDataQueue);
      if (messages < 5 && !virtCap_replay_ended && (msgRequestCounter < 1)) {
        supplyRequestDataProfile();
        msgRequestCounter++;
      }

      if ((virtCap_replay_ended && messages == 0)) {
        virtCap_replay_mode = false;
        virtCapInputCurrentmA = 0.0f;
        disableDeltaTimer();
      } else {
        uint32_t timer_val = TIM5->CNT;
        if (timer_val >= currentReplayParams.time) {
          virtCapInputCurrentmA = currentReplayParams.current;
          if (xQueueReceive(xProfileDataQueue, &currentReplayParams, 10) ==
                 pdPASS) {
            TIM5->EGR = 1;  // Reset timer
          } else {
            // error we do not have a next input value to process
            // so just continue until it arrives
          }
        }
      }
    }

    float currentSummA = 0;
    if (!virtCapIdealMode) {
      float compensatedImputCurrentmA = virtCapInputCurrentmA * compensateCurrent(virtCapInputEfficiencyTable, virtCapInputCurrentmA);
      currentSummA = compensatedImputCurrentmA - dutConsumedCurrentmA - virtCapQuiesCurrentmA + virtCapDebuggingCompensation;
    } else {
      currentSummA = virtCapInputCurrentmA - dutConsumedCurrentmA + virtCapDebuggingCompensation;
    }


    // (mA - mA) * uS / uF
    // (mA - mA) * S / F  -> 10^6 cancels out, divide by 1000 to compensate for mA

    virtCapVoltage = virtCapVoltage + (currentSummA * dtuS / (virtCapCapacitanceuF * 1000));



    if (virtCapVoltage > virtOutHighThreshholdV && !virtCapOutputOn) {
      virtCapOutputOn = true;
      setEmulatorOutput(true);
      // turn on
    } else if (virtCapVoltage < virtOutLowThreshholdV && virtCapOutputOn) {
      virtCapOutputOn = false;
      setEmulatorOutput(false);
    }

    if (virtCapVoltage > virtCapMaxVoltage) {
      virtCapVoltage = virtCapMaxVoltage;
    } else if (virtCapVoltage < 0) {
      virtCapVoltage = 0;
    }

    TIM2->EGR = 1;  // reset timer

    if(virtCapDacOutput){
      setVirtCapDac(virtCapVoltage);
    }

    vTaskDelay(1);

  }
}

void replay_supply_pause() {
  disableEmulatorTimer();
  disableDeltaTimer();
  paused = true;
}

void replay_supply_resume() {
  enableEmulatorTimer();
  enableDeltaTimer();
  paused = false;
}


void replay_send_parameters(){
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_virtCapCapacitanceuF, (int) (virtCapCapacitanceuF * 1000));
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_virtCapMaxVoltage, (int) (virtCapMaxVoltage * 1000));
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_virtCapOutputVoltage, (int) (virtCapOutputVoltage * 1000));
  supplyInitialParameterResponse_bool(Supply_SupplyValueOption_virtCapOutputting, virtCapDacOutput);
  supplyInitialParameterResponse_bool(Supply_SupplyValueOption_virtCapIdealMode, virtCapIdealMode);
  supplyInitialParameterResponse_bool(Supply_SupplyValueOption_profilerEnabled,virtCap_replay_mode);
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_virtCapVoltage, (int) (virtCapVoltage * 1000));
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_virtOutHighThreshholdV, (int) (virtOutHighThreshholdV * 1000));
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_virtOutLowThreshholdV, (int) (virtOutLowThreshholdV * 1000));
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_virtCapInputCurrent, (int) (virtCapInputCurrentmA * 1000));
  supplyInitialParameterResponse_int(Supply_SupplyValueOption_virtCapCompensation, (int) (virtCapDebuggingCompensation * 1000));
}

void replay_supply_set_param(Supply_SupplyValueChangeRequest command) {
  switch (command.key) {
    case Supply_SupplyValueOption_virtCapCapacitanceuF:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_f_tag) return;
      virtCapCapacitanceuF = command.value.value_f;
      break;
    case Supply_SupplyValueOption_virtCapMaxVoltage:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_f_tag) return;
      virtCapMaxVoltage = command.value.value_f;
      break;
    case Supply_SupplyValueOption_virtCapOutputVoltage:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_f_tag) return;
      virtCapOutputVoltage = command.value.value_f;
      setEmulatorConstantOutputMode(true, virtCapOutputVoltage * 1000);
      break;
    case Supply_SupplyValueOption_virtCapOutputting:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_b_tag) return;
      if(command.value.value_b){
        setVirtCapDac(virtCapVoltage);
        virtCapDacOutput = command.value.value_b;
      } else {
        virtCapDacOutput = command.value.value_b;
        setVirtCapDac(0);
      }
      break;
    case Supply_SupplyValueOption_virtCapIdealMode:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_b_tag) return;
      virtCapIdealMode = command.value.value_b;
      break;
    case Supply_SupplyValueOption_virtCapCompensation:
       if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
       virtCapDebuggingCompensation =  (float) command.value.value_i / 1000;
       break;
    case Supply_SupplyValueOption_virtCapVoltage:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_f_tag) return;
      virtCapVoltage = command.value.value_f;
      break;
    case Supply_SupplyValueOption_virtOutHighThreshholdV:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_f_tag) return;
      virtOutHighThreshholdV = command.value.value_f;
      break;
    case Supply_SupplyValueOption_virtOutLowThreshholdV:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_f_tag) return;
      virtOutLowThreshholdV = command.value.value_f;
      break;
    case Supply_SupplyValueOption_virtCapInputCurrent:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
      virtCapInputCurrentmA = (float) command.value.value_i / 1000;
      break;
    case Supply_SupplyValueOption_profilerEnabled:
      if(command.which_value != Supply_SupplyValueChangeRequest_value_b_tag) return;
      if (!command.value.value_b) {
        if (virtCap_replay_mode) {
          virtCap_replay_mode = false;
          disableDeltaTimer();
        }
      }
      break;
    default:
      break;
  }
}

void replay_supply_attach() {
  setEmulatorOutputPinMode(GPIO_MODE_OUTPUT_PP);
  setEmulatorOutput(false);
  disableEmulatorTimer();
  setVirtCapDac(0);

  setEmulatorConstantOutputMode(true, virtCapOutputVoltage * 1000);

  // here we use the microsecond timer to measure time between virt cap computations
  // so we set ARR to almost max
  // CCR is not used so set to 0
  setEmulatorPeriod(4200000,0);
  enableEmulatorTimer();

  vTaskResume(replayTaskHandle);
}

void addDataProfile(Supply_ProfileDataRespond command) {
  if (!virtCap_replay_mode) {
    xQueueReset(xProfileDataQueue);
    virtCap_replay_ended = false;
    enableDeltaTimer();
    virtCap_replay_mode = true;
    currentReplayParams.time = 0;
  }

  // Our last element
  if (command.time == 0) {
    virtCap_replay_ended = true;
  }

  xQueueSend(xProfileDataQueue, &command, 10);
  msgRequestCounter = 0;
}

void replay_supply_construct(supplyOperation *s) {
  if (!initialized){
    initialized = true;
    xProfileDataQueue = xQueueCreate(45, sizeof(Supply_ProfileDataRespond));
  }
  s->detach = replay_supply_detach;
  s->pause = replay_supply_pause;
  s->resume = replay_supply_resume;
  s->setParameter = replay_supply_set_param;
  s->attach = replay_supply_attach;
  s->mode = Supply_OperationMode_Replay;
  replay_operation = s;
}

void replay_supply_config(void){
  deltaTimer();

  replayTaskHandle = osThreadNew(replay_supply_period, NULL, &replayTask_attributes);
  vTaskSuspend(replayTaskHandle);
}
