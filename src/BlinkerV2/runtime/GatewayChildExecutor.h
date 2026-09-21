#ifndef BLINKER_RUNTIME_GATEWAYCHILDEXECUTOR_H
#define BLINKER_RUNTIME_GATEWAYCHILDEXECUTOR_H

#include <stdint.h>
#include <string.h>

#include "../core/SecureMemory.h"
#include "../protocol/bbp2/Frame.h"
#include "../protocol/bbp2/Messages.h"
#include "../protocol/gateway/Contracts.h"
#include "GatewayChildConnection.h"
#include "GatewayChildSession.h"
#include "IGatewayChildRequestHandler.h"

namespace blinker {

enum class GatewayChildFrameSource : uint8_t {
    Event = 0U,
    PendingResponse = 1U,
    RecentResponse = 2U
};

enum GatewayChildDemandSource : uint8_t {
    GatewayChildDemandCloud = 1U << 0U,
    GatewayChildDemandLocal = 1U << 1U
};

typedef void (*GatewayChildExecutorFrameReceiver)(
    void* context, ByteView frame, GatewayChildFrameSource source);
typedef void (*GatewayChildExecutorSessionReceiver)(void* context);
typedef void (*GatewayChildExecutorFaultReceiver)(
    void* context, ErrorCode error);
typedef void (*GatewayChildControlReceiver)(void* context, ByteView frame);

// Sole owner of one child's southbound BBP/2 lane. Upstream adapters keep
// their own authorization and correlation, but all of them share this one
// sequence allocator, one in-flight exchange and one DirectSecure session.
template <typename ChildSession>
class BasicGatewayChildExecutor {
public:
    BasicGatewayChildExecutor(
        ChildSession& child,
        IGatewayChildConnection& connection)
        : child_(child), connection_(connection), requestHandler_(nullptr),
          observer_(nullptr),
          observerSecure_(nullptr), observerDisconnected_(nullptr),
          observerFault_(nullptr), observerContext_(nullptr),
          pendingOwner_(nullptr), pendingReceiver_(nullptr),
          recentOwner_(nullptr), recentReceiver_(nullptr),
          controlReceiver_(nullptr), controlReceiverContext_(nullptr),
          nextSequence_(1U), pendingSequence_(0U), recentSequence_(0U),
          controlSequence_(0U), pendingKind_(0U), recentKind_(0U),
          controlResponseKind_(0U), demandSources_(0U), pending_(false),
          recent_(false), controlPending_(false) {
        child_.setReceiver(&BasicGatewayChildExecutor::childThunk, this);
        child_.setSessionHandlers(
            &BasicGatewayChildExecutor::secureThunk,
            &BasicGatewayChildExecutor::disconnectedThunk,
            this);
        child_.setFaultHandler(
            &BasicGatewayChildExecutor::faultThunk, this);
    }

    ~BasicGatewayChildExecutor() {
        child_.setReceiver(nullptr, nullptr);
        child_.setSessionHandlers(nullptr, nullptr, nullptr);
        child_.setFaultHandler(nullptr, nullptr);
        clearAll();
        clearObserver();
        controlReceiver_ = nullptr;
        controlReceiverContext_ = nullptr;
    }

    void setObserver(
        GatewayChildExecutorFrameReceiver frame,
        GatewayChildExecutorSessionReceiver secure,
        GatewayChildExecutorSessionReceiver disconnected,
        GatewayChildExecutorFaultReceiver fault,
        void* context) {
        observer_ = frame;
        observerSecure_ = secure;
        observerDisconnected_ = disconnected;
        observerFault_ = fault;
        observerContext_ = context;
    }

    void clearObserver(void* context = nullptr) {
        if (context != nullptr && observerContext_ != context) return;
        observer_ = nullptr;
        observerSecure_ = nullptr;
        observerDisconnected_ = nullptr;
        observerFault_ = nullptr;
        observerContext_ = nullptr;
    }

