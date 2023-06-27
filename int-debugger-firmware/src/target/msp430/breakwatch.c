
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

#include "target/msp430/breakwatch.h"

static void break_set_bw_addr(uint32_t emex_addr, uint32_t bp_addr)
{
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);
    DWT_Delay(15);

    for(int i = 0; i < 4; i++)
    {
        // Sends which BP is meant to be set
        DR_Shift32(emex_addr | MX_WRITE);

        // sends memory addr
        DR_Shift32(bp_addr);
    }
}

static void break_set_cntrl(uint32_t emex_addr)
{
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);
    DWT_Delay(15);

    for(int i = 0; i < 4; i++)
    {
        // Sends which BP is meant to be set
        DR_Shift32(emex_addr | MX_CNTRL | MX_WRITE);

        // sends control variables
        DR_Shift32(MAB + TRIG_0 + CMP_EQUAL);
    }
}

static void break_set_mask(uint32_t emex_addr, uint32_t mask)
{
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);
    DWT_Delay(15);

    for(int i = 0; i < 4; i++)
    {
        // Sends which BP is meant to be set
        DR_Shift32(emex_addr | MX_WRITE | MX_MASK);
        DWT_Delay(12);

        DR_Shift32(mask);
        DWT_Delay(12);
    }
}

static void break_set_comb(uint32_t emex_addr, uint32_t comb)
{
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);
    DWT_Delay(15);

    for(int i = 0; i < 4; i++)
    {
        // Sends which BP is meant to be set
        DR_Shift32(emex_addr | MX_WRITE | MX_COMB);
        DWT_Delay(12);

        DR_Shift32(comb);
        DWT_Delay(12);
    }
}

static void break_set_cpu_stop(uint32_t cpu_stop)
{
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);
    DWT_Delay(15);

    for(int i = 0; i < 4; i++)
    {
        // Sends which BP is meant to be set
        DR_Shift32(MX_WRITE | MX_CPUSTOP);
        DWT_Delay(12);

        DR_Shift32(cpu_stop);
        DWT_Delay(12);
    }
}

static void sbw_clear_bp_hard(target *t, struct breakwatch *bw)
{
    struct msp430_priv *msp_priv  = t->priv;
    uint8_t             bp        = bw->reserved[0];
    uint16_t            emex_addr = bp * 0x08;

    // Sets BP PC addr
    break_set_bw_addr(emex_addr, 0x0);
    DWT_Delay(400);

    // Sets masks of trigger
    break_set_mask(emex_addr, ~0x0);
    DWT_Delay(400);

    // Sets combination logic
    break_set_comb(emex_addr, 0);
    DWT_Delay(400);

    // Sets cpu stop
    break_set_cpu_stop(msp_priv->hw_breakpoint_mask);
    DWT_Delay(400);
}

// Reverse engineered
static int sbw_set_bp_hard(target *t, struct breakwatch *bw)
{
    struct msp430_priv *msp_priv  = t->priv;
    uint8_t             bp        = bw->reserved[0];
    uint8_t             cpu_mask  = msp_priv->hw_breakpoint_mask;
    uint16_t            emex_addr = bp * 0x08;

    // TODO: support various masks/ranges

    // set breakpoint
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);
    DR_Shift32(GENCTRL + MX_WRITE);
    DR_Shift32(EEM_EN + CLEAR_STOP + EMU_CLK_EN + EMU_FEAT_EN);

    // Sets BP PC addr - value register
    break_set_bw_addr(emex_addr, bw->addr);
    DWT_Delay(400);

    // Sets masks of trigger - mask register
    break_set_mask(emex_addr, 0xFFF00000);
    DWT_Delay(400);

    // Sets bp control - control register
    break_set_cntrl(emex_addr);
    DWT_Delay(400);

    // Sets combination logic - combination register
    break_set_comb(emex_addr, 1 << bp);
    DWT_Delay(400);

    // Sets cpu stop
    break_set_cpu_stop(cpu_mask);
    DWT_Delay(400);
    return 0;
}

int sbw_break_single_step(target *t)
{
    struct msp430_priv *msp_priv  = t->priv;
    uint16_t            emex_addr = 0 * 0x08;

    // set breakpoint
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);
    DR_Shift32(GENCTRL + MX_WRITE);
    DR_Shift32(BPCNTRL_EQ | BPCNTRL_RW_DISABLE | BPCNTRL_IF | BPCNTRL_MAB);

    // write mask register
    DR_Shift32(MX_MASK + MX_WRITE);
    DR_Shift32(0xFFFFFFFF);
    // write combination register
    DR_Shift32(MX_COMB + MX_WRITE);
    DR_Shift32(0x0001);
    // write CPU stop reaction register
    DR_Shift32(MX_CPUSTOP + MX_WRITE);
    DR_Shift32(0x0001);

    return SC_ERR_NONE;
}

/**
 * @brief Reads first breakpoint register and sets it to dummy values
 * 
 * @param t 
 * @param bw 
 */
void sbw_read_breakpoint(target *t, struct breakwatch *bw)
{
    IR_Shift(IR_EMEX_DATA_EXCHANGE32);

    // Sends which BP is meant to be set
    DR_Shift32(0x0 | MX_WRITE);
    uint32_t emex_addr = DR_Shift32(0x0);

    // Sets masks of trigger - mask register
    DR_Shift32(MX_WRITE | MX_MASK);
    uint32_t emex_mask = DR_Shift32(BPMASK_DONTCARE);

    // Sets combination logic - combination register
    DR_Shift32(emex_addr | MX_WRITE | MX_COMB);
    uint32_t emex_comb = DR_Shift32(0x0);

    bw->addr = emex_addr;
    bw->type = TARGET_BREAK_HARD;
    bw->reserved[0] = 0;
}

void sbw_restore_breakpoint(target *t, struct breakwatch *bw)
{
    sbw_set_bp_hard(t, bw);
}



int swb_breakwatch_set(target *t, struct breakwatch *bw)
{
    struct msp430_priv *msp_priv = t->priv;
    unsigned            i        = 0;

    switch(bw->type)
    {
        case TARGET_BREAK_SOFT:
        case TARGET_BREAK_HARD:
            for(i = 1; i < msp_priv->hw_breakpoint_max; i++)
            {
                if((msp_priv->hw_breakpoint_mask & (1 << i)) == 0)
                    break;
            }

            if(i == msp_priv->hw_breakpoint_max)
                return -1;

            bw->reserved[0] = i;
            msp_priv->hw_breakpoint_mask |= (1 << i);

            sbw_set_bp_hard(t, bw);
            return SC_ERR_NONE;
        case TARGET_WATCH_WRITE:
        case TARGET_WATCH_READ:
        case TARGET_WATCH_ACCESS:
            // TODO: not supported atm
            break;
        default:
            break;
    }
    return SC_ERR_GENERIC;
}

int swb_breakwatch_clear(target *t, struct breakwatch *bw)
{
    struct msp430_priv *msp_priv = t->priv;
    unsigned            i        = bw->reserved[0];

    switch(bw->type)
    {
        case TARGET_BREAK_SOFT:
        case TARGET_BREAK_HARD:
            msp_priv->hw_breakpoint_mask &= ~(1 << i);
            sbw_clear_bp_hard(t, bw);
            break;
        case TARGET_WATCH_WRITE:
        case TARGET_WATCH_READ:
        case TARGET_WATCH_ACCESS:
            break;
        default:
            break;
    }
    return 0;
}