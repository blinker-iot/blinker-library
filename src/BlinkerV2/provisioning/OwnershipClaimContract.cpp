#include "OwnershipClaimContract.h"

namespace blinker {

namespace {

const char kGrantDomain[] = "blinker.ownership-claim-grant.v1";
const char kCommitAckDomain[] = "blinker.ownership-claim-commit-ack.v1";

cbor::Limits contractLimits() {
    cbor::Limits limits;
    limits.maxTextLength = kLogicalDeviceIdCapacity;
    limits.maxByteStringLength = kOwnershipClaimGrantSignatureSize;
    limits.maxContainerItems = 16U;
    limits.maxDepth = 2U;
    return limits;
}

bool exactBytes(ByteView value, size_t expected) {
    return value.data != nullptr && value.size == expected;
}

bool allZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return true;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

bool nonZeroExact(ByteView value, size_t expected) {
    return exactBytes(value, expected) && !allZero(value);
}

bool validSignatureAlgorithm(ServerSignatureAlgorithm algorithm) {
    return algorithm == ServerSignatureAlgorithm::Ed25519 ||
           algorithm == ServerSignatureAlgorithm::EcdsaP256Sha256Raw;
}

Result validateRequest(const OwnershipClaimRequest& body) {
    return nonZeroExact(body.requestId, kOwnershipClaimRequestIdSize) &&
                   nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) &&
                   body.ownershipGeneration != 0U &&
                   nonZeroExact(body.claimNonce, kOwnershipClaimNonceSize)
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result validateGrant(
    const OwnershipClaimGrant& body,
    bool requireSignature) {
    if (!nonZeroExact(body.grantId, kOwnershipClaimGrantIdSize) ||
        !nonZeroExact(body.requestId, kOwnershipClaimRequestIdSize) ||
        !nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) ||
        !validateLogicalDeviceId(body.logicalDeviceId) ||
        body.ownershipGeneration == 0U ||
        !nonZeroExact(body.claimNonce, kOwnershipClaimNonceSize) ||
        body.issuedAt == 0U || body.expiresAt <= body.issuedAt ||
        body.serverKeyId == 0U ||
        !validSignatureAlgorithm(body.signatureAlgorithm) ||
        (requireSignature
             ? !exactBytes(
                   body.signature,
                   kOwnershipClaimGrantSignatureSize)
             : (!body.signature.empty() &&
                !exactBytes(
                    body.signature,
                    kOwnershipClaimGrantSignatureSize)))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result validateReceipt(const OwnershipClaimReceipt& body) {
    return nonZeroExact(body.grantId, kOwnershipClaimGrantIdSize) &&
                   nonZeroExact(body.requestId, kOwnershipClaimRequestIdSize) &&
                   nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) &&
                   validateLogicalDeviceId(body.logicalDeviceId).ok() &&
                   body.ownershipGeneration != 0U &&
                   nonZeroExact(
                       body.grantDigest,
                       kOwnershipClaimGrantDigestSize)
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result validateCommit(const OwnershipClaimCommitRequest& body) {
    return nonZeroExact(body.grantId, kOwnershipClaimGrantIdSize) &&
                   nonZeroExact(body.requestId, kOwnershipClaimRequestIdSize) &&
                   nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) &&
                   validateLogicalDeviceId(body.logicalDeviceId).ok() &&
                   body.ownershipGeneration != 0U &&
                   nonZeroExact(body.receiptDigest, kSha256Size) &&
                   nonZeroExact(
                       body.idempotencyKey,
                       kOwnershipClaimCommitIdempotencyKeySize)
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result validateCommitAck(
    const OwnershipClaimCommitAck& body,
    bool requireSignature) {
    if (!nonZeroExact(body.grantId, kOwnershipClaimGrantIdSize) ||
        !nonZeroExact(body.requestId, kOwnershipClaimRequestIdSize) ||
        !nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) ||
        !validateLogicalDeviceId(body.logicalDeviceId) ||
        body.ownershipGeneration == 0U ||
        !nonZeroExact(body.receiptDigest, kSha256Size) ||
        body.committedAt == 0U || body.serverKeyId == 0U ||
        !validSignatureAlgorithm(body.signatureAlgorithm) ||
        (requireSignature
             ? !exactBytes(
                   body.signature,
                   kOwnershipClaimGrantSignatureSize)
             : (!body.signature.empty() &&
                !exactBytes(
                    body.signature,
                    kOwnershipClaimGrantSignatureSize)))) {
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
    if (result) output = static_cast<uint32_t>(value);
    return result;
}

Result beginMapDecode(
    ByteView encoded,
    cbor::Reader& reader,
    size_t expectedCount) {
    Result result = cbor::validate(encoded, contractLimits());
    size_t count = 0U;
    if (result) result = reader.readMapSize(count);
    return result && count != expectedCount
               ? Result::failure(ErrorCode::InvalidEncoding)
               : result;
}

Result finishDecode(cbor::Reader& reader) {
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

Result readVersion(cbor::Reader& reader) {
    Result result = readKey(reader, 0U);
    uint64_t value = 0U;
    if (result) result = reader.readUnsigned(value);
    return result && value != kOwnershipClaimContractVersion
               ? Result::failure(ErrorCode::UnsupportedVersion)
               : result;
}

} // namespace

OwnershipClaimGrant::OwnershipClaimGrant()
    : ownershipGeneration(0U),
      issuedAt(0U),
      expiresAt(0U),
      serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}

OwnershipClaimCommitAck::OwnershipClaimCommitAck()
    : ownershipGeneration(0U),
      committedAt(0U),
      serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}

Result encodeOwnershipClaimRequest(
    const OwnershipClaimRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateRequest(body);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(5U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kOwnershipClaimContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeBytes(body.claimNonce);
    if (result) encoded = writer.view();
    return result;
}

Result decodeOwnershipClaimRequest(
    ByteView encoded,
    OwnershipClaimRequest& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 5U);
    OwnershipClaimRequest decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 3U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readBytes(decoded.claimNonce);
    if (result) result = finishDecode(reader);
    if (result) result = validateRequest(decoded);
    if (result) body = decoded;
    return result;
}

Result encodeOwnershipClaimGrant(
    const OwnershipClaimGrant& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateGrant(body, true);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(12U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kOwnershipClaimContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeBytes(body.claimNonce);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeUnsigned(body.issuedAt);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeUnsigned(body.expiresAt);
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) result = writeKey(writer, 10U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.signatureAlgorithm));
    }
    if (result) result = writeKey(writer, 11U);
    if (result) result = writer.writeBytes(body.signature);
    if (result) encoded = writer.view();
    return result;
}

