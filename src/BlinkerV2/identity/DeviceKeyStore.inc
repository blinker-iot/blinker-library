#include "DeviceKeyStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kDeviceKeyStoreMagic[4] = {'B', 'D', 'K', '2'};
const uint8_t kDeviceKeyStoreSchemaVersion = 1U;
const size_t kNarrowDeviceKeyOffset = 8U;
const size_t kDeviceKeyStoreCrcOffset = DeviceKeyStore::serializedSize - 4U;

void deviceKeyStoreWriteU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
    output[1] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    output[2] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    output[3] = static_cast<uint8_t>(value & 0xFFU);
}

uint32_t deviceKeyStoreReadU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

} // namespace

Result DeviceKeyStore::encode(
    const DeviceKey& key,
    MutableByteSpan output) {
    if (!validateDeviceKey(key)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    memset(output.data, 0, serializedSize);
    memcpy(output.data, kDeviceKeyStoreMagic, sizeof(kDeviceKeyStoreMagic));
    output.data[4] = kDeviceKeyStoreSchemaVersion;
    memcpy(
        output.data + kNarrowDeviceKeyOffset,
        key.bytes,
        sizeof(key.bytes));
    deviceKeyStoreWriteU32(
        output.data + kDeviceKeyStoreCrcOffset,
        computeCrc32(ByteView(output.data, kDeviceKeyStoreCrcOffset)));
    return Result::success();
}

Result DeviceKeyStore::decode(ByteView input, DeviceKey& output) {
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(
            input.data,
            kDeviceKeyStoreMagic,
            sizeof(kDeviceKeyStoreMagic)) != 0 ||
        input.data[4] != kDeviceKeyStoreSchemaVersion ||
        input.data[5] != 0U || input.data[6] != 0U ||
        input.data[7] != 0U ||
        deviceKeyStoreReadU32(input.data + kDeviceKeyStoreCrcOffset) !=
            computeCrc32(ByteView(input.data, kDeviceKeyStoreCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    DeviceKey decoded;
    memcpy(
        decoded.bytes,
        input.data + kNarrowDeviceKeyOffset,
        sizeof(decoded.bytes));
    Result result = validateDeviceKey(decoded);
    if (result) output = decoded;
    clearDeviceKey(decoded);
    return result ? result : Result::failure(ErrorCode::InvalidEncoding);
}

Result DeviceKeyStore::load(DeviceKey& output) {
    uint8_t record[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(record, sizeof(record)),
        written);
    if (result && written != sizeof(record)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    DeviceKey decoded;
    if (result) result = decode(ByteView(record, written), decoded);
    if (result) output = decoded;
    clearDeviceKey(decoded);
    secureZero(MutableByteSpan(record, sizeof(record)));
    return result;
}

Result DeviceKeyStore::replace(const DeviceKey& key) {
    uint8_t record[serializedSize] = {};
    Result result = encode(key, MutableByteSpan(record, sizeof(record)));
    if (result) result = storage_.replace(ByteView(record, sizeof(record)));
    secureZero(MutableByteSpan(record, sizeof(record)));
    if (!result) return result;

    DeviceKey verified;
    result = load(verified);
    if (result && !sameDeviceKey(key, verified)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    clearDeviceKey(verified);
    return result;
}

Result DeviceKeyStore::clear() {
    const Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : result;
}

} // namespace blinker
