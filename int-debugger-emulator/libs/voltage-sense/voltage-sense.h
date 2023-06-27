#ifndef LIBS_VOLTAGE_SENSE_VOLTAGE_SENSE_H_
#define LIBS_VOLTAGE_SENSE_VOLTAGE_SENSE_H_

#include "platform.h"

#define SDADC_BUFFER_SIZE_VOLTAGE 50 * 2
#define SDADC_BUFFER_SIZE_CHANNEL SDADC_BUFFER_SIZE_VOLTAGE/2

void voltageSenseConfig(void);
float getDutVoltage(void);
float getDebugVoltage(void);

#endif /* LIBS_VOLTAGE_SENSE_VOLTAGE_SENSE_H_ */
