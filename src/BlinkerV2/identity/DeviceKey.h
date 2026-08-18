#ifndef BLINKER_IDENTITY_DEVICEKEY_H
#define BLINKER_IDENTITY_DEVICEKEY_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum : size_t {
    kDeviceKeySize = 32U,
    kDeviceKeyTextSize = 43U,
    kDeviceKeyLocatorSize = 16U,
    kDeviceAuthKeySize = 32U
};

// Education/DIY root authentication secret. The external form is exactly
// 43 unpadded Base64URL characters. It has no prefix and is never a device ID.
struct DeviceKey {
    uint8_t bytes[kDeviceKeySize];

    DeviceKey() : bytes() {}
};

struct DeviceKeyLocator {
    uint8_t bytes[kDeviceKeyLocatorSize];

    DeviceKeyLocator() : bytes() {}
};

Result decodeDeviceKey(StringView encoded, DeviceKey& output);
Result encodeDeviceKey(
    const DeviceKey& key,
    MutableCharSpan output,
    StringView& encoded);
Result validateDeviceKey(const DeviceKey& key);
bool sameDeviceKey(const DeviceKey& first, const DeviceKey& second);
void clearDeviceKey(DeviceKey& key);

// locator = first 16 bytes of
// SHA-256("blinker/device-key/locator/v1" || 0x00 || raw DeviceKey).
Result deriveDeviceKeyLocator(
    const DeviceKey& key,
    DeviceKeyLocator& output);

// authKey = HKDF-SHA256(
//     salt = empty (RFC 5869 32 zero bytes),
//     IKM = raw DeviceKey,
//     info = "blinker/device-key/auth/v1",
//     L = 32).
Result deriveDeviceAuthKey(
    const DeviceKey& key,
    MutableByteSpan output);

} // namespace blinker

#endif
