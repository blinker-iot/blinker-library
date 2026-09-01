#ifndef BLINKER_IDENTITY_DEVICEPRESENCEKEY_H
#define BLINKER_IDENTITY_DEVICEPRESENCEKEY_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum : size_t {
    kDevicePresenceKeySize = 16U
};

// Per-logical-device secret used only to privately match BLE advertisements.
// It is not a controller credential and never authorizes a GATT session.
struct DevicePresenceKey {
    uint32_t version;
    uint8_t bytes[kDevicePresenceKeySize];

    DevicePresenceKey() : version(0U), bytes() {}
};

Result validateDevicePresenceKey(const DevicePresenceKey& value);
bool sameDevicePresenceKey(
    const DevicePresenceKey& first,
    const DevicePresenceKey& second);
void clearDevicePresenceKey(DevicePresenceKey& value);

} // namespace blinker

#endif
