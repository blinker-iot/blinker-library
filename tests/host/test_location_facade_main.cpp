#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>

#include "Blinker.h"

using namespace blinker;
#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #condition); std::exit(1); } } while (0)

namespace { size_t allocations = 0U; }
void* operator new(size_t size) {
    ++allocations;
    void* value = std::malloc(size == 0U ? 1U : size);
    if (value == nullptr) throw std::bad_alloc();
    return value;
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }

bool observeLocationFromCoreTu(Location&, const LocationFix&, uint32_t);
size_t locationSizeFromCoreTu();

namespace {
struct FacadeFootprint { Product* product; TimeSyncService* time; ErrorCode error; };
static_assert(sizeof(BlinkerClass) == sizeof(FacadeFootprint), "Location must not add global facade storage");
static_assert(!std::is_copy_constructible<Location>::value && !std::is_move_constructible<Location>::value,
              "Location contains self-referencing borrowed storage");
static_assert(!std::is_constructible<Location, LocationSpec&&>::value,
              "A temporary Spec must not leave a dangling schema");
static_assert(sizeof(Location) <= sizeof(LocationSnapshot) + sizeof(CloudLocationBinding) + 2U * sizeof(void*),
              "Location owns only one bounded snapshot/binding and two handles");

class Clock final : public IClock {
public:
    uint32_t now = 1000U;
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t& value) const override { value = 0U; return false; }
};

class Transport final : public IFrameTransport {
public:
    FrameReceiver receiver = nullptr;
    void* context = nullptr;
    TransportState state_ = TransportState::Stopped;
    uint8_t bytes[512] = {}, telemetry[512] = {};
    size_t size = 0U, telemetrySize = 0U, dataSends = 0U;
    Result start() override { state_ = TransportState::Online; return Result::success(); }
    void stop() override { state_ = TransportState::Stopped; }
    void poll(uint32_t) override {}
    TransportState state() const override { return state_; }
    TransportCapabilities capabilities() const override {
        TransportCapabilities value;
        value.maxFrameSize = sizeof(bytes);
        value.features = TransportFeatureBinary | TransportFeatureCloud | TransportFeatureAuthenticated;
        return value;
    }
    Result send(ByteView frame, const SendTarget&) override {
        CHECK(frame.size <= sizeof(bytes)); std::memcpy(bytes, frame.data, frame.size); size = frame.size;
        bbp2::FrameView parsed; CHECK(bbp2::parseFrame(ByteView(bytes, size), parsed));
        if (parsed.header.kind == static_cast<uint8_t>(bbp2::MessageKind::TelemetryData)) {
            std::memcpy(telemetry, bytes, size); telemetrySize = size; ++dataSends;
        }
        return Result::success();
    }
    void setReceiver(FrameReceiver value, void* selected) override { receiver = value; context = selected; }
    void inject(bbp2::MessageKind kind, uint8_t flags, uint16_t sequence, ByteView body) {
        CHECK(receiver != nullptr && body.size + bbp2::kBaseHeaderSize <= sizeof(bytes));
        uint8_t buffer[512]; bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(kind); header.flags = flags; header.sequence = sequence;
        header.bodyLength = static_cast<uint16_t>(body.size);
        CHECK(bbp2::encodeHeader(header, MutableByteSpan(buffer, sizeof(buffer))));
        std::memcpy(buffer + bbp2::kBaseHeaderSize, body.data, body.size);
        RxContext rx; rx.sessionId = 17U; rx.authenticated = true; rx.encrypted = true;
        receiver(context, ByteView(buffer, bbp2::kBaseHeaderSize + body.size), rx);
    }
};

class Lifecycle final : public IProductLifecycle {
public:
    Client* client = nullptr;
    Clock clock;
    Transport transport;
    bool withClock = true;
    unsigned timeAttaches = 0U;
    Result attach(Client& selected) override {
        client = &selected;
        Result result = withClock ? client->setMonotonicClock(&clock) : Result::success();
        return result ? client->addTransport(transport) : result;
    }
    Result attachTime(Client&, ITimeSync&) override { ++timeAttaches; return Result::failure(ErrorCode::UnsupportedFeature); }
    Result start() override { return client->begin(); }
    void poll(uint32_t budget) override { client->poll(budget); }
    void stop() override { if (client != nullptr) client->end(); }
    ProductLifecycleStatus status() const override { return ProductLifecycleStatus(ProductLifecycleState::Active); }
    ProductCapabilities capabilities() const override { return ProductCapabilities(ProductCapabilityCloudData); }
} testLifecycle;

