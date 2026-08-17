#ifndef BLINKER_PROVISIONING_OWNERSHIPRELEASECONTRACT_H
#define BLINKER_PROVISIONING_OWNERSHIPRELEASECONTRACT_H

#include "../core/Sha256.h"
#include "../identity/DeviceInstanceId.h"
#include "../identity/LogicalDeviceId.h"
#include "../interface/IServerSignatureVerifier.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {

enum class OwnershipReleaseOperation : uint8_t {
    Reset = 1U,
    Transfer = 2U
};

enum class OwnershipReleaseNetworkPolicy : uint8_t {
    Preserve = 1U,
    Clear = 2U
};

enum : size_t {
    kOwnershipReleaseRequestIdSize = 16U,
    kOwnershipReleaseNonceSize = 16U,
    kOwnershipReleaseGrantIdSize = 16U,
    kOwnershipReleaseSignatureSize = 64U,
    kOwnershipReleaseRequestMaxEncodedSize = 134U,
    kOwnershipReleaseGrantMaxEncodedSize = 288U,
    kOwnershipReleaseReceiptMaxEncodedSize = 175U,
    kOwnershipReleaseCommitAckMaxEncodedSize = 250U,
    kOwnershipReleaseWorkspaceSize = 384U
};

static const uint8_t kOwnershipReleaseContractVersion = 1U;

struct OwnershipReleaseRequest {
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t currentGeneration;
    ByteView releaseNonce;
    OwnershipReleaseOperation operation;
    OwnershipReleaseNetworkPolicy networkPolicy;

    OwnershipReleaseRequest();
};

struct OwnershipReleaseGrant {
    ByteView grantId;
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t currentGeneration;
    uint32_t nextGeneration;
    ByteView releaseNonce;
    OwnershipReleaseOperation operation;
    OwnershipReleaseNetworkPolicy networkPolicy;
    ByteView requestDigest;
    uint64_t issuedAt;
    uint64_t expiresAt;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView signature;

    OwnershipReleaseGrant();
};

struct OwnershipReleaseReceipt {
    ByteView grantId;
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t currentGeneration;
    uint32_t nextGeneration;
    OwnershipReleaseOperation operation;
    OwnershipReleaseNetworkPolicy networkPolicy;
    ByteView grantDigest;

    OwnershipReleaseReceipt();
};

struct OwnershipReleaseCommitAck {
    ByteView grantId;
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t currentGeneration;
    uint32_t nextGeneration;
    OwnershipReleaseOperation operation;
    OwnershipReleaseNetworkPolicy networkPolicy;
    ByteView receiptDigest;
    uint32_t serverKeyId;
    ServerSignatureAlgorithm signatureAlgorithm;
    ByteView signature;

    OwnershipReleaseCommitAck();
};

Result encodeOwnershipReleaseRequest(
    const OwnershipReleaseRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeOwnershipReleaseRequest(
    ByteView encoded,
    OwnershipReleaseRequest& body);

Result encodeOwnershipReleaseGrant(
    const OwnershipReleaseGrant& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeOwnershipReleaseGrant(
    ByteView encoded,
    OwnershipReleaseGrant& body);
Result encodeOwnershipReleaseGrantTranscript(
    const OwnershipReleaseGrant& body,
    MutableByteSpan output,
    ByteView& encoded);

Result encodeOwnershipReleaseReceipt(
    const OwnershipReleaseReceipt& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeOwnershipReleaseReceipt(
    ByteView encoded,
    OwnershipReleaseReceipt& body);

Result encodeOwnershipReleaseCommitAck(
    const OwnershipReleaseCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeOwnershipReleaseCommitAck(
    ByteView encoded,
    OwnershipReleaseCommitAck& body);
Result encodeOwnershipReleaseCommitAckTranscript(
    const OwnershipReleaseCommitAck& body,
    MutableByteSpan output,
    ByteView& encoded);

} // namespace blinker

#endif
