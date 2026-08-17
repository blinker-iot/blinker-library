#include "OwnershipReleaseContract.h"

namespace blinker {

namespace {

const char kGrantDomain[] = "blinker.ownership-release-grant.v1";
const char kAckDomain[] = "blinker.ownership-release-commit-ack.v1";

cbor::Limits contractLimits() {
    cbor::Limits limits;
    limits.maxTextLength = kLogicalDeviceIdCapacity;
    limits.maxByteStringLength = kOwnershipReleaseSignatureSize;
    limits.maxContainerItems = 20U;
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

bool validOperation(OwnershipReleaseOperation operation) {
    return operation == OwnershipReleaseOperation::Reset ||
           operation == OwnershipReleaseOperation::Transfer;
}

bool validPolicy(OwnershipReleaseNetworkPolicy policy) {
    return policy == OwnershipReleaseNetworkPolicy::Preserve ||
           policy == OwnershipReleaseNetworkPolicy::Clear;
}

bool validOperationPolicy(
    OwnershipReleaseOperation operation,
    OwnershipReleaseNetworkPolicy policy) {
    return validOperation(operation) && validPolicy(policy) &&
           (operation != OwnershipReleaseOperation::Transfer ||
            policy == OwnershipReleaseNetworkPolicy::Clear);
}

bool validSignatureAlgorithm(ServerSignatureAlgorithm algorithm) {
    return algorithm == ServerSignatureAlgorithm::Ed25519 ||
           algorithm == ServerSignatureAlgorithm::EcdsaP256Sha256Raw;
}

bool nextGeneration(uint32_t current, uint32_t next) {
    return current != 0U && current != UINT32_MAX && next == current + 1U;
}

Result validateRequest(const OwnershipReleaseRequest& body) {
    return nonZeroExact(body.requestId, kOwnershipReleaseRequestIdSize) &&
                   nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) &&
                   validateLogicalDeviceId(body.logicalDeviceId).ok() &&
                   body.currentGeneration != 0U &&
                   body.currentGeneration != UINT32_MAX &&
                   nonZeroExact(body.releaseNonce, kOwnershipReleaseNonceSize) &&
                   validOperationPolicy(body.operation, body.networkPolicy)
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result validateGrant(
    const OwnershipReleaseGrant& body,
    bool requireSignature) {
    if (!nonZeroExact(body.grantId, kOwnershipReleaseGrantIdSize) ||
        !nonZeroExact(body.requestId, kOwnershipReleaseRequestIdSize) ||
        !nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) ||
        !validateLogicalDeviceId(body.logicalDeviceId) ||
        !nextGeneration(body.currentGeneration, body.nextGeneration) ||
        !nonZeroExact(body.releaseNonce, kOwnershipReleaseNonceSize) ||
        !validOperationPolicy(body.operation, body.networkPolicy) ||
        !nonZeroExact(body.requestDigest, kSha256Size) ||
        body.issuedAt == 0U || body.expiresAt <= body.issuedAt ||
        body.serverKeyId == 0U ||
        !validSignatureAlgorithm(body.signatureAlgorithm) ||
        (requireSignature
             ? !exactBytes(body.signature, kOwnershipReleaseSignatureSize)
             : (!body.signature.empty() &&
                !exactBytes(body.signature, kOwnershipReleaseSignatureSize)))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result validateReceipt(const OwnershipReleaseReceipt& body) {
    return nonZeroExact(body.grantId, kOwnershipReleaseGrantIdSize) &&
                   nonZeroExact(body.requestId, kOwnershipReleaseRequestIdSize) &&
                   nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) &&
                   validateLogicalDeviceId(body.logicalDeviceId).ok() &&
                   nextGeneration(
                       body.currentGeneration,
                       body.nextGeneration) &&
                   validOperationPolicy(body.operation, body.networkPolicy) &&
                   nonZeroExact(body.grantDigest, kSha256Size)
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result validateAck(
    const OwnershipReleaseCommitAck& body,
    bool requireSignature) {
    if (!nonZeroExact(body.grantId, kOwnershipReleaseGrantIdSize) ||
        !nonZeroExact(body.requestId, kOwnershipReleaseRequestIdSize) ||
        !nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) ||
        !validateLogicalDeviceId(body.logicalDeviceId) ||
        !nextGeneration(body.currentGeneration, body.nextGeneration) ||
        !validOperationPolicy(body.operation, body.networkPolicy) ||
        !nonZeroExact(body.receiptDigest, kSha256Size) ||
        body.serverKeyId == 0U ||
        !validSignatureAlgorithm(body.signatureAlgorithm) ||
        (requireSignature
             ? !exactBytes(body.signature, kOwnershipReleaseSignatureSize)
             : (!body.signature.empty() &&
                !exactBytes(body.signature, kOwnershipReleaseSignatureSize)))) {
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

Result readOperation(
    cbor::Reader& reader,
    OwnershipReleaseOperation& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<OwnershipReleaseOperation>(value);
    return result;
}

Result readPolicy(
    cbor::Reader& reader,
    OwnershipReleaseNetworkPolicy& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<OwnershipReleaseNetworkPolicy>(value);
    return result;
}

Result readSignatureAlgorithm(
    cbor::Reader& reader,
    ServerSignatureAlgorithm& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<ServerSignatureAlgorithm>(value);
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
    return result && value != kOwnershipReleaseContractVersion
               ? Result::failure(ErrorCode::UnsupportedVersion)
               : result;
}

} // namespace

OwnershipReleaseRequest::OwnershipReleaseRequest()
    : currentGeneration(0U),
      operation(OwnershipReleaseOperation::Reset),
      networkPolicy(OwnershipReleaseNetworkPolicy::Preserve) {}

OwnershipReleaseGrant::OwnershipReleaseGrant()
    : currentGeneration(0U),
      nextGeneration(0U),
      operation(OwnershipReleaseOperation::Reset),
      networkPolicy(OwnershipReleaseNetworkPolicy::Preserve),
      issuedAt(0U),
      expiresAt(0U),
      serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}

OwnershipReleaseReceipt::OwnershipReleaseReceipt()
    : currentGeneration(0U),
      nextGeneration(0U),
      operation(OwnershipReleaseOperation::Reset),
      networkPolicy(OwnershipReleaseNetworkPolicy::Preserve) {}

OwnershipReleaseCommitAck::OwnershipReleaseCommitAck()
    : currentGeneration(0U),
      nextGeneration(0U),
      operation(OwnershipReleaseOperation::Reset),
      networkPolicy(OwnershipReleaseNetworkPolicy::Preserve),
      serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}

Result encodeOwnershipReleaseRequest(
    const OwnershipReleaseRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateRequest(body);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(8U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kOwnershipReleaseContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeUnsigned(body.currentGeneration);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeBytes(body.releaseNonce);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.networkPolicy));
    if (result) encoded = writer.view();
    return result;
}

Result decodeOwnershipReleaseRequest(
    ByteView encoded,
    OwnershipReleaseRequest& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 8U);
    OwnershipReleaseRequest decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readText(decoded.logicalDeviceId);
    if (result) result = readKey(reader, 4U);
    if (result) result = readU32(reader, decoded.currentGeneration);
    if (result) result = readKey(reader, 5U);
    if (result) result = reader.readBytes(decoded.releaseNonce);
    if (result) result = readKey(reader, 6U);
    if (result) result = readOperation(reader, decoded.operation);
    if (result) result = readKey(reader, 7U);
    if (result) result = readPolicy(reader, decoded.networkPolicy);
    if (result) result = finishDecode(reader);
    if (result) result = validateRequest(decoded);
    if (result) body = decoded;
    return result;
}

