#include "DeviceInstanceId.h"

#include <string.h>

namespace blinker {

bool isValidDeviceInstanceId(const DeviceInstanceId& value) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < sizeof(value.bytes); ++index) {
        combined = static_cast<uint8_t>(combined | value.bytes[index]);
    }
    return combined != 0U;
}

bool sameDeviceInstanceId(
    const DeviceInstanceId& first,
    const DeviceInstanceId& second) {
    return memcmp(first.bytes, second.bytes, sizeof(first.bytes)) == 0;
}

Result formatDeviceInstancePhysicalId(
    const DeviceInstanceId& value,
    MutableCharSpan output,
    StringView& physicalDeviceId) {
    physicalDeviceId = StringView();
    if (!isValidDeviceInstanceId(value)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr ||
        output.size < kDeviceInstancePhysicalIdSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    static const char kHex[] = "0123456789abcdef";
    output.data[0] = 'd';
    output.data[1] = 'e';
    output.data[2] = 'v';
    output.data[3] = '_';
    for (size_t index = 0U; index < sizeof(value.bytes); ++index) {
        const uint8_t byte = value.bytes[index];
        output.data[4U + index * 2U] = kHex[(byte >> 4U) & 0x0FU];
        output.data[5U + index * 2U] = kHex[byte & 0x0FU];
    }
    physicalDeviceId = StringView(output.data, kDeviceInstancePhysicalIdSize);
    return Result::success();
}

} // namespace blinker
