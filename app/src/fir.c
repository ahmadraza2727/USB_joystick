#include<stddef.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/dwc/otg_fs.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/usb/usbd.h>

__attribute__((
    aligned(4))) static const struct usb_device_descriptor dev_descr = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x0483,
    .idProduct = 0x5710,
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

// HID Report Descriptor
static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01, /* USAGE_PAGE (Generic Desktop)         */
    0x09, 0x02, /* USAGE (Mouse)                        */
    0xa1, 0x01, /* COLLECTION (Application)             */
    0x09, 0x01, /*   USAGE (Pointer)                    */
    0xa1, 0x00, /*   COLLECTION (Physical)              */
    0x05, 0x09, /*     USAGE_PAGE (Button)              */
    0x19, 0x01, /*     USAGE_MINIMUM (Button 1)         */
    0x29, 0x03, /*     USAGE_MAXIMUM (Button 3)         */
    0x15, 0x00, /*     LOGICAL_MINIMUM (0)              */
    0x25, 0x01, /*     LOGICAL_MAXIMUM (1)              */
    0x95, 0x03, /*     REPORT_COUNT (3)                 */
    0x75, 0x01, /*     REPORT_SIZE (1)                  */
    0x81, 0x02, /*     INPUT (Data,Var,Abs)             */
    0x95, 0x01, /*     REPORT_COUNT (1)                 */
    0x75, 0x05, /*     REPORT_SIZE (5)                  */
    0x81, 0x01, /*     INPUT (Cnst,Ary,Abs)             */
    0x05, 0x01, /*     USAGE_PAGE (Generic Desktop)     */
    0x09, 0x30, /*     USAGE (X)                        */
    0x09, 0x31, /*     USAGE (Y)                        */
    0x09, 0x38, /*     USAGE (Wheel)                    */
    0x15, 0x81, /*     LOGICAL_MINIMUM (-127)           */
    0x25, 0x7f, /*     LOGICAL_MAXIMUM (127)            */
    0x75, 0x08, /*     REPORT_SIZE (8)                  */
    0x95, 0x03, /*     REPORT_COUNT (3)                 */
    0x81, 0x06, /*     INPUT (Data,Var,Rel)             */
    0xc0,       /*   END_COLLECTION                     */
    0x09, 0x3c, /*   USAGE (Motion Wakeup)              */
    0x05, 0xff, /*   USAGE_PAGE (Vendor Defined Page 1) */
    0x09, 0x01, /*   USAGE (Vendor Usage 1)             */
    0x15, 0x00, /*   LOGICAL_MINIMUM (0)                */
    0x25, 0x01, /*   LOGICAL_MAXIMUM (1)                */
    0x75, 0x01, /*   REPORT_SIZE (1)                    */
    0x95, 0x02, /*   REPORT_COUNT (2)                   */
    0xb1, 0x22, /*   FEATURE (Data,Var,Abs,NPrf)        */
    0x75, 0x06, /*   REPORT_SIZE (6)                    */
    0x95, 0x01, /*   REPORT_COUNT (1)                   */
    0xb1, 0x01, /*   FEATURE (Cnst,Ary,Abs)             */
    0xc0        /* END_COLLECTION                       */
};

// HID Descriptor
static const struct {
  struct usb_hid_descriptor hid_descriptor;
  struct {
    uint8_t bReportDescriptorType;
    uint16_t wDescriptorLength;
  } __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
    .hid_descriptor =
        {
            .bLength = sizeof(hid_function),
            .bDescriptorType = USB_DT_HID,
            .bcdHID = 0x0100,
            .bCountryCode = 0,
            .bNumDescriptors = 1,
        },
    .hid_report = {
        .bReportDescriptorType = USB_DT_REPORT,
        .wDescriptorLength = sizeof(hid_report_descriptor),
    }};

// Endpoint Descriptor
const struct usb_endpoint_descriptor hid_endpoint = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x81,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 4,
    .bInterval = 0x20,
};
const struct usb_interface_descriptor hid_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_HID,
    .bInterfaceSubClass = 1, /* boot */
    .bInterfaceProtocol = 2, /* mouse */
    .iInterface = 0,

    .endpoint = &hid_endpoint,

    .extra = &hid_function,
    .extralen = sizeof(hid_function),
};

