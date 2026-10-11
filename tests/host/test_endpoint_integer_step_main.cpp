#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

#include "BlinkerV2/model/EndpointStateStore.h"

using namespace blinker;

namespace {

unsigned checks = 0U;
unsigned failures = 0U;
unsigned values = 0U;
const int64_t safeMaximum = INT64_C(9007199254740991);
const uint8_t readWrite = static_cast<uint8_t>(AccessRead | AccessWrite);

void check(bool passed, const char* label, int line) {
    ++checks;
    if (!passed) {
        ++failures;
        std::cerr << "line " << line << ": " << label << '\n';
    }
}

#define CHECK(value) check(static_cast<bool>(value), #value, __LINE__)

PropertyConstraints numeric(uint8_t flags, double minimum, double maximum,
                            double step) {
    return PropertyConstraints(flags, minimum, maximum, step, 0U,
                               StringView(), nullptr, 0U);
}

EndpointDescriptor property(ValueType type,
                            const PropertyConstraints* constraints) {
    return EndpointDescriptor("level", EndpointKind::Property, type,
                              readWrite, constraints);
}

void expectValue(const EndpointDescriptor& endpoint, ByteView encoded,
                 cbor::Type wireType, bool accepted, const char* label) {
    ++values;
    const Result result = validateEndpointValue(
        endpoint, encoded, wireType, AccessWrite);
    check(accepted ? result.ok() : result.code() == ErrorCode::ValueOutOfRange,
          label, __LINE__);
}

void expectSigned(const PropertyConstraints* constraints, int64_t value,
                  bool accepted, const char* label) {
    uint8_t buffer[16] = {};
    cbor::Writer writer(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(writer.writeInt(value));
    expectValue(property(ValueType::SignedInteger, constraints), writer.view(),
                value < 0 ? cbor::Type::Negative : cbor::Type::Unsigned,
                accepted, label);
}

void expectUnsigned(const PropertyConstraints* constraints, uint64_t value,
                    bool accepted, const char* label) {
    uint8_t buffer[16] = {};
    cbor::Writer writer(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(writer.writeUnsigned(value));
    expectValue(property(ValueType::UnsignedInteger, constraints), writer.view(),
                cbor::Type::Unsigned, accepted, label);
}

void hugeStepAndOffset() {
    // A valid integer step must not acquire quotient-relative tolerance.
    const PropertyConstraints huge = numeric(
        ConstraintStep, 0.0, 0.0, 1000000000001.0);
    CHECK(validateEndpointDescriptor(property(ValueType::UnsignedInteger, &huge)));
    expectUnsigned(&huge, 0U, true, "huge step accepts zero");
    expectUnsigned(&huge, 1U, false, "huge step rejects one");
    expectUnsigned(&huge, UINT32_MAX, false, "huge step rejects uint32 max");
    expectUnsigned(&huge, UINT64_C(1000000000001), true,
                   "huge step accepts exact multiple");
    expectUnsigned(&huge, UINT64_C(1000000000002), false,
                   "huge step rejects adjacent multiple");
    expectSigned(&huge, -INT64_C(1000000000001), true,
                 "huge step accepts negative multiple without minimum");
    expectSigned(&huge, -1, false, "huge step rejects negative one");

    const PropertyConstraints offset = numeric(
        static_cast<uint8_t>(ConstraintMinimum | ConstraintMaximum | ConstraintStep),
        1.0, 10.0, 3.0);
    CHECK(validateEndpointDescriptor(property(ValueType::UnsignedInteger, &offset)));
    for (uint64_t value = 0U; value <= 11U; ++value) {
        expectUnsigned(&offset, value,
                       value >= 1U && value <= 10U && (value - 1U) % 3U == 0U,
                       "unsigned minimum anchors step");
    }
    expectSigned(&offset, 1, true, "signed minimum is inclusive");
    expectSigned(&offset, 10, true, "signed maximum is inclusive");
    expectSigned(&offset, 11, false, "signed upper bound rejects");
}

void exactSafeDifferences() {
    const PropertyConstraints crossing = numeric(
        static_cast<uint8_t>(ConstraintMinimum | ConstraintMaximum | ConstraintStep),
        -static_cast<double>(safeMaximum), static_cast<double>(safeMaximum), 2.0);
    CHECK(validateEndpointDescriptor(property(ValueType::SignedInteger, &crossing)));
    expectSigned(&crossing, safeMaximum, true,
                 "full safe span even difference accepts");
    expectSigned(&crossing, safeMaximum - 1, false,
                 "full safe span adjacent odd difference rejects");
    expectSigned(&crossing, -safeMaximum, true, "negative safe boundary accepts");
    expectSigned(&crossing, -safeMaximum + 1, false,
                 "negative safe boundary adjacent rejects");
    expectSigned(&crossing, safeMaximum + 1, false,
                 "constrained value outside safe domain rejects");
    expectSigned(&crossing, -safeMaximum - 1, false,
                 "constrained negative outside safe domain rejects");

    const PropertyConstraints thirds = numeric(
        static_cast<uint8_t>(ConstraintMinimum | ConstraintStep),
        -static_cast<double>(safeMaximum), 0.0, 3.0);
    expectSigned(&thirds, safeMaximum - 2, true,
                 "safe span exact triple accepts");
    expectSigned(&thirds, safeMaximum - 1, false,
                 "safe span nonmultiple rejects despite large quotient");
    expectSigned(&thirds, safeMaximum, false,
                 "safe span second nonmultiple rejects");

    // No minimum means base zero, including negative signed values. All of
    // these quotients are large enough for the old tolerance to accept errors.
    const PropertyConstraints zeroBase = numeric(ConstraintStep, 0.0, 0.0, 3.0);
    expectSigned(&zeroBase, -safeMaximum + 1, true,
                 "large negative exact zero-based triple accepts");
    expectSigned(&zeroBase, -safeMaximum, false,
                 "large negative zero-based nonmultiple rejects");
    expectSigned(&zeroBase, -safeMaximum + 2, false,
                 "large negative adjacent nonmultiple rejects");
    expectUnsigned(&zeroBase, static_cast<uint64_t>(safeMaximum - 1), true,
                   "unsigned safe-domain exact triple accepts");
    expectUnsigned(&zeroBase, static_cast<uint64_t>(safeMaximum), false,
                   "unsigned safe-domain nonmultiple rejects");
    expectUnsigned(&zeroBase, static_cast<uint64_t>(safeMaximum + 1), false,
                   "constrained unsigned outside safe domain rejects");

    const PropertyConstraints negativeRange = numeric(
        static_cast<uint8_t>(ConstraintMinimum | ConstraintMaximum | ConstraintStep),
        -10.0, -1.0, 3.0);
    for (int64_t value = -11; value <= 0; ++value) {
        expectSigned(&negativeRange, value,
                     value >= -10 && value <= -1 && (value + 10) % 3 == 0,
                     "negative range bounds and offset step");
    }

    const PropertyConstraints maximumOnly = numeric(
        static_cast<uint8_t>(ConstraintMaximum | ConstraintStep),
        99.0, -1.0, 3.0);
    expectSigned(&maximumOnly, -3, true,
                 "maximum-only constraint retains zero step base");
    expectSigned(&maximumOnly, -1, false,
                 "maximum-only constraint rejects off-step maximum");
    expectSigned(&maximumOnly, 0, false,
                 "maximum-only constraint enforces upper bound");
    const PropertyConstraints boundsOnly = numeric(
        static_cast<uint8_t>(ConstraintMinimum | ConstraintMaximum),
        -1.0, 1.0, 0.0);
    expectSigned(&boundsOnly, -1, true, "numeric minimum without step remains inclusive");
    expectSigned(&boundsOnly, 1, true, "maximum numeric bound remains inclusive");
    expectSigned(&boundsOnly, 2, false, "numeric bound without step still rejects");
}

void invalidIntegerSteps() {
    const double invalid[] = {
        0.0, -1.0, 0.5, 1.5,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        9007199254740992.0, std::numeric_limits<double>::max()
    };
    for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        const PropertyConstraints constraints = numeric(
            ConstraintStep, 0.0, 0.0, invalid[index]);
        CHECK(validateEndpointDescriptor(property(ValueType::SignedInteger,
                                                  &constraints)).code() ==
              ErrorCode::ValueOutOfRange);
        // Call the shared value entry point directly, not only Registry::seal:
        // malformed step metadata must never reach unsafe casts or modulo zero.
        expectSigned(&constraints, 0, false, "invalid signed step fails closed");
        expectUnsigned(&constraints, 0U, false,
                       "invalid unsigned step fails closed");
    }
    const double invalidBases[] = {
        -0.5, std::numeric_limits<double>::quiet_NaN(),
        -std::numeric_limits<double>::infinity(), -9007199254740992.0
    };
    for (size_t index = 0U;
         index < sizeof(invalidBases) / sizeof(invalidBases[0]); ++index) {
        const PropertyConstraints constraints = numeric(
            static_cast<uint8_t>(ConstraintMinimum | ConstraintStep),
            invalidBases[index], 0.0, 1.0);
        expectSigned(&constraints, 0, false,
                     "malformed minimum cannot become integer step base");
    }
}

void unchangedWireAndFloatRules() {
    expectSigned(nullptr, INT64_MIN, true, "unconstrained full int64 minimum");
    expectSigned(nullptr, INT64_MAX, true, "unconstrained full int64 maximum");
    expectUnsigned(nullptr, UINT64_MAX, true, "unconstrained full uint64 maximum");

    uint8_t buffer[16] = {};
    cbor::Writer boolean(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(boolean.writeBool(true));
    const EndpointDescriptor boolEndpoint = property(ValueType::Boolean, nullptr);
    CHECK(validateEndpointValue(boolEndpoint, boolean.view(),
                                cbor::Type::Boolean, AccessWrite));
    const PropertyConstraints step = numeric(ConstraintStep, 0.0, 0.0, 3.0);
    CHECK(validateEndpointValue(property(ValueType::SignedInteger, &step),
                                boolean.view(), cbor::Type::Boolean,
                                AccessWrite).code() == ErrorCode::InvalidEncoding);
    cbor::Writer integer(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(integer.writeUnsigned(1U));
    CHECK(validateEndpointValue(boolEndpoint, integer.view(),
                                cbor::Type::Unsigned, AccessWrite).code() ==
          ErrorCode::InvalidEncoding);
    cbor::Writer negative(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(negative.writeInt(-3));
    CHECK(validateEndpointValue(property(ValueType::UnsignedInteger, &step),
                                negative.view(), cbor::Type::Negative,
                                AccessWrite).code() == ErrorCode::InvalidEncoding);

    const PropertyConstraints tenths = numeric(ConstraintStep, 0.0, 0.0, 0.1);
    cbor::Writer float32(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(float32.writeFloat32(0.3F));
    expectValue(property(ValueType::Float32, &tenths), float32.view(),
                cbor::Type::Float, true, "float32 existing tolerance accepts 0.3");
    cbor::Writer float64(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(float64.writeFloat64(0.30000000000000004));
    expectValue(property(ValueType::Float64, &tenths), float64.view(),
                cbor::Type::Float, true, "float64 existing tolerance accepts 0.3");
    cbor::Writer offStep(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(offStep.writeFloat64(0.31));
    expectValue(property(ValueType::Float64, &tenths), offStep.view(),
                cbor::Type::Float, false, "float64 existing off-step rejects");
}

void stateStoreRejectsAtomically() {
    const PropertyConstraints huge = numeric(ConstraintStep, 0.0, 0.0,
                                              1000000000001.0);
    const EndpointDescriptor fields[] = {
        EndpointDescriptor("power", EndpointKind::Property, ValueType::Boolean,
                           readWrite),
        property(ValueType::UnsignedInteger, &huge)
    };
    EndpointRegistry registry(fields, 2U);
    CHECK(registry.seal());
    EndpointStateSlot slots[2];
    uint8_t arena[32] = {};
    EndpointStateStore store(registry, slots, 2U,
                             MutableByteSpan(arena, sizeof(arena)));
    CHECK(store.validateStorage());
    uint8_t buffer[64] = {};
    bbp2::KeyedBodyWriter initial(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(initial.begin(2U));
    CHECK(initial.writeBool("power", false));
    CHECK(initial.writeUnsigned("level", 0U));
    ByteView body;
    CHECK(initial.finish(body));
    bool changed = false;
    CHECK(store.applyKeyed(body, AccessWrite, changed));
    CHECK(changed);
    const size_t beforeSize = store.usedBytes();
    uint8_t before[32] = {};
    std::memcpy(before, arena, sizeof(before));

    bbp2::KeyedBodyWriter invalid(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(invalid.begin(2U));
    CHECK(invalid.writeBool("power", true));
    CHECK(invalid.writeUnsigned("level", 1U));
    CHECK(invalid.finish(body));
    CHECK(store.applyKeyed(body, AccessWrite, changed).code() ==
          ErrorCode::ValueOutOfRange);
    CHECK(store.usedBytes() == beforeSize);
    CHECK(std::memcmp(before, arena, sizeof(before)) == 0);

    bbp2::IdBodyWriter invalidId(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(invalidId.begin(2U));
    CHECK(invalidId.writeBool(1U, true));
    CHECK(invalidId.writeUnsigned(2U, UINT32_MAX));
    CHECK(invalidId.finish(body));
    CHECK(store.applyById(body, AccessWrite, changed).code() ==
          ErrorCode::ValueOutOfRange);
    CHECK(store.usedBytes() == beforeSize);
    CHECK(std::memcmp(before, arena, sizeof(before)) == 0);

    // A valid update still commits, so this is not a blanket rejection gate.
    bbp2::IdBodyWriter validId(MutableByteSpan(buffer, sizeof(buffer)));
    CHECK(validId.begin(2U));
    CHECK(validId.writeBool(1U, true));
    CHECK(validId.writeUnsigned(2U, UINT64_C(1000000000001)));
    CHECK(validId.finish(body));
    CHECK(store.applyById(body, AccessWrite, changed));
    CHECK(changed);
    ByteView stored;
    cbor::Type type = cbor::Type::Invalid;
    CHECK(store.valueById(2U, stored, type));
    uint64_t level = 0U;
    cbor::Reader reader(stored);
    CHECK(reader.readUnsigned(level));
    CHECK(level == UINT64_C(1000000000001));
    CHECK(store.valueById(1U, stored, type));
    bool power = false;
    cbor::Reader powerReader(stored);
    CHECK(powerReader.readBool(power));
    CHECK(power);
}

} // namespace

int main() {
    hugeStepAndOffset();
    exactSafeDifferences();
    invalidIntegerSteps();
    unchangedWireAndFloatRules();
    stateStoreRejectsAtomically();
    std::cout << "endpoint integer step: " << values << " direct value cases, "
              << checks << " checks, " << failures << " failures\n";
    return failures == 0U ? 0 : 1;
}
