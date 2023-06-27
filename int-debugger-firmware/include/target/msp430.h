#ifndef __CORTEXM_H
#define __CORTEXM_H

#include "target.h"

#define SP_REG 0x01
#define SP_DUP_REG 0x0016
#define PC_REG 0x00
#define PC_DUP_REG 0x018
#define SR_REG 0x02

#define MSP430_MAX_WATCHPOINTS 3

static const char msp430_driver_str[] = "MSP430";

typedef struct
{
    uint16_t device_id;
    uint16_t core_id;
    uint16_t jtag_id;
    uint32_t device_id_ptr;

    uint32_t start_pc;
    uint8_t start_wdt;
} dev_dsc_t;

struct msp430_priv
{
    // MSP430 specific
    uint16_t jtagID;
    bool     jtagIDIsXv2;
    unsigned hw_watchpoint_max;
    uint16_t hw_breakpoint_mask;
    unsigned hw_breakpoint_max;
    uint16_t hw_watchpoint_mask;

    // Cached vriables
    uint32_t PC;
    uint32_t SR; 
    uint8_t WDT;

    bool bp_was_hit;

    // General purpose
    bool mmu_fault;
};

bool msp430_probe(dev_dsc_t *dsc);

#endif
