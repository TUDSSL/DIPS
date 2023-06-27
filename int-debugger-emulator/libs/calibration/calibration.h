#ifndef LIBS_CALIBRATION_CALIBRATION_H_
#define LIBS_CALIBRATION_CALIBRATION_H_
#include "platform.h"
#include "supply.h"

#define CalibrationArraySizeValues (MAX_DAC_VALUE - MIN_DAC_VALUE + 1) / 2
#define CalibrationArraySizeBytes  sizeof(uint16_t) * CalibrationArraySizeValues
#define CalibrationDacOffset MIN_DAC_VALUE

typedef struct {
  uint16_t* debugDacToMv;
  uint16_t* emulatorDacToMv;
  uint16_t* iLowComp;
  uint16_t* iHighComp;
} CalibrationConstants;

extern const CalibrationConstants calibConstants;

void calibrateCurrentOffset(void);
void calibrateVoltage(Supplies supply);

#endif /* LIBS_CALIBRATION_CALIBRATION_H_ */
