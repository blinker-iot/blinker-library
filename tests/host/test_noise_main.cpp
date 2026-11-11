#include <BlinkerV2Advanced.h>
#include "support/DeterministicNoiseCrypto.h"
#include <BlinkerV2/interface/ILocalProvisioningApplication.h>
#include <BlinkerV2/interface/ILocalProvisioningSession.h>
#include <BlinkerV2/interface/ISetupPskProvider.h>
#include <BlinkerV2/provisioning/BleLocalProvisioningEndpoint.h>
#include <BlinkerV2/provisioning/BleNoiseModePreparation.h>
#include <BlinkerV2/provisioning/BleNoiseProvisioningChannel.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "CHECK failed at line " << __LINE__ << ": "          \
                      << #condition << std::endl;                               \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

bool bytesAreZero(const uint8_t* data, size_t size) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < size; ++index) {
        combined = static_cast<uint8_t>(combined | data[index]);
    }
    return combined == 0U;
}

class FixedRandom : public blinker::IRandom {
public:
    explicit FixedRandom(uint8_t seed) : seed_(seed), calls_(0U) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        if (output.data == nullptr || output.size != 32U || calls_ != 0U) {
            return blinker::Result::failure(blinker::ErrorCode::InternalError);
        }
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = static_cast<uint8_t>(seed_ + index);
        }
        ++calls_;
        return blinker::Result::success();
    }

private:
    uint8_t seed_;
    uint8_t calls_;
};


class NoiseTestClock : public blinker::IClock {
public:
    NoiseTestClock() : nowMillis(0U) {}

    uint32_t monotonicMillis() const override { return nowMillis; }
    bool unixTime(uint64_t&) const override { return false; }
    void advance(uint32_t millis) { nowMillis += millis; }

    uint32_t nowMillis;
};

class NoiseTestBleLink : public blinker::IBleLink {
public:
    struct SentPacket {
        uint8_t bytes[244];
        size_t size;

        SentPacket() : bytes(), size(0U) {}
    };

    NoiseTestBleLink()
        : currentState(blinker::BleLinkState::Stopped),
          packetReceiver(nullptr),
          packetContext(nullptr),
          connectedHandler(nullptr),
          disconnectedHandler(nullptr),
          sessionContext(nullptr),
          session(),
          hasSession(false),
          sent(),
          sentCount(0U),
          blockSends(false),
          nextInboundRecordId(1U),
          disconnectRequests(0U) {}

    blinker::Result start() override {
        currentState = blinker::BleLinkState::Ready;
        return blinker::Result::success();
    }

    void stop() override { currentState = blinker::BleLinkState::Stopped; }
    void poll(uint32_t) override {}
    blinker::BleLinkState state() const override { return currentState; }
    blinker::ErrorCode lastError() const override {
        return blinker::ErrorCode::Ok;
    }
    size_t sessionCount() const override { return hasSession ? 1U : 0U; }

    blinker::Result sessionAt(
        size_t index,
        blinker::BleSessionInfo& output) const override {
        if (!hasSession || index != 0U) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        output = session;
        return blinker::Result::success();
    }

    blinker::Result sendPacket(
        uint32_t sessionId,
        blinker::ByteView packet) override {
        if (blockSends) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        if (!hasSession || sessionId != session.sessionId) {
            return blinker::Result::failure(blinker::ErrorCode::NotConnected);
        }
        if (packet.data == nullptr || packet.size > sizeof(sent[0].bytes) ||
            sentCount >= sizeof(sent) / sizeof(sent[0])) {
            return blinker::Result::failure(
                blinker::ErrorCode::CapacityExceeded);
        }
        SentPacket& target = sent[sentCount++];
        std::memcpy(target.bytes, packet.data, packet.size);
        target.size = packet.size;
        return blinker::Result::success();
    }

    blinker::Result disconnectSession(uint32_t sessionId) override {
        if (!hasSession || session.sessionId != sessionId) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        hasSession = false;
        ++disconnectRequests;
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

    void connect(uint32_t sessionId = 7U, uint16_t packetSize = 20U) {
        session = blinker::BleSessionInfo();
        session.sessionId = sessionId;
        session.maxPacketSize = packetSize;
        session.connected = true;
        session.notifyEnabled = true;
        hasSession = true;
        if (connectedHandler != nullptr) {
            connectedHandler(sessionContext, session);
        }
    }

    void disconnect() {
        if (!hasSession) return;
        const blinker::BleSessionInfo previous = session;
        hasSession = false;
        if (disconnectedHandler != nullptr) {
            disconnectedHandler(sessionContext, previous);
        }
    }

    void injectPacket(blinker::ByteView packet) {
        CHECK_TRUE(hasSession);
        CHECK_TRUE(packetReceiver != nullptr);
        packetReceiver(packetContext, session, packet);
    }

    void injectRecord(blinker::ByteView record) {
        CHECK_TRUE(record.data != nullptr);
        const size_t payloadCapacity =
            session.maxPacketSize - blinker::ble::kFragmentHeaderSize;
        size_t offset = 0U;
        uint8_t index = 0U;
        const uint8_t recordId = nextInboundRecordId++;
        while (offset < record.size) {
            const size_t remaining = record.size - offset;
            const size_t payloadSize = remaining < payloadCapacity
                                           ? remaining
                                           : payloadCapacity;
            blinker::ble::FragmentHeader header;
            header.flags = offset == 0U
                               ? blinker::ble::FragmentFlagStart
                               : blinker::ble::FragmentFlagNone;
            if (payloadSize == remaining) {
                header.flags = static_cast<uint8_t>(
                    header.flags | blinker::ble::FragmentFlagEnd);
            }
            header.frameId = recordId;
            header.index = index;
            uint8_t packet[244] = {};
            CHECK_TRUE(blinker::ble::encodeFragmentHeader(
                header,
                blinker::MutableByteSpan(packet, sizeof(packet))));
            std::memcpy(
                packet + blinker::ble::kFragmentHeaderSize,
                record.data + offset,
                payloadSize);
            injectPacket(blinker::ByteView(
                packet,
                blinker::ble::kFragmentHeaderSize + payloadSize));
            offset += payloadSize;
            ++index;
        }
    }

    void clearSent() { sentCount = 0U; }

    blinker::Result takeSentRecord(
        blinker::MutableByteSpan output,
        blinker::ByteView& record) {
        record = blinker::ByteView();
        if (sentCount == 0U || output.data == nullptr) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        size_t written = 0U;
        uint8_t expectedIndex = 0U;
        uint8_t expectedId = 0U;
        for (size_t packetIndex = 0U;
             packetIndex < sentCount;
             ++packetIndex) {
            blinker::ble::FragmentView fragment;
            blinker::Result result = blinker::ble::parseFragment(
                blinker::ByteView(
                    sent[packetIndex].bytes,
                    sent[packetIndex].size),
                fragment);
            if (!result) return result;
            const bool start =
                (fragment.header.flags &
                 blinker::ble::FragmentFlagStart) != 0U;
            const bool end =
                (fragment.header.flags &
                 blinker::ble::FragmentFlagEnd) != 0U;
            if (packetIndex == 0U) {
                if (!start) {
                    return blinker::Result::failure(
                        blinker::ErrorCode::ProtocolError);
                }
                expectedId = fragment.header.frameId;
            }
            if (fragment.header.frameId != expectedId ||
                fragment.header.index != expectedIndex ||
                written + fragment.payload.size > output.size) {
                return blinker::Result::failure(
                    blinker::ErrorCode::ProtocolError);
            }
            std::memcpy(
                output.data + written,
                fragment.payload.data,
                fragment.payload.size);
            written += fragment.payload.size;
            ++expectedIndex;
            if (end && packetIndex + 1U != sentCount) {
                return blinker::Result::failure(
                    blinker::ErrorCode::TrailingData);
            }
            if (!end && packetIndex + 1U == sentCount) {
                return blinker::Result::failure(
                    blinker::ErrorCode::TruncatedInput);
            }
        }
        record = blinker::ByteView(output.data, written);
        clearSent();
        return blinker::Result::success();
    }

    blinker::BleLinkState currentState;
    blinker::BlePacketReceiver packetReceiver;
    void* packetContext;
    blinker::BleSessionHandler connectedHandler;
    blinker::BleSessionHandler disconnectedHandler;
    void* sessionContext;
    blinker::BleSessionInfo session;
    bool hasSession;
    SentPacket sent[80];
    size_t sentCount;
    bool blockSends;
    uint8_t nextInboundRecordId;
    uint32_t disconnectRequests;
};

struct NoiseChannelCapture {
    NoiseChannelCapture()
        : payload(),
          payloadSize(0U),
          payloadKind(blinker::BleNoisePayloadKind::Transport),
          payloadCount(0U),
          fault(blinker::ErrorCode::Ok),
          faultCount(0U) {}

