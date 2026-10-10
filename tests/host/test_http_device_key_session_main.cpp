#include <BlinkerV2/control/HttpDeviceKeySessionProvider.h>
#include <BlinkerV2/protocol/bbp2/Frame.h>
#include <BlinkerV2/transport/ManagedMqttTransport.h>
#include <BlinkerV2/runtime/LocalAccessCloudTransport.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::cerr << "HTTP DeviceKey check failed at line "            \
                      << __LINE__ << ": " << #condition << std::endl;       \
            std::exit(1);                                                    \
        }                                                                    \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

bool equals(blinker::StringView value, const char* expected) {
    const size_t size = std::strlen(expected);
    return value.data != nullptr && value.size == size &&
           std::memcmp(value.data, expected, size) == 0;
}

bool contains(blinker::ByteView haystack, blinker::ByteView needle) {
    if (haystack.data == nullptr || needle.data == nullptr ||
        needle.empty() || needle.size > haystack.size) {
        return false;
    }
    for (size_t offset = 0U; offset <= haystack.size - needle.size; ++offset) {
        if (std::memcmp(haystack.data + offset, needle.data, needle.size) == 0) {
            return true;
        }
    }
    return false;
}

class FakeClock final : public blinker::IClock {
public:
    FakeClock() : now(0U) {}
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }
    void advance(uint32_t milliseconds) { now += milliseconds; }
    uint32_t now;
};

class SequenceRandom final : public blinker::IRandom {
public:
    SequenceRandom() : next(1U), calls(0U) {}
    blinker::Result fill(blinker::MutableByteSpan output) override {
        ++calls;
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = next++;
        }
        return blinker::Result::success();
    }
    uint8_t next;
    uint32_t calls;
};

class FakeKeySource final : public blinker::IDeviceKeySource {
public:
    FakeKeySource() : key(), present(true), loads(0U) {
        for (size_t index = 0U; index < sizeof(key.bytes); ++index) {
            key.bytes[index] = static_cast<uint8_t>(0x80U + index);
        }
    }
    blinker::Result load(blinker::DeviceKey& output) override {
        ++loads;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        output = key;
        return blinker::Result::success();
    }
    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::PlainFlash;
    }
    blinker::DeviceKey key;
    bool present;
    uint32_t loads;
};

class FakeHttpClient final : public blinker::IHttpClient {
public:
    FakeHttpClient()
        : request(),
          currentState(blinker::HttpClientState::Idle),
          error(blinker::ErrorCode::Ok),
          status(0U),
          responseSize(0U),
          startCalls(0U),
          stopCalls(0U) {}

    blinker::Result start(const blinker::HttpRequest& value) override {
        ++startCalls;
        request = value;
        currentState = blinker::HttpClientState::InProgress;
        error = blinker::ErrorCode::Ok;
        return blinker::Result::success();
    }
    void poll(uint32_t) override {}
    blinker::HttpClientState state() const override { return currentState; }
    blinker::Result response(blinker::HttpResponse& output) const override {
        if (currentState != blinker::HttpClientState::Complete) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        output.statusCode = status;
        output.body = blinker::ByteView(
            request.responseBuffer.data, responseSize);
        return blinker::Result::success();
    }
    blinker::ErrorCode lastError() const override { return error; }
    void stop() override {
        ++stopCalls;
        currentState = blinker::HttpClientState::Idle;
    }
    void complete(uint16_t statusCode, blinker::ByteView body) {
        CHECK_TRUE(body.data != nullptr &&
                   body.size <= request.responseBuffer.size);
        std::memcpy(request.responseBuffer.data, body.data, body.size);
        responseSize = body.size;
        status = statusCode;
        currentState = blinker::HttpClientState::Complete;
    }

    blinker::HttpRequest request;
    blinker::HttpClientState currentState;
    blinker::ErrorCode error;
    uint16_t status;
    size_t responseSize;
    uint32_t startCalls;
    uint32_t stopCalls;
};

class FakeMqttClient final : public blinker::IMqttClient {
public:
    FakeMqttClient()
        : handler(nullptr),
          handlerContext(nullptr),
          connectOptions(),
          subscribedTopic(),
          publishedTopic(),
          published(),
          publishedSize(0U),
          connectCalls(0U),
          subscribeCalls(0U),
          publishCalls(0U),
          disconnectCalls(0U),
          connectedState(false), failPublish(false) {}

    blinker::Result connect(
        const blinker::MqttConnectOptions& options) override {
        connectOptions = options;
        ++connectCalls;
        connectedState = true;
        return blinker::Result::success();
    }

    void disconnect() override {
        ++disconnectCalls;
        connectedState = false;
    }

    bool connected() const override { return connectedState; }

    blinker::Result subscribe(
        blinker::StringView topic,
        uint8_t qos) override {
        if (!connectedState || qos > 1U) {
            return blinker::Result::failure(
                blinker::ErrorCode::NotConnected);
        }
        subscribedTopic = topic;
        ++subscribeCalls;
        return blinker::Result::success();
    }

    blinker::Result publish(
        blinker::StringView topic,
        blinker::ByteView payload,
        const blinker::MqttPublishOptions& options) override {
        if (failPublish) return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        if (!connectedState || options.qos != 0U || options.retained ||
            payload.data == nullptr || payload.size > sizeof(published)) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        publishedTopic = topic;
        std::memcpy(published, payload.data, payload.size);
        publishedSize = payload.size;
        ++publishCalls;
        return blinker::Result::success();
    }

    void poll(uint32_t) override {}

    void setMessageHandler(
        blinker::MqttMessageHandler next,
        void* context) override {
        handler = next;
        handlerContext = context;
    }

    void inject(blinker::StringView topic, blinker::ByteView payload) {
        if (handler == nullptr) return;
        blinker::MqttMessageInfo info;
        info.qos = 0U;
        info.qosKnown = true;
        info.retained = false;
        info.retainedKnown = true;
        handler(handlerContext, topic, payload, info);
    }

    blinker::MqttMessageHandler handler;
    void* handlerContext;
    blinker::MqttConnectOptions connectOptions;
    blinker::StringView subscribedTopic;
    blinker::StringView publishedTopic;
    uint8_t published[256U];
    size_t publishedSize;
    uint32_t connectCalls;
    uint32_t subscribeCalls;
    uint32_t publishCalls;
    uint32_t disconnectCalls;
    bool connectedState;
    bool failPublish;
};

struct FrameCapture {
    FrameCapture()
        : calls(0U), size(0U), sessionId(0U), authenticated(false) {}

    uint32_t calls;
    size_t size;
    uint32_t sessionId;
    bool authenticated;
};

void captureFrame(
    void* context,
    blinker::ByteView frame,
    const blinker::RxContext& rx) {
    FrameCapture* capture = static_cast<FrameCapture*>(context);
    ++capture->calls;
    capture->size = frame.size;
    capture->sessionId = rx.sessionId;
    capture->authenticated = rx.authenticated;
}

