#include "WifiOnboardingFlow.h"

#include <limits.h>
#include <stdint.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

ByteView bytes(MutableByteSpan value) {
    return ByteView(value.data, value.size);
}

bool timeReached(uint32_t now, uint32_t target) {
    return static_cast<int32_t>(now - target) >= 0;
}

} // namespace

WifiOnboardingFlow::WifiOnboardingFlow(
    OwnershipClaimCoordinator& claim,
    CloudCredentialEnrollmentCoordinator& cloud,
    EnrollmentKeyHttpExchange& http,
    IClock& clock)
    : claim_(claim),
      cloud_(cloud),
      http_(http),
      clock_(clock),
      requestBuffer_(),
      responseBuffer_(),
      workspace_(),
      retryAtMillis_(0U),
      retryAfterSeconds_(0U),
      lastWireError_(0U),
      state_(WifiOnboardingState::Stopped),
      failedPhase_(WifiOnboardingState::Stopped),
      lastError_(ErrorCode::Ok),
      retryClass_(WifiOnboardingRetryClass::Never),
      hasWireError_(false),
      cloudWireError_(false),
      hasRetryAfter_(false) {}

WifiOnboardingFlow::~WifiOnboardingFlow() {
    stop();
}

bool WifiOnboardingFlow::overlaps(ByteView first, ByteView second) {
    if (first.data == nullptr || second.data == nullptr || first.empty() ||
        second.empty()) {
        return false;
    }
    const uintptr_t firstBegin = reinterpret_cast<uintptr_t>(first.data);
    const uintptr_t secondBegin = reinterpret_cast<uintptr_t>(second.data);
    if (first.size > UINTPTR_MAX - firstBegin ||
        second.size > UINTPTR_MAX - secondBegin) {
        return true;
    }
    return firstBegin < secondBegin + second.size &&
           secondBegin < firstBegin + first.size;
}

