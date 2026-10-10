#include <BlinkerV2/core/HmacSha256.h>
#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/protocol/bbp2/Frame.h>
#include <BlinkerV2/protocol/ble/Fragment.h>
#include <BlinkerV2/security/DirectSecureSession.h>
#include <BlinkerV2/transport/BleFrameTransport.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Direct secure check failed at line " << __LINE__    \
                      << ": " << #condition << std::endl;                      \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class DeterministicAead final
    : public blinker::IX25519AesGcmCryptoProvider {
public:
    blinker::Result x25519PublicKey(
        blinker::ByteView,
        blinker::MutableByteSpan) override {
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }

    blinker::Result x25519(
        blinker::ByteView,
        blinker::ByteView,
        blinker::MutableByteSpan) override {
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }

    blinker::Result aes256GcmEncrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView plaintext,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!valid(key, nonce, associatedData) || plaintext.data == nullptr ||
            output.data == nullptr || output.size < plaintext.size + 16U) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        for (size_t index = 0U; index < plaintext.size; ++index) {
            output.data[index] = static_cast<uint8_t>(
                plaintext.data[index] ^ key.data[index % key.size] ^
                nonce.data[index % nonce.size]);
        }
        uint8_t tag[32];
        blinker::Result result = computeTag(
            key,
            nonce,
            associatedData,
            blinker::ByteView(output.data, plaintext.size),
            tag);
        if (result) {
            std::memcpy(output.data + plaintext.size, tag, 16U);
            written = plaintext.size + 16U;
        }
        blinker::secureZero(blinker::MutableByteSpan(tag, sizeof(tag)));
        return result;
    }

    blinker::Result aes256GcmDecrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView ciphertextAndTag,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!valid(key, nonce, associatedData) ||
            ciphertextAndTag.data == nullptr ||
            ciphertextAndTag.size < 16U) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        const size_t ciphertextSize = ciphertextAndTag.size - 16U;
        if (output.data == nullptr || output.size < ciphertextSize) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        uint8_t tag[32];
        blinker::Result result = computeTag(
            key,
            nonce,
            associatedData,
            blinker::ByteView(ciphertextAndTag.data, ciphertextSize),
            tag);
        if (result && !blinker::constantTimeEqual(
                          blinker::ByteView(tag, 16U),
                          blinker::ByteView(
                              ciphertextAndTag.data + ciphertextSize,
                              16U))) {
            result = blinker::Result::failure(
                blinker::ErrorCode::AuthenticationRequired);
        }
        if (result) {
            for (size_t index = 0U; index < ciphertextSize; ++index) {
                output.data[index] = static_cast<uint8_t>(
                    ciphertextAndTag.data[index] ^ key.data[index % key.size] ^
                    nonce.data[index % nonce.size]);
            }
            written = ciphertextSize;
        }
        blinker::secureZero(blinker::MutableByteSpan(tag, sizeof(tag)));
        return result;
    }

private:
    static bool valid(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData) {
        return key.data != nullptr && key.size == 32U &&
               nonce.data != nullptr && nonce.size == 12U &&
               (associatedData.data != nullptr || associatedData.empty());
    }

    static blinker::Result computeTag(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView ciphertext,
        uint8_t output[32]) {
        blinker::HmacSha256 hmac;
        blinker::Result result = hmac.begin(key);
        if (result) result = hmac.update(nonce);
        if (result) result = hmac.update(associatedData);
        if (result) result = hmac.update(ciphertext);
        return result
                   ? hmac.finish(blinker::MutableByteSpan(output, 32U))
                   : result;
    }
};

class TestClock final : public blinker::IClock {
public:
    TestClock() : nowMillis(0U) {}
    uint32_t monotonicMillis() const override { return nowMillis; }
    bool unixTime(uint64_t&) const override { return false; }
    void advance(uint32_t millis) { nowMillis += millis; }

    uint32_t nowMillis;
};

class TestBleLink final : public blinker::IBleLink {
public:
    struct Packet {
        uint8_t bytes[20U];
        size_t size;

        Packet() : bytes(), size(0U) {}
    };

    TestBleLink()
        : current(blinker::BleLinkState::Stopped),
          session(),
          receiver(nullptr),
          receiverContext(nullptr),
          connected(nullptr),
          disconnected(nullptr),
          sessionContext(nullptr),
          packets(),
          packetCount(0U),
          disconnectRequests(0U) {}

