#ifndef BLINKER_PROVISIONING_OWNERSHIPCLAIMRECORD_H
#define BLINKER_PROVISIONING_OWNERSHIPCLAIMRECORD_H

#include "OwnershipClaimContract.h"

namespace blinker {

// Durable request window for one Unclaimed -> Active claim. It remains
// separate from OwnershipRecord and is cleared only after the server has
// acknowledged the device receipt.
struct OwnershipClaimRecord {
    uint32_t ownershipGeneration;
    uint8_t requestId[kOwnershipClaimRequestIdSize];
    uint8_t claimNonce[kOwnershipClaimNonceSize];

    OwnershipClaimRecord();
};

Result validateOwnershipClaimRecord(const OwnershipClaimRecord& record);
bool sameOwnershipClaimRecord(
    const OwnershipClaimRecord& first,
    const OwnershipClaimRecord& second);
void clearOwnershipClaimRecord(OwnershipClaimRecord& record);

} // namespace blinker

#endif
