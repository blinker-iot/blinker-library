#ifndef BLINKER_TEST_USB_SERIAL_JTAG_H
#define BLINKER_TEST_USB_SERIAL_JTAG_H
#include <stddef.h>
extern bool diagnosticDriverInstalled;
extern unsigned diagnosticDriverCalls;
extern unsigned diagnosticDriverWait;
extern int diagnosticDriverResult;
inline bool usb_serial_jtag_is_driver_installed() { return diagnosticDriverInstalled; }
inline int usb_serial_jtag_write_bytes(const void*, size_t, unsigned ticks) {
    ++diagnosticDriverCalls;
    diagnosticDriverWait = ticks;
    return diagnosticDriverResult;
}
#endif
