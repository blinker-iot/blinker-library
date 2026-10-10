#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

#include "BlinkerV2/interface/IBleLink.h"
#include "BlinkerV2/interface/IClock.h"
#include "BlinkerV2/control/DeviceKeySessionContract.h"
#include "BlinkerV2/protocol/bbp2/Frame.h"
#include "BlinkerV2/protocol/ble/Fragment.h"
#include "BlinkerV2/protocol/ble/Mode.h"
#include "BlinkerV2/protocol/cbor/Cbor.h"
#include "BlinkerV2/protocol/gateway/Contracts.h"
#include "BlinkerV2/protocol/local/Access.h"
#include "BlinkerV2/protocol/local/Control.h"
#include "BlinkerV2/protocol/local/Progress.h"
#include "BlinkerV2/provisioning/BleEnrollmentContract.h"
#include "BlinkerV2/provisioning/WifiCredentialStore.h"
#include "BlinkerV2/transport/BleFrameTransport.h"

namespace {

const size_t kMaximumInputSize = 4096U;

void requireInvariant(bool condition) {
    if (!condition) {
        std::abort();
    }
}

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

void fuzzFrame(const uint8_t* data, size_t size) {
    const blinker::ByteView input(data, size);
    blinker::bbp2::FrameHeader decodedHeader;
    const blinker::Result decoded =
        blinker::bbp2::decodeHeader(input, decodedHeader);
    blinker::bbp2::FrameView frame;
    const blinker::Result parsed = blinker::bbp2::parseFrame(input, frame);
    if (!parsed) {
        return;
    }

    requireInvariant(decoded.ok());
    requireInvariant(frame.header.bodyLength == frame.body.size);
    requireInvariant(frame.header.headerLength <= size);
    requireInvariant(
        frame.body.data == data + frame.header.headerLength);
    requireInvariant(
        blinker::bbp2::encodedFrameSize(frame.header) == size);

    if (frame.header.headerLength == blinker::bbp2::kBaseHeaderSize) {
        uint8_t encoded[blinker::bbp2::kBaseHeaderSize] = {};
        const blinker::Result result = blinker::bbp2::encodeHeader(
            frame.header,
            blinker::MutableByteSpan(encoded, sizeof(encoded)));
        requireInvariant(result.ok());
        requireInvariant(std::memcmp(encoded, data, sizeof(encoded)) == 0);
    }
}

void exerciseTypedCborReader(
    blinker::ByteView input,
    const blinker::cbor::Limits& limits,
    blinker::cbor::Type type) {
    blinker::cbor::Reader reader(input, limits);
    switch (type) {
        case blinker::cbor::Type::Unsigned: {
            uint64_t value = 0;
            requireInvariant(reader.readUnsigned(value).ok());
            break;
        }
        case blinker::cbor::Type::Negative: {
            int64_t value = 0;
            (void)reader.readInt(value);
            break;
        }
        case blinker::cbor::Type::Bytes: {
            blinker::ByteView value;
            requireInvariant(reader.readBytes(value).ok());
            break;
        }
        case blinker::cbor::Type::Text: {
            blinker::StringView value;
            requireInvariant(reader.readText(value).ok());
            break;
        }
        case blinker::cbor::Type::Array: {
            size_t count = 0;
            requireInvariant(reader.readArraySize(count).ok());
            break;
        }
        case blinker::cbor::Type::Map: {
            size_t count = 0;
            requireInvariant(reader.readMapSize(count).ok());
            break;
        }
        case blinker::cbor::Type::Boolean: {
            bool value = false;
            requireInvariant(reader.readBool(value).ok());
            break;
        }
        case blinker::cbor::Type::Null:
            requireInvariant(reader.readNull().ok());
            break;
        case blinker::cbor::Type::Float: {
            double value = 0;
            requireInvariant(reader.readFloat(value).ok());
            break;
        }
        case blinker::cbor::Type::Invalid:
            requireInvariant(false);
            break;
    }
}

void fuzzCbor(const uint8_t* data, size_t size) {
    blinker::cbor::Limits limits;
    limits.maxTextLength = kMaximumInputSize;
    limits.maxByteStringLength = kMaximumInputSize;
    limits.maxContainerItems = 64U;
    limits.maxDepth = 8U;
    const blinker::ByteView input(data, size);
    const blinker::Result validated = blinker::cbor::validate(input, limits);

    blinker::cbor::Reader reader(input, limits);
    blinker::cbor::Type type = blinker::cbor::Type::Invalid;
    const blinker::Result peeked = reader.peekType(type);
    blinker::ByteView captured;
    const blinker::Result capturedResult = reader.captureValue(captured, &type);

    if (!validated) {
        return;
    }

    requireInvariant(peeked.ok());
    requireInvariant(capturedResult.ok());
    requireInvariant(reader.finished());
    requireInvariant(captured.data == data);
    requireInvariant(captured.size == size);
    exerciseTypedCborReader(input, limits, type);

    uint8_t copy[kMaximumInputSize] = {};
    blinker::cbor::Writer writer(
        blinker::MutableByteSpan(copy, sizeof(copy)));
    requireInvariant(writer.writeEncodedValue(input, limits).ok());
    requireInvariant(writer.size() == size);
    requireInvariant(std::memcmp(copy, data, size) == 0);
}

void fuzzDeviceKeySession(const uint8_t* data, size_t size) {
    const blinker::ByteView input(data, size);
    uint8_t encoded[1024U] = {};
    blinker::ByteView roundTrip;

    blinker::DeviceKeyChallengeRequest challengeRequest;
    if (blinker::decodeDeviceKeyChallengeRequest(input, challengeRequest)) {
        requireInvariant(blinker::encodeDeviceKeyChallengeRequest(
            challengeRequest,
            blinker::MutableByteSpan(encoded, sizeof(encoded)),
            roundTrip).ok());
        requireInvariant(roundTrip.size == size);
        requireInvariant(std::memcmp(roundTrip.data, data, size) == 0);
    }
    blinker::DeviceKeyChallengeResponse challengeResponse;
    if (blinker::decodeDeviceKeyChallengeResponse(input, challengeResponse)) {
        requireInvariant(blinker::encodeDeviceKeyChallengeResponse(
            challengeResponse,
            blinker::MutableByteSpan(encoded, sizeof(encoded)),
            roundTrip).ok());
        requireInvariant(roundTrip.size == size);
        requireInvariant(std::memcmp(roundTrip.data, data, size) == 0);
    }
    blinker::DeviceKeySessionRequest sessionRequest;
    if (blinker::decodeDeviceKeySessionRequest(input, sessionRequest)) {
        requireInvariant(blinker::encodeDeviceKeySessionRequest(
            sessionRequest,
            blinker::MutableByteSpan(encoded, sizeof(encoded)),
            roundTrip).ok());
        requireInvariant(roundTrip.size == size);
        requireInvariant(std::memcmp(roundTrip.data, data, size) == 0);
    }
    blinker::DeviceKeySessionResponse sessionResponse;
    if (blinker::decodeDeviceKeySessionResponse(input, sessionResponse)) {
        requireInvariant(blinker::encodeDeviceKeySessionResponse(
            sessionResponse,
            blinker::MutableByteSpan(encoded, sizeof(encoded)),
            roundTrip).ok());
        requireInvariant(roundTrip.size == size);
        requireInvariant(std::memcmp(roundTrip.data, data, size) == 0);
    }
    blinker::DeviceKeySessionErrorBody error;
    if (blinker::decodeDeviceKeySessionErrorBody(input, error)) {
        requireInvariant(blinker::encodeDeviceKeySessionErrorBody(
            error,
            blinker::MutableByteSpan(encoded, sizeof(encoded)),
            roundTrip).ok());
        requireInvariant(roundTrip.size == size);
        requireInvariant(std::memcmp(roundTrip.data, data, size) == 0);
    }
}

void fuzzGatewayExecution(const uint8_t* data, size_t size) {
    blinker::gateway::GatewayDirectYieldView direct;
    if (blinker::gateway::decodeGatewayDirectYield(blinker::ByteView(data, size), direct)) {
        uint8_t encoded[blinker::gateway::kGatewayExecutionMaximumEncodedSize]; size_t written = 0U;
        requireInvariant(blinker::gateway::encodeGatewayDirectYield(direct,
            blinker::MutableByteSpan(encoded, sizeof(encoded)), written).ok());
        requireInvariant(written == size && std::memcmp(encoded, data, size) == 0);
        return;
    }
    blinker::gateway::GatewayExecutionDemandView demand;
    if (!blinker::gateway::decodeGatewayExecutionDemand(blinker::ByteView(data, size), demand)) return;
    uint8_t encoded[blinker::gateway::kGatewayExecutionMaximumEncodedSize];
    size_t written = 0U;
    requireInvariant(blinker::gateway::encodeGatewayExecutionDemand(demand,
        blinker::MutableByteSpan(encoded, sizeof(encoded)), written).ok());
    requireInvariant(written == size && std::memcmp(encoded, data, size) == 0);
    // Encoding a valid zero-duration probe/challenge/release must also decode;
    // nonzero-only integer helpers are not valid for these two optional fields.
    for (uint8_t operation = 0U; operation <= 2U; ++operation) {
        demand.operation = static_cast<blinker::gateway::GatewayExecutionOperation>(operation);
        demand.incarnation = operation == 0U ? blinker::ByteView() : demand.childDeviceInstanceId;
        demand.challengeId = operation == 0U ? 0U : 1U;
        demand.leaseMillis = 0U;
        requireInvariant(blinker::gateway::encodeGatewayExecutionDemand(demand,
            blinker::MutableByteSpan(encoded, sizeof(encoded)), written).ok());
        blinker::gateway::GatewayExecutionDemandView decoded;
        requireInvariant(blinker::gateway::decodeGatewayExecutionDemand(blinker::ByteView(encoded, written), decoded).ok());
        requireInvariant(decoded.operation == demand.operation && decoded.leaseMillis == 0U);
    }
}

void fuzzGatewayAdmission(const uint8_t* data, size_t size) {
    blinker::gateway::GatewayRouteAdmissionView admission;
    if (!blinker::gateway::decodeGatewayRouteAdmission(blinker::ByteView(data, size), admission)) return;
    uint8_t encoded[blinker::gateway::kGatewayRouteAdmissionMaximumEncodedSize];
    size_t written = 0U;
    requireInvariant(blinker::gateway::encodeGatewayRouteAdmission(admission,
        blinker::MutableByteSpan(encoded, sizeof(encoded)), written).ok());
    requireInvariant(written == size && std::memcmp(encoded, data, size) == 0);
}

void fuzzLocalAccess(const uint8_t* data, size_t size) {
    const blinker::ByteView input(data, size);
    uint8_t encoded[blinker::local_access::kGrantMaximumEncodedSize] = {};
    size_t written = 0U;
    blinker::local_access::ChallengeView challenge;
    if (blinker::local_access::decodeChallenge(input, challenge)) {
        requireInvariant(blinker::local_access::encodeChallenge(
            challenge, blinker::MutableByteSpan(encoded, sizeof(encoded)),
            written).ok());
        requireInvariant(written == size &&
                         std::memcmp(encoded, data, size) == 0);
    }
    blinker::local_access::GrantView grant;
    if (blinker::local_access::decodeGrant(input, grant)) {
        requireInvariant(blinker::local_access::encodeGrant(
            grant, blinker::MutableByteSpan(encoded, sizeof(encoded)),
            written).ok());
        requireInvariant(written == size &&
                         std::memcmp(encoded, data, size) == 0);
    }
    blinker::local_access::ControlView control;
    if (blinker::local_access::decodeControl(input, control)) {
        requireInvariant(blinker::local_access::encodeControl(
            control, blinker::MutableByteSpan(encoded, sizeof(encoded)), written).ok());
        requireInvariant(written == size && std::memcmp(encoded, data, size) == 0);
    }
    blinker::local_access::ProgressView progress;
    if (blinker::local_access::decodeProgress(input, progress)) {
        requireInvariant(blinker::local_access::encodeProgress(
            progress, blinker::MutableByteSpan(encoded, sizeof(encoded)), written).ok());
        requireInvariant(written == size && std::memcmp(encoded, data, size) == 0);
    }
}

void fuzzGatewayCredentialRenewal(const uint8_t* data, size_t size) {
    const blinker::ByteView input(data, size);
    uint8_t encoded[
        blinker::gateway::
            kGatewayCredentialRenewalEnvelopeMaximumEncodedSize] = {};
    size_t written = 0U;

    blinker::gateway::GatewayCredentialRenewalEnvelopeView envelope;
    if (blinker::gateway::decodeGatewayCredentialRenewalEnvelope(
            input, envelope)) {
        requireInvariant(blinker::gateway::
            encodeGatewayCredentialRenewalEnvelope(
                envelope,
                blinker::MutableByteSpan(encoded, sizeof(encoded)),
                written).ok());
        requireInvariant(written == size &&
            std::memcmp(encoded, data, size) == 0);
    }
    blinker::gateway::GatewayCredentialRenewalDeliveryAckView ack;
    if (blinker::gateway::decodeGatewayCredentialRenewalDeliveryAck(
            input, ack)) {
        requireInvariant(blinker::gateway::
            encodeGatewayCredentialRenewalDeliveryAck(
                ack,
                blinker::MutableByteSpan(encoded, sizeof(encoded)),
                written).ok());
        requireInvariant(written == size &&
            std::memcmp(encoded, data, size) == 0);
    }
    blinker::gateway::GatewayCredentialRenewalCommandView command;
    if (blinker::gateway::decodeGatewayCredentialRenewalCommand(
            input, command)) {
        requireInvariant(blinker::gateway::
            encodeGatewayCredentialRenewalCommand(
                command,
                blinker::MutableByteSpan(encoded, sizeof(encoded)),
                written).ok());
        requireInvariant(written == size &&
            std::memcmp(encoded, data, size) == 0);
    }
    blinker::gateway::GatewayCredentialRenewalResultView result;
    if (blinker::gateway::decodeGatewayCredentialRenewalResult(
            input, result)) {
        requireInvariant(blinker::gateway::
            encodeGatewayCredentialRenewalResult(
                result,
                blinker::MutableByteSpan(encoded, sizeof(encoded)),
                written).ok());
        requireInvariant(written == size &&
            std::memcmp(encoded, data, size) == 0);
    }
}

void fuzzBleEnrollmentGrant(const uint8_t* data, size_t size) {
    const blinker::ByteView input(data, size);
    blinker::BleEnrollmentGrant grant;
    if (!blinker::decodeBleEnrollmentGrant(input, grant)) return;

    uint8_t encoded[blinker::kBleEnrollmentGrantMaxEncodedSize] = {};
    blinker::ByteView roundTrip;
    requireInvariant(blinker::encodeBleEnrollmentGrant(
        grant,
        blinker::MutableByteSpan(encoded, sizeof(encoded)),
        roundTrip).ok());
    requireInvariant(roundTrip.size == size);
    requireInvariant(std::memcmp(roundTrip.data, data, size) == 0);
}

void fuzzBleEnrollmentRequest(const uint8_t* data, size_t size) {
    blinker::BleEnrollmentRequest request;
    if (!blinker::decodeBleEnrollmentRequest(
            blinker::ByteView(data, size), request)) {
        return;
    }
    requireInvariant(request.requestId != 0U);
    if (request.type ==
        blinker::BleEnrollmentMessageType::HelloRequest) {
        requireInvariant(request.grant.empty());
        requireInvariant(request.controllerSecret.empty());
    } else {
        requireInvariant(
            request.type == blinker::BleEnrollmentMessageType::EnrollRequest);
        requireInvariant(
            request.grant.size <= blinker::kBleEnrollmentGrantMaxEncodedSize);
        requireInvariant(
            request.controllerSecret.size ==
            blinker::kControllerCredentialSecretSize);
    }
}

void fuzzWifiCredentialProfile(const uint8_t* data, size_t size) {
    blinker::WifiCredentialProfile profile;
    if (!blinker::WifiCredentialStore::decodeProfile(
            blinker::ByteView(data, size),
            profile)) {
        return;
    }

    requireInvariant(
        size == blinker::WifiCredentialStore::serializedProfileSize);
    requireInvariant(
        blinker::validateWifiNetworkConfig(profile.networkConfig()).ok());
    uint8_t encoded[
        blinker::WifiCredentialStore::serializedProfileSize] = {};
    requireInvariant(blinker::WifiCredentialStore::encodeProfile(
        profile,
        blinker::MutableByteSpan(encoded, sizeof(encoded))).ok());
    requireInvariant(std::memcmp(encoded, data, sizeof(encoded)) == 0);
    blinker::clearWifiCredentialProfile(profile);
}

void fuzzBleMode(const uint8_t* data, size_t size) {
    const blinker::ByteView input(data, size);
    blinker::ble::ModeProfile profile;
    if (!blinker::ble::decodeModeServiceData(input, profile)) return;
    uint8_t encodedBytes[blinker::ble::kModeServiceDataSize] = {};
    blinker::ByteView encoded;
    requireInvariant(blinker::ble::encodeModeServiceData(
        profile,
        blinker::MutableByteSpan(encodedBytes, sizeof(encodedBytes)),
        encoded).ok());
    requireInvariant(encoded.size == size);
    requireInvariant(std::memcmp(encoded.data, data, size) == 0);
}

class FuzzClock : public blinker::IClock {
public:
    FuzzClock() : now_(0) {}

