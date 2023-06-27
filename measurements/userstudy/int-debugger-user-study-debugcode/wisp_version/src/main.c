#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <libmsp/periph.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>
#include <libmsp/mem.h>

#include <libedb/edb.h>
#ifdef TEST_EIF_PRINTF
#include <libio/printf.h>
#endif // TEST_EIF_PRINTF

// Include the tasks
#include "tasks/blink.h"
#include "tasks/naturalSum.h"
#include "tasks/adctask.h"
#include "checkpointing.h"

// Interfaces are used to mimic the same structure for the different architectures
#include "interfaces/interface_main.h"

#define delay_time 100000000

/**
 * This is the test of the WISP device. 3 bugs has been found in this workspace.
 * Each function has a single bug to find:
 * - Blink
 * - Recursive Sum
 * - ADC Print
 *
 * Use the debugger to find the bugs, a breakpoint can be set within the code by adding the line
 *  EXTERNAL_BREAKPOINT(0); within the libedb library. the argument here is the id of the breakpoint
 *
 * use flash.sh for flashing the firmware
 * use debug.sh for starting the debug console after flashing
 */

int main() {
    init_hw();
    EIF_PRINTF("\r\nRebooted. \r\n");

    //  Bug review starts here:
    setup_blink();

    
    int task = restore();
    switch (task) {
      case 2: goto sum;
      case 3: goto spi;
      default: break;
    }

    setup_adc();

    while(1) {
      EIF_PRINTF("1: Blinking.\r\n");
      execute_blink();
      checkpoint(2);
      for (int i = 0; i < delay_time; ++i); // do not remove
  sum:
      EIF_PRINTF("2: Calculating sum.\r\n");
      execute_natural_sum();
      checkpoint(3);
      for (int i = 0; i < delay_time; ++i); // do not remove
  spi:
      EIF_PRINTF("3: Printing Temp.\r\n");
      execute_temp_print();
      checkpoint(4);
      for (int i = 0; i < delay_time; ++i);// do not remove
    }

    return 0;
}
