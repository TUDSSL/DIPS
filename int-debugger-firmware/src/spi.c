#include "spi.h"

#include "platforms/native/platform.h"
#include <libopencm3/stm32/gpio.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/nvic.h>

#include "gdb_main.h"

bool pre_break_eg = false;
uint16_t pre_break_mv = 0;

void spi_init(void) {
    // Enable clock on pins and rcc
    rcc_clock_setup_in_hse_12mhz_out_72mhz();
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_SPI2);

    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  SPI_MOSI_PIN | SPI_SS_PIN | SPI_SCK_PIN);
    gpio_set_mode(SPI_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, SPI_MISO_PIN);

    spi_reset(SPI2);
    
    rcc_periph_reset_pulse(RST_SPI2);

    // peripheral clock APB2 = 36 Mhz
    // 36/32 = 1.125 Mhz
    spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_32, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

    spi_enable_software_slave_management(SPI2);
    spi_set_nss_high(SPI2);

    spi_enable(SPI2);
}

static uint16_t spi_transmit(uint8_t *data){
    uint16_t rx_value;

    for(int i = 0; i < 4; i++){
        spi_send(SPI2, data[i]);
    }

    return 0x0;
}

void spi_send_pause(bool pause){
    if (pause)
        enable_supply();
    else
        disable_supply();
}

void setEnergyGuard(bool preBP){
   // includes safety margin of 50 mV to prevent unwanted power flow
   uint16_t mv;
   if(preBP){
    mv = pre_break_mv + 50;
   } else {
    mv = platform_dut_mv() + 50; 
   }
   uint8_t mv1 = mv & 0xff;
   uint8_t mv2 = (mv >> 8);
   
   uint8_t data[] = {SUPPLY_MODE_ENERGY_THRESHOLD, SUPPLY_PARAM_SET, mv1, mv2};
   spi_transmit(data);
}

void clearEnergyGuard(void){
   uint8_t data[] = {SUPPLY_MODE_ENERGY_THRESHOLD, SUPPLY_PARAM_CLEAR, 0xFF, 0xFF};
   spi_transmit(data);
}

bool paused = false;

void enable_supply(void){
    // Retrieve voltage of DUT before setting BP
    if(m_debugger_mode != MODE_PROFILE){;
        pre_break_mv = platform_dut_mv();
    }

    gpio_set(P_INT_PORT, P_INT_PIN);
    paused = true;
}

void disable_supply(void){
    gpio_clear(P_INT_PORT, P_INT_PIN);
    paused = false;
}

void profile_start(void){
    gpio_set(DEBUGGER_PINS_PORT, DEBUGGER_TX_PIN);
}

void profile_stop(void){
    gpio_clear(DEBUGGER_PINS_PORT, DEBUGGER_TX_PIN);
}

 
void profile_check_for_boot(void){
    if(m_debugger_mode != MODE_PROFILE) return;
    return; //todo
    if(!(DEBUGGER_PINS_PORT & DEBUGGER_TX_PIN)){
        // DUT is still getting under JTAG control
        enable_supply();
    }
}
