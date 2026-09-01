#ifndef BLINKER_RUNTIME_GATEWAYPROOFCOORDINATOR_H
#define BLINKER_RUNTIME_GATEWAYPROOFCOORDINATOR_H

#include <string.h>

#include "../core/SecureMemory.h"
#include "../identity/GatewayAccessStore.h"
#include "../interface/IClock.h"
#include "GatewayChildSession.h"
#include "IGatewayManagementControl.h"

namespace blinker {

// Executes one exact, Service-authorized proof task. The authenticated MQTTS
// Hub identity is the attestation root; this coordinator only reports Secure
// after the exact durable access record reaches DirectSecure with the child.
// Transient child-link failures retry with a bounded cadence until expiry.
template <typename ChildSession>
class BasicGatewayProofCoordinator final : public IGatewayManagementControl {
public:
    BasicGatewayProofCoordinator(
        ChildSession& child,
        GatewayAccessStore& access,
        IClock& clock)
        : child_(child), access_(access), clock_(clock), command_(),
          result_(), resultSize_(0U), nextAttemptMillis_(0U),
          previousState_(GatewayChildSessionState::Stopped), active_(false),
          resultReady_(false), publishPending_(false) {}

    ~BasicGatewayProofCoordinator() override { reset(); }

    Result handleCommand(ByteView encoded) override {
        gateway::GatewayProofCommandView decoded;
        Result result = gateway::decodeGatewayProofCommand(encoded, decoded);
        if (!result) return result;
        if (active_) {
            if (sameCommand(decoded)) {
                if (resultReady_) publishPending_ = true;
                return Result::success();
            }
            if (!resultReady_) {
                return Result::failure(ErrorCode::StateConflict);
            }
            // A different authenticated command can only supersede a task
            // after this task reached a terminal result. Until then, changing
            // operation/topology/nonce would create two concurrent proofs.
            // The new command is also the only acknowledgement needed to
            // retire the in-RAM replay cache for the old terminal result.
            retireChild();
            clearCommand();
        }

        copyCommand(decoded);
        active_ = true;
        previousState_ = child_.state();
        nextAttemptMillis_ = clock_.monotonicMillis();

        uint64_t now = 0U;
        if (!clock_.unixTime(now) || now == 0U) {
            clearCommand();
            return Result::failure(ErrorCode::NotConfigured);
        }
        if (now >= command_.expiresAtUnixSeconds) {
            complete(gateway::GatewayProofStatus::Expired);
            return Result::success();
        }
        GatewayAccessRecord record;
        result = access_.load(record);
        const bool matches = result && recordMatches(record) &&
            command_.expiresAtUnixSeconds <= record.expiresAtUnixSeconds;
        clearGatewayAccessRecord(record);
        if (!matches) complete(gateway::GatewayProofStatus::Rejected);
        return Result::success();
    }

    void poll() override {
        if (!active_ || resultReady_) return;
        uint64_t now = 0U;
        if (!clock_.unixTime(now) || now == 0U ||
            now >= command_.expiresAtUnixSeconds) {
            retireChild();
            complete(gateway::GatewayProofStatus::Expired);
            return;
        }

        const GatewayChildSessionState state = child_.state();
        if (child_.secure()) {
            if (child_.matchedPresenceVersion() ==
                command_.presenceKeyVersion) {
                complete(gateway::GatewayProofStatus::Secure);
            } else {
                retireChild();
                complete(gateway::GatewayProofStatus::Rejected);
            }
            previousState_ = state;
            return;
        }

        if (state == GatewayChildSessionState::Fault) {
            complete(gateway::GatewayProofStatus::Rejected);
            previousState_ = state;
            return;
        }
        if (state != GatewayChildSessionState::Idle) {
            previousState_ = state;
            return;
        }

        const uint32_t monotonic = clock_.monotonicMillis();
        if (previousState_ != GatewayChildSessionState::Idle) {
            nextAttemptMillis_ = monotonic + kRetryDelayMillis;
        }
        previousState_ = state;
        if (static_cast<int32_t>(monotonic - nextAttemptMillis_) < 0) return;

        GatewayAccessRecord record;
        Result result = access_.load(record);
        const bool matches = result && recordMatches(record) &&
            now < record.expiresAtUnixSeconds;
        clearGatewayAccessRecord(record);
        if (!matches) {
            complete(gateway::GatewayProofStatus::Rejected);
            return;
        }

        result = child_.connect();
        if (result) {
            previousState_ = child_.state();
            return;
        }
        if (result.code() == ErrorCode::AuthenticationRequired ||
            result.code() == ErrorCode::NotConfigured ||
            result.code() == ErrorCode::StateConflict) {
            complete(gateway::GatewayProofStatus::Rejected);
            return;
        }
        nextAttemptMillis_ = monotonic + kRetryDelayMillis;
    }

