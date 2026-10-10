#include <BlinkerV2/protocol/bbp2/Frame.h>
#include <BlinkerV2/protocol/ble/Mode.h>
#include <BlinkerV2/transport/GattDirectChildLink.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

static_assert(
    sizeof(blinker::GattDirectChildLink) <= 640U,
    "single-child GATT link object exceeded its fixed-memory bound");

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "GATT child link check failed at line " << __LINE__  \
                      << ": " << #condition << std::endl;                     \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class FakeClock final : public blinker::IClock {
public:
    FakeClock() : now(0U) {}
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }
    void advance(uint32_t millis) { now += millis; }
    uint32_t now;
};

class FakeCentralPort final : public blinker::IBleCentralPort {
public:
    struct Packet {
        uint8_t bytes[64];
        size_t size;
        Packet() : bytes(), size(0U) {}
    };

    FakeCentralPort()
        : stateValue(blinker::BleCentralPortState::Stopped),
          error(blinker::ErrorCode::Ok), request(), connectionValue(),
          packetReceiver(nullptr), packetContext(nullptr),
          connectedHandler(nullptr), disconnectedHandler(nullptr),
          sessionContext(nullptr), advertisement(), advertisementSize(0U),
          advertisementPending(false), readyPending(false),
          disconnectPending(false), inbound(), inboundPending(false), sent(),
          sentCount(0U), matcherCalls(0U), pollCalls(0U), blockSends(false),
          holdDisconnect(false), rejectCancel(false), cancelCalls(0U) {}

    blinker::Result start() override {
        if (stateValue != blinker::BleCentralPortState::Stopped) {
            return blinker::Result::failure(
                blinker::ErrorCode::AlreadyExists);
        }
        stateValue = blinker::BleCentralPortState::Idle;
        error = blinker::ErrorCode::Ok;
        return blinker::Result::success();
    }

    void stop() override {
        stateValue = blinker::BleCentralPortState::Stopped;
        request = blinker::BleCentralConnectRequest();
        connectionValue = blinker::BleCentralConnectionInfo();
        advertisementPending = false;
        readyPending = false;
        disconnectPending = false;
        inboundPending = false;
    }

    void poll(uint32_t) override {
        ++pollCalls;
        if (advertisementPending &&
            stateValue == blinker::BleCentralPortState::Scanning) {
            advertisementPending = false;
            blinker::BleCentralAdvertisement candidate;
            candidate.modeServiceData = blinker::ByteView(
                advertisement, advertisementSize);
            candidate.rssi = -42;
            ++matcherCalls;
            if (request.matcher != nullptr &&
                request.matcher(request.matcherContext, candidate)) {
                stateValue = blinker::BleCentralPortState::Connecting;
            }
        }
        if (readyPending &&
            stateValue == blinker::BleCentralPortState::Connecting) {
            readyPending = false;
            stateValue = blinker::BleCentralPortState::Ready;
            connectionValue.attemptId = request.attemptId;
            connectionValue.maxPacketSize = 20U;
            connectionValue.connected = true;
            connectionValue.notifyEnabled = true;
            if (connectedHandler != nullptr) {
                connectedHandler(sessionContext, connectionValue);
            }
        }
        if (inboundPending &&
            stateValue == blinker::BleCentralPortState::Ready) {
            inboundPending = false;
            if (packetReceiver != nullptr) {
                packetReceiver(
                    packetContext,
                    connectionValue,
                    blinker::ByteView(inbound.bytes, inbound.size));
            }
        }
        if (disconnectPending && !holdDisconnect &&
            stateValue == blinker::BleCentralPortState::Disconnecting) {
            disconnectPending = false;
            const blinker::BleCentralConnectionInfo previous =
                connectionValue;
            const bool announced = connectionValue.connected;
            connectionValue = blinker::BleCentralConnectionInfo();
            request = blinker::BleCentralConnectRequest();
            stateValue = blinker::BleCentralPortState::Idle;
            if (announced && disconnectedHandler != nullptr) {
                disconnectedHandler(sessionContext, previous);
            }
        }
    }

    blinker::BleCentralPortState state() const override {
        return stateValue;
    }
    blinker::ErrorCode lastError() const override { return error; }