Result encodeOwnershipReleaseGrant(
    const OwnershipReleaseGrant& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateGrant(body, true);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(16U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kOwnershipReleaseContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.currentGeneration);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(body.nextGeneration);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeBytes(body.releaseNonce);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.networkPolicy));
    if (result) result = writeKey(writer, 10U);
    if (result) result = writer.writeBytes(body.requestDigest);
    if (result) result = writeKey(writer, 11U);
    if (result) result = writer.writeUnsigned(body.issuedAt);
    if (result) result = writeKey(writer, 12U);
    if (result) result = writer.writeUnsigned(body.expiresAt);
    if (result) result = writeKey(writer, 13U);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) result = writeKey(writer, 14U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.signatureAlgorithm));
    if (result) result = writeKey(writer, 15U);
    if (result) result = writer.writeBytes(body.signature);
    if (result) encoded = writer.view();
    return result;
}

Result decodeOwnershipReleaseGrant(
    ByteView encoded,
    OwnershipReleaseGrant& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 16U);
    OwnershipReleaseGrant decoded;
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
    if (result) result = readU32(reader, decoded.currentGeneration);
    if (result) result = readKey(reader, 6U);
    if (result) result = readU32(reader, decoded.nextGeneration);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readBytes(decoded.releaseNonce);
    if (result) result = readKey(reader, 8U);
    if (result) result = readOperation(reader, decoded.operation);
    if (result) result = readKey(reader, 9U);
    if (result) result = readPolicy(reader, decoded.networkPolicy);
    if (result) result = readKey(reader, 10U);
    if (result) result = reader.readBytes(decoded.requestDigest);
    if (result) result = readKey(reader, 11U);
    if (result) result = reader.readUnsigned(decoded.issuedAt);
    if (result) result = readKey(reader, 12U);
    if (result) result = reader.readUnsigned(decoded.expiresAt);
    if (result) result = readKey(reader, 13U);
    if (result) result = readU32(reader, decoded.serverKeyId);
    if (result) result = readKey(reader, 14U);
    if (result) result = readSignatureAlgorithm(reader, decoded.signatureAlgorithm);
    if (result) result = readKey(reader, 15U);
    if (result) result = reader.readBytes(decoded.signature);
    if (result) result = finishDecode(reader);
    if (result) result = validateGrant(decoded, true);
    if (result) body = decoded;
    return result;
}

