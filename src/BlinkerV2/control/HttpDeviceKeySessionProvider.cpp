#include "HttpDeviceKeySessionProvider.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "../core/SecureMemory.h"
#include "../protocol/bbp2/Frame.h"

namespace blinker {

namespace {

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
        reinterpret_cast<const uint8_t*>(value.data), value.size);
}

static constexpr HttpHeader kRequestHeaders[] = {
    HttpHeader(StringView("Accept"), StringView("application/cbor")),
    HttpHeader(
        StringView("X-Blinker-Contract"),
        StringView("device-key-session/1"))};

} // namespace

HttpDeviceKeySessionProvider::HttpDeviceKeySessionProvider(
    IDeviceKeySource& keySource,
    const DeviceInstanceId& deviceInstanceId,
    IRandom& random,
    IHttpClient& http,
    IClock& clock,
    const HttpDeviceKeySessionConfig& config,
    MutableByteSpan requestBuffer,
    MutableByteSpan responseBuffer,
    MutableCharSpan credentialArena)
    : keySource_(keySource),
      deviceInstanceId_(deviceInstanceId),
      random_(random),
      http_(http),
      clock_(clock),
      config_(config),
      requestBuffer_(requestBuffer),
      responseBuffer_(responseBuffer),
      credentialArena_(credentialArena),
      credentials_(),
      requestId_(),
      locator_(),
      clientNonce_(),
      challengeId_(),
      serverNonce_(),
      challengeCredentialVersion_(0U),
      authenticatedCredentialVersion_(0U),
      credentialGeneration_(0U),
      challengeExpiresAt_(0U),
      retryAt_(0U),
      expiresAt_(0U),
      refreshAt_(0U),
      retryAttempt_(0U),
      phase_(Phase::Challenge),
      state_(SessionProviderState::Stopped),
      lastError_(ErrorCode::Ok),
      lastWireError_(DeviceKeySessionWireError::Malformed),
      started_(false),
      networkAvailable_(true),
      hasTransaction_(false),
      hasChallenge_(false),
      hasCredentials_(false),
      hasWireError_(false) {}

HttpDeviceKeySessionProvider::~HttpDeviceKeySessionProvider() {
    stop();
}

bool HttpDeviceKeySessionProvider::buffersOverlap(
    ByteView first,
    ByteView second) {
    if (first.data == nullptr || second.data == nullptr) return false;
    const uintptr_t firstBegin = reinterpret_cast<uintptr_t>(first.data);
    const uintptr_t secondBegin = reinterpret_cast<uintptr_t>(second.data);
    if (first.size > UINTPTR_MAX - firstBegin ||
        second.size > UINTPTR_MAX - secondBegin) {
        return true;
    }
    return firstBegin < secondBegin + second.size &&
           secondBegin < firstBegin + first.size;
}

