#ifndef SRC_TARGET_MSP430_PC_H_
#define SRC_TARGET_MSP430_PC_H_

#include "target/msp430/common.h"

void SbwSetPC_430Xv2(target *t, uint32_t addr);
uint32_t SbwReadPC(target *t);
uint32_t SbwReadSR(target *t);

#endif // SRC_TARGET_MSP430_PC_H_