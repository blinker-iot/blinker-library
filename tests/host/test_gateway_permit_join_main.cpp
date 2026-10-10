#include <BlinkerV2/protocol/ble/Mode.h>
#include <BlinkerV2/protocol/gateway/Contracts.h>
#include <BlinkerV2/protocol/gateway/PermitJoinRelay.h>
#include <BlinkerV2/runtime/GatewayPermitJoinCoordinator.h>
#include <BlinkerV2/runtime/GatewayChildManagementRouter.h>
#include <BlinkerV2/runtime/GatewayGattPool.h>
#include <BlinkerV2/transport/GattDirectChildLink.h>
#include <BlinkerV2/transport/NativeGattPermitJoinAdapter.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "permit-join check failed at line " << __LINE__      \
                      << ": " << #condition << std::endl;                     \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

static_assert(
    sizeof(blinker::NativeGattPermitJoinAdapter) <= 1024U,
    "Native GATT permit-join adapter exceeded its fixed-memory bound");

class FakeClock final : public blinker::IClock {
public:
    FakeClock() : monotonic(0U), unixSeconds(1000U), synchronized(true) {}
    uint32_t monotonicMillis() const override { return monotonic; }
    bool unixTime(uint64_t& output) const override {
        if (!synchronized) return false;
        output = unixSeconds;
        return true;
    }
    uint32_t monotonic;
    uint64_t unixSeconds;
    bool synchronized;
};

class FakeAdapter final : public blinker::IGatewayPermitJoinAdapter {
public:
    FakeAdapter()
        : stateValue(blinker::GatewayPermitJoinAdapterState::Stopped),
          error(blinker::ErrorCode::Ok), openResult(blinker::ErrorCode::Ok),
          opens(0U), closes(0U), polls(0U), openReady(true), closeReady(true) {}
    uint16_t adapterId() const override {
        return blinker::gateway::kGatewaySouthboundAdapterNativeGatt;
    }
    blinker::Result openWindow() override {
        ++opens;
        if (openResult != blinker::ErrorCode::Ok) {
            if (openResult == blinker::ErrorCode::WouldBlock) {
                stateValue = blinker::GatewayPermitJoinAdapterState::Opening;
            }
            return blinker::Result::failure(openResult);
        }
        stateValue = blinker::GatewayPermitJoinAdapterState::Ready;
        return blinker::Result::success();
    }
    void closeWindow() override {
        ++closes;
        stateValue = closeReady ? blinker::GatewayPermitJoinAdapterState::Stopped
                                : blinker::GatewayPermitJoinAdapterState::Closing;
    }
    void poll(uint32_t) override {
        ++polls;
        if (stateValue == blinker::GatewayPermitJoinAdapterState::Opening && openReady) {
            stateValue = blinker::GatewayPermitJoinAdapterState::Ready;
        } else if (stateValue == blinker::GatewayPermitJoinAdapterState::Closing && closeReady) {
            stateValue = blinker::GatewayPermitJoinAdapterState::Stopped;
        }
    }
    blinker::GatewayPermitJoinAdapterState state() const override {
        return stateValue;
    }
    blinker::ErrorCode lastError() const override { return error; }

    blinker::GatewayPermitJoinAdapterState stateValue;
    blinker::ErrorCode error;
    blinker::ErrorCode openResult;
    uint32_t opens;
    uint32_t closes;
    uint32_t polls;
    bool openReady;
    bool closeReady;
};

// One genuinely empty protected binding; terminal absence must still be
// checked against a complete directory while permit owns/drains its port.
class EmptyBinding final : public blinker::IAtomicBlobStore, public blinker::IGatewayManagementDelivery {
public:
    EmptyBinding() : records(*this, *this), entry(records, *this), directory(&entry, 1U) {}
    blinker::Result load(blinker::MutableByteSpan, size_t& size) override {
        size = 0U; return blinker::Result::failure(blinker::ErrorCode::NotFound);
    }
    blinker::Result replace(blinker::ByteView) override {
        return blinker::Result::failure(blinker::ErrorCode::UnsupportedFeature);
    }
    blinker::Result clear() override { return blinker::Result::success(); }
    blinker::StorageProtection protection() const override { return blinker::StorageProtection::EncryptedFlash; }
    blinker::Result process(const blinker::GatewayAccessDeliveryContext&, blinker::ByteView,
                           blinker::MutableByteSpan, size_t& size) override {
        size = 0U; return blinker::Result::failure(blinker::ErrorCode::UnsupportedFeature);
    }
    blinker::GatewayChildRecords records;
    blinker::GatewayChildDirectoryEntry entry;
    blinker::GatewayChildDirectory directory;
};

struct EncodedCommand {
    uint8_t bytes[blinker::gateway::kGatewayPermitJoinCommandMaximumEncodedSize];
    size_t size;
    EncodedCommand() : bytes(), size(0U) {}
};

EncodedCommand command(
    uint8_t operationSeed,
    uint16_t adapterId,
    blinker::gateway::GatewayPermitJoinAction action,
    uint64_t expiresAt) {
    uint8_t operationId[blinker::gateway::kOperationIdSize] = {};
    std::memset(operationId, operationSeed, sizeof(operationId));
    blinker::gateway::GatewayPermitJoinCommandView value;
    value.operationId = blinker::ByteView(operationId, sizeof(operationId));
    value.adapterId = adapterId;
    value.action = action;
    value.expiresAtUnixSeconds = expiresAt;
    EncodedCommand encoded;
    CHECK_TRUE(blinker::gateway::encodeGatewayPermitJoinCommand(
        value,
        blinker::MutableByteSpan(encoded.bytes, sizeof(encoded.bytes)),
        encoded.size));
    return encoded;
}

blinker::gateway::GatewayPermitJoinResultView resultOf(
    const blinker::GatewayPermitJoinCoordinator& coordinator) {
    blinker::gateway::GatewayPermitJoinResultView result;
    CHECK_TRUE(blinker::gateway::decodeGatewayPermitJoinResult(
        coordinator.pendingResult(), result));
    return result;
}

void testYieldAdmission() {
    class Admission final : public blinker::IGatewayManagementAdmission {
    public:
        bool allowed = false;
        bool allowNewManagementWork() override { return allowed; }
    } admission;
    FakeClock clock; FakeAdapter adapter; EmptyBinding binding;
    blinker::GatewayPermitJoinCoordinator coordinator(adapter, clock);
    blinker::GatewayChildManagementRouter router(binding.directory, coordinator);
    router.setPermitAdmission(admission);
    const auto open = command(0x11, 1, blinker::gateway::GatewayPermitJoinAction::Open, 1100);
    const auto other = command(0x22, 1, blinker::gateway::GatewayPermitJoinAction::Open, 1100);
    const auto extended = command(0x11, 1, blinker::gateway::GatewayPermitJoinAction::Open, 1200);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, router.handleCommand(blinker::ByteView(open.bytes, open.size)).code());
    CHECK_EQ(0U, adapter.opens); CHECK_TRUE(!coordinator.ownsChildSession());
    admission.allowed = true;
    CHECK_TRUE(router.handleCommand(blinker::ByteView(open.bytes, open.size)));
    admission.allowed = false;
    CHECK_TRUE(router.handleCommand(blinker::ByteView(open.bytes, open.size)));
    CHECK_EQ(1U, adapter.opens);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, router.handleCommand(blinker::ByteView(other.bytes, other.size)).code());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, router.handleCommand(blinker::ByteView(extended.bytes, extended.size)).code());
    const auto close = command(0x11, 1, blinker::gateway::GatewayPermitJoinAction::Close, 1100);
    adapter.closeReady = false;
    CHECK_TRUE(router.handleCommand(blinker::ByteView(close.bytes, close.size)));
    CHECK_TRUE(router.ownsChildSession());
    router.reset(); CHECK_TRUE(router.ownsChildSession()); // Reset cannot reopen admission or forget close.
    CHECK_EQ(blinker::ErrorCode::WouldBlock, router.handleCommand(blinker::ByteView(other.bytes, other.size)).code());
    adapter.closeReady = true; router.poll(); CHECK_TRUE(!router.ownsChildSession());
}