    bool secure() const { return child_.secure(); }
    // No live-session mutation. An absent module advertises no request feature.
    Result setRequestHandler(IGatewayChildRequestHandler* handler) {
        const Result result = child_.setRequestFeatures(
            handler == nullptr ? 0U : handler->features());
        if (!result) return result;
        if (requestHandler_ != nullptr && requestHandler_ != handler)
            requestHandler_->reset();
        requestHandler_ = handler;
        if (requestHandler_ != nullptr) requestHandler_->reset();
        return Result::success();
    }
    uint32_t attemptId() const { return child_.attemptId(); }
    uint32_t authenticatedControllerCredentialVersion() const {
        return child_.authenticatedCredentialVersion();
    }
    uint32_t authenticatedControllerSessionAttemptId() const {
        return child_.secure() ? child_.attemptId() : 0U;
    }
    uint32_t negotiatedFeatures() const {
        return child_.negotiatedFeatures();
    }
    uint16_t negotiatedMaxFrameSize() const {
        return child_.negotiatedMaxFrameSize();
    }
    uint8_t remoteReliableReceiveWindow() const {
        return child_.remoteReliableReceiveWindow();
    }
    uint32_t manifestRevision() const {
        return child_.manifestRevision();
    }
    ByteView manifestFingerprint() const {
        return child_.manifestFingerprint();
    }

    void setExecutionDemand(uint8_t source, bool wanted) {
        if (wanted) demandSources_ = static_cast<uint8_t>(demandSources_ | source);
        else demandSources_ = static_cast<uint8_t>(demandSources_ & ~source);
    }
    bool executionWanted() const { return demandSources_ != 0U; }

    Result sendDataFrame(
        void* owner,
        GatewayChildExecutorFrameReceiver receiver,
        ByteView encoded) {
        if (owner == nullptr || receiver == nullptr || !child_.secure()) {
            return Result::failure(
                child_.secure() ? ErrorCode::InvalidArgument
                                : ErrorCode::NotConnected);
        }
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(encoded, frame);
        if (!result) return result;
        const bool expects = expectsResponse(frame);
        if (controlPending_ || (expects && pending_)) {
            return Result::failure(ErrorCode::StateConflict);
        }
        ByteView outbound = encoded;
        uint16_t sequence = 0U;
        if (expects) {
            sequence = nextSequence_;
            result = rewriteSequence(encoded, sequence, outbound);
        }
        if (result) result = child_.sendFrame(outbound);
        clearScratch();
        if (!result) return result;
        if (expects) {
            allocateSequence();
            pendingOwner_ = owner;
            pendingReceiver_ = receiver;
            pendingKind_ = frame.header.kind;
            pendingSequence_ = sequence;
            pending_ = true;
        }
        return Result::success();
    }

    Result replayDataFrame(void* owner, ByteView encoded, bool recent) {
        if (!child_.secure() || owner == nullptr) {
            return Result::failure(
                child_.secure() ? ErrorCode::InvalidArgument
                                : ErrorCode::NotConnected);
        }
        if (controlPending_) return Result::failure(ErrorCode::StateConflict);
        const bool matches = recent
            ? recent_ && recentOwner_ == owner
            : pending_ && pendingOwner_ == owner;
        if (!matches) return Result::failure(ErrorCode::StateConflict);
        ByteView outbound;
        Result result = rewriteSequence(
            encoded, recent ? recentSequence_ : pendingSequence_, outbound);
        if (result) result = child_.sendFrame(outbound);
        clearScratch();
        return result;
    }

