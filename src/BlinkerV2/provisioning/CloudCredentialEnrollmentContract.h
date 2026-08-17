#ifndef BLINKER_PROVISIONING_CLOUDCREDENTIALENROLLMENTCONTRACT_H
#define BLINKER_PROVISIONING_CLOUDCREDENTIALENROLLMENTCONTRACT_H

#include "../core/HmacSha256.h"
#include "../identity/DeviceInstanceId.h"
#include "../identity/CloudCredential.h"
#include "../interface/IServerSignatureVerifier.h"
#include "../protocol/cbor/Cbor.h"
#include "CloudEnrollmentRecord.h"

namespace blinker {

enum : size_t {
    kCloudEnrollmentEphemeralPublicKeySize = 32U,
    kCloudEnrollmentNonceSize = 12U,
    kCloudEnrollmentEncryptedCredentialSize =
        kCloudCredentialSecretSize + 16U,
    kCloudEnrollmentGrantSignatureSize = 64U,
    kCloudEnrollmentProofSize = 32U,
    kCloudEnrollmentRequestMaxEncodedSize = 228U,
    kCloudEnrollmentGrantMaxEncodedSize = 248U,
    kCloudEnrollmentReceiptMaxEncodedSize = 190U,
    kCloudEnrollmentCommitAckMaxEncodedSize = 190U,
    kCloudEnrollmentWorkspaceSize = 256U
};

static const uint8_t kCloudCredentialEnrollmentContractVersion = 1U;

enum class CloudCredentialEnvelopeAlgorithm : uint8_t {
    X25519Aes256GcmSha256 = 1U
};

enum class CloudEnrollmentProofAlgorithm : uint8_t {
    HmacSha256 = 1U
};

enum class CloudEnrollmentRetryClass : uint8_t {
    Never = 0U,
    SameRequest = 1U,
    Backoff = 2U,
    RestartEnrollment = 3U,
    ReclaimOwnership = 4U
};

enum class CloudEnrollmentWireError : uint16_t {
    Malformed = 3000U,
    UnsupportedVersion = 3001U,
    UnsupportedAlgorithm = 3002U,
    AuthenticationRequired = 3003U,
    OwnershipNotActive = 3100U,
    OwnershipGenerationConflict = 3101U,
    EncryptionKeyNotFound = 3102U,
    EnvelopeInvalid = 3103U,
    RequestConflict = 3104U,
    GrantInvalid = 3200U,
    GrantExpired = 3201U,
    CredentialConflict = 3202U,
    ReceiptProofInvalid = 3300U,
    RequestExpired = 3301U,
    DeviceRevoked = 3302U,
    RateLimited = 3500U,
    TemporarilyUnavailable = 3501U
};

// The profile-specific EnrollmentKey/Ticket authorization remains outside
// this body. cloudSecret is present only as an X25519/AES-GCM envelope.
struct CloudCredentialEnrollmentRequest {
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    CloudCredentialSuite credentialSuite;
    CloudCredentialEnvelopeAlgorithm envelopeAlgorithm;
    uint32_t encryptionKeyId;
    ByteView ephemeralPublicKey;
    ByteView nonce;
    ByteView encryptedCredential;

    CloudCredentialEnrollmentRequest();
};

struct CloudCredentialEnrollmentGrant {
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    ByteView requestDigest;
    uint64_t issuedAt;
    uint64_t expiresAt;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView signature;

    CloudCredentialEnrollmentGrant();
};

struct CloudCredentialEnrollmentReceipt {
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    ByteView grantDigest;
    CloudEnrollmentProofAlgorithm proofAlgorithm;
    ByteView proof;

    CloudCredentialEnrollmentReceipt();
};

struct CloudCredentialEnrollmentCommitAck {
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    ByteView receiptDigest;
    CloudEnrollmentProofAlgorithm proofAlgorithm;
    ByteView proof;

    CloudCredentialEnrollmentCommitAck();
};

struct CloudEnrollmentErrorBody {
    CloudEnrollmentWireError error;
    CloudEnrollmentRetryClass retryClass;
    uint32_t retryAfterSeconds;
    bool hasRetryAfter;

    CloudEnrollmentErrorBody();
};

Result encodeCloudCredentialEnrollmentRequest(
    const CloudCredentialEnrollmentRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeCloudCredentialEnrollmentRequest(
    ByteView encoded,
    CloudCredentialEnrollmentRequest& body);
Result encodeCloudCredentialEnvelopeTranscript(
    const CloudCredentialEnrollmentRequest& body,
    MutableByteSpan output,
    ByteView& encoded);

Result encodeCloudCredentialEnrollmentGrant(
    const CloudCredentialEnrollmentGrant& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeCloudCredentialEnrollmentGrant(
    ByteView encoded,
    CloudCredentialEnrollmentGrant& body);
Result encodeCloudCredentialEnrollmentGrantTranscript(
    const CloudCredentialEnrollmentGrant& body,
    MutableByteSpan output,
    ByteView& encoded);

Result encodeCloudCredentialEnrollmentReceipt(
    const CloudCredentialEnrollmentReceipt& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeCloudCredentialEnrollmentReceipt(
    ByteView encoded,
    CloudCredentialEnrollmentReceipt& body);
Result encodeCloudCredentialEnrollmentReceiptTranscript(
    const CloudCredentialEnrollmentReceipt& body,
    MutableByteSpan output,
    ByteView& encoded);
Result computeCloudCredentialEnrollmentReceiptProof(
    ByteView cloudSecret,
    const CloudCredentialEnrollmentReceipt& body,
    MutableByteSpan transcriptWorkspace,
    MutableByteSpan proofOutput);
Result verifyCloudCredentialEnrollmentReceiptProof(
    ByteView cloudSecret,
    const CloudCredentialEnrollmentReceipt& body,
    MutableByteSpan transcriptWorkspace);

Result encodeCloudCredentialEnrollmentCommitAck(
    const CloudCredentialEnrollmentCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeCloudCredentialEnrollmentCommitAck(
    ByteView encoded,
    CloudCredentialEnrollmentCommitAck& body);
Result encodeCloudCredentialEnrollmentCommitAckTranscript(
    const CloudCredentialEnrollmentCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded);
Result computeCloudCredentialEnrollmentCommitAckProof(
    ByteView cloudSecret,
    const CloudCredentialEnrollmentCommitAck& body,
    MutableByteSpan transcriptWorkspace,
    MutableByteSpan proofOutput);
Result verifyCloudCredentialEnrollmentCommitAckProof(
    ByteView cloudSecret,
    const CloudCredentialEnrollmentCommitAck& body,
    MutableByteSpan transcriptWorkspace);

Result encodeCloudEnrollmentErrorBody(
    const CloudEnrollmentErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeCloudEnrollmentErrorBody(
    ByteView encoded,
    CloudEnrollmentErrorBody& body);
uint16_t cloudEnrollmentHttpStatus(CloudEnrollmentWireError error);
bool cloudEnrollmentHttpStatusMatches(
    uint16_t statusCode,
    CloudEnrollmentWireError error);

} // namespace blinker

#endif
