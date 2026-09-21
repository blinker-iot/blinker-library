#ifndef BLINKER_ESP32_USBDIAGNOSTICOUTPUT_H
#define BLINKER_ESP32_USBDIAGNOSTICOUTPUT_H

#include <driver/usb_serial_jtag.h>

#include "../../../interface/IDiagnosticOutput.h"

#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT && \
    defined(ARDUINO_USB_MODE) && ARDUINO_USB_MODE
#error "IDF diagnostic USB output cannot share the peripheral with Arduino HWCDC on boot"
#endif

namespace blinker {
namespace esp32 {

// Borrows an already-installed IDF USB Serial/JTAG driver. The application
// owns install/uninstall and must exclude Arduino HWCDC (including CDC on
// boot) from this peripheral; driver lifetime must not change concurrently
// with tryWrite. Not a wrapper for Serial and never installs,
// reconfigures or takes over the user's console. Valid only on USB-JTAG MCUs.
class Esp32UsbDiagnosticOutput : public IDiagnosticOutput {
public:
    size_t tryWrite(const uint8_t* bytes, size_t size) override {
        if (bytes == nullptr || size == 0U ||
            !usb_serial_jtag_is_driver_installed()) return 0U;
        const int accepted = usb_serial_jtag_write_bytes(bytes, size, 0);
        return accepted > 0 ? static_cast<size_t>(accepted) : 0U;
    }
};

} // namespace esp32
} // namespace blinker

#endif
