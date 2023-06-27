#include <sys/cdefs.h>
//#include <malloc.h>

#include "supply.h"

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "current-sense.h"
#include "pc-comms.h"
#include "debugger-interface.pb.h"
#include "queue.h"
#include "task.h"
#include "calibration.h"
#include "stm32f3xx_hal_rtc.h"
#include "platform.h"
#include "voltage-sense.h"


extern osThreadId_t dataStreamProcessHandle;

DataStreamsConfig dataStreamConfig;

supplyOperation currentOperation;

TIM_HandleTypeDef htim2;

DMA_HandleTypeDef hdma_dac1_ch1;
DMA_HandleTypeDef hdma_dac1_ch2;
DAC_HandleTypeDef hdac1;
DAC_HandleTypeDef hdac2;
TIM_OC_InitTypeDef emulatorOutputChannelConfig;


uint16_t emulatorDacValue[EmulatorDacSteps] = {[0 ... EmulatorDacSteps - 1] = MAX_DAC_VALUE};
uint16_t debugDacValue = MAX_DAC_VALUE;

uint16_t minValuemV = 1800;
uint16_t minEmulatorValueDac = MAX_DAC_VALUE; // Minvalue in DAC units.

static bool onBreak = false;      // Used for setting breakpoint / Pause functions

// Instead of modifying the main output, directly output the mcu debug connector
static bool use_debug_supply = false;
static bool optionOutputDischarge = false;
static bool optionSampleAndHold = false;
static bool egInBreak = false;

uint16_t energyGuardmV = 0; // threshold in mV for the energyGuard
uint16_t energyGuardDacValue = MAX_DAC_VALUE; // threshold in DAC steps
const float VoltagePerBitDAC2 = 3.3f / (((uint32_t) 1 << 12) - 1);

static void DAC1_Init(void);
static void DAC2_Init(void);
static void TIM2_Init(void);

static void setEmulatorDacOverride(bool enable, uint16_t dacValue);
static void setEmulatorSquareWaveDischarge(bool value);
static uint16_t convertToDacOutput(uint16_t value);
static uint16_t BinarySearch(Supplies supply, uint16_t x);

/**
 * Change parameters of the replay / buck-boost emulator by PC_COMM
 * @param command
 */
void handleSupplyDataRequest(Supply_DataStreamRequest command) {
    switch (command.request.which_packet) {
        case (Supply_DataStream_volt_current_tag): {
            if (command.active) {
                dataStreamConfig.rawIV = true;
                if (dataStreamProcessHandle) {
                    vTaskResume(dataStreamProcessHandle);
                }
            }
            break;
        }
    }
}

/**
 * Change supply mode function from pc_comm
 * @param command
 */
void handleSupplyModeRequest(Supply_ModeRequest command) {
    if (onBreak)
        return;

    if (command.mode == currentOperation.mode)
        return;

    currentOperation.detach();

    switch (command.mode) {
        case Supply_OperationMode_Sawtooth:
            sawtooth_supply_construct(&currentOperation);
            break;
        case Supply_OperationMode_ConstantVoltage:
            constant_supply_construct(&currentOperation);
            break;
        case Supply_OperationMode_Replay:
            replay_supply_construct(&currentOperation);
            break;
        case Supply_OperationMode_Off:
            off_supply_construct(&currentOperation);
            break;
        case Supply_OperationMode_Square:
            square_supply_construct(&currentOperation);
            break;
        case Supply_OperationMode_Triangle:
            triangle_supply_construct(&currentOperation);
            break;
        case Supply_OperationMode_Passive:
          passive_supply_construct(&currentOperation);
            break;
        default:
            break;
    }
    currentOperation.attach();
}

/**
 * Change supply parameter function from pc_comm
 * @param command
 */
void handleSupplyValueRequest(Supply_SupplyValueChangeRequest command) {
  if (command.key == Supply_SupplyValueOption_minVoltage){
    if(command.which_value != Supply_SupplyValueChangeRequest_value_i_tag) return;
    minValuemV = command.value.value_i;
    minEmulatorValueDac = convertToDacOutput(BinarySearch(Supply_Emulator, minValuemV));
    setDebugDac(minValuemV);
  }
  else if (command.key == Supply_SupplyValueOption_enableVDebugOutput){
    if(command.which_value != Supply_SupplyValueChangeRequest_value_b_tag) return;
    use_debug_supply = command.value.value_b;
  } else if (command.key == Supply_SupplyValueOption_setCurMeasBypass){
    if(command.which_value != Supply_SupplyValueChangeRequest_value_b_tag) return;
    setBypass(command.value.value_b);
  } else if (command.key == Supply_SupplyValueOption_setEmulatorOutputDischarge){
    if(command.which_value != Supply_SupplyValueChangeRequest_value_b_tag) return;
    optionOutputDischarge = command.value.value_b;
    if(currentOperation.mode == Supply_OperationMode_Square){
      setEmulatorSquareWaveDischarge(optionOutputDischarge);
    }
  } else if(command.key == Supply_SupplyValueOption_setEmulatorSampleAndHold){
    if(command.which_value != Supply_SupplyValueChangeRequest_value_b_tag) return;
    optionSampleAndHold = command.value.value_b;
  } else{
    currentOperation.setParameter(command);
  }
}