    // Translate a southbound response back to an upstream sequence in the
    // executor's single shared workspace. The caller must consume the view
    // before invoking another executor operation.
    Result mapResponseSequence(
        ByteView encoded,
        uint16_t upstreamSequence,
        ByteView& rewritten) {
        if (encoded.data == nullptr || upstreamSequence == 0U ||
            encoded.size > sizeof(scratch_)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(encoded, frame);
        if (!result) return result;
        using bbp2::MessageKind;
        const MessageKind kind =
            static_cast<MessageKind>(frame.header.kind);
        if (kind == MessageKind::Hello || kind == MessageKind::Manifest ||
            kind == MessageKind::StatePage) {
            return rewriteSequence(encoded, upstreamSequence, rewritten);
        }
        bbp2::FrameHeader header = frame.header;
        ByteView body;
        result = Result::failure(ErrorCode::UnsupportedFeature);
        if (kind == MessageKind::Ack) {
            bbp2::AckBody ack;
            result = bbp2::decodeAckBody(frame.body, ack);
            if (result) {
                ack.acknowledgedSequence = upstreamSequence;
                result = bbp2::encodeAckBody(
                    ack,
                    MutableByteSpan(
                        scratch_ + bbp2::kBaseHeaderSize,
                        sizeof(scratch_) - bbp2::kBaseHeaderSize),
                    body);
            }
        } else if (kind == MessageKind::Error) {
            bbp2::ErrorBody error;
            result = bbp2::decodeErrorBody(frame.body, error);
            if (result) {
                error.relatedSequence = upstreamSequence;
                error.hasRelatedSequence = true;
                result = bbp2::encodeErrorBody(
                    error,
                    MutableByteSpan(
                        scratch_ + bbp2::kBaseHeaderSize,
                        sizeof(scratch_) - bbp2::kBaseHeaderSize),
                    body);
            }
        }
        if (!result) return result;
        header.bodyLength = static_cast<uint16_t>(body.size);
        result = bbp2::encodeHeader(
            header, MutableByteSpan(scratch_, sizeof(scratch_)));
        if (!result) return result;
        rewritten = ByteView(
            scratch_, bbp2::kBaseHeaderSize + body.size);
        return Result::success();
    }

    void completeDataRequest(void* owner, bool replayable) {
        if (!pending_ || pendingOwner_ != owner) return;
        clearRecent();
        if (replayable) {
            recentOwner_ = pendingOwner_;
            recentReceiver_ = pendingReceiver_;
            recentKind_ = pendingKind_;
            recentSequence_ = pendingSequence_;
            recent_ = true;
        }
        clearPending();
    }

    void cancelDataOwner(void* owner) {
        if (pending_ && pendingOwner_ == owner) clearPending();
        if (recent_ && recentOwner_ == owner) clearRecent();
    }

    void setControllerControlReceiver(
        GatewayChildControlReceiver receiver,
        void* context) {
        controlReceiver_ = receiver;
        controlReceiverContext_ = context;
    }
    bool controllerControlPending() const { return controlPending_; }
    Result ensureControllerControlSession() {
        return connection_.ensureConnected();
    }
    void cancelControllerControlRequest() {
        if (!controlPending_) return;
        clearControl();
        (void)connection_.retire(child_.attemptId());
    }
    bool retireControllerControlSession(uint32_t expectedAttemptId) {
        if (expectedAttemptId == 0U || child_.attemptId() != expectedAttemptId) {
            return true;
        }
        clearControl();
        return connection_.retire(expectedAttemptId);
    }

    Result sendControllerControlRequest(
        bbp2::MessageKind kind,
        ByteView body) {
        const bbp2::MessageKind expected =
            kind == bbp2::MessageKind::ControllerControlOpen
                ? bbp2::MessageKind::ControllerControlChallenge
                : bbp2::MessageKind::ControllerMutationReceipt;
        if ((kind != bbp2::MessageKind::ControllerControlOpen &&
             kind != bbp2::MessageKind::ControllerMutation) ||
            body.data == nullptr || controlReceiver_ == nullptr) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (pending_ || controlPending_) {
            return Result::failure(ErrorCode::StateConflict);
        }
        Result result = ensureControllerControlSession();
        if (!result) return result;
        if ((child_.negotiatedFeatures() &
             bbp2::FeatureControllerControl) == 0U) {
            return Result::failure(ErrorCode::UnsupportedFeature);
        }
        const uint16_t maximum = child_.negotiatedMaxFrameSize();
        if (maximum < bbp2::kBaseHeaderSize ||
            body.size > sizeof(scratch_) - bbp2::kBaseHeaderSize ||
            body.size > static_cast<size_t>(maximum) - bbp2::kBaseHeaderSize) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        const uint16_t sequence = nextSequence_;
        bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(kind);
        header.flags = bbp2::FlagNone;
        header.sequence = sequence;
        header.bodyLength = static_cast<uint16_t>(body.size);
        result = bbp2::encodeHeader(
            header, MutableByteSpan(scratch_, sizeof(scratch_)));
        if (result && !body.empty()) {
            memcpy(scratch_ + bbp2::kBaseHeaderSize, body.data, body.size);
        }
        if (result) {
            result = child_.sendFrame(ByteView(
                scratch_, bbp2::kBaseHeaderSize + body.size));
        }
        clearScratch();
        if (!result) return result;
        allocateSequence();
        controlResponseKind_ = static_cast<uint8_t>(expected);
        controlSequence_ = sequence;
        controlPending_ = true;
        return Result::success();
    }

    void retire(ErrorCode error) {
        clearPending();
        clearRecent();
        clearControl();
        if (observerFault_ != nullptr) {
            observerFault_(observerContext_, error);
        }
        (void)connection_.retire(child_.attemptId());
    }

private:
    static bool expectsResponse(const bbp2::FrameView& frame) {
        if ((frame.header.flags & bbp2::FlagIsResponse) != 0U ||
            frame.header.sequence == 0U) return false;
        using bbp2::MessageKind;
        const MessageKind kind = static_cast<MessageKind>(frame.header.kind);
        return kind == MessageKind::Hello ||
               kind == MessageKind::ManifestRequest ||
               kind == MessageKind::ManifestAccept ||
               kind == MessageKind::StateRequest ||
               kind == MessageKind::Command;
    }

    static bool matchesResponse(
        const bbp2::FrameView& frame,
        uint8_t requestKind,
        uint16_t sequence) {
        if (sequence == 0U ||
            (frame.header.flags & bbp2::FlagIsResponse) == 0U) return false;
        using bbp2::MessageKind;
        const MessageKind request = static_cast<MessageKind>(requestKind);
        const MessageKind response =
            static_cast<MessageKind>(frame.header.kind);
        if ((request == MessageKind::Hello && response == MessageKind::Hello) ||
            (request == MessageKind::ManifestRequest && response == MessageKind::Manifest) ||
            (request == MessageKind::StateRequest && response == MessageKind::StatePage)) {
            return frame.header.sequence == sequence;
        }
        if (response == MessageKind::Ack) {
            bbp2::AckBody body;
            return bbp2::decodeAckBody(frame.body, body) &&
                   body.acknowledgedSequence == sequence;
        }
        if (response == MessageKind::Error) {
            bbp2::ErrorBody body;
            return bbp2::decodeErrorBody(frame.body, body) &&
                   body.hasRelatedSequence && body.relatedSequence == sequence;
        }
        return false;
    }

    Result rewriteSequence(
        ByteView encoded,
        uint16_t sequence,
        ByteView& rewritten) {
        if (encoded.data == nullptr || sequence == 0U ||
            encoded.size > sizeof(scratch_)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(encoded, frame);
        if (!result) return result;
        memcpy(scratch_, encoded.data, encoded.size);
        scratch_[6] = static_cast<uint8_t>(sequence >> 8U);
        scratch_[7] = static_cast<uint8_t>(sequence & 0xFFU);
        rewritten = ByteView(scratch_, encoded.size);
        return Result::success();
    }

    void onChildFrame(ByteView encoded) {
        if (!child_.secure()) return;
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(encoded, frame);
        if (!result) {
            retire(result.code());
            return;
        }
        IGatewayChildRequestHandler* handler = requestHandler_;
        if (handler != nullptr && handler->handles(frame.header.kind)) {
            const GatewayChildRequestContext peer = {
                child_.attemptId(), child_.authenticatedPermissions(),
                child_.negotiatedFeatures(), child_.monotonicMillis()
            };
            const uint16_t maximum = child_.negotiatedMaxFrameSize();
            const size_t capacity = maximum < sizeof(scratch_)
                ? maximum : sizeof(scratch_);
            ByteView response;
            result = handler->reply(
                peer, frame, MutableByteSpan(scratch_, capacity), response);
            // A synchronous module/source may reenter lifecycle code. Never
            // send an old response on a replaced, expired or retiring session.
            if (result && !response.empty() && response.data == scratch_ &&
                response.size <= capacity && requestHandler_ == handler &&
                child_.secure() && child_.attemptId() == peer.attemptId &&
                child_.authenticatedPermissions() == peer.permissions &&
                child_.negotiatedFeatures() == peer.features) {
                // Existing exact-ciphertext slot; no retry/second queue here.
                (void)child_.sendFrame(response);
            }
            clearScratch();
            return; // Does not consume the ordinary pending/recent/management sequence.
        }
        // Time is a link-local exchange, never a Cloud/LAN upstream event,
        // including an unsolicited request when the optional module is absent.
        if (frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::TimeRequest) ||
            frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::TimeResponse)) return;
        if (controlPending_ &&
            (frame.header.flags & bbp2::FlagIsResponse) != 0U &&
            frame.header.sequence == controlSequence_ &&
            (frame.header.kind == controlResponseKind_ ||
             frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::Error))) {
            clearControl();
            controlReceiver_(controlReceiverContext_, encoded);
            return;
        }
        if (pending_ && matchesResponse(frame, pendingKind_, pendingSequence_)) {
            pendingReceiver_(pendingOwner_, encoded,
                             GatewayChildFrameSource::PendingResponse);
            return;
        }
        if (recent_ && matchesResponse(frame, recentKind_, recentSequence_)) {
            recentReceiver_(recentOwner_, encoded,
                            GatewayChildFrameSource::RecentResponse);
            return;
        }
        const bbp2::MessageKind kind =
            static_cast<bbp2::MessageKind>(frame.header.kind);
        if (kind == bbp2::MessageKind::Ack ||
            kind == bbp2::MessageKind::Error) return;
        if (observer_ != nullptr) {
            observer_(observerContext_, encoded, GatewayChildFrameSource::Event);
        }
    }

