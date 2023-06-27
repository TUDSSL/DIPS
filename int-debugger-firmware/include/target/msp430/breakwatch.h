#ifndef SRC_TARGET_MSP430_BREAKWATCH_H_
#define SRC_TARGET_MSP430_BREAKWATCH_H_

#include "target/msp430/common.h"

int swb_breakwatch_set(target *t, struct breakwatch *bw);
int swb_breakwatch_clear(target *t, struct breakwatch *bw);
int sbw_break_single_step(target *t);
void sbw_read_breakpoint(target *t, struct breakwatch *bw);
void sbw_restore_breakpoint(target *t, struct breakwatch *bw);

#endif  // SRC_TARGET_MSP430_BREAKWATCH_H_