struct TransactionSnapshot {
    uint8_t requestId[blinker::kDeviceKeyRequestIdSize];
    uint8_t locator[blinker::kDeviceKeyLocatorSize];
    uint8_t clientNonce[blinker::kDeviceKeyNonceSize];
    uint8_t challengeId[blinker::kDeviceKeyChallengeIdSize];
    uint8_t serverNonce[blinker::kDeviceKeyNonceSize];
};

struct Fixture {
    FakeKeySource keySource;
    blinker::DeviceInstanceId instance;
    SequenceRandom random;
    FakeHttpClient http;
    FakeClock clock;
    uint8_t requestBuffer[
        blinker::kHttpDeviceKeySessionMinimumRequestBufferSize];
    uint8_t responseBuffer[
        blinker::kHttpDeviceKeySessionMinimumResponseBufferSize];
    char credentialArena[blinker::kDeviceKeySessionCredentialArenaSize];
    blinker::HttpDeviceKeySessionConfig config;
    blinker::HttpDeviceKeySessionProvider provider;

    Fixture()
        : keySource(),
          instance(),
          random(),
          http(),
          clock(),
          requestBuffer(),
          responseBuffer(),
          credentialArena(),
          config(makeConfig()),
          provider(
              keySource,
              initializeInstance(instance),
              random,
              http,
              clock,
              config,
              blinker::MutableByteSpan(
                  requestBuffer, sizeof(requestBuffer)),
              blinker::MutableByteSpan(
                  responseBuffer, sizeof(responseBuffer)),
              blinker::MutableCharSpan(
                  credentialArena, sizeof(credentialArena))) {}

    static const blinker::DeviceInstanceId& initializeInstance(
        blinker::DeviceInstanceId& value) {
        for (size_t index = 0U; index < sizeof(value.bytes); ++index) {
            value.bytes[index] = static_cast<uint8_t>(0x40U + index);
        }
        return value;
    }

    static blinker::HttpDeviceKeySessionConfig makeConfig() {
        blinker::HttpDeviceKeySessionConfig value;
        value.host = blinker::StringView("gateway.example.com");
        value.port = 80U;
        value.firmwareVersion = blinker::StringView("2.0.0-test");
        value.httpSecurity = blinker::HttpSecurity::PlainTcp;
        value.mqttSecurity = blinker::MqttSecurity::PlainTcp;
        value.initialRetryMs = 10U;
        value.maximumRetryMs = 80U;
        value.maximumServerRetryMs = 2000U;
        return value;
    }
};

void checkHttp(const Fixture& fixture, const char* target) {
    CHECK_EQ(blinker::HttpMethod::Post, fixture.http.request.method);
    CHECK_EQ(blinker::HttpSecurity::PlainTcp,
             fixture.http.request.security);
    CHECK_TRUE(equals(fixture.http.request.target, target));
    CHECK_TRUE(equals(fixture.http.request.contentType, "application/cbor"));
    CHECK_EQ(static_cast<uint8_t>(2U), fixture.http.request.headerCount);
    CHECK_TRUE(equals(fixture.http.request.headers[0].name, "Accept"));
    CHECK_TRUE(equals(
        fixture.http.request.headers[1].value,
        "device-key-session/1"));
}

TransactionSnapshot readChallengeRequest(Fixture& fixture) {
    checkHttp(fixture, "/api/v2/device-auth/challenges");
    blinker::DeviceKeyChallengeRequest request;
    CHECK_TRUE(blinker::decodeDeviceKeyChallengeRequest(
        fixture.http.request.body, request));
    TransactionSnapshot snapshot = {};
    std::memcpy(snapshot.requestId, request.requestId.data,
                sizeof(snapshot.requestId));
    std::memcpy(snapshot.locator, request.locator.data,
                sizeof(snapshot.locator));
    std::memcpy(snapshot.clientNonce, request.clientNonce.data,
                sizeof(snapshot.clientNonce));
    for (size_t index = 0U; index < sizeof(snapshot.challengeId); ++index) {
        snapshot.challengeId[index] = static_cast<uint8_t>(0x60U + index);
        snapshot.serverNonce[index] = static_cast<uint8_t>(0x70U + index);
    }
    blinker::DeviceKeyLocator expected;
    CHECK_TRUE(blinker::deriveDeviceKeyLocator(
        fixture.keySource.key, expected));
    CHECK_TRUE(std::memcmp(
        snapshot.locator, expected.bytes, sizeof(expected.bytes)) == 0);
    CHECK_TRUE(request.deviceInstanceId.size == sizeof(fixture.instance.bytes) &&
               std::memcmp(
                   request.deviceInstanceId.data,
                   fixture.instance.bytes,
                   sizeof(fixture.instance.bytes)) == 0);
    char keyText[blinker::kDeviceKeyTextSize];
    blinker::StringView encodedKey;
    CHECK_TRUE(blinker::encodeDeviceKey(
        fixture.keySource.key,
        blinker::MutableCharSpan(keyText, sizeof(keyText)),
        encodedKey));
    CHECK_TRUE(!contains(
        fixture.http.request.body,
        blinker::ByteView(
            fixture.keySource.key.bytes,
            sizeof(fixture.keySource.key.bytes))));
    CHECK_TRUE(!contains(
        fixture.http.request.body,
        blinker::ByteView(
            reinterpret_cast<const uint8_t*>(encodedKey.data),
            encodedKey.size)));
    for (uint8_t index = 0U;
         index < fixture.http.request.headerCount;
         ++index) {
        CHECK_TRUE(!(fixture.http.request.headers[index].value.size ==
                         encodedKey.size &&
                     std::memcmp(
                         fixture.http.request.headers[index].value.data,
                         encodedKey.data,
                         encodedKey.size) == 0));
    }
    return snapshot;
}

void completeChallenge(
    Fixture& fixture,
    const TransactionSnapshot& snapshot,
    uint32_t credentialVersion = 7U,
    uint32_t lifetimeSeconds = 120U) {
    blinker::DeviceKeyChallengeResponse response;
    response.requestId = blinker::ByteView(
        snapshot.requestId, sizeof(snapshot.requestId));
    response.challengeId = blinker::ByteView(
        snapshot.challengeId, sizeof(snapshot.challengeId));
    response.serverNonce = blinker::ByteView(
        snapshot.serverNonce, sizeof(snapshot.serverNonce));
    response.credentialVersion = credentialVersion;
    response.expiresInSeconds = lifetimeSeconds;
    uint8_t encodedBuffer[128U];
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodeDeviceKeyChallengeResponse(
        response,
        blinker::MutableByteSpan(encodedBuffer, sizeof(encodedBuffer)),
        encoded));
    fixture.http.complete(200U, encoded);
    fixture.provider.poll(1000U);
}