    static void receive(
        void* context,
        blinker::BleNoisePayloadKind kind,
        blinker::ByteView value,
        const blinker::RxContext&) {
        NoiseChannelCapture* capture =
            static_cast<NoiseChannelCapture*>(context);
        CHECK_TRUE(capture != nullptr);
        CHECK_TRUE(value.size <= sizeof(capture->payload));
        if (value.size != 0U) {
            std::memcpy(capture->payload, value.data, value.size);
        }
        capture->payloadSize = value.size;
        capture->payloadKind = kind;
        ++capture->payloadCount;
    }

    static void fail(
        void* context,
        blinker::ErrorCode error,
        const blinker::RxContext&) {
        NoiseChannelCapture* capture =
            static_cast<NoiseChannelCapture*>(context);
        CHECK_TRUE(capture != nullptr);
        capture->fault = error;
        ++capture->faultCount;
    }

    uint8_t payload[blinker::kBleNoiseMaxTransportPayloadSize];
    size_t payloadSize;
    blinker::BleNoisePayloadKind payloadKind;
    size_t payloadCount;
    blinker::ErrorCode fault;
    size_t faultCount;
};

blinker::BleNoiseProvisioningConfig noiseChannelConfig(
    blinker::noise::NoiseNnPattern pattern =
        blinker::noise::NoiseNnPattern::NnPsk0) {
    blinker::BleNoiseProvisioningConfig config;
    config.pattern = pattern;
    config.reassemblyTimeoutMillis = 100U;
    config.secureSessionTimeoutMillis = 500U;
    config.maxPacketsPerPoll = 16U;
    return config;
}

struct NoiseChannelFixture {
    DeterministicTestCrypto crypto;
    FixedRandom deviceRandom;
    NoiseTestClock clock;
    NoiseTestBleLink link;
    uint8_t rx[blinker::kBleNoiseRecordSize];
    uint8_t tx[blinker::kBleNoiseRecordSize];
    uint8_t packet[244];
    uint8_t plaintext[blinker::kBleNoiseMaxTransportPayloadSize];
    blinker::BleNoiseProvisioningConfig config;
    blinker::BleNoiseProvisioningChannel channel;
    NoiseChannelCapture capture;

    explicit NoiseChannelFixture(
        bool autoStart = true,
        blinker::noise::NoiseNnPattern pattern =
            blinker::noise::NoiseNnPattern::NnPsk0)
        : crypto(),
          deviceRandom(0x51U),
          clock(),
          link(),
          rx(),
          tx(),
          packet(),
          plaintext(),
          config(noiseChannelConfig(pattern)),
          channel(
              link,
              clock,
              crypto,
              deviceRandom,
              blinker::MutableByteSpan(rx, sizeof(rx)),
              blinker::MutableByteSpan(tx, sizeof(tx)),
              blinker::MutableByteSpan(packet, sizeof(packet)),
              blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
              config),
          capture() {
        channel.setReceiver(&NoiseChannelCapture::receive, &capture);
        channel.setFaultHandler(&NoiseChannelCapture::fail, &capture);
        if (autoStart) {
            CHECK_TRUE(channel.start());
            link.connect();
        }
    }
};

void fillPsk(uint8_t psk[32], uint8_t seed = 0x31U) {
    for (size_t index = 0U; index < 32U; ++index) {
        psk[index] = static_cast<uint8_t>(seed + index);
    }
}

class TestSetupPskProvider : public blinker::ISetupPskProvider {
public:
    TestSetupPskProvider()
        : result(blinker::Result::success()),
          reportedSize(blinker::kSetupPskSize),
          allZero(false),
          calls(0U),
          lastSession() {}

    blinker::Result loadSetupPsk(
        blinker::ByteView setupSessionLocator,
        blinker::MutableByteSpan output,
        size_t& written) override {
        ++calls;
        written = 0U;
        if (setupSessionLocator.data == nullptr ||
            setupSessionLocator.size !=
                blinker::kSetupSessionLocatorSize ||
            output.data == nullptr ||
            output.size < blinker::kSetupPskSize) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        std::memcpy(
            lastSession,
            setupSessionLocator.data,
            sizeof(lastSession));
        if (allZero) {
            std::memset(output.data, 0, blinker::kSetupPskSize);
        } else {
            fillPsk(output.data);
        }
        written = reportedSize;
        return result;
    }

    blinker::Result result;
    size_t reportedSize;
    bool allZero;
    uint32_t calls;
    uint8_t lastSession[blinker::kSetupSessionLocatorSize];
};

class TestLocalProvisioningSession
    : public blinker::ILocalProvisioningSession {
public:
    TestLocalProvisioningSession()
        : beginResult(blinker::Result::success()),
          beginCalls(0U),
          secureCalls(0U),
          endCalls(0U),
          active(false),
          lastSession() {}

    blinker::Result beginSession(
        blinker::ByteView setupSessionLocator) override {
        ++beginCalls;
        if (setupSessionLocator.data == nullptr ||
            setupSessionLocator.size != sizeof(lastSession)) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        std::memcpy(
            lastSession,
            setupSessionLocator.data,
            sizeof(lastSession));
        active = beginResult.ok();
        return beginResult;
    }

    blinker::Result secureSessionReady(
        blinker::ByteView setupTranscriptHash) override {
        ++secureCalls;
        return setupTranscriptHash.data != nullptr &&
                       setupTranscriptHash.size ==
                           blinker::security::kNoiseKeySize
                   ? blinker::Result::success()
                   : blinker::Result::failure(
                         blinker::ErrorCode::InvalidArgument);
    }

    void endSession() override {
        ++endCalls;
        active = false;
        std::memset(lastSession, 0, sizeof(lastSession));
    }

    blinker::Result beginResult;
    uint32_t beginCalls;
    uint32_t secureCalls;
    uint32_t endCalls;
    bool active;
    uint8_t lastSession[blinker::kSetupSessionLocatorSize];
};

class TestLocalProvisioningApplication
    : public blinker::ILocalProvisioningApplication {
public:
    TestLocalProvisioningApplication()
        : handleResult(blinker::Result::success()),
          beginCalls(0U),
          secureCalls(0U),
          endCalls(0U),
          handleCalls(0U),
          active(false),
          lastRequest(),
          lastRequestSize(0U) {}

    blinker::Result beginSession(
        blinker::ByteView setupSessionLocator) override {
        ++beginCalls;
        if (setupSessionLocator.data == nullptr ||
            setupSessionLocator.size !=
                blinker::kSetupSessionLocatorSize) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        active = true;
        return blinker::Result::success();
    }

    blinker::Result secureSessionReady(
        blinker::ByteView setupTranscriptHash) override {
        ++secureCalls;
        if (setupTranscriptHash.data == nullptr ||
            setupTranscriptHash.size != sizeof(lastTranscriptHash)) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        std::memcpy(
            lastTranscriptHash,
            setupTranscriptHash.data,
            sizeof(lastTranscriptHash));
        return blinker::Result::success();
    }

    void endSession() override {
        ++endCalls;
        active = false;
        std::memset(lastRequest, 0, sizeof(lastRequest));
        lastRequestSize = 0U;
    }

    blinker::Result handle(
        blinker::ByteView request,
        blinker::MutableByteSpan operationWorkspace,
        blinker::MutableByteSpan output,
        blinker::ByteView& response) override {
        response = blinker::ByteView();
        ++handleCalls;
        if (!active) {
            return blinker::Result::failure(
                blinker::ErrorCode::NotConfigured);
        }
        if (!handleResult) return handleResult;
        if (request.data == nullptr || request.empty() ||
            request.size > sizeof(lastRequest) ||
            operationWorkspace.data == nullptr ||
            operationWorkspace.empty() || output.data == nullptr ||
            output.size < 3U) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        std::memcpy(lastRequest, request.data, request.size);
        lastRequestSize = request.size;
        std::memset(
            operationWorkspace.data,
            0xA5,
            operationWorkspace.size);
        output.data[0] = 0xA1U;
        output.data[1] = 0x00U;
        output.data[2] = static_cast<uint8_t>(handleCalls);
        response = blinker::ByteView(output.data, 3U);
        return blinker::Result::success();
    }

    blinker::Result handleResult;
    uint32_t beginCalls;
    uint32_t secureCalls;
    uint32_t endCalls;
    uint32_t handleCalls;
    bool active;
    uint8_t lastRequest[64];
    size_t lastRequestSize;
    uint8_t lastTranscriptHash[blinker::security::kNoiseKeySize] = {};
};

struct SessionPair {
    DeterministicTestCrypto crypto;
    FixedRandom initiatorRandom;
    FixedRandom responderRandom;
    blinker::security::NoiseNnSession initiator;
    blinker::security::NoiseNnSession responder;

