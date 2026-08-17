#include "ControllerControlContract.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const char kGrantDomain[] = "blinker.controller-grant.v1";
const char kReceiptDomain[] = "blinker.controller-receipt.v1";

cbor::Limits contractLimits() {
    cbor::Limits limits;
    limits.maxTextLength = kLogicalDeviceIdCapacity;
    limits.maxByteStringLength = kControllerGrantSignatureSize;
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

bool validOperation(ControllerMutationOperation operation) {
    return operation == ControllerMutationOperation::Install ||
           operation == ControllerMutationOperation::Rotate ||
           operation == ControllerMutationOperation::Revoke;
}

bool validSignatureAlgorithm(ServerSignatureAlgorithm algorithm) {
    return algorithm == ServerSignatureAlgorithm::Ed25519 ||
           algorithm == ServerSignatureAlgorithm::EcdsaP256Sha256Raw;
}

Result validateMutationFields(
    ControllerMutationOperation operation,
    ControllerCredentialSuite suite,
    uint32_t expectedVersion,
    uint32_t version,
    uint32_t permissions,
    ByteView secretDigest) {
    if (!validOperation(operation) ||
        suite != ControllerCredentialSuite::HmacSha256_32 ||
        version == 0U ||
        !exactBytes(secretDigest, kControllerSecretDigestSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (operation == ControllerMutationOperation::Install) {
        if (expectedVersion != 0U ||
            !validAuthorizationPermissions(permissions) ||
            allZero(secretDigest)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    } else if (operation == ControllerMutationOperation::Rotate) {
        if (expectedVersion == 0U || version <= expectedVersion ||
            !validAuthorizationPermissions(permissions) ||
            allZero(secretDigest)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    } else if (expectedVersion == 0U || version != expectedVersion ||
               permissions != 0U || !allZero(secretDigest)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result validateGrant(const ControllerGrant& body, bool requireSignature) {
    Result result = validateMutationFields(
        body.operation,
        body.suite,
        body.expectedCredentialVersion,
        body.credentialVersion,
        body.permissions,
        body.secretDigest);
    if (!result ||
        !nonZeroExact(body.grantId, kControllerGrantIdSize) ||
        !nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) ||
        !validateLogicalDeviceId(body.logicalDeviceId) ||
        body.domain != ControllerCredentialDomain::Ownership ||
        body.ownershipGeneration == 0U ||
        !nonZeroExact(body.controllerId, kControllerIdSize) ||
        !nonZeroExact(body.controlNonce, kControllerControlNonceSize) ||
        body.issuedAt == 0U || body.expiresAt <= body.issuedAt ||
        body.serverKeyId == 0U ||
        !validSignatureAlgorithm(body.signatureAlgorithm) ||
        (requireSignature
             ? !exactBytes(body.signature, kControllerGrantSignatureSize)
             : (!body.signature.empty() &&
                !exactBytes(
                    body.signature,
                    kControllerGrantSignatureSize)))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result validateReceipt(
    const ControllerMutationReceipt& body,
    bool requireProof) {
    if (!validOperation(body.operation) ||
        !nonZeroExact(body.grantId, kControllerGrantIdSize) ||
        !nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) ||
        body.ownershipGeneration == 0U ||
        !nonZeroExact(body.controllerId, kControllerIdSize) ||
        body.credentialVersion == 0U ||
        !exactBytes(body.secretDigest, kControllerSecretDigestSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (body.operation == ControllerMutationOperation::Revoke) {
        return body.permissions == 0U && allZero(body.secretDigest) &&
                       body.proofKind == ControllerReceiptProofKind::None &&
                       body.proof.empty()
                   ? Result::success()
                   : Result::failure(ErrorCode::InvalidArgument);
    }
    if (!validAuthorizationPermissions(body.permissions) ||
        allZero(body.secretDigest) ||
        body.proofKind != ControllerReceiptProofKind::HmacSha256 ||
        (requireProof
             ? !exactBytes(body.proof, kControllerReceiptProofSize)
             : (!body.proof.empty() &&
                !exactBytes(body.proof, kControllerReceiptProofSize)))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result validateCommit(const ControllerCommitRequest& body) {
    if (!validOperation(body.operation) ||
        !nonZeroExact(body.grantId, kControllerGrantIdSize) ||
        !nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) ||
        body.ownershipGeneration == 0U ||
        !nonZeroExact(body.controllerId, kControllerIdSize) ||
        body.credentialVersion == 0U ||
        !nonZeroExact(body.receiptDigest, kSha256Size) ||
        !nonZeroExact(
            body.idempotencyKey,
            kControllerCommitIdempotencyKeySize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const ControllerCommitVerification expected =
        body.operation == ControllerMutationOperation::Revoke
            ? ControllerCommitVerification::AuthorizedRevoke
            : ControllerCommitVerification::Method2Confirmed;
    return body.verification == expected
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
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
    return result && value != kControllerControlContractVersion
               ? Result::failure(ErrorCode::UnsupportedVersion)
               : result;
}

} // namespace

ControllerGrant::ControllerGrant()
    : operation(ControllerMutationOperation::Install),
      domain(ControllerCredentialDomain::Ownership),
      ownershipGeneration(0U),
      suite(ControllerCredentialSuite::HmacSha256_32),
      expectedCredentialVersion(0U),
      credentialVersion(0U),
      permissions(0U),
      issuedAt(0U),
      expiresAt(0U),
      serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}

ControllerMutationReceipt::ControllerMutationReceipt()
    : operation(ControllerMutationOperation::Install),
      ownershipGeneration(0U),
      credentialVersion(0U),
      permissions(0U),
      proofKind(ControllerReceiptProofKind::None) {}

ControllerCommitRequest::ControllerCommitRequest()
    : operation(ControllerMutationOperation::Install),
      ownershipGeneration(0U),
      credentialVersion(0U),
      verification(ControllerCommitVerification::Method2Confirmed) {}

Result encodeControllerGrant(
    const ControllerGrant& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateGrant(body, true);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(19U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kControllerControlContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.domain));
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeBytes(body.controllerId);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.suite));
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeUnsigned(body.expectedCredentialVersion);
    if (result) result = writeKey(writer, 10U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writeKey(writer, 11U);
    if (result) result = writer.writeUnsigned(body.permissions);
    if (result) result = writeKey(writer, 12U);
    if (result) result = writer.writeBytes(body.secretDigest);
    if (result) result = writeKey(writer, 13U);
    if (result) result = writer.writeBytes(body.controlNonce);
    if (result) result = writeKey(writer, 14U);
    if (result) result = writer.writeUnsigned(body.issuedAt);
    if (result) result = writeKey(writer, 15U);
    if (result) result = writer.writeUnsigned(body.expiresAt);
    if (result) result = writeKey(writer, 16U);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) result = writeKey(writer, 17U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.signatureAlgorithm));
    if (result) result = writeKey(writer, 18U);
    if (result) result = writer.writeBytes(body.signature);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeControllerGrant(ByteView encoded, ControllerGrant& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 19U);
    ControllerGrant decoded;
    uint64_t value = 0U;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.operation = static_cast<ControllerMutationOperation>(value);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.grantId);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readText(decoded.logicalDeviceId);
    if (result) result = readKey(reader, 5U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.domain = static_cast<ControllerCredentialDomain>(value);
    if (result) result = readKey(reader, 6U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readBytes(decoded.controllerId);
    if (result) result = readKey(reader, 8U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.suite = static_cast<ControllerCredentialSuite>(value);
    if (result) result = readKey(reader, 9U);
    if (result) result = readU32(reader, decoded.expectedCredentialVersion);
    if (result) result = readKey(reader, 10U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    if (result) result = readKey(reader, 11U);
    if (result) result = readU32(reader, decoded.permissions);
    if (result) result = readKey(reader, 12U);
    if (result) result = reader.readBytes(decoded.secretDigest);
    if (result) result = readKey(reader, 13U);
    if (result) result = reader.readBytes(decoded.controlNonce);
    if (result) result = readKey(reader, 14U);
    if (result) result = reader.readUnsigned(decoded.issuedAt);
    if (result) result = readKey(reader, 15U);
    if (result) result = reader.readUnsigned(decoded.expiresAt);
    if (result) result = readKey(reader, 16U);
    if (result) result = readU32(reader, decoded.serverKeyId);
    if (result) result = readKey(reader, 17U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.signatureAlgorithm = static_cast<ServerSignatureAlgorithm>(value);
    if (result) result = readKey(reader, 18U);
    if (result) result = reader.readBytes(decoded.signature);
    if (result) result = finishDecode(reader);
    if (result) result = validateGrant(decoded, true);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeControllerGrantTranscript(
    const ControllerGrant& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateGrant(body, false);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(19U);
    if (result) result = writer.writeText(StringView(kGrantDomain, sizeof(kGrantDomain) - 1U));
    if (result) result = writer.writeUnsigned(kControllerControlContractVersion);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.domain));
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writer.writeBytes(body.controllerId);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.suite));
    if (result) result = writer.writeUnsigned(body.expectedCredentialVersion);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writer.writeUnsigned(body.permissions);
    if (result) result = writer.writeBytes(body.secretDigest);
    if (result) result = writer.writeBytes(body.controlNonce);
    if (result) result = writer.writeUnsigned(body.issuedAt);
    if (result) result = writer.writeUnsigned(body.expiresAt);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.signatureAlgorithm));
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result encodeControllerMutationReceipt(
    const ControllerMutationReceipt& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateReceipt(body, true);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(11U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kControllerControlContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeBytes(body.controllerId);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeUnsigned(body.permissions);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeBytes(body.secretDigest);
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.proofKind));
    if (result) result = writeKey(writer, 10U);
    if (result) result = writer.writeBytes(body.proof);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeControllerMutationReceipt(
    ByteView encoded,
    ControllerMutationReceipt& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 11U);
    ControllerMutationReceipt decoded;
    uint64_t value = 0U;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.operation = static_cast<ControllerMutationOperation>(value);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.grantId);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 4U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = readKey(reader, 5U);
    if (result) result = reader.readBytes(decoded.controllerId);
    if (result) result = readKey(reader, 6U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    if (result) result = readKey(reader, 7U);
    if (result) result = readU32(reader, decoded.permissions);
    if (result) result = readKey(reader, 8U);
    if (result) result = reader.readBytes(decoded.secretDigest);
    if (result) result = readKey(reader, 9U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.proofKind = static_cast<ControllerReceiptProofKind>(value);
    if (result) result = readKey(reader, 10U);
    if (result) result = reader.readBytes(decoded.proof);
    if (result) result = finishDecode(reader);
    if (result) result = validateReceipt(decoded, true);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeControllerMutationReceiptTranscript(
    const ControllerMutationReceipt& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateReceipt(body, false);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(11U);
    if (result) result = writer.writeText(StringView(kReceiptDomain, sizeof(kReceiptDomain) - 1U));
    if (result) result = writer.writeUnsigned(kControllerControlContractVersion);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writer.writeBytes(body.controllerId);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writer.writeUnsigned(body.permissions);
    if (result) result = writer.writeBytes(body.secretDigest);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.proofKind));
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result computeControllerMutationReceiptProof(
    ByteView controllerSecret,
    const ControllerMutationReceipt& body,
    MutableByteSpan transcriptWorkspace,
    MutableByteSpan proofOutput) {
    if (!nonZeroExact(controllerSecret, kControllerCredentialSecretSize) ||
        transcriptWorkspace.data == nullptr ||
        transcriptWorkspace.size < kControllerControlWorkspaceSize ||
        proofOutput.data == nullptr ||
        proofOutput.size < kControllerReceiptProofSize ||
        body.operation == ControllerMutationOperation::Revoke) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    secureZero(transcriptWorkspace);
    ByteView transcript;
    Result result = encodeControllerMutationReceiptTranscript(
        body,
        transcriptWorkspace,
        transcript);
    if (result) {
        result = hmacSha256(
            controllerSecret,
            transcript,
            MutableByteSpan(proofOutput.data, kControllerReceiptProofSize));
    }
    secureZero(transcriptWorkspace);
    return result;
}

Result verifyControllerMutationReceiptProof(
    ByteView controllerSecret,
    const ControllerMutationReceipt& body,
    MutableByteSpan transcriptWorkspace) {
    if (body.proofKind != ControllerReceiptProofKind::HmacSha256 ||
        !exactBytes(body.proof, kControllerReceiptProofSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t expected[kControllerReceiptProofSize] = {};
    Result result = computeControllerMutationReceiptProof(
        controllerSecret,
        body,
        transcriptWorkspace,
        MutableByteSpan(expected, sizeof(expected)));
    if (result && !constantTimeEqual(
            ByteView(expected, sizeof(expected)), body.proof)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    return result;
}

Result encodeControllerCommitRequest(
    const ControllerCommitRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateCommit(body);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(10U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kControllerControlContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.grantId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.operation));
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeBytes(body.controllerId);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeBytes(body.receiptDigest);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.verification));
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeBytes(body.idempotencyKey);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeControllerCommitRequest(
    ByteView encoded,
    ControllerCommitRequest& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 10U);
    ControllerCommitRequest decoded;
    uint64_t value = 0U;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.grantId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.operation = static_cast<ControllerMutationOperation>(value);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 4U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = readKey(reader, 5U);
    if (result) result = reader.readBytes(decoded.controllerId);
    if (result) result = readKey(reader, 6U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readBytes(decoded.receiptDigest);
    if (result) result = readKey(reader, 8U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.verification = static_cast<ControllerCommitVerification>(value);
    if (result) result = readKey(reader, 9U);
    if (result) result = reader.readBytes(decoded.idempotencyKey);
    if (result) result = finishDecode(reader);
    if (result) result = validateCommit(decoded);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

} // namespace blinker