blinker::DeviceKeySessionRequest verifySessionRequest(
    Fixture& fixture,
    const TransactionSnapshot& snapshot,
    uint32_t credentialVersion = 7U) {
    checkHttp(fixture, "/api/v2/device-sessions");
    blinker::DeviceKeySessionRequest request;
    CHECK_TRUE(blinker::decodeDeviceKeySessionRequest(
        fixture.http.request.body, request));
    blinker::DeviceKeySessionProofContext context;
    context.requestId = blinker::ByteView(
        snapshot.requestId, sizeof(snapshot.requestId));
    context.locator = blinker::ByteView(
        snapshot.locator, sizeof(snapshot.locator));
    context.deviceInstanceId = blinker::ByteView(
        fixture.instance.bytes, sizeof(fixture.instance.bytes));
    context.clientNonce = blinker::ByteView(
        snapshot.clientNonce, sizeof(snapshot.clientNonce));
    context.challengeId = blinker::ByteView(
        snapshot.challengeId, sizeof(snapshot.challengeId));
    context.serverNonce = blinker::ByteView(
        snapshot.serverNonce, sizeof(snapshot.serverNonce));
    context.credentialVersion = credentialVersion;
    context.protocolVersion = 2U;
    context.firmwareVersion = fixture.config.firmwareVersion;
    context.requestedSecurity = fixture.config.mqttSecurity;
    context.proofAlgorithm = blinker::DeviceKeyProofAlgorithm::HmacSha256;
    CHECK_TRUE(blinker::verifyDeviceKeySessionProof(
        fixture.keySource.key, context, request.proof));
    return request;
}

void completeSession(
    Fixture& fixture,
    const TransactionSnapshot& snapshot,
    uint32_t credentialVersion = 7U, uint8_t proofSeed = 0x91U,
    uint32_t expiresInSeconds = 120U) {
    uint8_t sessionId[blinker::kDeviceKeySessionIdSize];
    for (size_t index = 0U; index < sizeof(sessionId); ++index) {
        sessionId[index] = static_cast<uint8_t>(proofSeed + index);
    }
    blinker::DeviceKeySessionResponse response;
    response.requestId = blinker::ByteView(
        snapshot.requestId, sizeof(snapshot.requestId));
    response.credentialVersion = credentialVersion;
    response.broker = blinker::BrokerKind::Blinker;
    response.host = blinker::StringView("mqtt.example.com");
    response.port = 1883U;
    response.security = blinker::MqttSecurity::PlainTcp;
    response.clientId = blinker::StringView("client-7");
    response.username = blinker::StringView("user-7");
    response.password = blinker::StringView("short-session-secret");
    response.publishTopic = blinker::StringView("/v2/device/7/up");
    response.subscribeTopic = blinker::StringView("/v2/device/7/down");
    response.sessionId = blinker::ByteView(sessionId, sizeof(sessionId));
    response.expiresInSeconds = expiresInSeconds;
    response.refreshAfterSeconds = expiresInSeconds / 2U;
    uint8_t encodedBuffer[640U];
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodeDeviceKeySessionResponse(
        response,
        blinker::MutableByteSpan(encodedBuffer, sizeof(encodedBuffer)),
        encoded));
    fixture.http.complete(200U, encoded);
    fixture.provider.poll(1000U);
}

void completeError(
    Fixture& fixture,
    uint16_t status,
    blinker::DeviceKeySessionWireError error,
    blinker::DeviceKeySessionRetryClass retryClass) {
    blinker::DeviceKeySessionErrorBody body;
    body.error = error;
    body.retryClass = retryClass;
    uint8_t buffer[64U];
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodeDeviceKeySessionErrorBody(
        body, blinker::MutableByteSpan(buffer, sizeof(buffer)), encoded));
    fixture.http.complete(status, encoded);
    fixture.provider.poll(1000U);
}

void testSuccessfulExchangeAndRefresh() {
    Fixture fixture;
    CHECK_TRUE(fixture.provider.start());
    CHECK_EQ(blinker::SessionProviderState::Requesting,
             fixture.provider.state());
    CHECK_EQ(static_cast<uint32_t>(2U), fixture.random.calls);
    const TransactionSnapshot first = readChallengeRequest(fixture);
    completeChallenge(fixture, first);
    CHECK_EQ(blinker::SessionProviderState::Requesting,
             fixture.provider.state());
    (void)verifySessionRequest(fixture, first);
    CHECK_TRUE(fixture.keySource.loads >= 3U);
    completeSession(fixture, first);
    CHECK_EQ(16U, fixture.provider.proofSessionId().size);
    CHECK_EQ(0x91U, fixture.provider.proofSessionId().data[0]);
    CHECK_EQ(blinker::SessionProviderState::Ready, fixture.provider.state());
    CHECK_EQ(static_cast<uint32_t>(7U),
             fixture.provider.authenticatedCredentialVersion());

    blinker::SessionCredentials credentials;
    CHECK_TRUE(fixture.provider.credentials(credentials));
    CHECK_TRUE(equals(credentials.host, "mqtt.example.com"));
    CHECK_TRUE(equals(credentials.password, "short-session-secret"));
    CHECK_EQ(static_cast<uint32_t>(1U),
             fixture.provider.credentialGeneration());

    CHECK_TRUE(fixture.provider.requestRefresh());
    CHECK_EQ(16U, fixture.provider.proofSessionId().size);
    CHECK_EQ(0xA0U, fixture.provider.proofSessionId().data[15]);
    CHECK_EQ(blinker::SessionProviderState::Requesting,
             fixture.provider.state());
    blinker::SessionCredentials refreshingCredentials;
    CHECK_TRUE(fixture.provider.credentials(refreshingCredentials));
    CHECK_TRUE(equals(refreshingCredentials.host, "mqtt.example.com"));
    CHECK_TRUE(equals(
        refreshingCredentials.password, "short-session-secret"));
    CHECK_EQ(static_cast<uint32_t>(4U), fixture.random.calls);
    const TransactionSnapshot second = readChallengeRequest(fixture);
    CHECK_TRUE(std::memcmp(
        first.requestId, second.requestId, sizeof(first.requestId)) != 0);
    fixture.provider.stop();
    CHECK_TRUE(fixture.provider.proofSessionId().empty());
    CHECK_EQ(blinker::SessionProviderState::Stopped,
             fixture.provider.state());
}

