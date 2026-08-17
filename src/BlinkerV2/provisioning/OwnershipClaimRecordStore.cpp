#include "OwnershipClaimRecordStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'O', 'C', '2'};
const uint8_t kSchemaVersion = 1U;
const size_t kGenerationOffset = 8U;
const size_t kRequestIdOffset = 12U;
const size_t kClaimNonceOffset = 28U;
const size_t kCrcOffset = OwnershipClaimRecordStore::serializedSize - 4U;

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

} // namespace

Result OwnershipClaimRecordStore::encode(
    const OwnershipClaimRecord& record,
    MutableByteSpan output) {
    Result result = validateOwnershipClaimRecord(record);
    if (!result) return result;
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    writeU32(output.data + kGenerationOffset, record.ownershipGeneration);
    memcpy(output.data + kRequestIdOffset, record.requestId, 16U);
    memcpy(output.data + kClaimNonceOffset, record.claimNonce, 16U);
    writeU32(
        output.data + kCrcOffset,
        computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result OwnershipClaimRecordStore::decode(
    ByteView input,
    OwnershipClaimRecord& record) {
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion || input.data[5] != 0U ||
        input.data[6] != 0U || input.data[7] != 0U ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    OwnershipClaimRecord decoded;
    decoded.ownershipGeneration = readU32(input.data + kGenerationOffset);
    memcpy(decoded.requestId, input.data + kRequestIdOffset, 16U);
    memcpy(decoded.claimNonce, input.data + kClaimNonceOffset, 16U);
    Result result = validateOwnershipClaimRecord(decoded);
    if (result) record = decoded;
    clearOwnershipClaimRecord(decoded);
    return result;
}

Result OwnershipClaimRecordStore::load(OwnershipClaimRecord& output) {
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)),
        written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    OwnershipClaimRecord decoded;
    if (result) result = decode(ByteView(encoded, written), decoded);
    if (result) output = decoded;
    clearOwnershipClaimRecord(decoded);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result OwnershipClaimRecordStore::replaceAndVerify(
    const OwnershipClaimRecord& record) {
    uint8_t encoded[serializedSize] = {};
    Result result = encode(record, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage_.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;
    OwnershipClaimRecord verified;
    result = load(verified);
    const bool same = result && sameOwnershipClaimRecord(verified, record);
    clearOwnershipClaimRecord(verified);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result OwnershipClaimRecordStore::stage(
    const OwnershipClaimRecord& record) {
    Result result = validateOwnershipClaimRecord(record);
    if (!result) return result;
    OwnershipClaimRecord existing;
    result = load(existing);
    if (result) {
        const bool same = sameOwnershipClaimRecord(existing, record);
        clearOwnershipClaimRecord(existing);
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }
    clearOwnershipClaimRecord(existing);
    if (result.code() != ErrorCode::NotFound) return result;
    return replaceAndVerify(record);
}

Result OwnershipClaimRecordStore::clearExact(
    const OwnershipClaimRecord& expected) {
    Result result = validateOwnershipClaimRecord(expected);
    if (!result) return result;
    OwnershipClaimRecord existing;
    result = load(existing);
    if (result) {
        const bool same = sameOwnershipClaimRecord(existing, expected);
        clearOwnershipClaimRecord(existing);
        if (!same) return Result::failure(ErrorCode::StateConflict);
        result = storage_.clear();
    } else if (result.code() == ErrorCode::NotFound) {
        clearOwnershipClaimRecord(existing);
        return Result::success();
    }
    clearOwnershipClaimRecord(existing);
    return result;
}

Result OwnershipClaimRecordStore::eraseAll() {
    Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : result;
}

} // namespace blinker
