#include "HttpCloudSessionProvider.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "../core/SecureMemory.h"
#include "../protocol/bbp2/Frame.h"

namespace blinker {

namespace {

StringView defaultPath() {
    static const char value[] = "/api/v2/device-sessions";
    return StringView(value, sizeof(value) - 1U);
}

bool timeReached(uint32_t now, uint32_t target) {
    return static_cast<int32_t>(now - target) >= 0;
}

bool allZero(ByteView value) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

ByteView bytes(MutableByteSpan value) {
    return ByteView(value.data, value.size);
}

ByteView bytes(MutableCharSpan value) {
    return ByteView(
        reinterpret_cast<const uint8_t*>(value.data),
        value.size);
}

} // namespace

HttpCloudSessionProvider::HttpCloudSessionProvider(
    ICloudCredentialSource& credentialSource,
    IRandom& random,
    IHttpClient& http,
    IClock& clock,
    const HttpCloudSessionConfig& config,
    MutableByteSpan requestBuffer,
    MutableByteSpan responseBuffer,
    MutableCharSpan credentialArena)
    : credentialSource_(credentialSource),
      random_(random),
      http_(http),
      clock_(clock),
      config_(config),
      requestBuffer_(requestBuffer),
      responseBuffer_(responseBuffer),
      credentialArena_(credentialArena),
      requestHeaders_(),
      credentials_(),
      requestId_(),
      requestCredentialVersion_(0U),
      requestGeneration_(0U),
      credentialGeneration_(0U),
      retryAt_(0U),
      expiresAt_(0U),
      refreshAt_(0U),
      retryAttempt_(0U),
      state_(SessionProviderState::Stopped),
      lastError_(ErrorCode::Ok),
      lastWireError_(CloudSessionWireError::Malformed),
      started_(false),
      networkAvailable_(true),
      hasRequestId_(false),
      newRequestPending_(false),
      hasCredentials_(false),
      hasWireError_(false) {
    if (config_.path.empty()) config_.path = defaultPath();
    requestHeaders_[0] = HttpHeader(
        StringView("Accept"),
        StringView("application/cbor"));
    requestHeaders_[1] = HttpHeader(
        StringView("X-Blinker-Contract"),
        StringView("device-cloud-session/2"));
}

HttpCloudSessionProvider::~HttpCloudSessionProvider() {
    stop();
}

bool HttpCloudSessionProvider::validPath(StringView value) {
    if (value.data == nullptr || value.empty() || value.data[0] != '/') {
        return false;
    }
    for (size_t index = 0U; index < value.size; ++index) {
        if (value.data[index] == '\0' || value.data[index] == '\r' ||
            value.data[index] == '\n') {
            return false;
        }
    }
    return true;
}

bool HttpCloudSessionProvider::buffersOverlap(
    ByteView left,
    ByteView right) {
    if (left.data == nullptr || right.data == nullptr) return false;
    const uintptr_t leftBegin = reinterpret_cast<uintptr_t>(left.data);
    const uintptr_t rightBegin = reinterpret_cast<uintptr_t>(right.data);
    if (left.size > UINTPTR_MAX - leftBegin ||
        right.size > UINTPTR_MAX - rightBegin) {
        return true;
    }
    return leftBegin < rightBegin + right.size &&
           rightBegin < leftBegin + left.size;
}

Result HttpCloudSessionProvider::validateConfiguration() const {
    const bool validHttpSecurity =
        config_.httpSecurity == HttpSecurity::PlainTcp ||
        config_.httpSecurity == HttpSecurity::Tls;
    const bool validMqttSecurity =
        config_.mqttSecurity == MqttSecurity::PlainTcp ||
        config_.mqttSecurity == MqttSecurity::Tls;
    if (config_.host.data == nullptr || config_.host.empty() ||
        config_.port == 0U || !validPath(config_.path) ||
        !validHttpSecurity || !validMqttSecurity ||
        config_.firmwareVersion.data == nullptr ||
        config_.firmwareVersion.empty() ||
        config_.firmwareVersion.size > kCloudSessionMaxFirmwareVersionSize ||
        config_.requestTimeoutMs == 0U || config_.initialRetryMs == 0U ||
        config_.maximumRetryMs < config_.initialRetryMs ||
        config_.maximumRetryMs > static_cast<uint32_t>(INT32_MAX) ||
        config_.maximumServerRetryMs == 0U ||
        config_.maximumServerRetryMs > static_cast<uint32_t>(INT32_MAX) ||
        requestBuffer_.data == nullptr ||
        requestBuffer_.size < kHttpCloudSessionMinimumRequestBufferSize ||
        responseBuffer_.data == nullptr ||
        responseBuffer_.size < kHttpCloudSessionMinimumResponseBufferSize ||
        credentialArena_.data == nullptr ||
        credentialArena_.size < kCloudSessionCredentialArenaSize ||
        buffersOverlap(bytes(requestBuffer_), bytes(responseBuffer_)) ||
        buffersOverlap(bytes(requestBuffer_), bytes(credentialArena_)) ||
        buffersOverlap(bytes(responseBuffer_), bytes(credentialArena_))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result HttpCloudSessionProvider::validateActiveCredential() {
    CloudCredential credential;
    Result result = credentialSource_.load(credential);
    if (result) result = validateCloudCredential(credential);
    clearCloudCredential(credential);
    return result;
}

Result HttpCloudSessionProvider::start() {
    if (started_) return Result::success();
    Result result = validateConfiguration();
    if (result) result = validateActiveCredential();
    if (!result) {
        fail(result.code());
        return result;
    }

    finishRequest();
    clearCredentials();
    secureZero(MutableByteSpan(requestId_, sizeof(requestId_)));
    requestCredentialVersion_ = 0U;
    requestGeneration_ = 0U;
    credentialGeneration_ = 0U;
    retryAt_ = 0U;
    retryAttempt_ = 0U;
    lastError_ = ErrorCode::Ok;
    hasWireError_ = false;
    hasRequestId_ = false;
    newRequestPending_ = true;
    started_ = true;
    state_ = SessionProviderState::Idle;
    return networkAvailable_ ? beginRequest(true) : Result::success();
}

Result HttpCloudSessionProvider::requestRefresh() {
    if (!started_ || !hasCredentials_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (state_ == SessionProviderState::Requesting ||
        state_ == SessionProviderState::Backoff ||
        state_ == SessionProviderState::Idle) {
        return Result::failure(ErrorCode::WouldBlock);
    }
    retryAttempt_ = 0U;
    lastError_ = ErrorCode::Ok;
    hasWireError_ = false;
    hasRequestId_ = false;
    newRequestPending_ = true;
    state_ = SessionProviderState::Idle;
    return networkAvailable_ ? beginRequest(true) : Result::success();
}

void HttpCloudSessionProvider::setNetworkAvailable(bool available) {
    if (networkAvailable_ == available) return;
    networkAvailable_ = available;
    if (!available &&
        (state_ == SessionProviderState::Requesting ||
         state_ == SessionProviderState::Backoff)) {
        finishRequest();
        state_ = SessionProviderState::Idle;
    }
}

Result HttpCloudSessionProvider::beginRequest(bool createRequestId) {
    clearRequestData();
    CloudCredential credential;
    Result result = credentialSource_.load(credential);
    if (result) result = validateCloudCredential(credential);

    if (result && createRequestId) {
        result = random_.fill(
            MutableByteSpan(requestId_, sizeof(requestId_)));
        if (result && allZero(ByteView(requestId_, sizeof(requestId_)))) {
            result = Result::failure(ErrorCode::InternalError);
        }
        if (result) {
            requestCredentialVersion_ = credential.credentialVersion;
            requestGeneration_ = credential.generation;
            hasRequestId_ = true;
        }
    } else if (result &&
               (!hasRequestId_ ||
                requestCredentialVersion_ != credential.credentialVersion ||
                requestGeneration_ != credential.generation)) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }

    CloudSessionRequest body;
    body.requestId = ByteView(requestId_, sizeof(requestId_));
    body.logicalDeviceId = credential.logicalId();
    body.credentialVersion = credential.credentialVersion;
    body.generation = credential.generation;
    body.protocolVersion = bbp2::kVersion;
    body.firmwareVersion = config_.firmwareVersion;
    body.proofAlgorithm = CloudSessionProofAlgorithm::HmacSha256;

    uint8_t proof[kCloudSessionProofSize] = {};
    if (result) {
        result = computeCloudSessionProof(
            credential.authenticationSecret(),
            body,
            requestBuffer_,
            MutableByteSpan(proof, sizeof(proof)));
    }
    ByteView encoded;
    if (result) {
        body.proof = ByteView(proof, sizeof(proof));
        result = encodeCloudSessionRequest(body, requestBuffer_, encoded);
    }
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    clearCloudCredential(credential);

    if (!result) {
        clearRequestData();
        fail(result.code());
        return result;
    }
    newRequestPending_ = false;
    return startHttp(encoded);
}

Result HttpCloudSessionProvider::startHttp(ByteView body) {
    if (!networkAvailable_) {
        state_ = SessionProviderState::Idle;
        return Result::success();
    }
    HttpRequest request;
    request.method = HttpMethod::Post;
    request.host = config_.host;
    request.port = config_.port;
    request.security = config_.httpSecurity;
    request.target = config_.path;
    request.contentType = StringView("application/cbor");
    request.headers = requestHeaders_;
    request.headerCount = 2U;
    request.body = body;
    request.responseBuffer = responseBuffer_;
    request.timeoutMillis = config_.requestTimeoutMs;

    const Result result = http_.start(request);
    if (result) {
        state_ = SessionProviderState::Requesting;
        lastError_ = ErrorCode::Ok;
        return Result::success();
    }
    if (retryableClientError(result.code())) {
        scheduleRetry(result.code());
        return Result::success();
    }
    fail(result.code());
    return result;
}

void HttpCloudSessionProvider::poll(uint32_t budgetMicros) {
    if (!started_ || state_ == SessionProviderState::Stopped ||
        state_ == SessionProviderState::Failed) {
        return;
    }
    const uint32_t now = clock_.monotonicMillis();
    if (hasCredentials_ && credentialsExpired(now)) {
        fail(ErrorCode::AuthenticationRequired);
        return;
    }
    if (state_ == SessionProviderState::Ready &&
        timeReached(now, refreshAt_)) {
        (void)requestRefresh();
        return;
    }
    if (!networkAvailable_) return;
    if (state_ == SessionProviderState::Idle) {
        (void)beginRequest(newRequestPending_ || !hasRequestId_);
        return;
    }
    if (state_ == SessionProviderState::Backoff) {
        if (retryDue(now)) (void)beginRequest(false);
        return;
    }
    if (state_ != SessionProviderState::Requesting) return;

    http_.poll(budgetMicros);
    if (http_.state() == HttpClientState::InProgress) return;
    if (http_.state() == HttpClientState::Complete) {
        processResponse();
        return;
    }
    const ErrorCode error = http_.state() == HttpClientState::Failed
                                ? http_.lastError()
                                : ErrorCode::InternalError;
    if (retryableClientError(error)) {
        scheduleRetry(error);
    } else {
        fail(error);
    }
}

void HttpCloudSessionProvider::processResponse() {
    HttpResponse response;
    Result result = http_.response(response);
    if (!result) {
        if (retryableClientError(result.code())) {
            scheduleRetry(result.code());
        } else {
            fail(result.code());
        }
        return;
    }
    if (response.statusCode == 200U) {
        processSuccess(response.body);
    } else if (response.statusCode >= 300U && response.statusCode < 400U) {
        fail(ErrorCode::UnsupportedFeature);
    } else {
        processError(response.statusCode, response.body);
    }
}

void HttpCloudSessionProvider::processSuccess(ByteView body) {
    CloudSessionResponse decoded;
    Result result = decodeCloudSessionResponse(body, decoded);
    if (result && !constantTimeEqual(
                      decoded.requestId,
                      ByteView(requestId_, sizeof(requestId_)))) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result && decoded.security != config_.mqttSecurity) {
        result = Result::failure(ErrorCode::ProtocolError);
    }
    SessionCredentials next;
    if (result) {
        result = copyCloudSessionCredentials(
            decoded,
            credentialArena_,
            next);
    }
    if (!result) {
        fail(result.code());
        return;
    }

    credentials_ = next;
    hasCredentials_ = true;
    ++credentialGeneration_;
    if (credentialGeneration_ == 0U) ++credentialGeneration_;
    const uint32_t now = clock_.monotonicMillis();
    expiresAt_ = now + decoded.expiresInSeconds * 1000U;
    refreshAt_ = now + decoded.refreshAfterSeconds * 1000U;
    retryAttempt_ = 0U;
    lastError_ = ErrorCode::Ok;
    hasWireError_ = false;
    finishRequest();
    secureZero(MutableByteSpan(requestId_, sizeof(requestId_)));
    hasRequestId_ = false;
    newRequestPending_ = false;
    state_ = SessionProviderState::Ready;
}

void HttpCloudSessionProvider::processError(
    uint16_t statusCode,
    ByteView body) {
    CloudSessionErrorBody errorBody;
    const Result decoded = decodeCloudSessionErrorBody(body, errorBody);
    if (!decoded) {
        fail(decoded.code());
        return;
    }
    if (!cloudSessionHttpStatusMatches(statusCode, errorBody.error)) {
        fail(ErrorCode::ProtocolError);
        return;
    }
    hasWireError_ = true;
    lastWireError_ = errorBody.error;
    const ErrorCode mapped = mapWireError(errorBody.error);
    if (errorBody.retryClass == CloudSessionRetryClass::NewRequest) {
        finishRequest();
        secureZero(MutableByteSpan(requestId_, sizeof(requestId_)));
        hasRequestId_ = false;
        newRequestPending_ = true;
        retryAttempt_ = 0U;
        lastError_ = mapped;
        state_ = SessionProviderState::Idle;
        return;
    }
    if (errorBody.retryClass == CloudSessionRetryClass::SameRequest ||
        errorBody.retryClass == CloudSessionRetryClass::Backoff) {
        scheduleRetry(
            mapped,
            errorBody.hasRetryAfter
                ? serverRetryDelay(errorBody.retryAfterSeconds)
                : 0U);
        return;
    }
    fail(mapped);
}

void HttpCloudSessionProvider::scheduleRetry(
    ErrorCode error,
    uint32_t delayMs) {
    finishRequest();
    if (retryAttempt_ != UINT8_MAX) ++retryAttempt_;
    lastError_ = error;
    retryAt_ = clock_.monotonicMillis() +
               (delayMs == 0U ? retryDelay() : delayMs);
    state_ = networkAvailable_ ? SessionProviderState::Backoff
                               : SessionProviderState::Idle;
}

void HttpCloudSessionProvider::fail(ErrorCode error) {
    finishRequest();
    secureZero(MutableByteSpan(requestId_, sizeof(requestId_)));
    hasRequestId_ = false;
    newRequestPending_ = false;
    clearCredentials();
    lastError_ = error;
    state_ = SessionProviderState::Failed;
}

void HttpCloudSessionProvider::finishRequest() {
    http_.stop();
    clearRequestData();
}

void HttpCloudSessionProvider::clearRequestData() {
    secureZero(requestBuffer_);
    secureZero(responseBuffer_);
}

void HttpCloudSessionProvider::clearCredentials() {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(credentialArena_.data),
        credentialArena_.size));
    credentials_ = SessionCredentials();
    hasCredentials_ = false;
    expiresAt_ = 0U;
    refreshAt_ = 0U;
}

