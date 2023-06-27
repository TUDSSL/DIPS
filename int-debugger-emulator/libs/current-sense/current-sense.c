#include "current-sense.h"
#include "calibration.h"

static void SDADC1_Init(void);
static void SDADC3_Init(void);

SDADC_HandleTypeDef hsdadc1;
SDADC_HandleTypeDef hsdadc3;
DMA_HandleTypeDef hdma_sdadc1;
DMA_HandleTypeDef hdma_sdadc3;

int16_t dut_current_high_buffer[SDADC_BUFFER_SIZE];
int16_t dut_current_low_buffer[SDADC_BUFFER_SIZE];

static const float vref = 3.0f;
const static float RSenseLow = 1000.0f;
const static float RSenseLowTweak = 50.0f;

const static float RSenseHigh = 5.6;
const static float RSenseHighTweak = 0.273;
const static float RSenseHighTweakOffset = 0.015;

static float iHighF = 0.0f;
static float iLowF = 0.0f;

static bool bypassStatus = false;

typedef enum CurrentSenseRange {
	SenseHigh = 0,
	SenseLow
} CurrentSenseRange;

CurrentSenseRange currentRange = SenseHigh;

bool getBypassState(){
  return bypassStatus;
}

/**
 * Initialize current Sense ADC
 */
