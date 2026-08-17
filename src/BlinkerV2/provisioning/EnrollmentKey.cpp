#include "EnrollmentKey.h"

namespace blinker {

Result validateEnrollmentKey(StringView key) {
    static const char prefix[] = "bek_";
    if (key.data == nullptr || key.size < kEnrollmentKeyMinimumSize ||
        key.size > kEnrollmentKeyMaximumSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    for (size_t index = 0U; index < kEnrollmentKeyPrefixSize; ++index) {
        if (key.data[index] != prefix[index]) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    }
    for (size_t index = kEnrollmentKeyPrefixSize; index < key.size; ++index) {
        const char value = key.data[index];
        if (!((value >= 'A' && value <= 'Z') ||
              (value >= 'a' && value <= 'z') ||
              (value >= '0' && value <= '9') || value == '-' ||
              value == '_')) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    }
    return Result::success();
}

} // namespace blinker