    SessionPair()
        : crypto(),
          initiatorRandom(0x11U),
          responderRandom(0x51U),
          initiator(crypto, initiatorRandom),
          responder(crypto, responderRandom) {}
};

void establish(
    SessionPair& pair,
    uint8_t firstMessage[128],
    blinker::noise::NoiseNnPattern pattern =
        blinker::noise::NoiseNnPattern::NnPsk0) {
    uint8_t psk[32];
    fillPsk(psk);
    const blinker::ByteView sessionPsk =
        pattern == blinker::noise::NoiseNnPattern::NnPsk0
            ? blinker::ByteView(psk, sizeof(psk))
            : blinker::ByteView();
    const blinker::ByteView prologue =
        blinker::noise::localSecureNoisePrologue(pattern);
    CHECK_TRUE(pair.initiator.begin(
        blinker::security::NoiseRole::Initiator,
        pattern,
        sessionPsk,
        prologue));
    CHECK_TRUE(pair.responder.begin(
        blinker::security::NoiseRole::Responder,
        pattern,
        sessionPsk,
        prologue));

    const uint8_t initiatorPayload[] = {0xA2U, 0x00U, 0x01U, 0x01U, 0x01U};
    blinker::ByteView first;
    CHECK_TRUE(pair.initiator.writeHandshake(
        blinker::ByteView(initiatorPayload, sizeof(initiatorPayload)),
        blinker::MutableByteSpan(firstMessage, 128U),
        first));
    CHECK_EQ(
        sizeof(initiatorPayload) + blinker::security::kNoiseDhSize +
            (pattern == blinker::noise::NoiseNnPattern::NnPsk0
                 ? blinker::security::kNoiseTagSize
                 : 0U),
        first.size);

    uint8_t receivedPayload[32] = {};
    blinker::ByteView received;
    CHECK_TRUE(pair.responder.readHandshake(
        first,
        blinker::MutableByteSpan(receivedPayload, sizeof(receivedPayload)),
        received));
    CHECK_EQ(sizeof(initiatorPayload), received.size);
    CHECK_TRUE(std::memcmp(
        initiatorPayload,
        received.data,
        received.size) == 0);

    const uint8_t responderPayload[] = {0xA1U, 0x00U, 0x01U};
    uint8_t secondMessage[128] = {};
    blinker::ByteView second;
    CHECK_TRUE(pair.responder.writeHandshake(
        blinker::ByteView(responderPayload, sizeof(responderPayload)),
        blinker::MutableByteSpan(secondMessage, sizeof(secondMessage)),
        second));
    CHECK_TRUE(pair.initiator.readHandshake(
        second,
        blinker::MutableByteSpan(receivedPayload, sizeof(receivedPayload)),
        received));
    CHECK_EQ(sizeof(responderPayload), received.size);
    CHECK_TRUE(std::memcmp(
        responderPayload,
        received.data,
        received.size) == 0);
    CHECK_TRUE(pair.initiator.ready());
    CHECK_TRUE(pair.responder.ready());

    blinker::ByteView initiatorHash;
    blinker::ByteView responderHash;
    CHECK_TRUE(pair.initiator.handshakeHash(initiatorHash));
    CHECK_TRUE(pair.responder.handshakeHash(responderHash));
    CHECK_TRUE(blinker::constantTimeEqual(initiatorHash, responderHash));
}

void testHandshakeTransportAndRecords() {
    static_assert(
        sizeof(blinker::security::NoiseNnSession) <= 320U,
        "Noise session state exceeded the reviewed allocation ceiling");
    SessionPair pair;
    uint8_t firstMessage[128] = {};

    uint8_t psk[32];
    fillPsk(psk);
    CHECK_TRUE(pair.initiator.begin(
        blinker::security::NoiseRole::Initiator,
        blinker::noise::NoiseNnPattern::NnPsk0,
        blinker::ByteView(psk, sizeof(psk)),
        blinker::noise::localSecureNoisePrologue(
            blinker::noise::NoiseNnPattern::NnPsk0)));
    uint8_t tooSmall[47] = {};
    blinker::ByteView first;
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        pair.initiator.writeHandshake(
            blinker::ByteView(),
            blinker::MutableByteSpan(tooSmall, sizeof(tooSmall)),
            first).code());
    CHECK_EQ(
        blinker::security::NoiseSessionState::WriteInitiatorHandshake,
        pair.initiator.state());
    pair.initiator.clear();

    establish(pair, firstMessage);

    const uint8_t request[] = {
        0x01U, 0x04U, 0x00U, 0x07U, 0xA1U, 0x00U, 0x01U
    };
    uint8_t undersizedCiphertext[sizeof(request) + 15U] = {};
    blinker::ByteView retryCiphertext;
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        pair.initiator.encryptTransport(
            blinker::ByteView(request, sizeof(request)),
            blinker::MutableByteSpan(
                undersizedCiphertext,
                sizeof(undersizedCiphertext)),
            retryCiphertext).code());
    CHECK_TRUE(pair.initiator.ready());
    uint8_t ciphertextBytes[128] = {};
    blinker::ByteView ciphertext;
    CHECK_TRUE(pair.initiator.encryptTransport(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(ciphertextBytes, sizeof(ciphertextBytes)),
        ciphertext));
    CHECK_EQ(sizeof(request) + 16U, ciphertext.size);

    uint8_t recordBytes[160] = {};
    blinker::ByteView encodedRecord;
    CHECK_TRUE(blinker::noise::encodeLocalSecureRecord(
        blinker::noise::LocalSecureRecordType::Transport,
        ciphertext,
        blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
        encodedRecord));
    CHECK_EQ(ciphertext.size + 4U, encodedRecord.size);
    blinker::noise::LocalSecureRecordView record;
    CHECK_TRUE(blinker::noise::parseLocalSecureRecord(
        encodedRecord,
        record));
    CHECK_EQ(blinker::noise::LocalSecureRecordType::Transport, record.type);

    uint8_t plaintextBytes[128] = {};
    blinker::ByteView plaintext;
    CHECK_TRUE(pair.responder.decryptTransport(
        record.body,
        blinker::MutableByteSpan(plaintextBytes, sizeof(plaintextBytes)),
        plaintext));
    CHECK_EQ(sizeof(request), plaintext.size);
    CHECK_TRUE(std::memcmp(request, plaintext.data, plaintext.size) == 0);

    const uint8_t response[] = {0x01U, 0x05U, 0x00U, 0x07U};
    CHECK_TRUE(pair.responder.encryptTransport(
        blinker::ByteView(response, sizeof(response)),
        blinker::MutableByteSpan(ciphertextBytes, sizeof(ciphertextBytes)),
        ciphertext));
    CHECK_TRUE(pair.initiator.decryptTransport(
        ciphertext,
        blinker::MutableByteSpan(plaintextBytes, sizeof(plaintextBytes)),
        plaintext));
    CHECK_EQ(sizeof(response), plaintext.size);
    CHECK_TRUE(std::memcmp(response, plaintext.data, plaintext.size) == 0);
}

