#include "checkpointing.h"

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

__nv int nv_checkpoint_counter = 0x0;

int restore(){
  return nv_checkpoint_counter;
};

void checkpoint(uint16_t taskID){
  nv_checkpoint_counter = taskID;
};
