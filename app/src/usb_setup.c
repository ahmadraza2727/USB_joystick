#include "usb_setup.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/dwc/otg_fs.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/usb/usbd.h>
#include <stddef.h>

bool usb_configured = false;

static usbd_device *usbd_dev;
static uint8_t usbd_control_buffer[256];
typedef struct {

  usbd_device *usbd_dev;
  uint8_t usbd_control_buffer[256];

  bool usb_configured;
} usb_configuration_data, ucd;
// USB Device Descriptor
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

    //----------------------------------------------------------------
    // Mouse (Report ID 1)
    //----------------------------------------------------------------
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x02, // USAGE (Mouse)
    0xA1, 0x01, // COLLECTION (Application)
    0x85, 0x01, //   REPORT_ID (1)

    0x09, 0x01, //   USAGE (Pointer)
    0xA1, 0x00, //   COLLECTION (Physical)

    0x05, 0x09, //     USAGE_PAGE (Button)
    0x19, 0x01, //     USAGE_MINIMUM (Button 1)
    0x29, 0x03, //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00, //     LOGICAL_MINIMUM (0)
    0x25, 0x01, //     LOGICAL_MAXIMUM (1)
    0x95, 0x03, //     REPORT_COUNT (3)
    0x75, 0x01, //     REPORT_SIZE (1)
    0x81, 0x02, //     INPUT (Data,Var,Abs) ; 3 button bits

    0x95, 0x01, //     REPORT_COUNT (1)
    0x75, 0x05, //     REPORT_SIZE (5)
    0x81, 0x01, //     INPUT (Cnst,Ary,Abs) ; 5 bit padding

    0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x09, 0x38, //     USAGE (Wheel)
    0x15, 0x81, //     LOGICAL_MINIMUM (-127)
    0x25, 0x7F, //     LOGICAL_MAXIMUM (127)
    0x75, 0x08, //     REPORT_SIZE (8)
    0x95, 0x03, //     REPORT_COUNT (3)
    0x81, 0x06, //     INPUT (Data,Var,Rel) ; X, Y, Wheel
    0xC0,       //   END_COLLECTION
    0xC0,       // END_COLLECTION (Mouse)

    //----------------------------------------------------------------
    // Keyboard (Report ID 2)
    //----------------------------------------------------------------
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xA1, 0x01, // COLLECTION (Application)
    0x85, 0x02, //   REPORT_ID (2)

    0x05, 0x07, //   USAGE_PAGE (Keyboard/Keypad)
    0x19, 0xE0, //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xE7, //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x95, 0x08, //   REPORT_COUNT (8)
    0x81, 0x02, //   INPUT (Data,Var,Abs) ; Modifier keys

    0x95, 0x01, //   REPORT_COUNT (1)
    0x75, 0x08, //   REPORT_SIZE (8)
    0x81, 0x01, //   INPUT (Cnst,Ary,Abs) ; Reserved byte

    0x05, 0x07, //   USAGE_PAGE (Keyboard/Keypad)
    0x19, 0x00, //   USAGE_MINIMUM (Reserved (0))
    0x29, 0x65, //   USAGE_MAXIMUM (Keyboard Application)
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x65, //   LOGICAL_MAXIMUM (101 keys)
    0x95, 0x06, //   REPORT_COUNT (6)
    0x75, 0x08, //   REPORT_SIZE (8)
    0x81, 0x00, //   INPUT (Data,Ary,Abs) ; Key array (6 keys)
    0xC0        // END_COLLECTION (Keyboard)

    // 0x95, 0x06, //   REPORT_COUNT (6)
    // 0x75, 0x08, //   REPORT_SIZE (8)
    // 0x15, 0x00, //   LOGICAL_MINIMUM (0)
    // 0x25, 0x65, //   LOGICAL_MAXIMUM (101 keys)
    // 0x05, 0x07, //   USAGE_PAGE (Keyboard/Keypad)
    // 0x19, 0x00, //   USAGE_MINIMUM (Reserved (0))
    // 0x29, 0x65, //   USAGE_MAXIMUM (Keyboard Application)
    // 0x81, 0x00, //   INPUT (Data,Ary,Abs) ; Key array (6 keys)
    // 0xC0        // END_COLLECTION (Keyboard)
    // 0x05, 0x01, /* USAGE_PAGE (Generic Desktop)         */
    // 0x09, 0x02, /* USAGE (Mouse)                        */
    // 0xa1, 0x01, /* COLLECTION (Application)             */
    // 0x09, 0x01, /*   USAGE (Pointer)                    */
    // 0xa1, 0x00, /*   COLLECTION (Physical)              */
    // 0x05, 0x09, /*     USAGE_PAGE (Button)              */
    // 0x19, 0x01, /*     USAGE_MINIMUM (Button 1)         */
    // 0x29, 0x03, /*     USAGE_MAXIMUM (Button 3)         */
    // 0x15, 0x00, /*     LOGICAL_MINIMUM (0)              */
    // 0x25, 0x01, /*     LOGICAL_MAXIMUM (1)              */
    // 0x95, 0x03, /*     REPORT_COUNT (3)                 */
    // 0x75, 0x01, /*     REPORT_SIZE (1)                  */
    // 0x81, 0x02, /*     INPUT (Data,Var,Abs)             */
    // 0x95, 0x01, /*     REPORT_COUNT (1)                 */
    // 0x75, 0x05, /*     REPORT_SIZE (5)                  */
    // 0x81, 0x01, /*     INPUT (Cnst,Ary,Abs)             */
    // 0x05, 0x01, /*     USAGE_PAGE (Generic Desktop)     */
    // 0x09, 0x30, /*     USAGE (X)                        */
    // 0x09, 0x31, /*     USAGE (Y)                        */
    // 0x09, 0x38, /*     USAGE (Wheel)                    */
    // 0x15, 0x81, /*     LOGICAL_MINIMUM (-127)           */
    // 0x25, 0x7f, /*     LOGICAL_MAXIMUM (127)            */
    // 0x75, 0x08, /*     REPORT_SIZE (8)                  */
    // 0x95, 0x03, /*     REPORT_COUNT (3)                 */
    // 0x81, 0x06, /*     INPUT (Data,Var,Rel)             */
    // 0xc0,       /*   END_COLLECTION                     */
    // 0x09, 0x3c, /*   USAGE (Motion Wakeup)              */
    // 0x05, 0xff, /*   USAGE_PAGE (Vendor Defined Page 1) */
    // 0x09, 0x01, /*   USAGE (Vendor Usage 1)             */
    // 0x15, 0x00, /*   LOGICAL_MINIMUM (0)                */
    // 0x25, 0x01, /*   LOGICAL_MAXIMUM (1)                */
    // 0x75, 0x01, /*   REPORT_SIZE (1)                    */
    // 0x95, 0x02, /*   REPORT_COUNT (2)                   */
    // 0xb1, 0x22, /*   FEATURE (Data,Var,Abs,NPrf)        */
    // 0x75, 0x06, /*   REPORT_SIZE (6)                    */
    // 0x95, 0x01, /*   REPORT_COUNT (1)                   */
    // 0xb1, 0x01, /*   FEATURE (Cnst,Ary,Abs)             */
    // 0xc0        /* END_COLLECTION                       */
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
    // .wMaxPacketSize = 4,
    .wMaxPacketSize = 16,
    .bInterval = 0x20,
};