void testOrdinaryNoiseNnHandshakeAndPatternValidation() {
    SessionPair pair;
    uint8_t firstMessage[128] = {};
    establish(
        pair,
        firstMessage,
        blinker::noise::NoiseNnPattern::Nn);
    const uint8_t initiatorPayload[] = {
        0xA2U, 0x00U, 0x01U, 0x01U, 0x01U};
    CHECK_TRUE(std::memcmp(
        firstMessage + blinker::security::kNoiseDhSize,
        initiatorPayload,
        sizeof(initiatorPayload)) == 0);

    DeterministicTestCrypto crypto;
    FixedRandom random(0x21U);
    blinker::security::NoiseNnSession session(crypto, random);
    uint8_t psk[32] = {};
    fillPsk(psk);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        session.begin(
            blinker::security::NoiseRole::Initiator,
            blinker::noise::NoiseNnPattern::Nn,
            blinker::ByteView(psk, sizeof(psk)),
            blinker::noise::localSecureNoisePrologue(
                blinker::noise::NoiseNnPattern::Nn)).code());
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        session.begin(
            blinker::security::NoiseRole::Initiator,
            blinker::noise::NoiseNnPattern::NnPsk0,
            blinker::ByteView(),
            blinker::noise::localSecureNoisePrologue(
                blinker::noise::NoiseNnPattern::NnPsk0)).code());
    CHECK_TRUE(
        blinker::noise::localSecureNoisePrologue(
            blinker::noise::NoiseNnPattern::Nn).data[
                blinker::noise::localSecureNoisePrologue(
                    blinker::noise::NoiseNnPattern::Nn).size - 1U] !=
        blinker::noise::localSecureNoisePrologue(
            blinker::noise::NoiseNnPattern::NnPsk0).data[
                blinker::noise::localSecureNoisePrologue(
                    blinker::noise::NoiseNnPattern::NnPsk0).size - 1U]);
}

void testWrongPskTamperAndReplayFailClosed() {
    SessionPair pair;
    uint8_t firstMessage[128] = {};
    establish(pair, firstMessage);

    const uint8_t content[] = {0x01U, 0x02U, 0x03U};
    uint8_t ciphertextBytes[64] = {};
    blinker::ByteView ciphertext;
    CHECK_TRUE(pair.initiator.encryptTransport(
        blinker::ByteView(content, sizeof(content)),
        blinker::MutableByteSpan(ciphertextBytes, sizeof(ciphertextBytes)),
        ciphertext));
    uint8_t plaintext[16] = {};
    blinker::ByteView decoded;
    CHECK_TRUE(pair.responder.decryptTransport(
        ciphertext,
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        decoded));
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        pair.responder.decryptTransport(
            ciphertext,
            blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
            decoded).code());
    CHECK_EQ(
        blinker::security::NoiseSessionState::Failed,
        pair.responder.state());

    SessionPair tamperedPair;
    establish(tamperedPair, firstMessage);
    CHECK_TRUE(tamperedPair.initiator.encryptTransport(
        blinker::ByteView(content, sizeof(content)),
        blinker::MutableByteSpan(ciphertextBytes, sizeof(ciphertextBytes)),
        ciphertext));
    ciphertextBytes[ciphertext.size - 1U] ^= 0x01U;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        tamperedPair.responder.decryptTransport(
            ciphertext,
            blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
            decoded).code());
    CHECK_EQ(
        blinker::security::NoiseSessionState::Failed,
        tamperedPair.responder.state());

    DeterministicTestCrypto crypto;
    FixedRandom wrongRandom(0x71U);
    blinker::security::NoiseNnSession wrongResponder(
        crypto,
        wrongRandom);
    uint8_t wrongPsk[32];
    fillPsk(wrongPsk, 0x91U);
    CHECK_TRUE(wrongResponder.begin(
        blinker::security::NoiseRole::Responder,
        blinker::noise::NoiseNnPattern::NnPsk0,
        blinker::ByteView(wrongPsk, sizeof(wrongPsk)),
        blinker::noise::localSecureNoisePrologue(
            blinker::noise::NoiseNnPattern::NnPsk0)));
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        wrongResponder.readHandshake(
            blinker::ByteView(
                firstMessage,
                blinker::security::kNoiseMaxHandshakeOverhead + 5U),
            blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
            decoded).code());
    CHECK_EQ(
        blinker::security::NoiseSessionState::Failed,
        wrongResponder.state());
}

void testRecordValidation() {
    const uint8_t body[] = {0xAAU};
    uint8_t encoded[8] = {};
    blinker::ByteView view;
    CHECK_TRUE(blinker::noise::encodeLocalSecureRecord(
        blinker::noise::LocalSecureRecordType::InitiatorHandshake,
        blinker::ByteView(body, sizeof(body)),
        blinker::MutableByteSpan(encoded, sizeof(encoded)),
        view));
    blinker::noise::LocalSecureRecordView parsed;
    CHECK_TRUE(blinker::noise::parseLocalSecureRecord(view, parsed));
    size_t decodedSize = 0U;
    CHECK_TRUE(blinker::noise::decodeLocalSecureRecordSize(
        blinker::ByteView(encoded, 4U),
        decodedSize));
    CHECK_EQ(view.size, decodedSize);

    uint8_t invalid[8] = {};
    std::memcpy(invalid, encoded, view.size);
    invalid[0] ^= 0x01U;
    CHECK_EQ(
        blinker::ErrorCode::InvalidMagic,
        blinker::noise::parseLocalSecureRecord(
            blinker::ByteView(invalid, view.size),
            parsed).code());
    std::memcpy(invalid, encoded, view.size);
    invalid[1] = 0x11U;
    invalid[4] = body[0];
    invalid[5] = 0x00U;
    CHECK_EQ(
        blinker::ErrorCode::TrailingData,
        blinker::noise::parseLocalSecureRecord(
            blinker::ByteView(invalid, view.size + 1U),
            parsed).code());
    std::memcpy(invalid, encoded, view.size);
    invalid[1] = 0x14U;
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        blinker::noise::parseLocalSecureRecord(
            blinker::ByteView(invalid, view.size),
            parsed).code());
}

