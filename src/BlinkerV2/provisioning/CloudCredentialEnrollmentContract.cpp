#include "CloudCredentialEnrollmentContract.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

static const char kEnvelopeDomain[] =
    "blinker.cloud-credential-envelope.v1";
static const char kGrantDomain[] =
    "blinker.cloud-credential-enrollment-grant.v1";
static const char kReceiptDomain[] =
    "blinker.cloud-credential-enrollment-receipt.v1";
static const char kCommitAckDomain[] =
    "blinker.cloud-credential-enrollment-commit-ack.v1";

cbor::Limits limits() {
    cbor::Limits value;
    value.maxTextLength = kCloudLogicalDeviceIdCapacity;
    value.maxByteStringLength = kCloudEnrollmentGrantMaxEncodedSize;
    value.maxContainerItems = 16U;
    value.maxDepth = 2U;
    return value;
}

bool exact(ByteView value, size_t expected) {
    return value.data != nullptr && value.size == expected;
}

bool exactNonZero(ByteView value, size_t expected) {
    if (!exact(value, expected)) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool validSignatureAlgorithm(ServerSignatureAlgorithm algorithm) {
    return algorithm == ServerSignatureAlgorithm::Ed25519 ||
           algorithm == ServerSignatureAlgorithm::EcdsaP256Sha256Raw;
}

bool validRequest(
    const CloudCredentialEnrollmentRequest& body,
    bool requireCiphertext) {
    return exactNonZero(body.requestId, kCloudEnrollmentRequestIdSize) &&
           exactNonZero(body.deviceInstanceId, kDeviceInstanceIdSize) &&
           validateLogicalDeviceId(body.logicalDeviceId).ok() &&
           body.ownershipGeneration != 0U &&
           body.credentialVersion != 0U &&
           body.credentialSuite == CloudCredentialSuite::HmacSha256_32 &&
           body.envelopeAlgorithm ==
               CloudCredentialEnvelopeAlgorithm::X25519Aes256GcmSha256 &&
           body.encryptionKeyId != 0U &&
           exactNonZero(
               body.ephemeralPublicKey,
               kCloudEnrollmentEphemeralPublicKeySize) &&
           (!requireCiphertext ||
            (exactNonZero(body.nonce, kCloudEnrollmentNonceSize) &&
             exactNonZero(
                 body.encryptedCredential,
                 kCloudEnrollmentEncryptedCredentialSize)));
}

bool validGrant(const CloudCredentialEnrollmentGrant& body) {
    return exactNonZero(body.requestId, kCloudEnrollmentRequestIdSize) &&
           exactNonZero(body.deviceInstanceId, kDeviceInstanceIdSize) &&
           validateLogicalDeviceId(body.logicalDeviceId).ok() &&
           body.ownershipGeneration != 0U &&
           body.credentialVersion != 0U &&
           exactNonZero(body.requestDigest, kSha256Size) &&
           body.issuedAt != 0U && body.expiresAt > body.issuedAt &&
           body.serverKeyId != 0U &&
           validSignatureAlgorithm(body.signatureAlgorithm) &&
           exact(body.signature, kCloudEnrollmentGrantSignatureSize);
}

struct ProofBody {
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    ByteView digest;
    CloudEnrollmentProofAlgorithm algorithm;
    ByteView proof;
};

bool validProofBody(const ProofBody& body, bool requireProof) {
    return exactNonZero(body.requestId, kCloudEnrollmentRequestIdSize) &&
           exactNonZero(body.deviceInstanceId, kDeviceInstanceIdSize) &&
           validateLogicalDeviceId(body.logicalDeviceId).ok() &&
           body.ownershipGeneration != 0U &&
           body.credentialVersion != 0U &&
           exactNonZero(body.digest, kSha256Size) &&
           body.algorithm == CloudEnrollmentProofAlgorithm::HmacSha256 &&
           (!requireProof || exact(body.proof, kCloudEnrollmentProofSize));
}

ProofBody proofBody(const CloudCredentialEnrollmentReceipt& body) {
    ProofBody value = {
        body.requestId,
        body.deviceInstanceId,
        body.logicalDeviceId,
        body.ownershipGeneration,
        body.credentialVersion,
        body.grantDigest,
        body.proofAlgorithm,
        body.proof};
    return value;
}

ProofBody proofBody(const CloudCredentialEnrollmentCommitAck& body) {
    ProofBody value = {
        body.requestId,
        body.deviceInstanceId,
        body.logicalDeviceId,
        body.ownershipGeneration,
        body.credentialVersion,
        body.receiptDigest,
        body.proofAlgorithm,
        body.proof};
    return value;
}

Result key(cbor::Writer& writer, uint8_t value) {
    return writer.writeUnsigned(value);
}

Result key(cbor::Reader& reader, uint8_t expected) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    return result && value == expected
               ? Result::success()
               : (result ? Result::failure(ErrorCode::InvalidEncoding)
                         : result);
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

Result beginDecode(
    ByteView encoded,
    cbor::Reader& reader,
    size_t expectedPairs) {
    Result result = cbor::validate(encoded, limits());
    size_t pairs = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs != expectedPairs) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    return result;
}

