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
    IOwnershipSource& ownership,
    IControllerCredentialStore& credentials,
    ControllerGrantVerifier& verifier)
    : deviceInstanceId_(deviceInstanceId),
      ownership_(ownership),
      credentials_(credentials),
      verifier_(verifier),
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
    uint64_t nowEpochSeconds,
    bool hasTrustedTime,
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

    OwnershipRecord ownership;
    Result result = ownership_.load(ownership);
    if (!result) return result;
    if (!ownership.active()) {
        clearOwnershipRecord(ownership);
        return Result::failure(ErrorCode::AuthenticationRequired);
    }

    ControllerGrantVerificationContext context;
    context.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    context.logicalDeviceId = ownership.logicalId();
    context.ownershipGeneration = ownership.generation;
    context.controlNonce = ByteView(controlNonce_, sizeof(controlNonce_));
    context.nowEpochSeconds = nowEpochSeconds;
    context.hasTrustedTime = hasTrustedTime;
    ControllerGrant grant;
    result = verifier_.verify(
        encodedGrant,
        controllerSecret,
        context,
        operationWorkspace,
        grant);
    clearOwnershipRecord(ownership);
    if (!result) return result;

    ControllerCredential credential;
    if (grant.operation != ControllerMutationOperation::Revoke) {
        credential.domain = grant.domain;
        credential.ownershipGeneration = grant.ownershipGeneration;
        credential.credentialVersion = grant.credentialVersion;
        credential.permissions = grant.permissions;
        credential.suite = grant.suite;
        memcpy(
            credential.controllerId,
            grant.controllerId.data,
            sizeof(credential.controllerId));
        memcpy(
            credential.secret,
            controllerSecret.data,
            sizeof(credential.secret));
    }

    if (grant.operation == ControllerMutationOperation::Install) {
        result = credentials_.installVerified(credential);
    } else if (grant.operation == ControllerMutationOperation::Rotate) {
        result = credentials_.rotateVerified(
            grant.expectedCredentialVersion,
            credential);
    } else {
        result = credentials_.revokeVerified(
            grant.controllerId,
            grant.domain,
            grant.ownershipGeneration,
            grant.expectedCredentialVersion);
    }
    if (!result) {
        clearControllerCredential(credential);
        return result;
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
    clearControllerCredential(credential);
    return result;
}

} // namespace blinker