/***
 * Internal function for applying pause, minimum or energy guard functionality.
 * Switches between debug output and emulator output according to configuration.
 * When both minimum and energy guards are active, minimum overrules guards
 * when guards < minimum.
 *
 * @param enableMinimum  Set when minimum is active
 */
static void updateSupplyOutput(bool enableMinimum) {
  uint16_t pauseSupplyVoltageDac = 0, pauseSupplyVoltagemV = 0;

  // Energy Guard logic
  if (energyGuardmV > minValuemV) {
    pauseSupplyVoltagemV = energyGuardmV;
    pauseSupplyVoltageDac = energyGuardDacValue;
  } else {
    pauseSupplyVoltagemV = minValuemV;
    pauseSupplyVoltageDac = minEmulatorValueDac;
  }

  if ((enableMinimum || energyGuardmV) && !use_debug_supply && !optionSampleAndHold) {
    uint16_t currentDacValue = DAC1->DOR2;

    // in square mode the output is switched on and off
    // hence, output might be 0 even with DAC on.
    // We revert to software operation of the pin
    // and when off assume highest DAC value.
    if (currentOperation.mode == Supply_OperationMode_Square) {
      if (TIM2->CNT < TIM2->CCR1) {
        setEmulatorOutput(true);
        if(optionOutputDischarge){
          setEmulatorDischarge(false);
        }
      } else {
        setEmulatorOutput(false);
        if(optionOutputDischarge){
          setEmulatorDischarge(true);
        }

        currentDacValue = MAX_DAC_VALUE;
      }
      setEmulatorOutputPinMode(GPIO_MODE_OUTPUT_PP);
    }

    // check if minimum is met, bigger is lower
    if ((currentDacValue & 0xfff) > pauseSupplyVoltageDac) {
      setEmulatorDacOverride(true, pauseSupplyVoltageDac);
      setEmulatorOutput(true);
    }
  } else if ((enableMinimum || energyGuardmV) && use_debug_supply) {
    setDebugDac(pauseSupplyVoltagemV);
    setDebugOutput(true);
  }
  if(optionSampleAndHold){
    uint16_t currentVoltagemV = getDutVoltage() * 1000;
    uint16_t dacValue = BinarySearch(Supply_Emulator, currentVoltagemV);
    dacValue = convertToDacOutput(dacValue);
    setEmulatorDacOverride(true, dacValue);
    setEmulatorOutput(true);
    setEmulatorOutputPinMode(GPIO_MODE_OUTPUT_PP);
  }
}

/**
 * Function for pausing or resuming the emulator or debug supply
 *
 * @param enable Set or clear the pause
 * @param enableMinimum Set when minimum is active
 */
void pauseResumeSupply(bool enable, bool enableMinimum) {
  if (enable && !onBreak) {
    currentOperation.pause();
    updateSupplyOutput(enableMinimum);
    onBreak = true;

    supplyTransmitBreakpoint(true);
  } else if (!enable && onBreak && !energyGuardmV) {
    onBreak = false;
    setDebugOutput(false);
    setEmulatorDacOverride(false, 0);

    supplyTransmitBreakpoint(false);

    currentOperation.resume();
  } else {
//    supplyTransmitBreakpoint(enable);
  }
}

/**
 * Function to set an energyGuard, energy guards pause the supply
 * @param threshold_mV value (mV)
 */
void setEnergyGuard(uint16_t threshold_mV){
  if(!energyGuardmV){
    egInBreak = onBreak;
  }

  energyGuardmV = threshold_mV;
  energyGuardDacValue = convertToDacOutput(BinarySearch(Supply_Emulator, threshold_mV));

  if(onBreak){
    // As energyguard is active it is safe to pass false,
    // recompute the output voltage if it changed.
    updateSupplyOutput(false);
  } else {
    pauseResumeSupply(true, true);
  }
}

/**
 * Function to clear the energyGuard and resume the supply
 */
