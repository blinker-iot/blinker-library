#ifndef BLINKER_PROTOCOL_GATEWAY_CONTRACTS_H
#define BLINKER_PROTOCOL_GATEWAY_CONTRACTS_H

#include "../../core/Result.h"
#include "../../core/Span.h"

namespace blinker {
namespace gateway {

enum : size_t {
    kOperationIdSize = 16U,
    kDeviceInstanceIdSize = 16U,
    kControllerIdSize = 16U,
    kCorrelationIdSize = 16U,
    kProofNonceSize = 16U,
    kEnvelopeNonceSize = 12U,
    kGatewaySecretSize = 32U,
    kGatewayPresenceKeySize = 16U,
    kGatewayAccessMaterialSize =
        kGatewaySecretSize + kGatewayPresenceKeySize,
    kEnvelopeTagSize = 16U,
    kAccessMaterialDigestSize = 32U,
    kMaximumEnvironmentIdSize = 32U,
    kMaximumTenantIdSize = 128U,
    kMaximumLogicalDeviceIdSize = 128U,
    kMaximumInnerFrameSize = 4096U,
    // G2A uses the existing constrained child profile. Larger route frames
    // remain reserved for a separately measured profile.
    kEdgeHubChildMaximumInnerFrameSize = 512U,
    kEdgeHubChildRouteEnvelopeMaximumEncodedSize = 696U,
    kGatewayAccessEnvelopeMaximumEncodedSize = 587U,
    kGatewayAccessAadMaximumEncodedSize = 520U,
    kChildRouteEnvelopeMaximumEncodedSize = 4280U,
    kGatewayAccessDeliveryAckMaximumEncodedSize = 58U,
    kGatewayProofCommandMaximumEncodedSize = 252U,
    kGatewayProofResultMaximumEncodedSize = 48U,
    kGatewayRevocationGrantMaximumSize = 193U,
    kGatewayRevocationReceiptMaximumSize = 145U,
    kGatewayRevocationCommandMaximumEncodedSize = 280U,
    kGatewayRevocationResultMaximumEncodedSize = 208U,
    kGatewayRouteAdmissionMaximumEncodedSize = 191U
};

enum : uint32_t {
    kGatewayPermissionObserve = 1UL << 0U,
    kGatewayPermissionControl = 1UL << 1U,
    kGatewayPermissions =
        kGatewayPermissionObserve | kGatewayPermissionControl
};

enum class TopologyState : uint8_t {
    Created = 1U,
    PendingAccessDelivery = 2U,
    PendingChildInstall = 3U,
    PendingGatewayProof = 4U,
    Active = 5U,
    Expired = 6U,
    Cancelled = 7U,
    RollbackRequired = 8U,
    Revoking = 9U,
    Detached = 10U
};

enum class GatewayProofStatus : uint8_t {
    Secure = 1U,
    Rejected = 2U,
    Expired = 3U
};

enum class GatewayRevocationPhase : uint8_t {
    Prepare = 1U,
    Apply = 2U,
    Finalize = 3U
};

enum class GatewayRevocationStatus : uint8_t {
    NonceReady = 1U,
    Revoked = 2U,
    Finalized = 3U,
    Rejected = 4U,
    Expired = 5U
};

bool validTopologyState(TopologyState state);
bool validTopologyTransition(TopologyState from, TopologyState to);

struct GatewayAccessEnvelopeView {
    uint8_t version;
    ByteView operationId;
    StringView environmentId;
    StringView tenantId;
    StringView edgeHubLogicalDeviceId;
    uint32_t deviceKeyVersion;
    StringView childLogicalDeviceId;
    ByteView childDeviceInstanceId;
    uint32_t accessEpoch;
    ByteView controllerId;
    uint32_t credentialVersion;
    uint32_t permissions;
    uint32_t presenceKeyVersion;
    uint64_t expiresAtUnixSeconds;
    ByteView nonce;
    ByteView ciphertext;
    ByteView tag;

