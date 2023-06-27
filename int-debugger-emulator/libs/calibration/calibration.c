#include "calibration.h"
#include "calibration.pb.h"
#include "current-sense.h"
#include "voltage-sense.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "debugger-interface.pb.h"

extern int16_t dut_current_high_buffer[SDADC_BUFFER_SIZE];
extern int16_t dut_current_low_buffer[SDADC_BUFFER_SIZE];
extern int16_t dut_vout_buffer[SDADC_BUFFER_SIZE_VOLTAGE];

extern uint32_t _calib_debug_dac;
extern uint32_t _calib_emulator_dac;
extern uint32_t _calib_current_ilow;
extern uint32_t _calib_current_ihigh;
const CalibrationConstants calibConstants = {
    (uint16_t*)&_calib_debug_dac,
    (uint16_t*)&_calib_emulator_dac,
    (uint16_t*)&_calib_current_ilow,
    (uint16_t*)&_calib_current_ihigh,
};



void handleEmulatorCalibrationRequest(Emulation_CalibrationCommand command){
  switch(command){
    case Emulation_CalibrationCommand_CurrentZero:{
      calibrateCurrentOffset();
      break;
    }
    case Emulation_CalibrationCommand_Voltage_EmulatorOutput:{
      calibrateVoltage(Supply_Emulator);
      break;
    }
    case Emulation_CalibrationCommand_Voltage_DebugOutput:{
      calibrateVoltage(Supply_Debug);
      break;
    }
  }
}

void writeFlash(uint32_t address, uint32_t* data, uint32_t size){
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef pEraseInit;
    pEraseInit.NbPages = 2;
    pEraseInit.PageAddress = address;
    pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    uint32_t PageError;
    HAL_FLASHEx_Erase(&pEraseInit, &PageError);
    FLASH_WaitForLastOperation(HAL_MAX_DELAY);

    for (int i = 0; i < size; i = i + 4) {
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i, data[i >> 2]);
      FLASH_WaitForLastOperation(HAL_MAX_DELAY);
    }

    HAL_FLASH_Lock();
}

/**
 * Internal DAC control function to bypass the supply handling.
 * Use only when supply is off!
 * @param supply DAC to set
 * @param dacValue Output in DAC units
 */
static void setRawDac(Supplies supply, uint16_t dacValue){
  if (dacValue < MIN_DAC_VALUE) dacValue = MIN_DAC_VALUE;
  if (dacValue > MAX_DAC_VALUE) dacValue = MAX_DAC_VALUE;

  if(supply == Supply_Emulator){
    DAC1->DHR12R2 = dacValue;
    // software trigger channel 2
    SET_BIT(DAC1->SWTRIGR, DAC_SWTRIGR_SWTRIG2);
  } else if(supply == Supply_Debug){
    // Write the value
    DAC1->DHR12R1 = dacValue;
    // software trigger channel 1
    SET_BIT(DAC1->SWTRIGR, DAC_SWTRIGR_SWTRIG1);
  }
}

static void setupSupplyCalibration(Supplies supply){
  if(supply == Supply_Emulator){
    setEmulatorOutputPinMode(GPIO_MODE_OUTPUT_PP);
    setEmulatorOutput(false);

    // turn to software trigger mode
    setEmulatorConstantOutputMode(true, 0);

    setEmulatorOutput(true); // needs to be on to measure emulator output
  } else if(supply == Supply_Debug){
    // always in software trigger mode so no config required
    setDebugOutput(false);
    // can be off to measure debug output
  }
}

void calibrateVoltage(Supplies supply){
  if(getCurrentOperationMode() != Supply_OperationMode_Off){
    return;
  }

  setupSupplyCalibration(supply);

  uint8_t* allocatedBuffer = pvPortMalloc(sizeof(CalibrationArraySizeBytes) + 4); // assuming not 32bit aligned
  uint8_t* alignedBuffer = allocatedBuffer;
  if(((uint32_t)alignedBuffer) & 0b11){
    alignedBuffer -= ((uint32_t)alignedBuffer & 0b11);
    alignedBuffer += 4;
  }
  uint16_t* calibrationBuffer = (uint16_t*)alignedBuffer;

  HAL_NVIC_DisableIRQ(DMA2_Channel3_IRQn);

  for(uint16_t x = MAX_DAC_VALUE - 1; x >= MIN_DAC_VALUE; x = x - 2){
    setRawDac(supply,x);
    osDelay(50);

    float vout = 0.0f;
    if(supply == Supply_Emulator){
      vout = getDutVoltage();
    } else if(supply == Supply_Debug){
      vout = getDebugVoltage();
    }

    calibrationBuffer[(x - CalibrationDacOffset) >> 1] = vout*1000;
  }


  if(supply == Supply_Emulator){
    setEmulatorOutput(false);
  }
  setRawDac(supply,MAX_DAC_VALUE);

  const uint16_t* calibTable;
  if(supply == Supply_Emulator){
    calibTable = calibConstants.emulatorDacToMv;
  } else if(supply == Supply_Debug){
    calibTable = calibConstants.debugDacToMv;
  }

  writeFlash((uint32_t)calibTable, (uint32_t*)calibrationBuffer, CalibrationArraySizeBytes);
  vPortFree(allocatedBuffer);
  HAL_NVIC_EnableIRQ(DMA2_Channel3_IRQn);
}

void calibrateCurrentOffset(void){
  if(getCurrentOperationMode() != Supply_OperationMode_Off){
    return;
  }

  setupSupplyCalibration(Supply_Emulator);

  uint8_t* allocatedBuffer = pvPortMalloc(sizeof(CalibrationArraySizeBytes) + 4); // assuming not 32bit aligned
  uint8_t* alignedBuffer = allocatedBuffer;
  if(((uint32_t)alignedBuffer) & 0b11){
    alignedBuffer -= ((uint32_t)alignedBuffer & 0b11);
    alignedBuffer += 4;
  }
  uint16_t* calibrationBuffer = (uint16_t*)alignedBuffer;

  for(uint16_t x = MAX_DAC_VALUE - 1; x >= MIN_DAC_VALUE; x = x - 2){
    setRawDac(Supply_Emulator, x);
    osDelay(50);

    uint32_t averageHigh = 0;
    for (int i = 0; i < SDADC_BUFFER_SIZE; ++i) {
      averageHigh += dut_current_high_buffer[i] + 32768;
    }
    averageHigh /= SDADC_BUFFER_SIZE;

    calibrationBuffer[(x - CalibrationDacOffset) >> 1] = averageHigh;
  }
  writeFlash((uint32_t)calibConstants.iHighComp, (uint32_t*)calibrationBuffer, CalibrationArraySizeBytes);

  for(uint16_t x = MAX_DAC_VALUE - 1; x >= MIN_DAC_VALUE; x = x - 2){
    setRawDac(Supply_Emulator, x);
    osDelay(50);

    uint32_t averageLow = 0;
    for (int i = 0; i < SDADC_BUFFER_SIZE; ++i) {
      averageLow += dut_current_low_buffer[i] + 32768;
    }
    averageLow /= SDADC_BUFFER_SIZE;

    calibrationBuffer[(x - CalibrationDacOffset) >> 1] = averageLow;
  }

  setEmulatorOutput(false);
  setRawDac(Supply_Emulator, MAX_DAC_VALUE);
  writeFlash((uint32_t)calibConstants.iLowComp, (uint32_t*)calibrationBuffer, CalibrationArraySizeBytes);
  vPortFree(allocatedBuffer);
}