void clearEnergyGuard(){
  energyGuardmV = 0;
  if(!egInBreak){
    pauseResumeSupply(false, true);
  }
}

/**
 * Initialization function from comm for retransmitting all parameters
 * to sync GUI and Emulator MCU
 */
void handleSupplyInitialDataRequest(){
  sawtooth_send_parameters();
  constant_send_parameters();
  replay_send_parameters();
  triangle_send_parameters();
  square_send_parameters();

  supplyInitialParameterResponse_int(Supply_SupplyValueOption_minVoltage, minValuemV);
  supplyInitialParameterResponse_bool(Supply_SupplyValueOption_enableVDebugOutput, use_debug_supply);
  supplyInitialParameterResponse_bool(Supply_SupplyValueOption_setCurMeasBypass, getBypassState());
  supplyInitialParameterResponse_bool(Supply_SupplyValueOption_setEmulatorSampleAndHold, optionSampleAndHold);
  supplyInitialParameterResponse_bool(Supply_SupplyValueOption_setEmulatorOutputDischarge, optionOutputDischarge);
}

/**
 * Comm for new data profile package (replay mode)
 * @param command
 */
void handleSupplyProfileDataResponds(Supply_ProfileDataRespond command) {
    if (currentOperation.mode == Supply_OperationMode_Replay)
      addDataProfile(command);
}

/**
 * Initialization of the supply
 */
