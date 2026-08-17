#include "CloudEnrollmentRecordStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'C', 'E', '2'};
const uint8_t kSchemaVersion = 1U;
const size_t kRequestIdOffset = 20U;
const size_t kPrivateKeyOffset = 36U;
const size_t kSecretOffset = 68U;
const size_t kGrantDigestOffset = 100U;
const size_t kCrcOffset = CloudEnrollmentRecordStore::serializedSize - 4U;

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

Result CloudEnrollmentRecordStore::encode(
    const CloudEnrollmentRecord& record,
    MutableByteSpan output) {
    Result result = validateCloudEnrollmentRecord(record);
    if (!result) return result;
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(record.state);
    writeU32(output.data + 8U, record.ownershipGeneration);
    writeU32(output.data + 12U, record.credentialVersion);
    writeU32(output.data + 16U, record.encryptionKeyId);
    memcpy(output.data + kRequestIdOffset, record.requestId, 16U);
    memcpy(output.data + kPrivateKeyOffset, record.ephemeralPrivateKey, 32U);
    memcpy(output.data + kSecretOffset, record.cloudSecret, 32U);
    memcpy(output.data + kGrantDigestOffset, record.grantDigest, 32U);
    writeU32(
        output.data + kCrcOffset,
        computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result CloudEnrollmentRecordStore::decode(
    ByteView input,
    CloudEnrollmentRecord& record) {
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion || input.data[6] != 0U ||
        input.data[7] != 0U ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    CloudEnrollmentRecord decoded;
    decoded.state = static_cast<CloudEnrollmentRecordState>(input.data[5]);
    decoded.ownershipGeneration = readU32(input.data + 8U);
    decoded.credentialVersion = readU32(input.data + 12U);
    decoded.encryptionKeyId = readU32(input.data + 16U);
    memcpy(decoded.requestId, input.data + kRequestIdOffset, 16U);
    memcpy(decoded.ephemeralPrivateKey, input.data + kPrivateKeyOffset, 32U);
    memcpy(decoded.cloudSecret, input.data + kSecretOffset, 32U);
    memcpy(decoded.grantDigest, input.data + kGrantDigestOffset, 32U);
    Result result = validateCloudEnrollmentRecord(decoded);
    if (result) record = decoded;
    clearCloudEnrollmentRecord(decoded);
    return result;
}

Result CloudEnrollmentRecordStore::load(CloudEnrollmentRecord& output) {
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)),
        written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    CloudEnrollmentRecord decoded;
    if (result) result = decode(ByteView(encoded, written), decoded);
    if (result) output = decoded;
    clearCloudEnrollmentRecord(decoded);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result CloudEnrollmentRecordStore::replaceAndVerify(
    const CloudEnrollmentRecord& record) {
    uint8_t encoded[serializedSize] = {};
    Result result = encode(record, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage_.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;
    CloudEnrollmentRecord verified;
    result = load(verified);
    const bool same = result && sameCloudEnrollmentRecord(verified, record);
    clearCloudEnrollmentRecord(verified);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result CloudEnrollmentRecordStore::stagePrepared(
    const CloudEnrollmentRecord& record) {
    Result result = validateCloudEnrollmentRecord(record);
    if (!result || record.state != CloudEnrollmentRecordState::Prepared) {
        return result ? Result::failure(ErrorCode::InvalidArgument) : result;
    }
    CloudEnrollmentRecord existing;
    result = load(existing);
    if (result) {
        const bool same = sameCloudEnrollmentRecord(existing, record);
        clearCloudEnrollmentRecord(existing);
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }
    clearCloudEnrollmentRecord(existing);
    if (result.code() != ErrorCode::NotFound) return result;
    return replaceAndVerify(record);
}

Result CloudEnrollmentRecordStore::acceptGrant(
    const CloudEnrollmentRecord& expectedPrepared,
    ByteView grantDigest) {
    Result result = validateCloudEnrollmentRecord(expectedPrepared);
    if (!result) return result;
    if (expectedPrepared.state != CloudEnrollmentRecordState::Prepared ||
        grantDigest.data == nullptr || grantDigest.size != kSha256Size) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    CloudEnrollmentRecord existing;
    result = load(existing);
    if (!result) return result;
    if (existing.state == CloudEnrollmentRecordState::GrantAccepted) {
        CloudEnrollmentRecord accepted = expectedPrepared;
        accepted.state = CloudEnrollmentRecordState::GrantAccepted;
        memcpy(accepted.grantDigest, grantDigest.data, grantDigest.size);
        const bool replay = sameCloudEnrollmentRecord(existing, accepted);
        clearCloudEnrollmentRecord(existing);
        clearCloudEnrollmentRecord(accepted);
        return replay ? Result::success()
                      : Result::failure(ErrorCode::StateConflict);
    }
    if (!sameCloudEnrollmentRecord(existing, expectedPrepared)) {
        clearCloudEnrollmentRecord(existing);
        return Result::failure(ErrorCode::StateConflict);
    }
    clearCloudEnrollmentRecord(existing);
    CloudEnrollmentRecord accepted = expectedPrepared;
    accepted.state = CloudEnrollmentRecordState::GrantAccepted;
    memcpy(accepted.grantDigest, grantDigest.data, grantDigest.size);
    result = replaceAndVerify(accepted);
    clearCloudEnrollmentRecord(accepted);
    return result;
}

Result CloudEnrollmentRecordStore::clearExact(
    const CloudEnrollmentRecord& expected) {
    Result result = validateCloudEnrollmentRecord(expected);
    if (!result) return result;
    CloudEnrollmentRecord existing;
    result = load(existing);
    if (result) {
        const bool same = sameCloudEnrollmentRecord(existing, expected);
        clearCloudEnrollmentRecord(existing);
        if (!same) return Result::failure(ErrorCode::StateConflict);
        result = storage_.clear();
    } else if (result.code() == ErrorCode::NotFound) {
        clearCloudEnrollmentRecord(existing);
        return Result::success();
    }
    clearCloudEnrollmentRecord(existing);
    return result;
}

Result CloudEnrollmentRecordStore::eraseAll() {
    Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : result;
}

} // namespace blinker
