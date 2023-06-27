//
// Created by tomh on 11-6-22.
//

#ifndef USERTESTING_WISP_BASE_INTERFACE_MAIN_H
#define USERTESTING_WISP_BASE_INTERFACE_MAIN_H


#include "pins.h"

static void init_hw()
{
  msp_watchdog_disable();
  msp_gpio_unlock();
  msp_clock_setup();

#ifdef CONFIG_EDB
  edb_init();
#endif
}

#endif //USERTESTING_WISP_BASE_INTERFACE_MAIN_H
