#ifndef BLINKER_MODEL_ENDPOINT_H
#define BLINKER_MODEL_ENDPOINT_H

#include "../core/Span.h"

namespace blinker {

static const size_t kMaxEndpointKeyLength = 64;
static const size_t kMaxPropertyUnitLength = 16;
static const size_t kMaxPropertyEnumValues = 16;

enum class EndpointKind : uint8_t {
    Property = 0,
    Action = 1,
    Event = 2,
    Invalid = 0xFF
};

enum class ValueType : uint8_t {
    Boolean = 0,
    SignedInteger = 1,
    UnsignedInteger = 2,
    Float32 = 3,
    Float64 = 4,
    Text = 5,
    Bytes = 6,
    Object = 7,
    Array = 8,
    Null = 9
};

enum PropertyAccess : uint8_t {
    AccessNone = 0,
    AccessRead = 1U << 0,
    AccessWrite = 1U << 1,
    AccessNotify = 1U << 2,
    AccessEvent = 1U << 3,
    // COMMAND is intentionally distinct from desired-state PATCH Apply.
    // A button gesture is an instruction, not authoritative device state.
    AccessCommand = 1U << 4
};

static const uint8_t kKnownPropertyAccess =
    AccessRead | AccessWrite | AccessNotify | AccessEvent | AccessCommand;

enum PropertyConstraintFlag : uint8_t {
    ConstraintNone = 0,
    ConstraintMinimum = 1U << 0,
    ConstraintMaximum = 1U << 1,
    ConstraintStep = 1U << 2,
    ConstraintMaxLength = 1U << 3,
    ConstraintUnit = 1U << 4,
    ConstraintEnumText = 1U << 5
};

static const uint8_t kKnownPropertyConstraintFlags =
    ConstraintMinimum | ConstraintMaximum | ConstraintStep |
    ConstraintMaxLength | ConstraintUnit | ConstraintEnumText;

// Numeric limits are finite IEEE-754 doubles. Constrained integer endpoints
// are limited to the exact cross-platform safe range (+/- (2^53 - 1)).
// maxLength uses bytes for text/bytes and item/pair count for arrays/maps.
struct PropertyConstraints {
    uint8_t flags;
    double minimum;
    double maximum;
    double step;
    uint32_t maxLength;
    StringView unit;
    const StringView* enumTextValues;
    size_t enumTextCount;

    constexpr PropertyConstraints()
        : flags(ConstraintNone),
          minimum(0.0),
          maximum(0.0),
          step(0.0),
          maxLength(0),
          unit(),
          enumTextValues(nullptr),
          enumTextCount(0) {}

    constexpr PropertyConstraints(
        uint8_t constraintFlags,
        double minimumValue,
        double maximumValue,
        double stepValue,
        uint32_t maximumLength,
        StringView valueUnit,
        const StringView* enumValues,
        size_t enumCount)
        : flags(constraintFlags),
          minimum(minimumValue),
          maximum(maximumValue),
          step(stepValue),
          maxLength(maximumLength),
          unit(valueUnit),
          enumTextValues(enumValues),
          enumTextCount(enumCount) {}
};

// One immutable descriptor is one independently addressable Property, Action
// or Event. It is intentionally flat: App widgets and PageSchema bindings are
// separate models and never appear in the device Manifest.
struct FieldSpec {
    StringView key;
    const PropertyConstraints* constraints;
    EndpointKind kind;
    ValueType type;
    uint8_t access;

    constexpr FieldSpec()
        : key(),
          constraints(nullptr),
          kind(EndpointKind::Property),
          type(ValueType::Boolean),
          access(AccessNone) {}

    constexpr FieldSpec(
        StringView fieldKey,
        EndpointKind fieldKind,
        ValueType valueType,
        uint8_t accessFlags,
        const PropertyConstraints* fieldConstraints = nullptr)
        : key(fieldKey),
          constraints(fieldConstraints),
          kind(fieldKind),
          type(valueType),
          access(accessFlags) {}
};

typedef FieldSpec EndpointDescriptor;

} // namespace blinker

#endif
