#ifndef BLINKER_RUNTIME_GATEWAYSESSIONACCESSSOURCE_H
#define BLINKER_RUNTIME_GATEWAYSESSIONACCESSSOURCE_H

#include "../core/SecureMemory.h"
#include "../identity/GatewayAccessStore.h"
#include "../identity/GatewayCredentialRenewalStore.h"

#include <string.h>

namespace blinker {

// Selects the outbound credential used to authenticate one child session.
// During a crash-recoverable Rotate, the child may have committed either the
// old active credential or the new journaled credential. Try the forward
// credential first, then alternate only after an authentication failure.
// Discovery continues to use the active PresenceKey, which renewal does not
// change.
class GatewaySessionAccessSource {
public:
    explicit GatewaySessionAccessSource(GatewayAccessStore& active)
        : active_(active), renewal_(nullptr), preferRenewal_(true),
          selectedRenewal_(false), hasAlternative_(false) {}

    GatewaySessionAccessSource(
        GatewayAccessStore& active,
        GatewayCredentialRenewalStore& renewal)
        : active_(active), renewal_(&renewal), preferRenewal_(true),
          selectedRenewal_(false), hasAlternative_(false) {}

    Result load(GatewayAccessRecord& output) {
        selectedRenewal_ = false;
        hasAlternative_ = false;
        Result result = active_.load(output);
        if (!result || renewal_ == nullptr) return result;

        GatewayCredentialRenewalRecord pending;
        result = renewal_->load(pending);
        if (result.code() == ErrorCode::NotFound ||
            (result && pending.state ==
                           GatewayCredentialRenewalState::Staged) ||
            (result && pending.state ==
                           GatewayCredentialRenewalState::Abandoned)) {
            clearGatewayCredentialRenewalRecord(pending);
            return Result::success();
        }
        if (!result) {
            clearGatewayCredentialRenewalRecord(pending);
            clearGatewayAccessRecord(output);
            return result;
        }

        result = validatePair(output, pending);
        if (result && output.credentialVersion ==
                          pending.expectedCredentialVersion) {
            hasAlternative_ = true;
            if (preferRenewal_) {
                output.credentialVersion = pending.credentialVersion;
                output.expiresAtUnixSeconds = pending.expiresAtUnixSeconds;
                memcpy(output.secret, pending.secret, sizeof(output.secret));
                memcpy(
                    output.accessMaterialDigest,
                    pending.accessMaterialDigest,
                    sizeof(output.accessMaterialDigest));
                selectedRenewal_ = true;
            }
        }
        clearGatewayCredentialRenewalRecord(pending);
        if (!result) clearGatewayAccessRecord(output);
        return result;
    }

    void authenticationFailed() {
        if (hasAlternative_) preferRenewal_ = !selectedRenewal_;
        selectedRenewal_ = false;
        hasAlternative_ = false;
    }

    void reset() {
        preferRenewal_ = true;
        selectedRenewal_ = false;
        hasAlternative_ = false;
    }

    Result activatePendingPresence(
        uint32_t expectedVersion,
        uint32_t& storageRevision) {
        return active_.activatePendingPresence(
            expectedVersion, storageRevision);
    }

    StorageProtection protection() const {
        if (active_.protection() == StorageProtection::PlainFlash ||
            (renewal_ != nullptr &&
             renewal_->protection() == StorageProtection::PlainFlash)) {
            return StorageProtection::PlainFlash;
        }
        return active_.protection();
    }

private:
    static bool same(ByteView first, ByteView second) {
        return constantTimeEqual(first, second);
    }

    static bool allZero(ByteView value) {
        if (value.data == nullptr) return false;
        uint8_t combined = 0U;
        for (size_t index = 0U; index < value.size; ++index) {
            combined = static_cast<uint8_t>(combined | value.data[index]);
        }
        return combined == 0U;
    }

    static Result validatePair(
        const GatewayAccessRecord& active,
        const GatewayCredentialRenewalRecord& pending) {
        if (pending.state !=
                GatewayCredentialRenewalState::MutationAttempted &&
            pending.state != GatewayCredentialRenewalState::ReceiptStored &&
            pending.state != GatewayCredentialRenewalState::Promoted) {
            return Result::failure(ErrorCode::StateConflict);
        }
        if (!same(ByteView(active.operationId, sizeof(active.operationId)),
                  ByteView(pending.topologyOperationId,
                           sizeof(pending.topologyOperationId))) ||
            !same(ByteView(active.childDeviceInstanceId,
                           sizeof(active.childDeviceInstanceId)),
                  ByteView(pending.childDeviceInstanceId,
                           sizeof(pending.childDeviceInstanceId))) ||
            active.accessEpoch != pending.accessEpoch ||
            !same(ByteView(active.controllerId,
                           sizeof(active.controllerId)),
                  ByteView(pending.controllerId,
                           sizeof(pending.controllerId))) ||
            active.activePresence.version != pending.presenceKeyVersion ||
            active.pendingPresence.version != 0U ||
            !allZero(ByteView(active.pendingPresence.key,
                              sizeof(active.pendingPresence.key)))) {
            return Result::failure(ErrorCode::SequenceConflict);
        }
        if (active.credentialVersion == pending.expectedCredentialVersion) {
            return Result::success();
        }
        if (active.credentialVersion != pending.credentialVersion ||
            active.expiresAtUnixSeconds != pending.expiresAtUnixSeconds ||
            !same(ByteView(active.secret, sizeof(active.secret)),
                  ByteView(pending.secret, sizeof(pending.secret))) ||
            !same(ByteView(active.accessMaterialDigest,
                           sizeof(active.accessMaterialDigest)),
                  ByteView(pending.accessMaterialDigest,
                           sizeof(pending.accessMaterialDigest)))) {
            return Result::failure(ErrorCode::SequenceConflict);
        }
        return Result::success();
    }

    GatewayAccessStore& active_;
    GatewayCredentialRenewalStore* renewal_;
    bool preferRenewal_;
    bool selectedRenewal_;
    bool hasAlternative_;

    GatewaySessionAccessSource(const GatewaySessionAccessSource&);
    GatewaySessionAccessSource& operator=(
        const GatewaySessionAccessSource&);
};

} // namespace blinker

#endif
