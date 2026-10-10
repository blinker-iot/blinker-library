#include <BlinkerV2/runtime/GatewayRouteClient.h>
#include <BlinkerV2/runtime/GatewayGattChildContext.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <memory>

struct ProbeSerial {
    std::vector<std::string> lines;
    void println(const char* text) { lines.emplace_back(text); }
    void printf(const char* format, ...) {
        char text[128];
        va_list args;
        va_start(args, format);
        std::vsnprintf(text, sizeof(text), format, args);
        va_end(args);
        lines.emplace_back(text);
    }
} Serial;
#include "../internal/arduino/Esp32EdgeHubDualBoard/IngressCapacityProbe.h"

namespace {
using namespace blinker;

#define CHECK(value) do { if (!(value)) { \
    std::cerr << "Route client line " << __LINE__ << ": " << #value << '\n'; \
    std::exit(1); } } while (false)

class Clock final : public IClock {
public:
    uint32_t now = 0U;
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t& value) const override {
        value = 1700000000ULL;
        return true;
    }
};

class Mqtt final : public IMqttClient {
public:
    bool online = false;
    bool rejectSubscription = false;
    unsigned subscriptions = 0;
    std::vector<uint8_t> published;
    MqttMessageHandler handler = nullptr;
    void* context = nullptr;
    Result connect(const MqttConnectOptions&) override {
        online = true;
        return Result::success();
    }
    void disconnect() override { online = false; }
    bool connected() const override { return online; }
    Result subscribe(StringView, uint8_t) override {
        ++subscriptions;
        return Result(rejectSubscription ? ErrorCode::WouldBlock : ErrorCode::Ok);
    }
    Result publish(StringView, ByteView value, const MqttPublishOptions& options) override {
        CHECK(options.qos == 0U && !options.retained);
        published.assign(value.data, value.data + value.size);
        return Result::success();
    }
    void poll(uint32_t) override {}
    void setMessageHandler(MqttMessageHandler value, void* owner) override {
        handler = value;
        context = owner;
    }
    void deliver(ByteView payload, uint8_t qos = 0U, bool retained = false) {
        CHECK(handler != nullptr);
        MqttMessageInfo info;
        info.qos = qos;
        info.qosKnown = true;
        info.retained = retained;
        info.retainedKnown = true;
        handler(context, StringView("/edge-hub/edge-test/route/down"), payload, info);
    }
};

SessionCredentials credentials() {
    SessionCredentials value;
    value.broker = BrokerKind::Blinker;
    value.security = MqttSecurity::Tls;
    value.host = StringView("broker.test");
    value.port = 8883U;
    value.clientId = StringView("edge-test");
    value.username = StringView("test-user");
    value.password = StringView("test-password");
    value.publishTopic = StringView("/device/edge/up");
    value.subscribeTopic = StringView("/device/edge/down");
    return value;
}

class Receiver {
public:
    ErrorCode result = ErrorCode::WouldBlock;
    std::vector<std::vector<uint8_t> > received;
    const uint8_t* borrowed = nullptr;
    GatewayRouteClient* resetDuringDispatch = nullptr;
    static Result receive(void* context, ByteView value) {
        Receiver& self = *static_cast<Receiver*>(context);
        self.received.push_back(std::vector<uint8_t>(value.data, value.data + value.size));
        self.borrowed = value.data;
        if (self.resetDuringDispatch != nullptr) self.resetDuringDispatch->reset();
        return Result(self.result);
    }
};

template <typename Route = GatewayRouteEndpoint>
struct BasicFixture {
    Clock clock;
    Mqtt mqtt;
    MqttFrameTransport transport;
    GatewayCloudMux cloud;
    GatewayRouteClient client;
    Route route;
    Receiver receiver;
    static bool matchAny(void*, ByteView) { return true; }
    BasicFixture(GatewayRouteMessageMatcher match = &matchAny, void* context = nullptr)
        : transport(mqtt, clock), cloud(transport), client(cloud, clock), route(client, match, context) {
        CHECK(transport.configure(credentials()));
        route.setReceiver(&Receiver::receive, &receiver);
        CHECK(client.configure(credentials()));
        CHECK(transport.start());
        transport.poll(1000U);
        client.poll();
        CHECK(route.subscribed());
    }
    void reconnectWithoutRoutePoll() {
        const uint32_t previous = transport.connectionGeneration();
        transport.setNetworkAvailable(false);
        transport.setNetworkAvailable(true);
        transport.poll(1000U);
        CHECK(transport.state() == TransportState::Online);
        CHECK(transport.connectionGeneration() != previous);
    }
};
typedef BasicFixture<> Fixture;

