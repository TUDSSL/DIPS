#ifndef LIBS_CURRENT_SENSE_CURRENT_SENSE_H_
#define LIBS_CURRENT_SENSE_CURRENT_SENSE_H_

#include "platform.h"

#define SDADC_BUFFER_SIZE 50

void currentSenseConfig(void);
float getDutLowCurrent(void);
float getDutHighCurrent(void);
float getDutCurrent(void);

void setBypass(bool status);
bool getBypassState();

#endif /* LIBS_CURRENT_SENSE_CURRENT_SENSE_H_ */
