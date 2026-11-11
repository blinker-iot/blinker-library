#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include "BlinkerV2/location/CloudLocationBinding.h"
#include "BlinkerV2/api/StateUpdate.h"

// Fail in the runner, not a Windows CRT assertion dialog.
#undef assert
#define assert(condition) do { if (!(condition)) { fprintf(stderr, "line %d: %s\n", __LINE__, #condition); exit(1); } } while (0)

using namespace blinker;
namespace { size_t allocations = 0U; }
void* operator new(size_t size) {
    ++allocations;
    void* value = malloc(size == 0U ? 1U : size);
    if (value == nullptr) throw std::bad_alloc();
    return value;
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { free(value); }
void operator delete[](void* value) noexcept { free(value); }

namespace {
class Clock final : public IClock {
public:
    uint32_t now;
    Clock() : now(1000U) {}
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }
};

class Transport final : public IFrameTransport {
public:
    FrameReceiver receiver;
    void* context;
    TransportState state_;
    uint16_t features;
    uint8_t bytes[512];
    size_t size;
    size_t dataSends;
    Transport() : receiver(nullptr), context(nullptr), state_(TransportState::Stopped),
        features(TransportFeatureBinary | TransportFeatureCloud | TransportFeatureAuthenticated),
        bytes(), size(0U), dataSends(0U) {}
    Result start() override { state_ = TransportState::Online; return Result::success(); }
    void stop() override { state_ = TransportState::Stopped; }
    void poll(uint32_t) override {}
    TransportState state() const override { return state_; }
    TransportCapabilities capabilities() const override {
        TransportCapabilities result; result.maxFrameSize = sizeof(bytes); result.features = features; return result;
    }
    Result send(ByteView frame, const SendTarget&) override {
        assert(frame.size <= sizeof(bytes)); memcpy(bytes, frame.data, frame.size); size = frame.size;
        bbp2::FrameView parsed; assert(bbp2::parseFrame(ByteView(bytes, size), parsed));
        if (parsed.header.kind == static_cast<uint8_t>(bbp2::MessageKind::TelemetryData)) ++dataSends;
        return Result::success();
    }
    void setReceiver(FrameReceiver next, void* value) override { receiver = next; context = value; }
    void inject(bbp2::MessageKind kind, uint8_t flags, uint16_t sequence, ByteView body) {
        uint8_t frame[512]; bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(kind); header.flags = flags;
        header.sequence = sequence; header.bodyLength = static_cast<uint16_t>(body.size);
        assert(bbp2::encodeHeader(header, MutableByteSpan(frame, sizeof(frame))));
        memcpy(frame + bbp2::kBaseHeaderSize, body.data, body.size);
        RxContext rx; rx.sessionId = 17U; rx.authenticated = true; rx.encrypted = true;
        receiver(context, ByteView(frame, bbp2::kBaseHeaderSize + body.size), rx);
    }
};

static const FieldSpec fields[] = {
    FieldSpec("temperature", EndpointKind::Property, ValueType::Float32, AccessRead, nullptr, 1000U),
    FieldSpec("position", EndpointKind::Property, ValueType::Object, AccessRead, nullptr, 1000U),
    FieldSpec("power", EndpointKind::Property, ValueType::Boolean, AccessRead | AccessWrite | AccessNotify)
};
Result temperature(void*, uint16_t id, bbp2::IdBodyWriter& writer) { return writer.writeFloat32(id, 21.5f); }
Result powerCommand(void*, const EndpointHandle& endpoint, const EndpointValueView& value, const RxContext&) {
    bool requested = false;
    const Result decoded = value.asBool(requested);
    return decoded ? endpoint.set(requested) : decoded;
}

class Fixture {
public:
    LocationSnapshot snapshot;
    CloudLocationBinding binding;
    ClientStorage<3, 512, 192, 128> storage;
    Client client;
    Clock clock;
    Transport transport;
    Fixture() : snapshot(), binding(snapshot), storage(), client(storage.buffers(fields)), clock(), transport() {
        assert(client.setMonotonicClock(&clock)); assert(client.addTransport(transport));
        assert(client.endpoints().find("temperature").onSample(&temperature, nullptr));
    }
    ~Fixture() { client.end(); }
    void start(bool attach = true) {
        if (attach) assert(binding.attach(client, "position"));
        assert(client.begin()); client.poll(1000U);
        bbp2::FrameView frame; assert(bbp2::parseFrame(ByteView(transport.bytes, transport.size), frame));
        bbp2::HelloBody hello; assert(bbp2::decodeHelloBody(frame.body, hello));
        bbp2::HelloBody peer;
        peer.role = bbp2::PeerRole::App; peer.versions[0] = bbp2::kVersion; peer.versionCount = 1U;
        peer.features = bbp2::FeatureManifest | bbp2::FeatureEndpointIds | bbp2::FeatureTelemetry |
                        bbp2::FeatureStateRevision | bbp2::FeatureReliableDelivery;
        peer.maxFrameSize = 512U; peer.maxReassemblySize = 512U;
        peer.reliableReceiveWindow = 4U;
        peer.manifestRevision = hello.manifestRevision; peer.hasManifestRevision = true;
        peer.manifestFingerprint = hello.manifestFingerprint; peer.hasManifestFingerprint = true;
        uint8_t data[192]; ByteView body;
        assert(bbp2::encodeHelloBody(peer, MutableByteSpan(data, sizeof(data)), body));
        transport.inject(bbp2::MessageKind::Hello, bbp2::FlagIsResponse, 1U, body);
    }
    bbp2::TelemetryStatusBody control(bbp2::TelemetryControlOperation operation, bool mixed = true, uint32_t epoch = 0U) {
        bbp2::TelemetryControlBody request; request.operation = operation; request.streamId = 1U;
        request.streamEpoch = epoch;
        if (operation != bbp2::TelemetryControlOperation::Close) request.leaseMillis = 5000U;
        if (operation == bbp2::TelemetryControlOperation::Open) {
            request.intervalMillis = 1000U;
            request.fieldIds[0] = mixed ? 1U : 2U; request.fieldIds[1] = 2U; request.fieldCount = mixed ? 2U : 1U;
        }
        uint8_t data[128]; ByteView body;
        assert(bbp2::encodeTelemetryControlBody(request, MutableByteSpan(data, sizeof(data)), body));
        transport.inject(bbp2::MessageKind::TelemetryControl, bbp2::FlagIdMode, 2U, body);
        bbp2::FrameView frame; assert(bbp2::parseFrame(ByteView(transport.bytes, transport.size), frame));
        assert(frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::TelemetryStatus));
        bbp2::TelemetryStatusBody result; assert(bbp2::decodeTelemetryStatusBody(frame.body, result)); return result;
    }
    void fix() {
        LocationFix value; value.valid = true; value.latitudeE7 = 123; value.longitudeE7 = -456;
        assert(snapshot.update(value, clock.now, clock.now));
    }
    bbp2::TelemetryDataBody sample() {
        client.poll(1000U); bbp2::FrameView frame;
        assert(bbp2::parseFrame(ByteView(transport.bytes, transport.size), frame));
        assert(frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::TelemetryData));
        bbp2::TelemetryDataBody data; assert(bbp2::decodeTelemetryDataBody(frame.body, data)); return data;
    }
};

