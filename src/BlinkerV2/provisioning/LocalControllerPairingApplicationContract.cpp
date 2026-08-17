#include "LocalControllerPairingApplicationContract.h"

#include "../identity/DeviceInstanceId.h"
#include "../protocol/SetupSession.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {

namespace {

cbor::Limits limits() {
    cbor::Limits value;
    value.maxTextLength = 0U;
    value.maxByteStringLength =
        kLocalControllerPairingRequestMaxEncodedSize;
    value.maxContainerItems = 5U;
    value.maxDepth = 2U;
    return value;
}

Result readKey(cbor::Reader& reader, uint64_t expected) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value != expected) {
        result = Result::failure(
            value < expected ? ErrorCode::DuplicateField
                             : ErrorCode::NonCanonicalEncoding);
    }
    return result;
}

Result readU32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT32_MAX) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<uint32_t>(value);
    return result;
}

Result writeHeader(cbor::Writer& writer,
                   size_t pairs,
                   LocalControllerPairingMessageType type,
                   uint32_t requestId) {
    Result result = writer.beginMap(pairs);
    if (result) result = writer.writeUnsigned(0U);
    if (result) {
        result = writer.writeUnsigned(
            kLocalControllerPairingApplicationVersion);
    }
    if (result) result = writer.writeUnsigned(1U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(type));
    }
    if (result) result = writer.writeUnsigned(2U);
    if (result) result = writer.writeUnsigned(requestId);
    return result;
}

bool validResponseType(LocalControllerPairingMessageType type) {
    return type == LocalControllerPairingMessageType::ChallengeResponse ||
           type == LocalControllerPairingMessageType::PairingReceipt;
}

} // namespace

