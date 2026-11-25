# USB HID Joystick

A USB joystick project built around the **STM32F4** microcontroller
using the **libopencm3** firmware library.

The project explores USB device development, HID joystick reports, USB
descriptors, and CDC-ACM serial communication for debugging.

## Features

-   USB HID joystick functionality using the STM32F4 USB peripheral.
-   HID report-based input communication with a connected host.
-   USB CDC-ACM serial interface for firmware debugging and device
    monitoring.
-   Practical work with USB enumeration, descriptors, interfaces,
    endpoints, and control transfers.
-   Firmware developed in C/C++ with libopencm3.

## Hardware and Software

  Component               Details
  ----------------------- ------------
  Microcontroller         STM32F4
  Firmware library        libopencm3
  USB device class        HID
  Debug interface         CDC-ACM
  Programming languages   C / C++

## How It Works

The STM32F4 operates as a USB device and provides joystick functionality
to the connected host through the HID interface.

During USB enumeration, the host discovers and configures the device.
USB descriptors describe the device and its interfaces, while the HID
report descriptor defines the format of the joystick input reports.

A CDC-ACM interface is also used for serial debugging and device
monitoring. This provides a way to inspect firmware behaviour during
development and testing.

## USB Concepts Explored

-   **USB enumeration:** how a host discovers and configures a connected
    USB device.
-   **Device and configuration descriptors:** how a USB device describes
    its capabilities and configurations.
-   **HID report descriptors:** how joystick input data is described to
    the host.
-   **Interfaces and endpoints:** how USB functions and data transfers
    are organized.
-   **Control transfers:** how the host communicates with a USB device
    during setup and configuration.
-   **CDC-ACM:** using a USB serial interface for debugging.

## Repository Structure

The exact source layout may vary. A typical layout is:

``` text
.
├── src/              # Firmware source files
├── include/          # Header files
├── libopencm3/       # libopencm3 dependency, if included as a submodule
├── Makefile          # Build configuration
└── README.md
```

## Building and Flashing

Build and flashing commands depend on the repository's configuration and
toolchain.

Before building, make sure you have:

-   An ARM embedded GCC toolchain.
-   `make`.
-   The required STM32F4 support and libopencm3 source.
-   A supported STM32F4 flashing/debugging tool.

If the repository includes a `Makefile`, use the targets defined there.
Add the exact build and flashing commands for your board and repository
before publishing.

## Testing

1.  Flash the firmware to the STM32F4 board.
2.  Connect the board to a computer over USB.
3.  Check that the host detects the joystick HID interface.
4.  Verify joystick input reports using the operating system's game
    controller or input-device tools.
5.  Open the CDC-ACM serial interface to view debugging output, if
    enabled in the firmware.

## Notes

This README describes the project at a high level. Board-specific pin
assignments, joystick control wiring, USB descriptor values, build
commands, and flashing instructions should be documented alongside the
source code once confirmed.

## License

Add the license you intend to use for this repository. If the project
contains third-party code, retain its original license and attribution
notices.