void currentSenseConfig(void) {
  /* DMA2_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel3_IRQn);

  /* DMA2_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel5_IRQn, 0, 0);
//  HAL_NVIC_EnableIRQ(DMA2_Channel5_IRQn);

  SDADC1_Init();
  SDADC3_Init();

  // iLow Bypass
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = EMULATOR_LOW_SENSE_BYPASS_PIN;
  HAL_GPIO_Init(EMULATOR_CONFIG_PINS_PORT, &GPIO_InitStruct);

  setBypass(true);
}

/**
 * Bypass the current sense
 * @param active
 */
void setBypass(bool active) {
    HAL_GPIO_WritePin(EMULATOR_CONFIG_PINS_PORT, EMULATOR_LOW_SENSE_BYPASS_PIN,
                      (GPIO_PinState) !active);
    bypassStatus = active;
}

/**
 * return current DutLowCurrent value (mA)
 */
float computeLowCurrent(void) {
  uint16_t dacValueCopy = DAC1->DOR2 & 0xfff;
  if(dacValueCopy >= MAX_DAC_VALUE || dacValueCopy < MIN_DAC_VALUE){
    return 0.0f; // no calibration exists out of measurement range
  }

  // For cleaner results, take average of 50 measurements
  uint32_t averageLow = 0;
  for (int i = 0; i < SDADC_BUFFER_SIZE; ++i) {
    averageLow += dut_current_low_buffer[i] + 32768;
  }
  averageLow /= SDADC_BUFFER_SIZE;

  // Compare average value with saved calibration values in flash memory
  int32_t ilow = averageLow - calibConstants.iLowComp[(dacValueCopy - CalibrationDacOffset) >> 1];
  float ilowFloat = ilow * vref / (1 * 65536) * 1000;
  ilowFloat /= 25 * (RSenseLow + RSenseLowTweak);
  return ilowFloat;
}

/**
 * Return DutHighCurrent value (mA)
 * @return
 */
float computeHighCurrent(void) {
  uint16_t dacValueCopy = DAC1->DOR2 & 0xfff;
  if(dacValueCopy >= MAX_DAC_VALUE || dacValueCopy < MIN_DAC_VALUE){
    return 0.0f; // no calibration exists out of measurement range
  }

  // For cleaner results, take average of 50 measurements
  uint32_t averageHigh = 0;
  for (int i = 0; i < SDADC_BUFFER_SIZE; ++i) {
    averageHigh += dut_current_high_buffer[i] + 32768;
  }
  averageHigh /= SDADC_BUFFER_SIZE;

  // Compare average value with saved calibration values in flash memory
  int32_t ihigh = averageHigh - calibConstants.iHighComp[(dacValueCopy - CalibrationDacOffset) >> 1];
  float ihighFloat = ihigh * vref / (1 * 65536) * 1000;
  ihighFloat /= 25 * (RSenseHigh + RSenseHighTweak);
  ihighFloat += RSenseHighTweakOffset;
  return ihighFloat;
}

/**
 * return the DutCurrent
 * @return
 */
void computeCurrentSense(void){
  uint16_t dacValueCopy = DAC1->DOR2 & 0xfff;
  uint32_t averageHigh = 0;
  for (int i = 0; i < SDADC_BUFFER_SIZE; ++i) {
    averageHigh += dut_current_high_buffer[i] + 32768;
  }
  averageHigh /= SDADC_BUFFER_SIZE;

  int32_t ihigh = averageHigh - calibConstants.iHighComp[(dacValueCopy - CalibrationDacOffset) >> 1];
  float ihighFloat = ihigh * vref / (1 * 65536) * 1000;
  ihighFloat /= 25 * (RSenseHigh + RSenseHighTweak);
  ihighFloat += RSenseHighTweakOffset;

  bool highCurrent = ihighFloat > 0.1f;// Current larger than 100uA turn off low range
  if(bypassStatus){
    currentRange = SenseHigh;
  } else if(!highCurrent && !bypassStatus){
    iLowF =  computeLowCurrent();
    currentRange = SenseLow;
  }

  iHighF = ihighFloat;
}

float getDutCurrent(void){
	if(currentRange == SenseLow){
		return iLowF;
	} else {
		return iHighF;
	}
}

float getDutLowCurrent(void){
	return iLowF;
}
float getDutHighCurrent(void){
	return iHighF;
}

/**
 * Initialize SDADC1
 */
static void SDADC1_Init(void) {
  SDADC_ConfParamTypeDef ConfParamStruct = {0};

  /** Configure the SDADC low power mode, fast conversion mode,
  slow clock mode and SDADC1 reference voltage
  */
  hsdadc1.Instance = SDADC1;
  hsdadc1.Init.IdleLowPowerMode = SDADC_LOWPOWER_NONE;
  hsdadc1.Init.FastConversionMode = SDADC_FAST_CONV_ENABLE;
  hsdadc1.Init.SlowClockMode = SDADC_SLOW_CLOCK_DISABLE;
  hsdadc1.Init.ReferenceVoltage = SDADC_VREF_EXT;
  if (HAL_SDADC_Init(&hsdadc1) != HAL_OK) {
    Error_Handler();
  }
  /** Configure The Regular Mode
   */
  if (HAL_SDADC_SelectRegularTrigger(&hsdadc1, SDADC_SOFTWARE_TRIGGER) !=
      HAL_OK) {
    Error_Handler();
  }
  /** Set parameters for SDADC configuration 0 Register
   */
  ConfParamStruct.InputMode = SDADC_INPUT_MODE_SE_ZERO_REFERENCE;
  ConfParamStruct.Gain = SDADC_GAIN_1;
  ConfParamStruct.CommonMode = SDADC_COMMON_MODE_VSSA;
  ConfParamStruct.Offset = 0;
  if (HAL_SDADC_PrepareChannelConfig(&hsdadc1, SDADC_CONF_INDEX_0,
                                     &ConfParamStruct) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_AssociateChannelConfig(&hsdadc1, DUT_CURRENT_HIGH_CHANNEL,
                                       SDADC_CONF_INDEX_0) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_ConfigChannel(&hsdadc1, DUT_CURRENT_HIGH_CHANNEL,
                              SDADC_CONTINUOUS_CONV_ON) != HAL_OK) {
    Error_Handler();
  }

  /* Start Calibration in polling mode */
  if (HAL_SDADC_CalibrationStart(&hsdadc1, SDADC_CALIBRATION_SEQ_1) != HAL_OK) {
    Error_Handler();
  }

  /* Pool for the end of calibration */
  if (HAL_SDADC_PollForCalibEvent(&hsdadc1, HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_Start_DMA(&hsdadc1, (uint32_t*)dut_current_high_buffer,
                          SDADC_BUFFER_SIZE) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * Initialize SDADC1
 */
static void SDADC3_Init(void) {
  SDADC_ConfParamTypeDef ConfParamStruct = {0};

  /** Configure the SDADC low power mode, fast conversion mode,
  slow clock mode and SDADC1 reference voltage
  */
  hsdadc3.Instance = SDADC3;
  hsdadc3.Init.IdleLowPowerMode = SDADC_LOWPOWER_NONE;
  hsdadc3.Init.FastConversionMode = SDADC_FAST_CONV_ENABLE;
  hsdadc3.Init.SlowClockMode = SDADC_SLOW_CLOCK_DISABLE;
  hsdadc3.Init.ReferenceVoltage = SDADC_VREF_EXT;
  if (HAL_SDADC_Init(&hsdadc3) != HAL_OK) {
    Error_Handler();
  }
  /** Configure The Regular Mode
   */
  if (HAL_SDADC_SelectRegularTrigger(&hsdadc3, SDADC_SOFTWARE_TRIGGER) !=
      HAL_OK) {
    Error_Handler();
  }
  /** Set parameters for SDADC configuration 0 Register
   */
  ConfParamStruct.InputMode = SDADC_INPUT_MODE_SE_ZERO_REFERENCE;
  ConfParamStruct.Gain = SDADC_GAIN_1;
  ConfParamStruct.CommonMode = SDADC_COMMON_MODE_VSSA;
  ConfParamStruct.Offset = 0;
  if (HAL_SDADC_PrepareChannelConfig(&hsdadc3, SDADC_CONF_INDEX_0,
                                     &ConfParamStruct) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_AssociateChannelConfig(&hsdadc3, DUT_CURRENT_LOW_CHANNEL,
                                       SDADC_CONF_INDEX_0) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_ConfigChannel(&hsdadc3, DUT_CURRENT_LOW_CHANNEL,
                              SDADC_CONTINUOUS_CONV_ON) != HAL_OK) {
    Error_Handler();
  }

  /* Start Calibration in polling mode */
  if (HAL_SDADC_CalibrationStart(&hsdadc3, SDADC_CALIBRATION_SEQ_1) != HAL_OK) {
    Error_Handler();
  }

  /* Pool for the end of calibration */
  if (HAL_SDADC_PollForCalibEvent(&hsdadc3, HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_SDADC_Start_DMA(&hsdadc3, (uint32_t*)dut_current_low_buffer,
                          SDADC_BUFFER_SIZE) != HAL_OK) {
    Error_Handler();
  }
}

void HAL_SDADC_ConvCpltCallback(SDADC_HandleTypeDef* hsdadc){
  if(hsdadc == &hsdadc1 ){
	  computeCurrentSense();
  }
}


void DMA2_Channel3_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_sdadc1); }

void DMA2_Channel5_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_sdadc3); }