void testCoordinator() {
    FakeClock clock;
    FakeAdapter adapter;
    blinker::GatewayPermitJoinCoordinator coordinator(adapter, clock);
    const EncodedCommand open = command(
        0x11U,
        blinker::gateway::kGatewaySouthboundAdapterNativeGatt,
        blinker::gateway::GatewayPermitJoinAction::Open,
        1100U);
    CHECK_TRUE(coordinator.handleCommand(
        blinker::ByteView(open.bytes, open.size)));
    CHECK_TRUE(coordinator.windowOpen());
    CHECK_EQ(1U, adapter.opens);
    CHECK_EQ(
        blinker::gateway::GatewayPermitJoinStatus::Ready,
        resultOf(coordinator).status);
    coordinator.markResultPublished();

    CHECK_TRUE(coordinator.handleCommand(
        blinker::ByteView(open.bytes, open.size)));
    CHECK_EQ(1U, adapter.opens);
    CHECK_EQ(
        blinker::gateway::GatewayPermitJoinStatus::Ready,
        resultOf(coordinator).status);
    coordinator.markResultPublished();

    const EncodedCommand competing = command(
        0x22U,
        blinker::gateway::kGatewaySouthboundAdapterNativeGatt,
        blinker::gateway::GatewayPermitJoinAction::Open,
        1100U);
    CHECK_TRUE(coordinator.handleCommand(
        blinker::ByteView(competing.bytes, competing.size)));
    CHECK_EQ(
        blinker::gateway::GatewayPermitJoinStatus::Busy,
        resultOf(coordinator).status);
    CHECK_TRUE(coordinator.windowOpen());
    coordinator.markResultPublished();

    clock.unixSeconds = 1100U;
    coordinator.poll();
    CHECK_TRUE(!coordinator.windowOpen());
    CHECK_EQ(1U, adapter.closes);
    CHECK_EQ(
        blinker::gateway::GatewayPermitJoinStatus::Expired,
        resultOf(coordinator).status);
    coordinator.markResultPublished();
    CHECK_TRUE(coordinator.handleCommand(
        blinker::ByteView(open.bytes, open.size)));
    CHECK_EQ(1U, adapter.opens);
    CHECK_EQ(
        blinker::gateway::GatewayPermitJoinStatus::Expired,
        resultOf(coordinator).status);
    coordinator.markResultPublished();

    const EncodedCommand unsupported = command(
        0x33U, 99U,
        blinker::gateway::GatewayPermitJoinAction::Open,
        1200U);
    CHECK_TRUE(coordinator.handleCommand(
        blinker::ByteView(unsupported.bytes, unsupported.size)));
    CHECK_EQ(
        blinker::gateway::GatewayPermitJoinStatus::Unsupported,
        resultOf(coordinator).status);

    FakeAdapter closeAdapter;
    blinker::GatewayPermitJoinCoordinator closeCoordinator(
        closeAdapter, clock);
    const EncodedCommand closeOpen = command(
        0x55U,
        blinker::gateway::kGatewaySouthboundAdapterNativeGatt,
        blinker::gateway::GatewayPermitJoinAction::Open,
        1300U);
    const EncodedCommand close = command(
        0x55U,
        blinker::gateway::kGatewaySouthboundAdapterNativeGatt,
        blinker::gateway::GatewayPermitJoinAction::Close,
        1300U);
    CHECK_TRUE(closeCoordinator.handleCommand(
        blinker::ByteView(closeOpen.bytes, closeOpen.size)));
    CHECK_TRUE(closeCoordinator.handleCommand(
        blinker::ByteView(close.bytes, close.size)));
    CHECK_EQ(
        blinker::gateway::GatewayPermitJoinStatus::Closed,
        resultOf(closeCoordinator).status);
    CHECK_TRUE(closeCoordinator.handleCommand(
        blinker::ByteView(closeOpen.bytes, closeOpen.size)));
    CHECK_EQ(1U, closeAdapter.opens);
    CHECK_TRUE(!closeCoordinator.windowOpen());
    CHECK_EQ(
        blinker::gateway::GatewayPermitJoinStatus::Closed,
        resultOf(closeCoordinator).status);

    coordinator.markResultPublished();
    EmptyBinding binding;
    blinker::GatewayChildManagementRouter router(binding.directory, coordinator);
    clock.unixSeconds = 1000U;
    const EncodedCommand throughRouter = command(
        0x44U,
        blinker::gateway::kGatewaySouthboundAdapterNativeGatt,
        blinker::gateway::GatewayPermitJoinAction::Open,
        1200U);
    CHECK_TRUE(router.handleCommand(
        blinker::ByteView(throughRouter.bytes, throughRouter.size)));
    CHECK_TRUE(!router.pendingResult().empty());
    CHECK_EQ(2U, adapter.opens);
}

void testPendingOpenBudget() {
    // Waiting for another owner is not Busy/Ready; duplicates retain the
    // original deadline. Exercise waiting, acquired, rollback, and uint32 wrap.
    for (unsigned int boundary = 0U; boundary < 5U; ++boundary) {
        FakeClock clock;
        clock.monotonic = UINT32_MAX - 100U;
        FakeAdapter adapter;
        adapter.openResult = blinker::ErrorCode::WouldBlock;
        adapter.openReady = false;
        blinker::GatewayPermitJoinCoordinator coordinator(adapter, clock);
        const EncodedCommand open = command(0x41U, 1U,
            blinker::gateway::GatewayPermitJoinAction::Open, 1100U);
        CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(open.bytes, open.size)));
        CHECK_TRUE(coordinator.busy());
        CHECK_TRUE(coordinator.ownsChildSession());
        CHECK_TRUE(!coordinator.windowOpen());
        CHECK_TRUE(coordinator.pendingResult().empty());
        clock.monotonic += 99000U;
        CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(open.bytes, open.size)));
        CHECK_EQ(1U, adapter.opens);
        CHECK_TRUE(coordinator.pendingResult().empty());
        if (boundary == 1U) {
            adapter.openReady = true;
            coordinator.poll();
            CHECK_TRUE(coordinator.windowOpen());
            CHECK_EQ(blinker::gateway::GatewayPermitJoinStatus::Ready, resultOf(coordinator).status);
            coordinator.markResultPublished();
        }
        if (boundary <= 1U) {
            clock.unixSeconds = 999U; // Rollback cannot extend the pinned budget.
            clock.monotonic += 1000U;
        } else if (boundary == 2U) {
            clock.unixSeconds = 1100U;
        } else if (boundary == 3U) {
            clock.synchronized = false;
        } else {
            const EncodedCommand close = command(0x41U, 1U,
                blinker::gateway::GatewayPermitJoinAction::Close, 1100U);
            CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(close.bytes, close.size)));
        }
        coordinator.poll();
        CHECK_TRUE(!coordinator.busy());
        CHECK_TRUE(!coordinator.ownsChildSession());
        const auto expected = boundary == 4U ? blinker::gateway::GatewayPermitJoinStatus::Closed
            : boundary == 3U ? blinker::gateway::GatewayPermitJoinStatus::Rejected
                            : blinker::gateway::GatewayPermitJoinStatus::Expired;
        CHECK_EQ(expected, resultOf(coordinator).status);
        coordinator.markResultPublished();
        CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(open.bytes, open.size)));
        CHECK_EQ(expected, resultOf(coordinator).status);
        CHECK_EQ(1U, adapter.opens);
    }
    FakeClock clock;
    FakeAdapter adapter;
    blinker::GatewayPermitJoinCoordinator coordinator(adapter, clock);
    const EncodedCommand tooLong = command(0x42U, 1U,
        blinker::gateway::GatewayPermitJoinAction::Open, 1901U);
    CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(tooLong.bytes, tooLong.size)));
    CHECK_EQ(blinker::gateway::GatewayPermitJoinStatus::Rejected, resultOf(coordinator).status);
    CHECK_EQ(0U, adapter.opens);
    const EncodedCommand maximum = command(0x43U, 1U,
        blinker::gateway::GatewayPermitJoinAction::Open, 1900U);
    CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(maximum.bytes, maximum.size)));
    CHECK_TRUE(coordinator.windowOpen());
    coordinator.reset();
    CHECK_TRUE(!coordinator.ownsChildSession());
    CHECK_TRUE(coordinator.pendingResult().empty());
}

