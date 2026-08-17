#ifndef BLINKER_PROVISIONING_ENROLLMENTKEY_H
#define BLINKER_PROVISIONING_ENROLLMENTKEY_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum : size_t {
    kEnrollmentKeyPrefixSize = 4U,
    kEnrollmentKeyMinimumSize = 26U,
    kEnrollmentKeyMaximumSize = 90U
};

// Opaque, server-generated WiFi-only bootstrap capability. The text form is
// "bek_" followed by unpadded base64url. It is never a device ID or a
// long-term cloud credential.
Result validateEnrollmentKey(StringView key);

} // namespace blinker

#endif