void checkRouteBinding(ByteView bytes, uint32_t epoch, uint32_t version);

void exactHandoff() {
    Fixture f;
    uint8_t first[] = {1U, 2U, 3U};
    const std::vector<uint8_t> expected(first, first + sizeof(first));
    f.mqtt.deliver(ByteView(first, sizeof(first)));
    first[0] = 9U;
    f.client.poll();
    CHECK(f.route.hasPendingEnvelope());
    CHECK(f.route.lastError() == ErrorCode::WouldBlock);
    CHECK(f.receiver.received.back() == expected);
    f.mqtt.deliver(ByteView(first, sizeof(first)));
    f.client.poll();
    CHECK(f.receiver.received.size() == 2U);
    CHECK(f.receiver.received.back() == expected);
    f.receiver.result = ErrorCode::Ok;
    f.client.poll();
    CHECK(!f.route.hasPendingEnvelope());
    CHECK(f.route.lastError() == ErrorCode::Ok);
    CHECK(f.receiver.received.back() == expected);
    // White-box lifetime witness only: the receiver must never retain this
    // borrowed view in production. The owning route is still alive here.
    for (size_t i = 0; i < sizeof(first); ++i) CHECK(f.receiver.borrowed[i] == 0U);
    f.client.poll();
    CHECK(f.receiver.received.size() == 3U);
    f.receiver.result = ErrorCode::AuthenticationRequired;
    f.mqtt.deliver(ByteView(first, sizeof(first)));
    f.client.poll();
    CHECK(!f.route.hasPendingEnvelope());
    CHECK(f.route.lastError() == ErrorCode::AuthenticationRequired);
}

void generationFence() {
    Fixture f;
    const uint8_t value[] = {7U};
    f.mqtt.deliver(ByteView(value, sizeof(value)));
    f.reconnectWithoutRoutePoll();
    CHECK(!f.route.subscribed());
    f.mqtt.rejectSubscription = true;
    f.client.poll();
    CHECK(!f.route.hasPendingEnvelope());
    CHECK(f.receiver.received.empty());
    // Even a queued callback before re-subscription cannot refill this slot.
    f.mqtt.deliver(ByteView(value, sizeof(value)));
    CHECK(!f.route.hasPendingEnvelope());
    f.mqtt.rejectSubscription = false;
    f.client.poll();
    CHECK(f.route.subscribed());
    CHECK(f.receiver.received.empty());
    f.mqtt.deliver(ByteView(value, sizeof(value)));
    CHECK(f.route.hasPendingEnvelope());
    f.transport.setNetworkAvailable(false);
    f.client.poll();
    CHECK(!f.route.hasPendingEnvelope());

    Fixture fast;
    fast.reconnectWithoutRoutePoll();
    fast.mqtt.deliver(ByteView(value, sizeof(value)));
    CHECK(!fast.route.hasPendingEnvelope());
    fast.client.poll();
    CHECK(fast.receiver.received.empty());
}

void ownerAndInputFence() {
    Fixture f;
    const uint8_t value[] = {3U};
    f.mqtt.deliver(ByteView(value, sizeof(value)), 1U);
    f.mqtt.deliver(ByteView(value, sizeof(value)), 0U, true);
    f.mqtt.deliver(ByteView());
    const std::vector<uint8_t> oversized(GatewayRouteClient::maximumEnvelopeSize + 1U, 1U);
    f.mqtt.deliver(ByteView(oversized.data(), oversized.size()));
    CHECK(!f.route.hasPendingEnvelope());
    f.mqtt.deliver(ByteView(value, sizeof(value)));
    // Reinstalling the same handler is not a new owner or a retry reset.
    f.route.setReceiver(&Receiver::receive, &f.receiver);
    CHECK(f.route.hasPendingEnvelope());
    f.route.setReceiver(nullptr, nullptr);
    CHECK(!f.route.hasPendingEnvelope());
    f.mqtt.deliver(ByteView(value, sizeof(value)));
    f.client.poll();
    CHECK(!f.route.hasPendingEnvelope());
    CHECK(f.route.lastError() == ErrorCode::Ok);
    f.route.setReceiver(&Receiver::receive, &f.receiver);
    f.mqtt.deliver(ByteView(value, sizeof(value)));
    f.receiver.resetDuringDispatch = &f.client;
    f.client.poll();
    CHECK(!f.route.hasPendingEnvelope());
    CHECK(!f.client.configured());
    CHECK(f.route.lastError() == ErrorCode::Ok);
    CHECK(f.client.configure(credentials()));
    f.client.poll();
    CHECK(!f.route.hasPendingEnvelope());
}