uint32_t HttpCloudSessionProvider::retryDelay() const {
    uint32_t delay = config_.initialRetryMs;
    uint8_t shifts = retryAttempt_ > 0U
                         ? static_cast<uint8_t>(retryAttempt_ - 1U)
                         : 0U;
    while (shifts-- != 0U && delay < config_.maximumRetryMs) {
        delay = delay > config_.maximumRetryMs / 2U
                    ? config_.maximumRetryMs
                    : delay * 2U;
    }
    return delay > config_.maximumRetryMs
               ? config_.maximumRetryMs
               : delay;
}

uint32_t HttpCloudSessionProvider::serverRetryDelay(
    uint32_t seconds) const {
    if (seconds > config_.maximumServerRetryMs / 1000U) {
        return config_.maximumServerRetryMs;
    }
    const uint32_t delay = seconds * 1000U;
    return delay > config_.maximumServerRetryMs
               ? config_.maximumServerRetryMs
               : delay;
}

bool HttpCloudSessionProvider::retryDue(uint32_t now) const {
    return timeReached(now, retryAt_);
}

bool HttpCloudSessionProvider::credentialsExpired(uint32_t now) const {
    return timeReached(now, expiresAt_);
}

bool HttpCloudSessionProvider::retryableClientError(ErrorCode error) {
    return error == ErrorCode::NotConnected ||
           error == ErrorCode::WouldBlock ||
           error == ErrorCode::InternalError;
}

