#include "Messages.h"

#include <limits.h>

namespace blinker {
namespace bbp2 {

namespace {

Result requireFinished(const cbor::Reader& reader) {
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

Result readKey(cbor::Reader& reader, uint64_t& key) {
    return reader.readUnsigned(key);
}

Result writeKey(cbor::Writer& writer, uint8_t key) {
    return writer.writeUnsigned(key);
}

} // namespace

Result encodeHelloBody(
    const HelloBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    const bool authenticationFeature =
        (body.features & FeatureAuthentication) != 0U;
    const bool reliableFeature =
        (body.features & FeatureReliableDelivery) != 0U;
    const bool manifestFeature =
        (body.features & FeatureManifest) != 0U;
    if (body.versionCount == 0U || body.versionCount > kMaxHelloVersions ||
        body.maxFrameSize < kBaseHeaderSize ||
        body.maxReassemblySize < body.maxFrameSize ||
        static_cast<uint8_t>(body.role) > static_cast<uint8_t>(PeerRole::Server) ||
        ((body.features & FeatureEndpointIds) != 0U && !manifestFeature) ||
        body.hasManifestRevision != body.hasManifestFingerprint ||
        (body.hasManifestRevision && !manifestFeature) ||
        (body.hasManifestFingerprint &&
         body.manifestFingerprint.size != kManifestFingerprintSize) ||
        authenticationFeature != (body.authorizationMethodCount != 0U) ||
        body.authorizationMethodCount > kMaxAuthorizationMethods ||
        reliableFeature != (body.reliableReceiveWindow != 0U) ||
        body.reliableReceiveWindow > kMaxReliableReceiveWindow) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    for (uint8_t index = 0;
         index < body.authorizationMethodCount;
         ++index) {
        if (body.authorizationMethods[index] == 0U) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        for (uint8_t prior = 0; prior < index; ++prior) {
            if (body.authorizationMethods[prior] ==
                body.authorizationMethods[index]) {
                return Result::failure(ErrorCode::DuplicateField);
            }
        }
    }
    for (uint8_t index = 0; index < body.versionCount; ++index) {
        if (body.versions[index] == 0U) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        for (uint8_t prior = 0; prior < index; ++prior) {
            if (body.versions[prior] == body.versions[index]) {
                return Result::failure(ErrorCode::DuplicateField);
            }
        }
    }

    size_t fieldCount = 5;
    fieldCount += body.hasManifestRevision ? 1U : 0U;
    fieldCount += body.hasManifestFingerprint ? 1U : 0U;
    fieldCount += authenticationFeature ? 1U : 0U;
    fieldCount += reliableFeature ? 1U : 0U;
    if (limits.maxDepth < 2U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (fieldCount > limits.maxContainerItems ||
        body.versionCount > limits.maxContainerItems ||
        body.authorizationMethodCount > limits.maxContainerItems ||
        (body.hasManifestFingerprint &&
         body.manifestFingerprint.size > limits.maxByteStringLength)) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    cbor::Writer writer(output);
    Result result = writer.beginMap(fieldCount);
    if (result) result = writeKey(writer, 0);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.role));
    if (result) result = writeKey(writer, 1);
    if (result) result = writer.beginArray(body.versionCount);
    for (uint8_t index = 0; result && index < body.versionCount; ++index) {
        result = writer.writeUnsigned(body.versions[index]);
    }
    if (result) result = writeKey(writer, 2);
    if (result) result = writer.writeUnsigned(body.features);
    if (result) result = writeKey(writer, 3);
    if (result) result = writer.writeUnsigned(body.maxFrameSize);
    if (result) result = writeKey(writer, 4);
    if (result) result = writer.writeUnsigned(body.maxReassemblySize);
    if (result && body.hasManifestRevision) result = writeKey(writer, 6);
    if (result && body.hasManifestRevision) {
        result = writer.writeUnsigned(body.manifestRevision);
    }
    if (result && body.hasManifestFingerprint) result = writeKey(writer, 7);
    if (result && body.hasManifestFingerprint) {
        result = writer.writeBytes(body.manifestFingerprint);
    }
    if (result && authenticationFeature) result = writeKey(writer, 8);
    if (result && authenticationFeature) {
        result = writer.beginArray(body.authorizationMethodCount);
    }
    for (uint8_t index = 0;
         result && index < body.authorizationMethodCount;
         ++index) {
        result = writer.writeUnsigned(body.authorizationMethods[index]);
    }
    if (result && reliableFeature) result = writeKey(writer, 9);
    if (result && reliableFeature) {
        result = writer.writeUnsigned(body.reliableReceiveWindow);
    }
    if (!result) {
        return result;
    }
    encoded = writer.view();
    return Result::success();
}

Result decodeHelloBody(
    ByteView encoded,
    HelloBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t fieldCount = 0;
    result = reader.readMapSize(fieldCount);
    if (!result) return result;

    HelloBody decoded;
    uint16_t seen = 0;
    for (size_t field = 0; field < fieldCount; ++field) {
        uint64_t key = 0;
        result = readKey(reader, key);
        if (!result) return result;
        if (key <= 9U) {
            const uint16_t bit = static_cast<uint16_t>(1U << key);
            if ((seen & bit) != 0U) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            seen = static_cast<uint16_t>(seen | bit);
        }

        uint64_t value = 0;
        switch (key) {
            case 0:
                result = reader.readUnsigned(value);
                if (!result) return result;
                if (value > static_cast<uint8_t>(PeerRole::Server)) {
                    return Result::failure(ErrorCode::ValueOutOfRange);
                }
                decoded.role = static_cast<PeerRole>(value);
                break;
            case 1: {
                size_t count = 0;
                result = reader.readArraySize(count);
                if (!result) return result;
                if (count == 0U || count > kMaxHelloVersions) {
                    return Result::failure(ErrorCode::CapacityExceeded);
                }
                decoded.versionCount = static_cast<uint8_t>(count);
                for (size_t index = 0; index < count; ++index) {
                    result = reader.readUnsigned(value);
                    if (!result) return result;
                    if (value == 0U || value > UINT8_MAX) {
                        return Result::failure(ErrorCode::ValueOutOfRange);
                    }
                    decoded.versions[index] = static_cast<uint8_t>(value);
                    for (size_t prior = 0; prior < index; ++prior) {
                        if (decoded.versions[prior] == decoded.versions[index]) {
                            return Result::failure(ErrorCode::DuplicateField);
                        }
                    }
                }
                break;
            }
            case 2:
                result = reader.readUnsigned(value);
                if (!result) return result;
                if (value > UINT32_MAX) {
                    return Result::failure(ErrorCode::ValueOutOfRange);
                }
                decoded.features = static_cast<uint32_t>(value);
                break;
            case 3:
                result = reader.readUnsigned(value);
                if (!result) return result;
                if (value < kBaseHeaderSize || value > UINT16_MAX) {
                    return Result::failure(ErrorCode::ValueOutOfRange);
                }
                decoded.maxFrameSize = static_cast<uint16_t>(value);
                break;
            case 4:
                result = reader.readUnsigned(value);
                if (!result) return result;
                if (value == 0U || value > UINT32_MAX) {
                    return Result::failure(ErrorCode::ValueOutOfRange);
                }
                decoded.maxReassemblySize = static_cast<uint32_t>(value);
                break;
            case 6:
                result = reader.readUnsigned(value);
                if (!result) return result;
                if (value > UINT32_MAX) {
                    return Result::failure(ErrorCode::ValueOutOfRange);
                }
                decoded.manifestRevision = static_cast<uint32_t>(value);
                decoded.hasManifestRevision = true;
                break;
            case 7:
                result = reader.readBytes(decoded.manifestFingerprint);
                if (!result) return result;
                if (decoded.manifestFingerprint.size !=
                    kManifestFingerprintSize) {
                    return Result::failure(ErrorCode::InvalidEncoding);
                }
                decoded.hasManifestFingerprint = true;
                break;
            case 8: {
                size_t count = 0;
                result = reader.readArraySize(count);
                if (!result) return result;
                if (count == 0U || count > kMaxAuthorizationMethods) {
                    return Result::failure(ErrorCode::CapacityExceeded);
                }
                decoded.authorizationMethodCount =
                    static_cast<uint8_t>(count);
                for (size_t index = 0; index < count; ++index) {
                    result = reader.readUnsigned(value);
                    if (!result) return result;
                    if (value == 0U || value > UINT16_MAX) {
                        return Result::failure(ErrorCode::ValueOutOfRange);
                    }
                    decoded.authorizationMethods[index] =
                        static_cast<uint16_t>(value);
                    for (size_t prior = 0; prior < index; ++prior) {
                        if (decoded.authorizationMethods[prior] ==
                            decoded.authorizationMethods[index]) {
                            return Result::failure(ErrorCode::DuplicateField);
                        }
                    }
                }
                break;
            }
            case 9:
                result = reader.readUnsigned(value);
                if (!result) return result;
                if (value == 0U || value > kMaxReliableReceiveWindow) {
                    return Result::failure(ErrorCode::ValueOutOfRange);
                }
                decoded.reliableReceiveWindow = static_cast<uint8_t>(value);
                break;
            default:
                result = reader.skipValue();
                if (!result) return result;
                break;
        }
    }
    if ((seen & 0x1FU) != 0x1FU) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (((decoded.features & FeatureAuthentication) != 0U) !=
        (decoded.authorizationMethodCount != 0U)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (((decoded.features & FeatureReliableDelivery) != 0U) !=
        (decoded.reliableReceiveWindow != 0U)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    const bool manifestFeature =
        (decoded.features & FeatureManifest) != 0U;
    if (((decoded.features & FeatureEndpointIds) != 0U &&
         !manifestFeature) ||
        decoded.hasManifestRevision != decoded.hasManifestFingerprint ||
        (decoded.hasManifestRevision && !manifestFeature)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (decoded.maxReassemblySize < decoded.maxFrameSize) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    result = requireFinished(reader);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeManifestRequestBody(
    const ManifestRequestBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    if (limits.maxDepth < 1U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (limits.maxContainerItems < 1U) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(1);
    if (result) result = writeKey(writer, 0);
    if (result) result = writer.writeUnsigned(body.cursor);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeManifestRequestBody(
    ByteView encoded,
    ManifestRequestBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    ManifestRequestBody decoded;
    bool hasCursor = false;
    for (size_t index = 0; index < count; ++index) {
        uint64_t key = 0;
        result = readKey(reader, key);
        if (!result) return result;
        if (key == 0U) {
            if (hasCursor) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            uint64_t value = 0;
            result = reader.readUnsigned(value);
            if (!result) return result;
            if (value > UINT16_MAX) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            decoded.cursor = static_cast<uint16_t>(value);
            hasCursor = true;
        } else {
            result = reader.skipValue();
            if (!result) return result;
        }
    }
    if (!hasCursor) return Result::failure(ErrorCode::NotConfigured);
    result = requireFinished(reader);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeManifestAcceptBody(
    const ManifestAcceptBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    if (body.fingerprint.data == nullptr ||
        body.fingerprint.size != kManifestFingerprintSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (limits.maxDepth < 1U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (limits.maxContainerItems < 2U ||
        limits.maxByteStringLength < kManifestFingerprintSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(2);
    if (result) result = writeKey(writer, 0);
    if (result) result = writer.writeUnsigned(body.revision);
    if (result) result = writeKey(writer, 1);
    if (result) result = writer.writeBytes(body.fingerprint);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeManifestAcceptBody(
    ByteView encoded,
    ManifestAcceptBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    ManifestAcceptBody decoded;
    uint8_t seen = 0;
    for (size_t index = 0; index < count; ++index) {
        uint64_t key = 0;
        result = readKey(reader, key);
        if (!result) return result;
        if (key <= 1U) {
            const uint8_t bit = static_cast<uint8_t>(1U << key);
            if ((seen & bit) != 0U) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            seen = static_cast<uint8_t>(seen | bit);
        }
        if (key == 0U) {
            uint64_t value = 0;
            result = reader.readUnsigned(value);
            if (!result) return result;
            if (value > UINT32_MAX) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            decoded.revision = static_cast<uint32_t>(value);
        } else if (key == 1U) {
            result = reader.readBytes(decoded.fingerprint);
            if (!result) return result;
            if (decoded.fingerprint.size != kManifestFingerprintSize) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
        } else {
            result = reader.skipValue();
            if (!result) return result;
        }
    }
    if (seen != 0x03U) return Result::failure(ErrorCode::NotConfigured);
    result = requireFinished(reader);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeStateRequestBody(
    const StateRequestBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    const bool initial = body.cursor == 0U;
    if (initial == body.hasObservedRevision) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t count = body.hasObservedRevision ? 2U : 1U;
    if (limits.maxDepth < 1U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (limits.maxContainerItems < count) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(count);
    if (result) result = writeKey(writer, 0);
    if (result) result = writer.writeUnsigned(body.cursor);
    if (result && body.hasObservedRevision) result = writeKey(writer, 1);
    if (result && body.hasObservedRevision) {
        result = writer.writeUnsigned(body.observedRevision);
    }
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeStateRequestBody(
    ByteView encoded,
    StateRequestBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    StateRequestBody decoded;
    uint8_t seen = 0U;
    for (size_t index = 0; index < count; ++index) {
        uint64_t key = 0;
        result = readKey(reader, key);
        if (!result) return result;
        if (key <= 1U) {
            const uint8_t bit = static_cast<uint8_t>(1U << key);
            if ((seen & bit) != 0U) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            seen = static_cast<uint8_t>(seen | bit);
            uint64_t value = 0;
            result = reader.readUnsigned(value);
            if (!result) return result;
            if ((key == 0U && value > UINT16_MAX) ||
                (key == 1U && value > UINT32_MAX)) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            if (key == 0U) {
                decoded.cursor = static_cast<uint16_t>(value);
            } else {
                decoded.observedRevision = static_cast<uint32_t>(value);
                decoded.hasObservedRevision = true;
            }
        } else {
            result = reader.skipValue();
            if (!result) return result;
        }
    }
    if ((seen & 0x01U) == 0U || !reader.finished() ||
        ((decoded.cursor == 0U) == decoded.hasObservedRevision)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    body = decoded;
    return Result::success();
}

Result encodeStatePageBody(
    const StatePageBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    if (body.cursor > body.nextCursor ||
        body.nextCursor > body.totalFields ||
        (body.cursor < body.totalFields &&
         body.nextCursor == body.cursor) ||
        body.values.data == nullptr || body.values.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (limits.maxDepth < 2U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (limits.maxContainerItems < 5U) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Limits valueLimits = limits;
    --valueLimits.maxDepth;
    cbor::Reader valuesReader(body.values, valueLimits);
    size_t valueCount = 0U;
    Result result = valuesReader.readMapSize(valueCount);
    if (!result) return result;
    if (valueCount >
        static_cast<size_t>(body.nextCursor - body.cursor)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    result = cbor::validate(body.values, valueLimits);
    if (!result) return result;

    cbor::Writer writer(output);
    result = writer.beginMap(5);
    if (result) result = writeKey(writer, 0);
    if (result) result = writer.writeUnsigned(body.revision);
    if (result) result = writeKey(writer, 1);
    if (result) result = writer.writeUnsigned(body.cursor);
    if (result) result = writeKey(writer, 2);
    if (result) result = writer.writeUnsigned(body.nextCursor);
    if (result) result = writeKey(writer, 3);
    if (result) result = writer.writeUnsigned(body.totalFields);
    if (result) result = writeKey(writer, 4);
    if (result) result = writer.writeEncodedValue(body.values, valueLimits);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeStatePageBody(
    ByteView encoded,
    StatePageBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    result = reader.readMapSize(count);
    if (!result) return result;
    StatePageBody decoded;
    uint8_t seen = 0U;
    for (size_t index = 0; index < count; ++index) {
        uint64_t key = 0U;
        result = readKey(reader, key);
        if (!result) return result;
        if (key <= 4U) {
            const uint8_t bit = static_cast<uint8_t>(1U << key);
            if ((seen & bit) != 0U) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            seen = static_cast<uint8_t>(seen | bit);
        }
        if (key <= 3U) {
            uint64_t value = 0U;
            result = reader.readUnsigned(value);
            if (!result) return result;
            if ((key == 0U && value > UINT32_MAX) ||
                (key != 0U && value > UINT16_MAX)) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            if (key == 0U) decoded.revision = static_cast<uint32_t>(value);
            if (key == 1U) decoded.cursor = static_cast<uint16_t>(value);
            if (key == 2U) decoded.nextCursor = static_cast<uint16_t>(value);
            if (key == 3U) decoded.totalFields = static_cast<uint16_t>(value);
        } else if (key == 4U) {
            cbor::Type type = cbor::Type::Invalid;
            result = reader.captureValue(decoded.values, &type);
            if (!result) return result;
            if (type != cbor::Type::Map) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
        } else {
            result = reader.skipValue();
            if (!result) return result;
        }
    }
    if (seen != 0x1FU || !reader.finished() ||
        decoded.cursor > decoded.nextCursor ||
        decoded.nextCursor > decoded.totalFields ||
        (decoded.cursor < decoded.totalFields &&
         decoded.nextCursor == decoded.cursor)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    cbor::Limits valueLimits = limits;
    if (valueLimits.maxDepth == 0U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    --valueLimits.maxDepth;
    cbor::Reader valuesReader(decoded.values, valueLimits);
    size_t valueCount = 0U;
    result = valuesReader.readMapSize(valueCount);
    if (!result) return result;
    if (valueCount >
        static_cast<size_t>(decoded.nextCursor - decoded.cursor)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    result = cbor::validate(decoded.values, valueLimits);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeAckBody(
    const AckBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    if (limits.maxDepth < 1U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    const size_t count = body.hasStateRevision ? 2U : 1U;
    if (limits.maxContainerItems < count) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(count);
    if (result) result = writeKey(writer, 0);
    if (result) result = writer.writeUnsigned(body.acknowledgedSequence);
    if (result && body.hasStateRevision) result = writeKey(writer, 1);
    if (result && body.hasStateRevision) {
        result = writer.writeUnsigned(body.stateRevision);
    }
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeAckBody(
    ByteView encoded,
    AckBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    uint8_t seen = 0;
    AckBody decoded;
    for (size_t index = 0; index < count; ++index) {
        uint64_t key = 0;
        result = readKey(reader, key);
        if (!result) return result;
        if (key <= 1U) {
            const uint8_t bit = static_cast<uint8_t>(1U << key);
            if ((seen & bit) != 0U) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            seen = static_cast<uint8_t>(seen | bit);
            uint64_t value = 0;
            result = reader.readUnsigned(value);
            if (!result) return result;
            if ((key == 0U && value > UINT16_MAX) || value > UINT32_MAX) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            if (key == 0U) {
                decoded.acknowledgedSequence = static_cast<uint16_t>(value);
            } else {
                decoded.stateRevision = static_cast<uint32_t>(value);
                decoded.hasStateRevision = true;
            }
        } else {
            result = reader.skipValue();
            if (!result) return result;
        }
    }
    if ((seen & 0x01U) == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    result = requireFinished(reader);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeErrorBody(
    const ErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    if (body.errorCode == 0U || (body.hasDetail && body.detail.empty())) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    size_t count = 1;
    count += body.hasRelatedSequence ? 1U : 0U;
    count += body.hasDetail ? 1U : 0U;
    count += body.hasStateRevision ? 1U : 0U;
    if (limits.maxDepth < 1U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (count > limits.maxContainerItems ||
        (body.hasDetail && body.detail.size > limits.maxTextLength)) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(count);
    if (result) result = writeKey(writer, 0);
    if (result) result = writer.writeUnsigned(body.errorCode);
    if (result && body.hasRelatedSequence) result = writeKey(writer, 1);
    if (result && body.hasRelatedSequence) {
        result = writer.writeUnsigned(body.relatedSequence);
    }
    if (result && body.hasDetail) result = writeKey(writer, 2);
    if (result && body.hasDetail) result = writer.writeText(body.detail);
    if (result && body.hasStateRevision) result = writeKey(writer, 3);
    if (result && body.hasStateRevision) {
        result = writer.writeUnsigned(body.stateRevision);
    }
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeErrorBody(
    ByteView encoded,
    ErrorBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    uint8_t seen = 0;
    ErrorBody decoded;
    for (size_t index = 0; index < count; ++index) {
        uint64_t key = 0;
        result = readKey(reader, key);
        if (!result) return result;
        if (key <= 3U) {
            const uint8_t bit = static_cast<uint8_t>(1U << key);
            if ((seen & bit) != 0U) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            seen = static_cast<uint8_t>(seen | bit);
        }
        uint64_t value = 0;
        if (key == 0U || key == 1U || key == 3U) {
            result = reader.readUnsigned(value);
            if (!result) return result;
            if ((key <= 1U && value > UINT16_MAX) || value > UINT32_MAX ||
                (key == 0U && value == 0U)) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            if (key == 0U) {
                decoded.errorCode = static_cast<uint16_t>(value);
            } else if (key == 1U) {
                decoded.relatedSequence = static_cast<uint16_t>(value);
                decoded.hasRelatedSequence = true;
            } else {
                decoded.stateRevision = static_cast<uint32_t>(value);
                decoded.hasStateRevision = true;
            }
        } else if (key == 2U) {
            result = reader.readText(decoded.detail);
            if (!result) return result;
            if (decoded.detail.empty()) {
                return Result::failure(ErrorCode::InvalidArgument);
            }
            decoded.hasDetail = true;
        } else {
            result = reader.skipValue();
            if (!result) return result;
        }
    }
    if ((seen & 0x01U) == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    result = requireFinished(reader);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeStatePatchBody(
    const StatePatchBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    if (static_cast<uint8_t>(body.mode) >
            static_cast<uint8_t>(StatePatchMode::Apply) ||
        body.values.empty() || body.values.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (limits.maxDepth < 2U) {
        return Result::failure(ErrorCode::NestingTooDeep);
    }
    if (limits.maxContainerItems < 3U) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Limits valueLimits = limits;
    --valueLimits.maxDepth;
    cbor::Reader valuesReader(body.values, valueLimits);
    size_t valueCount = 0;
    Result result = valuesReader.readMapSize(valueCount);
    if (!result) return result;
    result = cbor::validate(body.values, valueLimits);
    if (!result) return result;

    cbor::Writer writer(output);
    result = writer.beginMap(3);
    if (result) result = writeKey(writer, 0);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.mode));
    }
    if (result) result = writeKey(writer, 1);
    if (result) result = writer.writeUnsigned(body.revision);
    if (result) result = writeKey(writer, 2);
    if (result) result = writer.writeEncodedValue(body.values, valueLimits);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeStatePatchBody(
    ByteView encoded,
    StatePatchBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    uint8_t seen = 0;
    StatePatchBody decoded;
    for (size_t index = 0; index < count; ++index) {
        uint64_t key = 0;
        result = readKey(reader, key);
        if (!result) return result;
        if (key <= 2U) {
            const uint8_t bit = static_cast<uint8_t>(1U << key);
            if ((seen & bit) != 0U) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            seen = static_cast<uint8_t>(seen | bit);
        }
        uint64_t value = 0;
        if (key == 0U) {
            result = reader.readUnsigned(value);
            if (!result) return result;
            if (value > static_cast<uint8_t>(StatePatchMode::Apply)) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            decoded.mode = static_cast<StatePatchMode>(value);
        } else if (key == 1U) {
            result = reader.readUnsigned(value);
            if (!result) return result;
            if (value > UINT32_MAX) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            decoded.revision = static_cast<uint32_t>(value);
        } else if (key == 2U) {
            cbor::Type type = cbor::Type::Invalid;
            result = reader.captureValue(decoded.values, &type);
            if (!result) return result;
            if (type != cbor::Type::Map) {
                return Result::failure(ErrorCode::InvalidEncoding);
            }
        } else {
            result = reader.skipValue();
            if (!result) return result;
        }
    }
    if ((seen & 0x07U) != 0x07U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    result = requireFinished(reader);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeAuthRequestBody(
    const AuthRequestBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    if (body.method == 0U ||
        (body.payload.size != 0U && body.payload.data == nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (limits.maxContainerItems < 2U ||
        body.payload.size > limits.maxByteStringLength) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(2);
    if (result) result = writeKey(writer, 0);
    if (result) result = writer.writeUnsigned(body.method);
    if (result) result = writeKey(writer, 1);
    if (result) result = writer.writeBytes(body.payload);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeAuthRequestBody(
    ByteView encoded,
    AuthRequestBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    bool hasMethod = false;
    bool hasPayload = false;
    AuthRequestBody decoded;
    for (size_t index = 0; index < count; ++index) {
        uint64_t key = 0;
        result = readKey(reader, key);
        if (!result) return result;
        if (key == 0U) {
            if (hasMethod) return Result::failure(ErrorCode::DuplicateField);
            uint64_t method = 0;
            result = reader.readUnsigned(method);
            if (!result) return result;
            if (method == 0U || method > UINT16_MAX) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            decoded.method = static_cast<uint16_t>(method);
            hasMethod = true;
        } else if (key == 1U) {
            if (hasPayload) return Result::failure(ErrorCode::DuplicateField);
            result = reader.readBytes(decoded.payload);
            if (!result) return result;
            hasPayload = true;
        } else {
            result = reader.skipValue();
            if (!result) return result;
        }
    }
    if (!hasMethod || !hasPayload) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    result = requireFinished(reader);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeAuthResultBody(
    const AuthResultBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    if (body.method == 0U ||
        static_cast<uint8_t>(body.status) >
            static_cast<uint8_t>(AuthStatus::Locked) ||
        (body.hasPayload && body.payload.data == nullptr &&
         body.payload.size != 0U)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t count = body.hasPayload ? 3U : 2U;
    if (count > limits.maxContainerItems ||
        (body.hasPayload &&
         body.payload.size > limits.maxByteStringLength)) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(count);
    if (result) result = writeKey(writer, 0);
    if (result) result = writer.writeUnsigned(body.method);
    if (result) result = writeKey(writer, 1);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.status));
    }
    if (result && body.hasPayload) result = writeKey(writer, 2);
    if (result && body.hasPayload) result = writer.writeBytes(body.payload);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeAuthResultBody(
    ByteView encoded,
    AuthResultBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0;
    result = reader.readMapSize(count);
    if (!result) return result;
    bool hasMethod = false;
    bool hasStatus = false;
    AuthResultBody decoded;
    for (size_t index = 0; index < count; ++index) {
        uint64_t key = 0;
        result = readKey(reader, key);
        if (!result) return result;
        if (key == 0U) {
            if (hasMethod) return Result::failure(ErrorCode::DuplicateField);
            uint64_t method = 0;
            result = reader.readUnsigned(method);
            if (!result) return result;
            if (method == 0U || method > UINT16_MAX) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            decoded.method = static_cast<uint16_t>(method);
            hasMethod = true;
        } else if (key == 1U) {
            if (hasStatus) return Result::failure(ErrorCode::DuplicateField);
            uint64_t status = 0;
            result = reader.readUnsigned(status);
            if (!result) return result;
            if (status > static_cast<uint8_t>(AuthStatus::Locked)) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            decoded.status = static_cast<AuthStatus>(status);
            hasStatus = true;
        } else if (key == 2U) {
            if (decoded.hasPayload) {
                return Result::failure(ErrorCode::DuplicateField);
            }
            result = reader.readBytes(decoded.payload);
            if (!result) return result;
            decoded.hasPayload = true;
        } else {
            result = reader.skipValue();
            if (!result) return result;
        }
    }
    if (!hasMethod || !hasStatus) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    result = requireFinished(reader);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeControllerControlOpenBody(
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    encoded = ByteView();
    if (limits.maxContainerItems < 1U) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(1U);
    if (result) {
        result = writer.writeUnsigned(kControllerControlWireVersion);
    }
    if (result) encoded = writer.view();
    return result;
}

Result decodeControllerControlOpenBody(
    ByteView encoded,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t version = 0U;
    if (result) result = reader.readArraySize(count);
    if (result && count != 1U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = reader.readUnsigned(version);
    if (result && version != kControllerControlWireVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    return result ? requireFinished(reader) : result;
}

Result encodeControllerControlChallengeBody(
    const ControllerControlChallengeBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    encoded = ByteView();
    if (body.controlNonce.data == nullptr ||
        body.controlNonce.size != kControllerControlNonceSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (limits.maxContainerItems < 2U ||
        limits.maxByteStringLength < kControllerControlNonceSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(2U);
    if (result) {
        result = writer.writeUnsigned(kControllerControlWireVersion);
    }
    if (result) result = writer.writeBytes(body.controlNonce);
    if (result) encoded = writer.view();
    return result;
}

Result decodeControllerControlChallengeBody(
    ByteView encoded,
    ControllerControlChallengeBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t version = 0U;
    ControllerControlChallengeBody decoded;
    if (result) result = reader.readArraySize(count);
    if (result && count != 2U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = reader.readUnsigned(version);
    if (result && version != kControllerControlWireVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (result) result = reader.readBytes(decoded.controlNonce);
    if (result && decoded.controlNonce.size != kControllerControlNonceSize) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = requireFinished(reader);
    if (result) body = decoded;
    return result;
}

Result encodeControllerMutationBody(
    const ControllerMutationBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits) {
    encoded = ByteView();
    const bool secretSizeValid = body.controllerSecret.empty() ||
        body.controllerSecret.size == kControllerCredentialSecretSize;
    if (body.grant.data == nullptr || body.grant.empty() ||
        body.grant.size > kControllerGrantMaximumSize ||
        !secretSizeValid ||
        (!body.controllerSecret.empty() &&
         body.controllerSecret.data == nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (limits.maxContainerItems < 3U ||
        limits.maxByteStringLength < body.grant.size ||
        limits.maxByteStringLength < body.controllerSecret.size) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(3U);
    if (result) {
        result = writer.writeUnsigned(kControllerControlWireVersion);
    }
    if (result) result = writer.writeBytes(body.grant);
    if (result) result = writer.writeBytes(body.controllerSecret);
    if (result) encoded = writer.view();
    return result;
}

Result decodeControllerMutationBody(
    ByteView encoded,
    ControllerMutationBody& body,
    const cbor::Limits& limits) {
    Result result = cbor::validate(encoded, limits);
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t version = 0U;
    ControllerMutationBody decoded;
    if (result) result = reader.readArraySize(count);
    if (result && count != 3U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = reader.readUnsigned(version);
    if (result && version != kControllerControlWireVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (result) result = reader.readBytes(decoded.grant);
    if (result) result = reader.readBytes(decoded.controllerSecret);
    if (result && (decoded.grant.empty() ||
                   decoded.grant.size > kControllerGrantMaximumSize ||
                   (!decoded.controllerSecret.empty() &&
                    decoded.controllerSecret.size !=
                        kControllerCredentialSecretSize))) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = requireFinished(reader);
    if (result) body = decoded;
    return result;
}

} // namespace bbp2
} // namespace blinker