void boundedWait() {
    for (const uint32_t start : {0U, UINT32_MAX - 500U}) {
        Fixture f;
        const uint8_t first[] = {4U}, second[] = {5U};
        f.clock.now = start;
        f.mqtt.deliver(ByteView(first, sizeof(first)));
        f.client.poll();
        f.clock.now = start + GatewayRouteClient::maximumIngressWaitMillis - 1U;
        f.mqtt.deliver(ByteView(second, sizeof(second)));
        f.client.poll();
        CHECK(f.route.hasPendingEnvelope());
        CHECK(f.receiver.received.size() == 2U);
        CHECK(f.receiver.received.back()[0] == first[0]);
        ++f.clock.now;
        f.receiver.result = ErrorCode::Ok;
        f.client.poll();
        CHECK(!f.route.hasPendingEnvelope());
        CHECK(f.route.lastError() == ErrorCode::WouldBlock);
        CHECK(f.receiver.received.size() == 2U);
        CHECK(f.receiver.borrowed[0] == 0U);
        f.mqtt.deliver(ByteView(second, sizeof(second)));
        f.client.poll();
        CHECK(f.receiver.received.size() == 3U);
        CHECK(f.receiver.received.back()[0] == second[0]);
    }
}

void capacityProbeDoesNotExecuteExpiredCommand() {
    BasicFixture<IngressCapacityProbe> f;
    f.receiver.result = ErrorCode::Ok;
    uint8_t identity[16]; std::memset(identity, 1, sizeof(identity));
    uint8_t frame[bbp2::kBaseHeaderSize] = {}, encoded[256] = {};
    bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(bbp2::MessageKind::Command);
    header.sequence = 5;
    CHECK(bbp2::encodeHeader(header, MutableByteSpan(frame, sizeof(frame))));
    gateway::ChildRouteEnvelopeView envelope;
    envelope.topologyVersion = 1;
    envelope.childLogicalDeviceId = StringView("ble_child");
    envelope.childDeviceInstanceId = ByteView(identity, sizeof(identity));
    envelope.accessEpoch = 1;
    envelope.gatewayCredentialVersion = 1;
    envelope.correlationId = ByteView(identity, sizeof(identity));
    envelope.innerBbp2Frame = ByteView(frame, sizeof(frame));
    size_t size = 0;
    CHECK(gateway::encodeChildRouteEnvelope(envelope, MutableByteSpan(encoded, sizeof(encoded)), size));
    checkRouteBinding(ByteView(encoded, size), 1, 1);
    f.route.arm();
    for (const uint32_t at : {0U, 1750U}) {
        f.clock.now = at;
        f.mqtt.deliver(ByteView(encoded, size)); f.client.poll(); f.route.observe();
        CHECK(f.route.hasPendingEnvelope());
        f.clock.now = at + 999U; f.client.poll(); f.route.observe();
        CHECK(f.route.hasPendingEnvelope());
        f.clock.now = at + 1000U; f.client.poll(); f.route.observe();
        CHECK(!f.route.hasPendingEnvelope());
        CHECK(f.receiver.received.empty());
    }
    // Non-Commands keep flowing during the fault, including State requests.
    const uint8_t passthrough[] = {1};
    f.mqtt.deliver(ByteView(passthrough, sizeof(passthrough))); f.client.poll(); f.route.observe();
    CHECK(f.receiver.received.size() == 1);
    f.clock.now = 6000; f.client.poll(); f.route.observe();
    CHECK(Serial.lines.back() == "ingress=released count=2\n");
    CHECK(f.route.subscribed());
    f.mqtt.deliver(ByteView(encoded, size)); f.client.poll(); f.route.observe();
    CHECK(f.receiver.received.size() == 2);
    CHECK(!f.route.hasPendingEnvelope());
}

void closurePublishProfile() {
    Fixture f;
    uint8_t instance[16]; std::memset(instance, 1, sizeof(instance));
    gateway::GatewayRouteClosureView closure;
    closure.topologyVersion = 1;
    closure.childLogicalDeviceId = StringView("ble_child");
    closure.childDeviceInstanceId = ByteView(instance, sizeof(instance));
    closure.accessEpoch = 2; closure.gatewayCredentialVersion = 3; closure.generation = 4;
    uint8_t encoded[128]; size_t size = 0;
    CHECK(gateway::encodeGatewayRouteClosure(closure, MutableByteSpan(encoded, sizeof(encoded)), size));
    checkRouteBinding(ByteView(encoded, size), 2, 3);
    CHECK(f.route.publish(ByteView(encoded, size)));
    closure.acknowledgment = true;
    CHECK(gateway::encodeGatewayRouteClosure(closure, MutableByteSpan(encoded, sizeof(encoded)), size));
    CHECK(f.route.publish(ByteView(encoded, size)).code() == ErrorCode::InvalidArgument);
    CHECK(!f.route.publish(ByteView(encoded, size - 1)));
}