Result decodeOwnershipClaimGrant(
    ByteView encoded,
    OwnershipClaimGrant& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 12U);
    OwnershipClaimGrant decoded;
    uint64_t value = 0U;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.grantId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readText(decoded.logicalDeviceId);
    if (result) result = readKey(reader, 5U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = readKey(reader, 6U);
    if (result) result = reader.readBytes(decoded.claimNonce);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readUnsigned(decoded.issuedAt);
    if (result) result = readKey(reader, 8U);
    if (result) result = reader.readUnsigned(decoded.expiresAt);
    if (result) result = readKey(reader, 9U);
    if (result) result = readU32(reader, decoded.serverKeyId);
    if (result) result = readKey(reader, 10U);
    if (result) result = reader.readUnsigned(value);
    if (result) {
        decoded.signatureAlgorithm =
            static_cast<ServerSignatureAlgorithm>(value);
    }
    if (result) result = readKey(reader, 11U);
    if (result) result = reader.readBytes(decoded.signature);
    if (result) result = finishDecode(reader);
    if (result) result = validateGrant(decoded, true);
    if (result) body = decoded;
    return result;
}

Result encodeOwnershipClaimGrantTranscript(
    const OwnershipClaimGrant& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateGrant(body, false);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(12U);
    if (result) {
        result = writer.writeText(
            StringView(kGrantDomain, sizeof(kGrantDomain) - 1U));
    }
    if (result) result = writer.writeUnsigned(kOwnershipClaimContractVersion);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writer.writeBytes(body.claimNonce);
    if (result) result = writer.writeUnsigned(body.issuedAt);
    if (result) result = writer.writeUnsigned(body.expiresAt);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.signatureAlgorithm));
    }
    if (result) encoded = writer.view();
    return result;
}

