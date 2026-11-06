#ifndef BLINKER_SECURITY_INITIALCLOUDKEYREGISTRATIONENDPOINT_H
#define BLINKER_SECURITY_INITIALCLOUDKEYREGISTRATIONENDPOINT_H

#include "InitialCloudWifiHandoff.h"

#include <string.h>
#include "InitialCloudKeyRegistration.h"
#include "../core/SecureMemory.h"
#include "../protocol/bbp2/InitialCloudKeyInstall.h"
#include "../runtime/IPreCloudDirectEndpoint.h"
#include "../transport/BleFrameTransport.h"

namespace blinker {
namespace security {

struct InitialCloudKeyInstallOutcome {
    // Includes a rejection before the installer is called.
    Result installation;
    // A successful enqueue is not peer receipt. NotConfigured means no attempt.
    Result response;
    InitialCloudKeyInstallOutcome()
        : installation(Result::failure(ErrorCode::NotConfigured)),
          response(Result::failure(ErrorCode::NotConfigured)) {}
};

// Only the explicit purpose owner may call this from its negotiated
// BleFrameTransport receive callback. It does not install a generic handler,
// advertise a capability, or mint authorization from the RxContext flags.
class InitialCloudKeyRegistrationEndpoint final : public IPreCloudDirectEndpoint,
    public IInitialCloudWifiHandoffSource {
public:
    InitialCloudKeyRegistrationEndpoint(InitialCloudKeyRegistration& registration,
        IInitialCloudKeyInstaller& installer, BleFrameTransport& direct)
        : registration_(registration), installer_(installer), direct_(direct), handoff_(),
          binding_(), sessionId_(0U), installSequence_(0U), transportId_(0U), state_(State::Initial) {}
    InitialCloudKeyRegistrationEndpoint(const InitialCloudKeyRegistrationEndpoint&) = delete;
    InitialCloudKeyRegistrationEndpoint& operator=(const InitialCloudKeyRegistrationEndpoint&) = delete;

    Result receive(ByteView encodedFrame, const RxContext& rx, uint16_t negotiatedMaximumFrame) override {
        bbp2::FrameView frame;
        const Result result = bbp2::parseFrame(encodedFrame, frame);
        if (!result) return result;
        if (frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyHandoff))
            return authorizeHandoff(encodedFrame, rx);
        const InitialCloudKeyInstallOutcome outcome = handle(encodedFrame, rx, negotiatedMaximumFrame);
        return outcome.installation ? outcome.response : outcome.installation;
    }

    InitialCloudKeyInstallOutcome handle(ByteView encodedFrame, const RxContext& rx, uint16_t negotiatedMaximumFrame) {
        InitialCloudKeyInstallOutcome outcome;
        if (state_ != State::Initial) {
            outcome.installation = Result::failure(ErrorCode::StateConflict);
            return outcome;
        }
        if (!rx.encrypted || rx.sessionId == 0U) {
            outcome.installation = Result::failure(ErrorCode::AuthenticationRequired);
            return outcome;
        }
        bbp2::FrameView frame;
        outcome.installation = bbp2::parseFrame(encodedFrame, frame);
        if (!outcome.installation) return outcome;
        if (frame.header.kind != static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyInstall) ||
            frame.header.flags != bbp2::FlagNone || frame.header.sequence == 0U ||
            frame.header.headerLength != bbp2::kBaseHeaderSize) {
            outcome.installation = Result::failure(ErrorCode::InvalidEncoding);
            return outcome;
        }
        bbp2::InitialCloudKeyInstallBody request;
        outcome.installation = bbp2::decodeInitialCloudKeyInstallBody(frame.body, request);
        if (!outcome.installation) return outcome;
        if (negotiatedMaximumFrame < bbp2::kInitialCloudKeyInstallFrameSize) {
            outcome.installation = Result::failure(ErrorCode::BufferTooSmall);
            return outcome;
        }

