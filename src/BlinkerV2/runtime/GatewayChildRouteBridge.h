#ifndef BLINKER_RUNTIME_GATEWAYCHILDROUTEBRIDGE_H
#define BLINKER_RUNTIME_GATEWAYCHILDROUTEBRIDGE_H

#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "../identity/GatewayAccessStore.h"
#include "../interface/IClock.h"
#include "../interface/IRandom.h"
#include "../protocol/bbp2/Frame.h"
#include "../protocol/bbp2/Messages.h"
#include "GatewayChildSession.h"
#include "GatewayRouteClient.h"

namespace blinker {

typedef void (*GatewayChildControlReceiver)(void* context, ByteView frame);

// G2A's exact single-child data plane. It accepts a non-secret Active route
// admission, then forwards one BBP/2 exchange at a time over DirectSecure.
// It never derives topology authority from the child or from BLE metadata.
template <typename ChildSession, typename RouteClient>
class BasicGatewayChildRouteBridge {
public:
    BasicGatewayChildRouteBridge(
        ChildSession& child,
        RouteClient& route,
        GatewayAccessStore& access,
        IClock& clock,
        IRandom& random)
        : child_(child), route_(route), access_(access), clock_(clock),
          random_(random), topologyVersion_(0U), routeGeneration_(0U),
          pendingCorrelation_(),
          pendingFrameDigest_(), recentCorrelation_(), recentFrameDigest_(),
          helloFrame_(), frameScratch_(), output_(),
          pendingRequestKind_(0U), pendingCloudSequence_(0U),
          pendingChildSequence_(0U), nextChildSequence_(1U),
          recentRequestKind_(0U), recentCloudSequence_(0U),
          recentChildSequence_(0U), recentReplayable_(false),
          controlReceiver_(nullptr), controlReceiverContext_(nullptr),
          controlRequestKind_(0U), controlResponseKind_(0U),
          controlSequence_(0U), admitted_(false), cloudHelloSent_(false),
          awaitingChild_(false), awaitingControl_(false),
          lastError_(ErrorCode::Ok) {
        route_.setReceiver(
            &BasicGatewayChildRouteBridge::routeThunk, this);
        child_.setReceiver(
            &BasicGatewayChildRouteBridge::childThunk, this);
        child_.setSessionHandlers(
            &BasicGatewayChildRouteBridge::secureThunk,
            &BasicGatewayChildRouteBridge::disconnectedThunk,
            this);
        child_.setFaultHandler(
            &BasicGatewayChildRouteBridge::faultThunk, this);
    }

    ~BasicGatewayChildRouteBridge() {
        route_.setReceiver(nullptr, nullptr);
        child_.setReceiver(nullptr, nullptr);
        child_.setSessionHandlers(nullptr, nullptr, nullptr);
        child_.setFaultHandler(nullptr, nullptr);
        clearControlRequest();
        controlReceiver_ = nullptr;
        controlReceiverContext_ = nullptr;
        reset();
    }

    void reset() {
        topologyVersion_ = 0U;
        routeGeneration_ = 0U;
        admitted_ = false;
        cloudHelloSent_ = false;
        awaitingChild_ = false;
        pendingRequestKind_ = 0U;
        pendingCloudSequence_ = 0U;
        pendingChildSequence_ = 0U;
        nextChildSequence_ = 1U;
        lastError_ = ErrorCode::Ok;
        secureZero(MutableByteSpan(
            pendingCorrelation_, sizeof(pendingCorrelation_)));
        secureZero(MutableByteSpan(
            pendingFrameDigest_, sizeof(pendingFrameDigest_)));
        clearRecentRequest();
        secureZero(MutableByteSpan(helloFrame_, sizeof(helloFrame_)));
        secureZero(MutableByteSpan(frameScratch_, sizeof(frameScratch_)));
        secureZero(MutableByteSpan(output_, sizeof(output_)));
    }

    void poll() {
        if (!admitted_ || !route_.subscribed()) return;
        if (!recordStillValid()) {
            fail(ErrorCode::AuthenticationRequired);
            return;
        }
        if (child_.state() == GatewayChildSessionState::Stopped) {
            const Result result = child_.start();
            if (!result) {
                lastError_ = result.code();
                return;
            }
        }
        if (child_.secure()) {
            if (!cloudHelloSent_) onChildSecure();
            return;
        }
        if (child_.state() == GatewayChildSessionState::Idle) {
            const Result result = child_.connect();
            if (!result && result.code() != ErrorCode::AlreadyExists) {
                lastError_ = result.code();
            }
        }
    }