    blinker::Result start() override {
        current = blinker::BleLinkState::Ready;
        return blinker::Result::success();
    }
    void stop() override { current = blinker::BleLinkState::Stopped; }
    void poll(uint32_t) override {}
    blinker::BleLinkState state() const override { return current; }
    blinker::ErrorCode lastError() const override {
        return blinker::ErrorCode::Ok;
    }
    size_t sessionCount() const override {
        return session.connected ? 1U : 0U;
    }
    blinker::Result sessionAt(
        size_t index,
        blinker::BleSessionInfo& output) const override {
        if (index != 0U || !session.connected) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        output = session;
        return blinker::Result::success();
    }
    blinker::Result sendPacket(
        uint32_t sessionId,
        blinker::ByteView packet) override {
        if (!session.connected || sessionId != session.sessionId) {
            return blinker::Result::failure(
                blinker::ErrorCode::NotConnected);
        }
        if (packetCount >= sizeof(packets) / sizeof(packets[0]) ||
            packet.size > sizeof(packets[0].bytes)) {
            return blinker::Result::failure(
                blinker::ErrorCode::CapacityExceeded);
        }
        packets[packetCount].size = packet.size;
        std::memcpy(
            packets[packetCount].bytes,
            packet.data,
            packet.size);
        ++packetCount;
        return blinker::Result::success();
    }
    blinker::Result disconnectSession(uint32_t sessionId) override {
        if (!session.connected || session.sessionId != sessionId) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        session.connected = false;
        ++disconnectRequests;
        return blinker::Result::success();
    }
    void setPacketReceiver(
        blinker::BlePacketReceiver callback,
        void* context) override {
        receiver = callback;
        receiverContext = context;
    }
    void setSessionHandlers(
        blinker::BleSessionHandler onConnected,
        blinker::BleSessionHandler onDisconnected,
        void* context) override {
        connected = onConnected;
        disconnected = onDisconnected;
        sessionContext = context;
    }

    void connect(uint32_t sessionId) {
        session.sessionId = sessionId;
        session.maxPacketSize = 20U;
        session.connected = true;
        session.notifyEnabled = true;
        session.encrypted = false;
        session.bonded = false;
        session.authenticated = false;
        if (connected != nullptr) connected(sessionContext, session);
    }

    void clearPackets() { packetCount = 0U; }

    blinker::ByteView sentRecord(
        uint8_t* output,
        size_t capacity) const {
        size_t written = 0U;
        for (size_t index = 0U; index < packetCount; ++index) {
            blinker::ble::FragmentView fragment;
            CHECK_TRUE(blinker::ble::parseFragment(
                blinker::ByteView(packets[index].bytes, packets[index].size),
                fragment));
            CHECK_EQ(static_cast<uint8_t>(index), fragment.header.index);
            CHECK_TRUE(written + fragment.payload.size <= capacity);
            std::memcpy(
                output + written,
                fragment.payload.data,
                fragment.payload.size);
            written += fragment.payload.size;
        }
        return blinker::ByteView(output, written);
    }

    void injectRecord(blinker::ByteView record, uint8_t recordId) {
        CHECK_TRUE(receiver != nullptr);
        size_t offset = 0U;
        uint8_t index = 0U;
        while (offset < record.size) {
            const size_t remaining = record.size - offset;
            const size_t payloadSize = remaining < 16U ? remaining : 16U;
            uint8_t packet[20U];
            blinker::ble::FragmentHeader header;
            header.flags = offset == 0U
                               ? blinker::ble::FragmentFlagStart
                               : blinker::ble::FragmentFlagNone;
            if (payloadSize == remaining) {
                header.flags = static_cast<uint8_t>(
                    header.flags | blinker::ble::FragmentFlagEnd);
            }
            header.frameId = recordId;
            header.index = index++;
            CHECK_TRUE(blinker::ble::encodeFragmentHeader(
                header,
                blinker::MutableByteSpan(packet, sizeof(packet))));
            std::memcpy(
                packet + blinker::ble::kFragmentHeaderSize,
                record.data + offset,
                payloadSize);
            receiver(
                receiverContext,
                session,
                blinker::ByteView(
                    packet,
                    blinker::ble::kFragmentHeaderSize + payloadSize));
            offset += payloadSize;
        }
    }