void testPublishedReadyStillOwnsWindow() {
    for (unsigned int terminal = 0U; terminal < 4U; ++terminal) {
        FakeClock clock;
        FakeAdapter adapter;
        adapter.closeReady = false;
        blinker::GatewayPermitJoinCoordinator permit(adapter, clock);
        EmptyBinding binding;
        blinker::GatewayChildManagementRouter router(binding.directory, permit);
        const EncodedCommand open = command(0x51U, 1U,
            blinker::gateway::GatewayPermitJoinAction::Open, 1100U);
        CHECK_TRUE(router.handleCommand(blinker::ByteView(open.bytes, open.size)));
        router.markResultPublished(); // Ready was sent, but the lease is live.
        CHECK_TRUE(router.pendingResult().empty());
        CHECK_TRUE(router.busy());
        const uint32_t closes = adapter.closes;

        uint8_t id[16U];
        std::memset(id, 0x52, sizeof(id));
        blinker::gateway::GatewayRevocationCommandView supersede;
        supersede.operationId = blinker::ByteView(id, sizeof(id));
        supersede.topologyVersion = 5U;
        supersede.childDeviceInstanceId = supersede.operationId;
        supersede.accessEpoch = 7U;
        supersede.controllerId = supersede.operationId;
        supersede.gatewayCredentialVersion = 1U;
        supersede.accessStorageRevision = 1U;
        supersede.phase = blinker::gateway::GatewayRevocationPhase::Supersede;
        uint8_t digest[32U];
        std::memset(digest, 0x53, sizeof(digest));
        supersede.receiptDigest = blinker::ByteView(digest, sizeof(digest));
        uint8_t encoded[256U] = {};
        size_t size = 0U;
        CHECK_TRUE(blinker::gateway::encodeGatewayRevocationCommand(
            supersede, blinker::MutableByteSpan(encoded, sizeof(encoded)), size));
        CHECK_EQ(blinker::ErrorCode::WouldBlock,
            router.handleCommand(blinker::ByteView(encoded, size)).code());
        CHECK_TRUE(permit.windowOpen());
        CHECK_EQ(closes, adapter.closes);
        router.poll();
        CHECK_TRUE(adapter.polls > 0U);
        // Same-family duplicate/Close is still admitted while other work waits.
        CHECK_TRUE(router.handleCommand(blinker::ByteView(open.bytes, open.size)));
        router.markResultPublished();
        if (terminal == 0U) {
            const EncodedCommand close = command(0x51U, 1U,
                blinker::gateway::GatewayPermitJoinAction::Close, 1100U);
            CHECK_TRUE(router.handleCommand(blinker::ByteView(close.bytes, close.size)));
        } else if (terminal == 1U) {
            clock.unixSeconds = 1100U;
            router.poll();
        } else if (terminal == 2U) {
            permit.rejectWindow();
        } else {
            router.reset(); // Wire context disappears, native ownership does not.
        }
        CHECK_TRUE(!permit.windowOpen());
        CHECK_TRUE(router.ownsChildSession());
        CHECK_TRUE(router.pendingResult().empty());
        CHECK_EQ(blinker::ErrorCode::WouldBlock,
            router.handleCommand(blinker::ByteView(encoded, size)).code());
        const uint32_t polls = adapter.polls;
        router.poll();
        CHECK_TRUE(adapter.polls > polls);
        CHECK_TRUE(router.ownsChildSession());
        if (terminal != 3U) {
            CHECK_TRUE(router.busy());
            CHECK_TRUE(router.handleCommand(blinker::ByteView(open.bytes, open.size)));
            CHECK_TRUE(router.pendingResult().empty()); // No premature terminal replay.
        }
        adapter.closeReady = true;
        router.poll();
        CHECK_TRUE(!router.ownsChildSession());
        CHECK_TRUE(!router.busy());
        if (terminal != 3U) {
            CHECK_EQ(blinker::ErrorCode::WouldBlock,
                router.handleCommand(blinker::ByteView(encoded, size)).code());
        } else {
            CHECK_TRUE(router.pendingResult().empty()); // No result from old MQTT generation.
        }
        router.markResultPublished();
        // Only now can the router publish an exact whole-directory absence.
        CHECK_TRUE(router.handleCommand(blinker::ByteView(encoded, size)));
        blinker::gateway::GatewayRevocationResultView reply;
        CHECK_TRUE(blinker::gateway::decodeGatewayRevocationResult(router.pendingResult(), reply));
        CHECK_EQ(blinker::gateway::GatewayRevocationStatus::Retired, reply.status);
    }
}

class FakeLease final : public blinker::IGattPermitJoinPortLease {
public:
    FakeLease() : port(nullptr), acquired(false), acquires(0U), releases(0U), blocked(false) {}
    FakeLease& forPort(blinker::IBleCentralPort& value) { port = &value; return *this; }
    blinker::Result acquirePermitJoinPort(blinker::IBleCentralPort*& output) override {
        output = nullptr;
        if (blocked) return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        if (acquired) {
            return blinker::Result::failure(
                blinker::ErrorCode::AlreadyExists);
        }
        acquired = true;
        output = port;
        ++acquires;
        return blinker::Result::success();
    }
    void releasePermitJoinPort(blinker::IBleCentralPort& value) override {
        CHECK_TRUE(&value == port);
        if (acquired) ++releases;
        acquired = false;
    }
    bool allowPermitJoinScan(blinker::IBleCentralPort&) override { return scanAllowed; }
    bool allowPermitJoinTarget(const blinker::BleCentralAdvertisement&) override { return targetAllowed; }
    blinker::IBleCentralPort* port;
    bool scanAllowed = true, targetAllowed = true;
    bool acquired;
    uint32_t acquires;
    uint32_t releases;
    bool blocked;
};

class FakeCentralPort final : public blinker::IBleCentralPort {
public:
    struct Packet {
        uint8_t bytes[BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE];
        uint8_t size;
        Packet() : bytes(), size(0U) {}
    };

    FakeCentralPort()
        : stateValue(blinker::BleCentralPortState::Stopped), request(),
          connectionValue(), packetReceiver(nullptr), packetContext(nullptr),
          connectedHandler(nullptr), disconnectedHandler(nullptr),
          connectionContext(nullptr), advertisement(), advertisementSize(0U),
          advertisementPending(false), connectPending(false), inbound(),
          inboundPending(false), sent(), sentCount(0U),
          sendError(blinker::ErrorCode::Ok), cancelError(blinker::ErrorCode::Ok),
          asynchronousCancel(false), cancelComplete(false), cancels(0U), stops(0U) {}

