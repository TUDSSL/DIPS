
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

#include "target/msp430/pc.h"

/**
 * Read stored CPU status register
 * Note that this value can only be updated in a halt request
 *
 * @return contents of SR
 */
uint32_t SbwReadSR(target *t)
{
    // TODO retrieve value from priv
    struct msp430_priv *priv = t->priv;

    // TODO make more robust init value
    if(priv->SR == 0)
    {
        return 0x0;
    }

    return priv->SR;
}

/**
 * Read stored CPU register value
 * Note that this value can only be updated in a halt request
 *
 * @return addr of PC
 */
uint32_t SbwReadPC(target *t)
{
    // TODO retrieve value from priv
    struct msp430_priv *priv = t->priv;

    // TODO make more robust init value
    if(priv->PC == 0)
    {
        return 0x690;
    }

    return priv->PC;
}

/**
 * Loads a given address into the target CPU's program counter (PC).
 *
 * @param Addr destination address
 *
 */
void SbwSetPC_430Xv2(target *t, uint32_t Addr)
{
    struct msp430_priv *priv = t->priv;
    uint16_t            Mova;
    uint16_t            Pc_l;

    Mova = 0x0080;
    Mova += (uint16_t)((Addr >> 8) & 0x00000F00);
    Pc_l = (uint16_t)((Addr & 0xFFFF));

    // Check Full-Emulation-State at the beginning
    IR_Shift(IR_CNTRL_SIG_CAPTURE);
    if(DR_Shift16(0) & 0x0301)
    {
        // MOVA #imm20, PC
        clr_tclk_sbw();
        // take over bus control during clock LOW phase
        IR_Shift(IR_DATA_16BIT);
        set_tclk_sbw();
        DR_Shift16(Mova);
        // insert on 24.03.2010 Florian
        clr_tclk_sbw();
        // above is just for delay
        IR_Shift(IR_CNTRL_SIG_16BIT);
        DR_Shift16(0x1400);
        IR_Shift(IR_DATA_16BIT);
        tclk_sbw();
        DR_Shift16(Pc_l);
        tclk_sbw();
        DR_Shift16(0x4303);
        clr_tclk_sbw();
        IR_Shift(IR_ADDR_CAPTURE);
        DR_Shift20(0x00000);
    }

    if(t != NULL)
        priv->PC = Addr;
}