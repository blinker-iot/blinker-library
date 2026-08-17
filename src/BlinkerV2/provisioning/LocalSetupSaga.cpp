#include "LocalSetupSaga.h"

#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "ControllerControlContract.h"
#include "LocalSetupOperationContract.h"

namespace blinker {

namespace {

bool exact(ByteView value, const uint8_t* expected, size_t size) {
    return value.data != nullptr && value.size == size &&
           constantTimeEqual(value, ByteView(expected, size));
}

bool exactNonZero(ByteView value, size_t size) {
    if (value.data == nullptr || value.size != size) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool atLeast(LocalSetupSagaState actual, LocalSetupSagaState expected) {
    return static_cast<uint8_t>(actual) >= static_cast<uint8_t>(expected);
}

Result digest(ByteView value, uint8_t output[kSha256Size]) {
    return sha256(value, MutableByteSpan(output, kSha256Size));
}

bool sameDigest(const uint8_t first[kSha256Size],
                const uint8_t second[kSha256Size]) {
    return constantTimeEqual(ByteView(first, kSha256Size),
                             ByteView(second, kSha256Size));
}

bool sameNetwork(const WifiCredentialProfile& profile,
                 const WifiNetworkConfig& network) {
    if (profile.authentication != network.authentication ||
        profile.hidden() != network.hidden ||
        profile.ssidLength != network.ssid.size ||
        profile.credentialLength != network.credential.size) {
        return false;
    }
    if (!network.ssid.empty() &&
        memcmp(profile.ssid, network.ssid.data, network.ssid.size) != 0) {
        return false;
    }
    return constantTimeEqual(
        ByteView(profile.credential, profile.credentialLength),
        network.credential);
}

} // namespace

LocalSetupSaga::LocalSetupSaga(
    const DeviceInstanceId& deviceInstanceId,
    IOwnershipRecordStore& ownership,
    OwnershipClaimCoordinator& claim,
    IControllerCredentialStore& controllers,
    LocalSetupSagaStore& journal,
    IClock& clock,
    WifiCredentialStore* network,
    CloudCredentialEnrollmentCoordinator* cloud)
    : deviceInstanceId_(deviceInstanceId), ownership_(ownership), claim_(claim),
      controllers_(controllers), journal_(journal), clock_(clock),
      network_(network), cloud_(cloud), authorizedSession_(false) {}

LocalSetupSaga::~LocalSetupSaga() { endAuthorizedSession(); }

Result LocalSetupSaga::prepareSetupSession(ByteView setupSessionLocator,
                                           uint32_t& generation) {
    generation = 0U;
    if (!isValidDeviceInstanceId(deviceInstanceId_) ||
        !exactNonZero(setupSessionLocator, kSetupSessionLocatorSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    LocalSetupSagaRecord saga;
    Result result = journal_.load(saga);
    if (result) {
        if (saga.state == LocalSetupSagaState::Complete) {
            bool recoverable = false;
            result = saga.authorizedOperations ==
                             kEnrollmentTicketAuthorizeAll
                         ? networkRecoveryAvailable(recoverable)
                         : Result::success();
            if (result && !recoverable) {
                result = Result::failure(ErrorCode::StateConflict);
            }
        }
        if (result) generation = saga.ownershipGeneration;
        clearLocalSetupSagaRecord(saga);
        return result;
    }
    clearLocalSetupSagaRecord(saga);
    if (result.code() != ErrorCode::NotFound) return result;

    OwnershipRecord ownership;
    result = ownership_.load(ownership);
    if (!result && result.code() == ErrorCode::NotFound) {
        generation = 1U;
        clearOwnershipRecord(ownership);
        return Result::success();
    }
    if (!result) {
        clearOwnershipRecord(ownership);
        return result;
    }
    if (ownership.active() || ownership.generation == 0U) {
        clearOwnershipRecord(ownership);
        return Result::failure(ErrorCode::StateConflict);
    }
    generation = ownership.generation;
    clearOwnershipRecord(ownership);
    return Result::success();
}

Result LocalSetupSaga::validateContext(
    const LocalSetupSagaRecord& record,
    const LocalSetupAuthorization& authorization) const {
    return exact(authorization.ticketId,
                 record.ticketId,
                 sizeof(record.ticketId)) &&
                   exact(authorization.controllerId,
                         record.controllerId,
                         sizeof(record.controllerId)) &&
                   exact(authorization.controllerSecretDigest,
                         record.controllerSecretDigest,
                         sizeof(record.controllerSecretDigest)) &&
                   authorization.ownershipGeneration ==
                       record.ownershipGeneration &&
                   authorization.controllerPermissions ==
                       record.controllerPermissions &&
                   authorization.authorizedOperations ==
                       record.authorizedOperations
               ? Result::success()
               : Result::failure(ErrorCode::AuthenticationRequired);
}

Result LocalSetupSaga::beginAuthorizedSession(
    const LocalSetupAuthorization& authorization) {
    authorizedSession_ = false;
    if (!exactNonZero(authorization.ticketId, kEnrollmentTicketIdSize) ||
        !exactNonZero(authorization.controllerId, kControllerIdSize) ||
        !exactNonZero(authorization.controllerSecretDigest,
                      kEnrollmentTicketControllerSecretDigestSize) ||
        authorization.ownershipGeneration == 0U ||
        !validAuthorizationPermissions(authorization.controllerPermissions) ||
        !validLocalSetupAuthorizedOperations(
            authorization.authorizedOperations)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (authorization.authorizedOperations == kEnrollmentTicketAuthorizeAll &&
        (network_ == nullptr || cloud_ == nullptr)) {
        return Result::failure(ErrorCode::NotConfigured);
    }

    LocalSetupSagaRecord saga;
    Result result = journal_.load(saga);
    if (!result && result.code() == ErrorCode::NotFound) {
        saga = LocalSetupSagaRecord();
        saga.ownershipGeneration = authorization.ownershipGeneration;
        saga.controllerPermissions = authorization.controllerPermissions;
        saga.authorizedOperations = authorization.authorizedOperations;
        memcpy(saga.ticketId, authorization.ticketId.data, sizeof(saga.ticketId));
        memcpy(saga.controllerId,
               authorization.controllerId.data,
               sizeof(saga.controllerId));
        memcpy(saga.controllerSecretDigest,
               authorization.controllerSecretDigest.data,
               sizeof(saga.controllerSecretDigest));
        result = journal_.save(saga);
    } else if (result) {
        if (saga.state == LocalSetupSagaState::Complete) {
            result = validateContext(saga, authorization);
            bool recoverable = false;
            if (result) result = networkRecoveryAvailable(recoverable);
            if (result && !recoverable) {
                result = Result::failure(ErrorCode::StateConflict);
            }
        } else {
            result = validateContext(saga, authorization);
        }
    }
    clearLocalSetupSagaRecord(saga);
    if (result) authorizedSession_ = true;
    return result;
}

void LocalSetupSaga::endAuthorizedSession() { authorizedSession_ = false; }

Result LocalSetupSaga::encodeClaimRequest(
    const LocalSetupSagaRecord& record,
    MutableByteSpan output,
    ByteView& response) const {
    OwnershipClaimRequest request;
    request.requestId = ByteView(record.claimRequestId,
                                 sizeof(record.claimRequestId));
    request.deviceInstanceId = ByteView(deviceInstanceId_.bytes,
                                        sizeof(deviceInstanceId_.bytes));
    request.ownershipGeneration = record.ownershipGeneration;
    request.claimNonce = ByteView(record.claimNonce, sizeof(record.claimNonce));
    return encodeOwnershipClaimRequest(request, output, response);
}

Result LocalSetupSaga::handleBeginClaim(LocalSetupSagaRecord& record,
                                        MutableByteSpan output,
                                        ByteView& response) {
    if (record.state == LocalSetupSagaState::Authorized) {
        Result result = claim_.beginClaim(output, response);
        OwnershipClaimRequest decoded;
        if (result) result = decodeOwnershipClaimRequest(response, decoded);
        if (result &&
            (decoded.ownershipGeneration != record.ownershipGeneration ||
             !exact(decoded.deviceInstanceId,
                    deviceInstanceId_.bytes,
                    sizeof(deviceInstanceId_.bytes)))) {
            result = Result::failure(ErrorCode::InternalError);
        }
        if (result) {
            memcpy(record.claimRequestId,
                   decoded.requestId.data,
                   sizeof(record.claimRequestId));
            memcpy(record.claimNonce,
                   decoded.claimNonce.data,
                   sizeof(record.claimNonce));
            record.state = LocalSetupSagaState::ClaimPrepared;
            result = journal_.save(record);
        }
        return result;
    }
    if (atLeast(record.state, LocalSetupSagaState::ClaimPrepared)) {
        return encodeClaimRequest(record, output, response);
    }
    return Result::failure(ErrorCode::SequenceConflict);
}

Result LocalSetupSaga::encodeClaimReceipt(
    const LocalSetupSagaRecord& record,
    ByteView encodedGrant,
    MutableByteSpan output,
    ByteView& response) const {
    uint8_t grantDigest[kSha256Size] = {};
    Result result = digest(encodedGrant, grantDigest);
    OwnershipRecord ownership;
    if (result) result = ownership_.load(ownership);
    if (result &&
        (!ownership.active() ||
         ownership.generation != record.ownershipGeneration ||
         !sameDigest(grantDigest, ownership.grantFingerprint))) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    OwnershipClaimGrant grant;
    if (result) result = decodeOwnershipClaimGrant(encodedGrant, grant);
    OwnershipClaimReceipt receipt;
    if (result) {
        receipt.grantId = grant.grantId;
        receipt.requestId = ByteView(record.claimRequestId,
                                     sizeof(record.claimRequestId));
        receipt.deviceInstanceId = ByteView(deviceInstanceId_.bytes,
                                            sizeof(deviceInstanceId_.bytes));
        receipt.logicalDeviceId = ownership.logicalId();
        receipt.ownershipGeneration = ownership.generation;
        receipt.grantDigest = ByteView(grantDigest, sizeof(grantDigest));
        result = encodeOwnershipClaimReceipt(receipt, output, response);
    }
    secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
    clearOwnershipRecord(ownership);
    return result;
}

Result LocalSetupSaga::handleApplyClaimGrant(
    LocalSetupSagaRecord& record,
    ByteView body,
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& response) {
    if (record.state == LocalSetupSagaState::ClaimPrepared) {
        uint64_t now = 0U;
        const bool trustedTime = clock_.unixTime(now);
        Result result = claim_.applyGrant(
            body, now, trustedTime, workspace, output, response);
        if (result) {
            record.state = LocalSetupSagaState::OwnershipGranted;
            result = journal_.save(record);
        }
        return result;
    }
    if (atLeast(record.state, LocalSetupSagaState::OwnershipGranted)) {
        return encodeClaimReceipt(record, body, output, response);
    }
    return Result::failure(ErrorCode::SequenceConflict);
}

Result LocalSetupSaga::handleCompleteClaim(
    LocalSetupSagaRecord& record,
    ByteView body,
    MutableByteSpan workspace,
    ByteView& response) {
    response = ByteView();
    uint8_t bodyDigest[kSha256Size] = {};
    Result result = digest(body, bodyDigest);
    if (!result) return result;
    if (record.state == LocalSetupSagaState::OwnershipGranted) {
        result = claim_.verifyCommitAck(body, workspace);
        if (result) {
            memcpy(record.claimCommitAckDigest,
                   bodyDigest,
                   sizeof(record.claimCommitAckDigest));
            record.state = LocalSetupSagaState::ClaimCommitVerified;
            result = journal_.save(record);
        }
    } else if (record.state == LocalSetupSagaState::ClaimCommitVerified) {
        if (!sameDigest(bodyDigest, record.claimCommitAckDigest)) {
            result = Result::failure(ErrorCode::StateConflict);
        }
    } else if (atLeast(record.state,
                       LocalSetupSagaState::OwnershipCommitted)) {
        result = sameDigest(bodyDigest, record.claimCommitAckDigest)
                     ? Result::success()
                     : Result::failure(ErrorCode::StateConflict);
        secureZero(MutableByteSpan(bodyDigest, sizeof(bodyDigest)));
        return result;
    } else {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result && record.state == LocalSetupSagaState::ClaimCommitVerified) {
        result = claim_.finishVerifiedClaim();
        if (result) {
            record.state = LocalSetupSagaState::OwnershipCommitted;
            result = journal_.save(record);
        }
    }
    secureZero(MutableByteSpan(bodyDigest, sizeof(bodyDigest)));
    return result;
}

Result LocalSetupSaga::installControllerAndEncodeReceipt(
    const LocalSetupSagaRecord& record,
    ByteView secret,
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& response) {
    uint8_t secretDigest[kSha256Size] = {};
    Result result = digest(secret, secretDigest);
    if (result && !sameDigest(secretDigest, record.controllerSecretDigest)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    OwnershipRecord ownership;
    if (result) result = ownership_.load(ownership);
    if (result &&
        (!ownership.active() ||
         ownership.generation != record.ownershipGeneration)) {
        result = Result::failure(ErrorCode::StateConflict);
    }

    ControllerCredential credential;
    if (result) {
        credential.ownershipGeneration = record.ownershipGeneration;
        credential.credentialVersion = 1U;
        credential.permissions = record.controllerPermissions;
        credential.suite = ControllerCredentialSuite::HmacSha256_32;
        credential.domain = ControllerCredentialDomain::Ownership;
        memcpy(credential.controllerId,
               record.controllerId,
               sizeof(credential.controllerId));
        memcpy(credential.secret, secret.data, sizeof(credential.secret));
        result = controllers_.installVerified(credential);
    }

    uint8_t proof[kControllerReceiptProofSize] = {};
    ControllerMutationReceipt receipt;
    if (result) {
        receipt.operation = ControllerMutationOperation::Install;
        receipt.grantId = ByteView(record.ticketId, sizeof(record.ticketId));
        receipt.deviceInstanceId = ByteView(deviceInstanceId_.bytes,
                                            sizeof(deviceInstanceId_.bytes));
        receipt.ownershipGeneration = record.ownershipGeneration;
        receipt.controllerId = ByteView(record.controllerId,
                                        sizeof(record.controllerId));
        receipt.credentialVersion = 1U;
        receipt.permissions = record.controllerPermissions;
        receipt.secretDigest = ByteView(secretDigest, sizeof(secretDigest));
        receipt.proofKind = ControllerReceiptProofKind::HmacSha256;
        result = computeControllerMutationReceiptProof(
            secret,
            receipt,
            workspace,
            MutableByteSpan(proof, sizeof(proof)));
        if (result) {
            receipt.proof = ByteView(proof, sizeof(proof));
            result = encodeControllerMutationReceipt(receipt, output, response);
        }
    }
    clearOwnershipRecord(ownership);
    clearControllerCredential(credential);
    secureZero(MutableByteSpan(secretDigest, sizeof(secretDigest)));
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    return result;
}

Result LocalSetupSaga::handleInstallController(
    LocalSetupSagaRecord& record,
    ByteView body,
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& response) {
    if (!atLeast(record.state, LocalSetupSagaState::OwnershipCommitted)) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    InitialControllerInstall install;
    Result result = decodeInitialControllerInstall(body, install);
    uint8_t bodyDigest[kSha256Size] = {};
    if (result) result = digest(body, bodyDigest);
    if (result &&
        atLeast(record.state, LocalSetupSagaState::ControllerInstalled) &&
        !sameDigest(bodyDigest, record.controllerInstallDigest)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) {
        result = installControllerAndEncodeReceipt(
            record, install.controllerSecret, workspace, output, response);
    }
    if (result && record.state == LocalSetupSagaState::OwnershipCommitted) {
        memcpy(record.controllerInstallDigest,
               bodyDigest,
               sizeof(record.controllerInstallDigest));
        record.state = record.authorizedOperations == 0U
                           ? LocalSetupSagaState::Complete
                           : LocalSetupSagaState::ControllerInstalled;
        result = journal_.save(record);
    }
    secureZero(MutableByteSpan(bodyDigest, sizeof(bodyDigest)));
    return result;
}

Result LocalSetupSaga::handleConfigureNetwork(
    LocalSetupSagaRecord& record,
    ByteView body,
    MutableByteSpan output,
    ByteView& response) {
    if (network_ == nullptr ||
        record.authorizedOperations != kEnrollmentTicketAuthorizeAll) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (!atLeast(record.state, LocalSetupSagaState::ControllerInstalled)) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    ConfigureNetworkRequest request;
    Result result = decodeConfigureNetworkRequest(body, request);
    uint8_t bodyDigest[kSha256Size] = {};
    if (result) result = digest(body, bodyDigest);
    const bool hasCandidate =
        atLeast(record.state,
                LocalSetupSagaState::NetworkCandidateCommitted);
    const bool sameRequest =
        hasCandidate &&
        sameDigest(bodyDigest, record.networkRequestDigest);
    bool replaceCandidate = false;
    if (result && hasCandidate && !sameRequest) {
        if (record.state != LocalSetupSagaState::NetworkCandidateCommitted &&
            record.state != LocalSetupSagaState::Complete) {
            result = Result::failure(ErrorCode::StateConflict);
        } else {
            result = networkRecoveryAvailable(replaceCandidate);
            if (result && !replaceCandidate) {
                result = Result::failure(ErrorCode::StateConflict);
            }
        }
    }
    if (result && sameRequest) {
        WifiCredentialProfile active;
        result = network_->load(active);
        if (result && !sameNetwork(active, request.network)) {
            result = Result::failure(ErrorCode::StateConflict);
        } else if (!result && result.code() == ErrorCode::NotFound &&
                   (record.state ==
                        LocalSetupSagaState::NetworkCandidateCommitted ||
                    record.state == LocalSetupSagaState::Complete)) {
            result = network_->commit(request.network);
        }
        clearWifiCredentialProfile(active);
    } else if (result) {
        result = network_->commit(request.network);
    }
    if (result && network_->activeGeneration() == 0U) {
        result = Result::failure(ErrorCode::InternalError);
    }
    if (result &&
        (record.state == LocalSetupSagaState::ControllerInstalled ||
         replaceCandidate)) {
        memcpy(record.networkRequestDigest,
               bodyDigest,
               sizeof(record.networkRequestDigest));
        if (record.state == LocalSetupSagaState::ControllerInstalled) {
            record.state = LocalSetupSagaState::NetworkCandidateCommitted;
        }
        result = journal_.save(record);
    }
    if (result) {
        ConfigureNetworkResponse resultBody;
        resultBody.networkGeneration = network_->activeGeneration();
        result = encodeConfigureNetworkResponse(resultBody, output, response);
    }
    secureZero(MutableByteSpan(bodyDigest, sizeof(bodyDigest)));
    return result;
}

Result LocalSetupSaga::networkRecoveryAvailable(bool& available) {
    available = false;
    if (network_ == nullptr) return Result::success();
    WifiCredentialProfile active;
    Result result = network_->load(active);
    if (result) {
        available = !active.confirmed();
    } else if (result.code() == ErrorCode::NotFound) {
        available = true;
        result = Result::success();
    }
    clearWifiCredentialProfile(active);
    return result;
}

Result LocalSetupSaga::loadCloud() {
    return cloud_ == nullptr ? Result::failure(ErrorCode::NotConfigured)
                             : cloud_->load();
}

Result LocalSetupSaga::handleBeginCloud(
    LocalSetupSagaRecord& record,
    ByteView body,
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& response) {
    if (!body.empty()) return Result::failure(ErrorCode::InvalidEncoding);
    if (cloud_ == nullptr ||
        record.authorizedOperations != kEnrollmentTicketAuthorizeAll) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (record.state != LocalSetupSagaState::NetworkCandidateCommitted &&
        record.state != LocalSetupSagaState::CloudEnrollmentPending &&
        record.state != LocalSetupSagaState::CloudCredentialGranted) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    Result result = loadCloud();
    if (result && cloud_->state() == CloudCredentialEnrollmentState::Ready) {
        result = cloud_->prepare();
    }
    if (result &&
        cloud_->state() != CloudCredentialEnrollmentState::Prepared &&
        cloud_->state() !=
            CloudCredentialEnrollmentState::AwaitingCommitAck) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = cloud_->request(workspace, output, response);
    if (result &&
        record.state == LocalSetupSagaState::NetworkCandidateCommitted) {
        record.state = LocalSetupSagaState::CloudEnrollmentPending;
        result = journal_.save(record);
    }
    return result;
}

Result LocalSetupSaga::handleApplyCloudGrant(
    LocalSetupSagaRecord& record,
    ByteView body,
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& response) {
    if (cloud_ == nullptr ||
        record.authorizedOperations != kEnrollmentTicketAuthorizeAll) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (record.state != LocalSetupSagaState::CloudEnrollmentPending &&
        record.state != LocalSetupSagaState::CloudCredentialGranted) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    Result result = loadCloud();
    if (result && record.state == LocalSetupSagaState::CloudEnrollmentPending) {
        uint64_t now = 0U;
        const bool trustedTime = clock_.unixTime(now);
        result = cloud_->applyGrant(
            body, now, trustedTime, workspace, output, response);
        if (result) {
            record.state = LocalSetupSagaState::CloudCredentialGranted;
            result = journal_.save(record);
        }
    } else if (result) {
        // applyGrant compares the durable grant digest and regenerates the
        // HMAC receipt without applying a second mutation.
        uint64_t now = 0U;
        const bool trustedTime = clock_.unixTime(now);
        result = cloud_->applyGrant(
            body, now, trustedTime, workspace, output, response);
    }
    return result;
}

Result LocalSetupSaga::handleCompleteCloud(
    LocalSetupSagaRecord& record,
    ByteView body,
    MutableByteSpan workspace,
    ByteView& response) {
    response = ByteView();
    if (cloud_ == nullptr ||
        record.authorizedOperations != kEnrollmentTicketAuthorizeAll) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    uint8_t bodyDigest[kSha256Size] = {};
    Result result = digest(body, bodyDigest);
    if (!result) return result;
    if (record.state == LocalSetupSagaState::CloudCredentialGranted) {
        result = loadCloud();
        if (result) result = cloud_->verifyCommitAck(body, workspace);
        if (result) {
            memcpy(record.cloudCommitAckDigest,
                   bodyDigest,
                   sizeof(record.cloudCommitAckDigest));
            record.state = LocalSetupSagaState::CloudCommitVerified;
            result = journal_.save(record);
        }
    } else if (record.state == LocalSetupSagaState::CloudCommitVerified) {
        if (!sameDigest(bodyDigest, record.cloudCommitAckDigest)) {
            result = Result::failure(ErrorCode::StateConflict);
        } else {
            result = loadCloud();
        }
    } else if (record.state == LocalSetupSagaState::Complete) {
        result = sameDigest(bodyDigest, record.cloudCommitAckDigest)
                     ? Result::success()
                     : Result::failure(ErrorCode::StateConflict);
        secureZero(MutableByteSpan(bodyDigest, sizeof(bodyDigest)));
        return result;
    } else {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result && record.state == LocalSetupSagaState::CloudCommitVerified) {
        if (cloud_->state() ==
            CloudCredentialEnrollmentState::AwaitingCommitAck) {
            result = cloud_->finishVerifiedCommit();
        } else if (cloud_->state() != CloudCredentialEnrollmentState::Active) {
            result = Result::failure(ErrorCode::StateConflict);
        }
        if (result) {
            record.state = LocalSetupSagaState::Complete;
            result = journal_.save(record);
        }
    }
    secureZero(MutableByteSpan(bodyDigest, sizeof(bodyDigest)));
    return result;
}

Result LocalSetupSaga::handleAuthorizedOperation(
    const LocalSetupOperationContext& context,
    LocalSetupOperation operation,
    ByteView requestBody,
    MutableByteSpan operationScratch,
    MutableByteSpan responseBodyStorage,
    ByteView& responseBody) {
    responseBody = ByteView();
    if (!authorizedSession_ || operationScratch.data == nullptr ||
        operationScratch.size < kLocalSetupOperationScratchSize ||
        responseBodyStorage.data == nullptr ||
        responseBodyStorage.size < kLocalSetupOperationResponseMaxEncodedSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    LocalSetupSagaRecord record;
    Result result = journal_.load(record);
    if (result) result = validateContext(record, context.authorization);
    if (result) {
        switch (operation) {
        case LocalSetupOperation::BeginOwnershipClaim:
            if (!requestBody.empty()) {
                result = Result::failure(ErrorCode::InvalidEncoding);
            } else {
                result = handleBeginClaim(record,
                                          responseBodyStorage,
                                          responseBody);
            }
            break;
        case LocalSetupOperation::ApplyOwnershipGrant:
            result = handleApplyClaimGrant(record,
                                           requestBody,
                                           operationScratch,
                                           responseBodyStorage,
                                           responseBody);
            break;
        case LocalSetupOperation::CompleteOwnershipClaim:
            result = handleCompleteClaim(record,
                                         requestBody,
                                         operationScratch,
                                         responseBody);
            break;
        case LocalSetupOperation::InstallInitialController:
            result = handleInstallController(record,
                                             requestBody,
                                             operationScratch,
                                             responseBodyStorage,
                                             responseBody);
            break;
        case LocalSetupOperation::ConfigureNetwork:
            result = handleConfigureNetwork(record,
                                            requestBody,
                                            responseBodyStorage,
                                            responseBody);
            break;
        case LocalSetupOperation::BeginCloudEnrollment:
            result = handleBeginCloud(record,
                                      requestBody,
                                      operationScratch,
                                      responseBodyStorage,
                                      responseBody);
            break;
        case LocalSetupOperation::ApplyCloudEnrollmentGrant:
            result = handleApplyCloudGrant(record,
                                           requestBody,
                                           operationScratch,
                                           responseBodyStorage,
                                           responseBody);
            break;
        case LocalSetupOperation::CompleteCloudEnrollment:
            result = handleCompleteCloud(record,
                                         requestBody,
                                         operationScratch,
                                         responseBody);
            break;
        default:
            result = Result::failure(ErrorCode::UnsupportedFeature);
            break;
        }
    }
    clearLocalSetupSagaRecord(record);
    return result;
}

} // namespace blinker
