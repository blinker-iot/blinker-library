#include "EnrollmentTicketContract.h"

namespace blinker {

namespace {

const char kTicketDomain[] = "blinker.enrollment-ticket.v1";

cbor::Limits contractLimits() {
    cbor::Limits limits;
    limits.maxByteStringLength = kEnrollmentTicketSignatureSize;
    limits.maxContainerItems = 16U;
    limits.maxDepth = 2U;
    return limits;
}

bool exactNonZero(ByteView value, size_t expected) {
    if (value.data == nullptr || value.size != expected)
        return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool validAlgorithm(ServerSignatureAlgorithm algorithm) {
    return algorithm == ServerSignatureAlgorithm::Ed25519 ||
           algorithm == ServerSignatureAlgorithm::EcdsaP256Sha256Raw;
}

bool validOperations(uint32_t operations) {
    return operations == 0U || operations == kEnrollmentTicketAuthorizeAll;
}

Result validateTicket(const EnrollmentTicket& ticket, bool requireSignature) {
    if (!exactNonZero(ticket.ticketId, kEnrollmentTicketIdSize) ||
        !exactNonZero(ticket.deviceInstanceId, kDeviceInstanceIdSize) ||
        !exactNonZero(ticket.setupSessionId,
                      kEnrollmentTicketSetupSessionIdSize) ||
        !exactNonZero(ticket.setupTranscriptHash,
                      kEnrollmentTicketSetupTranscriptHashSize) ||
        ticket.ownershipGeneration == 0U ||
        !exactNonZero(ticket.controllerId, kControllerIdSize) ||
        !exactNonZero(ticket.controllerSecretDigest,
                      kEnrollmentTicketControllerSecretDigestSize) ||
        !validAuthorizationPermissions(ticket.controllerPermissions) ||
        !validOperations(ticket.authorizedOperations) ||
        !exactNonZero(ticket.ticketNonce, kEnrollmentTicketNonceSize) ||
        ticket.issuedAt == 0U || ticket.expiresAt <= ticket.issuedAt ||
        ticket.expiresAt - ticket.issuedAt >
            kEnrollmentTicketMaxLifetimeSeconds ||
        ticket.serverKeyId == 0U ||
        !validAlgorithm(ticket.signatureAlgorithm) ||
        (requireSignature
             ? !exactNonZero(ticket.signature, kEnrollmentTicketSignatureSize)
             : (!ticket.signature.empty() &&
                ticket.signature.size != kEnrollmentTicketSignatureSize))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result writeKey(cbor::Writer& writer, uint8_t key) {
    return writer.writeUnsigned(key);
}

Result readKey(cbor::Reader& reader, uint8_t expected) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    return result && value != expected
               ? Result::failure(ErrorCode::InvalidEncoding)
               : result;
}

Result readU32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT32_MAX) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result)
        output = static_cast<uint32_t>(value);
    return result;
}

Result readVersion(cbor::Reader& reader) {
    Result result = readKey(reader, 0U);
    uint64_t version = 0U;
    if (result)
        result = reader.readUnsigned(version);
    return result && version != kEnrollmentTicketContractVersion
               ? Result::failure(ErrorCode::UnsupportedVersion)
               : result;
}

} // namespace

EnrollmentTicket::EnrollmentTicket()
    : ownershipGeneration(0U), controllerPermissions(0U),
      authorizedOperations(0U), issuedAt(0U), expiresAt(0U), serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}

Result encodeEnrollmentTicket(const EnrollmentTicket& ticket,
                              MutableByteSpan output,
                              ByteView& encoded) {
    encoded = ByteView();
    Result result = validateTicket(ticket, true);
    if (!result)
        return result;

    cbor::Writer writer(output);
    result = writer.beginMap(16U);
    if (result)
        result = writeKey(writer, 0U);
    if (result)
        result = writer.writeUnsigned(kEnrollmentTicketContractVersion);
    if (result)
        result = writeKey(writer, 1U);
    if (result)
        result = writer.writeBytes(ticket.ticketId);
    if (result)
        result = writeKey(writer, 2U);
    if (result)
        result = writer.writeBytes(ticket.deviceInstanceId);
    if (result)
        result = writeKey(writer, 3U);
    if (result)
        result = writer.writeBytes(ticket.setupSessionId);
    if (result)
        result = writeKey(writer, 4U);
    if (result)
        result = writer.writeBytes(ticket.setupTranscriptHash);
    if (result)
        result = writeKey(writer, 5U);
    if (result)
        result = writer.writeUnsigned(ticket.ownershipGeneration);
    if (result)
        result = writeKey(writer, 6U);
    if (result)
        result = writer.writeBytes(ticket.controllerId);
    if (result)
        result = writeKey(writer, 7U);
    if (result)
        result = writer.writeBytes(ticket.controllerSecretDigest);
    if (result)
        result = writeKey(writer, 8U);
    if (result)
        result = writer.writeUnsigned(ticket.controllerPermissions);
    if (result)
        result = writeKey(writer, 9U);
    if (result)
        result = writer.writeUnsigned(ticket.authorizedOperations);
    if (result)
        result = writeKey(writer, 10U);
    if (result)
        result = writer.writeBytes(ticket.ticketNonce);
    if (result)
        result = writeKey(writer, 11U);
    if (result)
        result = writer.writeUnsigned(ticket.issuedAt);
    if (result)
        result = writeKey(writer, 12U);
    if (result)
        result = writer.writeUnsigned(ticket.expiresAt);
    if (result)
        result = writeKey(writer, 13U);
    if (result)
        result = writer.writeUnsigned(ticket.serverKeyId);
    if (result)
        result = writeKey(writer, 14U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(ticket.signatureAlgorithm));
    }
    if (result)
        result = writeKey(writer, 15U);
    if (result)
        result = writer.writeBytes(ticket.signature);
    if (result)
        encoded = writer.view();
    return result;
}