void testProtocolOrderAndTruncationFailClosed() {
    SessionPair pair;
    uint8_t firstMessage[128] = {};
    establish(pair, firstMessage);

    uint8_t output[64] = {};
    blinker::ByteView decoded;
    const uint8_t truncated[] = {0x01U, 0x02U, 0x03U};
    CHECK_EQ(
        blinker::ErrorCode::TruncatedInput,
        pair.responder.decryptTransport(
            blinker::ByteView(truncated, sizeof(truncated)),
            blinker::MutableByteSpan(output, sizeof(output)),
            decoded).code());
    CHECK_EQ(
        blinker::security::NoiseSessionState::Failed,
        pair.responder.state());

    DeterministicTestCrypto crypto;
    FixedRandom initiatorRandom(0x21U);
    uint8_t psk[32];
    fillPsk(psk, 0x31U);
    blinker::security::NoiseNnSession initiator(
        crypto,
        initiatorRandom);
    CHECK_TRUE(initiator.begin(
        blinker::security::NoiseRole::Initiator,
        blinker::noise::NoiseNnPattern::NnPsk0,
        blinker::ByteView(psk, sizeof(psk)),
        blinker::noise::localSecureNoisePrologue(
            blinker::noise::NoiseNnPattern::NnPsk0)));
    CHECK_EQ(
        blinker::ErrorCode::ProtocolError,
        initiator.readHandshake(
            blinker::ByteView(firstMessage, 48U),
            blinker::MutableByteSpan(output, sizeof(output)),
            decoded).code());
    CHECK_EQ(
        blinker::security::NoiseSessionState::Failed,
        initiator.state());
}

struct NoiseAppPeer {
    DeterministicTestCrypto crypto;
    FixedRandom random;
    blinker::security::NoiseNnSession session;

    NoiseAppPeer()
        : crypto(), random(0x11U), session(crypto, random) {}
};

void completeBleNoiseHandshake(
    NoiseChannelFixture& fixture,
    NoiseAppPeer& app) {
    uint8_t psk[32] = {};
    fillPsk(psk);
    CHECK_TRUE(fixture.channel.arm(
        blinker::ByteView(psk, sizeof(psk))));
    CHECK_TRUE(app.session.begin(
        blinker::security::NoiseRole::Initiator,
        blinker::noise::NoiseNnPattern::NnPsk0,
        blinker::ByteView(psk, sizeof(psk)),
        blinker::noise::localSecureNoisePrologue(
            blinker::noise::NoiseNnPattern::NnPsk0)));

    const uint8_t request[] = {0xA1U, 0x00U, 0x01U};
    uint8_t messageBytes[128] = {};
    blinker::ByteView message;
    CHECK_TRUE(app.session.writeHandshake(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(messageBytes, sizeof(messageBytes)),
        message));
    uint8_t recordBytes[160] = {};
    blinker::ByteView record;
    CHECK_TRUE(blinker::noise::encodeLocalSecureRecord(
        blinker::noise::LocalSecureRecordType::InitiatorHandshake,
        message,
        blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
        record));
    fixture.link.injectRecord(record);
    CHECK_EQ(static_cast<size_t>(1U), fixture.capture.payloadCount);
    CHECK_EQ(
        blinker::BleNoisePayloadKind::InitiatorHandshake,
        fixture.capture.payloadKind);
    CHECK_EQ(sizeof(request), fixture.capture.payloadSize);
    CHECK_TRUE(std::memcmp(
        request,
        fixture.capture.payload,
        sizeof(request)) == 0);

    const uint8_t response[] = {0xA1U, 0x01U, 0x02U};
    CHECK_TRUE(fixture.channel.sendHandshakeResponse(
        blinker::ByteView(response, sizeof(response))));
    fixture.channel.poll(1000U);
    uint8_t responseRecordBytes[160] = {};
    blinker::ByteView responseRecord;
    CHECK_TRUE(fixture.link.takeSentRecord(
        blinker::MutableByteSpan(
            responseRecordBytes,
            sizeof(responseRecordBytes)),
        responseRecord));
    blinker::noise::LocalSecureRecordView parsed;
    CHECK_TRUE(blinker::noise::parseLocalSecureRecord(
        responseRecord,
        parsed));
    CHECK_EQ(
        blinker::noise::LocalSecureRecordType::ResponderHandshake,
        parsed.type);
    uint8_t plaintext[64] = {};
    blinker::ByteView decoded;
    CHECK_TRUE(app.session.readHandshake(
        parsed.body,
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        decoded));
    CHECK_EQ(sizeof(response), decoded.size);
    CHECK_TRUE(std::memcmp(response, decoded.data, decoded.size) == 0);
    CHECK_TRUE(app.session.ready());
    CHECK_TRUE(fixture.channel.ready());
}

void injectEndpointHandshakeRequest(
    NoiseChannelFixture& fixture,
    NoiseAppPeer& app,
    blinker::ByteView request) {
    uint8_t messageBytes[160] = {};
    blinker::ByteView message;
    CHECK_TRUE(app.session.writeHandshake(
        request,
        blinker::MutableByteSpan(messageBytes, sizeof(messageBytes)),
        message));
    uint8_t recordBytes[192] = {};
    blinker::ByteView record;
    CHECK_TRUE(blinker::noise::encodeLocalSecureRecord(
        blinker::noise::LocalSecureRecordType::InitiatorHandshake,
        message,
        blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
        record));
    fixture.link.injectRecord(record);
}

void injectEndpointTransportRequest(
    NoiseChannelFixture& fixture,
    NoiseAppPeer& app,
    blinker::ByteView request) {
    uint8_t ciphertextBytes[160] = {};
    blinker::ByteView ciphertext;
    CHECK_TRUE(app.session.encryptTransport(
        request,
        blinker::MutableByteSpan(ciphertextBytes, sizeof(ciphertextBytes)),
        ciphertext));
    uint8_t recordBytes[192] = {};
    blinker::ByteView record;
    CHECK_TRUE(blinker::noise::encodeLocalSecureRecord(
        blinker::noise::LocalSecureRecordType::Transport,
        ciphertext,
        blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
        record));
    fixture.link.injectRecord(record);
}

void takeEndpointResponse(
    NoiseChannelFixture& fixture,
    NoiseAppPeer& app,
    blinker::noise::LocalSecureRecordType expectedType,
    uint8_t expectedSequence) {
    uint8_t recordBytes[192] = {};
    blinker::ByteView encoded;
    CHECK_TRUE(fixture.link.takeSentRecord(
        blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
        encoded));
    blinker::noise::LocalSecureRecordView record;
    CHECK_TRUE(blinker::noise::parseLocalSecureRecord(encoded, record));
    CHECK_EQ(expectedType, record.type);
    uint8_t plaintext[64] = {};
    blinker::ByteView decoded;
    const blinker::Result result =
        expectedType ==
                blinker::noise::LocalSecureRecordType::ResponderHandshake
            ? app.session.readHandshake(
                  record.body,
                  blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
                  decoded)
            : app.session.decryptTransport(
                  record.body,
                  blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
                  decoded);
    CHECK_TRUE(result);
    CHECK_EQ(static_cast<size_t>(3U), decoded.size);
    CHECK_EQ(static_cast<uint8_t>(0xA1U), decoded.data[0]);
    CHECK_EQ(static_cast<uint8_t>(0x00U), decoded.data[1]);
    CHECK_EQ(expectedSequence, decoded.data[2]);
}