void supplyConfig(void) {
    // We initialize as off
    off_supply_construct(&currentOperation);

    __HAL_RCC_DAC1_CLK_ENABLE();
    __HAL_RCC_DAC2_CLK_ENABLE();

    DAC1_Init();
    DAC2_Init();
    TIM2_Init();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = EMULATOR_LDO_EN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    // VOUT LDO
    HAL_GPIO_Init(EMULATOR_CONFIG_PINS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DEBUG_LDO_EN_PIN;
    HAL_GPIO_Init(DEBUG_EN_PIN_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DEBUG_OUTPUT_EN_PIN;
    HAL_GPIO_Init(DEBUG_CONFIG_PINS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = EMULATOR_VIRT_CAP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(EMULATOR_CONFIG_PINS_PORT, &GPIO_InitStruct);

    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    setDebugDac(1800); // default to 1.8V
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_2, (uint32_t*)&emulatorDacValue, EmulatorDacSteps, DAC_ALIGN_12B_R);
    HAL_DAC_Start(&hdac2, DAC_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    setEmulatorLDO(true);
    setDebugLDO(true);
    setEmulatorOutput(false);
    setDebugOutput(false);
    setEmulatorDischarge(false);
    setEmulatorSquareWaveDischarge(false);

    minEmulatorValueDac = convertToDacOutput(BinarySearch(Supply_Emulator, minValuemV));

    replay_supply_config();
    currentOperation.attach();


}

/**
 * Switch between alternate function and software control of the output enable
 * pin
 *
 * @param mode GPIO_MODE_AF_PP or GPIO_MODE_OUTPUT_PP
 */
void setEmulatorOutputPinMode(uint32_t mode)
{
    uint32_t reg = EMULATOR_CONFIG_PINS_PORT->MODER;
    reg &= ~(GPIO_MODER_MODER0_Msk);
    reg &= ~(GPIO_MODER_MODER1_Msk);
    reg |= (mode & GPIO_MODER_MODER0_Msk) << ((EMULATOR_OUTPUT_EN_PIN - 1u) << 1);
    reg |= (mode & GPIO_MODER_MODER0_Msk) << ((EMULATOR_DISCHARGE_PIN - 1u) << 1);
    EMULATOR_CONFIG_PINS_PORT->MODER = reg;
}

/**
 *  Quickly enable and disable the square wave compare channel for
 *  square wave mode.
 *
 * @param value enable (true) or disable (false)
 */
static void setEmulatorSquareWaveDischarge(bool value){
  if(value){
    TIM2->CCER |= TIM_CCER_CC2E;
  } else {
    TIM2->CCER &= ~(TIM_CCER_CC2E);
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void TIM2_Init(void) {
  GPIO_InitTypeDef   GPIO_InitStruct = {0};

  /* Enable all GPIO Channels Clock requested */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  GPIO_InitStruct.Pin = EMULATOR_OUTPUT_EN_PIN;
  HAL_GPIO_Init(EMULATOR_CONFIG_PINS_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  GPIO_InitStruct.Pin = EMULATOR_DISCHARGE_PIN;
  HAL_GPIO_Init(EMULATOR_CONFIG_PINS_PORT, &GPIO_InitStruct);

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  __HAL_RCC_TIM2_CLK_ENABLE();

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 72 - 1; // 1MHz counter frequency 72/(71+1)
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000 - 1; // 1khz TRGO updates 1Mhz/(999+1)
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  emulatorOutputChannelConfig.OCMode       = TIM_OCMODE_PWM1;
  emulatorOutputChannelConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
  emulatorOutputChannelConfig.OCFastMode   = TIM_OCFAST_DISABLE;
  emulatorOutputChannelConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
  emulatorOutputChannelConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  emulatorOutputChannelConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
  emulatorOutputChannelConfig.Pulse = 0; // 0% duty cycle initially (off)

  HAL_TIM_PWM_ConfigChannel(&htim2, &emulatorOutputChannelConfig, TIM_CHANNEL_1);

  emulatorOutputChannelConfig.OCPolarity   = TIM_OCPOLARITY_LOW;
  emulatorOutputChannelConfig.OCFastMode   = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim2, &emulatorOutputChannelConfig, TIM_CHANNEL_2);
}

void setEmulatorPeriod(uint32_t periodus, uint32_t dutycycleus){
  if(periodus <=1){
    return;
  }

  // Manually adjust the period and duty cycle to respond quicker instead of
  // re-initing the peripheral every time.
  // Event update (EGR = 1) prevents possible over-run of timer when reconfiguring
  TIM2->ARR = periodus - 1;
  TIM2->CCR1 = dutycycleus;
  TIM2->CCR2 = dutycycleus;
  TIM2->EGR=1;
}

/**
 * Initialization of the DAC
 */
static void DAC1_Init(void) {
    /* USER CODE BEGIN DAC1_Init 0 */

    /* USER CODE END DAC1_Init 0 */

    DAC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN DAC1_Init 1 */

    /* USER CODE END DAC1_Init 1 */
    /** DAC Initialization
     */
    hdac1.Instance = DAC1;
    if (HAL_DAC_Init(&hdac1) != HAL_OK) {
        Error_Handler();
    }
    /** DAC channel OUT1 config
     */
    sConfig.DAC_Trigger = DAC_TRIGGER_SOFTWARE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
    if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * Initialization of the Virt Cap DAC
 */
static void DAC2_Init(void) {
    /* USER CODE BEGIN DAC1_Init 0 */

    /* USER CODE END DAC1_Init 0 */

    DAC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN DAC1_Init 1 */

    /* USER CODE END DAC1_Init 1 */
    /** DAC Initialization
     */
    hdac2.Instance = DAC2;
    if (HAL_DAC_Init(&hdac2) != HAL_OK) {
        Error_Handler();
    }
    /** DAC channel OUT config
     */
    sConfig.DAC_Trigger = DAC_TRIGGER_SOFTWARE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    if (HAL_DAC_ConfigChannel(&hdac2, &sConfig, DAC_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
}

void setEmulatorDischarge(bool active) {
  HAL_GPIO_WritePin(EMULATOR_CONFIG_PINS_PORT, EMULATOR_DISCHARGE_PIN,
                          (GPIO_PinState) active);
}

void setEmulatorLDO(bool active) {
    if (active) {
        HAL_GPIO_WritePin(EMULATOR_CONFIG_PINS_PORT, EMULATOR_LDO_EN_PIN,
                          (GPIO_PinState) active);
    }
}

void setDebugLDO(bool active) {
    if (active) {
        HAL_GPIO_WritePin(DEBUG_EN_PIN_PORT, DEBUG_LDO_EN_PIN,
                          (GPIO_PinState) active);
    }
}

void setEmulatorOutput(bool active) {
    HAL_GPIO_WritePin(EMULATOR_CONFIG_PINS_PORT, EMULATOR_OUTPUT_EN_PIN,
                      (GPIO_PinState) active);
}

void setDebugOutput(bool active) {
    HAL_GPIO_WritePin(DEBUG_CONFIG_PINS_PORT, DEBUG_OUTPUT_EN_PIN,
                      (GPIO_PinState) active);
}

/**
 * BinarySearch on a voltage in the reference table after calibration
 * @param V in mV
 * @return V in hex
 */
static uint16_t BinarySearch(Supplies supply, uint16_t x) {
    int start = 0, end = CalibrationArraySizeValues, middle;
    middle = (start + end) / 2;

    const uint16_t* calibTable;
    if(supply == Supply_Emulator){
      calibTable = calibConstants.emulatorDacToMv;
    } else if(supply == Supply_Debug){
      calibTable = calibConstants.debugDacToMv;
    }

    while (start <= end) {
        if (calibTable[middle] == x) {
            return middle; // match
        } else if (calibTable[middle] > x) {
            start = middle + 1;
        } else {
            end = middle - 1;
        }
        middle = (start + end) / 2;
    }

    // when we don't match just take one of the closest
    return middle;
}

static uint16_t convertToDacOutput(uint16_t value){
  value = (value << 1) + CalibrationDacOffset;

  if (value < MIN_DAC_VALUE)
      value = MIN_DAC_VALUE;
  if (value > MAX_DAC_VALUE)
      value = MAX_DAC_VALUE;

 return value;
}

/**
 *
 * Function enables and disabled hardware timer2 control of the DAC channel 2
 * Used to quickly generate a constant output from wave generation mode by timer2
 *
 * @param enable Turns the constant mode on and off for DAC1 channel 2
 * @param value Constant output value in mV
 */
void setEmulatorConstantOutputMode(bool enable, uint16_t value){
  if(enable){
    uint16_t dacValue = BinarySearch(Supply_Emulator, value);
    dacValue = convertToDacOutput(dacValue);

    // disable DMA and enable software trigger channel 2
    MODIFY_REG(DAC1->CR, DAC_CR_TSEL2_Msk | DAC_CR_DMAEN2_Msk, DAC_CR_TEN2 | DAC_CR_TSEL2);
    DAC1->DHR12R2 = dacValue;
    // software trigger channel 2
    SET_BIT(DAC1->SWTRIGR, DAC_SWTRIGR_SWTRIG2);
  } else {
    // enable DMA and enable timer 2 hardware trigger
    MODIFY_REG(DAC1->CR, DAC_CR_TSEL2_Msk | DAC_CR_DMAEN2_Msk, DAC_CR_TEN2 | DAC_CR_TSEL2_2 | DAC_CR_DMAEN2);
  }
}

/**
 * Emulator DAC Override.
 * Used to quickly change DAC output value on pause.
 *
 * @param enable Enable the override
 * @param dacValue Constant output value during pause in DAC units
 */
static void setEmulatorDacOverride(bool enable, uint16_t dacValue) {
  static uint32_t dacRegisterCopy = 0;
  static uint16_t dacValueCopy = MAX_DAC_VALUE;
  if (enable) {
    if (dacValue < MIN_DAC_VALUE) dacValue = MIN_DAC_VALUE;
    if (dacValue > MAX_DAC_VALUE) dacValue = MAX_DAC_VALUE;

    dacRegisterCopy = DAC1->CR;
    dacValueCopy = DAC1->DOR2;
    // disable DMA and enable software trigger channel 2
    MODIFY_REG(DAC1->CR, DAC_CR_TSEL2_Msk | DAC_CR_DMAEN2_Msk, DAC_CR_TEN2 | DAC_CR_TSEL2);
    DAC1->DHR12R2 = dacValue;
    // software trigger channel 2
    SET_BIT(DAC1->SWTRIGR, DAC_SWTRIGR_SWTRIG2);
  } else if (dacRegisterCopy) {
    DAC1->DHR12R2 = dacValueCopy;
    SET_BIT(DAC1->SWTRIGR, DAC_SWTRIGR_SWTRIG2);

    // Restore the configuration
    DAC1->CR = dacRegisterCopy;

    dacRegisterCopy = 0;
    dacValueCopy = MAX_DAC_VALUE;
  }
}

/**
 * Set DAC Wavegen mode output value in mV
 *
 * @param value Output in mV
 * @param index DAC Output buffer index
 */
void setEmulatorDac(uint16_t value, uint16_t index) {
    uint16_t dacValue = BinarySearch(Supply_Emulator, value);
    emulatorDacValue[index] = convertToDacOutput(dacValue);
}

/**
 *  Set the output value of the debug output in mV
 * @param value Output in mV
 */
void setDebugDac(uint16_t value){
  uint16_t dacValue = BinarySearch(Supply_Debug, value);
  debugDacValue = convertToDacOutput(dacValue);

  // Write the value
  DAC1->DHR12R1 = debugDacValue;
  // software trigger channel 1
  SET_BIT(DAC1->SWTRIGR, DAC_SWTRIGR_SWTRIG1);
}

/**
 * Set the output value of the virtual capacitor output in V
 * @param valueV Output value in V
 */
void setVirtCapDac(float valueV){
  if (valueV > 3.0f) {
    valueV = 3.0f;
  }
  uint16_t dacValue = valueV / VoltagePerBitDAC2;
  // Write the value
  DAC2->DHR12R1 = dacValue;
  // software trigger channel 1
  SET_BIT(DAC2->SWTRIGR, DAC_SWTRIGR_SWTRIG1);
}

const Supply_OperationMode getCurrentOperationMode(void){
  return currentOperation.mode;
}


