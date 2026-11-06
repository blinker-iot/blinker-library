#ifndef BLINKER_SECURITY_INITIALCLOUDNETWORKRECOVERYENDPOINT_H
#define BLINKER_SECURITY_INITIALCLOUDNETWORKRECOVERYENDPOINT_H

#include <string.h>
#include "InitialCloudKeyRegistration.h"
#include "InitialCloudWifiHandoff.h"
#include "../core/SecureMemory.h"
#include "../identity/DeviceAccessStore.h"
#include "../protocol/bbp2/InitialCloudNetworkRecovery.h"
#include "../provisioning/InitialCloudNetworkBaseline.h"
#include "../runtime/IPreCloudDirectEndpoint.h"
#include "../transport/BleFrameTransport.h"

namespace blinker {
namespace security {

// Explicit installed-Key initial-network owner only. There is no installer,
// raw Key, rotation or ordinary Direct handler in this endpoint. Its acceptance
// only authorizes a later supervised radio handoff, never a network/Cloud result.
class InitialCloudNetworkRecoveryEndpoint final : public IPreCloudDirectEndpoint,
    public IInitialCloudWifiHandoffSource {
public:
    InitialCloudNetworkRecoveryEndpoint(InitialCloudKeyRegistration& registration,
        DeviceAccessStore& access, IInitialCloudNetworkBaseline& baseline, BleFrameTransport& direct,
        ByteView expectedOperation, const DeviceKeyLocator& expectedLocator)
        : registration_(registration), access_(access), baseline_(baseline), direct_(direct),
          operation_(), locator_(expectedLocator), handoff_(), state_(State::Initial),
          configured_(bbp2::exactInitialCloudNetworkBytes(expectedOperation, sizeof(operation_)) &&
              bbp2::exactInitialCloudNetworkBytes(ByteView(locator_.bytes, sizeof(locator_.bytes)), sizeof(locator_.bytes))) {
        if (configured_) memcpy(operation_, expectedOperation.data, sizeof(operation_));
    }
    ~InitialCloudNetworkRecoveryEndpoint() override { retire(); }
    InitialCloudNetworkRecoveryEndpoint(const InitialCloudNetworkRecoveryEndpoint&) = delete;
    InitialCloudNetworkRecoveryEndpoint& operator=(const InitialCloudNetworkRecoveryEndpoint&) = delete;

    Result receive(ByteView encodedFrame, const RxContext& rx, uint16_t negotiatedMaximumFrame) override {
        if (state_ != State::Initial) return Result::failure(ErrorCode::StateConflict);
        if (!configured_) return Result::failure(ErrorCode::NotConfigured);
        if (!rx.encrypted || rx.sessionId == 0U) return Result::failure(ErrorCode::AuthenticationRequired);
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(encodedFrame, frame);
        if (!result) return result;
        if (frame.header.kind != static_cast<uint8_t>(bbp2::MessageKind::InitialCloudNetworkHandoff) ||
            frame.header.flags != bbp2::FlagNone || frame.header.sequence == 0U ||
            frame.header.headerLength != bbp2::kBaseHeaderSize)
            return Result::failure(ErrorCode::InvalidEncoding);
        bbp2::InitialCloudNetworkRecoveryBody request;
        result = bbp2::decodeInitialCloudNetworkRecoveryBody(frame.body, request);
        if (!result) return result;
        if (negotiatedMaximumFrame < bbp2::kInitialCloudNetworkFrameSize)
            return Result::failure(ErrorCode::BufferTooSmall);
        if (!constantTimeEqual(request.operation, ByteView(operation_, sizeof(operation_))) ||
            !constantTimeEqual(request.locator, ByteView(locator_.bytes, sizeof(locator_.bytes))))
            return Result::failure(ErrorCode::AuthenticationRequired);

        // The first well-formed attempt is terminal across callbacks and even a
        // later Method 2 handshake. Copy borrowed coordinates before any work.
        state_ = State::Processing;
        Snapshot pending;
        pending.value.purpose = InitialCloudWifiHandoffPurpose::InstalledKeyNetwork;
        memcpy(pending.value.intent, operation_, sizeof(operation_));
        pending.value.locator = locator_;
        const RxContext originalRx = rx;
        const ByteView live = direct_.committedSessionBinding(originalRx.sessionId);
        if (!bbp2::exactInitialCloudNetworkBytes(live, sizeof(pending.value.sessionBinding)))
            return fail(Result::failure(ErrorCode::AuthenticationRequired));
        memcpy(pending.value.sessionBinding, live.data, sizeof(pending.value.sessionBinding));
        const ByteView binding(pending.value.sessionBinding, sizeof(pending.value.sessionBinding));
        uint8_t accepted[bbp2::kInitialCloudNetworkFrameSize] = {};
        bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(bbp2::MessageKind::InitialCloudNetworkAccepted);
        header.flags = bbp2::FlagIsResponse;
        header.sequence = frame.header.sequence;
        header.bodyLength = static_cast<uint16_t>(bbp2::kInitialCloudNetworkBodySize);
        result = bbp2::encodeHeader(header, MutableByteSpan(accepted, sizeof(accepted)));
        if (result) result = bbp2::encodeInitialCloudNetworkRecoveryBody(
            ByteView(operation_, sizeof(operation_)), locator_,
            MutableByteSpan(accepted + bbp2::kBaseHeaderSize, bbp2::kInitialCloudNetworkBodySize));
        if (result) result = registration_.consumeNetworkSession(originalRx, binding, pending.value.authority);
        if (result) result = validateAttempt(originalRx, pending.value);
        if (result) result = baseline_.validateEmptyNetwork();
        if (result) result = validateAttempt(originalRx, pending.value);
        if (!result || state_ != State::Processing)
            return fail(result ? Result::failure(ErrorCode::StateConflict) : result);
        SendTarget target;
        target.kind = SendTargetKind::Session;
        target.transportId = originalRx.transportId;
        target.sessionId = originalRx.sessionId;
        result = direct_.canSendBoundSecure(sizeof(accepted), target, binding);
        if (result) result = direct_.sendBoundSecure(ByteView(accepted, sizeof(accepted)), target, binding);
        if (!result || state_ != State::Processing)
            return fail(result ? Result::failure(ErrorCode::StateConflict) : result);
        // Queue success is not receipt. A lost response remains UNKNOWN; no
        // response replay or automatic reopening of this owner is available.
        handoff_ = pending.value;
        state_ = State::Authorized;
        return Result::success();
    }

    bool takeAuthorizedHandoff(InitialCloudKeyHandoff& output) override {
        output = InitialCloudKeyHandoff();
        if (state_ != State::Authorized) return false;
        output = handoff_;
        retire();
        return true;
    }
    void retire() override {
        state_ = State::Retired;
        secureZero(MutableByteSpan(handoff_.sessionBinding, sizeof(handoff_.sessionBinding)));
        handoff_ = InitialCloudKeyHandoff();
    }

private:
    enum class State : uint8_t { Initial, Processing, Authorized, Retired };
    struct Snapshot {
        InitialCloudKeyHandoff value;
        Snapshot() : value() {}
        ~Snapshot() { secureZero(MutableByteSpan(value.sessionBinding, sizeof(value.sessionBinding))); }
    };
    Result validateAttempt(const RxContext& rx, const InitialCloudKeyHandoff& value) {
        if (state_ != State::Processing) return Result::failure(ErrorCode::StateConflict);
        const ByteView binding(value.sessionBinding, sizeof(value.sessionBinding));
        Result result = registration_.validateConsumedNetworkSession(rx, binding, value.authority);
        if (result && state_ == State::Processing) result = access_.verifyInitialCloudWifiRoot(
            ByteView(value.authority.adminId, sizeof(value.authority.adminId)), value.authority.epoch,
            value.authority.credentialVersion, value.locator);
        if (result && state_ == State::Processing)
            result = registration_.validateConsumedNetworkSession(rx, binding, value.authority);
        return state_ == State::Processing ? result : Result::failure(ErrorCode::StateConflict);
    }
    Result fail(Result result) { retire(); return result; }

    InitialCloudKeyRegistration& registration_;
    DeviceAccessStore& access_;
    IInitialCloudNetworkBaseline& baseline_;
    BleFrameTransport& direct_;
    uint8_t operation_[bbp2::kInitialCloudNetworkOperationSize];
    DeviceKeyLocator locator_;
    InitialCloudKeyHandoff handoff_;
    State state_;
    bool configured_;
};

} // namespace security
} // namespace blinker
#endif
