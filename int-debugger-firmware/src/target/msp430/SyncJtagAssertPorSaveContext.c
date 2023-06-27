#include "target/msp430/SyncJtagAssertPorSaveContext.h"
#include "target/msp430.h"
#include "target/sbw_jtag.h"
#include "target/EEM_defs.h"
#include "target/sbw_transport.h"
#include "target/target_internal.h"
#include "target.h"
#include "target/jtag_devs.h"
#include "target/msp430/DisableLpmx5.h"

#include "target/msp430/memory.h"
#include "target/msp430/pc.h"

// ET wrapper module definitions
#define ETKEY 0x9600
#define ETKEYSEL 0x0110
#define ETCLKSEL 0x011E

/**
 * @brief wait for syncronization
 * @note Based around MSPDebug stack
 * @return returns SC_ERR_NONE on no error, SC_ERR_GENERIC otherwise
 */
static uint8_t wait_for_synch(void)
{
    unsigned short i = 0;
    IR_Shift(IR_CNTRL_SIG_CAPTURE);

    while(!(DR_Shift16(0) & 0x0200) && i++ < 50)
        ;
    if(i >= 50)
    {
        return SC_ERR_GENERIC;
    }
    return SC_ERR_NONE;
}

// TODO makes this for other devices, currently only for msp430fr5994
/**
 * @brief MSPDebugStack
 * @note Based around MSPDebug stack
 * @return returns SC_ERR_NONE on no error, SC_ERR_GENERIC otherwise
 */
static void clkModules(void)
{
    uint16_t mclk_modules[32];
    uint32_t mclkCntrl0 = 0;

    // TODO make this for other devices, currently only for msp430fr5994
    mclk_modules[0]  = 0x0A;  // 15 - WDT_A
    mclk_modules[1]  = 0x8D;  // 14 - TA4_2
    mclk_modules[2]  = 0x8C;  // 13 - TA2_3_B
    mclk_modules[3]  = 0x8B;  // 12 - TA2_2_B
    mclk_modules[4]  = 0x8F;  // 11 - TA3_1
    mclk_modules[5]  = 0x8E;  // 10 - TA3_0
    mclk_modules[6]  = 0x9D;  //  9 - TB7_0
    mclk_modules[7]  = 0x2C;  //  8 - eUSCIA0
    mclk_modules[8]  = 0x2E;  //  7 - eUSCIA2
    mclk_modules[9]  = 0x2D;  //  6 - eUSCIA1
    mclk_modules[10] = 0x31;  //  5 - eUSCIB1
    mclk_modules[11] = 0x30;  //  4 - eUSCIB0
    mclk_modules[12] = 0x8A;  //  3 - RTC
    mclk_modules[13] = 0xA8;  //  2 - COMP_E
    mclk_modules[14] = 0xD8;  //  1 - ADC12B
    mclk_modules[15] = 0x50;  //  0 - PORT
    mclk_modules[28] = 0x60;  // 19 - AES128
    mclk_modules[29] = 0x33;  // 18 - eUSCIB3
    mclk_modules[30] = 0x32;  // 17 - eUSCIB2
    mclk_modules[31] = 0x2F;  // 16 - eUSCIA3

    // mclkCntrl0 = 0b0001000000100000111111;
    mclkCntrl0 = 0b01111000000000000111111111111111;

    for(int i = 0; i < 32; i++)
    {
        uint16_t v = (mclkCntrl0 & ((unsigned long)0x00000001 << i)) != 0;
        WriteMem_430Xv2(F_WORD, ETKEYSEL, ETKEY | mclk_modules[i]);
        WriteMem_430Xv2(F_WORD, ETCLKSEL, v);
    }
}

/**
 * Execute a Power-On Reset (POR) using JTAG CNTRL SIG register
 *
 * @returns SC_ERR_NONE if target is in Full-Emulation-State afterwards, SC_ERR_GENERIC otherwise
 */
