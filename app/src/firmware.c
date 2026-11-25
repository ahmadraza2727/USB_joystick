// #include "libopencm3/stm32/f4/gpio.h"
#include "firmware.h"
#include "libopencm3/stm32/adc.h"
#include "libopencm3/stm32/gpio.h"
#include "usb_setup.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/usbd.h>
#include <stddef.h>

static void rcc_setup(void) {
  rcc_clock_setup_pll(&my_clock_config);

  /* SysTick interrupt every N clock pulses: set reload to N-1 */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_OTGFS);
  rcc_periph_clock_enable(RCC_ADC1);
}

static void gpio_setup(void) {
  // setup the button
  gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO0);
  // setup the analog read adc
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1 | GPIO4);
  // setup the led (onboard)
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO13);
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO13);

  // default led state on startup
  gpio_clear(GPIOC, GPIO13); // Start with LED ON (if active low)

  // gpio setup for the usb peripheral
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11);
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO12);
  gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

  nvic_enable_irq(NVIC_SYSTICK_IRQ);
}

void adc_setup(void) {

  // gpio_set(GPIOC, GPIO13); // LED ON
  adc_power_off(ADC1);                  // Ensure ADC is off before config
  adc_set_single_conversion_mode(ADC1); // We’ll trigger each sequence manually
  adc_set_sample_time(ADC1, ADC_CHANNEL1, ADC_SMPR_SMP_15CYC);
  adc_set_sample_time(ADC1, ADC_CHANNEL4, ADC_SMPR_SMP_15CYC);
  adc_set_regular_sequence(ADC1, 2, (uint8_t[]){ADC_CHANNEL1, ADC_CHANNEL4});
  adc_power_on(ADC1); // Power up ADC
  uint8_t a[] = "adc setup complete";
  usb_cdc_send_report(a, sizeof(a));
}
void systick_setup(void) {

  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_set_reload(84000000 / 1000 - 1); // 1 ms tick at 48 MHz
  systick_interrupt_enable();
  systick_counter_enable();
  uint8_t a[] = "systick setup complete";
  usb_cdc_send_report(a, sizeof(a));
}

void sys_tick_handler(void) { system_millis++; }

void delay_ms(uint32_t ms) {
  uint32_t wake_time = system_millis + ms;
  while (wake_time > system_millis)
    ;
}

uint16_t read_adc(uint8_t channel) {

  adc_set_regular_sequence(ADC1, 1, &channel); // Single channel read
  adc_start_conversion_regular(ADC1);
  while (!adc_eoc(ADC1))
    ; // Wait until conversion finishes
  return adc_read_regular(ADC1);
}

#define SMOOTHING 8 // higher = smoother, slower response

int smooth(int prev, int current) {
  return prev + (current - prev) / SMOOTHING;
}
int apply_deadzone(int val, int center, int dz) {
  if (val > center - dz && val < center + dz)
    return 0;
  return val - center;
}
int main(void) {
  rcc_setup();
  gpio_setup();
  usb_hid_setup();
  while (!usb_configured) {
    __asm__("nop");
  }
  adc_setup();
  systick_setup();

  // Initial LED test (proves LED works)
  delay_ms(500);
  gpio_set(GPIOC, GPIO13); // LED ON
  delay_ms(500);
  gpio_clear(GPIOC, GPIO13); // LED OFF
  delay_ms(500);
  // --- Critical USB Disconnect Sequence ---

  bool pressed = false;
  // bool is_pressed = false;
  static int x_filtered = 0;
  static int y_filtered = 0;

  while (1) {
    usb_hid_poll();

    if (gpio_get(GPIOA, GPIO0) == 0 && !pressed) {

      uint8_t a[] = "button pressed";
      // usb_cdc_send_report(a, sizeof(a));
      DEBUG_PRINT((char*)a)
      gpio_clear(GPIOC, GPIO13);
      pressed = true;
    } else if (gpio_get(GPIOA, GPIO0) == 1 && pressed) {
      pressed = false;
      gpio_set(GPIOC, GPIO13);
    }
    else{

      gpio_set(GPIOC, GPIO13);
    }

    int raw_x = read_adc(ADC_CHANNEL1);
    int raw_y = read_adc(ADC_CHANNEL4);

    x_filtered = smooth(x_filtered, raw_x);
    y_filtered = smooth(y_filtered, raw_y);
    int dx = apply_deadzone(x_filtered, 2048, 100); // 100 = dead zone width
    dx /= 100;                                      // scale down movement

    int dy = apply_deadzone(y_filtered, 2048, 100);
    dy /= 100;

    // if (f >= g) {

    // g = f + 1000;

    static int x = 0;
    static int dir = 1;
    // buf[1] = dir;
    x += dir;
    if (x > 30)
      dir = -dir;
    if (x < -30)
      dir = -dir;

    // uint8_t buf[9] = {0x02, 0, 0, 0x04, 0, 0, 0, 0, 0};
    uint8_t buf[5] = {0x01, pressed ? 0x01 : 0x00, (uint8_t)dx, (uint8_t)dy,
                      0x00};

    usb_hid_send_report(buf, sizeof(buf));
    // uint8_t buf[5] = {0x01, 0x00, 50, 30, 0};
    // usbd_ep_write_packet(usbd_dev, 0x81, buf, 4);
    // }
  }
}
// int main(void) {
//
//   // setup clock for the mcu
//   rcc_setup();
//   // setup the systick interrupt for busy wating
//   systick_setup();
//   // setup the button and led
//   gpio_setup();
//   // setup the gpio for usb
//   usb_clock_init();
//   gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12); // DP
//   pin gpio_clear(GPIOA, GPIO12); // Force disconnect delay_ms(500);
//   gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO12); // Back to
//   USB gpio_set_af(GPIOA, GPIO_AF10, GPIO12);
//   // usb hid settings
//   usb_hid_setup();
//   // usb_hid_poll();
//   while (!usb_configured) {
//     usb_hid_poll();
//     gpio_toggle(GPIOC, GPIO13); // Fast blink while not configured
//     delay_ms(100);
//   }
//   gpio_set(GPIOC, GPIO13);
//
//   while (1) {
//     if (usb_configured) { // Only send if USB is configured
//       if (gpio_get(GPIOA, GPIO0) == 0) {
//         uint8_t report[8] = {0x00, 0x00, 0x04, 0x00,
//                              0x00, 0x00, 0x00, 0x00}; // 'A' key
//         usb_hid_send_report(report, sizeof(report));
//         gpio_toggle(GPIOC, GPIO13);
//
//         // Send release report
//         delay_ms(20); // Short delay for debounce
//         uint8_t release[8] = {0};
//         usb_hid_send_report(release, sizeof(release));
//
//         delay_ms(200); // Wait before next press
//       }
//     }
//     delay_ms(1); // Small delay to prevent busy-wait
//   }
//
// rcc_setup();
// systick_setup();
// gpio_setup();
// usb_hid_setup();
//
// while (1) {
//   gpio_set(GPIOC,GPIO13);
//   // usbd_poll(usbd_dev);
//
//   usb_hid_poll();
//   // if (usbd_is_configured(usbd_dev)) {
//   uint8_t report[8] = {0x00, 0x00, 0x04, 0x00,
//                        0x00, 0x00, 0x00, 0x00}; // 'A' key
//   if (gpio_get(GPIOA, GPIO0) == 0) {
//     // Button pressed
//     usb_hid_send_report(report, sizeof(report));
//     gpio_toggle(GPIOC, GPIO13);
//     delay_ms(500);
//   }
//   // }
// }
// }