Result readVersion(cbor::Reader& reader) {
    Result result = key(reader, 0U);
    uint64_t version = 0U;
    if (result) result = reader.readUnsigned(version);
    if (result && version != kCloudCredentialEnrollmentContractVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    return result;
}

Result finish(cbor::Reader& reader) {
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

Result writeProofBody(
    const ProofBody& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validProofBody(body, true)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(9U);
    if (result) result = key(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(
            kCloudCredentialEnrollmentContractVersion);
    }
    if (result) result = key(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = key(writer, 2U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = key(writer, 3U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = key(writer, 4U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = key(writer, 5U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = key(writer, 6U);
    if (result) result = writer.writeBytes(body.digest);
    if (result) result = key(writer, 7U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.algorithm));
    }
    if (result) result = key(writer, 8U);
    if (result) result = writer.writeBytes(body.proof);
    if (result) encoded = writer.view();
    return result;
}

Result readProofBody(ByteView encoded, ProofBody& body) {
    cbor::Reader reader(encoded, limits());
    Result result = beginDecode(encoded, reader, 9U);
    if (result) result = readVersion(reader);
    if (result) result = key(reader, 1U);
    if (result) result = reader.readBytes(body.requestId);
    if (result) result = key(reader, 2U);
    if (result) result = reader.readBytes(body.deviceInstanceId);
    if (result) result = key(reader, 3U);
    if (result) result = reader.readText(body.logicalDeviceId);
    if (result) result = key(reader, 4U);
    if (result) result = readU32(reader, body.ownershipGeneration);
    if (result) result = key(reader, 5U);
    if (result) result = readU32(reader, body.credentialVersion);
    if (result) result = key(reader, 6U);
    if (result) result = reader.readBytes(body.digest);
    uint64_t algorithm = 0U;
    if (result) result = key(reader, 7U);
    if (result) result = reader.readUnsigned(algorithm);
    if (result && algorithm > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        body.algorithm = static_cast<CloudEnrollmentProofAlgorithm>(algorithm);
    }
    if (result) result = key(reader, 8U);
    if (result) result = reader.readBytes(body.proof);
    if (result) result = finish(reader);
    if (result && !validProofBody(body, true)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    return result;
}

Result writeProofTranscript(
    const char* domain,
    size_t domainSize,
    const ProofBody& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validProofBody(body, false)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(9U);
    if (result) {
        result = writer.writeText(StringView(domain, domainSize));
    }
    if (result) {
        result = writer.writeUnsigned(
            kCloudCredentialEnrollmentContractVersion);
    }
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writer.writeBytes(body.digest);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.algorithm));
    }
    if (result) encoded = writer.view();
    return result;
}

Result computeProof(
    ByteView cloudSecret,
    const char* domain,
    size_t domainSize,
    const ProofBody& body,
    MutableByteSpan transcriptWorkspace,
    MutableByteSpan proofOutput) {
    if (!exact(cloudSecret, kCloudCredentialSecretSize) ||
        transcriptWorkspace.data == nullptr ||
        transcriptWorkspace.size < kCloudEnrollmentWorkspaceSize ||
        proofOutput.data == nullptr ||
        proofOutput.size < kCloudEnrollmentProofSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    ByteView transcript;
    Result result = writeProofTranscript(
        domain,
        domainSize,
        body,
        transcriptWorkspace,
        transcript);
    if (result) {
        result = hmacSha256(
            cloudSecret,
            transcript,
            MutableByteSpan(proofOutput.data, kCloudEnrollmentProofSize));
    }
    return result;
}

Result verifyProof(
    ByteView cloudSecret,
    const char* domain,
    size_t domainSize,
    const ProofBody& body,
    MutableByteSpan transcriptWorkspace) {
    if (!exact(body.proof, kCloudEnrollmentProofSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t expected[kCloudEnrollmentProofSize] = {};
    Result result = computeProof(
        cloudSecret,
        domain,
        domainSize,
        body,
        transcriptWorkspace,
        MutableByteSpan(expected, sizeof(expected)));
    if (result && !constantTimeEqual(
                      ByteView(expected, sizeof(expected)),
                      body.proof)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    return result;
}

bool validRetryClass(CloudEnrollmentRetryClass value) {
    return static_cast<uint8_t>(value) <=
           static_cast<uint8_t>(
               CloudEnrollmentRetryClass::ReclaimOwnership);
}

bool validWireError(CloudEnrollmentWireError value) {
    switch (value) {
        case CloudEnrollmentWireError::Malformed:
        case CloudEnrollmentWireError::UnsupportedVersion:
        case CloudEnrollmentWireError::UnsupportedAlgorithm:
        case CloudEnrollmentWireError::AuthenticationRequired:
        case CloudEnrollmentWireError::OwnershipNotActive:
        case CloudEnrollmentWireError::OwnershipGenerationConflict:
        case CloudEnrollmentWireError::EncryptionKeyNotFound:
        case CloudEnrollmentWireError::EnvelopeInvalid:
        case CloudEnrollmentWireError::RequestConflict:
        case CloudEnrollmentWireError::GrantInvalid:
        case CloudEnrollmentWireError::GrantExpired:
        case CloudEnrollmentWireError::CredentialConflict:
        case CloudEnrollmentWireError::ReceiptProofInvalid:
        case CloudEnrollmentWireError::RequestExpired:
        case CloudEnrollmentWireError::DeviceRevoked:
        case CloudEnrollmentWireError::RateLimited:
        case CloudEnrollmentWireError::TemporarilyUnavailable:
            return true;
    }
    return false;
}

bool validError(const CloudEnrollmentErrorBody& body) {
    if (!validWireError(body.error) || !validRetryClass(body.retryClass) ||
        body.hasRetryAfter !=
            (body.retryClass == CloudEnrollmentRetryClass::Backoff) ||
        (body.hasRetryAfter && body.retryAfterSeconds == 0U)) {
        return false;
    }
    switch (body.error) {
        case CloudEnrollmentWireError::RateLimited:
            return body.retryClass == CloudEnrollmentRetryClass::Backoff;
        case CloudEnrollmentWireError::TemporarilyUnavailable:
            return body.retryClass == CloudEnrollmentRetryClass::SameRequest ||
                   body.retryClass == CloudEnrollmentRetryClass::Backoff;
        case CloudEnrollmentWireError::EncryptionKeyNotFound:
        case CloudEnrollmentWireError::GrantExpired:
        case CloudEnrollmentWireError::RequestExpired:
            return body.retryClass ==
                   CloudEnrollmentRetryClass::RestartEnrollment;
        case CloudEnrollmentWireError::OwnershipNotActive:
        case CloudEnrollmentWireError::OwnershipGenerationConflict:
        case CloudEnrollmentWireError::DeviceRevoked:
            return body.retryClass ==
                   CloudEnrollmentRetryClass::ReclaimOwnership;
        case CloudEnrollmentWireError::AuthenticationRequired:
        case CloudEnrollmentWireError::EnvelopeInvalid:
        case CloudEnrollmentWireError::GrantInvalid:
        case CloudEnrollmentWireError::ReceiptProofInvalid:
            return body.retryClass == CloudEnrollmentRetryClass::Never;
        case CloudEnrollmentWireError::RequestConflict:
        case CloudEnrollmentWireError::CredentialConflict:
            return body.retryClass == CloudEnrollmentRetryClass::SameRequest;
        case CloudEnrollmentWireError::Malformed:
        case CloudEnrollmentWireError::UnsupportedVersion:
        case CloudEnrollmentWireError::UnsupportedAlgorithm:
            return body.retryClass == CloudEnrollmentRetryClass::Never;
    }
    return false;
}

} // namespace

CloudCredentialEnrollmentRequest::CloudCredentialEnrollmentRequest()
    : ownershipGeneration(0U),
      credentialVersion(0U),
      credentialSuite(CloudCredentialSuite::HmacSha256_32),
      envelopeAlgorithm(
          CloudCredentialEnvelopeAlgorithm::X25519Aes256GcmSha256),
      encryptionKeyId(0U) {}

CloudCredentialEnrollmentGrant::CloudCredentialEnrollmentGrant()
    : ownershipGeneration(0U),
      credentialVersion(0U),
      issuedAt(0U),
      expiresAt(0U),
      serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}

CloudCredentialEnrollmentReceipt::CloudCredentialEnrollmentReceipt()
    : ownershipGeneration(0U),
      credentialVersion(0U),
      proofAlgorithm(CloudEnrollmentProofAlgorithm::HmacSha256) {}

CloudCredentialEnrollmentCommitAck::CloudCredentialEnrollmentCommitAck()
    : ownershipGeneration(0U),
      credentialVersion(0U),
      proofAlgorithm(CloudEnrollmentProofAlgorithm::HmacSha256) {}

CloudEnrollmentErrorBody::CloudEnrollmentErrorBody()
    : error(CloudEnrollmentWireError::Malformed),
      retryClass(CloudEnrollmentRetryClass::Never),
      retryAfterSeconds(0U),
      hasRetryAfter(false) {}

Result encodeCloudCredentialEnrollmentRequest(
    const CloudCredentialEnrollmentRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validRequest(body, true)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(12U);
    if (result) result = key(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(
            kCloudCredentialEnrollmentContractVersion);
    }
    if (result) result = key(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = key(writer, 2U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = key(writer, 3U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = key(writer, 4U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = key(writer, 5U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = key(writer, 6U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.credentialSuite));
    }
    if (result) result = key(writer, 7U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.envelopeAlgorithm));
    }
    if (result) result = key(writer, 8U);
    if (result) result = writer.writeUnsigned(body.encryptionKeyId);
    if (result) result = key(writer, 9U);
    if (result) result = writer.writeBytes(body.ephemeralPublicKey);
    if (result) result = key(writer, 10U);
    if (result) result = writer.writeBytes(body.nonce);
    if (result) result = key(writer, 11U);
    if (result) result = writer.writeBytes(body.encryptedCredential);
    if (result) encoded = writer.view();
    return result;
}

Result decodeCloudCredentialEnrollmentRequest(
    ByteView encoded,
    CloudCredentialEnrollmentRequest& body) {
    cbor::Reader reader(encoded, limits());
    Result result = beginDecode(encoded, reader, 12U);
    CloudCredentialEnrollmentRequest decoded;
    if (result) result = readVersion(reader);
    if (result) result = key(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = key(reader, 2U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = key(reader, 3U);
    if (result) result = reader.readText(decoded.logicalDeviceId);
    if (result) result = key(reader, 4U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = key(reader, 5U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    uint64_t value = 0U;
    if (result) result = key(reader, 6U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.credentialSuite = static_cast<CloudCredentialSuite>(value);
    if (result) result = key(reader, 7U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        decoded.envelopeAlgorithm =
            static_cast<CloudCredentialEnvelopeAlgorithm>(value);
    }
    if (result) result = key(reader, 8U);
    if (result) result = readU32(reader, decoded.encryptionKeyId);
    if (result) result = key(reader, 9U);
    if (result) result = reader.readBytes(decoded.ephemeralPublicKey);
    if (result) result = key(reader, 10U);
    if (result) result = reader.readBytes(decoded.nonce);
    if (result) result = key(reader, 11U);
    if (result) result = reader.readBytes(decoded.encryptedCredential);
    if (result) result = finish(reader);
    if (result && !validRequest(decoded, true)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) body = decoded;
    return result;
}

Result encodeCloudCredentialEnvelopeTranscript(
    const CloudCredentialEnrollmentRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validRequest(body, false)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(11U);
    if (result) {
        result = writer.writeText(StringView(
            kEnvelopeDomain,
            sizeof(kEnvelopeDomain) - 1U));
    }
    if (result) {
        result = writer.writeUnsigned(
            kCloudCredentialEnrollmentContractVersion);
    }
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.credentialSuite));
    }
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.envelopeAlgorithm));
    }
    if (result) result = writer.writeUnsigned(body.encryptionKeyId);
    if (result) result = writer.writeBytes(body.ephemeralPublicKey);
    if (result) encoded = writer.view();
    return result;
}

Result encodeCloudCredentialEnrollmentGrant(
    const CloudCredentialEnrollmentGrant& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validGrant(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(12U);
    if (result) result = key(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(
            kCloudCredentialEnrollmentContractVersion);
    }
    if (result) result = key(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = key(writer, 2U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = key(writer, 3U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = key(writer, 4U);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = key(writer, 5U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = key(writer, 6U);
    if (result) result = writer.writeBytes(body.requestDigest);
    if (result) result = key(writer, 7U);
    if (result) result = writer.writeUnsigned(body.issuedAt);
    if (result) result = key(writer, 8U);
    if (result) result = writer.writeUnsigned(body.expiresAt);
    if (result) result = key(writer, 9U);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) result = key(writer, 10U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.signatureAlgorithm));
    }
    if (result) result = key(writer, 11U);
    if (result) result = writer.writeBytes(body.signature);
    if (result) encoded = writer.view();
    return result;
}

Result decodeCloudCredentialEnrollmentGrant(
    ByteView encoded,
    CloudCredentialEnrollmentGrant& body) {
    cbor::Reader reader(encoded, limits());
    Result result = beginDecode(encoded, reader, 12U);
    CloudCredentialEnrollmentGrant decoded;
    if (result) result = readVersion(reader);
    if (result) result = key(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = key(reader, 2U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = key(reader, 3U);
    if (result) result = reader.readText(decoded.logicalDeviceId);
    if (result) result = key(reader, 4U);
    if (result) result = readU32(reader, decoded.ownershipGeneration);
    if (result) result = key(reader, 5U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    if (result) result = key(reader, 6U);
    if (result) result = reader.readBytes(decoded.requestDigest);
    if (result) result = key(reader, 7U);
    if (result) result = reader.readUnsigned(decoded.issuedAt);
    if (result) result = key(reader, 8U);
    if (result) result = reader.readUnsigned(decoded.expiresAt);
    if (result) result = key(reader, 9U);
    if (result) result = readU32(reader, decoded.serverKeyId);
    uint64_t value = 0U;
    if (result) result = key(reader, 10U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        decoded.signatureAlgorithm = static_cast<ServerSignatureAlgorithm>(value);
    }
    if (result) result = key(reader, 11U);
    if (result) result = reader.readBytes(decoded.signature);
    if (result) result = finish(reader);
    if (result && !validGrant(decoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) body = decoded;
    return result;
}

Result encodeCloudCredentialEnrollmentGrantTranscript(
    const CloudCredentialEnrollmentGrant& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validGrant(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(12U);
    if (result) {
        result = writer.writeText(StringView(
            kGrantDomain,
            sizeof(kGrantDomain) - 1U));
    }
    if (result) {
        result = writer.writeUnsigned(
            kCloudCredentialEnrollmentContractVersion);
    }
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writer.writeUnsigned(body.ownershipGeneration);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writer.writeBytes(body.requestDigest);
    if (result) result = writer.writeUnsigned(body.issuedAt);
    if (result) result = writer.writeUnsigned(body.expiresAt);
    if (result) result = writer.writeUnsigned(body.serverKeyId);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.signatureAlgorithm));
    }
    if (result) encoded = writer.view();
    return result;
}

Result encodeCloudCredentialEnrollmentReceipt(
    const CloudCredentialEnrollmentReceipt& body,
    MutableByteSpan output,
    ByteView& encoded) {
    return writeProofBody(proofBody(body), output, encoded);
}

Result decodeCloudCredentialEnrollmentReceipt(
    ByteView encoded,
    CloudCredentialEnrollmentReceipt& body) {
    ProofBody decoded = {};
    Result result = readProofBody(encoded, decoded);
    if (result) {
        body.requestId = decoded.requestId;
        body.deviceInstanceId = decoded.deviceInstanceId;
        body.logicalDeviceId = decoded.logicalDeviceId;
        body.ownershipGeneration = decoded.ownershipGeneration;
        body.credentialVersion = decoded.credentialVersion;
        body.grantDigest = decoded.digest;
        body.proofAlgorithm = decoded.algorithm;
        body.proof = decoded.proof;
    }
    return result;
}

Result encodeCloudCredentialEnrollmentReceiptTranscript(
    const CloudCredentialEnrollmentReceipt& body,
    MutableByteSpan output,
    ByteView& encoded) {
    return writeProofTranscript(
        kReceiptDomain,
        sizeof(kReceiptDomain) - 1U,
        proofBody(body),
        output,
        encoded);
}

Result computeCloudCredentialEnrollmentReceiptProof(
    ByteView cloudSecret,
    const CloudCredentialEnrollmentReceipt& body,
    MutableByteSpan transcriptWorkspace,
    MutableByteSpan proofOutput) {
    return computeProof(
        cloudSecret,
        kReceiptDomain,
        sizeof(kReceiptDomain) - 1U,
        proofBody(body),
        transcriptWorkspace,
        proofOutput);
}

Result verifyCloudCredentialEnrollmentReceiptProof(
    ByteView cloudSecret,
    const CloudCredentialEnrollmentReceipt& body,
    MutableByteSpan transcriptWorkspace) {
    return verifyProof(
        cloudSecret,
        kReceiptDomain,
        sizeof(kReceiptDomain) - 1U,
        proofBody(body),
        transcriptWorkspace);
}

Result encodeCloudCredentialEnrollmentCommitAck(
    const CloudCredentialEnrollmentCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded) {
    return writeProofBody(proofBody(body), output, encoded);
}

Result decodeCloudCredentialEnrollmentCommitAck(
    ByteView encoded,
    CloudCredentialEnrollmentCommitAck& body) {
    ProofBody decoded = {};
    Result result = readProofBody(encoded, decoded);
    if (result) {
        body.requestId = decoded.requestId;
        body.deviceInstanceId = decoded.deviceInstanceId;
        body.logicalDeviceId = decoded.logicalDeviceId;
        body.ownershipGeneration = decoded.ownershipGeneration;
        body.credentialVersion = decoded.credentialVersion;
        body.receiptDigest = decoded.digest;
        body.proofAlgorithm = decoded.algorithm;
        body.proof = decoded.proof;
    }
    return result;
}

Result encodeCloudCredentialEnrollmentCommitAckTranscript(
    const CloudCredentialEnrollmentCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded) {
    return writeProofTranscript(
        kCommitAckDomain,
        sizeof(kCommitAckDomain) - 1U,
        proofBody(body),
        output,
        encoded);
}

Result computeCloudCredentialEnrollmentCommitAckProof(
    ByteView cloudSecret,
    const CloudCredentialEnrollmentCommitAck& body,
    MutableByteSpan transcriptWorkspace,
    MutableByteSpan proofOutput) {
    return computeProof(
        cloudSecret,
        kCommitAckDomain,
        sizeof(kCommitAckDomain) - 1U,
        proofBody(body),
        transcriptWorkspace,
        proofOutput);
}

Result verifyCloudCredentialEnrollmentCommitAckProof(
    ByteView cloudSecret,
    const CloudCredentialEnrollmentCommitAck& body,
    MutableByteSpan transcriptWorkspace) {
    return verifyProof(
        cloudSecret,
        kCommitAckDomain,
        sizeof(kCommitAckDomain) - 1U,
        proofBody(body),
        transcriptWorkspace);
}

Result encodeCloudEnrollmentErrorBody(
    const CloudEnrollmentErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validError(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(body.hasRetryAfter ? 4U : 3U);
    if (result) result = key(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(
            kCloudCredentialEnrollmentContractVersion);
    }
    if (result) result = key(writer, 1U);
    if (result) result = writer.writeUnsigned(static_cast<uint16_t>(body.error));
    if (result) result = key(writer, 2U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.retryClass));
    }
    if (result && body.hasRetryAfter) result = key(writer, 3U);
    if (result && body.hasRetryAfter) {
        result = writer.writeUnsigned(body.retryAfterSeconds);
    }
    if (result) encoded = writer.view();
    return result;
}

Result decodeCloudEnrollmentErrorBody(
    ByteView encoded,
    CloudEnrollmentErrorBody& body) {
    cbor::Reader reader(encoded, limits());
    Result result = cbor::validate(encoded, limits());
    size_t pairs = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs != 3U && pairs != 4U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    CloudEnrollmentErrorBody decoded;
    if (result) result = readVersion(reader);
    uint64_t value = 0U;
    if (result) result = key(reader, 1U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT16_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.error = static_cast<CloudEnrollmentWireError>(value);
    if (result) result = key(reader, 2U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.retryClass = static_cast<CloudEnrollmentRetryClass>(value);
    decoded.hasRetryAfter = pairs == 4U;
    if (result && decoded.hasRetryAfter) result = key(reader, 3U);
    if (result && decoded.hasRetryAfter) {
        result = readU32(reader, decoded.retryAfterSeconds);
    }
    if (result) result = finish(reader);
    if (result && !validError(decoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) body = decoded;
    return result;
}

uint16_t cloudEnrollmentHttpStatus(CloudEnrollmentWireError error) {
    switch (error) {
        case CloudEnrollmentWireError::Malformed:
        case CloudEnrollmentWireError::UnsupportedVersion:
        case CloudEnrollmentWireError::UnsupportedAlgorithm:
            return 400U;
        case CloudEnrollmentWireError::AuthenticationRequired:
        case CloudEnrollmentWireError::EnvelopeInvalid:
        case CloudEnrollmentWireError::ReceiptProofInvalid:
            return 401U;
        case CloudEnrollmentWireError::OwnershipNotActive:
        case CloudEnrollmentWireError::DeviceRevoked:
            return 403U;
        case CloudEnrollmentWireError::EncryptionKeyNotFound:
            return 404U;
        case CloudEnrollmentWireError::OwnershipGenerationConflict:
        case CloudEnrollmentWireError::RequestConflict:
        case CloudEnrollmentWireError::GrantInvalid:
        case CloudEnrollmentWireError::GrantExpired:
        case CloudEnrollmentWireError::CredentialConflict:
        case CloudEnrollmentWireError::RequestExpired:
            return 409U;
        case CloudEnrollmentWireError::RateLimited:
            return 429U;
        case CloudEnrollmentWireError::TemporarilyUnavailable:
            return 503U;
    }
    return 0U;
}

bool cloudEnrollmentHttpStatusMatches(
    uint16_t statusCode,
    CloudEnrollmentWireError error) {
    return statusCode == cloudEnrollmentHttpStatus(error);
}

} // namespace blinker