    blinker::Result connect(
        const blinker::BleCentralConnectRequest& value) override {
        if (stateValue != blinker::BleCentralPortState::Idle) {
            return blinker::Result::failure(
                blinker::ErrorCode::AlreadyExists);
        }
        if (value.attemptId == 0U || value.matcher == nullptr) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        request = value;
        stateValue = blinker::BleCentralPortState::Scanning;
        return blinker::Result::success();
    }

    blinker::Result cancel(uint32_t attemptId) override {
        ++cancelCalls;
        if (rejectCancel) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        if (request.attemptId == 0U || request.attemptId != attemptId ||
            stateValue == blinker::BleCentralPortState::Stopped ||
            stateValue == blinker::BleCentralPortState::Idle) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        stateValue = blinker::BleCentralPortState::Disconnecting;
        disconnectPending = true;
        return blinker::Result::success();
    }

    blinker::Result connection(
        blinker::BleCentralConnectionInfo& output) const override {
        if (stateValue != blinker::BleCentralPortState::Ready ||
            !connectionValue.connected) {
            return blinker::Result::failure(
                blinker::ErrorCode::NotConnected);
        }
        output = connectionValue;
        return blinker::Result::success();
    }

    blinker::Result sendPacket(
        uint32_t attemptId,
        blinker::ByteView packet) override {
        if (stateValue != blinker::BleCentralPortState::Ready ||
            attemptId != connectionValue.attemptId) {
            return blinker::Result::failure(
                blinker::ErrorCode::NotConnected);
        }
        if (blockSends) {
            return blinker::Result::failure(
                blinker::ErrorCode::WouldBlock);
        }
        if (sentCount >= sizeof(sent) / sizeof(sent[0]) ||
            packet.data == nullptr || packet.size > sizeof(sent[0].bytes)) {
            return blinker::Result::failure(
                blinker::ErrorCode::CapacityExceeded);
        }
        sent[sentCount].size = packet.size;
        std::memcpy(sent[sentCount].bytes, packet.data, packet.size);
        ++sentCount;
        return blinker::Result::success();
    }

    void setPacketReceiver(
        blinker::BleCentralPacketReceiver receiver,
        void* context) override {
        packetReceiver = receiver;
        packetContext = context;
    }

    void setConnectionHandlers(
        blinker::BleCentralConnectionHandler connected,
        blinker::BleCentralConnectionHandler disconnected,
        void* context) override {
        connectedHandler = connected;
        disconnectedHandler = disconnected;
        sessionContext = context;
    }

    void offer(blinker::ByteView modeServiceData) {
        CHECK_TRUE(modeServiceData.size <= sizeof(advertisement));
        std::memcpy(
            advertisement,
            modeServiceData.data,
            modeServiceData.size);
        advertisementSize = modeServiceData.size;
        advertisementPending = true;
    }

    void completeConnection() { readyPending = true; }

    void inject(const Packet& packet) {
        inbound = packet;
        inboundPending = true;
    }

    void fail(blinker::ErrorCode value) {
        error = value;
        stateValue = blinker::BleCentralPortState::Error;
    }

    void dropWithoutCallback() {
        connectionValue = blinker::BleCentralConnectionInfo();
        request = blinker::BleCentralConnectRequest();
        stateValue = blinker::BleCentralPortState::Idle;
    }

    blinker::BleCentralPortState stateValue;
    blinker::ErrorCode error;
    blinker::BleCentralConnectRequest request;
    blinker::BleCentralConnectionInfo connectionValue;
    blinker::BleCentralPacketReceiver packetReceiver;
    void* packetContext;
    blinker::BleCentralConnectionHandler connectedHandler;
    blinker::BleCentralConnectionHandler disconnectedHandler;
    void* sessionContext;
    uint8_t advertisement[blinker::ble::kModeServiceDataSize];
    size_t advertisementSize;
    bool advertisementPending;
    bool readyPending;
    bool disconnectPending;
    Packet inbound;
    bool inboundPending;
    Packet sent[16];
    size_t sentCount;
    uint32_t matcherCalls;
    uint32_t pollCalls;
    bool blockSends;
    bool holdDisconnect;
    bool rejectCancel;
    uint32_t cancelCalls;
};

