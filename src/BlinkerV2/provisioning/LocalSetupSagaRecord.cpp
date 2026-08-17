#include "LocalSetupSagaRecord.h"

#include <string.h>

#include "../core/Authorization.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool nonZero(const uint8_t* value, size_t size) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < size; ++index) {
        combined = static_cast<uint8_t>(combined | value[index]);
    }
    return combined != 0U;
}

bool zero(const uint8_t* value, size_t size) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < size; ++index) {
        combined = static_cast<uint8_t>(combined | value[index]);
    }
    return combined == 0U;
}

bool atLeast(LocalSetupSagaState actual, LocalSetupSagaState expected) {
    return static_cast<uint8_t>(actual) >= static_cast<uint8_t>(expected);
}

} // namespace

LocalSetupSagaRecord::LocalSetupSagaRecord()
    : state(LocalSetupSagaState::Authorized), ownershipGeneration(0U),
      controllerPermissions(0U), authorizedOperations(0U), ticketId(),
      controllerId(), controllerSecretDigest(), claimRequestId(), claimNonce(),
      claimCommitAckDigest(), controllerInstallDigest(), networkRequestDigest(),
      cloudCommitAckDigest() {}

Result validateLocalSetupSagaRecord(const LocalSetupSagaRecord& record) {
    const uint8_t state = static_cast<uint8_t>(record.state);
    if (state < static_cast<uint8_t>(LocalSetupSagaState::Authorized) ||
        state > static_cast<uint8_t>(LocalSetupSagaState::Complete) ||
        record.ownershipGeneration == 0U ||
        !validAuthorizationPermissions(record.controllerPermissions) ||
        !validLocalSetupAuthorizedOperations(record.authorizedOperations) ||
        !nonZero(record.ticketId, sizeof(record.ticketId)) ||
        !nonZero(record.controllerId, sizeof(record.controllerId)) ||
        !nonZero(record.controllerSecretDigest,
                 sizeof(record.controllerSecretDigest))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    const bool hasClaim = nonZero(record.claimRequestId,
                                  sizeof(record.claimRequestId)) &&
                          nonZero(record.claimNonce, sizeof(record.claimNonce));
    if (atLeast(record.state, LocalSetupSagaState::ClaimPrepared) != hasClaim) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (!atLeast(record.state, LocalSetupSagaState::ClaimPrepared) &&
        (!zero(record.claimRequestId, sizeof(record.claimRequestId)) ||
         !zero(record.claimNonce, sizeof(record.claimNonce)))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    const bool hasClaimAck = nonZero(record.claimCommitAckDigest,
                                     sizeof(record.claimCommitAckDigest));
    if (atLeast(record.state, LocalSetupSagaState::ClaimCommitVerified) !=
        hasClaimAck) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const bool hasController = nonZero(record.controllerInstallDigest,
                                       sizeof(record.controllerInstallDigest));
    if (atLeast(record.state, LocalSetupSagaState::ControllerInstalled) !=
        hasController) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    const bool wifiProfile = record.authorizedOperations ==
                             kEnrollmentTicketAuthorizeAll;
    const bool hasNetwork = nonZero(record.networkRequestDigest,
                                    sizeof(record.networkRequestDigest));
    if (wifiProfile) {
        if (atLeast(record.state,
                    LocalSetupSagaState::NetworkCandidateCommitted) !=
            hasNetwork) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    } else if (hasNetwork ||
               (record.state == LocalSetupSagaState::Complete &&
                !hasController)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    const bool hasCloudAck = nonZero(record.cloudCommitAckDigest,
                                     sizeof(record.cloudCommitAckDigest));
    if (wifiProfile) {
        if (atLeast(record.state, LocalSetupSagaState::CloudCommitVerified) !=
            hasCloudAck) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    } else if (hasCloudAck ||
               (state > static_cast<uint8_t>(
                            LocalSetupSagaState::ControllerInstalled) &&
                record.state != LocalSetupSagaState::Complete)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

bool sameLocalSetupSagaRecord(const LocalSetupSagaRecord& first,
                              const LocalSetupSagaRecord& second) {
    return first.state == second.state &&
           first.ownershipGeneration == second.ownershipGeneration &&
           first.controllerPermissions == second.controllerPermissions &&
           first.authorizedOperations == second.authorizedOperations &&
           memcmp(first.ticketId, second.ticketId, sizeof(first.ticketId)) == 0 &&
           memcmp(first.controllerId,
                  second.controllerId,
                  sizeof(first.controllerId)) == 0 &&
           memcmp(first.controllerSecretDigest,
                  second.controllerSecretDigest,
                  sizeof(first.controllerSecretDigest)) == 0 &&
           memcmp(first.claimRequestId,
                  second.claimRequestId,
                  sizeof(first.claimRequestId)) == 0 &&
           memcmp(first.claimNonce, second.claimNonce, sizeof(first.claimNonce)) == 0 &&
           memcmp(first.claimCommitAckDigest,
                  second.claimCommitAckDigest,
                  sizeof(first.claimCommitAckDigest)) == 0 &&
           memcmp(first.controllerInstallDigest,
                  second.controllerInstallDigest,
                  sizeof(first.controllerInstallDigest)) == 0 &&
           memcmp(first.networkRequestDigest,
                  second.networkRequestDigest,
                  sizeof(first.networkRequestDigest)) == 0 &&
           memcmp(first.cloudCommitAckDigest,
                  second.cloudCommitAckDigest,
                  sizeof(first.cloudCommitAckDigest)) == 0;
}

void clearLocalSetupSagaRecord(LocalSetupSagaRecord& record) {
    secureZero(MutableByteSpan(reinterpret_cast<uint8_t*>(&record),
                               sizeof(record)));
    record.state = LocalSetupSagaState::Authorized;
}

} // namespace blinker
