#ifndef USERTESTING_WISP_BASE_CHECKPOINTING_H
#define USERTESTING_WISP_BASE_CHECKPOINTING_H
#include "stdint.h"

int restore();

void checkpoint(uint16_t taskID);

#endif //USERTESTING_WISP_BASE_CHECKPOINTING_H