    ByteView pendingResult() const override {
        return publishPending_
                   ? ByteView(result_, resultSize_)
                   : ByteView();
    }

    void markResultPublished() override { publishPending_ = false; }

    void reset() override {
        retireChild();
        clearCommand();
    }

private:
    enum : uint32_t { kRetryDelayMillis = 1000U };

    struct OwnedCommand {
        uint8_t operationId[gateway::kOperationIdSize];
        uint32_t topologyVersion;
        uint8_t childLogicalDeviceIdSize;
        char childLogicalDeviceId[gateway::kMaximumLogicalDeviceIdSize];
        uint8_t childDeviceInstanceId[gateway::kDeviceInstanceIdSize];
        uint32_t accessEpoch;
        uint32_t gatewayCredentialVersion;
        uint32_t presenceKeyVersion;
        uint8_t accessMaterialDigest[gateway::kAccessMaterialDigestSize];
        uint32_t accessStorageRevision;
        uint8_t proofNonce[gateway::kProofNonceSize];
        uint64_t expiresAtUnixSeconds;

        OwnedCommand()
            : operationId(), topologyVersion(0U),
              childLogicalDeviceIdSize(0U), childLogicalDeviceId(),
              childDeviceInstanceId(), accessEpoch(0U),
              gatewayCredentialVersion(0U), presenceKeyVersion(0U),
              accessMaterialDigest(), accessStorageRevision(0U), proofNonce(),
              expiresAtUnixSeconds(0U) {}
    };

    static bool bytesEqual(ByteView view, const uint8_t* bytes, size_t size) {
        return view.data != nullptr && view.size == size &&
               memcmp(view.data, bytes, size) == 0;
    }

    bool sameCommand(const gateway::GatewayProofCommandView& value) const {
        return bytesEqual(
                   value.operationId, command_.operationId,
                   sizeof(command_.operationId)) &&
               value.topologyVersion == command_.topologyVersion &&
               value.childLogicalDeviceId.size ==
                   command_.childLogicalDeviceIdSize &&
               memcmp(
                   value.childLogicalDeviceId.data,
                   command_.childLogicalDeviceId,
                   command_.childLogicalDeviceIdSize) == 0 &&
               bytesEqual(
                   value.childDeviceInstanceId,
                   command_.childDeviceInstanceId,
                   sizeof(command_.childDeviceInstanceId)) &&
               value.accessEpoch == command_.accessEpoch &&
               value.gatewayCredentialVersion ==
                   command_.gatewayCredentialVersion &&
               value.presenceKeyVersion == command_.presenceKeyVersion &&
               bytesEqual(
                   value.accessMaterialDigest,
                   command_.accessMaterialDigest,
                   sizeof(command_.accessMaterialDigest)) &&
               value.accessStorageRevision ==
                   command_.accessStorageRevision &&
               bytesEqual(
                   value.proofNonce, command_.proofNonce,
                   sizeof(command_.proofNonce)) &&
               value.expiresAtUnixSeconds ==
                   command_.expiresAtUnixSeconds;
    }

