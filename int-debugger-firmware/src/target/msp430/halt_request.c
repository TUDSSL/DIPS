
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

#include "target/msp430/halt_request.h"
#include "target/msp430/pc.h"
#include "target/msp430/memory.h"
#include "target/msp430/registers.h"
#include "target/msp430/DisableLpmx5.h"

static void SyncJtagXv2(void)
{
    unsigned short i = 50, lOut = 0;
    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x1501);
    IR_Shift(IR_CNTRL_SIG_CAPTURE);
    do
    {
        lOut = DR_Shift16(0x0000);
        i--;
    } while(((lOut == 0xFFFF) || !(lOut & 0x0200)) && i);
}

/**
 * @brief request a halt and save
 *
 * @param t
 *
 * @note Based around MSPDebugStacks' SyncJtagConditionalSaveContext
 */
void sbwHaltRequest(target *t)
{
    struct msp430_priv *msp_priv    = t->priv;
    uint32_t            shiftResult = 0;
    uint16_t            irRequest   = 0;
    uint16_t            MyOut[5];
    uint32_t            tempPc;

    // Disable Lpmx5
    DisableLpmx5(t);

    // Make sure that CPUOFF bit is set
    {
        IR_Shift(IR_EMEX_READ_CONTROL);
        shiftResult = DR_Shift16(0x0);

        // Check if device is already stopped by EEM
        if(!(shiftResult & EEM_STOPPED))
        {
            IR_Shift(IR_CNTRL_SIG_CAPTURE);
            shiftResult = DR_Shift16(0x0);

            // Check CPUOFF bit
            if(!(shiftResult & CNTRL_SIG_CPUOFF))
            {
                uint16_t tbValue, tbCntrl, tbMask, tbComb, tbBreak;

                IR_Shift(IR_EMEX_DATA_EXCHANGE32);
                DR_Shift32(MBTRIGxVAL + READ + TB0);  // load address
                // shift in dummy 0
                tbValue = DR_Shift32(0);  // Trigger Block 0 control register

                DR_Shift32(MBTRIGxCTL + READ + TB0);  // load address
                // shift in dummy 0
                tbCntrl = DR_Shift32(0);  // Trigger Block 0 mask register

                DR_Shift32(MBTRIGxMSK + READ + TB0);  // load address
                // shift in dummy 0
                tbMask = DR_Shift32(0);

                // Trigger Block 0 combination register
                DR_Shift32(MBTRIGxCMB + READ + TB0);  // load address
                tbComb = DR_Shift32(0);               // shift in dummy 0

                // Trigger Block 0 combination register
                DR_Shift32(BREAKREACT + READ);  // load address
                tbBreak = DR_Shift32(0);        // shift in dummy 0

                // now configure a trigger on the next instruction fetch
                DR_Shift32(MBTRIGxCTL + WRITE + TB0);  // load address
                DR_Shift32(CMP_EQUAL + TRIG_0 + MAB);
                DR_Shift32(MBTRIGxMSK + WRITE + TB0);  // load address
                DR_Shift32(MASK_ALL);
                DR_Shift32(MBTRIGxCMB + WRITE + TB0);  // load address
                DR_Shift32(EN0);
                DR_Shift32(BREAKREACT + WRITE);  // load address
                DR_Shift32(EN0);

                // enable EEM to stop the device
                DR_Shift32(GENCLKCTRL + WRITE);  // load address
                DR_Shift32(MCLK_SEL0 + SMCLK_SEL0 + ACLK_SEL0 + STOP_MCLK + STOP_SMCLK + STOP_ACLK);
                IR_Shift(IR_EMEX_WRITE_CONTROL);
                DR_Shift16(EMU_CLK_EN + CLEAR_STOP + EEM_EN);

                // Note timeout is set out much lower than the debug stack
                {
                    short lTimeout = 0;
                    do
                    {
                        IR_Shift(IR_EMEX_READ_CONTROL);
                        lTimeout++;
                    } while(!(DR_Shift16(0x0000) & EEM_STOPPED) && lTimeout < 10);
                }

                // restore the setting of Trigger Block 0 previously stored
                // Trigger Block 0 value register
                IR_Shift(IR_EMEX_DATA_EXCHANGE32);
                DR_Shift32(MBTRIGxVAL + WRITE + TB0);  // load address
                DR_Shift32(tbValue);
                DR_Shift32(MBTRIGxCTL + WRITE + TB0);  // load address
                DR_Shift32(tbCntrl);
                DR_Shift32(MBTRIGxMSK + WRITE + TB0);  // load address
                DR_Shift32(tbMask);
                DR_Shift32(MBTRIGxCMB + WRITE + TB0);  // load address
                DR_Shift32(tbComb);
                DR_Shift32(BREAKREACT + WRITE);  // load address
                DR_Shift32(tbBreak);
            }
        }
    }

    // Enable clock control before sync
    IR_Shift(IR_EMEX_WRITE_CONTROL);
    DR_Shift16(EMU_CLK_EN + EEM_EN);

    SyncJtagXv2();

    // reset CPU stop reaction - CPU is now under JTAG control
    // Note: does not work on F5438 due to note 772, State storage triggers twice on single stepping
    IR_Shift(IR_EMEX_WRITE_CONTROL);
    DR_Shift16(EMU_CLK_EN + CLEAR_STOP + EEM_EN);
    DR_Shift16(EMU_CLK_EN + CLEAR_STOP);

    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x1501);
    // clock system into Full Emulation State now...
    // while checking control signals CPUSUSP (pipe_empty), CPUOFF and HALT

    // Check if wait is set
    {
        IR_Shift(IR_CNTRL_SIG_CAPTURE);
        shiftResult = DR_Shift16(0);
        if((shiftResult & 0x8) == 0x8)
        {
            uint32_t TimeOut = 0;
            // wait until wait is end
            while((shiftResult & 0x8) == 0x8 && TimeOut++ < 30000)
            {
                clr_tclk_sbw();  // provide falling clock edge
                set_tclk_sbw();  // provide rising clock edge
                IR_Shift(IR_CNTRL_SIG_CAPTURE);
                shiftResult = DR_Shift16(0);
            }
        }
    }

    // check pipe empty
    {
        const unsigned long MaxCyclesForSync = 10000UL;
        uint8_t             pipe_empty       = FALSE;
        uint16_t            i                = 0;
        IR_Shift(IR_CNTRL_SIG_CAPTURE);
        do
        {
            clr_tclk_sbw();  // provide falling clock edge
            // check control signals during clock low phase
            // shift out current control signal register value
            (DR_Shift16(0x000) & CNTRL_SIG_CPUSUSP) ? (pipe_empty = TRUE) : (pipe_empty = FALSE);
            set_tclk_sbw();  // provide rising clock edge
            i++;             // increment loop counter for braek condition
        }
        // break condition:
        //    pipe_empty = 1
        //    or an error occured (MaxCyclesForSync exeeded!!)
        while(!pipe_empty && (i < MaxCyclesForSync));

        //! \TODO check error condition
        if(i >= MaxCyclesForSync)
        {
            ;
        }
    }

    // the interrupts will be diabled now - JTAG takes over control of the control signals
    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x0501);

    // i_SaveContext
    {
        uint8_t cpuOff = 0;

        // provide 1 clock in order to have the data ready in the first transaction slot
        tclk_sbw();
        IR_Shift(IR_ADDR_CAPTURE);
        tempPc = DR_Shift20(0);

        // shift out current control signal register value
        IR_Shift(IR_CNTRL_SIG_CAPTURE);
        shiftResult = DR_Shift16(0);
        if(shiftResult & CNTRL_SIG_CPU_OFF)
        {
            cpuOff = TRUE;
        }
        else
        {
            cpuOff = FALSE;
        }

        irRequest = (shiftResult & 0x4);

        // adjust program counter according to control signals
        tempPc -= (cpuOff) ? 2 : 4;

        {
            /********************************************************/
            /* Note 1495, 1637 special handling for program counter */
            /********************************************************/
            if((tempPc & 0xFFFF) == 0xFFFE)
            {
                tempPc = ReadMem_430Xv2(F_WORD, 0xFFFE);

                MyOut[1] = (unsigned short)(tempPc & 0xFFFF);
                MyOut[2] = (unsigned short)(tempPc >> 16);
            }
            /* End note 1495, 1637 */
            else
            {
                MyOut[1] = (unsigned short)(tempPc & 0xFFFF);
                MyOut[2] = (unsigned short)(tempPc >> 16);
            }
        }
    }

    // set EEM FEATURE enable now!!!
    IR_Shift(IR_EMEX_WRITE_CONTROL);
    DR_Shift16(EMU_FEAT_EN + EMU_CLK_EN + CLEAR_STOP);

    // check for Init State
    IR_Shift(IR_CNTRL_SIG_CAPTURE);
    DR_Shift16(0x0);

    // hold Watchdog Timer
    // uint16_t wdtAddress = jtagIdIsXv2 ? WDT_ADDR_XV2 : WDT_ADDR_CPU;
    {
        uint16_t wdtAddress = WDT_ADDR_XV2;
        shiftResult         = ReadMem_430Xv2(F_WORD, wdtAddress);
        MyOut[0]            = (uint16_t)shiftResult;

        // TODO figure out wdt_value
        uint16_t wdt_value = 0x5a00;  // wdt password
        wdt_value |= (MyOut[0] & 0xFF);
        WriteMem_430Xv2(F_WORD, wdtAddress, wdt_value);
    }

    {
        unsigned short Rx_l;
        unsigned long  Rx;

        // save Status Register content
        uint32_t SR = 2;
        SbwReadCpuRegXv2(t, SR_REG, &Rx);
        msp_priv->SR = Rx;

        // reset Status Register to 0 - needs to be restored again for release
        Rx_l = (((unsigned short)Rx) & 0xFFE7);  // clear CPUOFF/GIE bit
        SbwWriteCpuRegXv2(t, SR, Rx_l);
    }

    MyOut[4] = irRequest;

    // Transfer ouput array to msp430 priv
    uint32_t PC_addr = tempPc;
    msp_priv->PC     = PC_addr;
}