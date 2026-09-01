#include "BleEnrollmentContract.h"

#include "../protocol/cbor/Cbor.h"

namespace blinker {

namespace {

cbor::Limits limits() {
    cbor::Limits value;
    value.maxByteStringLength = kBleEnrollmentGrantMaxEncodedSize;
    value.maxContainerItems = 10U;
    value.maxDepth = 1U;
    return value;
}

bool exact(ByteView value, size_t size) {
    return value.data != nullptr && value.size == size;
}

Result key(cbor::Reader& reader, uint8_t expected) {
    uint64_t actual = 0U;
    Result result = reader.readUnsigned(actual);
    return result && actual != expected
               ? Result::failure(ErrorCode::InvalidEncoding)
               : result;
}

Result readU32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT32_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<uint32_t>(value);
    return result;
}

Result header(cbor::Writer& writer,
              size_t pairs,
              BleEnrollmentMessageType type,
              uint32_t requestId) {
    if (requestId == 0U) return Result::failure(ErrorCode::InvalidArgument);
    Result result = writer.beginMap(pairs);
    if (result) result = writer.writeUnsigned(0U);
    if (result) result = writer.writeUnsigned(kBleEnrollmentContractVersion);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(type));
    if (result) result = writer.writeUnsigned(2U);
    if (result) result = writer.writeUnsigned(requestId);
    return result;
}

Result decodeHeader(cbor::Reader& reader,
                    BleEnrollmentMessageType& type,
                    uint32_t& requestId) {
    uint64_t version = 0U;
    uint64_t rawType = 0U;
    Result result = key(reader, 0U);
    if (result) result = reader.readUnsigned(version);
    if (result && version != kBleEnrollmentContractVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (result) result = key(reader, 1U);
    if (result) result = reader.readUnsigned(rawType);
    if (result && (rawType == 0U || rawType > UINT8_MAX)) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) type = static_cast<BleEnrollmentMessageType>(rawType);
    if (result) result = key(reader, 2U);
    if (result) result = readU32(reader, requestId);
    if (result && requestId == 0U) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    return result;
}

Result finish(cbor::Reader& reader) {
    return reader.finished() ? Result::success()
                             : Result::failure(ErrorCode::TrailingData);
}

} // namespace

BleEnrollmentApplicationConfig::BleEnrollmentApplicationConfig()
    : securityProfile(BleEnrollmentSecurityProfile::EducationNearby),
      serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}

Result encodeBleEnrollmentHelloRequest(uint32_t requestId,
                                       MutableByteSpan output,
                                       ByteView& encoded) {
    encoded = ByteView();
    cbor::Writer writer(output);
    Result result = header(
        writer, 3U, BleEnrollmentMessageType::HelloRequest, requestId);
    if (result) encoded = writer.view();
    return result;
}