    blinker::Result start() override {
        if (stateValue != blinker::BleCentralPortState::Stopped) {
            return blinker::Result::failure(
                blinker::ErrorCode::AlreadyExists);
        }
        stateValue = blinker::BleCentralPortState::Idle;
        return blinker::Result::success();
    }
    void stop() override {
        ++stops;
        stateValue = blinker::BleCentralPortState::Stopped;
        request = blinker::BleCentralConnectRequest();
        connectionValue = blinker::BleCentralConnectionInfo();
        advertisementPending = false;
        connectPending = false;
        inboundPending = false;
    }
    void poll(uint32_t) override {
        if (cancelComplete && stateValue == blinker::BleCentralPortState::Disconnecting) {
            cancelComplete = false;
            stateValue = blinker::BleCentralPortState::Idle;
            blinker::BleCentralConnectionInfo disconnected;
            disconnected.attemptId = request.attemptId;
            if (disconnectedHandler != nullptr) disconnectedHandler(connectionContext, disconnected);
        }
        if (advertisementPending &&
            stateValue == blinker::BleCentralPortState::Scanning) {
            advertisementPending = false;
            blinker::BleCentralAdvertisement advertisementView;
            advertisementView.modeServiceData = blinker::ByteView(
                advertisement, advertisementSize);
            advertisementView.rssi = -35;
            if (request.matcher != nullptr &&
                request.matcher(
                    request.matcherContext, advertisementView)) {
                stateValue = blinker::BleCentralPortState::Connecting;
            }
        }
        if (connectPending &&
            stateValue == blinker::BleCentralPortState::Connecting) {
            connectPending = false;
            stateValue = blinker::BleCentralPortState::Ready;
            connectionValue.attemptId = request.attemptId;
            connectionValue.maxPacketSize =
                BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE;
            connectionValue.connected = true;
            connectionValue.notifyEnabled = true;
            if (connectedHandler != nullptr) {
                connectedHandler(connectionContext, connectionValue);
            }
        }
        if (inboundPending &&
            stateValue == blinker::BleCentralPortState::Ready) {
            inboundPending = false;
            if (packetReceiver != nullptr) {
                packetReceiver(
                    packetContext, connectionValue,
                    blinker::ByteView(inbound.bytes, inbound.size));
            }
        }
    }
    blinker::BleCentralPortState state() const override { return stateValue; }
    blinker::ErrorCode lastError() const override {
        return blinker::ErrorCode::Ok;
    }
    blinker::Result connect(
        const blinker::BleCentralConnectRequest& value) override {
        if (stateValue != blinker::BleCentralPortState::Idle) {
            return blinker::Result::failure(
                blinker::ErrorCode::AlreadyExists);
        }
        request = value;
        stateValue = blinker::BleCentralPortState::Scanning;
        return blinker::Result::success();
    }
    blinker::Result cancel(uint32_t attemptId) override {
        ++cancels;
        if (cancelError != blinker::ErrorCode::Ok) return blinker::Result::failure(cancelError);
        if (attemptId != request.attemptId) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        stateValue = asynchronousCancel ? blinker::BleCentralPortState::Disconnecting
                                        : blinker::BleCentralPortState::Idle;
        return blinker::Result::success();
    }
    blinker::Result connection(
        blinker::BleCentralConnectionInfo& output) const override {
        if (stateValue != blinker::BleCentralPortState::Ready) {
            return blinker::Result::failure(
                blinker::ErrorCode::NotConnected);
        }
        output = connectionValue;
        return blinker::Result::success();
    }
    blinker::Result sendPacket(
        uint32_t attemptId,
        blinker::ByteView packet) override {
        if (sendError != blinker::ErrorCode::Ok) {
            return blinker::Result::failure(sendError);
        }
        if (attemptId != connectionValue.attemptId ||
            sentCount >= sizeof(sent) / sizeof(sent[0])) {
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        sent[sentCount].size = static_cast<uint8_t>(packet.size);
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
        connectionContext = context;
    }

    void offer(blinker::ByteView serviceData) {
        std::memcpy(advertisement, serviceData.data, serviceData.size);
        advertisementSize = serviceData.size;
        advertisementPending = true;
    }
    void finishConnect() { connectPending = true; }
    void inject(uint8_t seed) {
        inbound.size = 7U;
        std::memset(inbound.bytes, seed, inbound.size);
        inboundPending = true;
    }

    blinker::BleCentralPortState stateValue;
    blinker::BleCentralConnectRequest request;
    blinker::BleCentralConnectionInfo connectionValue;
    blinker::BleCentralPacketReceiver packetReceiver;
    void* packetContext;
    blinker::BleCentralConnectionHandler connectedHandler;
    blinker::BleCentralConnectionHandler disconnectedHandler;
    void* connectionContext;
    uint8_t advertisement[blinker::ble::kModeServiceDataSize];
    size_t advertisementSize;
    bool advertisementPending;
    bool connectPending;
    Packet inbound;
    bool inboundPending;
    Packet sent[8];
    size_t sentCount;
    blinker::ErrorCode sendError;
    blinker::ErrorCode cancelError;
    bool asynchronousCancel;
    bool cancelComplete;
    uint32_t cancels;
    uint32_t stops;
};

blinker::ByteView provisioningAdvertisement(
    uint8_t (&storage)[blinker::ble::kModeServiceDataSize],
    uint8_t (&locator)[blinker::kSetupSessionLocatorSize]) {
    const blinker::ble::ModeProfile profile =
        blinker::ble::makeProvisioningModeProfile(
            blinker::ByteView(locator, sizeof(locator)), false);
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::ble::encodeModeServiceData(
        profile,
        blinker::MutableByteSpan(storage, sizeof(storage)), encoded));
    return encoded;
}

blinker::ByteView authorizedDirectAdvertisement(
    uint8_t (&storage)[blinker::ble::kModeServiceDataSize],
    uint8_t (&locator)[blinker::kSetupSessionLocatorSize]) {
    const blinker::ble::ModeProfile profile =
        blinker::ble::makeAuthorizedDirectModeProfile(
            blinker::ByteView(locator, sizeof(locator)));
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::ble::encodeModeServiceData(
        profile,
        blinker::MutableByteSpan(storage, sizeof(storage)), encoded));
    return encoded;
}

void testNativeGattAdapter() {
    FakeClock clock;
    FakeLease lease;
    FakeCentralPort port;
    port.stateValue = blinker::BleCentralPortState::Ready;
    {
        blinker::NativeGattPermitJoinAdapter idleAdapter(clock, lease.forPort(port));
        idleAdapter.clearRelaySelection(); // No lease: must not touch someone else's port.
    }
    CHECK_EQ(blinker::BleCentralPortState::Ready, port.state());
    CHECK_EQ(0U, lease.releases);
    port.stateValue = blinker::BleCentralPortState::Stopped;

    // Provisioning and Direct use separate permit-join windows. Direct is
    // exposed only with the rotating authorized-presence locator; legacy
    // all-zero Direct advertisements never become selectable candidates.
    {
        FakeCentralPort directPort;
        FakeLease directLease;
        blinker::NativeGattPermitJoinAdapter directAdapter(clock, directLease.forPort(directPort));
        CHECK_TRUE(directAdapter.openWindow());
        uint8_t directLocator[blinker::kSetupSessionLocatorSize] = {
            9U, 10U, 11U, 12U, 13U, 14U, 15U, 16U
        };
        uint8_t directServiceData[blinker::ble::kModeServiceDataSize] = {};
        directPort.offer(authorizedDirectAdvertisement(
            directServiceData, directLocator));
        directAdapter.poll(1000U);
        CHECK_EQ(
            static_cast<size_t>(1U),
            directAdapter.relayCandidateCount());
        blinker::GatewayPermitJoinCandidate directCandidate;
        CHECK_TRUE(directAdapter.readRelayCandidate(0U, directCandidate));
        CHECK_EQ(blinker::ble::kAuthorizedDirectBbp2WireVersion,
                 directCandidate.wireVersion);
        CHECK_TRUE(std::memcmp(
            directCandidate.token,
            directLocator,
            sizeof(directLocator)) == 0);
        directAdapter.closeWindow();
    }

    blinker::NativeGattPermitJoinConfig config;
    config.candidateTtlMillis = 50U;
    blinker::NativeGattPermitJoinAdapter adapter(clock, lease.forPort(port), config);
    CHECK_TRUE(adapter.openWindow());
    CHECK_EQ(1U, lease.acquires);
    CHECK_EQ(blinker::BleCentralPortState::Scanning, port.state());

    uint8_t locator[blinker::kSetupSessionLocatorSize] = {
        1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U
    };
    uint8_t serviceData[blinker::ble::kModeServiceDataSize] = {};
    const blinker::ByteView advertisement =
        provisioningAdvertisement(serviceData, locator);
    port.offer(advertisement);
    adapter.poll(1000U);
    CHECK_EQ(static_cast<size_t>(1U), adapter.relayCandidateCount());
    CHECK_EQ(blinker::BleCentralPortState::Scanning, port.state());
    blinker::GatewayPermitJoinCandidate candidate;
    CHECK_TRUE(adapter.readRelayCandidate(0U, candidate));
    CHECK_TRUE(std::memcmp(candidate.token, locator, sizeof(locator)) == 0);
    CHECK_TRUE(adapter.selectRelayCandidate(
        blinker::ByteView(locator, sizeof(locator))));

    port.offer(advertisement);
    adapter.poll(1000U);
    CHECK_EQ(blinker::BleCentralPortState::Connecting, port.state());
    port.finishConnect();
    adapter.poll(1000U);
    CHECK_TRUE(adapter.relayConnected());

    const uint8_t outbound[] = {1U, 2U, 3U, 4U, 5U};
    CHECK_TRUE(adapter.queueRelayPacket(
        blinker::ByteView(outbound, sizeof(outbound))));
    CHECK_EQ(static_cast<size_t>(1U), port.sentCount);
    CHECK_TRUE(std::memcmp(
        port.sent[0].bytes, outbound, sizeof(outbound)) == 0);

    // Backpressure preserves this ACL/security session; a disconnect does not.
    // The exact packet may retry only while this same connection remains alive.
    port.sendError = blinker::ErrorCode::WouldBlock;
    const blinker::Result blockedSend = adapter.queueRelayPacket(
        blinker::ByteView(outbound, sizeof(outbound)));
    CHECK_EQ(blinker::ErrorCode::WouldBlock, blockedSend.code());
    CHECK_EQ(
        blinker::GatewayPermitJoinAdapterState::Ready,
        adapter.state());
    CHECK_EQ(static_cast<size_t>(1U), port.sentCount);
    CHECK_EQ(blinker::BleCentralPortState::Ready, port.state());
    port.sendError = blinker::ErrorCode::Ok;
    adapter.poll(1000U);
    CHECK_TRUE(adapter.relayConnected());
    CHECK_TRUE(adapter.queueRelayPacket(
        blinker::ByteView(outbound, sizeof(outbound))));
    CHECK_EQ(static_cast<size_t>(2U), port.sentCount);

    // Even before a port delivers its disconnect event, NotConnected is
    // terminal for this selected window, not transport backpressure.
    port.sendError = blinker::ErrorCode::NotConnected;
    CHECK_EQ(blinker::ErrorCode::NotConnected,
        adapter.queueRelayPacket(blinker::ByteView(outbound, sizeof(outbound))).code());
    CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Fault, adapter.state());
    adapter.closeWindow();
    CHECK_EQ(blinker::ErrorCode::AlreadyExists, adapter.openWindow().code());
    adapter.poll(1000U);
    CHECK_TRUE(adapter.openWindow());
    port.sendError = blinker::ErrorCode::Ok;
    port.offer(advertisement);
    adapter.poll(1000U);
    CHECK_TRUE(adapter.selectRelayCandidate(blinker::ByteView(locator, sizeof(locator))));
    port.offer(advertisement);
    adapter.poll(1000U);
    port.finishConnect();
    adapter.poll(1000U);

    port.inject(0xA5U);
    adapter.poll(1000U);
    uint8_t received[BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE] = {};
    size_t written = 0U;
    CHECK_TRUE(adapter.takeRelayPacket(
        blinker::MutableByteSpan(received, sizeof(received)), written));
    CHECK_EQ(static_cast<size_t>(7U), written);
    CHECK_EQ(static_cast<uint8_t>(0xA5U), received[0]);

    for (uint8_t index = 0U;
         index < BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH;
         ++index) {
        port.inject(index);
        adapter.poll(1000U);
    }
    port.inject(0xFFU);
    adapter.poll(1000U);
    CHECK_EQ(
        blinker::GatewayPermitJoinAdapterState::Fault,
        adapter.state());
    CHECK_EQ(1U, adapter.droppedPacketCount());

    adapter.closeWindow();
    adapter.poll(1000U);
    CHECK_EQ(2U, lease.releases);
    CHECK_EQ(
        blinker::GatewayPermitJoinAdapterState::Stopped,
        adapter.state());
}

