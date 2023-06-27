#include "target/msp430.h"
#include "target/sbw_jtag.h"
#include "target/sbw_transport.h"

#include "target/msp430/DisableLpmx5.h"

/**
 * @brief disable Lpmx5/ulp mode
 */
void DisableLpmx5(target *t)
{
    IR_Shift(IR_TEST_3V_REG);
    DR_Shift16(0x0);
    DR_Shift16(0x40A0);

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