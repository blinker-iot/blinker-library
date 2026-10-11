#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <type_traits>

#include "BlinkerWiFi.h"
#include "BlinkerV2Advanced.h"
#include "ArduinoClientHttp.h"
#include "PubSubMqtt.h"

void runConnectivityLifecycleTests();

namespace {

int failures = 0;

#define CHECK_TRUE(expression)                                                   \
    do {                                                                         \
        if (!(expression)) {                                                     \
            std::cerr << __FILE__ << ":" << __LINE__                            \
                      << " check failed: " #expression << std::endl;              \
            ++failures;                                                          \
        }                                                                        \
    } while (false)

#define CHECK_EQ(expected, actual)                                                \
    do {                                                                         \
        if (!((expected) == (actual))) {                                          \
            std::cerr << __FILE__ << ":" << __LINE__                            \
                      << " check failed: " #expected " == " #actual << std::endl; \
            ++failures;                                                          \
        }                                                                        \
    } while (false)

class UnsupportedAead final
    : public blinker::IX25519AesGcmCryptoProvider {
public:
    blinker::Result x25519PublicKey(
        blinker::ByteView,
        blinker::MutableByteSpan) override {
        return unsupported();
    }
    blinker::Result x25519(
        blinker::ByteView,
        blinker::ByteView,
        blinker::MutableByteSpan) override {
        return unsupported();
    }
    blinker::Result aes256GcmEncrypt(
        blinker::ByteView,
        blinker::ByteView,
        blinker::ByteView,
        blinker::ByteView,
        blinker::MutableByteSpan,
        size_t& written) override {
        written = 0U;
        return unsupported();
    }
    blinker::Result aes256GcmDecrypt(
        blinker::ByteView,
        blinker::ByteView,
        blinker::ByteView,
        blinker::ByteView,
        blinker::MutableByteSpan,
        size_t& written) override {
        written = 0U;
        return unsupported();
    }

private:
    static blinker::Result unsupported() {
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }
};

struct DiagnosticCapture {
    blinker::DiagnosticEvent events[8];
    size_t count;
    uint32_t now;

    DiagnosticCapture() : events(), count(0U), now(0U) {}
};

uint32_t diagnosticClock(void* context) {
    return static_cast<DiagnosticCapture*>(context)->now;
}

bool captureDiagnostic(
    void* context,
    const blinker::DiagnosticEvent& event) {
    DiagnosticCapture* capture =
        static_cast<DiagnosticCapture*>(context);
    if (capture->count < sizeof(capture->events) / sizeof(capture->events[0])) {
        capture->events[capture->count++] = event;
    }
    return true;
}

void testDiagnostics() {
    blinker::Diagnostics diagnostics;

    // Counters are useful even when text output is disabled.
    diagnostics.recordState(true, blinker::Result::success(), 1U);
    CHECK_EQ(static_cast<uint32_t>(1U), diagnostics.counters().stateChanges);

    DiagnosticCapture capture;
    diagnostics.configure(
        &captureDiagnostic,
        &capture,
        blinker::LogLevel::Debug,
        &diagnosticClock,
        &capture);
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(1U), capture.count);
    CHECK_EQ(
        blinker::DiagnosticCode::DiagnosticsEnabled,
        static_cast<blinker::DiagnosticCode>(capture.events[0].code));

    const blinker::Result blocked =
        blinker::Result::failure(blinker::ErrorCode::WouldBlock);
    diagnostics.recordOperation(
        blinker::DiagnosticOperation::StateReport, blocked);
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(2U), capture.count);
    CHECK_EQ(static_cast<uint32_t>(1U), diagnostics.counters().wouldBlock);

    // Repeated output is summarized without allocating a log ring buffer.
    diagnostics.recordOperation(
        blinker::DiagnosticOperation::StateReport, blocked);
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(2U), capture.count);
    capture.now = 1000U;
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(3U), capture.count);
    CHECK_EQ(
        blinker::DiagnosticCode::OutputSuppressed,
        static_cast<blinker::DiagnosticCode>(capture.events[2].code));
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        diagnostics.counters().outputSuppressed);

    diagnostics.recordState(false, blinker::Result::success(), 1U);
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(3U), capture.count);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        diagnostics.counters().stateUnchanged);

    diagnostics.recordState(true, blinker::Result::success(), 2U);
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(4U), capture.count);
    CHECK_EQ(
        blinker::DiagnosticCode::StateChanged,
        static_cast<blinker::DiagnosticCode>(capture.events[3].code));
    CHECK_EQ(static_cast<uint32_t>(2U), diagnostics.counters().stateChanges);

    diagnostics.recordState(true, blinker::Result::success(), 3U);
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(4U), capture.count);
    capture.now = 1100U;
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(5U), capture.count);

    // Throttling is measured from the actual flush, not the time at which a
    // delayed event was queued.
    diagnostics.recordState(true, blinker::Result::success(), 4U);
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(5U), capture.count);
    capture.now = 1200U;
    diagnostics.flush();
    CHECK_EQ(static_cast<size_t>(6U), capture.count);
}

class FakeTransport : public blinker::IFrameTransport {
public:
    FakeTransport()
        : receiver(nullptr),
          receiverContext(nullptr),
          sessionConnected(nullptr),
          sessionDisconnected(nullptr),
          sessionContext(nullptr),
          currentState(blinker::TransportState::Stopped),
          startResult(blinker::Result::success()),
          sendResult(blinker::Result::success()),
          pollCount(0),
          lastBudget(0),
          sendCount(0),
          stopCount(0),
          lastSentSize(0),
          featureFlags(blinker::TransportFeatureBinary),
          pollMarker(0),
          pollLog(nullptr),
          pollLogSize(nullptr) {}

    blinker::Result start() override {
        if (startResult) {
            currentState = blinker::TransportState::Online;
        }
        return startResult;
    }

    void stop() override {
        ++stopCount;
        currentState = blinker::TransportState::Stopped;
    }

    void poll(uint32_t budgetMicros) override {
        ++pollCount;
        lastBudget = budgetMicros;
        if (pollLog != nullptr && pollLogSize != nullptr) {
            pollLog[(*pollLogSize)++] = pollMarker;
        }
    }

    blinker::TransportState state() const override {
        return currentState;
    }

    blinker::TransportCapabilities capabilities() const override {
        blinker::TransportCapabilities value;
        value.maxFrameSize = 1024;
        value.features = featureFlags;
        return value;
    }

    blinker::Result send(
        blinker::ByteView frame,
        const blinker::SendTarget& target) override {
        if (frame.empty()) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        ++sendCount;
        lastTarget = target;
        lastSentSize = frame.size < sizeof(lastSent)
                           ? frame.size
                           : sizeof(lastSent);
        std::memcpy(lastSent, frame.data, lastSentSize);
        return sendResult;
    }

    void setReceiver(
        blinker::FrameReceiver nextReceiver,
        void* context) override {
        receiver = nextReceiver;
        receiverContext = context;
    }

    void setSessionHandlers(
        blinker::FrameSessionHandler connected,
        blinker::FrameSessionHandler disconnected,
        void* context) override {
        sessionConnected = connected;
        sessionDisconnected = disconnected;
        sessionContext = context;
    }

    void inject(
        blinker::ByteView frame,
        uint8_t id,
        bool authenticated = true,
        uint32_t sessionId = 7,
        bool encrypted = false,
        bool bonded = false) {
        if (receiver == nullptr) {
            return;
        }
        blinker::RxContext rx;
        rx.transportId = id;
        rx.sessionId = sessionId;
        rx.encrypted = encrypted;
        rx.bonded = bonded;
        rx.authenticated = authenticated;
        receiver(receiverContext, frame, rx);
    }

    void disconnect(uint32_t sessionId) {
        if (sessionDisconnected == nullptr) return;
        blinker::RxContext rx;
        rx.sessionId = sessionId;
        sessionDisconnected(sessionContext, rx);
    }

    blinker::FrameReceiver receiver;
    void* receiverContext;
    blinker::FrameSessionHandler sessionConnected;
    blinker::FrameSessionHandler sessionDisconnected;
    void* sessionContext;
    blinker::TransportState currentState;
    blinker::Result startResult;
    blinker::Result sendResult;
    uint32_t pollCount;
    uint32_t lastBudget;
    uint32_t sendCount;
    uint32_t stopCount;
    uint8_t lastSent[512];
    size_t lastSentSize;
    blinker::SendTarget lastTarget;
    uint16_t featureFlags;
    uint8_t pollMarker;
    uint8_t* pollLog;
    size_t* pollLogSize;
};

class ProductTestClock final : public blinker::IClock {
public:
    uint32_t monotonicMillis() const override { return 0U; }
    bool unixTime(uint64_t&) const override { return false; }
};

class FakeProductLifecycle : public blinker::IProductLifecycle {
public:
    explicit FakeProductLifecycle(
        FakeTransport& transport,
        bool attachClock = false,
        bool accessReset = false,
        bool networkReset = false)
        : transport_(transport),
          client_(nullptr),
          attachClock_(attachClock),
          accessReset_(accessReset),
          networkReset_(networkReset),
          startResult_(blinker::Result::success()),
          resetResult_(blinker::Result::success()),
          networkResetResult_(blinker::Result::success()),
          state_(blinker::ProductLifecycleState::Stopped),
          lastError_(blinker::ErrorCode::Ok),
          attachCalls(0U),
          startCalls(0U),
          pollCalls(0U),
          stopCalls(0U),
          resetCalls(0U),
          networkResetCalls(0U),
          lastBudget(0U) {}

    blinker::Result attach(blinker::Client& client) override {
        ++attachCalls;
        if (client_ != nullptr) {
            return client_ == &client
                       ? blinker::Result::success()
                       : blinker::Result::failure(
                             blinker::ErrorCode::AlreadyExists);
        }
        blinker::Result result = client.addTransport(transport_);
        if (result && attachClock_) {
            result = client.setMonotonicClock(&clock_);
        }
        if (result) client_ = &client;
        return result;
    }

    blinker::Result start() override {
        ++startCalls;
        state_ = blinker::ProductLifecycleState::Starting;
        if (!startResult_) {
            lastError_ = startResult_.code();
            return startResult_;
        }
        if (client_ == nullptr) {
            lastError_ = blinker::ErrorCode::NotConfigured;
            return blinker::Result::failure(lastError_);
        }
        const blinker::Result result = client_->begin();
        if (!result) {
            lastError_ = result.code();
            return result;
        }
        state_ = blinker::ProductLifecycleState::Active;
        lastError_ = blinker::ErrorCode::Ok;
        return blinker::Result::success();
    }

    void poll(uint32_t totalBudgetMicros) override {
        ++pollCalls;
        lastBudget = totalBudgetMicros;
        if (client_ != nullptr) client_->poll(totalBudgetMicros);
    }

    void stop() override {
        ++stopCalls;
        if (client_ != nullptr) client_->end();
        state_ = blinker::ProductLifecycleState::Stopped;
    }

    blinker::Result resetAccess() override {
        ++resetCalls;
        return accessReset_
                   ? resetResult_
                   : blinker::Result::failure(
                         blinker::ErrorCode::UnsupportedFeature);
    }

    blinker::Result resetNetwork() override {
        ++networkResetCalls;
        return networkReset_
                   ? networkResetResult_
                   : blinker::Result::failure(
                         blinker::ErrorCode::UnsupportedFeature);
    }

    blinker::ProductLifecycleStatus status() const override {
        return blinker::ProductLifecycleStatus(
            state_,
            lastError_,
            state_ == blinker::ProductLifecycleState::Active);
    }

    blinker::ProductCapabilities capabilities() const override {
        uint16_t flags = static_cast<uint16_t>(
            blinker::ProductCapabilityCloudData |
            blinker::ProductCapabilityDirectBleData);
        if (accessReset_) {
            flags = static_cast<uint16_t>(
                flags | blinker::ProductCapabilityAccessReset);
        }
        if (networkReset_) {
            flags = static_cast<uint16_t>(
                flags | blinker::ProductCapabilityNetworkReset);
        }
        return blinker::ProductCapabilities(flags);
    }

    FakeTransport& transport_;
    blinker::Client* client_;
    bool attachClock_;
    bool accessReset_;
    bool networkReset_;
    ProductTestClock clock_;
    blinker::Result startResult_;
    blinker::Result resetResult_;
    blinker::Result networkResetResult_;
    blinker::ProductLifecycleState state_;
    blinker::ErrorCode lastError_;
    uint32_t attachCalls;
    uint32_t startCalls;
    uint32_t pollCalls;
    uint32_t stopCalls;
    uint32_t resetCalls;
    uint32_t networkResetCalls;
    uint32_t lastBudget;
};

FakeTransport facadeTransport;
FakeProductLifecycle facadeLifecycle(facadeTransport);

} // namespace

namespace blinker {
namespace integration {

IProductLifecycle& lifecycle(const WifiDeviceKeyProfile&) {
    return facadeLifecycle;
}

} // namespace integration
} // namespace blinker

namespace {

class FakeClock : public blinker::IClock {
public:
    FakeClock() : now(0) {}

    uint32_t monotonicMillis() const override {
        return now;
    }

    bool unixTime(uint64_t&) const override {
        return false;
    }

    void advance(uint32_t millis) {
        now += millis;
    }

    uint32_t now;
};

class SequenceRandom : public blinker::IRandom {
public:
    explicit SequenceRandom(uint8_t first = 0)
        : next(first), calls(0), fillResult(blinker::Result::success()) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        ++calls;
        if (!fillResult) return fillResult;
        if (output.data == nullptr && output.size != 0U) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        for (size_t index = 0; index < output.size; ++index) {
            output.data[index] = next++;
        }
        return blinker::Result::success();
    }

    uint8_t next;
    uint32_t calls;
    blinker::Result fillResult;
};

class FakeAtomicBlobStore : public blinker::IAtomicBlobStore {
public:
    enum : size_t { capacity = 192U };

    FakeAtomicBlobStore()
        : data(),
          size(0U),
          present(false),
          loadResult(blinker::Result::success()),
          replaceResult(blinker::Result::success()),
          clearResult(blinker::Result::success()),
          loadCalls(0U),
          replaceCalls(0U),
          clearCalls(0U) {}

    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        ++loadCalls;
        written = 0U;
        if (!loadResult) return loadResult;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (output.data == nullptr || output.size < size) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memcpy(output.data, data, size);
        written = size;
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView value) override {
        ++replaceCalls;
        if (!replaceResult) return replaceResult;
        if (value.data == nullptr || value.size > sizeof(data)) {
            return blinker::Result::failure(
                blinker::ErrorCode::CapacityExceeded);
        }
        std::memcpy(data, value.data, value.size);
        size = value.size;
        present = true;
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        ++clearCalls;
        if (!clearResult) return clearResult;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        std::memset(data, 0, sizeof(data));
        size = 0U;
        present = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }

    uint8_t data[capacity];
    size_t size;
    bool present;
    blinker::Result loadResult;
    blinker::Result replaceResult;
    blinker::Result clearResult;
    uint32_t loadCalls;
    uint32_t replaceCalls;
    uint32_t clearCalls;
};

class FakeBleLink : public blinker::IBleLink {
public:
    struct SentPacket {
        uint32_t sessionId;
        uint8_t data[244];
        size_t size;

        SentPacket() : sessionId(0), data(), size(0) {}
    };

    FakeBleLink()
        : currentState(blinker::BleLinkState::Stopped),
          currentError(blinker::ErrorCode::Ok),
          packetReceiver(nullptr),
          packetContext(nullptr),
          connectedHandler(nullptr),
          disconnectedHandler(nullptr),
          sessionContext(nullptr),
          sessions(),
          sessionsUsed(0),
          sent(),
          sentCount(0),
          blockSends(false),
          pollCount(0) {}

    blinker::Result start() override {
        currentState = blinker::BleLinkState::Ready;
        return blinker::Result::success();
    }

    void stop() override {
        currentState = blinker::BleLinkState::Stopped;
    }

    void poll(uint32_t) override { ++pollCount; }
    blinker::BleLinkState state() const override { return currentState; }
    blinker::ErrorCode lastError() const override { return currentError; }
    size_t sessionCount() const override { return sessionsUsed; }

    blinker::Result sessionAt(
        size_t index,
        blinker::BleSessionInfo& session) const override {
        if (index >= sessionsUsed) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        session = sessions[index];
        return blinker::Result::success();
    }

    blinker::Result sendPacket(
        uint32_t sessionId,
        blinker::ByteView packet) override {
        if (blockSends) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        if (sentCount >= sizeof(sent) / sizeof(sent[0]) ||
            packet.size > sizeof(sent[0].data)) {
            return blinker::Result::failure(
                blinker::ErrorCode::CapacityExceeded);
        }
        SentPacket& output = sent[sentCount++];
        output.sessionId = sessionId;
        output.size = packet.size;
        std::memcpy(output.data, packet.data, packet.size);
        return blinker::Result::success();
    }

    void setPacketReceiver(
        blinker::BlePacketReceiver receiver,
        void* context) override {
        packetReceiver = receiver;
        packetContext = context;
    }

    void setSessionHandlers(
        blinker::BleSessionHandler connected,
        blinker::BleSessionHandler disconnected,
        void* context) override {
        connectedHandler = connected;
        disconnectedHandler = disconnected;
        sessionContext = context;
    }

    void addSession(
        uint32_t sessionId,
        uint16_t maxPacketSize,
        bool authenticated = true,
        bool notifyEnabled = true,
        bool encrypted = false,
        bool bonded = false) {
        CHECK_TRUE(sessionsUsed < sizeof(sessions) / sizeof(sessions[0]));
        if (sessionsUsed >= sizeof(sessions) / sizeof(sessions[0])) return;
        blinker::BleSessionInfo& session = sessions[sessionsUsed++];
        session.sessionId = sessionId;
        session.maxPacketSize = maxPacketSize;
        session.connected = true;
        session.notifyEnabled = notifyEnabled;
        session.encrypted = encrypted;
        session.bonded = bonded;
        session.authenticated = authenticated;
        if (connectedHandler != nullptr) {
            connectedHandler(sessionContext, session);
        }
    }

    void announceSession(uint32_t sessionId) {
        for (size_t index = 0; index < sessionsUsed; ++index) {
            if (sessions[index].sessionId == sessionId) {
                if (connectedHandler != nullptr) {
                    connectedHandler(sessionContext, sessions[index]);
                }
                return;
            }
        }
        CHECK_TRUE(false);
    }

    void setSecurity(
        uint32_t sessionId,
        bool encrypted,
        bool bonded,
        bool authenticated) {
        for (size_t index = 0; index < sessionsUsed; ++index) {
            if (sessions[index].sessionId == sessionId) {
                sessions[index].encrypted = encrypted;
                sessions[index].bonded = bonded;
                sessions[index].authenticated = authenticated;
                return;
            }
        }
        CHECK_TRUE(false);
    }

    void disconnect(uint32_t sessionId) {
        for (size_t index = 0; index < sessionsUsed; ++index) {
            if (sessions[index].sessionId != sessionId) continue;
            const blinker::BleSessionInfo disconnected = sessions[index];
            for (size_t move = index + 1; move < sessionsUsed; ++move) {
                sessions[move - 1U] = sessions[move];
            }
            --sessionsUsed;
            if (disconnectedHandler != nullptr) {
                disconnectedHandler(sessionContext, disconnected);
            }
            return;
        }
    }

    void setPacketSize(uint32_t sessionId, uint16_t maxPacketSize) {
        for (size_t index = 0; index < sessionsUsed; ++index) {
            if (sessions[index].sessionId == sessionId) {
                sessions[index].maxPacketSize = maxPacketSize;
                return;
            }
        }
        CHECK_TRUE(false);
    }

    void inject(size_t sentIndex) {
        CHECK_TRUE(sentIndex < sentCount);
        if (sentIndex >= sentCount || packetReceiver == nullptr) return;
        const SentPacket& input = sent[sentIndex];
        for (size_t index = 0; index < sessionsUsed; ++index) {
            if (sessions[index].sessionId == input.sessionId) {
                packetReceiver(
                    packetContext,
                    sessions[index],
                    blinker::ByteView(input.data, input.size));
                return;
            }
        }
        CHECK_TRUE(false);
    }

    void clearSent() { sentCount = 0; }

    blinker::BleLinkState currentState;
    blinker::ErrorCode currentError;
    blinker::BlePacketReceiver packetReceiver;
    void* packetContext;
    blinker::BleSessionHandler connectedHandler;
    blinker::BleSessionHandler disconnectedHandler;
    void* sessionContext;
    blinker::BleSessionInfo sessions[6];
    size_t sessionsUsed;
    SentPacket sent[64];
    size_t sentCount;
    bool blockSends;
    uint32_t pollCount;
};

class FakeHttpClient : public blinker::IHttpClient {
public:
    FakeHttpClient()
        : startResult(blinker::Result::success()),
          currentState(blinker::HttpClientState::Idle),
          currentError(blinker::ErrorCode::Ok),
          startCount(0),
          pollCount(0),
          stopCount(0),
          responseStatus(0),
          responseSize(0) {}

    blinker::Result start(const blinker::HttpRequest& request) override {
        ++startCount;
        lastRequest = request;
        if (!startResult) {
            currentState = blinker::HttpClientState::Failed;
            currentError = startResult.code();
            return startResult;
        }
        currentState = blinker::HttpClientState::InProgress;
        currentError = blinker::ErrorCode::Ok;
        return blinker::Result::success();
    }

    void poll(uint32_t) override { ++pollCount; }
    blinker::HttpClientState state() const override { return currentState; }

    blinker::Result response(blinker::HttpResponse& response) const override {
        if (currentState != blinker::HttpClientState::Complete) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        response.statusCode = responseStatus;
        response.body = blinker::ByteView(
            lastRequest.responseBuffer.data,
            responseSize);
        return blinker::Result::success();
    }

    blinker::ErrorCode lastError() const override { return currentError; }

    void stop() override {
        ++stopCount;
        currentState = blinker::HttpClientState::Idle;
    }

    void complete(uint16_t status, const char* json) {
        const size_t size = std::strlen(json);
        CHECK_TRUE(size <= lastRequest.responseBuffer.size);
        if (size <= lastRequest.responseBuffer.size) {
            std::memcpy(lastRequest.responseBuffer.data, json, size);
            responseSize = size;
            responseStatus = status;
            currentState = blinker::HttpClientState::Complete;
        }
    }

    void fail(blinker::ErrorCode error) {
        currentError = error;
        currentState = blinker::HttpClientState::Failed;
    }

    blinker::Result startResult;
    blinker::HttpClientState currentState;
    blinker::ErrorCode currentError;
    uint32_t startCount;
    uint32_t pollCount;
    uint32_t stopCount;
    uint16_t responseStatus;
    size_t responseSize;
    blinker::HttpRequest lastRequest;
};

class FakeArduinoClient {
public:
    FakeArduinoClient()
        : connectResult(true),
          connectedState(false),
          inbound(nullptr),
          inboundSize(0),
          inboundOffset(0),
          written(),
          writtenSize(0),
          connectCount(0),
          stopCount(0),
          lastPort(0) {
        lastHost[0] = '\0';
    }

    bool connect(const char* host, uint16_t port) {
        ++connectCount;
        const size_t hostSize = std::strlen(host);
        const size_t copied = hostSize < sizeof(lastHost) - 1U
                                  ? hostSize
                                  : sizeof(lastHost) - 1U;
        std::memcpy(lastHost, host, copied);
        lastHost[copied] = '\0';
        lastPort = port;
        connectedState = connectResult;
        return connectResult;
    }

    size_t write(const uint8_t* data, size_t size) {
        if (!connectedState || size > sizeof(written) - writtenSize) return 0;
        std::memcpy(written + writtenSize, data, size);
        writtenSize += size;
        return size;
    }

    int available() const {
        return static_cast<int>(inboundSize - inboundOffset);
    }

    int read() {
        if (inboundOffset >= inboundSize) return -1;
        return inbound[inboundOffset++];
    }

    bool connected() const { return connectedState; }

    void stop() {
        ++stopCount;
        connectedState = false;
    }

    void setInbound(const char* data) {
        inbound = reinterpret_cast<const uint8_t*>(data);
        inboundSize = std::strlen(data);
        inboundOffset = 0;
    }

    bool connectResult;
    bool connectedState;
    const uint8_t* inbound;
    size_t inboundSize;
    size_t inboundOffset;
    uint8_t written[1024];
    size_t writtenSize;
    uint32_t connectCount;
    uint32_t stopCount;
    char lastHost[128];
    uint16_t lastPort;
};

class FakeMqttClient : public blinker::IMqttClient {
public:
    FakeMqttClient()
        : capacityResult(blinker::Result::success()),
          connectResult(blinker::Result::success()),
          subscribeResult(blinker::Result::success()),
          publishResult(blinker::Result::success()),
          handler(nullptr),
          handlerContext(nullptr),
          connectedState(false),
          capacityCheckCount(0),
          connectCount(0),
          disconnectCount(0),
          subscribeCount(0),
          publishCount(0),
          pollCount(0),
          lastPublishedSize(0),
          lastPublishRetained(false),
          lastPublishQos(0),
          lastSubscribeQos(0) {}

    blinker::Result validatePacketCapacity(
        blinker::StringView,
        size_t,
        uint8_t,
        blinker::MqttPacketDirection) const override {
        ++capacityCheckCount;
        return capacityResult;
    }

    blinker::Result connect(
        const blinker::MqttConnectOptions& options) override {
        ++connectCount;
        lastConnectOptions = options;
        connectedState = connectResult.ok();
        return connectResult;
    }

    void disconnect() override {
        ++disconnectCount;
        connectedState = false;
    }

    bool connected() const override {
        return connectedState;
    }

    blinker::Result subscribe(
        blinker::StringView topic,
        uint8_t qos) override {
        ++subscribeCount;
        lastSubscribeTopic = topic;
        lastSubscribeQos = qos;
        return subscribeResult;
    }

    blinker::Result publish(
        blinker::StringView topic,
        blinker::ByteView payload,
        const blinker::MqttPublishOptions& options) override {
        ++publishCount;
        lastPublishTopic = topic;
        lastPublishedSize = payload.size;
        lastPublishRetained = options.retained;
        lastPublishQos = options.qos;
        return publishResult;
    }

    void poll(uint32_t) override {
        ++pollCount;
    }

    void setMessageHandler(
        blinker::MqttMessageHandler nextHandler,
        void* context) override {
        handler = nextHandler;
        handlerContext = context;
    }

    void inject(
        blinker::StringView topic,
        blinker::ByteView payload,
        bool retained = false) {
        if (handler != nullptr) {
            blinker::MqttMessageInfo info;
            info.qos = 0;
            info.qosKnown = true;
            info.retained = retained;
            info.retainedKnown = true;
            handler(
                handlerContext,
                topic,
                payload,
                info);
        }
    }

    blinker::Result capacityResult;
    blinker::Result connectResult;
    blinker::Result subscribeResult;
    blinker::Result publishResult;
    blinker::MqttMessageHandler handler;
    void* handlerContext;
    bool connectedState;
    mutable uint32_t capacityCheckCount;
    uint32_t connectCount;
    uint32_t disconnectCount;
    uint32_t subscribeCount;
    uint32_t publishCount;
    uint32_t pollCount;
    size_t lastPublishedSize;
    bool lastPublishRetained;
    uint8_t lastPublishQos;
    uint8_t lastSubscribeQos;
    blinker::MqttConnectOptions lastConnectOptions;
    blinker::StringView lastSubscribeTopic;
    blinker::StringView lastPublishTopic;
};

class ProductSessionProvider : public blinker::ISessionProvider {
public:
    ProductSessionProvider()
        : currentState(blinker::SessionProviderState::Stopped),
          currentError(blinker::ErrorCode::Ok),
          generation(1U),
          network(false),
          started(false),
          pollCalls(0U) {}

    blinker::Result start() override {
        started = true;
        currentState = network ? blinker::SessionProviderState::Ready
                               : blinker::SessionProviderState::Idle;
        return blinker::Result::success();
    }
    blinker::Result requestRefresh() override {
        ++generation;
        currentState = network ? blinker::SessionProviderState::Ready
                               : blinker::SessionProviderState::Idle;
        return blinker::Result::success();
    }
    void setNetworkAvailable(bool available) override {
        network = available;
        if (started) {
            currentState = network ? blinker::SessionProviderState::Ready
                                   : blinker::SessionProviderState::Idle;
        }
    }
    void poll(uint32_t) override { ++pollCalls; }
    blinker::SessionProviderState state() const override {
        return currentState;
    }
    blinker::ErrorCode lastError() const override { return currentError; }
    uint32_t credentialGeneration() const override { return generation; }
    blinker::Result credentials(
        blinker::SessionCredentials& output) const override {
        if (currentState != blinker::SessionProviderState::Ready) {
            return blinker::Result::failure(
                blinker::ErrorCode::NotConnected);
        }
        output = blinker::SessionCredentials();
        output.broker = blinker::BrokerKind::Blinker;
        output.host = blinker::StringView("broker.example", 14U);
        output.port = 8883U;
        output.security = blinker::MqttSecurity::Tls;
        output.deviceName = blinker::StringView("device-product", 14U);
        output.clientId = output.deviceName;
        output.username = blinker::StringView("mqtt-user", 9U);
        output.password = blinker::StringView("mqtt-password", 13U);
        output.publishTopic = blinker::StringView("/device/product/s", 17U);
        output.subscribeTopic = blinker::StringView("/device/product/r", 17U);
        return blinker::Result::success();
    }
    void stop() override {
        started = false;
        currentState = blinker::SessionProviderState::Stopped;
    }

    blinker::SessionProviderState currentState;
    blinker::ErrorCode currentError;
    uint32_t generation;
    bool network;
    bool started;
    uint32_t pollCalls;
};

struct ReceiveCapture {
    uint32_t calls;
    uint8_t transportId;
    uint32_t sessionId;
    size_t frameSize;
    bool authenticated;
    uint8_t frame[512];

    ReceiveCapture()
        : calls(0),
          transportId(0),
          sessionId(0),
          frameSize(0),
          authenticated(false),
          frame() {}
};

struct BleSessionCapture {
    uint32_t connected;
    uint32_t disconnected;
    uint32_t lastSessionId;

    BleSessionCapture()
        : connected(0U), disconnected(0U), lastSessionId(0U) {}
};

void captureBleConnected(void* context, const blinker::RxContext& rx) {
    BleSessionCapture* capture =
        static_cast<BleSessionCapture*>(context);
    ++capture->connected;
    capture->lastSessionId = rx.sessionId;
}

void captureBleDisconnected(void* context, const blinker::RxContext& rx) {
    BleSessionCapture* capture =
        static_cast<BleSessionCapture*>(context);
    ++capture->disconnected;
    capture->lastSessionId = rx.sessionId;
}

void captureFrame(
    void* context,
    blinker::ByteView frame,
    const blinker::RxContext& rx) {
    ReceiveCapture* capture = static_cast<ReceiveCapture*>(context);
    ++capture->calls;
    capture->transportId = rx.transportId;
    capture->sessionId = rx.sessionId;
    capture->frameSize = frame.size;
    capture->authenticated = rx.authenticated;
    if (frame.size <= sizeof(capture->frame)) {
        std::memcpy(capture->frame, frame.data, frame.size);
    }
}

struct MqttCapture {
    uint32_t calls;
    size_t payloadSize;
    bool retainKnown;

    MqttCapture() : calls(0), payloadSize(0), retainKnown(true) {}
};

void captureMqtt(
    void* context,
    blinker::StringView,
    blinker::ByteView payload,
    const blinker::MqttMessageInfo& info) {
    MqttCapture* capture = static_cast<MqttCapture*>(context);
    ++capture->calls;
    capture->payloadSize = payload.size;
    capture->retainKnown = info.retainedKnown;
}

void testArduinoClientHttpAdapter() {
    FakeClock clock;
    FakeArduinoClient client;
    client.setInbound(
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
        "Content-Length: 2\r\nConnection: close\r\n\r\n{}");
    blinker::ArduinoClientHttpAdapter<FakeArduinoClient> adapter(
        client,
        blinker::HttpSecurity::Tls,
        clock);
    uint8_t responseBuffer[32] = {};
    blinker::HttpRequest request;
    request.host = blinker::StringView("iot.example.com", 15);
    request.port = 443;
    request.security = blinker::HttpSecurity::Tls;
    request.target = blinker::StringView("/login?authKey=secret", 21);
    blinker::HttpHeader headers[2] = {
        blinker::HttpHeader(
            blinker::StringView("Accept"),
            blinker::StringView("application/cbor")),
        blinker::HttpHeader(
            blinker::StringView("X-Blinker-Contract"),
            blinker::StringView("enrollment/1"))};
    request.headers = headers;
    request.headerCount = 2U;
    request.responseBuffer = blinker::MutableByteSpan(
        responseBuffer,
        sizeof(responseBuffer));
    CHECK_TRUE(adapter.start(request).ok());
    adapter.poll(4096);
    CHECK_TRUE(adapter.state() == blinker::HttpClientState::Complete);
    CHECK_EQ(static_cast<uint32_t>(1), client.connectCount);
    CHECK_EQ(static_cast<uint16_t>(443), client.lastPort);
    CHECK_TRUE(std::strcmp(client.lastHost, "iot.example.com") == 0);
    CHECK_TRUE(client.writtenSize > request.target.size);
    CHECK_TRUE(std::strstr(
        reinterpret_cast<const char*>(client.written),
        "Accept: application/cbor\r\n") != nullptr);
    CHECK_TRUE(std::strstr(
        reinterpret_cast<const char*>(client.written),
        "X-Blinker-Contract: enrollment/1\r\n") != nullptr);
    blinker::HttpResponse response;
    CHECK_TRUE(adapter.response(response).ok());
    CHECK_EQ(static_cast<uint16_t>(200), response.statusCode);
    CHECK_EQ(static_cast<size_t>(2), response.body.size);
    CHECK_TRUE(std::memcmp(response.body.data, "{}", 2) == 0);
    adapter.stop();

    blinker::HttpHeader forbidden(
        blinker::StringView("Content-Length"),
        blinker::StringView("9"));
    request.headers = &forbidden;
    request.headerCount = 1U;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        adapter.start(request).code());
    request.headers = headers;
    request.headerCount = 2U;

    request.security = blinker::HttpSecurity::PlainTcp;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        adapter.start(request).code());

    FakeArduinoClient chunkedClient;
    chunkedClient.setInbound(
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
        "2\r\n{}\r\n0\r\n\r\n");
    blinker::ArduinoClientHttpAdapter<FakeArduinoClient> chunked(
        chunkedClient,
        blinker::HttpSecurity::Tls,
        clock);
    request.security = blinker::HttpSecurity::Tls;
    CHECK_TRUE(chunked.start(request).ok());
    chunked.poll(4096);
    CHECK_TRUE(chunked.state() == blinker::HttpClientState::Failed);
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        chunked.lastError());
}

void testManagedMqttTransport() {
    FakeClock clock;
    ProductSessionProvider session;
    FakeMqttClient mqtt;
    blinker::MqttFrameTransport mqttTransport(mqtt, clock);
    blinker::ManagedMqttTransport managed(session, mqttTransport);
    BleSessionCapture sessions;
    managed.setSessionHandlers(
        &captureBleConnected,
        &captureBleDisconnected,
        &sessions);
    managed.setNetworkAvailable(false);
    CHECK_TRUE(managed.start().ok());
    CHECK_TRUE(managed.state() == blinker::TransportState::Starting);
    managed.poll(2000);
    CHECK_EQ(static_cast<uint32_t>(0), session.pollCalls);
    CHECK_EQ(static_cast<uint32_t>(0), mqtt.connectCount);

    managed.setNetworkAvailable(true);
    session.currentState = blinker::SessionProviderState::Backoff;
    session.currentError = blinker::ErrorCode::NotConnected;
    managed.poll(2000U);
    CHECK_TRUE(managed.state() == blinker::TransportState::Backoff);
    CHECK_EQ(blinker::ErrorCode::NotConnected, managed.lastError());
    CHECK_EQ(static_cast<uint32_t>(0), mqtt.connectCount);
    session.currentState = blinker::SessionProviderState::Ready;
    session.currentError = blinker::ErrorCode::Ok;
    managed.poll(2000);
    CHECK_TRUE(managed.state() == blinker::TransportState::Online);
    CHECK_EQ(blinker::ErrorCode::Ok, managed.lastError());
    CHECK_EQ(static_cast<uint32_t>(1), mqtt.connectCount);
    CHECK_EQ(static_cast<uint32_t>(1), mqtt.subscribeCount);
    CHECK_EQ(static_cast<uint32_t>(1), sessions.connected);
    CHECK_EQ(static_cast<uint32_t>(0), sessions.disconnected);
    CHECK_TRUE(std::memcmp(
                   mqtt.lastConnectOptions.password.data,
                   "mqtt-password",
                   mqtt.lastConnectOptions.password.size) == 0);

    CHECK_TRUE(session.requestRefresh().ok());
    managed.poll(2000);
    CHECK_TRUE(managed.state() == blinker::TransportState::Online);
    CHECK_EQ(static_cast<uint32_t>(2), mqtt.connectCount);
    CHECK_EQ(static_cast<uint32_t>(2), sessions.connected);
    CHECK_EQ(static_cast<uint32_t>(1), sessions.disconnected);

    // A rejected credential retires MQTT itself; an outer WiFi lifecycle must
    // not need to stop the shared Client to make this path fail closed.
    session.currentState = blinker::SessionProviderState::Failed;
    session.currentError = blinker::ErrorCode::AuthenticationRequired;
    managed.poll(2000U);
    CHECK_TRUE(managed.state() == blinker::TransportState::Error);
    CHECK_EQ(blinker::ErrorCode::AuthenticationRequired, managed.lastError());
    CHECK_EQ(static_cast<uint32_t>(2), sessions.disconnected);
    const uint8_t payload[] = {0xA0U};
    const uint32_t publishes = mqtt.publishCount;
    CHECK_EQ(blinker::ErrorCode::NotConnected,
             managed.send(blinker::ByteView(payload, sizeof(payload)),
                          blinker::SendTarget()).code());
    managed.poll(2000U);
    CHECK_EQ(publishes, mqtt.publishCount);
    CHECK_EQ(static_cast<uint32_t>(2), mqtt.connectCount);
    CHECK_EQ(static_cast<uint32_t>(2), sessions.disconnected);

    managed.stop();
    CHECK_TRUE(managed.state() == blinker::TransportState::Stopped);
    CHECK_EQ(static_cast<uint32_t>(2), sessions.disconnected);

    // An ordinary socket loss reports its reason during retry, then clears it
    // after reconnect. It must not be reported as a rejected DeviceKey.
    ProductSessionProvider retrySession;
    FakeMqttClient retryMqtt;
    blinker::MqttFrameTransport retryTransport(retryMqtt, clock);
    blinker::ManagedMqttTransport retryManaged(retrySession, retryTransport);
    CHECK_TRUE(retryManaged.start());
    retryManaged.poll(2000U);
    CHECK_TRUE(retryManaged.state() == blinker::TransportState::Online);
    retryMqtt.connectedState = false;
    retryManaged.poll(2000U);
    CHECK_TRUE(retryManaged.state() == blinker::TransportState::Backoff);
    CHECK_EQ(blinker::ErrorCode::NotConnected, retryManaged.lastError());
    clock.now += 60000U;
    retryManaged.poll(2000U);
    CHECK_TRUE(retryManaged.state() == blinker::TransportState::Online);
    CHECK_EQ(blinker::ErrorCode::Ok, retryManaged.lastError());
    retryManaged.stop();
}

struct KeyedCapture {
    uint32_t calls;
    blinker::cbor::Type firstType;
    blinker::cbor::Type secondType;

    KeyedCapture()
        : calls(0),
          firstType(blinker::cbor::Type::Invalid),
          secondType(blinker::cbor::Type::Invalid) {}
};

struct IdCapture {
    uint32_t calls;
    uint16_t firstId;
    uint16_t secondId;
    blinker::cbor::Type firstType;

    IdCapture()
        : calls(0),
          firstId(0),
          secondId(0),
          firstType(blinker::cbor::Type::Invalid) {}
};

blinker::Result captureKeyedValue(
    void* context,
    blinker::StringView,
    blinker::ByteView,
    blinker::cbor::Type type) {
    KeyedCapture* capture = static_cast<KeyedCapture*>(context);
    if (capture->calls == 0U) {
        capture->firstType = type;
    } else if (capture->calls == 1U) {
        capture->secondType = type;
    }
    ++capture->calls;
    return blinker::Result::success();
}

blinker::Result captureIdValue(
    void* context,
    uint16_t id,
    blinker::ByteView,
    blinker::cbor::Type type) {
    IdCapture* capture = static_cast<IdCapture*>(context);
    if (capture->calls == 0U) {
        capture->firstId = id;
        capture->firstType = type;
    } else if (capture->calls == 1U) {
        capture->secondId = id;
    }
    ++capture->calls;
    return blinker::Result::success();
}

void testSha256() {
    const uint8_t expectedEmpty[] = {
        0xE3, 0xB0, 0xC4, 0x42, 0x98, 0xFC, 0x1C, 0x14,
        0x9A, 0xFB, 0xF4, 0xC8, 0x99, 0x6F, 0xB9, 0x24,
        0x27, 0xAE, 0x41, 0xE4, 0x64, 0x9B, 0x93, 0x4C,
        0xA4, 0x95, 0x99, 0x1B, 0x78, 0x52, 0xB8, 0x55
    };
    uint8_t digest[blinker::kSha256Size] = {};
    CHECK_TRUE(blinker::sha256(
                   blinker::ByteView(),
                   blinker::MutableByteSpan(digest, sizeof(digest)))
                   .ok());
    CHECK_TRUE(std::memcmp(
                   digest,
                   expectedEmpty,
                   sizeof(expectedEmpty)) == 0);

    const uint8_t expectedAbc[] = {
        0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
        0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
        0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
        0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
    };
    blinker::Sha256 streaming;
    CHECK_TRUE(streaming.update(blinker::ByteView(
                   reinterpret_cast<const uint8_t*>("a"), 1)).ok());
    CHECK_TRUE(streaming.update(blinker::ByteView(
                   reinterpret_cast<const uint8_t*>("bc"), 2)).ok());
    CHECK_TRUE(streaming.finish(
                   blinker::MutableByteSpan(digest, sizeof(digest)))
                   .ok());
    CHECK_TRUE(std::memcmp(digest, expectedAbc, sizeof(expectedAbc)) == 0);
    CHECK_EQ(
        blinker::ErrorCode::ProtocolError,
        streaming.update(blinker::ByteView()).code());
}

void testHmacSha256() {
    uint8_t hmacKey[20];
    std::memset(hmacKey, 0x0B, sizeof(hmacKey));
    const uint8_t expectedHmac[] = {
        0xB0, 0x34, 0x4C, 0x61, 0xD8, 0xDB, 0x38, 0x53,
        0x5C, 0xA8, 0xAF, 0xCE, 0xAF, 0x0B, 0xF1, 0x2B,
        0x88, 0x1D, 0xC2, 0x00, 0xC9, 0x83, 0x3D, 0xA7,
        0x26, 0xE9, 0x37, 0x6C, 0x2E, 0x32, 0xCF, 0xF7
    };
    uint8_t digest[blinker::kSha256Size] = {};
    const char message[] = "Hi There";
    CHECK_TRUE(blinker::hmacSha256(
                   blinker::ByteView(hmacKey, sizeof(hmacKey)),
                   blinker::ByteView(
                       reinterpret_cast<const uint8_t*>(message),
                       sizeof(message) - 1U),
                   blinker::MutableByteSpan(digest, sizeof(digest)))
                   .ok());
    CHECK_TRUE(std::memcmp(
                   digest,
                   expectedHmac,
                   sizeof(expectedHmac)) == 0);
    CHECK_TRUE(blinker::constantTimeEqual(
        blinker::ByteView(digest, sizeof(digest)),
        blinker::ByteView(expectedHmac, sizeof(expectedHmac))));
    digest[31] ^= 1U;
    CHECK_TRUE(!blinker::constantTimeEqual(
        blinker::ByteView(digest, sizeof(digest)),
        blinker::ByteView(expectedHmac, sizeof(expectedHmac))));
    CHECK_TRUE(!blinker::constantTimeEqual(
        blinker::ByteView(digest, sizeof(digest) - 1U),
        blinker::ByteView(expectedHmac, sizeof(expectedHmac))));
    blinker::secureZero(blinker::MutableByteSpan(digest, sizeof(digest)));
    for (size_t index = 0; index < sizeof(digest); ++index) {
        CHECK_EQ(static_cast<uint8_t>(0), digest[index]);
    }

}

void testRestrictedCbor() {
    uint8_t buffer[64] = {};
    blinker::cbor::Writer writer(
        blinker::MutableByteSpan(buffer, sizeof(buffer)));
    CHECK_TRUE(writer.beginMap(2).ok());
    CHECK_TRUE(writer.writeUnsigned(0).ok());
    CHECK_TRUE(writer.writeUnsigned(23).ok());
    CHECK_TRUE(writer.writeUnsigned(1).ok());
    CHECK_TRUE(writer.writeText(blinker::StringView("on", 2)).ok());

    const uint8_t expected[] = {0xA2, 0x00, 0x17, 0x01, 0x62, 0x6F, 0x6E};
    CHECK_EQ(sizeof(expected), writer.size());
    CHECK_TRUE(std::memcmp(expected, buffer, sizeof(expected)) == 0);

    blinker::cbor::Reader reader(writer.view());
    size_t count = 0;
    uint64_t key = 0;
    uint64_t number = 0;
    blinker::StringView text;
    CHECK_TRUE(reader.readMapSize(count).ok());
    CHECK_EQ(static_cast<size_t>(2), count);
    CHECK_TRUE(reader.readUnsigned(key).ok());
    CHECK_TRUE(reader.readUnsigned(number).ok());
    CHECK_EQ(static_cast<uint64_t>(23), number);
    CHECK_TRUE(reader.readUnsigned(key).ok());
    CHECK_TRUE(reader.readText(text).ok());
    CHECK_EQ(static_cast<size_t>(2), text.size);
    CHECK_TRUE(reader.finished());

    const uint8_t nonCanonical[] = {0x18, 0x17};
    blinker::cbor::Reader nonCanonicalReader(
        blinker::ByteView(nonCanonical, sizeof(nonCanonical)));
    CHECK_EQ(
        blinker::ErrorCode::NonCanonicalEncoding,
        nonCanonicalReader.readUnsigned(number).code());

    const uint8_t invalidUtf8[] = {0x62, 0xC0, 0xAF};
    blinker::cbor::Reader invalidUtf8Reader(
        blinker::ByteView(invalidUtf8, sizeof(invalidUtf8)));
    CHECK_EQ(
        blinker::ErrorCode::InvalidUtf8,
        invalidUtf8Reader.readText(text).code());

    const uint8_t indefinite[] = {0x9F, 0xFF};
    blinker::cbor::Reader indefiniteReader(
        blinker::ByteView(indefinite, sizeof(indefinite)));
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        indefiniteReader.skipValue().code());

    const uint8_t nested[] = {0x81, 0x81, 0x00};
    blinker::cbor::Limits shallow;
    shallow.maxDepth = 1;
    blinker::cbor::Reader nestedReader(
        blinker::ByteView(nested, sizeof(nested)), shallow);
    CHECK_EQ(
        blinker::ErrorCode::NestingTooDeep,
        nestedReader.skipValue().code());

    const uint8_t trailing[] = {0x00, 0x00};
    CHECK_EQ(
        blinker::ErrorCode::TrailingData,
        blinker::cbor::validate(
            blinker::ByteView(trailing, sizeof(trailing))).code());

    uint8_t floatBuffer[9] = {};
    blinker::cbor::Writer floatWriter(
        blinker::MutableByteSpan(floatBuffer, sizeof(floatBuffer)));
    CHECK_EQ(
        blinker::ErrorCode::ValueOutOfRange,
        floatWriter.writeFloat32(
            std::numeric_limits<float>::infinity()).code());
}

void testControlBodies() {
    blinker::bbp2::HelloBody hello;
    hello.role = blinker::bbp2::PeerRole::Device;
    hello.versions[0] = 2;
    hello.versionCount = 1;
    hello.features = blinker::bbp2::FeatureManifest;
    hello.maxFrameSize = 1024;
    hello.maxReassemblySize = 4096;
    hello.manifestRevision = 7;
    hello.hasManifestRevision = true;
    const uint8_t fingerprint[blinker::bbp2::kManifestFingerprintSize] = {
        0xAA, 0x55
    };
    hello.manifestFingerprint =
        blinker::ByteView(fingerprint, sizeof(fingerprint));
    hello.hasManifestFingerprint = true;

    uint8_t buffer[160] = {};
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   hello,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedHello[] = {
        0xA7,
        0x00, 0x00,
        0x01, 0x81, 0x02,
        0x02, 0x01,
        0x03, 0x19, 0x04, 0x00,
        0x04, 0x19, 0x10, 0x00,
        0x06, 0x07,
        0x07, 0x58, 0x20,
        0xAA, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    CHECK_EQ(sizeof(expectedHello), encoded.size);
    CHECK_TRUE(std::memcmp(expectedHello, encoded.data, encoded.size) == 0);

    blinker::bbp2::HelloBody decodedHello;
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(encoded, decodedHello).ok());
    CHECK_EQ(static_cast<uint8_t>(1), decodedHello.versionCount);
    CHECK_EQ(static_cast<uint8_t>(2), decodedHello.versions[0]);
    CHECK_EQ(static_cast<uint16_t>(1024), decodedHello.maxFrameSize);
    CHECK_TRUE(decodedHello.hasManifestFingerprint);
    CHECK_EQ(
        static_cast<size_t>(blinker::bbp2::kManifestFingerprintSize),
        decodedHello.manifestFingerprint.size);
    CHECK_EQ(static_cast<uint16_t>(0), decodedHello.maxBlePacketSize);

    // The optional bearer limit leaves legacy Hello bytes unchanged. It is
    // distinct from the frame limit and rejects zero/oversized wire claims.
    for (uint16_t packet = 20U; packet <= 244U; packet += 224U) {
        blinker::bbp2::HelloBody packetHello = hello;
        packetHello.maxBlePacketSize = packet;
        CHECK_TRUE(blinker::bbp2::encodeHelloBody(
            packetHello, blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded).ok());
        CHECK_TRUE(blinker::bbp2::decodeHelloBody(encoded, decodedHello).ok());
        CHECK_EQ(packet, decodedHello.maxBlePacketSize);
        CHECK_EQ(static_cast<uint16_t>(1024), decodedHello.maxFrameSize);
    }
    blinker::bbp2::HelloBody invalidPacketHello = hello;
    invalidPacketHello.maxBlePacketSize = 245U;
    CHECK_EQ(blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeHelloBody(invalidPacketHello,
            blinker::MutableByteSpan(buffer, sizeof(buffer)), encoded).code());
    invalidPacketHello.maxBlePacketSize = 19U;
    CHECK_EQ(blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeHelloBody(invalidPacketHello,
            blinker::MutableByteSpan(buffer, sizeof(buffer)), encoded).code());
    const uint8_t zeroPacketHello[] = {
        0xA6, 0x00, 0x00, 0x01, 0x81, 0x02, 0x02, 0x00,
        0x03, 0x18, 0x40, 0x04, 0x18, 0x40, 0x0C, 0x00
    };
    CHECK_EQ(blinker::ErrorCode::ValueOutOfRange,
        blinker::bbp2::decodeHelloBody(
            blinker::ByteView(zeroPacketHello, sizeof(zeroPacketHello)),
            decodedHello).code());

    blinker::bbp2::HelloBody shortFingerprintHello = hello;
    shortFingerprintHello.manifestFingerprint =
        blinker::ByteView(fingerprint, 2);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeHelloBody(
            shortFingerprintHello,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded)
            .code());

    blinker::bbp2::HelloBody invalidLimitsHello = hello;
    invalidLimitsHello.maxReassemblySize = 512;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeHelloBody(
            invalidLimitsHello,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded)
            .code());
    blinker::bbp2::HelloBody invalidIdFeatureHello = hello;
    invalidIdFeatureHello.features = blinker::bbp2::FeatureEndpointIds;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeHelloBody(
            invalidIdFeatureHello,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded)
            .code());

    blinker::bbp2::HelloBody authenticationHello = hello;
    authenticationHello.features |=
        blinker::bbp2::FeatureAuthentication;
    authenticationHello.authorizationMethods[0] =
        blinker::security::kControllerHmacSha256Method;
    authenticationHello.authorizationMethodCount = 1;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   authenticationHello,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(encoded, decodedHello).ok());
    CHECK_TRUE(
        (decodedHello.features &
         blinker::bbp2::FeatureAuthentication) != 0U);
    CHECK_EQ(
        static_cast<uint8_t>(1),
        decodedHello.authorizationMethodCount);
    CHECK_EQ(
        blinker::security::kControllerHmacSha256Method,
        decodedHello.authorizationMethods[0]);
    authenticationHello.authorizationMethodCount = 0;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeHelloBody(
            authenticationHello,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded)
            .code());

    blinker::bbp2::HelloBody reliableHello = hello;
    reliableHello.features |=
        blinker::bbp2::FeatureReliableDelivery;
    reliableHello.reliableReceiveWindow = 2;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   reliableHello,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(encoded, decodedHello).ok());
    CHECK_TRUE(
        (decodedHello.features &
         blinker::bbp2::FeatureReliableDelivery) != 0U);
    CHECK_EQ(
        static_cast<uint8_t>(2),
        decodedHello.reliableReceiveWindow);
    reliableHello.reliableReceiveWindow = 0;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeHelloBody(
            reliableHello,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded)
            .code());
    blinker::bbp2::ManifestRequestBody manifestRequest;
    manifestRequest.cursor = 17;
    CHECK_TRUE(blinker::bbp2::encodeManifestRequestBody(
                   manifestRequest,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedManifestRequest[] = {0xA1, 0x00, 0x11};
    CHECK_EQ(sizeof(expectedManifestRequest), encoded.size);
    blinker::bbp2::ManifestRequestBody decodedManifestRequest;
    CHECK_TRUE(blinker::bbp2::decodeManifestRequestBody(
                   encoded,
                   decodedManifestRequest)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(17), decodedManifestRequest.cursor);
    blinker::bbp2::ManifestAcceptBody manifestAccept;
    manifestAccept.revision = 7;
    manifestAccept.fingerprint =
        blinker::ByteView(fingerprint, sizeof(fingerprint));
    CHECK_TRUE(blinker::bbp2::encodeManifestAcceptBody(
                   manifestAccept,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    blinker::bbp2::ManifestAcceptBody decodedManifestAccept;
    CHECK_TRUE(blinker::bbp2::decodeManifestAcceptBody(
                   encoded,
                   decodedManifestAccept)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(7), decodedManifestAccept.revision);
    CHECK_EQ(
        static_cast<size_t>(blinker::bbp2::kManifestFingerprintSize),
        decodedManifestAccept.fingerprint.size);

    blinker::cbor::Limits shortHelloFingerprint;
    shortHelloFingerprint.maxByteStringLength = 31;
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        blinker::bbp2::encodeHelloBody(
            hello,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded,
            shortHelloFingerprint)
            .code());

    blinker::cbor::Limits helloDepth;
    helloDepth.maxDepth = 1;
    CHECK_EQ(
        blinker::ErrorCode::NestingTooDeep,
        blinker::bbp2::decodeHelloBody(
            blinker::ByteView(expectedHello, sizeof(expectedHello)),
            decodedHello,
            helloDepth)
            .code());

    blinker::bbp2::AckBody ack;
    ack.acknowledgedSequence = 0x1234;
    CHECK_TRUE(blinker::bbp2::encodeAckBody(
                   ack,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedAck[] = {0xA1, 0x00, 0x19, 0x12, 0x34};
    CHECK_EQ(sizeof(expectedAck), encoded.size);
    CHECK_TRUE(std::memcmp(expectedAck, encoded.data, encoded.size) == 0);
    blinker::bbp2::AckBody decodedAck;
    CHECK_TRUE(blinker::bbp2::decodeAckBody(encoded, decodedAck).ok());
    CHECK_EQ(static_cast<uint16_t>(0x1234), decodedAck.acknowledgedSequence);

    ack.stateRevision = 42;
    ack.hasStateRevision = true;
    CHECK_TRUE(blinker::bbp2::encodeAckBody(
                   ack,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedVersionedAck[] = {
        0xA2, 0x00, 0x19, 0x12, 0x34, 0x01, 0x18, 0x2A
    };
    CHECK_EQ(sizeof(expectedVersionedAck), encoded.size);
    CHECK_TRUE(std::memcmp(
                   expectedVersionedAck,
                   encoded.data,
                   encoded.size) == 0);
    CHECK_TRUE(blinker::bbp2::decodeAckBody(encoded, decodedAck).ok());
    CHECK_TRUE(decodedAck.hasStateRevision);
    CHECK_EQ(static_cast<uint32_t>(42), decodedAck.stateRevision);

    const uint8_t duplicateAck[] = {0xA2, 0x00, 0x01, 0x00, 0x02};
    CHECK_EQ(
        blinker::ErrorCode::DuplicateField,
        blinker::bbp2::decodeAckBody(
            blinker::ByteView(duplicateAck, sizeof(duplicateAck)),
            decodedAck)
            .code());

    blinker::bbp2::ErrorBody error;
    error.errorCode = 42;
    error.relatedSequence = 0x1234;
    error.hasRelatedSequence = true;
    error.detail = blinker::StringView("bad", 3);
    error.hasDetail = true;
    CHECK_TRUE(blinker::bbp2::encodeErrorBody(
                   error,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedError[] = {
        0xA3, 0x00, 0x18, 0x2A, 0x01, 0x19, 0x12, 0x34,
        0x02, 0x63, 0x62, 0x61, 0x64
    };
    CHECK_EQ(sizeof(expectedError), encoded.size);
    CHECK_TRUE(std::memcmp(expectedError, encoded.data, encoded.size) == 0);
    blinker::bbp2::ErrorBody decodedError;
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(encoded, decodedError).ok());
    CHECK_EQ(static_cast<uint16_t>(42), decodedError.errorCode);
    CHECK_TRUE(decodedError.hasDetail);

    error.stateRevision = 42;
    error.hasStateRevision = true;
    CHECK_TRUE(blinker::bbp2::encodeErrorBody(
                   error,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(encoded, decodedError).ok());
    CHECK_TRUE(decodedError.hasStateRevision);
    CHECK_EQ(static_cast<uint32_t>(42), decodedError.stateRevision);
    const uint8_t expectedVersionedError[] = {
        0xA4, 0x00, 0x18, 0x2A, 0x01, 0x19, 0x12, 0x34,
        0x02, 0x63, 0x62, 0x61, 0x64, 0x03, 0x18, 0x2A
    };
    CHECK_EQ(sizeof(expectedVersionedError), encoded.size);
    CHECK_TRUE(std::memcmp(
                   expectedVersionedError,
                   encoded.data,
                   encoded.size) == 0);

    const uint8_t keyedValues[] = {
        0xA1, 0x64, 0x6C, 0x61, 0x6D, 0x70, 0xF5
    };
    blinker::bbp2::StatePatchBody statePatch;
    statePatch.mode = blinker::bbp2::StatePatchMode::Apply;
    statePatch.revision = 42;
    statePatch.values =
        blinker::ByteView(keyedValues, sizeof(keyedValues));
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(
                   statePatch,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedStatePatch[] = {
        0xA3, 0x00, 0x01, 0x01, 0x18, 0x2A, 0x02,
        0xA1, 0x64, 0x6C, 0x61, 0x6D, 0x70, 0xF5
    };
    CHECK_EQ(sizeof(expectedStatePatch), encoded.size);
    CHECK_TRUE(std::memcmp(
                   expectedStatePatch,
                   encoded.data,
                   encoded.size) == 0);
    blinker::bbp2::StatePatchBody decodedStatePatch;
    CHECK_TRUE(blinker::bbp2::decodeStatePatchBody(
                   encoded,
                   decodedStatePatch)
                   .ok());
    CHECK_TRUE(
        decodedStatePatch.mode == blinker::bbp2::StatePatchMode::Apply);
    CHECK_EQ(static_cast<uint32_t>(42), decodedStatePatch.revision);
    CHECK_EQ(sizeof(keyedValues), decodedStatePatch.values.size);

    blinker::bbp2::StateRequestBody stateRequest;
    CHECK_TRUE(blinker::bbp2::encodeStateRequestBody(
                   stateRequest,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedInitialStateRequest[] = {0xA1, 0x00, 0x00};
    CHECK_EQ(sizeof(expectedInitialStateRequest), encoded.size);
    CHECK_TRUE(std::memcmp(
                   expectedInitialStateRequest,
                   encoded.data,
                   encoded.size) == 0);
    blinker::bbp2::StateRequestBody decodedStateRequest;
    CHECK_TRUE(blinker::bbp2::decodeStateRequestBody(
                   encoded,
                   decodedStateRequest)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(0), decodedStateRequest.cursor);
    CHECK_TRUE(!decodedStateRequest.hasObservedRevision);

    stateRequest.cursor = 2;
    stateRequest.observedRevision = 42;
    stateRequest.hasObservedRevision = true;
    CHECK_TRUE(blinker::bbp2::encodeStateRequestBody(
                   stateRequest,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedContinuationStateRequest[] = {
        0xA2, 0x00, 0x02, 0x01, 0x18, 0x2A
    };
    CHECK_EQ(sizeof(expectedContinuationStateRequest), encoded.size);
    CHECK_TRUE(std::memcmp(
                   expectedContinuationStateRequest,
                   encoded.data,
                   encoded.size) == 0);
    CHECK_TRUE(blinker::bbp2::decodeStateRequestBody(
                   encoded,
                   decodedStateRequest)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(2), decodedStateRequest.cursor);
    CHECK_TRUE(decodedStateRequest.hasObservedRevision);
    CHECK_EQ(static_cast<uint32_t>(42), decodedStateRequest.observedRevision);

    blinker::bbp2::StatePageBody statePage;
    statePage.revision = 42;
    statePage.cursor = 0;
    statePage.nextCursor = 2;
    statePage.totalFields = 2;
    statePage.values = blinker::ByteView(keyedValues, sizeof(keyedValues));
    CHECK_TRUE(blinker::bbp2::encodeStatePageBody(
                   statePage,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedStatePage[] = {
        0xA5, 0x00, 0x18, 0x2A, 0x01, 0x00, 0x02, 0x02,
        0x03, 0x02, 0x04, 0xA1, 0x64, 0x6C, 0x61, 0x6D,
        0x70, 0xF5
    };
    CHECK_EQ(sizeof(expectedStatePage), encoded.size);
    CHECK_TRUE(std::memcmp(
                   expectedStatePage,
                   encoded.data,
                   encoded.size) == 0);
    blinker::bbp2::StatePageBody decodedStatePage;
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   encoded,
                   decodedStatePage)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(42), decodedStatePage.revision);
    CHECK_EQ(static_cast<uint16_t>(0), decodedStatePage.cursor);
    CHECK_EQ(static_cast<uint16_t>(2), decodedStatePage.nextCursor);
    CHECK_EQ(static_cast<uint16_t>(2), decodedStatePage.totalFields);
    CHECK_EQ(sizeof(keyedValues), decodedStatePage.values.size);

    const uint8_t routePeerId[blinker::bbp2::kRouteIdentitySize] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    const uint8_t routeRequestId[blinker::bbp2::kRouteIdentitySize] = {
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
    };
    blinker::bbp2::RoutedMessageBody route;
    route.peerKind = blinker::bbp2::RoutePeerKind::LogicalDevice;
    route.peerId = blinker::ByteView(routePeerId, sizeof(routePeerId));
    route.requestId =
        blinker::ByteView(routeRequestId, sizeof(routeRequestId));
    route.hasRequestId = true;
    route.messageKind =
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Command);
    route.messageFlags = blinker::bbp2::FlagAckRequired |
        blinker::bbp2::FlagIdMode;
    route.messageBody = blinker::ByteView(keyedValues, sizeof(keyedValues));
    CHECK_TRUE(blinker::bbp2::encodeRouteBody(
                   route,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedRoute[] = {
        0xA6, 0x00, 0x00, 0x01, 0x50,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
        0x02, 0x50,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
        0x03, 0x12, 0x04, 0x05, 0x05,
        0xA1, 0x64, 0x6C, 0x61, 0x6D, 0x70, 0xF5
    };
    CHECK_EQ(sizeof(expectedRoute), encoded.size);
    CHECK_TRUE(std::memcmp(expectedRoute, encoded.data, encoded.size) == 0);
    blinker::bbp2::RoutedMessageBody decodedRoute;
    CHECK_TRUE(blinker::bbp2::decodeRouteBody(encoded, decodedRoute).ok());
    CHECK_TRUE(decodedRoute.hasRequestId);
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Command),
        decodedRoute.messageKind);
    CHECK_EQ(sizeof(keyedValues), decodedRoute.messageBody.size);

    blinker::bbp2::RoutedMessageBody delivery = route;
    delivery.peerKind = blinker::bbp2::RoutePeerKind::Platform;
    delivery.requestId = blinker::ByteView();
    delivery.hasRequestId = false;
    delivery.messageKind =
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Patch);
    delivery.messageFlags = blinker::bbp2::FlagNone;
    CHECK_TRUE(blinker::bbp2::encodeDeliveryBody(
                   delivery,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    blinker::bbp2::RoutedMessageBody decodedDelivery;
    CHECK_TRUE(blinker::bbp2::decodeDeliveryBody(
                   encoded, decodedDelivery).ok());
    CHECK_TRUE(!decodedDelivery.hasRequestId);
    CHECK_TRUE(
        decodedDelivery.peerKind == blinker::bbp2::RoutePeerKind::Platform);

    route.hasRequestId = false;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeRouteBody(
            route,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded)
            .code());
    route.hasRequestId = true;
    route.messageKind = static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeRouteBody(
            route,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded)
            .code());

    const uint8_t invalidInitialStateRequest[] = {
        0xA2, 0x00, 0x00, 0x01, 0x18, 0x2A
    };
    CHECK_TRUE(!blinker::bbp2::decodeStateRequestBody(
                    blinker::ByteView(
                        invalidInitialStateRequest,
                        sizeof(invalidInitialStateRequest)),
                    decodedStateRequest));

    const uint8_t authPayload[] = {0xAA, 0x55};
    blinker::bbp2::AuthRequestBody authRequest;
    authRequest.method = 1;
    authRequest.payload =
        blinker::ByteView(authPayload, sizeof(authPayload));
    CHECK_TRUE(blinker::bbp2::encodeAuthRequestBody(
                   authRequest,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedAuthRequest[] = {
        0xA2, 0x00, 0x01, 0x01, 0x42, 0xAA, 0x55
    };
    CHECK_EQ(sizeof(expectedAuthRequest), encoded.size);
    CHECK_TRUE(
        std::memcmp(expectedAuthRequest, encoded.data, encoded.size) == 0);
    blinker::bbp2::AuthRequestBody decodedAuthRequest;
    CHECK_TRUE(blinker::bbp2::decodeAuthRequestBody(
                   encoded,
                   decodedAuthRequest)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(1), decodedAuthRequest.method);
    CHECK_EQ(sizeof(authPayload), decodedAuthRequest.payload.size);

    const uint8_t challenge[] = {0x10};
    blinker::bbp2::AuthResultBody authResult;
    authResult.method = 1;
    authResult.status = blinker::bbp2::AuthStatus::Continue;
    authResult.payload = blinker::ByteView(challenge, sizeof(challenge));
    authResult.hasPayload = true;
    CHECK_TRUE(blinker::bbp2::encodeAuthResultBody(
                   authResult,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expectedAuthResult[] = {
        0xA3, 0x00, 0x01, 0x01, 0x00, 0x02, 0x41, 0x10
    };
    CHECK_EQ(sizeof(expectedAuthResult), encoded.size);
    CHECK_TRUE(
        std::memcmp(expectedAuthResult, encoded.data, encoded.size) == 0);
    blinker::bbp2::AuthResultBody decodedAuthResult;
    CHECK_TRUE(blinker::bbp2::decodeAuthResultBody(
                   encoded,
                   decodedAuthResult)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(1), decodedAuthResult.method);
    CHECK_TRUE(
        decodedAuthResult.status == blinker::bbp2::AuthStatus::Continue);
    CHECK_TRUE(decodedAuthResult.hasPayload);

    authRequest.method = 0;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::bbp2::encodeAuthRequestBody(
            authRequest,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded)
            .code());
    const uint8_t invalidAuthStatus[] = {
        0xA2, 0x00, 0x01, 0x01, 0x04
    };
    CHECK_EQ(
        blinker::ErrorCode::ValueOutOfRange,
        blinker::bbp2::decodeAuthResultBody(
            blinker::ByteView(
                invalidAuthStatus,
                sizeof(invalidAuthStatus)),
            decodedAuthResult)
            .code());
}

void testKeyedBodies() {
    uint8_t buffer[64] = {};
    blinker::bbp2::KeyedBodyWriter writer(
        blinker::MutableByteSpan(buffer, sizeof(buffer)));
    CHECK_TRUE(writer.begin(2).ok());
    CHECK_TRUE(writer.writeBool(blinker::StringView("power", 5), true).ok());
    CHECK_TRUE(writer.writeInt(blinker::StringView("temp", 4), -5).ok());
    blinker::ByteView encoded;
    CHECK_TRUE(writer.finish(encoded).ok());
    const uint8_t expected[] = {
        0xA2, 0x65, 0x70, 0x6F, 0x77, 0x65, 0x72, 0xF5,
        0x64, 0x74, 0x65, 0x6D, 0x70, 0x24
    };
    CHECK_EQ(sizeof(expected), encoded.size);
    CHECK_TRUE(std::memcmp(expected, encoded.data, encoded.size) == 0);

    KeyedCapture capture;
    CHECK_TRUE(blinker::bbp2::visitKeyedBody(
                   encoded,
                   &captureKeyedValue,
                   &capture)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(2), capture.calls);
    CHECK_TRUE(capture.firstType == blinker::cbor::Type::Boolean);
    CHECK_TRUE(capture.secondType == blinker::cbor::Type::Negative);

    const uint8_t duplicate[] = {
        0xA2, 0x61, 0x78, 0x01, 0x61, 0x78, 0x02
    };
    CHECK_EQ(
        blinker::ErrorCode::DuplicateField,
        blinker::bbp2::visitKeyedBody(
            blinker::ByteView(duplicate, sizeof(duplicate)),
            &captureKeyedValue,
            &capture)
            .code());

    blinker::cbor::Limits shortKeys;
    shortKeys.maxTextLength = 3;
    blinker::bbp2::KeyedBodyWriter limitedWriter(
        blinker::MutableByteSpan(buffer, sizeof(buffer)),
        shortKeys);
    CHECK_TRUE(limitedWriter.begin(1).ok());
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        limitedWriter.writeBool(
            blinker::StringView("power", 5), true).code());

    uint8_t idBuffer[32] = {};
    blinker::bbp2::IdBodyWriter idWriter(
        blinker::MutableByteSpan(idBuffer, sizeof(idBuffer)));
    CHECK_TRUE(idWriter.begin(2).ok());
    CHECK_TRUE(idWriter.writeBool(1, true).ok());
    CHECK_TRUE(idWriter.writeInt(2, -5).ok());
    CHECK_TRUE(idWriter.finish(encoded).ok());
    const uint8_t expectedIds[] = {0xA2, 0x01, 0xF5, 0x02, 0x24};
    CHECK_EQ(sizeof(expectedIds), encoded.size);
    CHECK_TRUE(std::memcmp(expectedIds, encoded.data, encoded.size) == 0);
    IdCapture idCapture;
    CHECK_TRUE(blinker::bbp2::visitIdBody(
                   encoded,
                   &captureIdValue,
                   &idCapture)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(2), idCapture.calls);
    CHECK_EQ(static_cast<uint16_t>(1), idCapture.firstId);
    CHECK_EQ(static_cast<uint16_t>(2), idCapture.secondId);
    CHECK_TRUE(idCapture.firstType == blinker::cbor::Type::Boolean);

    blinker::bbp2::KeyedBodyWriter floatWriter(
        blinker::MutableByteSpan(buffer, sizeof(buffer)));
    CHECK_TRUE(floatWriter.begin(2).ok());
    CHECK_TRUE(floatWriter.writeFloat32(
                   blinker::StringView("f32", 3), 1.5f).ok());
    CHECK_TRUE(floatWriter.writeFloat64(
                   blinker::StringView("f64", 3), 2.5).ok());
    CHECK_TRUE(floatWriter.finish(encoded).ok());
    KeyedCapture floatCapture;
    CHECK_TRUE(blinker::bbp2::visitKeyedBody(
                   encoded,
                   &captureKeyedValue,
                   &floatCapture)
                   .ok());
    CHECK_TRUE(floatCapture.firstType == blinker::cbor::Type::Float);
    CHECK_TRUE(floatCapture.secondType == blinker::cbor::Type::Float);

    blinker::bbp2::IdBodyWriter idFloatWriter(
        blinker::MutableByteSpan(idBuffer, sizeof(idBuffer)));
    CHECK_TRUE(idFloatWriter.begin(1).ok());
    CHECK_TRUE(idFloatWriter.writeFloat64(3, -0.25).ok());
    CHECK_TRUE(idFloatWriter.finish(encoded).ok());
    IdCapture idFloatCapture;
    CHECK_TRUE(blinker::bbp2::visitIdBody(
                   encoded,
                   &captureIdValue,
                   &idFloatCapture)
                   .ok());
    CHECK_TRUE(idFloatCapture.firstType == blinker::cbor::Type::Float);

    const uint8_t zeroId[] = {0xA1, 0x00, 0xF5};
    CHECK_EQ(
        blinker::ErrorCode::ValueOutOfRange,
        blinker::bbp2::visitIdBody(
            blinker::ByteView(zeroId, sizeof(zeroId)),
            &captureIdValue,
            &idCapture)
            .code());
    const uint8_t duplicateId[] = {
        0xA2, 0x01, 0xF5, 0x01, 0xF4};
    CHECK_EQ(
        blinker::ErrorCode::DuplicateField,
        blinker::bbp2::visitIdBody(
            blinker::ByteView(duplicateId, sizeof(duplicateId)),
            &captureIdValue,
            &idCapture)
            .code());
}

void testEndpointRegistryAndManifest() {
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify));
    const blinker::EndpointDescriptor* storage[1] = {};
    blinker::EndpointRegistry registry(storage, 1);
    CHECK_TRUE(registry.add(endpoint).ok());
    CHECK_EQ(blinker::ErrorCode::AlreadyExists, registry.add(endpoint).code());
    CHECK_TRUE(registry.find(blinker::StringView("lamp", 4)) == &endpoint);
    CHECK_TRUE(registry.find(blinker::StringView("missing", 7)) == nullptr);

    uint8_t buffer[64] = {};
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodeManifest(
                   1,
                   registry,
                   blinker::MutableByteSpan(buffer, sizeof(buffer)),
                   encoded)
                   .ok());
    const uint8_t expected[] = {
        0xA2, 0x00, 0x01, 0x01, 0x81,
        0xA5, 0x00, 0x64, 0x6C, 0x61, 0x6D, 0x70,
        0x01, 0x00, 0x02, 0x00, 0x03, 0x07, 0x04, 0x01
    };
    CHECK_EQ(sizeof(expected), encoded.size);
    CHECK_TRUE(std::memcmp(expected, encoded.data, encoded.size) == 0);
    CHECK_TRUE(blinker::cbor::validate(encoded).ok());
    uint8_t fingerprint[blinker::kSha256Size] = {};
    CHECK_TRUE(blinker::computeManifestFingerprint(
                   1,
                   registry,
                   blinker::MutableByteSpan(fingerprint, sizeof(fingerprint)))
                   .ok());
    const uint8_t expectedFingerprint[] = {
        0x23, 0x2A, 0xDD, 0x3A, 0xDC, 0xBC, 0x3A, 0xEB,
        0x81, 0xBB, 0xCE, 0x2C, 0xF4, 0x61, 0x0C, 0x4C,
        0xE9, 0x9C, 0x1F, 0x61, 0x23, 0xF0, 0x13, 0xCC,
        0x22, 0x1F, 0xB5, 0xC4, 0x9E, 0xBF, 0x05, 0x10
    };
    CHECK_TRUE(std::memcmp(
                   fingerprint,
                   expectedFingerprint,
                   sizeof(expectedFingerprint)) == 0);
    uint8_t pageBuffer[96] = {};
    blinker::ByteView page;
    uint16_t nextCursor = 0;
    CHECK_TRUE(blinker::encodeManifestPage(
                   1,
                   registry,
                   blinker::ByteView(fingerprint, sizeof(fingerprint)),
                   0,
                   blinker::MutableByteSpan(pageBuffer, sizeof(pageBuffer)),
                   page,
                   nextCursor)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(1), nextCursor);
    blinker::ManifestPageView decodedPage;
    CHECK_TRUE(blinker::decodeManifestPage(page, decodedPage).ok());
    CHECK_EQ(static_cast<uint32_t>(1), decodedPage.revision);
    CHECK_EQ(static_cast<uint16_t>(0), decodedPage.cursor);
    CHECK_EQ(static_cast<uint16_t>(1), decodedPage.nextCursor);
    CHECK_EQ(static_cast<uint16_t>(1), decodedPage.totalFields);
    CHECK_EQ(static_cast<uint16_t>(1), decodedPage.fieldCount);

    const blinker::FieldSpec pagingFields[] = {
        blinker::FieldSpec(
            blinker::StringView("channel-one", 11),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            blinker::AccessRead),
        blinker::FieldSpec(
            blinker::StringView("channel-two", 11),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            blinker::AccessRead),
        blinker::FieldSpec(
            blinker::StringView("channel-tri", 11),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            blinker::AccessRead)
    };
    blinker::EndpointRegistry pagingRegistry(
        pagingFields,
        sizeof(pagingFields) / sizeof(pagingFields[0]));
    CHECK_TRUE(pagingRegistry.seal().ok());
    uint8_t pagingFingerprint[blinker::kSha256Size] = {};
    CHECK_TRUE(blinker::computeManifestFingerprint(
                   9,
                   pagingRegistry,
                   blinker::MutableByteSpan(
                       pagingFingerprint,
                       sizeof(pagingFingerprint)))
                   .ok());
    // The complete Manifest does not fit, but the streaming fingerprint and
    // stateless cursor pages do not require a complete scratch copy.
    uint8_t pagingBuffer[68] = {};
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        blinker::encodeManifest(
            9,
            pagingRegistry,
            blinker::MutableByteSpan(pagingBuffer, sizeof(pagingBuffer)),
            page)
            .code());
    uint16_t cursor = 0;
    uint8_t pageCount = 0;
    while (cursor < 3U) {
        CHECK_TRUE(blinker::encodeManifestPage(
                       9,
                       pagingRegistry,
                       blinker::ByteView(
                           pagingFingerprint,
                           sizeof(pagingFingerprint)),
                       cursor,
                       blinker::MutableByteSpan(
                           pagingBuffer,
                           sizeof(pagingBuffer)),
                       page,
                       nextCursor)
                       .ok());
        CHECK_TRUE(nextCursor > cursor);
        CHECK_TRUE(page.size <= sizeof(pagingBuffer));
        CHECK_TRUE(blinker::decodeManifestPage(page, decodedPage).ok());
        CHECK_EQ(cursor, decodedPage.cursor);
        CHECK_EQ(nextCursor, decodedPage.nextCursor);
        CHECK_EQ(static_cast<uint16_t>(3), decodedPage.totalFields);
        CHECK_TRUE(std::memcmp(
                       decodedPage.fingerprint.data,
                       pagingFingerprint,
                       sizeof(pagingFingerprint)) == 0);
        cursor = nextCursor;
        ++pageCount;
    }
    CHECK_EQ(static_cast<uint8_t>(3), pageCount);
    uint16_t endpointId = 0;
    CHECK_TRUE(registry.idOf(blinker::StringView("lamp", 4), endpointId).ok());
    CHECK_EQ(static_cast<uint16_t>(1), endpointId);
    CHECK_TRUE(registry.findById(1) == &endpoint);
    CHECK_TRUE(registry.findById(0) == nullptr);

    blinker::cbor::Limits shallowManifest;
    shallowManifest.maxDepth = 3;
    CHECK_EQ(
        blinker::ErrorCode::NestingTooDeep,
        blinker::encodeManifest(
            1,
            registry,
            blinker::MutableByteSpan(buffer, sizeof(buffer)),
            encoded,
            shallowManifest)
            .code());
    CHECK_TRUE(registry.seal().ok());
    CHECK_TRUE(registry.sealed());
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        registry.add(endpoint).code());
    const blinker::FieldSpec duplicateStaticFields[] = {endpoint, endpoint};
    blinker::EndpointRegistry duplicateStaticRegistry(
        duplicateStaticFields,
        sizeof(duplicateStaticFields) / sizeof(duplicateStaticFields[0]));
    CHECK_EQ(
        blinker::ErrorCode::DuplicateField,
        duplicateStaticRegistry.seal().code());
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        duplicateStaticRegistry.add(endpoint).code());
    endpointId = 123;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        registry.idOf(blinker::StringView(nullptr, 1), endpointId).code());
    CHECK_EQ(static_cast<uint16_t>(0), endpointId);

    blinker::EndpointRegistry invalidAccessorRegistry(
        blinker::EndpointDescriptorTable(nullptr, nullptr, 1U));
    CHECK_TRUE(invalidAccessorRegistry.at(0U) == nullptr);
    CHECK_TRUE(invalidAccessorRegistry.find("lamp") == nullptr);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        invalidAccessorRegistry.seal().code());
}
void testEndpointConstraints() {
    const blinker::PropertyConstraints range(
        static_cast<uint8_t>(
            blinker::ConstraintMinimum | blinker::ConstraintMaximum |
            blinker::ConstraintStep | blinker::ConstraintUnit),
        0.0,
        100.0,
        5.0,
        0U,
        blinker::StringView("%"),
        nullptr,
        0U);
    const blinker::EndpointDescriptor levelEndpoint(
        "level",
        blinker::EndpointKind::Property,
        blinker::ValueType::SignedInteger,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite),
        &range);
    CHECK_TRUE(blinker::validateEndpointDescriptor(levelEndpoint).ok());

    uint8_t valueBuffer[32] = {};
    blinker::cbor::Writer value50(
        blinker::MutableByteSpan(valueBuffer, sizeof(valueBuffer)));
    CHECK_TRUE(value50.writeInt(50).ok());
    CHECK_TRUE(blinker::validateEndpointValue(
                   levelEndpoint,
                   value50.view(),
                   blinker::cbor::Type::Unsigned,
                   blinker::AccessWrite)
                   .ok());

    blinker::cbor::Writer value51(
        blinker::MutableByteSpan(valueBuffer, sizeof(valueBuffer)));
    CHECK_TRUE(value51.writeInt(51).ok());
    CHECK_EQ(
        blinker::ErrorCode::ValueOutOfRange,
        blinker::validateEndpointValue(
            levelEndpoint,
            value51.view(),
            blinker::cbor::Type::Unsigned,
            blinker::AccessWrite)
            .code());

    const blinker::StringView modes[] = {"auto", "manual"};
    const blinker::PropertyConstraints modeConstraints(
        static_cast<uint8_t>(
            blinker::ConstraintMaxLength | blinker::ConstraintEnumText),
        0.0,
        0.0,
        0.0,
        6U,
        blinker::StringView(),
        modes,
        2U);
    const blinker::EndpointDescriptor modeEndpoint(
        "mode",
        blinker::EndpointKind::Property,
        blinker::ValueType::Text,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite),
        &modeConstraints);
    CHECK_TRUE(blinker::validateEndpointDescriptor(modeEndpoint).ok());

    blinker::cbor::Writer autoValue(
        blinker::MutableByteSpan(valueBuffer, sizeof(valueBuffer)));
    CHECK_TRUE(autoValue.writeText("auto").ok());
    CHECK_TRUE(blinker::validateEndpointValue(
                   modeEndpoint,
                   autoValue.view(),
                   blinker::cbor::Type::Text,
                   blinker::AccessWrite)
                   .ok());

    blinker::cbor::Writer invalidMode(
        blinker::MutableByteSpan(valueBuffer, sizeof(valueBuffer)));
    CHECK_TRUE(invalidMode.writeText("invalid").ok());
    CHECK_EQ(
        blinker::ErrorCode::ValueOutOfRange,
        blinker::validateEndpointValue(
            modeEndpoint,
            invalidMode.view(),
            blinker::cbor::Type::Text,
            blinker::AccessWrite)
            .code());

    const blinker::EndpointDescriptor fields[] = {levelEndpoint, modeEndpoint};
    blinker::EndpointRegistry registry(fields, 2U);
    CHECK_TRUE(registry.seal().ok());
    uint8_t manifestBuffer[256] = {};
    blinker::ByteView manifest;
    CHECK_TRUE(blinker::encodeManifest(
                   4U,
                   registry,
                   blinker::MutableByteSpan(
                       manifestBuffer,
                       sizeof(manifestBuffer)),
                   manifest)
                   .ok());
    CHECK_TRUE(blinker::cbor::validate(manifest).ok());

    blinker::PropertyConstraints invalidRange = range;
    invalidRange.minimum = 101.0;
    const blinker::EndpointDescriptor invalidProperty(
        "invalid",
        blinker::EndpointKind::Property,
        blinker::ValueType::SignedInteger,
        blinker::AccessRead,
        &invalidRange);
    CHECK_EQ(
        blinker::ErrorCode::ValueOutOfRange,
        blinker::validateEndpointDescriptor(invalidProperty).code());

    const blinker::EndpointDescriptor invalidAction(
        "badAction",
        blinker::EndpointKind::Action,
        blinker::ValueType::Null,
        blinker::AccessWrite);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::validateEndpointDescriptor(invalidAction).code());
    const blinker::EndpointDescriptor invalidEvent(
        "badEvent",
        blinker::EndpointKind::Event,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessEvent));
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        blinker::validateEndpointDescriptor(invalidEvent).code());
}
struct CommandCapture {
    uint32_t calls;
    bool value;
    uint32_t sessionId;

    CommandCapture() : calls(0), value(false), sessionId(0) {}
};

struct DeliveryCapture {
    uint32_t calls;
    blinker::ReliableDeliveryEvent last;

    DeliveryCapture() : calls(0), last() {}
};

void captureDelivery(
    void* context,
    const blinker::ReliableDeliveryEvent& event) {
    DeliveryCapture* capture = static_cast<DeliveryCapture*>(context);
    ++capture->calls;
    capture->last = event;
}

blinker::Result rejectCommand(
    void* context,
    const blinker::EndpointDescriptor&,
    blinker::ByteView,
    blinker::cbor::Type,
    const blinker::RxContext&) {
    uint32_t* calls = static_cast<uint32_t*>(context);
    ++(*calls);
    return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
}

class AuthorizationCapture : public blinker::IAuthorizationProvider {
public:
    uint32_t calls;
    uint32_t lastSessionId;
    bool sawEncrypted;
    bool sawBonded;
    bool sawTransportAuthentication;
    uint32_t grantedPermissions;
    uint32_t queuedCalls;
    uint32_t closedCalls;
    uint32_t resetCalls;
    uint8_t lastTransportId;
    uint16_t methodCode;

    AuthorizationCapture()
        : calls(0),
          lastSessionId(0),
          sawEncrypted(false),
          sawBonded(false),
          sawTransportAuthentication(false),
          grantedPermissions(blinker::kAuthorizationPermissionControl),
          queuedCalls(0U),
          closedCalls(0U), resetCalls(0U), lastTransportId(0U), methodCode(1U) {}

    size_t methodCount() const override { return 1U; }
    uint16_t methodAt(size_t index) const override {
        return index == 0U ? methodCode : 0U;
    }

    blinker::Result authorize(
        uint16_t method,
        blinker::ByteView requestPayload,
        const blinker::AuthorizationSessionContext& session,
        blinker::AuthorizationDecision& decision) override {
        if (method != methodCode) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        ++calls;
        lastSessionId = session.sessionId;
        lastTransportId = session.transportId;
        sawEncrypted = session.encrypted;
        sawBonded = session.bonded;
        sawTransportAuthentication = session.transportAuthenticated;
        decision = blinker::AuthorizationDecision();
        if (requestPayload.empty()) {
            static const uint8_t challenge[] = {0xCA, 0xFE};
            decision.status = blinker::bbp2::AuthStatus::Continue;
            decision.responsePayload =
                blinker::ByteView(challenge, sizeof(challenge));
        } else if (requestPayload.size == 1U &&
                   requestPayload.data[0] == 0xA5U) {
            decision.status = blinker::bbp2::AuthStatus::Authorized;
            decision.permissions = grantedPermissions;
        }
        return blinker::Result::success();
    }

    void authorizationResultQueued(
        const blinker::AuthorizationSessionContext&) override {
        ++queuedCalls;
    }
    void sessionClosed(
        const blinker::AuthorizationSessionContext& session) override {
        ++closedCalls;
        lastSessionId = session.sessionId;
        lastTransportId = session.transportId;
    }
    void resetSessions() override { ++resetCalls; }
};

class ControllerControlCapture final
    : public blinker::IControllerControlEndpoint {
public:
    ControllerControlCapture()
        : beginCalls(0U),
          mutationCalls(0U),
          closeCalls(0U),
          lastSessionId(0U),
          lastTransportId(0U),
          presenceEnabled(false), presenceCalls(0U),
          nonce(),
          receipt() {
        for (size_t index = 0U; index < sizeof(nonce); ++index) {
            nonce[index] = static_cast<uint8_t>(0x40U + index);
        }
        receipt[0] = 0xA0U;
    }

    blinker::Result beginControlWindow(
        const blinker::RxContext& rx,
        blinker::ByteView& output) override {
        ++beginCalls;
        lastSessionId = rx.sessionId;
        output = blinker::ByteView(nonce, sizeof(nonce));
        return blinker::Result::success();
    }

    blinker::Result applyControllerMutation(
        const blinker::RxContext& rx,
        blinker::ByteView grant,
        blinker::ByteView secret,
        blinker::MutableByteSpan output,
        blinker::ByteView& encodedReceipt) override {
        ++mutationCalls;
        lastSessionId = rx.sessionId;
        if (grant.empty() || secret.size != 32U || output.size < 1U) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        output.data[0] = receipt[0];
        encodedReceipt = blinker::ByteView(output.data, 1U);
        return blinker::Result::success();
    }

    void controllerSessionClosed(
        const blinker::RxContext& rx) override {
        ++closeCalls;
        lastSessionId = rx.sessionId;
        lastTransportId = rx.transportId;
    }

    bool supportsPresenceKeyControl() const override { return presenceEnabled; }
    blinker::Result applyPresenceKeyMutation(
        const blinker::RxContext& rx, blinker::ByteView,
        blinker::MutableByteSpan output, blinker::ByteView& encoded) override {
        ++presenceCalls;
        lastSessionId = rx.sessionId;
        lastTransportId = rx.transportId;
        if (output.size < 1U) return blinker::Result::failure(
            blinker::ErrorCode::BufferTooSmall);
        output.data[0] = 0xA0U;
        encoded = blinker::ByteView(output.data, 1U);
        return blinker::Result::success();
    }

    uint32_t beginCalls;
    uint32_t mutationCalls;
    uint32_t closeCalls;
    uint32_t lastSessionId;
    uint8_t lastTransportId;
    bool presenceEnabled;
    uint32_t presenceCalls;
    uint8_t nonce[16];
    uint8_t receipt[1];
};

blinker::Result captureCommand(
    void* context,
    const blinker::EndpointDescriptor&,
    blinker::ByteView encodedValue,
    blinker::cbor::Type valueType,
    const blinker::RxContext& rx) {
    if (valueType != blinker::cbor::Type::Boolean) {
        return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
    }
    blinker::cbor::Reader reader(encodedValue);
    bool value = false;
    blinker::Result result = reader.readBool(value);
    if (!result || !reader.finished()) return result;
    CommandCapture* capture = static_cast<CommandCapture*>(context);
    ++capture->calls;
    capture->value = value;
    capture->sessionId = rx.sessionId;
    return blinker::Result::success();
}

blinker::Result encodeTestState(
    void*,
    blinker::bbp2::KeyedBodyWriter& writer) {
    blinker::Result result = writer.begin(1);
    if (result) {
        result = writer.writeBool(
            blinker::StringView("lamp", 4),
            true);
    }
    return result;
}

blinker::Result encodeTestStatePage(
    void* context,
    uint32_t revision,
    uint16_t cursor,
    bool idMode,
    blinker::MutableByteSpan output,
    blinker::ByteView& encoded,
    uint16_t& nextCursor,
    uint16_t& totalFields) {
    totalFields = context != nullptr
                      ? *static_cast<const uint16_t*>(context)
                      : 1U;
    if (cursor > totalFields) {
        return blinker::Result::failure(blinker::ErrorCode::ValueOutOfRange);
    }

    nextCursor = totalFields;
    blinker::cbor::Writer writer(output);
    blinker::Result result = writer.beginMap(5);
    if (result) result = writer.writeUnsigned(0);
    if (result) result = writer.writeUnsigned(revision);
    if (result) result = writer.writeUnsigned(1);
    if (result) result = writer.writeUnsigned(cursor);
    if (result) result = writer.writeUnsigned(2);
    if (result) result = writer.writeUnsigned(nextCursor);
    if (result) result = writer.writeUnsigned(3);
    if (result) result = writer.writeUnsigned(totalFields);
    if (result) result = writer.writeUnsigned(4);
    if (result) result = writer.beginMap(cursor == 0U ? 1U : 0U);
    if (result && cursor == 0U) {
        result = idMode
                     ? writer.writeUnsigned(1)
                     : writer.writeText(blinker::StringView("lamp", 4));
    }
    if (result && cursor == 0U) result = writer.writeBool(true);
    if (!result) return result;
    encoded = writer.view();
    return blinker::Result::success();
}

blinker::Result encodeTestStateRequest(
    uint16_t cursor,
    uint32_t observedRevision,
    bool hasObservedRevision,
    blinker::MutableByteSpan output,
    blinker::ByteView& encoded) {
    blinker::bbp2::StateRequestBody request;
    request.cursor = cursor;
    request.observedRevision = observedRevision;
    request.hasObservedRevision = hasObservedRevision;
    return blinker::bbp2::encodeStateRequestBody(request, output, encoded);
}

blinker::Result encodeStoredStatePage(
    void* context,
    uint32_t revision,
    uint16_t cursor,
    bool idMode,
    blinker::MutableByteSpan output,
    blinker::ByteView& encoded,
    uint16_t& nextCursor,
    uint16_t& totalFields) {
    blinker::EndpointStateStore* store =
        static_cast<blinker::EndpointStateStore*>(context);
    return store != nullptr
               ? store->encodeSnapshotPage(
                     revision,
                     cursor,
                     idMode,
                     output,
                     encoded,
                     nextCursor,
                     totalFields)
               : blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
}

blinker::Result applyStoredState(
    void* context,
    blinker::ByteView values,
    bool idMode,
    const blinker::RxContext&,
    bool& changed) {
    blinker::EndpointStateStore* store =
        static_cast<blinker::EndpointStateStore*>(context);
    if (store == nullptr) {
        return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
    }
    return idMode
               ? store->applyById(values, blinker::AccessWrite, changed)
               : store->applyKeyed(values, blinker::AccessWrite, changed);
}

blinker::Result buildTestFrame(
    blinker::bbp2::MessageKind kind,
    uint8_t flags,
    uint16_t sequence,
    blinker::ByteView body,
    blinker::MutableByteSpan output,
    blinker::ByteView& frame) {
    if (body.size > output.size - blinker::bbp2::kBaseHeaderSize) {
        return blinker::Result::failure(blinker::ErrorCode::BufferTooSmall);
    }
    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(kind);
    header.flags = flags;
    header.sequence = sequence;
    header.bodyLength = static_cast<uint16_t>(body.size);
    blinker::Result result = blinker::bbp2::encodeHeader(header, output);
    if (!result) return result;
    if (body.size != 0U) {
        std::memcpy(
            output.data + blinker::bbp2::kBaseHeaderSize,
            body.data,
            body.size);
    }
    frame = blinker::ByteView(
        output.data,
        blinker::bbp2::kBaseHeaderSize + body.size);
    return blinker::Result::success();
}

struct CanonicalCommandCapture {
    uint32_t calls;
    bool value;
    uint32_t sessionId;

    CanonicalCommandCapture() : calls(0), value(false), sessionId(0) {}
};

struct CanonicalStateWriteCapture {
    uint32_t calls;
    bool idMode;
    uint32_t sessionId;
    bool accept;

    CanonicalStateWriteCapture()
        : calls(0), idMode(false), sessionId(0), accept(true) {}
};
blinker::Property<bool>* semanticPowerTarget = nullptr;
uint32_t semanticPowerCalls = 0U;
bool semanticPowerRequested = false;
uint32_t semanticRestartCalls = 0U;

void captureSemanticPowerWrite(bool requested) {
    ++semanticPowerCalls;
    semanticPowerRequested = requested;
    if (semanticPowerTarget != nullptr) {
        semanticPowerTarget->report(requested);
    }
}

blinker::Result captureSemanticRestart() {
    ++semanticRestartCalls;
    return blinker::Result::success();
}

BLINKER_PROPERTY(facadePower, bool, blinker::readWrite());
BLINKER_PROPERTY(facadeTemperature, float, blinker::readOnly());
BLINKER_ACTION(facadeRestart);
BLINKER_EVENT(facadeAlarm, uint32_t);

uint32_t facadePowerCalls = 0U;
bool facadePowerRequested = false;

void captureFacadePower(bool requested) {
    ++facadePowerCalls;
    facadePowerRequested = requested;
    facadePower.report(requested);
}

blinker::Result captureCanonicalCommand(
    void* context,
    const blinker::EndpointHandle& endpoint,
    const blinker::EndpointValueView& value,
    const blinker::RxContext& rx) {
    CanonicalCommandCapture* capture =
        static_cast<CanonicalCommandCapture*>(context);
    if (capture == nullptr || endpoint.kind() != blinker::EndpointKind::Property) {
        return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
    }
    bool decoded = false;
    blinker::Result result = value.asBool(decoded);
    if (!result) return result;
    ++capture->calls;
    capture->value = decoded;
    capture->sessionId = rx.sessionId;
    return blinker::Result::success();
}

blinker::Result acceptCanonicalStateWrite(
    void* context,
    blinker::ByteView,
    bool idMode,
    const blinker::RxContext& rx) {
    CanonicalStateWriteCapture* capture =
        static_cast<CanonicalStateWriteCapture*>(context);
    if (capture == nullptr) {
        return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
    }
    ++capture->calls;
    capture->idMode = idMode;
    capture->sessionId = rx.sessionId;
    return capture->accept
               ? blinker::Result::success()
               : blinker::Result::failure(
                     blinker::ErrorCode::InvalidArgument);
}

void testEndpointStateStore() {
    blinker::PropertyConstraints textConstraints;
    textConstraints.flags = blinker::ConstraintMaxLength;
    textConstraints.maxLength = 32;
    const blinker::EndpointDescriptor powerEndpoint(
        blinker::StringView("power", 5),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify));
    const blinker::EndpointDescriptor textEndpoint(
        blinker::StringView("label", 5),
        blinker::EndpointKind::Property,
        blinker::ValueType::Text,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify),
        &textConstraints);
    const blinker::EndpointDescriptor* registryStorage[2] = {};
    blinker::EndpointRegistry registry(registryStorage, 2);
    CHECK_TRUE(registry.add(powerEndpoint).ok());
    CHECK_TRUE(registry.add(textEndpoint).ok());
    CHECK_TRUE(registry.seal().ok());

    uint8_t stateArena[8] = {};
    blinker::EndpointStateSlot slots[2];
    blinker::EndpointStateStore store(
        registry,
        slots,
        2,
        blinker::MutableByteSpan(stateArena, sizeof(stateArena)));
    CHECK_TRUE(store.validateStorage().ok());
    static_assert(
        sizeof(blinker::EndpointStateSlot) <= 8U,
        "EndpointStateSlot exceeds the compact metadata gate");

    uint8_t bodyBuffer[64] = {};
    blinker::bbp2::IdBodyWriter idWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(idWriter.begin(2).ok());
    CHECK_TRUE(idWriter.writeBool(1, true).ok());
    CHECK_TRUE(idWriter.writeText(2, blinker::StringView("ok", 2)).ok());
    blinker::ByteView body;
    CHECK_TRUE(idWriter.finish(body).ok());
    bool changed = false;
    CHECK_TRUE(store.applyById(body, blinker::AccessWrite, changed).ok());
    CHECK_TRUE(changed);
    CHECK_EQ(static_cast<uint32_t>(4), store.usedBytes());

    blinker::ByteView storedPower;
    blinker::cbor::Type storedType = blinker::cbor::Type::Invalid;
    CHECK_TRUE(store.value(powerEndpoint, storedPower, storedType).ok());
    blinker::cbor::Reader powerReader(storedPower);
    bool power = false;
    CHECK_TRUE(powerReader.readBool(power).ok());
    CHECK_TRUE(power);
    CHECK_TRUE(store.applyById(body, blinker::AccessWrite, changed).ok());
    CHECK_TRUE(!changed);

    blinker::bbp2::KeyedBodyWriter rejectedWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(rejectedWriter.begin(2).ok());
    CHECK_TRUE(rejectedWriter.writeBool(
                   blinker::StringView("power", 5), false)
                   .ok());
    CHECK_TRUE(rejectedWriter.writeText(
                   blinker::StringView("label", 5),
                   blinker::StringView("too-long", 8))
                   .ok());
    CHECK_TRUE(rejectedWriter.finish(body).ok());
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        store.applyKeyed(body, blinker::AccessWrite, changed).code());
    CHECK_TRUE(store.value(powerEndpoint, storedPower, storedType).ok());
    powerReader = blinker::cbor::Reader(storedPower);
    CHECK_TRUE(powerReader.readBool(power).ok());
    CHECK_TRUE(power);

    uint8_t snapshotBuffer[64] = {};
    blinker::bbp2::KeyedBodyWriter snapshotWriter(
        blinker::MutableByteSpan(snapshotBuffer, sizeof(snapshotBuffer)));
    CHECK_TRUE(store.encodeSnapshot(snapshotWriter).ok());
    blinker::ByteView snapshot;
    CHECK_TRUE(snapshotWriter.finish(snapshot).ok());
    blinker::cbor::Reader snapshotOrder(snapshot);
    size_t snapshotFields = 0U;
    blinker::StringView firstSnapshotKey;
    blinker::StringView secondSnapshotKey;
    CHECK_TRUE(snapshotOrder.readMapSize(snapshotFields).ok());
    CHECK_EQ(static_cast<size_t>(2U), snapshotFields);
    CHECK_TRUE(snapshotOrder.readText(firstSnapshotKey).ok());
    CHECK_TRUE(snapshotOrder.skipValue().ok());
    CHECK_TRUE(snapshotOrder.readText(secondSnapshotKey).ok());
    CHECK_TRUE(snapshotOrder.skipValue().ok());
    CHECK_TRUE(snapshotOrder.finished());
    CHECK_EQ(static_cast<size_t>(5U), firstSnapshotKey.size);
    CHECK_TRUE(std::memcmp(firstSnapshotKey.data, "label", 5U) == 0);
    CHECK_EQ(static_cast<size_t>(5U), secondSnapshotKey.size);
    CHECK_TRUE(std::memcmp(secondSnapshotKey.data, "power", 5U) == 0);
    KeyedCapture capture;
    CHECK_TRUE(blinker::bbp2::visitKeyedBody(
                   snapshot,
                   &captureKeyedValue,
                   &capture)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(2), capture.calls);

    uint8_t selectedFields[] = {0x03U};
    blinker::bbp2::KeyedBodyWriter selectionWriter(
        blinker::MutableByteSpan(snapshotBuffer, sizeof(snapshotBuffer)));
    CHECK_TRUE(store.encodeSelection(
                   selectionWriter,
                   blinker::ByteView(selectedFields, sizeof(selectedFields)),
                   2U)
                   .ok());
    CHECK_TRUE(selectionWriter.finish(snapshot).ok());
    snapshotOrder = blinker::cbor::Reader(snapshot);
    CHECK_TRUE(snapshotOrder.readMapSize(snapshotFields).ok());
    CHECK_TRUE(snapshotOrder.readText(firstSnapshotKey).ok());
    CHECK_TRUE(snapshotOrder.skipValue().ok());
    CHECK_TRUE(snapshotOrder.readText(secondSnapshotKey).ok());
    CHECK_EQ(static_cast<size_t>(5U), firstSnapshotKey.size);
    CHECK_TRUE(std::memcmp(firstSnapshotKey.data, "label", 5U) == 0);
    CHECK_EQ(static_cast<size_t>(5U), secondSnapshotKey.size);
    CHECK_TRUE(std::memcmp(secondSnapshotKey.data, "power", 5U) == 0);

    uint8_t canonicalPageBuffer[64] = {};
    blinker::ByteView canonicalPage;
    uint16_t canonicalNextCursor = 0U;
    uint16_t canonicalTotalFields = 0U;
    CHECK_TRUE(store.encodeSnapshotPage(
                   9U,
                   0U,
                   false,
                   blinker::MutableByteSpan(
                       canonicalPageBuffer,
                       sizeof(canonicalPageBuffer)),
                   canonicalPage,
                   canonicalNextCursor,
                   canonicalTotalFields)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(2U), canonicalNextCursor);
    blinker::bbp2::StatePageBody canonicalDecodedPage;
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   canonicalPage,
                   canonicalDecodedPage)
                   .ok());
    snapshotOrder = blinker::cbor::Reader(canonicalDecodedPage.values);
    CHECK_TRUE(snapshotOrder.readMapSize(snapshotFields).ok());
    CHECK_TRUE(snapshotOrder.readText(firstSnapshotKey).ok());
    CHECK_TRUE(snapshotOrder.skipValue().ok());
    CHECK_TRUE(snapshotOrder.readText(secondSnapshotKey).ok());
    CHECK_EQ(static_cast<size_t>(5U), firstSnapshotKey.size);
    CHECK_TRUE(std::memcmp(firstSnapshotKey.data, "label", 5U) == 0);
    CHECK_EQ(static_cast<size_t>(5U), secondSnapshotKey.size);
    CHECK_TRUE(std::memcmp(secondSnapshotKey.data, "power", 5U) == 0);

    // A snapshot page is selected on complete field boundaries and is
    // encoded directly into the caller's frame-sized output buffer.
    uint8_t pageBuffer[22] = {};
    blinker::ByteView encodedPage;
    uint16_t nextCursor = 0;
    uint16_t totalFields = 0;
    CHECK_TRUE(store.encodeSnapshotPage(
                   9,
                   0,
                   false,
                   blinker::MutableByteSpan(pageBuffer, sizeof(pageBuffer)),
                   encodedPage,
                   nextCursor,
                   totalFields)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(1), nextCursor);
    CHECK_EQ(static_cast<uint16_t>(2), totalFields);
    CHECK_TRUE(encodedPage.size <= sizeof(pageBuffer));
    blinker::bbp2::StatePageBody decodedPage;
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   encodedPage,
                   decodedPage)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(9), decodedPage.revision);
    CHECK_EQ(static_cast<uint16_t>(0), decodedPage.cursor);
    CHECK_EQ(static_cast<uint16_t>(1), decodedPage.nextCursor);
    KeyedCapture firstPageCapture;
    CHECK_TRUE(blinker::bbp2::visitKeyedBody(
                   decodedPage.values,
                   &captureKeyedValue,
                   &firstPageCapture)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(1), firstPageCapture.calls);

    CHECK_TRUE(store.encodeSnapshotPage(
                   9,
                   nextCursor,
                   false,
                   blinker::MutableByteSpan(pageBuffer, sizeof(pageBuffer)),
                   encodedPage,
                   nextCursor,
                   totalFields)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(2), nextCursor);
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   encodedPage,
                   decodedPage)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(1), decodedPage.cursor);
    CHECK_EQ(static_cast<uint16_t>(2), decodedPage.nextCursor);
    KeyedCapture secondPageCapture;
    CHECK_TRUE(blinker::bbp2::visitKeyedBody(
                   decodedPage.values,
                   &captureKeyedValue,
                   &secondPageCapture)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(1), secondPageCapture.calls);

    uint8_t tooSmallPage[17] = {};
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        store.encodeSnapshotPage(
                 9,
                 0,
                 false,
                 blinker::MutableByteSpan(
                     tooSmallPage,
                     sizeof(tooSmallPage)),
                 encodedPage,
                 nextCursor,
                 totalFields)
            .code());
    blinker::cbor::Limits shallowLimits;
    shallowLimits.maxDepth = 1;
    CHECK_EQ(
        blinker::ErrorCode::NestingTooDeep,
        store.encodeSnapshotPage(
                 9,
                 0,
                 false,
                 blinker::MutableByteSpan(pageBuffer, sizeof(pageBuffer)),
                 encodedPage,
                 nextCursor,
                 totalFields,
                 shallowLimits)
            .code());

    CHECK_TRUE(store.encodeSnapshotPage(
                   9,
                   0,
                   true,
                   blinker::MutableByteSpan(pageBuffer, sizeof(pageBuffer)),
                   encodedPage,
                   nextCursor,
                   totalFields)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(2), nextCursor);
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   encodedPage,
                   decodedPage)
                   .ok());
    IdCapture idPageCapture;
    CHECK_TRUE(blinker::bbp2::visitIdBody(
                   decodedPage.values,
                   &captureIdValue,
                   &idPageCapture)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(2), idPageCapture.calls);
    CHECK_EQ(static_cast<uint16_t>(1), idPageCapture.firstId);
    CHECK_EQ(static_cast<uint16_t>(2), idPageCapture.secondId);

    // A per-container item limit is also a page boundary. It must not turn a
    // snapshot with more initialized fields into a permanent CapacityExceeded.
    const blinker::EndpointDescriptor itemEndpoints[6] = {
        blinker::EndpointDescriptor(
            blinker::StringView("a", 1),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify)),
        blinker::EndpointDescriptor(
            blinker::StringView("b", 1),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify)),
        blinker::EndpointDescriptor(
            blinker::StringView("c", 1),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify)),
        blinker::EndpointDescriptor(
            blinker::StringView("d", 1),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify)),
        blinker::EndpointDescriptor(
            blinker::StringView("e", 1),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify)),
        blinker::EndpointDescriptor(
            blinker::StringView("f", 1),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify))
    };
    const blinker::EndpointDescriptor* itemRegistryStorage[6] = {};
    blinker::EndpointRegistry itemRegistry(itemRegistryStorage, 6);
    for (size_t index = 0; index < 6U; ++index) {
        CHECK_TRUE(itemRegistry.add(itemEndpoints[index]).ok());
    }
    CHECK_TRUE(itemRegistry.seal().ok());
    uint8_t itemArena[6] = {};
    blinker::EndpointStateSlot itemSlots[6];
    blinker::EndpointStateStore itemStore(
        itemRegistry,
        itemSlots,
        6,
        blinker::MutableByteSpan(itemArena, sizeof(itemArena)));
    uint8_t itemValuesBuffer[32] = {};
    blinker::bbp2::KeyedBodyWriter itemValuesWriter(
        blinker::MutableByteSpan(itemValuesBuffer, sizeof(itemValuesBuffer)));
    CHECK_TRUE(itemValuesWriter.begin(6).ok());
    for (size_t index = 0; index < 6U; ++index) {
        CHECK_TRUE(itemValuesWriter.writeBool(
                       itemEndpoints[index].key,
                       true)
                       .ok());
    }
    blinker::ByteView itemValues;
    CHECK_TRUE(itemValuesWriter.finish(itemValues).ok());
    CHECK_TRUE(itemStore.applyKeyed(
                   itemValues,
                   blinker::AccessWrite,
                   changed)
                   .ok());
    blinker::cbor::Limits itemLimits;
    itemLimits.maxContainerItems = 5;
    uint8_t itemPageBuffer[96] = {};
    CHECK_TRUE(itemStore.encodeSnapshotPage(
                   10,
                   0,
                   false,
                   blinker::MutableByteSpan(
                       itemPageBuffer,
                       sizeof(itemPageBuffer)),
                   encodedPage,
                   nextCursor,
                   totalFields,
                   itemLimits)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(5), nextCursor);
    CHECK_EQ(static_cast<uint16_t>(6), totalFields);
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   encodedPage,
                   decodedPage,
                   itemLimits)
                   .ok());
    KeyedCapture itemPageCapture;
    CHECK_TRUE(blinker::bbp2::visitKeyedBody(
                   decodedPage.values,
                   &captureKeyedValue,
                   &itemPageCapture,
                   itemLimits)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(5), itemPageCapture.calls);
    CHECK_TRUE(itemStore.encodeSnapshotPage(
                   10,
                   nextCursor,
                   false,
                   blinker::MutableByteSpan(
                       itemPageBuffer,
                       sizeof(itemPageBuffer)),
                   encodedPage,
                   nextCursor,
                   totalFields,
                   itemLimits)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(6), nextCursor);

    // The arena is sized for the final state, not for the largest value of
    // every field. A growing first field followed by a shrinking second field
    // must commit atomically even when the intermediate sequential size would
    // exceed the arena.
    const blinker::EndpointDescriptor packedFirstEndpoint(
        blinker::StringView("a", 1),
        blinker::EndpointKind::Property,
        blinker::ValueType::Text,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify));
    const blinker::EndpointDescriptor packedSecondEndpoint(
        blinker::StringView("b", 1),
        blinker::EndpointKind::Property,
        blinker::ValueType::Text,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify));
    const blinker::EndpointDescriptor* packedRegistryStorage[2] = {};
    blinker::EndpointRegistry packedRegistry(packedRegistryStorage, 2);
    CHECK_TRUE(packedRegistry.add(packedFirstEndpoint).ok());
    CHECK_TRUE(packedRegistry.add(packedSecondEndpoint).ok());
    CHECK_TRUE(packedRegistry.seal().ok());
    uint8_t packedArena[10] = {};
    blinker::EndpointStateSlot packedSlots[2];
    blinker::EndpointStateStore packedStore(
        packedRegistry,
        packedSlots,
        2,
        blinker::MutableByteSpan(packedArena, sizeof(packedArena)));
    CHECK_TRUE(packedStore.validateStorage().ok());

    blinker::bbp2::KeyedBodyWriter packedInitialWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(packedInitialWriter.begin(2).ok());
    CHECK_TRUE(packedInitialWriter.writeText(
                   blinker::StringView("a", 1),
                   blinker::StringView("x", 1))
                   .ok());
    CHECK_TRUE(packedInitialWriter.writeText(
                   blinker::StringView("b", 1),
                   blinker::StringView("1234567", 7))
                   .ok());
    CHECK_TRUE(packedInitialWriter.finish(body).ok());
    CHECK_TRUE(packedStore.applyKeyed(
                   body, blinker::AccessWrite, changed)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(10), packedStore.usedBytes());

    blinker::bbp2::KeyedBodyWriter packedSwapWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(packedSwapWriter.begin(2).ok());
    CHECK_TRUE(packedSwapWriter.writeText(
                   blinker::StringView("a", 1),
                   blinker::StringView("7654321", 7))
                   .ok());
    CHECK_TRUE(packedSwapWriter.writeText(
                   blinker::StringView("b", 1),
                   blinker::StringView("y", 1))
                   .ok());
    CHECK_TRUE(packedSwapWriter.finish(body).ok());
    CHECK_TRUE(packedStore.applyKeyed(
                   body, blinker::AccessWrite, changed)
                   .ok());
    CHECK_TRUE(changed);
    CHECK_EQ(static_cast<uint32_t>(10), packedStore.usedBytes());

    blinker::ByteView packedBeforeFailure;
    CHECK_TRUE(packedStore.value(
                   packedSecondEndpoint,
                   packedBeforeFailure,
                   storedType)
                   .ok());
    uint8_t packedBeforeFailureCopy[2] = {};
    CHECK_EQ(
        static_cast<uint32_t>(sizeof(packedBeforeFailureCopy)),
        packedBeforeFailure.size);
    memcpy(
        packedBeforeFailureCopy,
        packedBeforeFailure.data,
        sizeof(packedBeforeFailureCopy));
    blinker::bbp2::KeyedBodyWriter packedOverflowWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(packedOverflowWriter.begin(2).ok());
    CHECK_TRUE(packedOverflowWriter.writeText(
                   blinker::StringView("a", 1),
                   blinker::StringView("7654321", 7))
                   .ok());
    CHECK_TRUE(packedOverflowWriter.writeText(
                   blinker::StringView("b", 1),
                   blinker::StringView("1234567", 7))
                   .ok());
    CHECK_TRUE(packedOverflowWriter.finish(body).ok());
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        packedStore.applyKeyed(body, blinker::AccessWrite, changed).code());
    blinker::ByteView packedAfterFailure;
    CHECK_TRUE(packedStore.value(
                   packedSecondEndpoint,
                   packedAfterFailure,
                   storedType)
                   .ok());
    CHECK_EQ(
        static_cast<uint32_t>(sizeof(packedBeforeFailureCopy)),
        packedAfterFailure.size);
    CHECK_TRUE(memcmp(
                   packedBeforeFailureCopy,
                   packedAfterFailure.data,
                   sizeof(packedBeforeFailureCopy)) == 0);
}

void testRevisionConsistentStatePagination() {
    const blinker::EndpointDescriptor alpha(
        blinker::StringView("alpha", 5),
        blinker::EndpointKind::Property,
        blinker::ValueType::Text,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify));
    const blinker::EndpointDescriptor bravo(
        blinker::StringView("bravo", 5),
        blinker::EndpointKind::Property,
        blinker::ValueType::Text,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify));
    const blinker::EndpointDescriptor charlie(
        blinker::StringView("charlie", 7),
        blinker::EndpointKind::Property,
        blinker::ValueType::Text,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify));
    const blinker::EndpointDescriptor* registryStorage[3] = {};
    blinker::EndpointRegistry registry(registryStorage, 3);
    CHECK_TRUE(registry.add(alpha).ok());
    CHECK_TRUE(registry.add(bravo).ok());
    CHECK_TRUE(registry.add(charlie).ok());
    CHECK_TRUE(registry.seal().ok());

    blinker::EndpointStateSlot stateSlots[3];
    uint8_t stateArena[32] = {};
    blinker::EndpointStateStore store(
        registry,
        stateSlots,
        3,
        blinker::MutableByteSpan(stateArena, sizeof(stateArena)));
    uint8_t valuesBuffer[96] = {};
    blinker::bbp2::KeyedBodyWriter initialValues(
        blinker::MutableByteSpan(valuesBuffer, sizeof(valuesBuffer)));
    CHECK_TRUE(initialValues.begin(3).ok());
    CHECK_TRUE(initialValues.writeText(
                   blinker::StringView("alpha", 5),
                   blinker::StringView("12345678", 8))
                   .ok());
    CHECK_TRUE(initialValues.writeText(
                   blinker::StringView("bravo", 5),
                   blinker::StringView("12345678", 8))
                   .ok());
    CHECK_TRUE(initialValues.writeText(
                   blinker::StringView("charlie", 7),
                   blinker::StringView("12345678", 8))
                   .ok());
    blinker::ByteView values;
    CHECK_TRUE(initialValues.finish(values).ok());
    bool changed = false;
    CHECK_TRUE(store.applyKeyed(values, blinker::AccessWrite, changed).ok());
    CHECK_TRUE(changed);

    FakeTransport transport;
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(transport).ok());
    uint8_t transmitBuffer[96] = {};
    blinker::DeviceRuntimeConfig config;
    config.protocolFeatures =
        blinker::bbp2::FeatureReliableDelivery |
        blinker::bbp2::FeatureStateRevision;
    config.maxFrameSize = sizeof(transmitBuffer);
    config.maxReassemblySize = sizeof(transmitBuffer);
    config.initialStateRevision = 20;
    config.reliableReceiveWindow = 2;
    blinker::DeviceRuntime runtime(
        hub,
        registry,
        blinker::MutableByteSpan(transmitBuffer, sizeof(transmitBuffer)),
        config);
    runtime.setStatePageEncoder(&encodeStoredStatePage, &store);
    runtime.setStateApplyTransactionHandler(&applyStoredState, &store);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000);

    uint8_t bodyBuffer[96] = {};
    uint8_t frameBuffer[128] = {};
    blinker::ByteView body;
    blinker::ByteView frame;
    blinker::bbp2::HelloBody hello;
    hello.role = blinker::bbp2::PeerRole::App;
    hello.versions[0] = blinker::bbp2::kVersion;
    hello.versionCount = 1;
    hello.features = config.protocolFeatures;
    hello.reliableReceiveWindow = 2;
    hello.maxFrameSize = 39;
    hello.maxReassemblySize = sizeof(transmitBuffer);
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   hello,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   90,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 77);

    CHECK_TRUE(encodeTestStateRequest(
                   0,
                   0,
                   false,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   100,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 77);
    blinker::bbp2::FrameView response;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::StatePage),
        response.header.kind);
    CHECK_TRUE(transport.lastSentSize <= hello.maxFrameSize);
    blinker::bbp2::StatePageBody page;
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(response.body, page).ok());
    CHECK_EQ(static_cast<uint32_t>(20), page.revision);
    CHECK_EQ(static_cast<uint16_t>(0), page.cursor);
    CHECK_EQ(static_cast<uint16_t>(1), page.nextCursor);
    CHECK_EQ(static_cast<uint16_t>(3), page.totalFields);

    CHECK_TRUE(encodeTestStateRequest(
                   1,
                   20,
                   true,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   101,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 77);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(response.body, page).ok());
    CHECK_EQ(static_cast<uint16_t>(1), page.cursor);
    CHECK_EQ(static_cast<uint16_t>(2), page.nextCursor);
    uint8_t secondPage[40] = {};
    const size_t secondPageSize = response.body.size;
    CHECK_TRUE(secondPageSize <= sizeof(secondPage));
    if (secondPageSize <= sizeof(secondPage)) {
        std::memcpy(secondPage, response.body.data, secondPageSize);
    }

    // A lost StatePage may be recovered by repeating the last continuation.
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   102,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 77);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_EQ(secondPageSize, response.body.size);
    CHECK_TRUE(secondPageSize > sizeof(secondPage) ||
               std::memcmp(secondPage, response.body.data, secondPageSize) == 0);

    CHECK_TRUE(encodeTestStateRequest(
                   3,
                   20,
                   true,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   103,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 77);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    blinker::bbp2::ErrorBody error;
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(response.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::StateConflict),
        error.errorCode);
    CHECK_EQ(static_cast<uint32_t>(20), error.stateRevision);

    blinker::bbp2::KeyedBodyWriter mutation(
        blinker::MutableByteSpan(valuesBuffer, sizeof(valuesBuffer)));
    CHECK_TRUE(mutation.begin(1).ok());
    CHECK_TRUE(mutation.writeText(
                   blinker::StringView("alpha", 5),
                   blinker::StringView("87654321", 8))
                   .ok());
    CHECK_TRUE(mutation.finish(values).ok());
    CHECK_TRUE(store.applyKeyed(values, blinker::AccessWrite, changed).ok());
    CHECK_TRUE(changed);
    uint32_t revision = 0;
    CHECK_TRUE(runtime.advanceStateRevision(revision).ok());
    CHECK_EQ(static_cast<uint32_t>(21), revision);

    CHECK_TRUE(encodeTestStateRequest(
                   2,
                   20,
                   true,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   104,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 77);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(response.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::StateConflict),
        error.errorCode);
    CHECK_EQ(static_cast<uint32_t>(21), error.stateRevision);

    // The partially observed snapshot cannot authorize a write at the new
    // revision; all pages must be collected first.
    blinker::bbp2::StatePatchBody apply;
    apply.mode = blinker::bbp2::StatePatchMode::Apply;
    apply.revision = 21;
    apply.values = values;
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(
                   apply,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Patch,
                   blinker::bbp2::FlagAckRequired,
                   105,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 77);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(response.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::NegotiationRequired),
        error.errorCode);

    uint16_t cursor = 0;
    for (uint16_t sequence = 106; sequence <= 108; ++sequence) {
        CHECK_TRUE(encodeTestStateRequest(
                       cursor,
                       21,
                       cursor != 0U,
                       blinker::MutableByteSpan(
                           bodyBuffer,
                           sizeof(bodyBuffer)),
                       body)
                       .ok());
        CHECK_TRUE(buildTestFrame(
                       blinker::bbp2::MessageKind::StateRequest,
                       blinker::bbp2::FlagNone,
                       sequence,
                       body,
                       blinker::MutableByteSpan(
                           frameBuffer,
                           sizeof(frameBuffer)),
                       frame)
                       .ok());
        transport.inject(frame, 0, true, 77);
        CHECK_TRUE(blinker::bbp2::parseFrame(
                       blinker::ByteView(
                           transport.lastSent,
                           transport.lastSentSize),
                       response)
                       .ok());
        CHECK_EQ(
            static_cast<uint8_t>(blinker::bbp2::MessageKind::StatePage),
            response.header.kind);
        CHECK_TRUE(transport.lastSentSize <= hello.maxFrameSize);
        CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                       response.body,
                       page)
                       .ok());
        CHECK_EQ(static_cast<uint32_t>(21), page.revision);
        CHECK_EQ(cursor, page.cursor);
        CHECK_TRUE(page.nextCursor > cursor);
        cursor = page.nextCursor;
    }
    CHECK_EQ(static_cast<uint16_t>(3), cursor);

    // Completion makes the exact revision token writable. The transaction
    // changes state, so the applying peer receives the next revision in ACK.
    blinker::bbp2::KeyedBodyWriter finalValue(
        blinker::MutableByteSpan(valuesBuffer, sizeof(valuesBuffer)));
    CHECK_TRUE(finalValue.begin(1).ok());
    CHECK_TRUE(finalValue.writeText(
                   blinker::StringView("alpha", 5),
                   blinker::StringView("final", 5))
                   .ok());
    CHECK_TRUE(finalValue.finish(values).ok());
    apply.values = values;
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(
                   apply,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Patch,
                   blinker::bbp2::FlagAckRequired,
                   109,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 77);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Ack),
        response.header.kind);
    blinker::bbp2::AckBody ack;
    CHECK_TRUE(blinker::bbp2::decodeAckBody(response.body, ack).ok());
    CHECK_TRUE(ack.hasStateRevision);
    CHECK_EQ(static_cast<uint32_t>(22), ack.stateRevision);
    CHECK_EQ(static_cast<uint32_t>(22), runtime.stateRevision());

    // Low-level sendPatch is also a fact revision change. It must invalidate
    // the source peer's previous observation just like advanceStateRevision.
    blinker::SendTarget target;
    target.kind = blinker::SendTargetKind::Session;
    target.transportId = 0;
    target.sessionId = 77;
    CHECK_TRUE(runtime.sendPatch(values, target).ok());
    CHECK_EQ(static_cast<uint32_t>(23), runtime.stateRevision());
    apply.revision = 23;
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(
                   apply,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Patch,
                   blinker::bbp2::FlagAckRequired,
                   110,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 77);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Error),
        response.header.kind);
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(response.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::NegotiationRequired),
        error.errorCode);
    runtime.stop();
}

void testCanonicalClientApi() {
    static const blinker::PropertyConstraints temperatureConstraints(
        static_cast<uint8_t>(
            blinker::ConstraintMinimum | blinker::ConstraintMaximum),
        -40.0,
        125.0,
        0.0,
        0U,
        blinker::StringView(),
        nullptr,
        0U);
    static const blinker::PropertyConstraints statusConstraints(
        blinker::ConstraintMaxLength,
        0.0,
        0.0,
        0.0,
        80U,
        blinker::StringView(),
        nullptr,
        0U);
    static const blinker::FieldSpec firstFields[] = {
        blinker::FieldSpec(
            blinker::StringView("lamp", 4),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify)),
        blinker::FieldSpec(
            blinker::StringView("temperature", 11),
            blinker::EndpointKind::Property,
            blinker::ValueType::Float32,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify),
            &temperatureConstraints),
        blinker::FieldSpec(
            blinker::StringView("status", 6),
            blinker::EndpointKind::Property,
            blinker::ValueType::Text,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify),
            &statusConstraints)
    };
    static const blinker::FieldSpec secondFields[] = {
        blinker::FieldSpec(
            blinker::StringView("second-flag", 11),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessWrite |
                blinker::AccessNotify))
    };
    blinker::ClientStorage<3, 256, 96, 48> firstStorage;
    blinker::ClientStorage<1, 128, 48, 16> secondStorage;
    blinker::DeviceRuntimeConfig firstConfig;
    firstConfig.protocolFeatures =
        blinker::bbp2::FeatureManifest |
        blinker::bbp2::FeatureReliableDelivery |
        blinker::bbp2::FeatureStateRevision;
    firstConfig.maxFrameSize = 256;
    firstConfig.maxReassemblySize = 256;
    blinker::DeviceRuntimeConfig secondConfig;
    secondConfig.protocolFeatures = blinker::bbp2::FeatureStateRevision;
    secondConfig.maxFrameSize = 128;
    secondConfig.maxReassemblySize = 128;
    blinker::Client first(firstStorage.buffers(firstFields), firstConfig);
    blinker::Client second(secondStorage.buffers(secondFields), secondConfig);
    FakeTransport firstTransport;
    FakeTransport mirrorTransport;
    FakeTransport untrustedLocalTransport;
    FakeTransport secondTransport;
    untrustedLocalTransport.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary | blinker::TransportFeatureLocal);

    blinker::EndpointHandle lamp = first.endpoints().find("lamp");
    CHECK_TRUE(lamp.valid());
    CHECK_TRUE(lamp.key().size == 4U);
    CHECK_TRUE(std::memcmp(lamp.key().data, "lamp", 4) == 0);
    blinker::EndpointHandle temperature =
        first.endpoints().find("temperature");
    CHECK_TRUE(temperature.valid());
    blinker::EndpointHandle status = first.endpoints().find("status");
    CHECK_TRUE(status.valid());
    CHECK_TRUE(first.endpoints().find("lamp").valid());

    CanonicalCommandCapture capture;
    CanonicalStateWriteCapture stateWriteCapture;
    CHECK_TRUE(lamp.onCommand(&captureCanonicalCommand, &capture).ok());
    CHECK_TRUE(first.setStateWriteHandler(
                   &acceptCanonicalStateWrite,
                   &stateWriteCapture)
                   .ok());
    CHECK_TRUE(first.addTransport(firstTransport).ok());
    CHECK_TRUE(first.addTransport(mirrorTransport).ok());
    CHECK_TRUE(first.addTransport(untrustedLocalTransport).ok());
    CHECK_TRUE(second.addTransport(secondTransport).ok());
    CHECK_TRUE(first.begin().ok());
    CHECK_TRUE(second.begin().ok());
    CHECK_TRUE(first.endpointRegistry().sealed());
    CHECK_TRUE(second.endpointRegistry().sealed());
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        first.removeTransport(firstTransport).code());
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        first.setReliableOutbox(nullptr).code());
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        first.setStateWriteHandler(nullptr, nullptr).code());
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        first.endpoints().find("late-endpoint").error());

    const uint32_t sentBeforeSet = firstTransport.sendCount;
    CHECK_TRUE(lamp.set(true).ok());
    // State is retained immediately, but no report is exposed before a peer
    // completes HELLO/auth negotiation.
    CHECK_EQ(sentBeforeSet, firstTransport.sendCount);
    blinker::bbp2::FrameView outbound;
    blinker::bbp2::StatePatchBody statePatch;
    blinker::EndpointValueView currentLamp;
    bool lampValue = false;
    CHECK_TRUE(lamp.current(currentLamp).ok());
    CHECK_TRUE(currentLamp.asBool(lampValue).ok());
    CHECK_TRUE(lampValue);

    const uint32_t sentBeforeNoOp = firstTransport.sendCount;
    CHECK_TRUE(lamp.set(true).ok());
    CHECK_EQ(sentBeforeNoOp, firstTransport.sendCount);
    CHECK_EQ(static_cast<uint32_t>(1), first.runtime().stateRevision());

    const uint32_t sentBeforeTransaction = firstTransport.sendCount;
    blinker::StateUpdate update(first);
    CHECK_TRUE(update.begin(2).ok());
    CHECK_TRUE(update.set(lamp, false).ok());
    CHECK_TRUE(update.set(temperature, 23.5f).ok());
    CHECK_TRUE(update.commit().ok());
    CHECK_EQ(sentBeforeTransaction, firstTransport.sendCount);
    CHECK_EQ(static_cast<uint32_t>(2), first.runtime().stateRevision());
    CHECK_TRUE(lamp.current(currentLamp).ok());
    CHECK_TRUE(currentLamp.asBool(lampValue).ok());
    CHECK_TRUE(!lampValue);
    blinker::EndpointValueView currentTemperature;
    double temperatureValue = 0.0;
    CHECK_TRUE(temperature.current(currentTemperature).ok());
    CHECK_TRUE(currentTemperature.asFloat(temperatureValue).ok());
    CHECK_TRUE(temperatureValue == 23.5);

    const uint32_t sentBeforeBusyCheck = firstTransport.sendCount;
    blinker::StateUpdate busyUpdate(first);
    CHECK_TRUE(busyUpdate.begin(1).ok());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, lamp.set(true).code());
    blinker::StateUpdate secondBusyUpdate(first);
    CHECK_EQ(
        blinker::ErrorCode::WouldBlock,
        secondBusyUpdate.begin(1).code());
    CHECK_TRUE(busyUpdate.set(lamp, false).ok());
    CHECK_TRUE(busyUpdate.commit().ok());
    CHECK_EQ(sentBeforeBusyCheck, firstTransport.sendCount);

    char oversizedStatus[60];
    std::memset(oversizedStatus, 'x', sizeof(oversizedStatus));
    const uint32_t sentBeforeRejectedTransaction = firstTransport.sendCount;
    blinker::StateUpdate rejectedUpdate(first);
    CHECK_TRUE(rejectedUpdate.begin(2).ok());
    CHECK_TRUE(rejectedUpdate.set(lamp, true).ok());
    CHECK_TRUE(rejectedUpdate.setText(
                   status,
                   blinker::StringView(
                       oversizedStatus,
                       sizeof(oversizedStatus)))
                   .ok());
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        rejectedUpdate.commit().code());
    CHECK_EQ(sentBeforeRejectedTransaction, firstTransport.sendCount);
    CHECK_EQ(static_cast<uint32_t>(2), first.runtime().stateRevision());
    CHECK_TRUE(lamp.current(currentLamp).ok());
    CHECK_TRUE(currentLamp.asBool(lampValue).ok());
    CHECK_TRUE(!lampValue);
    blinker::EndpointValueView missingStatus;
    CHECK_EQ(blinker::ErrorCode::NotFound, status.current(missingStatus).code());

    firstTransport.sendResult =
        blinker::Result::failure(blinker::ErrorCode::WouldBlock);
    mirrorTransport.sendResult =
        blinker::Result::failure(blinker::ErrorCode::WouldBlock);
    // With no negotiated peers, transport backpressure is irrelevant to the
    // authoritative local state commit.
    CHECK_TRUE(lamp.set(true).ok());
    CHECK_EQ(static_cast<uint32_t>(3), first.runtime().stateRevision());
    CHECK_TRUE(lamp.current(currentLamp).ok());
    CHECK_TRUE(currentLamp.asBool(lampValue).ok());
    CHECK_TRUE(lampValue);
    firstTransport.sendResult = blinker::Result::success();
    mirrorTransport.sendResult = blinker::Result::success();

    CHECK_EQ(
        blinker::ErrorCode::ValueOutOfRange,
        temperature.set(200.0f).code());
    CHECK_EQ(static_cast<uint32_t>(3), first.runtime().stateRevision());

    uint8_t bodyBuffer[96] = {};
    blinker::bbp2::HelloBody hello;
    hello.versions[0] = 2;
    hello.versionCount = 1;
    hello.features = firstConfig.protocolFeatures;
    hello.reliableReceiveWindow = 2;
    hello.maxFrameSize = 256;
    hello.maxReassemblySize = 256;
    blinker::ByteView body;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   hello,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    uint8_t frameBuffer[160] = {};
    blinker::ByteView frame;
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   30,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    firstTransport.inject(frame, 0, true, 41);
    mirrorTransport.inject(frame, 1, true, 52);
    // HELLO alone is not authorization for a local link. This session remains
    // unencrypted and must never receive state fan-out.
    untrustedLocalTransport.inject(frame, 2, true, 63, false, false);

    CHECK_TRUE(encodeTestStateRequest(
                   0,
                   0,
                   false,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   31,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    firstTransport.inject(frame, 0, true, 41);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       firstTransport.lastSent,
                       firstTransport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::StatePage),
        outbound.header.kind);
    blinker::bbp2::StatePageBody statePage;
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   outbound.body,
                   statePage)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(3), statePage.revision);
    CHECK_EQ(static_cast<uint16_t>(0), statePage.cursor);
    CHECK_EQ(static_cast<uint16_t>(3), statePage.nextCursor);
    CHECK_EQ(static_cast<uint16_t>(3), statePage.totalFields);
    uint8_t firstSnapshot[96] = {};
    const size_t firstSnapshotSize = statePage.values.size;
    CHECK_TRUE(firstSnapshotSize <= sizeof(firstSnapshot));
    if (firstSnapshotSize <= sizeof(firstSnapshot)) {
        std::memcpy(firstSnapshot, statePage.values.data, firstSnapshotSize);
    }
    blinker::cbor::Reader snapshotReader(statePage.values);
    size_t snapshotFields = 0U;
    CHECK_TRUE(snapshotReader.readMapSize(snapshotFields).ok());
    CHECK_EQ(static_cast<size_t>(2), snapshotFields);
    blinker::StringView snapshotKey;
    CHECK_TRUE(snapshotReader.readText(snapshotKey).ok());
    CHECK_TRUE(snapshotKey.size == 4U &&
               std::memcmp(snapshotKey.data, "lamp", 4) == 0);
    bool snapshotLamp = false;
    CHECK_TRUE(snapshotReader.readBool(snapshotLamp).ok());
    CHECK_TRUE(snapshotLamp);
    CHECK_TRUE(snapshotReader.readText(snapshotKey).ok());
    CHECK_TRUE(snapshotKey.size == 11U &&
               std::memcmp(snapshotKey.data, "temperature", 11) == 0);
    double snapshotTemperature = 0.0;
    CHECK_TRUE(snapshotReader.readFloat(snapshotTemperature).ok());
    CHECK_TRUE(snapshotTemperature == 23.5);
    CHECK_TRUE(snapshotReader.finished());

    mirrorTransport.inject(frame, 1, true, 52);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       mirrorTransport.lastSent,
                       mirrorTransport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   outbound.body,
                   statePage)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(3), statePage.revision);
    CHECK_EQ(firstSnapshotSize, statePage.values.size);
    CHECK_TRUE(firstSnapshotSize > sizeof(firstSnapshot) ||
               std::memcmp(
                   firstSnapshot,
                   statePage.values.data,
                   firstSnapshotSize) == 0);

    uint8_t applyValuesBuffer[64] = {};
    blinker::bbp2::KeyedBodyWriter applyValuesWriter(
        blinker::MutableByteSpan(
            applyValuesBuffer,
            sizeof(applyValuesBuffer)));
    CHECK_TRUE(applyValuesWriter.begin(1).ok());
    CHECK_TRUE(applyValuesWriter.writeBool(
                   blinker::StringView("lamp", 4), false)
                   .ok());
    blinker::ByteView applyValues;
    CHECK_TRUE(applyValuesWriter.finish(applyValues).ok());
    blinker::bbp2::StatePatchBody applyPatch;
    applyPatch.mode = blinker::bbp2::StatePatchMode::Apply;
    applyPatch.revision = 3;
    applyPatch.values = applyValues;
    uint8_t applyEnvelopeBuffer[96] = {};
    blinker::ByteView applyEnvelope;
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(
                   applyPatch,
                   blinker::MutableByteSpan(
                       applyEnvelopeBuffer,
                       sizeof(applyEnvelopeBuffer)),
                   applyEnvelope)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Patch,
                   blinker::bbp2::FlagAckRequired,
                   32,
                   applyEnvelope,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    const uint32_t sourceSendsBeforeApply = firstTransport.sendCount;
    const uint32_t mirrorSendsBeforeApply = mirrorTransport.sendCount;
    const uint32_t untrustedSendsBeforeApply =
        untrustedLocalTransport.sendCount;
    firstTransport.inject(frame, 0, true, 41);
    CHECK_EQ(sourceSendsBeforeApply + 1U, firstTransport.sendCount);
    CHECK_EQ(mirrorSendsBeforeApply + 1U, mirrorTransport.sendCount);
    CHECK_EQ(
        untrustedSendsBeforeApply,
        untrustedLocalTransport.sendCount);
    CHECK_EQ(static_cast<uint32_t>(4), first.runtime().stateRevision());
    CHECK_EQ(static_cast<uint32_t>(1), stateWriteCapture.calls);
    CHECK_TRUE(!stateWriteCapture.idMode);
    CHECK_EQ(static_cast<uint32_t>(41), stateWriteCapture.sessionId);
    CHECK_TRUE(lamp.current(currentLamp).ok());
    CHECK_TRUE(currentLamp.asBool(lampValue).ok());
    CHECK_TRUE(!lampValue);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       firstTransport.lastSent,
                       firstTransport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Ack),
        outbound.header.kind);
    blinker::bbp2::AckBody applyAck;
    CHECK_TRUE(blinker::bbp2::decodeAckBody(outbound.body, applyAck).ok());
    CHECK_TRUE(applyAck.hasStateRevision);
    CHECK_EQ(static_cast<uint32_t>(4), applyAck.stateRevision);

    // Apply is acknowledged only to its source. Other negotiated peers get a
    // canonical keyed full-state report, never the source's ID-mode body.
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       mirrorTransport.lastSent,
                       mirrorTransport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Patch),
        outbound.header.kind);
    CHECK_TRUE(blinker::bbp2::decodeStatePatchBody(
                   outbound.body,
                   statePatch)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(4), statePatch.revision);
    CHECK_TRUE(
        statePatch.mode == blinker::bbp2::StatePatchMode::Report);
    blinker::cbor::Reader fanOutReader(statePatch.values);
    size_t fanOutFields = 0U;
    CHECK_TRUE(fanOutReader.readMapSize(fanOutFields).ok());
    CHECK_EQ(static_cast<size_t>(2), fanOutFields);

    blinker::bbp2::KeyedBodyWriter rejectedApplyValuesWriter(
        blinker::MutableByteSpan(
            applyValuesBuffer,
            sizeof(applyValuesBuffer)));
    CHECK_TRUE(rejectedApplyValuesWriter.begin(1).ok());
    CHECK_TRUE(rejectedApplyValuesWriter.writeBool(
                   blinker::StringView("lamp", 4), true)
                   .ok());
    CHECK_TRUE(rejectedApplyValuesWriter.finish(applyValues).ok());
    applyPatch.revision = 4;
    applyPatch.values = applyValues;
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(
                   applyPatch,
                   blinker::MutableByteSpan(
                       applyEnvelopeBuffer,
                       sizeof(applyEnvelopeBuffer)),
                   applyEnvelope)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Patch,
                   blinker::bbp2::FlagAckRequired,
                   33,
                   applyEnvelope,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    stateWriteCapture.accept = false;
    firstTransport.inject(frame, 0, true, 41);
    stateWriteCapture.accept = true;
    CHECK_EQ(static_cast<uint32_t>(2), stateWriteCapture.calls);
    CHECK_EQ(static_cast<uint32_t>(4), first.runtime().stateRevision());
    CHECK_TRUE(lamp.current(currentLamp).ok());
    CHECK_TRUE(currentLamp.asBool(lampValue).ok());
    CHECK_TRUE(!lampValue);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       firstTransport.lastSent,
                       firstTransport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Error),
        outbound.header.kind);

    CHECK_TRUE(encodeTestStateRequest(
                   0,
                   0,
                   false,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   34,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    mirrorTransport.inject(frame, 1, true, 52);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       mirrorTransport.lastSent,
                       mirrorTransport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   outbound.body,
                   statePage)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(4), statePage.revision);
    snapshotReader = blinker::cbor::Reader(statePage.values);
    CHECK_TRUE(snapshotReader.readMapSize(snapshotFields).ok());
    CHECK_TRUE(snapshotReader.readText(snapshotKey).ok());
    CHECK_TRUE(snapshotReader.readBool(snapshotLamp).ok());
    CHECK_TRUE(!snapshotLamp);

    blinker::bbp2::KeyedBodyWriter command(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(command.begin(1).ok());
    CHECK_TRUE(command.writeBool(blinker::StringView("lamp", 4), false).ok());
    CHECK_TRUE(command.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagAckRequired,
                   35,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    firstTransport.inject(frame, 0, true, 41);
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);
    CHECK_TRUE(!capture.value);
    CHECK_EQ(static_cast<uint32_t>(41), capture.sessionId);

    first.end();
    second.end();
}

void testClientConnectionStatus() {
    static const blinker::FieldSpec fields[] = {
        blinker::FieldSpec(
            blinker::StringView("ready", 5U),
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify))
    };
    blinker::ClientStorage<1, 96, 48, 16> storage;
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = 96U;
    config.maxReassemblySize = 96U;
    blinker::Client client(storage.buffers(fields), config);
    FakeTransport local;
    FakeTransport cloud;
    local.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary | blinker::TransportFeatureLocal);
    cloud.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary | blinker::TransportFeatureCloud |
        blinker::TransportFeatureAuthenticated);

    blinker::ClientStatus status = client.status();
    CHECK_TRUE(!status.started);
    CHECK_EQ(static_cast<size_t>(0U), status.transportCount);
    CHECK_TRUE(!status.localReady && !status.cloudReady);

    CHECK_TRUE(client.addTransport(local).ok());
    CHECK_TRUE(client.addTransport(cloud).ok());
    status = client.status();
    CHECK_EQ(static_cast<size_t>(2U), status.transportCount);
    CHECK_EQ(static_cast<size_t>(0U), status.onlineCount);

    CHECK_TRUE(client.begin().ok());
    status = client.status();
    CHECK_TRUE(status.started);
    CHECK_EQ(static_cast<size_t>(2U), status.onlineCount);
    CHECK_TRUE(status.localReady);
    CHECK_TRUE(status.cloudReady);

    blinker::TransportState state = blinker::TransportState::Stopped;
    blinker::TransportCapabilities capabilities;
    CHECK_TRUE(client.transportStatus(1U, state, capabilities).ok());
    CHECK_EQ(blinker::TransportState::Online, state);
    CHECK_TRUE(
        (capabilities.features & blinker::TransportFeatureCloud) != 0U);
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        client.transportStatus(2U, state, capabilities).code());

    cloud.currentState = blinker::TransportState::Backoff;
    status = client.status();
    CHECK_EQ(static_cast<size_t>(1U), status.onlineCount);
    CHECK_EQ(static_cast<size_t>(1U), status.backoffCount);
    CHECK_TRUE(status.localReady);
    CHECK_TRUE(!status.cloudReady);

    client.end();
    status = client.status();
    CHECK_TRUE(!status.started);
    CHECK_EQ(static_cast<size_t>(0U), status.onlineCount);
    CHECK_TRUE(!status.localReady && !status.cloudReady);
}

void testPreparedOfflineState() {
    static const blinker::FieldSpec fields[] = {
        blinker::FieldSpec(
            "power",
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify)),
        blinker::FieldSpec(
            "temperature",
            blinker::EndpointKind::Property,
            blinker::ValueType::Float32,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify)),
        blinker::FieldSpec(
            "alarm",
            blinker::EndpointKind::Event,
            blinker::ValueType::Null,
            blinker::AccessEvent)
    };
    blinker::ClientStorage<3, 192, 96, 32> storage;
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = 192U;
    config.maxReassemblySize = 192U;
    blinker::Client client(storage.buffers(fields), config);

    blinker::EndpointHandle power = client.endpoints().find("power");
    blinker::EndpointHandle temperature =
        client.endpoints().find("temperature");
    blinker::EndpointHandle alarm = client.endpoints().find("alarm");
    CHECK_TRUE(power.valid() && temperature.valid() && alarm.valid());
    CHECK_EQ(blinker::ErrorCode::NotConfigured, power.set(true).code());

    CHECK_TRUE(client.prepare().ok());
    CHECK_TRUE(client.prepare().ok());
    CHECK_TRUE(client.status().prepared);
    CHECK_TRUE(!client.status().started);
    CHECK_TRUE(client.endpointRegistry().sealed());
    CHECK_TRUE(power.set(true).ok());
    CHECK_EQ(static_cast<uint32_t>(1U), client.runtime().stateRevision());

    blinker::StateUpdate update(client);
    CHECK_TRUE(update.begin(2U).ok());
    CHECK_TRUE(update.set(power, false).ok());
    CHECK_TRUE(update.set(temperature, 23.5f).ok());
    CHECK_TRUE(update.commit().ok());
    CHECK_EQ(static_cast<uint32_t>(2U), client.runtime().stateRevision());

    blinker::EndpointValueView currentPower;
    bool powerValue = true;
    CHECK_TRUE(power.current(currentPower).ok());
    CHECK_TRUE(currentPower.asBool(powerValue).ok());
    CHECK_TRUE(!powerValue);
    blinker::EndpointValueView currentTemperature;
    double temperatureValue = 0.0;
    CHECK_TRUE(temperature.current(currentTemperature).ok());
    CHECK_TRUE(currentTemperature.asFloat(temperatureValue).ok());
    CHECK_TRUE(temperatureValue == 23.5);
    CHECK_EQ(blinker::ErrorCode::NotConnected, alarm.emitNull().code());

    FakeTransport transport;
    CHECK_TRUE(client.addTransport(transport).ok());
    CHECK_TRUE(client.begin().ok());
    CHECK_TRUE(client.status().prepared && client.status().started);
    CHECK_TRUE(alarm.emitNull().ok());
    client.end();
    CHECK_TRUE(client.status().prepared);
    CHECK_TRUE(!client.status().started);
    CHECK_TRUE(power.set(true).ok());
    CHECK_EQ(static_cast<uint32_t>(3U), client.runtime().stateRevision());
}

void testOutboundStateScheduling() {
    static const blinker::FieldSpec fields[] = {
        blinker::FieldSpec(
            "power",
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify)),
        blinker::FieldSpec(
            "ready",
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify))
    };
    blinker::ClientStorage<2, 192, 96, 16> storage;
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = 192U;
    config.maxReassemblySize = 192U;
    blinker::Client client(storage.buffers(fields), config);
    FakeClock clock;
    FakeTransport transport;

    CHECK_TRUE(client.setMonotonicClock(&clock).ok());
    CHECK_TRUE(client.addTransport(transport).ok());
    CHECK_TRUE(client.begin().ok());
    blinker::EndpointHandle power = client.endpoints().find("power");
    blinker::EndpointHandle ready = client.endpoints().find("ready");
    CHECK_TRUE(power.valid() && ready.valid());

    uint8_t helloBodyBuffer[96] = {};
    uint8_t helloFrameBuffer[192] = {};
    blinker::ByteView helloBody;
    blinker::ByteView helloFrame;
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1U;
    peerHello.features = blinker::bbp2::FeatureStateRevision;
    peerHello.maxFrameSize = sizeof(helloFrameBuffer);
    peerHello.maxReassemblySize = sizeof(helloFrameBuffer);
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(
                       helloBodyBuffer,
                       sizeof(helloBodyBuffer)),
                   helloBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   1U,
                   helloBody,
                   blinker::MutableByteSpan(
                       helloFrameBuffer,
                       sizeof(helloFrameBuffer)),
                   helloFrame)
                   .ok());
    transport.inject(helloFrame, 0U, true, 7U);
    const uint32_t sendsBeforeState = transport.sendCount;

    // Multiple writes to one property retain only the latest value and do
    // not allocate one state revision per call.
    CHECK_TRUE(power.set(true).ok());
    CHECK_TRUE(power.set(false).ok());
    CHECK_EQ(static_cast<uint32_t>(0U), transport.sendCount);
    CHECK_EQ(static_cast<uint32_t>(0U), client.runtime().stateRevision());
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        client.outboundCounters().stateAccepted);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        client.outboundCounters().stateCoalesced);

    clock.advance(249U);
    client.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(0U), client.runtime().stateRevision());
    clock.advance(1U);
    client.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(1U), client.runtime().stateRevision());
    CHECK_EQ(sendsBeforeState + 1U, transport.sendCount);
    blinker::bbp2::FrameView outboundState;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outboundState)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Patch),
        outboundState.header.kind);

    // Multiple dirty fields share one revision while observing both the
    // coalescing window and minimum interval from the preceding state frame.
    CHECK_TRUE(power.set(true).ok());
    CHECK_TRUE(ready.set(true).ok());
    clock.advance(249U);
    client.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(1U), client.runtime().stateRevision());
    clock.advance(1U);
    client.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(2U), client.runtime().stateRevision());
    CHECK_EQ(sendsBeforeState + 2U, transport.sendCount);
    CHECK_EQ(
        static_cast<uint32_t>(2U),
        client.outboundCounters().stateFlushes);

    transport.sendResult =
        blinker::Result::failure(blinker::ErrorCode::WouldBlock);
    CHECK_TRUE(power.set(false).ok());
    clock.advance(250U);
    client.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(3U), client.runtime().stateRevision());
    CHECK_EQ(sendsBeforeState + 3U, transport.sendCount);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        client.outboundCounters().stateTransportErrors);
    transport.sendResult = blinker::Result::success();

    // A remote snapshot request is an observation boundary. It must settle
    // pending state before exposing a revision and values to the peer.
    CHECK_TRUE(power.set(true).ok());
    CHECK_TRUE(encodeTestStateRequest(
                   0U,
                   0U,
                   false,
                   blinker::MutableByteSpan(
                       helloBodyBuffer,
                       sizeof(helloBodyBuffer)),
                   helloBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagAckRequired,
                   2U,
                   helloBody,
                   blinker::MutableByteSpan(
                       helloFrameBuffer,
                       sizeof(helloFrameBuffer)),
                   helloFrame)
                   .ok());
    transport.inject(helloFrame, 0U, true, 7U);
    CHECK_EQ(static_cast<uint32_t>(3U), client.runtime().stateRevision());
    const uint32_t sendsBeforeSettlement = transport.sendCount;
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   3U,
                   helloBody,
                   blinker::MutableByteSpan(
                       helloFrameBuffer,
                       sizeof(helloFrameBuffer)),
                   helloFrame)
                   .ok());
    transport.inject(helloFrame, 0U, true, 7U);
    CHECK_EQ(static_cast<uint32_t>(4U), client.runtime().stateRevision());
    // The requesting peer receives only the settled StatePage. Sending the
    // pending Patch to it first changes the broker's expected request
    // sequence and makes the following StatePage invalid.
    CHECK_EQ(sendsBeforeSettlement + 1U, transport.sendCount);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outboundState)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::StatePage),
        outboundState.header.kind);
    blinker::bbp2::StatePageBody statePage;
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   outboundState.body,
                   statePage)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(4U), statePage.revision);

    // A lifecycle stop is also a settlement boundary: pending state is
    // flushed immediately instead of being silently discarded.
    CHECK_TRUE(ready.set(false).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Patch,
                   blinker::bbp2::FlagAckRequired,
                   4U,
                   helloBody,
                   blinker::MutableByteSpan(
                       helloFrameBuffer,
                       sizeof(helloFrameBuffer)),
                   helloFrame)
                   .ok());
    transport.inject(helloFrame, 0U, true, 7U);
    CHECK_EQ(static_cast<uint32_t>(4U), client.runtime().stateRevision());
    const uint32_t sendsBeforeStop = transport.sendCount;
    client.end();
    CHECK_EQ(static_cast<uint32_t>(5U), client.runtime().stateRevision());
    CHECK_EQ(sendsBeforeStop + 1U, transport.sendCount);

    uint8_t dirty[1] = {};
    uint8_t selected[1] = {};
    blinker::OutboundScheduler scheduler(
        blinker::MutableByteSpan(dirty, sizeof(dirty)),
        blinker::MutableByteSpan(selected, sizeof(selected)));
    CHECK_TRUE(scheduler.prepare(3U).ok());
    const uint32_t nearWrap = UINT32_MAX - 100U;
    CHECK_TRUE(scheduler.markStateDirty(0U, nearWrap).ok());
    CHECK_TRUE(!scheduler.stateFlushDue(nearWrap + 249U));
    CHECK_TRUE(scheduler.stateFlushDue(nearWrap + 250U));

    CHECK_TRUE(scheduler.markStateDirty(1U, nearWrap).ok());
    CHECK_EQ(static_cast<size_t>(1U), scheduler.selectStateFields(1U));
    CHECK_TRUE(scheduler.commitStateSelection(
        nearWrap + 250U,
        blinker::Result::failure(blinker::ErrorCode::NotConnected)).ok());
    CHECK_EQ(static_cast<size_t>(1U), scheduler.pendingStateCount());
    CHECK_TRUE(!scheduler.stateDirty(0U));
    CHECK_TRUE(scheduler.stateDirty(1U));
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        scheduler.counters().stateTransportErrors);

    for (size_t index = 0U; index < 8U; ++index) {
        CHECK_TRUE(scheduler.admitEvent(nearWrap).ok());
    }
    CHECK_EQ(
        blinker::ErrorCode::WouldBlock,
        scheduler.admitEvent(nearWrap).code());
    CHECK_EQ(
        blinker::ErrorCode::WouldBlock,
        scheduler.admitEvent(nearWrap + 199U).code());
    CHECK_TRUE(scheduler.admitEvent(nearWrap + 200U).ok());
    scheduler.recordEventDispatch(blinker::Result::success());
    scheduler.recordEventDispatch(
        blinker::Result::failure(blinker::ErrorCode::NotConnected));
    CHECK_EQ(
        static_cast<uint32_t>(9U),
        scheduler.counters().eventAccepted);
    CHECK_EQ(
        static_cast<uint32_t>(2U),
        scheduler.counters().eventThrottled);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        scheduler.counters().eventDispatched);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        scheduler.counters().eventTransportErrors);

    const uint32_t remoteLimitAt = nearWrap + 300U;
    scheduler.applyRemoteBackoff(remoteLimitAt, 1000U);
    CHECK_TRUE(!scheduler.stateFlushDue(remoteLimitAt + 999U, true));
    CHECK_EQ(
        blinker::ErrorCode::WouldBlock,
        scheduler.admitEvent(remoteLimitAt + 999U).code());
    CHECK_TRUE(scheduler.stateFlushDue(remoteLimitAt + 1000U, true));
    CHECK_TRUE(scheduler.admitEvent(remoteLimitAt + 1000U).ok());

    static const blinker::FieldSpec eventFields[] = {
        blinker::FieldSpec(
            "alarm",
            blinker::EndpointKind::Event,
            blinker::ValueType::Null,
            blinker::AccessEvent)
    };
    blinker::ClientStorage<1, 192, 64, 8> eventStorage;
    blinker::Client eventClient(eventStorage.buffers(eventFields), config);
    FakeClock eventClock;
    FakeTransport eventTransport;
    CHECK_TRUE(eventClient.setMonotonicClock(&eventClock).ok());
    CHECK_TRUE(eventClient.addTransport(eventTransport).ok());
    CHECK_TRUE(eventClient.begin().ok());
    blinker::EndpointHandle alarm = eventClient.endpoints().find("alarm");
    CHECK_TRUE(alarm.valid());
    for (size_t index = 0U; index < 8U; ++index) {
        CHECK_TRUE(alarm.emitNull().ok());
    }
    CHECK_EQ(
        blinker::ErrorCode::WouldBlock,
        alarm.emitNull().code());
    eventClock.advance(200U);
    CHECK_TRUE(alarm.emitNull().ok());
    CHECK_EQ(
        static_cast<uint32_t>(9U),
        eventClient.outboundCounters().eventDispatched);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        eventClient.outboundCounters().eventThrottled);
    eventClient.end();

    static const blinker::FieldSpec splitFields[] = {
        blinker::FieldSpec(
            "a",
            blinker::EndpointKind::Property,
            blinker::ValueType::Text,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify)),
        blinker::FieldSpec(
            "b",
            blinker::EndpointKind::Property,
            blinker::ValueType::Text,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify))
    };
    blinker::ClientStorage<2, 192, 64, 64> splitStorage;
    blinker::Client splitClient(splitStorage.buffers(splitFields), config);
    FakeClock splitClock;
    FakeTransport splitTransport;
    CHECK_TRUE(splitClient.setMonotonicClock(&splitClock).ok());
    CHECK_TRUE(splitClient.addTransport(splitTransport).ok());
    CHECK_TRUE(splitClient.begin().ok());
    const blinker::StringView longText(
        "123456789012345678901234567890",
        30U);
    CHECK_TRUE(splitClient.endpoints().find("a").setText(longText).ok());
    CHECK_TRUE(splitClient.endpoints().find("b").setText(longText).ok());
    splitClock.advance(250U);
    splitClient.poll(1000U);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        splitClient.runtime().stateRevision());
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        splitClient.outboundCounters().stateFlushes);
    splitClock.advance(250U);
    splitClient.poll(1000U);
    CHECK_EQ(
        static_cast<uint32_t>(2U),
        splitClient.runtime().stateRevision());
    CHECK_EQ(
        static_cast<uint32_t>(2U),
        splitClient.outboundCounters().stateFlushes);
    splitClient.end();
}

void testPublicDeviceFacade() {
    static const blinker::FieldSpec fields[] = {
        blinker::FieldSpec(
            "ready",
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify))
    };

    static_assert(
        blinker::ActiveResourceProfile::fieldCapacity ==
            BLINKER_DEVICE_MAX_FIELDS,
        "active field capacity does not match its build-wide macro");
    static_assert(
        blinker::ActiveResourceProfile::frameSize ==
            BLINKER_DEVICE_FRAME_SIZE,
        "active frame capacity does not match its build-wide macro");
    static_assert(
        blinker::ActiveResourceProfile::patchSize ==
            BLINKER_DEVICE_PATCH_SIZE,
        "active patch capacity does not match its build-wide macro");
    static_assert(
        blinker::ActiveResourceProfile::stateArenaSize ==
            BLINKER_DEVICE_STATE_ARENA_SIZE,
        "active state capacity does not match its build-wide macro");

    blinker::Device device(fields);
    FakeTransport transport;
    transport.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary | blinker::TransportFeatureLocal);
    CHECK_TRUE(device.addTransport(transport).ok());
    CHECK_TRUE(device.begin().ok());

    blinker::EndpointHandle ready = device.endpoints().find("ready");
    CHECK_TRUE(ready.valid());
    CHECK_TRUE(ready.set(true).ok());
    blinker::EndpointValueView current;
    bool decoded = false;
    CHECK_TRUE(ready.current(current).ok());
    CHECK_TRUE(current.asBool(decoded).ok());
    CHECK_TRUE(decoded);

    device.run();
    CHECK_EQ(static_cast<size_t>(1U), device.status().transportCount);
    CHECK_EQ(static_cast<uint32_t>(1U), transport.pollCount);
    CHECK_EQ(static_cast<uint32_t>(1000U), transport.lastBudget);
    device.end();
    CHECK_TRUE(!device.status().started);
}

void testUnifiedProductFacade() {
    static const blinker::FieldSpec fields[] = {
        blinker::FieldSpec(
            "ready",
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify))
    };

    blinker::Device device(fields);
    FakeTransport transport;
    transport.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary |
        blinker::TransportFeatureLocal |
        blinker::TransportFeatureCloud);
    FakeProductLifecycle lifecycle(transport);
    blinker::Product product(device, lifecycle);

    static_assert(
        !std::is_copy_constructible<blinker::Product>::value,
        "Product must keep one Device/lifecycle owner");
    lifecycle.startResult_ = blinker::Result::failure(
        blinker::ErrorCode::NotConnected);
    CHECK_EQ(blinker::ErrorCode::NotConnected, product.begin().code());
    CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.attachCalls);
    CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.startCalls);
    CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.stopCalls);
    CHECK_TRUE(!product.status().started);
    CHECK_EQ(blinker::ErrorCode::NotConnected, product.status().lastError);

    lifecycle.startResult_ = blinker::Result::success();
    CHECK_TRUE(product.begin().ok());
    CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.attachCalls);
    CHECK_EQ(static_cast<uint32_t>(2U), lifecycle.startCalls);
    CHECK_EQ(
        blinker::ErrorCode::AlreadyExists,
        product.begin().code());

    const blinker::ProductStatus active = product.status();
    CHECK_TRUE(active.started);
    CHECK_TRUE(active.networkReady);
    CHECK_TRUE(active.localReady);
    CHECK_TRUE(active.cloudReady);
    CHECK_EQ(blinker::ProductLifecycleState::Active, active.state);
    CHECK_TRUE(product.capabilities().supports(
        blinker::ProductCapabilityCloudData));
    CHECK_TRUE(product.capabilities().supports(
        blinker::ProductCapabilityDirectBleData));

    product.run(321U);
    CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.pollCalls);
    CHECK_EQ(static_cast<uint32_t>(321U), lifecycle.lastBudget);
    CHECK_EQ(static_cast<uint32_t>(1U), transport.pollCount);
    CHECK_EQ(static_cast<uint32_t>(321U), transport.lastBudget);
    product.end();
    CHECK_TRUE(!product.status().started);
    const uint32_t stoppedPollCalls = lifecycle.pollCalls;
    product.run(999U);
    CHECK_EQ(stoppedPollCalls, lifecycle.pollCalls);

    CHECK_TRUE(product.begin().ok());
    CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.attachCalls);
    CHECK_EQ(static_cast<uint32_t>(3U), lifecycle.startCalls);
    product.end();

    blinker::Device otherDevice(fields);
    blinker::Product other(otherDevice, lifecycle);
    CHECK_EQ(
        blinker::ErrorCode::AlreadyExists,
        other.begin().code());
    CHECK_EQ(static_cast<uint32_t>(2U), lifecycle.attachCalls);
    CHECK_EQ(static_cast<uint32_t>(3U), lifecycle.startCalls);
}

void testProductAttachesClockBeforeTelemetryPreparation() {
    static const blinker::PropertySpec<uint32_t> uptimeSpec(
        "uptimeSeconds",
        blinker::realtime(1000U));
    blinker::Property<uint32_t> uptime(uptimeSpec);
    const auto schema = blinker::makeDeviceSchema(uptime);
    blinker::Device device(schema);
    FakeTransport transport;
    transport.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary | blinker::TransportFeatureCloud);
    FakeProductLifecycle lifecycle(transport, true);
    blinker::Product product(device, lifecycle);

    CHECK_TRUE(product.begin().ok());
    CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.attachCalls);
    product.end();
}

void testProductAccessReset() {
    static const blinker::FieldSpec fields[] = {
        blinker::FieldSpec(
            "ready",
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify))
    };

    {
        blinker::Device device(fields);
        FakeTransport transport;
        FakeProductLifecycle lifecycle(transport);
        blinker::Product product(device, lifecycle);

        CHECK_TRUE(product.begin().ok());
        CHECK_EQ(
            blinker::ErrorCode::UnsupportedFeature,
            product.resetAccess().code());
        CHECK_TRUE(product.status().started);
        CHECK_EQ(static_cast<uint32_t>(0U), lifecycle.stopCalls);
        CHECK_EQ(static_cast<uint32_t>(0U), lifecycle.resetCalls);
        product.end();
    }

    {
        blinker::Device device(fields);
        FakeTransport transport;
        FakeProductLifecycle lifecycle(transport, false, true);
        blinker::Product product(device, lifecycle);

        CHECK_TRUE(product.begin().ok());
        CHECK_TRUE(product.resetAccess().ok());
        CHECK_TRUE(product.status().started);
        CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.stopCalls);
        CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.resetCalls);
        CHECK_EQ(static_cast<uint32_t>(2U), lifecycle.startCalls);

        lifecycle.resetResult_ = blinker::Result::failure(
            blinker::ErrorCode::WouldBlock);
        CHECK_EQ(
            blinker::ErrorCode::WouldBlock,
            product.resetAccess().code());
        CHECK_TRUE(!product.status().started);
        CHECK_EQ(static_cast<uint32_t>(2U), lifecycle.stopCalls);
        CHECK_EQ(static_cast<uint32_t>(2U), lifecycle.resetCalls);
    }
}

void testProductNetworkReset() {
    static const blinker::FieldSpec fields[] = {
        blinker::FieldSpec(
            "ready",
            blinker::EndpointKind::Property,
            blinker::ValueType::Boolean,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify))
    };

    {
        blinker::Device device(fields);
        FakeTransport transport;
        FakeProductLifecycle lifecycle(transport);
        blinker::Product product(device, lifecycle);

        CHECK_TRUE(product.begin().ok());
        CHECK_EQ(
            blinker::ErrorCode::UnsupportedFeature,
            product.resetNetwork().code());
        CHECK_TRUE(product.status().started);
        CHECK_EQ(static_cast<uint32_t>(0U), lifecycle.stopCalls);
        CHECK_EQ(static_cast<uint32_t>(0U), lifecycle.networkResetCalls);
        product.end();
    }

    {
        blinker::Device device(fields);
        FakeTransport transport;
        FakeProductLifecycle lifecycle(transport, false, false, true);
        blinker::Product product(device, lifecycle);

        CHECK_TRUE(product.begin().ok());
        CHECK_TRUE(product.resetNetwork().ok());
        CHECK_TRUE(product.status().started);
        CHECK_EQ(static_cast<uint32_t>(1U), lifecycle.stopCalls);
        CHECK_EQ(
            static_cast<uint32_t>(1U),
            lifecycle.networkResetCalls);
        CHECK_EQ(static_cast<uint32_t>(2U), lifecycle.startCalls);

        lifecycle.networkResetResult_ = blinker::Result::failure(
            blinker::ErrorCode::WouldBlock);
        CHECK_EQ(
            blinker::ErrorCode::WouldBlock,
            product.resetNetwork().code());
        CHECK_TRUE(!product.status().started);
        CHECK_EQ(static_cast<uint32_t>(2U), lifecycle.stopCalls);
        CHECK_EQ(
            static_cast<uint32_t>(2U),
            lifecycle.networkResetCalls);
    }
}
void testSemanticInteractions() {
    static const blinker::PropertySpec<bool> powerSpec(
        "power",
        blinker::property(blinker::PropertyMode::ReadWrite));
    static const blinker::PropertySpec<float> temperatureSpec(
        "temperature",
        blinker::rangedProperty(
            blinker::PropertyMode::ReadOnly,
            -40.0,
            125.0,
            0.1,
            blinker::StringView("C")));
    static const blinker::ActionSpec<void> restartSpec("restart");
    static const blinker::EventSpec<float> overTemperatureSpec(
        "overTemperature");

    blinker::Property<bool> power(powerSpec);
    blinker::Property<float> temperature(temperatureSpec);
    blinker::Action<void> restart(restartSpec);
    blinker::Event<float> overTemperature(overTemperatureSpec);
    const auto schema = blinker::makeDeviceSchema(
        power,
        temperature,
        restart,
        overTemperature);

    static_assert(
        decltype(schema)::fieldCount == 4U,
        "semantic schema must contain four interactions");
    CHECK_EQ(blinker::EndpointKind::Property, powerSpec.field().kind);
    CHECK_EQ(
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify),
        powerSpec.field().access);
    CHECK_EQ(blinker::EndpointKind::Action, restartSpec.field().kind);
    CHECK_EQ(
        static_cast<uint8_t>(blinker::AccessCommand),
        restartSpec.field().access);
    CHECK_EQ(
        blinker::EndpointKind::Event,
        overTemperatureSpec.field().kind);
    CHECK_EQ(
        static_cast<uint8_t>(blinker::AccessEvent),
        overTemperatureSpec.field().access);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        temperature.onWrite(nullptr).code());
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        overTemperature.emit(80.0f).code());

    semanticPowerTarget = &power;
    semanticPowerCalls = 0U;
    semanticPowerRequested = false;
    semanticRestartCalls = 0U;
    CHECK_TRUE(power.onWrite(&captureSemanticPowerWrite).ok());
    CHECK_TRUE(restart.onInvokeChecked(&captureSemanticRestart).ok());

    blinker::Device device(schema);
    CHECK_EQ(static_cast<size_t>(4U), device.endpointRegistry().size());
    CHECK_TRUE(
        device.endpointRegistry().find("power") == &powerSpec.field());
    CHECK_TRUE(
        device.endpointRegistry().find("restart") == &restartSpec.field());

    FakeTransport transport;
    transport.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary | blinker::TransportFeatureLocal);
    CHECK_TRUE(device.addTransport(transport).ok());
    CHECK_TRUE(device.begin().ok());
    CHECK_TRUE(power.valid());
    CHECK_TRUE(temperature.valid());
    CHECK_TRUE(restart.valid());
    CHECK_TRUE(overTemperature.valid());

    CHECK_TRUE(temperature.report(24.5f).ok());
    float currentTemperature = 0.0f;
    CHECK_TRUE(temperature.current(currentTemperature).ok());
    CHECK_TRUE(currentTemperature == 24.5f);

    uint8_t bodyStorage[128] = {};
    uint8_t frameStorage[192] = {};
    blinker::ByteView body;
    blinker::ByteView frame;
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1U;
    peerHello.features = blinker::bbp2::FeatureStateRevision;
    peerHello.maxFrameSize = sizeof(frameStorage);
    peerHello.maxReassemblySize = sizeof(frameStorage);
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(
                       bodyStorage,
                       sizeof(bodyStorage)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   10U,
                   body,
                   blinker::MutableByteSpan(
                       frameStorage,
                       sizeof(frameStorage)),
                   frame)
                   .ok());
    transport.inject(frame, 0U, true, 41U, true);

    blinker::bbp2::KeyedBodyWriter propertyCommand(
        blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)));
    CHECK_TRUE(propertyCommand.begin(1U).ok());
    CHECK_TRUE(propertyCommand.writeBool("power", true).ok());
    CHECK_TRUE(propertyCommand.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagNone,
                   11U,
                   body,
                   blinker::MutableByteSpan(
                       frameStorage,
                       sizeof(frameStorage)),
                   frame)
                   .ok());
    transport.inject(frame, 0U, true, 41U, true);
    CHECK_EQ(static_cast<uint32_t>(1U), semanticPowerCalls);
    CHECK_TRUE(semanticPowerRequested);
    bool currentPower = false;
    CHECK_TRUE(power.current(currentPower).ok());
    CHECK_TRUE(currentPower);

    blinker::bbp2::KeyedBodyWriter actionCommand(
        blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)));
    CHECK_TRUE(actionCommand.begin(1U).ok());
    CHECK_TRUE(actionCommand.writeNull("restart").ok());
    CHECK_TRUE(actionCommand.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagNone,
                   12U,
                   body,
                   blinker::MutableByteSpan(
                       frameStorage,
                       sizeof(frameStorage)),
                   frame)
                   .ok());
    transport.inject(frame, 0U, true, 41U, true);
    CHECK_EQ(static_cast<uint32_t>(1U), semanticRestartCalls);

    CHECK_TRUE(overTemperature.emit(80.0f).ok());
    blinker::bbp2::FrameView outbound;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Event),
        outbound.header.kind);

    device.end();
    semanticPowerTarget = nullptr;
}

void testCanonicalBlinkerFacade() {
    static const char deviceKey[] =
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8";
    facadePowerCalls = 0U;
    facadePowerRequested = false;
    CHECK_TRUE(facadePower.onWrite(&captureFacadePower).ok());
    CHECK_TRUE(Blinker.begin(
        deviceKey,
        "test-network",
        "test-password",
        facadePower,
        facadeTemperature,
        facadeRestart,
        facadeAlarm));
    CHECK_TRUE(Blinker.status().started);
    CHECK_TRUE(facadeLifecycle.client_ != nullptr);
    const uint32_t resetStopCalls = facadeLifecycle.stopCalls;
    CHECK_TRUE(!Blinker.resetAccess());
    CHECK_EQ(blinker::ErrorCode::UnsupportedFeature, Blinker.lastError());
    CHECK_TRUE(Blinker.status().started);
    CHECK_EQ(resetStopCalls, facadeLifecycle.stopCalls);
    CHECK_TRUE(!Blinker.resetNetwork());
    CHECK_EQ(blinker::ErrorCode::UnsupportedFeature, Blinker.lastError());
    CHECK_TRUE(Blinker.status().started);
    CHECK_EQ(resetStopCalls, facadeLifecycle.stopCalls);

    const uint32_t revision = facadeLifecycle.client_->runtime().stateRevision();
    CHECK_TRUE(Blinker.report(
        facadePower,
        true,
        facadeTemperature,
        24.5f));
    CHECK_EQ(
        revision + 1U,
        facadeLifecycle.client_->runtime().stateRevision());
    bool powerValue = false;
    float temperatureValue = 0.0f;
    CHECK_TRUE(facadePower.current(powerValue).ok());
    CHECK_TRUE(facadeTemperature.current(temperatureValue).ok());
    CHECK_TRUE(powerValue);
    CHECK_TRUE(temperatureValue == 24.5f);

    CHECK_TRUE(!Blinker.report(facadePower, false, facadePower, true));
    CHECK_EQ(blinker::ErrorCode::DuplicateField, Blinker.lastError());
    CHECK_TRUE(std::strlen(Blinker.lastErrorText()) != 0U);

    const uint32_t polls = facadeLifecycle.pollCalls;
    Blinker.run(321U);
    CHECK_EQ(polls + 1U, facadeLifecycle.pollCalls);
    CHECK_EQ(static_cast<uint32_t>(321U), facadeLifecycle.lastBudget);
    CHECK_TRUE(facadeAlarm.emit(7U).ok());
    Blinker.end();
    CHECK_TRUE(!Blinker.status().started);
    const uint32_t offlineRevision =
        facadeLifecycle.client_->runtime().stateRevision();
    CHECK_TRUE(Blinker.report(
        facadePower,
        false,
        facadeTemperature,
        20.0f));
    CHECK_EQ(
        offlineRevision + 1U,
        facadeLifecycle.client_->runtime().stateRevision());

    static const blinker::PropertySpec<bool> otherPowerSpec(
        "other-power",
        blinker::readWrite());
    static const blinker::PropertySpec<float> otherTemperatureSpec(
        "other-temperature",
        blinker::readOnly());
    static const blinker::ActionSpec<void> otherRestartSpec("other-restart");
    static const blinker::EventSpec<uint32_t> otherAlarmSpec("other-alarm");
    blinker::Property<bool> otherPower(otherPowerSpec);
    blinker::Property<float> otherTemperature(otherTemperatureSpec);
    blinker::Action<void> otherRestart(otherRestartSpec);
    blinker::Event<uint32_t> otherAlarm(otherAlarmSpec);
    CHECK_TRUE(!Blinker.begin(
        deviceKey,
        "test-network",
        "test-password",
        otherPower,
        otherTemperature,
        otherRestart,
        otherAlarm));
    CHECK_EQ(blinker::ErrorCode::AlreadyExists, Blinker.lastError());

    CHECK_TRUE(Blinker.begin(
        deviceKey,
        "test-network",
        "test-password",
        facadePower,
        facadeTemperature,
        facadeRestart,
        facadeAlarm));
    Blinker.end();
}

blinker::Result encodeRuntimeTelemetrySample(
    void*,
    blinker::ByteView selected,
    size_t fieldCount,
    blinker::bbp2::IdBodyWriter& writer) {
    if (selected.data == nullptr || selected.size == 0U ||
        (selected.data[0] & 0x01U) == 0U || fieldCount != 1U) {
        return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
    }
    return writer.writeFloat32(1U, 21.5f);
}

blinker::Result encodeEndpointTelemetrySample(
    void*,
    uint16_t endpointId,
    blinker::bbp2::IdBodyWriter& writer) {
    return writer.writeFloat32(endpointId, 21.5f);
}

void negotiateRuntimeTelemetryPeer(
    FakeTransport& transport,
    const blinker::bbp2::HelloBody& deviceHello,
    uint32_t sessionId,
    bool encrypted) {
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1U;
    peerHello.features =
        blinker::bbp2::FeatureManifest |
        blinker::bbp2::FeatureEndpointIds |
        blinker::bbp2::FeatureTelemetry;
    peerHello.maxFrameSize = 256U;
    peerHello.maxReassemblySize = 256U;
    peerHello.manifestRevision = deviceHello.manifestRevision;
    peerHello.hasManifestRevision = true;
    peerHello.manifestFingerprint = deviceHello.manifestFingerprint;
    peerHello.hasManifestFingerprint = true;

    uint8_t bodyStorage[128] = {};
    blinker::ByteView body;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
        peerHello,
        blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)),
        body));
    uint8_t frameStorage[192] = {};
    blinker::ByteView frame;
    CHECK_TRUE(buildTestFrame(
        blinker::bbp2::MessageKind::Hello,
        blinker::bbp2::FlagIsResponse,
        1U,
        body,
        blinker::MutableByteSpan(frameStorage, sizeof(frameStorage)),
        frame));
    transport.inject(frame, 0U, true, sessionId, encrypted);
}

blinker::bbp2::TelemetryStatusBody openRuntimeTelemetryLease(
    FakeTransport& transport,
    uint32_t sessionId,
    uint32_t streamId,
    bool encrypted) {
    blinker::bbp2::TelemetryControlBody open;
    open.operation = blinker::bbp2::TelemetryControlOperation::Open;
    open.streamId = streamId;
    open.leaseMillis = 30000U;
    open.intervalMillis = 200U;
    open.fieldIds[0] = 1U;
    open.fieldCount = 1U;

    uint8_t bodyStorage[128] = {};
    blinker::ByteView body;
    CHECK_TRUE(blinker::bbp2::encodeTelemetryControlBody(
        open,
        blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)),
        body));
    uint8_t frameStorage[192] = {};
    blinker::ByteView frame;
    CHECK_TRUE(buildTestFrame(
        blinker::bbp2::MessageKind::TelemetryControl,
        blinker::bbp2::FlagIdMode,
        2U,
        body,
        blinker::MutableByteSpan(frameStorage, sizeof(frameStorage)),
        frame));
    transport.inject(frame, 0U, true, sessionId, encrypted);

    blinker::bbp2::FrameView outbound;
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(transport.lastSent, transport.lastSentSize),
        outbound));
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::TelemetryStatus),
        outbound.header.kind);
    blinker::bbp2::TelemetryStatusBody status;
    CHECK_TRUE(blinker::bbp2::decodeTelemetryStatusBody(
        outbound.body, status));
    return status;
}

void testClientRateLimitedTelemetryBackoff() {
    static const blinker::FieldSpec fields[] = {
        blinker::FieldSpec(
            "temperature",
            blinker::EndpointKind::Property,
            blinker::ValueType::Float32,
            static_cast<uint8_t>(
                blinker::AccessRead | blinker::AccessNotify),
            nullptr,
            200U)
    };
    blinker::ClientStorage<1, 256, 96, 16> storage;
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = 256U;
    config.maxReassemblySize = 256U;
    config.manifestRevision = 7U;
    blinker::Client client(storage.buffers(fields), config);
    FakeClock clock;
    FakeTransport cloud;
    cloud.featureFlags |= blinker::TransportFeatureCloud;
    CHECK_TRUE(client.setMonotonicClock(&clock));
    CHECK_TRUE(client.addTransport(cloud));
    CHECK_TRUE(client.begin());
    CHECK_TRUE(client.endpoints()
                   .find("temperature")
                   .onSample(&encodeEndpointTelemetrySample, nullptr));
    client.poll(1000U);

    blinker::bbp2::FrameView outbound;
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(cloud.lastSent, cloud.lastSentSize), outbound));
    blinker::bbp2::HelloBody deviceHello;
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(outbound.body, deviceHello));
    negotiateRuntimeTelemetryPeer(cloud, deviceHello, 33U, false);
    (void)openRuntimeTelemetryLease(cloud, 33U, 71U, false);
    client.poll(1000U);
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(cloud.lastSent, cloud.lastSentSize), outbound));
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::TelemetryData),
        outbound.header.kind);

    blinker::bbp2::ErrorBody limited;
    limited.errorCode = static_cast<uint16_t>(blinker::WireError::RateLimited);
    limited.relatedSequence = outbound.header.sequence;
    limited.hasRelatedSequence = true;
    uint8_t bodyStorage[64] = {};
    blinker::ByteView body;
    CHECK_TRUE(blinker::bbp2::encodeErrorBody(
        limited,
        blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)),
        body));
    uint8_t frameStorage[128] = {};
    blinker::ByteView frame;
    CHECK_TRUE(buildTestFrame(
        blinker::bbp2::MessageKind::Error,
        blinker::bbp2::FlagIsResponse,
        3U,
        body,
        blinker::MutableByteSpan(frameStorage, sizeof(frameStorage)),
        frame));
    cloud.inject(frame, 0U, true, 33U);

    const uint32_t sendsAtLimit = cloud.sendCount;
    clock.advance(999U);
    client.poll(1000U);
    CHECK_EQ(sendsAtLimit, cloud.sendCount);
    clock.advance(1U);
    client.poll(1000U);
    CHECK_EQ(sendsAtLimit + 1U, cloud.sendCount);
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(cloud.lastSent, cloud.lastSentSize), outbound));
    blinker::bbp2::TelemetryDataBody resumed;
    CHECK_TRUE(blinker::bbp2::decodeTelemetryDataBody(
        outbound.body, resumed));
    CHECK_EQ(static_cast<uint32_t>(2U), resumed.sampleSequence);
    client.end();
}

void testTelemetryRuntimeAcrossCloudAndDirect() {
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("temperature", 11U),
        blinker::EndpointKind::Property,
        blinker::ValueType::Float32,
        static_cast<uint8_t>(blinker::AccessRead | blinker::AccessNotify),
        nullptr,
        200U);
    blinker::EndpointRegistry endpoints(&endpoint, 1U);
    FakeTransport cloud;
    cloud.featureFlags |= blinker::TransportFeatureCloud;
    FakeTransport direct;
    direct.featureFlags |= blinker::TransportFeatureLocal;
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(cloud));
    CHECK_TRUE(hub.addTransport(direct));

    uint8_t transmit[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(transmit);
    config.maxReassemblySize = sizeof(transmit);
    config.manifestRevision = 6U;
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(transmit, sizeof(transmit)),
        config);
    FakeClock clock;
    CHECK_TRUE(runtime.setTelemetrySampler(
        &clock, &encodeRuntimeTelemetrySample, &runtime));
    CHECK_TRUE(runtime.start());
    runtime.poll(1000U);

    blinker::bbp2::FrameView outbound;
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(cloud.lastSent, cloud.lastSentSize),
        outbound));
    blinker::bbp2::HelloBody deviceHello;
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(outbound.body, deviceHello));
    negotiateRuntimeTelemetryPeer(cloud, deviceHello, 9U, false);
    negotiateRuntimeTelemetryPeer(direct, deviceHello, 17U, true);

    const blinker::bbp2::TelemetryStatusBody cloudStatus =
        openRuntimeTelemetryLease(cloud, 9U, 42U, false);
    const blinker::bbp2::TelemetryStatusBody directStatus =
        openRuntimeTelemetryLease(direct, 17U, 42U, true);
    CHECK_EQ(static_cast<size_t>(2U), runtime.activeTelemetryLeases());
    CHECK_TRUE(cloudStatus.streamEpoch != directStatus.streamEpoch);

    const uint32_t cloudBeforeSample = cloud.sendCount;
    const uint32_t directBeforeSample = direct.sendCount;
    runtime.poll(1000U);
    CHECK_EQ(cloudBeforeSample + 1U, cloud.sendCount);
    CHECK_EQ(directBeforeSample + 1U, direct.sendCount);
    CHECK_EQ(blinker::SendTargetKind::Session, cloud.lastTarget.kind);
    CHECK_EQ(static_cast<uint8_t>(0U), cloud.lastTarget.transportId);
    CHECK_EQ(static_cast<uint32_t>(9U), cloud.lastTarget.sessionId);
    CHECK_EQ(blinker::SendTargetKind::Session, direct.lastTarget.kind);
    CHECK_EQ(static_cast<uint8_t>(1U), direct.lastTarget.transportId);
    CHECK_EQ(static_cast<uint32_t>(17U), direct.lastTarget.sessionId);

    blinker::bbp2::TelemetryDataBody cloudData;
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(cloud.lastSent, cloud.lastSentSize),
        outbound));
    CHECK_TRUE(blinker::bbp2::decodeTelemetryDataBody(
        outbound.body, cloudData));
    blinker::bbp2::TelemetryDataBody directData;
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(direct.lastSent, direct.lastSentSize),
        outbound));
    CHECK_TRUE(blinker::bbp2::decodeTelemetryDataBody(
        outbound.body, directData));
    CHECK_EQ(static_cast<uint32_t>(42U), cloudData.streamId);
    CHECK_EQ(static_cast<uint32_t>(42U), directData.streamId);
    CHECK_EQ(static_cast<uint32_t>(1U), cloudData.sampleSequence);
    CHECK_EQ(static_cast<uint32_t>(1U), directData.sampleSequence);

    // A Broker RateLimited response pauses only the matching Cloud source.
    // Direct keeps sampling and no stale Cloud sample is queued for replay.
    blinker::RxContext cloudSource;
    cloudSource.transportId = 0U;
    cloudSource.sessionId = 9U;
    runtime.deferTelemetrySamples(cloudSource, clock.monotonicMillis(), 1000U);
    clock.advance(200U);
    const uint32_t cloudDuringBackoff = cloud.sendCount;
    const uint32_t directDuringBackoff = direct.sendCount;
    runtime.poll(1000U);
    CHECK_EQ(cloudDuringBackoff, cloud.sendCount);
    CHECK_EQ(directDuringBackoff + 1U, direct.sendCount);
    clock.advance(800U);
    runtime.poll(1000U);
    CHECK_EQ(cloudDuringBackoff + 1U, cloud.sendCount);
    CHECK_EQ(directDuringBackoff + 2U, direct.sendCount);

    // A Direct disconnect revokes only its own lease. Cloud keeps sampling.
    direct.disconnect(17U);
    CHECK_EQ(static_cast<size_t>(1U), runtime.activeTelemetryLeases());
    clock.advance(200U);
    const uint32_t cloudBeforeDisconnectSample = cloud.sendCount;
    const uint32_t directAfterDisconnect = direct.sendCount;
    runtime.poll(1000U);
    CHECK_EQ(cloudBeforeDisconnectSample + 1U, cloud.sendCount);
    CHECK_EQ(directAfterDisconnect, direct.sendCount);

    cloud.currentState = blinker::TransportState::Error;
    runtime.poll(1000U);
    CHECK_EQ(static_cast<size_t>(0U), runtime.activeTelemetryLeases());
    runtime.stop();
}

void testTelemetryRuntime() {
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("temperature", 11U),
        blinker::EndpointKind::Property,
        blinker::ValueType::Float32,
        static_cast<uint8_t>(blinker::AccessRead | blinker::AccessNotify),
        nullptr,
        200U);
    blinker::EndpointRegistry endpoints(&endpoint, 1U);
    FakeTransport transport;
    transport.featureFlags |= blinker::TransportFeatureCloud;
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(transport).ok());
    uint8_t transmit[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(transmit);
    config.maxReassemblySize = sizeof(transmit);
    config.manifestRevision = 5U;
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(transmit, sizeof(transmit)),
        config);
    FakeClock clock;
    CHECK_TRUE(runtime.setTelemetrySampler(
        &clock, &encodeRuntimeTelemetrySample, &runtime));
    CHECK_TRUE(runtime.start());
    runtime.poll(1000U);

    blinker::bbp2::FrameView outbound;
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(transport.lastSent, transport.lastSentSize),
        outbound));
    blinker::bbp2::HelloBody deviceHello;
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(outbound.body, deviceHello));
    CHECK_TRUE(
        (deviceHello.features & blinker::bbp2::FeatureTelemetry) != 0U);

    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1U;
    peerHello.features =
        blinker::bbp2::FeatureManifest |
        blinker::bbp2::FeatureEndpointIds |
        blinker::bbp2::FeatureTelemetry;
    peerHello.maxFrameSize = sizeof(transmit);
    peerHello.maxReassemblySize = sizeof(transmit);
    peerHello.manifestRevision = deviceHello.manifestRevision;
    peerHello.hasManifestRevision = true;
    peerHello.manifestFingerprint = deviceHello.manifestFingerprint;
    peerHello.hasManifestFingerprint = true;
    uint8_t bodyStorage[128] = {};
    blinker::ByteView body;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
        peerHello,
        blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)),
        body));
    uint8_t frameStorage[192] = {};
    blinker::ByteView frame;
    CHECK_TRUE(buildTestFrame(
        blinker::bbp2::MessageKind::Hello,
        blinker::bbp2::FlagIsResponse,
        1U,
        body,
        blinker::MutableByteSpan(frameStorage, sizeof(frameStorage)),
        frame));
    transport.inject(frame, 0U, true, 9U);

    blinker::bbp2::TelemetryControlBody open;
    open.operation = blinker::bbp2::TelemetryControlOperation::Open;
    open.streamId = 42U;
    open.leaseMillis = 30000U;
    open.intervalMillis = 100U;
    open.fieldIds[0] = 1U;
    open.fieldCount = 1U;
    CHECK_TRUE(blinker::bbp2::encodeTelemetryControlBody(
        open,
        blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)),
        body));
    CHECK_TRUE(buildTestFrame(
        blinker::bbp2::MessageKind::TelemetryControl,
        blinker::bbp2::FlagIdMode,
        2U,
        body,
        blinker::MutableByteSpan(frameStorage, sizeof(frameStorage)),
        frame));
    transport.inject(frame, 0U, true, 9U);
    CHECK_EQ(static_cast<size_t>(1U), runtime.activeTelemetryLeases());
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(transport.lastSent, transport.lastSentSize),
        outbound));
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::TelemetryStatus),
        outbound.header.kind);
    blinker::bbp2::TelemetryStatusBody status;
    CHECK_TRUE(blinker::bbp2::decodeTelemetryStatusBody(
        outbound.body, status));
    CHECK_EQ(static_cast<uint32_t>(200U), status.effectiveIntervalMillis);

    runtime.poll(1000U);
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(transport.lastSent, transport.lastSentSize),
        outbound));
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::TelemetryData),
        outbound.header.kind);
    blinker::bbp2::TelemetryDataBody data;
    CHECK_TRUE(blinker::bbp2::decodeTelemetryDataBody(outbound.body, data));
    CHECK_EQ(static_cast<uint32_t>(42U), data.streamId);
    CHECK_EQ(static_cast<uint32_t>(1U), data.sampleSequence);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        runtime.telemetryCounters().samplesSent);
    CHECK_EQ(
        static_cast<uint32_t>(0U),
        runtime.telemetryCounters().samplesDropped);

    transport.sendResult =
        blinker::Result::failure(blinker::ErrorCode::WouldBlock);
    clock.advance(200U);
    runtime.poll(1000U);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        runtime.telemetryCounters().samplesDropped);

    transport.sendResult = blinker::Result::success();
    clock.advance(200U);
    runtime.poll(1000U);
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(transport.lastSent, transport.lastSentSize),
        outbound));
    CHECK_TRUE(blinker::bbp2::decodeTelemetryDataBody(outbound.body, data));
    CHECK_EQ(static_cast<uint32_t>(3U), data.sampleSequence);
    CHECK_EQ(
        static_cast<uint32_t>(2U),
        runtime.telemetryCounters().samplesSent);

    clock.advance(29600U);
    runtime.poll(1000U);
    CHECK_EQ(static_cast<size_t>(0U), runtime.activeTelemetryLeases());
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(transport.lastSent, transport.lastSentSize),
        outbound));
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::TelemetryStatus),
        outbound.header.kind);
    CHECK_TRUE(blinker::bbp2::decodeTelemetryStatusBody(
        outbound.body, status));
    CHECK_EQ(blinker::bbp2::TelemetryStatusCode::Expired, status.status);
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        runtime.telemetryCounters().leasesExpired);
    runtime.stop();
}

void testDeviceRuntime() {
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite));
    const blinker::EndpointDescriptor* storage[1] = {};
    blinker::EndpointRegistry endpoints(storage, 1);
    CHECK_TRUE(endpoints.add(endpoint).ok());

    FakeTransport transport;
    transport.featureFlags |= blinker::TransportFeatureCloud;
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(transport).ok());
    uint8_t transmitBuffer[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(transmitBuffer);
    config.maxReassemblySize = sizeof(transmitBuffer);
    config.manifestRevision = 3;
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(
            transmitBuffer,
            sizeof(transmitBuffer)),
        config);
    CommandCapture capture;
    runtime.setCommandHandler(&captureCommand, &capture);
    runtime.setStateEncoder(&encodeTestState, nullptr);
    runtime.setStatePageEncoder(&encodeTestStatePage, nullptr);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000);
    CHECK_EQ(static_cast<uint32_t>(1), transport.sendCount);
    blinker::bbp2::FrameView outbound;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello),
        outbound.header.kind);
    blinker::bbp2::HelloBody deviceHello;
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(
                   outbound.body,
                   deviceHello)
                   .ok());
    CHECK_TRUE(deviceHello.hasManifestFingerprint);
    CHECK_EQ(
        static_cast<size_t>(blinker::kSha256Size),
        deviceHello.manifestFingerprint.size);
    CHECK_TRUE(
        (deviceHello.features & blinker::bbp2::FeatureEndpointIds) != 0U);
    CHECK_TRUE(
        (deviceHello.features &
         blinker::bbp2::FeatureReliableDelivery) != 0U);
    CHECK_EQ(
        config.reliableReceiveWindow,
        deviceHello.reliableReceiveWindow);

    uint8_t bodyBuffer[128] = {};
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = 2;
    peerHello.versionCount = 1;
    peerHello.features =
        blinker::bbp2::FeatureManifest |
        blinker::bbp2::FeatureEndpointIds |
        blinker::bbp2::FeatureStateRevision;
    // The Runtime must honor this per-session ceiling even though its own
    // transmit buffer is 256 bytes.
    peerHello.maxFrameSize = 80;
    peerHello.maxReassemblySize = 80;
    blinker::ByteView body;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(
                       bodyBuffer,
                       sizeof(bodyBuffer)),
                   body)
                   .ok());
    uint8_t frameBuffer[256] = {};
    blinker::ByteView frame;
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   10,
                   body,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 7);
    CHECK_EQ(static_cast<uint32_t>(1), transport.sendCount);

    blinker::bbp2::KeyedBodyWriter commandWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(commandWriter.begin(1).ok());
    CHECK_TRUE(commandWriter.writeBool(
                   blinker::StringView("lamp", 4), true).ok());
    CHECK_TRUE(commandWriter.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagAckRequired,
                   11,
                   body,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 7);
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);
    CHECK_TRUE(capture.value);
    CHECK_EQ(static_cast<uint32_t>(7), capture.sessionId);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Ack),
        outbound.header.kind);
    blinker::bbp2::AckBody ack;
    CHECK_TRUE(blinker::bbp2::decodeAckBody(outbound.body, ack).ok());
    CHECK_EQ(static_cast<uint16_t>(11), ack.acknowledgedSequence);

    blinker::SendTarget preManifestTarget;
    preManifestTarget.kind = blinker::SendTargetKind::Session;
    preManifestTarget.transportId = 0;
    preManifestTarget.sessionId = 7;
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        runtime.sendCurrentPatch(body, preManifestTarget).code());
    blinker::SendTarget preManifestBroadcast;
    preManifestBroadcast.kind = blinker::SendTargetKind::Broadcast;
    const uint32_t beforePreManifestBroadcast = transport.sendCount;
    CHECK_TRUE(runtime.sendCurrentPatch(body, preManifestBroadcast).ok());
    CHECK_EQ(beforePreManifestBroadcast, transport.sendCount);

    blinker::bbp2::IdBodyWriter idCommandWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(idCommandWriter.begin(1).ok());
    CHECK_TRUE(idCommandWriter.writeBool(1, false).ok());
    CHECK_TRUE(idCommandWriter.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   static_cast<uint8_t>(
                       blinker::bbp2::FlagAckRequired |
                       blinker::bbp2::FlagIdMode),
                   14,
                   body,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 7);
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Error),
        outbound.header.kind);

    blinker::SendTarget idTarget;
    idTarget.kind = blinker::SendTargetKind::Session;
    idTarget.transportId = 0;
    idTarget.sessionId = 7;
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        runtime.sendPatchById(body, idTarget).code());

    blinker::bbp2::ManifestRequestBody manifestRequest;
    manifestRequest.cursor = 0;
    CHECK_TRUE(blinker::bbp2::encodeManifestRequestBody(
                   manifestRequest,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::ManifestRequest,
                   blinker::bbp2::FlagNone,
                   12,
                   body,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 7);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Manifest),
        outbound.header.kind);
    CHECK_EQ(static_cast<uint16_t>(12), outbound.header.sequence);
    CHECK_TRUE(
        (outbound.header.flags & blinker::bbp2::FlagIsResponse) != 0U);
    CHECK_TRUE(transport.lastSentSize <= peerHello.maxFrameSize);
    blinker::ManifestPageView manifestPage;
    CHECK_TRUE(blinker::decodeManifestPage(
                   outbound.body,
                   manifestPage)
                   .ok());
    CHECK_EQ(static_cast<uint16_t>(0), manifestPage.cursor);
    CHECK_EQ(manifestPage.totalFields, manifestPage.nextCursor);

    uint8_t acceptedFingerprint[blinker::kSha256Size] = {};
    std::memcpy(
        acceptedFingerprint,
        manifestPage.fingerprint.data,
        sizeof(acceptedFingerprint));
    blinker::bbp2::ManifestAcceptBody manifestAccept;
    manifestAccept.revision = manifestPage.revision;
    uint8_t wrongFingerprint[blinker::kSha256Size] = {};
    std::memcpy(
        wrongFingerprint,
        acceptedFingerprint,
        sizeof(wrongFingerprint));
    wrongFingerprint[0] ^= 0x01U;
    manifestAccept.fingerprint = blinker::ByteView(
        wrongFingerprint,
        sizeof(wrongFingerprint));
    CHECK_TRUE(blinker::bbp2::encodeManifestAcceptBody(
                   manifestAccept,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::ManifestAccept,
                   blinker::bbp2::FlagAckRequired,
                   15,
                   body,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 7);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Error),
        outbound.header.kind);
    blinker::bbp2::ErrorBody manifestError;
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(
                   outbound.body,
                   manifestError)
                   .ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::ManifestConflict),
        manifestError.errorCode);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        runtime.sendPatchById(body, idTarget).code());

    manifestAccept.fingerprint = blinker::ByteView(
        acceptedFingerprint,
        sizeof(acceptedFingerprint));
    CHECK_TRUE(blinker::bbp2::encodeManifestAcceptBody(
                   manifestAccept,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::ManifestAccept,
                   blinker::bbp2::FlagAckRequired,
                   17,
                   body,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 7);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Ack),
        outbound.header.kind);

    blinker::bbp2::IdBodyWriter acceptedIdWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(acceptedIdWriter.begin(1).ok());
    CHECK_TRUE(acceptedIdWriter.writeBool(1, false).ok());
    CHECK_TRUE(acceptedIdWriter.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   static_cast<uint8_t>(
                       blinker::bbp2::FlagAckRequired |
                       blinker::bbp2::FlagIdMode),
                   16,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 7);
    CHECK_EQ(static_cast<uint32_t>(2), capture.calls);
    CHECK_TRUE(!capture.value);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Ack),
        outbound.header.kind);

    CHECK_TRUE(runtime.sendPatchById(body, idTarget).ok());
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Patch),
        outbound.header.kind);
    CHECK_TRUE(
        (outbound.header.flags & blinker::bbp2::FlagIdMode) != 0U);
    CHECK_TRUE(transport.lastSentSize <= peerHello.maxFrameSize);
    blinker::SendTarget idBroadcast;
    idBroadcast.kind = blinker::SendTargetKind::Broadcast;
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        runtime.sendPatchById(body, idBroadcast).code());

    uint8_t stateRequestBuffer[16] = {};
    blinker::ByteView stateRequestBody;
    CHECK_TRUE(encodeTestStateRequest(
                   0,
                   0,
                   false,
                   blinker::MutableByteSpan(
                       stateRequestBuffer,
                       sizeof(stateRequestBuffer)),
                   stateRequestBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagIdMode,
                   13,
                   stateRequestBody,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 7);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::StatePage),
        outbound.header.kind);
    CHECK_EQ(static_cast<uint16_t>(13), outbound.header.sequence);
    CHECK_TRUE(
        (outbound.header.flags & blinker::bbp2::FlagIdMode) != 0U);
    blinker::bbp2::StatePageBody statePage;
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   outbound.body,
                   statePage)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(1), statePage.revision);
    CHECK_EQ(static_cast<uint16_t>(1), statePage.nextCursor);
    CHECK_EQ(static_cast<uint16_t>(1), statePage.totalFields);
    IdCapture stateCapture;
    CHECK_TRUE(blinker::bbp2::visitIdBody(
                   statePage.values,
                   &captureIdValue,
                   &stateCapture)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(1), stateCapture.calls);
    CHECK_EQ(static_cast<uint16_t>(1), stateCapture.firstId);
    CHECK_TRUE(stateCapture.firstType == blinker::cbor::Type::Boolean);

    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagAckRequired,
                   11,
                   body,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, false, 7);
    CHECK_EQ(static_cast<uint32_t>(2), capture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    blinker::bbp2::ErrorBody error;
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(outbound.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::AuthenticationRequired),
        error.errorCode);

    transport.inject(frame, 99, true, 8);
    CHECK_EQ(static_cast<uint32_t>(2), capture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(outbound.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::NegotiationRequired),
        error.errorCode);

    // A different session may skip the download only when its HELLO carries
    // the exact already-validated device revision+fingerprint.
    peerHello.manifestRevision = config.manifestRevision;
    peerHello.hasManifestRevision = true;
    peerHello.manifestFingerprint = blinker::ByteView(
        acceptedFingerprint,
        sizeof(acceptedFingerprint));
    peerHello.hasManifestFingerprint = true;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   18,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 8);
    blinker::bbp2::IdBodyWriter cachedIdWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(cachedIdWriter.begin(1).ok());
    CHECK_TRUE(cachedIdWriter.writeBool(1, true).ok());
    CHECK_TRUE(cachedIdWriter.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   static_cast<uint8_t>(
                       blinker::bbp2::FlagAckRequired |
                       blinker::bbp2::FlagIdMode),
                   19,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 99, true, 8);
    CHECK_EQ(static_cast<uint32_t>(3), capture.calls);
    CHECK_EQ(static_cast<uint32_t>(8), capture.sessionId);
    CHECK_TRUE(capture.value);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Ack),
        outbound.header.kind);

    runtime.stop();
}
// An accepted frame occupies the sole TX slot until transport.poll(). Unlike
// FakeTransport, a rejected send cannot overwrite the accepted frame evidence.
class SingleSlotTransport : public FakeTransport {
public:
    bool full = false;
    bool stalled = false;
    uint32_t attempts = 0U;
    uint32_t permissions = blinker::kAuthorizationPermissionObserve |
                           blinker::kAuthorizationPermissionControl;

    blinker::Result send(blinker::ByteView frame,
                         const blinker::SendTarget& target) override {
        ++attempts;
        if (full) return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        const blinker::Result result = FakeTransport::send(frame, target);
        if (result) full = true;
        return result;
    }
    void poll(uint32_t budget) override {
        FakeTransport::poll(budget);
        if (!stalled) full = false;
    }
    uint32_t sessionPermissions(uint32_t) const override { return permissions; }
};

void testSingleSlotCommandAck() {
    static const blinker::FieldSpec fields[] = {
        blinker::FieldSpec("power", blinker::EndpointKind::Property,
            blinker::ValueType::Boolean, blinker::AccessRead | blinker::AccessWrite),
        blinker::FieldSpec("button", blinker::EndpointKind::Action,
            blinker::ValueType::Null, blinker::AccessCommand),
        blinker::FieldSpec("notice", blinker::EndpointKind::Event,
            blinker::ValueType::Boolean, blinker::AccessEvent)
    };
    blinker::ClientStorage<3, 256, 96, 16> storage;
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = 256U;
    config.maxReassemblySize = 256U;
    config.reliableReceiveWindow = 1U;
    blinker::Client client(storage.buffers(fields), config);
    FakeClock clock;
    SingleSlotTransport local;
    FakeTransport cloud;
    local.featureFlags |= blinker::TransportFeatureLocal |
                          blinker::TransportFeatureAuthenticated |
                          blinker::TransportFeaturePeerInitiatesHello;
    cloud.featureFlags |= blinker::TransportFeaturePeerInitiatesHello;
    struct Capture { blinker::Client* client; uint32_t calls; } capture = {&client, 0U};
    auto handler = [](void* context, const blinker::EndpointHandle&,
                      const blinker::EndpointValueView& value,
                      const blinker::RxContext&) -> blinker::Result {
        Capture& state = *static_cast<Capture*>(context);
        ++state.calls;
        // A null Action changes state too, exercising the same settlement path.
        bool next = false;
        if (value.type() == blinker::cbor::Type::Boolean) {
            const blinker::Result result = value.asBool(next);
            if (!result) return result;
        }
        return state.client->endpoints().find("power").set(next);
    };
    CHECK_TRUE(client.endpoints().find("power").onCommand(handler, &capture).ok());
    CHECK_TRUE(client.endpoints().find("button").onCommand(handler, &capture).ok());
    CanonicalStateWriteCapture stateWrites;
    CHECK_TRUE(client.setStateWriteHandler(&acceptCanonicalStateWrite, &stateWrites).ok());
    CHECK_TRUE(client.setMonotonicClock(&clock).ok());
    CHECK_TRUE(client.addTransport(local).ok());
    CHECK_TRUE(client.addTransport(cloud).ok());
    CHECK_TRUE(client.begin().ok());
    uint8_t bodyBytes[128] = {}, frameBytes[256] = {};
    blinker::ByteView body, frame;
    auto hello = [&](FakeTransport& port, uint32_t sessionId) {
        blinker::bbp2::HelloBody value;
        value.role = blinker::bbp2::PeerRole::App;
        value.versions[0] = blinker::bbp2::kVersion;
        value.versionCount = 1U;
        value.features = blinker::bbp2::FeatureReliableDelivery |
                         blinker::bbp2::FeatureStateRevision;
        value.reliableReceiveWindow = 1U;
        value.maxFrameSize = 256U;
        value.maxReassemblySize = 256U;
        CHECK_TRUE(blinker::bbp2::encodeHelloBody(value,
            blinker::MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body).ok());
        CHECK_TRUE(buildTestFrame(blinker::bbp2::MessageKind::Hello,
            blinker::bbp2::FlagIsResponse, 1U, body,
            blinker::MutableByteSpan(frameBytes, sizeof(frameBytes)), frame).ok());
        port.inject(frame, 0U, true, sessionId);
    };
    auto command = [&](uint16_t sequence, bool button, bool value = true) {
        static const uint8_t action[] = {0xa1U, 0x66U, 'b', 'u', 't', 't', 'o', 'n', 0xf6U};
        blinker::bbp2::KeyedBodyWriter writer(
            blinker::MutableByteSpan(bodyBytes, sizeof(bodyBytes)));
        CHECK_TRUE(writer.begin(1U).ok());
        CHECK_TRUE(writer.writeBool(blinker::StringView("power", 5U), value).ok());
        CHECK_TRUE(writer.finish(body).ok());
        if (button) body = blinker::ByteView(action, sizeof(action));
        CHECK_TRUE(buildTestFrame(blinker::bbp2::MessageKind::Command,
            blinker::bbp2::FlagAckRequired, sequence, body,
            blinker::MutableByteSpan(frameBytes, sizeof(frameBytes)), frame).ok());
        local.inject(frame, 0U, true, 7U);
    };
    auto kind = [](const FakeTransport& port) {
        blinker::bbp2::FrameView value;
        CHECK_TRUE(blinker::bbp2::parseFrame(
            blinker::ByteView(port.lastSent, port.lastSentSize), value).ok());
        return value.header.kind;
    };
    auto ack = [&](uint16_t sequence) {
        blinker::bbp2::FrameView value;
        blinker::bbp2::AckBody result;
        CHECK_TRUE(blinker::bbp2::parseFrame(
            blinker::ByteView(local.lastSent, local.lastSentSize), value).ok());
        CHECK_EQ(static_cast<uint8_t>(blinker::bbp2::MessageKind::Ack), value.header.kind);
        CHECK_TRUE(blinker::bbp2::decodeAckBody(value.body, result).ok());
        CHECK_EQ(sequence, result.acknowledgedSequence);
        CHECK_EQ(7U, local.lastTarget.sessionId);
        CHECK_EQ(0U, local.lastTarget.transportId);
    };
    hello(local, 7U);
    hello(cloud, 0U);
    command(10U, false);
    CHECK_EQ(1U, capture.calls);
    CHECK_EQ(static_cast<uint8_t>(blinker::bbp2::MessageKind::Patch), kind(local));
    CHECK_EQ(static_cast<uint8_t>(blinker::bbp2::MessageKind::Patch), kind(cloud));
    const uint32_t cloudSends = cloud.sendCount;
    local.stalled = true;
    const uint32_t attempts = local.attempts;
    client.poll(1000U);
    CHECK_EQ(attempts + 1U, local.attempts); // Bounded, no busy loop.
    local.stalled = false;
    client.poll(1000U); // No peer retry or elapsed clock time needed.
    ack(10U);
    CHECK_EQ(cloudSends, cloud.sendCount); // ACK stays on original lane.
    const uint32_t accepted = local.sendCount;
    client.poll(1000U);
    CHECK_EQ(accepted, local.sendCount); // Accepted ACK isn't resent by poll.
    command(11U, false); // Unchanged value needs no state Patch.
    ack(11U);
    client.poll(1000U);
    command(12U, true);
    CHECK_EQ(static_cast<uint8_t>(blinker::bbp2::MessageKind::Patch), kind(local));
    local.inject(frame, 0U, true, 7U); // Same Action while ACK still blocked.
    CHECK_EQ(3U, capture.calls);
    client.poll(1000U);
    ack(12U);
    client.poll(1000U);
    local.inject(frame, 0U, true, 7U); // Exact replay remains supported.
    ack(12U);
    CHECK_EQ(3U, capture.calls);
    client.poll(1000U);
    command(13U, false);
    local.permissions = blinker::kAuthorizationPermissionObserve;
    const uint32_t revokedSends = local.sendCount;
    client.poll(1000U);
    local.permissions |= blinker::kAuthorizationPermissionControl;
    client.poll(1000U);
    CHECK_EQ(revokedSends, local.sendCount); // No dormant auto-ACK after revoke.
    command(14U, true);
    local.disconnect(7U);
    hello(local, 8U);
    const uint32_t closedSends = local.sendCount;
    client.poll(1000U);
    CHECK_EQ(closedSends, local.sendCount); // Never send to replacement session.
    local.disconnect(8U);
    hello(local, 7U);

    // One-slot negotiated receive window: replacement keeps only the newest
    // result, without growing a separate pending queue.
    command(15U, false);
    command(16U, false);
    client.poll(1000U);
    ack(16U);
    client.poll(1000U);

    CommandCapture events;
    client.setEventHandler(&captureCommand, &events);
    static const uint8_t eventValues[] = {0xa1U, 0x66U, 'n', 'o', 't', 'i', 'c', 'e', 0xf5U};
    CHECK_TRUE(buildTestFrame(blinker::bbp2::MessageKind::Event,
        blinker::bbp2::FlagAckRequired, 17U,
        blinker::ByteView(eventValues, sizeof(eventValues)),
        blinker::MutableByteSpan(frameBytes, sizeof(frameBytes)), frame).ok());
    local.full = true;
    local.inject(frame, 0U, true, 7U);
    client.poll(1000U);
    ack(17U);
    CHECK_EQ(1U, events.calls);
    client.poll(1000U);

    // Session zero is transport-scoped (e.g. MQTT), not a synthetic local ID.
    cloud.sendResult = blinker::Result::failure(blinker::ErrorCode::WouldBlock);
    cloud.inject(frame, 0U, true, 0U);
    cloud.sendResult = blinker::Result::success();
    client.poll(1000U);
    CHECK_EQ(static_cast<uint8_t>(blinker::bbp2::MessageKind::Ack), kind(cloud));
    CHECK_EQ(blinker::SendTargetKind::Transport, cloud.lastTarget.kind);
    CHECK_EQ(1U, cloud.lastTarget.transportId);
    CHECK_EQ(2U, events.calls);

    CHECK_TRUE(encodeTestStateRequest(0U, 0U, false,
        blinker::MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body).ok());
    CHECK_TRUE(buildTestFrame(blinker::bbp2::MessageKind::StateRequest,
        blinker::bbp2::FlagNone, 18U, body,
        blinker::MutableByteSpan(frameBytes, sizeof(frameBytes)), frame).ok());
    local.inject(frame, 0U, true, 7U); // StatePage fills the slot before Apply.
    static const uint8_t patchValues[] = {0xa1U, 0x65U, 'p', 'o', 'w', 'e', 'r', 0xf4U};
    blinker::bbp2::StatePatchBody patch;
    patch.mode = blinker::bbp2::StatePatchMode::Apply;
    patch.revision = client.runtime().stateRevision();
    patch.values = blinker::ByteView(patchValues, sizeof(patchValues));
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(patch,
        blinker::MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body).ok());
    CHECK_TRUE(buildTestFrame(blinker::bbp2::MessageKind::Patch,
        blinker::bbp2::FlagAckRequired, 19U, body,
        blinker::MutableByteSpan(frameBytes, sizeof(frameBytes)), frame).ok());
    local.inject(frame, 0U, true, 7U);
    const uint32_t acceptedRevision = client.runtime().stateRevision();
    CHECK_EQ(patch.revision + 1U, acceptedRevision);
    CHECK_TRUE(client.endpoints().find("power").set(true).ok());
    clock.advance(250U);
    client.poll(1000U);
    CHECK_EQ(acceptedRevision + 1U, client.runtime().stateRevision());
    ack(19U);
    blinker::bbp2::FrameView response;
    blinker::bbp2::AckBody result;
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(local.lastSent, local.lastSentSize), response).ok());
    CHECK_TRUE(blinker::bbp2::decodeAckBody(response.body, result).ok());
    CHECK_TRUE(result.hasStateRevision);
    CHECK_EQ(acceptedRevision, result.stateRevision); // Execution revision, not send-time revision.
    CHECK_EQ(1U, stateWrites.calls);
    client.poll(1000U);
    command(20U, false, false);
    local.currentState = blinker::TransportState::Backoff;
    const uint32_t offlineSends = local.sendCount;
    client.poll(1000U);
    local.currentState = blinker::TransportState::Online;
    hello(local, 7U);
    client.poll(1000U);
    CHECK_EQ(offlineSends, local.sendCount); // Retirement also handles missing disconnect callbacks.
    local.sendResult = blinker::Result::failure(blinker::ErrorCode::NotConnected);
    command(21U, false, false); // No change, permanent send error isn't auto-retried.
    const uint32_t failedAttempts = local.attempts;
    local.sendResult = blinker::Result::success();
    client.poll(1000U);
    CHECK_EQ(failedAttempts, local.attempts);
    client.end();
}

void testReliableCommandIdempotency() {
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite));
    const blinker::EndpointDescriptor* storage[1] = {};
    blinker::EndpointRegistry endpoints(storage, 1);
    CHECK_TRUE(endpoints.add(endpoint).ok());

    FakeTransport transport;
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(transport).ok());
    uint8_t transmitBuffer[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(transmitBuffer);
    config.maxReassemblySize = sizeof(transmitBuffer);
    config.reliableReceiveWindow = 1;
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(transmitBuffer, sizeof(transmitBuffer)),
        config);
    CommandCapture capture;
    runtime.setCommandHandler(&captureCommand, &capture);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000);

    uint8_t bodyBuffer[128] = {};
    uint8_t frameBuffer[256] = {};
    blinker::ByteView body;
    blinker::ByteView frame;
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1;
    peerHello.features =
        blinker::bbp2::FeatureReliableDelivery |
        blinker::bbp2::FeatureStateRevision;
    peerHello.reliableReceiveWindow = 1;
    peerHello.maxFrameSize = 256;
    peerHello.maxReassemblySize = 256;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   90,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);

    blinker::bbp2::KeyedBodyWriter commandWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(commandWriter.begin(1).ok());
    CHECK_TRUE(commandWriter.writeBool(
                   blinker::StringView("lamp", 4), true).ok());
    CHECK_TRUE(commandWriter.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagAckRequired,
                   100,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.sendResult =
        blinker::Result::failure(blinker::ErrorCode::WouldBlock);
    transport.inject(frame, 0, true, 7);
    transport.inject(frame, 0, true, 7);
    transport.sendResult = blinker::Result::success();
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);
    CHECK_EQ(static_cast<uint32_t>(1), runtime.duplicateCommandCount());
    CHECK_EQ(static_cast<uint32_t>(0), runtime.rejectedFrameCount());
    blinker::bbp2::FrameView response;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Ack),
        response.header.kind);

    blinker::bbp2::KeyedBodyWriter conflictingWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(conflictingWriter.begin(1).ok());
    CHECK_TRUE(conflictingWriter.writeBool(
                   blinker::StringView("lamp", 4), false).ok());
    CHECK_TRUE(conflictingWriter.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagAckRequired,
                   100,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);
    CHECK_EQ(static_cast<uint32_t>(1), runtime.sequenceConflictCount());
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    blinker::bbp2::ErrorBody error;
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(response.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::SequenceConflict),
        error.errorCode);

    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagAckRequired,
                   101,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<uint32_t>(2), capture.calls);

    blinker::bbp2::KeyedBodyWriter evictedWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(evictedWriter.begin(1).ok());
    CHECK_TRUE(evictedWriter.writeBool(
                   blinker::StringView("lamp", 4), true).ok());
    CHECK_TRUE(evictedWriter.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagAckRequired,
                   100,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<uint32_t>(3), capture.calls);

    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   91,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 8);
    blinker::bbp2::KeyedBodyWriter isolatedWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(isolatedWriter.begin(1).ok());
    CHECK_TRUE(isolatedWriter.writeBool(
                   blinker::StringView("lamp", 4), true).ok());
    CHECK_TRUE(isolatedWriter.finish(body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagAckRequired,
                   100,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 8);
    CHECK_EQ(static_cast<uint32_t>(4), capture.calls);
    CHECK_EQ(static_cast<uint32_t>(8), capture.sessionId);

    uint32_t rejectedCalls = 0;
    runtime.setCommandHandler(&rejectCommand, &rejectedCalls);
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagAckRequired,
                   102,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 8);
    transport.inject(frame, 0, true, 8);
    CHECK_EQ(static_cast<uint32_t>(1), rejectedCalls);
    CHECK_EQ(static_cast<uint32_t>(2), runtime.duplicateCommandCount());
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(response.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::CommandRejected),
        error.errorCode);

    runtime.stop();
}

void testVersionedStateAndEvents() {
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite));
    const blinker::EndpointDescriptor alarm(
        blinker::StringView("alarm", 5),
        blinker::EndpointKind::Event,
        blinker::ValueType::Boolean,
        blinker::AccessEvent);
    const blinker::EndpointDescriptor* storage[2] = {};
    blinker::EndpointRegistry endpoints(storage, 2);
    CHECK_TRUE(endpoints.add(endpoint).ok());
    CHECK_TRUE(endpoints.add(alarm).ok());

    FakeTransport transport;
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(transport).ok());
    uint8_t transmitBuffer[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(transmitBuffer);
    config.maxReassemblySize = sizeof(transmitBuffer);
    config.initialStateRevision = 7;
    config.reliableReceiveWindow = 2;
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(transmitBuffer, sizeof(transmitBuffer)),
        config);
    CommandCapture applyCapture;
    CommandCapture eventCapture;
    uint16_t manifestFieldCount = 2U;
    runtime.setStateEncoder(&encodeTestState, nullptr);
    runtime.setStatePageEncoder(
        &encodeTestStatePage,
        &manifestFieldCount);
    runtime.setStateApplyHandler(&captureCommand, &applyCapture);
    runtime.setEventHandler(&captureCommand, &eventCapture);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000);

    uint8_t valuesBuffer[64] = {};
    uint8_t bodyBuffer[128] = {};
    uint8_t frameBuffer[256] = {};
    blinker::ByteView values;
    blinker::ByteView body;
    blinker::ByteView frame;

    blinker::bbp2::HelloBody hello;
    hello.role = blinker::bbp2::PeerRole::App;
    hello.versions[0] = blinker::bbp2::kVersion;
    hello.versionCount = 1;
    hello.features =
        blinker::bbp2::FeatureReliableDelivery |
        blinker::bbp2::FeatureStateRevision;
    hello.reliableReceiveWindow = 2;
    hello.maxFrameSize = sizeof(transmitBuffer);
    hello.maxReassemblySize = sizeof(transmitBuffer);
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   hello,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   300,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);

    blinker::bbp2::KeyedBodyWriter valuesWriter(
        blinker::MutableByteSpan(valuesBuffer, sizeof(valuesBuffer)));
    CHECK_TRUE(valuesWriter.begin(1).ok());
    CHECK_TRUE(valuesWriter.writeBool(
                   blinker::StringView("lamp", 4),
                   true)
                   .ok());
    CHECK_TRUE(valuesWriter.finish(values).ok());
    blinker::bbp2::StatePatchBody apply;
    apply.mode = blinker::bbp2::StatePatchMode::Apply;
    apply.revision = 7;
    apply.values = values;
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(
                   apply,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Patch,
                   blinker::bbp2::FlagAckRequired,
                   301,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());

    // A revision token is only valid after this peer observes a snapshot in
    // the current session.
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<uint32_t>(0), applyCapture.calls);
    blinker::bbp2::FrameView response;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    blinker::bbp2::ErrorBody error;
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(response.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::NegotiationRequired),
        error.errorCode);

    CHECK_TRUE(encodeTestStateRequest(
                   0,
                   0,
                   false,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   302,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    blinker::bbp2::StatePageBody statePage;
    CHECK_TRUE(blinker::bbp2::decodeStatePageBody(
                   response.body,
                   statePage)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(7), statePage.revision);

    // Rebuild because frameBuffer was reused for StateRequest.
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(
                   apply,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Patch,
                   blinker::bbp2::FlagAckRequired,
                   303,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<uint32_t>(1), applyCapture.calls);
    CHECK_EQ(static_cast<uint32_t>(8), runtime.stateRevision());
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    blinker::bbp2::AckBody ack;
    CHECK_TRUE(blinker::bbp2::decodeAckBody(response.body, ack).ok());
    CHECK_TRUE(ack.hasStateRevision);
    CHECK_EQ(static_cast<uint32_t>(8), ack.stateRevision);

    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<uint32_t>(1), applyCapture.calls);
    CHECK_EQ(static_cast<uint32_t>(1), runtime.duplicateRequestCount());
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeAckBody(response.body, ack).ok());
    CHECK_EQ(static_cast<uint32_t>(8), ack.stateRevision);

    apply.revision = 7;
    CHECK_TRUE(blinker::bbp2::encodeStatePatchBody(
                   apply,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Patch,
                   blinker::bbp2::FlagAckRequired,
                   304,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<uint32_t>(1), applyCapture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(response.body, error).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::StateConflict),
        error.errorCode);
    CHECK_TRUE(error.hasStateRevision);
    CHECK_EQ(static_cast<uint32_t>(8), error.stateRevision);
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<uint32_t>(2), runtime.duplicateRequestCount());

    uint8_t eventValuesBuffer[32] = {};
    blinker::bbp2::KeyedBodyWriter eventValuesWriter(
        blinker::MutableByteSpan(
            eventValuesBuffer,
            sizeof(eventValuesBuffer)));
    CHECK_TRUE(eventValuesWriter.begin(1U).ok());
    CHECK_TRUE(eventValuesWriter.writeBool("alarm", true).ok());
    blinker::ByteView eventValues;
    CHECK_TRUE(eventValuesWriter.finish(eventValues).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Event,
                   blinker::bbp2::FlagAckRequired,
                   305,
                   eventValues,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<uint32_t>(1), eventCapture.calls);
    CHECK_EQ(static_cast<uint32_t>(3), runtime.duplicateRequestCount());
    CHECK_EQ(static_cast<uint32_t>(8), runtime.stateRevision());

    uint32_t sharedRevision = 0;
    CHECK_TRUE(runtime.advanceStateRevision(sharedRevision).ok());
    CHECK_EQ(static_cast<uint32_t>(9), sharedRevision);
    blinker::SendTarget target;
    target.kind = blinker::SendTargetKind::Session;
    target.transportId = 0;
    target.sessionId = 7;
    CHECK_TRUE(runtime.sendCurrentPatch(values, target).ok());
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    blinker::bbp2::StatePatchBody report;
    CHECK_TRUE(blinker::bbp2::decodeStatePatchBody(
                   response.body,
                   report)
                   .ok());
    CHECK_EQ(sharedRevision, report.revision);
    CHECK_TRUE(runtime.sendCurrentPatch(values, target).ok());
    CHECK_EQ(sharedRevision, runtime.stateRevision());

    runtime.stop();
}

void testReliablePatchOutbox() {
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite));
    const blinker::EndpointDescriptor* storage[1] = {};
    blinker::EndpointRegistry endpoints(storage, 1);
    CHECK_TRUE(endpoints.add(endpoint).ok());

    FakeClock clock;
    FakeTransport transport;
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(transport).ok());
    uint8_t slotStorage[256] = {};
    blinker::ReliableFrameSlot slots[1];
    slots[0].storage =
        blinker::MutableByteSpan(slotStorage, sizeof(slotStorage));
    blinker::ReliableRetryPolicy policy;
    policy.initialDelayMillis = 100;
    policy.maximumDelayMillis = 200;
    policy.overallTimeoutMillis = 1000;
    policy.maxAttempts = 2;
    blinker::ReliableOutbox outbox(hub, clock, slots, 1, policy);
    DeliveryCapture delivery;
    outbox.setHandler(&captureDelivery, &delivery);

    uint8_t transmitBuffer[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(transmitBuffer);
    config.maxReassemblySize = sizeof(transmitBuffer);
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(transmitBuffer, sizeof(transmitBuffer)),
        config);
    runtime.setReliableOutbox(&outbox);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000);

    uint8_t bodyBuffer[128] = {};
    uint8_t frameBuffer[256] = {};
    blinker::ByteView body;
    blinker::ByteView frame;
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1;
    peerHello.features =
        blinker::bbp2::FeatureReliableDelivery |
        blinker::bbp2::FeatureStateRevision;
    peerHello.reliableReceiveWindow = 1;
    peerHello.maxFrameSize = 256;
    peerHello.maxReassemblySize = 256;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   200,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);

    blinker::bbp2::KeyedBodyWriter patchWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(patchWriter.begin(1).ok());
    CHECK_TRUE(patchWriter.writeBool(
                   blinker::StringView("lamp", 4), true).ok());
    CHECK_TRUE(patchWriter.finish(body).ok());
    blinker::SendTarget target;
    target.kind = blinker::SendTargetKind::Session;
    target.transportId = 0;
    target.sessionId = 7;
    uint16_t sequence = 0;
    const uint32_t sendsBefore = transport.sendCount;
    CHECK_TRUE(runtime.sendReliablePatch(body, target, sequence).ok());
    CHECK_TRUE(sequence != 0U);
    CHECK_EQ(static_cast<size_t>(1), outbox.pendingCount());
    CHECK_EQ(sendsBefore + 1U, transport.sendCount);
    uint8_t firstFrame[256] = {};
    const size_t firstFrameSize = transport.lastSentSize;
    std::memcpy(firstFrame, transport.lastSent, firstFrameSize);
    blinker::bbp2::FrameView sent;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(firstFrame, firstFrameSize), sent).ok());
    CHECK_EQ(sequence, sent.header.sequence);
    CHECK_TRUE(
        (sent.header.flags & blinker::bbp2::FlagAckRequired) != 0U);
    blinker::bbp2::StatePatchBody sentPatch;
    CHECK_TRUE(blinker::bbp2::decodeStatePatchBody(
                   sent.body,
                   sentPatch)
                   .ok());
    CHECK_TRUE(sentPatch.mode == blinker::bbp2::StatePatchMode::Report);
    CHECK_EQ(static_cast<uint32_t>(1), sentPatch.revision);
    CHECK_EQ(static_cast<uint32_t>(1), runtime.stateRevision());

    uint16_t blockedSequence = 123;
    CHECK_EQ(
        blinker::ErrorCode::WouldBlock,
        runtime.sendReliablePatch(body, target, blockedSequence).code());
    CHECK_EQ(static_cast<uint16_t>(0), blockedSequence);

    clock.advance(99);
    runtime.poll(1000);
    CHECK_EQ(sendsBefore + 1U, transport.sendCount);
    clock.advance(1);
    runtime.poll(1000);
    CHECK_EQ(sendsBefore + 2U, transport.sendCount);
    CHECK_EQ(firstFrameSize, transport.lastSentSize);
    CHECK_TRUE(std::memcmp(
                   firstFrame,
                   transport.lastSent,
                   firstFrameSize) == 0);

    blinker::bbp2::AckBody ack;
    ack.acknowledgedSequence = sequence;
    CHECK_TRUE(blinker::bbp2::encodeAckBody(
                   ack,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Ack,
                   blinker::bbp2::FlagIsResponse,
                   201,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 8);
    CHECK_EQ(static_cast<size_t>(1), outbox.pendingCount());
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<size_t>(0), outbox.pendingCount());
    CHECK_EQ(static_cast<uint32_t>(1), delivery.calls);
    CHECK_TRUE(
        delivery.last.outcome ==
        blinker::ReliableDeliveryOutcome::Acknowledged);
    CHECK_EQ(sequence, delivery.last.sequence);

    CHECK_TRUE(runtime.sendReliablePatch(body, target, sequence).code() ==
               blinker::ErrorCode::InvalidEncoding);
    blinker::bbp2::KeyedBodyWriter secondPatchWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(secondPatchWriter.begin(1).ok());
    CHECK_TRUE(secondPatchWriter.writeBool(
                   blinker::StringView("lamp", 4), false).ok());
    CHECK_TRUE(secondPatchWriter.finish(body).ok());
    CHECK_TRUE(runtime.sendReliablePatch(body, target, sequence).ok());
    blinker::bbp2::ErrorBody error;
    error.errorCode = static_cast<uint16_t>(
        blinker::WireError::CommandRejected);
    error.relatedSequence = sequence;
    error.hasRelatedSequence = true;
    CHECK_TRUE(blinker::bbp2::encodeErrorBody(
                   error,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Error,
                   blinker::bbp2::FlagIsResponse,
                   202,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 7);
    CHECK_EQ(static_cast<size_t>(0), outbox.pendingCount());
    CHECK_EQ(static_cast<uint32_t>(2), delivery.calls);
    CHECK_TRUE(
        delivery.last.outcome ==
        blinker::ReliableDeliveryOutcome::PeerRejected);
    CHECK_EQ(error.errorCode, delivery.last.peerError);

    blinker::bbp2::KeyedBodyWriter timeoutPatchWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(timeoutPatchWriter.begin(1).ok());
    CHECK_TRUE(timeoutPatchWriter.writeBool(
                   blinker::StringView("lamp", 4), true).ok());
    CHECK_TRUE(timeoutPatchWriter.finish(body).ok());
    CHECK_TRUE(runtime.sendReliablePatch(body, target, sequence).ok());
    clock.advance(100);
    runtime.poll(1000);
    clock.advance(200);
    runtime.poll(1000);
    CHECK_EQ(static_cast<size_t>(0), outbox.pendingCount());
    CHECK_EQ(static_cast<uint32_t>(3), delivery.calls);
    CHECK_TRUE(
        delivery.last.outcome ==
        blinker::ReliableDeliveryOutcome::RetryExhausted);

    CHECK_TRUE(runtime.sendReliablePatch(body, target, sequence).ok());
    transport.currentState = blinker::TransportState::Backoff;
    runtime.poll(1000);
    CHECK_EQ(static_cast<size_t>(0), outbox.pendingCount());
    CHECK_EQ(static_cast<uint32_t>(4), delivery.calls);
    CHECK_TRUE(
        delivery.last.outcome ==
        blinker::ReliableDeliveryOutcome::SessionClosed);
    runtime.stop();
}

void testControllerControlRuntime() {
    {
        const blinker::EndpointDescriptor* limitedStorage[1] = {};
        blinker::EndpointRegistry limitedEndpoints(limitedStorage, 1U);
        FakeTransport limitedTransport;
        blinker::TransportHub limitedHub;
        AuthorizationCapture limitedAuthorization;
        ControllerControlCapture limitedControl;
        limitedTransport.featureFlags |= blinker::TransportFeatureLocal;
        CHECK_TRUE(limitedHub.addTransport(limitedTransport,
            blinker::TransportLifecyclePolicy::Managed,
            blinker::TransportAccess(&limitedAuthorization, &limitedControl)).ok());
        uint8_t limitedBuffer[
            blinker::bbp2::kControllerMutationMaximumFrameSize - 1U] = {};
        blinker::DeviceRuntimeConfig limitedConfig;
        limitedConfig.maxFrameSize = sizeof(limitedBuffer);
        limitedConfig.maxReassemblySize = sizeof(limitedBuffer);
        blinker::DeviceRuntime limitedRuntime(
            limitedHub,
            limitedEndpoints,
            blinker::MutableByteSpan(
                limitedBuffer,
                sizeof(limitedBuffer)),
            limitedConfig);
        CHECK_EQ(
            blinker::ErrorCode::NotConfigured,
            limitedRuntime.start().code());
    }

    const blinker::EndpointDescriptor* storage[1] = {};
    blinker::EndpointRegistry endpoints(storage, 1U);
    FakeTransport transport;
    transport.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary |
        blinker::TransportFeatureLocal |
        blinker::TransportFeatureAuthenticated);
    blinker::TransportHub hub;
    AuthorizationCapture authorization;
    authorization.grantedPermissions = blinker::kAuthorizationPermissionControl;
    ControllerControlCapture controllerControl;
    CHECK_TRUE(hub.addTransport(transport, blinker::TransportLifecyclePolicy::Managed,
        blinker::TransportAccess(&authorization, &controllerControl)).ok());
    uint8_t transmitBuffer[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(transmitBuffer);
    config.maxReassemblySize = sizeof(transmitBuffer);
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(transmitBuffer, sizeof(transmitBuffer)),
        config);
    AuthorizationCapture otherAuthorization;
    ControllerControlCapture otherControl;
    CHECK_EQ(blinker::ErrorCode::AlreadyExists,
        hub.addTransport(transport, blinker::TransportLifecyclePolicy::Managed,
            blinker::TransportAccess(&otherAuthorization, &otherControl)).code());
    CHECK_TRUE(hub.accessAt(0U).authorization == &authorization);
    CHECK_TRUE(hub.accessAt(0U).control == &controllerControl);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000U);

    blinker::bbp2::FrameView outbound;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound).ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello),
        outbound.header.kind);
    blinker::bbp2::HelloBody deviceHello;
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(
                   outbound.body,
                   deviceHello).ok());
    CHECK_TRUE((deviceHello.features &
                blinker::bbp2::FeatureControllerControl) != 0U);

    uint8_t bodyBuffer[blinker::bbp2::kControllerMutationMaxEncodedSize] = {};
    uint8_t frameBuffer[256] = {};
    blinker::ByteView body;
    blinker::ByteView frame;
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1U;
    peerHello.features =
        blinker::bbp2::FeatureAuthentication |
        blinker::bbp2::FeatureControllerControl;
    peerHello.maxFrameSize = sizeof(frameBuffer);
    peerHello.maxReassemblySize = sizeof(frameBuffer);
    peerHello.authorizationMethods[0] = 1U;
    peerHello.authorizationMethodCount = 1U;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   400U,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame).ok());
    transport.inject(frame, 0U, false, 77U, true, true);

    CHECK_TRUE(blinker::bbp2::encodeControllerControlOpenBody(
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body).ok());
    CHECK_EQ(
        static_cast<size_t>(
            blinker::bbp2::kControllerControlOpenMaxEncodedSize),
        body.size);
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::ControllerControlOpen,
                   blinker::bbp2::FlagNone,
                   401U,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame).ok());
    // A transport-level authenticated flag must not authorize a signed
    // controller Grant relay. Explicit Method 2 Control permission is still
    // required before the server-signed Grant is decoded and verified.
    transport.inject(frame, 0U, true, 77U, true, true);
    CHECK_EQ(static_cast<uint32_t>(0U), controllerControl.beginCalls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound).ok());
    blinker::bbp2::ErrorBody wireError;
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(
                   outbound.body,
                   wireError).ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::AuthenticationRequired),
        wireError.errorCode);

    const uint8_t proof[] = {0xA5U};
    blinker::bbp2::AuthRequestBody authRequest;
    authRequest.method = 1U;
    authRequest.payload = blinker::ByteView(proof, sizeof(proof));
    CHECK_TRUE(blinker::bbp2::encodeAuthRequestBody(
                   authRequest,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Authenticate,
                   blinker::bbp2::FlagNone,
                   402U,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame).ok());
    transport.inject(frame, 0U, false, 77U, true, true);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound).ok());
    blinker::bbp2::AuthResultBody authResult;
    CHECK_TRUE(blinker::bbp2::decodeAuthResultBody(
                   outbound.body,
                   authResult).ok());
    CHECK_TRUE(authResult.status == blinker::bbp2::AuthStatus::Authorized);

    CHECK_TRUE(blinker::bbp2::encodeControllerControlOpenBody(
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::ControllerControlOpen,
                   blinker::bbp2::FlagNone,
                   403U,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame).ok());
    transport.inject(frame, 0U, false, 77U, false, true);
    CHECK_EQ(static_cast<uint32_t>(0U), controllerControl.beginCalls);
    transport.inject(frame, 0U, false, 77U, true, true);
    CHECK_EQ(static_cast<uint32_t>(1U), controllerControl.beginCalls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound).ok());
    CHECK_EQ(
        static_cast<uint8_t>(
            blinker::bbp2::MessageKind::ControllerControlChallenge),
        outbound.header.kind);
    CHECK_EQ(static_cast<uint16_t>(403U), outbound.header.sequence);
    blinker::bbp2::ControllerControlChallengeBody challenge;
    CHECK_TRUE(blinker::bbp2::decodeControllerControlChallengeBody(
                   outbound.body,
                   challenge).ok());
    CHECK_TRUE(std::memcmp(
                   challenge.controlNonce.data,
                   controllerControl.nonce,
                   sizeof(controllerControl.nonce)) == 0);

    uint8_t maximumGrant[blinker::bbp2::kControllerGrantMaximumSize] = {};
    uint8_t controllerSecret[
        blinker::bbp2::kControllerCredentialSecretSize] = {};
    maximumGrant[0] = 0x80U;
    controllerSecret[0] = 1U;
    blinker::bbp2::ControllerMutationBody mutation;
    mutation.grant = blinker::ByteView(maximumGrant, sizeof(maximumGrant));
    mutation.controllerSecret =
        blinker::ByteView(controllerSecret, sizeof(controllerSecret));
    CHECK_TRUE(blinker::bbp2::encodeControllerMutationBody(
                   mutation,
                   blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)),
                   body).ok());
    CHECK_EQ(
        static_cast<size_t>(
            blinker::bbp2::kControllerMutationMaxEncodedSize),
        body.size);
    CHECK_TRUE(body.size + blinker::bbp2::kBaseHeaderSize <=
               sizeof(frameBuffer));
    blinker::bbp2::ControllerMutationBody decodedMutation;
    CHECK_TRUE(blinker::bbp2::decodeControllerMutationBody(
                   body,
                   decodedMutation).ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::ControllerMutation,
                   blinker::bbp2::FlagNone,
                   404U,
                   body,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame).ok());
    transport.inject(frame, 0U, false, 77U, true, true);
    CHECK_EQ(static_cast<uint32_t>(1U), controllerControl.mutationCalls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound).ok());
    CHECK_EQ(
        static_cast<uint8_t>(
            blinker::bbp2::MessageKind::ControllerMutationReceipt),
        outbound.header.kind);
    CHECK_EQ(static_cast<uint16_t>(404U), outbound.header.sequence);
    CHECK_EQ(static_cast<size_t>(1U), outbound.body.size);
    CHECK_EQ(static_cast<uint8_t>(0xA0U), outbound.body.data[0]);

    transport.disconnect(77U);
    CHECK_EQ(static_cast<uint32_t>(0U), otherAuthorization.calls);
    CHECK_EQ(static_cast<uint32_t>(0U), otherControl.beginCalls);
    CHECK_EQ(static_cast<uint32_t>(1U), controllerControl.closeCalls);
    CHECK_EQ(static_cast<uint32_t>(77U), controllerControl.lastSessionId);
    CHECK_EQ(static_cast<uint32_t>(1U), authorization.closedCalls);
    runtime.stop();
    CHECK_EQ(static_cast<uint32_t>(1U), controllerControl.closeCalls);
}

void testLocalAuthorizationRuntime() {
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite));
    const blinker::EndpointDescriptor* storage[1] = {};
    blinker::EndpointRegistry endpoints(storage, 1);
    CHECK_TRUE(endpoints.add(endpoint).ok());

    FakeTransport transport;
    transport.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary |
        blinker::TransportFeatureLocal |
        blinker::TransportFeatureAuthenticated);
    blinker::TransportHub hub;
    AuthorizationCapture authorizationCapture;
    CHECK_TRUE(hub.addTransport(transport, blinker::TransportLifecyclePolicy::Managed,
        blinker::TransportAccess(&authorizationCapture)).ok());
    uint8_t transmitBuffer[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(transmitBuffer);
    config.maxReassemblySize = sizeof(transmitBuffer);
    config.maxAuthorizationAttempts = 2;
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(transmitBuffer, sizeof(transmitBuffer)),
        config);
    CommandCapture commandCapture;
    runtime.setCommandHandler(&captureCommand, &commandCapture);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000);

    uint8_t helloBodyBuffer[96] = {};
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1;
    peerHello.features =
        blinker::bbp2::FeatureAuthentication |
        blinker::bbp2::FeatureManifest;
    peerHello.maxFrameSize = sizeof(transmitBuffer);
    peerHello.maxReassemblySize = sizeof(transmitBuffer);
    peerHello.authorizationMethods[0] = 1U;
    peerHello.authorizationMethodCount = 1U;
    blinker::ByteView helloBody;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(
                       helloBodyBuffer,
                       sizeof(helloBodyBuffer)),
                   helloBody)
                   .ok());

    uint8_t commandBodyBuffer[32] = {};
    blinker::bbp2::KeyedBodyWriter commandWriter(
        blinker::MutableByteSpan(
            commandBodyBuffer,
            sizeof(commandBodyBuffer)));
    CHECK_TRUE(commandWriter.begin(1).ok());
    CHECK_TRUE(commandWriter.writeBool(
                   blinker::StringView("lamp", 4),
                   true)
                   .ok());
    blinker::ByteView commandBody;
    CHECK_TRUE(commandWriter.finish(commandBody).ok());

    uint8_t frameBuffer[256] = {};
    blinker::ByteView frame;
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   30,
                   helloBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 77, true, true);

    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagNone,
                   31,
                   commandBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 77, true, true);
    CHECK_EQ(static_cast<uint32_t>(0), commandCapture.calls);
    blinker::bbp2::FrameView outbound;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    blinker::bbp2::ErrorBody wireError;
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(
                   outbound.body,
                   wireError)
                   .ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::AuthenticationRequired),
        wireError.errorCode);

    uint8_t authBodyBuffer[64] = {};
    blinker::bbp2::AuthRequestBody authRequest;
    authRequest.method = 1;
    authRequest.payload = blinker::ByteView();
    blinker::ByteView authBody;
    CHECK_TRUE(blinker::bbp2::encodeAuthRequestBody(
                   authRequest,
                   blinker::MutableByteSpan(
                       authBodyBuffer,
                       sizeof(authBodyBuffer)),
                   authBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Authenticate,
                   blinker::bbp2::FlagNone,
                   32,
                   authBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 77, false, true);
    CHECK_EQ(static_cast<uint32_t>(0), authorizationCapture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(
                   outbound.body,
                   wireError)
                   .ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::AuthenticationRequired),
        wireError.errorCode);

    transport.inject(frame, 0, false, 77, true, true);
    CHECK_EQ(static_cast<uint32_t>(1), authorizationCapture.calls);
    CHECK_EQ(static_cast<uint32_t>(77), authorizationCapture.lastSessionId);
    CHECK_TRUE(authorizationCapture.sawEncrypted);
    CHECK_TRUE(authorizationCapture.sawBonded);
    CHECK_TRUE(!authorizationCapture.sawTransportAuthentication);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::AuthResult),
        outbound.header.kind);
    blinker::bbp2::AuthResultBody authResult;
    CHECK_TRUE(blinker::bbp2::decodeAuthResultBody(
                   outbound.body,
                   authResult)
                   .ok());
    CHECK_TRUE(authResult.status == blinker::bbp2::AuthStatus::Continue);
    CHECK_TRUE(authResult.hasPayload);
    CHECK_EQ(static_cast<size_t>(2), authResult.payload.size);

    const uint8_t proof[] = {0xA5};
    authRequest.payload = blinker::ByteView(proof, sizeof(proof));
    CHECK_TRUE(blinker::bbp2::encodeAuthRequestBody(
                   authRequest,
                   blinker::MutableByteSpan(
                       authBodyBuffer,
                       sizeof(authBodyBuffer)),
                   authBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Authenticate,
                   blinker::bbp2::FlagNone,
                   33,
                   authBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 77, true, true);
    CHECK_EQ(static_cast<uint32_t>(2), authorizationCapture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeAuthResultBody(
                   outbound.body,
                   authResult)
                   .ok());
    CHECK_TRUE(authResult.status == blinker::bbp2::AuthStatus::Authorized);
    CHECK_EQ(static_cast<uint32_t>(1U), authorizationCapture.queuedCalls);
    CHECK_EQ(static_cast<uint32_t>(0U), authorizationCapture.closedCalls);

    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagNone,
                   34,
                   commandBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 77, true, true);
    CHECK_EQ(static_cast<uint32_t>(1), commandCapture.calls);

    blinker::bbp2::ManifestRequestBody manifestRequest;
    manifestRequest.cursor = 0U;
    CHECK_TRUE(blinker::bbp2::encodeManifestRequestBody(
                   manifestRequest,
                   blinker::MutableByteSpan(
                       authBodyBuffer,
                       sizeof(authBodyBuffer)),
                   authBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::ManifestRequest,
                   blinker::bbp2::FlagNone,
                   340,
                   authBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 77, true, true);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(
                   outbound.body,
                   wireError)
                   .ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::AuthenticationRequired),
        wireError.errorCode);

    transport.inject(frame, 0, false, 77, false, true);
    CHECK_EQ(static_cast<uint32_t>(1), commandCapture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeErrorBody(
                   outbound.body,
                   wireError)
                   .ok());
    CHECK_EQ(
        static_cast<uint16_t>(blinker::WireError::AuthenticationRequired),
        wireError.errorCode);

    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   35,
                   helloBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 78, true, false);
    const uint8_t invalidProof[] = {0x00};
    authRequest.payload =
        blinker::ByteView(invalidProof, sizeof(invalidProof));
    CHECK_TRUE(blinker::bbp2::encodeAuthRequestBody(
                   authRequest,
                   blinker::MutableByteSpan(
                       authBodyBuffer,
                       sizeof(authBodyBuffer)),
                   authBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Authenticate,
                   blinker::bbp2::FlagNone,
                   36,
                   authBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 78, true, false);
    CHECK_EQ(static_cast<uint32_t>(3), authorizationCapture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeAuthResultBody(
                   outbound.body,
                   authResult)
                   .ok());
    CHECK_TRUE(authResult.status == blinker::bbp2::AuthStatus::Rejected);
    transport.inject(frame, 0, false, 78, true, false);
    CHECK_EQ(static_cast<uint32_t>(4), authorizationCapture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeAuthResultBody(
                   outbound.body,
                   authResult)
                   .ok());
    CHECK_TRUE(authResult.status == blinker::bbp2::AuthStatus::Locked);
    transport.inject(frame, 0, false, 78, true, false);
    CHECK_EQ(static_cast<uint32_t>(4), authorizationCapture.calls);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeAuthResultBody(
                   outbound.body,
                   authResult)
                   .ok());
    CHECK_TRUE(authResult.status == blinker::bbp2::AuthStatus::Locked);

    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagNone,
                   37,
                   commandBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 78, true, false);
    CHECK_EQ(static_cast<uint32_t>(1), commandCapture.calls);

    authorizationCapture.grantedPermissions =
        blinker::kAuthorizationPermissionObserve;
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   371,
                   helloBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 80, true, true);
    authRequest.payload = blinker::ByteView(proof, sizeof(proof));
    CHECK_TRUE(blinker::bbp2::encodeAuthRequestBody(
                   authRequest,
                   blinker::MutableByteSpan(
                       authBodyBuffer,
                       sizeof(authBodyBuffer)),
                   authBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Authenticate,
                   blinker::bbp2::FlagNone,
                   372,
                   authBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 80, true, true);
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagNone,
                   373,
                   commandBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 80, true, true);
    CHECK_EQ(static_cast<uint32_t>(1), commandCapture.calls);
    CHECK_TRUE(blinker::bbp2::encodeManifestRequestBody(
                   manifestRequest,
                   blinker::MutableByteSpan(
                       authBodyBuffer,
                       sizeof(authBodyBuffer)),
                   authBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::ManifestRequest,
                   blinker::bbp2::FlagNone,
                   374,
                   authBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, false, 80, true, true);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Manifest),
        outbound.header.kind);

    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   38,
                   helloBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 79, true, true);
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Command,
                   blinker::bbp2::FlagNone,
                   39,
                   commandBody,
                   blinker::MutableByteSpan(frameBuffer, sizeof(frameBuffer)),
                   frame)
                   .ok());
    transport.inject(frame, 0, true, 79, true, true);
    CHECK_EQ(static_cast<uint32_t>(2), commandCapture.calls);
    CHECK_EQ(static_cast<uint32_t>(79), commandCapture.sessionId);

    runtime.stop();
}


void testDualTransportRuntime(bool failLocalFirst) {
    static_assert(sizeof(blinker::RxContext) <= 12U,
                  "RxContext exceeds the request-scope resource gate");
    static_assert(sizeof(blinker::SendTarget) <= 8U,
                  "SendTarget exceeds the request-scope resource gate");
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite));
    const blinker::EndpointDescriptor* storage[1] = {};
    blinker::EndpointRegistry endpoints(storage, 1);
    CHECK_TRUE(endpoints.add(endpoint).ok());

    FakeTransport localBle;
    FakeTransport cloudMqtt;
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(localBle).ok());
    CHECK_TRUE(hub.addTransport(cloudMqtt).ok());
    uint8_t transmitBuffer[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(transmitBuffer);
    config.maxReassemblySize = sizeof(transmitBuffer);
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(
            transmitBuffer,
            sizeof(transmitBuffer)),
        config);
    runtime.setStateEncoder(&encodeTestState, nullptr);
    runtime.setStatePageEncoder(&encodeTestStatePage, nullptr);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000);
    CHECK_EQ(static_cast<uint32_t>(1), localBle.sendCount);
    CHECK_EQ(static_cast<uint32_t>(1), cloudMqtt.sendCount);

    uint8_t bodyBuffer[96] = {};
    blinker::bbp2::HelloBody hello;
    hello.role = blinker::bbp2::PeerRole::App;
    hello.versions[0] = blinker::bbp2::kVersion;
    hello.versionCount = 1;
    hello.features = blinker::bbp2::FeatureStateRevision;
    hello.maxFrameSize = 256;
    hello.maxReassemblySize = 256;
    blinker::ByteView body;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   hello,
                   blinker::MutableByteSpan(
                       bodyBuffer,
                       sizeof(bodyBuffer)),
                   body)
                   .ok());
    uint8_t frameBuffer[160] = {};
    blinker::ByteView frame;
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   20,
                   body,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    localBle.inject(frame, 0, true, 42);

    uint8_t stateRequestBuffer[16] = {};
    blinker::ByteView stateRequestBody;
    CHECK_TRUE(encodeTestStateRequest(
                   0,
                   0,
                   false,
                   blinker::MutableByteSpan(
                       stateRequestBuffer,
                       sizeof(stateRequestBuffer)),
                   stateRequestBody)
                   .ok());
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   21,
                   stateRequestBody,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    localBle.inject(frame, 0, true, 42);
    CHECK_EQ(static_cast<uint32_t>(2), localBle.sendCount);
    CHECK_TRUE(localBle.lastTarget.kind ==
               blinker::SendTargetKind::Session);
    CHECK_EQ(static_cast<uint8_t>(0), localBle.lastTarget.transportId);
    CHECK_EQ(static_cast<uint32_t>(42), localBle.lastTarget.sessionId);

    cloudMqtt.inject(frame, 0, true, 42);
    CHECK_EQ(static_cast<uint32_t>(2), cloudMqtt.sendCount);
    blinker::bbp2::FrameView outbound;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       cloudMqtt.lastSent,
                       cloudMqtt.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Error),
        outbound.header.kind);

    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagIsResponse,
                   22,
                   body,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    cloudMqtt.inject(frame, 0, true, 42);
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::StateRequest,
                   blinker::bbp2::FlagNone,
                   23,
                   stateRequestBody,
                   blinker::MutableByteSpan(
                       frameBuffer,
                       sizeof(frameBuffer)),
                   frame)
                   .ok());
    cloudMqtt.inject(frame, 0, true, 42);
    CHECK_EQ(static_cast<uint32_t>(3), cloudMqtt.sendCount);
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       cloudMqtt.lastSent,
                       cloudMqtt.lastSentSize),
                   outbound)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::StatePage),
        outbound.header.kind);
    CHECK_EQ(static_cast<uint8_t>(1), cloudMqtt.lastTarget.transportId);

    blinker::bbp2::KeyedBodyWriter valuesWriter(
        blinker::MutableByteSpan(bodyBuffer, sizeof(bodyBuffer)));
    CHECK_TRUE(valuesWriter.begin(1).ok());
    CHECK_TRUE(valuesWriter.writeBool(
                   blinker::StringView("lamp", 4),
                   true)
                   .ok());
    blinker::ByteView values;
    CHECK_TRUE(valuesWriter.finish(values).ok());
    uint32_t revision = 0;
    CHECK_TRUE(runtime.advanceStateRevision(revision).ok());
    blinker::SendTarget bleTarget;
    bleTarget.kind = blinker::SendTargetKind::Session;
    bleTarget.transportId = 0;
    bleTarget.sessionId = 42;
    blinker::SendTarget mqttTarget = bleTarget;
    mqttTarget.transportId = 1;
    CHECK_TRUE(runtime.sendCurrentPatch(values, bleTarget).ok());
    blinker::bbp2::FrameView bleReport;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       localBle.lastSent,
                       localBle.lastSentSize),
                   bleReport)
                   .ok());
    blinker::bbp2::StatePatchBody statePatch;
    CHECK_TRUE(blinker::bbp2::decodeStatePatchBody(
                   bleReport.body,
                   statePatch)
                   .ok());
    CHECK_EQ(revision, statePatch.revision);
    CHECK_TRUE(runtime.sendCurrentPatch(values, mqttTarget).ok());
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       cloudMqtt.lastSent,
                       cloudMqtt.lastSentSize),
                   outbound)
                   .ok());
    CHECK_TRUE(blinker::bbp2::decodeStatePatchBody(
                   outbound.body,
                   statePatch)
                   .ok());
    CHECK_EQ(revision, statePatch.revision);
    CHECK_EQ(revision, runtime.stateRevision());

    // A routing address is not authority: an unknown source/session cannot
    // receive application data and must not affect either valid peer.
    blinker::SendTarget wrongTarget = mqttTarget;
    wrongTarget.sessionId = 99U;
    const uint32_t bleBeforeWrongTarget = localBle.sendCount;
    const uint32_t mqttBeforeWrongTarget = cloudMqtt.sendCount;
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        runtime.sendCurrentPatch(values, wrongTarget).code());
    CHECK_EQ(bleBeforeWrongTarget, localBle.sendCount);
    CHECK_EQ(mqttBeforeWrongTarget, cloudMqtt.sendCount);

    // Proactive state is independent of the last inbound peer. Both eligible
    // sessions receive it; no inbound-driven isAlive/last-sender gate exists.
    CHECK_TRUE(runtime.broadcastPatch(values).ok());
    CHECK_EQ(bleBeforeWrongTarget + 1U, localBle.sendCount);
    CHECK_EQ(mqttBeforeWrongTarget + 1U, cloudMqtt.sendCount);

    // Closing one transport expires only that source scope. The other peer
    // remains usable and a stale target cannot redirect a response.
    FakeTransport& failed = failLocalFirst ? localBle : cloudMqtt;
    FakeTransport& remaining = failLocalFirst ? cloudMqtt : localBle;
    const blinker::SendTarget& failedTarget =
        failLocalFirst ? bleTarget : mqttTarget;
    const blinker::SendTarget& remainingTarget =
        failLocalFirst ? mqttTarget : bleTarget;
    failed.currentState = failLocalFirst ? blinker::TransportState::Stopped
                                         : blinker::TransportState::Error;
    runtime.poll(1000U);
    runtime.poll(1000U);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        runtime.sendCurrentPatch(values, failedTarget).code());
    const uint32_t beforePeerSend = remaining.sendCount;
    CHECK_TRUE(runtime.sendCurrentPatch(values, remainingTarget).ok());
    CHECK_EQ(beforePeerSend + 1U, remaining.sendCount);
    remaining.currentState = blinker::TransportState::Error;
    runtime.poll(1000U);
    runtime.stop();
}

void testTransportScopedAccess() {
    // Three actual Runtime paths; these fake providers test dispatch, not crypto.
    class SecureAuthorizer final : public AuthorizationCapture {
    public:
        bool establishesSecureTransport() const override { return true; }
    };
    AuthorizationCapture bleAuth;
    SecureAuthorizer lanAuth;
    lanAuth.methodCode = 2U;
    lanAuth.grantedPermissions = blinker::kAuthorizationPermissionControl |
        blinker::kAuthorizationPermissionManageControllers;
    ControllerControlCapture bleControl;
    ControllerControlCapture lanControl;
    lanControl.presenceEnabled = true;
    FakeTransport cloud, ble, lan, extra;
    cloud.featureFlags |= blinker::TransportFeatureCloud;
    ble.featureFlags |= blinker::TransportFeatureLocal;
    lan.featureFlags |= blinker::TransportFeatureLocal;
    blinker::TransportHub hub;
    const blinker::TransportLifecyclePolicy managed =
        blinker::TransportLifecyclePolicy::Managed;
    CHECK_EQ(blinker::ErrorCode::InvalidArgument,
        hub.addTransport(extra, managed, blinker::TransportAccess(nullptr, &bleControl)).code());
    CHECK_EQ(static_cast<size_t>(0U), hub.size());
    CHECK_TRUE(hub.addTransport(cloud).ok());
    CHECK_TRUE(hub.addTransport(ble, managed,
        blinker::TransportAccess(&bleAuth, &bleControl)).ok());
    CHECK_TRUE(hub.addTransport(lan, managed,
        blinker::TransportAccess(&lanAuth, &lanControl)).ok());
    CHECK_EQ(blinker::ErrorCode::AlreadyExists,
        hub.addTransport(ble, managed,
            blinker::TransportAccess(&lanAuth, &lanControl)).code());
    CHECK_EQ(blinker::ErrorCode::CapacityExceeded, hub.addTransport(extra).code());
    CHECK_TRUE(hub.accessAt(1U).authorization == &bleAuth);
    CHECK_TRUE(hub.accessAt(2U).control == &lanControl);
    CHECK_TRUE(hub.accessAt(99U).authorization == nullptr);

    const blinker::EndpointDescriptor* slots[1] = {};
    blinker::EndpointRegistry endpoints(slots, 1U);
    uint8_t tx[256] = {}, bodyBytes[256] = {}, frameBytes[256] = {};
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = sizeof(tx);
    config.maxReassemblySize = sizeof(tx);
    blinker::DeviceRuntime runtime(
        hub, endpoints, blinker::MutableByteSpan(tx, sizeof(tx)), config);
    CHECK_TRUE(runtime.start().ok());
    CHECK_EQ(blinker::ErrorCode::NotConfigured, hub.removeTransport(cloud).code());
    CHECK_EQ(blinker::ErrorCode::NotConfigured, hub.addTransport(extra).code());
    runtime.poll(1000U);
    CHECK_EQ(1U, bleAuth.resetCalls);
    CHECK_EQ(1U, lanAuth.resetCalls);

    const uint32_t accessFeatures = blinker::bbp2::FeatureAuthentication |
        blinker::bbp2::FeatureControllerControl | blinker::bbp2::FeaturePresenceKeyControl;
    auto hello = [&](FakeTransport& port) {
        blinker::bbp2::FrameView frame;
        blinker::bbp2::HelloBody value;
        CHECK_TRUE(blinker::bbp2::parseFrame(
            blinker::ByteView(port.lastSent, port.lastSentSize), frame).ok());
        CHECK_TRUE(blinker::bbp2::decodeHelloBody(frame.body, value).ok());
        return value;
    };
    CHECK_EQ(0U, hello(cloud).features & accessFeatures);
    CHECK_EQ(0U, hello(cloud).authorizationMethodCount);
    CHECK_EQ(1U, hello(ble).authorizationMethodCount);
    CHECK_EQ(1U, hello(ble).authorizationMethods[0]);
    CHECK_EQ(0U, hello(ble).features & blinker::bbp2::FeaturePresenceKeyControl);
    CHECK_EQ(2U, hello(lan).authorizationMethods[0]);
    CHECK_TRUE((hello(lan).features & blinker::bbp2::FeaturePresenceKeyControl) != 0U);

    uint16_t sequence = 1U;
    auto deliver = [&](FakeTransport& port, blinker::bbp2::MessageKind kind,
                       blinker::ByteView body, bool encrypted = true,
                       uint32_t sessionId = 77U) {
        blinker::ByteView frame;
        CHECK_TRUE(buildTestFrame(kind, blinker::bbp2::FlagNone, sequence++, body,
            blinker::MutableByteSpan(frameBytes, sizeof(frameBytes)), frame).ok());
        // Incoming transportId is deliberately wrong: Hub must normalize it.
        port.inject(frame, 99U, false, sessionId, encrypted, false);
    };
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1U;
    peerHello.features = accessFeatures;
    peerHello.maxFrameSize = sizeof(tx);
    peerHello.maxReassemblySize = sizeof(tx);
    peerHello.authorizationMethods[0] = 1U;
    peerHello.authorizationMethods[1] = 2U;
    peerHello.authorizationMethodCount = 2U;
    auto negotiate = [&](FakeTransport& port, uint32_t sid = 77U) {
        blinker::ByteView body;
        CHECK_TRUE(blinker::bbp2::encodeHelloBody(peerHello,
            blinker::MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body).ok());
        deliver(port, blinker::bbp2::MessageKind::Hello, body, true, sid);
    };
    negotiate(cloud);
    negotiate(ble);
    negotiate(lan);
    CHECK_EQ(0U, hello(cloud).features & accessFeatures);
    auto authenticate = [&](FakeTransport& port, uint16_t method, bool encrypted = true) {
        const uint8_t proof = 0xA5U;
        blinker::bbp2::AuthRequestBody request;
        request.method = method;
        request.payload = blinker::ByteView(&proof, 1U);
        blinker::ByteView body;
        CHECK_TRUE(blinker::bbp2::encodeAuthRequestBody(request,
            blinker::MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body).ok());
        deliver(port, blinker::bbp2::MessageKind::Authenticate, body, encrypted);
    };
    auto open = [&](FakeTransport& port, uint32_t sid = 77U) {
        blinker::ByteView body;
        CHECK_TRUE(blinker::bbp2::encodeControllerControlOpenBody(
            blinker::MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body).ok());
        deliver(port, blinker::bbp2::MessageKind::ControllerControlOpen, body, true, sid);
    };
    authenticate(ble, 2U); // Another transport's method never reaches a provider.
    authenticate(cloud, 1U);
    authenticate(ble, 1U, false); // LAN's secure-establishment flag cannot leak.
    CHECK_EQ(0U, bleAuth.calls);
    CHECK_EQ(0U, lanAuth.calls);
    authenticate(ble, 1U);
    CHECK_EQ(1U, bleAuth.calls);
    CHECK_EQ(1U, bleAuth.queuedCalls);
    CHECK_EQ(1U, bleAuth.lastTransportId);
    CHECK_EQ(77U, bleAuth.lastSessionId);
    open(lan);
    CHECK_EQ(0U, lanControl.beginCalls); // Same sessionId is not shared authority.
    open(ble);
    CHECK_EQ(1U, bleControl.beginCalls);
    authenticate(lan, 2U, false);
    CHECK_EQ(1U, lanAuth.calls);
    CHECK_EQ(1U, lanAuth.queuedCalls);
    CHECK_EQ(2U, lanAuth.lastTransportId);
    open(lan);
    CHECK_EQ(1U, lanControl.beginCalls);
    open(cloud);
    open(ble, 78U);
    CHECK_EQ(1U, bleControl.beginCalls);
    CHECK_EQ(1U, lanControl.beginCalls);

    const uint8_t emptyMap = 0xA0U;
    uint8_t secret[blinker::bbp2::kControllerCredentialSecretSize] = {};
    secret[0] = 1U;
    blinker::bbp2::ControllerMutationBody mutation;
    mutation.grant = blinker::ByteView(&emptyMap, 1U);
    mutation.controllerSecret = blinker::ByteView(secret, sizeof(secret));
    blinker::ByteView mutationBody;
    CHECK_TRUE(blinker::bbp2::encodeControllerMutationBody(mutation,
        blinker::MutableByteSpan(bodyBytes, sizeof(bodyBytes)), mutationBody).ok());
    deliver(ble, blinker::bbp2::MessageKind::ControllerMutation, mutationBody);
    CHECK_EQ(1U, bleControl.mutationCalls);
    CHECK_EQ(0U, lanControl.mutationCalls);
    deliver(lan, blinker::bbp2::MessageKind::ControllerMutation, mutationBody);
    CHECK_EQ(1U, bleControl.mutationCalls);
    CHECK_EQ(1U, lanControl.mutationCalls);
    deliver(cloud, blinker::bbp2::MessageKind::ControllerMutation, mutationBody);
    CHECK_EQ(1U, bleControl.mutationCalls);
    CHECK_EQ(1U, lanControl.mutationCalls);
    deliver(ble, blinker::bbp2::MessageKind::PresenceKeyMutation,
        blinker::ByteView(&emptyMap, 1U));
    CHECK_EQ(0U, bleControl.presenceCalls);
    CHECK_EQ(0U, lanControl.presenceCalls);
    deliver(lan, blinker::bbp2::MessageKind::PresenceKeyMutation,
        blinker::ByteView(&emptyMap, 1U));
    CHECK_EQ(1U, lanControl.presenceCalls);

    // Non-native failure closes only the bound session; ordinary Cloud has
    // no local control window or auth provider to reset.
    cloud.currentState = blinker::TransportState::Error;
    runtime.poll(1000U);
    CHECK_EQ(0U, bleAuth.closedCalls);
    CHECK_EQ(0U, lanAuth.closedCalls);
    ble.currentState = blinker::TransportState::Error;
    runtime.poll(1000U);
    CHECK_EQ(1U, bleAuth.closedCalls);
    CHECK_EQ(1U, bleControl.closeCalls);
    CHECK_EQ(0U, lanAuth.closedCalls);
    CHECK_EQ(1U, bleAuth.resetCalls);
    runtime.poll(1000U);
    CHECK_EQ(1U, bleAuth.closedCalls);
    open(lan);
    CHECK_EQ(2U, lanControl.beginCalls);
    lan.disconnect(77U);
    CHECK_EQ(1U, lanAuth.closedCalls);
    CHECK_EQ(1U, lanControl.closeCalls);
    CHECK_EQ(2U, lanControl.lastTransportId);
    runtime.stop();
    CHECK_EQ(1U, lanControl.closeCalls);

    // Removing the first slot reindexes transports and access services as one.
    CHECK_TRUE(hub.removeTransport(cloud).ok());
    CHECK_TRUE(hub.accessAt(0U).authorization == &bleAuth);
    CHECK_TRUE(hub.accessAt(1U).authorization == &lanAuth);
    CHECK_TRUE(hub.accessAt(2U).authorization == nullptr);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000U);
    CHECK_EQ(1U, hello(ble).authorizationMethods[0]);
    CHECK_EQ(2U, hello(lan).authorizationMethods[0]);
    negotiate(ble);
    negotiate(lan);
    authenticate(lan, 2U);
    CHECK_EQ(1U, lanAuth.lastTransportId);
    open(ble);
    CHECK_EQ(1U, bleControl.beginCalls); // Old permission did not survive restart.
    open(lan);
    CHECK_EQ(3U, lanControl.beginCalls);
    runtime.stop();
    CHECK_TRUE(hub.removeTransport(lan).ok());
    CHECK_TRUE(hub.addTransport(extra).ok());
    CHECK_TRUE(hub.accessAt(1U).authorization == nullptr);
    CHECK_TRUE(hub.accessAt(1U).control == nullptr);

    // Reject a local auth binding on a cloud-only path before starting IO.
    blinker::TransportHub invalidHub;
    CHECK_TRUE(invalidHub.addTransport(cloud, managed,
        blinker::TransportAccess(&bleAuth)).ok());
    blinker::DeviceRuntime invalid(invalidHub, endpoints,
        blinker::MutableByteSpan(tx, sizeof(tx)), config);
    CHECK_EQ(blinker::ErrorCode::NotConfigured, invalid.start().code());

    // A provider explicitly shared by two paths is reset once, not once per
    // slot. Real providers must themselves support exact session contexts.
    blinker::TransportHub sharedHub;
    CHECK_TRUE(sharedHub.addTransport(ble, managed,
        blinker::TransportAccess(&bleAuth)).ok());
    CHECK_TRUE(sharedHub.addTransport(lan, managed,
        blinker::TransportAccess(&bleAuth)).ok());
    blinker::DeviceRuntime shared(sharedHub, endpoints,
        blinker::MutableByteSpan(tx, sizeof(tx)), config);
    const uint32_t beforeReset = bleAuth.resetCalls;
    CHECK_TRUE(shared.start().ok());
    CHECK_EQ(beforeReset + 1U, bleAuth.resetCalls);
    shared.stop();
    CHECK_EQ(beforeReset + 2U, bleAuth.resetCalls);

    // Failed transport startup unlocks registration and retains the binding
    // for an explicit retry, without mutating another path's owner.
    lan.startResult = blinker::Result::failure(blinker::ErrorCode::InternalError);
    CHECK_EQ(blinker::ErrorCode::InternalError, shared.start().code());
    CHECK_TRUE(sharedHub.removeTransport(lan).ok());
    CHECK_TRUE(sharedHub.accessAt(0U).authorization == &bleAuth);
    CHECK_TRUE(sharedHub.accessAt(1U).authorization == nullptr);
    CHECK_TRUE(shared.start().ok());
    shared.stop();
}

void testFrameRoundTrip() {
    blinker::bbp2::FrameHeader header;
    header.kind =
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Command);
    header.flags = blinker::bbp2::FlagAckRequired;
    header.sequence = 0x1234;
    header.bodyLength = 3;

    uint8_t frameBytes[13] = {};
    const blinker::Result encoded = blinker::bbp2::encodeHeader(
        header,
        blinker::MutableByteSpan(frameBytes, sizeof(frameBytes)));
    CHECK_TRUE(encoded.ok());

    frameBytes[10] = 0xA1;
    frameBytes[11] = 0x01;
    frameBytes[12] = 0xF5;

    blinker::bbp2::FrameView frame;
    const blinker::Result parsed = blinker::bbp2::parseFrame(
        blinker::ByteView(frameBytes, sizeof(frameBytes)),
        frame);

    CHECK_TRUE(parsed.ok());
    CHECK_EQ(static_cast<uint8_t>(blinker::bbp2::MessageKind::Command),
             frame.header.kind);
    CHECK_EQ(static_cast<uint16_t>(0x1234), frame.header.sequence);
    CHECK_EQ(static_cast<size_t>(3), frame.body.size);
    CHECK_EQ(static_cast<uint8_t>(0xA1), frame.body.data[0]);
}

void testFrameValidation() {
    uint8_t bytes[10] = {
        blinker::bbp2::kMagic0,
        blinker::bbp2::kMagic1,
        blinker::bbp2::kVersion,
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello),
        0,
        blinker::bbp2::kBaseHeaderSize,
        0,
        1,
        0,
        0
    };

    blinker::bbp2::FrameView frame;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(bytes, sizeof(bytes)), frame)
                   .ok());

    bytes[0] = 0;
    CHECK_EQ(blinker::ErrorCode::InvalidMagic,
             blinker::bbp2::parseFrame(
                 blinker::ByteView(bytes, sizeof(bytes)), frame)
                 .code());
    bytes[0] = blinker::bbp2::kMagic0;

    bytes[2] = 3;
    CHECK_EQ(blinker::ErrorCode::UnsupportedVersion,
             blinker::bbp2::parseFrame(
                 blinker::ByteView(bytes, sizeof(bytes)), frame)
                 .code());
    bytes[2] = blinker::bbp2::kVersion;

    bytes[4] = 0x80;
    CHECK_EQ(blinker::ErrorCode::UnsupportedFeature,
             blinker::bbp2::parseFrame(
                 blinker::ByteView(bytes, sizeof(bytes)), frame)
                 .code());
    bytes[4] = 0;

    bytes[8] = 0;
    bytes[9] = 1;
    CHECK_EQ(blinker::ErrorCode::TruncatedInput,
             blinker::bbp2::parseFrame(
                 blinker::ByteView(bytes, sizeof(bytes)), frame)
                 .code());
    bytes[9] = 0;

    uint8_t trailing[11] = {};
    std::memcpy(trailing, bytes, sizeof(bytes));
    CHECK_EQ(blinker::ErrorCode::TrailingData,
             blinker::bbp2::parseFrame(
                 blinker::ByteView(trailing, sizeof(trailing)), frame)
                 .code());

    bytes[4] = blinker::bbp2::FlagHasCrc32c;
    CHECK_EQ(blinker::ErrorCode::UnsupportedFeature,
             blinker::bbp2::parseFrame(
                 blinker::ByteView(bytes, sizeof(bytes)), frame)
                 .code());
}

void testTransportHub() {
    FakeTransport first;
    FakeTransport second;
    FakeTransport external;
    first.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary |
        blinker::TransportFeatureCloud);
    second.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary |
        blinker::TransportFeatureLocal);
    uint8_t pollLog[2] = {};
    size_t pollLogSize = 0;
    first.pollMarker = 1;
    second.pollMarker = 2;
    first.pollLog = pollLog;
    second.pollLog = pollLog;
    first.pollLogSize = &pollLogSize;
    second.pollLogSize = &pollLogSize;
    external.currentState = blinker::TransportState::Online;
    external.startResult = blinker::Result::failure(
        blinker::ErrorCode::StateConflict);
    blinker::TransportHub hub;

    CHECK_TRUE(hub.addTransport(first).ok());
    CHECK_TRUE(hub.addTransport(second).ok());
    CHECK_TRUE(hub.addTransport(
                      external,
                      blinker::TransportLifecyclePolicy::External)
                   .ok());
    CHECK_EQ(blinker::ErrorCode::AlreadyExists,
             hub.addTransport(first).code());
    CHECK_EQ(static_cast<size_t>(3), hub.size());
    CHECK_TRUE(hub.startAll().ok());

    hub.poll(100);
    CHECK_EQ(static_cast<uint32_t>(1), first.pollCount);
    CHECK_EQ(static_cast<uint32_t>(1), second.pollCount);
    CHECK_EQ(static_cast<uint32_t>(0), external.pollCount);
    CHECK_EQ(static_cast<uint32_t>(50), first.lastBudget);
    CHECK_EQ(static_cast<uint32_t>(50), second.lastBudget);
    CHECK_EQ(static_cast<size_t>(2), pollLogSize);
    CHECK_EQ(static_cast<uint8_t>(2), pollLog[0]);
    CHECK_EQ(static_cast<uint8_t>(1), pollLog[1]);

    uint8_t frameBytes[10] = {
        blinker::bbp2::kMagic0,
        blinker::bbp2::kMagic1,
        blinker::bbp2::kVersion,
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello),
        0,
        blinker::bbp2::kBaseHeaderSize,
        0,
        0,
        0,
        0
    };

    ReceiveCapture capture;
    hub.setReceiver(&captureFrame, &capture);
    // TransportHub owns stable transport IDs and must not trust an adapter's
    // incoming ID value.
    second.inject(blinker::ByteView(frameBytes, sizeof(frameBytes)), 99);
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);
    CHECK_EQ(static_cast<uint8_t>(1), capture.transportId);
    CHECK_EQ(sizeof(frameBytes), capture.frameSize);

    CHECK_TRUE(
        hub.broadcast(blinker::ByteView(frameBytes, sizeof(frameBytes))).ok());
    CHECK_EQ(static_cast<uint32_t>(1), first.sendCount);
    CHECK_EQ(static_cast<uint32_t>(1), second.sendCount);
    CHECK_EQ(static_cast<uint32_t>(1), external.sendCount);

    CHECK_EQ(blinker::ErrorCode::NotConfigured,
             hub.removeTransport(external).code());
    CHECK_EQ(blinker::ErrorCode::NotConfigured,
             hub.addTransport(first).code());
    hub.stopAll();
    CHECK_TRUE(hub.removeTransport(external).ok());
    CHECK_EQ(static_cast<uint32_t>(0), external.stopCount);

    CHECK_TRUE(hub.removeTransport(first).ok());
    CHECK_EQ(static_cast<size_t>(1), hub.size());
    CHECK_EQ(static_cast<uint32_t>(2), first.stopCount);
    second.inject(blinker::ByteView(frameBytes, sizeof(frameBytes)), 99);
    CHECK_EQ(static_cast<uint32_t>(2), capture.calls);
    CHECK_EQ(static_cast<uint8_t>(0), capture.transportId);
    CHECK_EQ(blinker::ErrorCode::NotFound,
             hub.removeTransport(first).code());
}

void testMqttFrameTransport() {
    FakeClock clock;
    FakeMqttClient mqtt;
    blinker::MqttFrameTransportConfig config;
    config.maxFrameSize = 256;
    config.reconnectDelayMillis = 1000;
    config.publishQos = 1;

    blinker::MqttFrameTransport transport(mqtt, clock, config);

    blinker::SessionCredentials credentials;
    credentials.broker = blinker::BrokerKind::Blinker;
    credentials.host = blinker::StringView("broker.example.com", 18);
    credentials.port = 8883;
    credentials.security = blinker::MqttSecurity::Tls;
    credentials.deviceName = blinker::StringView("device-42", 9);
    credentials.clientId = blinker::StringView("device-42", 9);
    credentials.username = blinker::StringView("mqtt-user", 9);
    credentials.password = blinker::StringView("secret", 6);
    credentials.publishTopic =
        blinker::StringView("/device/device-42/s", 19);
    credentials.subscribeTopic =
        blinker::StringView("/device/device-42/r", 19);

    CHECK_TRUE(transport.configure(credentials).ok());
    CHECK_EQ(static_cast<uint32_t>(2), mqtt.capacityCheckCount);

    FakeMqttClient tooSmallMqtt;
    tooSmallMqtt.capacityResult =
        blinker::Result::failure(blinker::ErrorCode::BufferTooSmall);
    blinker::MqttFrameTransport tooSmallTransport(
        tooSmallMqtt,
        clock,
        config);
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        tooSmallTransport.configure(credentials).code());
    CHECK_EQ(
        static_cast<uint32_t>(1),
        tooSmallMqtt.capacityCheckCount);

    ReceiveCapture capture;
    BleSessionCapture sessions;
    transport.setReceiver(&captureFrame, &capture);
    transport.setSessionHandlers(
        &captureBleConnected,
        &captureBleDisconnected,
        &sessions);

    mqtt.connectResult =
        blinker::Result::failure(blinker::ErrorCode::NotConnected);
    transport.setNetworkAvailable(false);
    CHECK_TRUE(transport.start().ok());
    CHECK_TRUE(
        transport.state() == blinker::TransportState::Starting);
    CHECK_EQ(static_cast<uint32_t>(0), mqtt.connectCount);

    // An unavailable platform link keeps potentially blocking DNS/TCP/TLS work
    // out of poll(), so BLE can continue to run during WiFi association.
    transport.poll(100);
    CHECK_EQ(static_cast<uint32_t>(0), mqtt.connectCount);
    transport.setNetworkAvailable(true);
    // The first connection attempt is also deferred until cooperative polling,
    // so Client::begin() itself cannot starve BLE startup.
    transport.poll(100);
    CHECK_TRUE(
        transport.state() == blinker::TransportState::Backoff);
    CHECK_EQ(static_cast<uint32_t>(1), mqtt.connectCount);

    clock.advance(999);
    transport.poll(100);
    CHECK_EQ(static_cast<uint32_t>(1), mqtt.connectCount);

    mqtt.connectResult = blinker::Result::success();
    clock.advance(1);
    transport.poll(100);
    CHECK_EQ(static_cast<uint32_t>(2), mqtt.connectCount);
    CHECK_EQ(static_cast<uint32_t>(1), mqtt.subscribeCount);
    CHECK_TRUE(
        transport.state() == blinker::TransportState::Online);
    CHECK_EQ(static_cast<uint32_t>(1), sessions.connected);
    CHECK_EQ(static_cast<uint32_t>(0), sessions.disconnected);
    CHECK_TRUE(
        mqtt.lastConnectOptions.security == blinker::MqttSecurity::Tls);

    uint8_t validFrame[10] = {
        blinker::bbp2::kMagic0,
        blinker::bbp2::kMagic1,
        blinker::bbp2::kVersion,
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello),
        0,
        blinker::bbp2::kBaseHeaderSize,
        0,
        1,
        0,
        0
    };

    mqtt.inject(
        blinker::StringView("/wrong/topic", 12),
        blinker::ByteView(validFrame, sizeof(validFrame)));
    CHECK_EQ(static_cast<uint32_t>(0), capture.calls);

    mqtt.inject(
        credentials.subscribeTopic,
        blinker::ByteView(validFrame, sizeof(validFrame)),
        true);
    CHECK_EQ(static_cast<uint32_t>(0), capture.calls);

    uint8_t malformed[10] = {};
    mqtt.inject(
        credentials.subscribeTopic,
        blinker::ByteView(malformed, sizeof(malformed)));
    CHECK_EQ(static_cast<uint32_t>(0), capture.calls);

    mqtt.inject(
        credentials.subscribeTopic,
        blinker::ByteView(validFrame, sizeof(validFrame)));
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);
    CHECK_TRUE(capture.authenticated);

    blinker::SendTarget target;
    CHECK_TRUE(transport.send(
                   blinker::ByteView(validFrame, sizeof(validFrame)),
                   target)
                   .ok());
    CHECK_EQ(static_cast<uint32_t>(1), mqtt.publishCount);
    CHECK_EQ(sizeof(validFrame), mqtt.lastPublishedSize);
    CHECK_TRUE(!mqtt.lastPublishRetained);
    CHECK_EQ(static_cast<uint8_t>(1), mqtt.lastPublishQos);

    mqtt.connectedState = false;
    transport.poll(100);
    CHECK_TRUE(
        transport.state() == blinker::TransportState::Backoff);
    CHECK_EQ(static_cast<uint32_t>(1), sessions.disconnected);

    clock.advance(1000);
    transport.poll(100);
    CHECK_EQ(static_cast<uint32_t>(3), mqtt.connectCount);
    CHECK_TRUE(
        transport.state() == blinker::TransportState::Online);
    CHECK_EQ(static_cast<uint32_t>(2), sessions.connected);

    transport.stop();
    CHECK_TRUE(
        transport.state() == blinker::TransportState::Stopped);
    CHECK_EQ(static_cast<uint32_t>(2), sessions.disconnected);
}

void testPubSubClientAdapter() {
    PubSubClient native;
    blinker::PubSubClientAdapter<1> adapter(
        native,
        blinker::MqttSecurity::Tls,
        128);

    blinker::MqttConnectOptions connect;
    connect.host = blinker::StringView("broker.example.com", 18);
    connect.port = 8883;
    connect.security = blinker::MqttSecurity::Tls;
    connect.clientId = blinker::StringView("device", 6);
    connect.username = blinker::StringView("user", 4);
    connect.password = blinker::StringView("password", 8);
    connect.keepAliveSeconds = 45;

    CHECK_TRUE(adapter.connect(connect).ok());
    CHECK_EQ(static_cast<uint32_t>(1), native.setBufferCalls);
    CHECK_EQ(static_cast<uint16_t>(128), native.bufferSize);
    CHECK_EQ(static_cast<uint32_t>(1), native.setSocketTimeoutCalls);
    CHECK_EQ(static_cast<uint16_t>(2), native.socketTimeout);
    CHECK_EQ(static_cast<uint16_t>(8883), native.serverPort);
    CHECK_EQ(static_cast<uint16_t>(45), native.keepAlive);
    CHECK_TRUE(std::strcmp(native.serverHost, "broker.example.com") == 0);
    CHECK_TRUE(std::strcmp(native.clientId, "device") == 0);

    CHECK_TRUE(adapter.connect(connect).ok());
    CHECK_EQ(static_cast<uint32_t>(1), native.setBufferCalls);
    CHECK_EQ(static_cast<uint32_t>(1), native.setSocketTimeoutCalls);

    const char invalidHost[] = {'b', 'a', 'd', '\0', 'x'};
    blinker::MqttConnectOptions invalidConnect = connect;
    invalidConnect.host =
        blinker::StringView(invalidHost, sizeof(invalidHost));
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        adapter.connect(invalidConnect).code());

    const blinker::StringView topic("/device/device/r", 16);
    CHECK_TRUE(adapter.subscribe(topic, 1).ok());
    CHECK_EQ(static_cast<uint8_t>(1), native.subscribeQos);
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        adapter.subscribe(topic, 2).code());
    CHECK_TRUE(adapter.validatePacketCapacity(
                   topic,
                   105,
                   0,
                   blinker::MqttPacketDirection::Outbound)
                   .ok());
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        adapter.validatePacketCapacity(
            topic,
            106,
            0,
            blinker::MqttPacketDirection::Outbound)
            .code());
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        adapter.validatePacketCapacity(
            topic,
            104,
            1,
            blinker::MqttPacketDirection::Inbound)
            .code());

    uint8_t payload[32] = {};
    blinker::MqttPublishOptions publish;
    CHECK_TRUE(adapter.publish(
                   topic,
                   blinker::ByteView(payload, sizeof(payload)),
                   publish)
                   .ok());
    CHECK_EQ(static_cast<unsigned int>(sizeof(payload)),
             native.publishedLength);

    publish.qos = 1;
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        adapter.publish(
            topic,
            blinker::ByteView(payload, sizeof(payload)),
            publish)
            .code());

    publish.qos = 0;
    uint8_t tooLarge[120] = {};
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        adapter.publish(
            topic,
            blinker::ByteView(tooLarge, sizeof(tooLarge)),
            publish)
            .code());

    MqttCapture capture;
    adapter.setMessageHandler(&captureMqtt, &capture);
    char inboundTopic[] = "/device/device/r";
    uint8_t binary[] = {0x42, 0x00, 0xFF};
    native.inject(inboundTopic, binary, sizeof(binary));
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);
    CHECK_EQ(sizeof(binary), capture.payloadSize);
    CHECK_TRUE(!capture.retainKnown);

    blinker::PubSubClientAdapter<1> duplicateSlot(
        native,
        blinker::MqttSecurity::Tls,
        128);
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        duplicateSlot.connect(connect).code());

    PubSubClient plainNative;
    blinker::PubSubClientAdapter<2> plainAdapter(
        plainNative,
        blinker::MqttSecurity::PlainTcp,
        128);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        plainAdapter.connect(connect).code());

    PubSubClient noTimeoutNative;
    blinker::PubSubClientAdapter<3> noTimeoutAdapter(
        noTimeoutNative,
        blinker::MqttSecurity::Tls,
        128,
        0);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        noTimeoutAdapter.connect(connect).code());

    native.connectResult = false;
    adapter.disconnect();
    native.stateValue = MQTT_CONNECT_BAD_CREDENTIALS;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        adapter.connect(connect).code());
}

void testBleFragment() {
    CHECK_TRUE(std::strcmp(
                   blinker::ble::kServiceUuid,
                   "5f6d0001-3f5b-4e4f-9f4d-626c696e6b32") == 0);
    CHECK_TRUE(std::strcmp(
                   blinker::ble::kReceiveUuid,
                   "5f6d0002-3f5b-4e4f-9f4d-626c696e6b32") == 0);
    CHECK_TRUE(std::strcmp(
                   blinker::ble::kTransmitUuid,
                   "5f6d0003-3f5b-4e4f-9f4d-626c696e6b32") == 0);
    uint8_t packet[20] = {};
    blinker::ble::FragmentHeader header;
    header.flags = blinker::ble::FragmentFlagStart;
    header.frameId = 0x34;
    header.index = 0;
    CHECK_TRUE(blinker::ble::encodeFragmentHeader(
                   header,
                   blinker::MutableByteSpan(packet, sizeof(packet)))
                   .ok());
    for (size_t index = blinker::ble::kFragmentHeaderSize;
         index < sizeof(packet);
         ++index) {
        packet[index] = static_cast<uint8_t>(index);
    }
    const uint8_t expectedHeader[] = {
        0xB2, 0x11, 0x34, 0x00};
    CHECK_TRUE(std::memcmp(
                   packet,
                   expectedHeader,
                   sizeof(expectedHeader)) == 0);

    blinker::ble::FragmentView decoded;
    CHECK_TRUE(blinker::ble::parseFragment(
                   blinker::ByteView(packet, sizeof(packet)),
                   decoded)
                   .ok());
    CHECK_EQ(static_cast<uint8_t>(0x34), decoded.header.frameId);
    CHECK_EQ(static_cast<uint8_t>(0), decoded.header.index);
    CHECK_EQ(static_cast<size_t>(16), decoded.payload.size);

    packet[1] = 0x14;
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        blinker::ble::parseFragment(
            blinker::ByteView(packet, sizeof(packet)),
            decoded)
            .code());
    packet[1] = 0x21;
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedVersion,
        blinker::ble::parseFragment(
            blinker::ByteView(packet, sizeof(packet)),
            decoded)
            .code());
    packet[1] = 0x10;
    CHECK_EQ(
        blinker::ErrorCode::InvalidEncoding,
        blinker::ble::parseFragment(
            blinker::ByteView(packet, sizeof(packet)),
            decoded)
            .code());
    packet[1] = 0x11;
    packet[0] = 0;
    CHECK_EQ(
        blinker::ErrorCode::InvalidMagic,
        blinker::ble::parseFragment(
            blinker::ByteView(packet, sizeof(packet)),
            decoded)
            .code());

    header.flags = blinker::ble::FragmentFlagEnd;
    header.frameId = 0xFF;
    header.index = 0xFF;
    CHECK_TRUE(blinker::ble::encodeFragmentHeader(
                   header,
                   blinker::MutableByteSpan(packet, sizeof(packet)))
                   .ok());
    CHECK_TRUE(blinker::ble::parseFragment(
                   blinker::ByteView(packet, sizeof(packet)),
                   decoded)
                   .ok());
    CHECK_EQ(static_cast<uint8_t>(0xFF), decoded.header.index);
    static_assert(
        blinker::ble::kFragmentHeaderSize == 4U,
        "BLE fragment header must remain 4 bytes");
    static_assert(
        blinker::ble::kMaximumRecordSizeAtMinimumPacket == 4096U,
        "ATT20 must cover exactly 4096 bytes in 256 fragments");
}

void testBleFrameTransport() {
    FakeClock clock;
    UnsupportedAead crypto;
    uint8_t plaintext[4073] = {};
    FakeBleLink link;
    uint8_t rxStorage[88 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t txStorage[88 * 4] = {};
    uint8_t packetScratch[244] = {};
    blinker::BleFrameTransportConfig config;
    // This exercises the raw record MTU/stride layer, not Direct negotiation.
    config.initialPacketSize = 0U;
    config.maxFrameSize = 64;
    config.reassemblyTimeoutMillis = 50;
    config.maxPacketsPerPoll = 2;
    blinker::BleFrameTransport transport(
        link,
        clock,
        crypto,
        blinker::MutableByteSpan(rxStorage, sizeof(rxStorage)),
        blinker::MutableByteSpan(txStorage, sizeof(txStorage)),
        blinker::MutableByteSpan(packetScratch, sizeof(packetScratch)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    ReceiveCapture capture;
    transport.setReceiver(&captureFrame, &capture);
    CHECK_TRUE(transport.start().ok());
    link.addSession(101, 20, true);

    uint8_t frame[45] = {};
    blinker::bbp2::FrameHeader frameHeader;
    frameHeader.kind = static_cast<uint8_t>(
        blinker::bbp2::MessageKind::Patch);
    frameHeader.sequence = 7;
    frameHeader.bodyLength = static_cast<uint16_t>(
        sizeof(frame) - blinker::bbp2::kBaseHeaderSize);
    CHECK_TRUE(blinker::bbp2::encodeHeader(
                   frameHeader,
                   blinker::MutableByteSpan(frame, sizeof(frame)))
                   .ok());
    for (size_t index = blinker::bbp2::kBaseHeaderSize;
         index < sizeof(frame);
         ++index) {
        frame[index] = static_cast<uint8_t>(index);
    }

    blinker::SendTarget sessionTarget;
    sessionTarget.kind = blinker::SendTargetKind::Session;
    sessionTarget.sessionId = 101;
    CHECK_TRUE(transport.send(
                   blinker::ByteView(frame, sizeof(frame)),
                   sessionTarget)
                   .ok());
    CHECK_EQ(static_cast<size_t>(1), transport.queuedFrameCount());
    transport.poll(1000);
    transport.poll(1000);
    CHECK_EQ(static_cast<size_t>(0), transport.queuedFrameCount());
    CHECK_EQ(static_cast<size_t>(3), link.sentCount);

    const uint8_t expectedFragments[][20] = {
        {
            0xB2, 0x11, 0x01, 0x00,
            0x42, 0x4B, 0x02, 0x11, 0x00, 0x0A, 0x00, 0x07, 0x00,
            0x23, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
        },
        {
            0xB2, 0x10, 0x01, 0x01,
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
        },
        {
            0xB2, 0x12, 0x01, 0x02,
            0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
            0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x00, 0x00, 0x00
        }
    };
    for (size_t index = 0;
         index < sizeof(expectedFragments) / sizeof(expectedFragments[0]);
         ++index) {
        const size_t expectedSize =
            index == 2U ? 17U : sizeof(expectedFragments[index]);
        CHECK_EQ(expectedSize, link.sent[index].size);
        CHECK_TRUE(std::memcmp(
                       expectedFragments[index],
                       link.sent[index].data,
                       expectedSize) == 0);
    }

    size_t expectedOffset = 0;
    for (size_t index = 0; index < link.sentCount; ++index) {
        blinker::ble::FragmentView fragment;
        CHECK_TRUE(blinker::ble::parseFragment(
                       blinker::ByteView(
                           link.sent[index].data,
                           link.sent[index].size),
                       fragment)
                       .ok());
        CHECK_EQ(static_cast<uint8_t>(index), fragment.header.index);
        CHECK_EQ(static_cast<uint32_t>(101), link.sent[index].sessionId);
        CHECK_TRUE(std::memcmp(
                       frame + expectedOffset,
                       fragment.payload.data,
                       fragment.payload.size) == 0);
        expectedOffset += fragment.payload.size;
    }
    CHECK_EQ(sizeof(frame), expectedOffset);

    for (size_t index = 0; index < link.sentCount; ++index) {
        link.inject(index);
    }
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);
    CHECK_EQ(static_cast<uint32_t>(101), capture.sessionId);
    CHECK_EQ(sizeof(frame), capture.frameSize);
    CHECK_TRUE(capture.authenticated);
    CHECK_TRUE(std::memcmp(frame, capture.frame, sizeof(frame)) == 0);
    CHECK_EQ(static_cast<uint32_t>(1), transport.completedFrameCount());

    link.clearSent();
    CHECK_TRUE(transport.send(
                   blinker::ByteView(frame, sizeof(frame)),
                   sessionTarget)
                   .ok());
    transport.poll(1000);
    transport.poll(1000);
    const uint32_t dropsBeforeTimeout = transport.droppedFragmentCount();
    link.inject(0);
    clock.advance(50);
    transport.poll(1000);
    CHECK_EQ(dropsBeforeTimeout + 1U, transport.droppedFragmentCount());
    link.inject(1);
    CHECK_EQ(dropsBeforeTimeout + 2U, transport.droppedFragmentCount());
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);

    // A duplicate START for the same frame aborts the in-progress assembly;
    // later fragments cannot revive it.
    link.clearSent();
    CHECK_TRUE(transport.send(
                   blinker::ByteView(frame, sizeof(frame)),
                   sessionTarget)
                   .ok());
    transport.poll(1000);
    transport.poll(1000);
    const uint32_t dropsBeforeDuplicate =
        transport.droppedFragmentCount();
    link.inject(0);
    link.inject(0);
    link.inject(1);
    link.inject(2);
    CHECK_EQ(
        dropsBeforeDuplicate + 3U,
        transport.droppedFragmentCount());
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);

    // A gap/forward reordering is rejected rather than inferred from packet
    // sizes, and the assembly remains dead after the first mismatch.
    link.clearSent();
    CHECK_TRUE(transport.send(
                   blinker::ByteView(frame, sizeof(frame)),
                   sessionTarget)
                   .ok());
    transport.poll(1000);
    transport.poll(1000);
    const uint32_t dropsBeforeGap = transport.droppedFragmentCount();
    link.inject(0);
    link.inject(2);
    link.inject(1);
    CHECK_EQ(dropsBeforeGap + 2U, transport.droppedFragmentCount());
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);

    // Non-final fragments must keep the payload capacity established by
    // START. A shorter middle packet would otherwise create an overlap/gap in
    // the index-derived offset model.
    link.clearSent();
    CHECK_TRUE(transport.send(
                   blinker::ByteView(frame, sizeof(frame)),
                   sessionTarget)
                   .ok());
    transport.poll(1000);
    transport.poll(1000);
    const uint32_t dropsBeforeStrideMismatch =
        transport.droppedFragmentCount();
    link.inject(0);
    --link.sent[1].size;
    link.inject(1);
    link.inject(2);
    CHECK_EQ(
        dropsBeforeStrideMismatch + 2U,
        transport.droppedFragmentCount());
    CHECK_EQ(static_cast<uint32_t>(1), capture.calls);

    // ATT payload 244 carries this complete frame in one packet with both
    // START and END set. Correctness must remain identical to ATT20.
    link.setPacketSize(101, 244);
    link.clearSent();
    CHECK_TRUE(transport.send(
                   blinker::ByteView(frame, sizeof(frame)),
                   sessionTarget)
                   .ok());
    transport.poll(1000);
    CHECK_EQ(static_cast<size_t>(1), link.sentCount);
    CHECK_EQ(sizeof(frame) + blinker::ble::kFragmentHeaderSize,
             link.sent[0].size);
    blinker::ble::FragmentView largeMtuFragment;
    CHECK_TRUE(blinker::ble::parseFragment(
                   blinker::ByteView(
                       link.sent[0].data,
                       link.sent[0].size),
                   largeMtuFragment)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(
            blinker::ble::FragmentFlagStart |
            blinker::ble::FragmentFlagEnd),
        largeMtuFragment.header.flags);
    CHECK_EQ(sizeof(frame), largeMtuFragment.payload.size);
    CHECK_TRUE(std::memcmp(
                   frame,
                   largeMtuFragment.payload.data,
                   sizeof(frame)) == 0);
    link.inject(0);
    CHECK_EQ(static_cast<uint32_t>(2), capture.calls);

    // Disconnect destroys reassembly state even if an adapter accidentally
    // reuses a numeric session ID after reconnect.
    link.setPacketSize(101, 20);
    link.clearSent();
    CHECK_TRUE(transport.send(
                   blinker::ByteView(frame, sizeof(frame)),
                   sessionTarget)
                   .ok());
    transport.poll(1000);
    transport.poll(1000);
    link.inject(0);
    const uint32_t dropsBeforeDisconnect =
        transport.droppedFragmentCount();
    link.disconnect(101);
    link.addSession(101, 20, true);
    link.inject(1);
    CHECK_EQ(
        dropsBeforeDisconnect + 1U,
        transport.droppedFragmentCount());
    CHECK_EQ(static_cast<uint32_t>(2), capture.calls);

    // An MTU increase does not change the stride of a frame already in
    // progress. The remaining fragments keep the original 16-byte payload
    // capacity and therefore retain unambiguous index-derived offsets.
    link.clearSent();
    CHECK_TRUE(transport.send(
                   blinker::ByteView(frame, sizeof(frame)),
                   sessionTarget)
                   .ok());
    transport.poll(1000);
    CHECK_EQ(static_cast<size_t>(2), link.sentCount);
    link.setPacketSize(101, 244);
    transport.poll(1000);
    CHECK_EQ(static_cast<size_t>(3), link.sentCount);
    uint8_t growingMtuFrameId = 0U;
    for (size_t index = 0; index < link.sentCount; ++index) {
        blinker::ble::FragmentView growingMtuFragment;
        CHECK_TRUE(blinker::ble::parseFragment(
                       blinker::ByteView(
                           link.sent[index].data,
                           link.sent[index].size),
                       growingMtuFragment)
                       .ok());
        if (index == 0U) {
            growingMtuFrameId = growingMtuFragment.header.frameId;
        }
        CHECK_EQ(growingMtuFrameId, growingMtuFragment.header.frameId);
        CHECK_EQ(static_cast<uint8_t>(index),
                 growingMtuFragment.header.index);
        CHECK_EQ(index == 2U ? static_cast<size_t>(13)
                             : static_cast<size_t>(16),
                 growingMtuFragment.payload.size);
        link.inject(index);
    }
    CHECK_EQ(static_cast<uint32_t>(3), capture.calls);
    transport.stop();

    // If the negotiated MTU shrinks after a frame has started, the sender
    // restarts that queued frame at index 0 with a fresh frame ID and a fixed
    // smaller payload capacity. Mixing 240-byte and 16-byte index strides
    // would make index-derived offsets ambiguous.
    FakeBleLink mtuLink;
    uint8_t mtuRx[536 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t mtuTx[536] = {};
    uint8_t mtuPacket[244] = {};
    blinker::BleFrameTransportConfig mtuConfig;
    mtuConfig.initialPacketSize = 0U;
    mtuConfig.maxFrameSize = 512;
    mtuConfig.maxPacketsPerPoll = 1;
    blinker::BleFrameTransport mtuTransport(
        mtuLink,
        clock,
        crypto,
        blinker::MutableByteSpan(mtuRx, sizeof(mtuRx)),
        blinker::MutableByteSpan(mtuTx, sizeof(mtuTx)),
        blinker::MutableByteSpan(mtuPacket, sizeof(mtuPacket)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        mtuConfig);
    ReceiveCapture mtuCapture;
    mtuTransport.setReceiver(&captureFrame, &mtuCapture);
    CHECK_TRUE(mtuTransport.start().ok());
    mtuLink.addSession(77, 244, true);
    uint8_t mtuFrame[300] = {};
    frameHeader.bodyLength = static_cast<uint16_t>(
        sizeof(mtuFrame) - blinker::bbp2::kBaseHeaderSize);
    CHECK_TRUE(blinker::bbp2::encodeHeader(
                   frameHeader,
                   blinker::MutableByteSpan(mtuFrame, sizeof(mtuFrame)))
                   .ok());
    for (size_t index = blinker::bbp2::kBaseHeaderSize;
         index < sizeof(mtuFrame);
         ++index) {
        mtuFrame[index] = static_cast<uint8_t>(index);
    }
    blinker::SendTarget mtuTarget;
    mtuTarget.kind = blinker::SendTargetKind::Session;
    mtuTarget.sessionId = 77;
    CHECK_TRUE(mtuTransport.send(
                   blinker::ByteView(mtuFrame, sizeof(mtuFrame)),
                   mtuTarget)
                   .ok());
    mtuTransport.poll(1000);
    CHECK_EQ(static_cast<size_t>(1), mtuLink.sentCount);
    mtuLink.setPacketSize(77, 20);
    mtuTransport.poll(1000);
    CHECK_EQ(static_cast<size_t>(2), mtuLink.sentCount);
    blinker::ble::FragmentView oldMtuStart;
    blinker::ble::FragmentView restartedMtuStart;
    CHECK_TRUE(blinker::ble::parseFragment(
                   blinker::ByteView(
                       mtuLink.sent[0].data,
                       mtuLink.sent[0].size),
                   oldMtuStart)
                   .ok());
    CHECK_TRUE(blinker::ble::parseFragment(
                   blinker::ByteView(
                       mtuLink.sent[1].data,
                       mtuLink.sent[1].size),
                   restartedMtuStart)
                   .ok());
    CHECK_EQ(static_cast<uint8_t>(0), oldMtuStart.header.index);
    CHECK_EQ(static_cast<uint8_t>(0), restartedMtuStart.header.index);
    CHECK_TRUE(oldMtuStart.header.frameId !=
               restartedMtuStart.header.frameId);
    CHECK_EQ(static_cast<size_t>(240), oldMtuStart.payload.size);
    CHECK_EQ(static_cast<size_t>(16), restartedMtuStart.payload.size);
    while (mtuTransport.queuedFrameCount() != 0U) {
        mtuTransport.poll(1000);
    }
    CHECK_EQ(static_cast<size_t>(20), mtuLink.sentCount);
    for (size_t index = 0; index < mtuLink.sentCount; ++index) {
        mtuLink.inject(index);
    }
    CHECK_EQ(static_cast<uint32_t>(1), mtuCapture.calls);
    CHECK_EQ(sizeof(mtuFrame), mtuCapture.frameSize);
    CHECK_TRUE(std::memcmp(
                   mtuFrame,
                   mtuCapture.frame,
                   sizeof(mtuFrame)) == 0);
    mtuTransport.stop();

    // Frame IDs are non-zero, 8-bit and local to each BLE session. The first
    // session wraps 255 -> 1; a new session independently starts at 1.
    FakeBleLink wrapLink;
    uint8_t wrapRx[88 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t wrapTx[88] = {};
    uint8_t wrapPacket[20] = {};
    blinker::BleFrameTransport wrapTransport(
        wrapLink,
        clock,
        crypto,
        blinker::MutableByteSpan(wrapRx, sizeof(wrapRx)),
        blinker::MutableByteSpan(wrapTx, sizeof(wrapTx)),
        blinker::MutableByteSpan(wrapPacket, sizeof(wrapPacket)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    ReceiveCapture wrapCapture;
    wrapTransport.setReceiver(&captureFrame, &wrapCapture);
    CHECK_TRUE(wrapTransport.start().ok());
    wrapLink.addSession(88, 20, true);
    uint8_t wrapFrame[blinker::bbp2::kBaseHeaderSize] = {};
    frameHeader.bodyLength = 0;
    CHECK_TRUE(blinker::bbp2::encodeHeader(
                   frameHeader,
                   blinker::MutableByteSpan(wrapFrame, sizeof(wrapFrame)))
                   .ok());
    blinker::SendTarget wrapTarget;
    wrapTarget.kind = blinker::SendTargetKind::Session;
    wrapTarget.sessionId = 88;
    uint8_t firstFrameId = 0;
    uint8_t lastBeforeWrap = 0;
    uint8_t wrappedFrameId = 0;
    for (size_t index = 0; index < 256U; ++index) {
        wrapLink.clearSent();
        CHECK_TRUE(wrapTransport.send(
                       blinker::ByteView(wrapFrame, sizeof(wrapFrame)),
                       wrapTarget)
                       .ok());
        wrapTransport.poll(1000);
        CHECK_EQ(static_cast<size_t>(1), wrapLink.sentCount);
        blinker::ble::FragmentView wrapFragment;
        CHECK_TRUE(blinker::ble::parseFragment(
                       blinker::ByteView(
                           wrapLink.sent[0].data,
                           wrapLink.sent[0].size),
                       wrapFragment)
                       .ok());
        if (index == 0U) firstFrameId = wrapFragment.header.frameId;
        if (index == 254U) lastBeforeWrap = wrapFragment.header.frameId;
        if (index == 255U) wrappedFrameId = wrapFragment.header.frameId;
        wrapLink.inject(0);
    }
    CHECK_EQ(static_cast<uint8_t>(1), firstFrameId);
    CHECK_EQ(static_cast<uint8_t>(255), lastBeforeWrap);
    CHECK_EQ(static_cast<uint8_t>(1), wrappedFrameId);
    CHECK_EQ(static_cast<uint32_t>(256), wrapCapture.calls);
    wrapLink.addSession(99, 20, true);
    wrapTarget.sessionId = 99;
    wrapLink.clearSent();
    CHECK_TRUE(wrapTransport.send(
                   blinker::ByteView(wrapFrame, sizeof(wrapFrame)),
                   wrapTarget)
                   .ok());
    wrapTransport.poll(1000);
    blinker::ble::FragmentView secondSessionFragment;
    CHECK_TRUE(blinker::ble::parseFragment(
                   blinker::ByteView(
                       wrapLink.sent[0].data,
                       wrapLink.sent[0].size),
                   secondSessionFragment)
                   .ok());
    CHECK_EQ(static_cast<uint8_t>(1),
             secondSessionFragment.header.frameId);
    wrapTransport.stop();

    // ATT20 leaves 16 data bytes per packet. Index 0..255 therefore permits
    // exactly a 4096-byte secure record, hence 4072 bytes of BBP/2 plaintext.
    // The profile rejects 4073 because its protected record would need 257
    // fragments even when a larger negotiated MTU could carry it.
    FakeBleLink boundaryLink;
    uint8_t boundaryRx[
        4096 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t boundaryTx[4096] = {};
    uint8_t boundaryPacket[20] = {};
    blinker::BleFrameTransportConfig boundaryConfig;
    boundaryConfig.maxFrameSize = 4072;
    boundaryConfig.maxPacketsPerPoll = 1;
    blinker::BleFrameTransport boundaryTransport(
        boundaryLink,
        clock,
        crypto,
        blinker::MutableByteSpan(boundaryRx, sizeof(boundaryRx)),
        blinker::MutableByteSpan(boundaryTx, sizeof(boundaryTx)),
        blinker::MutableByteSpan(boundaryPacket, sizeof(boundaryPacket)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        boundaryConfig);
    CHECK_TRUE(boundaryTransport.start().ok());
    boundaryLink.addSession(66, 20, true);
    uint8_t boundaryFrame[4072] = {};
    frameHeader.bodyLength = static_cast<uint16_t>(
        sizeof(boundaryFrame) - blinker::bbp2::kBaseHeaderSize);
    CHECK_TRUE(blinker::bbp2::encodeHeader(
                   frameHeader,
                   blinker::MutableByteSpan(
                       boundaryFrame,
                       sizeof(boundaryFrame)))
                   .ok());
    blinker::SendTarget boundaryTarget;
    boundaryTarget.kind = blinker::SendTargetKind::Session;
    boundaryTarget.sessionId = 66;
    CHECK_TRUE(boundaryTransport.send(
                   blinker::ByteView(
                       boundaryFrame,
                       sizeof(boundaryFrame)),
                   boundaryTarget)
                   .ok());
    CHECK_EQ(static_cast<size_t>(1),
             boundaryTransport.queuedFrameCount());
    boundaryTransport.stop();

    FakeBleLink oversizedBoundaryLink;
    uint8_t oversizedBoundaryRx[
        4097 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t oversizedBoundaryTx[4097] = {};
    blinker::BleFrameTransportConfig oversizedBoundaryConfig;
    oversizedBoundaryConfig.maxFrameSize = 4073;
    blinker::BleFrameTransport oversizedBoundaryTransport(
        oversizedBoundaryLink,
        clock,
        crypto,
        blinker::MutableByteSpan(
            oversizedBoundaryRx,
            sizeof(oversizedBoundaryRx)),
        blinker::MutableByteSpan(
            oversizedBoundaryTx,
            sizeof(oversizedBoundaryTx)),
        blinker::MutableByteSpan(boundaryPacket, sizeof(boundaryPacket)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        oversizedBoundaryConfig);
    CHECK_EQ(blinker::ErrorCode::NotConfigured,
             oversizedBoundaryTransport.start().code());

    FakeBleLink queueLink;
    uint8_t queueRx[88 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t queueTx[88 * 2] = {};
    uint8_t queuePacket[32] = {};
    blinker::BleFrameTransport queueTransport(
        queueLink,
        clock,
        crypto,
        blinker::MutableByteSpan(queueRx, sizeof(queueRx)),
        blinker::MutableByteSpan(queueTx, sizeof(queueTx)),
        blinker::MutableByteSpan(queuePacket, sizeof(queuePacket)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    CHECK_TRUE(queueTransport.start().ok());
    queueLink.addSession(101, 20);
    queueLink.addSession(202, 20);
    blinker::SendTarget broadcast;
    broadcast.kind = blinker::SendTargetKind::Broadcast;
    CHECK_TRUE(queueTransport.send(
                   blinker::ByteView(frame, sizeof(frame)),
                   broadcast)
                   .ok());
    CHECK_EQ(static_cast<size_t>(2), queueTransport.queuedFrameCount());
    queueLink.blockSends = true;
    queueTransport.poll(1000);
    CHECK_EQ(static_cast<size_t>(2), queueTransport.queuedFrameCount());
    queueLink.disconnect(101);
    CHECK_EQ(static_cast<size_t>(1), queueTransport.queuedFrameCount());
    queueLink.blockSends = false;
    while (queueTransport.queuedFrameCount() != 0U) {
        queueTransport.poll(1000);
    }
    CHECK_TRUE(queueLink.sentCount != 0U);
    for (size_t index = 0; index < queueLink.sentCount; ++index) {
        CHECK_EQ(static_cast<uint32_t>(202), queueLink.sent[index].sessionId);
    }
    queueTransport.stop();

    FakeBleLink atomicLink;
    uint8_t atomicRx[88 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t atomicTx[88] = {};
    uint8_t atomicPacket[20] = {};
    blinker::BleFrameTransport atomicTransport(
        atomicLink,
        clock,
        crypto,
        blinker::MutableByteSpan(atomicRx, sizeof(atomicRx)),
        blinker::MutableByteSpan(atomicTx, sizeof(atomicTx)),
        blinker::MutableByteSpan(atomicPacket, sizeof(atomicPacket)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    CHECK_TRUE(atomicTransport.start().ok());
    atomicLink.addSession(1, 20);
    atomicLink.addSession(2, 20);
    CHECK_EQ(
        blinker::ErrorCode::WouldBlock,
        atomicTransport.send(
            blinker::ByteView(frame, sizeof(frame)),
            broadcast)
            .code());
    CHECK_EQ(static_cast<size_t>(0), atomicTransport.queuedFrameCount());
    atomicTransport.stop();

    FakeBleLink overflowLink;
    uint8_t overflowRx[88 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t overflowTx[88 * 6] = {};
    uint8_t overflowPacket[20] = {};
    blinker::BleFrameTransport overflowTransport(
        overflowLink,
        clock,
        crypto,
        blinker::MutableByteSpan(overflowRx, sizeof(overflowRx)),
        blinker::MutableByteSpan(overflowTx, sizeof(overflowTx)),
        blinker::MutableByteSpan(overflowPacket, sizeof(overflowPacket)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    CHECK_TRUE(overflowTransport.start().ok());
    for (uint32_t id = 1; id <= BLINKER_BLE_MAX_SESSIONS + 1U; ++id) {
        overflowLink.addSession(id, 20);
    }
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        overflowTransport.send(
            blinker::ByteView(frame, sizeof(frame)),
            broadcast)
            .code());
    CHECK_EQ(static_cast<size_t>(0), overflowTransport.queuedFrameCount());
}

void testBleSessionFaultGates() {
    FakeClock clock;
    UnsupportedAead crypto;
    FakeBleLink link;
    uint8_t rxStorage[88 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t txStorage[88] = {};
    uint8_t packetScratch[20] = {};
    uint8_t plaintext[64] = {};
    blinker::BleFrameTransportConfig config;
    config.maxFrameSize = 64U;
    config.maxPacketsPerPoll = 2U;
    blinker::BleFrameTransport transport(
        link,
        clock,
        crypto,
        blinker::MutableByteSpan(rxStorage, sizeof(rxStorage)),
        blinker::MutableByteSpan(txStorage, sizeof(txStorage)),
        blinker::MutableByteSpan(packetScratch, sizeof(packetScratch)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    ReceiveCapture frames;
    BleSessionCapture sessions;
    transport.setReceiver(&captureFrame, &frames);
    transport.setSessionHandlers(
        &captureBleConnected,
        &captureBleDisconnected,
        &sessions);
    CHECK_TRUE(transport.start());

    uint8_t frame[blinker::bbp2::kBaseHeaderSize] = {};
    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(blinker::bbp2::MessageKind::Patch);
    header.bodyLength = 0U;
    CHECK_TRUE(blinker::bbp2::encodeHeader(
        header,
        blinker::MutableByteSpan(frame, sizeof(frame))));
    blinker::SendTarget target;
    target.kind = blinker::SendTargetKind::Session;
    target.sessionId = 303U;

    link.addSession(303U, 20U, true, true, true, true);
    CHECK_EQ(static_cast<uint32_t>(1U), sessions.connected);
    link.blockSends = true;
    CHECK_TRUE(transport.send(blinker::ByteView(frame, sizeof(frame)), target));
    transport.poll(1000U);
    CHECK_EQ(static_cast<size_t>(1U), transport.queuedFrameCount());

    // A security fact established for an admitted connection may upgrade but
    // never downgrade. Detect loss even when a platform misses its disconnect
    // callback, close the logical session and discard its queued plaintext.
    link.setSecurity(303U, false, true, true);
    transport.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(1U), sessions.disconnected);
    CHECK_EQ(static_cast<size_t>(0U), transport.queuedFrameCount());
    CHECK_EQ(
        blinker::ErrorCode::NotConnected,
        transport.send(blinker::ByteView(frame, sizeof(frame)), target).code());
    link.disconnect(303U);
    CHECK_EQ(static_cast<uint32_t>(1U), sessions.disconnected);

    // Reusing a numeric ID is valid only after disconnect. Re-announcing one
    // while it is active is quarantined, so a retained packet from that peer
    // cannot inherit the previous connection's authorization/reassembly state.
    link.addSession(303U, 20U, true, true, true, true);
    CHECK_EQ(static_cast<uint32_t>(2U), sessions.connected);
    link.blockSends = false;
    link.clearSent();
    CHECK_TRUE(transport.send(blinker::ByteView(frame, sizeof(frame)), target));
    transport.poll(1000U);
    CHECK_EQ(static_cast<size_t>(1U), link.sentCount);
    link.announceSession(303U);
    CHECK_EQ(static_cast<uint32_t>(2U), sessions.disconnected);
    const uint32_t drops = transport.droppedFragmentCount();
    link.inject(0U);
    CHECK_EQ(drops + 1U, transport.droppedFragmentCount());
    CHECK_EQ(static_cast<uint32_t>(0U), frames.calls);
    CHECK_EQ(
        blinker::ErrorCode::NotConnected,
        transport.send(blinker::ByteView(frame, sizeof(frame)), target).code());

    link.disconnect(303U);
    link.addSession(303U, 20U, true, true, true, true);
    CHECK_EQ(static_cast<uint32_t>(3U), sessions.connected);
    link.clearSent();
    CHECK_TRUE(transport.send(blinker::ByteView(frame, sizeof(frame)), target));
    transport.poll(1000U);
    link.inject(0U);
    CHECK_EQ(static_cast<uint32_t>(1U), frames.calls);
    CHECK_EQ(static_cast<uint32_t>(303U), frames.sessionId);
    transport.stop();
}

void testBleRuntimeSessionLifecycle() {
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite));
    const blinker::EndpointDescriptor* endpointStorage[1] = {};
    blinker::EndpointRegistry endpoints(endpointStorage, 1);
    CHECK_TRUE(endpoints.add(endpoint).ok());

    FakeClock clock;
    UnsupportedAead crypto;
    FakeBleLink link;
    uint8_t rxStorage[152 * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t txStorage[152] = {};
    uint8_t packetScratch[20] = {};
    uint8_t plaintext[128] = {};
    blinker::BleFrameTransportConfig bleConfig;
    bleConfig.maxFrameSize = 128;
    bleConfig.maxPacketsPerPoll = 1;
    blinker::BleFrameTransport bleTransport(
        link,
        clock,
        crypto,
        blinker::MutableByteSpan(rxStorage, sizeof(rxStorage)),
        blinker::MutableByteSpan(txStorage, sizeof(txStorage)),
        blinker::MutableByteSpan(packetScratch, sizeof(packetScratch)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        bleConfig);
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(bleTransport).ok());
    uint8_t runtimeBuffer[128] = {};
    blinker::DeviceRuntimeConfig runtimeConfig;
    runtimeConfig.maxFrameSize = sizeof(runtimeBuffer);
    runtimeConfig.maxReassemblySize = sizeof(runtimeBuffer);
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(runtimeBuffer, sizeof(runtimeBuffer)),
        runtimeConfig);
    CHECK_TRUE(runtime.start().ok());
    runtime.poll(1000);
    CHECK_EQ(static_cast<size_t>(0), bleTransport.queuedFrameCount());

    for (uint32_t cycle = 0; cycle < 12U; ++cycle) {
        const uint32_t sessionId = 1000U + cycle;
        link.addSession(sessionId, 20, true);
        runtime.poll(1000);
        CHECK_EQ(static_cast<size_t>(0), bleTransport.queuedFrameCount());
        link.disconnect(sessionId);
        CHECK_EQ(static_cast<size_t>(0), bleTransport.queuedFrameCount());
    }
    runtime.stop();
}

void testPeerInitiatedHelloTransport() {
    class NegotiatingTransport : public FakeTransport {
    public:
        unsigned packetNegotiations = 0U;
        uint16_t requestedPacket = 0U;
        uint16_t negotiatePacketSize(
            const blinker::RxContext& rx, uint16_t requested) override {
            ++packetNegotiations;
            requestedPacket = requested;
            return rx.encrypted ? 100U : 0U;
        }
    };
    const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite));
    const blinker::EndpointDescriptor* endpointStorage[1] = {};
    blinker::EndpointRegistry endpoints(endpointStorage, 1);
    CHECK_TRUE(endpoints.add(endpoint).ok());

    NegotiatingTransport transport;
    transport.featureFlags = static_cast<uint16_t>(
        blinker::TransportFeatureBinary |
        blinker::TransportFeatureLocal |
        blinker::TransportFeaturePeerInitiatesHello);
    blinker::TransportHub hub;
    CHECK_TRUE(hub.addTransport(transport).ok());
    uint8_t runtimeBuffer[192] = {};
    blinker::DeviceRuntimeConfig runtimeConfig;
    runtimeConfig.maxFrameSize = sizeof(runtimeBuffer);
    runtimeConfig.maxReassemblySize = sizeof(runtimeBuffer);
    blinker::DeviceRuntime runtime(
        hub,
        endpoints,
        blinker::MutableByteSpan(runtimeBuffer, sizeof(runtimeBuffer)),
        runtimeConfig);
    CHECK_TRUE(runtime.start().ok());

    // A peer-initiated transport remains silent both before and after its
    // physical session opens. This prevents the device and App from racing
    // two independent BBP/2 HELLO exchanges on one BLE notification stream.
    runtime.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(0U), transport.sendCount);
    blinker::RxContext session;
    session.transportId = 0U;
    session.sessionId = 41U;
    session.encrypted = true;
    session.bonded = true;
    session.authenticated = true;
    CHECK_TRUE(transport.sessionConnected != nullptr);
    transport.sessionConnected(transport.sessionContext, session);
    runtime.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(0U), transport.sendCount);

    uint8_t bodyStorage[96] = {};
    uint8_t frameStorage[192] = {};
    blinker::bbp2::HelloBody peerHello;
    peerHello.role = blinker::bbp2::PeerRole::App;
    peerHello.versions[0] = blinker::bbp2::kVersion;
    peerHello.versionCount = 1U;
    peerHello.maxFrameSize = sizeof(frameStorage);
    peerHello.maxReassemblySize = sizeof(frameStorage);
    blinker::ByteView body;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
                   peerHello,
                   blinker::MutableByteSpan(
                       bodyStorage,
                       sizeof(bodyStorage)),
                   body)
                   .ok());
    blinker::ByteView frame;
    CHECK_TRUE(buildTestFrame(
                   blinker::bbp2::MessageKind::Hello,
                   blinker::bbp2::FlagNone,
                   77U,
                   body,
                   blinker::MutableByteSpan(
                       frameStorage,
                       sizeof(frameStorage)),
                   frame)
                   .ok());
    transport.inject(frame, 0U, true, session.sessionId, true, true);

    CHECK_EQ(static_cast<uint32_t>(1U), transport.sendCount);
    CHECK_EQ(blinker::SendTargetKind::Session, transport.lastTarget.kind);
    CHECK_EQ(session.sessionId, transport.lastTarget.sessionId);
    blinker::bbp2::FrameView response;
    CHECK_TRUE(blinker::bbp2::parseFrame(
                   blinker::ByteView(
                       transport.lastSent,
                       transport.lastSentSize),
                   response)
                   .ok());
    CHECK_EQ(
        static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello),
        response.header.kind);
    CHECK_TRUE(
        (response.header.flags & blinker::bbp2::FlagIsResponse) != 0U);
    CHECK_EQ(static_cast<uint16_t>(77U), response.header.sequence);
    CHECK_EQ(0U, transport.packetNegotiations);
    blinker::bbp2::HelloBody responseHello;
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(response.body, responseHello).ok());
    CHECK_EQ(static_cast<uint16_t>(0U), responseHello.maxBlePacketSize);
    peerHello.maxBlePacketSize = 244U;
    CHECK_TRUE(blinker::bbp2::encodeHelloBody(
        peerHello, blinker::MutableByteSpan(bodyStorage, sizeof(bodyStorage)),
        body).ok());
    CHECK_TRUE(buildTestFrame(blinker::bbp2::MessageKind::Hello,
        blinker::bbp2::FlagNone, 78U, body,
        blinker::MutableByteSpan(frameStorage, sizeof(frameStorage)), frame).ok());
    transport.inject(frame, 0U, true, session.sessionId, true, true);
    CHECK_EQ(1U, transport.packetNegotiations);
    CHECK_EQ(static_cast<uint16_t>(244U), transport.requestedPacket);
    CHECK_TRUE(blinker::bbp2::parseFrame(
        blinker::ByteView(transport.lastSent, transport.lastSentSize), response).ok());
    CHECK_TRUE(blinker::bbp2::decodeHelloBody(response.body, responseHello).ok());
    CHECK_EQ(static_cast<uint16_t>(100U), responseHello.maxBlePacketSize);
    runtime.stop();
}

void testStringViewLiteralLength() {
    const blinker::StringView ascii("portable-core");
    CHECK_EQ(static_cast<size_t>(13), ascii.size);
    CHECK_TRUE(std::memcmp(ascii.data, "portable-core", ascii.size) == 0);

    const blinker::StringView utf8("温度");
    CHECK_EQ(sizeof("温度") - 1U, utf8.size);

    const char runtimeText[] = {'a', '\0', 'b'};
    const blinker::StringView explicitRuntime(runtimeText, sizeof(runtimeText));
    CHECK_EQ(sizeof(runtimeText), explicitRuntime.size);
}

} // namespace

int main() {
    testDiagnostics();
    testStringViewLiteralLength();
    testSha256();
    testHmacSha256();
    testArduinoClientHttpAdapter();
    testManagedMqttTransport();
    testRestrictedCbor();
    testControlBodies();
    testKeyedBodies();
    testEndpointRegistryAndManifest();
    testEndpointConstraints();
    testEndpointStateStore();
    testRevisionConsistentStatePagination();
    testCanonicalClientApi();
    testClientConnectionStatus();
    testPreparedOfflineState();
    testOutboundStateScheduling();
    testPublicDeviceFacade();
    testUnifiedProductFacade();
    testProductAttachesClockBeforeTelemetryPreparation();
    testProductAccessReset();
    testProductNetworkReset();
    testSemanticInteractions();
    testCanonicalBlinkerFacade();
    testClientRateLimitedTelemetryBackoff();
    testTelemetryRuntimeAcrossCloudAndDirect();
    testTelemetryRuntime();
    testDeviceRuntime();
    testSingleSlotCommandAck();
    testReliableCommandIdempotency();
    testVersionedStateAndEvents();
    testReliablePatchOutbox();
    testControllerControlRuntime();
    testLocalAuthorizationRuntime();
    testTransportScopedAccess();
    testDualTransportRuntime(true);
    testDualTransportRuntime(false);
    testFrameRoundTrip();
    testFrameValidation();
    testTransportHub();
    runConnectivityLifecycleTests();
    testMqttFrameTransport();
    testPubSubClientAdapter();
    testBleFragment();
    testBleFrameTransport();
    testBleSessionFaultGates();
    testBleRuntimeSessionLifecycle();
    testPeerInitiatedHelloTransport();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "All device-v2 host tests passed" << std::endl;
    return EXIT_SUCCESS;
}
