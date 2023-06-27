#include "target/sbw_jtag.h"
#include "target/sbw_transport.h"
#include "target/target_internal.h"
#include "gdb_packet.h"

#include "target/EEM_defs.h"

#include "general.h"
#include "target/sbw_scan.h"
#include "target.h"
#include "target/adiv5.h"
#include "target/jtag_devs.h"
#include "target/msp430.h"

#include "target/msp430/registers.h"
#include "target/msp430/pc.h"

void SbwWriteCpuRegXv2(target *t, uint32_t reg, uint32_t val)
{
    if(reg == PC_REG)
    {
        SbwSetPC_430Xv2(t, val);
        return;
    }

    if(reg == SR_REG)
    {
        // Check to see if 16 bits are written
        // SR_REG is only 16 bits wide
        if(val & 0xff0000)
        {
            return;
        }
    }

    // TODO: support different jtag_ids and alt addr
    uint16_t       jtagId                  = 0x99;
    // bool           altRomAddressForCpuRead = false;
    // const uint16_t jmbAddr                 = (jtagId == 0x98) ? 0x14c : 0x18c;
    uint16_t       regAddr;

    // translate register number to address
    {
        regAddr = (reg & 0x000f) + 0x0080;

        // Append data to second byte of the addr
        // Needed as 20 bits data must be fitted in 16 bit data shift
        regAddr += (uint16_t)((val >> 8) & 0x00000F00);
    }

    clr_tclk_sbw();
    IR_Shift(IR_DATA_16BIT);
    set_tclk_sbw();
    DR_Shift16(regAddr);

    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x1401);

    IR_Shift(IR_DATA_16BIT);
    tclk_sbw();
    DR_Shift16(val);
    tclk_sbw();
    DR_Shift16(0x3ffd);
    tclk_sbw();
    clr_tclk_sbw();

    IR_Shift(IR_ADDR_CAPTURE);
    DR_Shift20(0x0);
    set_tclk_sbw();

    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x0501);
    tclk_sbw();
    clr_tclk_sbw();

    IR_Shift(IR_DATA_CAPTURE);
    set_tclk_sbw();
}

void SbwWriteCpuRegsXv2(target *t, uint32_t *registerValues)
{
    uint16_t registerIndex = 0;

    for(registerIndex = 0; registerIndex < 16; registerIndex++)
    {
        SbwWriteCpuRegXv2(t, registerIndex, registerValues[registerIndex]);
    }
}

/**
 * Read CPU register value, except R0 and R2
 *
 * @param reg register number
 * @param value register value pointer
 */
void SbwReadCpuRegXv2(target *t, uint32_t reg, uint32_t *val)
{
    if(reg == PC_REG)
    {
        *val = SbwReadPC(t);
        return ;
    }

    if(reg == SR_REG)
    {
        // TODO special handling for SR regs
    }

    uint16_t Rx_l = 0;
    uint16_t Rx_h = 0;
    // TODO: support different jtag_ids and alt addr
    uint16_t       jtagId                  = 0x99;
    bool           altRomAddressForCpuRead = false;
    const uint16_t jmbAddr                 = (jtagId == 0x98) ? 0x14c : 0x18c;

    // translate register number to address
    uint16_t regAddr = (((reg << 8) & 0x0F00)) + 0x0060;

    clr_tclk_sbw();
    IR_Shift(IR_DATA_16BIT);
    set_tclk_sbw();
    DR_Shift16(regAddr);

    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x1401);

    IR_Shift(IR_DATA_16BIT);
    tclk_sbw();
    DR_Shift16((altRomAddressForCpuRead) ? 0x0ff6 : jmbAddr);
    tclk_sbw();
    DR_Shift16(0x3ffd);
    clr_tclk_sbw();

    if(altRomAddressForCpuRead)
    {
        IR_Shift(IR_CNTRL_SIG_16BIT);
        DR_Shift16(0x0501);
    }

    IR_Shift(IR_DATA_CAPTURE);
    set_tclk_sbw();
    Rx_l = DR_Shift16(0x0000);
    tclk_sbw();
    Rx_h = DR_Shift16(0x0000);

    tclk_sbw();
    tclk_sbw();
    tclk_sbw();

    if(!altRomAddressForCpuRead)
    {
        IR_Shift(IR_CNTRL_SIG_16BIT);
        DR_Shift16(0x0501);
    }

    clr_tclk_sbw();
    IR_Shift(IR_DATA_CAPTURE);
    set_tclk_sbw();

    *val = (Rx_h << 16) | Rx_l;
}

/**
 * Read CPU register values, except R0 and R2
 *
 * @param reg register values
 */
void SbwReadCpuRegsXv2(target *t, uint32_t *registerValues)
{
    uint16_t registerIndex = 0;

    // Read PC
    registerValues[registerIndex] = SbwReadPC(t);
    registerIndex += 1;

    // Read SP
    registerValues[registerIndex] = SbwReadSR(t);
    registerIndex += 1;

    // Read Status Register
    // TODO
    registerValues[registerIndex] = 0x66;
    registerIndex += 1;

    // Read the rest of General Purpose registers.
    for(registerIndex = 0; registerIndex < 16; registerIndex++)
    {
        SbwReadCpuRegXv2(t, registerIndex, &(registerValues[registerIndex]));
    }

#ifndef GDB_ARM_WORKAROUND
    return;
#endif

    // deliver compatibility values
    {
        // sp
        registerValues[registerIndex] = registerValues[SP_REG];
        registerIndex += 1;

        // lr
        registerValues[registerIndex] = 0x0;
        registerIndex += 1;

        // pc
        registerValues[registerIndex] = registerValues[PC_REG];
        registerIndex += 1;

        // xpsr
        registerValues[registerIndex] = 0x0;
        registerIndex += 1;
    }
}