    blinker::BleLinkState current;
    blinker::BleSessionInfo session;
    blinker::BlePacketReceiver receiver;
    void* receiverContext;
    blinker::BleSessionHandler connected;
    blinker::BleSessionHandler disconnected;
    void* sessionContext;
    Packet packets[8U];
    size_t packetCount;
    uint32_t disconnectRequests;
};

struct FrameCapture {
    uint8_t bytes[64U];
    size_t size;
    uint32_t disconnects;
    bool encrypted;

    FrameCapture()
        : bytes(), size(0U), disconnects(0U), encrypted(false) {}
};

void captureFrame(
    void* context,
    blinker::ByteView frame,
    const blinker::RxContext& rx) {
    FrameCapture* capture = static_cast<FrameCapture*>(context);
    CHECK_TRUE(frame.size <= sizeof(capture->bytes));
    std::memcpy(capture->bytes, frame.data, frame.size);
    capture->size = frame.size;
    capture->encrypted = rx.encrypted;
}

void captureDisconnect(
    void* context,
    const blinker::RxContext&) {
    ++static_cast<FrameCapture*>(context)->disconnects;
}

struct Fixture {
    DeterministicAead crypto;
    uint8_t secret[blinker::kControllerCredentialSecretSize];
    uint8_t controllerId[blinker::kControllerIdSize];
    uint8_t clientNonce[blinker::security::kDirectSecureAuthNonceSize];
    uint8_t deviceNonce[blinker::security::kDirectSecureAuthNonceSize];
    blinker::security::DirectSecureContext context;
    blinker::security::DirectSecureSession initiator;
    blinker::security::DirectSecureSession responder;

    Fixture()
        : crypto(),
          secret(),
          controllerId(),
          clientNonce(),
          deviceNonce(),
          context(),
          initiator(),
          responder() {
        for (size_t index = 0U; index < sizeof(secret); ++index) {
            secret[index] = static_cast<uint8_t>(0x80U + index);
        }
        for (size_t index = 0U; index < sizeof(controllerId); ++index) {
            controllerId[index] = static_cast<uint8_t>(0x10U + index);
            clientNonce[index] = static_cast<uint8_t>(0x30U + index);
            deviceNonce[index] = static_cast<uint8_t>(0x50U + index);
        }
        context.controllerId = blinker::ByteView(
            controllerId,
            sizeof(controllerId));
        context.domain =
            blinker::ControllerCredentialDomain::PlatformAccess;
        context.ownershipGeneration = 0x01020304U;
        context.credentialVersion = 7U;
        context.permissions =
            blinker::kControllerPermissionObserve |
            blinker::kControllerPermissionControl;
        context.clientNonce = blinker::ByteView(
            clientNonce,
            sizeof(clientNonce));
        context.deviceNonce = blinker::ByteView(
            deviceNonce,
            sizeof(deviceNonce));
    }

    void begin() {
        CHECK_TRUE(initiator.begin(
            blinker::security::DirectSecureRole::Initiator,
            blinker::ByteView(secret, sizeof(secret)),
            context));
        CHECK_TRUE(responder.begin(
            blinker::security::DirectSecureRole::Responder,
            blinker::ByteView(secret, sizeof(secret)),
            context));
        CHECK_TRUE(blinker::constantTimeEqual(
            initiator.sessionBinding(),
            responder.sessionBinding()));
    }
};

blinker::ByteView makeFrame(uint8_t* output, size_t size, uint16_t sequence) {
    CHECK_TRUE(size >= blinker::bbp2::kBaseHeaderSize);
    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(blinker::bbp2::MessageKind::Patch);
    header.sequence = sequence;
    header.bodyLength = static_cast<uint16_t>(
        size - blinker::bbp2::kBaseHeaderSize);
    CHECK_TRUE(blinker::bbp2::encodeHeader(
        header,
        blinker::MutableByteSpan(output, size)));
    for (size_t index = blinker::bbp2::kBaseHeaderSize; index < size; ++index) {
        output[index] = static_cast<uint8_t>(0xA0U + index);
    }
    return blinker::ByteView(output, size);
}