void testChallengeExpiredRestartsAndRotatedStops() {
    Fixture fixture;
    CHECK_TRUE(fixture.provider.start());
    const TransactionSnapshot first = readChallengeRequest(fixture);
    completeChallenge(fixture, first, 7U, 1U);
    (void)verifySessionRequest(fixture, first);
    fixture.clock.advance(1100U);
    completeError(
        fixture,
        409U,
        blinker::DeviceKeySessionWireError::ChallengeExpired,
        blinker::DeviceKeySessionRetryClass::NewChallenge);
    CHECK_EQ(blinker::SessionProviderState::Requesting,
             fixture.provider.state());
    CHECK_EQ(static_cast<uint32_t>(4U), fixture.random.calls);
    const TransactionSnapshot second = readChallengeRequest(fixture);
    CHECK_TRUE(std::memcmp(
        first.requestId, second.requestId, sizeof(first.requestId)) != 0);

    completeError(
        fixture,
        403U,
        blinker::DeviceKeySessionWireError::DeviceKeyRotated,
        blinker::DeviceKeySessionRetryClass::AuthorizationRequired);
    CHECK_EQ(blinker::SessionProviderState::Failed, fixture.provider.state());
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired,
             fixture.provider.lastError());
    CHECK_TRUE(fixture.provider.hasWireError());
    CHECK_EQ(blinker::DeviceKeySessionWireError::DeviceKeyRotated,
             fixture.provider.lastWireError());
    blinker::SessionCredentials credentials;
    CHECK_EQ(blinker::ErrorCode::NotConfigured,
             fixture.provider.credentials(credentials).code());
}

void testNetworkPausePreservesExactTransaction() {
    Fixture fixture;
    CHECK_TRUE(fixture.provider.start());
    const TransactionSnapshot first = readChallengeRequest(fixture);
    fixture.provider.setNetworkAvailable(false);
    CHECK_EQ(blinker::SessionProviderState::Idle, fixture.provider.state());
    fixture.provider.setNetworkAvailable(true);
    fixture.provider.poll(1000U);
    const TransactionSnapshot retry = readChallengeRequest(fixture);
    CHECK_TRUE(std::memcmp(
        first.requestId, retry.requestId, sizeof(first.requestId)) == 0);
    CHECK_TRUE(std::memcmp(
        first.clientNonce, retry.clientNonce, sizeof(first.clientNonce)) == 0);
    CHECK_EQ(static_cast<uint32_t>(2U), fixture.random.calls);
}

void testMalformedTransientResponseBacksOffAndRetries() {
    Fixture fixture;
    CHECK_TRUE(fixture.provider.start());
    const TransactionSnapshot first = readChallengeRequest(fixture);
    static const uint8_t unavailable[] = {
        '{', '"', 'e', 'r', 'r', 'o', 'r', '"', ':', '"',
        'u', 'n', 'a', 'v', 'a', 'i', 'l', 'a', 'b', 'l', 'e',
        '"', '}'};
    fixture.http.complete(
        503U,
        blinker::ByteView(unavailable, sizeof(unavailable)));
    fixture.provider.poll(1000U);
    CHECK_EQ(
        blinker::SessionProviderState::Backoff,
        fixture.provider.state());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, fixture.provider.lastError());

    fixture.clock.advance(fixture.config.initialRetryMs);
    fixture.provider.poll(1000U);
    CHECK_EQ(
        blinker::SessionProviderState::Requesting,
        fixture.provider.state());
    const TransactionSnapshot retry = readChallengeRequest(fixture);
    CHECK_TRUE(std::memcmp(
        first.requestId, retry.requestId, sizeof(first.requestId)) == 0);
    CHECK_TRUE(std::memcmp(
        first.clientNonce, retry.clientNonce, sizeof(first.clientNonce)) == 0);
}

void testInterruptedHttpRetriesExactTransaction() {
    using namespace blinker;
    for (const bool sessionPhase : {false, true}) {
        Fixture f;
        CHECK_TRUE(f.provider.start());
        const TransactionSnapshot transaction = readChallengeRequest(f);
        if (sessionPhase) completeChallenge(f, transaction);
        uint8_t original[kHttpDeviceKeySessionMinimumRequestBufferSize];
        const size_t size = f.http.request.body.size;
        CHECK_TRUE(size <= sizeof(original));
        std::memcpy(original, f.http.request.body.data, size);
        const uint32_t starts = f.http.startCalls;
        // Socket closed before the HTTP headers or Content-Length completed.
        f.http.currentState = HttpClientState::Failed;
        f.http.error = ErrorCode::TruncatedInput;
        f.provider.poll(1000U);
        CHECK_EQ(SessionProviderState::Backoff, f.provider.state());
        CHECK_EQ(ErrorCode::TruncatedInput, f.provider.lastError());
        f.clock.advance(f.config.initialRetryMs);
        f.provider.poll(1000U);
        CHECK_EQ(starts + 1U, f.http.startCalls);
        CHECK_EQ(SessionProviderState::Requesting, f.provider.state());
        CHECK_EQ(ErrorCode::Ok, f.provider.lastError());
        CHECK_EQ(size, f.http.request.body.size);
        CHECK_TRUE(std::memcmp(original, f.http.request.body.data, size) == 0);
        if (!sessionPhase) completeChallenge(f, transaction);
        (void)verifySessionRequest(f, transaction);
        completeSession(f, transaction);
        CHECK_EQ(SessionProviderState::Ready, f.provider.state());
        CHECK_EQ(1U, f.provider.credentialGeneration());
    }
}

void testCompletedMalformedCborDoesNotRetry() {
    using namespace blinker;
    for (const bool sessionPhase : {false, true}) {
        Fixture f;
        CHECK_TRUE(f.provider.start());
        const TransactionSnapshot transaction = readChallengeRequest(f);
        if (sessionPhase) completeChallenge(f, transaction);
        const uint32_t starts = f.http.startCalls;
        // HTTP completed, but the CBOR map itself is truncated: not a
        // transport failure and never grounds to accept/refresh credentials.
        const uint8_t invalid[] = {0xa1U};
        f.http.complete(200U, ByteView(invalid, sizeof(invalid)));
        f.provider.poll(1000U);
        CHECK_EQ(SessionProviderState::Failed, f.provider.state());
        CHECK_TRUE(f.provider.lastError() != ErrorCode::Ok);
        f.clock.advance(100000U);
        f.provider.poll(1000U);
        CHECK_EQ(starts, f.http.startCalls);
        CHECK_EQ(0U, f.provider.credentialGeneration());
    }
}