struct Capture {
    uint32_t connected;
    uint32_t disconnected;
    uint32_t records;
    uint32_t faults;
    uint32_t lastAttempt;
    blinker::ErrorCode lastError;
    uint8_t record[128];
    size_t recordSize;

    Capture()
        : connected(0U), disconnected(0U), records(0U), faults(0U),
          lastAttempt(0U), lastError(blinker::ErrorCode::Ok), record(),
          recordSize(0U) {}
};

void onRecord(
    void* context,
    blinker::ByteView record,
    const blinker::RxContext&) {
    Capture* capture = static_cast<Capture*>(context);
    ++capture->records;
    CHECK_TRUE(record.size <= sizeof(capture->record));
    capture->recordSize = record.size;
    std::memcpy(capture->record, record.data, record.size);
}

void onConnected(void* context, const blinker::RxContext& rx) {
    Capture* capture = static_cast<Capture*>(context);
    ++capture->connected;
    capture->lastAttempt = rx.sessionId;
}

void onDisconnected(void* context, const blinker::RxContext& rx) {
    Capture* capture = static_cast<Capture*>(context);
    ++capture->disconnected;
    capture->lastAttempt = rx.sessionId;
}

void onFault(
    void* context,
    blinker::ErrorCode error,
    uint32_t attemptId) {
    Capture* capture = static_cast<Capture*>(context);
    ++capture->faults;
    capture->lastError = error;
    capture->lastAttempt = attemptId;
}

struct Matcher {
    uint8_t expectedWireVersion;
    uint32_t calls;
    Matcher() : expectedWireVersion(0U), calls(0U) {}
};

bool matchDirect(
    void* context,
    const blinker::BleCentralAdvertisement& advertisement) {
    Matcher* matcher = static_cast<Matcher*>(context);
    ++matcher->calls;
    blinker::ble::ModeProfile profile;
    return blinker::ble::decodeModeServiceData(
               advertisement.modeServiceData,
               profile) &&
           profile.mode == blinker::ble::ApplicationMode::Direct &&
           profile.wireVersion == matcher->expectedWireVersion;
}

void encodeFrame(uint8_t (&frame)[45]) {
    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(
        blinker::bbp2::MessageKind::Patch);
    header.sequence = 7U;
    header.bodyLength = static_cast<uint16_t>(
        sizeof(frame) - blinker::bbp2::kBaseHeaderSize);
    CHECK_TRUE(blinker::bbp2::encodeHeader(
        header,
        blinker::MutableByteSpan(frame, sizeof(frame))));
    for (size_t index = blinker::bbp2::kBaseHeaderSize;
         index < sizeof(frame);
         ++index) {
        frame[index] = static_cast<uint8_t>(index);
    }
}

blinker::ByteView directServiceData(
    uint8_t (&storage)[blinker::ble::kModeServiceDataSize]) {
    uint8_t locator[blinker::kSetupSessionLocatorSize] = {
        1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U
    };
    const blinker::ble::ModeProfile profile =
        blinker::ble::makeAuthorizedDirectModeProfile(
            blinker::ByteView(locator, sizeof(locator)));
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::ble::encodeModeServiceData(
        profile,
        blinker::MutableByteSpan(storage, sizeof(storage)),
        encoded));
    return encoded;
}

void establish(
    blinker::GattDirectChildLink& link,
    FakeCentralPort& port,
    Matcher& matcher,
    blinker::ByteView serviceData) {
    CHECK_TRUE(link.connect(&matchDirect, &matcher));
    CHECK_TRUE(link.connecting());
    CHECK_TRUE(!link.connected());
    port.offer(serviceData);
    link.poll(1000U);
    CHECK_TRUE(port.state() == blinker::BleCentralPortState::Connecting);
    CHECK_TRUE(!link.connected());
    port.completeConnection();
    link.poll(1000U);
    CHECK_TRUE(link.connected());
    CHECK_TRUE(!link.connecting());
}

