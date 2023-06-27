/*
 * sbw_scan.h
 *
 *  Created on: Mar 12, 2022
 *      Author: jasper
 */

#ifndef SRC_TARGET_SBW_SCAN_H_
#define SRC_TARGET_SBW_SCAN_H_

#include "target.h"

#define BP_HIT_MASK_J              0x0400000000000000ull
#define LPMX5_MASK_J               0x4000000000000000ull
#define LPM4_1MASK_J               0x8000000000000000ull
#define LPM4_2MASK_J               0x8300000000000000ull
#define EIGHT_JSTATE_BITS          0x100000000000000ull


#define JSTATE_FLOW_CONTROL_BITS   0xC7
#define JSTATE_BP_HIT              0x4
#define JSTATE_SYNC_ONGOING        0x83
#define JSTATE_LOCKED_STATE        0x40
#define JSTATE_INVALID_STATE       0x81
#define JSTATE_LPM_ONE_TWO         0x82
#define JSTATE_LPM_THREE_FOUR      0x80
#define JSTATE_VALID_CAPTURE       0x03
#define JSTATE_LPM_X_FIVE          0xC0

void sbwHaltResume(target *t);
int sbw_close(void);

#endif /* SRC_TARGET_SBW_SCAN_H_ */