Result HttpDeviceKeySessionProvider::validateConfiguration() const {
    const bool validHttpSecurity =
        config_.httpSecurity == HttpSecurity::PlainTcp ||
        config_.httpSecurity == HttpSecurity::Tls;
    const bool validMqttSecurity =
        config_.mqttSecurity == MqttSecurity::PlainTcp ||
        config_.mqttSecurity == MqttSecurity::Tls;
    if (config_.host.data == nullptr || config_.host.empty() ||
        config_.port == 0U || !validHttpSecurity || !validMqttSecurity ||
        config_.firmwareVersion.data == nullptr ||
        config_.firmwareVersion.empty() ||
        config_.firmwareVersion.size >
            kDeviceKeyMaximumFirmwareVersionSize ||
        config_.requestTimeoutMs == 0U || config_.initialRetryMs == 0U ||
        config_.maximumRetryMs < config_.initialRetryMs ||
        config_.maximumRetryMs > static_cast<uint32_t>(INT32_MAX) ||
        config_.maximumServerRetryMs == 0U ||
        config_.maximumServerRetryMs > static_cast<uint32_t>(INT32_MAX) ||
        !isValidDeviceInstanceId(deviceInstanceId_) ||
        requestBuffer_.data == nullptr ||
        requestBuffer_.size < kHttpDeviceKeySessionMinimumRequestBufferSize ||
        responseBuffer_.data == nullptr ||
        responseBuffer_.size <
            kHttpDeviceKeySessionMinimumResponseBufferSize ||
        credentialArena_.data == nullptr ||
        credentialArena_.size < kDeviceKeySessionCredentialArenaSize ||
        buffersOverlap(bytes(requestBuffer_), bytes(responseBuffer_)) ||
        buffersOverlap(bytes(requestBuffer_), bytes(credentialArena_)) ||
        buffersOverlap(bytes(responseBuffer_), bytes(credentialArena_))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result HttpDeviceKeySessionProvider::validateKeySource() {
    DeviceKey key;
    Result result = keySource_.load(key);
    if (result) result = validateDeviceKey(key);
    clearDeviceKey(key);
    return result;
}

Result HttpDeviceKeySessionProvider::start() {
    if (started_) return Result::success();
    Result result = validateConfiguration();
    if (result) result = validateKeySource();
    if (!result) {
        fail(result.code());
        return result;
    }

    finishHttp();
    clearTransaction();
    clearCredentials();
    credentialGeneration_ = 0U;
    retryAt_ = 0U;
    retryAttempt_ = 0U;
    lastError_ = ErrorCode::Ok;
    hasWireError_ = false;
    started_ = true;
    state_ = SessionProviderState::Idle;
    return networkAvailable_ ? beginChallenge(true) : Result::success();
}

Result HttpDeviceKeySessionProvider::requestRefresh() {
    if (!started_ || !hasCredentials_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (state_ == SessionProviderState::Requesting ||
        state_ == SessionProviderState::Backoff ||
        state_ == SessionProviderState::Idle) {
        return Result::failure(ErrorCode::WouldBlock);
    }
    clearTransaction();
    retryAttempt_ = 0U;
    lastError_ = ErrorCode::Ok;
    hasWireError_ = false;
    state_ = SessionProviderState::Idle;
    return networkAvailable_ ? beginChallenge(true) : Result::success();
}

void HttpDeviceKeySessionProvider::setNetworkAvailable(bool available) {
    if (networkAvailable_ == available) return;
    networkAvailable_ = available;
    if (!available &&
        (state_ == SessionProviderState::Requesting ||
         state_ == SessionProviderState::Backoff)) {
        finishHttp();
        state_ = SessionProviderState::Idle;
    }
}

Result HttpDeviceKeySessionProvider::beginChallenge(bool createTransaction) {
    finishHttp();
    if (createTransaction) {
        clearTransaction();
        DeviceKey key;
        DeviceKeyLocator locator;
        Result result = keySource_.load(key);
        if (result) result = validateDeviceKey(key);
        if (result) result = deriveDeviceKeyLocator(key, locator);
        clearDeviceKey(key);
        if (result) {
            result = random_.fill(
                MutableByteSpan(requestId_, sizeof(requestId_)));
        }
        if (result) {
            result = random_.fill(
                MutableByteSpan(clientNonce_, sizeof(clientNonce_)));
        }
        if (result &&
            (allZero(ByteView(requestId_, sizeof(requestId_))) ||
             allZero(ByteView(clientNonce_, sizeof(clientNonce_))))) {
            result = Result::failure(ErrorCode::InternalError);
        }
        if (!result) {
            secureZero(MutableByteSpan(
                locator.bytes, sizeof(locator.bytes)));
            clearTransaction();
            fail(result.code());
            return result;
        }
        memcpy(locator_, locator.bytes, sizeof(locator_));
        secureZero(MutableByteSpan(locator.bytes, sizeof(locator.bytes)));
        hasTransaction_ = true;
    } else if (!hasTransaction_) {
        return Result::failure(ErrorCode::StateConflict);
    }

    phase_ = Phase::Challenge;
    DeviceKeyChallengeRequest body;
    body.requestId = ByteView(requestId_, sizeof(requestId_));
    body.locator = ByteView(locator_, sizeof(locator_));
    body.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes, sizeof(deviceInstanceId_.bytes));
    body.clientNonce = ByteView(clientNonce_, sizeof(clientNonce_));
    ByteView encoded;
    Result result = encodeDeviceKeyChallengeRequest(
        body, requestBuffer_, encoded);
    if (!result) {
        fail(result.code());
        return result;
    }
    return startHttp(deviceKeyChallengeTarget(), encoded);
}

Result HttpDeviceKeySessionProvider::beginSession() {
    finishHttp();
    if (!hasTransaction_ || !hasChallenge_) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (challengeExpired(clock_.monotonicMillis())) {
        restartChallenge();
        return Result::success();
    }

    DeviceKey key;
    DeviceKeyLocator currentLocator;
    Result result = keySource_.load(key);
    if (result) result = validateDeviceKey(key);
    if (result) result = deriveDeviceKeyLocator(key, currentLocator);
    if (result && !constantTimeEqual(
                      ByteView(currentLocator.bytes, sizeof(currentLocator.bytes)),
                      ByteView(locator_, sizeof(locator_)))) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }

    DeviceKeySessionProofContext context;
    context.requestId = ByteView(requestId_, sizeof(requestId_));
    context.locator = ByteView(locator_, sizeof(locator_));
    context.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes, sizeof(deviceInstanceId_.bytes));
    context.clientNonce = ByteView(clientNonce_, sizeof(clientNonce_));
    context.challengeId = ByteView(challengeId_, sizeof(challengeId_));
    context.serverNonce = ByteView(serverNonce_, sizeof(serverNonce_));
    context.credentialVersion = challengeCredentialVersion_;
    context.protocolVersion = bbp2::kVersion;
    context.firmwareVersion = config_.firmwareVersion;
    context.requestedSecurity = config_.mqttSecurity;
    context.proofAlgorithm = DeviceKeyProofAlgorithm::HmacSha256;
    uint8_t proof[kDeviceKeyProofSize] = {};
    if (result) {
        result = computeDeviceKeySessionProof(
            key, context, MutableByteSpan(proof, sizeof(proof)));
    }
    clearDeviceKey(key);
    secureZero(MutableByteSpan(
        currentLocator.bytes, sizeof(currentLocator.bytes)));

    DeviceKeySessionRequest body;
    body.requestId = context.requestId;
    body.challengeId = context.challengeId;
    body.protocolVersion = context.protocolVersion;
    body.firmwareVersion = context.firmwareVersion;
    body.requestedSecurity = context.requestedSecurity;
    body.proofAlgorithm = context.proofAlgorithm;
    body.proof = ByteView(proof, sizeof(proof));
    ByteView encoded;
    if (result) {
        result = encodeDeviceKeySessionRequest(
            body, requestBuffer_, encoded);
    }
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    if (!result) {
        fail(result.code());
        return result;
    }
    phase_ = Phase::Session;
    return startHttp(deviceKeySessionTarget(), encoded);
}

