#ifndef SRC_TARGET_MSP430_REGISTERS_H_
#define SRC_TARGET_MSP430_REGISTERS_H_

#include "target/msp430/common.h"

void SbwReadCpuRegsXv2(target *t, uint32_t *registerValues);
void SbwReadCpuRegXv2(target *t, uint32_t reg, uint32_t *val);
void SbwWriteCpuRegsXv2(target *t, uint32_t *registerValues);
void SbwWriteCpuRegXv2(target *t, uint32_t reg, uint32_t val);

#endif  // SRC_TARGET_MSP430_REGISTERS_H_