#include "Manifest.h"

#include <limits.h>

#include "../core/Sha256.h"

namespace blinker {

namespace {

Result discardSink(void* context, ByteView) {
    return context != nullptr
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result sha256Sink(void* context, ByteView bytes) {
    Sha256* hash = static_cast<Sha256*>(context);
    return hash != nullptr
               ? hash->update(bytes)
               : Result::failure(ErrorCode::InvalidArgument);
}

Result validateManifestLimits(
    const EndpointRegistry& registry,
    const cbor::Limits& limits,
    size_t rootPairs) {
    if (limits.maxDepth < 4U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (limits.maxContainerItems < rootPairs ||
        registry.size() > limits.maxContainerItems ||
        registry.size() > UINT16_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    return Result::success();
}

size_t fieldPairCount(const EndpointDescriptor& endpoint) {
    size_t count = 5U;
    count += endpoint.telemetryMinimumIntervalMs != 0U ? 1U : 0U;
    const PropertyConstraints* constraints = endpoint.constraints;
    if (constraints == nullptr) return count;
    count += (constraints->flags & ConstraintMinimum) != 0U ? 1U : 0U;
    count += (constraints->flags & ConstraintMaximum) != 0U ? 1U : 0U;
    count += (constraints->flags & ConstraintStep) != 0U ? 1U : 0U;
    count += (constraints->flags & ConstraintMaxLength) != 0U ? 1U : 0U;
    count += (constraints->flags & ConstraintUnit) != 0U ? 1U : 0U;
    count += (constraints->flags & ConstraintEnumText) != 0U ? 1U : 0U;
    return count;
}

Result encodeField(
    cbor::Writer& writer,
    const EndpointDescriptor& endpoint,
    size_t endpointIndex,
    const cbor::Limits& limits) {
    Result result = validateEndpointDescriptor(endpoint);
    if (!result) return result;
    if (endpoint.key.size > limits.maxTextLength ||
        endpointIndex >= UINT16_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    const size_t pairCount = fieldPairCount(endpoint);
    if (pairCount > limits.maxContainerItems) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    result = writer.beginMap(pairCount);
    if (result) result = writer.writeUnsigned(0);
    if (result) result = writer.writeText(endpoint.key);
    if (result) result = writer.writeUnsigned(1);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(endpoint.kind));
    }
    if (result) result = writer.writeUnsigned(2);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(endpoint.type));
    }
    if (result) result = writer.writeUnsigned(3);
    if (result) result = writer.writeUnsigned(endpoint.access);
    if (result) result = writer.writeUnsigned(4);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint16_t>(endpointIndex + 1U));
    }

    const PropertyConstraints* constraints = endpoint.constraints;
    if (result && constraints != nullptr &&
        (constraints->flags & ConstraintMinimum) != 0U) {
        result = writer.writeUnsigned(5);
        if (result) result = writer.writeFloat64(constraints->minimum);
    }
    if (result && constraints != nullptr &&
        (constraints->flags & ConstraintMaximum) != 0U) {
        result = writer.writeUnsigned(6);
        if (result) result = writer.writeFloat64(constraints->maximum);
    }
    if (result && constraints != nullptr &&
        (constraints->flags & ConstraintStep) != 0U) {
        result = writer.writeUnsigned(7);
        if (result) result = writer.writeFloat64(constraints->step);
    }
    if (result && constraints != nullptr &&
        (constraints->flags & ConstraintMaxLength) != 0U) {
        result = writer.writeUnsigned(8);
        if (result) result = writer.writeUnsigned(constraints->maxLength);
    }
    if (result && constraints != nullptr &&
        (constraints->flags & ConstraintUnit) != 0U) {
        if (constraints->unit.size > limits.maxTextLength) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        result = writer.writeUnsigned(9);
        if (result) result = writer.writeText(constraints->unit);
    }
    if (result && constraints != nullptr &&
        (constraints->flags & ConstraintEnumText) != 0U) {
        if (constraints->enumTextCount > limits.maxContainerItems) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        result = writer.writeUnsigned(10);
        if (result) result = writer.beginArray(constraints->enumTextCount);
        for (size_t enumIndex = 0;
             result && enumIndex < constraints->enumTextCount;
             ++enumIndex) {
            if (constraints->enumTextValues[enumIndex].size >
                limits.maxTextLength) {
                return Result::failure(ErrorCode::CapacityExceeded);
            }
            result = writer.writeText(constraints->enumTextValues[enumIndex]);
        }
    }
    if (result && endpoint.telemetryMinimumIntervalMs != 0U) {
        result = writer.writeUnsigned(11);
        if (result) {
            result = writer.writeUnsigned(
                endpoint.telemetryMinimumIntervalMs);
        }
    }
    return result;
}