Result decodeEnrollmentTicket(ByteView encoded, EnrollmentTicket& ticket) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = cbor::validate(encoded, contractLimits());
    size_t count = 0U;
    if (result)
        result = reader.readMapSize(count);
    if (result && count != 16U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }

    EnrollmentTicket decoded;
    uint64_t algorithm = 0U;
    if (result)
        result = readVersion(reader);
    if (result)
        result = readKey(reader, 1U);
    if (result)
        result = reader.readBytes(decoded.ticketId);
    if (result)
        result = readKey(reader, 2U);
    if (result)
        result = reader.readBytes(decoded.deviceInstanceId);
    if (result)
        result = readKey(reader, 3U);
    if (result)
        result = reader.readBytes(decoded.setupSessionId);
    if (result)
        result = readKey(reader, 4U);
    if (result)
        result = reader.readBytes(decoded.setupTranscriptHash);
    if (result)
        result = readKey(reader, 5U);
    if (result)
        result = readU32(reader, decoded.ownershipGeneration);
    if (result)
        result = readKey(reader, 6U);
    if (result)
        result = reader.readBytes(decoded.controllerId);
    if (result)
        result = readKey(reader, 7U);
    if (result)
        result = reader.readBytes(decoded.controllerSecretDigest);
    if (result)
        result = readKey(reader, 8U);
    if (result)
        result = readU32(reader, decoded.controllerPermissions);
    if (result)
        result = readKey(reader, 9U);
    if (result)
        result = readU32(reader, decoded.authorizedOperations);
    if (result)
        result = readKey(reader, 10U);
    if (result)
        result = reader.readBytes(decoded.ticketNonce);
    if (result)
        result = readKey(reader, 11U);
    if (result)
        result = reader.readUnsigned(decoded.issuedAt);
    if (result)
        result = readKey(reader, 12U);
    if (result)
        result = reader.readUnsigned(decoded.expiresAt);
    if (result)
        result = readKey(reader, 13U);
    if (result)
        result = readU32(reader, decoded.serverKeyId);
    if (result)
        result = readKey(reader, 14U);
    if (result)
        result = reader.readUnsigned(algorithm);
    if (result && algorithm > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        decoded.signatureAlgorithm =
            static_cast<ServerSignatureAlgorithm>(algorithm);
    }
    if (result)
        result = readKey(reader, 15U);
    if (result)
        result = reader.readBytes(decoded.signature);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result)
        result = validateTicket(decoded, true);
    if (result)
        ticket = decoded;
    return result;
}

Result encodeEnrollmentTicketTranscript(const EnrollmentTicket& ticket,
                                        MutableByteSpan output,
                                        ByteView& encoded) {
    encoded = ByteView();
    Result result = validateTicket(ticket, false);
    if (!result)
        return result;

    cbor::Writer writer(output);
    result = writer.beginArray(16U);
    if (result) {
        result = writer.writeText(
            StringView(kTicketDomain, sizeof(kTicketDomain) - 1U));
    }
    if (result)
        result = writer.writeUnsigned(kEnrollmentTicketContractVersion);
    if (result)
        result = writer.writeBytes(ticket.ticketId);
    if (result)
        result = writer.writeBytes(ticket.deviceInstanceId);
    if (result)
        result = writer.writeBytes(ticket.setupSessionId);
    if (result)
        result = writer.writeBytes(ticket.setupTranscriptHash);
    if (result)
        result = writer.writeUnsigned(ticket.ownershipGeneration);
    if (result)
        result = writer.writeBytes(ticket.controllerId);
    if (result)
        result = writer.writeBytes(ticket.controllerSecretDigest);
    if (result)
        result = writer.writeUnsigned(ticket.controllerPermissions);
    if (result)
        result = writer.writeUnsigned(ticket.authorizedOperations);
    if (result)
        result = writer.writeBytes(ticket.ticketNonce);
    if (result)
        result = writer.writeUnsigned(ticket.issuedAt);
    if (result)
        result = writer.writeUnsigned(ticket.expiresAt);
    if (result)
        result = writer.writeUnsigned(ticket.serverKeyId);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(ticket.signatureAlgorithm));
    }
    if (result)
        encoded = writer.view();
    return result;
}

} // namespace blinker