void testRoundTripAndDirections() {
    Fixture fixture;
    fixture.begin();
    uint8_t frameStorage[37];
    const blinker::ByteView frame = makeFrame(
        frameStorage,
        sizeof(frameStorage),
        9U);
    uint8_t recordStorage[80];
    blinker::ByteView record;
    CHECK_TRUE(fixture.initiator.seal(
        fixture.crypto,
        frame,
        blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
        record));
    CHECK_EQ(
        blinker::security::directSecureRecordSize(frame.size),
        record.size);
    CHECK_EQ(blinker::security::kDirectSecureMagic, record.data[0]);
    size_t decodedSize = 0U;
    CHECK_TRUE(blinker::security::decodeDirectSecureRecordSize(
        record,
        decodedSize));
    CHECK_EQ(record.size, decodedSize);

    uint8_t openedStorage[64];
    blinker::ByteView opened;
    CHECK_TRUE(fixture.responder.open(
        fixture.crypto,
        record,
        blinker::MutableByteSpan(openedStorage, sizeof(openedStorage)),
        opened));
    CHECK_EQ(frame.size, opened.size);
    CHECK_TRUE(std::memcmp(frame.data, opened.data, frame.size) == 0);
    CHECK_EQ(1U, fixture.initiator.nextSendSequence());
    CHECK_EQ(1U, fixture.responder.nextReceiveSequence());

    const blinker::ByteView response = makeFrame(
        frameStorage,
        sizeof(frameStorage),
        10U);
    CHECK_TRUE(fixture.responder.seal(
        fixture.crypto,
        response,
        blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
        record));
    CHECK_TRUE(fixture.initiator.open(
        fixture.crypto,
        record,
        blinker::MutableByteSpan(openedStorage, sizeof(openedStorage)),
        opened));
    CHECK_TRUE(std::memcmp(response.data, opened.data, response.size) == 0);
}

void testFrozenKeyMaterial() {
    Fixture fixture;
    blinker::security::DirectSecureKeyMaterial material;
    CHECK_TRUE(blinker::security::deriveDirectSecureKeyMaterial(
        blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
        fixture.context,
        material));
    const uint8_t expected[] = {
        0xF6, 0xC5, 0xE5, 0xC7, 0xC0, 0x6C, 0xC6, 0x41,
        0x85, 0x95, 0x77, 0x86, 0x91, 0x7C, 0xD1, 0xA2,
        0x13, 0x95, 0xF7, 0xA9, 0xF8, 0x43, 0xC6, 0x19,
        0x69, 0x35, 0xD3, 0xD9, 0x0C, 0x0A, 0xBC, 0xD4,
        0x4C, 0x60, 0x66, 0xBE, 0xE7, 0x73, 0xE6, 0xE2,
        0xFE, 0x49, 0x00, 0x44, 0x55, 0xF6, 0x27, 0x6D,
        0xB3, 0x50, 0x6B, 0xD2, 0x6C, 0x85, 0x57, 0xC5,
        0x47, 0x5A, 0xFC, 0x66, 0x8B, 0xA9, 0xC7, 0x95,
        0xF9, 0x7F, 0x07, 0xEF, 0x4E, 0x3D, 0x9A, 0x1C,
        0xDF, 0x7D, 0x3B, 0x37, 0x30, 0x21, 0x4B, 0xAD,
        0x9C, 0x48, 0x88, 0xB5, 0x35, 0x13, 0x5D, 0x5C,
        0xF7, 0xBE, 0x9B, 0x08, 0xDC, 0xBC, 0x0B, 0xD6
    };
    static_assert(
        sizeof(expected) == sizeof(material),
        "frozen direct key material size changed");
    CHECK_TRUE(std::memcmp(&material, expected, sizeof(expected)) == 0);
    material.clear();
}