void leasePublishProfile() {
    // Exercise the real RouteClient -> CloudMux -> MQTT path; a fake Route in
    // bridge tests alone cannot detect a missing outbound profile whitelist.
    Fixture f;
    uint8_t instance[16]; std::memset(instance, 1, sizeof(instance));
    uint8_t nonce[8]; std::memset(nonce, 2, sizeof(nonce));
    gateway::GatewayRouteLeaseView lease;
    lease.route.topologyVersion = 1;
    lease.route.childLogicalDeviceId = StringView("ble_child");
    lease.route.childDeviceInstanceId = ByteView(instance, sizeof(instance));
    lease.route.accessEpoch = 2; lease.route.gatewayCredentialVersion = 3; lease.route.generation = 4;
    lease.challenge = ByteView(nonce, sizeof(nonce));
    uint8_t encoded[128]; size_t size = 0;
    CHECK(gateway::encodeGatewayRouteLease(lease, MutableByteSpan(encoded, sizeof(encoded)), size));
    checkRouteBinding(ByteView(encoded, size), 2, 3);
    CHECK(f.route.publish(ByteView(encoded, size)).code() == ErrorCode::InvalidArgument);
    CHECK(f.mqtt.published.empty()); // Challenge is Broker -> Hub only.
    lease.route.acknowledgment = true;
    CHECK(gateway::encodeGatewayRouteLease(lease, MutableByteSpan(encoded, sizeof(encoded)), size));
    CHECK(f.route.publish(ByteView(encoded, size)));
    CHECK(f.mqtt.published == std::vector<uint8_t>(encoded, encoded + size));
    f.mqtt.published.clear();
    CHECK(!f.route.publish(ByteView(encoded, size - 1)));
    encoded[size] = 0;
    CHECK(!f.route.publish(ByteView(encoded, size + 1)));
    CHECK(f.mqtt.published.empty());
}
void executionPublishProfile() {
    Fixture f;
    uint8_t id[16]; std::memset(id, 1, sizeof(id));
    gateway::GatewayExecutionDemandView d;
    d.topologyVersion = 1; d.childLogicalDeviceId = StringView("ble_child");
    d.childDeviceInstanceId = ByteView(id, sizeof(id)); d.accessEpoch = 1; d.gatewayCredentialVersion = 1;
    uint8_t encoded[128]; size_t size = 0;
    CHECK(gateway::encodeGatewayExecutionDemand(d, MutableByteSpan(encoded, sizeof(encoded)), size));
    checkRouteBinding(ByteView(encoded, size), 1, 1);
    CHECK(!f.route.publish(ByteView(encoded, size)));
    d.operation = gateway::GatewayExecutionOperation::Challenge; d.incarnation = ByteView(id, sizeof(id)); d.challengeId = 1;
    CHECK(gateway::encodeGatewayExecutionDemand(d, MutableByteSpan(encoded, sizeof(encoded)), size));
    gateway::GatewayExecutionDemandView decoded;
    CHECK(gateway::decodeGatewayExecutionDemand(ByteView(encoded, size), decoded));
    CHECK(decoded.operation == gateway::GatewayExecutionOperation::Challenge);
    CHECK(f.route.publish(ByteView(encoded, size)));
    CHECK(f.mqtt.published == std::vector<uint8_t>(encoded, encoded + size));
    d.operation = gateway::GatewayExecutionOperation::Decision; d.leaseMillis = 30000;
    CHECK(gateway::encodeGatewayExecutionDemand(d, MutableByteSpan(encoded, sizeof(encoded)), size));
    f.mqtt.published.clear(); CHECK(!f.route.publish(ByteView(encoded, size))); CHECK(f.mqtt.published.empty());
}
void directYieldPublishProfile() {
    // Real RouteClient -> CloudMux -> MQTT, not the Bridge's permissive fake.
    Fixture f;
    uint8_t id[16]; std::memset(id, 1, sizeof(id));
    gateway::GatewayDirectYieldView control;
    auto& d = control.demand;
    d.topologyVersion = 1; d.childLogicalDeviceId = StringView("ble_child");
    d.childDeviceInstanceId = ByteView(id, sizeof(id)); d.accessEpoch = 1; d.gatewayCredentialVersion = 1;
    d.incarnation = ByteView(id, sizeof(id)); control.controlId = 1;
    uint8_t encoded[128]; size_t size = 0;
    for (const auto operation : {gateway::GatewayExecutionOperation::Probe, gateway::GatewayExecutionOperation::Challenge,
            gateway::GatewayExecutionOperation::Decision, gateway::GatewayExecutionOperation::Yielded}) {
        d.operation = operation;
        d.challengeId = operation == gateway::GatewayExecutionOperation::Probe ? 0 : 1;
        d.leaseMillis = (operation == gateway::GatewayExecutionOperation::Decision ||
            operation == gateway::GatewayExecutionOperation::Yielded) ? 30000 : 0;
        CHECK(gateway::encodeGatewayDirectYield(control, MutableByteSpan(encoded, sizeof(encoded)), size));
        checkRouteBinding(ByteView(encoded, size), 1, 1);
        f.mqtt.published.clear();
        const bool upstream = operation == gateway::GatewayExecutionOperation::Challenge ||
            operation == gateway::GatewayExecutionOperation::Yielded;
        CHECK(f.route.publish(ByteView(encoded, size)).ok() == upstream);
        CHECK(f.mqtt.published == (upstream ? std::vector<uint8_t>(encoded, encoded + size) : std::vector<uint8_t>()));
        f.mqtt.published.clear();
        CHECK(!f.route.publish(ByteView(encoded, size - 1)));
        encoded[size] = 0; CHECK(!f.route.publish(ByteView(encoded, size + 1)));
        CHECK(f.mqtt.published.empty());
    }
}

