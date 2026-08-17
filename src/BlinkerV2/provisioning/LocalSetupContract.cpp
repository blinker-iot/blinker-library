#include "LocalSetupContract.h"

#include "../identity/DeviceInstanceId.h"
#include "../protocol/cbor/Cbor.h"
#include "EnrollmentTicketContract.h"

namespace blinker {

namespace {

cbor::Limits limits() {
    cbor::Limits value;
    value.maxTextLength = 0U;
    value.maxByteStringLength = kLocalSetupOperationBodyMaxEncodedSize;
    value.maxContainerItems = 8U;
    value.maxDepth = 2U;
    return value;
}

Result readKey(cbor::Reader& reader, uint64_t expected) {
    uint64_t key = 0U;
    Result result = reader.readUnsigned(key);
    if (result && key != expected) {
        result =
            Result::failure(key < expected ? ErrorCode::DuplicateField
                                           : ErrorCode::NonCanonicalEncoding);
    }
    return result;
}

Result readUint32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT32_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result)
        output = static_cast<uint32_t>(value);
    return result;
}

Result writeHeader(cbor::Writer& writer,
                   size_t pairs,
                   LocalSetupMessageType type,
                   uint32_t requestId) {
    Result result = writer.beginMap(pairs);
    if (result)
        result = writer.writeUnsigned(0U);
    if (result)
        result = writer.writeUnsigned(kLocalSetupContractVersion);
    if (result)
        result = writer.writeUnsigned(1U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(type));
    }
    if (result)
        result = writer.writeUnsigned(2U);
    if (result)
        result = writer.writeUnsigned(requestId);
    return result;
}

bool validBytes(ByteView value, size_t expected) {
    return value.data != nullptr && value.size == expected;
}

} // namespace

bool validLocalSetupOperation(LocalSetupOperation operation) {
    const uint8_t value = static_cast<uint8_t>(operation);
    return value >=
               static_cast<uint8_t>(LocalSetupOperation::BeginOwnershipClaim) &&
           value <= static_cast<uint8_t>(
                        LocalSetupOperation::CompleteCloudEnrollment);
}

bool validLocalSetupAuthorizedOperations(uint32_t operations) {
    return operations == 0U || operations == kEnrollmentTicketAuthorizeAll;
}

bool localSetupOperationAllowed(LocalSetupOperation operation,
                                uint32_t authorizedOperations) {
    if (!validLocalSetupOperation(operation) ||
        !validLocalSetupAuthorizedOperations(authorizedOperations)) {
        return false;
    }
    if (operation == LocalSetupOperation::ConfigureNetwork) {
        return (authorizedOperations &
                kEnrollmentTicketAuthorizeNetworkProvisioning) != 0U;
    }
    if (operation == LocalSetupOperation::BeginCloudEnrollment ||
        operation == LocalSetupOperation::ApplyCloudEnrollmentGrant ||
        operation == LocalSetupOperation::CompleteCloudEnrollment) {
        return (authorizedOperations &
                kEnrollmentTicketAuthorizeCloudEnrollment) != 0U;
    }
    return true;
}

