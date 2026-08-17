#ifndef BLINKER_PROVISIONING_OWNERSHIPCLAIMCONTRACT_H
#define BLINKER_PROVISIONING_OWNERSHIPCLAIMCONTRACT_H

#include "../core/Sha256.h"
#include "../identity/DeviceInstanceId.h"
#include "../identity/LogicalDeviceId.h"
#include "../interface/IServerSignatureVerifier.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {

enum : size_t {
    kOwnershipClaimRequestIdSize = 16U,
    kOwnershipClaimNonceSize = 16U,
    kOwnershipClaimGrantIdSize = 16U,
    kOwnershipClaimGrantSignatureSize = 64U,
    kOwnershipClaimGrantDigestSize = kSha256Size,
    kOwnershipClaimCommitIdempotencyKeySize = 16U,
    kOwnershipClaimRequestMaxEncodedSize = 63U,
    kOwnershipClaimGrantMaxEncodedSize = 243U,
    kOwnershipClaimReceiptMaxEncodedSize = 165U,
    kOwnershipClaimCommitMaxEncodedSize = 183U,
    kOwnershipClaimCommitAckMaxEncodedSize = 250U,
    kOwnershipClaimWorkspaceSize = 256U
};

static const uint8_t kOwnershipClaimContractVersion = 1U;

// Common device request. EnrollmentKey, EnrollmentTicket, user JWT and
// factory authorization belong to the profile-specific authenticated
// envelope, not to this reusable ownership body.
struct OwnershipClaimRequest {
    ByteView requestId;
    ByteView deviceInstanceId;
    uint32_t ownershipGeneration;
    ByteView claimNonce;

    OwnershipClaimRequest() : ownershipGeneration(0U) {}
};

// Server-signed authorization for one Unclaimed -> Active transition.
struct OwnershipClaimGrant {
    ByteView grantId;
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    ByteView claimNonce;
    uint64_t issuedAt;
    uint64_t expiresAt;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView signature;

    OwnershipClaimGrant();
};

// Device result after the exact grant is durable. This receipt intentionally
// has no independent bearer proof: the signed grant authorizes the mutation,
// while the surrounding setup/cloud channel authenticates its sender.
struct OwnershipClaimReceipt {
    ByteView grantId;
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    ByteView grantDigest;

    OwnershipClaimReceipt() : ownershipGeneration(0U) {}
};

// App/Gateway bookkeeping. Device activation is already complete when the
// signed grant has been persisted; commit acknowledgement is safely retryable.
struct OwnershipClaimCommitRequest {
    ByteView grantId;
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    ByteView receiptDigest;
    ByteView idempotencyKey;

    OwnershipClaimCommitRequest() : ownershipGeneration(0U) {}
};

// Server proof that the exact device receipt was committed. This Ack is
// required before a Plain-HTTP device may clear its durable Claim journal.
struct OwnershipClaimCommitAck {
    ByteView grantId;
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    ByteView receiptDigest;
    uint64_t committedAt;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView signature;

    OwnershipClaimCommitAck();
};

Result encodeOwnershipClaimRequest(
    const OwnershipClaimRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeOwnershipClaimRequest(
    ByteView encoded,
    OwnershipClaimRequest& body);

Result encodeOwnershipClaimGrant(
    const OwnershipClaimGrant& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeOwnershipClaimGrant(
    ByteView encoded,
    OwnershipClaimGrant& body);
Result encodeOwnershipClaimGrantTranscript(
    const OwnershipClaimGrant& body,
    MutableByteSpan output,
    ByteView& encoded);

Result encodeOwnershipClaimReceipt(
    const OwnershipClaimReceipt& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeOwnershipClaimReceipt(
    ByteView encoded,
    OwnershipClaimReceipt& body);

Result encodeOwnershipClaimCommitRequest(
    const OwnershipClaimCommitRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeOwnershipClaimCommitRequest(
    ByteView encoded,
    OwnershipClaimCommitRequest& body);

Result encodeOwnershipClaimCommitAck(
    const OwnershipClaimCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeOwnershipClaimCommitAck(
    ByteView encoded,
    OwnershipClaimCommitAck& body);
Result encodeOwnershipClaimCommitAckTranscript(
    const OwnershipClaimCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded);

} // namespace blinker

#endif