bool matchTag(void* context, ByteView bytes) {
    return bytes.data != nullptr && bytes.size != 0 && bytes.data[0] == *static_cast<uint8_t*>(context);
}

void independentChildIngress() {
    uint8_t tagA = 1, tagB = 2;
    Fixture f(&matchTag, &tagA);
    Receiver second;
    second.result = ErrorCode::Ok;
    {
        GatewayRouteEndpoint b(f.client, &matchTag, &tagB);
        b.setReceiver(&Receiver::receive, &second);
        uint8_t aBytes[] = {1, 5}, bBytes[] = {2, 6};
        f.mqtt.deliver(ByteView(aBytes, sizeof(aBytes)));
        f.mqtt.deliver(ByteView(bBytes, sizeof(bBytes)));
        f.client.poll();
        CHECK(f.route.hasPendingEnvelope()); CHECK(!b.hasPendingEnvelope());
        CHECK(second.received.size() == 1 && f.receiver.received.size() == 1);
        CHECK(second.received[0][0] == tagB); CHECK(f.receiver.received[0][0] == tagA);
        const unsigned subscriptions = f.mqtt.subscriptions;
        f.clock.now = 999;
        f.mqtt.deliver(ByteView(bBytes, sizeof(bBytes))); f.client.poll();
        CHECK(second.received.size() == 2 && f.route.hasPendingEnvelope());
        f.clock.now = 1000; f.client.poll(); CHECK(!f.route.hasPendingEnvelope());
        CHECK(f.route.lastError() == ErrorCode::WouldBlock);
        CHECK(f.mqtt.subscriptions == subscriptions);
        // Reset/rebind of A does not erase B's queued delivery.
        f.mqtt.deliver(ByteView(bBytes, sizeof(bBytes)));
        f.route.setReceiver(nullptr, nullptr); f.client.poll();
        CHECK(second.received.size() == 3);
    }
    // Destruction of B only detaches B; A and the shared subscription survive.
    f.route.setReceiver(&Receiver::receive, &f.receiver);
    f.receiver.result = ErrorCode::Ok;
    f.mqtt.deliver(ByteView(&tagB, 1)); f.mqtt.deliver(ByteView(&tagA, 1)); f.client.poll();
    CHECK(f.receiver.received.back()[0] == tagA);
    CHECK(f.route.subscribed());
}

