#include "LogicalDeviceId.h"

#include "../core/Utf8.h"

namespace blinker {

Result validateLogicalDeviceId(StringView value) {
    if (value.data == nullptr || value.empty() ||
        value.size > kLogicalDeviceIdCapacity || !isValidUtf8(value)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    for (size_t index = 0U; index < value.size; ++index) {
        if (value.data[index] == '\0') {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
    }
    return Result::success();
}

} // namespace blinker
