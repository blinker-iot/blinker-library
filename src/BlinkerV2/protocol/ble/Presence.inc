#include "Presence.h"

#include <string.h>

#include "../../core/HmacSha256.h"
#include "../../core/SecureMemory.h"

namespace blinker {
namespace ble {

namespace {

const char kPresenceDomain[] = "blinker/ble/presence/v1";

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

} // namespace

Result derivePresenceLocator(
    ByteView presenceKey,
    const DeviceInstanceId& deviceInstanceId,
    uint32_t accessEpoch,
    uint32_t presenceKeyVersion,
    ByteView nonce,
    MutableByteSpan output) {
    if (presenceKey.size != kPresenceKeySize || !nonZero(presenceKey) ||
        !isValidDeviceInstanceId(deviceInstanceId) || accessEpoch == 0U ||
        presenceKeyVersion == 0U || nonce.data == nullptr ||
        nonce.size != kPresenceNonceSize || output.data == nullptr ||
        output.size < kPresenceLocatorSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    uint8_t metadata[8U] = {};
    writeU32(metadata, accessEpoch);
    writeU32(metadata + 4U, presenceKeyVersion);
    uint8_t digest[kSha256Size] = {};
    HmacSha256 hmac;
    Result result = hmac.begin(presenceKey);
    if (result) {
        // sizeof includes the exact trailing 0x00 domain separator.
        result = hmac.update(ByteView(
            reinterpret_cast<const uint8_t*>(kPresenceDomain),
            sizeof(kPresenceDomain)));
    }
    if (result) {
        result = hmac.update(ByteView(
            deviceInstanceId.bytes,
            sizeof(deviceInstanceId.bytes)));
    }
    if (result) result = hmac.update(ByteView(metadata, sizeof(metadata)));
    if (result) result = hmac.update(nonce);
    if (result) {
        result = hmac.finish(MutableByteSpan(digest, sizeof(digest)));
    }
    if (result) {
        memcpy(output.data, nonce.data, kPresenceNonceSize);
        memcpy(
            output.data + kPresenceNonceSize,
            digest,
            kPresenceTagSize);
    }
    secureZero(MutableByteSpan(metadata, sizeof(metadata)));
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    return result;
}

Result verifyPresenceLocator(
    ByteView presenceKey,
    const DeviceInstanceId& deviceInstanceId,
    uint32_t accessEpoch,
    uint32_t presenceKeyVersion,
    ByteView locator,
    bool& matches) {
    matches = false;
    if (locator.data == nullptr || locator.size != kPresenceLocatorSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t expected[kPresenceLocatorSize] = {};
    Result result = derivePresenceLocator(
        presenceKey,
        deviceInstanceId,
        accessEpoch,
        presenceKeyVersion,
        ByteView(locator.data, kPresenceNonceSize),
        MutableByteSpan(expected, sizeof(expected)));
    if (result) {
        matches = constantTimeEqual(
            ByteView(expected, sizeof(expected)),
            locator);
    }
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    return result;
}

Result generatePresenceNonce(
    IRandom& random,
    ByteView previousNonce,
    MutableByteSpan output) {
    if (output.data == nullptr || output.size != kPresenceNonceSize ||
        (previousNonce.size != 0U &&
         (previousNonce.data == nullptr ||
          previousNonce.size != kPresenceNonceSize))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    for (uint8_t attempt = 0U; attempt < 4U; ++attempt) {
        Result result = random.fill(output);
        if (!result) {
            secureZero(output);
            return result;
        }
        if (previousNonce.empty() ||
            !constantTimeEqual(
                previousNonce,
                ByteView(output.data, output.size))) {
            return Result::success();
        }
    }
    secureZero(output);
    return Result::failure(ErrorCode::SequenceConflict);
}

} // namespace ble
} // namespace blinker
