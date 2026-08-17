#include "DeviceInstanceIdStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'D', 'N', '2'};
const uint8_t kSchemaVersion = 1U;
const size_t kIdOffset = 8U;
const size_t kCrcOffset = DeviceInstanceIdStore::serializedSize - 4U;

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

} // namespace

Result DeviceInstanceIdStore::encode(
    const DeviceInstanceId& value,
    MutableByteSpan output) {
    if (!isValidDeviceInstanceId(value)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    memset(output.data, 0, serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    memcpy(output.data + kIdOffset, value.bytes, sizeof(value.bytes));
    writeU32(
        output.data + kCrcOffset,
        computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result DeviceInstanceIdStore::decode(
    ByteView input,
    DeviceInstanceId& output) {
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion || input.data[5] != 0U ||
        input.data[6] != 0U || input.data[7] != 0U ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    DeviceInstanceId decoded;
    memcpy(decoded.bytes, input.data + kIdOffset, sizeof(decoded.bytes));
    if (!isValidDeviceInstanceId(decoded)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    output = decoded;
    return Result::success();
}

Result DeviceInstanceIdStore::load(DeviceInstanceId& output) {
    uint8_t record[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(record, sizeof(record)),
        written);
    if (result && written != sizeof(record)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    DeviceInstanceId decoded;
    if (result) result = decode(ByteView(record, written), decoded);
    if (result) output = decoded;
    secureZero(MutableByteSpan(record, sizeof(record)));
    return result;
}

Result DeviceInstanceIdStore::loadOrCreate(
    IRandom& random,
    DeviceInstanceId& output) {
    Result result = load(output);
    if (result || result.code() != ErrorCode::NotFound) return result;

    DeviceInstanceId candidate;
    result = random.fill(MutableByteSpan(
        candidate.bytes,
        sizeof(candidate.bytes)));
    if (result && !isValidDeviceInstanceId(candidate)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }

    uint8_t record[serializedSize] = {};
    if (result) {
        result = encode(candidate, MutableByteSpan(record, sizeof(record)));
    }
    if (result) result = storage_.replace(ByteView(record, sizeof(record)));
    secureZero(MutableByteSpan(record, sizeof(record)));
    if (!result) return result;

    DeviceInstanceId verified;
    result = load(verified);
    if (result && !sameDeviceInstanceId(candidate, verified)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) output = verified;
    return result;
}

} // namespace blinker