void injectInitiatorRecordPrefix(
    NoiseChannelFixture& fixture,
    NoiseAppPeer& app,
    uint8_t recordId,
    bool wrongIndex) {
    uint8_t psk[32] = {};
    fillPsk(psk);
    CHECK_TRUE(fixture.channel.arm(
        blinker::ByteView(psk, sizeof(psk))));
    CHECK_TRUE(app.session.begin(
        blinker::security::NoiseRole::Initiator,
        blinker::noise::NoiseNnPattern::NnPsk0,
        blinker::ByteView(psk, sizeof(psk)),
        blinker::noise::localSecureNoisePrologue(
            blinker::noise::NoiseNnPattern::NnPsk0)));
    const uint8_t payload[] = {0x01U};
    uint8_t messageBytes[64] = {};
    blinker::ByteView message;
    CHECK_TRUE(app.session.writeHandshake(
        blinker::ByteView(payload, sizeof(payload)),
        blinker::MutableByteSpan(messageBytes, sizeof(messageBytes)),
        message));
    uint8_t recordBytes[80] = {};
    blinker::ByteView record;
    CHECK_TRUE(blinker::noise::encodeLocalSecureRecord(
        blinker::noise::LocalSecureRecordType::InitiatorHandshake,
        message,
        blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
        record));

    uint8_t packet[20] = {};
    blinker::ble::FragmentHeader header;
    header.flags = blinker::ble::FragmentFlagStart;
    header.frameId = recordId;
    header.index = 0U;
    CHECK_TRUE(blinker::ble::encodeFragmentHeader(
        header,
        blinker::MutableByteSpan(packet, sizeof(packet))));
    std::memcpy(
        packet + blinker::ble::kFragmentHeaderSize,
        record.data,
        16U);
    fixture.link.injectPacket(blinker::ByteView(packet, sizeof(packet)));

    if (!wrongIndex) return;
    header.flags = blinker::ble::FragmentFlagNone;
    header.index = 2U;
    CHECK_TRUE(blinker::ble::encodeFragmentHeader(
        header,
        blinker::MutableByteSpan(packet, sizeof(packet))));
    std::memcpy(
        packet + blinker::ble::kFragmentHeaderSize,
        record.data + 16U,
        16U);
    fixture.link.injectPacket(blinker::ByteView(packet, sizeof(packet)));
}

void testBleNoiseChannelRoundTripAndReplay() {
    NoiseChannelFixture fixture;
    NoiseAppPeer app;
    completeBleNoiseHandshake(fixture, app);

    const uint8_t request[] = {0x10U, 0x20U, 0x30U, 0x40U};
    uint8_t ciphertextBytes[64] = {};
    blinker::ByteView ciphertext;
    CHECK_TRUE(app.session.encryptTransport(
        blinker::ByteView(request, sizeof(request)),
        blinker::MutableByteSpan(ciphertextBytes, sizeof(ciphertextBytes)),
        ciphertext));
    uint8_t recordBytes[80] = {};
    blinker::ByteView record;
    CHECK_TRUE(blinker::noise::encodeLocalSecureRecord(
        blinker::noise::LocalSecureRecordType::Transport,
        ciphertext,
        blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
        record));
    fixture.link.injectRecord(record);
    CHECK_EQ(static_cast<size_t>(2U), fixture.capture.payloadCount);
    CHECK_EQ(blinker::BleNoisePayloadKind::Transport,
             fixture.capture.payloadKind);
    CHECK_EQ(sizeof(request), fixture.capture.payloadSize);
    CHECK_TRUE(std::memcmp(
        request,
        fixture.capture.payload,
        sizeof(request)) == 0);

    // A fresh BLE record id cannot make the same Noise ciphertext valid twice.
    fixture.link.injectRecord(record);
    CHECK_TRUE(!fixture.channel.armed());
    CHECK_EQ(
        blinker::security::NoiseSessionState::Empty,
        fixture.channel.noiseState());
    CHECK_EQ(static_cast<size_t>(1U), fixture.capture.faultCount);
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        fixture.capture.fault);
}

void testBleNoiseChannelOrderingTimeoutAndDisconnect() {
    {
        NoiseChannelFixture fixture;
        NoiseAppPeer app;
        injectInitiatorRecordPrefix(fixture, app, 31U, true);
        CHECK_TRUE(!fixture.channel.armed());
        CHECK_EQ(blinker::ErrorCode::ProtocolError, fixture.capture.fault);
    }
    {
        NoiseChannelFixture fixture;
        NoiseAppPeer app;
        injectInitiatorRecordPrefix(fixture, app, 32U, false);
        CHECK_TRUE(fixture.channel.armed());
        fixture.clock.advance(101U);
        fixture.channel.poll(1000U);
        CHECK_TRUE(!fixture.channel.armed());
        CHECK_EQ(blinker::ErrorCode::ProtocolError, fixture.capture.fault);
        CHECK_EQ(static_cast<uint32_t>(1U), fixture.link.disconnectRequests);
    }
    {
        NoiseChannelFixture fixture;
        uint8_t psk[32] = {};
        fillPsk(psk);
        CHECK_TRUE(fixture.channel.arm(
            blinker::ByteView(psk, sizeof(psk))));
        fixture.link.disconnect();
        CHECK_TRUE(!fixture.channel.armed());
        CHECK_EQ(static_cast<uint32_t>(0U),
                 fixture.channel.activeSessionId());
        CHECK_EQ(
            blinker::security::NoiseSessionState::Empty,
            fixture.channel.noiseState());
    }
    {
        NoiseChannelFixture fixture;
        uint8_t psk[32] = {};
        fillPsk(psk);
        CHECK_TRUE(fixture.channel.arm(
            blinker::ByteView(psk, sizeof(psk))));
        fixture.clock.advance(501U);
        fixture.channel.poll(1000U);
        CHECK_TRUE(!fixture.channel.armed());
        CHECK_EQ(blinker::ErrorCode::ProtocolError, fixture.capture.fault);
        CHECK_EQ(static_cast<uint32_t>(1U), fixture.link.disconnectRequests);
    }
    {
        NoiseChannelFixture fixture;
        uint8_t psk[32] = {};
        fillPsk(psk);
        CHECK_TRUE(fixture.channel.arm(
            blinker::ByteView(psk, sizeof(psk))));
        const uint8_t body[16] = {};
        uint8_t encoded[24] = {};
        blinker::ByteView record;
        CHECK_TRUE(blinker::noise::encodeLocalSecureRecord(
            blinker::noise::LocalSecureRecordType::Transport,
            blinker::ByteView(body, sizeof(body)),
            blinker::MutableByteSpan(encoded, sizeof(encoded)),
            record));
        fixture.link.injectRecord(record);
        CHECK_TRUE(!fixture.channel.armed());
        CHECK_EQ(blinker::ErrorCode::ProtocolError, fixture.capture.fault);
    }
}

void testBleNoiseChannelBackpressureDoesNotAdvanceNonce() {
    NoiseChannelFixture fixture;
    NoiseAppPeer app;
    completeBleNoiseHandshake(fixture, app);

    const uint8_t first[] = {0x51U, 0x52U};
    const uint8_t second[] = {0x61U, 0x62U, 0x63U};
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        fixture.channel.sendTransport(
            blinker::ByteView(fixture.tx, 1U)).code());
    CHECK_TRUE(fixture.channel.ready());
    CHECK_TRUE(fixture.channel.sendTransport(
        blinker::ByteView(first, sizeof(first))));
    CHECK_EQ(
        blinker::ErrorCode::WouldBlock,
        fixture.channel.sendTransport(
            blinker::ByteView(second, sizeof(second))).code());
    CHECK_TRUE(fixture.channel.ready());

    uint8_t recordBytes[96] = {};
    blinker::ByteView encoded;
    blinker::noise::LocalSecureRecordView record;
    uint8_t plaintext[32] = {};
    blinker::ByteView decoded;
    fixture.channel.poll(1000U);
    CHECK_TRUE(fixture.link.takeSentRecord(
        blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
        encoded));
    CHECK_TRUE(blinker::noise::parseLocalSecureRecord(encoded, record));
    CHECK_TRUE(app.session.decryptTransport(
        record.body,
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        decoded));
    CHECK_EQ(sizeof(first), decoded.size);
    CHECK_TRUE(std::memcmp(first, decoded.data, decoded.size) == 0);

    CHECK_TRUE(fixture.channel.sendTransport(
        blinker::ByteView(second, sizeof(second))));
    fixture.channel.poll(1000U);
    CHECK_TRUE(fixture.link.takeSentRecord(
        blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
        encoded));
    CHECK_TRUE(blinker::noise::parseLocalSecureRecord(encoded, record));
    CHECK_TRUE(app.session.decryptTransport(
        record.body,
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        decoded));
    CHECK_EQ(sizeof(second), decoded.size);
    CHECK_TRUE(std::memcmp(second, decoded.data, decoded.size) == 0);
}

