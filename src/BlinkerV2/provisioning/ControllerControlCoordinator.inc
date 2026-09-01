#include "ControllerControlCoordinator.h"

#include <stdint.h>
#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool validControlNonce(ByteView value) {
    if (value.data == nullptr ||
        value.size != kControllerControlNonceSize) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool overlaps(ByteView input, MutableByteSpan storage) {
    if (input.empty() || storage.empty() || input.data == nullptr ||
        storage.data == nullptr) {
        return false;
    }
    const uintptr_t inputBegin = reinterpret_cast<uintptr_t>(input.data);
    const uintptr_t storageBegin = reinterpret_cast<uintptr_t>(storage.data);
    if (input.size > UINTPTR_MAX - inputBegin ||
        storage.size > UINTPTR_MAX - storageBegin) {
        return true;
    }
    return inputBegin < storageBegin + storage.size &&
           storageBegin < inputBegin + input.size;
}

} // namespace

ControllerControlCoordinator::ControllerControlCoordinator(
    const DeviceInstanceId& deviceInstanceId,
    IAccessEpochSource& accessEpoch,
    IControllerCredentialStore& credentials,
    ControllerGrantVerifier& verifier,
    IDeviceAccessStore* presenceAccess)
    : deviceInstanceId_(deviceInstanceId),
      accessEpoch_(accessEpoch),
      credentials_(credentials),
      verifier_(verifier),
      presenceAccess_(presenceAccess),
      controlNonce_(),
      controlWindowActive_(false) {}

ControllerControlCoordinator::~ControllerControlCoordinator() {
    endControlWindow();
}

