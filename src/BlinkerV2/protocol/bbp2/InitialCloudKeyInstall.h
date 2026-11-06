#ifndef BLINKER_PROTOCOL_BBP2_INITIALCLOUDKEYINSTALL_H
#define BLINKER_PROTOCOL_BBP2_INITIALCLOUDKEYINSTALL_H

#include "Frame.h"
#include "../cbor/Cbor.h"
#include "../../identity/DeviceKey.h"

namespace blinker {
namespace bbp2 {

enum : size_t {
    kInitialCloudKeyInstallIntentSize = 16U,
    kInitialCloudKeyInstallProofSize = 32U,
    kInitialCloudKeyInstallBodySize = 87U,
    kInitialCloudKeyInstalledBodySize = 36U,
    kInitialCloudKeyInstallFrameSize = kBaseHeaderSize + kInitialCloudKeyInstallBodySize,
    kInitialCloudKeyInstalledFrameSize = kBaseHeaderSize + kInitialCloudKeyInstalledBodySize,
    kInitialCloudKeyHandoffFrameSize = kInitialCloudKeyInstalledFrameSize
};

struct InitialCloudKeyInstallBody {
    ByteView intent;
    ByteView key;
    ByteView proof;
};

struct InitialCloudKeyHandoffBody {
    ByteView intent;
    ByteView locator;
};

inline bool exactInitialCloudKeyBytes(ByteView bytes, size_t size) {
    if (bytes.data == nullptr || bytes.size != size) return false;
    uint8_t any = 0U;
    for (size_t i = 0U; i < bytes.size; ++i) any = static_cast<uint8_t>(any | bytes.data[i]);
    return any != 0U;
}

inline Result decodeInitialCloudKeyInstallBody(ByteView encoded, InitialCloudKeyInstallBody& output) {
    output = InitialCloudKeyInstallBody();
    if (encoded.size != kInitialCloudKeyInstallBodySize || encoded.data == nullptr)
        return Result::failure(ErrorCode::InvalidEncoding);
    cbor::Reader reader(encoded);
    InitialCloudKeyInstallBody value;
    size_t count = 0U;
    uint64_t version = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 4U) return Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = reader.readUnsigned(version);
    if (result && version != 1U) return Result::failure(ErrorCode::UnsupportedVersion);
    if (result) result = reader.readBytes(value.intent);
    if (result) result = reader.readBytes(value.key);
    if (result) result = reader.readBytes(value.proof);
    if (!result) return result;
    if (!reader.finished() || !exactInitialCloudKeyBytes(value.intent, kInitialCloudKeyInstallIntentSize) ||
        !exactInitialCloudKeyBytes(value.key, kDeviceKeySize) ||
        !exactInitialCloudKeyBytes(value.proof, kInitialCloudKeyInstallProofSize))
        return Result::failure(ErrorCode::InvalidEncoding);
    output = value;
    return Result::success();
}

inline Result encodeInitialCloudKeyInstalledBody(ByteView intent, const DeviceKeyLocator& locator,
    MutableByteSpan output) {
    if (!exactInitialCloudKeyBytes(intent, kInitialCloudKeyInstallIntentSize) ||
        !exactInitialCloudKeyBytes(ByteView(locator.bytes, sizeof(locator.bytes)), sizeof(locator.bytes)))
        return Result::failure(ErrorCode::InvalidArgument);
    if (output.data == nullptr || output.size < kInitialCloudKeyInstalledBodySize)
        return Result::failure(ErrorCode::BufferTooSmall);
    cbor::Writer writer(output);
    Result result = writer.beginArray(3U);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(intent);
    if (result) result = writer.writeBytes(ByteView(locator.bytes, sizeof(locator.bytes)));
    return result;
}

inline Result decodeInitialCloudKeyHandoffBody(ByteView encoded, InitialCloudKeyHandoffBody& output) {
    output = InitialCloudKeyHandoffBody();
    if (encoded.data == nullptr || encoded.size != kInitialCloudKeyInstalledBodySize)
        return Result::failure(ErrorCode::InvalidEncoding);
    cbor::Reader reader(encoded);
    InitialCloudKeyHandoffBody value;
    size_t count = 0U;
    uint64_t version = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 3U) return Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = reader.readUnsigned(version);
    if (result && version != 1U) return Result::failure(ErrorCode::UnsupportedVersion);
    if (result) result = reader.readBytes(value.intent);
    if (result) result = reader.readBytes(value.locator);
    if (!result) return result;
    if (!reader.finished() || !exactInitialCloudKeyBytes(value.intent, kInitialCloudKeyInstallIntentSize) ||
        !exactInitialCloudKeyBytes(value.locator, sizeof(DeviceKeyLocator::bytes)))
        return Result::failure(ErrorCode::InvalidEncoding);
    output = value;
    return Result::success();
}

} // namespace bbp2
} // namespace blinker
#endif
