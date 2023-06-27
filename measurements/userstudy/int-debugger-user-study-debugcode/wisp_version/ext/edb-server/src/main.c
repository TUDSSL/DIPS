#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

#include <libmsp/periph.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libio/log.h>
#include <libedbserver/edb.h>
#include <libedbserver/pin_assign.h>

static inline void pin_setup()
{
    // Uncomment this if R3 is not populated since in that case pin is unconnected
    // GPIO(PORT_CONT_POWER, DIR) |= BIT(PIN_CONT_POWER);
    // GPIO(PORT_CONT_POWER, OUT) &= ~BIT(PIN_CONT_POWER);

#ifdef CONFIG_DEBUG_MODE_LED
    GPIO(PORT_LED_DEBUG_MODE, OUT) &= ~BIT(PIN_LED_DEBUG_MODE);
    GPIO(PORT_LED_DEBUG_MODE, DIR) |= BIT(PIN_LED_DEBUG_MODE);
#endif // CONFIG_DEBUG_MODE_LED

#ifdef CONFIG_MAIN_LOOP_LED
    GPIO(PORT_LED_MAIN_LOOP, OUT) &= ~BIT(PIN_LED_MAIN_LOOP);
    GPIO(PORT_LED_MAIN_LOOP, DIR) |= BIT(PIN_LED_MAIN_LOOP);
#endif // CONFIG_MAIN_LOOP_LED

#ifdef CONFIG_BOOT_LED
    GPIO(PORT_LED_BOOT, OUT) &= ~BIT(PIN_LED_BOOT);
    GPIO(PORT_LED_BOOT, DIR) |= BIT(PIN_LED_BOOT);
#endif // CONFIG_BOOT_LED
}

int main(void)
{
#ifdef CONFIG_WATCHDOG
	msp_watchdog_enable(CONFIG_WDT_BITS);
#else // !CONFIG_WATCHDOG
	msp_watchdog_disable();
#endif // !CONFIG_WATCHDOG

    pin_setup();

#ifdef CONFIG_BOOT_LED
    GPIO(PORT_LED_BOOT, OUT) |= BIT(PIN_LED_BOOT);
#endif // CONFIG_BOOT_LED

    msp_clock_setup(); // set up unified clock system

#ifdef CONFIG_CLOCK_TEST_MODE
    GPIO(PORT_LED, OUT) &= ~(BIT(PIN_LED_GREEN) | BIT(PIN_LED_RED));
    GPIO(PORT_LED, DIR) |= BIT(PIN_LED_GREEN) | BIT(PIN_LED_RED);
    BLINK_LOOP(PIN_LED_GREEN, 1000000); // to check clock configuration
#endif

#ifdef CONFIG_DEV_CONSOLE
    INIT_CONSOLE();
#endif // CONFIG_DEV_CONSOLE

    __enable_interrupt();                   // enable all interrupts

    LOG("\r\nEDB\r\n");

#ifdef CONFIG_MAIN_LOOP_LED
    unsigned main_loop_count = 0;
#endif // CONFIG_MAIN_LOOP_LED

    LOG("init EDB server\r\n");
    edb_server_init();

#ifdef CONFIG_BOOT_LED
    GPIO(PORT_LED_BOOT, OUT) &= ~BIT(PIN_LED_BOOT);
#endif // CONFIG_BOOT_LED

    while(1) {

#ifdef CONFIG_WATCHDOG
        msp_watchdog_kick();
#endif // !CONFIG_WATCHDOG

        edb_service();

#ifdef CONFIG_MAIN_LOOP_LED
        if (main_loop_count++ == ~0) {
            GPIO(PORT_LED_MAIN_LOOP, OUT) ^= BIT(PIN_LED_MAIN_LOOP);
        }
#endif

#ifdef CONFIG_SLEEP_IN_MAIN_LOOP
        LOG("sleep\r\n");
        // sleep, wait for event flag to be set, then handle it in loop
        __bis_SR_register(CONFIG_MAIN_LOOP_SLEEP_STATE + GIE);
        LOG("woke up\r\n");
#endif // CONFIG_SLEEP_IN_MAIN_LOOP
    }
}