void testRecordLink() {
    FakeClock clock;
    FakeCentralPort port;
    uint8_t rx[536] = {};
    uint8_t tx[536] = {};
    uint8_t packet[20] = {};
    blinker::GattDirectChildLinkConfig config;
    config.connectTimeoutMillis = 100U;
    config.reassemblyTimeoutMillis = 50U;
    config.maxPacketsPerPoll = 4U;
    blinker::GattDirectChildLink link(
        port,
        clock,
        blinker::MutableByteSpan(rx, sizeof(rx)),
        blinker::MutableByteSpan(tx, sizeof(tx)),
        blinker::MutableByteSpan(packet, sizeof(packet)),
        config);
    Capture capture;
    link.setReceiver(&onRecord, &capture);
    link.setSessionHandlers(&onConnected, &onDisconnected, &capture);
    link.setFaultHandler(&onFault, &capture);
    CHECK_TRUE(link.start());
    CHECK_TRUE(port.state() == blinker::BleCentralPortState::Idle);

    uint8_t serviceStorage[blinker::ble::kModeServiceDataSize] = {};
    const blinker::ByteView serviceData = directServiceData(serviceStorage);
    Matcher matcher;
    matcher.expectedWireVersion =
        blinker::ble::kAuthorizedDirectBbp2WireVersion;
    establish(link, port, matcher, serviceData);
    CHECK_EQ(static_cast<uint32_t>(1U), capture.connected);
    CHECK_EQ(static_cast<uint32_t>(1U), matcher.calls);

    uint8_t frame[45] = {};
    encodeFrame(frame);
    CHECK_TRUE(link.sendRecord(blinker::ByteView(frame, sizeof(frame))));
    CHECK_EQ(static_cast<size_t>(1U), link.queuedRecordCount());
    link.poll(1000U);
    CHECK_EQ(static_cast<size_t>(3U), port.sentCount);
    CHECK_EQ(static_cast<size_t>(0U), link.queuedRecordCount());
    for (size_t index = 0U; index < port.sentCount; ++index) {
        blinker::ble::FragmentView fragment;
        CHECK_TRUE(blinker::ble::parseFragment(
            blinker::ByteView(port.sent[index].bytes, port.sent[index].size),
            fragment));
        CHECK_EQ(static_cast<uint8_t>(index), fragment.header.index);
        port.inject(port.sent[index]);
        link.poll(1000U);
    }
    CHECK_EQ(static_cast<uint32_t>(1U), capture.records);
    CHECK_EQ(sizeof(frame), capture.recordSize);
    CHECK_TRUE(std::memcmp(frame, capture.record, sizeof(frame)) == 0);
    CHECK_EQ(static_cast<uint32_t>(1U), link.completedRecordCount());

    // Reassembly stall is a connection fault for an on-demand Gateway link,
    // not a reason to keep occupying the child and retry on the same session.
    port.sentCount = 0U;
    CHECK_TRUE(link.sendRecord(blinker::ByteView(frame, sizeof(frame))));
    link.poll(1000U);
    port.inject(port.sent[0]);
    link.poll(1000U);
    clock.advance(50U);
    link.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(1U), capture.faults);
    CHECK_TRUE(capture.lastError == blinker::ErrorCode::ProtocolError);
    CHECK_TRUE(!link.connected());
    link.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(1U), capture.disconnected);
    CHECK_TRUE(port.state() == blinker::BleCentralPortState::Idle);
    link.stop();
}