void testNativeAsynchronousHandoff() {
    // An Opening request cannot replace the old owner's callbacks or cancel
    // its GAP attempt. Cancelling before acquisition must leave it untouched.
    FakeClock clock;
    FakeLease lease;
    FakeCentralPort port;
    lease.blocked = true;
    port.stateValue = blinker::BleCentralPortState::Ready;
    port.connectionContext = &lease;
    blinker::NativeGattPermitJoinAdapter adapter(clock, lease.forPort(port));
    CHECK_EQ(blinker::ErrorCode::WouldBlock, adapter.openWindow().code());
    adapter.poll(1000U);
    CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Opening, adapter.state());
    CHECK_TRUE(port.connectionContext == &lease);
    CHECK_EQ(0U, lease.acquires);
    adapter.closeWindow();
    CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Stopped, adapter.state());
    adapter.clearRelaySelection(); // The returned physical slot no longer belongs to this adapter.
    CHECK_EQ(0U, port.cancels);
    CHECK_EQ(0U, port.stops);
    CHECK_EQ(0U, lease.releases);
    CHECK_TRUE(port.connectionContext == &lease);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, adapter.openWindow().code());
    port.stateValue = blinker::BleCentralPortState::Stopped; // old owner finished
    lease.blocked = false;
    adapter.poll(1000U);
    CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Ready, adapter.state());
    CHECK_TRUE(port.connectionContext == &adapter);
    adapter.closeWindow();
    adapter.poll(1000U);
    CHECK_EQ(1U, lease.releases);

    // Scan, in-flight CONNECT and established ACL all keep the lease until the
    // Port reports completion. Failed cancellation retries; accepted does not.
    for (unsigned int stage = 0U; stage < 3U; ++stage) {
        FakeCentralPort closingPort;
        FakeLease closingLease;
        closingPort.asynchronousCancel = true;
        blinker::NativeGattPermitJoinAdapter closing(clock, closingLease.forPort(closingPort));
        CHECK_TRUE(closing.openWindow());
        uint8_t locator[blinker::kSetupSessionLocatorSize] = {1U};
        uint8_t serviceData[blinker::ble::kModeServiceDataSize] = {};
        const blinker::ByteView advert = provisioningAdvertisement(serviceData, locator);
        closingPort.offer(advert);
        closing.poll(1000U);
        if (stage != 0U) {
            CHECK_TRUE(closing.selectRelayCandidate(blinker::ByteView(locator, sizeof(locator))));
            closingPort.offer(advert);
            closing.poll(1000U);
        }
        if (stage == 2U) {
            closingPort.finishConnect();
            closing.poll(1000U);
            CHECK_TRUE(closing.relayConnected());
        }
        const auto connected = closingPort.connectedHandler;
        const auto packet = closingPort.packetReceiver;
        const auto disconnected = closingPort.disconnectedHandler;
        closing.closeWindow();
        CHECK_TRUE(!closing.relayConnected());
        CHECK_EQ(0U, closing.relayCandidateCount());
        const uint8_t byte[] = {1U};
        CHECK_EQ(blinker::ErrorCode::NotConnected,
            closing.queueRelayPacket(blinker::ByteView(byte, sizeof(byte))).code());
        CHECK_EQ(0U, closingPort.stops);
        closingPort.cancelError = blinker::ErrorCode::WouldBlock;
        closing.poll(1000U);
        CHECK_EQ(1U, closingPort.cancels);
        CHECK_EQ(0U, closingLease.releases);
        closingPort.cancelError = blinker::ErrorCode::Ok;
        closing.poll(1000U);
        closing.poll(1000U);
        CHECK_EQ(2U, closingPort.cancels);
        CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Closing, closing.state());
        CHECK_EQ(0U, closingPort.stops);
        CHECK_EQ(0U, closingLease.releases);
        CHECK_TRUE(closingPort.connectionContext == &closing);
        // Late callbacks cannot revive relay I/O; stale disconnect cannot
        // retire this attempt. The Port still owns the in-flight connection.
        blinker::BleCentralConnectionInfo late;
        late.attemptId = closingPort.request.attemptId;
        late.connected = true;
        late.notifyEnabled = true;
        late.maxPacketSize = BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE;
        connected(&closing, late);
        packet(&closing, late, blinker::ByteView(byte, sizeof(byte)));
        ++late.attemptId;
        disconnected(&closing, late);
        CHECK_TRUE(!closing.relayConnected());
        CHECK_EQ(blinker::ErrorCode::AlreadyExists, closing.openWindow().code());
        closingPort.cancelComplete = true;
        closing.poll(1000U);
        CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Stopped, closing.state());
        CHECK_EQ(1U, closingPort.stops);
        CHECK_EQ(1U, closingLease.releases);
        CHECK_TRUE(closingPort.connectionContext == nullptr);
        CHECK_TRUE(closingPort.packetReceiver == nullptr);
    }
}