        // Snapshot the original reply address, session, request and exact ACK
        // before any installer/storage callback. No borrowed Key crosses return.
        const RxContext originalRx = rx;
        uint8_t binding[kDirectSecureBindingSize] = {};
        const ByteView expectedBinding(binding, sizeof(binding));
        SendTarget target;
        target.kind = SendTargetKind::Session;
        target.transportId = originalRx.transportId;
        target.sessionId = originalRx.sessionId;
        uint8_t ack[bbp2::kInitialCloudKeyInstalledFrameSize] = {};
        InitialCloudKeyHandoff installed = {};
        bool attempted = false;
        {
            Candidate candidate;
            memcpy(candidate.key.bytes, request.key.data, sizeof(candidate.key.bytes));
            memcpy(candidate.intent, request.intent.data, sizeof(candidate.intent));
            memcpy(candidate.proof, request.proof.data, sizeof(candidate.proof));
            const ByteView intent(candidate.intent, sizeof(candidate.intent));
            const ByteView live = direct_.committedSessionBinding(originalRx.sessionId);
            if (!bbp2::exactInitialCloudKeyBytes(live, sizeof(binding))) {
                outcome.installation = Result::failure(ErrorCode::AuthenticationRequired);
                return outcome;
            }
            memcpy(binding, live.data, sizeof(binding));
            memcpy(installed.intent, candidate.intent, sizeof(installed.intent));
            outcome.installation = deriveDeviceKeyLocator(candidate.key, installed.locator);
            bbp2::FrameHeader header;
            header.kind = static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyInstalled);
            header.flags = bbp2::FlagIsResponse;
            header.sequence = frame.header.sequence;
            header.bodyLength = static_cast<uint16_t>(bbp2::kInitialCloudKeyInstalledBodySize);
            if (outcome.installation) outcome.installation = bbp2::encodeHeader(header, MutableByteSpan(ack, sizeof(ack)));
            if (outcome.installation) outcome.installation = bbp2::encodeInitialCloudKeyInstalledBody(intent, installed.locator,
                MutableByteSpan(ack + bbp2::kBaseHeaderSize, bbp2::kInitialCloudKeyInstalledBodySize));
            if (outcome.installation) outcome.installation = direct_.canSendBoundSecure(sizeof(ack), target, expectedBinding);
            if (outcome.installation) outcome.installation = registration_.attemptInstall(originalRx, intent, candidate.key,
                ByteView(candidate.proof, sizeof(candidate.proof)), installer_, attempted);
        }
        if (outcome.installation && state_ == State::Initial) {
            // This result does not overwrite the already-successful install.
            outcome.response = direct_.sendBoundSecure(ByteView(ack, sizeof(ack)), target, expectedBinding);
            if (outcome.response && state_ == State::Initial) {
                handoff_ = installed;
                memcpy(binding_, binding, sizeof(binding_));
                sessionId_ = originalRx.sessionId;
                transportId_ = originalRx.transportId;
                installSequence_ = frame.header.sequence;
                state_ = State::AwaitingHandoff;
            }
        }
        // Failure/UNKNOWN after entering storage is terminal for this owner,
        // even if a later Method 2 handshake resets the session's attempt bit.
        if (attempted && state_ != State::AwaitingHandoff) retire();
        return outcome;
    }

    // An authenticated App assertion after service commit, not a device-side
    // server signature check. No wire response or native owner change here.
    Result authorizeHandoff(ByteView encodedFrame, const RxContext& rx) {
        if (!rx.encrypted || rx.sessionId == 0U) return Result::failure(ErrorCode::AuthenticationRequired);
        if (state_ != State::AwaitingHandoff) return Result::failure(ErrorCode::StateConflict);
        if (rx.transportId != transportId_ || rx.sessionId != sessionId_)
            return Result::failure(ErrorCode::AuthenticationRequired);
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(encodedFrame, frame);
        if (!result) return result;
        if (frame.header.kind != static_cast<uint8_t>(bbp2::MessageKind::InitialCloudKeyHandoff) ||
            frame.header.flags != bbp2::FlagNone || frame.header.sequence == 0U ||
            frame.header.sequence == installSequence_ || frame.header.headerLength != bbp2::kBaseHeaderSize)
            return Result::failure(ErrorCode::InvalidEncoding);
        bbp2::InitialCloudKeyHandoffBody value;
        result = bbp2::decodeInitialCloudKeyHandoffBody(frame.body, value);
        if (!result) return result;
        if (!constantTimeEqual(value.intent, ByteView(handoff_.intent, sizeof(handoff_.intent))) ||
            !constantTimeEqual(value.locator, ByteView(handoff_.locator.bytes, sizeof(handoff_.locator.bytes))))
            return Result::failure(ErrorCode::AuthenticationRequired);
        InitialCloudKeyHandoffAuthority authority;
        result = registration_.validateHandoffSession(rx, ByteView(binding_, sizeof(binding_)), authority);
        if (!result) return result;
        if (state_ != State::AwaitingHandoff) return Result::failure(ErrorCode::StateConflict);
        handoff_.authority = authority;
        memcpy(handoff_.sessionBinding, binding_, sizeof(handoff_.sessionBinding));
        state_ = State::Authorized;
        return Result::success();
    }

    // Once accepted, the instruction can outlive the expected link close.
    // The supervisor still needs native shutdown and a fresh durable root/Key
    // check before starting any new owner. Explicit cancellation is terminal.
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
        secureZero(MutableByteSpan(binding_, sizeof(binding_)));
    }

private:
    enum class State : uint8_t { Initial, AwaitingHandoff, Authorized, Retired };
    struct Candidate {
        DeviceKey key;
        uint8_t intent[bbp2::kInitialCloudKeyInstallIntentSize];
        uint8_t proof[bbp2::kInitialCloudKeyInstallProofSize];
        Candidate() : key(), intent(), proof() {}
        ~Candidate() { clearDeviceKey(key); secureZero(MutableByteSpan(proof, sizeof(proof))); }
    };
    InitialCloudKeyRegistration& registration_;
    IInitialCloudKeyInstaller& installer_;
    BleFrameTransport& direct_;
    InitialCloudKeyHandoff handoff_;
    uint8_t binding_[kDirectSecureBindingSize];
    uint32_t sessionId_;
    uint16_t installSequence_;
    uint8_t transportId_;
    State state_;
};

} // namespace security
} // namespace blinker
#endif