void selectionAndSessionFences() {
    uint8_t tagA = 1, tagB = 2;
    Fixture f(&matchTag, &tagA);
    Receiver other;
    GatewayRouteEndpoint b(f.client, &matchTag, &tagB);
    b.setReceiver(&Receiver::receive, &other);
    f.mqtt.deliver(ByteView(&tagA, 1)); f.mqtt.deliver(ByteView(&tagB, 1));
    f.reconnectWithoutRoutePoll(); f.client.poll();
    CHECK(!f.route.hasPendingEnvelope() && !b.hasPendingEnvelope());
    CHECK(f.receiver.received.empty() && other.received.empty());
    // A binding changed while a message waited; dispatch cannot target its successor.
    f.mqtt.deliver(ByteView(&tagA, 1)); tagA = 3; f.client.poll();
    CHECK(f.receiver.received.empty()); CHECK(f.route.lastError() == ErrorCode::StateConflict);
    uint8_t unknown = 9; f.mqtt.deliver(ByteView(&unknown, 1)); f.client.poll();
    CHECK(f.receiver.received.empty() && other.received.empty());
    {
        Receiver duplicateReceiver;
        GatewayRouteEndpoint duplicate(f.client, &matchTag, &tagB);
        duplicate.setReceiver(&Receiver::receive, &duplicateReceiver);
        f.mqtt.deliver(ByteView(&tagB, 1)); f.client.poll();
        CHECK(other.received.empty() && duplicateReceiver.received.empty()); // Not first match wins.
        CHECK(b.publish(ByteView(&tagB, 1)).code() == ErrorCode::StateConflict);
        CHECK(duplicate.publish(ByteView(&tagB, 1)).code() == ErrorCode::StateConflict);
        CHECK(f.mqtt.published.empty());
    }
    // Duplicating a selector after ingress also cannot consume the queued copy.
    f.mqtt.deliver(ByteView(&tagB, 1));
    {
        GatewayRouteEndpoint duplicate(f.client, &matchTag, &tagB);
        duplicate.setReceiver(&Receiver::receive, &other);
        f.client.poll(); CHECK(other.received.empty()); CHECK(!b.hasPendingEnvelope());
    }
    // Whole MQTT reset during A dispatch retires pending B as well.
    f.mqtt.deliver(ByteView(&tagA, 1)); f.mqtt.deliver(ByteView(&tagB, 1));
    other.resetDuringDispatch = &f.client;
    f.client.poll();
    CHECK(!f.client.configured() && !f.route.hasPendingEnvelope() && !b.hasPendingEnvelope());
    CHECK(f.receiver.received.empty());
}

void clientDestructionDetachesBorrowers() {
    Clock clock; Mqtt mqtt; MqttFrameTransport transport(mqtt, clock); GatewayCloudMux cloud(transport);
    std::unique_ptr<GatewayRouteClient> client(new GatewayRouteClient(cloud, clock));
    uint8_t tag = 1; Receiver receiver;
    GatewayRouteEndpoint endpoint(*client, &matchTag, &tag);
    endpoint.setReceiver(&Receiver::receive, &receiver);
    CHECK(transport.configure(credentials())); CHECK(client->configure(credentials()));
    CHECK(transport.start()); transport.poll(1000); client->poll();
    mqtt.deliver(ByteView(&tag, 1)); CHECK(endpoint.hasPendingEnvelope());
    client.reset();
    CHECK(!endpoint.hasPendingEnvelope() && !endpoint.subscribed());
    CHECK(endpoint.publish(ByteView(&tag, 1)).code() == ErrorCode::NotConnected);
}

class Blob final : public IAtomicBlobStore {
public:
    unsigned reads = 0U;
    std::vector<uint8_t> bytes;
    Result load(MutableByteSpan output, size_t& written) override {
        ++reads;
        written = 0;
        if (bytes.empty()) return Result::failure(ErrorCode::NotFound);
        if (output.size < bytes.size()) return Result::failure(ErrorCode::BufferTooSmall);
        std::memcpy(output.data, bytes.data(), bytes.size()); written = bytes.size(); return Result::success();
    }
    Result replace(ByteView value) override { bytes.assign(value.data, value.data + value.size); return Result::success(); }
    Result clear() override { bytes.clear(); return Result::success(); }
    StorageProtection protection() const override { return StorageProtection::EncryptedFlash; }
};

GatewayAccessRecord recordFor(const char* childId, uint8_t seed) {
    GatewayAccessRecord record;
    record.childLogicalDeviceIdSize = static_cast<uint8_t>(std::strlen(childId));
    std::memcpy(record.childLogicalDeviceId, childId, record.childLogicalDeviceIdSize);
    std::memset(record.operationId, seed, sizeof(record.operationId));
    std::memset(record.childDeviceInstanceId, seed, sizeof(record.childDeviceInstanceId));
    std::memset(record.envelopeDigest, seed, sizeof(record.envelopeDigest));
    std::memset(record.accessMaterialDigest, seed, sizeof(record.accessMaterialDigest));
    std::memset(record.controllerId, seed, sizeof(record.controllerId));
    std::memset(record.secret, seed, sizeof(record.secret));
    std::memset(record.activePresence.key, seed, sizeof(record.activePresence.key));
    record.accessEpoch = 3; record.credentialVersion = 4;
    record.permissions = gateway::kGatewayPermissions; record.expiresAtUnixSeconds = 1700003600ULL;
    record.activePresence.version = 2;
    return record;
}

void commit(GatewayAccessStore& store, const GatewayAccessRecord& record) {
    GatewayAccessCommitOutcome outcome; CHECK(store.commit(record, outcome));
}