void testCandidatesPreferLatestObservation() {
    FakeClock clock;
    FakeCentralPort port;
    FakeLease lease;
    blinker::NativeGattPermitJoinAdapter adapter(clock, lease.forPort(port));
    CHECK_TRUE(adapter.openWindow());
    uint8_t locator[blinker::kSetupSessionLocatorSize] = {};
    uint8_t serviceData[blinker::ble::kModeServiceDataSize] = {};
    // The fixed table can retain multiple authorized generations for a child.
    // Rank by last observation even across the monotonic clock wrap.
    for (uint8_t index = 0U; index < 4U; ++index) {
        clock.monotonic = 0xFFFFFFF0U + static_cast<uint32_t>(index) * 10U;
        locator[0] = index + 1U;
        port.offer(provisioningAdvertisement(serviceData, locator));
        adapter.poll(1000U);
    }
    blinker::GatewayPermitJoinCandidate candidate;
    for (size_t index = 0U; index < 4U; ++index) {
        CHECK_TRUE(adapter.readRelayCandidate(index, candidate));
        CHECK_EQ(static_cast<uint8_t>(4U - index), candidate.token[0]);
    }
    CHECK_EQ(blinker::ErrorCode::NotFound, adapter.readRelayCandidate(4U, candidate).code());
    // Refresh a previously inserted slot; equal timestamps retain stable slot order.
    clock.monotonic += 10U;
    const uint8_t refreshed[] = {1U, 3U};
    for (const uint8_t value : refreshed) {
        locator[0] = value;
        port.offer(provisioningAdvertisement(serviceData, locator));
        adapter.poll(1000U);
    }
    const uint8_t expected[] = {1U, 3U, 4U, 2U};
    for (size_t index = 0U; index < 4U; ++index) {
        CHECK_TRUE(adapter.readRelayCandidate(index, candidate));
        CHECK_EQ(expected[index], candidate.token[0]);
    }
    clock.monotonic += 10000U;
    adapter.poll(1000U);
    CHECK_EQ(static_cast<size_t>(0U), adapter.relayCandidateCount());
    adapter.closeWindow();
}

void testSelectedDisconnectEndsWindow() {
    for (unsigned int ready = 0U; ready < 2U; ++ready) {
        for (unsigned int callback = 0U; callback < 2U; ++callback) {
            FakeClock clock;
            FakeCentralPort port;
            FakeLease lease;
            blinker::NativeGattPermitJoinAdapter adapter(clock, lease.forPort(port));
            blinker::GatewayPermitJoinCoordinator coordinator(adapter, clock);
            const EncodedCommand open = command(0x31U, 1U,
                blinker::gateway::GatewayPermitJoinAction::Open, 1100U);
            CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(open.bytes, open.size)));
            coordinator.markResultPublished();
            uint8_t locator[blinker::kSetupSessionLocatorSize] = {1U};
            uint8_t serviceData[blinker::ble::kModeServiceDataSize] = {};
            const blinker::ByteView advertisement = provisioningAdvertisement(serviceData, locator);
            port.offer(advertisement);
            adapter.poll(1000U);
            CHECK_TRUE(adapter.selectRelayCandidate(blinker::ByteView(locator, sizeof(locator))));
            port.offer(advertisement);
            adapter.poll(1000U);
            if (ready != 0U) {
                port.finishConnect();
                adapter.poll(1000U);
                CHECK_TRUE(adapter.relayConnected());
            }
            blinker::BleCentralConnectionInfo previous = port.connectionValue;
            previous.attemptId = port.request.attemptId;
            port.stateValue = blinker::BleCentralPortState::Idle;
            if (callback != 0U) port.disconnectedHandler(port.connectionContext, previous);
            adapter.poll(1000U);
            CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Fault, adapter.state());
            CHECK_EQ(blinker::BleCentralPortState::Idle, port.state());
            CHECK_TRUE(!adapter.relayConnected());
            coordinator.poll();
            CHECK_TRUE(!coordinator.windowOpen());
            CHECK_EQ(blinker::gateway::GatewayPermitJoinStatus::Rejected, resultOf(coordinator).status);
            CHECK_EQ(1U, lease.releases);
            // Exact Open replay cannot silently resurrect the failed selection.
            CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(open.bytes, open.size)));
            CHECK_EQ(blinker::gateway::GatewayPermitJoinStatus::Rejected, resultOf(coordinator).status);
            CHECK_EQ(1U, lease.acquires);
            const EncodedCommand next = command(0x32U, 1U,
                blinker::gateway::GatewayPermitJoinAction::Open, 1100U);
            CHECK_TRUE(coordinator.handleCommand(blinker::ByteView(next.bytes, next.size)));
            CHECK_TRUE(coordinator.windowOpen());
            CHECK_EQ(2U, lease.acquires);
            coordinator.reset();
        }
    }
}

