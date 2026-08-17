#include "P256DeveloperIdentityRecord.h"

#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "../security/P256Signature.h"

namespace blinker {

namespace {

const uint8_t kRecordPrefix[8] = {
    'B', 'I', 'D', '2',
    1U,
    3U,
    0U,
    0U
};

const char kRecordDomain[] = "blinker.developer-key-record.v1";

Result checksum(ByteView prefix, MutableByteSpan output) {
    if (output.data == nullptr || output.size < kSha256Size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Sha256 hash;
    Result result = hash.update(ByteView(
        reinterpret_cast<const uint8_t*>(kRecordDomain),
        sizeof(kRecordDomain) - 1U));
    if (result) result = hash.update(prefix);
    if (result) result = hash.finish(output);
    return result;
}

} // namespace

Result encodeP256DeveloperIdentityRecord(
    ByteView privateKey,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (!isValidP256Scalar(privateKey)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr ||
        output.size < kP256DeveloperIdentityRecordSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    memcpy(output.data, kRecordPrefix, sizeof(kRecordPrefix));
    memcpy(
        output.data + sizeof(kRecordPrefix),
        privateKey.data,
        kP256PrivateKeySize);
    Result result = checksum(
        ByteView(output.data, sizeof(kRecordPrefix) + kP256PrivateKeySize),
        MutableByteSpan(
            output.data + sizeof(kRecordPrefix) + kP256PrivateKeySize,
            kSha256Size));
    if (!result) {
        secureZero(MutableByteSpan(
            output.data,
            kP256DeveloperIdentityRecordSize));
        return result;
    }
    encoded = ByteView(output.data, kP256DeveloperIdentityRecordSize);
    return Result::success();
}

Result decodeP256DeveloperIdentityRecord(
    ByteView encoded,
    MutableByteSpan privateKey) {
    if (privateKey.data == nullptr ||
        privateKey.size < kP256PrivateKeySize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    secureZero(MutableByteSpan(privateKey.data, kP256PrivateKeySize));
    if (encoded.data == nullptr ||
        encoded.size != kP256DeveloperIdentityRecordSize ||
        memcmp(encoded.data, kRecordPrefix, sizeof(kRecordPrefix)) != 0) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    uint8_t expected[kSha256Size] = {};
    Result result = checksum(
        ByteView(
            encoded.data,
            sizeof(kRecordPrefix) + kP256PrivateKeySize),
        MutableByteSpan(expected, sizeof(expected)));
    const ByteView actual(
        encoded.data + sizeof(kRecordPrefix) + kP256PrivateKeySize,
        kSha256Size);
    if (result && !constantTimeEqual(ByteView(expected, sizeof(expected)), actual)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    const ByteView scalar(
        encoded.data + sizeof(kRecordPrefix),
        kP256PrivateKeySize);
    if (result && !isValidP256Scalar(scalar)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (!result) return result;
    memcpy(privateKey.data, scalar.data, kP256PrivateKeySize);
    return Result::success();
}

} // namespace blinker
