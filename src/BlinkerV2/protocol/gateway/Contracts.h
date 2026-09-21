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
    // Correlation prefix: nonzero uint64 BE route generation. Suffix: exchange ID.
    kRouteGenerationSize = 8U,
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
    kGatewayCredentialRenewalEnvelopeMaximumEncodedSize = 627U,
    kGatewayCredentialRenewalAadMaximumEncodedSize = 576U,
    kGatewayCredentialRenewalDeliveryAckMaximumEncodedSize = 76U,
    kChildRouteEnvelopeMaximumEncodedSize = 4280U,
    kGatewayAccessDeliveryAckMaximumEncodedSize = 58U,
    kGatewayProofCommandMaximumEncodedSize = 252U,
    kGatewayProofResultMaximumEncodedSize = 48U,
    kGatewayRevocationGrantMaximumSize = 193U,
    kGatewayRevocationReceiptMaximumSize = 145U,
    // Rotate and Revoke reuse the same signed child-control envelope limits,
    // but callers should not depend on the other operation's name.
    kGatewayCredentialRenewalGrantMaximumSize =
        kGatewayRevocationGrantMaximumSize,
    kGatewayCredentialRenewalReceiptMaximumSize =
        kGatewayRevocationReceiptMaximumSize,
    kGatewayRevocationCommandMaximumEncodedSize = 280U,
    kGatewayRevocationResultMaximumEncodedSize = 208U,
    kGatewayCredentialRenewalCommandMaximumEncodedSize = 337U,
    kGatewayCredentialRenewalResultMaximumEncodedSize = 274U,
    kGatewayPermitJoinCommandMaximumEncodedSize = 33U,
    kGatewayPermitJoinResultMaximumEncodedSize = 33U,
    kGatewayRouteAdmissionMaximumEncodedSize = 200U
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
    Finalize = 3U,
    // A newer child access root proves that the old Gateway credential no
    // longer exists on the child. The Hub therefore retires only its exact
    // matching outbound record without opening a child connection.
    Supersede = 4U
};

enum class GatewayRevocationStatus : uint8_t {
    NonceReady = 1U,
    Revoked = 2U,
    Finalized = 3U,
    Rejected = 4U,
    Expired = 5U,
    Retired = 6U
};

enum class GatewayCredentialRenewalPhase : uint8_t {
    Prepare = 1U,
    Apply = 2U,
    Finalize = 3U,
    Cancel = 4U
};

enum class GatewayCredentialRenewalStatus : uint8_t {
    NonceReady = 1U,
    Rotated = 2U,
    Finalized = 3U,
    Cancelled = 4U,
    ForwardRecoveryRequired = 5U,
    Rejected = 6U,
    Expired = 7U
};

// Adapter ids describe the southbound protocol composition, not a physical
// peer address. Decoders accept any non-zero uint16 id so an older Hub can
// return Unsupported for an additive adapter instead of failing the wire.
enum : uint16_t {
    kGatewaySouthboundAdapterNativeGatt = 1U
};

enum class GatewayPermitJoinAction : uint8_t {
    Open = 1U,
    Close = 2U
};

enum class GatewayPermitJoinStatus : uint8_t {
    Ready = 1U,
    Closed = 2U,
    Expired = 3U,
    Rejected = 4U,
    Busy = 5U,
    Unsupported = 6U
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

// A renewal envelope changes only the Gateway secret of one live topology.
// Its current PresenceKey remains active and is included indirectly through
// accessMaterialDigest = SHA-256(newSecret || currentPresenceKey). The
// topology operation id is deliberately separate from the renewal operation
// id so retrying a renewal never creates or replaces a topology.
struct GatewayCredentialRenewalEnvelopeView {
    uint8_t version;
    ByteView renewalOperationId;
    ByteView topologyOperationId;
    StringView environmentId;
    StringView tenantId;
    StringView edgeHubLogicalDeviceId;
    uint32_t deviceKeyVersion;
    StringView childLogicalDeviceId;
    ByteView childDeviceInstanceId;
    uint32_t accessEpoch;
    ByteView controllerId;
    uint32_t expectedCredentialVersion;
    uint32_t credentialVersion;
    uint32_t permissions;
    uint32_t presenceKeyVersion;
    ByteView accessMaterialDigest;
    uint64_t expiresAtUnixSeconds;
    ByteView nonce;
    ByteView ciphertext;
    ByteView tag;