Result decodeLocalSetupRequest(ByteView encoded, LocalSetupRequest& request) {
    request = LocalSetupRequest();
    if (encoded.data == nullptr || encoded.empty() ||
        encoded.size > kLocalSetupRequestMaxEncodedSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = cbor::validate(encoded, limits());
    cbor::Reader reader(encoded, limits());
    size_t pairs = 0U;
    uint32_t version = 0U;
    uint32_t type = 0U;
    if (result)
        result = reader.readMapSize(pairs);
    if (result && pairs < 3U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result)
        result = readKey(reader, 0U);
    if (result)
        result = readUint32(reader, version);
    if (result && version != kLocalSetupContractVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (result)
        result = readKey(reader, 1U);
    if (result)
        result = readUint32(reader, type);
    if (result)
        result = readKey(reader, 2U);
    if (result)
        result = readUint32(reader, request.requestId);
    if (result && request.requestId == 0U) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }

    if (result &&
        type == static_cast<uint8_t>(LocalSetupMessageType::HelloRequest)) {
        if (pairs != 3U) {
            result = Result::failure(ErrorCode::InvalidEncoding);
        } else {
            request.type = LocalSetupMessageType::HelloRequest;
        }
    } else if (result && type == static_cast<uint8_t>(
                                     LocalSetupMessageType::TicketOffer)) {
        if (pairs != 4U) {
            result = Result::failure(ErrorCode::InvalidEncoding);
        } else {
            request.type = LocalSetupMessageType::TicketOffer;
            result = readKey(reader, 3U);
            if (result)
                result = reader.readBytes(request.body);
            if (result &&
                (request.body.empty() ||
                 request.body.size > kEnrollmentTicketMaxEncodedSize)) {
                result = Result::failure(ErrorCode::ValueOutOfRange);
            }
        }
    } else if (result && type == static_cast<uint8_t>(
                                     LocalSetupMessageType::OperationRequest)) {
        uint32_t operation = 0U;
        if (pairs != 5U) {
            result = Result::failure(ErrorCode::InvalidEncoding);
        } else {
            request.type = LocalSetupMessageType::OperationRequest;
            result = readKey(reader, 3U);
            if (result)
                result = readUint32(reader, operation);
            request.operation = static_cast<LocalSetupOperation>(operation);
            if (result && !validLocalSetupOperation(request.operation)) {
                result = Result::failure(ErrorCode::UnsupportedFeature);
            }
            if (result)
                result = readKey(reader, 4U);
            if (result)
                result = reader.readBytes(request.body);
        }
    } else if (result) {
        result = Result::failure(ErrorCode::ProtocolError);
    }
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (!result)
        request = LocalSetupRequest();
    return result;
}

Result encodeLocalSetupHelloResponse(const LocalSetupHelloResponse& response,
                                     MutableByteSpan output,
                                     ByteView& encoded) {
    encoded = ByteView();
    if (response.requestId == 0U ||
        !validBytes(response.deviceInstanceId, kDeviceInstanceIdSize) ||
        !validBytes(response.setupSessionId, kLocalSetupSessionIdSize) ||
        !validBytes(response.setupSessionLocator, kSetupSessionLocatorSize) ||
        response.ownershipGeneration == 0U ||
        !validLocalSetupAuthorizedOperations(response.ticketOperations)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writeHeader(
        writer, 8U, LocalSetupMessageType::HelloResponse, response.requestId);
    if (result)
        result = writer.writeUnsigned(3U);
    if (result)
        result = writer.writeBytes(response.deviceInstanceId);
    if (result)
        result = writer.writeUnsigned(4U);
    if (result)
        result = writer.writeBytes(response.setupSessionId);
    if (result)
        result = writer.writeUnsigned(5U);
    if (result)
        result = writer.writeBytes(response.setupSessionLocator);
    if (result)
        result = writer.writeUnsigned(6U);
    if (result)
        result = writer.writeUnsigned(response.ownershipGeneration);
    if (result)
        result = writer.writeUnsigned(7U);
    if (result)
        result = writer.writeUnsigned(response.ticketOperations);
    if (result)
        encoded = writer.view();
    return result;
}

Result encodeLocalSetupTicketAccepted(const LocalSetupTicketAccepted& response,
                                      MutableByteSpan output,
                                      ByteView& encoded) {
    encoded = ByteView();
    if (response.requestId == 0U ||
        !validBytes(response.ticketId, kEnrollmentTicketIdSize) ||
        !validLocalSetupAuthorizedOperations(response.authorizedOperations)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writeHeader(
        writer, 5U, LocalSetupMessageType::TicketAccepted, response.requestId);
    if (result)
        result = writer.writeUnsigned(3U);
    if (result)
        result = writer.writeBytes(response.ticketId);
    if (result)
        result = writer.writeUnsigned(4U);
    if (result)
        result = writer.writeUnsigned(response.authorizedOperations);
    if (result)
        encoded = writer.view();
    return result;
}

Result
encodeLocalSetupOperationResponse(const LocalSetupOperationResponse& response,
                                  MutableByteSpan output,
                                  ByteView& encoded) {
    encoded = ByteView();
    if (response.requestId == 0U ||
        !validLocalSetupOperation(response.operation) ||
        response.body.size > kLocalSetupOperationResponseMaxEncodedSize ||
        (response.body.data == nullptr && !response.body.empty())) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writeHeader(writer,
                                5U,
                                LocalSetupMessageType::OperationResponse,
                                response.requestId);
    if (result)
        result = writer.writeUnsigned(3U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(response.operation));
    }
    if (result)
        result = writer.writeUnsigned(4U);
    if (result)
        result = writer.writeBytes(response.body);
    if (result)
        encoded = writer.view();
    return result;
}

Result encodeLocalSetupErrorResponse(const LocalSetupErrorResponse& response,
                                     MutableByteSpan output,
                                     ByteView& encoded) {
    encoded = ByteView();
    if (static_cast<uint16_t>(response.error) < 1000U ||
        static_cast<uint16_t>(response.error) > 1099U ||
        static_cast<uint8_t>(response.retryClass) >
            static_cast<uint8_t>(LocalSetupRetryClass::RestartSession)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writeHeader(
        writer, 5U, LocalSetupMessageType::ErrorResponse, response.requestId);
    if (result)
        result = writer.writeUnsigned(3U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint16_t>(response.error));
    }
    if (result)
        result = writer.writeUnsigned(4U);
    if (result) {
        result =
            writer.writeUnsigned(static_cast<uint8_t>(response.retryClass));
    }
    if (result)
        encoded = writer.view();
    return result;
}

void localSetupErrorFor(ErrorCode error,
                        LocalSetupWireError& wireError,
                        LocalSetupRetryClass& retryClass) {
    retryClass = LocalSetupRetryClass::Never;
    switch (error) {
    case ErrorCode::UnsupportedVersion:
        wireError = LocalSetupWireError::UnsupportedVersion;
        return;
    case ErrorCode::UnsupportedFeature:
        wireError = LocalSetupWireError::UnsupportedOperation;
        return;
    case ErrorCode::SequenceConflict:
        wireError = LocalSetupWireError::SequenceConflict;
        retryClass = LocalSetupRetryClass::CorrectSequence;
        return;
    case ErrorCode::AuthenticationRequired:
        wireError = LocalSetupWireError::AuthenticationRequired;
        retryClass = LocalSetupRetryClass::RestartSession;
        return;
    case ErrorCode::StateConflict:
    case ErrorCode::NotConfigured:
    case ErrorCode::NotConnected:
        wireError = LocalSetupWireError::StateConflict;
        retryClass = LocalSetupRetryClass::RestartSession;
        return;
    case ErrorCode::WouldBlock:
        wireError = LocalSetupWireError::Busy;
        retryClass = LocalSetupRetryClass::Backoff;
        return;
    case ErrorCode::InternalError:
        wireError = LocalSetupWireError::Internal;
        retryClass = LocalSetupRetryClass::Backoff;
        return;
    default:
        wireError = LocalSetupWireError::Malformed;
        return;
    }
}

} // namespace blinker