const struct usb_interface ifaces[] = {{
                                           .num_altsetting = 1,
                                           .altsetting = &hid_iface,
#ifdef INCLUDE_DFU_INTERFACE
                                       },
                                       {
                                           .num_altsetting = 1,
                                           .altsetting = &dfu_iface,
#endif
                                       }};
// Configuration Descriptor
const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0xC0,
    .bMaxPower = 0x32,

    .interface = ifaces,
};

static const char *usb_strings[] = {
    "Black Sphere Technologies",
    "HID Demo",
    "DEMO",
};

usbd_device *usbd_dev;
static uint8_t usbd_control_buffer[256];

static enum usbd_request_return_codes
hid_control_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf,
                    uint16_t *len,
                    void (**complete)(usbd_device *, struct usb_setup_data *)) {
  (void)complete;
  (void)dev;

  if ((req->bmRequestType != 0x81) ||
      (req->bRequest != USB_REQ_GET_DESCRIPTOR) || (req->wValue != 0x2200))
    return USBD_REQ_NOTSUPP;

  /* Handle the HID report descriptor. */
  *buf = (uint8_t *)hid_report_descriptor;
  *len = sizeof(hid_report_descriptor);

  return USBD_REQ_HANDLED;
}
static void hid_set_config(usbd_device *dev, uint16_t wValue) {
  (void)wValue;
  (void)dev;

  usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 4, NULL);

  usbd_register_control_callback(
      dev, USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
      USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT, hid_control_request);
}

#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/flash.h>

const struct rcc_clock_scale my_clock_config = {
    .pllm = 8,
    .plln = 96,
    .pllp = 4,
    .pllq = 4,
    .pllr = 2,
    .pll_source = RCC_HSI,
    .flash_config = FLASH_ACR_LATENCY_1WS | FLASH_ACR_PRFTEN,
    .hpre = RCC_CFGR_HPRE_DIV_NONE,
    .ppre1 = RCC_CFGR_PPRE_DIV2,
    .ppre2 = RCC_CFGR_PPRE_DIV_NONE,
    .voltage_scale = PWR_SCALE1,
    .ahb_frequency = 48e6,
    .apb1_frequency = 24e6,
    .apb2_frequency = 48e6
};

volatile uint32_t systick_count = 0;  // Renamed for clarity
volatile uint32_t next_toggle = 1000; // Toggle after 1000ms

int main(void) {
    // 1. Clock setup
    rcc_clock_setup_pll(&my_clock_config);
    
    // 2. Enable GPIO clocks
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOC);
    
    // 3. Configure LED pin
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO13);
    gpio_set_output_options(GPIOC, GPIO_OSPEED_50MHZ, GPIO_OTYPE_PP, GPIO13);
    gpio_set(GPIOC, GPIO13);  // Start with LED ON (if active low)
    
    // 4. Configure SysTick for 1ms interrupts
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(48000000 / 1000 - 1);  // Fixed for 48MHz
    systick_interrupt_enable();
    systick_counter_enable();
    
    
    // 6. USB initialization
    rcc_periph_clock_enable(RCC_OTGFS);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO12);
    gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);
    
    // Short delay before USB init
    for (volatile int i = 0; i < 500000; i++) __asm__("nop");
    
    usbd_dev = usbd_init(&otgfs_usb_driver, &dev_descr, &config, usb_strings, 3,
                        usbd_control_buffer, sizeof(usbd_control_buffer));
    OTG_FS_GCCFG |= OTG_GCCFG_NOVBUSSENS;
    usbd_register_set_config_callback(usbd_dev, hid_set_config);

    // 7. Main loop with LED toggle logic
    while (1) {
        usbd_poll(usbd_dev);
        
        if (systick_count >= next_toggle) {
            next_toggle = systick_count + 1000;
            gpio_toggle(GPIOC, GPIO13);
        }
    }
}

// SysTick handler
void sys_tick_handler(void) {
    systick_count++;
}
