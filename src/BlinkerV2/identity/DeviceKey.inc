#include "DeviceKey.h"

#include <string.h>

#include "../core/HkdfSha256.h"
#include "../core/SecureMemory.h"
#include "../core/Sha256.h"

namespace blinker {

namespace {

static const char kBase64UrlAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static const char kLocatorDomain[] = "blinker/device-key/locator/v1";
static const char kAuthenticationInfo[] = "blinker/device-key/auth/v1";

int8_t decodeCharacter(char value) {
    if (value >= 'A' && value <= 'Z') {
        return static_cast<int8_t>(value - 'A');
    }
    if (value >= 'a' && value <= 'z') {
        return static_cast<int8_t>(value - 'a' + 26);
    }
    if (value >= '0' && value <= '9') {
        return static_cast<int8_t>(value - '0' + 52);
    }
    if (value == '-') return 62;
    if (value == '_') return 63;
    return -1;
}

bool deviceKeyAllZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return true;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

} // namespace

Result validateDeviceKey(const DeviceKey& key) {
    return deviceKeyAllZero(ByteView(key.bytes, sizeof(key.bytes)))
               ? Result::failure(ErrorCode::InvalidEncoding)
               : Result::success();
}

Result decodeDeviceKey(StringView encoded, DeviceKey& output) {
    if (encoded.data == nullptr || encoded.size != kDeviceKeyTextSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    DeviceKey decoded;
    size_t input = 0U;
    size_t written = 0U;
    while (input < 40U) {
        const int8_t a = decodeCharacter(encoded.data[input]);
        const int8_t b = decodeCharacter(encoded.data[input + 1U]);
        const int8_t c = decodeCharacter(encoded.data[input + 2U]);
        const int8_t d = decodeCharacter(encoded.data[input + 3U]);
        if (a < 0 || b < 0 || c < 0 || d < 0) {
            clearDeviceKey(decoded);
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        decoded.bytes[written++] = static_cast<uint8_t>(
            (static_cast<uint8_t>(a) << 2U) |
            (static_cast<uint8_t>(b) >> 4U));
        decoded.bytes[written++] = static_cast<uint8_t>(
            (static_cast<uint8_t>(b) << 4U) |
            (static_cast<uint8_t>(c) >> 2U));
        decoded.bytes[written++] = static_cast<uint8_t>(
            (static_cast<uint8_t>(c) << 6U) |
            static_cast<uint8_t>(d));
        input += 4U;
    }

    const int8_t a = decodeCharacter(encoded.data[40U]);
    const int8_t b = decodeCharacter(encoded.data[41U]);
    const int8_t c = decodeCharacter(encoded.data[42U]);
    if (a < 0 || b < 0 || c < 0 ||
        (static_cast<uint8_t>(c) & 0x03U) != 0U) {
        clearDeviceKey(decoded);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    decoded.bytes[written++] = static_cast<uint8_t>(
        (static_cast<uint8_t>(a) << 2U) |
        (static_cast<uint8_t>(b) >> 4U));
    decoded.bytes[written++] = static_cast<uint8_t>(
        (static_cast<uint8_t>(b) << 4U) |
        (static_cast<uint8_t>(c) >> 2U));
    if (written != sizeof(decoded.bytes) || !validateDeviceKey(decoded)) {
        clearDeviceKey(decoded);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    output = decoded;
    clearDeviceKey(decoded);
    return Result::success();
}

Result encodeDeviceKey(
    const DeviceKey& key,
    MutableCharSpan output,
    StringView& encoded) {
    encoded = StringView();
    Result result = validateDeviceKey(key);
    if (!result) return result;
    if (output.data == nullptr || output.size < kDeviceKeyTextSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    size_t input = 0U;
    size_t written = 0U;
    while (input < 30U) {
        const uint8_t a = key.bytes[input++];
        const uint8_t b = key.bytes[input++];
        const uint8_t c = key.bytes[input++];
        output.data[written++] = kBase64UrlAlphabet[a >> 2U];
        output.data[written++] = kBase64UrlAlphabet[
            ((a & 0x03U) << 4U) | (b >> 4U)];
        output.data[written++] = kBase64UrlAlphabet[
            ((b & 0x0FU) << 2U) | (c >> 6U)];
        output.data[written++] = kBase64UrlAlphabet[c & 0x3FU];
    }
    const uint8_t a = key.bytes[input++];
    const uint8_t b = key.bytes[input];
    output.data[written++] = kBase64UrlAlphabet[a >> 2U];
    output.data[written++] = kBase64UrlAlphabet[
        ((a & 0x03U) << 4U) | (b >> 4U)];
    output.data[written++] = kBase64UrlAlphabet[(b & 0x0FU) << 2U];
    encoded = StringView(output.data, written);
    return Result::success();
}

bool sameDeviceKey(const DeviceKey& first, const DeviceKey& second) {
    return constantTimeEqual(
        ByteView(first.bytes, sizeof(first.bytes)),
        ByteView(second.bytes, sizeof(second.bytes)));
}

void clearDeviceKey(DeviceKey& key) {
    secureZero(MutableByteSpan(key.bytes, sizeof(key.bytes)));
}

Result deriveDeviceKeyLocator(
    const DeviceKey& key,
    DeviceKeyLocator& output) {
    Result result = validateDeviceKey(key);
    uint8_t digest[kSha256Size] = {};
    if (result) {
        Sha256 hash;
        result = hash.update(ByteView(
            reinterpret_cast<const uint8_t*>(kLocatorDomain),
            sizeof(kLocatorDomain) - 1U));
        const uint8_t separator = 0U;
        if (result) result = hash.update(ByteView(&separator, 1U));
        if (result) {
            result = hash.update(ByteView(key.bytes, sizeof(key.bytes)));
        }
        if (result) {
            result = hash.finish(MutableByteSpan(digest, sizeof(digest)));
        }
    }
    if (result) memcpy(output.bytes, digest, sizeof(output.bytes));
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    return result;
}

Result deriveDeviceAuthKey(
    const DeviceKey& key,
    MutableByteSpan output) {
    Result result = validateDeviceKey(key);
    if (!result) return result;
    if (output.data == nullptr || output.size < kDeviceAuthKeySize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    return hkdfSha256(
        ByteView(),
        ByteView(key.bytes, sizeof(key.bytes)),
        ByteView(
            reinterpret_cast<const uint8_t*>(kAuthenticationInfo),
            sizeof(kAuthenticationInfo) - 1U),
        MutableByteSpan(output.data, kDeviceAuthKeySize));
}

} // namespace blinker
