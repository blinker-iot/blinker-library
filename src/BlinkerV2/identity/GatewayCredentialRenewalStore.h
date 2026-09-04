#ifndef BLINKER_IDENTITY_GATEWAYCREDENTIALRENEWALSTORE_H
#define BLINKER_IDENTITY_GATEWAYCREDENTIALRENEWALSTORE_H

#include "../interface/IAtomicBlobStore.h"
#include "../protocol/gateway/Contracts.h"

namespace blinker {

enum class GatewayCredentialRenewalState : uint8_t {
    Staged = 1U,
    MutationAttempted = 2U,
    ReceiptStored = 3U,
    Promoted = 4U,
    Abandoned = 5U
};

// One active-renewal transaction. It is intentionally separate from the
// active GatewayAccessStore record: a reset must never replace a working
// credential with an unproven candidate. The exact child receipt is retained
// only until proof/finalization makes the new active record authoritative.
struct GatewayCredentialRenewalRecord {
    uint32_t storageRevision;
    uint32_t deliveryRevision;
    GatewayCredentialRenewalState state;
    uint8_t renewalOperationId[gateway::kOperationIdSize];
    uint8_t topologyOperationId[gateway::kOperationIdSize];
    uint8_t childDeviceInstanceId[gateway::kDeviceInstanceIdSize];
    uint32_t accessEpoch;
    uint8_t controllerId[gateway::kControllerIdSize];
    uint32_t expectedCredentialVersion;
    uint32_t credentialVersion;
    uint32_t presenceKeyVersion;
    uint64_t expiresAtUnixSeconds;
    uint8_t secret[gateway::kGatewaySecretSize];
    uint8_t accessMaterialDigest[gateway::kAccessMaterialDigestSize];
    uint8_t grantDigest[gateway::kAccessMaterialDigestSize];
    uint16_t receiptSize;
    uint8_t receipt[gateway::kGatewayCredentialRenewalReceiptMaximumSize];
    uint32_t activeStorageRevision;

    GatewayCredentialRenewalRecord();
};

void clearGatewayCredentialRenewalRecord(
    GatewayCredentialRenewalRecord& value);

enum class GatewayCredentialRenewalCommitKind : uint8_t {
    Stored = 1U,
    ExactReplay = 2U
};

struct GatewayCredentialRenewalCommitOutcome {
    GatewayCredentialRenewalCommitKind kind;
    uint32_t pendingStorageRevision;

    GatewayCredentialRenewalCommitOutcome()
        : kind(GatewayCredentialRenewalCommitKind::Stored),
          pendingStorageRevision(0U) {}
};

class GatewayCredentialRenewalStore {
public:
    enum : size_t { serializedSize = 360U };

    explicit GatewayCredentialRenewalStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(GatewayCredentialRenewalRecord& output);

    // candidate must be a pristine Staged record. Exact delivery replay is
    // write-free even after later local transitions and returns the original
    // pendingStorageRevision used by the durable ACK.
    Result commit(
        const GatewayCredentialRenewalRecord& candidate,
        GatewayCredentialRenewalCommitOutcome& outcome);

    // Must complete durably before a Rotate Grant can be sent to the child.
    // Cancel is forbidden while the outcome is unknown. A fresh Method 2
    // session may later prove either N+1 (continue forward) or N (mark the
    // transaction Abandoned, then permit an explicit Cancel).
    Result markMutationAttempted(
        ByteView renewalOperationId,
        ByteView topologyOperationId,
        ByteView accessMaterialDigest,
        ByteView grantDigest,
        uint32_t& storageRevision);

    // Legal only after a fresh Method 2 session proves the child still uses
    // expectedCredentialVersion. Retaining this terminal marker makes a lost
    // Rejected result replayable until Cancel clears the transaction.
    Result markAbandoned(
        ByteView renewalOperationId,
        ByteView topologyOperationId,
        ByteView accessMaterialDigest,
        ByteView grantDigest,
        uint32_t& storageRevision);

    Result storeReceipt(
        ByteView renewalOperationId,
        ByteView topologyOperationId,
        ByteView accessMaterialDigest,
        ByteView grantDigest,
        ByteView exactReceipt,
        uint32_t& storageRevision);

    Result markPromoted(
        ByteView renewalOperationId,
        ByteView topologyOperationId,
        ByteView accessMaterialDigest,
        ByteView receiptDigest,
        uint32_t activeStorageRevision,
        uint32_t& storageRevision);

    // Legal while Staged or after a proved-uncommitted Abandoned outcome.
    // Empty storage is an idempotent success.
    Result cancel(
        ByteView renewalOperationId,
        ByteView topologyOperationId,
        ByteView accessMaterialDigest);

    // Legal only after exact active promotion. Empty storage is idempotent.
    Result finalize(
        ByteView renewalOperationId,
        ByteView topologyOperationId,
        ByteView accessMaterialDigest,
        ByteView receiptDigest,
        uint32_t activeStorageRevision);

    Result clear();

    StorageProtection protection() const { return storage_.protection(); }

    static Result receiptDigest(
        const GatewayCredentialRenewalRecord& value,
        MutableByteSpan output);
    static Result validateDurable(
        const GatewayCredentialRenewalRecord& value);
    static Result encode(
        const GatewayCredentialRenewalRecord& value,
        MutableByteSpan output);
    static Result decode(
        ByteView input,
        GatewayCredentialRenewalRecord& output);

private:
    IAtomicBlobStore& storage_;

    GatewayCredentialRenewalStore(const GatewayCredentialRenewalStore&);
    GatewayCredentialRenewalStore& operator=(
        const GatewayCredentialRenewalStore&);
};

} // namespace blinker

#endif
