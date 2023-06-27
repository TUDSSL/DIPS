#include "voltage-sense.h"

const float vref = 3.0f;
const float voltageDividerRatio = 1.95f;

void SDADC2_Init(void);

SDADC_HandleTypeDef hsdadc2;
DMA_HandleTypeDef hdma_sdadc2;

int16_t dut_vout_buffer[SDADC_BUFFER_SIZE_VOLTAGE];

/**
 * Configure voltage sense initialization
 */
void voltageSenseConfig(void) {
  /* DMA2_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel4_IRQn, 0, 0);
//  HAL_NVIC_EnableIRQ(DMA2_Channel4_IRQn);

  SDADC2_Init();
}

// get current DUTY Voltage (V)
float getDutVoltage(void) {
  uint32_t average = 0;
  for (int i = 0; i < SDADC_BUFFER_SIZE_VOLTAGE; i+=2) {
    average += dut_vout_buffer[i] + 32768;
  }
  average /= SDADC_BUFFER_SIZE_CHANNEL;

  float vadc = average * vref / (1 * 65536);
  float vout = vadc * voltageDividerRatio;  // 1.47 voltage divider

  return vout;
}

float getDebugVoltage(void) {
  uint32_t average = 0;
  for (int i = 1; i < SDADC_BUFFER_SIZE_VOLTAGE; i+=2) {
    average += dut_vout_buffer[i] + 32768;
  }
  average /= SDADC_BUFFER_SIZE_CHANNEL;

  float vadc = average * vref / (1 * 65536);
  float vout = vadc * voltageDividerRatio;

  return vout;
}

// Initialize SDADC2
void SDADC2_Init(void) {
  SDADC_ConfParamTypeDef ConfParamStruct = {0};

  /** Configure the SDADC low power mode, fast conversion mode,
  slow clock mode and SDADC1 reference voltage
  */
  hsdadc2.Instance = SDADC2;
  hsdadc2.Init.IdleLowPowerMode = SDADC_LOWPOWER_NONE;
  hsdadc2.Init.FastConversionMode = SDADC_FAST_CONV_DISABLE;
  hsdadc2.Init.SlowClockMode = SDADC_SLOW_CLOCK_DISABLE;
  hsdadc2.Init.ReferenceVoltage = SDADC_VREF_EXT;
  if (HAL_SDADC_Init(&hsdadc2) != HAL_OK) {
    Error_Handler();
  }
  /** Configure Injected Mode
   */
  if (HAL_SDADC_SelectInjectedTrigger(&hsdadc2, SDADC_SOFTWARE_TRIGGER) !=
      HAL_OK) {
    Error_Handler();
  }
  /** Set parameters for SDADC configuration 0 Register
   */
  ConfParamStruct.InputMode = SDADC_INPUT_MODE_SE_ZERO_REFERENCE;
  ConfParamStruct.Gain = SDADC_GAIN_1;
  ConfParamStruct.CommonMode = SDADC_COMMON_MODE_VSSA;
  ConfParamStruct.Offset = 0;

  if (HAL_SDADC_PrepareChannelConfig(&hsdadc2, SDADC_CONF_INDEX_0,
                                     &ConfParamStruct) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_AssociateChannelConfig(&hsdadc2, DUT_VOUT_CHANNEL,
                                       SDADC_CONF_INDEX_0) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_AssociateChannelConfig(&hsdadc2, DEBUG_VOUT_CHANNEL,
                                       SDADC_CONF_INDEX_0) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_InjectedConfigChannel(&hsdadc2, DUT_VOUT_CHANNEL | DEBUG_VOUT_CHANNEL,
                              SDADC_CONTINUOUS_CONV_ON) != HAL_OK) {
    Error_Handler();
  }

  /* Start Calibration in polling mode */
  if (HAL_SDADC_CalibrationStart(&hsdadc2, SDADC_CALIBRATION_SEQ_1) != HAL_OK) {
    Error_Handler();
  }

  /* Pool for the end of calibration */
  if (HAL_SDADC_PollForCalibEvent(&hsdadc2, HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_InjectedStart_DMA(&hsdadc2, (uint32_t*)dut_vout_buffer,
                          SDADC_BUFFER_SIZE_VOLTAGE) != HAL_OK) {
    Error_Handler();
  }
}

void DMA2_Channel4_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_sdadc2); }