void checkRouteBinding(ByteView bytes, uint32_t epoch, uint32_t version) {
    Blob blob, journal; GatewayChildRecords records(blob, journal); auto& access = records.access;
    auto record = recordFor("ble_child", 1); record.accessEpoch = epoch; record.credentialVersion = version;
    commit(access, record);
    CHECK(gatewayRouteMatchesChild(bytes, records));
    const unsigned reads = blob.reads + journal.reads;
    for (unsigned i = 0; i != 100; ++i) CHECK(gatewayRouteMatchesChild(bytes, records));
    CHECK(reads == blob.reads + journal.reads);
    CHECK(!gatewayRouteMatchesChild(ByteView(bytes.data, bytes.size - 1), records));
    std::vector<uint8_t> trailing(bytes.data, bytes.data + bytes.size); trailing.push_back(0);
    CHECK(!gatewayRouteMatchesChild(ByteView(trailing.data(), trailing.size()), records));
    CHECK(access.clear()); ++record.credentialVersion; commit(access, record);
    CHECK(!gatewayRouteMatchesChild(bytes, records));
    CHECK(access.clear()); CHECK(!gatewayRouteMatchesChild(bytes, records));
}

std::vector<uint8_t> admission(const GatewayAccessRecord& record) {
    gateway::GatewayRouteAdmissionView value;
    value.operationId = ByteView(record.operationId, sizeof(record.operationId));
    value.topologyVersion = 1;
    value.childLogicalDeviceId = StringView(record.childLogicalDeviceId, record.childLogicalDeviceIdSize);
    value.childDeviceInstanceId = ByteView(record.childDeviceInstanceId, sizeof(record.childDeviceInstanceId));
    value.accessEpoch = record.accessEpoch; value.gatewayCredentialVersion = record.credentialVersion;
    value.expiresAtUnixSeconds = record.expiresAtUnixSeconds;
    std::vector<uint8_t> bytes(256); size_t written = 0;
    CHECK(gateway::encodeGatewayRouteAdmission(value, MutableByteSpan(bytes.data(), bytes.size()), written));
    bytes.resize(written); return bytes;
}

class Port final : public IBleCentralPort {
public:
    unsigned starts = 0, connects = 0;
    Result start() override { ++starts; return Result::success(); }
    void stop() override {}
    void poll(uint32_t) override {}
    BleCentralPortState state() const override { return BleCentralPortState::Stopped; }
    ErrorCode lastError() const override { return ErrorCode::Ok; }
    Result connect(const BleCentralConnectRequest&) override { ++connects; return Result::failure(ErrorCode::NotConfigured); }
    Result cancel(uint32_t) override { return Result::success(); }
    Result connection(BleCentralConnectionInfo&) const override { return Result::failure(ErrorCode::NotConnected); }
    Result sendPacket(uint32_t, ByteView) override { return Result::failure(ErrorCode::NotConnected); }
    void setPacketReceiver(BleCentralPacketReceiver, void*) override {}
    void setConnectionHandlers(BleCentralConnectionHandler, BleCentralConnectionHandler, void*) override {}
};

class Random final : public IRandom {
public:
    Result fill(MutableByteSpan output) override { std::memset(output.data, 7, output.size); return Result::success(); }
};
class Crypto final : public IX25519AesGcmCryptoProvider {
public:
    Result x25519PublicKey(ByteView, MutableByteSpan) override { return Result::failure(ErrorCode::NotConfigured); }
    Result x25519(ByteView, ByteView, MutableByteSpan) override { return Result::failure(ErrorCode::NotConfigured); }
    Result aes256GcmEncrypt(ByteView, ByteView, ByteView, ByteView, MutableByteSpan, size_t&) override {
        return Result::failure(ErrorCode::NotConfigured);
    }
    Result aes256GcmDecrypt(ByteView, ByteView, ByteView, ByteView, MutableByteSpan, size_t&) override {
        return Result::failure(ErrorCode::NotConfigured);
    }
};

