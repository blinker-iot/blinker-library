#ifndef BLINKER_PROVISIONING_LOCALSETUPSAGARECORD_H
#define BLINKER_PROVISIONING_LOCALSETUPSAGARECORD_H

#include "EnrollmentTicketContract.h"
#include "LocalSetupContract.h"
#include "OwnershipClaimContract.h"

namespace blinker {

enum class LocalSetupSagaState : uint8_t {
    Authorized = 1U,
    ClaimPrepared = 2U,
    OwnershipGranted = 3U,
    ClaimCommitVerified = 4U,
    OwnershipCommitted = 5U,
    ControllerInstalled = 6U,
    NetworkCandidateCommitted = 7U,
    CloudEnrollmentPending = 8U,
    CloudCredentialGranted = 9U,
    CloudCommitVerified = 10U,
    Complete = 11U
};

struct LocalSetupSagaRecord {
    LocalSetupSagaState state;
    uint32_t ownershipGeneration;
    uint32_t controllerPermissions;
    uint32_t authorizedOperations;
    uint8_t ticketId[kEnrollmentTicketIdSize];
    uint8_t controllerId[kControllerIdSize];
    uint8_t controllerSecretDigest[
        kEnrollmentTicketControllerSecretDigestSize];
    uint8_t claimRequestId[kOwnershipClaimRequestIdSize];
    uint8_t claimNonce[kOwnershipClaimNonceSize];
    uint8_t claimCommitAckDigest[kSha256Size];
    uint8_t controllerInstallDigest[kSha256Size];
    uint8_t networkRequestDigest[kSha256Size];
    uint8_t cloudCommitAckDigest[kSha256Size];

    LocalSetupSagaRecord();
};

Result validateLocalSetupSagaRecord(const LocalSetupSagaRecord& record);
bool sameLocalSetupSagaRecord(const LocalSetupSagaRecord& first,
                              const LocalSetupSagaRecord& second);
void clearLocalSetupSagaRecord(LocalSetupSagaRecord& record);

} // namespace blinker

#endif