    uint32_t monotonicMillis() const override { return now_; }
    bool unixTime(uint64_t&) const override { return false; }
    void advance(uint32_t millis) { now_ += millis; }

private:
    uint32_t now_;
};

class FuzzBleLink : public blinker::IBleLink {
public:
    FuzzBleLink()
        : state_(blinker::BleLinkState::Stopped),
          packetReceiver_(nullptr),
          packetContext_(nullptr),
          connectedHandler_(nullptr),
          disconnectedHandler_(nullptr),
          sessionContext_(nullptr),
          session_() {}

    blinker::Result start() override {
        state_ = blinker::BleLinkState::Ready;
        return blinker::Result::success();
    }

    void stop() override { state_ = blinker::BleLinkState::Stopped; }
    void poll(uint32_t) override {}
    blinker::BleLinkState state() const override { return state_; }
    blinker::ErrorCode lastError() const override {
        return blinker::ErrorCode::Ok;
    }

    size_t sessionCount() const override {
        return session_.connected ? 1U : 0U;
    }

    blinker::Result sessionAt(
        size_t index,
        blinker::BleSessionInfo& session) const override {
        if (index != 0U || !session_.connected) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        session = session_;
        return blinker::Result::success();
    }

    blinker::Result sendPacket(
        uint32_t sessionId,
        blinker::ByteView packet) override {
        if (!session_.connected || sessionId != session_.sessionId) {
            return blinker::Result::failure(blinker::ErrorCode::NotConnected);
        }
        if (packet.data == nullptr || packet.size > session_.maxPacketSize) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        if (packetReceiver_ != nullptr) {
            packetReceiver_(packetContext_, session_, packet);
        }
        return blinker::Result::success();
    }

