#include "inttypes.h"
#include "../interfaces/interface_blink.h"

void setup_blink() {
  pinMode(LED_BUILTIN, OUTPUT);
}

bool led_on = 0;

int toggle_led()
{
  if (led_on){
    led_on = 0;
    digitalWrite(LED_BUILTIN, led_on);

  }
  else
  {
    led_on = 1;
    digitalWrite(LED_BUILTIN, led_on);
  }

  return PLED1OUT;
}

void execute_blink() {
  toggle_led();

  for(int blink_COUNTER=0; blink_C0UNTER < 10000; blink_COUNTER++);
}

