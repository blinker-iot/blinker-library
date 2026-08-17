#ifndef BLINKER_IDENTITY_OWNERSHIPRECORD_H
#define BLINKER_IDENTITY_OWNERSHIPRECORD_H

#include "LogicalDeviceId.h"

namespace blinker {

enum class OwnershipState : uint8_t {
    Unclaimed = 0U,
    Active = 1U
};

enum : size_t { kOwnershipGrantFingerprintSize = 32U };

// Durable result of an already verified OwnershipClaimGrant. The exact signed
// grant and its transaction journal belong to the ownership wire layer.
// An Unclaimed record is a generation tombstone, not an empty fresh device.
struct OwnershipRecord {
    uint32_t generation;
    OwnershipState state;
    uint8_t logicalDeviceIdLength;
    uint8_t grantFingerprint[kOwnershipGrantFingerprintSize];
    char logicalDeviceId[kLogicalDeviceIdCapacity];

    OwnershipRecord();

    bool active() const { return state == OwnershipState::Active; }
    StringView logicalId() const {
        return StringView(logicalDeviceId, logicalDeviceIdLength);
    }
};

Result validateOwnershipRecord(const OwnershipRecord& record);
bool sameOwnershipRecord(
    const OwnershipRecord& first,
    const OwnershipRecord& second);
void clearOwnershipRecord(OwnershipRecord& record);
Result makeOwnershipTombstone(
    uint32_t nextGeneration,
    OwnershipRecord& output);

} // namespace blinker

#endif