    GatewayAccessEnvelopeView();
};

Result validateGatewayAccessEnvelope(
    const GatewayAccessEnvelopeView& value);
Result encodeGatewayAccessEnvelope(
    const GatewayAccessEnvelopeView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayAccessEnvelope(
    ByteView encoded,
    GatewayAccessEnvelopeView& value);
Result encodeGatewayAccessAad(
    const GatewayAccessEnvelopeView& value,
    MutableByteSpan output,
    size_t& written);

struct GatewayAccessDeliveryAckView {
    uint8_t version;
    ByteView operationId;
    ByteView accessMaterialDigest;
    uint32_t storageRevision;

    GatewayAccessDeliveryAckView();
};

Result validateGatewayAccessDeliveryAck(
    const GatewayAccessDeliveryAckView& value);
Result encodeGatewayAccessDeliveryAck(
    const GatewayAccessDeliveryAckView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayAccessDeliveryAck(
    ByteView encoded,
    GatewayAccessDeliveryAckView& value);

// A proof command is a short-lived, idempotent control task. It binds the
// exact durable Gateway access record to the exact pending topology. MQTTS
// authenticates the reporting Hub; this message deliberately does not add a
// self-computable "child proof" MAC that would create a false trust root.
struct GatewayProofCommandView {
    uint8_t version;
    ByteView operationId;
    uint32_t topologyVersion;
    StringView childLogicalDeviceId;
    ByteView childDeviceInstanceId;
    uint32_t accessEpoch;
    uint32_t gatewayCredentialVersion;
    uint32_t presenceKeyVersion;
    ByteView accessMaterialDigest;
    uint32_t accessStorageRevision;
    ByteView proofNonce;
    uint64_t expiresAtUnixSeconds;

    GatewayProofCommandView();
};

Result validateGatewayProofCommand(const GatewayProofCommandView& value);
Result encodeGatewayProofCommand(
    const GatewayProofCommandView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayProofCommand(
    ByteView encoded,
    GatewayProofCommandView& value);

struct GatewayProofResultView {
    uint8_t version;
    ByteView operationId;
    uint32_t topologyVersion;
    ByteView proofNonce;
    GatewayProofStatus status;
    uint32_t presenceKeyVersion;

    GatewayProofResultView();
};

Result validateGatewayProofResult(const GatewayProofResultView& value);
Result encodeGatewayProofResult(
    const GatewayProofResultView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayProofResult(
    ByteView encoded,
    GatewayProofResultView& value);

// Authenticated management transaction for removing one child credential.
// Prepare carries no Grant and returns a fresh device nonce. Apply carries the
// signed Revoke Grant and returns the exact durable child receipt. Finalize
// carries the receipt digest and only clears the Hub tombstone; stale
// finalization cannot clear another active topology.
struct GatewayRevocationCommandView {
    uint8_t version;
    ByteView operationId;
    uint32_t topologyVersion;
    ByteView childDeviceInstanceId;
    uint32_t accessEpoch;
    ByteView controllerId;
    uint32_t gatewayCredentialVersion;
    uint32_t accessStorageRevision;
    GatewayRevocationPhase phase;
    ByteView grant;
    ByteView receiptDigest;
    uint64_t expiresAtUnixSeconds;

    GatewayRevocationCommandView();
};

Result validateGatewayRevocationCommand(
    const GatewayRevocationCommandView& value);
Result encodeGatewayRevocationCommand(
    const GatewayRevocationCommandView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayRevocationCommand(
    ByteView encoded,
    GatewayRevocationCommandView& value);

struct GatewayRevocationResultView {
    uint8_t version;
    ByteView operationId;
    uint32_t topologyVersion;
    GatewayRevocationPhase phase;
    GatewayRevocationStatus status;
    ByteView payload;
    ByteView receiptDigest;

    GatewayRevocationResultView();
};

Result validateGatewayRevocationResult(
    const GatewayRevocationResultView& value);
Result encodeGatewayRevocationResult(
    const GatewayRevocationResultView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayRevocationResult(
    ByteView encoded,
    GatewayRevocationResultView& value);

struct ChildRouteEnvelopeView {
    uint8_t version;
    uint32_t topologyVersion;
    StringView childLogicalDeviceId;
    ByteView childDeviceInstanceId;
    uint32_t accessEpoch;
    uint32_t gatewayCredentialVersion;
    ByteView correlationId;
    ByteView innerBbp2Frame;

    ChildRouteEnvelopeView();
};

// Non-secret bootstrap sent only after the Broker resolves an Active
// topology. It lets a rebooted Hub bind the route to its durable access
// record before opening the child DirectSecure session.
struct GatewayRouteAdmissionView {
    uint8_t version;
    ByteView operationId;
    uint32_t topologyVersion;
    StringView childLogicalDeviceId;
    ByteView childDeviceInstanceId;
    uint32_t accessEpoch;
    uint32_t gatewayCredentialVersion;
    uint64_t expiresAtUnixSeconds;

    GatewayRouteAdmissionView();
};

Result validateGatewayRouteAdmission(
    const GatewayRouteAdmissionView& value);
Result encodeGatewayRouteAdmission(
    const GatewayRouteAdmissionView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayRouteAdmission(
    ByteView encoded,
    GatewayRouteAdmissionView& value);

Result validateChildRouteEnvelope(const ChildRouteEnvelopeView& value);
Result encodeChildRouteEnvelope(
    const ChildRouteEnvelopeView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeChildRouteEnvelope(
    ByteView encoded,
    ChildRouteEnvelopeView& value);

} // namespace gateway
} // namespace blinker

#endif