Result decodeLocalControllerPairingApplicationRequest(
    ByteView encoded,
    LocalControllerPairingApplicationRequest& request) {
    request = LocalControllerPairingApplicationRequest();
    if (encoded.data == nullptr || encoded.empty() ||
        encoded.size >
            kLocalControllerPairingApplicationRequestMaxEncodedSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = cbor::validate(encoded, limits());
    cbor::Reader reader(encoded, limits());
    size_t pairs = 0U;
    uint32_t version = 0U;
    uint32_t type = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs < 3U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = readKey(reader, 0U);
    if (result) result = readU32(reader, version);
    if (result && version != kLocalControllerPairingApplicationVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (result) result = readKey(reader, 1U);
    if (result) result = readU32(reader, type);
    if (result) result = readKey(reader, 2U);
    if (result) result = readU32(reader, request.requestId);
    if (result && request.requestId == 0U) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }

    if (result &&
        type == static_cast<uint8_t>(
                    LocalControllerPairingMessageType::HelloRequest)) {
        if (pairs != 3U) {
            result = Result::failure(ErrorCode::InvalidEncoding);
        } else {
            request.type = LocalControllerPairingMessageType::HelloRequest;
        }
    } else if (result &&
               type == static_cast<uint8_t>(
                           LocalControllerPairingMessageType::ChallengeRequest)) {
        if (pairs != 3U) {
            result = Result::failure(ErrorCode::InvalidEncoding);
        } else {
            request.type =
                LocalControllerPairingMessageType::ChallengeRequest;
        }
    } else if (result &&
               type == static_cast<uint8_t>(
                           LocalControllerPairingMessageType::PairingRequest)) {
        if (pairs != 4U) {
            result = Result::failure(ErrorCode::InvalidEncoding);
        } else {
            request.type = LocalControllerPairingMessageType::PairingRequest;
            result = readKey(reader, 3U);
            if (result) result = reader.readBytes(request.body);
            if (result &&
                (request.body.empty() ||
                 request.body.size >
                     kLocalControllerPairingRequestMaxEncodedSize)) {
                result = Result::failure(ErrorCode::ValueOutOfRange);
            }
        }
    } else if (result) {
        result = Result::failure(ErrorCode::ProtocolError);
    }
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (!result) request = LocalControllerPairingApplicationRequest();
    return result;
}

Result encodeLocalControllerPairingHelloResponse(
    const LocalControllerPairingHelloResponse& response,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (response.requestId == 0U ||
        response.deviceInstanceId.data == nullptr ||
        response.deviceInstanceId.size != kDeviceInstanceIdSize ||
        response.setupSessionLocator.data == nullptr ||
        response.setupSessionLocator.size != kSetupSessionLocatorSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writeHeader(
        writer,
        5U,
        LocalControllerPairingMessageType::HelloResponse,
        response.requestId);
    if (result) result = writer.writeUnsigned(3U);
    if (result) result = writer.writeBytes(response.deviceInstanceId);
    if (result) result = writer.writeUnsigned(4U);
    if (result) result = writer.writeBytes(response.setupSessionLocator);
    if (result) encoded = writer.view();
    return result;
}

Result encodeLocalControllerPairingApplicationResponse(
    const LocalControllerPairingApplicationResponse& response,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    const size_t maximum =
        response.type == LocalControllerPairingMessageType::ChallengeResponse
            ? kLocalControllerPairingChallengeMaxEncodedSize
            : kLocalControllerPairingReceiptMaxEncodedSize;
    if (response.requestId == 0U || !validResponseType(response.type) ||
        response.body.data == nullptr || response.body.empty() ||
        response.body.size > maximum) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writeHeader(writer, 4U, response.type, response.requestId);
    if (result) result = writer.writeUnsigned(3U);
    if (result) result = writer.writeBytes(response.body);
    if (result) encoded = writer.view();
    return result;
}

Result encodeLocalControllerPairingErrorResponse(
    const LocalControllerPairingErrorResponse& response,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (static_cast<uint16_t>(response.error) < 1200U ||
        static_cast<uint16_t>(response.error) > 1299U ||
        static_cast<uint8_t>(response.retryClass) >
            static_cast<uint8_t>(
                LocalControllerPairingRetryClass::RestartSession)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writeHeader(
        writer,
        5U,
        LocalControllerPairingMessageType::ErrorResponse,
        response.requestId);
    if (result) result = writer.writeUnsigned(3U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint16_t>(response.error));
    }
    if (result) result = writer.writeUnsigned(4U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(response.retryClass));
    }
    if (result) encoded = writer.view();
    return result;
}

void localControllerPairingErrorFor(
    ErrorCode error,
    LocalControllerPairingWireError& wireError,
    LocalControllerPairingRetryClass& retryClass) {
    retryClass = LocalControllerPairingRetryClass::Never;
    switch (error) {
    case ErrorCode::UnsupportedVersion:
        wireError = LocalControllerPairingWireError::UnsupportedVersion;
        return;
    case ErrorCode::AuthenticationRequired:
        wireError =
            LocalControllerPairingWireError::AuthenticationRequired;
        retryClass = LocalControllerPairingRetryClass::PhysicalConfirm;
        return;
    case ErrorCode::SequenceConflict:
    case ErrorCode::AlreadyExists:
        wireError = LocalControllerPairingWireError::SequenceConflict;
        retryClass = LocalControllerPairingRetryClass::RestartSession;
        return;
    case ErrorCode::StateConflict:
    case ErrorCode::NotConfigured:
    case ErrorCode::NotConnected:
        wireError = LocalControllerPairingWireError::StateConflict;
        retryClass = LocalControllerPairingRetryClass::RestartSession;
        return;
    case ErrorCode::WouldBlock:
        wireError = LocalControllerPairingWireError::Busy;
        retryClass = LocalControllerPairingRetryClass::Backoff;
        return;
    case ErrorCode::InternalError:
    case ErrorCode::CapacityExceeded:
        wireError = LocalControllerPairingWireError::Internal;
        retryClass = LocalControllerPairingRetryClass::Backoff;
        return;
    default:
        wireError = LocalControllerPairingWireError::Malformed;
        return;
    }
}

} // namespace blinker
