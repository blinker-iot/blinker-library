#ifndef BLINKER_RUNTIME_GATEWAYCREDENTIALRENEWALCOORDINATOR_H
#define BLINKER_RUNTIME_GATEWAYCREDENTIALRENEWALCOORDINATOR_H

#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "../identity/GatewayAccessStore.h"
#include "../identity/GatewayCredentialRenewalStore.h"
#include "../interface/IClock.h"
#include "../protocol/bbp2/Frame.h"
#include "../protocol/bbp2/Messages.h"
#include "../provisioning/ControllerControlContract.h"
#include "IGatewayManagementControl.h"

namespace blinker {

// Edge-Hub-only N->N+1 transaction coordinator. It reuses the existing
// authenticated child session and ControllerControl Rotate contract. Durable
// journal state, rather than an in-memory phase, defines crash recovery.
template <typename ControlChannel>
class BasicGatewayCredentialRenewalCoordinator final
    : public IGatewayManagementControl {
public:
    BasicGatewayCredentialRenewalCoordinator(
        ControlChannel& channel,
        GatewayAccessStore& active,
        GatewayCredentialRenewalStore& pending,
        IClock& clock)
        : channel_(channel), active_(active), pending_(pending),
          clock_(clock), command_(), result_(), resultSize_(0U),
          nextAttemptMillis_(0U), mutationAttemptId_(0U),
          activeCommand_(false), recoverMutationAttempted_(false),
          waitingResponse_(false), resultReady_(false),
          publishPending_(false), retireSession_(false) {}

    ~BasicGatewayCredentialRenewalCoordinator() override { reset(); }

    Result handleCommand(ByteView encoded) override {
        gateway::GatewayCredentialRenewalCommandView decoded;
        Result result = gateway::decodeGatewayCredentialRenewalCommand(
            encoded, decoded);
        if (!result) return result;
        if (activeCommand_) {
            if (sameCommand(decoded)) {
                if (resultReady_) publishPending_ = true;
                return Result::success();
            }
            if (!resultReady_) {
                return Result::failure(ErrorCode::StateConflict);
            }
            clearCommand();
        }

        copyCommand(decoded);
        activeCommand_ = true;
        nextAttemptMillis_ = clock_.monotonicMillis();

        GatewayCredentialRenewalRecord pending;
        result = pending_.load(pending);
        if (!result && result.code() == ErrorCode::NotFound &&
            command_.phase ==
                gateway::GatewayCredentialRenewalPhase::Cancel) {
            clearGatewayCredentialRenewalRecord(pending);
            complete(gateway::GatewayCredentialRenewalStatus::Cancelled);
            return Result::success();
        }
        if (!result && result.code() == ErrorCode::NotFound &&
            command_.phase ==
                gateway::GatewayCredentialRenewalPhase::Finalize) {
            result = pending_.finalize(
                renewalOperation(), topologyOperation(), materialDigest(),
                ByteView(command_.receiptDigest,
                         sizeof(command_.receiptDigest)),
                command_.activeStorageRevision);
            clearGatewayCredentialRenewalRecord(pending);
            if (!result) {
                clearCommand();
                return result;
            }
            complete(
                gateway::GatewayCredentialRenewalStatus::Finalized,
                ByteView(),
                ByteView(command_.receiptDigest,
                         sizeof(command_.receiptDigest)),
                command_.activeStorageRevision);
            return Result::success();
        }
        if (!result) {
            clearGatewayCredentialRenewalRecord(pending);
            clearCommand();
            return result;
        }
        if (!pendingMatches(pending)) {
            clearGatewayCredentialRenewalRecord(pending);
            complete(gateway::GatewayCredentialRenewalStatus::Rejected);
            return Result::success();
        }

        if (command_.phase ==
            gateway::GatewayCredentialRenewalPhase::Finalize) {
            result = pending_.finalize(
                renewalOperation(), topologyOperation(), materialDigest(),
                ByteView(command_.receiptDigest,
                         sizeof(command_.receiptDigest)),
                command_.activeStorageRevision);
            clearGatewayCredentialRenewalRecord(pending);
            if (!result) {
                clearCommand();
                return result;
            }
            complete(
                gateway::GatewayCredentialRenewalStatus::Finalized,
                ByteView(),
                ByteView(command_.receiptDigest,
                         sizeof(command_.receiptDigest)),
                command_.activeStorageRevision);
            return Result::success();
        }

        if (command_.phase ==
            gateway::GatewayCredentialRenewalPhase::Cancel) {
            result = pending_.cancel(
                renewalOperation(), topologyOperation(), materialDigest());
            clearGatewayCredentialRenewalRecord(pending);
            complete(result
                         ? gateway::GatewayCredentialRenewalStatus::Cancelled
                         : gateway::GatewayCredentialRenewalStatus::Rejected);
            return Result::success();
        }

        uint64_t now = 0U;
        if (!clock_.unixTime(now) || now == 0U) {
            clearGatewayCredentialRenewalRecord(pending);
            clearCommand();
            return Result::failure(ErrorCode::NotConfigured);
        }
        if (now >= command_.expiresAtUnixSeconds ||
            command_.expiresAtUnixSeconds > pending.expiresAtUnixSeconds) {
            const bool irreversible = pending.state !=
                GatewayCredentialRenewalState::Staged;
            clearGatewayCredentialRenewalRecord(pending);
            complete(irreversible &&
                             command_.phase ==
                                 gateway::GatewayCredentialRenewalPhase::Apply
                         ? gateway::GatewayCredentialRenewalStatus::
                               ForwardRecoveryRequired
                         : gateway::GatewayCredentialRenewalStatus::Expired);
            return Result::success();
        }

        if (command_.phase ==
            gateway::GatewayCredentialRenewalPhase::Prepare) {
            if (pending.state != GatewayCredentialRenewalState::Staged ||
                !activeMatches(pending)) {
                clearGatewayCredentialRenewalRecord(pending);
                complete(gateway::GatewayCredentialRenewalStatus::Rejected);
                return Result::success();
            }
            clearGatewayCredentialRenewalRecord(pending);
            return Result::success();
        }

        uint8_t grantDigest[kSha256Size] = {};
        if ((pending.state == GatewayCredentialRenewalState::Staged ||
             pending.state ==
                 GatewayCredentialRenewalState::MutationAttempted ||
             pending.state == GatewayCredentialRenewalState::Abandoned) &&
            !activeMatches(pending)) {
            const bool recoveryRequired = pending.state !=
                GatewayCredentialRenewalState::Staged;
            clearGatewayCredentialRenewalRecord(pending);
            complete(recoveryRequired
                         ? gateway::GatewayCredentialRenewalStatus::
                               ForwardRecoveryRequired
                         : gateway::GatewayCredentialRenewalStatus::Rejected);
            return Result::success();
        }
        result = sha256(
            ByteView(command_.grant, command_.grantSize),
            MutableByteSpan(grantDigest, sizeof(grantDigest)));
        if (!result || !validRotateGrant(pending)) {
            secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
            clearGatewayCredentialRenewalRecord(pending);
            complete(gateway::GatewayCredentialRenewalStatus::Rejected);
            return Result::success();
        }

        const GatewayCredentialRenewalState durableState = pending.state;
        if (durableState == GatewayCredentialRenewalState::Staged) {
            uint32_t revision = 0U;
            result = pending_.markMutationAttempted(
                renewalOperation(), topologyOperation(), materialDigest(),
                ByteView(grantDigest, sizeof(grantDigest)), revision);
        } else if (!constantTimeEqual(
                       ByteView(pending.grantDigest,
                                sizeof(pending.grantDigest)),
                       ByteView(grantDigest, sizeof(grantDigest)))) {
            result = Result::failure(ErrorCode::SequenceConflict);
        }
        secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
        clearGatewayCredentialRenewalRecord(pending);
        if (!result) {
            complete(gateway::GatewayCredentialRenewalStatus::Rejected);
            return Result::success();
        }
        if (durableState == GatewayCredentialRenewalState::Abandoned) {
            complete(gateway::GatewayCredentialRenewalStatus::Rejected);
            return Result::success();
        }
        recoverMutationAttempted_ = durableState ==
            GatewayCredentialRenewalState::MutationAttempted;
        return Result::success();
    }

    void handleControllerControlResponse(ByteView encoded) override {
        onResponse(encoded);
    }

    void poll() override {
        if (retireSession_) {
            channel_.retireControllerControlSession();
            retireSession_ = false;
        }
        if (!activeCommand_ || resultReady_) return;

        uint64_t now = 0U;
        if (!clock_.unixTime(now) || now == 0U ||
            now >= command_.expiresAtUnixSeconds) {
            channel_.cancelControllerControlRequest();
            waitingResponse_ = false;
            complete(command_.phase ==
                             gateway::GatewayCredentialRenewalPhase::Apply
                         ? gateway::GatewayCredentialRenewalStatus::
                               ForwardRecoveryRequired
                         : gateway::GatewayCredentialRenewalStatus::Expired);
            return;
        }

        if (command_.phase ==
                gateway::GatewayCredentialRenewalPhase::Apply &&
            resumeDurableApply()) {
            return;
        }
        if (command_.phase ==
                gateway::GatewayCredentialRenewalPhase::Apply &&
            recoverDurableMutationAttempt()) {
            return;
        }
        if (waitingResponse_) {
            if (channel_.controllerControlPending()) return;
            waitingResponse_ = false;
            nextAttemptMillis_ =
                clock_.monotonicMillis() + kRetryDelayMillis;
        }
        const uint32_t monotonic = clock_.monotonicMillis();
        if (static_cast<int32_t>(monotonic - nextAttemptMillis_) < 0) return;

        uint8_t bodyBytes[bbp2::kControllerMutationMaxEncodedSize] = {};
        ByteView body;
        Result result;
        bbp2::MessageKind kind;
        if (command_.phase ==
            gateway::GatewayCredentialRenewalPhase::Prepare) {
            kind = bbp2::MessageKind::ControllerControlOpen;
            result = bbp2::encodeControllerControlOpenBody(
                MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body);
        } else {
            GatewayCredentialRenewalRecord pending;
            result = pending_.load(pending);
            kind = bbp2::MessageKind::ControllerMutation;
            bbp2::ControllerMutationBody mutation;
            mutation.grant = ByteView(command_.grant, command_.grantSize);
            mutation.controllerSecret = ByteView(
                pending.secret, sizeof(pending.secret));
            if (result) {
                result = bbp2::encodeControllerMutationBody(
                    mutation,
                    MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body);
            }
            clearGatewayCredentialRenewalRecord(pending);
        }
        if (result) {
            result = channel_.sendControllerControlRequest(kind, body);
        }
        secureZero(MutableByteSpan(bodyBytes, sizeof(bodyBytes)));
        if (result) {
            if (command_.phase ==
                gateway::GatewayCredentialRenewalPhase::Apply) {
                mutationAttemptId_ =
                    channel_.authenticatedControllerSessionAttemptId();
            }
            waitingResponse_ = true;
            return;
        }
        if (result.code() == ErrorCode::InvalidArgument ||
            result.code() == ErrorCode::CapacityExceeded ||
            result.code() == ErrorCode::UnsupportedFeature) {
            complete(command_.phase ==
                             gateway::GatewayCredentialRenewalPhase::Apply
                         ? gateway::GatewayCredentialRenewalStatus::
                               ForwardRecoveryRequired
                         : gateway::GatewayCredentialRenewalStatus::Rejected);
            return;
        }
        nextAttemptMillis_ = monotonic + kRetryDelayMillis;
    }

    ByteView pendingResult() const override {
        return publishPending_ ? ByteView(result_, resultSize_) : ByteView();
    }

    void markResultPublished() override { publishPending_ = false; }

    void reset() override {
        channel_.cancelControllerControlRequest();
        clearCommand();
    }

private:
    enum : uint32_t { kRetryDelayMillis = 1000U };

    struct OwnedCommand {
        uint8_t renewalOperationId[gateway::kOperationIdSize];
        uint8_t topologyOperationId[gateway::kOperationIdSize];
        uint32_t topologyVersion;
        uint8_t childDeviceInstanceId[gateway::kDeviceInstanceIdSize];
        uint32_t accessEpoch;
        uint8_t controllerId[gateway::kControllerIdSize];
        uint32_t expectedCredentialVersion;
        uint32_t credentialVersion;
        uint32_t pendingStorageRevision;
        uint32_t activeStorageRevision;
        uint8_t accessMaterialDigest[gateway::kAccessMaterialDigestSize];
        gateway::GatewayCredentialRenewalPhase phase;
        uint16_t grantSize;
        uint8_t grant[gateway::kGatewayCredentialRenewalGrantMaximumSize];
        uint8_t receiptDigest[gateway::kAccessMaterialDigestSize];
        uint64_t expiresAtUnixSeconds;

        OwnedCommand()
            : renewalOperationId(), topologyOperationId(),
              topologyVersion(0U), childDeviceInstanceId(), accessEpoch(0U),
              controllerId(), expectedCredentialVersion(0U),
              credentialVersion(0U), pendingStorageRevision(0U),
              activeStorageRevision(0U), accessMaterialDigest(),
              phase(gateway::GatewayCredentialRenewalPhase::Prepare),
              grantSize(0U), grant(), receiptDigest(),
              expiresAtUnixSeconds(0U) {}
    };

    ByteView renewalOperation() const {
        return ByteView(command_.renewalOperationId,
                        sizeof(command_.renewalOperationId));
    }
    ByteView topologyOperation() const {
        return ByteView(command_.topologyOperationId,
                        sizeof(command_.topologyOperationId));
    }
    ByteView materialDigest() const {
        return ByteView(command_.accessMaterialDigest,
                        sizeof(command_.accessMaterialDigest));
    }

    static bool same(ByteView view, const uint8_t* bytes, size_t size) {
        return view.data != nullptr && view.size == size &&
               memcmp(view.data, bytes, size) == 0;
    }

    static bool same(ByteView first, ByteView second, size_t size) {
        return first.data != nullptr && second.data != nullptr &&
               first.size == size && second.size == size &&
               memcmp(first.data, second.data, size) == 0;
    }

    bool sameCommand(
        const gateway::GatewayCredentialRenewalCommandView& value) const {
        return same(value.renewalOperationId, command_.renewalOperationId,
                    sizeof(command_.renewalOperationId)) &&
               same(value.topologyOperationId, command_.topologyOperationId,
                    sizeof(command_.topologyOperationId)) &&
               value.topologyVersion == command_.topologyVersion &&
               same(value.childDeviceInstanceId,
                    command_.childDeviceInstanceId,
                    sizeof(command_.childDeviceInstanceId)) &&
               value.accessEpoch == command_.accessEpoch &&
               same(value.controllerId, command_.controllerId,
                    sizeof(command_.controllerId)) &&
               value.expectedCredentialVersion ==
                   command_.expectedCredentialVersion &&
               value.credentialVersion == command_.credentialVersion &&
               value.pendingStorageRevision ==
                   command_.pendingStorageRevision &&
               value.activeStorageRevision == command_.activeStorageRevision &&
               same(value.accessMaterialDigest,
                    command_.accessMaterialDigest,
                    sizeof(command_.accessMaterialDigest)) &&
               value.phase == command_.phase &&
               value.grant.size == command_.grantSize &&
               (value.grant.empty() ||
                same(value.grant, command_.grant, command_.grantSize)) &&
               value.receiptDigest.size ==
                   (command_.phase ==
                            gateway::GatewayCredentialRenewalPhase::Finalize
                        ? sizeof(command_.receiptDigest)
                        : 0U) &&
               (value.receiptDigest.empty() ||
                same(value.receiptDigest, command_.receiptDigest,
                     sizeof(command_.receiptDigest))) &&
               value.expiresAtUnixSeconds == command_.expiresAtUnixSeconds;
    }

    void copyCommand(
        const gateway::GatewayCredentialRenewalCommandView& value) {
        memcpy(command_.renewalOperationId, value.renewalOperationId.data,
               sizeof(command_.renewalOperationId));
        memcpy(command_.topologyOperationId, value.topologyOperationId.data,
               sizeof(command_.topologyOperationId));
        command_.topologyVersion = value.topologyVersion;
        memcpy(command_.childDeviceInstanceId,
               value.childDeviceInstanceId.data,
               sizeof(command_.childDeviceInstanceId));
        command_.accessEpoch = value.accessEpoch;
        memcpy(command_.controllerId, value.controllerId.data,
               sizeof(command_.controllerId));
        command_.expectedCredentialVersion =
            value.expectedCredentialVersion;
        command_.credentialVersion = value.credentialVersion;
        command_.pendingStorageRevision = value.pendingStorageRevision;
        command_.activeStorageRevision = value.activeStorageRevision;
        memcpy(command_.accessMaterialDigest,
               value.accessMaterialDigest.data,
               sizeof(command_.accessMaterialDigest));
        command_.phase = value.phase;
        command_.grantSize = static_cast<uint16_t>(value.grant.size);
        if (!value.grant.empty()) {
            memcpy(command_.grant, value.grant.data, value.grant.size);
        }
        if (!value.receiptDigest.empty()) {
            memcpy(command_.receiptDigest, value.receiptDigest.data,
                   value.receiptDigest.size);
        }
        command_.expiresAtUnixSeconds = value.expiresAtUnixSeconds;
    }

    bool pendingMatches(
        const GatewayCredentialRenewalRecord& pending) const {
        return memcmp(pending.renewalOperationId,
                      command_.renewalOperationId,
                      sizeof(pending.renewalOperationId)) == 0 &&
               memcmp(pending.topologyOperationId,
                      command_.topologyOperationId,
                      sizeof(pending.topologyOperationId)) == 0 &&
               memcmp(pending.childDeviceInstanceId,
                      command_.childDeviceInstanceId,
                      sizeof(pending.childDeviceInstanceId)) == 0 &&
               pending.accessEpoch == command_.accessEpoch &&
               memcmp(pending.controllerId, command_.controllerId,
                      sizeof(pending.controllerId)) == 0 &&
               pending.expectedCredentialVersion ==
                   command_.expectedCredentialVersion &&
               pending.credentialVersion == command_.credentialVersion &&
               pending.deliveryRevision == command_.pendingStorageRevision &&
               constantTimeEqual(
                   ByteView(pending.accessMaterialDigest,
                            sizeof(pending.accessMaterialDigest)),
                   materialDigest()) &&
               (command_.phase !=
                        gateway::GatewayCredentialRenewalPhase::Finalize ||
                pending.activeStorageRevision ==
                    command_.activeStorageRevision);
    }

    bool activeMatches(
        const GatewayCredentialRenewalRecord& pending) {
        GatewayAccessRecord active;
        const Result result = active_.load(active);
        const bool matches = result &&
            memcmp(active.operationId, pending.topologyOperationId,
                   sizeof(active.operationId)) == 0 &&
            memcmp(active.childDeviceInstanceId,
                   pending.childDeviceInstanceId,
                   sizeof(active.childDeviceInstanceId)) == 0 &&
            active.accessEpoch == pending.accessEpoch &&
            memcmp(active.controllerId, pending.controllerId,
                   sizeof(active.controllerId)) == 0 &&
            active.credentialVersion ==
                pending.expectedCredentialVersion &&
            active.deliveryRevision != 0U &&
            active.activePresence.version == pending.presenceKeyVersion &&
            active.pendingPresence.version == 0U;
        clearGatewayAccessRecord(active);
        return matches;
    }

    bool validRotateGrant(
        const GatewayCredentialRenewalRecord& pending) const {
        ControllerGrant grant;
        uint8_t secretDigest[kSha256Size] = {};
        Result result = decodeControllerGrant(
            ByteView(command_.grant, command_.grantSize), grant);
        if (result) {
            result = sha256(
                ByteView(pending.secret, sizeof(pending.secret)),
                MutableByteSpan(secretDigest, sizeof(secretDigest)));
        }
        const bool matches = result &&
            grant.operation == ControllerMutationOperation::Rotate &&
            same(grant.deviceInstanceId,
                 command_.childDeviceInstanceId,
                 sizeof(command_.childDeviceInstanceId)) &&
            grant.ownershipGeneration == command_.accessEpoch &&
            same(grant.controllerId, command_.controllerId,
                 sizeof(command_.controllerId)) &&
            grant.expectedCredentialVersion ==
                command_.expectedCredentialVersion &&
            grant.credentialVersion == command_.credentialVersion &&
            grant.permissions == gateway::kGatewayPermissions &&
            constantTimeEqual(grant.secretDigest,
                              ByteView(secretDigest,
                                       sizeof(secretDigest)));
        secureZero(MutableByteSpan(secretDigest, sizeof(secretDigest)));
        return matches;
    }

    bool receiptMatches(
        ByteView encoded,
        const GatewayCredentialRenewalRecord& pending) const {
        ControllerGrant grant;
        ControllerMutationReceipt receipt;
        Result result = decodeControllerGrant(
            ByteView(command_.grant, command_.grantSize), grant);
        if (result) result = decodeControllerMutationReceipt(encoded, receipt);
        if (!result || receipt.operation !=
                           ControllerMutationOperation::Rotate ||
            !same(receipt.grantId, grant.grantId,
                  kControllerGrantIdSize) ||
            !same(receipt.deviceInstanceId,
                  command_.childDeviceInstanceId,
                  sizeof(command_.childDeviceInstanceId)) ||
            receipt.ownershipGeneration != command_.accessEpoch ||
            !same(receipt.controllerId, command_.controllerId,
                  sizeof(command_.controllerId)) ||
            receipt.credentialVersion != command_.credentialVersion ||
            receipt.permissions != gateway::kGatewayPermissions ||
            !constantTimeEqual(receipt.secretDigest, grant.secretDigest) ||
            receipt.proofKind != ControllerReceiptProofKind::HmacSha256) {
            return false;
        }
        uint8_t workspace[kControllerControlWorkspaceSize] = {};
        result = verifyControllerMutationReceiptProof(
            ByteView(pending.secret, sizeof(pending.secret)), receipt,
            MutableByteSpan(workspace, sizeof(workspace)));
        secureZero(MutableByteSpan(workspace, sizeof(workspace)));
        return result.ok();
    }

    bool resumeDurableApply() {
        GatewayCredentialRenewalRecord pending;
        Result result = pending_.load(pending);
        if (!result || !pendingMatches(pending)) {
            clearGatewayCredentialRenewalRecord(pending);
            complete(gateway::GatewayCredentialRenewalStatus::
                         ForwardRecoveryRequired);
            return true;
        }
        if (pending.state != GatewayCredentialRenewalState::ReceiptStored &&
            pending.state != GatewayCredentialRenewalState::Promoted) {
            clearGatewayCredentialRenewalRecord(pending);
            return false;
        }

        uint8_t digest[kSha256Size] = {};
        result = GatewayCredentialRenewalStore::receiptDigest(
            pending, MutableByteSpan(digest, sizeof(digest)));
        uint32_t activeRevision = 0U;
        if (result) {
            result = active_.promoteCredentialRenewal(
                pending, activeRevision);
        }
        uint32_t journalRevision = 0U;
        if (result) {
            result = pending_.markPromoted(
                renewalOperation(), topologyOperation(), materialDigest(),
                ByteView(digest, sizeof(digest)), activeRevision,
                journalRevision);
        }
        if (result) {
            retireSession_ = true;
            complete(
                gateway::GatewayCredentialRenewalStatus::Rotated,
                ByteView(pending.receipt, pending.receiptSize),
                ByteView(digest, sizeof(digest)), activeRevision);
        }
        secureZero(MutableByteSpan(digest, sizeof(digest)));
        clearGatewayCredentialRenewalRecord(pending);
        if (!result) {
            nextAttemptMillis_ =
                clock_.monotonicMillis() + kRetryDelayMillis;
        }
        return result.ok();
    }

    bool recoverDurableMutationAttempt() {
        const uint32_t sessionAttempt =
            channel_.authenticatedControllerSessionAttemptId();
        const bool reauthenticated = mutationAttemptId_ != 0U &&
            sessionAttempt != 0U && sessionAttempt != mutationAttemptId_;
        if (!recoverMutationAttempted_ && !reauthenticated) return false;

        GatewayCredentialRenewalRecord pending;
        Result result = pending_.load(pending);
        if (!result || !pendingMatches(pending) ||
            pending.state !=
                GatewayCredentialRenewalState::MutationAttempted) {
            clearGatewayCredentialRenewalRecord(pending);
            complete(gateway::GatewayCredentialRenewalStatus::
                         ForwardRecoveryRequired);
            return true;
        }

        const uint32_t authenticatedVersion =
            channel_.authenticatedControllerCredentialVersion();
        if (authenticatedVersion == 0U) {
            result = channel_.ensureControllerControlSession();
            clearGatewayCredentialRenewalRecord(pending);
            if (!result && result.code() != ErrorCode::WouldBlock &&
                result.code() != ErrorCode::AlreadyExists) {
                nextAttemptMillis_ =
                    clock_.monotonicMillis() + kRetryDelayMillis;
            }
            return true;
        }
        if (authenticatedVersion == pending.expectedCredentialVersion) {
            uint32_t revision = 0U;
            result = pending_.markAbandoned(
                renewalOperation(), topologyOperation(), materialDigest(),
                ByteView(pending.grantDigest,
                         sizeof(pending.grantDigest)),
                revision);
            clearGatewayCredentialRenewalRecord(pending);
            if (result) {
                complete(gateway::GatewayCredentialRenewalStatus::Rejected);
            } else {
                nextAttemptMillis_ =
                    clock_.monotonicMillis() + kRetryDelayMillis;
            }
            return true;
        }
        if (authenticatedVersion != pending.credentialVersion) {
            clearGatewayCredentialRenewalRecord(pending);
            complete(gateway::GatewayCredentialRenewalStatus::
                         ForwardRecoveryRequired);
            return true;
        }

        ControllerGrant grant;
        result = decodeControllerGrant(
            ByteView(command_.grant, command_.grantSize), grant);
        uint8_t workspace[kControllerControlWorkspaceSize] = {};
        ByteView receipt;
        if (result) {
            result = buildControllerMutationReceipt(
                grant,
                ByteView(pending.secret, sizeof(pending.secret)),
                MutableByteSpan(workspace, sizeof(workspace)),
                MutableByteSpan(result_, sizeof(result_)),
                receipt);
        }
        uint32_t revision = 0U;
        if (result) {
            result = pending_.storeReceipt(
                renewalOperation(), topologyOperation(), materialDigest(),
                ByteView(pending.grantDigest,
                         sizeof(pending.grantDigest)),
                receipt, revision);
        }
        secureZero(MutableByteSpan(workspace, sizeof(workspace)));
        clearGatewayCredentialRenewalRecord(pending);
        if (!result) {
            secureZero(MutableByteSpan(result_, sizeof(result_)));
            nextAttemptMillis_ =
                clock_.monotonicMillis() + kRetryDelayMillis;
            return true;
        }
        (void)resumeDurableApply();
        return true;
    }

    void onResponse(ByteView encodedFrame) {
        waitingResponse_ = false;
        if (!activeCommand_ || resultReady_) return;
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(encodedFrame, frame);
        if (!result || frame.header.kind ==
                           static_cast<uint8_t>(bbp2::MessageKind::Error)) {
            complete(command_.phase ==
                             gateway::GatewayCredentialRenewalPhase::Apply
                         ? gateway::GatewayCredentialRenewalStatus::
                               ForwardRecoveryRequired
                         : gateway::GatewayCredentialRenewalStatus::Rejected);
            return;
        }
        if (command_.phase ==
            gateway::GatewayCredentialRenewalPhase::Prepare) {
            bbp2::ControllerControlChallengeBody challenge;
            result = bbp2::decodeControllerControlChallengeBody(
                frame.body, challenge);
            if (!result) {
                complete(gateway::GatewayCredentialRenewalStatus::Rejected);
                return;
            }
            complete(gateway::GatewayCredentialRenewalStatus::NonceReady,
                     challenge.controlNonce);
            return;
        }
        if (frame.header.kind != static_cast<uint8_t>(
                                     bbp2::MessageKind::
                                         ControllerMutationReceipt)) {
            complete(gateway::GatewayCredentialRenewalStatus::
                         ForwardRecoveryRequired);
            return;
        }

        GatewayCredentialRenewalRecord pending;
        result = pending_.load(pending);
        if (!result || !pendingMatches(pending) ||
            !receiptMatches(frame.body, pending)) {
            clearGatewayCredentialRenewalRecord(pending);
            complete(gateway::GatewayCredentialRenewalStatus::
                         ForwardRecoveryRequired);
            return;
        }
        uint32_t revision = 0U;
        result = pending_.storeReceipt(
            renewalOperation(), topologyOperation(), materialDigest(),
            ByteView(pending.grantDigest, sizeof(pending.grantDigest)),
            frame.body, revision);
        clearGatewayCredentialRenewalRecord(pending);
        if (!result || !resumeDurableApply()) {
            nextAttemptMillis_ =
                clock_.monotonicMillis() + kRetryDelayMillis;
        }
    }

    void complete(
        gateway::GatewayCredentialRenewalStatus status,
        ByteView payload = ByteView(),
        ByteView receiptDigest = ByteView(),
        uint32_t activeStorageRevision = 0U) {
        gateway::GatewayCredentialRenewalResultView value;
        value.renewalOperationId = renewalOperation();
        value.topologyOperationId = topologyOperation();
        value.topologyVersion = command_.topologyVersion;
        value.phase = command_.phase;
        value.status = status;
        value.pendingStorageRevision = command_.pendingStorageRevision;
        value.activeStorageRevision = activeStorageRevision;
        value.credentialVersion = command_.credentialVersion;
        value.accessMaterialDigest = materialDigest();
        value.payload = payload;
        value.receiptDigest = receiptDigest;
        size_t written = 0U;
        const Result result = gateway::encodeGatewayCredentialRenewalResult(
            value, MutableByteSpan(result_, sizeof(result_)), written);
        if (!result) {
            clearCommand();
            return;
        }
        resultSize_ = written;
        resultReady_ = true;
        publishPending_ = true;
    }

    void clearCommand() {
        if (waitingResponse_ || channel_.controllerControlPending()) {
            channel_.cancelControllerControlRequest();
        }
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(&command_), sizeof(command_)));
        secureZero(MutableByteSpan(result_, sizeof(result_)));
        command_ = OwnedCommand();
        resultSize_ = 0U;
        nextAttemptMillis_ = 0U;
        mutationAttemptId_ = 0U;
        activeCommand_ = false;
        recoverMutationAttempted_ = false;
        waitingResponse_ = false;
        resultReady_ = false;
        publishPending_ = false;
        retireSession_ = false;
    }

    ControlChannel& channel_;
    GatewayAccessStore& active_;
    GatewayCredentialRenewalStore& pending_;
    IClock& clock_;
    OwnedCommand command_;
    uint8_t result_[
        gateway::kGatewayCredentialRenewalResultMaximumEncodedSize];
    size_t resultSize_;
    uint32_t nextAttemptMillis_;
    uint32_t mutationAttemptId_;
    bool activeCommand_;
    bool recoverMutationAttempted_;
    bool waitingResponse_;
    bool resultReady_;
    bool publishPending_;
    bool retireSession_;

    BasicGatewayCredentialRenewalCoordinator(
        const BasicGatewayCredentialRenewalCoordinator&);
    BasicGatewayCredentialRenewalCoordinator& operator=(
        const BasicGatewayCredentialRenewalCoordinator&);
};

} // namespace blinker

#endif