void testPermitJoinRelayCodec() {
    uint8_t operationId[blinker::gateway::kOperationIdSize] = {};
    for (size_t index = 0U; index < sizeof(operationId); ++index) {
        operationId[index] = static_cast<uint8_t>(0x10U + index);
    }
    const uint8_t token[] = {0xA1U, 0xB2U, 0xC3U, 0xD4U};
    const uint8_t snapshotGolden[] = {
        0x85U, 0x01U, 0x07U, 0x50U,
        0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
        0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
        0x01U, 0x81U, 0x84U, 0x44U, 0xA1U, 0xB2U, 0xC3U, 0xD4U,
        0x01U, 0x00U, 0x18U, 0x55U
    };
    const uint8_t selectGolden[] = {
        0x84U, 0x01U, 0x08U, 0x50U,
        0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
        0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
        0x44U, 0xA1U, 0xB2U, 0xC3U, 0xD4U
    };
    const uint8_t batchGolden[] = {
        0x86U, 0x01U, 0x0AU, 0x50U,
        0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
        0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
        0x01U, 0x01U, 0x81U, 0x43U, 0x01U, 0x02U, 0x03U
    };
    const uint8_t ackGolden[] = {
        0x86U, 0x01U, 0x0BU, 0x50U,
        0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
        0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
        0x01U, 0x01U, 0x01U
    };
    uint8_t encoded[
        blinker::gateway::kPermitJoinRelayBatchMaximumEncodedSize] = {};
    size_t written = 0U;

    blinker::gateway::PermitJoinCandidateSnapshotView snapshot;
    snapshot.operationId = blinker::ByteView(operationId, sizeof(operationId));
    snapshot.revision = 1U;
    snapshot.candidateCount = 1U;
    snapshot.candidates[0].token = blinker::ByteView(token, sizeof(token));
    snapshot.candidates[0].wireVersion = 1U;
    snapshot.candidates[0].capabilities = 0U;
    snapshot.candidates[0].signalQuality = 85U;
    CHECK_TRUE(blinker::gateway::encodePermitJoinCandidateSnapshot(
        snapshot, blinker::MutableByteSpan(encoded, sizeof(encoded)), written));
    CHECK_EQ(sizeof(snapshotGolden), written);
    CHECK_TRUE(std::memcmp(encoded, snapshotGolden, written) == 0);
    blinker::gateway::PermitJoinCandidateSnapshotView decodedSnapshot;
    CHECK_TRUE(blinker::gateway::decodePermitJoinCandidateSnapshot(
        blinker::ByteView(encoded, written), decodedSnapshot));
    CHECK_EQ(1U, decodedSnapshot.revision);
    CHECK_EQ(static_cast<size_t>(1U), decodedSnapshot.candidateCount);
    CHECK_EQ(85U, decodedSnapshot.candidates[0].signalQuality);
    CHECK_TRUE(std::memcmp(
        decodedSnapshot.candidates[0].token.data, token, sizeof(token)) == 0);

    blinker::gateway::PermitJoinSelectCommandView select;
    select.operationId = blinker::ByteView(operationId, sizeof(operationId));
    select.candidateToken = blinker::ByteView(token, sizeof(token));
    CHECK_TRUE(blinker::gateway::encodePermitJoinSelectCommand(
        select, blinker::MutableByteSpan(encoded, sizeof(encoded)), written));
    CHECK_EQ(sizeof(selectGolden), written);
    CHECK_TRUE(std::memcmp(encoded, selectGolden, written) == 0);
    blinker::gateway::PermitJoinSelectCommandView decodedSelect;
    CHECK_TRUE(blinker::gateway::decodePermitJoinSelectCommand(
        blinker::ByteView(encoded, written), decodedSelect));
    CHECK_EQ(sizeof(token), decodedSelect.candidateToken.size);

    blinker::gateway::PermitJoinSelectResultView selectResult;
    selectResult.operationId =
        blinker::ByteView(operationId, sizeof(operationId));
    selectResult.candidateToken = blinker::ByteView(token, sizeof(token));
    selectResult.status = blinker::gateway::PermitJoinSelectStatus::Connected;
    selectResult.maxPacketSize = 20U;
    CHECK_TRUE(blinker::gateway::encodePermitJoinSelectResult(
        selectResult, blinker::MutableByteSpan(encoded, sizeof(encoded)), written));
    blinker::gateway::PermitJoinSelectResultView decodedResult;
    CHECK_TRUE(blinker::gateway::decodePermitJoinSelectResult(
        blinker::ByteView(encoded, written), decodedResult));
    CHECK_EQ(blinker::gateway::PermitJoinSelectStatus::Connected,
             decodedResult.status);
    CHECK_EQ(20U, decodedResult.maxPacketSize);

    const uint8_t packet[] = {0x01U, 0x02U, 0x03U};
    blinker::gateway::PermitJoinRelayBatchView batch;
    batch.operationId = blinker::ByteView(operationId, sizeof(operationId));
    batch.direction = blinker::gateway::PermitJoinRelayDirection::Down;
    batch.sequence = 1U;
    batch.packets[0] = blinker::ByteView(packet, sizeof(packet));
    batch.packetCount = 1U;
    CHECK_TRUE(blinker::gateway::encodePermitJoinRelayBatch(
        batch, blinker::MutableByteSpan(encoded, sizeof(encoded)), written));
    CHECK_EQ(sizeof(batchGolden), written);
    CHECK_TRUE(std::memcmp(encoded, batchGolden, written) == 0);
    blinker::gateway::PermitJoinRelayBatchView decodedBatch;
    CHECK_TRUE(blinker::gateway::decodePermitJoinRelayBatch(
        blinker::ByteView(encoded, written), decodedBatch));
    CHECK_EQ(1U, decodedBatch.sequence);
    CHECK_EQ(static_cast<size_t>(1U), decodedBatch.packetCount);
    CHECK_EQ(sizeof(packet), decodedBatch.packets[0].size);

    blinker::gateway::PermitJoinRelayAckView ack;
    ack.operationId = blinker::ByteView(operationId, sizeof(operationId));
    ack.direction = blinker::gateway::PermitJoinRelayDirection::Down;
    ack.sequence = 1U;
    ack.status = blinker::gateway::PermitJoinRelayAckStatus::Accepted;
    CHECK_TRUE(blinker::gateway::encodePermitJoinRelayAck(
        ack, blinker::MutableByteSpan(encoded, sizeof(encoded)), written));
    CHECK_EQ(sizeof(ackGolden), written);
    CHECK_TRUE(std::memcmp(encoded, ackGolden, written) == 0);
    blinker::gateway::PermitJoinRelayAckView decodedAck;
    CHECK_TRUE(blinker::gateway::decodePermitJoinRelayAck(
        blinker::ByteView(encoded, written), decodedAck));
    CHECK_EQ(1U, decodedAck.sequence);
    CHECK_EQ(blinker::gateway::PermitJoinRelayAckStatus::Accepted,
             decodedAck.status);

    const uint8_t zeroToken[] = {0U};
    select.candidateToken = blinker::ByteView(zeroToken, sizeof(zeroToken));
    CHECK_TRUE(!blinker::gateway::encodePermitJoinSelectCommand(
        select, blinker::MutableByteSpan(encoded, sizeof(encoded)), written));
    batch.sequence = 0U;
    CHECK_TRUE(!blinker::gateway::encodePermitJoinRelayBatch(
        batch, blinker::MutableByteSpan(encoded, sizeof(encoded)), written));
}

class PoolBlob final : public blinker::IAtomicBlobStore {
public:
    std::vector<uint8_t> bytes;
    blinker::Result load(blinker::MutableByteSpan output, size_t& size) override {
        size = 0;
        if (bytes.empty()) return blinker::Result::failure(blinker::ErrorCode::NotFound);
        if (output.size < bytes.size()) return blinker::Result::failure(blinker::ErrorCode::BufferTooSmall);
        std::memcpy(output.data, bytes.data(), bytes.size()); size = bytes.size(); return blinker::Result::success();
    }
    blinker::Result replace(blinker::ByteView value) override { bytes.assign(value.data, value.data + value.size); return blinker::Result::success(); }
    blinker::Result clear() override { bytes.clear(); return blinker::Result::success(); }
    blinker::StorageProtection protection() const override { return blinker::StorageProtection::EncryptedFlash; }
};
struct PoolControl : blinker::IGatewayManagementControl {
    bool held = false;
    blinker::Result handleCommand(blinker::ByteView) override { held = true; return blinker::Result::success(); }
    bool busy() const override { return held; }
    bool ownsChildSession() const override { return held; }
    void poll() override {}
    blinker::ByteView pendingResult() const override { return {}; }
    void markResultPublished() override {}
    void reset() override { held = false; }
};
struct PoolBridge {
    bool wanted = false, direct = false;
    unsigned dataPolls = 0, receipts = 0;
    bool executionWanted() const { return wanted; }
    bool directYieldHolding() const { return direct; }
    void reset() { wanted = false; }
    bool poll() { ++dataPolls; return true; }
    bool pollControlPlane() { return true; }
    void pollDirectYield(bool released) { if (direct && released) ++receipts; }
};
struct PoolChild {
    FakeCentralPort& port;
    blinker::GatewayChildSessionState s = blinker::GatewayChildSessionState::Stopped;
    unsigned connects = 0;
    uint32_t attempt = 0;
    explicit PoolChild(FakeCentralPort& value) : port(value) {}
    blinker::GatewayChildSessionState state() const { return s; }
    uint32_t attemptId() const { return attempt; }
    bool secure() const { return s == blinker::GatewayChildSessionState::Secure; }
    blinker::Result start() { auto result = port.start(); if (result) s = blinker::GatewayChildSessionState::Idle; return result; }
    blinker::Result connect() {
        ++connects; blinker::BleCentralConnectRequest request; request.attemptId = ++attempt;
        request.matcher = [](void*, const blinker::BleCentralAdvertisement&) { return false; };
        auto result = port.connect(request); if (result) s = blinker::GatewayChildSessionState::Connecting; return result;
    }
    blinker::Result disconnect() { s = blinker::GatewayChildSessionState::Idle; port.stateValue = blinker::BleCentralPortState::Idle; return blinker::Result::success(); }
    void stop() { port.stop(); s = blinker::GatewayChildSessionState::Stopped; }
    void poll(uint32_t budget) { port.poll(budget); }
};
struct PoolRow {
    FakeCentralPort port;
    PoolBlob access, journal;
    blinker::GatewayChildRecords records;
    PoolChild child;
    blinker::BasicGatewayChildConnection<PoolChild> connection;
    PoolBridge bridge;
    PoolControl proof, revoke, renew;
    blinker::GatewayManagementControlMux management;
    typedef blinker::BasicGatewayGattRadio<PoolChild, FakeCentralPort, PoolBridge> Radio;
    Radio radio;
    PoolRow(blinker::GatewayGattBootGuard& boot, FakeClock& clock)
        : records(access, journal), child(port), connection(child), management(proof, revoke, renew),
          radio(child, port, connection, bridge, management, boot, clock) {}
};
struct PoolFixture {
    FakeClock clock;
    blinker::GatewayGattBootGuard boot;
    PoolRow a, b;
    typedef blinker::BasicGatewayGattPool<PoolRow::Radio, FakeCentralPort> Pool;
    Pool::Slot slots[2];
    Pool pool;
    blinker::NativeGattPermitJoinAdapter adapter;
    PoolFixture() : boot(clock), a(boot, clock), b(boot, clock),
        slots{{a.radio, a.port, a.records}, {b.radio, b.port, b.records}}, pool(slots, 2U, boot, clock), adapter(clock, pool) {
        pool.prepare(false); clock.monotonic += blinker::gateway::kGatewayExecutionMaximumLeaseMillis; pool.prepare(true);
    }
    void ready(PoolRow& row) {
        row.bridge.wanted = true; row.child.s = blinker::GatewayChildSessionState::Secure;
        row.child.attempt = 1; row.port.stateValue = blinker::BleCentralPortState::Ready;
    }
    void open() { CHECK_EQ(blinker::ErrorCode::WouldBlock, adapter.openWindow().code()); pool.prepare(true); adapter.poll(1000); CHECK_EQ(blinker::GatewayPermitJoinAdapterState::Ready, adapter.state()); }
};