// CDC
static const struct usb_endpoint_descriptor comm_endp[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x83,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 16,
    .bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x01,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = 64,
        .bInterval = 1,
    },
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x82,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = 64,
        .bInterval = 1,
    }};
static const struct {
  struct usb_cdc_header_descriptor header;
  struct usb_cdc_call_management_descriptor call_mgmt;
  struct usb_cdc_acm_descriptor acm;
  struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
    .header =
        {
            .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
            .bDescriptorType = CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
            .bcdCDC = 0x0110,
        },
    .call_mgmt =
        {
            .bFunctionLength =
                sizeof(struct usb_cdc_call_management_descriptor),
            .bDescriptorType = CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
            .bmCapabilities = 0,
            .bDataInterface = 2,
        },
    .acm =
        {
            .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
            .bDescriptorType = CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_TYPE_ACM,
            .bmCapabilities = 0,
        },
    .cdc_union =
        {
            .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
            .bDescriptorType = CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_TYPE_UNION,
            .bControlInterface = 1,
            .bSubordinateInterface0 = 2,
        },
};
static const struct usb_interface_descriptor comm_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
    .iInterface = 0,

    .endpoint = comm_endp,

    .extra = &cdcacm_functional_descriptors,
    .extralen = sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 2,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,

    .endpoint = data_endp,
}};

// static const struct usb_interface ifaces[] = {};

