#ifndef BLINKER_RUNTIME_GATEWAYREVOCATIONCOORDINATOR_H
#define BLINKER_RUNTIME_GATEWAYREVOCATIONCOORDINATOR_H

#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "../identity/GatewayAccessStore.h"
#include "../interface/IClock.h"
#include "../protocol/bbp2/Frame.h"
#include "../protocol/bbp2/Messages.h"
#include "../provisioning/ControllerControlContract.h"
#include "IGatewayManagementControl.h"

namespace blinker {

// Executes the device half of one Revoking topology. The ordinary route never
// carries these frames: the authenticated management command is translated to
// the route bridge's private ControllerControl side channel.
template <typename ControlChannel>
class BasicGatewayRevocationCoordinator final
    : public IGatewayManagementControl {
public:
    BasicGatewayRevocationCoordinator(
        ControlChannel& channel,
        GatewayAccessStore& access,
        IClock& clock)
        : channel_(channel), access_(access), clock_(clock), command_(),
          result_(), resultSize_(0U), nextAttemptMillis_(0U),
          active_(false), waitingResponse_(false), resultReady_(false),
          publishPending_(false) {}

    ~BasicGatewayRevocationCoordinator() override { reset(); }

    Result handleCommand(ByteView encoded) override {
        gateway::GatewayRevocationCommandView decoded;
        Result result = gateway::decodeGatewayRevocationCommand(
            encoded, decoded);
        if (!result) return result;
        if (active_) {
            if (sameCommand(decoded)) {
                if (resultReady_) publishPending_ = true;
                return Result::success();
            }
            const bool recoveryFinalize =
                decoded.phase ==
                    gateway::GatewayRevocationPhase::Finalize &&
                decoded.topologyVersion > command_.topologyVersion;
            if (!resultReady_ && !recoveryFinalize) {
                return Result::failure(ErrorCode::StateConflict);
            }
            clearCommand();
        }

        copyCommand(decoded);
        active_ = true;
        nextAttemptMillis_ = clock_.monotonicMillis();
        if (command_.phase == gateway::GatewayRevocationPhase::Finalize) {
            result = access_.finalizeRevocation(
                ByteView(command_.operationId, sizeof(command_.operationId)),
                command_.topologyVersion,
                ByteView(
                    command_.receiptDigest,
                    sizeof(command_.receiptDigest)),
                ByteView(
                    command_.childDeviceInstanceId,
                    sizeof(command_.childDeviceInstanceId)),
                command_.accessEpoch,
                ByteView(
                    command_.controllerId,
                    sizeof(command_.controllerId)),
                command_.gatewayCredentialVersion,
                command_.accessStorageRevision);
            if (!result) {
                clearCommand();
                return result;
            }
            complete(
                gateway::GatewayRevocationStatus::Finalized,
                ByteView(),
                ByteView(
                    command_.receiptDigest,
                    sizeof(command_.receiptDigest)));
            return Result::success();
        }

        uint64_t now = 0U;
        if (!clock_.unixTime(now) || now == 0U) {
            clearCommand();
            return Result::failure(ErrorCode::NotConfigured);
        }
        if (now >= command_.expiresAtUnixSeconds) {
            complete(gateway::GatewayRevocationStatus::Expired);
            return Result::success();
        }

        uint8_t grantDigest[kSha256Size] = {};
        if (command_.phase == gateway::GatewayRevocationPhase::Apply) {
            result = sha256(
                ByteView(command_.grant, command_.grantSize),
                MutableByteSpan(grantDigest, sizeof(grantDigest)));
            if (!result || !validRevokeGrant()) {
                secureZero(MutableByteSpan(
                    grantDigest, sizeof(grantDigest)));
                complete(gateway::GatewayRevocationStatus::Rejected);
                return Result::success();
            }
        }

        GatewayRevocationTombstone tombstone;
        const Result tombstoneResult = access_.loadRevocation(tombstone);
        if (tombstoneResult) {
            const bool replay =
                command_.phase == gateway::GatewayRevocationPhase::Apply &&
                tombstoneMatches(tombstone, grantDigest);
            if (replay) {
                complete(
                    gateway::GatewayRevocationStatus::Revoked,
                    ByteView(tombstone.receipt, tombstone.receiptSize),
                    ByteView(
                        tombstone.receiptDigest,
                        sizeof(tombstone.receiptDigest)));
            } else {
                complete(gateway::GatewayRevocationStatus::Rejected);
            }
            clearGatewayRevocationTombstone(tombstone);
            secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
            return Result::success();
        }
        clearGatewayRevocationTombstone(tombstone);
        if (tombstoneResult.code() != ErrorCode::NotFound) {
            secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
            clearCommand();
            return tombstoneResult;
        }

        GatewayAccessRecord record;
        result = access_.load(record);
        const bool matches = result && recordMatches(record) &&
            now < record.expiresAtUnixSeconds;
        clearGatewayAccessRecord(record);
        secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
        if (!matches) complete(gateway::GatewayRevocationStatus::Rejected);
        return Result::success();
    }

    void handleControllerControlResponse(ByteView encoded) override {
        onResponse(encoded);
    }

    void poll() override {
        if (!active_ || resultReady_) return;
        uint64_t now = 0U;
        if (!clock_.unixTime(now) || now == 0U ||
            now >= command_.expiresAtUnixSeconds) {
            channel_.cancelControllerControlRequest();
            waitingResponse_ = false;
            complete(gateway::GatewayRevocationStatus::Expired);
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
        if (command_.phase == gateway::GatewayRevocationPhase::Prepare) {
            kind = bbp2::MessageKind::ControllerControlOpen;
            result = bbp2::encodeControllerControlOpenBody(
                MutableByteSpan(bodyBytes, sizeof(bodyBytes)), body);
        } else {
            kind = bbp2::MessageKind::ControllerMutation;
            bbp2::ControllerMutationBody mutation;
            mutation.grant = ByteView(command_.grant, command_.grantSize);
            mutation.controllerSecret = ByteView();
            result = bbp2::encodeControllerMutationBody(
                mutation,
                MutableByteSpan(bodyBytes, sizeof(bodyBytes)),
                body);
        }
        if (result) {
            result = channel_.sendControllerControlRequest(kind, body);
        }
        secureZero(MutableByteSpan(bodyBytes, sizeof(bodyBytes)));
        if (result) {
            waitingResponse_ = true;
            return;
        }
        if (result.code() == ErrorCode::InvalidArgument ||
            result.code() == ErrorCode::CapacityExceeded ||
            result.code() == ErrorCode::UnsupportedFeature) {
            complete(gateway::GatewayRevocationStatus::Rejected);
            return;
        }
        nextAttemptMillis_ = monotonic + kRetryDelayMillis;
    }

    ByteView pendingResult() const override {
        return publishPending_ ? ByteView(result_, resultSize_) : ByteView();
    }

    void markResultPublished() override { publishPending_ = false; }

    bool active() const { return active_; }
    bool waitingResponse() const { return waitingResponse_; }
    bool resultReady() const { return resultReady_; }

    void reset() override {
        channel_.cancelControllerControlRequest();
        clearCommand();
    }

private:
    enum : uint32_t { kRetryDelayMillis = 1000U };

    struct OwnedCommand {
        uint8_t operationId[gateway::kOperationIdSize];
        uint32_t topologyVersion;
        uint8_t childDeviceInstanceId[gateway::kDeviceInstanceIdSize];
        uint32_t accessEpoch;
        uint8_t controllerId[gateway::kControllerIdSize];
        uint32_t gatewayCredentialVersion;
        uint32_t accessStorageRevision;
        gateway::GatewayRevocationPhase phase;
        uint16_t grantSize;
        uint8_t grant[gateway::kGatewayRevocationGrantMaximumSize];
        uint8_t receiptDigest[gateway::kAccessMaterialDigestSize];
        uint64_t expiresAtUnixSeconds;

        OwnedCommand()
            : operationId(), topologyVersion(0U), childDeviceInstanceId(),
              accessEpoch(0U), controllerId(), gatewayCredentialVersion(0U),
              accessStorageRevision(0U),
              phase(gateway::GatewayRevocationPhase::Prepare),
              grantSize(0U), grant(), receiptDigest(),
              expiresAtUnixSeconds(0U) {}
    };

    static bool same(
        ByteView value,
        const uint8_t* bytes,
        size_t size) {
        return value.size == size &&
               (size == 0U ||
                (value.data != nullptr && memcmp(value.data, bytes, size) == 0));
    }

    static bool same(ByteView first, ByteView second, size_t size) {
        return first.size == size && second.size == size &&
               first.data != nullptr && second.data != nullptr &&
               memcmp(first.data, second.data, size) == 0;
    }

    bool sameCommand(
        const gateway::GatewayRevocationCommandView& value) const {
        return same(value.operationId, command_.operationId,
                    sizeof(command_.operationId)) &&
               value.topologyVersion == command_.topologyVersion &&
               same(value.childDeviceInstanceId,
                    command_.childDeviceInstanceId,
                    sizeof(command_.childDeviceInstanceId)) &&
               value.accessEpoch == command_.accessEpoch &&
               same(value.controllerId, command_.controllerId,
                    sizeof(command_.controllerId)) &&
               value.gatewayCredentialVersion ==
                   command_.gatewayCredentialVersion &&
               value.accessStorageRevision ==
                   command_.accessStorageRevision &&
               value.phase == command_.phase &&
               same(value.grant, command_.grant, command_.grantSize) &&
               same(value.receiptDigest,
                    command_.receiptDigest,
                    value.receiptDigest.size) &&
               value.receiptDigest.size ==
                   (command_.phase == gateway::GatewayRevocationPhase::Finalize
                        ? sizeof(command_.receiptDigest)
                        : 0U) &&
               value.expiresAtUnixSeconds ==
                   command_.expiresAtUnixSeconds;
    }

    void copyCommand(
        const gateway::GatewayRevocationCommandView& value) {
        memcpy(command_.operationId, value.operationId.data,
               sizeof(command_.operationId));
        command_.topologyVersion = value.topologyVersion;
        memcpy(command_.childDeviceInstanceId,
               value.childDeviceInstanceId.data,
               sizeof(command_.childDeviceInstanceId));
        command_.accessEpoch = value.accessEpoch;
        memcpy(command_.controllerId, value.controllerId.data,
               sizeof(command_.controllerId));
        command_.gatewayCredentialVersion =
            value.gatewayCredentialVersion;
        command_.accessStorageRevision = value.accessStorageRevision;
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

    bool recordMatches(const GatewayAccessRecord& record) const {
        return memcmp(record.operationId, command_.operationId,
                      sizeof(record.operationId)) == 0 &&
               memcmp(record.childDeviceInstanceId,
                      command_.childDeviceInstanceId,
                      sizeof(record.childDeviceInstanceId)) == 0 &&
               record.accessEpoch == command_.accessEpoch &&
               memcmp(record.controllerId, command_.controllerId,
                      sizeof(record.controllerId)) == 0 &&
               record.credentialVersion ==
                   command_.gatewayCredentialVersion &&
               record.deliveryRevision == command_.accessStorageRevision;
    }

    bool validRevokeGrant() const {
        ControllerGrant grant;
        const Result result = decodeControllerGrant(
            ByteView(command_.grant, command_.grantSize), grant);
        return result &&
               grant.operation == ControllerMutationOperation::Revoke &&
               same(grant.deviceInstanceId,
                    command_.childDeviceInstanceId,
                    sizeof(command_.childDeviceInstanceId)) &&
               grant.ownershipGeneration == command_.accessEpoch &&
               same(grant.controllerId, command_.controllerId,
                    sizeof(command_.controllerId)) &&
               grant.expectedCredentialVersion ==
                   command_.gatewayCredentialVersion &&
               grant.credentialVersion ==
                   command_.gatewayCredentialVersion;
    }

    bool tombstoneMatches(
        const GatewayRevocationTombstone& tombstone,
        const uint8_t* grantDigest) const {
        return memcmp(tombstone.operationId, command_.operationId,
                      sizeof(tombstone.operationId)) == 0 &&
               tombstone.topologyVersion == command_.topologyVersion &&
               memcmp(tombstone.grantDigest, grantDigest,
                      sizeof(tombstone.grantDigest)) == 0;
    }

    bool receiptMatchesGrant(ByteView encodedReceipt) const {
        ControllerGrant grant;
        ControllerMutationReceipt receipt;
        const Result grantResult = decodeControllerGrant(
            ByteView(command_.grant, command_.grantSize), grant);
        const Result receiptResult = decodeControllerMutationReceipt(
            encodedReceipt, receipt);
        return grantResult && receiptResult &&
               receipt.operation == ControllerMutationOperation::Revoke &&
               same(receipt.grantId, grant.grantId, kControllerGrantIdSize) &&
               same(receipt.deviceInstanceId, grant.deviceInstanceId,
                    kDeviceInstanceIdSize) &&
               receipt.ownershipGeneration == grant.ownershipGeneration &&
               same(receipt.controllerId, grant.controllerId,
                    kControllerIdSize) &&
               receipt.credentialVersion == grant.credentialVersion;
    }

    void onResponse(ByteView encodedFrame) {
        waitingResponse_ = false;
        if (!active_ || resultReady_) return;
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(encodedFrame, frame);
        if (!result || frame.header.kind == static_cast<uint8_t>(
                                             bbp2::MessageKind::Error)) {
            complete(gateway::GatewayRevocationStatus::Rejected);
            return;
        }
        if (command_.phase == gateway::GatewayRevocationPhase::Prepare) {
            bbp2::ControllerControlChallengeBody challenge;
            result = bbp2::decodeControllerControlChallengeBody(
                frame.body, challenge);
            if (!result) {
                complete(gateway::GatewayRevocationStatus::Rejected);
                return;
            }
            complete(
                gateway::GatewayRevocationStatus::NonceReady,
                challenge.controlNonce);
            return;
        }
        if (frame.header.kind != static_cast<uint8_t>(
                                     bbp2::MessageKind::
                                         ControllerMutationReceipt) ||
            !receiptMatchesGrant(frame.body)) {
            complete(gateway::GatewayRevocationStatus::Rejected);
            return;
        }

        GatewayRevocationTombstone tombstone;
        memcpy(tombstone.operationId, command_.operationId,
               sizeof(tombstone.operationId));
        tombstone.topologyVersion = command_.topologyVersion;
        result = sha256(
            ByteView(command_.grant, command_.grantSize),
            MutableByteSpan(
                tombstone.grantDigest,
                sizeof(tombstone.grantDigest)));
        if (result) {
            result = sha256(
                frame.body,
                MutableByteSpan(
                    tombstone.receiptDigest,
                    sizeof(tombstone.receiptDigest)));
        }
        if (result) {
            tombstone.receiptSize =
                static_cast<uint16_t>(frame.body.size);
            memcpy(tombstone.receipt, frame.body.data, frame.body.size);
            uint32_t storageRevision = 0U;
            result = access_.commitRevocation(
                command_.accessStorageRevision,
                tombstone,
                storageRevision);
        }
        if (result) {
            complete(
                gateway::GatewayRevocationStatus::Revoked,
                ByteView(tombstone.receipt, tombstone.receiptSize),
                ByteView(
                    tombstone.receiptDigest,
                    sizeof(tombstone.receiptDigest)));
        } else {
            // The child mutation is idempotent. While this DirectSecure
            // session survives, resend the same signed Grant and let its
            // durable store replay the exact receipt before reporting cloud
            // completion.
            nextAttemptMillis_ =
                clock_.monotonicMillis() + kRetryDelayMillis;
        }
        clearGatewayRevocationTombstone(tombstone);
    }

    void complete(
        gateway::GatewayRevocationStatus status,
        ByteView payload = ByteView(),
        ByteView receiptDigest = ByteView()) {
        gateway::GatewayRevocationResultView result;
        result.operationId = ByteView(
            command_.operationId, sizeof(command_.operationId));
        result.topologyVersion = command_.topologyVersion;
        result.phase = command_.phase;
        result.status = status;
        result.payload = payload;
        result.receiptDigest = receiptDigest;
        size_t written = 0U;
        const Result encoded = gateway::encodeGatewayRevocationResult(
            result,
            MutableByteSpan(result_, sizeof(result_)),
            written);
        if (!encoded) {
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
        active_ = false;
        waitingResponse_ = false;
        resultReady_ = false;
        publishPending_ = false;
    }

    ControlChannel& channel_;
    GatewayAccessStore& access_;
    IClock& clock_;
    OwnedCommand command_;
    uint8_t result_[gateway::kGatewayRevocationResultMaximumEncodedSize];
    size_t resultSize_;
    uint32_t nextAttemptMillis_;
    bool active_;
    bool waitingResponse_;
    bool resultReady_;
    bool publishPending_;

    BasicGatewayRevocationCoordinator(
        const BasicGatewayRevocationCoordinator&);
    BasicGatewayRevocationCoordinator& operator=(
        const BasicGatewayRevocationCoordinator&);
};

} // namespace blinker

#endif