    bool admitted() const { return admitted_; }
    uint32_t topologyVersion() const { return topologyVersion_; }
    ErrorCode lastError() const { return lastError_; }
    uint32_t authenticatedControllerCredentialVersion() const {
        return child_.authenticatedCredentialVersion();
    }
    uint32_t authenticatedControllerSessionAttemptId() const {
        return child_.secure() ? child_.attemptId() : 0U;
    }

    void setControllerControlReceiver(
        GatewayChildControlReceiver receiver,
        void* context) {
        controlReceiver_ = receiver;
        controlReceiverContext_ = context;
    }

    bool controllerControlPending() const { return awaitingControl_; }

    // Brings up DirectSecure for a signed management operation even while the
    // ordinary route is intentionally withdrawn. It does not open a control
    // nonce or authorize data-plane forwarding.
    Result ensureControllerControlSession() {
        if (child_.secure()) return Result::success();
        GatewayChildSessionState state = child_.state();
        if (state == GatewayChildSessionState::Stopped) {
            const Result result = child_.start();
            if (!result) return result;
            state = child_.state();
        }
        if (state == GatewayChildSessionState::Idle) {
            const Result result = child_.connect();
            if (!result && result.code() != ErrorCode::AlreadyExists) {
                return result;
            }
            return Result::failure(ErrorCode::WouldBlock);
        }
        if (state == GatewayChildSessionState::Fault) {
            return Result::failure(ErrorCode::NotConnected);
        }
        return Result::failure(ErrorCode::WouldBlock);
    }

    void cancelControllerControlRequest() {
        if (!awaitingControl_) return;
        clearControlRequest();
        if (child_.state() == GatewayChildSessionState::Connecting ||
            child_.state() == GatewayChildSessionState::Authenticating ||
            child_.state() == GatewayChildSessionState::Secure) {
            (void)child_.disconnect();
        }
    }

    // Retires the authenticated child session after a credential mutation.
    // Unlike cancelControllerControlRequest(), this also applies after the
    // exact response has arrived and the awaiting slot has been cleared.
    // A subsequent proof must therefore run a fresh Method 2 exchange.
    void retireControllerControlSession() {
        clearControlRequest();
        if (child_.state() == GatewayChildSessionState::Connecting ||
            child_.state() == GatewayChildSessionState::Authenticating ||
            child_.state() == GatewayChildSessionState::Secure) {
            (void)child_.disconnect();
        }
    }

    // The route bridge owns the sole child BBP/2 sequence allocator. Control
    // requests use this narrow side channel, but are never accepted from the
    // ordinary cloud route topic.
    Result sendControllerControlRequest(
        bbp2::MessageKind kind,
        ByteView body) {
        const bbp2::MessageKind expected =
            kind == bbp2::MessageKind::ControllerControlOpen
                ? bbp2::MessageKind::ControllerControlChallenge
                : bbp2::MessageKind::ControllerMutationReceipt;
        if ((kind != bbp2::MessageKind::ControllerControlOpen &&
             kind != bbp2::MessageKind::ControllerMutation) ||
            body.data == nullptr) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (awaitingChild_ || awaitingControl_ ||
            controlReceiver_ == nullptr) {
            return Result::failure(ErrorCode::StateConflict);
        }
        if (!child_.secure()) {
            const Result ready = ensureControllerControlSession();
            if (!ready && ready.code() != ErrorCode::WouldBlock) return ready;
            // Revoking retires the ordinary route before Prepare is sent. A
            // signed management command may therefore bring up DirectSecure
            // from the durable AccessRecord even though no data-plane route is
            // admitted. The coordinator retries once authentication finishes.
            return Result::failure(ErrorCode::WouldBlock);
        }
        if ((child_.negotiatedFeatures() &
             bbp2::FeatureControllerControl) == 0U) {
            return Result::failure(ErrorCode::UnsupportedFeature);
        }
        const uint16_t maximumFrameSize = child_.negotiatedMaxFrameSize();
        if (maximumFrameSize < bbp2::kBaseHeaderSize ||
            body.size + bbp2::kBaseHeaderSize > sizeof(frameScratch_) ||
            body.size > maximumFrameSize - bbp2::kBaseHeaderSize) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        const uint16_t sequence = allocateChildSequence();
        bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(kind);
        // ControllerControl is a request/response exchange of its own. The
        // device runtime accepts these management requests with FlagNone and
        // returns the correlated Challenge/Receipt with FlagIsResponse; using
        // AckRequired here makes a real device reject the request before the
        // controller-control endpoint sees it.
        header.flags = bbp2::FlagNone;
        header.sequence = sequence;
        header.bodyLength = static_cast<uint16_t>(body.size);
        Result result = bbp2::encodeHeader(
            header, MutableByteSpan(frameScratch_, sizeof(frameScratch_)));
        if (result && !body.empty()) {
            memcpy(
                frameScratch_ + bbp2::kBaseHeaderSize,
                body.data,
                body.size);
        }
        if (result) {
            result = child_.sendFrame(ByteView(
                frameScratch_, bbp2::kBaseHeaderSize + body.size));
        }
        secureZero(MutableByteSpan(frameScratch_, sizeof(frameScratch_)));
        if (!result) return result;
        controlRequestKind_ = static_cast<uint8_t>(kind);
        controlResponseKind_ = static_cast<uint8_t>(expected);
        controlSequence_ = sequence;
        awaitingControl_ = true;
        return Result::success();
    }

private:
    static bool same(ByteView value, const uint8_t* bytes, size_t size) {
        return value.data != nullptr && value.size == size &&
               memcmp(value.data, bytes, size) == 0;
    }