    void copyCommand(const gateway::GatewayProofCommandView& value) {
        memcpy(command_.operationId, value.operationId.data,
               sizeof(command_.operationId));
        command_.topologyVersion = value.topologyVersion;
        command_.childLogicalDeviceIdSize =
            static_cast<uint8_t>(value.childLogicalDeviceId.size);
        memcpy(command_.childLogicalDeviceId,
               value.childLogicalDeviceId.data,
               value.childLogicalDeviceId.size);
        memcpy(command_.childDeviceInstanceId,
               value.childDeviceInstanceId.data,
               sizeof(command_.childDeviceInstanceId));
        command_.accessEpoch = value.accessEpoch;
        command_.gatewayCredentialVersion =
            value.gatewayCredentialVersion;
        command_.presenceKeyVersion = value.presenceKeyVersion;
        memcpy(command_.accessMaterialDigest,
               value.accessMaterialDigest.data,
               sizeof(command_.accessMaterialDigest));
        command_.accessStorageRevision = value.accessStorageRevision;
        memcpy(command_.proofNonce, value.proofNonce.data,
               sizeof(command_.proofNonce));
        command_.expiresAtUnixSeconds = value.expiresAtUnixSeconds;
    }

    bool recordMatches(const GatewayAccessRecord& record) const {
        const bool presenceMatches =
            record.activePresence.version == command_.presenceKeyVersion ||
            record.pendingPresence.version == command_.presenceKeyVersion;
        return record.deliveryRevision == command_.accessStorageRevision &&
               record.childLogicalDeviceIdSize ==
                   command_.childLogicalDeviceIdSize &&
               memcmp(record.operationId, command_.operationId,
                      sizeof(command_.operationId)) == 0 &&
               memcmp(record.childLogicalDeviceId,
                      command_.childLogicalDeviceId,
                      command_.childLogicalDeviceIdSize) == 0 &&
               memcmp(record.childDeviceInstanceId,
                      command_.childDeviceInstanceId,
                      sizeof(command_.childDeviceInstanceId)) == 0 &&
               record.accessEpoch == command_.accessEpoch &&
               record.credentialVersion ==
                   command_.gatewayCredentialVersion &&
               memcmp(record.accessMaterialDigest,
                      command_.accessMaterialDigest,
                      sizeof(command_.accessMaterialDigest)) == 0 &&
               presenceMatches;
    }

    void complete(gateway::GatewayProofStatus status) {
        gateway::GatewayProofResultView value;
        value.operationId = ByteView(
            command_.operationId, sizeof(command_.operationId));
        value.topologyVersion = command_.topologyVersion;
        value.proofNonce = ByteView(
            command_.proofNonce, sizeof(command_.proofNonce));
        value.status = status;
        value.presenceKeyVersion = command_.presenceKeyVersion;
        size_t written = 0U;
        const Result result = gateway::encodeGatewayProofResult(
            value, MutableByteSpan(result_, sizeof(result_)), written);
        if (!result) {
            clearCommand();
            return;
        }
        resultSize_ = written;
        resultReady_ = true;
        publishPending_ = true;
    }

    void retireChild() {
        const GatewayChildSessionState state = child_.state();
        if (state == GatewayChildSessionState::Connecting ||
            state == GatewayChildSessionState::Authenticating ||
            state == GatewayChildSessionState::Secure) {
            (void)child_.disconnect();
        }
    }

    void clearCommand() {
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(&command_), sizeof(command_)));
        secureZero(MutableByteSpan(result_, sizeof(result_)));
        resultSize_ = 0U;
        nextAttemptMillis_ = 0U;
        previousState_ = GatewayChildSessionState::Stopped;
        active_ = false;
        resultReady_ = false;
        publishPending_ = false;
    }

    ChildSession& child_;
    GatewayAccessStore& access_;
    IClock& clock_;
    OwnedCommand command_;
    uint8_t result_[gateway::kGatewayProofResultMaximumEncodedSize];
    size_t resultSize_;
    uint32_t nextAttemptMillis_;
    GatewayChildSessionState previousState_;
    bool active_;
    bool resultReady_;
    bool publishPending_;

    BasicGatewayProofCoordinator(const BasicGatewayProofCoordinator&);
    BasicGatewayProofCoordinator& operator=(
        const BasicGatewayProofCoordinator&);
};

} // namespace blinker

#endif