void attachAndState() {
    Fixture f; f.start();
    assert(f.binding.attach(f.client, "position").code() == ErrorCode::AlreadyExists);
    CloudLocationBinding another(f.snapshot);
    assert(another.attach(f.client, "position").code() == ErrorCode::AlreadyExists);
    EndpointValueView value;
    assert(f.client.endpoints().find("position").current(value).code() == ErrorCode::NotFound);
    const uint8_t object[] = {0xA0U};
    const uint32_t revision = f.client.runtime().stateRevision();
    assert(f.client.endpoints().find("position").setEncoded(ByteView(object, sizeof(object))).code() == ErrorCode::UnsupportedFeature);
    {
        StateUpdate update(f.client); assert(update.begin(2U));
        assert(update.set(f.client.endpoints().find("power"), true));
        assert(update.setEncoded(f.client.endpoints().find("position"), ByteView(object, sizeof(object))));
        assert(update.commit().code() == ErrorCode::UnsupportedFeature);
    }
    assert(f.client.runtime().stateRevision() == revision);
    assert(f.client.endpoints().find("power").current(value).code() == ErrorCode::NotFound);
    assert(f.client.endpoints().find("power").set(true));
    assert(f.client.endpoints().find("power").current(value));
}

void rejectExistingAndInvalid() {
    Fixture f; f.start(false);
    const uint8_t object[] = {0xA0U};
    assert(f.client.endpoints().find("position").setEncoded(ByteView(object, sizeof(object))));
    const uint32_t revision = f.client.runtime().stateRevision();
    assert(f.binding.attach(f.client, "position").code() == ErrorCode::StateConflict);
    assert(f.client.runtime().stateRevision() == revision);
    assert(f.binding.attach(f.client, "missing").code() == ErrorCode::NotFound);
    assert(f.binding.attach(f.client, "power").code() == ErrorCode::InvalidArgument);
    PropertyConstraints constraints;
    constraints.flags = ConstraintMaxLength;
    constraints.maxLength = 7U;
    const FieldSpec bounded[] = {FieldSpec("limited", EndpointKind::Property, ValueType::Object,
        AccessRead, &constraints, 1000U)};
    ClientStorage<1, 512, 192, 128> storage;
    Client client(storage.buffers(bounded));
    CloudLocationBinding binding(f.snapshot);
    assert(binding.attach(client, "limited").code() == ErrorCode::InvalidArgument);
}