// HID
// Interface Descriptor
const struct usb_interface_descriptor hid_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_HID,
    // .bInterfaceSubClass = 1, /* boot */
    // .bInterfaceProtocol = 2, /* mouse */
    .bInterfaceSubClass = 0, /* boot */
    .bInterfaceProtocol = 0, /* mouse */
    .iInterface = 0,

    .endpoint = &hid_endpoint,

    .extra = &hid_function,
    .extralen = sizeof(hid_function),
};

const struct usb_interface ifaces[] = {{
                                           .num_altsetting = 1,
                                           .altsetting = &hid_iface,
                                       },
                                       {
                                           .num_altsetting = 1,
                                           .altsetting = comm_iface,
                                       },
                                       {
                                           .num_altsetting = 1,
                                           .altsetting = data_iface,
                                       }};

// Configuration Descriptor
const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 3,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0xC0,
    .bMaxPower = 0x32,

    .interface = ifaces,
};

static const char *usb_strings[] = {
    "Ahmad Usb Controller",
    "Aim Controller",
    "phase:1",
};

// CDC

static enum usbd_request_return_codes cdcacm_control_request(
    usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
    void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
  (void)complete;
  (void)buf;
  (void)dev;

  switch (req->bRequest) {
  case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
    /*
     * This Linux cdc_acm driver requires this to be implemented
     * even though it's optional in the CDC spec, and we don't
     * advertise it in the ACM functional descriptor.
     */
    // char local_buf[10];
    // struct usb_cdc_notification *notif = (void *)local_buf;

    /* We echo signals back to host as notification. */
    // notif->bmRequestType = 0xA1;
    // notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
    // notif->wValue = 0;
    // notif->wIndex = 0;
    // notif->wLength = 2;
    // local_buf[8] = req->wValue & 3;
    // local_buf[9] = 0;
    // usbd_ep_write_packet(usbd_dev,0x83, buf, 10);
    return USBD_REQ_HANDLED;
  }
  case USB_CDC_REQ_SET_LINE_CODING:
    if (*len < sizeof(struct usb_cdc_line_coding))
      return USBD_REQ_NOTSUPP;
    return USBD_REQ_HANDLED;
  }
  return USBD_REQ_NOTSUPP;
}

static void cdcacm_data_rx_cb(usbd_device *dev, uint8_t ep) {
  (void)ep;
  (void)dev;

  char buf[64];
  int len = usbd_ep_read_packet(dev, 0x01, buf, 64);

  if (len) {
    gpio_clear(GPIOC, GPIO13); // LED OFF
    for (volatile int i = 0; i < 500000; i++)
      __asm__("nop");

    gpio_set(GPIOC, GPIO13); // LED ON
    buf[len] = '\r';
    buf[len + 1] = '\n';
    // buf[0] = len;
    usbd_ep_write_packet(dev, 0x82, buf, len + 2);
  }
}
// static void cdcacm_set_config(usbd_device *dev, uint16_t wValue) {
//   (void)wValue;
//   (void)dev;

// usbd_ep_setup(dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
// usbd_ep_setup(dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
// usbd_ep_setup(dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);
//
// usbd_register_control_callback(
//     dev, USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
//     USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT, cdcacm_control_request);
// }