Result HttpDeviceKeySessionProvider::startHttp(
    StringView target,
    ByteView body) {
    if (!networkAvailable_) {
        state_ = SessionProviderState::Idle;
        return Result::success();
    }
    secureZero(responseBuffer_);
    HttpRequest request;
    request.method = HttpMethod::Post;
    request.host = config_.host;
    request.port = config_.port;
    request.security = config_.httpSecurity;
    request.target = target;
    request.contentType = StringView("application/cbor");
    request.headers = kRequestHeaders;
    request.headerCount = static_cast<uint8_t>(
        sizeof(kRequestHeaders) / sizeof(kRequestHeaders[0]));
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

void HttpDeviceKeySessionProvider::poll(uint32_t budgetMicros) {
    if (!started_ || state_ == SessionProviderState::Stopped ||
        state_ == SessionProviderState::Failed) {
        return;
    }
    const uint32_t now = clock_.monotonicMillis();
    if (hasCredentials_ && credentialsExpired(now)) {
        fail(ErrorCode::AuthenticationRequired);
        return;
    }
    if (state_ == SessionProviderState::Ready && timeReached(now, refreshAt_)) {
        (void)requestRefresh();
        return;
    }
    if (!networkAvailable_) return;
    if (state_ == SessionProviderState::Idle) {
        if (hasChallenge_ && !challengeExpired(now)) {
            (void)beginSession();
        } else {
            (void)beginChallenge(!hasTransaction_ || hasChallenge_);
        }
        return;
    }
    if (state_ == SessionProviderState::Backoff) {
        if (!retryDue(now)) return;
        if (phase_ == Phase::Session &&
            (!hasChallenge_ || challengeExpired(now))) {
            restartChallenge();
        } else if (phase_ == Phase::Session) {
            (void)beginSession();
        } else {
            (void)beginChallenge(false);
        }
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

void HttpDeviceKeySessionProvider::processResponse() {
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
        if (phase_ == Phase::Challenge) {
            processChallengeSuccess(response.body);
        } else {
            processSessionSuccess(response.body);
        }
    } else if (response.statusCode >= 300U && response.statusCode < 400U) {
        fail(ErrorCode::UnsupportedFeature);
    } else {
        processError(response.statusCode, response.body);
    }
}

void HttpDeviceKeySessionProvider::processChallengeSuccess(ByteView body) {
    DeviceKeyChallengeResponse decoded;
    Result result = decodeDeviceKeyChallengeResponse(body, decoded);
    if (result && !constantTimeEqual(
                      decoded.requestId,
                      ByteView(requestId_, sizeof(requestId_)))) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (!result) {
        fail(result.code());
        return;
    }
    memcpy(challengeId_, decoded.challengeId.data, sizeof(challengeId_));
    memcpy(serverNonce_, decoded.serverNonce.data, sizeof(serverNonce_));
    challengeCredentialVersion_ = decoded.credentialVersion;
    challengeExpiresAt_ = clock_.monotonicMillis() +
                          decoded.expiresInSeconds * 1000U;
    hasChallenge_ = true;
    retryAttempt_ = 0U;
    finishHttp();
    (void)beginSession();
}

void HttpDeviceKeySessionProvider::processSessionSuccess(ByteView body) {
    DeviceKeySessionResponse decoded;
    Result result = decodeDeviceKeySessionResponse(body, decoded);
    if (result && !constantTimeEqual(
                      decoded.requestId,
                      ByteView(requestId_, sizeof(requestId_)))) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result &&
        decoded.credentialVersion != challengeCredentialVersion_) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result && decoded.security != config_.mqttSecurity) {
        result = Result::failure(ErrorCode::ProtocolError);
    }
    SessionCredentials next;
    if (result) {
        result = copyDeviceKeySessionCredentials(
            decoded, credentialArena_, next);
    }
    if (!result) {
        fail(result.code());
        return;
    }

    credentials_ = next;
    hasCredentials_ = true;
    authenticatedCredentialVersion_ = decoded.credentialVersion;
    ++credentialGeneration_;
    if (credentialGeneration_ == 0U) ++credentialGeneration_;
    const uint32_t now = clock_.monotonicMillis();
    expiresAt_ = now + decoded.expiresInSeconds * 1000U;
    refreshAt_ = now + decoded.refreshAfterSeconds * 1000U;
    retryAttempt_ = 0U;
    lastError_ = ErrorCode::Ok;
    hasWireError_ = false;
    finishHttp();
    clearTransaction();
    state_ = SessionProviderState::Ready;
}

