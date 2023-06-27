/**
 * This file is not part of the bug testing assignement.
 * These functions are mainly for keeping the interface consistent with the other device...
 */

#ifndef INTERFACE_WISP_DEMO_CODE_BLINK
#define INTERFACE_WISP_DEMO_CODE_BLINK

#include <stdbool.h>
#include <msp430.h>
#include <libmsp/gpio.h>

// P4.0 - LED1 OUTPUT
#define		PLED1OUT			(P4OUT)
#define 	PIN_LED1			(BIT0)
#define 	PDIR_LED1			(P4DIR)
#define   LED_BUILTIN   PIN_LED1
#define   OUTPUT        1
#define   INPUT         0

void pinMode(int pin, bool mode){
  if (mode)
    PDIR_LED1 |= pin;
  else
    PDIR_LED1 &= ~pin;
}

extern int blink_C0UNTER;

void digitalWrite(int pin, bool on){
  if (on)
    PDIR_LED1 |= pin;
  else
    PLED1OUT &= ~PIN_LED1;
}

#endif // INTERFACE_WISP_DEMO_CODE_BLINK
