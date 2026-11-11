#include <BlinkerV2/identity/GatewayAccessStore.h>
#include <BlinkerV2/runtime/GatewayChildRouteBridge.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

#define CHECK(condition) do { if (!(condition)) { \
    std::cerr << "Gateway route bridge check failed at line " << __LINE__ \
              << ": " << #condition << std::endl; std::exit(1); \
} } while (false)

class Blob final : public blinker::IAtomicBlobStore {
public:
    Blob() : bytes(), size(0U), present(false) {}
    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!present) return blinker::Result::failure(
            blinker::ErrorCode::NotFound);
        if (output.size < size) return blinker::Result::failure(
            blinker::ErrorCode::BufferTooSmall);
        std::memcpy(output.data, bytes, size);
        written = size;
        return blinker::Result::success();
    }
    blinker::Result replace(blinker::ByteView value) override {
        if (value.size > sizeof(bytes)) return blinker::Result::failure(
            blinker::ErrorCode::BufferTooSmall);
        std::memcpy(bytes, value.data, value.size);
        size = value.size;
        present = true;
        return blinker::Result::success();
    }
    blinker::Result clear() override {
        present = false;
        size = 0U;
        return blinker::Result::success();
    }
    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }
    uint8_t bytes[blinker::GatewayAccessStore::serializedSize];
    size_t size;
    bool present;
};

class Clock final : public blinker::IClock {
public:
    Clock() : now(1700000000ULL), millis(0U) {}
    uint32_t monotonicMillis() const override { return millis; }
    bool unixTime(uint64_t& output) const override {
        output = now;
        return true;
    }
    uint64_t now;
    uint32_t millis;
};

class Random final : public blinker::IRandom {
public:
    Random() : calls(0U) {}
    blinker::Result fill(blinker::MutableByteSpan output) override {
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = static_cast<uint8_t>(
                0x80U + calls * 0x10U + index);
        }
        ++calls;
        return blinker::Result::success();
    }
    uint8_t calls;
};

class Route {
public:
    Route()
        : receiver(nullptr), context(nullptr), online(true), generation(1U),
          published(), publishCalls(0U), failNextPublish(false) {}
    void setReceiver(
        blinker::GatewayRouteMessageReceiver value,
        void* receiverContext) {
        receiver = value;
        context = receiverContext;
    }
    bool subscribed() const { return online; }
    uint32_t subscriptionGeneration() const {
        return online ? generation : 0U;
    }
    blinker::Result publish(blinker::ByteView value) {
        if (value.size != 0U && value.data[0] == 0x8cU) {
            blinker::gateway::GatewayDirectYieldView control;
            CHECK(blinker::gateway::decodeGatewayDirectYield(value, control));
            CHECK(control.demand.operation == blinker::gateway::GatewayExecutionOperation::Challenge ||
                control.demand.operation == blinker::gateway::GatewayExecutionOperation::Yielded);
            directPublished.emplace_back(value.data, value.data + value.size);
            return blinker::Result::success();
        }
        if (value.size != 0U && value.data[0] == 0x8bU) {
            blinker::gateway::GatewayExecutionDemandView demand;
            CHECK(blinker::gateway::decodeGatewayExecutionDemand(value, demand));
            CHECK(demand.operation == blinker::gateway::GatewayExecutionOperation::Challenge);
            executionPublished.emplace_back(value.data, value.data + value.size);
            return blinker::Result::success();
        }
        ++publishCalls;
        published.assign(value.data, value.data + value.size);
        if (failNextPublish) {
            failNextPublish = false;
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        return blinker::Result::success();
    }
    blinker::Result deliver(blinker::ByteView value) {
        CHECK(receiver != nullptr);
        return receiver(context, value);
    }
    blinker::GatewayRouteMessageReceiver receiver;
    void* context;
    bool online;
    uint32_t generation;
    std::vector<uint8_t> published;
    unsigned publishCalls;
    bool failNextPublish;
    std::vector<std::vector<uint8_t> > executionPublished;
    std::vector<std::vector<uint8_t> > directPublished;
};

class Child {
public:
    Child()
        : current(blinker::GatewayChildSessionState::Idle), receiver(nullptr),
          receiverContext(nullptr), secureHandler(nullptr),
          disconnectedHandler(nullptr), sessionContext(nullptr),
          faultHandler(nullptr), faultContext(nullptr), startCalls(0U),
          connectCalls(0U),
          disconnectCalls(0U), sendCalls(0U), blockNextSend(false),
          attempt(1U), blockDisconnect(false), asyncDisconnect(false), sent() {}
    void setReceiver(
        blinker::GatewayChildFrameReceiver value,
        void* context) {
        receiver = value;
        receiverContext = context;
    }
    void setSessionHandlers(
        blinker::GatewayChildSessionHandler secureValue,
        blinker::GatewayChildSessionHandler disconnectedValue,
        void* context) {
        secureHandler = secureValue;
        disconnectedHandler = disconnectedValue;
        sessionContext = context;
    }
    void setFaultHandler(
        blinker::GatewayChildSessionFaultHandler value,
        void* context) {
        faultHandler = value;
        faultContext = context;
    }
    blinker::GatewayChildSessionState state() const { return current; }
    uint32_t attemptId() const { return attempt; }
    bool secure() const {
        return current == blinker::GatewayChildSessionState::Secure;
    }
    uint32_t negotiatedFeatures() const {
        return blinker::kChildSessionKnownFeatures;
    }
    uint32_t authenticatedPermissions() const {
        return secure() ? blinker::gateway::kGatewayPermissions : 0U;
    }
    uint32_t monotonicMillis() const { return 100U; }
    uint16_t negotiatedMaxFrameSize() const { return 512U; }
    uint8_t remoteReliableReceiveWindow() const { return 4U; }
    uint32_t manifestRevision() const { return 9U; }
    blinker::ByteView manifestFingerprint() const {
        return blinker::ByteView(fingerprint, sizeof(fingerprint));
    }
    blinker::Result start() {
        ++startCalls;
        current = blinker::GatewayChildSessionState::Idle;
        return blinker::Result::success();
    }
    blinker::Result connect() {
        ++connectCalls;
        ++attempt;
        current = blinker::GatewayChildSessionState::Connecting;
        return blinker::Result::success();
    }
    blinker::Result disconnect() {
        ++disconnectCalls;
        if (blockDisconnect) return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        current = asyncDisconnect ? blinker::GatewayChildSessionState::Disconnecting :
                                    blinker::GatewayChildSessionState::Idle;
        return blinker::Result::success();
    }
    blinker::Result sendFrame(blinker::ByteView frame) {
        if (!secure()) return blinker::Result::failure(
            blinker::ErrorCode::AuthenticationRequired);
        ++sendCalls;
        if (blockNextSend) {
            blockNextSend = false;
            return blinker::Result::failure(
                blinker::ErrorCode::WouldBlock);
        }
        sent.assign(frame.data, frame.data + frame.size);
        return blinker::Result::success();
    }
    void becomeSecure() {
        current = blinker::GatewayChildSessionState::Secure;
        if (secureHandler != nullptr) secureHandler(sessionContext);
    }
    void disconnectFromPeer() {
        current = blinker::GatewayChildSessionState::Idle;
        if (disconnectedHandler != nullptr) {
            disconnectedHandler(sessionContext);
        }
    }
    void receive(blinker::ByteView frame) {
        CHECK(receiver != nullptr);
        receiver(receiverContext, frame);
    }
    blinker::GatewayChildSessionState current;
    blinker::GatewayChildFrameReceiver receiver;
    void* receiverContext;
    blinker::GatewayChildSessionHandler secureHandler;
    blinker::GatewayChildSessionHandler disconnectedHandler;
    void* sessionContext;
    blinker::GatewayChildSessionFaultHandler faultHandler;
    void* faultContext;
    uint32_t startCalls;
    uint32_t connectCalls;
    uint32_t disconnectCalls;
    uint32_t sendCalls;
    bool blockNextSend;
    uint32_t attempt;
    bool blockDisconnect;
    bool asyncDisconnect;
    std::vector<uint8_t> sent;
    uint8_t fingerprint[blinker::bbp2::kManifestFingerprintSize] = {};
};

std::vector<uint8_t> frame(
    blinker::bbp2::MessageKind kind,
    uint8_t flags,
    uint16_t sequence = 1U,
    blinker::ByteView body = blinker::ByteView()) {
    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(kind);
    header.flags = flags;
    header.sequence = sequence;
    header.bodyLength = static_cast<uint16_t>(body.size);
    std::vector<uint8_t> output(
        blinker::bbp2::kBaseHeaderSize + body.size);
    CHECK(blinker::bbp2::encodeHeader(
        header, blinker::MutableByteSpan(output.data(), output.size())));
    if (!body.empty()) {
        std::memcpy(
            output.data() + blinker::bbp2::kBaseHeaderSize,
            body.data,
            body.size);
    }
    return output;
}

std::vector<uint8_t> ackFrame(uint16_t acknowledgedSequence) {
    blinker::bbp2::AckBody ack;
    ack.acknowledgedSequence = acknowledgedSequence;
    uint8_t encoded[32U];
    blinker::ByteView body;
    CHECK(blinker::bbp2::encodeAckBody(
        ack, blinker::MutableByteSpan(encoded, sizeof(encoded)), body));
    return frame(
        blinker::bbp2::MessageKind::Ack,
        blinker::bbp2::FlagIsResponse,
        9U,
        body);
}

struct ControlCapture {
    ControlCapture() : calls(0U), frame() {}
    static void receive(void* context, blinker::ByteView value) {
        ControlCapture* capture = static_cast<ControlCapture*>(context);
        CHECK(capture != nullptr);
        ++capture->calls;
        capture->frame.assign(value.data, value.data + value.size);
    }
    uint32_t calls;
    std::vector<uint8_t> frame;
};

struct ExecutorCapture {
    ExecutorCapture()
        : calls(0U), source(blinker::GatewayChildFrameSource::Event),
          received() {}
    static void receive(
        void* context,
        blinker::ByteView value,
        blinker::GatewayChildFrameSource source) {
        ExecutorCapture* capture =
            static_cast<ExecutorCapture*>(context);
        CHECK(capture != nullptr);
        ++capture->calls;
        capture->source = source;
        capture->received.assign(value.data, value.data + value.size);
    }
    uint32_t calls;
    blinker::GatewayChildFrameSource source;
    std::vector<uint8_t> received;
};

} // namespace

