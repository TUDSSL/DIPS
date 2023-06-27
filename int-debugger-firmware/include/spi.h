#ifndef __SPI_H
#define __SPI_H

#include "platforms/native/platform.h"

// SPI ALWAYS SIZE 4
// MODES:
#define SUPPLY_MODE_BEAT 0x00
#define SUPPLY_MODE_CONTINUES 0x01
#define SUPPLY_MODE_BREAKPOINT 0x02
#define SUPPLY_MODE_ENERGY_THRESHOLD 0x03
#define SUPPLY_MODE_ENERGY_BREAKPOINT 0x04
#define SUPPLY_MODE_INTERMIT_NEXT_LINE 0x05
#define SUPPLY_MODE_COMPENSATE 0x06

// PARAMS:
#define SUPPLY_PARAM_RESERVED 0x00
#define SUPPLY_PARAM_BREAKPOINT 0x01
#define SUPPLY_PARAM_WATCHPOINT 0x02
#define SUPPLY_PARAM_SET 0x03
#define SUPPLY_PARAM_CLEAR 0x04
#define SUPPLY_PARAM_OTHER 0xFF

void spi_init(void);
void spi_send_pause(bool pause);

void setEnergyGuard(bool);
void clearEnergyGuard(void);
void enable_supply(void);
void disable_supply(void);
void profile_start(void);
void profile_stop(void);
void intermit_on_next_line(void);
void compensate(bool enable);
void profile_check_for_boot(void);

#endif  //__SPI_H