    void setPacketReceiver(
        blinker::BlePacketReceiver receiver,
        void* context) override {
        packetReceiver_ = receiver;
        packetContext_ = context;
    }

    void setSessionHandlers(
        blinker::BleSessionHandler connected,
        blinker::BleSessionHandler disconnected,
        void* context) override {
        connectedHandler_ = connected;
        disconnectedHandler_ = disconnected;
        sessionContext_ = context;
    }

    void connect(uint16_t maxPacketSize) {
        session_.sessionId = 1U;
        session_.maxPacketSize = maxPacketSize;
        session_.connected = true;
        session_.notifyEnabled = true;
        session_.encrypted = true;
        session_.bonded = true;
        session_.authenticated = true;
        if (connectedHandler_ != nullptr) {
            connectedHandler_(sessionContext_, session_);
        }
    }

    void disconnect() {
        if (!session_.connected) {
            return;
        }
        session_.connected = false;
        if (disconnectedHandler_ != nullptr) {
            disconnectedHandler_(sessionContext_, session_);
        }
    }

    void changePacketSize(uint16_t maxPacketSize) {
        session_.maxPacketSize = maxPacketSize;
    }

    void inject(blinker::ByteView packet) {
        if (packetReceiver_ != nullptr) {
            packetReceiver_(packetContext_, session_, packet);
        }
    }

private:
    blinker::BleLinkState state_;
    blinker::BlePacketReceiver packetReceiver_;
    void* packetContext_;
    blinker::BleSessionHandler connectedHandler_;
    blinker::BleSessionHandler disconnectedHandler_;
    void* sessionContext_;
    blinker::BleSessionInfo session_;
};

struct ReceivedFrames {
    uint32_t count;

