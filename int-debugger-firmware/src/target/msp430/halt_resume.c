
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

#include "target/msp430/halt_resume.h"
#include "target/msp430/pc.h"
#include "target/msp430/memory.h"
#include "target/msp430/registers.h"

#include "gdb_main.h"
#include "spi.h"

void sbwHaltResume(target *t)
{
    struct msp430_priv *msp_priv    = t->priv;
    uint32_t            shiftResult = 0;

    msp_priv->bp_was_hit = false;

    DWT_Delay(400);

    // Restore PC, SR, WDT
    {
        // SR
        SbwWriteCpuRegXv2(t, 2, msp_priv->SR); // val 1

        //TODO determine WDT value 
        // WD
        msp_priv->WDT;
        WriteMem_430Xv2(F_WORD, 0x015c, 0x5A80);

        if(msp_priv->SR & CPUOFF)
        {
            msp_priv->PC -= 2;
        }

        SbwSetPC_430Xv2(t, msp_priv->PC);
    }

    // Restore and release
    {
        set_tclk_sbw();
        IR_Shift(IR_CNTRL_SIG_16BIT);
        DR_Shift16(0x401);

        IR_Shift(IR_ADDR_CAPTURE);

        IR_Shift(IR_CNTRL_SIG_CAPTURE);
        clr_tclk_sbw();

        shiftResult = DR_Shift16(0x0);
        if(shiftResult & (0x0001 << 1))
        {  // CNTRL_SIG_HALT
            // not executed
            IR_Shift(IR_CNTRL_SIG_16BIT);
            DR_Shift16(0x0403);
        }

        set_tclk_sbw();
    }

    // Enable EEM
    {
        shiftResult = IR_Shift(IR_EMEX_WRITE_CONTROL);
        DR_Shift16(EEM_EN | CLEAR_STOP | EMU_CLK_EN);

        IR_Shift(IR_CNTRL_SIG_CAPTURE);
    }

    // Power mode handling start
    // note: ulp not supported
    {
        // Disable LPMx.5
        {
            IR_Shift(IR_TEST_3V_REG);
            DR_Shift16(0x0);
            DR_Shift16(0x40a0);

            DWT_Delay(5000);
            DWT_Delay(5000);
            DWT_Delay(5000);
            DWT_Delay(5000);

            IR_Shift(IR_TEST_REG);
            DR_Shift32(0x10000);
            DR_Shift32(0x10000);

            DWT_Delay(5000);
            DWT_Delay(5000);
            DWT_Delay(5000);
            DWT_Delay(5000);
        }

        // Manually disable DBGJTAGON bit
        // TODO: support other jtag versions
        if(0x99 == JTAG_ID99)
        {
            IR_Shift(IR_TEST_3V_REG);
            shiftResult = DR_Shift16(0x0000);
            DR_Shift16(shiftResult & ~(DBGJTAGON));
        }
    }

    // sync with target running var
    {
        // ??
    }

    // // Manually disable
    // IR_Shift(IR_TEST_3V_REG);
    // DR_Shift16(0x0);
    // DR_Shift16(0x4020);

    if(disable_supply_after_next_continue && m_debugger_mode == MODE_PROFILE){
        disable_supply_after_next_continue = false;
        disable_supply();
    }   

    IR_Shift(IR_CNTRL_SIG_RELEASE);

    // set_sbwtck(true);
    // DWT_Delay(500);
    // set_sbwtck(true);
}