static uint8_t ExecutePOR_430Xv2(uint32_t* start_pc, uint8_t *start_wdt)
{
    uint32_t pc = 0;
    uint32_t sr = 0;

    // Empty the pipe
    tclk_sbw();

    // prepare access to the JTAG CNTRL SIG register
    {
        IR_Shift(IR_CNTRL_SIG_16BIT);

        // release CPUSUSP signal and apply POR signal
        DR_Shift16(0x0C01);

        // was original 40ms, new 20ms
        for(int i = 0; i < 4; i++)
        {
            DWT_Delay(5000);
        }

        // release POR signal again
        DR_Shift16(0x0401);
    }

    // TODO: jtag id variable
    uint8_t id = JTAG_ID99;

    // Set PC to 'safe' memory location
    if(id == JTAG_ID91 || id == JTAG_ID99 || id == JTAG_ID98)
    {
        IR_Shift(IR_DATA_16BIT);
        tclk_sbw();
        tclk_sbw();
        DR_Shift16(SAFE_FRAM_PC);

        // PC is set to 0x4 - MAB value can be 0x6 or 0x8

        // drive safe address into PC
        tclk_sbw();
        if(id == JTAG_ID91)
            tclk_sbw();

        IR_Shift(IR_DATA_CAPTURE);
    }
    else
    {
        for(int i; i < 3; i++)
        {
            tclk_sbw();
        }
    }

    // two more to release CPU internal POR delay signals
    tclk_sbw();
    tclk_sbw();

    // set CPUSUSP signal again
    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x0501);
    tclk_sbw();

    // set EEM FEATURE enable now!!!
    IR_Shift(IR_EMEX_WRITE_CONTROL);
    DR_Shift16(EMU_FEAT_EN + EMU_CLK_EN + CLEAR_STOP);

    // Check that sequence exits on Init State
    IR_Shift(IR_CNTRL_SIG_CAPTURE);
    DR_Shift16(0x0000);

    // disable Watchdog Timer on target device now by setting the HOLD signal
    // in the WDT_CNTRL register
    {
        // TODO exception for fr41xx
        *start_wdt = ReadMem_430Xv2(F_WORD, 0x15C);

        uint16_t wdtVal = *start_wdt | 0x5A80;

        WriteMem_430Xv2(F_WORD, 0x15C, wdtVal);
    }

    // Capture MAB - the actual PC value is (MAB - 4)
    IR_Shift(IR_ADDR_CAPTURE);
    uint32_t shiftResult = DR_Shift20(0x0);

    /*****************************************/
    /* Note 1495, 1637 special handling      */
    /*****************************************/
    if(((shiftResult & 0xFFFF) == 0xFFFE) || (id == shiftResult) || (id == JTAG_ID99) || (id == JTAG_ID98))
    {
        pc = ReadMem_430Xv2(F_WORD, 0xFFFE) & 0x000FFFFE;
    }
    else
    {
        pc = shiftResult - 4;
    }

    // Status Register should be always 0 after a POR
    sr = 0;
    if(wait_for_synch() != SC_ERR_NONE)
        return SC_ERR_GENERIC;

    // Disabled as this is very device dependent
    // clkModules();

    // switch back system clocks to original clock source but keep them stopped
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);
    DR_Shift32(GENCLKCTRL + WRITE);
    DR_Shift32(MCLK_SEL0 + SMCLK_SEL0 + ACLK_SEL0 + STOP_MCLK + STOP_SMCLK + STOP_ACLK);

    // reset Vacant Memory Interrupt Flag inside SFRIFG1
    if(id == JTAG_ID91)
    {
        uint16_t specialFunc = ReadMem_430Xv2(F_WORD, 0x0102);
        if(specialFunc & 0x8)
        {
            SbwSetPC_430Xv2(NULL, SAFE_FRAM_PC);
            IR_Shift(IR_CNTRL_SIG_16BIT);
            DR_Shift16(0x0501);
            set_tclk_sbw();
            IR_Shift(IR_DATA_16BIT);

            specialFunc &= ~0x8;
            WriteMem_430Xv2(F_WORD, 0x0102, specialFunc);
        }
    }

    *start_pc = pc;

    return (SC_ERR_NONE);
}

/**
 * @brief Resync the JTAG connection and execute a Power-On-Reset
 *
 * @param t target
 * @returns SC_ERR_NONE if operation was successful, SC_ERR_GENERIC otherwise
 * @note Loosely around MSPDebug stack
 */
uint8_t SyncJtagAssertPorSaveContext(uint32_t* start_pc, uint8_t* start_wdt)
{
    uint32_t shiftResult = 0;
    uint16_t irRequest   = 0;
    uint16_t MyOut[5];
    uint16_t id = 0x99;

    // Disabled disabling as it takes too long
    // DisableLpmx5(NULL);

    // Enable clock control before sync
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);
    DR_Shift32(GENCLKCTRL + WRITE);
    DR_Shift32(MCLK_SEL3 + SMCLK_SEL3 + ACLK_SEL3 + STOP_MCLK + STOP_SMCLK + STOP_ACLK);

    // Enable emulation clocks
    IR_Shift(IR_EMEX_WRITE_CONTROL);
    DR_Shift16(EMU_CLK_EN + EEM_EN);

    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x1501);  // Set device into JTAG mode + read

    if((IR_Shift(IR_CNTRL_SIG_CAPTURE) != JTAG_ID91) && (IR_Shift(IR_CNTRL_SIG_CAPTURE) != JTAG_ID99) &&
       (IR_Shift(IR_CNTRL_SIG_CAPTURE) != JTAG_ID98))
    {
        return (SC_ERR_GENERIC);
    }

    // wait for sync
    if(wait_for_synch() != SC_ERR_NONE)
        return SC_ERR_GENERIC;

    // execute a Power-On-Reset
    if(ExecutePOR_430Xv2(start_pc, start_wdt) != SC_ERR_NONE)
    {
        return (SC_ERR_GENERIC);
    }

    return SC_ERR_NONE;
}