void testExpiredSessionDuringOutageKeepsRetrying() {
    Fixture fixture;
    CHECK_TRUE(fixture.provider.start());
    const TransactionSnapshot first = readChallengeRequest(fixture);
    completeChallenge(fixture, first);
    (void)verifySessionRequest(fixture, first);
    completeSession(fixture, first);
    CHECK_EQ(
        blinker::SessionProviderState::Ready,
        fixture.provider.state());

    fixture.clock.advance(60000U);
    fixture.provider.poll(1000U);
    CHECK_EQ(
        blinker::SessionProviderState::Requesting,
        fixture.provider.state());
    const TransactionSnapshot refresh = readChallengeRequest(fixture);

    static const uint8_t unavailable[] = {
        '{', '"', 'e', 'r', 'r', 'o', 'r', '"', ':', '"',
        'u', 'n', 'a', 'v', 'a', 'i', 'l', 'a', 'b', 'l', 'e',
        '"', '}'};
    fixture.http.complete(
        503U,
        blinker::ByteView(unavailable, sizeof(unavailable)));
    fixture.provider.poll(1000U);
    CHECK_EQ(
        blinker::SessionProviderState::Backoff,
        fixture.provider.state());

    fixture.clock.advance(60000U);
    fixture.provider.poll(1000U);
    CHECK_EQ(
        blinker::SessionProviderState::Requesting,
        fixture.provider.state());
    const TransactionSnapshot retry = readChallengeRequest(fixture);
    CHECK_TRUE(std::memcmp(
        refresh.requestId, retry.requestId, sizeof(refresh.requestId)) == 0);

    completeChallenge(fixture, retry);
    (void)verifySessionRequest(fixture, retry);
    completeSession(fixture, retry);
    CHECK_EQ(
        blinker::SessionProviderState::Ready,
        fixture.provider.state());
    CHECK_EQ(static_cast<uint32_t>(2U),
             fixture.provider.credentialGeneration());
}

void testDeviceKeyToMqttDataPlane() {
    Fixture fixture;
    FakeMqttClient mqtt;
    blinker::MqttFrameTransportConfig mqttConfig;
    mqttConfig.maxFrameSize = sizeof(mqtt.published);
    mqttConfig.publishQos = 0U;
    mqttConfig.subscribeQos = 0U;
    blinker::MqttFrameTransport mqttTransport(
        mqtt, fixture.clock, mqttConfig);
    blinker::ManagedMqttTransport cloud(
        fixture.provider, mqttTransport);
    FrameCapture received;
    cloud.setReceiver(&captureFrame, &received);

    // The product lifecycle starts the cloud path before WiFi association but
    // must not perform HTTP or MQTT I/O until the network gate is opened.
    cloud.setNetworkAvailable(false);
    CHECK_TRUE(cloud.start());
    cloud.poll(2000U);
    CHECK_EQ(static_cast<uint32_t>(0U), fixture.http.startCalls);
    CHECK_EQ(static_cast<uint32_t>(0U), mqtt.connectCalls);

    cloud.setNetworkAvailable(true);
    cloud.poll(2000U);
    const TransactionSnapshot transaction =
        readChallengeRequest(fixture);
    completeChallenge(fixture, transaction);
    (void)verifySessionRequest(fixture, transaction);
    completeSession(fixture, transaction);

    // A ready DeviceKey session is not cloud-ready until MQTT CONNECT and the
    // exact downlink subscription have both succeeded.
    CHECK_EQ(blinker::SessionProviderState::Ready,
             fixture.provider.state());
    CHECK_TRUE(cloud.state() != blinker::TransportState::Online);
    cloud.poll(2000U);
    CHECK_EQ(blinker::TransportState::Online, cloud.state());
    CHECK_EQ(static_cast<uint32_t>(1U), mqtt.connectCalls);
    CHECK_EQ(static_cast<uint32_t>(1U), mqtt.subscribeCalls);
    CHECK_TRUE(equals(mqtt.connectOptions.host, "mqtt.example.com"));
    CHECK_EQ(static_cast<uint16_t>(1883U), mqtt.connectOptions.port);
    CHECK_EQ(blinker::MqttSecurity::PlainTcp,
             mqtt.connectOptions.security);
    CHECK_TRUE(equals(mqtt.connectOptions.clientId, "client-7"));
    CHECK_TRUE(equals(mqtt.connectOptions.username, "user-7"));
    CHECK_TRUE(equals(
        mqtt.connectOptions.password, "short-session-secret"));
    CHECK_TRUE(equals(mqtt.subscribedTopic, "/v2/device/7/down"));

    const uint8_t frame[] = {
        blinker::bbp2::kMagic0,
        blinker::bbp2::kMagic1,
        blinker::bbp2::kVersion,
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello),
        blinker::bbp2::FlagNone,
        blinker::bbp2::kBaseHeaderSize,
        0U,
        1U,
        0U,
        0U};
    blinker::SendTarget target;
    CHECK_TRUE(cloud.send(
        blinker::ByteView(frame, sizeof(frame)), target));
    CHECK_EQ(static_cast<uint32_t>(1U), mqtt.publishCalls);
    CHECK_TRUE(equals(mqtt.publishedTopic, "/v2/device/7/up"));
    CHECK_EQ(sizeof(frame), mqtt.publishedSize);
    CHECK_TRUE(std::memcmp(frame, mqtt.published, sizeof(frame)) == 0);

    mqtt.inject(
        blinker::StringView("/v2/device/7/down"),
        blinker::ByteView(frame, sizeof(frame)));
    CHECK_EQ(static_cast<uint32_t>(1U), received.calls);
    CHECK_EQ(sizeof(frame), received.size);
    CHECK_EQ(static_cast<uint32_t>(1U), received.sessionId);
    CHECK_TRUE(received.authenticated);

    cloud.stop();
    CHECK_EQ(blinker::TransportState::Stopped, cloud.state());
    CHECK_TRUE(!mqtt.connected());
}

size_t controlFrame(const blinker::DeviceKey& key, blinker::local_access::ControlView value,
                    uint8_t* output, size_t capacity) {
    uint8_t mac[32U];
    CHECK_TRUE(blinker::security::computeLocalAccessControlAuthenticator(key, value,
        blinker::MutableByteSpan(mac, sizeof(mac))));
    value.authenticator = blinker::ByteView(mac, sizeof(mac));
    size_t size = 0U;
    CHECK_TRUE(blinker::local_access::encodeControl(value,
        blinker::MutableByteSpan(output + 10U, capacity - 10U), size));
    blinker::bbp2::FrameHeader header;
    header.kind = 0x36U; header.bodyLength = static_cast<uint16_t>(size);
    CHECK_TRUE(blinker::bbp2::encodeHeader(header, blinker::MutableByteSpan(output, 10U)));
    return size + 10U;
}