int main() {
    {
        // Cloud and future LAN adapters share one exact southbound lane.
        // Demand may be concurrent, but a second request cannot allocate a
        // sequence or replace the first owner's response callback.
        Child child;
        child.current = blinker::GatewayChildSessionState::Secure;
        blinker::BasicGatewayChildConnection<Child> connection(child);
        connection.allowAcquisition(true);
        blinker::BasicGatewayChildExecutor<Child> executor(child, connection);
        ExecutorCapture cloud;
        ExecutorCapture local;
        executor.setExecutionDemand(blinker::GatewayChildDemandCloud, true);
        executor.setExecutionDemand(blinker::GatewayChildDemandLocal, true);
        CHECK(executor.executionWanted());
        executor.setExecutionDemand(blinker::GatewayChildDemandCloud, false);
        CHECK(executor.executionWanted());

        const std::vector<uint8_t> command = frame(
            blinker::bbp2::MessageKind::Command,
            blinker::bbp2::FlagNone,
            77U);
        CHECK(executor.sendDataFrame(
            &cloud, &ExecutorCapture::receive,
            blinker::ByteView(command.data(), command.size())));
        blinker::bbp2::FrameView sent;
        CHECK(blinker::bbp2::parseFrame(
            blinker::ByteView(child.sent.data(), child.sent.size()), sent));
        CHECK(sent.header.sequence == 1U);
        const unsigned sends = child.sendCalls;
        CHECK(executor.sendDataFrame(
            &local, &ExecutorCapture::receive,
            blinker::ByteView(command.data(), command.size())).code() ==
              blinker::ErrorCode::StateConflict);
        CHECK(child.sendCalls == sends);

        const std::vector<uint8_t> response = ackFrame(1U);
        child.receive(blinker::ByteView(response.data(), response.size()));
        CHECK(cloud.calls == 1U && local.calls == 0U);
        CHECK(cloud.source ==
              blinker::GatewayChildFrameSource::PendingResponse);
        executor.completeDataRequest(&cloud, true);
        CHECK(executor.replayDataFrame(
            &cloud, blinker::ByteView(command.data(), command.size()), true));
        CHECK(blinker::bbp2::parseFrame(
            blinker::ByteView(child.sent.data(), child.sent.size()), sent));
        CHECK(sent.header.sequence == 1U);

        CHECK(executor.sendDataFrame(
            &local, &ExecutorCapture::receive,
            blinker::ByteView(command.data(), command.size())));
        CHECK(blinker::bbp2::parseFrame(
            blinker::ByteView(child.sent.data(), child.sent.size()), sent));
        CHECK(sent.header.sequence == 2U);
        executor.cancelDataOwner(&cloud);
        const std::vector<uint8_t> localResponse = ackFrame(2U);
        child.receive(blinker::ByteView(
            localResponse.data(), localResponse.size()));
        CHECK(local.calls == 1U && local.source ==
              blinker::GatewayChildFrameSource::PendingResponse);
        executor.completeDataRequest(&local, false);
        executor.setExecutionDemand(blinker::GatewayChildDemandLocal, false);
        CHECK(!executor.executionWanted());
    }

    Blob blob;
    blinker::GatewayAccessStore store(blob);
    blinker::GatewayAccessRecord record;
    const char childId[] = "ble_child_1";
    record.childLogicalDeviceIdSize = sizeof(childId) - 1U;
    std::memcpy(record.childLogicalDeviceId, childId, sizeof(childId) - 1U);
    std::memset(record.operationId, 0x11, sizeof(record.operationId));
    std::memset(record.childDeviceInstanceId, 0x22,
                sizeof(record.childDeviceInstanceId));
    std::memset(record.envelopeDigest, 0x33, sizeof(record.envelopeDigest));
    std::memset(record.accessMaterialDigest, 0x44,
                sizeof(record.accessMaterialDigest));
    std::memset(record.controllerId, 0x55, sizeof(record.controllerId));
    std::memset(record.secret, 0x66, sizeof(record.secret));
    std::memset(record.activePresence.key, 0x77,
                sizeof(record.activePresence.key));
    record.accessEpoch = 3U;
    record.credentialVersion = 4U;
    record.permissions = blinker::gateway::kGatewayPermissions;
    record.expiresAtUnixSeconds = 1700003600ULL;
    record.activePresence.version = 2U;
    blinker::GatewayAccessCommitOutcome committed;
    CHECK(store.commit(record, committed));
    blinker::clearGatewayAccessRecord(record);

    Clock clock;
    Random random;
    for (const auto state : {blinker::GatewayChildSessionState::Connecting,
                             blinker::GatewayChildSessionState::Authenticating}) {
        for (const auto kind : {blinker::bbp2::MessageKind::StateRequest, blinker::bbp2::MessageKind::Command}) {
            // The retired BBP projection cannot own a successor's native/auth
            // attempt, even before that attempt has published its new Hello.
            Clock time; Random entropy; Route wire; Child peripheral;
            blinker::BasicGatewayChildConnection<Child> owner(peripheral);
            owner.allowAcquisition(true);
            blinker::BasicGatewayChildExecutor<Child> executor(peripheral, owner);
            blinker::BasicGatewayChildRouteBridge<Child, Route> routed(executor, wire, store, time, entropy);
            CHECK(store.load(record));
            blinker::gateway::GatewayRouteAdmissionView grant;
            grant.operationId = blinker::ByteView(record.operationId, sizeof(record.operationId));
            grant.topologyVersion = 7U; grant.childLogicalDeviceId = blinker::StringView(childId);
            grant.childDeviceInstanceId = blinker::ByteView(record.childDeviceInstanceId, sizeof(record.childDeviceInstanceId));
            grant.accessEpoch = record.accessEpoch; grant.gatewayCredentialVersion = record.credentialVersion;
            grant.expiresAtUnixSeconds = record.expiresAtUnixSeconds;
            uint8_t bytes[512U]; size_t size = 0U;
            CHECK(blinker::gateway::encodeGatewayRouteAdmission(grant, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
            CHECK(wire.deliver(blinker::ByteView(bytes, size)));
            peripheral.becomeSecure();
            blinker::gateway::ChildRouteEnvelopeView message;
            CHECK(blinker::gateway::decodeChildRouteEnvelope(blinker::ByteView(wire.published.data(), wire.published.size()), message));
            const auto request = frame(kind, blinker::bbp2::FlagAckRequired, 7U);
            message.innerBbp2Frame = blinker::ByteView(request.data(), request.size());
            CHECK(blinker::gateway::encodeChildRouteEnvelope(message, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
            const std::vector<uint8_t> late(bytes, bytes + size);
            peripheral.disconnectFromPeer();
            CHECK(owner.ensureConnected().code() == blinker::ErrorCode::WouldBlock);
            peripheral.current = state;
            const auto attempt = peripheral.attemptId();
            CHECK(wire.deliver(blinker::ByteView(late.data(), late.size())).code() == blinker::ErrorCode::StateConflict);
            CHECK(peripheral.disconnectCalls == 0U && peripheral.sendCalls == 0U);
            CHECK(peripheral.state() == state && peripheral.attemptId() == attempt && !owner.closing());
            peripheral.becomeSecure();
            CHECK(wire.deliver(blinker::ByteView(late.data(), late.size())).code() == blinker::ErrorCode::StateConflict);
            CHECK(peripheral.secure() && peripheral.disconnectCalls == 0U && peripheral.sendCalls == 0U);
            CHECK(blinker::gateway::decodeChildRouteEnvelope(blinker::ByteView(wire.published.data(), wire.published.size()), message));
            message.innerBbp2Frame = blinker::ByteView(request.data(), request.size());
            CHECK(blinker::gateway::encodeChildRouteEnvelope(message, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
            CHECK(wire.deliver(blinker::ByteView(bytes, size)));
            CHECK(peripheral.sendCalls == 1U && peripheral.disconnectCalls == 0U);
            blinker::clearGatewayAccessRecord(record);
        }
    }
    {
        // Exact cleanup must observe completion, not merely accepted I/O.
        Child peripheral;
        Route wire;
        blinker::BasicGatewayChildConnection<Child> connection(peripheral);
        connection.allowAcquisition(true);
        blinker::BasicGatewayChildExecutor<Child> executor(peripheral, connection);
        blinker::BasicGatewayChildRouteBridge<Child, Route> bridge(executor, wire, store, clock, random);
        peripheral.current = blinker::GatewayChildSessionState::Secure;
        CHECK(executor.retireControllerControlSession(0U));
        CHECK(executor.retireControllerControlSession(peripheral.attemptId() + 1U));
        CHECK(peripheral.disconnectCalls == 0U);
        peripheral.blockDisconnect = true;
        CHECK(!executor.retireControllerControlSession(peripheral.attemptId()));
        CHECK(peripheral.disconnectCalls == 1U);
        peripheral.blockDisconnect = false;
        peripheral.asyncDisconnect = true;
        CHECK(!executor.retireControllerControlSession(peripheral.attemptId()));
        CHECK(!executor.retireControllerControlSession(peripheral.attemptId()));
        CHECK(peripheral.disconnectCalls == 2U);
        peripheral.current = blinker::GatewayChildSessionState::Idle;
        CHECK(executor.retireControllerControlSession(peripheral.attemptId()));
        const uint32_t oldAttempt = peripheral.attemptId();
        CHECK(peripheral.connect());
        CHECK(executor.retireControllerControlSession(oldAttempt));
        CHECK(peripheral.disconnectCalls == 2U);
        peripheral.asyncDisconnect = false;
        CHECK(executor.retireControllerControlSession(peripheral.attemptId()));
        CHECK(peripheral.disconnectCalls == 3U);
    }
    {
        // A secure flag cannot bypass another coordinator's pending retirement.
        Child peripheral;
        Route wire;
        ControlCapture control;
        blinker::BasicGatewayChildConnection<Child> connection(peripheral);
        connection.allowAcquisition(true);
        blinker::BasicGatewayChildExecutor<Child> executor(peripheral, connection);
        blinker::BasicGatewayChildRouteBridge<Child, Route> bridge(
            executor, wire, store, clock, random);
        executor.setControllerControlReceiver(&ControlCapture::receive, &control);
        peripheral.current = blinker::GatewayChildSessionState::Secure;
        peripheral.blockDisconnect = true;
        CHECK(!connection.retire(peripheral.attemptId()));
        uint8_t bodyBytes[32U] = {};
        blinker::ByteView body;
        CHECK(blinker::bbp2::encodeControllerControlOpenBody(
            blinker::MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body));
        CHECK(executor.sendControllerControlRequest(
            blinker::bbp2::MessageKind::ControllerControlOpen,
            body).code() == blinker::ErrorCode::WouldBlock);
        CHECK(peripheral.sent.empty());
        CHECK(peripheral.connectCalls == 0U);
        CHECK(wire.published.empty());
    }
    {
        // Real bridge: demand is independent of BBP, physical control and NVS.
        Clock time; Random entropy; Route wire; Child peripheral;
        blinker::BasicGatewayChildConnection<Child> connection(peripheral);
        connection.allowAcquisition(true);
        blinker::BasicGatewayChildExecutor<Child> executor(peripheral, connection);
        blinker::BasicGatewayChildRouteBridge<Child, Route> bridge(executor, wire, store, time, entropy);
        CHECK(store.load(record));
        blinker::gateway::GatewayRouteAdmissionView admission;
        admission.operationId = blinker::ByteView(record.operationId, sizeof(record.operationId));
        admission.topologyVersion = 7U; admission.childLogicalDeviceId = blinker::StringView(childId);
        admission.childDeviceInstanceId = blinker::ByteView(record.childDeviceInstanceId, sizeof(record.childDeviceInstanceId));
        admission.accessEpoch = record.accessEpoch; admission.gatewayCredentialVersion = record.credentialVersion;
        admission.expiresAtUnixSeconds = record.expiresAtUnixSeconds;
        uint8_t grant[200U], bytes[200U]; size_t grantSize = 0U, size = 0U;
        CHECK(blinker::gateway::encodeGatewayRouteAdmission(admission, blinker::MutableByteSpan(grant, sizeof(grant)), grantSize));
        CHECK(wire.deliver(blinker::ByteView(grant, grantSize))); bridge.poll();
        CHECK(wire.executionPublished.size() == 1U && entropy.calls == 1U);
        const std::vector<uint8_t> first = wire.executionPublished.back();
        blinker::gateway::GatewayExecutionDemandView d;
        CHECK(blinker::gateway::decodeGatewayExecutionDemand(blinker::ByteView(first.data(), first.size()), d));
        CHECK(d.challengeId == 1U && !bridge.executionWanted());
        {
            // Real route decoder + shared route incarnation + bounded sender.
            blinker::gateway::GatewayDirectYieldView direct; direct.demand = d;
            direct.controlId = 1U; direct.demand.operation = blinker::gateway::GatewayExecutionOperation::Probe;
            direct.demand.challengeId = direct.demand.leaseMillis = 0U;
            auto deliverDirect = [&]() {
                CHECK(blinker::gateway::encodeGatewayDirectYield(direct, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
                (void)wire.deliver(blinker::ByteView(bytes, size));
            };
            ++direct.demand.accessEpoch; deliverDirect(); CHECK(bridge.lastError() == blinker::ErrorCode::StateConflict);
            --direct.demand.accessEpoch; deliverDirect(); bridge.pollDirectYield(false);
            CHECK(wire.directPublished.size() == 1U && !bridge.directYieldHolding());
            const auto directChallenge = wire.directPublished.back();
            CHECK(blinker::gateway::decodeGatewayDirectYield(blinker::ByteView(directChallenge.data(), directChallenge.size()), direct));
            direct.demand.operation = blinker::gateway::GatewayExecutionOperation::Decision; direct.demand.leaseMillis = 30000U;
            deliverDirect(); CHECK(bridge.directYieldHolding() && !bridge.executionWanted());
            bridge.pollDirectYield(false); CHECK(wire.directPublished.size() == 1U);
            connection.allowAcquisition(false); bridge.pollDirectYield(true);
            CHECK(wire.directPublished.size() == 2U);
            const auto receipt = wire.directPublished.back(); blinker::gateway::GatewayDirectYieldView yielded;
            CHECK(blinker::gateway::decodeGatewayDirectYield(blinker::ByteView(receipt.data(), receipt.size()), yielded));
            CHECK(yielded.demand.operation == blinker::gateway::GatewayExecutionOperation::Yielded);
            CHECK(yielded.controlId == direct.controlId && yielded.demand.leaseMillis == 30000U);
            deliverDirect(); bridge.pollDirectYield(true); CHECK(wire.directPublished.size() == 2U); // Duplicate cannot reset retry budget.
            direct.demand.leaseMillis = 0U; deliverDirect();
            CHECK(!bridge.directYieldHolding() && !bridge.executionWanted());
            bridge.pollDirectYield(true); CHECK(wire.directPublished.size() == 2U);
            CHECK(peripheral.connectCalls == 0U && peripheral.disconnectCalls == 0U);
            connection.allowAcquisition(true);
        }
        const unsigned connects = peripheral.connectCalls, sends = peripheral.sendCalls;
        auto deliver = [&]() {
            CHECK(blinker::gateway::encodeGatewayExecutionDemand(d, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
            (void)wire.deliver(blinker::ByteView(bytes, size));
        };
        d.operation = blinker::gateway::GatewayExecutionOperation::Decision; d.leaseMillis = 30000U;
        time.millis = 200U; deliver(); CHECK(bridge.executionWanted());
        CHECK(wire.deliver(blinker::ByteView(grant, grantSize))); CHECK(entropy.calls == 1U); // Same authority does not reset nonce.
        ++d.accessEpoch; deliver(); CHECK(bridge.lastError() == blinker::ErrorCode::StateConflict); --d.accessEpoch;
        time.millis = 1000U; deliver(); CHECK(bridge.lastError() == blinker::ErrorCode::SequenceConflict);
        CHECK(peripheral.connectCalls == connects && peripheral.sendCalls == sends && peripheral.disconnectCalls == 0U);
        time.millis = 29999U; CHECK(bridge.executionWanted()); time.millis = 30000U; CHECK(!bridge.executionWanted());
        auto probe = [&]() {
            d.operation = blinker::gateway::GatewayExecutionOperation::Probe;
            d.incarnation = blinker::ByteView(); d.challengeId = d.leaseMillis = 0U;
            deliver(); bridge.poll();
        };
        time.millis = 31000U; probe();
        const std::vector<uint8_t> second = wire.executionPublished.back();
        CHECK(blinker::gateway::decodeGatewayExecutionDemand(blinker::ByteView(second.data(), second.size()), d));
        CHECK(d.challengeId == 2U);
        d.operation = blinker::gateway::GatewayExecutionOperation::Decision;
        deliver(); d.leaseMillis = 30000U; deliver(); CHECK(!bridge.executionWanted()); // Release overtakes grant.
        time.millis = 32000U; probe();
        const std::vector<uint8_t> third = wire.executionPublished.back();
        CHECK(blinker::gateway::decodeGatewayExecutionDemand(blinker::ByteView(third.data(), third.size()), d));
        d.operation = blinker::gateway::GatewayExecutionOperation::Decision; d.leaseMillis = 30000U;
        deliver(); CHECK(bridge.executionWanted());
        d.challengeId = 2U; d.leaseMillis = 0U; deliver(); CHECK(bridge.executionWanted());
        ++wire.generation; CHECK(!bridge.executionWanted()); bridge.poll(); CHECK(!bridge.admitted());
        CHECK(wire.deliver(blinker::ByteView(grant, grantSize))); bridge.poll(); CHECK(entropy.calls == 2U);
        d.challengeId = 1U; d.leaseMillis = 30000U; deliver(); CHECK(!bridge.executionWanted()); // Old incarnation.
        const size_t beforeRetries = wire.executionPublished.size();
        const auto exact = wire.executionPublished.back();
        for (unsigned attempt = 0U; attempt < 4U; ++attempt) {
            time.millis += 1000U; bridge.poll(); CHECK(wire.executionPublished.back() == exact);
        }
        time.millis += 6000U; bridge.poll();
        CHECK(wire.executionPublished.size() == beforeRetries + 4U); // Five attempts, never an infinite handshake.
        CHECK(wire.deliver(blinker::ByteView(grant, grantSize))); bridge.poll();
        CHECK(entropy.calls == 2U && wire.executionPublished.size() == beforeRetries + 5U);
        const auto retry = wire.executionPublished.back();
        CHECK(blinker::gateway::decodeGatewayExecutionDemand(blinker::ByteView(retry.data(), retry.size()), d));
        CHECK(d.challengeId == 2U); // First five lost: one fresh, bounded confirmation batch.
        for (unsigned attempt = 0U; attempt < 4U; ++attempt) {
            time.millis += 1000U;
            CHECK(wire.deliver(blinker::ByteView(grant, grantSize))); // Cannot re-anchor pending challenge.
            bridge.poll(); CHECK(wire.executionPublished.back() == retry);
        }
        time.millis += 1000U; bridge.poll();
        CHECK(wire.executionPublished.size() == beforeRetries + 9U);
        for (unsigned replay = 0U; replay < 8U; ++replay) {
            time.millis += 30000U;
            CHECK(wire.deliver(blinker::ByteView(grant, grantSize))); bridge.poll();
        }
        CHECK(wire.executionPublished.size() == beforeRetries + 9U); // No third batch or implicit background retry.
        // A later explicit source-owner Probe can recover confirmation; it
        // still cannot grant demand until a fresh exact Decision returns.
        time.millis += 1000U; probe();
        const auto resumed = wire.executionPublished.back();
        CHECK(blinker::gateway::decodeGatewayExecutionDemand(blinker::ByteView(resumed.data(), resumed.size()), d));
        CHECK(d.challengeId == 3U);
        d.operation = blinker::gateway::GatewayExecutionOperation::Decision;
        d.leaseMillis = 0U; deliver(); CHECK(!bridge.executionWanted());
        time.millis += 6000U;
        const size_t confirmed = wire.executionPublished.size();
        CHECK(wire.deliver(blinker::ByteView(grant, grantSize))); bridge.poll();
        CHECK(wire.executionPublished.size() == confirmed);
        blinker::gateway::GatewayDirectYieldView retired; retired.demand = d;
        retired.controlId = 1U; retired.demand.operation = blinker::gateway::GatewayExecutionOperation::Probe;
        retired.demand.challengeId = retired.demand.leaseMillis = 0U;
        auto directFrame = [&]() {
            CHECK(blinker::gateway::encodeGatewayDirectYield(retired, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
            (void)wire.deliver(blinker::ByteView(bytes, size));
        };
        directFrame(); bridge.pollDirectYield(false);
        const auto freshDirect = wire.directPublished.back();
        CHECK(blinker::gateway::decodeGatewayDirectYield(blinker::ByteView(freshDirect.data(), freshDirect.size()), retired));
        retired.demand.operation = blinker::gateway::GatewayExecutionOperation::Decision;
        retired.demand.leaseMillis = 10000U; directFrame(); bridge.pollDirectYield(true);
        CHECK(bridge.directYieldHolding());
        const auto publishedDirect = wire.directPublished.size();
        wire.online = false; bridge.poll(); CHECK(!bridge.admitted() && !bridge.executionWanted());
        CHECK(bridge.directYieldHolding()); bridge.reset(); CHECK(bridge.directYieldHolding());
        time.millis += 1000U; wire.online = true; ++wire.generation;
        CHECK(wire.deliver(blinker::ByteView(grant, grantSize))); bridge.poll();
        retired.demand.leaseMillis = 0U; directFrame();
        CHECK(bridge.lastError() == blinker::ErrorCode::StateConflict && bridge.directYieldHolding());
        bridge.pollDirectYield(true); CHECK(wire.directPublished.size() == publishedDirect);
        time.millis += 8999U; CHECK(bridge.directYieldHolding());
        ++time.millis; CHECK(!bridge.directYieldHolding());
    }
    {
        // Lease proofs are read-only Hub assertions, even with a busy BBP lane.
        Route leaseRoute;
        Child leaseChild;
        blinker::BasicGatewayChildConnection<Child> leaseConnection(leaseChild);
        leaseConnection.allowAcquisition(true);
        blinker::BasicGatewayChildExecutor<Child> leaseExecutor(leaseChild, leaseConnection);
        blinker::BasicGatewayChildRouteBridge<Child, Route> leaseBridge(
            leaseExecutor, leaseRoute, store, clock, random);
        CHECK(store.load(record));
        blinker::gateway::GatewayRouteAdmissionView grant;
        grant.operationId = blinker::ByteView(record.operationId, sizeof(record.operationId));
        grant.topologyVersion = 7U;
        grant.childLogicalDeviceId = blinker::StringView(childId);
        grant.childDeviceInstanceId = blinker::ByteView(record.childDeviceInstanceId, sizeof(record.childDeviceInstanceId));
        grant.accessEpoch = record.accessEpoch;
        grant.gatewayCredentialVersion = record.credentialVersion;
        grant.expiresAtUnixSeconds = record.expiresAtUnixSeconds;
        uint8_t bytes[200U];
        size_t size = 0U;
        CHECK(blinker::gateway::encodeGatewayRouteAdmission(grant, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
        CHECK(leaseRoute.deliver(blinker::ByteView(bytes, size)));
        leaseChild.becomeSecure();
        blinker::gateway::GatewayRouteLeaseView lease;
        lease.route.topologyVersion = 7U;
        lease.route.childLogicalDeviceId = grant.childLogicalDeviceId;
        lease.route.childDeviceInstanceId = grant.childDeviceInstanceId;
        lease.route.accessEpoch = grant.accessEpoch;
        lease.route.gatewayCredentialVersion = grant.gatewayCredentialVersion;
        lease.route.generation = 1U;
        uint8_t nonce[8U]; std::memset(nonce, 0x33, sizeof(nonce));
        lease.challenge = blinker::ByteView(nonce, sizeof(nonce));
        CHECK(blinker::gateway::encodeGatewayRouteLease(lease, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
        const std::vector<uint8_t> challenge(bytes, bytes + size);
        const char golden[] = "8a010300076b626c655f6368696c645f3150222222222222222222222222222222220304480000000000000001483333333333333333";
        const char digits[] = "0123456789abcdef";
        CHECK(size * 2U == sizeof(golden) - 1U);
        for (size_t i = 0U; i < size; ++i) {
            CHECK(digits[bytes[i] >> 4U] == golden[i * 2U]);
            CHECK(digits[bytes[i] & 15U] == golden[i * 2U + 1U]);
        }
        CHECK(leaseRoute.deliver(blinker::ByteView(challenge.data(), challenge.size())));
        blinker::gateway::GatewayRouteLeaseView proof;
        CHECK(blinker::gateway::decodeGatewayRouteLease(blinker::ByteView(leaseRoute.published.data(), leaseRoute.published.size()), proof));
        CHECK(proof.route.acknowledgment && proof.route.generation == 1U);
        CHECK(std::memcmp(proof.challenge.data, nonce, sizeof(nonce)) == 0);
        const std::vector<uint8_t> exactProof = leaseRoute.published;
        CHECK(leaseRoute.deliver(blinker::ByteView(challenge.data(), challenge.size())));
        CHECK(leaseRoute.published == exactProof); // Lost proof: exact retry is safe.
        CHECK(leaseChild.sendCalls == 0U && leaseChild.connectCalls == 0U);
        const unsigned published = leaseRoute.publishCalls;
        ++lease.route.generation;
        CHECK(blinker::gateway::encodeGatewayRouteLease(lease, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
        CHECK(!leaseRoute.deliver(blinker::ByteView(bytes, size)));
        lease.route.generation = 1U;
        lease.route.acknowledgment = true;
        CHECK(blinker::gateway::encodeGatewayRouteLease(lease, blinker::MutableByteSpan(bytes, sizeof(bytes)), size));
        CHECK(!leaseRoute.deliver(blinker::ByteView(bytes, size))); // No reflection loop.
        leaseChild.disconnectFromPeer();
        CHECK(!leaseRoute.deliver(blinker::ByteView(challenge.data(), challenge.size())));
        CHECK(leaseRoute.publishCalls == published);
        CHECK(leaseChild.connectCalls == 0U); // A probe cannot wake disconnected BLE.
        leaseChild.becomeSecure();
        const unsigned afterHello = leaseRoute.publishCalls;
        CHECK(!leaseRoute.deliver(blinker::ByteView(challenge.data(), challenge.size()))); // Old generation.
        CHECK(leaseRoute.publishCalls == afterHello);
        blinker::GatewayAccessRecord unchanged;
        CHECK(store.load(unchanged));
        CHECK(std::memcmp(unchanged.secret, record.secret, sizeof(record.secret)) == 0);
        CHECK(unchanged.deliveryRevision == record.deliveryRevision);
        blinker::clearGatewayAccessRecord(unchanged);
        blinker::clearGatewayAccessRecord(record);
    }
    {
        // Revocation management remains able to reach the child after the
        // Broker retires route admission. This must not publish or reopen the
        // ordinary child data plane.
        Route managementRoute;
        Child managementChild;
        managementChild.current =
            blinker::GatewayChildSessionState::Stopped;
        blinker::BasicGatewayChildConnection<Child> managementConnection(managementChild);
        managementConnection.allowAcquisition(true);
        blinker::BasicGatewayChildExecutor<Child> managementExecutor(
            managementChild, managementConnection);
        blinker::BasicGatewayChildRouteBridge<Child, Route> managementBridge(
            managementExecutor, managementRoute, store, clock, random);
        ControlCapture managementControl;
        managementExecutor.setControllerControlReceiver(
            &ControlCapture::receive, &managementControl);
        uint8_t managementBodyBytes[32U] = {};
        blinker::ByteView managementBody;
        CHECK(blinker::bbp2::encodeControllerControlOpenBody(
            blinker::MutableByteSpan(
                managementBodyBytes, sizeof(managementBodyBytes)),
            managementBody));
        const blinker::Result connecting =
            managementExecutor.sendControllerControlRequest(
                blinker::bbp2::MessageKind::ControllerControlOpen,
                managementBody);
        CHECK(!connecting);
        CHECK(connecting.code() == blinker::ErrorCode::WouldBlock);
        CHECK(managementChild.startCalls == 1U);
        CHECK(managementChild.connectCalls == 1U);
        CHECK(managementRoute.published.empty());
        managementChild.becomeSecure();
        CHECK(managementRoute.published.empty());
        const std::vector<uint8_t> managementHello = frame(
            blinker::bbp2::MessageKind::Hello,
            blinker::bbp2::FlagNone,
            1U);
        managementChild.receive(blinker::ByteView(
            managementHello.data(), managementHello.size()));
        CHECK(managementChild.disconnectCalls == 0U);
        CHECK(managementRoute.published.empty());
        for (size_t attempt = 0U; attempt < 100U; ++attempt) {
            managementChild.blockNextSend = true;
            CHECK(managementExecutor.sendControllerControlRequest(
                blinker::bbp2::MessageKind::ControllerControlOpen,
                managementBody).code() == blinker::ErrorCode::WouldBlock);
        }
        CHECK(managementExecutor.sendControllerControlRequest(
            blinker::bbp2::MessageKind::ControllerControlOpen,
            managementBody));
        blinker::bbp2::FrameView managementRequest;
        CHECK(blinker::bbp2::parseFrame(
            blinker::ByteView(
                managementChild.sent.data(), managementChild.sent.size()),
            managementRequest));
        CHECK(managementRequest.header.flags == blinker::bbp2::FlagNone);
        CHECK(managementRequest.header.sequence == 1U);
    }
    Route route;
    Child child;
    child.current = blinker::GatewayChildSessionState::Stopped;
    blinker::BasicGatewayChildConnection<Child> connection(child);
    connection.allowAcquisition(true);
    blinker::BasicGatewayChildExecutor<Child> executor(child, connection);
    blinker::BasicGatewayChildRouteBridge<Child, Route> bridge(
        executor, route, store, clock, random);

    uint8_t operation[16];
    uint8_t instance[16];
    std::memset(operation, 0x11, sizeof(operation));
    std::memset(instance, 0x22, sizeof(instance));
    blinker::gateway::GatewayRouteAdmissionView admission;
    admission.operationId = blinker::ByteView(operation, sizeof(operation));
    admission.topologyVersion = 7U;
    admission.childLogicalDeviceId = blinker::StringView(childId);
    admission.childDeviceInstanceId = blinker::ByteView(instance, sizeof(instance));
    admission.accessEpoch = 3U;
    admission.gatewayCredentialVersion = 4U;
    admission.expiresAtUnixSeconds = 1700003600ULL;
    uint8_t encoded[blinker::gateway::kGatewayRouteAdmissionMaximumEncodedSize];
    size_t encodedSize = 0U;
    CHECK(blinker::gateway::encodeGatewayRouteAdmission(
        admission, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    const std::vector<uint8_t> admissionBytes(
        encoded, encoded + encodedSize);
    route.deliver(blinker::ByteView(encoded, encodedSize));
    CHECK(bridge.admitted());
    CHECK(bridge.poll()); // Valid admission never starts BLE by itself.
    CHECK(child.startCalls == 0U && child.connectCalls == 0U);
    CHECK(connection.ensureConnected().code() == blinker::ErrorCode::WouldBlock);
    CHECK(child.startCalls == 1U);
    CHECK(child.connectCalls == 1U);
    child.becomeSecure();
    blinker::gateway::ChildRouteEnvelopeView upstream;
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()),
        upstream));
    CHECK(upstream.topologyVersion == 7U);
    blinker::bbp2::FrameView cloudHello;
    CHECK(blinker::bbp2::parseFrame(upstream.innerBbp2Frame, cloudHello));
    CHECK(cloudHello.header.kind == static_cast<uint8_t>(
        blinker::bbp2::MessageKind::Hello));
    CHECK(cloudHello.header.flags == blinker::bbp2::FlagAckRequired);
    const uint8_t firstGeneration[8U] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U};
    CHECK(std::memcmp(upstream.correlationId.data, firstGeneration, sizeof(firstGeneration)) == 0);
    const std::vector<uint8_t> exactHello = route.published;
    for (uint32_t due : {1000U, 3000U, 7000U, 12000U, 13000U, 15000U, 19000U}) {
        const unsigned before = route.publishCalls;
        clock.millis = due;
        bridge.poll();
        CHECK(route.publishCalls == before + 1U);
        CHECK(route.published == exactHello);
        CHECK(route.deliver(blinker::ByteView(admissionBytes.data(), admissionBytes.size())));
        bridge.poll(); CHECK(route.publishCalls == before + 1U); // Neither round can be re-anchored.
    }
    clock.millis = 23000U;
    bridge.poll();
    const unsigned expiredCalls = route.publishCalls;
    clock.millis = 40000U;
    bridge.poll();
    CHECK(route.publishCalls == expiredCalls);
    CHECK(child.secure()); // Hello exhaustion does not erase binding or churn BLE.
    CHECK(route.deliver(blinker::ByteView(admissionBytes.data(), admissionBytes.size())));
    bridge.poll();
    CHECK(route.publishCalls == expiredCalls);
    CHECK(route.published == exactHello); // Admission duplicates cannot reset the exhausted local budget.

    const std::vector<uint8_t> manifestRequest = frame(
        blinker::bbp2::MessageKind::ManifestRequest,
        blinker::bbp2::FlagNone,
        7U);
    blinker::gateway::ChildRouteEnvelopeView downstream;
    downstream.topologyVersion = 7U;
    downstream.childLogicalDeviceId = blinker::StringView(childId);
    downstream.childDeviceInstanceId = blinker::ByteView(instance, sizeof(instance));
    downstream.accessEpoch = 3U;
    downstream.gatewayCredentialVersion = 4U;
    downstream.correlationId = upstream.correlationId;
    downstream.innerBbp2Frame = blinker::ByteView(
        manifestRequest.data(), manifestRequest.size());
    encodedSize = 0U;
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    route.deliver(blinker::ByteView(encoded, encodedSize));
    blinker::bbp2::FrameView childRequest;
    CHECK(blinker::bbp2::parseFrame(
        blinker::ByteView(child.sent.data(), child.sent.size()),
        childRequest));
    CHECK(childRequest.header.kind == static_cast<uint8_t>(
        blinker::bbp2::MessageKind::ManifestRequest));
    CHECK(childRequest.header.sequence == 1U);
    CHECK(child.sendCalls == 1U);

    // The bridge does not cache child responses. An exact Broker retry must
    // reach the child's BBP/2 replay cache so a previously lost ACK/response
    // can be emitted again.
    route.deliver(blinker::ByteView(encoded, encodedSize));
    CHECK(blinker::bbp2::parseFrame(
        blinker::ByteView(child.sent.data(), child.sent.size()),
        childRequest));
    CHECK(childRequest.header.sequence == 1U);
    CHECK(child.sendCalls == 2U);

    // A distinct, authorized State request competes for the same child lane.
    // Backpressure must retain the first exchange, not tear down DirectSecure.
    const std::vector<uint8_t> originalEnvelope(encoded, encoded + encodedSize);
    const std::vector<uint8_t> competingState = frame(
        blinker::bbp2::MessageKind::StateRequest, blinker::bbp2::FlagIdMode, 8U);
    downstream.innerBbp2Frame = blinker::ByteView(competingState.data(), competingState.size());
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    const unsigned disconnectsBeforeBusy = child.disconnectCalls;
    for (unsigned attempt = 0U; attempt < 100U; ++attempt) {
        CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)).code() == blinker::ErrorCode::WouldBlock);
    }
    CHECK(child.secure());
    CHECK(child.disconnectCalls == disconnectsBeforeBusy);
    CHECK(child.sendCalls == 2U);
    std::memcpy(encoded, originalEnvelope.data(), originalEnvelope.size());
    encodedSize = originalEnvelope.size();

    // A child may report state while a Broker request is in flight. It gets
    // its own correlation and must not consume the Manifest request slot.
    const std::vector<uint8_t> patch = frame(
        blinker::bbp2::MessageKind::Patch,
        blinker::bbp2::FlagNone,
        8U);
    const std::vector<uint8_t> requestCorrelation(
        upstream.correlationId.data,
        upstream.correlationId.data + upstream.correlationId.size);
    child.receive(blinker::ByteView(patch.data(), patch.size()));
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()),
        upstream));
    CHECK(std::memcmp(upstream.correlationId.data,
                      requestCorrelation.data(),
                      requestCorrelation.size()) != 0);

    // ACK/Error are terminal sideband frames. Forwarding one while another
    // request is pending must not create a second response wait or a deadlock.
    const std::vector<uint8_t> patchCorrelation(
        upstream.correlationId.data,
        upstream.correlationId.data + upstream.correlationId.size);
    const std::vector<uint8_t> patchAck = ackFrame(8U);
    downstream.correlationId = blinker::ByteView(
        patchCorrelation.data(), patchCorrelation.size());
    downstream.innerBbp2Frame = blinker::ByteView(
        patchAck.data(), patchAck.size());
    encodedSize = 0U;
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    route.deliver(blinker::ByteView(encoded, encodedSize));
    CHECK(child.sent == patchAck);

    const std::vector<uint8_t> manifest = frame(
        blinker::bbp2::MessageKind::Manifest,
        blinker::bbp2::FlagIsResponse,
        1U);
    child.receive(blinker::ByteView(manifest.data(), manifest.size()));
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()),
        upstream));
    CHECK(std::memcmp(upstream.correlationId.data,
                      requestCorrelation.data(),
                      requestCorrelation.size()) == 0);
    blinker::bbp2::FrameView cloudManifest;
    CHECK(blinker::bbp2::parseFrame(
        upstream.innerBbp2Frame, cloudManifest));
    CHECK(cloudManifest.header.sequence == 7U);

    // A fast cloud credential refresh can reconnect within one product poll.
    // The route generation, not an observed outer lifecycle state, must make
    // the still-secure child announce itself again.
    route.generation = 2U;
    route.published.clear();
    route.deliver(blinker::ByteView(
        admissionBytes.data(), admissionBytes.size()));
    CHECK(bridge.admitted());
    bridge.poll();
    CHECK(!route.published.empty());
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()),
        upstream));
    CHECK(blinker::bbp2::parseFrame(upstream.innerBbp2Frame, cloudHello));
    CHECK(cloudHello.header.kind == static_cast<uint8_t>(
        blinker::bbp2::MessageKind::Hello));

    // A cloud protocol session may restart while DirectSecure remains alive.
    // Reusing cloud sequence 7 must allocate a fresh child sequence instead
    // of colliding with the child's reliable-request replay window.
    const std::vector<uint8_t> command = frame(
        blinker::bbp2::MessageKind::Command,
        blinker::bbp2::FlagAckRequired,
        7U);
    const std::vector<uint8_t> commandCorrelation(
        upstream.correlationId.data,
        upstream.correlationId.data + upstream.correlationId.size);
    downstream.correlationId = blinker::ByteView(
        commandCorrelation.data(), commandCorrelation.size());
    downstream.innerBbp2Frame = blinker::ByteView(
        command.data(), command.size());
    encodedSize = 0U;
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    route.deliver(blinker::ByteView(encoded, encodedSize));
    CHECK(blinker::bbp2::parseFrame(
        blinker::ByteView(child.sent.data(), child.sent.size()),
        childRequest));
    CHECK(childRequest.header.sequence == 2U);
    route.deliver(blinker::ByteView(encoded, encodedSize));
    CHECK(blinker::bbp2::parseFrame(
        blinker::ByteView(child.sent.data(), child.sent.size()),
        childRequest));
    CHECK(childRequest.header.sequence == 2U);

    const std::vector<uint8_t> commandAck = ackFrame(2U);
    child.receive(blinker::ByteView(commandAck.data(), commandAck.size()));
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()),
        upstream));
    CHECK(std::memcmp(upstream.correlationId.data,
                      commandCorrelation.data(),
                      commandCorrelation.size()) == 0);
    blinker::bbp2::FrameView cloudAck;
    CHECK(blinker::bbp2::parseFrame(upstream.innerBbp2Frame, cloudAck));
    blinker::bbp2::AckBody cloudAckBody;
    CHECK(blinker::bbp2::decodeAckBody(cloudAck.body, cloudAckBody));
    CHECK(cloudAckBody.acknowledgedSequence == 7U);

    // Multiple exact cloud retries can already be queued before the first
    // child ACK returns. Late duplicate ACKs must keep the original cloud
    // sequence/correlation instead of leaking the child-local sequence.
    child.receive(blinker::ByteView(commandAck.data(), commandAck.size()));
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()),
        upstream));
    CHECK(std::memcmp(upstream.correlationId.data,
                      commandCorrelation.data(),
                      commandCorrelation.size()) == 0);
    CHECK(blinker::bbp2::parseFrame(upstream.innerBbp2Frame, cloudAck));
    CHECK(blinker::bbp2::decodeAckBody(cloudAck.body, cloudAckBody));
    CHECK(cloudAckBody.acknowledgedSequence == 7U);

    // If the first upstream ACK is lost, the Broker repeats the exact cloud
    // frame. Reuse the completed mapping so the child sees the same reliable
    // sequence and can replay its cached ACK without executing twice.
    route.deliver(blinker::ByteView(encoded, encodedSize));
    CHECK(blinker::bbp2::parseFrame(
        blinker::ByteView(child.sent.data(), child.sent.size()),
        childRequest));
    CHECK(childRequest.header.sequence == 2U);
    child.receive(blinker::ByteView(commandAck.data(), commandAck.size()));
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()),
        upstream));
    CHECK(blinker::bbp2::parseFrame(upstream.innerBbp2Frame, cloudAck));
    CHECK(blinker::bbp2::decodeAckBody(cloudAck.body, cloudAckBody));
    CHECK(cloudAckBody.acknowledgedSequence == 7U);

    // An ACK without a pending or recent sequence mapping is not an
    // autonomous child event and must never be assigned a random correlation.
    const std::vector<uint8_t> publishedBeforeUnknownAck = route.published;
    const std::vector<uint8_t> unknownAck = ackFrame(99U);
    child.receive(blinker::ByteView(unknownAck.data(), unknownAck.size()));
    CHECK(route.published == publishedBeforeUnknownAck);

    // Replaying the same admission in the same MQTT generation is idempotent.
    route.published.clear();
    route.deliver(blinker::ByteView(
        admissionBytes.data(), admissionBytes.size()));
    bridge.poll();
    CHECK(route.published.empty());

    // Controller management uses the same child sequence allocator, but the
    // frame never enters route/up and ordinary route/down cannot overlap it.
    ControlCapture control;
    executor.setControllerControlReceiver(
        &ControlCapture::receive, &control);
    uint8_t controlBodyBytes[32U] = {};
    blinker::ByteView controlBody;
    CHECK(blinker::bbp2::encodeControllerControlOpenBody(
        blinker::MutableByteSpan(
            controlBodyBytes, sizeof(controlBodyBytes)),
        controlBody));
    CHECK(executor.sendControllerControlRequest(
        blinker::bbp2::MessageKind::ControllerControlOpen,
        controlBody));
    CHECK(executor.controllerControlPending());
    blinker::bbp2::FrameView controlRequest;
    CHECK(blinker::bbp2::parseFrame(
        blinker::ByteView(child.sent.data(), child.sent.size()),
        controlRequest));
    CHECK(controlRequest.header.kind == static_cast<uint8_t>(
        blinker::bbp2::MessageKind::ControllerControlOpen));
    CHECK(controlRequest.header.flags == blinker::bbp2::FlagNone);
    CHECK(controlRequest.header.sequence == 3U);
    const std::vector<uint8_t> publishedBeforeControl = route.published;
    uint8_t controlNonce[blinker::bbp2::kControllerControlNonceSize];
    std::memset(controlNonce, 0xA1, sizeof(controlNonce));
    blinker::bbp2::ControllerControlChallengeBody challenge;
    challenge.controlNonce = blinker::ByteView(
        controlNonce, sizeof(controlNonce));
    CHECK(blinker::bbp2::encodeControllerControlChallengeBody(
        challenge,
        blinker::MutableByteSpan(
            controlBodyBytes, sizeof(controlBodyBytes)),
        controlBody));
    const std::vector<uint8_t> challengeFrame = frame(
        blinker::bbp2::MessageKind::ControllerControlChallenge,
        blinker::bbp2::FlagIsResponse,
        controlRequest.header.sequence,
        controlBody);
    child.receive(blinker::ByteView(
        challengeFrame.data(), challengeFrame.size()));
    CHECK(!executor.controllerControlPending());
    CHECK(control.calls == 1U);
    CHECK(control.frame == challengeFrame);
    CHECK(route.published == publishedBeforeControl);

    // Bearer queue pressure means the record was not enqueued. Preserve the
    // authenticated child session and admission; return WouldBlock so the
    // route's existing ingress slot can retry without allocating a sequence.
    const std::vector<uint8_t> retryableCommand = frame(
        blinker::bbp2::MessageKind::Command,
        blinker::bbp2::FlagAckRequired,
        12U);
    downstream.correlationId = blinker::ByteView(
        commandCorrelation.data(), commandCorrelation.size());
    downstream.innerBbp2Frame = blinker::ByteView(
        retryableCommand.data(), retryableCommand.size());
    encodedSize = 0U;
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)),
        encodedSize));
    const uint32_t disconnectsBeforePressure = child.disconnectCalls;
    for (size_t attempt = 0U; attempt < 100U; ++attempt) {
        child.blockNextSend = true;
        CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)).code() ==
              blinker::ErrorCode::WouldBlock);
    }
    CHECK(bridge.lastError() == blinker::ErrorCode::WouldBlock);
    CHECK(child.secure());
    CHECK(child.disconnectCalls == disconnectsBeforePressure);
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)));
    CHECK(bridge.lastError() == blinker::ErrorCode::Ok);
    CHECK(child.secure());
    CHECK(child.disconnectCalls == disconnectsBeforePressure);

    CHECK(blinker::bbp2::parseFrame(
        blinker::ByteView(child.sent.data(), child.sent.size()), childRequest));
    CHECK(childRequest.header.sequence == 4U);

    // Credential renewal is durable before the Service commits the matching
    // topology. A child frame in that transition must retire only the stale
    // cloud route: DirectSecure remains available to finish management work.
    blinker::GatewayAccessRecord renewed;
    CHECK(store.load(renewed));
    ++renewed.storageRevision;
    renewed.deliveryRevision = renewed.storageRevision;
    renewed.credentialVersion = 5U;
    std::memset(renewed.secret, 0x68, sizeof(renewed.secret));
    std::memset(renewed.accessMaterialDigest, 0x46,
                sizeof(renewed.accessMaterialDigest));
    uint8_t renewedBytes[blinker::GatewayAccessStore::serializedSize] = {};
    CHECK(blinker::GatewayAccessStore::encode(
        renewed,
        blinker::MutableByteSpan(renewedBytes, sizeof(renewedBytes))));
    CHECK(blob.replace(blinker::ByteView(
        renewedBytes, sizeof(renewedBytes))));
    blinker::clearGatewayAccessRecord(renewed);

    route.published.clear();
    const uint32_t disconnectsBeforeRenewal = child.disconnectCalls;
    child.receive(blinker::ByteView(patch.data(), patch.size()));
    CHECK(!bridge.admitted());
    CHECK(child.secure());
    CHECK(child.disconnectCalls == disconnectsBeforeRenewal);
    CHECK(route.published.empty());

    admission.topologyVersion = 8U;
    admission.gatewayCredentialVersion = 5U;
    encodedSize = 0U;
    CHECK(blinker::gateway::encodeGatewayRouteAdmission(
        admission, blinker::MutableByteSpan(encoded, sizeof(encoded)),
        encodedSize));
    route.deliver(blinker::ByteView(encoded, encodedSize));
    CHECK(bridge.admitted());
    CHECK(bridge.topologyVersion() == 8U);
    bridge.poll();
    CHECK(!route.published.empty());
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()),
        upstream));
    CHECK(upstream.topologyVersion == 8U);
    CHECK(upstream.gatewayCredentialVersion == 5U);
    CHECK(blinker::bbp2::parseFrame(upstream.innerBbp2Frame, cloudHello));
    CHECK(cloudHello.header.kind == static_cast<uint8_t>(
        blinker::bbp2::MessageKind::Hello));

    // Once admitted, a natural BLE disconnect is owned by the route bridge;
    // it reconnects without requiring a second topology-proof command.
    const std::vector<uint8_t> priorGeneration(
        upstream.correlationId.data,
        upstream.correlationId.data + upstream.correlationId.size);
    downstream.topologyVersion = 8U;
    downstream.gatewayCredentialVersion = 5U;
    downstream.correlationId = blinker::ByteView(
        priorGeneration.data(), priorGeneration.size());
    downstream.innerBbp2Frame = blinker::ByteView(command.data(), command.size());
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)));
    // Model acceptance/execution with no ACK before disconnect. Replaying the
    // old exact request in a new DirectSecure session must not send it again.
    const uint32_t previousConnectCalls = child.connectCalls;
    child.disconnectFromPeer();
    bridge.poll();
    CHECK(child.connectCalls == previousConnectCalls);
    CHECK(connection.ensureConnected().code() == blinker::ErrorCode::WouldBlock);
    CHECK(child.connectCalls == previousConnectCalls + 1U);
    child.becomeSecure();
    downstream.topologyVersion = 8U;
    downstream.gatewayCredentialVersion = 5U;
    downstream.correlationId = blinker::ByteView(
        priorGeneration.data(), priorGeneration.size());
    downstream.innerBbp2Frame = blinker::ByteView(command.data(), command.size());
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    const uint32_t sendsBeforeOldAction = child.sendCalls;
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)).code() ==
          blinker::ErrorCode::StateConflict);
    CHECK(child.sendCalls == sendsBeforeOldAction);
    CHECK(child.secure());
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()), upstream));
    CHECK(std::memcmp(upstream.correlationId.data, priorGeneration.data(), 8U) > 0);
    const std::vector<uint8_t> currentCorrelation(
        upstream.correlationId.data,
        upstream.correlationId.data + upstream.correlationId.size);

    // A queued route frame must revalidate admission, even when a fast MQTT
    // reconnect or durable replacement precedes the bridge's next poll.
    downstream.correlationId = blinker::ByteView(
        currentCorrelation.data(), currentCorrelation.size());
    downstream.topologyVersion = 8U;
    downstream.gatewayCredentialVersion = 5U;
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    std::vector<uint8_t> queuedFrame(encoded, encoded + encodedSize);
    const uint32_t sendsBeforeFence = child.sendCalls;
    ++route.generation;
    route.published.clear();
    child.receive(blinker::ByteView(patch.data(), patch.size()));
    CHECK(route.published.empty());
    CHECK(route.deliver(blinker::ByteView(queuedFrame.data(), queuedFrame.size())).code() ==
          blinker::ErrorCode::AuthenticationRequired);
    CHECK(child.sendCalls == sendsBeforeFence);

    CHECK(blinker::gateway::encodeGatewayRouteAdmission(
        admission, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)));
    bridge.poll();
    child.becomeSecure();
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()), upstream));
    downstream.correlationId = upstream.correlationId;
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    queuedFrame.assign(encoded, encoded + encodedSize);
    child.blockNextSend = true;
    CHECK(route.deliver(blinker::ByteView(queuedFrame.data(), queuedFrame.size())).code() ==
          blinker::ErrorCode::WouldBlock);
    const uint32_t sendsBeforeReplacement = child.sendCalls;
    CHECK(store.load(renewed));
    ++renewed.storageRevision;
    renewed.deliveryRevision = renewed.storageRevision;
    CHECK(blinker::GatewayAccessStore::encode(
        renewed, blinker::MutableByteSpan(renewedBytes, sizeof(renewedBytes))));
    CHECK(blob.replace(blinker::ByteView(renewedBytes, sizeof(renewedBytes))));
    blinker::clearGatewayAccessRecord(renewed);
    CHECK(route.deliver(blinker::ByteView(queuedFrame.data(), queuedFrame.size())).code() ==
          blinker::ErrorCode::AuthenticationRequired);
    CHECK(child.sendCalls == sendsBeforeReplacement);

    // Resetting this bridge does not create a new Broker MQTT context. Its
    // generation allocator must not roll back even though request state clears.
    const std::vector<uint8_t> beforeReset(
        upstream.correlationId.data, upstream.correlationId.data + 8U);
    bridge.reset();
    CHECK(blinker::gateway::encodeGatewayRouteAdmission(
        admission, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)));
    bridge.poll();
    child.becomeSecure();
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()), upstream));
    CHECK(std::memcmp(upstream.correlationId.data, beforeReset.data(), 8U) > 0);

    // Broker context loss is not BLE re-enrollment. Retire just the observed
    // projection once, including an accepted Action whose outcome is unknown.
    uint64_t retired = 0U;
    for (size_t i = 0U; i < 8U; ++i) retired = (retired << 8U) | upstream.correlationId.data[i];
    downstream.correlationId = upstream.correlationId;
    downstream.innerBbp2Frame = blinker::ByteView(command.data(), command.size());
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    const std::vector<uint8_t> oldAction(encoded, encoded + encodedSize);
    CHECK(route.deliver(blinker::ByteView(oldAction.data(), oldAction.size())));
    const unsigned sendsBeforeRecovery = child.sendCalls;
    const unsigned disconnectsBeforeRecovery = child.disconnectCalls;
    admission.retiredGeneration = retired;
    CHECK(blinker::gateway::encodeGatewayRouteAdmission(
        admission, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    const std::vector<uint8_t> recoveryAdmission(encoded, encoded + encodedSize);
    CHECK(route.deliver(blinker::ByteView(recoveryAdmission.data(), recoveryAdmission.size())));
    route.failNextPublish = true;
    clock.millis = UINT32_MAX - 500U;
    bridge.poll();
    CHECK(bridge.lastError() == blinker::ErrorCode::WouldBlock);
    CHECK(child.secure());
    const std::vector<uint8_t> recoveryHello = route.published;
    clock.millis = 499U; // wrap-safe 1000 ms later
    bridge.poll();
    CHECK(route.published == recoveryHello);
    CHECK(bridge.lastError() == blinker::ErrorCode::Ok);
    CHECK(route.deliver(blinker::ByteView(oldAction.data(), oldAction.size())).code() ==
          blinker::ErrorCode::StateConflict);
    CHECK(child.sendCalls == sendsBeforeRecovery);
    const unsigned beforeDuplicate = route.publishCalls;
    CHECK(route.deliver(blinker::ByteView(recoveryAdmission.data(), recoveryAdmission.size())));
    bridge.poll();
    CHECK(route.publishCalls == beforeDuplicate);
    CHECK(child.disconnectCalls == disconnectsBeforeRecovery);
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()), upstream));
    uint64_t recovered = 0U;
    for (size_t i = 0U; i < 8U; ++i) recovered = (recovered << 8U) | upstream.correlationId.data[i];
    CHECK(recovered == retired + 1U);
    downstream.correlationId = upstream.correlationId;
    downstream.innerBbp2Frame = blinker::ByteView(manifestRequest.data(), manifestRequest.size());
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)));
    clock.millis += 12000U;
    bridge.poll();
    CHECK(route.publishCalls == beforeDuplicate); // Valid downlink stopped Hello retries.
    CHECK(route.deliver(blinker::ByteView(recoveryAdmission.data(), recoveryAdmission.size())));
    bridge.poll();
    CHECK(route.publishCalls == beforeDuplicate);
    admission.retiredGeneration = recovered + 1U;
    CHECK(blinker::gateway::encodeGatewayRouteAdmission(
        admission, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)).code() == blinker::ErrorCode::StateConflict);
    CHECK(child.secure());

    // MQTT replacement preserves DirectSecure and its reliable replay cache.
    // Reusing child sequence 1 would turn a new Action into an old cached ACK.
    blinker::bbp2::FrameView beforeCloudReset;
    CHECK(blinker::bbp2::parseFrame(
        blinker::ByteView(child.sent.data(), child.sent.size()), beforeCloudReset));
    const uint16_t sequenceBeforeCloudReset = beforeCloudReset.header.sequence;
    const unsigned disconnectsBeforeCloudReset = child.disconnectCalls;
    ++route.generation;
    for (unsigned i = 0U; i < 100U; ++i) bridge.reset();
    CHECK(child.secure());
    admission.retiredGeneration = 0U;
    CHECK(blinker::gateway::encodeGatewayRouteAdmission(
        admission, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)));
    bridge.poll(); // No BLE disconnect/re-authentication occurred.
    CHECK(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(route.published.data(), route.published.size()), upstream));
    downstream.correlationId = upstream.correlationId;
    downstream.innerBbp2Frame = blinker::ByteView(command.data(), command.size());
    CHECK(blinker::gateway::encodeChildRouteEnvelope(
        downstream, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)));
    blinker::bbp2::FrameView afterCloudReset;
    CHECK(blinker::bbp2::parseFrame(
        blinker::ByteView(child.sent.data(), child.sent.size()), afterCloudReset));
    CHECK(afterCloudReset.header.sequence > sequenceBeforeCloudReset);
    CHECK(child.disconnectCalls == disconnectsBeforeCloudReset);

    // Link closure is loop-side, bounded, exact and independent of child BBP.
    const unsigned sendsBeforeClosure = child.sendCalls;
    const unsigned publishesBeforeClosure = route.publishCalls;
    const unsigned connectsBeforeClosure = child.connectCalls;
    const unsigned startsBeforeClosure = child.startCalls;
    clock.millis = UINT32_MAX - 500U;
    child.disconnectFromPeer();
    CHECK(route.publishCalls == publishesBeforeClosure);
    route.failNextPublish = true;
    CHECK(bridge.pollControlPlane()); // Radio belongs to permit-join, not this bridge.
    CHECK(bridge.lastError() == blinker::ErrorCode::WouldBlock);
    const std::vector<uint8_t> notice = route.published;
    blinker::gateway::GatewayRouteClosureView closure;
    CHECK(blinker::gateway::decodeGatewayRouteClosure(blinker::ByteView(notice.data(), notice.size()), closure));
    CHECK(!closure.acknowledgment);
    child.disconnectFromPeer(); // Cannot restart the retry timer.
    clock.millis = 498U; CHECK(bridge.pollControlPlane());
    CHECK(route.publishCalls == publishesBeforeClosure + 1U);
    clock.millis = 499U; CHECK(bridge.pollControlPlane());
    CHECK(route.publishCalls == publishesBeforeClosure + 2U);
    CHECK(route.published == notice);
    CHECK(child.sendCalls == sendsBeforeClosure);
    closure.acknowledgment = true;
    ++closure.accessEpoch;
    CHECK(blinker::gateway::encodeGatewayRouteClosure(closure, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    CHECK(route.deliver(blinker::ByteView(encoded, encodedSize)).code() == blinker::ErrorCode::StateConflict);
    --closure.accessEpoch;
    CHECK(blinker::gateway::encodeGatewayRouteClosure(closure, blinker::MutableByteSpan(encoded, sizeof(encoded)), encodedSize));
    const std::vector<uint8_t> receipt(encoded, encoded + encodedSize);
    CHECK(route.deliver(blinker::ByteView(receipt.data(), receipt.size())));
    clock.millis += 3000U; CHECK(bridge.pollControlPlane());
    CHECK(route.publishCalls == publishesBeforeClosure + 2U);
    CHECK(child.connectCalls == connectsBeforeClosure);
    CHECK(child.startCalls == startsBeforeClosure);
    CHECK(bridge.admitted());
    CHECK(bridge.poll()); // Lease returned: composition decides when to reconnect.
    CHECK(child.connectCalls == connectsBeforeClosure);
    CHECK(connection.ensureConnected().code() == blinker::ErrorCode::WouldBlock);
    CHECK(child.connectCalls == connectsBeforeClosure + 1U);
    child.becomeSecure();
    const unsigned newHelloPublished = route.publishCalls;
    CHECK(route.deliver(blinker::ByteView(receipt.data(), receipt.size())).code() == blinker::ErrorCode::StateConflict);
    CHECK(child.secure());
    CHECK(route.publishCalls == newHelloPublished);
    child.disconnectFromPeer();
    const uint32_t closureStarted = clock.millis;
    for (const uint32_t elapsed : {0U, 1000U, 3000U, 7000U, 11000U, 40000U}) {
        clock.millis = closureStarted + elapsed; bridge.poll();
    }
    CHECK(route.publishCalls == newHelloPublished + 4U);
    child.becomeSecure();
    child.disconnectFromPeer(); bridge.poll();
    const std::vector<uint8_t> superseded = route.published;
    child.becomeSecure(); // No waiting 11 s for a closure receipt.
    const unsigned afterFastReconnect = route.publishCalls;
    CHECK(blinker::gateway::decodeChildRouteEnvelope(blinker::ByteView(route.published.data(), route.published.size()), upstream));
    CHECK(blinker::gateway::decodeGatewayRouteClosure(blinker::ByteView(superseded.data(), superseded.size()), closure));
    uint64_t freshGeneration = 0U;
    for (size_t i = 0U; i < 8U; ++i) freshGeneration = (freshGeneration << 8U) | upstream.correlationId.data[i];
    CHECK(freshGeneration == closure.generation + 1U);
    bridge.poll(); CHECK(route.publishCalls == afterFastReconnect);
    child.disconnectFromPeer();
    ++route.generation; bridge.poll();
    CHECK(!bridge.admitted());
    CHECK(route.publishCalls == afterFastReconnect);

    std::cout << "Gateway child route bridge tests passed; bridge bytes=" << sizeof(bridge) << std::endl;
    return 0;
}