    static bool same(StringView value, const char* text, size_t size) {
        return value.data != nullptr && value.size == size &&
               memcmp(value.data, text, size) == 0;
    }

    bool admissionMatches(
        const gateway::GatewayRouteAdmissionView& admission,
        const GatewayAccessRecord& record,
        uint64_t now) const {
        return now < admission.expiresAtUnixSeconds &&
               admission.expiresAtUnixSeconds ==
                   record.expiresAtUnixSeconds &&
               same(admission.operationId, record.operationId,
                    sizeof(record.operationId)) &&
               same(admission.childLogicalDeviceId,
                    record.childLogicalDeviceId,
                    record.childLogicalDeviceIdSize) &&
               same(admission.childDeviceInstanceId,
                    record.childDeviceInstanceId,
                    sizeof(record.childDeviceInstanceId)) &&
               admission.accessEpoch == record.accessEpoch &&
               admission.gatewayCredentialVersion ==
                   record.credentialVersion;
    }

    bool envelopeMatches(
        const gateway::ChildRouteEnvelopeView& envelope,
        const GatewayAccessRecord& record) const {
        return admitted_ &&
               envelope.topologyVersion == topologyVersion_ &&
               same(envelope.childLogicalDeviceId,
                    record.childLogicalDeviceId,
                    record.childLogicalDeviceIdSize) &&
               same(envelope.childDeviceInstanceId,
                    record.childDeviceInstanceId,
                    sizeof(record.childDeviceInstanceId)) &&
               envelope.accessEpoch == record.accessEpoch &&
               envelope.gatewayCredentialVersion ==
                   record.credentialVersion;
    }

    bool recordStillValid() {
        GatewayAccessRecord record;
        uint64_t now = 0U;
        const Result result = access_.load(record);
        const bool valid = result && clock_.unixTime(now) && now != 0U &&
                           now < record.expiresAtUnixSeconds;
        clearGatewayAccessRecord(record);
        return valid;
    }

    static bool allowedDownstream(uint8_t kind) {
        using bbp2::MessageKind;
        return kind == static_cast<uint8_t>(MessageKind::Hello) ||
               kind == static_cast<uint8_t>(MessageKind::ManifestRequest) ||
               kind == static_cast<uint8_t>(MessageKind::ManifestAccept) ||
               kind == static_cast<uint8_t>(MessageKind::StateRequest) ||
               kind == static_cast<uint8_t>(MessageKind::Command) ||
               kind == static_cast<uint8_t>(MessageKind::Ack) ||
               kind == static_cast<uint8_t>(MessageKind::Error);
    }

    static bool allowedUpstream(uint8_t kind) {
        using bbp2::MessageKind;
        return kind == static_cast<uint8_t>(MessageKind::Hello) ||
               kind == static_cast<uint8_t>(MessageKind::Manifest) ||
               kind == static_cast<uint8_t>(MessageKind::StatePage) ||
               kind == static_cast<uint8_t>(MessageKind::Patch) ||
               kind == static_cast<uint8_t>(MessageKind::Ack) ||
               kind == static_cast<uint8_t>(MessageKind::Error);
    }

