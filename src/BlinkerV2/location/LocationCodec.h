#ifndef BLINKER_LOCATION_CODEC_H
#define BLINKER_LOCATION_CODEC_H

#include "LocationFix.h"
#include "../protocol/cbor/Cbor.h"
#include <string.h>

namespace blinker { namespace location {

static const size_t kMaximumEncodedSampleSize = 41U;

// Device-GNSS schema 1: ordered integer-key Map, WGS84 E7, nullable accuracy/UTC.
// This codec never registers an endpoint or sends data. Owner-only admission
// and private projections must exist before a future network binding is used.
inline Result encodeSample(const LocationSample& sample, MutableByteSpan output,
                           ByteView& encoded) {
    Result result = validateLocationSample(sample);
    if (!result) return result;
    if (output.data == nullptr && output.size != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t bytes[kMaximumEncodedSampleSize];
    cbor::Writer writer(MutableByteSpan(bytes, sizeof(bytes)));
    result = writer.beginMap(8U);
    for (uint8_t key = 0U; result && key < 8U; ++key) {
        result = writer.writeUnsigned(key);
        if (!result) break;
        switch (key) {
            case 0U: result = writer.writeUnsigned(1U); break;
            case 1U: result = writer.writeUnsigned(1U); break; // GNSS, not IP/manual/phone.
            case 2U: result = writer.writeInt(sample.fix.latitudeE7); break;
            case 3U: result = writer.writeInt(sample.fix.longitudeE7); break;
            case 4U:
                result = sample.fix.hasHorizontalAccuracy
                    ? writer.writeUnsigned(sample.fix.horizontalAccuracyCm) : writer.writeNull();
                break;
            case 5U:
                result = sample.fix.timeQuality == LocationTimeQuality::GnssUtc
                    ? writer.writeUnsigned(sample.fix.capturedAtUtcMillis) : writer.writeNull();
                break;
            case 6U: result = writer.writeUnsigned(static_cast<uint8_t>(sample.fix.timeQuality)); break;
            case 7U: result = writer.writeUnsigned(sample.ageMillis); break;
        }
    }
    if (!result) return result;
    if (output.size < writer.size()) return Result::failure(ErrorCode::BufferTooSmall);
    // No partial output on failure, including a previous view into this buffer.
    memcpy(output.data, bytes, writer.size());
    encoded = ByteView(output.data, writer.size());
    return Result::success();
}

namespace detail {
inline Result readNullableUnsigned(cbor::Reader& reader, uint64_t& value, bool& present) {
    cbor::Type type = cbor::Type::Invalid;
    Result result = reader.peekType(type);
    if (!result) return result;
    present = type != cbor::Type::Null;
    value = 0U;
    return present ? reader.readUnsigned(value) : reader.readNull();
}
} // namespace detail

inline Result decodeSample(ByteView encoded, LocationSample& sample) {
    if (encoded.size > kMaximumEncodedSampleSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Limits limits;
    limits.maxContainerItems = 8U;
    limits.maxTextLength = 0U;
    limits.maxByteStringLength = 0U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    Result result = reader.readMapSize(count);
    if (!result) return result;
    if (count != 8U) return Result::failure(ErrorCode::InvalidEncoding);
    LocationSample candidate;
    bool hasUtc = false;
    for (uint8_t expected = 0U; expected < 8U; ++expected) {
        uint64_t key = 0U, value = 0U;
        int64_t coordinate = 0;
        result = reader.readUnsigned(key);
        if (!result) return result;
        // Reader checks canonical scalar encoding, but not Map uniqueness/order.
        if (key != expected) return Result::failure(ErrorCode::InvalidEncoding);
        switch (expected) {
            case 0U:
                result = reader.readUnsigned(value);
                if (result && value != 1U) return Result::failure(ErrorCode::UnsupportedVersion);
                break;
            case 1U:
                result = reader.readUnsigned(value);
                if (result && value != 1U) return Result::failure(ErrorCode::UnsupportedFeature);
                break;
            case 2U:
            case 3U: {
                result = reader.readInt(coordinate);
                const int64_t bound = expected == 2U
                    ? kLocationLatitudeLimitE7 : kLocationLongitudeLimitE7;
                if (result && (coordinate < -bound || coordinate > bound)) {
                    return Result::failure(ErrorCode::ValueOutOfRange);
                }
                if (expected == 2U) candidate.fix.latitudeE7 = static_cast<int32_t>(coordinate);
                else candidate.fix.longitudeE7 = static_cast<int32_t>(coordinate);
                break;
            }
            case 4U:
                result = detail::readNullableUnsigned(reader, value, candidate.fix.hasHorizontalAccuracy);
                if (result && value > UINT32_MAX) return Result::failure(ErrorCode::ValueOutOfRange);
                candidate.fix.horizontalAccuracyCm = static_cast<uint32_t>(value);
                break;
            case 5U:
                result = detail::readNullableUnsigned(reader, candidate.fix.capturedAtUtcMillis, hasUtc);
                break;
            case 6U:
                result = reader.readUnsigned(value);
                if (result && value > 1U) return Result::failure(ErrorCode::UnsupportedFeature);
                candidate.fix.timeQuality = static_cast<LocationTimeQuality>(value);
                break;
            case 7U:
                result = reader.readUnsigned(value);
                if (result && value >= kLocationTickHalfRange) return Result::failure(ErrorCode::ValueOutOfRange);
                candidate.ageMillis = static_cast<uint32_t>(value);
                break;
        }
        if (!result) return result;
    }
    if (!reader.finished()) return Result::failure(ErrorCode::TrailingData);
    if (hasUtc != (candidate.fix.timeQuality == LocationTimeQuality::GnssUtc)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    candidate.fix.valid = true;
    result = validateLocationSample(candidate);
    if (result) sample = candidate;
    return result;
}

} } // namespace blinker::location

#endif
