#ifndef SRC_TARGET_MSP430_MEMORY_H_
#define SRC_TARGET_MSP430_MEMORY_H_

#include "target/msp430/common.h"

void sbw_mem_read(target *t, void *dest, target_addr_t src, size_t len);
void sbw_mem_write(target *t, target_addr_t dest, const void *src, size_t len);

void     ReadMemQuick_430Xv2(unsigned long StartAddr, unsigned long Length, uint16_t *DataArray);
int      WriteMem_430Xv2(uint16_t Format, uint32_t Addr, uint16_t Data);
uint16_t ReadMem_430Xv2(uint16_t Format, uint32_t Addr);

#endif  // SRC_TARGET_MSP430_MEMORY_H_