void testLocalCloudControlComposition() {
    using namespace blinker;
    using namespace blinker::local_access;
    Fixture f;
    FakeMqttClient mqtt;
    MqttFrameTransport raw(mqtt, f.clock);
    ManagedMqttTransport managed(f.provider, raw);
    SelfLocalAccessTargetPolicy policy;
    LocalAccessCloudTransport<1U> cloud(managed, raw, f.provider, f.instance,
        f.keySource, f.random, f.clock, policy);
    FrameCapture ordinary;
    cloud.setReceiver(&captureFrame, &ordinary);
    CHECK_TRUE(cloud.start());
    const uint32_t waitingGeneration = cloud.authority().challengeGeneration();
    for (size_t tick = 0U; tick < 100U; ++tick) cloud.poll(2000U);
    CHECK_EQ(waitingGeneration, cloud.authority().challengeGeneration());
    const TransactionSnapshot login = readChallengeRequest(f);
    completeChallenge(f, login); completeSession(f, login, 7U, 0x91U, 3600U);
    cloud.poll(2000U);
    CHECK_EQ(1U, mqtt.connectCalls);
    CHECK_EQ(1U, mqtt.publishCalls);
    uint8_t hello[122U]; const size_t helloSize = mqtt.publishedSize;
    std::memcpy(hello, mqtt.published, helloSize);
    bbp2::FrameView frame;
    CHECK_TRUE(bbp2::parseFrame(ByteView(hello, helloSize), frame));
    ProgressView query;
    CHECK_TRUE(decodeProgress(frame.body, query));
    CHECK_TRUE(security::verifyLocalAccessProgressAuthenticator(f.keySource.key, query, f.provider.proofSessionId()));
    ControlView command;
    command.recipientDeviceInstanceId = query.recipientDeviceInstanceId;
    command.deviceKeyVersion = query.deviceKeyVersion; command.controlSessionId = query.controlSessionId;
    command.kind = ControlKind::Command; command.operation = ControlOperation::Enable;
    command.sequence = 1U; command.authorityRevision = 11U;
    uint8_t wire[122U]; size_t wireSize = controlFrame(f.keySource.key, command, wire, sizeof(wire));
    const uint32_t loads = f.keySource.loads;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize));
    CHECK_EQ(loads, f.keySource.loads); // SDK callback only copies bounded bytes.
    CHECK_EQ(1U, mqtt.publishCalls);
    CHECK_TRUE(!cloud.authority().configured());
    // Full inbox retains the first packet, never overwrites it with a second.
    wire[wireSize - 1U] ^= 1U;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize));
    wire[wireSize - 1U] ^= 1U;
    cloud.poll(2000U);
    CHECK_TRUE(cloud.authority().enabled()); CHECK_EQ(11U, cloud.authority().authorityRevision());
    CHECK_EQ(2U, mqtt.publishCalls); CHECK_EQ(0U, ordinary.calls);
    uint8_t ack[122U]; const size_t ackSize = mqtt.publishedSize;
    std::memcpy(ack, mqtt.published, ackSize);
    CHECK_TRUE(bbp2::parseFrame(ByteView(ack, ackSize), frame));
    ControlView receipt; CHECK_TRUE(decodeControl(frame.body, receipt));
    CHECK_EQ(ControlKind::Ack, receipt.kind);
    CHECK_TRUE(security::verifyLocalAccessControlAuthenticator(f.keySource.key, receipt));
    const uint32_t challengeGeneration = cloud.authority().challengeGeneration();
    mqtt.failPublish = true;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_EQ(ErrorCode::WouldBlock, cloud.controlError()); CHECK_EQ(2U, mqtt.publishCalls);
    CHECK_TRUE(cloud.authority().enabled());
    mqtt.failPublish = false;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_EQ(challengeGeneration, cloud.authority().challengeGeneration());
    CHECK_EQ(ackSize, mqtt.publishedSize); CHECK_TRUE(std::memcmp(ack, mqtt.published, ackSize) == 0);
    // Invalid MAC/direction/outer flags never mutate authority or emit ACK.
    const uint32_t published = mqtt.publishCalls;
    wire[wireSize - 1U] ^= 1U; mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    wire[wireSize - 1U] ^= 1U;
    mqtt.inject(mqtt.subscribedTopic, ByteView(ack, ackSize)); cloud.poll(2000U);
    wire[4] = bbp2::FlagAckRequired; mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    wire[4] = 0U; CHECK_EQ(published, mqtt.publishCalls);
    // Non-control data still reaches the original Runtime handler.
    const uint8_t data[] = {0x42, 0x4b, 2, 1, 0, 10, 0, 0, 0, 0};
    mqtt.inject(mqtt.subscribedTopic, ByteView(data, sizeof(data)));
    CHECK_EQ(1U, ordinary.calls);
    // Discard old physical-connection inbox; a reconnect keeps boot/authority.
    command.sequence = 2U; command.operation = ControlOperation::Disable;
    wireSize = controlFrame(f.keySource.key, command, wire, sizeof(wire));
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize));
    cloud.setNetworkAvailable(false); cloud.poll(2000U);
    CHECK_TRUE(cloud.authority().enabled());
    cloud.setNetworkAvailable(true); cloud.poll(2000U);
    CHECK_TRUE(cloud.authority().enabled());
    CHECK_TRUE(bbp2::parseFrame(ByteView(mqtt.published, mqtt.publishedSize), frame));
    ProgressView resumed; CHECK_TRUE(decodeProgress(frame.body, resumed));
    CHECK_EQ(1U, resumed.sequence);
    CHECK_TRUE(std::memcmp(resumed.controlSessionId.data, command.controlSessionId.data, 16U) == 0);
    // A current retransmission applies; same-proof old inbox never did.
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_TRUE(!cloud.authority().enabled());
    // Fresh successful login changes Hello proof, not the running boot nonce.
    CHECK_TRUE(f.provider.requestRefresh());
    const TransactionSnapshot refresh = readChallengeRequest(f);
    completeChallenge(f, refresh); completeSession(f, refresh, 7U, 0xB1U, 3600U);
    cloud.poll(2000U);
    CHECK_TRUE(bbp2::parseFrame(ByteView(mqtt.published, mqtt.publishedSize), frame));
    CHECK_TRUE(decodeProgress(frame.body, resumed)); CHECK_EQ(2U, resumed.sequence);
    CHECK_TRUE(std::memcmp(resumed.controlSessionId.data, command.controlSessionId.data, 16U) == 0);
    CHECK_TRUE(security::verifyLocalAccessProgressAuthenticator(f.keySource.key, resumed, f.provider.proofSessionId()));
    CHECK_EQ(0xB1U, f.provider.proofSessionId().data[0]);
    // Fresh signed synchronized observations keep an idle device healthy,
    // without turning absent commands into periodic credential churn.
    const uint32_t idleRequests = f.http.startCalls;
    for (uint32_t now = 0U; now <= 540000U; now += 60000U) {
        f.clock.now = now; cloud.poll(2000U);
        CHECK_TRUE(bbp2::parseFrame(ByteView(mqtt.published, mqtt.publishedSize), frame));
        CHECK_TRUE(decodeProgress(frame.body, resumed));
        resumed.kind = ProgressKind::Status; resumed.state = ProgressState::Synchronized;
        uint8_t mac[32U];
        CHECK_TRUE(security::computeLocalAccessProgressAuthenticator(f.keySource.key, resumed, f.provider.proofSessionId(), MutableByteSpan(mac, sizeof(mac))));
        resumed.authenticator = ByteView(mac, sizeof(mac));
        size_t count = 0U;
        CHECK_TRUE(encodeProgress(resumed, MutableByteSpan(wire + 10U, sizeof(wire) - 10U), count));
        bbp2::FrameHeader h; h.kind = 0x36U; h.bodyLength = static_cast<uint16_t>(count);
        CHECK_TRUE(bbp2::encodeHeader(h, MutableByteSpan(wire, 10U)));
        mqtt.inject(mqtt.subscribedTopic, ByteView(wire, count + 10U)); cloud.poll(2000U);
        CHECK_EQ(ErrorCode::Ok, cloud.controlError());
    }
    CHECK_EQ(idleRequests, f.http.startCalls);
    // Key generation change retires the old receiver; an old valid MAC must
    // not cross into the new boot/Key control session.
    CHECK_TRUE(f.provider.requestRefresh());
    const TransactionSnapshot rotated = readChallengeRequest(f);
    completeChallenge(f, rotated, 8U); completeSession(f, rotated, 8U, 0xC1U, 3600U);
    cloud.poll(2000U);
    CHECK_TRUE(!cloud.authority().configured());
    const uint32_t beforeStale = mqtt.publishCalls;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_EQ(beforeStale, mqtt.publishCalls); CHECK_TRUE(!cloud.authority().configured());
    // Test rejection from an ENABLED authority, not an already-cleared one.
    // The new Key version needs its own current control-session Enable/ACK.
    CHECK_TRUE(bbp2::parseFrame(ByteView(mqtt.published, mqtt.publishedSize), frame));
    CHECK_TRUE(decodeProgress(frame.body, resumed));
    command.recipientDeviceInstanceId = resumed.recipientDeviceInstanceId;
    command.deviceKeyVersion = resumed.deviceKeyVersion;
    command.controlSessionId = resumed.controlSessionId;
    command.sequence = 1U; command.authorityRevision = 12U;
    command.operation = ControlOperation::Enable;
    wireSize = controlFrame(f.keySource.key, command, wire, sizeof(wire));
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_TRUE(cloud.authority().enabled());
    CHECK_TRUE(f.provider.requestRefresh());
    CHECK_TRUE(cloud.authority().enabled()); // Refresh is not evidence of revocation.
    completeError(f, 403U, DeviceKeySessionWireError::DeviceKeyRotated,
                  DeviceKeySessionRetryClass::AuthorizationRequired);
    cloud.poll(2000U); CHECK_TRUE(!cloud.authority().configured());
    CHECK_EQ(0U, f.provider.authenticatedCredentialVersion());
    CHECK_TRUE(f.provider.proofSessionId().empty());
    const uint32_t rejectedPublishes = mqtt.publishCalls;
    const uint32_t rejectedRequests = f.http.startCalls;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize));
    for (size_t tick = 0U; tick < 100U; ++tick) cloud.poll(2000U);
    CHECK_TRUE(!cloud.authority().configured());
    CHECK_EQ(rejectedPublishes, mqtt.publishCalls);
    CHECK_EQ(rejectedRequests, f.http.startCalls); // No automatic retry of a rejected Key.
    cloud.stop(); CHECK_TRUE(!cloud.authority().configured()); CHECK_TRUE(f.provider.proofSessionId().empty());
    CHECK_EQ(TransportState::Stopped, cloud.state());
}