// HID

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
  // CDC

  usbd_ep_setup(dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
  usbd_ep_setup(dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
  usbd_ep_setup(dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

  usbd_register_control_callback(
      dev, USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
      USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT, cdcacm_control_request);

  // HOD

  usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

  usbd_register_control_callback(
      dev, USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
      USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT, hid_control_request);
}

void usb_hid_setup(void) {

  for (unsigned i = 0; i < 800000; i++) {
    __asm__("nop");
  }

  usbd_dev = usbd_init(&otgfs_usb_driver, &dev_descr, &config, usb_strings, 3,
                       usbd_control_buffer, sizeof(usbd_control_buffer));

  OTG_FS_GCCFG |= OTG_GCCFG_NOVBUSSENS;

  usbd_register_set_config_callback(usbd_dev, hid_set_config);
  usb_configured = true;
}
void usbd_close(bool disconnected) { usbd_disconnect(usbd_dev, disconnected); }

void usb_hid_poll(void) { usbd_poll(usbd_dev); }

uint16_t usb_cdc_rcv_report(uint8_t *data) {
  return usbd_ep_read_packet(usbd_dev, 0x01, data, 64);
}
void debug_print(char *string) {
  uint16_t s = 0;
  char *p = string;
  while (*p != '\0') {
    s++;
    p++;
  }
  usb_cdc_send_report((uint8_t *)string, s);
}
void usb_cdc_send_report(uint8_t *data, uint16_t len) {
  data[len + 0] = '\r';
  data[len + 1] = '\n';
  data[len + 2] = '\0';
  usbd_ep_write_packet(usbd_dev, 0x82, data, len + 3);
  // data[len] = '\r';
  // data[len] = '2';
  // data[len + 1] = '1';
  // making a new line character
  // usbd_ep_write_packet(usbd_dev, 0x82, buf, 2);
}
void usb_hid_send_report(const uint8_t *data, uint16_t len) {

  usbd_ep_write_packet(usbd_dev, 0x81, data, len);
}
// void usb_hid_poll(void) { usbd_poll(usbd_dev); }
//
// void usb_hid_send_report(const uint8_t *data, uint16_t len) {
//   if (usb_configured) {
//     usbd_ep_write_packet(usbd_dev, 0x81, data, len);
//   }
// }

// /* usb_hid.cpp - Custom HID example for STM32F411 + libopencm3 */
//
//
// #include "usb_hid.h"
// // #include <cstddef>
// // #include <cstdint>
//
// extern "C" {
// #include <libopencm3/stm32/rcc.h>
// #include <libopencm3/stm32/gpio.h>
// #include <libopencm3/usb/dwc/otg_fs.h>
// #include <libopencm3/usb/hid.h>
// #include <libopencm3/usb/usbd.h>
// #include <libopencm3/usb/usbstd.h>
// }
//
// // Rest of your existing usb_hid.cpp...
// /* ---------------- Device Descriptor ---------------- */
// static const usb_device_descriptor dev_descr = {.bLength =
// USB_DT_DEVICE_SIZE,
//                                                 .bDescriptorType =
//                                                     USB_DT_DEVICE,
//                                                 .bcdUSB = 0x0200,
//                                                 .bDeviceClass = 0x00,
//                                                 .bDeviceSubClass = 0x00,
//                                                 .bDeviceProtocol = 0x00,
//                                                 .bMaxPacketSize0 = 64,
//                                                 .idVendor = 0x1209,
//                                                 .idProduct = 0x0001,
//                                                 .bcdDevice = 0x0100,
//                                                 .iManufacturer = 1,
//                                                 .iProduct = 2,
//                                                 .iSerialNumber = 3,
//                                                 .bNumConfigurations = 1};
//
// /* ---------------- HID Report Descriptor ---------------- */
// static const uint8_t hid_report_descriptor[] = {
//     0x05, 0x01, // Usage Page (Generic Desktop)
//     0x09, 0x06, // Usage (Keyboard)
//     0xA1, 0x01, // Collection (Application)
//     0x05, 0x07, //   Usage Page (Key Codes)
//     0x19, 0xE0, //   Usage Minimum (Left Control)
//     0x29, 0xE7, //   Usage Maximum (Right GUI)
//     0x15, 0x00, //   Logical Minimum (0)
//     0x25, 0x01, //   Logical Maximum (1)
//     0x75, 0x01, //   Report Size (1)
//     0x95, 0x08, //   Report Count (8)
//     0x81, 0x02, //   Input (Data, Variable, Absolute) ; Modifier byte
//     0x95, 0x01, //   Report Count (1)
//     0x75, 0x08, //   Report Size (8)
//     0x81, 0x01, //   Input (Constant) ; Reserved byte
//     0x95, 0x06, //   Report Count (6)
//     0x75, 0x08, //   Report Size (8)
//     0x15, 0x00, //   Logical Minimum (0)
//     0x25, 0x65, //   Logical Maximum (101)
//     0x05, 0x07, //   Usage Page (Key codes)
//     0x19, 0x00, //   Usage Minimum (0)
//     0x29, 0x65, //   Usage Maximum (101)
//     0x81, 0x00, //   Input (Data, Array) ; Key array (6 bytes)
//     0xC0};
//
// /* ---------------- HID Descriptor + Report Reference ---------------- */
// static const struct {
//   struct usb_hid_descriptor hid_descriptor;
//   struct {
//     uint8_t bReportDescriptorType;
//     uint16_t wDescriptorLength;
//   } __attribute__((packed)) report;
// } __attribute__((packed)) hid_function = {
//     .hid_descriptor = {.bLength = sizeof(usb_hid_descriptor),
//                        .bDescriptorType = USB_DT_HID,
//                        .bcdHID = 0x0100,
//                        .bCountryCode = 0x00,
//                        .bNumDescriptors = 1},
//     .report = {.bReportDescriptorType = USB_DT_REPORT,
//                .wDescriptorLength = sizeof(hid_report_descriptor)}};
//
// /* ---------------- Endpoints ---------------- */
// static const usb_endpoint_descriptor hid_endp[] = {
//     {.bLength = USB_DT_ENDPOINT_SIZE,
//      .bDescriptorType = USB_DT_ENDPOINT,
//      .bEndpointAddress = USB_ENDPOINT_ADDR_IN(1),
//      .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
//      .wMaxPacketSize = 8,
//      .bInterval = 10}};
//
// /* ---------------- Interface Descriptor ---------------- */
// static const usb_interface_descriptor hid_iface[] = {
//     {.bLength = USB_DT_INTERFACE_SIZE,
//      .bDescriptorType = USB_DT_INTERFACE,
//      .bInterfaceNumber = 0,
//      .bAlternateSetting = 0,
//      .bNumEndpoints = 1,
//      .bInterfaceClass = USB_CLASS_HID,
//      .bInterfaceSubClass = 1,
//      .bInterfaceProtocol = 1,
//      .iInterface = 0,
//      .endpoint = hid_endp,
//      .extra = &hid_function,
//      .extralen = sizeof(hid_function)}};
//
// /* ---------------- Interface Collection ---------------- */
// static const usb_interface ifaces[] = {
//     {.num_altsetting = 1, .altsetting = hid_iface}};
//
// /* ---------------- Configuration Descriptor ---------------- */
// static const usb_config_descriptor config = {
//     .bLength = USB_DT_CONFIGURATION_SIZE,
//     .bDescriptorType = USB_DT_CONFIGURATION,
//     .wTotalLength = 0, // auto-filled
//     .bNumInterfaces = 1,
//     .bConfigurationValue = 1,
//     .iConfiguration = 0,
//     .bmAttributes = 0x80,
//     .bMaxPower = 50,
//     .interface = ifaces};
//
// /* ---------------- Strings ---------------- */
// static const char *usb_strings[] = {"My Manufacturer", "Custom HID Device",
//                                     "123456"};
//
// /* ---------------- USB State ---------------- */
// static usbd_device *usb_dev;
// static uint8_t usbd_control_buffer[128];
//
// /* ---------------- Callbacks ---------------- */
// static void hid_set_config_cb(usbd_device *dev, uint16_t wValue) {
//   (void)wValue;
//   usbd_ep_setup(dev, USB_ENDPOINT_ADDR_IN(1), USB_ENDPOINT_ATTR_INTERRUPT,
//   8,
//                 NULL);
// }
//
// static enum usbd_request_return_codes hid_control_request(
//     usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t
//     *len, void (**complete)(usbd_device *dev, struct usb_setup_data *req))
//     {
//   (void)complete;
//   (void)dev;
//
//   if ((req->bmRequestType == 0x81) && // IN, standard, interface
//       (req->bRequest == USB_REQ_GET_DESCRIPTOR)) {
//
//     if ((req->wValue >> 8) == USB_DT_REPORT) {
//       *buf = (uint8_t *)hid_report_descriptor;
//       *len = sizeof(hid_report_descriptor);
//       return USBD_REQ_HANDLED;
//     } else if ((req->wValue >> 8) == USB_DT_HID) {
//       *buf = (uint8_t *)&hid_function;
//       *len = sizeof(hid_function);
//       return USBD_REQ_HANDLED;
//     }
//   }
//
//   return USBD_REQ_NOTSUPP;
// }
//
// /* ---------------- Public API ---------------- */
// void usb_hid_setup(void) {
//   usb_dev = usbd_init(&otgfs_usb_driver, &dev_descr, &config, usb_strings,
//   3,
//                       usbd_control_buffer, sizeof(usbd_control_buffer));
//
//   usbd_register_control_callback(
//       usb_dev, USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
//       USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT, hid_control_request);
//
//   usbd_register_set_config_callback(usb_dev, hid_set_config_cb);
// }
//
// void usb_hid_poll(void) { usbd_poll(usb_dev); }
//
// void usb_hid_send_report(const uint8_t *data, uint16_t len) {
//   usbd_ep_write_packet(usb_dev, USB_ENDPOINT_ADDR_IN(1), data, len);
// }
