#ifndef BLINKER_IDENTITY_GATEWAYCHILDRECORDS_H
#define BLINKER_IDENTITY_GATEWAYCHILDRECORDS_H

#include "GatewayAccessStore.h"
#include "GatewayCredentialRenewalStore.h"

namespace blinker {

enum class GatewayChildRecordState : uint8_t { Empty, Active, Revoked, RenewalOnly };

// Rebuildable, non-secret index, never an authority or another durable ledger.
// A tombstone/journal reserves its storage pair until existing finalization.
struct GatewayChildIdentity {
    GatewayChildRecordState state;
    bool hasRenewal;
    uint8_t logicalIdSize;
    char logicalId[gateway::kMaximumLogicalDeviceIdSize];
    uint8_t instance[gateway::kDeviceInstanceIdSize];
    uint8_t operation[gateway::kOperationIdSize];
    uint32_t accessEpoch;
    uint32_t credentialVersion;

    GatewayChildIdentity() : state(GatewayChildRecordState::Empty), hasRenewal(false),
        logicalIdSize(0U), logicalId(), instance(), operation(), accessEpoch(0U), credentialVersion(0U) {}
};

// Owns the ONLY mutation paths for one injected durable storage pair. All
// processors/coordinators borrow these stores, never the underlying blobs.
// Invalidate before opening/closing/replacing a backend outside this owner.
// Storage/selection run on the product loop, not concurrently in SDK callbacks.
class GatewayChildRecords {
private:
    class TrackedBlob final : public IAtomicBlobStore {
    public:
        TrackedBlob(IAtomicBlobStore& storage, GatewayChildRecords& owner)
            : storage_(storage), owner_(owner) {}
        Result load(MutableByteSpan output, size_t& written) override { return storage_.load(output, written); }
        Result replace(ByteView value) override {
            owner_.invalidate(); // Also invalidate an uncertain/failed write.
            return storage_.replace(value);
        }
        Result clear() override { owner_.invalidate(); return storage_.clear(); }
        StorageProtection protection() const override { return storage_.protection(); }
    private:
        IAtomicBlobStore& storage_;
        GatewayChildRecords& owner_;
    };

    GatewayChildIdentity identity_;
    bool current_;
    TrackedBlob accessBlob_, renewalBlob_;
    Result refresh(); // Cold decoder workspaces must not burden a warm match.

public:
    GatewayAccessStore access;
    GatewayCredentialRenewalStore renewal;

    GatewayChildRecords(IAtomicBlobStore& accessBlob, IAtomicBlobStore& renewalBlob)
        : identity_(), current_(false), accessBlob_(accessBlob, *this), renewalBlob_(renewalBlob, *this),
          access(accessBlob_), renewal(renewalBlob_) {}

    void invalidate() { current_ = false; identity_ = GatewayChildIdentity(); }
    // Borrow until the next mutation/invalidation. No raw secret is cached.
    Result identity(const GatewayChildIdentity*& output);

    GatewayChildRecords(const GatewayChildRecords&) = delete;
    GatewayChildRecords& operator=(const GatewayChildRecords&) = delete;
};

} // namespace blinker
#endif