    ReceivedFrames() : count(0) {}
};

void receiveFrame(
    void* context,
    blinker::ByteView frame,
    const blinker::RxContext& rx) {
    ReceivedFrames* received = static_cast<ReceivedFrames*>(context);
    requireInvariant(received != nullptr);
    requireInvariant(rx.sessionId == 1U);
    requireInvariant(rx.encrypted);
    blinker::bbp2::FrameView parsed;
    requireInvariant(blinker::bbp2::parseFrame(frame, parsed).ok());
    ++received->count;
}

void fuzzBle(const uint8_t* data, size_t size) {
    const blinker::ByteView input(data, size);
    blinker::ble::FragmentView fragment;
    const blinker::Result parsedFragment =
        blinker::ble::parseFragment(input, fragment);
    if (parsedFragment) {
        uint8_t encoded[blinker::ble::kFragmentHeaderSize] = {};
        requireInvariant(
            blinker::ble::encodeFragmentHeader(
                fragment.header,
                blinker::MutableByteSpan(encoded, sizeof(encoded)))
                .ok());
        requireInvariant(std::memcmp(encoded, data, sizeof(encoded)) == 0);
        requireInvariant(
            fragment.payload.data == data + blinker::ble::kFragmentHeaderSize);
    }

    FuzzClock clock;
    UnsupportedAead crypto;
    FuzzBleLink link;
    const uint16_t initialPacketSize = static_cast<uint16_t>(
        20U + (size == 0U ? 0U : data[0] % 225U));
    const uint16_t maxFrameSize = 256U;
    const size_t maxRecordSize = maxFrameSize +
        blinker::security::kDirectSecureOverhead;
    uint8_t rxStorage[maxRecordSize * BLINKER_BLE_MAX_SESSIONS] = {};
    uint8_t txStorage[maxRecordSize * BLINKER_BLE_MAX_TX_FRAMES] = {};
    uint8_t packetScratch[244] = {};
    uint8_t plaintext[maxFrameSize] = {};
    blinker::BleFrameTransportConfig config;
    config.maxFrameSize = maxFrameSize;
    config.reassemblyTimeoutMillis = 500U;
    config.maxPacketsPerPoll = 8U;
    blinker::BleFrameTransport transport(
        link,
        clock,
        crypto,
        blinker::MutableByteSpan(rxStorage, sizeof(rxStorage)),
        blinker::MutableByteSpan(txStorage, sizeof(txStorage)),
        blinker::MutableByteSpan(packetScratch, sizeof(packetScratch)),
        blinker::MutableByteSpan(plaintext, sizeof(plaintext)),
        config);
    ReceivedFrames received;
    transport.setReceiver(&receiveFrame, &received);
    requireInvariant(transport.start().ok());
    link.connect(initialPacketSize);
    link.inject(input);

    blinker::bbp2::FrameView parsedFrame;
    if (size <= maxFrameSize &&
        blinker::bbp2::parseFrame(input, parsedFrame)) {
        blinker::SendTarget target;
        target.kind = blinker::SendTargetKind::Session;
        target.sessionId = 1U;
        const blinker::Result sent = transport.send(input, target);
        if (sent) {
            for (size_t index = 0; index < 32U; ++index) {
                transport.poll(1000U);
            }
            requireInvariant(transport.queuedFrameCount() == 0U);
        }
    }

    size_t cursor = 0;
    while (cursor < size) {
        const uint8_t operation = data[cursor++];
        switch (operation >> 6U) {
            case 0: {
                const size_t requested =
                    static_cast<size_t>(operation & 0x3FU) + 1U;
                const size_t available = size - cursor;
                const size_t packetSize =
                    requested < available ? requested : available;
                link.inject(blinker::ByteView(data + cursor, packetSize));
                cursor += packetSize;
                break;
            }
            case 1:
                clock.advance(
                    static_cast<uint32_t>(operation & 0x3FU) * 100U);
                transport.poll(1000U);
                break;
            case 2:
                link.changePacketSize(static_cast<uint16_t>(
                    20U + static_cast<uint16_t>(operation & 0x3FU) * 3U));
                transport.poll(1000U);
                break;
            case 3:
                link.disconnect();
                link.connect(static_cast<uint16_t>(
                    20U + static_cast<uint16_t>(operation & 0x3FU) * 3U));
                break;
            default:
                requireInvariant(false);
                break;
        }
    }

    clock.advance(1000U);
    transport.poll(1000U);
    requireInvariant(transport.completedFrameCount() >= received.count);
    link.disconnect();
    transport.stop();
}

int fuzzOneInput(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0U) {
        return 0;
    }
    if (size > kMaximumInputSize + 1U) {
        size = kMaximumInputSize + 1U;
    }
    const uint8_t selector = data[0];
    ++data;
    --size;
    switch (selector % 12U) {
        case 11:
            fuzzLocalAccess(data, size);
            break;
        case 10:
            fuzzGatewayExecution(data, size);
            break;
        case 9:
            fuzzGatewayAdmission(data, size);
            break;
        case 0:
            fuzzFrame(data, size);
            break;
        case 1:
            fuzzCbor(data, size);
            break;
        case 2:
            fuzzBle(data, size);
            break;
        case 3:
            fuzzBleMode(data, size);
            break;
        case 4:
            fuzzWifiCredentialProfile(data, size);
            break;
        case 5:
            fuzzBleEnrollmentGrant(data, size);
            break;
        case 6:
            fuzzBleEnrollmentRequest(data, size);
            break;
        case 7:
            fuzzDeviceKeySession(data, size);
            break;
        case 8:
            fuzzGatewayCredentialRenewal(data, size);
            break;
        default:
            requireInvariant(false);
            break;
    }
    return 0;
}