ErrorCode HttpCloudSessionProvider::mapWireError(
    CloudSessionWireError error) {
    switch (error) {
        case CloudSessionWireError::UnsupportedVersion:
            return ErrorCode::UnsupportedVersion;
        case CloudSessionWireError::UnsupportedAlgorithm:
            return ErrorCode::UnsupportedFeature;
        case CloudSessionWireError::AuthenticationRequired:
        case CloudSessionWireError::CredentialNotFound:
        case CloudSessionWireError::CredentialRevoked:
        case CloudSessionWireError::ProofInvalid:
        case CloudSessionWireError::DeviceRevoked:
            return ErrorCode::AuthenticationRequired;
        case CloudSessionWireError::RequestExpired:
            return ErrorCode::WouldBlock;
        case CloudSessionWireError::CredentialVersionConflict:
        case CloudSessionWireError::RequestConflict:
            return ErrorCode::SequenceConflict;
        case CloudSessionWireError::RateLimited:
        case CloudSessionWireError::TemporarilyUnavailable:
            return ErrorCode::WouldBlock;
        case CloudSessionWireError::Malformed:
            return ErrorCode::ProtocolError;
    }
    return ErrorCode::ProtocolError;
}

Result HttpCloudSessionProvider::credentials(
    SessionCredentials& output) const {
    if (!hasCredentials_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    output = credentials_;
    return Result::success();
}

void HttpCloudSessionProvider::stop() {
    finishRequest();
    clearCredentials();
    secureZero(MutableByteSpan(requestId_, sizeof(requestId_)));
    requestCredentialVersion_ = 0U;
    requestGeneration_ = 0U;
    credentialGeneration_ = 0U;
    retryAt_ = 0U;
    retryAttempt_ = 0U;
    lastError_ = ErrorCode::Ok;
    hasWireError_ = false;
    hasRequestId_ = false;
    newRequestPending_ = false;
    started_ = false;
    state_ = SessionProviderState::Stopped;
}

} // namespace blinker