void testTimeoutAndRecovery() {
    FakeClock clock;
    FakeCentralPort port;
    uint8_t rx[536] = {};
    uint8_t tx[536] = {};
    uint8_t packet[20] = {};
    blinker::GattDirectChildLinkConfig config;
    config.connectTimeoutMillis = 25U;
    blinker::GattDirectChildLink link(
        port,
        clock,
        blinker::MutableByteSpan(rx, sizeof(rx)),
        blinker::MutableByteSpan(tx, sizeof(tx)),
        blinker::MutableByteSpan(packet, sizeof(packet)),
        config);
    Capture capture;
    link.setFaultHandler(&onFault, &capture);
    CHECK_TRUE(link.start());
    Matcher matcher;
    matcher.expectedWireVersion =
        blinker::ble::kAuthorizedDirectBbp2WireVersion;

    CHECK_TRUE(link.connect(&matchDirect, &matcher));
    const uint32_t expired = link.attemptId();
    clock.advance(25U);
    link.poll(1000U);
    CHECK_TRUE(!link.connecting());
    CHECK_EQ(static_cast<uint32_t>(1U), capture.faults);
    CHECK_EQ(expired, capture.lastAttempt);
    CHECK_TRUE(capture.lastError == blinker::ErrorCode::NotConnected);
    link.poll(1000U);
    CHECK_TRUE(port.state() == blinker::BleCentralPortState::Idle);

    uint8_t serviceStorage[blinker::ble::kModeServiceDataSize] = {};
    establish(
        link,
        port,
        matcher,
        directServiceData(serviceStorage));
    CHECK_TRUE(link.attemptId() != expired);
    CHECK_TRUE(link.disconnect());
    CHECK_TRUE(!link.connected());
    link.poll(1000U);
    CHECK_TRUE(port.state() == blinker::BleCentralPortState::Idle);

    // A Port failure is reported once, rather than once per poll.
    port.fail(blinker::ErrorCode::InternalError);
    link.poll(1000U);
    link.poll(1000U);
    CHECK_EQ(static_cast<uint32_t>(2U), capture.faults);
    CHECK_TRUE(capture.lastError == blinker::ErrorCode::InternalError);
    link.stop();
}

void testConnectionFactAndGenerationRecovery() {
    FakeClock clock;
    FakeCentralPort port;
    uint8_t rx[536] = {};
    uint8_t tx[536] = {};
    uint8_t packet[20] = {};
    blinker::GattDirectChildLink link(
        port,
        clock,
        blinker::MutableByteSpan(rx, sizeof(rx)),
        blinker::MutableByteSpan(tx, sizeof(tx)),
        blinker::MutableByteSpan(packet, sizeof(packet)));
    Capture capture;
    link.setSessionHandlers(&onConnected, &onDisconnected, &capture);
    link.setFaultHandler(&onFault, &capture);
    CHECK_TRUE(link.start());
    uint8_t serviceStorage[blinker::ble::kModeServiceDataSize] = {};
    const blinker::ByteView serviceData = directServiceData(serviceStorage);
    Matcher matcher;
    matcher.expectedWireVersion =
        blinker::ble::kAuthorizedDirectBbp2WireVersion;

    // A missed native disconnect callback is reconciled from current Port
    // facts and cannot leave a usable cached child session.
    establish(link, port, matcher, serviceData);
    port.dropWithoutCallback();
    link.poll(1000U);
    CHECK_TRUE(!link.connected());
    CHECK_EQ(static_cast<uint32_t>(1U), capture.disconnected);
    CHECK_EQ(static_cast<uint32_t>(1U), capture.faults);
    CHECK_TRUE(capture.lastError == blinker::ErrorCode::NotConnected);

    // Repeated on-demand sessions receive fresh non-zero generations and
    // release the one physical slot without carrying reassembly state.
    uint32_t previous = 0U;
    for (uint32_t cycle = 0U; cycle < 32U; ++cycle) {
        establish(link, port, matcher, serviceData);
        CHECK_TRUE(link.attemptId() != 0U);
        CHECK_TRUE(link.attemptId() != previous);
        previous = link.attemptId();
        CHECK_TRUE(link.disconnect());
        link.poll(1000U);
        CHECK_TRUE(port.state() == blinker::BleCentralPortState::Idle);
        CHECK_TRUE(!link.connected());
    }
    CHECK_EQ(static_cast<uint32_t>(33U), capture.connected);
    CHECK_EQ(static_cast<uint32_t>(33U), capture.disconnected);
    CHECK_EQ(static_cast<uint32_t>(1U), capture.faults);
    link.stop();
}

