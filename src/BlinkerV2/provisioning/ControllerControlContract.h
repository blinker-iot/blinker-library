#ifndef BLINKER_PROVISIONING_CONTROLLERCONTROLCONTRACT_H
#define BLINKER_PROVISIONING_CONTROLLERCONTROLCONTRACT_H

#include "../core/HmacSha256.h"
#include "../identity/ControllerCredential.h"
#include "../identity/DeviceInstanceId.h"
#include "../interface/IServerSignatureVerifier.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {

enum : size_t {
    kControllerGrantIdSize = 16U,
    kControllerControlNonceSize = 16U,
    kControllerSecretDigestSize = 32U,
    kControllerReceiptProofSize = 32U,
    kControllerCommitIdempotencyKeySize = 16U,
    kControllerGrantSignatureSize = 64U,
    kControllerGrantMaxEncodedSize = 193U,
    kControllerReceiptMaxEncodedSize = 145U,
    kControllerCommitMaxEncodedSize = 126U,
    kControllerControlWorkspaceSize = 256U
};

static const uint8_t kControllerControlContractVersion = 2U;

enum class ControllerMutationOperation : uint8_t {
    Install = 1U,
    Rotate = 2U,
    Revoke = 3U
};

enum class ControllerReceiptProofKind : uint8_t {
    None = 0U,
    HmacSha256 = 1U
};

enum class ControllerCommitVerification : uint8_t {
    Method2Confirmed = 1U,
    AuthorizedRevoke = 2U
};

// Server-signed authorization for one platform-direct mutation. The server
// keeps account/logical-device mapping; the device binds this compact grant
// to its unique instance, current access epoch and fresh control nonce. The
// ownershipGeneration member name is retained only by the v2 wire contract.
// The secret never appears in this body; only its SHA-256 digest is signed.
struct ControllerGrant {
    ControllerMutationOperation operation;
    ByteView grantId;
    ByteView deviceInstanceId;
    uint32_t ownershipGeneration;
    ByteView controllerId;
    uint32_t expectedCredentialVersion;
    uint32_t credentialVersion;
    uint32_t permissions;
    ByteView secretDigest;
    ByteView controlNonce;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView signature;

    ControllerGrant();
};

// Device result after the verified mutation is durable. Install/rotate carry
// an HMAC made with the new controller secret; revoke intentionally has no
// secret proof and is confirmed by the authorized management path.
struct ControllerMutationReceipt {
    ControllerMutationOperation operation;
    ByteView grantId;
    ByteView deviceInstanceId;
    uint32_t ownershipGeneration;
    ByteView controllerId;
    uint32_t credentialVersion;
    uint32_t permissions;
    ByteView secretDigest;
    ControllerReceiptProofKind proofKind;
    ByteView proof;

    ControllerMutationReceipt();
};

// App-to-server bookkeeping after direct verification. This does not activate
// the device credential; the signed grant already authorizes the device-side
// mutation. The server never receives the raw controller secret.
struct ControllerCommitRequest {
    ByteView grantId;
    ControllerMutationOperation operation;
    ByteView deviceInstanceId;
    uint32_t ownershipGeneration;
    ByteView controllerId;
    uint32_t credentialVersion;
    ByteView receiptDigest;
    ControllerCommitVerification verification;
    ByteView idempotencyKey;

    ControllerCommitRequest();
};

Result encodeControllerGrant(
    const ControllerGrant& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeControllerGrant(ByteView encoded, ControllerGrant& body);
Result encodeControllerGrantTranscript(
    const ControllerGrant& body,
    MutableByteSpan output,
    ByteView& encoded);

Result encodeControllerMutationReceipt(
    const ControllerMutationReceipt& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeControllerMutationReceipt(
    ByteView encoded,
    ControllerMutationReceipt& body);
Result encodeControllerMutationReceiptTranscript(
    const ControllerMutationReceipt& body,
    MutableByteSpan output,
    ByteView& encoded);
Result computeControllerMutationReceiptProof(
    ByteView controllerSecret,
    const ControllerMutationReceipt& body,
    MutableByteSpan transcriptWorkspace,
    MutableByteSpan proofOutput);
// Builds the canonical deterministic receipt for a decoded Grant. The HMAC
// proves possession of the installed secret; it is not device
// non-repudiation. Callers must establish the mutation outcome separately.
Result buildControllerMutationReceipt(
    const ControllerGrant& grant,
    ByteView controllerSecret,
    MutableByteSpan transcriptWorkspace,
    MutableByteSpan output,
    ByteView& encoded);
Result verifyControllerMutationReceiptProof(
    ByteView controllerSecret,
    const ControllerMutationReceipt& body,
    MutableByteSpan transcriptWorkspace);

Result encodeControllerCommitRequest(
    const ControllerCommitRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeControllerCommitRequest(
    ByteView encoded,
    ControllerCommitRequest& body);

} // namespace blinker

#endif
