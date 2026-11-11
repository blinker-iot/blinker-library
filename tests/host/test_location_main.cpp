#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <new>
#include <type_traits>

#include "BlinkerV2/location/LocationFix.h"
#include "BlinkerV2/location/LocationCodec.h"
#include "BlinkerV2/location/LocationSnapshot.h"

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)

namespace { size_t heapAllocations = 0U; }
void* operator new(size_t size) {
    ++heapAllocations;
    void* allocation = std::malloc(size == 0U ? 1U : size);
    if (allocation == nullptr) throw std::bad_alloc();
    return allocation;
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }

static_assert(sizeof(LocationFix) <= 32U, "Bound one GNSS fix");
static_assert(sizeof(LocationSample) <= 40U, "Bound one location sample");
static_assert(sizeof(LocationSnapshot) <= 64U, "No growing location history");
static_assert(!std::is_polymorphic<LocationSnapshot>::value, "No virtual dispatch");
static_assert(location::kMaximumEncodedSampleSize == 41U, "Bounded schema-1 payload");

namespace {
const uint32_t half = 0x80000000U;
const uint8_t zeroFix[] = {
    0xA8U, 0x00U, 0x01U, 0x01U, 0x01U, 0x02U, 0x00U, 0x03U, 0x00U,
    0x04U, 0xF6U, 0x05U, 0xF6U, 0x06U, 0x00U, 0x07U, 0x00U
};

LocationFix fix(int32_t latitude = 0, int32_t longitude = 0) {
    LocationFix result;
    result.valid = true;
    result.latitudeE7 = latitude;
    result.longitudeE7 = longitude;
    return result;
}

LocationSample sample(const LocationFix& value, uint32_t age = 0U) {
    LocationSample result;
    result.fix = value;
    result.ageMillis = age;
    return result;
}

bool same(const LocationFix& first, const LocationFix& second) {
    return first.valid == second.valid && first.latitudeE7 == second.latitudeE7 &&
        first.longitudeE7 == second.longitudeE7 &&
        first.hasHorizontalAccuracy == second.hasHorizontalAccuracy &&
        first.horizontalAccuracyCm == second.horizontalAccuracyCm &&
        first.timeQuality == second.timeQuality &&
        first.capturedAtUtcMillis == second.capturedAtUtcMillis;
}

bool same(const LocationSample& first, const LocationSample& second) {
    return same(first.fix, second.fix) && first.ageMillis == second.ageMillis;
}

LocationSample maximum() {
    LocationSample result = sample(fix(900000000, 1800000000), half - 1U);
    result.fix.hasHorizontalAccuracy = true;
    result.fix.horizontalAccuracyCm = UINT32_MAX;
    result.fix.timeQuality = LocationTimeQuality::GnssUtc;
    result.fix.capturedAtUtcMillis = kWallTimeEndMillis - 1U;
    return result;
}

ByteView encode(const LocationSample& value, uint8_t* output, size_t capacity) {
    ByteView encoded;
    CHECK(location::encodeSample(value, MutableByteSpan(output, capacity), encoded));
    CHECK(encoded.data == output && encoded.size <= capacity);
    return encoded;
}

void reject(ByteView input) {
    const LocationSample expected = sample(fix(1234567, -7654321), 42U);
    LocationSample output = expected;
    CHECK(!location::decodeSample(input, output));
    CHECK(same(output, expected));
}

void values() {
    CHECK(!validateLocationFix(LocationFix()));
    CHECK(validateLocationFix(fix())); // (0,0) is a real coordinate, not a sentinel.
    for (int32_t latitude : {-900000000, 0, 900000000}) {
        for (int32_t longitude : {-1800000000, 0, 1800000000}) {
            LocationFix value = fix(latitude, longitude);
            CHECK(validateLocationFix(value));
            value.hasHorizontalAccuracy = true;
            CHECK(validateLocationFix(value)); // Explicitly known zero is distinct from unknown.
            value.timeQuality = LocationTimeQuality::GnssUtc;
            value.capturedAtUtcMillis = kWallTimeMinimumMillis;
            CHECK(validateLocationFix(value));
            value.capturedAtUtcMillis = kWallTimeEndMillis - 1U;
            CHECK(validateLocationFix(value));
        }
    }
    LocationFix value = fix(900000001, 0);
    CHECK(!validateLocationFix(value));
    value.latitudeE7 = -900000001;
    CHECK(!validateLocationFix(value));
    value = fix(0, 1800000001);
    CHECK(!validateLocationFix(value));
    value.longitudeE7 = -1800000001;
    CHECK(!validateLocationFix(value));
    value = fix(); value.horizontalAccuracyCm = 1U;
    CHECK(!validateLocationFix(value));
    value = fix(); value.capturedAtUtcMillis = 1U;
    CHECK(!validateLocationFix(value));
    value = fix(); value.timeQuality = static_cast<LocationTimeQuality>(2U);
    CHECK(!validateLocationFix(value));
    value = fix(); value.timeQuality = LocationTimeQuality::GnssUtc;
    for (uint64_t utc : {static_cast<uint64_t>(0U), kWallTimeMinimumMillis - 1U, kWallTimeEndMillis}) {
        value.capturedAtUtcMillis = utc;
        CHECK(!validateLocationFix(value));
    }
    CHECK(validateLocationSample(maximum()));
    CHECK(!validateLocationSample(sample(fix(), half)));
}

void roundTrips() {
    uint8_t buffer[location::kMaximumEncodedSampleSize] = {};
    ByteView encoded = encode(sample(fix()), buffer, sizeof(buffer));
    CHECK(encoded.size == sizeof(zeroFix));
    CHECK(std::memcmp(encoded.data, zeroFix, sizeof(zeroFix)) == 0);
    LocationSample decoded;
    CHECK(location::decodeSample(encoded, decoded));
    CHECK(same(decoded, sample(fix())));
    const LocationSample extremes[] = {maximum(), sample(fix(-900000000, -1800000000)),
                                       sample(fix(10000000, -10000000), 30U)};
    for (const auto& value : extremes) {
        encoded = encode(value, buffer, sizeof(buffer));
        CHECK(location::decodeSample(encoded, decoded));
        CHECK(same(decoded, value));
    }
    encoded = encode(maximum(), buffer, sizeof(buffer));
    CHECK(encoded.size == location::kMaximumEncodedSampleSize);
    LocationSample zeroAccuracy = sample(fix());
    zeroAccuracy.fix.hasHorizontalAccuracy = true;
    encoded = encode(zeroAccuracy, buffer, sizeof(buffer));
    CHECK(location::decodeSample(encoded, decoded));
    CHECK(decoded.fix.hasHorizontalAccuracy && decoded.fix.horizontalAccuracyCm == 0U);
}

void encodeFailures() {
    uint8_t buffer[location::kMaximumEncodedSampleSize];
    uint8_t sentinel = 0x3CU;
    const ByteView original(&sentinel, 1U);
    for (size_t capacity = 0U; capacity < sizeof(buffer); ++capacity) {
        std::memset(buffer, 0xA5, sizeof(buffer));
        ByteView output = original;
        CHECK(!location::encodeSample(maximum(), MutableByteSpan(buffer, capacity), output));
        CHECK(output.data == original.data && output.size == original.size);
        for (uint8_t byte : buffer) CHECK(byte == 0xA5U);
    }
    std::memset(buffer, 0xA5, sizeof(buffer));
    ByteView output = original;
    CHECK(!location::encodeSample(LocationSample(), MutableByteSpan(buffer, sizeof(buffer)), output));
    CHECK(output.data == original.data && output.size == original.size);
    for (uint8_t byte : buffer) CHECK(byte == 0xA5U);
    CHECK(!location::encodeSample(maximum(), MutableByteSpan(nullptr, sizeof(buffer)), output));
    CHECK(output.data == original.data && output.size == original.size);
}

void malformed() {
    uint8_t buffer[64] = {};
    const ByteView encoded = encode(maximum(), buffer, sizeof(buffer));
    for (size_t size = 0U; size < encoded.size; ++size) reject(ByteView(buffer, size));
    buffer[encoded.size] = 0U;
    reject(ByteView(buffer, encoded.size + 1U));
    reject(ByteView(nullptr, 1U));
    const uint8_t mutations[][2] = {
        {0U, 0xA7U}, {0U, 0xA9U}, // Missing/extra pair count.
        {2U, 2U}, {4U, 2U},       // Unknown schema / non-GNSS source.
        {7U, 2U}, {5U, 3U}, {15U, 8U}, // Duplicate, reordered or unknown key.
        {6U, 0xF5U}, {8U, 0xF6U}, // Coordinates cannot be Boolean/null.
        {10U, 0x20U}, {10U, 0xF5U}, // Accuracy cannot be negative/Boolean.
        {12U, 0U}, {12U, 0xF5U},  // Unknown time must be null, not numeric zero.
        {14U, 1U}, {14U, 2U},     // GNSS quality needs time; quality is versioned.
        {16U, 0x20U}, {16U, 0xF6U} // Age cannot be negative/null.
    };
    for (const auto& mutation : mutations) {
        std::memcpy(buffer, zeroFix, sizeof(zeroFix));
        buffer[mutation[0]] = mutation[1];
        reject(ByteView(buffer, sizeof(zeroFix)));
    }
    // A shortest integer encoding is required for both keys and values.
    for (size_t at : {1U, 2U}) {
        std::memcpy(buffer, zeroFix, at);
        buffer[at] = 0x18U;
        buffer[at + 1U] = zeroFix[at];
        std::memcpy(buffer + at + 2U, zeroFix + at + 1U, sizeof(zeroFix) - at - 1U);
        reject(ByteView(buffer, sizeof(zeroFix) + 1U));
    }
}

void rejectedRanges() {
    // Independently construct structurally valid maps to test decoder semantics.
    for (unsigned mode = 0U; mode < 5U; ++mode) {
        uint8_t buffer[64] = {};
        cbor::Writer writer(MutableByteSpan(buffer, sizeof(buffer)));
        CHECK(writer.beginMap(8U));
        for (uint64_t key = 0U; key < 8U; ++key) {
            CHECK(writer.writeUnsigned(key));
            switch (key) {
                case 0U: case 1U: CHECK(writer.writeUnsigned(1U)); break;
                case 2U: CHECK(writer.writeInt(mode == 0U ? 900000001LL : 0LL)); break;
                case 3U: CHECK(writer.writeInt(mode == 1U ? -1800000001LL : 0LL)); break;
                case 4U:
                    if (mode == 2U) CHECK(writer.writeUnsigned(static_cast<uint64_t>(UINT32_MAX) + 1U));
                    else CHECK(writer.writeNull());
                    break;
                case 5U:
                    if (mode == 3U) CHECK(writer.writeUnsigned(kWallTimeEndMillis));
                    else CHECK(writer.writeNull());
                    break;
                case 6U: CHECK(writer.writeUnsigned(mode == 3U ? 1U : 0U)); break;
                case 7U: CHECK(writer.writeUnsigned(mode == 4U ? half : 0U)); break;
                default: CHECK(false);
            }
        }
        reject(writer.view());
    }
}

void freshnessAndOrder() {
    LocationSnapshot snapshot(30U);
    CHECK(snapshot.lastFix() == nullptr);
    LocationSample output = sample(fix(7, 8), 9U), expected = output;
    CHECK(!snapshot.read(0U, output)); CHECK(same(output, expected));
    CHECK(snapshot.update(fix(1, 2), 100U, 105U));
    CHECK(snapshot.read(110U, output)); CHECK(output.ageMillis == 10U);
    CHECK(snapshot.read(130U, output)); CHECK(output.ageMillis == 30U);
    expected = output;
    CHECK(!snapshot.read(131U, output)); CHECK(same(output, expected));
    CHECK(snapshot.lastFix() != nullptr && same(*snapshot.lastFix(), fix(1, 2)));
    CHECK(snapshot.update(fix(3, 4), 95U, 110U).code() == ErrorCode::SequenceConflict);
    CHECK(snapshot.update(fix(3, 4), 100U, 110U).code() == ErrorCode::SequenceConflict);
    CHECK(!snapshot.update(fix(3, 4), 200U, 199U)); // Future sample.
    CHECK(!snapshot.update(fix(3, 4), 150U, 200U)); // Already too old.
    CHECK(!snapshot.update(LocationFix(), 110U, 110U));
    CHECK(same(*snapshot.lastFix(), fix(1, 2)));
    CHECK(snapshot.update(fix(3, 4), 110U, 110U));
    snapshot.invalidate();
    expected = output;
    CHECK(!snapshot.read(111U, output)); CHECK(same(output, expected));
    CHECK(same(*snapshot.lastFix(), fix(3, 4))); // Lost fix does not zero last position.
    CHECK(snapshot.update(fix(5, 6), 111U, 112U));
    CHECK(snapshot.read(112U, output)); CHECK(output.ageMillis == 1U);
    snapshot.clear();
    CHECK(snapshot.lastFix() == nullptr);
    expected = output;
    CHECK(!snapshot.read(112U, output)); CHECK(same(output, expected));
    CHECK(snapshot.update(fix(), 0U, 0U)); // Explicit boot reset.
}

void tickBoundaries() {
    LocationSnapshot snapshot(30U);
    LocationSample output;
    CHECK(snapshot.update(fix(1, 2), UINT32_MAX - 10U, 4U));
    CHECK(snapshot.read(4U, output)); CHECK(output.ageMillis == 15U);
    CHECK(snapshot.read(19U, output)); CHECK(output.ageMillis == 30U);
    CHECK(!snapshot.read(20U, output));
    CHECK(snapshot.update(fix(3, 4), 8U, 10U));
    CHECK(snapshot.read(10U, output)); CHECK(output.ageMillis == 2U);
    CHECK(!snapshot.update(fix(5, 6), UINT32_MAX - 11U, 10U));
    snapshot.clear();
    CHECK(snapshot.update(fix(), 100U, 100U));
    CHECK(!snapshot.read(0U, output)); // Tick reset requires explicit clear.
    for (uint32_t maximumAge : {0U, half, UINT32_MAX}) {
        LocationSnapshot invalid(maximumAge);
        CHECK(!invalid.update(fix(), 0U, 0U));
        const LocationSample previous = output;
        CHECK(!invalid.read(0U, output)); CHECK(same(output, previous));
    }
    LocationSnapshot longest(half - 1U);
    CHECK(longest.update(fix(), 0U, 0U));
    CHECK(longest.read(half - 1U, output));
    CHECK(!longest.read(half, output));
    const LocationSample beforeBadTick = output;
    CHECK(!longest.read(1U, output)); // A bad tick latches unavailable, even after a small tick returns.
    CHECK(same(output, beforeBadTick));
    CHECK(longest.lastFix() != nullptr && same(*longest.lastFix(), fix()));
    CHECK(longest.update(fix(7, 8), 1U, 1U));
    CHECK(longest.read(1U, output)); CHECK(same(output.fix, fix(7, 8)));

    LocationSnapshot expired(30U);
    CHECK(expired.update(fix(1, 2), 0U, 0U));
    CHECK(expired.read(0U, output));
    const LocationSample beforeExpiry = output;
    CHECK(!expired.read(31U, output)); CHECK(same(output, beforeExpiry));
    CHECK(!expired.read(UINT32_MAX, output)); CHECK(same(output, beforeExpiry));
    CHECK(!expired.read(0U, output)); CHECK(same(output, beforeExpiry));
    CHECK(expired.lastFix() != nullptr && same(*expired.lastFix(), fix(1, 2)));
    CHECK(expired.update(fix(3, 4), 1U, 1U)); // Only a new accepted observation restores freshness.
    CHECK(expired.read(1U, output)); CHECK(same(output.fix, fix(3, 4)));
}
}

int main() {
    const size_t before = heapAllocations;
    values();
    roundTrips();
    encodeFailures();
    malformed();
    rejectedRanges();
    freshnessAndOrder();
    tickBoundaries();
    CHECK(heapAllocations == before);
    std::printf("Location tests passed: 7 groups; no new/new[] allocations; fix=%zu sample=%zu snapshot=%zu bytes\n",
                sizeof(LocationFix), sizeof(LocationSample), sizeof(LocationSnapshot));
    return 0;
}