Result ControllerControlCoordinator::beginControlWindow(
    ByteView controlNonce) {
    if (!validControlNonce(controlNonce)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (controlWindowActive_) {
        return constantTimeEqual(
                   ByteView(controlNonce_, sizeof(controlNonce_)),
                   controlNonce)
                   ? Result::success()
                   : Result::failure(ErrorCode::StateConflict);
    }
    memcpy(controlNonce_, controlNonce.data, sizeof(controlNonce_));
    controlWindowActive_ = true;
    return Result::success();
}

void ControllerControlCoordinator::endControlWindow() {
    secureZero(MutableByteSpan(controlNonce_, sizeof(controlNonce_)));
    controlWindowActive_ = false;
}

Result ControllerControlCoordinator::apply(
    ByteView encodedGrant,
    ByteView controllerSecret,
    MutableByteSpan operationWorkspace,
    MutableByteSpan output,
    ByteView& receipt) {
    receipt = ByteView();
    if (!isValidDeviceInstanceId(deviceInstanceId_) ||
        !controlWindowActive_ ||
        operationWorkspace.data == nullptr ||
        operationWorkspace.size < kControllerControlWorkspaceSize ||
        output.data == nullptr ||
        overlaps(encodedGrant, operationWorkspace) ||
        overlaps(encodedGrant, output) ||
        overlaps(
            ByteView(operationWorkspace.data, operationWorkspace.size),
            output)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    secureZero(operationWorkspace);

    uint32_t accessEpoch = 0U;
    Result result = accessEpoch_.loadAccessEpoch(accessEpoch);
    if (!result) return result;
    if (accessEpoch == 0U) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }

    ControllerGrantVerificationContext context;
    context.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    // The v2 wire name remains ownershipGeneration for compatibility; its
    // device-side meaning is strictly the direct-access epoch.
    context.ownershipGeneration = accessEpoch;
    context.controlNonce = ByteView(controlNonce_, sizeof(controlNonce_));
    ControllerGrant grant;
    result = verifier_.verify(
        encodedGrant,
        controllerSecret,
        context,
        operationWorkspace,
        grant);
    if (!result) return result;

    ControllerCredential credential;
    if (grant.operation != ControllerMutationOperation::Revoke) {
        credential.domain = ControllerCredentialDomain::PlatformAccess;
        credential.ownershipGeneration = grant.ownershipGeneration;
        credential.credentialVersion = grant.credentialVersion;
        credential.permissions = grant.permissions;
        credential.suite = ControllerCredentialSuite::HmacSha256_32;
        memcpy(
            credential.controllerId,
            grant.controllerId.data,
            sizeof(credential.controllerId));
        memcpy(
            credential.secret,
            controllerSecret.data,
            sizeof(credential.secret));
    }

    ControllerMutationReceipt body;
    body.operation = grant.operation;
    body.grantId = grant.grantId;
    body.deviceInstanceId = grant.deviceInstanceId;
    body.ownershipGeneration = grant.ownershipGeneration;
    body.controllerId = grant.controllerId;
    body.credentialVersion = grant.credentialVersion;
    body.permissions = grant.permissions;
    body.secretDigest = grant.secretDigest;
    uint8_t proof[kControllerReceiptProofSize] = {};
    if (grant.operation == ControllerMutationOperation::Revoke) {
        body.proofKind = ControllerReceiptProofKind::None;
        body.proof = ByteView();
    } else {
        body.proofKind = ControllerReceiptProofKind::HmacSha256;
        result = computeControllerMutationReceiptProof(
            controllerSecret,
            body,
            operationWorkspace,
            MutableByteSpan(proof, sizeof(proof)));
        if (result) body.proof = ByteView(proof, sizeof(proof));
    }
    if (result) {
        result = encodeControllerMutationReceipt(body, output, receipt);
    }
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    secureZero(operationWorkspace);
    // Complete every fallible proof/encoding step before durable mutation.
    // Otherwise a small output buffer or crypto failure could update storage
    // without returning the deterministic receipt needed for recovery.
    if (result && grant.operation == ControllerMutationOperation::Install) {
        result = credentials_.installVerified(credential);
    } else if (result &&
               grant.operation == ControllerMutationOperation::Rotate) {
        result = credentials_.rotateVerified(
            grant.expectedCredentialVersion,
            credential);
    } else if (result) {
        result = credentials_.revokeVerified(
            grant.controllerId,
            ControllerCredentialDomain::PlatformAccess,
            grant.ownershipGeneration,
            grant.expectedCredentialVersion);
    }
    if (!result) receipt = ByteView();
    clearControllerCredential(credential);
    return result;
}

Result ControllerControlCoordinator::applyPresence(
    ByteView encodedMutation,
    MutableByteSpan output,
    ByteView& receipt) {
    receipt = ByteView();
    if (presenceAccess_ == nullptr || output.data == nullptr ||
        output.size < kPresenceKeyReceiptMaxEncodedSize) {
        return Result::failure(
            presenceAccess_ == nullptr
                ? ErrorCode::UnsupportedFeature
                : ErrorCode::BufferTooSmall);
    }
    PresenceKeyMutation mutation;
    Result result = decodePresenceKeyMutation(encodedMutation, mutation);
    uint32_t accessEpoch = 0U;
    if (result) result = accessEpoch_.loadAccessEpoch(accessEpoch);
    if (result && accessEpoch != mutation.accessEpoch) {
        result = Result::failure(ErrorCode::StateConflict);
    }

    uint8_t digest[kPresenceKeyDigestSize] = {};
    uint8_t proof[kPresenceKeyReceiptProofSize] = {};
    PresenceKeyReceipt body;
    body.accessEpoch = mutation.accessEpoch;
    body.expectedVersion = mutation.expectedVersion;
    body.presenceKeyVersion = mutation.presence.version;
    if (result) {
        result = sha256(
            ByteView(
                mutation.presence.bytes,
                sizeof(mutation.presence.bytes)),
            MutableByteSpan(digest, sizeof(digest)));
    }
    body.keyDigest = ByteView(digest, sizeof(digest));
    body.proofKind = 1U;
    if (result) {
        result = computePresenceKeyReceiptProof(
            mutation.presence,
            body,
            MutableByteSpan(digest, sizeof(digest)),
            MutableByteSpan(proof, sizeof(proof)));
    }
    body.proof = ByteView(proof, sizeof(proof));
    if (result) result = encodePresenceKeyReceipt(body, output, receipt);
    // Complete every fallible proof/encoding step before durable mutation.
    if (result) {
        result = presenceAccess_->replacePresenceVerified(
            mutation.accessEpoch,
            mutation.expectedVersion,
            mutation.presence);
    }
    if (!result) receipt = ByteView();
    clearDevicePresenceKey(mutation.presence);
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    return result;
}

} // namespace blinker