Result encodeLogicalManifest(
    uint32_t revision,
    const EndpointRegistry& registry,
    cbor::Writer& writer,
    const cbor::Limits& limits) {
    Result result = validateManifestLimits(registry, limits, 2U);
    if (!result) return result;
    result = writer.beginMap(2);
    if (result) result = writer.writeUnsigned(0);
    if (result) result = writer.writeUnsigned(revision);
    if (result) result = writer.writeUnsigned(1);
    if (result) result = writer.beginArray(registry.size());
    for (size_t index = 0; result && index < registry.size(); ++index) {
        const EndpointDescriptor* endpoint = registry.at(index);
        if (endpoint == nullptr) {
            return Result::failure(ErrorCode::InternalError);
        }
        result = encodeField(writer, *endpoint, index, limits);
    }
    return result;
}

Result encodePageBody(
    uint32_t revision,
    const EndpointRegistry& registry,
    ByteView fingerprint,
    uint16_t cursor,
    uint16_t count,
    cbor::Writer& writer,
    const cbor::Limits& limits) {
    const uint16_t total = static_cast<uint16_t>(registry.size());
    const uint16_t next = static_cast<uint16_t>(cursor + count);
    Result result = writer.beginMap(6);
    if (result) result = writer.writeUnsigned(0);
    if (result) result = writer.writeUnsigned(revision);
    if (result) result = writer.writeUnsigned(1);
    if (result) result = writer.writeBytes(fingerprint);
    if (result) result = writer.writeUnsigned(2);
    if (result) result = writer.writeUnsigned(cursor);
    if (result) result = writer.writeUnsigned(3);
    if (result) result = writer.writeUnsigned(next);
    if (result) result = writer.writeUnsigned(4);
    if (result) result = writer.writeUnsigned(total);
    if (result) result = writer.writeUnsigned(5);
    if (result) result = writer.beginArray(count);
    for (uint16_t offset = 0; result && offset < count; ++offset) {
        const size_t index = static_cast<size_t>(cursor) + offset;
        const EndpointDescriptor* endpoint = registry.at(index);
        if (endpoint == nullptr) {
            return Result::failure(ErrorCode::InternalError);
        }
        result = encodeField(writer, *endpoint, index, limits);
    }
    return result;
}

} // namespace

