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
#include "target/target_internal.h"

#include "target/msp430/step.h"
#include "target/msp430/breakwatch.h"
#include "target/msp430/halt_resume.h"

void SbwStep(target *t)
{
    // cache breakpoint settings
    struct breakwatch bw;
    sbw_read_breakpoint(t, &bw);
    // set Single Step Trigger
    sbw_break_single_step(t);

    // Release device
    sbwHaltResume(t);

    while(t->halt_poll(t, 0x0) == TARGET_HALT_RUNNING){
        // wait for device to run
    }

    sbw_restore_breakpoint(t, &bw);
}