/*
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

/*
 * This implementation of the SBW transport layer is based on code provided
 * by TI (slau320 and slaa754). It provides the basic routines to serialize
 * the JTAG TMS, TDO and TDI signals over a two wire interface.
 */

#include "general.h"
#include "target/sbw_scan.h"
#include "target.h"
#include "target/adiv5.h"
#include "target/jtag_devs.h"
#include "target/sbw_transport.h"

#include "libopencmsis/core_cm3.h"
#include "libopencm3/cm3/dwt.h"
#include "libopencm3/stm32/timer.h"
#include <libopencm3/stm32/rcc.h>

static bool tclk_state = 0;

/*
 * Ignore the f_clk parameter and make sure that clock frequency is around 500k. This number is taken from
 * TI's slaa754 reference implementation and works reliably where other values do not work.
 * In SLAU320AJ section 2.2.3.1., the 'delay' is specified as 5 clock cycles at 18MHz, but this seems
 * to not work reliably and contradicts the reference implementation.
 */
const unsigned int clk_delay_cycles = 72000000 / 500000 / 2;

#define   nNOPS   {asm(".rept 72 ; nop ; .endr");}

static struct {
	unsigned int sbwtck;
	unsigned int sbwtdio;
} pins;

static void tmsh(void)
{
  gpio_set_val(JTAG_PORT, pins.sbwtdio, true);
  nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, false);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, true);
}

static void tmsl(void)
{
  gpio_set_val(JTAG_PORT, pins.sbwtdio, false);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, false);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, true);
}

static void tmsldh(void)
{
  gpio_set_val(JTAG_PORT, pins.sbwtdio, false);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, false);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtdio, true);
	gpio_set_val(JTAG_PORT, pins.sbwtck, true);
}

static void tdih(void)
{
	gpio_set_val(JTAG_PORT, pins.sbwtdio, true);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, false);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, true);
}

static void tdil(void)
{
	gpio_set_val(JTAG_PORT, pins.sbwtdio, false);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, false);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, true);
}

static bool tdo_rd(void)
{
	bool res;

	SWDIO_MODE_FLOAT();
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, false);
	nNOPS;
	res = gpio_get(JTAG_PORT, pins.sbwtdio);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, true);
	gpio_set_val(JTAG_PORT, pins.sbwtdio, true);
	SWDIO_MODE_DRIVE();

	return res;
}

static void tdo_sbw(void)
{
  SWDIO_MODE_FLOAT();
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, false);
	nNOPS;
	gpio_set_val(JTAG_PORT, pins.sbwtck, true);
	gpio_set_val(JTAG_PORT, pins.sbwtdio, true);
	SWDIO_MODE_DRIVE();
}

void set_sbwtdio(bool state)
{
	gpio_set_val(JTAG_PORT, pins.sbwtdio, state);
}

void set_sbwtck(bool state)
{
	gpio_set_val(JTAG_PORT, pins.sbwtck, state);
}

void tmsl_tdil(void)
{
	tmsl();
	tdil();
	tdo_sbw();
}

void tmsh_tdil(void)
{
	tmsh();
	tdil();
	tdo_sbw();
}

void tmsl_tdih(void)
{
	tmsl();
	tdih();
	tdo_sbw();
}

void tmsh_tdih(void)
{
	tmsh();
	tdih();
	tdo_sbw();
}

bool tmsl_tdih_tdo_rd(void)
{
	tmsl();
	tdih();
	return tdo_rd();
}

bool tmsl_tdil_tdo_rd(void)
{
	tmsl();
	tdil();
	return tdo_rd();
}

bool tmsh_tdih_tdo_rd(void)
{
	tmsh();
	tdih();
	return tdo_rd();
}

bool tmsh_tdil_tdo_rd(void)
{
	tmsh();
	tdil();
	return tdo_rd();
}

void clr_tclk_sbw(void)
{
	if (tclk_state == true) {
		tmsldh();
	} else {
		tmsl();
	}

	gpio_set_val(JTAG_PORT, pins.sbwtdio, false);

	tdil();
	tdo_sbw();
	tclk_state = false;
}

void set_tclk_sbw(void)
{
	if (tclk_state == true) {
		tmsldh();
	} else {
		tmsl();
	}
	gpio_set_val(JTAG_PORT, pins.sbwtdio, true);

	tdih();
	tdo_sbw();
	tclk_state = true;
}

void tclk_sbw(void)
{
    clr_tclk_sbw();
    set_tclk_sbw();
}

bool get_tclk(void)
{
	return tclk_state;
}

int sbw_transport_disconnect(void)
{
#define SWDIO_DIR_PORT  JTAG_PORT
#define SWDIO_PORT  JTAG_PORT
#define SWCLK_PORT  JTAG_PORT
#define SWDIO_DIR_PIN TMS_DIR_PIN

  gpio_set_mode(SWDIO_PORT, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_FLOAT, SWDIO_PIN);
  gpio_set_mode(SWCLK_PORT, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_FLOAT, SWCLK_PIN);

	tclk_state = false;
	return 0;
}

int sbw_transport_connect(void)
{
	gpio_set_val(SWDIO_PORT, pins.sbwtdio, true);
  gpio_set_mode(SWDIO_PORT, GPIO_MODE_OUTPUT_10_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, SWDIO_PIN);
	gpio_set_val(SWCLK_PORT, pins.sbwtck, true);
  gpio_set_mode(SWCLK_PORT, GPIO_MODE_OUTPUT_10_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, SWCLK_PIN);
  gpio_set_mode(TMS_DIR_PORT, GPIO_MODE_OUTPUT_10_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, TMS_DIR_PIN);
	gpio_set_val(TMS_DIR_PORT, TMS_DIR_PIN, true); // output

	tclk_state = false;
	return 0;
}

void DWT_Delay(uint32_t us) // microseconds
{
  uint32_t startTick = timer_get_counter(TIM4),
      delayTicks = us ;//* (72000000/1000000);
  if( startTick + delayTicks  > UINT16_MAX){
    while (timer_get_counter(TIM4) > startTick)
      ;

    delayTicks -= (UINT16_MAX-startTick);
  }
  while (timer_get_counter(TIM4) - startTick < delayTicks);
}

static void DWT_Init(void)
{
  rcc_periph_clock_enable(RCC_TIM4);
  timer_set_mode(TIM4, TIM_CR1_CKD_CK_INT,
           TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

  timer_set_prescaler(TIM4,71);
  timer_enable_counter(TIM4);
}



int sbw_transport_init(void)
{
	pins.sbwtck = SWCLK_PIN;
	pins.sbwtdio = SWDIO_PIN;

	DWT_Init();

	return 0;
}
