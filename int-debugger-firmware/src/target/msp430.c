#include "target/sbw_jtag.h"
#include "target/sbw_transport.h"
#include "target/target_internal.h"

#include "gdb_packet.h"
#include "gdb_main.h"

#include "target/EEM_defs.h"

#include "general.h"
#include "target/sbw_scan.h"
#include "target.h"
#include "target/adiv5.h"
#include "target/jtag_devs.h"
#include "exception.h"
#include "target/msp430.h"

#include "target/msp430/registers.h"
#include "target/msp430/pc.h"
#include "target/msp430/breakwatch.h"
#include "target/msp430/memory.h"
#include "target/msp430/halt_poll.h"
#include "target/msp430/halt_request.h"
#include "target/msp430/halt_resume.h"
#include "target/msp430/step.h"

#define MSP430_MAX_WATCHPOINTS 3

// Following define is use to let arm-gdb work with the msp430
// Note that it is not a real workaround, but just a hack to make it work
// #define GDB_ARM_WORKAROUND

#ifdef GDB_ARM_WORKAROUND
static const char tdesc_msp430[] = "<?xml version=\"1.0\"?>"
                                   "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
                                   "<target>"
                                   "  <architecture>arm</architecture>"
                                   "  <feature name=\"org.gnu.gdb.arm.m-profile\">"
                                   "    <reg name=\"r0\" bitsize=\"32\"/>"
                                   "    <reg name=\"r1\" bitsize=\"32\"/>"
                                   "    <reg name=\"r2\" bitsize=\"32\"/>"
                                   "    <reg name=\"r3\" bitsize=\"32\"/>"
                                   "    <reg name=\"r4\" bitsize=\"32\"/>"
                                   "    <reg name=\"r5\" bitsize=\"32\"/>"
                                   "    <reg name=\"r6\" bitsize=\"32\"/>"
                                   "    <reg name=\"r7\" bitsize=\"32\"/>"
                                   "    <reg name=\"r8\" bitsize=\"32\"/>"
                                   "    <reg name=\"r9\" bitsize=\"32\"/>"
                                   "    <reg name=\"r10\" bitsize=\"32\"/>"
                                   "    <reg name=\"r11\" bitsize=\"32\"/>"
                                   "    <reg name=\"r12\" bitsize=\"32\"/>"
                                   "    <reg name=\"r13\" bitsize=\"32\"/>"
                                   "    <reg name=\"r14\" bitsize=\"32\"/>"
                                   "    <reg name=\"r15\" bitsize=\"32\"/>"
                                   "    <reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>"
                                   "    <reg name=\"lr\" bitsize=\"32\" type=\"code_ptr\"/>"
                                   "    <reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>"
                                   "    <reg name=\"xpsr\" bitsize=\"32\"/>"
                                   "    <reg name=\"msp\" bitsize=\"32\" save-restore=\"no\" type=\"data_ptr\"/>"
                                   "    <reg name=\"psp\" bitsize=\"32\" save-restore=\"no\" type=\"data_ptr\"/>"
                                   "    <reg name=\"primask\" bitsize=\"8\" save-restore=\"no\"/>"
                                   "    <reg name=\"basepri\" bitsize=\"8\" save-restore=\"no\"/>"
                                   "    <reg name=\"faultmask\" bitsize=\"8\" save-restore=\"no\"/>"
                                   "    <reg name=\"control\" bitsize=\"8\" save-restore=\"no\"/>"
                                   "  </feature>"
                                   "</target>";

/* Register number tables */
static const uint32_t regnum_msp430[] = {
    0,    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, /* standard r0-r15 */
    0x10,                                                    /* xpsr */
    0x11,                                                    /* msp */
    0x12,                                                    /* psp */
    0x14                                                     /* special */
};

#endif

static void msp430_priv_free(void *priv)
{
    // adiv5_ap_unref(((struct cortexm_priv *)priv)->ap);
    free(priv);
}

static bool msp430_attach(target *t)
{
    struct msp430_priv *priv = t->priv;
    unsigned            i;

#ifdef GDB_ARM_WORKAROUND
    t->tdesc = tdesc_msp430;
#endif

    // /* Clear any pending fault condition */
    target_check_error(t);

    // target_halt_request(t);

    // check if it is in halt

    /* Clear any stale breakpoints */
    for(i = 0; i < priv->hw_breakpoint_max; i++)
    {
        // TODO Actually clear it from mem
    }
    priv->hw_breakpoint_mask = 0;
    priv->hw_breakpoint_max  = 3;
    priv->hw_watchpoint_max  = 0;
    priv->hw_watchpoint_mask = 0;

    return true;
}

