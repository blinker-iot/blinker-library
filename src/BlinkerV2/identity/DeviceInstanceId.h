#ifndef BLINKER_IDENTITY_DEVICEINSTANCEID_H
#define BLINKER_IDENTITY_DEVICEINSTANCEID_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum : size_t {
    kDeviceInstanceIdSize = 16U,
    kDeviceInstancePhysicalIdSize = 36U
};

// Public, stable identity of one physical DIY device instance. It is neither
// an authentication secret nor a WiFi/BLE MAC address.
struct DeviceInstanceId {
    uint8_t bytes[kDeviceInstanceIdSize];

    DeviceInstanceId() : bytes() {}
};

bool isValidDeviceInstanceId(const DeviceInstanceId& value);
bool sameDeviceInstanceId(
    const DeviceInstanceId& first,
    const DeviceInstanceId& second);

// Formats "dev_" followed by 32 lowercase hexadecimal digits. The returned
// StringView borrows output and is intentionally not NUL-terminated.
Result formatDeviceInstancePhysicalId(
    const DeviceInstanceId& value,
    MutableCharSpan output,
    StringView& physicalDeviceId);

} // namespace blinker

#endif