#ifndef BLINKER_USE_LIBFUZZER

uint32_t nextRandom(uint32_t& state) {
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state;
}

void mutate(std::vector<uint8_t>& input, uint32_t& state) {
    const size_t mutationCount = 1U + nextRandom(state) % 12U;
    for (size_t mutation = 0; mutation < mutationCount; ++mutation) {
        const uint32_t choice = nextRandom(state) % 4U;
        if (choice == 0U && input.size() < kMaximumInputSize + 1U) {
            const size_t offset = nextRandom(state) % (input.size() + 1U);
            input.insert(
                input.begin() + static_cast<std::ptrdiff_t>(offset),
                static_cast<uint8_t>(nextRandom(state)));
        } else if (choice == 1U && input.size() > 1U) {
            const size_t offset = nextRandom(state) % input.size();
            input.erase(input.begin() + static_cast<std::ptrdiff_t>(offset));
        } else if (!input.empty()) {
            const size_t offset = nextRandom(state) % input.size();
            if (choice == 2U) {
                input[offset] ^= static_cast<uint8_t>(
                    1U << (nextRandom(state) % 8U));
            } else {
                input[offset] = static_cast<uint8_t>(nextRandom(state));
            }
        }
    }
    if (input.empty()) {
        input.push_back(static_cast<uint8_t>(nextRandom(state) % 3U));
    }
}