Result encodeBleEnrollmentRequest(uint32_t requestId,
                                  ByteView grant,
                                  ByteView controllerSecret,
                                  ByteView presenceKey,
                                  MutableByteSpan output,
                                  ByteView& encoded) {
    encoded = ByteView();
    if (grant.data == nullptr || grant.empty() ||
        grant.size > kBleEnrollmentGrantMaxEncodedSize ||
        !exact(controllerSecret, kControllerCredentialSecretSize) ||
        !exact(presenceKey, kDevicePresenceKeySize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = header(
        writer, 6U, BleEnrollmentMessageType::EnrollRequest, requestId);
    if (result) result = writer.writeUnsigned(3U);
    if (result) result = writer.writeBytes(grant);
    if (result) result = writer.writeUnsigned(4U);
    if (result) result = writer.writeBytes(controllerSecret);
    if (result) result = writer.writeUnsigned(5U);
    if (result) result = writer.writeBytes(presenceKey);
    if (result) encoded = writer.view();
    return result;
}

Result decodeBleEnrollmentRequest(ByteView encoded,
                                  BleEnrollmentRequest& request) {
    request = BleEnrollmentRequest();
    if (encoded.data == nullptr || encoded.empty() ||
        encoded.size > kBleEnrollmentRequestMaxEncodedSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = cbor::validate(encoded, limits());
    cbor::Reader reader(encoded, limits());
    size_t count = 0U;
    BleEnrollmentRequest value;
    if (result) result = reader.readMapSize(count);
    if (result) result = decodeHeader(reader, value.type, value.requestId);
    if (result && value.type == BleEnrollmentMessageType::HelloRequest) {
        if (count != 3U) result = Result::failure(ErrorCode::InvalidEncoding);
    } else if (result &&
               value.type == BleEnrollmentMessageType::EnrollRequest) {
        if (count != 6U) result = Result::failure(ErrorCode::InvalidEncoding);
        if (result) result = key(reader, 3U);
        if (result) result = reader.readBytes(value.grant);
        if (result) result = key(reader, 4U);
        if (result) result = reader.readBytes(value.controllerSecret);
        if (result) result = key(reader, 5U);
        if (result) result = reader.readBytes(value.presenceKey);
        if (result && (value.grant.empty() ||
                       value.grant.size > kBleEnrollmentGrantMaxEncodedSize ||
                       !exact(value.controllerSecret,
                              kControllerCredentialSecretSize) ||
                       !exact(value.presenceKey,
                              kDevicePresenceKeySize))) {
            result = Result::failure(ErrorCode::ValueOutOfRange);
        }
    } else if (result) {
        result = Result::failure(ErrorCode::ProtocolError);
    }
    if (result) result = finish(reader);
    if (result) request = value;
    return result;
}

Result encodeBleEnrollmentHelloResponse(
    const BleEnrollmentResponse& response,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (response.type != BleEnrollmentMessageType::HelloResponse ||
        !exact(response.deviceInstanceId, kDeviceInstanceIdSize) ||
        !exact(response.setupSessionId, kSetupSessionIdSize) ||
        !exact(response.setupSessionLocator, kSetupSessionLocatorSize) ||
        response.accessEpoch == 0U || response.serverKeyId == 0U ||
        (response.securityProfile !=
             BleEnrollmentSecurityProfile::EducationNearby &&
         response.securityProfile != BleEnrollmentSecurityProfile::ProductOob) ||
        (response.signatureAlgorithm != ServerSignatureAlgorithm::Ed25519 &&
         response.signatureAlgorithm !=
             ServerSignatureAlgorithm::EcdsaP256Sha256Raw)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = header(
        writer, 10U, response.type, response.requestId);
    if (result) result = writer.writeUnsigned(3U);
    if (result) result = writer.writeBytes(response.deviceInstanceId);
    if (result) result = writer.writeUnsigned(4U);
    if (result) result = writer.writeBytes(response.setupSessionId);
    if (result) result = writer.writeUnsigned(5U);
    if (result) result = writer.writeBytes(response.setupSessionLocator);
    if (result) result = writer.writeUnsigned(6U);
    if (result) result = writer.writeUnsigned(response.accessEpoch);
    if (result) result = writer.writeUnsigned(7U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(response.securityProfile));
    }
    if (result) result = writer.writeUnsigned(8U);
    if (result) result = writer.writeUnsigned(response.serverKeyId);
    if (result) result = writer.writeUnsigned(9U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(response.signatureAlgorithm));
    }
    if (result) encoded = writer.view();
    return result;
}

Result encodeBleEnrollmentResponse(uint32_t requestId,
                                   ByteView receipt,
                                   MutableByteSpan output,
                                   ByteView& encoded) {
    encoded = ByteView();
    if (receipt.data == nullptr || receipt.empty() ||
        receipt.size > kControllerReceiptMaxEncodedSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = header(
        writer, 4U, BleEnrollmentMessageType::EnrollResponse, requestId);
    if (result) result = writer.writeUnsigned(3U);
    if (result) result = writer.writeBytes(receipt);
    if (result) encoded = writer.view();
    return result;
}

Result encodeBleEnrollmentError(uint32_t requestId,
                                BleEnrollmentWireError error,
                                BleEnrollmentRetryClass retryClass,
                                MutableByteSpan output,
                                ByteView& encoded) {
    encoded = ByteView();
    cbor::Writer writer(output);
    Result result = header(
        writer, 5U, BleEnrollmentMessageType::ErrorResponse, requestId);
    if (result) result = writer.writeUnsigned(3U);
    if (result) result = writer.writeUnsigned(static_cast<uint16_t>(error));
    if (result) result = writer.writeUnsigned(4U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(retryClass));
    if (result) encoded = writer.view();
    return result;
}

Result decodeBleEnrollmentResponse(ByteView encoded,
                                   BleEnrollmentResponse& response) {
    response = BleEnrollmentResponse();
    Result result = cbor::validate(encoded, limits());
    cbor::Reader reader(encoded, limits());
    size_t count = 0U;
    BleEnrollmentResponse value;
    if (result) result = reader.readMapSize(count);
    if (result) result = decodeHeader(reader, value.type, value.requestId);
    if (result && value.type == BleEnrollmentMessageType::HelloResponse) {
        uint64_t securityProfile = 0U;
        uint64_t signatureAlgorithm = 0U;
        if (count != 10U) result = Result::failure(ErrorCode::InvalidEncoding);
        if (result) result = key(reader, 3U);
        if (result) result = reader.readBytes(value.deviceInstanceId);
        if (result) result = key(reader, 4U);
        if (result) result = reader.readBytes(value.setupSessionId);
        if (result) result = key(reader, 5U);
        if (result) result = reader.readBytes(value.setupSessionLocator);
        if (result) result = key(reader, 6U);
        if (result) result = readU32(reader, value.accessEpoch);
        if (result) result = key(reader, 7U);
        if (result) result = reader.readUnsigned(securityProfile);
        if (result) result = key(reader, 8U);
        if (result) result = readU32(reader, value.serverKeyId);
        if (result) result = key(reader, 9U);
        if (result) result = reader.readUnsigned(signatureAlgorithm);
        if (result && securityProfile <= UINT8_MAX) {
            value.securityProfile =
                static_cast<BleEnrollmentSecurityProfile>(securityProfile);
        }
        if (result && signatureAlgorithm <= UINT8_MAX) {
            value.signatureAlgorithm =
                static_cast<ServerSignatureAlgorithm>(signatureAlgorithm);
        }
        if (result && (!exact(value.deviceInstanceId, kDeviceInstanceIdSize) ||
                       !exact(value.setupSessionId, kSetupSessionIdSize) ||
                       !exact(value.setupSessionLocator,
                               kSetupSessionLocatorSize) ||
                       value.accessEpoch == 0U || value.serverKeyId == 0U ||
                       (value.securityProfile !=
                            BleEnrollmentSecurityProfile::EducationNearby &&
                        value.securityProfile !=
                            BleEnrollmentSecurityProfile::ProductOob) ||
                       (value.signatureAlgorithm !=
                            ServerSignatureAlgorithm::Ed25519 &&
                        value.signatureAlgorithm !=
                            ServerSignatureAlgorithm::EcdsaP256Sha256Raw))) {
            result = Result::failure(ErrorCode::ValueOutOfRange);
        }
    } else if (result &&
               value.type == BleEnrollmentMessageType::EnrollResponse) {
        if (count != 4U) result = Result::failure(ErrorCode::InvalidEncoding);
        if (result) result = key(reader, 3U);
        if (result) result = reader.readBytes(value.receipt);
        if (result && (value.receipt.empty() ||
                       value.receipt.size > kControllerReceiptMaxEncodedSize)) {
            result = Result::failure(ErrorCode::ValueOutOfRange);
        }
    } else if (result &&
               value.type == BleEnrollmentMessageType::ErrorResponse) {
        uint64_t error = 0U;
        uint64_t retry = 0U;
        if (count != 5U) result = Result::failure(ErrorCode::InvalidEncoding);
        if (result) result = key(reader, 3U);
        if (result) result = reader.readUnsigned(error);
        if (result) result = key(reader, 4U);
        if (result) result = reader.readUnsigned(retry);
        if (result && (error < 1000U || error > 1099U || retry > 2U)) {
            result = Result::failure(ErrorCode::ValueOutOfRange);
        }
        if (result) {
            value.error = static_cast<BleEnrollmentWireError>(error);
            value.retryClass = static_cast<BleEnrollmentRetryClass>(retry);
        }
    } else if (result) {
        result = Result::failure(ErrorCode::ProtocolError);
    }
    if (result) result = finish(reader);
    if (result) response = value;
    return result;
}

void bleEnrollmentErrorFor(ErrorCode error,
                           BleEnrollmentWireError& wireError,
                           BleEnrollmentRetryClass& retryClass) {
    retryClass = BleEnrollmentRetryClass::Never;
    switch (error) {
    case ErrorCode::UnsupportedVersion:
        wireError = BleEnrollmentWireError::UnsupportedVersion;
        break;
    case ErrorCode::AuthenticationRequired:
        wireError = BleEnrollmentWireError::AuthenticationRequired;
        retryClass = BleEnrollmentRetryClass::RestartSession;
        break;
    case ErrorCode::StateConflict:
        wireError = BleEnrollmentWireError::StateConflict;
        break;
    case ErrorCode::SequenceConflict:
        wireError = BleEnrollmentWireError::SequenceConflict;
        retryClass = BleEnrollmentRetryClass::CorrectSequence;
        break;
    case ErrorCode::InvalidArgument:
    case ErrorCode::InvalidEncoding:
    case ErrorCode::NonCanonicalEncoding:
    case ErrorCode::DuplicateField:
    case ErrorCode::TrailingData:
    case ErrorCode::ValueOutOfRange:
    case ErrorCode::ProtocolError:
        wireError = BleEnrollmentWireError::Malformed;
        break;
    default:
        wireError = BleEnrollmentWireError::Internal;
        retryClass = BleEnrollmentRetryClass::RestartSession;
        break;
    }
}

} // namespace blinker