void cloudMixed() {
    Fixture f; f.start(); f.fix();
    f.control(bbp2::TelemetryControlOperation::Open);
    bbp2::TelemetryDataBody data = f.sample();
    cbor::Reader reader(data.values); size_t count = 0U; uint64_t id = 0U; double temperatureValue = 0;
    assert(reader.readMapSize(count)); assert(count == 2U);
    assert(reader.readUnsigned(id) && id == 1U); assert(reader.readFloat(temperatureValue));
    assert(temperatureValue == 21.5f); assert(reader.readUnsigned(id) && id == 2U);
    ByteView encoded; assert(reader.captureValue(encoded)); LocationSample sample;
    assert(location::decodeSample(encoded, sample)); assert(sample.fix.latitudeE7 == 123);
    assert(sample.ageMillis == 0U);
    f.snapshot.invalidate(); f.clock.now += 1000U;
    data = f.sample(); cbor::Reader missing(data.values);
    assert(missing.readMapSize(count) && count == 1U); assert(missing.readUnsigned(id) && id == 1U);
    f.clock.now += 1000U; f.fix(); data = f.sample(); cbor::Reader restored(data.values);
    assert(restored.readMapSize(count) && count == 2U); // Retained lease was never pruned.
    EndpointValueView value;
    assert(f.client.endpoints().find("position").current(value).code() == ErrorCode::NotFound);
}

void originalControlAndClose() {
    Fixture f; f.start(); f.fix();
    assert(f.client.endpoints().find("power").onCommand(&powerCommand, nullptr));
    uint8_t bytes[64]; ByteView body;
    bbp2::IdBodyWriter command(MutableByteSpan(bytes, sizeof(bytes)));
    assert(command.begin(1U)); assert(command.writeBool(3U, true)); assert(command.finish(body));
    f.transport.inject(bbp2::MessageKind::Command, bbp2::FlagIdMode | bbp2::FlagAckRequired, 30U, body);
    bbp2::FrameView frame; assert(bbp2::parseFrame(ByteView(f.transport.bytes, f.transport.size), frame));
    assert(frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::Ack));
    bbp2::AckBody ack; assert(bbp2::decodeAckBody(frame.body, ack)); assert(ack.acknowledgedSequence == 30U);
    bbp2::StateRequestBody request;
    assert(bbp2::encodeStateRequestBody(request, MutableByteSpan(bytes, sizeof(bytes)), body));
    f.transport.inject(bbp2::MessageKind::StateRequest, bbp2::FlagIdMode, 31U, body);
    assert(bbp2::parseFrame(ByteView(f.transport.bytes, f.transport.size), frame));
    assert(frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::StatePage));
    bbp2::StatePageBody state; assert(bbp2::decodeStatePageBody(frame.body, state));
    cbor::Reader reader(state.values); size_t count = 0U; uint64_t id = 0U; bool power = false;
    assert(reader.readMapSize(count) && count == 1U);
    assert(reader.readUnsigned(id) && id == 3U); assert(reader.readBool(power) && power);
    const bbp2::TelemetryStatusBody opened = f.control(bbp2::TelemetryControlOperation::Open);
    f.sample(); const size_t sends = f.transport.dataSends;
    f.control(bbp2::TelemetryControlOperation::Close, true, opened.streamEpoch);
    f.clock.now += 1000U; f.client.poll(1000U); assert(f.transport.dataSends == sends);
}

void privateOnlyAndExpiry() {
    Fixture f; f.start();
    f.control(bbp2::TelemetryControlOperation::Open, false);
    const TelemetryCounters before = f.client.telemetryCounters();
    f.client.poll(1000U); f.client.poll(1000U);
    assert(f.transport.dataSends == 0U);
    assert(f.client.telemetryCounters().samplesSent == before.samplesSent);
    assert(f.client.telemetryCounters().samplesDropped == before.samplesDropped);
    f.clock.now += 1000U; f.fix(); f.sample(); assert(f.transport.dataSends == 1U);
    f.clock.now += 5000U; f.client.poll(1000U); assert(f.transport.dataSends == 1U);
    assert(f.client.telemetryCounters().leasesExpired == 1U);
}

void localNeverReads() {
    for (int mixedFeatures = 0; mixedFeatures < 2; ++mixedFeatures) {
        Fixture f;
        f.transport.features = TransportFeatureBinary | TransportFeatureLocal | TransportFeatureAuthenticated;
        if (mixedFeatures) f.transport.features |= TransportFeatureCloud;
        f.start(); f.fix(); f.control(bbp2::TelemetryControlOperation::Open);
        const bbp2::TelemetryDataBody data = f.sample(); cbor::Reader reader(data.values); size_t count = 0U;
        assert(reader.readMapSize(count) && count == 1U);
    }
}
}

int main() {
    const size_t before = allocations;
    attachAndState(); rejectExistingAndInvalid(); cloudMixed(); privateOnlyAndExpiry(); localNeverReads(); originalControlAndClose();
    assert(allocations == before);
    printf("cloud location: 6 groups passed; binding=%zu B; new/new[]=0\n", sizeof(CloudLocationBinding));
    return 0;
}