void testBleLocalProvisioningEndpointFlowAndBackpressure() {
    NoiseChannelFixture fixture(false);
    TestLocalProvisioningApplication application;
    uint8_t operation[blinker::kBleEnrollmentWorkspaceSize] = {};
    uint8_t response[blinker::kBleEnrollmentWorkspaceSize] = {};
    blinker::BleLocalProvisioningEndpoint endpoint(
        fixture.channel,
        application,
        blinker::MutableByteSpan(operation, sizeof(operation)),
        blinker::MutableByteSpan(response, sizeof(response)));
    const uint8_t setupSession[] = {
        0x81U, 0x82U, 0x83U, 0x84U,
        0x85U, 0x86U, 0x87U, 0x88U};
    CHECK_TRUE(application.beginSession(blinker::ByteView(
        setupSession,
        sizeof(setupSession))));
    uint8_t psk[32] = {};
    fillPsk(psk);
    CHECK_TRUE(fixture.channel.arm(blinker::ByteView(psk, sizeof(psk))));
    CHECK_TRUE(endpoint.start());
    fixture.link.connect();
    CHECK_EQ(static_cast<uint32_t>(7U), endpoint.activeSessionId());

    NoiseAppPeer app;
    CHECK_TRUE(app.session.begin(
        blinker::security::NoiseRole::Initiator,
        blinker::noise::NoiseNnPattern::NnPsk0,
        blinker::ByteView(psk, sizeof(psk)),
        blinker::noise::localSecureNoisePrologue(
            blinker::noise::NoiseNnPattern::NnPsk0)));
    const uint8_t handshakeRequest[] = {0xA1U, 0x00U, 0x01U};
    injectEndpointHandshakeRequest(
        fixture,
        app,
        blinker::ByteView(handshakeRequest, sizeof(handshakeRequest)));
    CHECK_EQ(static_cast<uint32_t>(1U), application.handleCalls);
    CHECK_TRUE(!endpoint.responsePending());
    CHECK_TRUE(bytesAreZero(operation, sizeof(operation)));
    CHECK_TRUE(bytesAreZero(response, sizeof(response)));
    endpoint.poll(1000U);
    takeEndpointResponse(
        fixture,
        app,
        blinker::noise::LocalSecureRecordType::ResponderHandshake,
        1U);
    CHECK_TRUE(app.session.ready());
    CHECK_TRUE(fixture.channel.ready());
    CHECK_EQ(static_cast<uint32_t>(1U), application.secureCalls);
    blinker::ByteView appHandshakeHash;
    CHECK_TRUE(app.session.handshakeHash(appHandshakeHash));
    CHECK_TRUE(std::memcmp(
                   application.lastTranscriptHash,
                   appHandshakeHash.data,
                   appHandshakeHash.size) == 0);

    fixture.link.blockSends = true;
    const uint8_t firstRequest[] = {0x10U, 0x11U};
    injectEndpointTransportRequest(
        fixture,
        app,
        blinker::ByteView(firstRequest, sizeof(firstRequest)));
    CHECK_EQ(static_cast<uint32_t>(2U), application.handleCalls);
    CHECK_TRUE(!endpoint.responsePending());
    endpoint.poll(1000U);
    CHECK_EQ(static_cast<size_t>(0U), fixture.link.sentCount);

    const uint8_t secondRequest[] = {0x20U, 0x21U, 0x22U};
    injectEndpointTransportRequest(
        fixture,
        app,
        blinker::ByteView(secondRequest, sizeof(secondRequest)));
    CHECK_EQ(static_cast<uint32_t>(3U), application.handleCalls);
    CHECK_TRUE(endpoint.responsePending());
    CHECK_EQ(static_cast<uint8_t>(3U), response[2]);
    CHECK_TRUE(bytesAreZero(operation, sizeof(operation)));

    fixture.link.blockSends = false;
    endpoint.poll(1000U);
    CHECK_TRUE(!endpoint.responsePending());
    CHECK_TRUE(bytesAreZero(response, sizeof(response)));
    CHECK_EQ(static_cast<uint32_t>(3U), application.handleCalls);
    takeEndpointResponse(
        fixture,
        app,
        blinker::noise::LocalSecureRecordType::Transport,
        2U);
    endpoint.poll(1000U);
    takeEndpointResponse(
        fixture,
        app,
        blinker::noise::LocalSecureRecordType::Transport,
        3U);
    CHECK_EQ(static_cast<uint32_t>(3U), endpoint.metrics().requests);
    CHECK_EQ(static_cast<uint32_t>(3U), endpoint.metrics().responses);
    CHECK_TRUE(endpoint.metrics().backpressureEvents >= 2U);

    fixture.link.connect(99U);
    CHECK_TRUE(!endpoint.recoveryRequired());
    CHECK_EQ(
        static_cast<uint32_t>(1U),
        endpoint.metrics().ignoredForeignSessionEvents);
    endpoint.stop();
    CHECK_TRUE(!application.active);
    CHECK_TRUE(bytesAreZero(operation, sizeof(operation)));
    CHECK_TRUE(bytesAreZero(response, sizeof(response)));
}

