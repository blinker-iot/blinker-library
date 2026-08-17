#ifndef BLINKER_IDENTITY_CLOUDCREDENTIAL_H
#define BLINKER_IDENTITY_CLOUDCREDENTIAL_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "LogicalDeviceId.h"

namespace blinker {

enum class CloudCredentialSuite : uint8_t {
    HmacSha256_32 = 1U
};

enum : size_t {
    kCloudCredentialSecretSize = 32U,
    kCloudLogicalDeviceIdCapacity = kLogicalDeviceIdCapacity
};

// One Active long-term cloud authentication root. Enrollment/claim request
// IDs and pending state belong to a separate transaction journal.
struct CloudCredential {
    CloudCredentialSuite suite;
    uint8_t logicalDeviceIdLength;
    uint32_t generation;
    uint32_t credentialVersion;
    uint8_t secret[kCloudCredentialSecretSize];
    char logicalDeviceId[kCloudLogicalDeviceIdCapacity];

    CloudCredential();

    ByteView authenticationSecret() const {
        return ByteView(secret, sizeof(secret));
    }
    StringView logicalId() const {
        return StringView(logicalDeviceId, logicalDeviceIdLength);
    }
};

Result validateCloudCredential(const CloudCredential& credential);
bool sameCloudCredential(
    const CloudCredential& first,
    const CloudCredential& second);
void clearCloudCredential(CloudCredential& credential);

} // namespace blinker

#endif
