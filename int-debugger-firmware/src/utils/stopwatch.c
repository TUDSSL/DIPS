#include "utils/stopwatch.h"

#include "libopencmsis/core_cm3.h"
#include "libopencm3/cm3/dwt.h"
#include "libopencm3/stm32/timer.h"
#include <libopencm3/stm32/rcc.h>
#include <stdint.h>
#include <libopencm3/cm3/nvic.h>
#include "target/sbw_transport.h"
#include "spi.h"

uint64_t combined_timer_reg_value = 0;
uint64_t stopwatch_time_ms = 0;
uint32_t largest_time = 0;
uint32_t smallest_time = 0;
uint32_t cycles = 0;
float timerIntervalNs;

// Function to reset the timer
void timer_reset(void) {
    TIM1_CNT = 0;
    TIM2_CNT = 0;
}

// Function to get the current prescaler value for the timer
uint32_t timer_get_prescaler(void) {
    return TIM2_PSC;
}

void init_stopwatch(void){
    // Enable Timer 1 clock
    rcc_periph_clock_enable(RCC_TIM1);
    rcc_periph_clock_enable(RCC_TIM2);

    timer_set_counter(TIM2, 0);
    timer_set_counter(TIM1, 0);

    // Set the mode for TIM1 to count up in gated mode
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP | TIM_CR1_CEN);

    // Set the mode for TIM2 to count up continuously
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    // Enable the update event for TIM1 (slave) and set it as the trigger for TIM2 (master)
    TIM2_SMCR |= TIM_SMCR_MSM; // Set master/slave mode
    timer_slave_set_mode(TIM1, TIM_SMCR_SMS_GM); // set gated mode
    timer_slave_set_trigger(TIM1, TIM_SMCR_TS_ITR1);


    timer_set_master_mode(TIM2, TIM_CR2_MMS_UPDATE);
    timer_enable_update_event(TIM2);
    timer_update_on_overflow(TIM2);

    // prescaler value 0: timer interval = 13.8 ns, max duration = 59 s
    timer_set_prescaler(TIM2, 0);

    // Enable Timer 1
    sbw_transport_init();

    // Calculate the timer interval in nanoseconds
    uint32_t prescalerValueFast = timer_get_prescaler();
    uint32_t timerFrequency = 72000000 / (prescalerValueFast + 1);
    timerIntervalNs = (float) 1e9 / timerFrequency;
}

void halt_stopwatch(){
    timer_disable_counter(TIM2);
    ++cycles;
    end_stopwatch();
}

void resume_stopwatch(){
    timer_enable_counter(TIM2);
}

void start_stopwatch(void){
    timer_reset();
    stopwatch_time_ms = 0;
    largest_time = 0;
    smallest_time = 0;
    cycles = 0;
    combined_timer_reg_value = 0;
}

uint16_t end_stopwatch(void){
    // Make a 32 bit value from the two 16 bit values
    uint16_t counterValueFast = timer_get_counter(TIM2);
    uint16_t counterValueSlow = timer_get_counter(TIM1);
    uint32_t counterValueCombined = (uint64_t) counterValueSlow << 16 | counterValueFast;

    // Calculate some statistics
    uint32_t time_relative = counterValueCombined - combined_timer_reg_value;

    if(time_relative > largest_time){
        largest_time = time_relative;
    }
    if(time_relative < smallest_time || smallest_time == 0){
        smallest_time = time_relative;
    }

    combined_timer_reg_value = counterValueCombined;
    stopwatch_time_ms = ((uint64_t) counterValueCombined) *  timerIntervalNs;
    return stopwatch_time_ms;
}
