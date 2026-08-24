#include "EndpointValue.h"

#include <float.h>
#include <math.h>
#include <string.h>

namespace blinker {

namespace {

bool endpointValueEqual(StringView first, StringView second) {
    return first.size == second.size &&
           (first.size == 0U ||
            memcmp(first.data, second.data, first.size) == 0);
}

bool endpointValueFiniteNumber(double value) {
    return value == value && value <= DBL_MAX && value >= -DBL_MAX;
}

Result validateNumericConstraints(
    const PropertyConstraints& constraints,
    double value,
    ValueType type) {
    if (!endpointValueFiniteNumber(value)) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if ((constraints.flags & ConstraintMinimum) != 0U &&
        value < constraints.minimum) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if ((constraints.flags & ConstraintMaximum) != 0U &&
        value > constraints.maximum) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if ((constraints.flags & ConstraintStep) != 0U) {
        const double base =
            (constraints.flags & ConstraintMinimum) != 0U
                ? constraints.minimum
                : 0.0;
        const double quotient = (value - base) / constraints.step;
        const double nearest = quotient >= 0.0
                                   ? floor(quotient + 0.5)
                                   : ceil(quotient - 0.5);
        const double scale = fabs(quotient) > 1.0 ? fabs(quotient) : 1.0;
        const double tolerance =
            type == ValueType::Float32 ? 1.0e-6 * scale : 1.0e-12 * scale;
        if (fabs(quotient - nearest) > tolerance) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
    }
    return Result::success();
}

Result validatePropertyValue(
    const EndpointDescriptor& property,
    ByteView encoded,
    cbor::Type type,
    uint8_t requiredAnyAccess,
    const cbor::Limits& limits) {
    if ((property.access & requiredAnyAccess) == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Reader reader(encoded, limits);
    double numericValue = 0.0;
    size_t length = 0;
    StringView text;
    ByteView bytes;
    Result result = Result::success();
    switch (property.type) {
        case ValueType::Boolean: {
            bool value = false;
            if (type != cbor::Type::Boolean) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            result = reader.readBool(value);
            break;
        }
        case ValueType::SignedInteger: {
            if (type != cbor::Type::Unsigned &&
                type != cbor::Type::Negative) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            int64_t value = 0;
            result = reader.readInt(value);
            if (property.constraints != nullptr &&
                (value < INT64_C(-9007199254740991) ||
                 value > INT64_C(9007199254740991))) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            numericValue = static_cast<double>(value);
            break;
        }
        case ValueType::UnsignedInteger: {
            if (type != cbor::Type::Unsigned) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            uint64_t value = 0;
            result = reader.readUnsigned(value);
            if (property.constraints != nullptr &&
                value > UINT64_C(9007199254740991)) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            numericValue = static_cast<double>(value);
            break;
        }
        case ValueType::Float32:
            if (type != cbor::Type::Float || encoded.size == 0U ||
                encoded.data[0] != 0xFAU) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            result = reader.readFloat(numericValue);
            break;
        case ValueType::Float64:
            if (type != cbor::Type::Float) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            result = reader.readFloat(numericValue);
            break;
        case ValueType::Text:
            if (type != cbor::Type::Text) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            result = reader.readText(text);
            length = text.size;
            break;
        case ValueType::Bytes:
            if (type != cbor::Type::Bytes) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            result = reader.readBytes(bytes);
            length = bytes.size;
            break;
        case ValueType::Object:
            if (type != cbor::Type::Map) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            result = reader.readMapSize(length);
            while (result && !reader.finished()) result = reader.skipValue();
            break;
        case ValueType::Array:
            if (type != cbor::Type::Array) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            result = reader.readArraySize(length);
            while (result && !reader.finished()) result = reader.skipValue();
            break;
        case ValueType::Null:
            if (type != cbor::Type::Null) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            result = reader.readNull();
            break;
        default:
            return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (!result || !reader.finished()) {
        return result ? Result::failure(ErrorCode::TrailingData) : result;
    }
    if (property.constraints == nullptr) return Result::success();
    const PropertyConstraints& constraints = *property.constraints;
    if (property.type == ValueType::SignedInteger ||
        property.type == ValueType::UnsignedInteger ||
        property.type == ValueType::Float32 ||
        property.type == ValueType::Float64) {
        result = validateNumericConstraints(
            constraints,
            numericValue,
            property.type);
        if (!result) return result;
    }
    if ((constraints.flags & ConstraintMaxLength) != 0U &&
        length > constraints.maxLength) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if ((constraints.flags & ConstraintEnumText) != 0U) {
        bool found = false;
        for (size_t index = 0; index < constraints.enumTextCount; ++index) {
        if (endpointValueEqual(text, constraints.enumTextValues[index])) {
                found = true;
                break;
            }
        }
        if (!found) return Result::failure(ErrorCode::ValueOutOfRange);
    }
    return Result::success();
}

} // namespace

Result validateEndpointValue(
    const EndpointDescriptor& endpoint,
    ByteView encodedValue,
    cbor::Type valueType,
    uint8_t requiredAnyAccess,
    const cbor::Limits& limits) {
    if (requiredAnyAccess == AccessNone ||
        (requiredAnyAccess & ~kKnownPropertyAccess) != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return validatePropertyValue(
        endpoint,
        encodedValue,
        valueType,
        requiredAnyAccess,
        limits);
}

} // namespace blinker