Result encodeOwnershipReleaseGrantTranscript(
    const OwnershipReleaseGrant& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateGrant(body, false);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(16U);
    if (result) result = writer.writeText(StringView(kGrantDomain, sizeof(kGrantDomain) - 1U));
    if (result) result = writer.writeUnsigned(kOwnershipReleaseContractVersion);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writer.writeUnsigned(body.currentGeneration);
    if (result) result = writer.writeUnsigned(body.nextGeneration);
    if (result) result = writer.writeBytes(body.releaseNonce);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.networkPolicy));
    if (result) result = writer.writeBytes(body.requestDigest);
    if (result) result = writer.writeUnsigned(body.issuedAt);
    if (result) result = writer.writeUnsigned(body.expiresAt);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.signatureAlgorithm));
    if (result) encoded = writer.view();
    return result;
}

Result encodeOwnershipReleaseReceipt(
    const OwnershipReleaseReceipt& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateReceipt(body);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(10U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kOwnershipReleaseContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.currentGeneration);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(body.nextGeneration);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.networkPolicy));
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeBytes(body.grantDigest);
    if (result) encoded = writer.view();
    return result;
}

Result decodeOwnershipReleaseReceipt(
    ByteView encoded,
    OwnershipReleaseReceipt& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 10U);
    OwnershipReleaseReceipt decoded;
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
    if (result) result = readU32(reader, decoded.currentGeneration);
    if (result) result = readKey(reader, 6U);
    if (result) result = readU32(reader, decoded.nextGeneration);
    if (result) result = readKey(reader, 7U);
    if (result) result = readOperation(reader, decoded.operation);
    if (result) result = readKey(reader, 8U);
    if (result) result = readPolicy(reader, decoded.networkPolicy);
    if (result) result = readKey(reader, 9U);
    if (result) result = reader.readBytes(decoded.grantDigest);
    if (result) result = finishDecode(reader);
    if (result) result = validateReceipt(decoded);
    if (result) body = decoded;
    return result;
}

Result encodeOwnershipReleaseCommitAck(
    const OwnershipReleaseCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateAck(body, true);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(13U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kOwnershipReleaseContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.currentGeneration);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(body.nextGeneration);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.networkPolicy));
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeBytes(body.receiptDigest);
    if (result) result = writeKey(writer, 10U);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) result = writeKey(writer, 11U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.signatureAlgorithm));
    if (result) result = writeKey(writer, 12U);
    if (result) result = writer.writeBytes(body.signature);
    if (result) encoded = writer.view();
    return result;
}

Result decodeOwnershipReleaseCommitAck(
    ByteView encoded,
    OwnershipReleaseCommitAck& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 13U);
    OwnershipReleaseCommitAck decoded;
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
    if (result) result = readU32(reader, decoded.currentGeneration);
    if (result) result = readKey(reader, 6U);
    if (result) result = readU32(reader, decoded.nextGeneration);
    if (result) result = readKey(reader, 7U);
    if (result) result = readOperation(reader, decoded.operation);
    if (result) result = readKey(reader, 8U);
    if (result) result = readPolicy(reader, decoded.networkPolicy);
    if (result) result = readKey(reader, 9U);
    if (result) result = reader.readBytes(decoded.receiptDigest);
    if (result) result = readKey(reader, 10U);
    if (result) result = readU32(reader, decoded.serverKeyId);
    if (result) result = readKey(reader, 11U);
    if (result) result = readSignatureAlgorithm(reader, decoded.signatureAlgorithm);
    if (result) result = readKey(reader, 12U);
    if (result) result = reader.readBytes(decoded.signature);
    if (result) result = finishDecode(reader);
    if (result) result = validateAck(decoded, true);
    if (result) body = decoded;
    return result;
}

Result encodeOwnershipReleaseCommitAckTranscript(
    const OwnershipReleaseCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateAck(body, false);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(13U);
    if (result) result = writer.writeText(StringView(kAckDomain, sizeof(kAckDomain) - 1U));
    if (result) result = writer.writeUnsigned(kOwnershipReleaseContractVersion);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writer.writeUnsigned(body.currentGeneration);
    if (result) result = writer.writeUnsigned(body.nextGeneration);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.networkPolicy));
    if (result) result = writer.writeBytes(body.receiptDigest);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.signatureAlgorithm));
    if (result) encoded = writer.view();
    return result;
}

} // namespace blinker