void testCapacityDoesNotConsumeSequence() {
    Fixture fixture;
    fixture.begin();
    uint8_t frameStorage[24];
    const blinker::ByteView frame = makeFrame(
        frameStorage,
        sizeof(frameStorage),
        1U);
    uint8_t tooSmall[20];
    blinker::ByteView encoded;
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        fixture.initiator.seal(
            fixture.crypto,
            frame,
            blinker::MutableByteSpan(tooSmall, sizeof(tooSmall)),
            encoded).code());
    CHECK_TRUE(fixture.initiator.ready());
    CHECK_EQ(0U, fixture.initiator.nextSendSequence());

    uint8_t recordStorage[64];
    CHECK_TRUE(fixture.initiator.seal(
        fixture.crypto,
        frame,
        blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
        encoded));
    uint8_t tinyPlaintext[10];
    blinker::ByteView plaintext;
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        fixture.responder.open(
            fixture.crypto,
            encoded,
            blinker::MutableByteSpan(tinyPlaintext, sizeof(tinyPlaintext)),
            plaintext).code());
    CHECK_TRUE(fixture.responder.ready());
    CHECK_EQ(0U, fixture.responder.nextReceiveSequence());
    uint8_t plaintextStorage[32];
    CHECK_TRUE(fixture.responder.open(
        fixture.crypto,
        encoded,
        blinker::MutableByteSpan(
            plaintextStorage,
            sizeof(plaintextStorage)),
        plaintext));
}

void testReplayAndTamperFailClosed() {
    Fixture fixture;
    fixture.begin();
    uint8_t frameStorage[20];
    const blinker::ByteView frame = makeFrame(
        frameStorage,
        sizeof(frameStorage),
        2U);
    uint8_t recordStorage[64];
    blinker::ByteView encoded;
    CHECK_TRUE(fixture.initiator.seal(
        fixture.crypto,
        frame,
        blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
        encoded));
    uint8_t plaintextStorage[32];
    blinker::ByteView plaintext;
    CHECK_TRUE(fixture.responder.open(
        fixture.crypto,
        encoded,
        blinker::MutableByteSpan(
            plaintextStorage,
            sizeof(plaintextStorage)),
        plaintext));
    CHECK_EQ(
        blinker::ErrorCode::SequenceConflict,
        fixture.responder.open(
            fixture.crypto,
            encoded,
            blinker::MutableByteSpan(
                plaintextStorage,
                sizeof(plaintextStorage)),
            plaintext).code());
    CHECK_TRUE(!fixture.responder.ready());

    Fixture tampered;
    tampered.begin();
    CHECK_TRUE(tampered.initiator.seal(
        tampered.crypto,
        frame,
        blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
        encoded));
    recordStorage[encoded.size - 1U] ^= 0x01U;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        tampered.responder.open(
            tampered.crypto,
            encoded,
            blinker::MutableByteSpan(
                plaintextStorage,
                sizeof(plaintextStorage)),
            plaintext).code());
    CHECK_TRUE(!tampered.responder.ready());
}

void testContextAndDirectionBinding() {
    Fixture fixture;
    fixture.begin();
    uint8_t frameStorage[20];
    const blinker::ByteView frame = makeFrame(
        frameStorage,
        sizeof(frameStorage),
        3U);
    uint8_t recordStorage[64];
    blinker::ByteView encoded;
    CHECK_TRUE(fixture.initiator.seal(
        fixture.crypto,
        frame,
        blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
        encoded));
    uint8_t plaintextStorage[32];
    blinker::ByteView plaintext;
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        fixture.initiator.open(
            fixture.crypto,
            encoded,
            blinker::MutableByteSpan(
                plaintextStorage,
                sizeof(plaintextStorage)),
            plaintext).code());

    Fixture changed;
    ++changed.deviceNonce[0];
    changed.begin();
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        changed.responder.open(
            changed.crypto,
            encoded,
            blinker::MutableByteSpan(
                plaintextStorage,
                sizeof(plaintextStorage)),
            plaintext).code());
}

void testHeaderValidation() {
    uint8_t prefix[blinker::security::kDirectSecureHeaderSize] = {};
    size_t size = 123U;
    CHECK_EQ(
        blinker::ErrorCode::InvalidMagic,
        blinker::security::decodeDirectSecureRecordSize(
            blinker::ByteView(prefix, sizeof(prefix)),
            size).code());
    CHECK_EQ(0U, size);
    prefix[0] = blinker::security::kDirectSecureMagic;
    prefix[1] = 0x20U;
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedVersion,
        blinker::security::decodeDirectSecureRecordSize(
            blinker::ByteView(prefix, sizeof(prefix)),
            size).code());
    prefix[1] = 0x11U;
    CHECK_EQ(
        blinker::ErrorCode::UnsupportedFeature,
        blinker::security::decodeDirectSecureRecordSize(
            blinker::ByteView(prefix, sizeof(prefix)),
            size).code());
}