    GatewayCredentialRenewalEnvelopeView();
};

Result validateGatewayCredentialRenewalEnvelope(
    const GatewayCredentialRenewalEnvelopeView& value);
Result encodeGatewayCredentialRenewalEnvelope(
    const GatewayCredentialRenewalEnvelopeView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayCredentialRenewalEnvelope(
    ByteView encoded,
    GatewayCredentialRenewalEnvelopeView& value);
Result encodeGatewayCredentialRenewalAad(
    const GatewayCredentialRenewalEnvelopeView& value,
    MutableByteSpan output,
    size_t& written);

struct GatewayCredentialRenewalDeliveryAckView {
    uint8_t version;
    ByteView renewalOperationId;
    ByteView topologyOperationId;
    ByteView accessMaterialDigest;
    uint32_t pendingStorageRevision;

    GatewayCredentialRenewalDeliveryAckView();
};

Result validateGatewayCredentialRenewalDeliveryAck(
    const GatewayCredentialRenewalDeliveryAckView& value);
Result encodeGatewayCredentialRenewalDeliveryAck(
    const GatewayCredentialRenewalDeliveryAckView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayCredentialRenewalDeliveryAck(
    ByteView encoded,
    GatewayCredentialRenewalDeliveryAckView& value);

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

// Independent active-credential transaction. Prepare obtains the child
// nonce, Apply relays one signed Rotate Grant and promotes only after its
// durable receipt, Finalize clears the protected pending journal after the
// existing GatewayProof contract succeeds, and Cancel is legal only before
// child mutation. Runtime state enforces that irreversible boundary.
struct GatewayCredentialRenewalCommandView {
    uint8_t version;
    ByteView renewalOperationId;
    ByteView topologyOperationId;
    uint32_t topologyVersion;
    ByteView childDeviceInstanceId;
    uint32_t accessEpoch;
    ByteView controllerId;
    uint32_t expectedCredentialVersion;
    uint32_t credentialVersion;
    uint32_t pendingStorageRevision;
    uint32_t activeStorageRevision;
    ByteView accessMaterialDigest;
    GatewayCredentialRenewalPhase phase;
    ByteView grant;
    ByteView receiptDigest;
    uint64_t expiresAtUnixSeconds;

    GatewayCredentialRenewalCommandView();
};

Result validateGatewayCredentialRenewalCommand(
    const GatewayCredentialRenewalCommandView& value);
Result encodeGatewayCredentialRenewalCommand(
    const GatewayCredentialRenewalCommandView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayCredentialRenewalCommand(
    ByteView encoded,
    GatewayCredentialRenewalCommandView& value);

struct GatewayCredentialRenewalResultView {
    uint8_t version;
    ByteView renewalOperationId;
    ByteView topologyOperationId;
    uint32_t topologyVersion;
    GatewayCredentialRenewalPhase phase;
    GatewayCredentialRenewalStatus status;
    uint32_t pendingStorageRevision;
    uint32_t activeStorageRevision;
    uint32_t credentialVersion;
    ByteView accessMaterialDigest;
    ByteView payload;
    ByteView receiptDigest;

    GatewayCredentialRenewalResultView();
};

Result validateGatewayCredentialRenewalResult(
    const GatewayCredentialRenewalResultView& value);
Result encodeGatewayCredentialRenewalResult(
    const GatewayCredentialRenewalResultView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayCredentialRenewalResult(
    ByteView encoded,
    GatewayCredentialRenewalResultView& value);

// Short, exact-Gateway permit-join control. The authenticated management
// topic and Service ledger bind account/tenant/Hub. The wire deliberately
// carries no child identity, discovery address, credential or secret.
struct GatewayPermitJoinCommandView {
    uint8_t version;
    ByteView operationId;
    uint16_t adapterId;
    GatewayPermitJoinAction action;
    uint64_t expiresAtUnixSeconds;

    GatewayPermitJoinCommandView();
};

Result validateGatewayPermitJoinCommand(
    const GatewayPermitJoinCommandView& value);
Result encodeGatewayPermitJoinCommand(
    const GatewayPermitJoinCommandView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayPermitJoinCommand(
    ByteView encoded,
    GatewayPermitJoinCommandView& value);

struct GatewayPermitJoinResultView {
    uint8_t version;
    ByteView operationId;
    uint16_t adapterId;
    GatewayPermitJoinStatus status;
    uint64_t expiresAtUnixSeconds;

    GatewayPermitJoinResultView();
};

Result validateGatewayPermitJoinResult(
    const GatewayPermitJoinResultView& value);
Result encodeGatewayPermitJoinResult(
    const GatewayPermitJoinResultView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGatewayPermitJoinResult(
    ByteView encoded,
    GatewayPermitJoinResultView& value);

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

// Route-topic lifecycle control, never a child BBP/2 frame or topology mutation.
// Canonical [1, 2, acknowledgment(0/1), topologyVersion, child, instance,
//            accessEpoch, credentialVersion, generation(bstr8 BE)].
struct GatewayRouteClosureView {
    bool acknowledgment;
    uint32_t topologyVersion;
    StringView childLogicalDeviceId;
    ByteView childDeviceInstanceId;
    uint32_t accessEpoch;
    uint32_t gatewayCredentialVersion;
    uint64_t generation;

    GatewayRouteClosureView();
};

Result encodeGatewayRouteClosure(
    const GatewayRouteClosureView& value, MutableByteSpan output, size_t& written);
Result decodeGatewayRouteClosure(ByteView encoded, GatewayRouteClosureView& value);

// Read-only Hub reachability assertion. Same exact route identity as closure,
// kind 3 / ten items, followed by a fresh Broker challenge (bstr8). It never
// authorizes a connection, changes access storage, or reaches the child BBP lane.
struct GatewayRouteLeaseView {
    GatewayRouteClosureView route;
    ByteView challenge;
};
Result encodeGatewayRouteLease(
    const GatewayRouteLeaseView& value, MutableByteSpan output, size_t& written);
Result decodeGatewayRouteLease(ByteView encoded, GatewayRouteLeaseView& value);

// Yielded is valid only in the independent DirectYield contract, never in
// GatewayExecutionDemand (which still accepts exactly operations 0..2).
enum class GatewayExecutionOperation : uint8_t { Probe = 0U, Challenge = 1U, Decision = 2U, Yielded = 3U };
enum : size_t {
    kGatewayExecutionIncarnationSize = 16U,
    kGatewayExecutionMaximumEncodedSize = 200U,
    kGatewayExecutionMaximumLeaseMillis = 30000U,
    kGatewayExecutionChallengeMillis = 5000U
};

// Independent connection demand, not reachability or a child BBP frame.
// [1,4,operation,topology,child,instance,epoch,credential,incarnation,id,leaseMs]
// Probe only asks for a fresh challenge (empty incarnation, id/lease zero).
// A Decision is bounded from HUB challenge issue time, never receive time.
struct GatewayExecutionDemandView {
    GatewayExecutionOperation operation;
    uint32_t topologyVersion;
    StringView childLogicalDeviceId;
    ByteView childDeviceInstanceId;
    uint32_t accessEpoch;
    uint32_t gatewayCredentialVersion;
    ByteView incarnation;
    uint32_t challengeId;
    uint32_t leaseMillis;

    GatewayExecutionDemandView();
};
Result encodeGatewayExecutionDemand(
    const GatewayExecutionDemandView& value, MutableByteSpan output, size_t& written);
Result decodeGatewayExecutionDemand(ByteView encoded, GatewayExecutionDemandView& value);

// [1,5,operation,topology,child,instance,epoch,credential,incarnation,id,leaseMs,controlId]
// Same bounded exchange fields, separate semantics: positive Decision asks the
// Hub to remain yielded, NEVER to acquire BLE. controlId is Broker-session-local
// nonzero monotonic uint32; Yielded echoes the exact Decision, not remaining TTL.
// Unlike ordinary execution Probe, Direct Probe includes the known CURRENT
// Hub route incarnation (learned from its execution Challenge). Reusing a
// counter on a new MQTT identity cannot match an old Probe/challenge anchor.
struct GatewayDirectYieldView {
    GatewayExecutionDemandView demand;
    uint32_t controlId;
    GatewayDirectYieldView() : demand(), controlId(0U) {}
};
Result encodeGatewayDirectYield(
    const GatewayDirectYieldView& value, MutableByteSpan output, size_t& written);
Result decodeGatewayDirectYield(ByteView encoded, GatewayDirectYieldView& value);

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
    // V2 recovery fence, scoped to the current MQTT identity; never authority.
    uint64_t retiredGeneration;

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