void testPhysicalReleaseAfterRecordRetirementAndCancelledWake() {
    FakeClock clock;
    FakeCentralPort port;
    uint8_t rx[536] = {};
    uint8_t tx[536] = {};
    uint8_t packet[20] = {};
    blinker::GattDirectChildLinkConfig config;
    config.connectTimeoutMillis = 25U;
    blinker::GattDirectChildLink link(
        port, clock, blinker::MutableByteSpan(rx, sizeof(rx)),
        blinker::MutableByteSpan(tx, sizeof(tx)),
        blinker::MutableByteSpan(packet, sizeof(packet)), config);
    Capture capture;
    link.setSessionHandlers(&onConnected, &onDisconnected, &capture);
    link.setFaultHandler(&onFault, &capture);
    Matcher matcher;
    matcher.expectedWireVersion = blinker::ble::kAuthorizedDirectBbp2WireVersion;
    uint8_t wire[blinker::ble::kModeServiceDataSize] = {};
    const blinker::ByteView serviceData = directServiceData(wire);
    CHECK_TRUE(link.start());

    // Record retirement disables I/O now, notifies resource release later.
    establish(link, port, matcher, serviceData);
    const blinker::BleCentralConnectionInfo retired = port.connectionValue;
    port.holdDisconnect = true;
    CHECK_TRUE(link.disconnect());
    CHECK_TRUE(!link.connected());
    CHECK_EQ(0U, capture.disconnected);
    for (unsigned i = 0U; i < 4U; ++i) {
        link.poll(1000U);
        CHECK_EQ(retired.attemptId, link.attemptId());
        CHECK_EQ(0U, capture.disconnected);
        CHECK_TRUE(!link.connect(&matchDirect, &matcher));
    }
    CHECK_EQ(1U, port.cancelCalls); // accepted cancellation is idempotent
    port.holdDisconnect = false;
    link.poll(1000U);
    CHECK_EQ(1U, capture.disconnected);
    CHECK_EQ(0U, link.attemptId());

    // Cancellation before notification admission must also signal completion.
    CHECK_TRUE(link.connect(&matchDirect, &matcher));
    const uint32_t scanning = link.attemptId();
    port.holdDisconnect = true;
    CHECK_TRUE(link.disconnect());
    link.poll(1000U);
    CHECK_EQ(scanning, link.attemptId());
    CHECK_EQ(1U, capture.disconnected);
    port.holdDisconnect = false;
    link.poll(1000U);
    CHECK_EQ(2U, capture.disconnected);
    CHECK_EQ(0U, link.attemptId());

    // Timeout with a rejected cancel retains capacity and retries only cancel.
    CHECK_TRUE(link.connect(&matchDirect, &matcher));
    port.offer(serviceData);
    link.poll(1000U);
    CHECK_TRUE(port.state() == blinker::BleCentralPortState::Connecting);
    const uint32_t timedOut = link.attemptId();
    port.rejectCancel = true;
    clock.advance(25U);
    for (unsigned i = 0U; i < 3U; ++i) link.poll(1000U);
    CHECK_EQ(timedOut, link.attemptId());
    CHECK_EQ(1U, capture.faults);
    CHECK_EQ(2U, capture.disconnected);
    CHECK_TRUE(!link.connect(&matchDirect, &matcher));
    port.rejectCancel = false;
    link.poll(1000U);
    link.poll(1000U);
    CHECK_EQ(3U, capture.disconnected);
    CHECK_EQ(0U, link.attemptId());

    // Old native completion cannot retire a newly established generation.
    establish(link, port, matcher, serviceData);
    const uint32_t fresh = link.attemptId();
    port.disconnectedHandler(port.sessionContext, retired);
    link.poll(1000U);
    CHECK_TRUE(link.connected());
    CHECK_EQ(fresh, link.attemptId());
    CHECK_EQ(3U, capture.disconnected);
    CHECK_TRUE(link.disconnect());
    port.dropWithoutCallback();
    link.poll(1000U);
    CHECK_EQ(4U, capture.disconnected);
    link.poll(1000U);
    CHECK_EQ(4U, capture.disconnected);
    link.stop();
}

} // namespace

int main() {
    testRecordLink();
    testTimeoutAndRecovery();
    testConnectionFactAndGenerationRecovery();
    testPhysicalReleaseAfterRecordRetirementAndCancelledWake();
    std::cout << "GATT direct child link tests passed" << std::endl;
    return 0;
}
