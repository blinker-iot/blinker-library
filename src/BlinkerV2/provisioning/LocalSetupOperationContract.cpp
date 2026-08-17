#include "LocalSetupOperationContract.h"

#include "../protocol/cbor/Cbor.h"
#include "WifiCredential.h"

namespace blinker {

namespace {

cbor::Limits limits(size_t maximumBytes, size_t pairs) {
    cbor::Limits value;
    value.maxTextLength = 0U;
    value.maxByteStringLength = maximumBytes;
    value.maxContainerItems = pairs;
    value.maxDepth = 1U;
    return value;
}

Result key(cbor::Reader& reader, uint64_t expected) {
    uint64_t actual = 0U;
    Result result = reader.readUnsigned(actual);
    if (result && actual != expected) {
        result = Result::failure(actual < expected
                                     ? ErrorCode::DuplicateField
                                     : ErrorCode::NonCanonicalEncoding);
    }
    return result;
}

Result version(cbor::Reader& reader) {
    uint64_t value = 0U;
    Result result = key(reader, 0U);
    if (result) result = reader.readUnsigned(value);
    if (result && value != kLocalSetupOperationContractVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    return result;
}

Result finish(cbor::Reader& reader) {
    return reader.finished() ? Result::success()
                             : Result::failure(ErrorCode::TrailingData);
}

} // namespace

Result encodeInitialControllerInstall(
    const InitialControllerInstall& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (body.controllerSecret.data == nullptr ||
        body.controllerSecret.size != kControllerCredentialSecretSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(2U);
    if (result) result = writer.writeUnsigned(0U);
    if (result) result = writer.writeUnsigned(kLocalSetupOperationContractVersion);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(body.controllerSecret);
    if (result) encoded = writer.view();
    return result;
}

Result decodeInitialControllerInstall(
    ByteView encoded,
    InitialControllerInstall& body) {
    body = InitialControllerInstall();
    const cbor::Limits valueLimits = limits(kControllerCredentialSecretSize, 2U);
    Result result = cbor::validate(encoded, valueLimits);
    cbor::Reader reader(encoded, valueLimits);
    size_t pairs = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs != 2U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = version(reader);
    if (result) result = key(reader, 1U);
    if (result) result = reader.readBytes(body.controllerSecret);
    if (result && body.controllerSecret.size != kControllerCredentialSecretSize) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) result = finish(reader);
    if (!result) body = InitialControllerInstall();
    return result;
}

Result encodeConfigureNetworkRequest(
    const ConfigureNetworkRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateWifiNetworkConfig(body.network);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(5U);
    if (result) result = writer.writeUnsigned(0U);
    if (result) result = writer.writeUnsigned(kLocalSetupOperationContractVersion);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(body.network.ssid);
    if (result) result = writer.writeUnsigned(2U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.network.authentication));
    }
    if (result) result = writer.writeUnsigned(3U);
    if (result) result = writer.writeBytes(body.network.credential);
    if (result) result = writer.writeUnsigned(4U);
    if (result) result = writer.writeBool(body.network.hidden);
    if (result) encoded = writer.view();
    return result;
}

Result decodeConfigureNetworkRequest(
    ByteView encoded,
    ConfigureNetworkRequest& body) {
    body = ConfigureNetworkRequest();
    const cbor::Limits valueLimits = limits(kWifiCredentialMaxSize, 5U);
    Result result = cbor::validate(encoded, valueLimits);
    cbor::Reader reader(encoded, valueLimits);
    size_t pairs = 0U;
    uint64_t authentication = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs != 5U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = version(reader);
    if (result) result = key(reader, 1U);
    if (result) result = reader.readBytes(body.network.ssid);
    if (result) result = key(reader, 2U);
    if (result) result = reader.readUnsigned(authentication);
    if (result && authentication > static_cast<uint8_t>(WifiAuthentication::Wpa2Wpa3Personal)) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        body.network.authentication = static_cast<WifiAuthentication>(authentication);
    }
    if (result) result = key(reader, 3U);
    if (result) result = reader.readBytes(body.network.credential);
    if (result) result = key(reader, 4U);
    if (result) result = reader.readBool(body.network.hidden);
    if (result) result = finish(reader);
    if (result) result = validateWifiNetworkConfig(body.network);
    if (!result) body = ConfigureNetworkRequest();
    return result;
}

Result encodeConfigureNetworkResponse(
    const ConfigureNetworkResponse& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (body.networkGeneration == 0U ||
        body.disposition != ConfigureNetworkDisposition::CandidateStored) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(3U);
    if (result) result = writer.writeUnsigned(0U);
    if (result) result = writer.writeUnsigned(kLocalSetupOperationContractVersion);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeUnsigned(body.networkGeneration);
    if (result) result = writer.writeUnsigned(2U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.disposition));
    if (result) encoded = writer.view();
    return result;
}

Result decodeConfigureNetworkResponse(
    ByteView encoded,
    ConfigureNetworkResponse& body) {
    body = ConfigureNetworkResponse();
    const cbor::Limits valueLimits = limits(0U, 3U);
    Result result = cbor::validate(encoded, valueLimits);
    cbor::Reader reader(encoded, valueLimits);
    size_t pairs = 0U;
    uint64_t generation = 0U;
    uint64_t disposition = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs != 3U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = version(reader);
    if (result) result = key(reader, 1U);
    if (result) result = reader.readUnsigned(generation);
    if (result && (generation == 0U || generation > UINT32_MAX)) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) result = key(reader, 2U);
    if (result) result = reader.readUnsigned(disposition);
    if (result && disposition != static_cast<uint8_t>(ConfigureNetworkDisposition::CandidateStored)) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) result = finish(reader);
    if (result) {
        body.networkGeneration = static_cast<uint32_t>(generation);
        body.disposition = static_cast<ConfigureNetworkDisposition>(disposition);
    } else {
        body = ConfigureNetworkResponse();
    }
    return result;
}

} // namespace blinker