    uint16_t allocateSequence() {
        const uint16_t current = nextSequence_;
        nextSequence_ = current == UINT16_MAX
            ? 1U : static_cast<uint16_t>(current + 1U);
        return current;
    }
    void clearPending() {
        pending_ = false;
        pendingOwner_ = nullptr;
        pendingReceiver_ = nullptr;
        pendingKind_ = 0U;
        pendingSequence_ = 0U;
    }
    void clearRecent() {
        recent_ = false;
        recentOwner_ = nullptr;
        recentReceiver_ = nullptr;
        recentKind_ = 0U;
        recentSequence_ = 0U;
    }
    void clearControl() {
        controlPending_ = false;
        controlResponseKind_ = 0U;
        controlSequence_ = 0U;
    }
    void clearScratch() {
        secureZero(MutableByteSpan(scratch_, sizeof(scratch_)));
    }
    void clearAll() {
        clearPending();
        clearRecent();
        clearControl();
        demandSources_ = 0U;
        clearScratch();
    }

    static void childThunk(void* context, ByteView frame) {
        BasicGatewayChildExecutor* self =
            static_cast<BasicGatewayChildExecutor*>(context);
        if (self != nullptr) self->onChildFrame(frame);
    }
    static void secureThunk(void* context) {
        BasicGatewayChildExecutor* self =
            static_cast<BasicGatewayChildExecutor*>(context);
        if (self == nullptr || !self->child_.secure()) return;
        // A full bearer stop deliberately releases callbacks before shutdown,
        // so it need not report disconnect. A NEW successful Method 2 session
        // also starts fresh module state; no live reconfiguration/reset API.
        if (self->requestHandler_ != nullptr) self->requestHandler_->reset();
        if (self->observerSecure_ != nullptr) {
            self->observerSecure_(self->observerContext_);
        }
    }
    static void disconnectedThunk(void* context) {
        BasicGatewayChildExecutor* self =
            static_cast<BasicGatewayChildExecutor*>(context);
        if (self == nullptr) return;
        self->clearPending();
        self->clearRecent();
        self->clearControl();
        self->nextSequence_ = 1U;
        if (self->requestHandler_ != nullptr) self->requestHandler_->reset();
        if (self->observerDisconnected_ != nullptr) {
            self->observerDisconnected_(self->observerContext_);
        }
    }
    static void faultThunk(void* context, ErrorCode error) {
        BasicGatewayChildExecutor* self =
            static_cast<BasicGatewayChildExecutor*>(context);
        if (self != nullptr && self->observerFault_ != nullptr) {
            self->observerFault_(self->observerContext_, error);
        }
    }

