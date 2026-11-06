#ifndef BLINKER_PROTOCOL_BBP2_INITIALCLOUDNETWORKRECOVERY_H
#define BLINKER_PROTOCOL_BBP2_INITIALCLOUDNETWORKRECOVERY_H

#include "Frame.h"
#include "../cbor/Cbor.h"
#include "../../identity/DeviceKey.h"

namespace blinker {
namespace bbp2 {

enum : size_t {
    kInitialCloudNetworkOperationSize = 16U,
    kInitialCloudNetworkBodySize = 36U,
    kInitialCloudNetworkFrameSize = kBaseHeaderSize + kInitialCloudNetworkBodySize
};

struct InitialCloudNetworkRecoveryBody {
    ByteView operation;
    ByteView locator;
};

inline bool exactInitialCloudNetworkBytes(ByteView value, size_t size) {
    if (value.data == nullptr || value.size != size) return false;
    uint8_t any = 0U;
    for (size_t index = 0U; index < size; ++index) any = static_cast<uint8_t>(any | value.data[index]);
    return any != 0U;
}

// Both the handoff request and the bound acceptance echo [1, operation, locator].
// Neither contains a Key or network credentials. Acceptance is not a WiFi write.
inline Result encodeInitialCloudNetworkRecoveryBody(ByteView operation, const DeviceKeyLocator& locator,
    MutableByteSpan output) {
    if (!exactInitialCloudNetworkBytes(operation, kInitialCloudNetworkOperationSize) ||
        !exactInitialCloudNetworkBytes(ByteView(locator.bytes, sizeof(locator.bytes)), sizeof(locator.bytes)))
        return Result::failure(ErrorCode::InvalidArgument);
    if (output.data == nullptr || output.size < kInitialCloudNetworkBodySize)
        return Result::failure(ErrorCode::BufferTooSmall);
    cbor::Writer writer(output);
    Result result = writer.beginArray(3U);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(operation);
    if (result) result = writer.writeBytes(ByteView(locator.bytes, sizeof(locator.bytes)));
    return result;
}

inline Result decodeInitialCloudNetworkRecoveryBody(ByteView encoded, InitialCloudNetworkRecoveryBody& output) {
    output = InitialCloudNetworkRecoveryBody();
    if (encoded.data == nullptr || encoded.size != kInitialCloudNetworkBodySize)
        return Result::failure(ErrorCode::InvalidEncoding);
    cbor::Reader reader(encoded);
    InitialCloudNetworkRecoveryBody value;
    size_t count = 0U;
    uint64_t version = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 3U) return Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = reader.readUnsigned(version);
    if (result && version != 1U) return Result::failure(ErrorCode::UnsupportedVersion);
    if (result) result = reader.readBytes(value.operation);
    if (result) result = reader.readBytes(value.locator);
    if (!result) return result;
    if (!reader.finished() || !exactInitialCloudNetworkBytes(value.operation, kInitialCloudNetworkOperationSize) ||
        !exactInitialCloudNetworkBytes(value.locator, sizeof(DeviceKeyLocator::bytes)))
        return Result::failure(ErrorCode::InvalidEncoding);
    output = value;
    return Result::success();
}

} // namespace bbp2
} // namespace blinker
#endif
