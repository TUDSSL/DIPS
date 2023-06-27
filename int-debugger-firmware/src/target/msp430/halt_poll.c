
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
#include "gdb_main.h"
#include "target/msp430/halt_poll.h"
#include "spi.h"

/***
 * Adapted from MSPDebug
 */
static enum target_halt_reason PollforBpHit(unsigned long long JStateValue)
{
    if((!(JStateValue & LPMX5_MASK_J)) && (JStateValue & BP_HIT_MASK_J))
    {
        return TARGET_HALT_BREAKPOINT;
    }
    return TARGET_HALT_RUNNING;
}

/**
 * Function to check if DUT is under JTAG Control
 *
 * Not documented nor implemented on MSP-FET.
 * Only done to avoid the assumption that a halt request = halted DUT
 *
 */
static enum target_halt_reason PollforHalt(unsigned long long JStateValue)
{
    if(!(JStateValue & 0x1000000000000) || (JStateValue == 0xFFFFFFFFFFFFFFFF))
    {
        return TARGET_HALT_REQUEST;
    }

    return TARGET_HALT_RUNNING;
}

enum target_halt_reason sbwHaltPoll(target *t)
{
    struct msp430_priv     *msp_priv = t->priv;
    enum target_halt_reason reason   = TARGET_HALT_RUNNING;

    uint32_t returnVal = IR_Shift(IR_JSTATE_ID);
    if(returnVal != JTAG_ID99)
    {                
        profile_stop();
        return TARGET_HALT_INTERMITTENT;
    }

    uint64_t lOut_long_long;
    lOut_long_long = DR_Shift64(0xFFFFFFFFFFFFFFFF);

    reason = PollforBpHit(lOut_long_long);
    if(reason == TARGET_HALT_BREAKPOINT)
    {
        msp_priv->bp_was_hit = true;
    }

    // When BP is hit, it will remain halted until the next resume
    // Readout of bp jstate will not be correct until then 
    if(msp_priv->bp_was_hit && reason == TARGET_HALT_RUNNING)
    {
        reason = TARGET_HALT_BREAKPOINT;
    }


    if(reason != TARGET_HALT_RUNNING)
    {
        if(m_debugger_mode != MODE_PROFILE) enable_supply();
        DWT_Delay(400);
        t->halt_request(t);
    }

    return reason;
}