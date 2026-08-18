#include "DeviceKeyStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

static const uint8_t kMagic[4] = {'B', 'D', 'K', '2'};
static const uint8_t kSchemaVersion = 1U;
static const size_t kKeyOffset = 8U;
static const size_t kCrcOffset = DeviceKeyStore::serializedSize - 4U;

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

Result DeviceKeyStore::encode(
    const DeviceKey& key,
    MutableByteSpan output) {
    Result result = validateDeviceKey(key);
    if (!result) return result;
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    memcpy(output.data + kKeyOffset, key.bytes, sizeof(key.bytes));
    writeU32(
        output.data + kCrcOffset,
        computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result DeviceKeyStore::decode(ByteView input, DeviceKey& key) {
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion || input.data[5] != 0U ||
        input.data[6] != 0U || input.data[7] != 0U ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    DeviceKey decoded;
    memcpy(decoded.bytes, input.data + kKeyOffset, sizeof(decoded.bytes));
    Result result = validateDeviceKey(decoded);
    if (result) key = decoded;
    clearDeviceKey(decoded);
    return result;
}

Result DeviceKeyStore::load(DeviceKey& output) {
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)), written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    DeviceKey decoded;
    if (result) result = decode(ByteView(encoded, written), decoded);
    if (result) output = decoded;
    clearDeviceKey(decoded);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result DeviceKeyStore::replace(const DeviceKey& key) {
    uint8_t encoded[serializedSize] = {};
    Result result = encode(key, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage_.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;

    DeviceKey verified;
    result = load(verified);
    const bool same = result && sameDeviceKey(verified, key);
    clearDeviceKey(verified);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result DeviceKeyStore::clear() {
    const Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : result;
}

} // namespace blinker