uint64_t parseIterations(const char* text) {
    if (text == nullptr || text[0] == '\0' || text[0] == '-') {
        return 0U;
    }
    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed == 0U) {
        return 0U;
    }
    return static_cast<uint64_t>(parsed);
}

#endif

} // namespace

#ifdef BLINKER_USE_LIBFUZZER

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    return fuzzOneInput(data, size);
}

#else

int main(int argc, char** argv) {
    uint64_t iterations = 100000U;
    if (argc == 3 && std::strcmp(argv[1], "--iterations") == 0) {
        iterations = parseIterations(argv[2]);
        if (iterations == 0U) {
            std::cerr << "invalid fuzz iteration count" << std::endl;
            return EXIT_FAILURE;
        }
    } else if (argc != 1) {
        std::cerr << "usage: blinker_device_v2_protocol_fuzz "
                     "[--iterations COUNT]"
                  << std::endl;
        return EXIT_FAILURE;
    }

    static const uint8_t frameSeed[] = {
        0x00, 0x42, 0x4B, 0x02, 0x11, 0x00,
        0x0A, 0x00, 0x01, 0x00, 0x01, 0xA0
    };
    static const uint8_t cborSeed[] = {
        0x01, 0xA2, 0x01, 0x65, 'h', 'e', 'l', 'l', 'o',
        0x02, 0x82, 0xF5, 0xF6
    };
    static const uint8_t bleSeed[] = {
        0x02, 0xB2, 0x13, 0x01, 0x00,
        0x42, 0x4B, 0x02, 0x11, 0x00,
        0x0A, 0x00, 0x01, 0x00, 0x01, 0xA0
    };
    static const uint8_t modeSeed[] = {
        0x03, 0x01, 0x01, 0x01, 0x0F, 0x00,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    };
    uint8_t wifiProfileSeed[
        1U + blinker::WifiCredentialStore::serializedProfileSize] = {};
    wifiProfileSeed[0] = 0x04U;
    blinker::WifiCredentialProfile wifiProfile;
    wifiProfile.generation = 1U;
    wifiProfile.authentication = blinker::WifiAuthentication::Wpa2Personal;
    wifiProfile.ssidLength = 4U;
    wifiProfile.credentialLength = 8U;
    const uint8_t wifiSsid[] = {'t', 'e', 's', 't'};
    const uint8_t wifiPassword[] = {
        'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    std::memcpy(wifiProfile.ssid, wifiSsid, sizeof(wifiSsid));
    std::memcpy(
        wifiProfile.credential,
        wifiPassword,
        sizeof(wifiPassword));
    requireInvariant(blinker::WifiCredentialStore::encodeProfile(
        wifiProfile,
        blinker::MutableByteSpan(
            wifiProfileSeed + 1U,
            sizeof(wifiProfileSeed) - 1U)).ok());
    blinker::clearWifiCredentialProfile(wifiProfile);
    uint8_t enrollmentGrantSeed[
        1U + blinker::kBleEnrollmentGrantMaxEncodedSize] = {};
    enrollmentGrantSeed[0] = 0x05U;
    uint8_t grantId[blinker::kBleEnrollmentGrantIdSize] = {};
    uint8_t deviceId[blinker::kDeviceInstanceIdSize] = {};
    uint8_t setupSessionId[blinker::kSetupSessionIdSize] = {};
    uint8_t setupTranscriptHash[blinker::kSha256Size] = {};
    uint8_t controllerId[blinker::kControllerIdSize] = {};
    uint8_t controllerDigest[blinker::kSha256Size] = {};
    uint8_t presenceDigest[blinker::kSha256Size] = {};
    uint8_t grantNonce[blinker::kBleEnrollmentGrantNonceSize] = {};
    uint8_t grantSignature[blinker::kBleEnrollmentGrantSignatureSize] = {};
    for (size_t index = 0U; index < sizeof(grantId); ++index) {
        grantId[index] = static_cast<uint8_t>(0x10U + index);
        deviceId[index] = static_cast<uint8_t>(0x20U + index);
        setupSessionId[index] = static_cast<uint8_t>(0x30U + index);
        controllerId[index] = static_cast<uint8_t>(0x40U + index);
        grantNonce[index] = static_cast<uint8_t>(0x50U + index);
    }
    for (size_t index = 0U; index < sizeof(setupTranscriptHash); ++index) {
        setupTranscriptHash[index] = static_cast<uint8_t>(0x60U + index);
        controllerDigest[index] = static_cast<uint8_t>(0x80U + index);
        presenceDigest[index] = static_cast<uint8_t>(0xA0U + index);
    }
    std::memset(grantSignature, 0xA5, sizeof(grantSignature));
    blinker::BleEnrollmentGrant grant;
    grant.grantId = blinker::ByteView(grantId, sizeof(grantId));
    grant.deviceInstanceId = blinker::ByteView(deviceId, sizeof(deviceId));
    grant.setupSessionId =
        blinker::ByteView(setupSessionId, sizeof(setupSessionId));
    grant.setupTranscriptHash = blinker::ByteView(
        setupTranscriptHash,
        sizeof(setupTranscriptHash));
    grant.accessEpoch = 1U;
    grant.controllerId =
        blinker::ByteView(controllerId, sizeof(controllerId));
    grant.controllerSecretDigest =
        blinker::ByteView(controllerDigest, sizeof(controllerDigest));
    grant.controllerPermissions = blinker::kControllerPermissionAll;
    grant.presenceKeyVersion = 1U;
    grant.presenceKeyDigest =
        blinker::ByteView(presenceDigest, sizeof(presenceDigest));
    grant.nonce = blinker::ByteView(grantNonce, sizeof(grantNonce));
    grant.issuedAt = 1000U;
    grant.expiresAt = 1600U;
    grant.securityProfile =
        blinker::BleEnrollmentSecurityProfile::EducationNearby;
    grant.serverKeyId = 1U;
    grant.signatureAlgorithm =
        blinker::ServerSignatureAlgorithm::Ed25519;
    grant.signature =
        blinker::ByteView(grantSignature, sizeof(grantSignature));
    blinker::ByteView grantEncoded;
    requireInvariant(blinker::encodeBleEnrollmentGrant(
        grant,
        blinker::MutableByteSpan(
            enrollmentGrantSeed + 1U,
            sizeof(enrollmentGrantSeed) - 1U),
        grantEncoded).ok());
    uint8_t enrollmentRequestSeed[32U] = {};
    enrollmentRequestSeed[0] = 0x06U;
    blinker::ByteView enrollmentRequestEncoded;
    requireInvariant(blinker::encodeBleEnrollmentHelloRequest(
        1U,
        blinker::MutableByteSpan(
            enrollmentRequestSeed + 1U,
            sizeof(enrollmentRequestSeed) - 1U),
        enrollmentRequestEncoded).ok());
    uint8_t deviceKeySessionSeed[32U] = {};
    deviceKeySessionSeed[0] = 0x07U;
    blinker::DeviceKeySessionErrorBody deviceKeyError;
    deviceKeyError.error = blinker::DeviceKeySessionWireError::RateLimited;
    deviceKeyError.retryClass = blinker::DeviceKeySessionRetryClass::Backoff;
    deviceKeyError.retryAfterSeconds = 1U;
    deviceKeyError.hasRetryAfter = true;
    blinker::ByteView deviceKeyErrorEncoded;
    requireInvariant(blinker::encodeDeviceKeySessionErrorBody(
        deviceKeyError,
        blinker::MutableByteSpan(
            deviceKeySessionSeed + 1U,
            sizeof(deviceKeySessionSeed) - 1U),
        deviceKeyErrorEncoded).ok());
    uint8_t localAccessSeed[
        1U + blinker::local_access::kGrantMaximumEncodedSize] = {};
    localAccessSeed[0] = 0x0bU;
    blinker::local_access::GrantView localGrant;
    localGrant.grantId = blinker::ByteView(grantId, sizeof(grantId));
    localGrant.authorizationId = blinker::ByteView(deviceId, sizeof(deviceId));
    localGrant.recipientDeviceInstanceId =
        blinker::ByteView(deviceId, sizeof(deviceId));
    localGrant.deviceKeyVersion = 1U;
    localGrant.receiverSessionId =
        blinker::ByteView(setupSessionId, sizeof(setupSessionId));
    localGrant.challenge = blinker::ByteView(grantNonce, sizeof(grantNonce));
    localGrant.callerSessionId =
        blinker::ByteView(controllerId, sizeof(controllerId));
    localGrant.authorityRevision = 1U;
    localGrant.targetDeviceInstanceId =
        blinker::ByteView(deviceId, sizeof(deviceId));
    localGrant.permissions = blinker::kAuthorizationPermissionObserve;
    localGrant.lifetimeMillis = 1000U;
    localGrant.authenticator =
        blinker::ByteView(controllerDigest, sizeof(controllerDigest));
    size_t localAccessSize = 0U;
    requireInvariant(blinker::local_access::encodeGrant(
        localGrant,
        blinker::MutableByteSpan(
            localAccessSeed + 1U, sizeof(localAccessSeed) - 1U),
        localAccessSize).ok());
    uint8_t localControlSeed[1U + blinker::local_access::kControlMaximumEncodedSize] = {};
    localControlSeed[0] = 0x0bU;
    blinker::local_access::ControlView localControl;
    localControl.kind = blinker::local_access::ControlKind::Command;
    localControl.recipientDeviceInstanceId = localGrant.recipientDeviceInstanceId;
    localControl.deviceKeyVersion = 1U;
    localControl.controlSessionId = localGrant.receiverSessionId;
    localControl.sequence = 1U;
    localControl.authorityRevision = 1U;
    localControl.operation = blinker::local_access::ControlOperation::Retire;
    localControl.authorizationId = localGrant.authorizationId;
    localControl.authenticator = localGrant.authenticator;
    size_t localControlSize = 0U;
    requireInvariant(blinker::local_access::encodeControl(localControl,
        blinker::MutableByteSpan(localControlSeed + 1U, sizeof(localControlSeed) - 1U), localControlSize).ok());
    uint8_t progressSeed[1U + blinker::local_access::kControlMaximumEncodedSize] = {};
    progressSeed[0] = 0x0bU;
    blinker::local_access::ProgressView progress;
    progress.kind = blinker::local_access::ProgressKind::Status;
    progress.recipientDeviceInstanceId = localGrant.recipientDeviceInstanceId;
    progress.deviceKeyVersion = 1U; progress.controlSessionId = localGrant.receiverSessionId;
    progress.requestNonce = localGrant.authorizationId; progress.sequence = progress.authorityRevision = 1U;
    progress.state = blinker::local_access::ProgressState::Refresh; progress.authenticator = localGrant.authenticator;
    size_t progressSize = 0U;
    requireInvariant(blinker::local_access::encodeProgress(progress,
        blinker::MutableByteSpan(progressSeed + 1U, sizeof(progressSeed) - 1U), progressSize).ok());
    uint8_t gatewayRenewalSeed[
        1U + blinker::gateway::
            kGatewayCredentialRenewalDeliveryAckMaximumEncodedSize] = {};
    gatewayRenewalSeed[0] = 0x08U;
    uint8_t renewalOperationId[blinker::gateway::kOperationIdSize] = {};
    uint8_t topologyOperationId[blinker::gateway::kOperationIdSize] = {};
    uint8_t accessMaterialDigest[
        blinker::gateway::kAccessMaterialDigestSize] = {};
    for (size_t index = 0U; index < sizeof(renewalOperationId); ++index) {
        renewalOperationId[index] = static_cast<uint8_t>(0x70U + index);
        topologyOperationId[index] = static_cast<uint8_t>(0x10U + index);
    }
    for (size_t index = 0U; index < sizeof(accessMaterialDigest); ++index) {
        accessMaterialDigest[index] = static_cast<uint8_t>(0x60U + index);
    }
    blinker::gateway::GatewayCredentialRenewalDeliveryAckView renewalAck;
    renewalAck.renewalOperationId = blinker::ByteView(
        renewalOperationId, sizeof(renewalOperationId));
    renewalAck.topologyOperationId = blinker::ByteView(
        topologyOperationId, sizeof(topologyOperationId));
    renewalAck.accessMaterialDigest = blinker::ByteView(
        accessMaterialDigest, sizeof(accessMaterialDigest));
    renewalAck.pendingStorageRevision = 1U;
    size_t gatewayRenewalSize = 0U;
    requireInvariant(blinker::gateway::
        encodeGatewayCredentialRenewalDeliveryAck(
            renewalAck,
            blinker::MutableByteSpan(
                gatewayRenewalSeed + 1U,
                sizeof(gatewayRenewalSeed) - 1U),
            gatewayRenewalSize).ok());
    uint8_t admissionSeed[1U + blinker::gateway::kGatewayRouteAdmissionMaximumEncodedSize] = {9U};
    blinker::gateway::GatewayRouteAdmissionView admission;
    admission.operationId = blinker::ByteView(topologyOperationId, sizeof(topologyOperationId));
    admission.childDeviceInstanceId = blinker::ByteView(renewalOperationId, sizeof(renewalOperationId));
    admission.childLogicalDeviceId = blinker::StringView("ble_fuzz");
    admission.topologyVersion = 4U;
    admission.accessEpoch = 3U;
    admission.gatewayCredentialVersion = 2U;
    admission.expiresAtUnixSeconds = 1700003600ULL;
    admission.retiredGeneration = 0x8000000000000001ULL;
    size_t admissionSize = 0U;
    requireInvariant(blinker::gateway::encodeGatewayRouteAdmission(admission,
        blinker::MutableByteSpan(admissionSeed + 1U, sizeof(admissionSeed) - 1U), admissionSize).ok());
    static const uint8_t executionSeed[] = { 0x0aU, 0x8bU, 0x01U, 0x04U, 0x02U, 0x07U, 0x69U, 0x62U, 0x6cU, 0x65U, 0x5fU, 0x63U, 0x68U, 0x69U, 0x6cU, 0x64U, 0x50U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U, 0x09U, 0x0aU, 0x0bU, 0x0cU, 0x0dU, 0x0eU, 0x0fU, 0x10U, 0x03U, 0x04U, 0x50U, 0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2aU, 0x2bU, 0x2cU, 0x2dU, 0x2eU, 0x2fU, 0x01U, 0x19U, 0x75U, 0x30U };
    uint8_t directYieldSeed[sizeof(executionSeed) + 1U];
    memcpy(directYieldSeed, executionSeed, sizeof(executionSeed));
    directYieldSeed[1] = 0x8cU;
    directYieldSeed[3] = 0x05U;
    directYieldSeed[sizeof(executionSeed)] = 0x01U;
    const uint8_t* seeds[] = {
        frameSeed, cborSeed, bleSeed, modeSeed, wifiProfileSeed,
        enrollmentGrantSeed, enrollmentRequestSeed, deviceKeySessionSeed,
        gatewayRenewalSeed, admissionSeed, executionSeed, directYieldSeed,
        localAccessSeed, localControlSeed, progressSeed
    };
    const size_t seedSizes[] = {
        sizeof(frameSeed), sizeof(cborSeed), sizeof(bleSeed),
        sizeof(modeSeed), sizeof(wifiProfileSeed),
        grantEncoded.size + 1U,
        enrollmentRequestEncoded.size + 1U,
        deviceKeyErrorEncoded.size + 1U,
        gatewayRenewalSize + 1U, admissionSize + 1U, sizeof(executionSeed),
        sizeof(directYieldSeed), localAccessSize + 1U, localControlSize + 1U, progressSize + 1U
    };
    const size_t seedCount = sizeof(seeds) / sizeof(seeds[0]);
    requireInvariant(seedCount == sizeof(seedSizes) / sizeof(seedSizes[0]));
    uint32_t randomState = 0xB11E2F02U;

    for (uint64_t iteration = 0; iteration < iterations; ++iteration) {
        const size_t seedIndex = static_cast<size_t>(iteration % seedCount);
        std::vector<uint8_t> input(
            seeds[seedIndex], seeds[seedIndex] + seedSizes[seedIndex]);
        if (iteration >= seedCount) {
            mutate(input, randomState);
        }
        fuzzOneInput(input.data(), input.size());
    }

    std::cout << "Device v2 protocol fuzz smoke passed: "
              << iterations << " deterministic cases" << std::endl;
    return EXIT_SUCCESS;
}

#endif