// A later interaction fails once. A second Device prepare must safely revisit
// the already attached Location rather than register a second private source.
class RetryInteraction {
public:
    enum : size_t { fieldCount = 1U };
    unsigned attempts = 0U;
    FieldSpec field = FieldSpec("retry", EndpointKind::Property, ValueType::Boolean, AccessRead);
    const FieldSpec* fieldAt(size_t index) const { return index == 0U ? &field : nullptr; }
    Result bind(Client&) { return ++attempts == 1U ? Result::failure(ErrorCode::WouldBlock) : Result::success(); }
} retry;

BLINKER_LOCATION(position);
BLINKER_LOCATION_KEY(unbound, "other-position");
BLINKER_PROPERTY(power, bool, readWrite());

LocationFix fix(int32_t latitude = 0, int32_t longitude = 0) {
    LocationFix value; value.valid = true; value.latitudeE7 = latitude; value.longitudeE7 = longitude; return value;
}

void handshake() {
    Blinker.run();
    bbp2::FrameView sent; CHECK(bbp2::parseFrame(ByteView(testLifecycle.transport.bytes, testLifecycle.transport.size), sent));
    CHECK(sent.header.kind == static_cast<uint8_t>(bbp2::MessageKind::Hello));
    bbp2::HelloBody hello; CHECK(bbp2::decodeHelloBody(sent.body, hello));
    bbp2::HelloBody peer;
    peer.role = bbp2::PeerRole::App; peer.versions[0] = bbp2::kVersion; peer.versionCount = 1U;
    peer.features = bbp2::FeatureManifest | bbp2::FeatureEndpointIds | bbp2::FeatureTelemetry |
                    bbp2::FeatureStateRevision | bbp2::FeatureReliableDelivery;
    peer.maxFrameSize = 512U; peer.maxReassemblySize = 512U; peer.reliableReceiveWindow = 4U;
    peer.manifestRevision = hello.manifestRevision; peer.hasManifestRevision = true;
    peer.manifestFingerprint = hello.manifestFingerprint; peer.hasManifestFingerprint = true;
    uint8_t buffer[192]; ByteView body;
    CHECK(bbp2::encodeHelloBody(peer, MutableByteSpan(buffer, sizeof(buffer)), body));
    testLifecycle.transport.inject(bbp2::MessageKind::Hello, bbp2::FlagIsResponse, 1U, body);
}

uint32_t control(bbp2::TelemetryControlOperation operation, uint32_t epoch = 0U) {
    bbp2::TelemetryControlBody request; request.operation = operation; request.streamId = 1U; request.streamEpoch = epoch;
    if (operation != bbp2::TelemetryControlOperation::Close) request.leaseMillis = 60000U;
    if (operation == bbp2::TelemetryControlOperation::Open) {
        request.intervalMillis = 1000U; request.fieldCount = 1U;
        request.fieldIds[0] = testLifecycle.client->endpoints().find("position").id();
    }
    uint8_t buffer[128]; ByteView body;
    CHECK(bbp2::encodeTelemetryControlBody(request, MutableByteSpan(buffer, sizeof(buffer)), body));
    testLifecycle.transport.inject(bbp2::MessageKind::TelemetryControl, bbp2::FlagIdMode, 2U, body);
    bbp2::FrameView frame; CHECK(bbp2::parseFrame(ByteView(testLifecycle.transport.bytes, testLifecycle.transport.size), frame));
    CHECK(frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::TelemetryStatus));
    bbp2::TelemetryStatusBody status; CHECK(bbp2::decodeTelemetryStatusBody(frame.body, status));
    return status.streamEpoch;
}

LocationSample sample(uint32_t advance = 1000U) {
    const size_t before = testLifecycle.transport.dataSends;
    testLifecycle.clock.now += advance; Blinker.run(); CHECK(testLifecycle.transport.dataSends == before + 1U);
    bbp2::FrameView frame;
    CHECK(bbp2::parseFrame(ByteView(testLifecycle.transport.telemetry, testLifecycle.transport.telemetrySize), frame));
    bbp2::TelemetryDataBody data; CHECK(bbp2::decodeTelemetryDataBody(frame.body, data));
    cbor::Reader reader(data.values); size_t fields = 0U; uint64_t id = 0U; ByteView encoded;
    CHECK(reader.readMapSize(fields) && fields == 1U); CHECK(reader.readUnsigned(id));
    CHECK(id == testLifecycle.client->endpoints().find("position").id()); CHECK(reader.captureValue(encoded));
    LocationSample value; CHECK(location::decodeSample(encoded, value)); return value;
}

void noSample(uint32_t advance = 1000U) {
    const size_t before = testLifecycle.transport.dataSends;
    testLifecycle.clock.now += advance; Blinker.run(); CHECK(testLifecycle.transport.dataSends == before);
}

