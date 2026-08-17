#include "OwnershipReleaseRecordStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'O', 'R', '2'};
const uint8_t kSchemaVersion = 1U;
const size_t kRequestIdOffset = 20U;
const size_t kNonceOffset = 36U;
const size_t kGrantIdOffset = 52U;
const size_t kGrantDigestOffset = 68U;
const size_t kLogicalIdOffset = 100U;
const size_t kCrcOffset = OwnershipReleaseRecordStore::serializedSize - 4U;

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

Result OwnershipReleaseRecordStore::encode(
    const OwnershipReleaseRecord& record,
    MutableByteSpan output) {
    Result result = validateOwnershipReleaseRecord(record);
    if (!result) return result;
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(record.state);
    output.data[6] = static_cast<uint8_t>(record.operation);
    output.data[7] = static_cast<uint8_t>(record.networkPolicy);
    writeU32(output.data + 8U, record.currentGeneration);
    writeU32(output.data + 12U, record.nextGeneration);
    output.data[16] = record.logicalDeviceIdLength;
    memcpy(output.data + kRequestIdOffset, record.requestId, sizeof(record.requestId));
    memcpy(output.data + kNonceOffset, record.releaseNonce, sizeof(record.releaseNonce));
    memcpy(output.data + kGrantIdOffset, record.grantId, sizeof(record.grantId));
    memcpy(output.data + kGrantDigestOffset, record.grantDigest, sizeof(record.grantDigest));
    memcpy(output.data + kLogicalIdOffset, record.logicalDeviceId, sizeof(record.logicalDeviceId));
    writeU32(
        output.data + kCrcOffset,
        computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result OwnershipReleaseRecordStore::decode(
    ByteView input,
    OwnershipReleaseRecord& record) {
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion ||
        input.data[17] != 0U || input.data[18] != 0U ||
        input.data[19] != 0U ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    OwnershipReleaseRecord decoded;
    decoded.state = static_cast<OwnershipReleaseRecordState>(input.data[5]);
    decoded.operation = static_cast<OwnershipReleaseOperation>(input.data[6]);
    decoded.networkPolicy =
        static_cast<OwnershipReleaseNetworkPolicy>(input.data[7]);
    decoded.currentGeneration = readU32(input.data + 8U);
    decoded.nextGeneration = readU32(input.data + 12U);
    decoded.logicalDeviceIdLength = input.data[16];
    memcpy(decoded.requestId, input.data + kRequestIdOffset, sizeof(decoded.requestId));
    memcpy(decoded.releaseNonce, input.data + kNonceOffset, sizeof(decoded.releaseNonce));
    memcpy(decoded.grantId, input.data + kGrantIdOffset, sizeof(decoded.grantId));
    memcpy(decoded.grantDigest, input.data + kGrantDigestOffset, sizeof(decoded.grantDigest));
    memcpy(decoded.logicalDeviceId, input.data + kLogicalIdOffset, sizeof(decoded.logicalDeviceId));
    Result result = validateOwnershipReleaseRecord(decoded);
    if (result) record = decoded;
    clearOwnershipReleaseRecord(decoded);
    return result;
}

Result OwnershipReleaseRecordStore::load(OwnershipReleaseRecord& output) {
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(MutableByteSpan(encoded, sizeof(encoded)), written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    OwnershipReleaseRecord decoded;
    if (result) result = decode(ByteView(encoded, written), decoded);
    if (result) output = decoded;
    clearOwnershipReleaseRecord(decoded);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result OwnershipReleaseRecordStore::replaceAndVerify(
    const OwnershipReleaseRecord& record) {
    uint8_t encoded[serializedSize] = {};
    Result result = encode(record, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage_.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;
    OwnershipReleaseRecord verified;
    result = load(verified);
    const bool same = result && sameOwnershipReleaseRecord(verified, record);
    clearOwnershipReleaseRecord(verified);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result OwnershipReleaseRecordStore::stagePrepared(
    const OwnershipReleaseRecord& record) {
    Result result = validateOwnershipReleaseRecord(record);
    if (!result) return result;
    if (record.state != OwnershipReleaseRecordState::Prepared) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipReleaseRecord existing;
    result = load(existing);
    if (result) {
        const bool same = sameOwnershipReleaseRecord(existing, record);
        clearOwnershipReleaseRecord(existing);
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }
    clearOwnershipReleaseRecord(existing);
    if (result.code() != ErrorCode::NotFound) return result;
    return replaceAndVerify(record);
}

Result OwnershipReleaseRecordStore::acceptGrant(
    const OwnershipReleaseRecord& expectedPrepared,
    ByteView grantId,
    uint32_t nextGeneration,
    ByteView grantDigest) {
    Result result = validateOwnershipReleaseRecord(expectedPrepared);
    if (!result) return result;
    if (expectedPrepared.state != OwnershipReleaseRecordState::Prepared ||
        grantId.data == nullptr || grantId.size != kOwnershipReleaseGrantIdSize ||
        grantDigest.data == nullptr || grantDigest.size != kSha256Size ||
        nextGeneration != expectedPrepared.currentGeneration + 1U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipReleaseRecord accepted = expectedPrepared;
    accepted.state = OwnershipReleaseRecordState::GrantAccepted;
    accepted.nextGeneration = nextGeneration;
    memcpy(accepted.grantId, grantId.data, grantId.size);
    memcpy(accepted.grantDigest, grantDigest.data, grantDigest.size);

    OwnershipReleaseRecord existing;
    result = load(existing);
    if (result) {
        const bool replay = sameOwnershipReleaseRecord(existing, accepted);
        const bool expected = sameOwnershipReleaseRecord(existing, expectedPrepared);
        clearOwnershipReleaseRecord(existing);
        if (replay) {
            clearOwnershipReleaseRecord(accepted);
            return Result::success();
        }
        if (!expected) {
            clearOwnershipReleaseRecord(accepted);
            return Result::failure(ErrorCode::StateConflict);
        }
    } else {
        clearOwnershipReleaseRecord(existing);
        clearOwnershipReleaseRecord(accepted);
        return result;
    }
    result = replaceAndVerify(accepted);
    clearOwnershipReleaseRecord(accepted);
    return result;
}

Result OwnershipReleaseRecordStore::acknowledgeCommit(
    const OwnershipReleaseRecord& expectedAccepted) {
    Result result = validateOwnershipReleaseRecord(expectedAccepted);
    if (!result) return result;
    if (expectedAccepted.state != OwnershipReleaseRecordState::GrantAccepted) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipReleaseRecord acknowledged = expectedAccepted;
    acknowledged.state = OwnershipReleaseRecordState::CommitAcknowledged;
    OwnershipReleaseRecord existing;
    result = load(existing);
    if (!result) {
        clearOwnershipReleaseRecord(existing);
        clearOwnershipReleaseRecord(acknowledged);
        return result;
    }
    const bool replay = sameOwnershipReleaseRecord(existing, acknowledged);
    const bool expected = sameOwnershipReleaseRecord(existing, expectedAccepted);
    clearOwnershipReleaseRecord(existing);
    if (replay) {
        clearOwnershipReleaseRecord(acknowledged);
        return Result::success();
    }
    if (!expected) {
        clearOwnershipReleaseRecord(acknowledged);
        return Result::failure(ErrorCode::StateConflict);
    }
    result = replaceAndVerify(acknowledged);
    clearOwnershipReleaseRecord(acknowledged);
    return result;
}

Result OwnershipReleaseRecordStore::clearExact(
    const OwnershipReleaseRecord& expected) {
    Result result = validateOwnershipReleaseRecord(expected);
    if (!result) return result;
    OwnershipReleaseRecord existing;
    result = load(existing);
    if (result) {
        const bool same = sameOwnershipReleaseRecord(existing, expected);
        clearOwnershipReleaseRecord(existing);
        if (!same) return Result::failure(ErrorCode::StateConflict);
        result = storage_.clear();
    } else if (result.code() == ErrorCode::NotFound) {
        clearOwnershipReleaseRecord(existing);
        return Result::success();
    }
    clearOwnershipReleaseRecord(existing);
    return result;
}

Result OwnershipReleaseRecordStore::eraseAll() {
    Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : result;
}

} // namespace blinker
