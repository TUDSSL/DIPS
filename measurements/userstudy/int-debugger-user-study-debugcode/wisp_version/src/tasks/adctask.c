#include "adctask.h"
#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <libmsp/periph.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>
#include <libmsp/mem.h>

#include <libwispbase/globals.h>
#include <libwispbase/spi.h>
#include <libwispbase/accel.h>
//#include <libwispbase/adc.h>

#include <libedb/edb.h>
#ifdef TEST_EIF_PRINTF
#include <libio/printf.h>
#endif // TEST_EIF_PRINTF


void setup_adc(){ // Assume this function is correct.
      REFCTL0 = REFVSEL_1 | REFTCOFF;

      __delay_cycles(5000);

      ADC12MCTL0 |= ADC12VRSEL_1;


      // Enable the  ADC
      ADC12CTL0 |= ADC12ON;
      ADC12CTL0 &= ~(0x0F00);
      ADC12CTL0 |= ADC12SHT0_4;

      ADC12CTL1 |= ADC12SHP | ADC12SSEL1;
      ADC12CTL2 |= ADC12RES__10BIT;
      ADC12CTL3 |= ADC12TCMAP;

      REFCTL0 &= ~REFTCOFF;
      ADC12MCTL0 &= ~(0x1F);
      ADC12MCTL0 |= ADC12INCH_30;


      // Select conversion sequence to single channel-single conversion. (see user guide 25.2.8)
      ADC12CTL1 |= ADC12CONSEQ_0;
}


void execute_temp_print(){ // Assume this function is correct.
    // Start
    ADC12CTL0 |= ADC12SC + ADC12ENC;

    // Wait until conversion is done.
    while (!!(ADC12CTL1 & ADC12BUSY))
        ;

    // Read
    uint16_t adc_value = ADC12MEM0;
    
    if(adc_value <= 1000){
      EIF_PRINTF("Error measuring temperature.\r\n");
      return;
    }

    char buffer [100];
    snprintf (buffer, 100,"Uncompensated Temperature:  (%d)\r\n", adc_value);

    EIF_PRINTF(buffer);
}
