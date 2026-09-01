#include "BleEnrollmentApplication.h"

#include <stdint.h>
#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool exactNonZero(ByteView value, size_t expected) {
    if (value.data == nullptr || value.size != expected) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool overlaps(ByteView left, MutableByteSpan right) {
    if (left.empty() || right.empty() || left.data == nullptr ||
        right.data == nullptr) {
        return false;
    }
    const uintptr_t leftBegin = reinterpret_cast<uintptr_t>(left.data);
    const uintptr_t rightBegin = reinterpret_cast<uintptr_t>(right.data);
    return left.size <= UINTPTR_MAX - leftBegin &&
                   right.size <= UINTPTR_MAX - rightBegin
               ? leftBegin < rightBegin + right.size &&
                     rightBegin < leftBegin + left.size
               : true;
}

bool overlaps(MutableByteSpan left, MutableByteSpan right) {
    return overlaps(ByteView(left.data, left.size), right);
}

uint32_t readU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool validConfig(const BleEnrollmentApplicationConfig& config) {
    return config.serverKeyId != 0U &&
           (config.securityProfile ==
                BleEnrollmentSecurityProfile::EducationNearby ||
            config.securityProfile == BleEnrollmentSecurityProfile::ProductOob) &&
           (config.signatureAlgorithm == ServerSignatureAlgorithm::Ed25519 ||
            config.signatureAlgorithm ==
                ServerSignatureAlgorithm::EcdsaP256Sha256Raw);
}

} // namespace

BleEnrollmentApplication::BleEnrollmentApplication(
    const DeviceInstanceId& deviceInstanceId,
    IRandom& random,
    IClock& clock,
    BleEnrollmentGrantVerifier& grantVerifier,
    IDeviceAccessStore& accessStore,
    const BleEnrollmentApplicationConfig& config)
    : deviceInstanceId_(deviceInstanceId), random_(random), clock_(clock),
      grantVerifier_(grantVerifier), accessStore_(accessStore),
      config_(config),
      accessEpoch_(0U), lastRequestId_(0U), setupSessionLocator_(),
      setupSessionId_(), setupTranscriptHash_(), lastRequestDigest_(),
      state_(BleEnrollmentApplicationState::Idle), windowValid_(false) {}

BleEnrollmentApplication::~BleEnrollmentApplication() { clearWindow(); }