void HttpDeviceKeySessionProvider::processError(
    uint16_t statusCode,
    ByteView body) {
    DeviceKeySessionErrorBody errorBody;
    const Result decoded = decodeDeviceKeySessionErrorBody(body, errorBody);
    if (!decoded) {
        fail(decoded.code());
        return;
    }
    if (!deviceKeySessionHttpStatusMatches(statusCode, errorBody.error)) {
        fail(ErrorCode::ProtocolError);
        return;
    }
    hasWireError_ = true;
    lastWireError_ = errorBody.error;
    const ErrorCode mapped = mapWireError(errorBody.error);
    if (errorBody.retryClass == DeviceKeySessionRetryClass::NewChallenge) {
        finishHttp();
        restartChallenge();
        return;
    }
    if (errorBody.retryClass == DeviceKeySessionRetryClass::SameRequest ||
        errorBody.retryClass == DeviceKeySessionRetryClass::Backoff) {
        scheduleRetry(
            mapped,
            errorBody.hasRetryAfter
                ? serverRetryDelay(errorBody.retryAfterSeconds)
                : 0U);
        return;
    }
    fail(mapped);
}

void HttpDeviceKeySessionProvider::scheduleRetry(
    ErrorCode error,
    uint32_t delayMs) {
    finishHttp();
    if (retryAttempt_ != UINT8_MAX) ++retryAttempt_;
    lastError_ = error;
    retryAt_ = clock_.monotonicMillis() +
               (delayMs == 0U ? retryDelay() : delayMs);
    state_ = networkAvailable_ ? SessionProviderState::Backoff
                               : SessionProviderState::Idle;
}

void HttpDeviceKeySessionProvider::restartChallenge() {
    finishHttp();
    clearTransaction();
    retryAttempt_ = 0U;
    state_ = SessionProviderState::Idle;
    if (networkAvailable_) (void)beginChallenge(true);
}

void HttpDeviceKeySessionProvider::fail(ErrorCode error) {
    finishHttp();
    clearTransaction();
    clearCredentials();
    lastError_ = error == ErrorCode::Ok ? ErrorCode::InternalError : error;
    state_ = SessionProviderState::Failed;
}