void testLocalCloudBootstrapBoundedRecovery() {
    using namespace blinker;
    Fixture f; FakeMqttClient mqtt;
    MqttFrameTransport raw(mqtt, f.clock); ManagedMqttTransport managed(f.provider, raw);
    SelfLocalAccessTargetPolicy policy;
    LocalAccessCloudTransport<1U> cloud(managed, raw, f.provider, f.instance,
        f.keySource, f.random, f.clock, policy);
    CHECK_TRUE(cloud.start());
    const TransactionSnapshot login = readChallengeRequest(f);
    completeChallenge(f, login); completeSession(f, login, 7U, 0x91U, 3600U);
    cloud.poll(2000U);
    for (const uint32_t now : {1000U, 3000U, 7000U, 15000U, 59999U}) {
        f.clock.now = now; cloud.poll(2000U);
    }
    CHECK_EQ(4U, mqtt.publishCalls); CHECK_EQ(SessionProviderState::Ready, f.provider.state());
    const uint32_t before = f.http.startCalls;
    f.clock.now = 240000U; cloud.poll(2000U);
    CHECK_EQ(before + 1U, f.http.startCalls); CHECK_EQ(SessionProviderState::Requesting, f.provider.state());
    for (size_t repeat = 0U; repeat < 100U; ++repeat) cloud.poll(2000U);
    CHECK_EQ(before + 1U, f.http.startCalls);
    const TransactionSnapshot refresh = readChallengeRequest(f);
    completeChallenge(f, refresh); completeSession(f, refresh, 7U, 0xB1U, 3600U);
    cloud.poll(2000U); CHECK_EQ(5U, mqtt.publishCalls);
    CHECK_EQ(0xB1U, f.provider.proofSessionId().data[0]);
    // Independent minimum refresh interval survives successful proof replacement.
    for (const uint32_t now : {241000U, 243000U, 247000U, 480000U}) {
        f.clock.now = now; cloud.poll(2000U);
    }
    CHECK_EQ(SessionProviderState::Ready, f.provider.state());
    f.clock.now = 540000U; cloud.poll(2000U);
    CHECK_EQ(SessionProviderState::Requesting, f.provider.state());
}