void testBleLocalProvisioningEndpointRecoveryAndConfiguration() {
    for (unsigned int failureStage = 0U; failureStage < 3U; ++failureStage) {
        NoiseChannelFixture fixture(false);
        TestLocalProvisioningApplication application;
        uint8_t operation[blinker::kBleEnrollmentWorkspaceSize] = {};
        uint8_t response[blinker::kBleEnrollmentWorkspaceSize] = {};
        blinker::BleLocalProvisioningEndpoint endpoint(
            fixture.channel,
            application,
            blinker::MutableByteSpan(operation, sizeof(operation)),
            blinker::MutableByteSpan(response, sizeof(response)));
        const uint8_t setupSession[] = {
            1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
        CHECK_TRUE(application.beginSession(blinker::ByteView(
            setupSession,
            sizeof(setupSession))));
        uint8_t psk[32] = {};
        fillPsk(psk);
        CHECK_TRUE(fixture.channel.arm(
            blinker::ByteView(psk, sizeof(psk))));
        CHECK_TRUE(endpoint.start());
        fixture.link.connect();
        if (failureStage == 0U) {
            fixture.link.disconnect();
        } else {
            NoiseAppPeer app;
            CHECK_TRUE(app.session.begin(
                blinker::security::NoiseRole::Initiator,
                blinker::noise::NoiseNnPattern::NnPsk0,
                blinker::ByteView(psk, sizeof(psk)),
                blinker::noise::localSecureNoisePrologue(blinker::noise::NoiseNnPattern::NnPsk0)));
            const uint8_t request[] = {0xa1U, 0U, 1U};
            if (failureStage == 1U) application.handleResult = blinker::Result::failure(
                blinker::ErrorCode::AuthenticationRequired);
            injectEndpointHandshakeRequest(fixture, app, blinker::ByteView(request, sizeof(request)));
            if (failureStage == 2U) {
                endpoint.poll(1000U);
                takeEndpointResponse(fixture, app, blinker::noise::LocalSecureRecordType::ResponderHandshake, 1U);
                CHECK_EQ(static_cast<uint32_t>(7U), endpoint.activeSessionId());
                application.handleResult = blinker::Result::failure(blinker::ErrorCode::AuthenticationRequired);
                injectEndpointTransportRequest(fixture, app, blinker::ByteView(request, sizeof(request)));
            }
        }
        CHECK_TRUE(endpoint.recoveryRequired());
        CHECK_EQ(failureStage == 0U ? blinker::ErrorCode::NotConnected
                                   : blinker::ErrorCode::AuthenticationRequired, endpoint.lastError());
        CHECK_EQ(blinker::TransportState::Error, endpoint.state());
        CHECK_TRUE(!application.active);
        endpoint.poll(1000U);
        CHECK_EQ(blinker::BleLinkState::Stopped, fixture.link.state());
        // stop() need not emit an onDisconnected callback. The endpoint must
        // release its own session identity, or the setup lifecycle retries
        // enrollment despite a durable install instead of selecting Direct.
        CHECK_EQ(static_cast<uint32_t>(0U), endpoint.activeSessionId());
        CHECK_EQ(static_cast<uint32_t>(0U), fixture.channel.activeSessionId());
        CHECK_TRUE(bytesAreZero(operation, sizeof(operation)));
        CHECK_TRUE(bytesAreZero(response, sizeof(response)));
    }
    {
        NoiseChannelFixture fixture(false);
        TestLocalProvisioningApplication application;
    uint8_t shared[blinker::kBleEnrollmentWorkspaceSize] = {};
        const uint8_t setupSession[] = {
            1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
        CHECK_TRUE(application.beginSession(blinker::ByteView(
            setupSession,
            sizeof(setupSession))));
        uint8_t psk[32] = {};
        fillPsk(psk);
        CHECK_TRUE(fixture.channel.arm(
            blinker::ByteView(psk, sizeof(psk))));
        blinker::BleLocalProvisioningEndpoint endpoint(
            fixture.channel,
            application,
            blinker::MutableByteSpan(shared, sizeof(shared)),
            blinker::MutableByteSpan(shared, sizeof(shared)));
        CHECK_EQ(
            blinker::ErrorCode::NotConfigured,
            endpoint.start().code());
        CHECK_TRUE(!application.active);
        CHECK_TRUE(!fixture.channel.armed());
    }
}

void testBleNoiseModePreparationIsSessionBoundAndFailClosed() {
    NoiseChannelFixture fixture;
    TestSetupPskProvider provider;
    TestLocalProvisioningSession localSession;
    blinker::BleNoiseModePreparation preparation(
        fixture.channel,
        provider,
        localSession);
    const uint8_t session[] = {
        0x81U, 0x82U, 0x83U, 0x84U,
        0x85U, 0x86U, 0x87U, 0x88U};
    const blinker::ble::ModeProfile provisioning =
        blinker::ble::makePskProvisioningModeProfile(
            blinker::ByteView(session, sizeof(session)),
            true);

    CHECK_TRUE(preparation.prepareBleMode(provisioning, false));
    CHECK_TRUE(fixture.channel.armed());
    CHECK_TRUE(localSession.active);
    CHECK_EQ(static_cast<uint32_t>(1U), localSession.beginCalls);
    CHECK_EQ(static_cast<uint32_t>(1U), provider.calls);
    CHECK_TRUE(std::memcmp(
        session,
        provider.lastSession,
        sizeof(session)) == 0);

    const blinker::ble::ModeProfile direct =
        blinker::ble::makeDirectModeProfile();
    CHECK_TRUE(preparation.prepareBleMode(direct, false));
    CHECK_TRUE(!fixture.channel.armed());
    CHECK_TRUE(!localSession.active);
    CHECK_EQ(static_cast<uint32_t>(1U), provider.calls);

    provider.reportedSize = blinker::kSetupPskSize - 1U;
    CHECK_EQ(
        blinker::ErrorCode::ProtocolError,
        preparation.prepareBleMode(provisioning, true).code());
    CHECK_TRUE(!fixture.channel.armed());
    CHECK_TRUE(!localSession.active);

    provider.reportedSize = blinker::kSetupPskSize;
    provider.result = blinker::Result::failure(
        blinker::ErrorCode::NotConfigured);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        preparation.prepareBleMode(provisioning, false).code());
    CHECK_TRUE(!fixture.channel.armed());
    CHECK_TRUE(!localSession.active);

    provider.result = blinker::Result::success();
    provider.allZero = true;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        preparation.prepareBleMode(provisioning, false).code());
    CHECK_TRUE(!fixture.channel.armed());
    CHECK_TRUE(!localSession.active);

    provider.allZero = false;
    CHECK_TRUE(preparation.prepareBleMode(provisioning, true));
    CHECK_TRUE(fixture.channel.armed());
    CHECK_TRUE(localSession.active);

    localSession.beginResult = blinker::Result::failure(
        blinker::ErrorCode::StateConflict);
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        preparation.prepareBleMode(provisioning, false).code());
    CHECK_TRUE(!fixture.channel.armed());
    CHECK_TRUE(!localSession.active);
    localSession.beginResult = blinker::Result::success();
    CHECK_TRUE(preparation.prepareBleMode(provisioning, true));
    CHECK_TRUE(localSession.active);
    const uint32_t providerCalls = provider.calls;
    blinker::ble::ModeProfile invalid = direct;
    invalid.modeLocator[0] = 1U;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        preparation.prepareBleMode(invalid, false).code());
    CHECK_TRUE(!fixture.channel.armed());
    CHECK_TRUE(!localSession.active);
    CHECK_EQ(providerCalls, provider.calls);
}

void testOrdinaryBleNoiseModePreparationNeedsNoSetupPsk() {
    NoiseChannelFixture fixture(
        true,
        blinker::noise::NoiseNnPattern::Nn);
    TestLocalProvisioningSession localSession;
    blinker::BleNoiseModePreparation preparation(
        fixture.channel,
        localSession);
    const uint8_t session[] = {
        0x11U, 0x12U, 0x13U, 0x14U,
        0x15U, 0x16U, 0x17U, 0x18U};
    const blinker::ble::ModeProfile provisioning =
        blinker::ble::makeProvisioningModeProfile(
            blinker::ByteView(session, sizeof(session)),
            true);
    CHECK_TRUE(preparation.prepareBleMode(provisioning, false));
    CHECK_TRUE(fixture.channel.armed());
    CHECK_TRUE(localSession.active);

    const blinker::ble::ModeProfile enhanced =
        blinker::ble::makePskProvisioningModeProfile(
            blinker::ByteView(session, sizeof(session)),
            true);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        preparation.prepareBleMode(enhanced, false).code());
    CHECK_TRUE(!fixture.channel.armed());
    CHECK_TRUE(!localSession.active);
}

} // namespace

int main() {
    testHandshakeTransportAndRecords();
    testOrdinaryNoiseNnHandshakeAndPatternValidation();
    testWrongPskTamperAndReplayFailClosed();
    testRecordValidation();
    testProtocolOrderAndTruncationFailClosed();
    testBleNoiseChannelRoundTripAndReplay();
    testBleNoiseChannelOrderingTimeoutAndDisconnect();
    testBleNoiseChannelBackpressureDoesNotAdvanceNonce();
    testBleLocalProvisioningEndpointFlowAndBackpressure();
    testBleLocalProvisioningEndpointRecoveryAndConfiguration();
    testBleNoiseModePreparationIsSessionBoundAndFailClosed();
    testOrdinaryBleNoiseModePreparationNeedsNoSetupPsk();
    std::cout << "portable Noise state tests passed; session="
              << sizeof(blinker::security::NoiseNnSession)
              << " B; BLE secure channel="
              << sizeof(blinker::BleNoiseProvisioningChannel)
              << " B; mode preparation="
               << sizeof(blinker::BleNoiseModePreparation)
              << " B; local endpoint="
              << sizeof(blinker::BleLocalProvisioningEndpoint)
              << " B" << std::endl;
    return 0;
}
