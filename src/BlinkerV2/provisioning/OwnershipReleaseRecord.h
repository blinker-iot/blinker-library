#ifndef BLINKER_PROVISIONING_OWNERSHIPRELEASERECORD_H
#define BLINKER_PROVISIONING_OWNERSHIPRELEASERECORD_H

#include "OwnershipReleaseContract.h"

namespace blinker {

enum class OwnershipReleaseRecordState : uint8_t {
    Prepared = 1U,
    GrantAccepted = 2U,
    CommitAcknowledged = 3U
};

// Durable transaction state. The prior logical ID is retained because the
// ownership store becomes a generation tombstone before Receipt is sent.
struct OwnershipReleaseRecord {
    OwnershipReleaseRecordState state;
    OwnershipReleaseOperation operation;
    OwnershipReleaseNetworkPolicy networkPolicy;
    uint8_t logicalDeviceIdLength;
    uint32_t currentGeneration;
    uint32_t nextGeneration;
    uint8_t requestId[kOwnershipReleaseRequestIdSize];
    uint8_t releaseNonce[kOwnershipReleaseNonceSize];
    uint8_t grantId[kOwnershipReleaseGrantIdSize];
    uint8_t grantDigest[kSha256Size];
    char logicalDeviceId[kLogicalDeviceIdCapacity];

    OwnershipReleaseRecord();

    StringView logicalId() const {
        return StringView(logicalDeviceId, logicalDeviceIdLength);
    }
};

Result validateOwnershipReleaseRecord(const OwnershipReleaseRecord& record);
bool sameOwnershipReleaseRecord(
    const OwnershipReleaseRecord& first,
    const OwnershipReleaseRecord& second);
void clearOwnershipReleaseRecord(OwnershipReleaseRecord& record);

} // namespace blinker

#endif