void bindingAndClock() {
    CHECK(!Blinker.location(position, 0.0, 0.0) && Blinker.lastError() == ErrorCode::NotConfigured);
    CHECK(!Blinker.locationLost(unbound) && !Blinker.clearLocation(unbound));
    CHECK(position.fieldAt(1U) == nullptr);
    CHECK(position.fieldAt(0U)->type == ValueType::Object && position.fieldAt(0U)->access == AccessRead);
    CHECK(position.fieldAt(0U)->telemetryMinimumIntervalMs == 1000U);
    CHECK(std::strcmp(unbound.fieldAt(0U)->key.data, "other-position") == 0);

    CHECK(!Blinker.begin("key", "ssid", "password", position, retry, power));
    CHECK(Blinker.lastError() == ErrorCode::WouldBlock);
    CHECK(Blinker.begin("key", "ssid", "password", position, retry, power));
    CHECK(retry.attempts == 2U && testLifecycle.timeAttaches == 0U);
    CHECK(locationSizeFromCoreTu() == sizeof(Location));
    handshake(); noSample();
    CHECK(Blinker.location(position, 0.0, 0.0)); noSample(); // Input does not authorize/send a stream.
    EndpointValueView value;
    CHECK(testLifecycle.client->endpoints().find("position").current(value).code() == ErrorCode::NotFound);
    const uint8_t object[] = {0xA0U};
    CHECK(testLifecycle.client->endpoints().find("position").setEncoded(ByteView(object, sizeof(object))).code()
          == ErrorCode::UnsupportedFeature);
    CHECK(Blinker.report(power, true)); bool powerValue = false; CHECK(power.current(powerValue) && powerValue);

    LocationSpec otherSpec("local"); Location other(otherSpec);
    auto schema = makeDeviceSchema(other);
    Device device(schema); Lifecycle noClock; noClock.withClock = false; Product product(device, noClock);
    CHECK(product.begin().code() == ErrorCode::NotConfigured);
    CHECK(!Blinker.location(other, fix()) && Blinker.lastError() == ErrorCode::NotConfigured);
    CHECK(noClock.client->setMonotonicClock(&noClock.clock)); CHECK(product.begin());
    CHECK(Blinker.location(other, fix())); product.end();

    auto reusedSchema = makeDeviceSchema(position);
    Device reused(reusedSchema); Lifecycle otherClient; Product rejected(reused, otherClient);
    CHECK(rejected.begin().code() == ErrorCode::AlreadyExists);
}

void valuesAndAge() {
    const uint32_t epoch = control(bbp2::TelemetryControlOperation::Open);
    LocationSample value = sample(0U);
    CHECK(value.fix.latitudeE7 == 0 && value.fix.longitudeE7 == 0);
    CHECK(!value.fix.hasHorizontalAccuracy && value.fix.horizontalAccuracyCm == 0U);
    CHECK(value.fix.timeQuality == LocationTimeQuality::Unknown && value.fix.capturedAtUtcMillis == 0U);
    const double nan = std::numeric_limits<double>::quiet_NaN(), infinity = std::numeric_limits<double>::infinity();
    CHECK(!Blinker.location(position, nan, 0.0)); CHECK(!Blinker.location(position, 0.0, infinity));
    CHECK(!Blinker.location(position, -infinity, 0.0)); CHECK(!Blinker.location(position, 90.01, 0.0));
    CHECK(!Blinker.location(position, 0.0, -180.01));
    value = sample(); CHECK(value.fix.latitudeE7 == 0 && value.fix.longitudeE7 == 0);

    CHECK(Blinker.location(position, 1.25, -2.5));
    value = sample(); CHECK(value.fix.latitudeE7 == 12500000 && value.fix.longitudeE7 == -25000000);
    CHECK(Blinker.location(position, 0.00000006, -0.00000006));
    value = sample(); CHECK(value.fix.latitudeE7 == 1 && value.fix.longitudeE7 == -1);
    LocationFix advanced = fix(-123456789, 987654321);
    advanced.hasHorizontalAccuracy = true; advanced.horizontalAccuracyCm = 0U;
    advanced.timeQuality = LocationTimeQuality::GnssUtc; advanced.capturedAtUtcMillis = 1789260000123ULL;
    CHECK(observeLocationFromCoreTu(position, advanced, testLifecycle.clock.now));
    value = sample(); CHECK(value.fix.latitudeE7 == advanced.latitudeE7 && value.fix.longitudeE7 == advanced.longitudeE7);
    CHECK(value.fix.hasHorizontalAccuracy && value.fix.horizontalAccuracyCm == 0U);
    CHECK(value.fix.timeQuality == LocationTimeQuality::GnssUtc && value.fix.capturedAtUtcMillis == advanced.capturedAtUtcMillis);
    uint64_t utc = 123U; CHECK(!Blinker.time(utc) && utc == 0U); CHECK(testLifecycle.timeAttaches == 0U);

    testLifecycle.clock.now += 1000U; const uint32_t sampledAt = testLifecycle.clock.now - 25U;
    CHECK(Blinker.location(position, advanced, sampledAt)); value = sample(0U); CHECK(value.ageMillis == 25U);
    CHECK(!Blinker.location(position, advanced, sampledAt) && Blinker.lastError() == ErrorCode::SequenceConflict);
    CHECK(!Blinker.location(position, advanced, sampledAt - 1U) && Blinker.lastError() == ErrorCode::SequenceConflict);
    CHECK(!Blinker.location(position, advanced, testLifecycle.clock.now + 1U));
    CHECK(!Blinker.location(position, advanced, testLifecycle.clock.now - 30001U));
    LocationFix invalid = advanced; invalid.valid = false; CHECK(!Blinker.location(position, invalid));
    invalid = advanced; invalid.latitudeE7 = 900000001; CHECK(!Blinker.location(position, invalid));
    invalid = advanced; invalid.hasHorizontalAccuracy = false; invalid.horizontalAccuracyCm = 1U;
    CHECK(!Blinker.location(position, invalid));
    invalid = advanced; invalid.timeQuality = LocationTimeQuality::Unknown; CHECK(!Blinker.location(position, invalid));
    value = sample(); CHECK(value.fix.latitudeE7 == advanced.latitudeE7 && value.ageMillis == 1025U);

    CHECK(Blinker.locationLost(position)); noSample();
    testLifecycle.clock.now += 1000U; CHECK(Blinker.location(position, 90.0, -180.0));
    value = sample(0U); CHECK(value.fix.latitudeE7 == 900000000 && value.fix.longitudeE7 == -1800000000);
    CHECK(Blinker.clearLocation(position)); noSample();
    control(bbp2::TelemetryControlOperation::Close, epoch); noSample();
}