void testBleTransportAtomicSwitchAndReplayRetirement() {
    Fixture fixture;
    TestClock clock;
    TestBleLink link;
    uint8_t rx[88U * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t tx[88U * BLINKER_BLE_MAX_TX_FRAMES] = {};
    uint8_t packet[20U] = {};
    uint8_t plaintext[64U] = {};
    blinker::BleFrameTransportConfig config;
    config.maxFrameSize = 64U;
    config.maxPacketsPerPoll = 8U;
    blinker::BleFrameTransport transport(
        link,
        clock,
        fixture.crypto,
        blinker::MutableByteSpan(rx, sizeof(rx)),
        blinker::MutableByteSpan(tx, sizeof(tx)),
        blinker::MutableByteSpan(packet, sizeof(packet)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    FrameCapture capture;
    transport.setReceiver(&captureFrame, &capture);
    transport.setSessionHandlers(
        nullptr,
        &captureDisconnect,
        &capture);
    CHECK_TRUE(transport.start());
    link.connect(9U);
    CHECK_TRUE(transport.prepare(
        9U,
        blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
        fixture.context));

    uint8_t frameStorage[20U];
    const blinker::ByteView frame = makeFrame(
        frameStorage,
        sizeof(frameStorage),
        1U);
    blinker::SendTarget target;
    target.kind = blinker::SendTargetKind::Session;
    target.sessionId = 9U;
    CHECK_TRUE(transport.send(frame, target));
    transport.poll(1000U);
    uint8_t recordStorage[88U];
    blinker::ByteView record = link.sentRecord(
        recordStorage,
        sizeof(recordStorage));
    CHECK_EQ(frame.size, record.size);
    CHECK_TRUE(std::memcmp(frame.data, record.data, frame.size) == 0);

    link.clearPackets();
    transport.commit(9U);
    CHECK_TRUE(transport.send(frame, target));
    transport.poll(1000U);
    record = link.sentRecord(recordStorage, sizeof(recordStorage));
    CHECK_EQ(
        blinker::security::directSecureRecordSize(frame.size),
        record.size);

    blinker::security::DirectSecureSession initiator;
    CHECK_TRUE(initiator.begin(
        blinker::security::DirectSecureRole::Initiator,
        blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
        fixture.context));
    uint8_t openedStorage[64U];
    blinker::ByteView opened;
    CHECK_TRUE(initiator.open(
        fixture.crypto,
        record,
        blinker::MutableByteSpan(openedStorage, sizeof(openedStorage)),
        opened));
    CHECK_EQ(frame.size, opened.size);
    CHECK_TRUE(std::memcmp(frame.data, opened.data, frame.size) == 0);

    blinker::ByteView appRecord;
    CHECK_TRUE(initiator.seal(
        fixture.crypto,
        frame,
        blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
        appRecord));
    link.injectRecord(appRecord, 7U);
    CHECK_EQ(frame.size, capture.size);
    CHECK_TRUE(std::memcmp(frame.data, capture.bytes, frame.size) == 0);
    CHECK_TRUE(capture.encrypted);

    // A new fragment record ID cannot make the same secure sequence valid.
    // Replay retires the admitted logical session instead of merely dropping
    // one packet and allowing a plaintext fallback.
    link.injectRecord(appRecord, 8U);
    CHECK_EQ(static_cast<uint32_t>(1U), capture.disconnects);
    CHECK_EQ(
        blinker::ErrorCode::NotConnected,
        transport.send(frame, target).code());
    transport.stop();

    TestBleLink rawLink;
    uint8_t rawRx[88U * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t rawTx[88U * BLINKER_BLE_MAX_TX_FRAMES] = {};
    uint8_t rawPlaintext[64U] = {};
    blinker::BleFrameTransport rawTransport(
        rawLink,
        clock,
        fixture.crypto,
        blinker::MutableByteSpan(rawRx, sizeof(rawRx)),
        blinker::MutableByteSpan(rawTx, sizeof(rawTx)),
        blinker::MutableByteSpan(packet, sizeof(packet)),
        blinker::MutableByteSpan(rawPlaintext, sizeof(rawPlaintext)),
        config);
    FrameCapture rawCapture;
    rawTransport.setSessionHandlers(
        nullptr,
        &captureDisconnect,
        &rawCapture);
    CHECK_TRUE(rawTransport.start());
    rawLink.connect(10U);
    CHECK_TRUE(rawTransport.prepare(
        10U,
        blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
        fixture.context));
    rawTransport.commit(10U);
    rawLink.injectRecord(frame, 1U);
    CHECK_EQ(static_cast<uint32_t>(1U), rawCapture.disconnects);
    rawTransport.stop();
}

void testBleTransportAuthenticationDeadline() {
    Fixture fixture;
    TestClock clock;
    TestBleLink link;
    uint8_t rx[88U * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t tx[88U * BLINKER_BLE_MAX_TX_FRAMES] = {};
    uint8_t packet[20U] = {};
    uint8_t plaintext[64U] = {};
    blinker::BleFrameTransportConfig config;
    config.maxFrameSize = 64U;
    config.authenticationTimeoutMillis = 100U;
    blinker::BleFrameTransport transport(
        link,
        clock,
        fixture.crypto,
        blinker::MutableByteSpan(rx, sizeof(rx)),
        blinker::MutableByteSpan(tx, sizeof(tx)),
        blinker::MutableByteSpan(packet, sizeof(packet)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    FrameCapture capture;
    transport.setSessionHandlers(nullptr, &captureDisconnect, &capture);
    CHECK_TRUE(transport.start());
    link.connect(41U);
    clock.advance(99U);
    transport.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(0U), link.disconnectRequests);
    clock.advance(1U);
    transport.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(1U), link.disconnectRequests);
    CHECK_EQ(static_cast<uint32_t>(1U), capture.disconnects);

    // A stream of unauthenticated centrals must not consume the only BLE
    // session slot permanently. Each deadline retires both the physical and
    // logical session before the next connection is admitted.
    for (uint32_t index = 0U; index < 128U; ++index) {
        link.connect(100U + index);
        clock.advance(config.authenticationTimeoutMillis);
        transport.poll(1000U);
        CHECK_EQ(2U + index, link.disconnectRequests);
        CHECK_EQ(2U + index, capture.disconnects);
    }
    transport.stop();

    TestClock activeClock;
    TestBleLink activeLink;
    blinker::BleFrameTransport activeTransport(
        activeLink,
        activeClock,
        fixture.crypto,
        blinker::MutableByteSpan(rx, sizeof(rx)),
        blinker::MutableByteSpan(tx, sizeof(tx)),
        blinker::MutableByteSpan(packet, sizeof(packet)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    CHECK_TRUE(activeTransport.start());
    activeLink.connect(42U);
    CHECK_TRUE(activeTransport.prepare(
        42U,
        blinker::ByteView(fixture.secret, sizeof(fixture.secret)),
        fixture.context));
    activeTransport.commit(42U);
    activeClock.advance(1000U);
    activeTransport.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(0U), activeLink.disconnectRequests);
    activeTransport.stop();

    // An unauthenticated BLE peer must never be allowed to reserve a slot for
    // an unbounded interval. The compact runtime stores this deadline in
    // milliseconds as uint16_t and rejects larger configurations fail-closed.
    blinker::BleFrameTransportConfig oversizedConfig = config;
    oversizedConfig.authenticationTimeoutMillis = 65536UL;
    TestClock oversizedClock;
    TestBleLink oversizedLink;
    blinker::BleFrameTransport oversizedTransport(
        oversizedLink,
        oversizedClock,
        fixture.crypto,
        blinker::MutableByteSpan(rx, sizeof(rx)),
        blinker::MutableByteSpan(tx, sizeof(tx)),
        blinker::MutableByteSpan(packet, sizeof(packet)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        oversizedConfig);
    CHECK_TRUE(!oversizedTransport.start());
}

} // namespace

int main() {
    testRoundTripAndDirections();
    testFrozenKeyMaterial();
    testCapacityDoesNotConsumeSequence();
    testReplayAndTamperFailClosed();
    testContextAndDirectionBinding();
    testHeaderValidation();
    testBleTransportAtomicSwitchAndReplayRetirement();
    testBleTransportAuthenticationDeadline();
    std::cout << "Direct secure session tests passed" << std::endl;
    return 0;
}
