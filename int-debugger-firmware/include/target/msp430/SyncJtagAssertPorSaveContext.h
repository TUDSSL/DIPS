#ifndef SRC_TARGET_MSP430_ASSERT_POR_H_
#define SRC_TARGET_MSP430_ASSERT_POR_H_

#include "target.h"

uint8_t SyncJtagAssertPorSaveContext(uint32_t* start_pc, uint8_t* start_wdt);

#endif