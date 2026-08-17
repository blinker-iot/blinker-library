#include "CloudCredentialStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'C', 'S', '2'};
const uint8_t kSchemaVersion = 1U;
const size_t kSecretOffset = 16U;
const size_t kLogicalIdOffset = 48U;
const size_t kCrcOffset = CloudCredentialStore::serializedSize - 4U;

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
    output[1] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    output[2] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    output[3] = static_cast<uint8_t>(value & 0xFFU);
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

Result CloudCredentialStore::encode(
    const CloudCredential& credential,
    MutableByteSpan output) {
    Result result = validateCloudCredential(credential);
    if (!result) return result;
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    memset(output.data, 0, serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(credential.suite);
    output.data[6] = credential.logicalDeviceIdLength;
    writeU32(output.data + 8U, credential.generation);
    writeU32(output.data + 12U, credential.credentialVersion);
    memcpy(
        output.data + kSecretOffset,
        credential.secret,
        sizeof(credential.secret));
    memcpy(
        output.data + kLogicalIdOffset,
        credential.logicalDeviceId,
        credential.logicalDeviceIdLength);
    writeU32(
        output.data + kCrcOffset,
        computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result CloudCredentialStore::decode(
    ByteView input,
    CloudCredential& credential) {
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion || input.data[7] != 0U ||
        !zeroPadding(
            input,
            kLogicalIdOffset,
            input.data[6],
            kCloudLogicalDeviceIdCapacity) ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    CloudCredential decoded;
    decoded.suite = static_cast<CloudCredentialSuite>(input.data[5]);
    decoded.logicalDeviceIdLength = input.data[6];
    decoded.generation = readU32(input.data + 8U);
    decoded.credentialVersion = readU32(input.data + 12U);
    memcpy(
        decoded.secret,
        input.data + kSecretOffset,
        sizeof(decoded.secret));
    memcpy(
        decoded.logicalDeviceId,
        input.data + kLogicalIdOffset,
        sizeof(decoded.logicalDeviceId));
    Result result = validateCloudCredential(decoded);
    if (result) credential = decoded;
    clearCloudCredential(decoded);
    return result;
}

Result CloudCredentialStore::load(CloudCredential& output) {
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)),
        written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    CloudCredential decoded;
    if (result) result = decode(ByteView(encoded, written), decoded);
    if (result) output = decoded;
    clearCloudCredential(decoded);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result CloudCredentialStore::installFresh(
    const CloudCredential& credential) {
    Result result = validateCloudCredential(credential);
    if (!result) return result;

    CloudCredential existing;
    result = load(existing);
    if (result) {
        const bool same = sameCloudCredential(existing, credential);
        clearCloudCredential(existing);
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }
    clearCloudCredential(existing);
    if (result.code() != ErrorCode::NotFound) return result;

    uint8_t encoded[serializedSize] = {};
    result = encode(credential, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage_.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;

    CloudCredential verified;
    result = load(verified);
    const bool same = result && sameCloudCredential(verified, credential);
    clearCloudCredential(verified);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result CloudCredentialStore::clear() {
    Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : result;
}

} // namespace blinker