    static bool expectsChildResponse(const bbp2::FrameView& frame) {
        if ((frame.header.flags & bbp2::FlagIsResponse) != 0U ||
            frame.header.sequence == 0U) {
            return false;
        }
        using bbp2::MessageKind;
        const MessageKind kind =
            static_cast<MessageKind>(frame.header.kind);
        return kind == MessageKind::Hello ||
               kind == MessageKind::ManifestRequest ||
               kind == MessageKind::ManifestAccept ||
               kind == MessageKind::StateRequest ||
               kind == MessageKind::Command;
    }

    static bool matchesResponse(
        const bbp2::FrameView& frame,
        uint8_t requestKind,
        uint16_t childSequence) {
        if (childSequence == 0U ||
            (frame.header.flags & bbp2::FlagIsResponse) == 0U) {
            return false;
        }
        using bbp2::MessageKind;
        const MessageKind request =
            static_cast<MessageKind>(requestKind);
        const MessageKind response =
            static_cast<MessageKind>(frame.header.kind);
        if ((request == MessageKind::Hello &&
             response == MessageKind::Hello) ||
            (request == MessageKind::ManifestRequest &&
             response == MessageKind::Manifest) ||
            (request == MessageKind::StateRequest &&
             response == MessageKind::StatePage)) {
            return frame.header.sequence == childSequence;
        }
        if (response == MessageKind::Ack) {
            bbp2::AckBody body;
            return bbp2::decodeAckBody(frame.body, body) &&
                    body.acknowledgedSequence == childSequence;
        }
        if (response == MessageKind::Error) {
            bbp2::ErrorBody body;
            return bbp2::decodeErrorBody(frame.body, body) &&
                   body.hasRelatedSequence &&
                    body.relatedSequence == childSequence;
        }
        return false;
    }

    bool matchesPendingResponse(const bbp2::FrameView& frame) const {
        return awaitingChild_ && matchesResponse(
            frame, pendingRequestKind_, pendingChildSequence_);
    }

    bool matchesRecentResponse(const bbp2::FrameView& frame) const {
        return recentReplayable_ && matchesResponse(
            frame, recentRequestKind_, recentChildSequence_);
    }

    void clearPendingRequest() {
        awaitingChild_ = false;
        pendingRequestKind_ = 0U;
        pendingCloudSequence_ = 0U;
        pendingChildSequence_ = 0U;
        secureZero(MutableByteSpan(
            pendingCorrelation_, sizeof(pendingCorrelation_)));
        secureZero(MutableByteSpan(
            pendingFrameDigest_, sizeof(pendingFrameDigest_)));
    }

    void clearRecentRequest() {
        recentReplayable_ = false;
        recentRequestKind_ = 0U;
        recentCloudSequence_ = 0U;
        recentChildSequence_ = 0U;
        secureZero(MutableByteSpan(
            recentCorrelation_, sizeof(recentCorrelation_)));
        secureZero(MutableByteSpan(
            recentFrameDigest_, sizeof(recentFrameDigest_)));
    }

    void rememberCompletedRequest() {
        using bbp2::MessageKind;
        const MessageKind kind =
            static_cast<MessageKind>(pendingRequestKind_);
        if (kind != MessageKind::Command &&
            kind != MessageKind::ManifestAccept) {
            return;
        }
        memcpy(recentCorrelation_, pendingCorrelation_,
               sizeof(recentCorrelation_));
        memcpy(recentFrameDigest_, pendingFrameDigest_,
               sizeof(recentFrameDigest_));
        recentRequestKind_ = pendingRequestKind_;
        recentCloudSequence_ = pendingCloudSequence_;
        recentChildSequence_ = pendingChildSequence_;
        recentReplayable_ = true;
    }

    void clearControlRequest() {
        awaitingControl_ = false;
        controlRequestKind_ = 0U;
        controlResponseKind_ = 0U;
        controlSequence_ = 0U;
    }

    uint16_t allocateChildSequence() {
        const uint16_t sequence = nextChildSequence_;
        nextChildSequence_ = sequence == UINT16_MAX
                                 ? 1U
                                 : static_cast<uint16_t>(sequence + 1U);
        return sequence;
    }