void productionContextsShareOnlyNorthbound() {
    // Exercise the exact context/bridge/matcher used by the ESP32 product.
    // No crypto/native Ready is fabricated: routing admission must not acquire BLE.
    Fixture f; f.route.setReceiver(nullptr, nullptr);
    Blob aBlob, aRenewal, bBlob, bRenewal;
    Random random; Crypto crypto; Port aPort, bPort;
    GatewayChildRecords aRecords(aBlob, aRenewal), bRecords(bBlob, bRenewal);
    BasicGatewayGattChildContext<> a(aPort, aRecords, f.client, f.clock, random, crypto);
    const auto aRecord = recordFor("ble_child_a", 1), bRecord = recordFor("ble_child_b", 2);
    commit(a.access, aRecord); const auto aMessage = admission(aRecord), bMessage = admission(bRecord);
    const auto deliver = [&f](const std::vector<uint8_t>& bytes) { f.mqtt.deliver(ByteView(bytes.data(), bytes.size())); };
    {
        BasicGatewayGattChildContext<> b(bPort, bRecords, f.client, f.clock, random, crypto);
        commit(b.access, bRecord);
        CHECK(!gatewayRouteMatchesChild(ByteView(aMessage.data(), aMessage.size() - 1), aRecords));
        deliver(aMessage); deliver(bMessage); f.client.poll();
        CHECK(a.bridge.admitted() && b.bridge.admitted());
        CHECK(a.bridge.lastError() == ErrorCode::Ok && b.bridge.lastError() == ErrorCode::Ok);
        const auto aStored = aBlob.bytes, bStored = bBlob.bytes;
        a.bridge.reset(); CHECK(!a.bridge.admitted() && b.bridge.admitted());
        deliver(aMessage); f.client.poll(); CHECK(a.bridge.admitted());
        // Same logical id with another instance/epoch/version must not reach either context.
        for (unsigned field = 0; field != 4; ++field) {
            auto wrong = aRecord;
            if (field == 0) wrong.childDeviceInstanceId[0] ^= 0x40;
            if (field == 1) ++wrong.accessEpoch;
            if (field == 2) ++wrong.credentialVersion;
            if (field == 3) wrong.childLogicalDeviceId[0] = 'x';
            deliver(admission(wrong)); f.client.poll();
            CHECK(a.bridge.admitted() && b.bridge.admitted());
            CHECK(a.bridge.lastError() == ErrorCode::Ok && b.bridge.lastError() == ErrorCode::Ok);
        }
        CHECK(aBlob.bytes == aStored && bBlob.bytes == bStored);
        // Selection alone never replaces the bridge's operation/authority checks.
        a.bridge.reset(); auto wrongOperation = aRecord; wrongOperation.operationId[0] ^= 0x80;
        const auto wrongAdmission = admission(wrongOperation);
        CHECK(gatewayRouteMatchesChild(ByteView(wrongAdmission.data(), wrongAdmission.size()), aRecords));
        deliver(wrongAdmission); f.client.poll();
        CHECK(!a.bridge.admitted() && a.bridge.lastError() == ErrorCode::AuthenticationRequired);
        CHECK(b.bridge.admitted() && b.bridge.lastError() == ErrorCode::Ok);
        deliver(aMessage); f.client.poll(); CHECK(a.bridge.admitted());
        gateway::GatewayRouteClosureView closure;
        closure.topologyVersion = 1; closure.generation = 1;
        closure.childLogicalDeviceId = StringView(bRecord.childLogicalDeviceId, bRecord.childLogicalDeviceIdSize);
        closure.childDeviceInstanceId = ByteView(bRecord.childDeviceInstanceId, sizeof(bRecord.childDeviceInstanceId));
        closure.accessEpoch = bRecord.accessEpoch; closure.gatewayCredentialVersion = bRecord.credentialVersion;
        uint8_t encoded[256]; size_t written = 0;
        CHECK(gateway::encodeGatewayRouteClosure(closure, MutableByteSpan(encoded, sizeof(encoded)), written));
        f.mqtt.published.clear();
        CHECK(a.route.publish(ByteView(encoded, written)).code() == ErrorCode::StateConflict);
        CHECK(f.mqtt.published.empty()); CHECK(b.route.publish(ByteView(encoded, written)));
        CHECK(aPort.starts == 0 && bPort.starts == 0 && aPort.connects == 0 && bPort.connects == 0);
    }
    a.bridge.reset(); deliver(bMessage); deliver(aMessage); f.client.poll(); CHECK(a.bridge.admitted());
    CHECK(f.client.subscribed());
    // The selector uses current durable identity again at dispatch.
    deliver(aMessage); CHECK(a.route.hasPendingEnvelope()); CHECK(a.access.clear()); f.client.poll();
    CHECK(!a.route.hasPendingEnvelope() && a.route.lastError() == ErrorCode::StateConflict);
    CHECK(aPort.connects == 0 && bPort.connects == 0);
}
} // namespace

int main() {
    exactHandoff();
    generationFence();
    ownerAndInputFence();
    boundedWait();
    capacityProbeDoesNotExecuteExpiredCommand();
    closurePublishProfile();
    leasePublishProfile();
    executionPublishProfile();
    directYieldPublishProfile();
    independentChildIngress();
    selectionAndSessionFences();
    clientDestructionDetachesBorrowers();
    productionContextsShareOnlyNorthbound();
    std::cout << "Gateway route client tests passed\n";
}