void failedAttachCanRetry() {
    const uint32_t invalid[][2] = {{99U, 30000U}, {60001U, 30000U}, {1000U, 0U}, {1000U, 0x80000000U}};
    for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        LocationSpec spec("invalid", invalid[index][0], invalid[index][1]); Location candidate(spec);
        auto schema = makeDeviceSchema(candidate); Device device(schema); Lifecycle life; Product product(device, life);
        CHECK(product.begin().code() == ErrorCode::InvalidArgument);
        CHECK(!Blinker.location(candidate, fix()) && Blinker.lastError() == ErrorCode::NotConfigured);
    }
    LocationSpec firstSpec("first"), secondSpec("second"); Location first(firstSpec), second(secondSpec);
    auto two = makeDeviceSchema(first, second); Device occupied(two); Lifecycle firstClient; Product failed(occupied, firstClient);
    CHECK(failed.begin().code() == ErrorCode::AlreadyExists);
    CHECK(!Blinker.location(second, fix()) && Blinker.lastError() == ErrorCode::NotConfigured);
    auto one = makeDeviceSchema(second); Device available(one); Lifecycle secondClient; Product retried(available, secondClient);
    CHECK(retried.begin()); CHECK(Blinker.location(second, fix())); retried.end();
}

void wrapAndExpiry() {
    Blinker.end(); testLifecycle.clock.now = 4U; CHECK(Blinker.clearLocation(position));
    CHECK(Blinker.location(position, fix(7, 8), UINT32_MAX - 10U));
    CHECK(Blinker.begin("key", "ssid", "password", position, retry, power)); handshake();
    const uint32_t epoch = control(bbp2::TelemetryControlOperation::Open);
    LocationSample value = sample(0U); CHECK(value.ageMillis == 15U && value.fix.latitudeE7 == 7);
    noSample(30001U); // An expired point is not made new by the wrapper.
    CHECK(Blinker.location(position, -90.0, 180.0, testLifecycle.clock.now));
    value = sample(); CHECK(value.fix.latitudeE7 == -900000000 && value.fix.longitudeE7 == 1800000000);
    control(bbp2::TelemetryControlOperation::Close, epoch); Blinker.end();
}
} // namespace

namespace blinker { namespace integration {
IProductLifecycle& lifecycle(const WifiDeviceKeyProfile&) { return ::testLifecycle; }
} }

int main() {
    const size_t before = allocations;
    bindingAndClock(); valuesAndAge(); failedAttachCanRetry(); wrapAndExpiry();
    CHECK(allocations == before);
    std::printf("location facade: 4 groups passed; no new/new[]; facade=%zu location=%zu bytes\n",
                sizeof(BlinkerClass), sizeof(Location));
    return 0;
}
