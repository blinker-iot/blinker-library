#include "DevicePresenceKey.h"

#include "../core/SecureMemory.h"

namespace blinker {

Result validateDevicePresenceKey(const DevicePresenceKey& value) {
    if (value.version == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t combined = 0U;
    for (size_t index = 0U; index < sizeof(value.bytes); ++index) {
        combined = static_cast<uint8_t>(combined | value.bytes[index]);
    }
    return combined != 0U
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

bool sameDevicePresenceKey(
    const DevicePresenceKey& first,
    const DevicePresenceKey& second) {
    return first.version == second.version &&
           constantTimeEqual(
               ByteView(first.bytes, sizeof(first.bytes)),
               ByteView(second.bytes, sizeof(second.bytes)));
}

void clearDevicePresenceKey(DevicePresenceKey& value) {
    value.version = 0U;
    secureZero(MutableByteSpan(value.bytes, sizeof(value.bytes)));
}

} // namespace blinker