void testPoolBorrowIsolationAndFullCapacity() {
    using namespace blinker;
    PoolFixture f; f.ready(f.a); f.open();
    CHECK_EQ(BleCentralPortState::Scanning, f.b.port.state());
    const auto aPolls = f.a.bridge.dataPolls;
    f.pool.poll(true, 1000); CHECK_TRUE(f.a.bridge.dataPolls > aPolls); CHECK_TRUE(f.a.child.secure());
    f.b.port.asynchronousCancel = true; f.adapter.closeWindow(); f.adapter.poll(1000);
    CHECK_EQ(GatewayPermitJoinAdapterState::Closing, f.adapter.state());
    CHECK_TRUE(!f.b.radio.canLend());
    f.pool.releasePermitJoinPort(f.a.port); CHECK_TRUE(!f.b.radio.canLend()); // Wrong port cannot return B.
    f.b.port.cancelComplete = true; f.adapter.poll(1000);
    CHECK_EQ(GatewayPermitJoinAdapterState::Stopped, f.adapter.state()); CHECK_TRUE(f.a.child.secure());
    CHECK_EQ(0U, f.a.port.cancels); CHECK_EQ(0U, f.a.port.stops);
    // Production Link restores its own callbacks when the borrowed port returns.
    uint8_t rx[600]{}, tx[600]{}, packet[20]{};
    GattDirectChildLink link(f.b.port, f.clock, MutableByteSpan(rx, sizeof(rx)), MutableByteSpan(tx, sizeof(tx)), MutableByteSpan(packet, sizeof(packet)));
    CHECK_TRUE(link.start()); CHECK_TRUE(f.b.port.packetReceiver != nullptr); link.stop();

    PoolFixture full; full.ready(full.a); full.ready(full.b); full.pool.prepare(true);
    GatewayPermitJoinCoordinator permit(full.adapter, full.clock);
    const auto open = command(0x21, 1, gateway::GatewayPermitJoinAction::Open, 1100);
    CHECK_TRUE(permit.handleCommand(ByteView(open.bytes, open.size)));
    CHECK_EQ(gateway::GatewayPermitJoinStatus::Busy, resultOf(permit).status);
    CHECK_TRUE(full.a.child.secure() && full.b.child.secure());
    CHECK_EQ(0U, full.a.port.cancels + full.b.port.cancels);
}

void testPoolDiscoveryYieldAndRoundRobin() {
    using namespace blinker;
    PoolFixture f; f.a.bridge.direct = true; f.open(); // A cannot be borrowed; scan on B.
    CHECK_EQ(BleCentralPortState::Scanning, f.b.port.state());
    f.pool.poll(true, 1000); CHECK_TRUE(f.a.bridge.receipts > 0U); // B's activity is not A occupancy.
    f.a.bridge.direct = false; f.a.bridge.wanted = true;
    f.pool.prepare(true); f.clock.monotonic += 2000; f.pool.prepare(true); f.adapter.poll(1000);
    CHECK_EQ(BleCentralPortState::Idle, f.b.port.state());
    f.pool.poll(true, 1000); CHECK_EQ(1U, f.a.child.connects);
    CHECK_EQ(GatewayPermitJoinAdapterState::Ready, f.adapter.state()); // Same bounded window, not a new relay.
    f.adapter.closeWindow(); f.adapter.poll(1000);

    PoolFixture both; both.a.bridge.wanted = both.b.bridge.wanted = true;
    both.pool.prepare(true); both.pool.poll(true, 1000);
    CHECK_EQ(1U, both.a.child.connects); CHECK_EQ(0U, both.b.child.connects);
    both.a.port.stateValue = BleCentralPortState::Discovering;
    both.pool.prepare(true); both.pool.poll(true, 1000);
    CHECK_EQ(1U, both.b.child.connects); // A keeps discovery; only initiation was serialized.
    both.ready(both.a); both.ready(both.b); both.pool.poll(true, 1000);
    both.a.bridge.wanted = false; both.pool.poll(true, 1000);
    CHECK_TRUE(both.b.child.secure()); CHECK_EQ(0U, both.b.port.stops);
}

void testPoolBoundTargetPromise() {
    using namespace blinker;
    PoolFixture f;
    GatewayAccessRecord record;
    std::memcpy(record.childLogicalDeviceId, "child_a", 7); record.childLogicalDeviceIdSize = 7;
    std::memset(record.operationId, 1, sizeof(record.operationId));
    std::memset(record.childDeviceInstanceId, 2, sizeof(record.childDeviceInstanceId));
    std::memset(record.envelopeDigest, 3, sizeof(record.envelopeDigest));
    std::memset(record.accessMaterialDigest, 4, sizeof(record.accessMaterialDigest));
    std::memset(record.controllerId, 5, sizeof(record.controllerId));
    std::memset(record.secret, 6, sizeof(record.secret));
    std::memset(record.activePresence.key, 7, sizeof(record.activePresence.key));
    record.accessEpoch = 1; record.credentialVersion = 1; record.activePresence.version = 1;
    record.permissions = gateway::kGatewayPermissions; record.expiresAtUnixSeconds = 2000;
    GatewayAccessCommitOutcome outcome; CHECK_TRUE(f.a.records.access.commit(record, outcome));
    DeviceInstanceId instance; std::memcpy(instance.bytes, record.childDeviceInstanceId, sizeof(instance.bytes));
    uint8_t nonce[4] = {1}, locator[8]{}, serviceData[ble::kModeServiceDataSize]{};
    CHECK_TRUE(ble::derivePresenceLocator(ByteView(record.activePresence.key, sizeof(record.activePresence.key)), instance,
        record.accessEpoch, record.activePresence.version, ByteView(nonce, sizeof(nonce)), MutableByteSpan(locator, sizeof(locator))));
    const auto advertised = authorizedDirectAdvertisement(serviceData, locator);
    f.a.bridge.direct = true; f.open(); f.b.port.offer(advertised); f.adapter.poll(1000);
    CHECK_TRUE(f.adapter.selectRelayCandidate(ByteView(locator, sizeof(locator))));
    f.b.port.offer(advertised); f.adapter.poll(1000);
    CHECK_EQ(BleCentralPortState::Scanning, f.b.port.state()); // Free B does not bypass A's promise.
    f.a.bridge.direct = false;
    f.b.access.bytes.push_back(0); f.b.records.invalidate();
    f.b.port.offer(advertised); f.adapter.poll(1000);
    CHECK_EQ(BleCentralPortState::Scanning, f.b.port.state()); // Corrupt other binding cannot be treated as absent.
    CHECK_TRUE(f.b.records.access.clear());
    f.b.port.offer(advertised); f.adapter.poll(1000);
    CHECK_EQ(BleCentralPortState::Connecting, f.b.port.state());
    CHECK_TRUE(!f.a.radio.canLend()); // Target A reserved separately from physical B.
    f.a.bridge.wanted = true; f.pool.poll(true, 1000); CHECK_EQ(0U, f.a.child.connects);
    f.adapter.closeWindow(); f.adapter.poll(1000); f.pool.poll(true, 1000); CHECK_EQ(1U, f.a.child.connects);
    clearGatewayAccessRecord(record);
}

} // namespace

int main() {
    testPoolBorrowIsolationAndFullCapacity();
    testPoolDiscoveryYieldAndRoundRobin();
    testPoolBoundTargetPromise();
    testYieldAdmission();
    testCoordinator();
    testPendingOpenBudget();
    testPublishedReadyStillOwnsWindow();
    testNativeGattAdapter();
    testNativeAsynchronousHandoff();
    testCandidatesPreferLatestObservation();
    testSelectedDisconnectEndsWindow();
    testPermitJoinRelayCodec();
    std::cout << "gateway permit-join tests passed" << std::endl;
    return 0;
}
