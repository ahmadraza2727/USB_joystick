#ifndef USB_SETUP_H
#define USB_SETUP_H

// #include "libopencm3/usb/usbd.h"
#include <stdbool.h>
#include <stdint.h>
// Add to usb_hid.h:
extern bool usb_configured;

// extern struct  usbd_device;
void usbd_close(bool disconnected);
bool usb_connected(void);
void usb_hid_setup(void);
void usb_hid_poll(void);
void usb_hid_send_report(const uint8_t *data, uint16_t len);

uint16_t usb_cdc_rcv_report(uint8_t *data);
void usb_cdc_send_report( uint8_t *data, uint16_t len);

void debug_print( char * string);

// If DEBUG is defined, enable debug prints
#ifdef DEBUG
    #define DEBUG_PRINT(str) \
        debug_print(str);
#else
    #define DEBUG_PRINT(str) 

#endif
#endif