Result encodeOwnershipClaimReceipt(
    const OwnershipClaimReceipt& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateReceipt(body);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(7U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kOwnershipClaimContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeBytes(body.grantDigest);
    if (result) encoded = writer.view();
    return result;
}

Result decodeOwnershipClaimReceipt(
    ByteView encoded,
    OwnershipClaimReceipt& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 7U);
    OwnershipClaimReceipt decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.grantId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readText(decoded.logicalDeviceId);
    if (result) result = readKey(reader, 5U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = readKey(reader, 6U);
    if (result) result = reader.readBytes(decoded.grantDigest);
    if (result) result = finishDecode(reader);
    if (result) result = validateReceipt(decoded);
    if (result) body = decoded;
    return result;
}

Result encodeOwnershipClaimCommitRequest(
    const OwnershipClaimCommitRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateCommit(body);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(8U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kOwnershipClaimContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeBytes(body.receiptDigest);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeBytes(body.idempotencyKey);
    if (result) encoded = writer.view();
    return result;
}

Result decodeOwnershipClaimCommitRequest(
    ByteView encoded,
    OwnershipClaimCommitRequest& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 8U);
    OwnershipClaimCommitRequest decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.grantId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readText(decoded.logicalDeviceId);
    if (result) result = readKey(reader, 5U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = readKey(reader, 6U);
    if (result) result = reader.readBytes(decoded.receiptDigest);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readBytes(decoded.idempotencyKey);
    if (result) result = finishDecode(reader);
    if (result) result = validateCommit(decoded);
    if (result) body = decoded;
    return result;
}

Result encodeOwnershipClaimCommitAck(
    const OwnershipClaimCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateCommitAck(body, true);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(11U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kOwnershipClaimContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeBytes(body.receiptDigest);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeUnsigned(body.committedAt);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) result = writeKey(writer, 9U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.signatureAlgorithm));
    }
    if (result) result = writeKey(writer, 10U);
    if (result) result = writer.writeBytes(body.signature);
    if (result) encoded = writer.view();
    return result;
}

Result decodeOwnershipClaimCommitAck(
    ByteView encoded,
    OwnershipClaimCommitAck& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 11U);
    OwnershipClaimCommitAck decoded;
    uint64_t value = 0U;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.grantId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readText(decoded.logicalDeviceId);
    if (result) result = readKey(reader, 5U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = readKey(reader, 6U);
    if (result) result = reader.readBytes(decoded.receiptDigest);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readUnsigned(decoded.committedAt);
    if (result) result = readKey(reader, 8U);
    if (result) result = readU32(reader, decoded.serverKeyId);
    if (result) result = readKey(reader, 9U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        decoded.signatureAlgorithm =
            static_cast<ServerSignatureAlgorithm>(value);
    }
    if (result) result = readKey(reader, 10U);
    if (result) result = reader.readBytes(decoded.signature);
    if (result) result = finishDecode(reader);
    if (result) result = validateCommitAck(decoded, true);
    if (result) body = decoded;
    return result;
}

Result encodeOwnershipClaimCommitAckTranscript(
    const OwnershipClaimCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateCommitAck(body, false);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(11U);
    if (result) {
        result = writer.writeText(
            StringView(kCommitAckDomain, sizeof(kCommitAckDomain) - 1U));
    }
    if (result) result = writer.writeUnsigned(kOwnershipClaimContractVersion);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writer.writeBytes(body.receiptDigest);
    if (result) result = writer.writeUnsigned(body.committedAt);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.signatureAlgorithm));
    }
    if (result) encoded = writer.view();
    return result;
}

} // namespace blinker
