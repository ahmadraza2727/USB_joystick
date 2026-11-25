#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/usbd.h>

// extern usbd_device *usbd_dev;
static volatile uint32_t system_millis;

void delay_ms(uint32_t ms);
void systick_setup(void);

void adc_setup(void);
uint16_t read_adc(uint8_t channel);
int smooth(int prev, int current);
int apply_deadzone(int val, int center, int dz);

const struct rcc_clock_scale my_clock_config = {
    .pllm = 8,             // HSI divider (16MHz / 8 = 2MHz)
    .plln = 96,            // PLL multiplier (2MHz * 96 = 192MHz)
    .pllp = 4,             // System clock divider (192MHz / 4 = 48MHz)
    .pllq = 4,             // USB clock divider (192MHz / 4 = 48MHz)
    .pllr = 2,             // Not used for F4, but must be set
    .pll_source = RCC_HSI, // Use HSI as PLL source
    .flash_config = FLASH_ACR_LATENCY_1WS | FLASH_ACR_PRFTEN,
    .hpre = RCC_CFGR_HPRE_DIV_NONE,  // AHB prescaler (no division)
    .ppre1 = RCC_CFGR_PPRE_DIV2,     // APB1 prescaler (48MHz/2=24MHz)
    .ppre2 = RCC_CFGR_PPRE_DIV_NONE, // APB2 prescaler (no division)
    .voltage_scale = PWR_SCALE1,     // Voltage scale for 3.3V operation
    .ahb_frequency = 48e6,           // 48MHz AHB frequency
    .apb1_frequency = 24e6,          // 24MHz APB1 frequency
    .apb2_frequency = 48e6           // 48MHz APB2 frequency
};
#endif // !FIRMWARE_H