    Result rewriteFrameSequence(
        ByteView encoded,
        uint16_t sequence,
        ByteView& rewritten) {
        if (encoded.data == nullptr || sequence == 0U ||
            encoded.size > sizeof(frameScratch_)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(encoded, frame);
        if (!result) return result;
        memcpy(frameScratch_, encoded.data, encoded.size);
        frameScratch_[6] = static_cast<uint8_t>(sequence >> 8U);
        frameScratch_[7] = static_cast<uint8_t>(sequence & 0xFFU);
        rewritten = ByteView(frameScratch_, encoded.size);
        return Result::success();
    }

    Result rewriteMappedResponse(
        ByteView encoded,
        const bbp2::FrameView& frame,
        uint16_t cloudSequence,
        ByteView& rewritten) {
        using bbp2::MessageKind;
        const MessageKind kind = static_cast<MessageKind>(frame.header.kind);
        if (kind == MessageKind::Hello || kind == MessageKind::Manifest ||
            kind == MessageKind::StatePage) {
            return rewriteFrameSequence(
                encoded, cloudSequence, rewritten);
        }

        bbp2::FrameHeader header = frame.header;
        ByteView body;
        Result result = Result::failure(ErrorCode::UnsupportedFeature);
        if (kind == MessageKind::Ack) {
            bbp2::AckBody ack;
            result = bbp2::decodeAckBody(frame.body, ack);
            if (result) {
                ack.acknowledgedSequence = cloudSequence;
                result = bbp2::encodeAckBody(
                    ack,
                    MutableByteSpan(
                        frameScratch_ + bbp2::kBaseHeaderSize,
                        sizeof(frameScratch_) - bbp2::kBaseHeaderSize),
                    body);
            }
        } else if (kind == MessageKind::Error) {
            bbp2::ErrorBody error;
            result = bbp2::decodeErrorBody(frame.body, error);
            if (result) {
                error.relatedSequence = cloudSequence;
                error.hasRelatedSequence = true;
                result = bbp2::encodeErrorBody(
                    error,
                    MutableByteSpan(
                        frameScratch_ + bbp2::kBaseHeaderSize,
                        sizeof(frameScratch_) - bbp2::kBaseHeaderSize),
                    body);
            }
        }
        if (!result) return result;
        header.bodyLength = static_cast<uint16_t>(body.size);
        result = bbp2::encodeHeader(
            header, MutableByteSpan(frameScratch_, sizeof(frameScratch_)));
        if (!result) return result;
        rewritten = ByteView(
            frameScratch_, bbp2::kBaseHeaderSize + body.size);
        return Result::success();
    }

    void onRouteMessage(ByteView message) {
        gateway::GatewayRouteAdmissionView admission;
        Result result = gateway::decodeGatewayRouteAdmission(
            message, admission);
        if (result) {
            acceptAdmission(admission);
            return;
        }
        if (awaitingControl_) {
            lastError_ = ErrorCode::StateConflict;
            return;
        }

        gateway::ChildRouteEnvelopeView envelope;
        result = gateway::decodeChildRouteEnvelope(message, envelope);
        if (!result || envelope.innerBbp2Frame.size >
                           gateway::kEdgeHubChildMaximumInnerFrameSize) {
            fail(result ? ErrorCode::CapacityExceeded : result.code());
            return;
        }
        bbp2::FrameView frame;
        result = bbp2::parseFrame(envelope.innerBbp2Frame, frame);
        if (!result || !allowedDownstream(frame.header.kind) ||
            !child_.secure()) {
            fail(result ? ErrorCode::StateConflict : result.code());
            return;
        }

        GatewayAccessRecord record;
        uint64_t now = 0U;
        result = access_.load(record);
        const bool valid = result && clock_.unixTime(now) && now != 0U &&
            now < record.expiresAtUnixSeconds &&
            envelopeMatches(envelope, record);
        clearGatewayAccessRecord(record);
        if (!valid) {
            fail(ErrorCode::AuthenticationRequired);
            return;
        }
        const bool expectsResponse = expectsChildResponse(frame);
        uint8_t frameDigest[kSha256Size] = {};
        if (expectsResponse) {
            result = sha256(
                envelope.innerBbp2Frame,
                MutableByteSpan(frameDigest, sizeof(frameDigest)));
            if (!result) {
                fail(result.code());
                return;
            }
            if (awaitingChild_) {
                const bool replay = same(
                    envelope.correlationId,
                    pendingCorrelation_, sizeof(pendingCorrelation_)) &&
                    memcmp(frameDigest, pendingFrameDigest_,
                           sizeof(frameDigest)) == 0;
                secureZero(MutableByteSpan(
                    frameDigest, sizeof(frameDigest)));
                if (replay) {
                    // The child owns the reliable-request replay cache. The
                    // bridge has no response cache, so swallowing an exact
                    // Broker retry would turn a lost child ACK into a timeout.
                    ByteView childFrame;
                    result = rewriteFrameSequence(
                        envelope.innerBbp2Frame,
                        pendingChildSequence_,
                        childFrame);
                    if (result) result = child_.sendFrame(childFrame);
                    if (!result) {
                        if (result.code() == ErrorCode::WouldBlock) {
                            lastError_ = result.code();
                        } else {
                            fail(result.code());
                        }
                    } else {
                        lastError_ = ErrorCode::Ok;
                    }
                    return;
                }
                fail(ErrorCode::StateConflict);
                return;
            }
            const bool completedReplay = recentReplayable_ && same(
                envelope.correlationId,
                recentCorrelation_, sizeof(recentCorrelation_)) &&
                memcmp(frameDigest, recentFrameDigest_,
                       sizeof(frameDigest)) == 0;
            if (completedReplay) {
                secureZero(MutableByteSpan(
                    frameDigest, sizeof(frameDigest)));
                ByteView childFrame;
                result = rewriteFrameSequence(
                    envelope.innerBbp2Frame,
                    recentChildSequence_,
                    childFrame);
                if (result) result = child_.sendFrame(childFrame);
                if (!result) {
                    if (result.code() == ErrorCode::WouldBlock) {
                        lastError_ = result.code();
                    } else {
                        fail(result.code());
                    }
                } else {
                    lastError_ = ErrorCode::Ok;
                }
                return;
            }
        }
        ByteView childFrame = envelope.innerBbp2Frame;
        uint16_t childSequence = 0U;
        if (expectsResponse) {
            childSequence = allocateChildSequence();
            result = rewriteFrameSequence(
                envelope.innerBbp2Frame, childSequence, childFrame);
        }
        if (result) result = child_.sendFrame(childFrame);
        if (!result) {
            secureZero(MutableByteSpan(frameDigest, sizeof(frameDigest)));
            if (result.code() == ErrorCode::WouldBlock) {
                // The child bearer rejected the record before enqueue. Keep
                // DirectSecure and route admission alive; the Broker owns the
                // exact-frame retry and will deliver it again after pressure
                // clears.
                lastError_ = result.code();
            } else {
                fail(result.code());
            }
            return;
        }
        if (expectsResponse) {
            memcpy(pendingCorrelation_, envelope.correlationId.data,
                   sizeof(pendingCorrelation_));
            memcpy(pendingFrameDigest_, frameDigest,
                   sizeof(pendingFrameDigest_));
            pendingRequestKind_ = frame.header.kind;
            pendingCloudSequence_ = frame.header.sequence;
            pendingChildSequence_ = childSequence;
            awaitingChild_ = true;
        }
        secureZero(MutableByteSpan(frameDigest, sizeof(frameDigest)));
        lastError_ = ErrorCode::Ok;
    }

    void acceptAdmission(
        const gateway::GatewayRouteAdmissionView& admission) {
        GatewayAccessRecord record;
        uint64_t now = 0U;
        Result result = access_.load(record);
        const bool valid = result && clock_.unixTime(now) && now != 0U &&
                           admissionMatches(admission, record, now);
        clearGatewayAccessRecord(record);
        if (!valid) {
            fail(ErrorCode::AuthenticationRequired);
            return;
        }
        if (admitted_ && topologyVersion_ != admission.topologyVersion) {
            fail(ErrorCode::StateConflict);
            return;
        }
        const uint32_t generation = route_.subscriptionGeneration();
        if (generation == 0U) {
            fail(ErrorCode::NotConnected);
            return;
        }
        if (routeGeneration_ != generation) {
            // The topology is durable, but route admission and the BBP/2
            // child projection are scoped to one MQTT connection. A fast
            // credential rebind may never expose CloudConnecting to the
            // product scheduler, so the connection generation is the only
            // authoritative boundary.
            cloudHelloSent_ = false;
            clearPendingRequest();
            clearRecentRequest();
        }
        topologyVersion_ = admission.topologyVersion;
        routeGeneration_ = generation;
        admitted_ = true;
        lastError_ = ErrorCode::Ok;
    }

    void onChildFrame(ByteView innerFrame) {
        if (innerFrame.size > gateway::kEdgeHubChildMaximumInnerFrameSize) {
            fail(ErrorCode::AuthenticationRequired);
            return;
        }
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(innerFrame, frame);
        if (result && awaitingControl_ &&
            (frame.header.flags & bbp2::FlagIsResponse) != 0U &&
            frame.header.sequence == controlSequence_ &&
            (frame.header.kind == controlResponseKind_ ||
             frame.header.kind == static_cast<uint8_t>(
                                      bbp2::MessageKind::Error))) {
            clearControlRequest();
            controlReceiver_(controlReceiverContext_, innerFrame);
            return;
        }
        if (!result) {
            fail(result.code());
            return;
        }
        if (!admitted_ || !route_.subscribed()) {
            // A management-only DirectSecure session has no ordinary route,
            // but the child may still emit its negotiated Hello/state frames.
            // They are authenticated yet not authorized for cloud routing:
            // consume them locally instead of reopening route/up or tearing
            // down the control session before its response can arrive.
            return;
        }
        if (!allowedUpstream(frame.header.kind)) {
            fail(ErrorCode::UnsupportedFeature);
            return;
        }

        const bool pendingResponse = matchesPendingResponse(frame);
        const bool recentResponse = !pendingResponse &&
                                    matchesRecentResponse(frame);
        const bbp2::MessageKind kind =
            static_cast<bbp2::MessageKind>(frame.header.kind);
        if (!pendingResponse && !recentResponse &&
            (kind == bbp2::MessageKind::Ack ||
             kind == bbp2::MessageKind::Error)) {
            // ACK/Error are responses, never autonomous child events. A late
            // response outside the bounded translation window cannot be
            // correlated safely and must not be forwarded with a random ID.
            return;
        }
        uint8_t correlation[gateway::kCorrelationIdSize];
        if (pendingResponse) {
            memcpy(correlation, pendingCorrelation_, sizeof(correlation));
        } else if (recentResponse) {
            memcpy(correlation, recentCorrelation_, sizeof(correlation));
        } else {
            result = random_.fill(MutableByteSpan(
                correlation, sizeof(correlation)));
            if (!result) {
                fail(result.code());
                return;
            }
        }

        GatewayAccessRecord record;
        uint64_t now = 0U;
        result = access_.load(record);
        if (!result || !clock_.unixTime(now) || now == 0U ||
            now >= record.expiresAtUnixSeconds) {
            clearGatewayAccessRecord(record);
            secureZero(MutableByteSpan(correlation, sizeof(correlation)));
            fail(ErrorCode::AuthenticationRequired);
            return;
        }
        ByteView routedFrame = innerFrame;
        if (pendingResponse || recentResponse) {
            result = rewriteMappedResponse(
                innerFrame,
                frame,
                pendingResponse ? pendingCloudSequence_
                                : recentCloudSequence_,
                routedFrame);
            if (!result) {
                clearGatewayAccessRecord(record);
                secureZero(MutableByteSpan(correlation, sizeof(correlation)));
                fail(result.code());
                return;
            }
        }
        gateway::ChildRouteEnvelopeView envelope;
        envelope.topologyVersion = topologyVersion_;
        envelope.childLogicalDeviceId = StringView(
            record.childLogicalDeviceId,
            record.childLogicalDeviceIdSize);
        envelope.childDeviceInstanceId = ByteView(
            record.childDeviceInstanceId,
            sizeof(record.childDeviceInstanceId));
        envelope.accessEpoch = record.accessEpoch;
        envelope.gatewayCredentialVersion = record.credentialVersion;
        envelope.correlationId = ByteView(
            correlation, sizeof(correlation));
        envelope.innerBbp2Frame = routedFrame;
        size_t written = 0U;
        result = gateway::encodeChildRouteEnvelope(
            envelope, MutableByteSpan(output_, sizeof(output_)), written);
        clearGatewayAccessRecord(record);
        secureZero(MutableByteSpan(correlation, sizeof(correlation)));
        if (result) result = route_.publish(ByteView(output_, written));
        secureZero(MutableByteSpan(output_, sizeof(output_)));
        if (pendingResponse) {
            rememberCompletedRequest();
            clearPendingRequest();
        }
        if (!result) {
            fail(result.code());
            return;
        }
        lastError_ = ErrorCode::Ok;
    }

    void onChildSecure() {
        if (!admitted_ || cloudHelloSent_ || !child_.secure()) return;
        const ByteView fingerprint = child_.manifestFingerprint();
        if (fingerprint.size != bbp2::kManifestFingerprintSize) {
            fail(ErrorCode::ProtocolError);
            return;
        }
        bbp2::HelloBody hello;
        hello.role = bbp2::PeerRole::Device;
        hello.versions[0] = bbp2::kVersion;
        hello.versionCount = 1U;
        hello.features = child_.negotiatedFeatures();
        hello.maxFrameSize = child_.negotiatedMaxFrameSize();
        hello.maxReassemblySize = hello.maxFrameSize;
        hello.manifestRevision = child_.manifestRevision();
        hello.hasManifestRevision = true;
        hello.manifestFingerprint = fingerprint;
        hello.hasManifestFingerprint = true;
        if ((hello.features & bbp2::FeatureAuthentication) != 0U) {
            hello.authorizationMethods[0] =
                security::kControllerHmacSha256Method;
            hello.authorizationMethodCount = 1U;
        }
        if ((hello.features & bbp2::FeatureReliableDelivery) != 0U) {
            hello.reliableReceiveWindow =
                child_.remoteReliableReceiveWindow();
        }
        ByteView body;
        Result result = bbp2::encodeHelloBody(
            hello,
            MutableByteSpan(
                helloFrame_ + bbp2::kBaseHeaderSize,
                sizeof(helloFrame_) - bbp2::kBaseHeaderSize),
            body);
        if (result) {
            bbp2::FrameHeader header;
            header.kind = static_cast<uint8_t>(bbp2::MessageKind::Hello);
            header.flags = bbp2::FlagAckRequired;
            header.sequence = 1U;
            header.bodyLength = static_cast<uint16_t>(body.size);
            result = bbp2::encodeHeader(
                header, MutableByteSpan(helloFrame_, sizeof(helloFrame_)));
        }
        if (!result) {
            fail(result.code());
            return;
        }
        cloudHelloSent_ = true;
        onChildFrame(ByteView(
            helloFrame_, bbp2::kBaseHeaderSize + body.size));
        secureZero(MutableByteSpan(helloFrame_, sizeof(helloFrame_)));
        if (lastError_ != ErrorCode::Ok) cloudHelloSent_ = false;
    }

    void fail(ErrorCode error) {
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::ProtocolError
                         : error;
        clearPendingRequest();
        clearRecentRequest();
        if (child_.state() == GatewayChildSessionState::Connecting ||
            child_.state() == GatewayChildSessionState::Authenticating ||
            child_.state() == GatewayChildSessionState::Secure) {
            (void)child_.disconnect();
        }
    }

    static void routeThunk(void* context, ByteView message) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self != nullptr) self->onRouteMessage(message);
    }
    static void childThunk(void* context, ByteView frame) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self != nullptr) self->onChildFrame(frame);
    }
    static void secureThunk(void* context) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self != nullptr) self->onChildSecure();
    }
    static void disconnectedThunk(void* context) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self != nullptr) {
            self->cloudHelloSent_ = false;
            self->clearPendingRequest();
            self->clearRecentRequest();
            self->clearControlRequest();
            self->nextChildSequence_ = 1U;
        }
    }
    static void faultThunk(
        void* context,
        ErrorCode error) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self != nullptr) self->lastError_ = error;
    }

    ChildSession& child_;
    RouteClient& route_;
    GatewayAccessStore& access_;
    IClock& clock_;
    IRandom& random_;
    uint32_t topologyVersion_;
    uint32_t routeGeneration_;
    uint8_t pendingCorrelation_[gateway::kCorrelationIdSize];
    uint8_t pendingFrameDigest_[kSha256Size];
    uint8_t recentCorrelation_[gateway::kCorrelationIdSize];
    uint8_t recentFrameDigest_[kSha256Size];
    uint8_t helloFrame_[128U];
    uint8_t frameScratch_[gateway::kEdgeHubChildMaximumInnerFrameSize];
    uint8_t output_[gateway::kEdgeHubChildRouteEnvelopeMaximumEncodedSize];
    uint8_t pendingRequestKind_;
    uint16_t pendingCloudSequence_;
    uint16_t pendingChildSequence_;
    uint16_t nextChildSequence_;
    uint8_t recentRequestKind_;
    uint16_t recentCloudSequence_;
    uint16_t recentChildSequence_;
    bool recentReplayable_;
    GatewayChildControlReceiver controlReceiver_;
    void* controlReceiverContext_;
    uint8_t controlRequestKind_;
    uint8_t controlResponseKind_;
    uint16_t controlSequence_;
    bool admitted_;
    bool cloudHelloSent_;
    bool awaitingChild_;
    bool awaitingControl_;
    ErrorCode lastError_;

    BasicGatewayChildRouteBridge(
        const BasicGatewayChildRouteBridge&);
    BasicGatewayChildRouteBridge& operator=(
        const BasicGatewayChildRouteBridge&);
};

} // namespace blinker

#endif