Result WifiOnboardingFlow::validateBuffers() const {
    if (requestBuffer_.data == nullptr ||
        requestBuffer_.size < kWifiOnboardingMinimumRequestBufferSize ||
        responseBuffer_.data == nullptr ||
        responseBuffer_.size < kWifiOnboardingMinimumResponseBufferSize ||
        workspace_.data == nullptr ||
        workspace_.size < kWifiOnboardingMinimumWorkspaceSize ||
        overlaps(bytes(requestBuffer_), bytes(responseBuffer_)) ||
        overlaps(bytes(requestBuffer_), bytes(workspace_)) ||
        overlaps(bytes(responseBuffer_), bytes(workspace_))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result WifiOnboardingFlow::start(
    MutableByteSpan requestBuffer,
    MutableByteSpan responseBuffer,
    MutableByteSpan workspace) {
    if (state_ != WifiOnboardingState::Stopped) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    requestBuffer_ = requestBuffer;
    responseBuffer_ = responseBuffer;
    workspace_ = workspace;
    Result result = validateBuffers();
    if (!result) {
        requestBuffer_ = MutableByteSpan();
        responseBuffer_ = MutableByteSpan();
        workspace_ = MutableByteSpan();
        return result;
    }
    clearBuffers();
    clearError();
    return resume();
}

Result WifiOnboardingFlow::resume() {
    ByteView body;
    Result result = claim_.beginClaim(requestBuffer_, body);
    if (result) {
        return send(
            WifiOnboardingState::ClaimGrantPending,
            wifiOwnershipClaimTarget(),
            body);
    }
    if (result.code() != ErrorCode::StateConflict) {
        fail(
            result.code(),
            WifiOnboardingRetryClass::Never,
            WifiOnboardingState::ClaimGrantPending);
        return result;
    }
    return beginCloud();
}

Result WifiOnboardingFlow::beginCloud() {
    Result result = cloud_.load();
    if (!result) {
        fail(
            result.code(),
            WifiOnboardingRetryClass::Never,
            WifiOnboardingState::CloudGrantPending);
        return result;
    }
    if (cloud_.state() == CloudCredentialEnrollmentState::Ready) {
        result = cloud_.prepare();
        if (!result) {
            fail(
                result.code(),
                WifiOnboardingRetryClass::Never,
                WifiOnboardingState::CloudGrantPending);
            return result;
        }
    }

    ByteView body;
    if (cloud_.state() == CloudCredentialEnrollmentState::Prepared) {
        result = cloud_.request(workspace_, requestBuffer_, body);
        if (result) {
            return send(
                WifiOnboardingState::CloudGrantPending,
                wifiCloudCredentialTarget(),
                body);
        }
    } else if (cloud_.state() ==
               CloudCredentialEnrollmentState::AwaitingCommitAck) {
        result = cloud_.receipt(workspace_, requestBuffer_, body);
        if (result) {
            return send(
                WifiOnboardingState::CloudCommitAckPending,
                wifiCloudCredentialReceiptTarget(),
                body);
        }
    } else if (cloud_.state() == CloudCredentialEnrollmentState::Active) {
        complete();
        return Result::success();
    } else {
        result = Result::failure(ErrorCode::StateConflict);
    }

    fail(
        result.code(),
        WifiOnboardingRetryClass::Never,
        WifiOnboardingState::CloudGrantPending);
    return result;
}

Result WifiOnboardingFlow::send(
    WifiOnboardingState phase,
    StringView target,
    ByteView body) {
    http_.stop();
    secureZero(responseBuffer_);
    const Result result = http_.start(target, body, responseBuffer_);
    if (!result) {
        fail(result.code(), WifiOnboardingRetryClass::SameRequest, phase);
        return result;
    }
    state_ = phase;
    failedPhase_ = WifiOnboardingState::Stopped;
    lastError_ = ErrorCode::Ok;
    return Result::success();
}

void WifiOnboardingFlow::poll(uint32_t budgetMicros) {
    if (state_ == WifiOnboardingState::Stopped ||
        state_ == WifiOnboardingState::Active ||
        state_ == WifiOnboardingState::Fault) {
        return;
    }
    http_.poll(budgetMicros);
    if (http_.state() == HttpClientState::InProgress) return;
    if (http_.state() == HttpClientState::Complete) {
        processResponse();
        return;
    }
    const ErrorCode error = http_.state() == HttpClientState::Failed
                                ? http_.lastError()
                                : ErrorCode::InternalError;
    fail(error, WifiOnboardingRetryClass::SameRequest, state_);
}

void WifiOnboardingFlow::processResponse() {
    HttpResponse response;
    const Result result = http_.response(response);
    if (!result) {
        fail(result.code(), WifiOnboardingRetryClass::SameRequest, state_);
        return;
    }
    if (response.statusCode >= 300U && response.statusCode < 400U) {
        fail(
            ErrorCode::UnsupportedFeature,
            WifiOnboardingRetryClass::Never,
            state_);
        return;
    }

    const WifiOnboardingState phase = state_;
    if (phase == WifiOnboardingState::ClaimGrantPending) {
        if (response.statusCode == 200U) {
            processClaimGrant(response.body);
        } else {
            processClaimError(response.statusCode, response.body);
        }
    } else if (phase == WifiOnboardingState::ClaimReceiptAckPending) {
        if (response.statusCode == 200U) {
            processClaimCommitAck(response.body);
        } else {
            processClaimError(response.statusCode, response.body);
        }
    } else if (phase == WifiOnboardingState::CloudGrantPending) {
        if (response.statusCode == 200U) {
            processCloudGrant(response.body);
        } else {
            processCloudError(response.statusCode, response.body);
        }
    } else if (phase == WifiOnboardingState::CloudCommitAckPending) {
        if (response.statusCode == 200U) {
            processCloudCommitAck(response.body);
        } else {
            processCloudError(response.statusCode, response.body);
        }
    } else {
        fail(
            ErrorCode::InternalError,
            WifiOnboardingRetryClass::Never,
            phase);
    }
}

void WifiOnboardingFlow::processClaimGrant(ByteView body) {
    uint64_t now = 0U;
    const bool trustedTime = clock_.unixTime(now);
    ByteView receipt;
    Result result = claim_.applyGrant(
        body,
        now,
        trustedTime,
        workspace_,
        requestBuffer_,
        receipt);
    if (!result) {
        fail(
            result.code(),
            WifiOnboardingRetryClass::Never,
            WifiOnboardingState::ClaimGrantPending);
        return;
    }
    (void)send(
        WifiOnboardingState::ClaimReceiptAckPending,
        wifiOwnershipReceiptTarget(),
        receipt);
}

void WifiOnboardingFlow::processClaimCommitAck(ByteView body) {
    Result result = claim_.completeClaim(body, workspace_);
    http_.stop();
    if (result) result = beginCloud();
    if (!result && state_ != WifiOnboardingState::Fault) {
        fail(
            result.code(),
            WifiOnboardingRetryClass::Never,
            WifiOnboardingState::ClaimReceiptAckPending);
    }
}

void WifiOnboardingFlow::processCloudGrant(ByteView body) {
    uint64_t now = 0U;
    const bool trustedTime = clock_.unixTime(now);
    ByteView receipt;
    Result result = cloud_.applyGrant(
        body,
        now,
        trustedTime,
        workspace_,
        requestBuffer_,
        receipt);
    if (!result) {
        fail(
            result.code(),
            WifiOnboardingRetryClass::Never,
            WifiOnboardingState::CloudGrantPending);
        return;
    }
    (void)send(
        WifiOnboardingState::CloudCommitAckPending,
        wifiCloudCredentialReceiptTarget(),
        receipt);
}

void WifiOnboardingFlow::processCloudCommitAck(ByteView body) {
    const Result result = cloud_.acceptCommitAck(body, workspace_);
    if (!result) {
        fail(
            result.code(),
            WifiOnboardingRetryClass::Never,
            WifiOnboardingState::CloudCommitAckPending);
        return;
    }
    complete();
}

void WifiOnboardingFlow::processClaimError(
    uint16_t statusCode,
    ByteView body) {
    WifiOnboardingErrorBody decoded;
    Result result = decodeWifiOnboardingErrorBody(body, decoded);
    if (result && !wifiOnboardingHttpStatusMatches(statusCode, decoded.error)) {
        result = Result::failure(ErrorCode::ProtocolError);
    }
    if (!result) {
        fail(result.code(), WifiOnboardingRetryClass::Never, state_);
        return;
    }
    hasWireError_ = true;
    cloudWireError_ = false;
    lastWireError_ = static_cast<uint16_t>(decoded.error);
    hasRetryAfter_ = decoded.hasRetryAfter;
    retryAfterSeconds_ = decoded.retryAfterSeconds;
    if (decoded.hasRetryAfter) {
        const uint32_t maximumSeconds =
            static_cast<uint32_t>(INT32_MAX) / 1000U;
        const uint32_t seconds = decoded.retryAfterSeconds > maximumSeconds
                                     ? maximumSeconds
                                     : decoded.retryAfterSeconds;
        retryAtMillis_ = clock_.monotonicMillis() + seconds * 1000U;
    }
    fail(mapClaimError(decoded.error), decoded.retryClass, state_);
}

void WifiOnboardingFlow::processCloudError(
    uint16_t statusCode,
    ByteView body) {
    CloudEnrollmentErrorBody decoded;
    Result result = decodeCloudEnrollmentErrorBody(body, decoded);
    if (result && !cloudEnrollmentHttpStatusMatches(statusCode, decoded.error)) {
        result = Result::failure(ErrorCode::ProtocolError);
    }
    if (!result) {
        fail(result.code(), WifiOnboardingRetryClass::Never, state_);
        return;
    }
    hasWireError_ = true;
    cloudWireError_ = true;
    lastWireError_ = static_cast<uint16_t>(decoded.error);
    hasRetryAfter_ = decoded.hasRetryAfter;
    retryAfterSeconds_ = decoded.retryAfterSeconds;
    if (decoded.hasRetryAfter) {
        const uint32_t maximumSeconds =
            static_cast<uint32_t>(INT32_MAX) / 1000U;
        const uint32_t seconds = decoded.retryAfterSeconds > maximumSeconds
                                     ? maximumSeconds
                                     : decoded.retryAfterSeconds;
        retryAtMillis_ = clock_.monotonicMillis() + seconds * 1000U;
    }
    fail(
        mapCloudError(decoded.error),
        mapCloudRetry(decoded.retryClass),
        state_);
}

Result WifiOnboardingFlow::retry() {
    if (state_ != WifiOnboardingState::Fault) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (!retryDue()) return Result::failure(ErrorCode::WouldBlock);
    if (retryClass_ == WifiOnboardingRetryClass::Never ||
        retryClass_ == WifiOnboardingRetryClass::ReplaceEnrollmentKey ||
        retryClass_ == WifiOnboardingRetryClass::ReclaimOwnership) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (retryClass_ == WifiOnboardingRetryClass::RestartTransaction) {
        Result result;
        if (failedPhase_ == WifiOnboardingState::ClaimGrantPending) {
            result = claim_.cancelClaim();
        } else if (failedPhase_ == WifiOnboardingState::CloudGrantPending) {
            result = cloud_.load();
            if (result && cloud_.state() ==
                              CloudCredentialEnrollmentState::Prepared) {
                result = cloud_.restartPrepared();
            } else if (result) {
                result = Result::failure(ErrorCode::StateConflict);
            }
        } else {
            result = Result::failure(ErrorCode::StateConflict);
        }
        if (!result) return result;
    }
    state_ = WifiOnboardingState::Stopped;
    clearBuffers();
    clearError();
    return resume();
}

bool WifiOnboardingFlow::retryDue() const {
    return retryClass_ != WifiOnboardingRetryClass::Backoff ||
           timeReached(clock_.monotonicMillis(), retryAtMillis_);
}

void WifiOnboardingFlow::complete() {
    http_.stop();
    clearBuffers();
    requestBuffer_ = MutableByteSpan();
    responseBuffer_ = MutableByteSpan();
    workspace_ = MutableByteSpan();
    clearError();
    state_ = WifiOnboardingState::Active;
    failedPhase_ = WifiOnboardingState::Stopped;
}

void WifiOnboardingFlow::fail(
    ErrorCode error,
    WifiOnboardingRetryClass retryClass,
    WifiOnboardingState phase) {
    http_.stop();
    clearBuffers();
    state_ = WifiOnboardingState::Fault;
    failedPhase_ = phase;
    lastError_ = error == ErrorCode::Ok ? ErrorCode::InternalError : error;
    retryClass_ = retryClass;
}

void WifiOnboardingFlow::clearBuffers() {
    secureZero(requestBuffer_);
    secureZero(responseBuffer_);
    secureZero(workspace_);
}

void WifiOnboardingFlow::clearError() {
    retryAtMillis_ = 0U;
    retryAfterSeconds_ = 0U;
    lastWireError_ = 0U;
    failedPhase_ = WifiOnboardingState::Stopped;
    lastError_ = ErrorCode::Ok;
    retryClass_ = WifiOnboardingRetryClass::Never;
    hasWireError_ = false;
    cloudWireError_ = false;
    hasRetryAfter_ = false;
}

void WifiOnboardingFlow::stop() {
    http_.stop();
    clearBuffers();
    requestBuffer_ = MutableByteSpan();
    responseBuffer_ = MutableByteSpan();
    workspace_ = MutableByteSpan();
    clearError();
    state_ = WifiOnboardingState::Stopped;
}

ErrorCode WifiOnboardingFlow::mapClaimError(
    WifiOnboardingWireError error) {
    switch (error) {
        case WifiOnboardingWireError::UnsupportedVersion:
            return ErrorCode::UnsupportedVersion;
        case WifiOnboardingWireError::AuthenticationRequired:
        case WifiOnboardingWireError::EnrollmentKeyExpired:
        case WifiOnboardingWireError::EnrollmentKeyConsumed:
        case WifiOnboardingWireError::EnrollmentKeyConflict:
            return ErrorCode::AuthenticationRequired;
        case WifiOnboardingWireError::OwnershipAlreadyActive:
        case WifiOnboardingWireError::OwnershipGenerationConflict:
        case WifiOnboardingWireError::ClaimConflict:
            return ErrorCode::SequenceConflict;
        case WifiOnboardingWireError::RequestExpired:
        case WifiOnboardingWireError::RateLimited:
        case WifiOnboardingWireError::TemporarilyUnavailable:
            return ErrorCode::WouldBlock;
        case WifiOnboardingWireError::Malformed:
            return ErrorCode::ProtocolError;
    }
    return ErrorCode::ProtocolError;
}

ErrorCode WifiOnboardingFlow::mapCloudError(
    CloudEnrollmentWireError error) {
    switch (error) {
        case CloudEnrollmentWireError::UnsupportedVersion:
            return ErrorCode::UnsupportedVersion;
        case CloudEnrollmentWireError::UnsupportedAlgorithm:
            return ErrorCode::UnsupportedFeature;
        case CloudEnrollmentWireError::AuthenticationRequired:
        case CloudEnrollmentWireError::EnvelopeInvalid:
        case CloudEnrollmentWireError::GrantInvalid:
        case CloudEnrollmentWireError::ReceiptProofInvalid:
        case CloudEnrollmentWireError::DeviceRevoked:
            return ErrorCode::AuthenticationRequired;
        case CloudEnrollmentWireError::OwnershipNotActive:
        case CloudEnrollmentWireError::OwnershipGenerationConflict:
        case CloudEnrollmentWireError::RequestConflict:
        case CloudEnrollmentWireError::CredentialConflict:
            return ErrorCode::SequenceConflict;
        case CloudEnrollmentWireError::EncryptionKeyNotFound:
            return ErrorCode::NotFound;
        case CloudEnrollmentWireError::GrantExpired:
        case CloudEnrollmentWireError::RequestExpired:
        case CloudEnrollmentWireError::RateLimited:
        case CloudEnrollmentWireError::TemporarilyUnavailable:
            return ErrorCode::WouldBlock;
        case CloudEnrollmentWireError::Malformed:
            return ErrorCode::ProtocolError;
    }
    return ErrorCode::ProtocolError;
}

WifiOnboardingRetryClass WifiOnboardingFlow::mapCloudRetry(
    CloudEnrollmentRetryClass retryClass) {
    switch (retryClass) {
        case CloudEnrollmentRetryClass::Never:
            return WifiOnboardingRetryClass::Never;
        case CloudEnrollmentRetryClass::SameRequest:
            return WifiOnboardingRetryClass::SameRequest;
        case CloudEnrollmentRetryClass::Backoff:
            return WifiOnboardingRetryClass::Backoff;
        case CloudEnrollmentRetryClass::RestartEnrollment:
            return WifiOnboardingRetryClass::RestartTransaction;
        case CloudEnrollmentRetryClass::ReclaimOwnership:
            return WifiOnboardingRetryClass::ReclaimOwnership;
    }
    return WifiOnboardingRetryClass::Never;
}

} // namespace blinker
