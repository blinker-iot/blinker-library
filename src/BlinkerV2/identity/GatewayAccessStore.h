#ifndef BLINKER_IDENTITY_GATEWAYACCESSSTORE_H
#define BLINKER_IDENTITY_GATEWAYACCESSSTORE_H

#include "../interface/IAtomicBlobStore.h"
#include "../protocol/gateway/Contracts.h"
#include "ControllerCredential.h"

namespace blinker {

// One outbound child credential owned by an Edge Hub. This is deliberately
// separate from DeviceAccessStore: child access slots authorize controllers
// entering this device, while this record authorizes the Hub leaving toward a
// child. G2-0D starts with one record and measures before adding capacity.
struct GatewayPresenceLocator {
    uint32_t version;
    uint8_t key[gateway::kGatewayPresenceKeySize];

    GatewayPresenceLocator() : version(0U), key() {}
};

// One outbound child access record. The Method 2 credential is shared by the
// active and staged locator; a bounded pending slot prevents PresenceKey
// rotation from making the child undiscoverable before proof/activation.
struct GatewayAccessRecord {
    uint32_t storageRevision;
    // Revision returned by the last durable delivery ACK. Local activation or
    // rollback may advance storageRevision, but must not change replayed ACKs.
    uint32_t deliveryRevision;
    uint8_t operationId[gateway::kOperationIdSize];
    uint8_t envelopeDigest[gateway::kAccessMaterialDigestSize];
    uint8_t accessMaterialDigest[gateway::kAccessMaterialDigestSize];
    uint8_t childLogicalDeviceIdSize;
    char childLogicalDeviceId[gateway::kMaximumLogicalDeviceIdSize];
    uint8_t childDeviceInstanceId[gateway::kDeviceInstanceIdSize];
    uint32_t accessEpoch;
    uint8_t controllerId[gateway::kControllerIdSize];
    uint32_t credentialVersion;
    uint32_t permissions;
    uint64_t expiresAtUnixSeconds;
    uint8_t secret[gateway::kGatewaySecretSize];
    GatewayPresenceLocator activePresence;
    GatewayPresenceLocator pendingPresence;

    GatewayAccessRecord();
};

void clearGatewayAccessRecord(GatewayAccessRecord& value);

// Materializes the exact outbound Method 2 credential only while the durable
// gateway record is valid at trusted Unix time. Callers must clear output when
// the authentication attempt finishes.
Result makeGatewayControllerCredential(
    const GatewayAccessRecord& record,
    uint64_t unixSeconds,
    ControllerCredential& output);

enum class GatewayAccessCommitKind : uint8_t {
    Stored = 1U,
    ExactReplay = 2U,
    RewrappedReplay = 3U
};

struct GatewayAccessCommitOutcome {
    GatewayAccessCommitKind kind;
    uint32_t storageRevision;

    GatewayAccessCommitOutcome()
        : kind(GatewayAccessCommitKind::Stored), storageRevision(0U) {}
};

// Durable proof that the child credential was removed. It intentionally
// contains no controller secret or PresenceKey. The exact receipt is retained
// only until the Service confirms Detached, so an MQTT/result loss or Hub
// reboot can replay the same device fact without reconnecting to the child.
struct GatewayRevocationTombstone {
    enum : size_t { maximumReceiptSize = 145U };

    uint32_t storageRevision;
    uint8_t operationId[gateway::kOperationIdSize];
    uint32_t topologyVersion;
    uint8_t grantDigest[gateway::kAccessMaterialDigestSize];
    uint8_t receiptDigest[gateway::kAccessMaterialDigestSize];
    uint16_t receiptSize;
    uint8_t receipt[maximumReceiptSize];

    GatewayRevocationTombstone();
};

void clearGatewayRevocationTombstone(
    GatewayRevocationTombstone& value);

class GatewayAccessStore {
public:
    enum : size_t { serializedSize = 352U };

    explicit GatewayAccessStore(IAtomicBlobStore& storage)
        : storage_(storage) {}

    Result load(GatewayAccessRecord& output);

    // Candidate revisions must be zero. A new operation increments both the
    // durable storage revision and the delivery revision used by its ACK.
    // Exact QoS replay performs no write. Re-sealing the same operation after
    // DeviceKey rotation preserves the original delivery/ACK identity.
    Result commit(
        const GatewayAccessRecord& candidate,
        GatewayAccessCommitOutcome& outcome);

    // Promotes exactly the staged PresenceKey after the Hub has rediscovered
    // and authenticated the child. Promotion is an atomic durable mutation.
    Result activatePendingPresence(
        uint32_t expectedVersion,
        uint32_t& storageRevision);

    // Cancels an unproven staged locator without changing the active key.
    Result discardPendingPresence(
        uint32_t expectedVersion,
        uint32_t& storageRevision);

    // Atomically removes the active secret and replaces it with a replayable
    // tombstone after an exact child Revoke receipt has been validated.
    // Candidate storageRevision must be zero. Replaying the exact candidate
    // is write-free; any changed Grant/receipt is a sequence conflict.
    Result commitRevocation(
        uint32_t expectedDeliveryRevision,
        const GatewayRevocationTombstone& candidate,
        uint32_t& storageRevision);
    Result loadRevocation(GatewayRevocationTombstone& output);

    // Final Service acknowledgement. Empty storage is an idempotent success;
    // an active record or a different tombstone is never cleared.
    Result clearRevocation(
        ByteView operationId,
        uint32_t topologyVersion,
        ByteView receiptDigest);

    // Finalize normally clears a matching tombstone. In the narrow crash
    // window where the child durably revoked its credential before this Hub
    // could store that tombstone, the same authenticated Cloud finalization
    // may clear only the exact still-active outbound record. Every durable
    // access binding is required; a different record is never removed.
    Result finalizeRevocation(
        ByteView operationId,
        uint32_t topologyVersion,
        ByteView receiptDigest,
        ByteView childDeviceInstanceId,
        uint32_t accessEpoch,
        ByteView controllerId,
        uint32_t credentialVersion,
        uint32_t deliveryRevision);

    Result clear();

    StorageProtection protection() const {
        return storage_.protection();
    }

    static Result encode(
        const GatewayAccessRecord& value,
        MutableByteSpan output);
    static Result decode(
        ByteView input,
        GatewayAccessRecord& output);

private:
    IAtomicBlobStore& storage_;

    GatewayAccessStore(const GatewayAccessStore&);
    GatewayAccessStore& operator=(const GatewayAccessStore&);
};

} // namespace blinker

#endif