Result encodeManifest(
    uint32_t revision,
    const EndpointRegistry& registry,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    cbor::Writer writer(output);
    Result result = encodeLogicalManifest(revision, registry, writer, limits);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result computeManifestFingerprint(
    uint32_t revision,
    const EndpointRegistry& registry,
    MutableByteSpan fingerprint,
    const cbor::Limits& limits) {
    if (fingerprint.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (fingerprint.size < kSha256Size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Sha256 hash;
    cbor::Writer writer(&sha256Sink, &hash);
    Result result = encodeLogicalManifest(revision, registry, writer, limits);
    if (result) result = hash.finish(fingerprint);
    return result;
}

Result computeManifestFingerprint(
    uint32_t revision,
    const EndpointRegistry& registry,
    MutableByteSpan,
    MutableByteSpan fingerprint,
    const cbor::Limits& limits) {
    return computeManifestFingerprint(revision, registry, fingerprint, limits);
}

Result encodeManifestPage(
    uint32_t revision,
    const EndpointRegistry& registry,
    ByteView fingerprint,
    uint16_t cursor,
    MutableByteSpan output,
    ByteView& encoded,
    uint16_t& nextCursor,
    const cbor::Limits& limits) {
    Result result = validateManifestLimits(registry, limits, 6U);
    if (!result) return result;
    if (fingerprint.data == nullptr || fingerprint.size != kSha256Size) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (fingerprint.size > limits.maxByteStringLength) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    const uint16_t total = static_cast<uint16_t>(registry.size());
    if (cursor > total) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }

    const uint16_t remaining = static_cast<uint16_t>(total - cursor);
    uint16_t selected = 0;
    uint8_t countContext = 0;
    for (size_t candidate = 0U;
         candidate <= static_cast<size_t>(remaining);
         ++candidate) {
        cbor::Writer counter(&discardSink, &countContext);
        result = encodePageBody(
            revision,
            registry,
            fingerprint,
            cursor,
            static_cast<uint16_t>(candidate),
            counter,
            limits);
        if (!result) return result;
        if (counter.size() > output.size) break;
        selected = static_cast<uint16_t>(candidate);
    }
    if (remaining != 0U && selected == 0U) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    cbor::Writer writer(output);
    result = encodePageBody(
        revision,
        registry,
        fingerprint,
        cursor,
        selected,
        writer,
        limits);
    if (!result) return result;
    encoded = writer.view();
    nextCursor = static_cast<uint16_t>(cursor + selected);
    return Result::success();
}

Result decodeManifestPage(
    ByteView encoded,
    ManifestPageView& page,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t pairCount = 0;
    result = reader.readMapSize(pairCount);
    if (!result) return result;
    if (pairCount > limits.maxContainerItems) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    ManifestPageView decoded;
    uint8_t seen = 0;
    for (size_t pair = 0; pair < pairCount; ++pair) {
        uint64_t key = 0;
        result = reader.readUnsigned(key);
        if (!result) return result;
        if (key <= 5U) {
            const uint8_t bit = static_cast<uint8_t>(1U << key);
            if ((seen & bit) != 0U) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            seen = static_cast<uint8_t>(seen | bit);
        }
        uint64_t value = 0;
        switch (key) {
            case 0:
                result = reader.readUnsigned(value);
                if (!result) return result;
                if (value > UINT32_MAX) {
                    return Result::failure(ErrorCode::ValueOutOfRange);
                }
                decoded.revision = static_cast<uint32_t>(value);
                break;
            case 1:
                result = reader.readBytes(decoded.fingerprint);
                if (!result) return result;
                if (decoded.fingerprint.size != kSha256Size) {
                    return Result::failure(ErrorCode::InvalidEncoding);
                }
                break;
            case 2:
            case 3:
            case 4:
                result = reader.readUnsigned(value);
                if (!result) return result;
                if (value > UINT16_MAX) {
                    return Result::failure(ErrorCode::ValueOutOfRange);
                }
                if (key == 2U) decoded.cursor = static_cast<uint16_t>(value);
                if (key == 3U) decoded.nextCursor = static_cast<uint16_t>(value);
                if (key == 4U) decoded.totalFields = static_cast<uint16_t>(value);
                break;
            case 5: {
                cbor::Type type = cbor::Type::Invalid;
                result = reader.captureValue(decoded.encodedFields, &type);
                if (!result) return result;
                if (type != cbor::Type::Array) {
                    return Result::failure(ErrorCode::InvalidEncoding);
                }
                break;
            }
            default:
                result = reader.skipValue();
                if (!result) return result;
                break;
        }
    }
    if (seen != 0x3FU || !reader.finished() ||
        decoded.cursor > decoded.nextCursor ||
        decoded.nextCursor > decoded.totalFields) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    cbor::Reader fields(decoded.encodedFields, limits);
    size_t fieldCount = 0;
    result = fields.readArraySize(fieldCount);
    if (!result) return result;
    if (fieldCount !=
            static_cast<size_t>(decoded.nextCursor - decoded.cursor) ||
        fieldCount > UINT16_MAX) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    for (size_t index = 0; index < fieldCount; ++index) {
        cbor::Type type = cbor::Type::Invalid;
        result = fields.peekType(type);
        if (!result) return result;
        if (type != cbor::Type::Map) {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        result = fields.skipValue();
        if (!result) return result;
    }
    if (!fields.finished()) {
        return Result::failure(ErrorCode::TrailingData);
    }
    decoded.fieldCount = static_cast<uint16_t>(fieldCount);
    page = decoded;
    return Result::success();
}

} // namespace blinker