static void msp430_detach(target *t)
{
    struct msp430_priv *priv = t->priv;
}

/**
 * For compatibility reasons with gdb, some registers are duplicated
 *
 * @param reg register number
 * @param value register value pointer
 */
static uint8_t registerMapper(uint8_t registerNumber)
{
    switch(registerNumber)
    {
        case SP_DUP_REG:
            return SP_REG;
        case PC_DUP_REG:
            return PC_REG;
        default:
            return registerNumber;
    }
}

static void msp430_regs_read(target *t, void *data)
{
    uint32_t *regs = data;
    SbwReadCpuRegsXv2(t, regs);
}

static void msp430_regs_write(target *t, const void *data)
{
    uint32_t *regs = data;
    SbwWriteCpuRegsXv2(t, regs);
}

static ssize_t msp430_reg_read(target *t, int reg, void *data, size_t max)
{
    uint32_t *regsValue = data;
    SbwReadCpuRegXv2(t, registerMapper(reg), regsValue);
    return sizeof(uint32_t);
}

static ssize_t msp430_reg_write(target *t, int reg, const void *data, size_t size)
{
    uint32_t *regsValue = data;
    SbwWriteCpuRegXv2(t, registerMapper(reg), *regsValue);
    return sizeof(uint32_t);
}

static void msp430_reset(target *t)
{
    // not implemented
}

static void msp430_halt_request(target *t)
{
    sbwHaltRequest(t);
}

static enum target_halt_reason msp430_halt_poll(target *t, target_addr_t *watch)
{
    return sbwHaltPoll(t);
}

static void msp430_halt_resume(target *t, bool step)
{
    struct msp430_priv *msp_priv = t->priv;

    if(step )
    {
        SbwStep(t);
    }
    else
    {
        DWT_Delay(400);
        // enable breakpoints
        // not documentated in TI documentations
        {
            IR_Shift(IR_EMEX_DATA_EXCHANGE32);

            for(int i = 0; i < 4; i++)
            {
                DR_Shift32(IR_EMEX_DATA_EXCHANGE32);
                DR_Shift32(0x45);
            }
        }

        sbwHaltResume(t);
    }
}

static int msp430_breakwatch_set(target *t, struct breakwatch *bw)
{
    return swb_breakwatch_set(t, bw);
}

static int msp430_breakwatch_clear(target *t, struct breakwatch *bw)
{
    return swb_breakwatch_clear(t, bw);
}

static bool msp430_check_error(target *t)
{
    struct msp430_priv *priv = t->priv;
    bool                err  = priv->mmu_fault;
    priv->mmu_fault          = false;
    return err;
}
bool msp430_probe(dev_dsc_t *dsc)
{
    target *t;

    t = target_new();
    if(!t)
    {
        return false;
    }

    struct msp430_priv *priv = calloc(1, sizeof(*priv));
    if(!priv)
    { /* calloc failed: heap exhaustion */
        DEBUG_WARN("calloc: failed in %s\n", __func__);
        return false;
    }

    priv->WDT = dsc->start_wdt;
    priv->PC  = dsc->start_pc;
    priv->SR  = 0;

    // TODO make it dynamic
    priv->jtagID      = 0x99;
    priv->jtagIDIsXv2 = true;

    t->priv      = priv;
    t->priv_free = msp430_priv_free;
    t->mem_read  = sbw_mem_read;
    t->mem_write = sbw_mem_write;

    // TODO support other mcu's
    priv->hw_breakpoint_max = 3;
    priv->hw_watchpoint_max = 3;

    t->check_error = msp430_check_error;

    t->driver = msp430_driver_str;

    t->attach = msp430_attach;
    t->detach = msp430_detach;

    t->regs_read  = msp430_regs_read;
    t->regs_write = msp430_regs_write;
    t->reg_read   = msp430_reg_read;
    t->reg_write  = msp430_reg_write;

    t->reset        = msp430_reset;
    t->halt_request = msp430_halt_request;
    t->halt_poll    = msp430_halt_poll;
    t->halt_resume  = msp430_halt_resume;

#ifdef GDB_ARM_WORKAROUND
    t->regs_size = sizeof(regnum_msp430);
#else
    t->regs_size = 64;
#endif

    t->breakwatch_set   = msp430_breakwatch_set;
    t->breakwatch_clear = msp430_breakwatch_clear;


    m_current_architecture = MSP_ARCH;

    return true;
}