void testConfiguredProgressRecovery(bool repeatPending) {
    using namespace blinker; using namespace blinker::local_access;
    Fixture f; FakeMqttClient mqtt;
    MqttFrameTransport raw(mqtt, f.clock); ManagedMqttTransport managed(f.provider, raw);
    SelfLocalAccessTargetPolicy policy;
    LocalAccessCloudTransport<1U> cloud(managed, raw, f.provider, f.instance, f.keySource, f.random, f.clock, policy);
    CHECK_TRUE(cloud.start());
    const auto login = readChallengeRequest(f);
    completeChallenge(f, login); completeSession(f, login, 7U, 0x91U, 3600U); cloud.poll(2000U);
    uint8_t queryBytes[122U]; memcpy(queryBytes, mqtt.published, mqtt.publishedSize);
    bbp2::FrameView frame; CHECK_TRUE(bbp2::parseFrame(ByteView(queryBytes, mqtt.publishedSize), frame));
    ProgressView query; CHECK_TRUE(decodeProgress(frame.body, query));
    ControlView enable;
    enable.kind = ControlKind::Command; enable.operation = ControlOperation::Enable;
    enable.recipientDeviceInstanceId = query.recipientDeviceInstanceId; enable.deviceKeyVersion = query.deviceKeyVersion;
    enable.controlSessionId = query.controlSessionId; enable.sequence = 1U; enable.authorityRevision = 11U;
    uint8_t wire[122U]; size_t wireSize = controlFrame(f.keySource.key, enable, wire, sizeof(wire));
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_TRUE(cloud.authority().enabled());
    const uint32_t authorityGeneration = cloud.authority().challengeGeneration();
    auto status = [&](ProgressView value, ProgressState state, ByteView proof) {
        value.kind = ProgressKind::Status; value.state = state;
        uint8_t mac[32U]; size_t count = 0U;
        CHECK_TRUE(security::computeLocalAccessProgressAuthenticator(f.keySource.key, value, proof, MutableByteSpan(mac, sizeof(mac))));
        value.authenticator = ByteView(mac, sizeof(mac));
        CHECK_TRUE(encodeProgress(value, MutableByteSpan(wire + 10U, sizeof(wire) - 10U), count));
        bbp2::FrameHeader h; h.kind = 0x36U; h.bodyLength = static_cast<uint16_t>(count);
        CHECK_TRUE(bbp2::encodeHeader(h, MutableByteSpan(wire, 10U))); return count + 10U;
    };
    // A signed but unacknowledged checkpoint is NOT synchronized merely
    // because the MCU already applied Enable. It produces no ACK of its own.
    wireSize = status(query, ProgressState::Pending, f.provider.proofSessionId());
    uint32_t published = mqtt.publishCalls;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_EQ(ErrorCode::Ok, cloud.controlError()); CHECK_EQ(published, mqtt.publishCalls);
    // The consumed request cannot be used a second time, even with a valid MAC.
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_TRUE(cloud.controlError() != ErrorCode::Ok);
    f.clock.now = 60000U; cloud.poll(2000U);
    memcpy(queryBytes, mqtt.published, mqtt.publishedSize);
    CHECK_TRUE(bbp2::parseFrame(ByteView(queryBytes, mqtt.publishedSize), frame)); CHECK_TRUE(decodeProgress(frame.body, query));
    wireSize = status(query, ProgressState::Synchronized, f.provider.proofSessionId());
    uint8_t previous[122U]; memcpy(previous, wire, wireSize); const size_t previousSize = wireSize;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_EQ(ErrorCode::Ok, cloud.controlError());
    f.clock.now = 120000U; cloud.poll(2000U);
    mqtt.inject(mqtt.subscribedTopic, ByteView(previous, previousSize)); cloud.poll(2000U);
    CHECK_EQ(ErrorCode::AuthenticationRequired, cloud.controlError());
    memcpy(queryBytes, mqtt.published, mqtt.publishedSize);
    CHECK_TRUE(bbp2::parseFrame(ByteView(queryBytes, mqtt.publishedSize), frame)); CHECK_TRUE(decodeProgress(frame.body, query));
    wireSize = status(query, ProgressState::Refresh, f.provider.proofSessionId());
    wire[wireSize - 1U] ^= 1U;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_EQ(ErrorCode::AuthenticationRequired, cloud.controlError());
    CHECK_EQ(SessionProviderState::Ready, f.provider.state());
    wire[wireSize - 1U] ^= 1U;
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U); cloud.poll(2000U);
    CHECK_EQ(SessionProviderState::Requesting, f.provider.state());
    CHECK_EQ(authorityGeneration, cloud.authority().challengeGeneration()); CHECK_TRUE(cloud.authority().enabled());
    const auto refresh = readChallengeRequest(f);
    completeChallenge(f, refresh); completeSession(f, refresh, 7U, 0xB1U, 3600U); cloud.poll(2000U);
    // Old proof/nonce cannot authenticate a fresh query; no local authority reset.
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_EQ(ErrorCode::AuthenticationRequired, cloud.controlError());
    CHECK_EQ(authorityGeneration, cloud.authority().challengeGeneration());
    memcpy(queryBytes, mqtt.published, mqtt.publishedSize);
    CHECK_TRUE(bbp2::parseFrame(ByteView(queryBytes, mqtt.publishedSize), frame)); CHECK_TRUE(decodeProgress(frame.body, query));
    wireSize = status(query, ProgressState::Synchronized, f.provider.proofSessionId());
    f.clock.now = 130000U; // Exact response deadline, even a correct MAC is late.
    mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
    CHECK_TRUE(cloud.controlError() != ErrorCode::Ok);
    // Repeated authenticated Pending without checkpoint advancement must not
    // hide a stuck worker. Test both that case and completely lost responses.
    if (repeatPending) for (const uint32_t now : {180000U, 240000U, 300000U}) {
        f.clock.now = now; cloud.poll(2000U);
        memcpy(queryBytes, mqtt.published, mqtt.publishedSize);
        CHECK_TRUE(bbp2::parseFrame(ByteView(queryBytes, mqtt.publishedSize), frame)); CHECK_TRUE(decodeProgress(frame.body, query));
        wireSize = status(query, ProgressState::Pending, f.provider.proofSessionId());
        mqtt.inject(mqtt.subscribedTopic, ByteView(wire, wireSize)); cloud.poll(2000U);
        CHECK_EQ(ErrorCode::Ok, cloud.controlError());
    }
    // A configured device also recovers when all subsequent progress is lost.
    // The independent 300-second refresh budget cannot be bypassed by a proof
    // change, nor does absence of a reply reset anyone's existing authority.
    f.clock.now = 360000U; cloud.poll(2000U);
    CHECK_EQ(SessionProviderState::Ready, f.provider.state());
    f.clock.now = 420000U; cloud.poll(2000U);
    CHECK_EQ(SessionProviderState::Requesting, f.provider.state());
    CHECK_TRUE(cloud.authority().enabled()); CHECK_EQ(authorityGeneration, cloud.authority().challengeGeneration());
}

} // namespace

int main() {
    testSuccessfulExchangeAndRefresh();
    testChallengeExpiredRestartsAndRotatedStops();
    testNetworkPausePreservesExactTransaction();
    testMalformedTransientResponseBacksOffAndRetries();
    testInterruptedHttpRetriesExactTransaction();
    testCompletedMalformedCborDoesNotRetry();
    testExpiredSessionDuringOutageKeepsRetrying();
    testDeviceKeyToMqttDataPlane();
    testLocalCloudControlComposition();
    testLocalCloudBootstrapBoundedRecovery();
    testConfiguredProgressRecovery(false);
    testConfiguredProgressRecovery(true);
    std::cout << "Local control ABI: provider=" << sizeof(blinker::HttpDeviceKeySessionProvider)
              << ", wrapper1=" << sizeof(blinker::LocalAccessCloudTransport<1U>) << std::endl;
    std::cout << "DeviceKey session and MQTT E2E tests passed" << std::endl;
    return 0;
}