void HttpDeviceKeySessionProvider::finishHttp() {
    http_.stop();
    clearWireBuffers();
}

void HttpDeviceKeySessionProvider::clearWireBuffers() {
    secureZero(requestBuffer_);
    secureZero(responseBuffer_);
}

void HttpDeviceKeySessionProvider::clearTransaction() {
    secureZero(MutableByteSpan(requestId_, sizeof(requestId_)));
    secureZero(MutableByteSpan(locator_, sizeof(locator_)));
    secureZero(MutableByteSpan(clientNonce_, sizeof(clientNonce_)));
    secureZero(MutableByteSpan(challengeId_, sizeof(challengeId_)));
    secureZero(MutableByteSpan(serverNonce_, sizeof(serverNonce_)));
    challengeCredentialVersion_ = 0U;
    challengeExpiresAt_ = 0U;
    phase_ = Phase::Challenge;
    hasTransaction_ = false;
    hasChallenge_ = false;
}

void HttpDeviceKeySessionProvider::clearCredentials() {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(credentialArena_.data),
        credentialArena_.size));
    credentials_ = SessionCredentials();
    authenticatedCredentialVersion_ = 0U;
    hasCredentials_ = false;
    expiresAt_ = 0U;
    refreshAt_ = 0U;
}

uint32_t HttpDeviceKeySessionProvider::retryDelay() const {
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

uint32_t HttpDeviceKeySessionProvider::serverRetryDelay(
    uint32_t seconds) const {
    if (seconds > config_.maximumServerRetryMs / 1000U) {
        return config_.maximumServerRetryMs;
    }
    const uint32_t delay = seconds * 1000U;
    return delay > config_.maximumServerRetryMs
               ? config_.maximumServerRetryMs
               : delay;
}

bool HttpDeviceKeySessionProvider::retryDue(uint32_t now) const {
    return timeReached(now, retryAt_);
}

bool HttpDeviceKeySessionProvider::challengeExpired(uint32_t now) const {
    return !hasChallenge_ || timeReached(now, challengeExpiresAt_);
}

bool HttpDeviceKeySessionProvider::credentialsExpired(uint32_t now) const {
    return timeReached(now, expiresAt_);
}

bool HttpDeviceKeySessionProvider::retryableClientError(ErrorCode error) {
    return error == ErrorCode::NotConnected ||
           error == ErrorCode::WouldBlock ||
           error == ErrorCode::InternalError;
}

ErrorCode HttpDeviceKeySessionProvider::mapWireError(
    DeviceKeySessionWireError error) {
    switch (error) {
        case DeviceKeySessionWireError::UnsupportedVersion:
            return ErrorCode::UnsupportedVersion;
        case DeviceKeySessionWireError::UnsupportedAlgorithm:
            return ErrorCode::UnsupportedFeature;
        case DeviceKeySessionWireError::DeviceKeyInvalid:
        case DeviceKeySessionWireError::DeviceKeyRotated:
        case DeviceKeySessionWireError::CredentialRevoked:
        case DeviceKeySessionWireError::SessionRevoked:
        case DeviceKeySessionWireError::DeviceRevoked:
            return ErrorCode::AuthenticationRequired;
        case DeviceKeySessionWireError::ChallengeExpired:
            return ErrorCode::WouldBlock;
        case DeviceKeySessionWireError::ChallengeConflict:
            return ErrorCode::SequenceConflict;
        case DeviceKeySessionWireError::RateLimited:
        case DeviceKeySessionWireError::TemporarilyUnavailable:
            return ErrorCode::WouldBlock;
        case DeviceKeySessionWireError::Malformed:
            return ErrorCode::ProtocolError;
    }
    return ErrorCode::ProtocolError;
}

Result HttpDeviceKeySessionProvider::credentials(
    SessionCredentials& output) const {
    if (!hasCredentials_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    output = credentials_;
    return Result::success();
}

void HttpDeviceKeySessionProvider::stop() {
    finishHttp();
    clearTransaction();
    clearCredentials();
    credentialGeneration_ = 0U;
    retryAt_ = 0U;
    retryAttempt_ = 0U;
    lastError_ = ErrorCode::Ok;
    hasWireError_ = false;
    started_ = false;
    state_ = SessionProviderState::Stopped;
}

} // namespace blinker
