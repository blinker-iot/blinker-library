#include "LocalControllerPairingCoordinator.h"

#include <stdint.h>
#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"

namespace blinker {

namespace {

bool nonZero(ByteView value, size_t expected) {
    if (value.data == nullptr || value.size != expected) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool overlaps(ByteView input, MutableByteSpan output) {
    if (input.empty() || output.empty() || input.data == nullptr ||
        output.data == nullptr) {
        return false;
    }
    const uintptr_t inputBegin = reinterpret_cast<uintptr_t>(input.data);
    const uintptr_t outputBegin = reinterpret_cast<uintptr_t>(output.data);
    if (input.size > UINTPTR_MAX - inputBegin ||
        output.size > UINTPTR_MAX - outputBegin) {
        return true;
    }
    return inputBegin < outputBegin + output.size &&
           outputBegin < inputBegin + input.size;
}

} // namespace

LocalControllerPairingCoordinator::LocalControllerPairingCoordinator(
    const DeviceInstanceId& deviceInstanceId,
    IControllerCredentialStore& credentials,
    IRandom& random,
    IClock& clock,
    const LocalControllerPairingConfig& config)
    : deviceInstanceId_(deviceInstanceId),
      credentials_(credentials),
      random_(random),
      clock_(clock),
      config_(config),
      pairingNonce_(),
      setupTranscriptHash_(),
      appliedRequestDigest_(),
      startedAtMillis_(0U),
      active_(false),
      applied_(false) {}

LocalControllerPairingCoordinator::~LocalControllerPairingCoordinator() {
    endPairing();
}

Result LocalControllerPairingCoordinator::beginConfirmedPairing(
    ByteView setupTranscriptHash) {
    if (!isValidDeviceInstanceId(deviceInstanceId_) ||
        !nonZero(
            setupTranscriptHash,
            kLocalControllerPairingTranscriptHashSize) ||
        !validAuthorizationPermissions(config_.permissions) ||
        config_.windowMillis == 0U ||
        config_.windowMillis > kLocalControllerPairingMaxWindowMillis) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (active_) {
        Result result = requireActive();
        if (result) return Result::failure(ErrorCode::AlreadyExists);
    }

    endPairing();
    Result result = random_.fill(
        MutableByteSpan(pairingNonce_, sizeof(pairingNonce_)));
    if (!result || !nonZero(
            ByteView(pairingNonce_, sizeof(pairingNonce_)),
            sizeof(pairingNonce_))) {
        endPairing();
        return result ? Result::failure(ErrorCode::InternalError) : result;
    }
    memcpy(setupTranscriptHash_,
           setupTranscriptHash.data,
           sizeof(setupTranscriptHash_));
    startedAtMillis_ = clock_.monotonicMillis();
    active_ = true;
    return Result::success();
}

uint32_t LocalControllerPairingCoordinator::remainingMillis() const {
    if (!active_) return 0U;
    const uint32_t elapsed =
        clock_.monotonicMillis() - startedAtMillis_;
    return elapsed >= config_.windowMillis
               ? 0U
               : config_.windowMillis - elapsed;
}

Result LocalControllerPairingCoordinator::requireActive() {
    if (!active_) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (remainingMillis() == 0U) {
        endPairing();
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    return Result::success();
}

Result LocalControllerPairingCoordinator::challenge(
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = requireActive();
    if (!result) return result;
    LocalControllerPairingChallenge body;
    body.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    body.pairingNonce = ByteView(pairingNonce_, sizeof(pairingNonce_));
    body.setupTranscriptHash = ByteView(
        setupTranscriptHash_,
        sizeof(setupTranscriptHash_));
    body.permissions = config_.permissions;
    body.expiresInMillis = remainingMillis();
    return encodeLocalControllerPairingChallenge(body, output, encoded);
}

Result LocalControllerPairingCoordinator::apply(
    ByteView encodedRequest,
    MutableByteSpan output,
    ByteView& encodedReceipt) {
    encodedReceipt = ByteView();
    Result result = requireActive();
    if (!result) return result;
    if (output.data == nullptr || overlaps(encodedRequest, output)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    LocalControllerPairingRequest request;
    result = decodeLocalControllerPairingRequest(encodedRequest, request);
    uint8_t requestDigest[kSha256Size] = {};
    if (result) {
        result = sha256(
            encodedRequest,
            MutableByteSpan(requestDigest, sizeof(requestDigest)));
    }
    if (result &&
        (!constantTimeEqual(
             request.deviceInstanceId,
             ByteView(
                 deviceInstanceId_.bytes,
                 sizeof(deviceInstanceId_.bytes))) ||
         !constantTimeEqual(
             request.pairingNonce,
             ByteView(pairingNonce_, sizeof(pairingNonce_))) ||
         !constantTimeEqual(
             request.setupTranscriptHash,
             ByteView(
                 setupTranscriptHash_,
                 sizeof(setupTranscriptHash_))) ||
         request.permissions != config_.permissions)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (result && applied_ && !constantTimeEqual(
            ByteView(
                appliedRequestDigest_,
                sizeof(appliedRequestDigest_)),
            ByteView(requestDigest, sizeof(requestDigest)))) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }

    ControllerCredential credential;
    if (result) {
        credential.domain = ControllerCredentialDomain::Local;
        credential.ownershipGeneration = 0U;
        credential.credentialVersion = request.credentialVersion;
        credential.permissions = request.permissions;
        memcpy(credential.controllerId,
               request.controllerId.data,
               sizeof(credential.controllerId));
        memcpy(credential.secret,
               request.controllerSecret.data,
               sizeof(credential.secret));
        result = credentials_.installVerified(credential);
    }
    // Persistence is the mutation boundary. Lock this physical window to the
    // exact request immediately, even if receipt encoding or delivery later
    // fails; otherwise a different request could install a second identity
    // after response loss.
    if (result && !applied_) {
        memcpy(appliedRequestDigest_,
               requestDigest,
               sizeof(appliedRequestDigest_));
        applied_ = true;
    }

    uint8_t secretDigest[kSha256Size] = {};
    uint8_t proof[kLocalControllerPairingProofSize] = {};
    LocalControllerPairingReceipt receipt;
    if (result) {
        result = sha256(
            request.controllerSecret,
            MutableByteSpan(secretDigest, sizeof(secretDigest)));
    }
    if (result) {
        receipt.deviceInstanceId = request.deviceInstanceId;
        receipt.pairingNonce = request.pairingNonce;
        receipt.requestDigest = ByteView(requestDigest, sizeof(requestDigest));
        receipt.controllerId = request.controllerId;
        receipt.credentialVersion = request.credentialVersion;
        receipt.permissions = request.permissions;
        receipt.secretDigest = ByteView(secretDigest, sizeof(secretDigest));
        result = computeLocalControllerPairingReceiptProof(
            request.controllerSecret,
            receipt,
            MutableByteSpan(proof, sizeof(proof)));
    }
    if (result) {
        receipt.proof = ByteView(proof, sizeof(proof));
        result = encodeLocalControllerPairingReceipt(
            receipt,
            output,
            encodedReceipt);
    }
    clearControllerCredential(credential);
    secureZero(MutableByteSpan(requestDigest, sizeof(requestDigest)));
    secureZero(MutableByteSpan(secretDigest, sizeof(secretDigest)));
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    return result;
}

void LocalControllerPairingCoordinator::endPairing() {
    secureZero(MutableByteSpan(pairingNonce_, sizeof(pairingNonce_)));
    secureZero(MutableByteSpan(
        setupTranscriptHash_,
        sizeof(setupTranscriptHash_)));
    secureZero(MutableByteSpan(
        appliedRequestDigest_,
        sizeof(appliedRequestDigest_)));
    startedAtMillis_ = 0U;
    active_ = false;
    applied_ = false;
}

} // namespace blinker