Result BleEnrollmentApplication::beginSession(ByteView locator) {
    clearConnection();
    if (!validConfig(config_)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (!isValidDeviceInstanceId(deviceInstanceId_) ||
        !exactNonZero(locator, kSetupSessionLocatorSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    uint32_t installedEpoch = 0U;
    Result result = accessStore_.loadAccessEpoch(installedEpoch);
    if (result) return Result::failure(ErrorCode::StateConflict);
    if (result.code() != ErrorCode::NotFound) return result;

    const bool sameWindow =
        windowValid_ && constantTimeEqual(
            locator,
            ByteView(setupSessionLocator_, sizeof(setupSessionLocator_)));
    if (!sameWindow) {
        clearWindow();
        result = random_.fill(
            MutableByteSpan(setupSessionId_, sizeof(setupSessionId_)));
        uint8_t epochBytes[sizeof(accessEpoch_)] = {};
        if (result) {
            result = random_.fill(
                MutableByteSpan(epochBytes, sizeof(epochBytes)));
        }
        if (result) accessEpoch_ = readU32(epochBytes);
        secureZero(MutableByteSpan(epochBytes, sizeof(epochBytes)));
        if (!result || accessEpoch_ == 0U ||
            !exactNonZero(
                ByteView(setupSessionId_, sizeof(setupSessionId_)),
                sizeof(setupSessionId_))) {
            clearWindow();
            return result ? Result::failure(ErrorCode::InternalError) : result;
        }
        memcpy(setupSessionLocator_, locator.data, sizeof(setupSessionLocator_));
        windowValid_ = true;
    }
    state_ = BleEnrollmentApplicationState::AwaitingSecureSession;
    return Result::success();
}

Result BleEnrollmentApplication::secureSessionReady(ByteView transcriptHash) {
    if (state_ != BleEnrollmentApplicationState::AwaitingSecureSession) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (!exactNonZero(transcriptHash, sizeof(setupTranscriptHash_))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    memcpy(setupTranscriptHash_,
           transcriptHash.data,
           sizeof(setupTranscriptHash_));
    state_ = lastRequestId_ == 0U
        ? BleEnrollmentApplicationState::AwaitingHello
        : BleEnrollmentApplicationState::AwaitingEnrollment;
    return Result::success();
}

void BleEnrollmentApplication::endSession() { clearConnection(); }

BleEnrollmentApplication::SequenceDisposition
BleEnrollmentApplication::classify(uint32_t requestId, ByteView digest) const {
    if (lastRequestId_ == 0U ||
        (lastRequestId_ != UINT32_MAX && requestId == lastRequestId_ + 1U)) {
        return SequenceDisposition::NewRequest;
    }
    if (requestId == lastRequestId_) {
        return constantTimeEqual(
                   digest,
                   ByteView(lastRequestDigest_, sizeof(lastRequestDigest_)))
                   ? SequenceDisposition::ExactReplay
                   : SequenceDisposition::Conflict;
    }
    return SequenceDisposition::Gap;
}

void BleEnrollmentApplication::remember(uint32_t requestId, ByteView digest) {
    lastRequestId_ = requestId;
    memcpy(lastRequestDigest_, digest.data, sizeof(lastRequestDigest_));
}

Result BleEnrollmentApplication::encodeError(
    uint32_t requestId,
    ErrorCode error,
    MutableByteSpan output,
    ByteView& response) const {
    BleEnrollmentWireError wireError;
    BleEnrollmentRetryClass retryClass;
    bleEnrollmentErrorFor(error, wireError, retryClass);
    return encodeBleEnrollmentError(
        requestId, wireError, retryClass, output, response);
}

Result BleEnrollmentApplication::handleHello(uint32_t requestId,
                                             MutableByteSpan output,
                                             ByteView& response) {
    BleEnrollmentResponse hello;
    hello.type = BleEnrollmentMessageType::HelloResponse;
    hello.requestId = requestId;
    hello.deviceInstanceId =
        ByteView(deviceInstanceId_.bytes, sizeof(deviceInstanceId_.bytes));
    hello.setupSessionId =
        ByteView(setupSessionId_, sizeof(setupSessionId_));
    hello.setupSessionLocator =
        ByteView(setupSessionLocator_, sizeof(setupSessionLocator_));
    hello.accessEpoch = accessEpoch_;
    hello.securityProfile = config_.securityProfile;
    hello.serverKeyId = config_.serverKeyId;
    hello.signatureAlgorithm = config_.signatureAlgorithm;
    Result result = encodeBleEnrollmentHelloResponse(hello, output, response);
    if (result && state_ == BleEnrollmentApplicationState::AwaitingHello) {
        state_ = BleEnrollmentApplicationState::AwaitingEnrollment;
    }
    return result;
}

Result BleEnrollmentApplication::handleEnrollment(
    const BleEnrollmentRequest& request,
    bool exactReplay,
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& response) {
    BleEnrollmentGrantContext context;
    context.deviceInstanceId =
        ByteView(deviceInstanceId_.bytes, sizeof(deviceInstanceId_.bytes));
    context.setupSessionId =
        ByteView(setupSessionId_, sizeof(setupSessionId_));
    context.setupTranscriptHash =
        ByteView(setupTranscriptHash_, sizeof(setupTranscriptHash_));
    context.accessEpoch = accessEpoch_;
    context.securityProfile = config_.securityProfile;
    context.serverKeyId = config_.serverKeyId;
    context.signatureAlgorithm = config_.signatureAlgorithm;
    context.hasTrustedTime = !exactReplay && clock_.unixTime(context.nowEpochSeconds);

    BleEnrollmentGrant grant;
    Result result = grantVerifier_.verify(
        request.grant, context, workspace, grant);
    uint8_t secretDigest[kSha256Size] = {};
    uint8_t presenceDigest[kSha256Size] = {};
    if (result) {
        result = sha256(
            request.controllerSecret,
            MutableByteSpan(secretDigest, sizeof(secretDigest)));
    }
    if (result && !constantTimeEqual(
                      ByteView(secretDigest, sizeof(secretDigest)),
                      grant.controllerSecretDigest)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (result) {
        result = sha256(
            request.presenceKey,
            MutableByteSpan(presenceDigest, sizeof(presenceDigest)));
    }
    if (result && !constantTimeEqual(
                      ByteView(presenceDigest, sizeof(presenceDigest)),
                      grant.presenceKeyDigest)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }

    ControllerCredential credential;
    DevicePresenceKey presence;
    if (result) {
        credential.ownershipGeneration = grant.accessEpoch;
        credential.credentialVersion = 1U;
        credential.permissions = grant.controllerPermissions;
        credential.suite = ControllerCredentialSuite::HmacSha256_32;
        credential.domain = ControllerCredentialDomain::PlatformAccess;
        memcpy(credential.controllerId,
               grant.controllerId.data,
               sizeof(credential.controllerId));
        memcpy(credential.secret,
               request.controllerSecret.data,
               sizeof(credential.secret));
        presence.version = grant.presenceKeyVersion;
        memcpy(presence.bytes,
               request.presenceKey.data,
               sizeof(presence.bytes));
        result = accessStore_.bootstrapAccessWithPresence(
            grant.accessEpoch, credential, presence);
    }

    uint8_t proof[kControllerReceiptProofSize] = {};
    ControllerMutationReceipt receipt;
    ByteView encodedReceipt;
    if (result) {
        receipt.operation = ControllerMutationOperation::Install;
        receipt.grantId = grant.grantId;
        receipt.deviceInstanceId = context.deviceInstanceId;
        receipt.ownershipGeneration = grant.accessEpoch;
        receipt.controllerId = grant.controllerId;
        receipt.credentialVersion = 1U;
        receipt.permissions = grant.controllerPermissions;
        receipt.secretDigest = ByteView(secretDigest, sizeof(secretDigest));
        receipt.proofKind = ControllerReceiptProofKind::HmacSha256;
        result = computeControllerMutationReceiptProof(
            request.controllerSecret,
            receipt,
            workspace,
            MutableByteSpan(proof, sizeof(proof)));
    }
    if (result) {
        receipt.proof = ByteView(proof, sizeof(proof));
        result = encodeControllerMutationReceipt(
            receipt, workspace, encodedReceipt);
    }
    if (result) {
        result = encodeBleEnrollmentResponse(
            request.requestId, encodedReceipt, output, response);
    }
    if (result) state_ = BleEnrollmentApplicationState::Enrolled;

    clearControllerCredential(credential);
    clearDevicePresenceKey(presence);
    secureZero(MutableByteSpan(secretDigest, sizeof(secretDigest)));
    secureZero(MutableByteSpan(presenceDigest, sizeof(presenceDigest)));
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    return result;
}

Result BleEnrollmentApplication::handle(ByteView encodedRequest,
                                        MutableByteSpan workspace,
                                        MutableByteSpan output,
                                        ByteView& response) {
    response = ByteView();
    if (state_ == BleEnrollmentApplicationState::Idle ||
        workspace.data == nullptr ||
        workspace.size < kBleEnrollmentWorkspaceSize ||
        output.data == nullptr ||
        output.size < kBleEnrollmentResponseMaxEncodedSize ||
        overlaps(encodedRequest, workspace) ||
        overlaps(encodedRequest, output) || overlaps(workspace, output)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    BleEnrollmentRequest request;
    Result result = decodeBleEnrollmentRequest(encodedRequest, request);
    uint8_t requestDigest[kSha256Size] = {};
    if (result) {
        result = sha256(encodedRequest,
                        MutableByteSpan(requestDigest, sizeof(requestDigest)));
    }
    if (!result) {
        secureZero(workspace);
        secureZero(MutableByteSpan(requestDigest, sizeof(requestDigest)));
        return result;
    }

    const ByteView digest(requestDigest, sizeof(requestDigest));
    const SequenceDisposition sequence = classify(request.requestId, digest);
    if (sequence == SequenceDisposition::Conflict) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    } else if (sequence == SequenceDisposition::Gap) {
        result = encodeError(
            request.requestId, ErrorCode::SequenceConflict, output, response);
    } else {
        const bool replay = sequence == SequenceDisposition::ExactReplay;
        bool consumed = false;
        if (request.type == BleEnrollmentMessageType::HelloRequest &&
            ((!replay &&
              (state_ == BleEnrollmentApplicationState::AwaitingSecureSession ||
               state_ == BleEnrollmentApplicationState::AwaitingHello)) ||
             (replay &&
              state_ >= BleEnrollmentApplicationState::AwaitingEnrollment))) {
            result = handleHello(request.requestId, output, response);
            consumed = true;
        } else if (request.type ==
                       BleEnrollmentMessageType::EnrollRequest &&
                   ((!replay && state_ ==
                                    BleEnrollmentApplicationState::AwaitingEnrollment) ||
                    (replay && state_ ==
                                   BleEnrollmentApplicationState::Enrolled))) {
            result = handleEnrollment(
                request, replay, workspace, output, response);
            consumed = result.ok();
        } else {
            result = encodeError(
                request.requestId, ErrorCode::StateConflict, output, response);
        }
        if (result && !replay && consumed) {
            remember(request.requestId, digest);
        }
    }

    secureZero(workspace);
    secureZero(MutableByteSpan(requestDigest, sizeof(requestDigest)));
    return result;
}

void BleEnrollmentApplication::clearConnection() {
    lastRequestId_ = 0U;
    secureZero(MutableByteSpan(
        setupTranscriptHash_, sizeof(setupTranscriptHash_)));
    secureZero(MutableByteSpan(lastRequestDigest_, sizeof(lastRequestDigest_)));
    state_ = BleEnrollmentApplicationState::Idle;
}

void BleEnrollmentApplication::clearWindow() {
    clearConnection();
    secureZero(MutableByteSpan(
        setupSessionLocator_, sizeof(setupSessionLocator_)));
    secureZero(MutableByteSpan(setupSessionId_, sizeof(setupSessionId_)));
    accessEpoch_ = 0U;
    windowValid_ = false;
}

} // namespace blinker