    ChildSession& child_;
    IGatewayChildConnection& connection_;
    IGatewayChildRequestHandler* requestHandler_;
    GatewayChildExecutorFrameReceiver observer_;
    GatewayChildExecutorSessionReceiver observerSecure_;
    GatewayChildExecutorSessionReceiver observerDisconnected_;
    GatewayChildExecutorFaultReceiver observerFault_;
    void* observerContext_;
    void* pendingOwner_;
    GatewayChildExecutorFrameReceiver pendingReceiver_;
    void* recentOwner_;
    GatewayChildExecutorFrameReceiver recentReceiver_;
    GatewayChildControlReceiver controlReceiver_;
    void* controlReceiverContext_;
    uint8_t scratch_[gateway::kEdgeHubChildMaximumInnerFrameSize];
    uint16_t nextSequence_;
    uint16_t pendingSequence_;
    uint16_t recentSequence_;
    uint16_t controlSequence_;
    uint8_t pendingKind_;
    uint8_t recentKind_;
    uint8_t controlResponseKind_;
    uint8_t demandSources_;
    bool pending_;
    bool recent_;
    bool controlPending_;

    BasicGatewayChildExecutor(const BasicGatewayChildExecutor&);
    BasicGatewayChildExecutor& operator=(const BasicGatewayChildExecutor&);
};

} // namespace blinker

#endif
