#ifndef BLINKER_PROVISIONING_OWNERSHIPRELEASECLEANUP_H
#define BLINKER_PROVISIONING_OWNERSHIPRELEASECLEANUP_H

#include "../interface/ICloudCredentialStore.h"
#include "../interface/ICloudEnrollmentRecordStore.h"
#include "../interface/IControllerCredentialStore.h"
#include "../interface/INetworkCredentialStore.h"
#include "../interface/IOwnershipClaimRecordStore.h"
#include "OwnershipReleaseContract.h"

namespace blinker {

// Idempotent post-CommitAck cleanup. The release journal remains durable
// until every selected domain has been cleared successfully.
class OwnershipReleaseCleanup {
public:
    OwnershipReleaseCleanup(
        ICloudCredentialStore& cloud,
        ICloudEnrollmentRecordStore& cloudEnrollment,
        IOwnershipClaimRecordStore& ownershipClaim,
        IControllerCredentialStore& controllers,
        INetworkCredentialStore* network = nullptr)
        : cloud_(cloud),
          cloudEnrollment_(cloudEnrollment),
          ownershipClaim_(ownershipClaim),
          controllers_(controllers),
          network_(network) {}

    Result finalize(OwnershipReleaseNetworkPolicy networkPolicy);

private:
    ICloudCredentialStore& cloud_;
    ICloudEnrollmentRecordStore& cloudEnrollment_;
    IOwnershipClaimRecordStore& ownershipClaim_;
    IControllerCredentialStore& controllers_;
    INetworkCredentialStore* network_;
};

} // namespace blinker

#endif
