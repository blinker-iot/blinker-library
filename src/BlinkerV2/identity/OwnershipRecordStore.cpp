#include "OwnershipRecordStore.h"

#include <string.h>

#include "../core/Crc32.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'O', 'S', '2'};
const uint8_t kSchemaVersion = 1U;
const size_t kFingerprintOffset = 12U;
const size_t kLogicalIdOffset = 44U;
const size_t kCrcOffset = OwnershipRecordStore::serializedSize - 4U;

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t readU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool zeroPadding(
    ByteView input,
    size_t offset,
    size_t used,
    size_t capacity) {
    if (used > capacity) return false;
    for (size_t index = used; index < capacity; ++index) {
        if (input.data[offset + index] != 0U) return false;
    }
    return true;
}

} // namespace

Result OwnershipRecordStore::encode(
    const OwnershipRecord& record,
    MutableByteSpan output) {
    Result result = validateOwnershipRecord(record);
    if (!result) return result;
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    memset(output.data, 0, serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(record.state);
    output.data[6] = record.logicalDeviceIdLength;
    writeU32(output.data + 8U, record.generation);
    memcpy(
        output.data + kFingerprintOffset,
        record.grantFingerprint,
        sizeof(record.grantFingerprint));
    memcpy(
        output.data + kLogicalIdOffset,
        record.logicalDeviceId,
        record.logicalDeviceIdLength);
    writeU32(
        output.data + kCrcOffset,
        computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result OwnershipRecordStore::decode(
    ByteView input,
    OwnershipRecord& record) {
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion || input.data[7] != 0U ||
        !zeroPadding(
            input,
            kLogicalIdOffset,
            input.data[6],
            kLogicalDeviceIdCapacity) ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    OwnershipRecord decoded;
    decoded.state = static_cast<OwnershipState>(input.data[5]);
    decoded.logicalDeviceIdLength = input.data[6];
    decoded.generation = readU32(input.data + 8U);
    memcpy(
        decoded.grantFingerprint,
        input.data + kFingerprintOffset,
        sizeof(decoded.grantFingerprint));
    memcpy(
        decoded.logicalDeviceId,
        input.data + kLogicalIdOffset,
        sizeof(decoded.logicalDeviceId));
    Result result = validateOwnershipRecord(decoded);
    if (result) record = decoded;
    clearOwnershipRecord(decoded);
    return result;
}

Result OwnershipRecordStore::load(OwnershipRecord& output) {
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)),
        written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    OwnershipRecord decoded;
    if (result) result = decode(ByteView(encoded, written), decoded);
    if (result) output = decoded;
    clearOwnershipRecord(decoded);
    return result;
}

Result OwnershipRecordStore::replaceAndVerify(
    const OwnershipRecord& record) {
    uint8_t encoded[serializedSize] = {};
    Result result = encode(record, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage_.replace(ByteView(encoded, sizeof(encoded)));
    if (!result) return result;

    OwnershipRecord verified;
    result = load(verified);
    const bool same = result && sameOwnershipRecord(verified, record);
    clearOwnershipRecord(verified);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result OwnershipRecordStore::installVerifiedActive(
    const OwnershipRecord& record) {
    Result result = validateOwnershipRecord(record);
    if (!result || !record.active()) {
        return result ? Result::failure(ErrorCode::InvalidArgument) : result;
    }

    OwnershipRecord existing;
    result = load(existing);
    if (result && existing.active()) {
        const bool same = sameOwnershipRecord(existing, record);
        clearOwnershipRecord(existing);
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }
    if (result && record.generation != existing.generation) {
        clearOwnershipRecord(existing);
        return Result::failure(ErrorCode::SequenceConflict);
    }
    if (!result && result.code() != ErrorCode::NotFound) {
        clearOwnershipRecord(existing);
        return result;
    }
    clearOwnershipRecord(existing);
    if (result.code() == ErrorCode::NotFound && record.generation != 1U) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    return replaceAndVerify(record);
}

Result OwnershipRecordStore::retireVerified(
    uint32_t expectedGeneration,
    uint32_t nextGeneration) {
    if (expectedGeneration == 0U || nextGeneration <= expectedGeneration) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    OwnershipRecord existing;
    Result result = load(existing);
    if (!result) return result;
    if (!existing.active()) {
        const bool replay = existing.generation == nextGeneration;
        clearOwnershipRecord(existing);
        return replay ? Result::success()
                      : Result::failure(ErrorCode::SequenceConflict);
    }
    if (existing.generation != expectedGeneration) {
        clearOwnershipRecord(existing);
        return Result::failure(ErrorCode::SequenceConflict);
    }
    clearOwnershipRecord(existing);

    OwnershipRecord tombstone;
    result = makeOwnershipTombstone(nextGeneration, tombstone);
    if (result) result = replaceAndVerify(tombstone);
    clearOwnershipRecord(tombstone);
    return result;
}

Result OwnershipRecordStore::eraseAll() {
    Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : result;
}

} // namespace blinker
