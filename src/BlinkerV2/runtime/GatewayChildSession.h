#ifndef BLINKER_RUNTIME_GATEWAYCHILDSESSION_H
#define BLINKER_RUNTIME_GATEWAYCHILDSESSION_H

#include "../core/SecureMemory.h"
#include "../identity/GatewayAccessStore.h"
#include "../interface/IClock.h"
#include "../interface/IRandom.h"
#include "../interface/IX25519AesGcmCryptoProvider.h"
#include "../protocol/DirectRecord.h"
#include "../transport/IFrameTransport.h"
#include "ChildSessionCoordinator.h"
#include "GatewaySessionAccessSource.h"

namespace blinker {

enum class GatewayChildSessionState : uint8_t {
    Stopped = 0U,
    Idle,
    Connecting,
    Authenticating,
    Secure,
    Disconnecting,
    Fault
};

struct GatewayChildSessionConfig {
    uint16_t maxFrameSize;
    uint32_t authenticationTimeoutMillis;

    GatewayChildSessionConfig()
        : maxFrameSize(512U), authenticationTimeoutMillis(15000U) {}
};

typedef void (*GatewayChildFrameReceiver)(
    void* context,
    ByteView frame);
typedef void (*GatewayChildSessionHandler)(void* context);
typedef void (*GatewayChildSessionFaultHandler)(
    void* context,
    ErrorCode error);

// Blinker-native one-child secure session. Link owns bearer packetization;
// Selector owns bearer-specific discovery. This class owns Method 2 and
// DirectSecure only, and deliberately does not model Zigbee/BLE Mesh security,
// cloud topology, routing policy or retry queues.
template <typename Link, typename Selector>
class BasicGatewayChildSession {
public:
    BasicGatewayChildSession(
        Link& link,
        GatewayAccessStore& access,
        IClock& clock,
        IRandom& random,
        IX25519AesGcmCryptoProvider& crypto,
        MutableByteSpan handshakeScratch,
        MutableByteSpan plaintextScratch,
        MutableByteSpan secureRecordScratch,
        const GatewayChildSessionConfig& config =
            GatewayChildSessionConfig())
        : link_(link), access_(access), clock_(clock), crypto_(crypto),
          config_(config), coordinator_(random, childConfig(config)),
          selector_(), credential_(),
          handshakeScratch_(handshakeScratch.data),
          plaintextScratch_(plaintextScratch.data),
          secureRecordScratch_(secureRecordScratch.data),
          handshakeCapacity_(boundedSize(handshakeScratch.size)),
          plaintextCapacity_(boundedSize(plaintextScratch.size)),
          secureRecordCapacity_(boundedSize(secureRecordScratch.size)),
          pendingSecureRecordSize_(0U),
          receiver_(nullptr), receiverContext_(nullptr),
          secureHandler_(nullptr), disconnectedHandler_(nullptr),
          sessionContext_(nullptr), faultHandler_(nullptr),
          faultContext_(nullptr), activeAttemptId_(0U),
          expiresAtUnixSeconds_(0U), credentialVersion_(0U),
          pendingError_(ErrorCode::Ok), lastError_(ErrorCode::Ok),
          state_(GatewayChildSessionState::Stopped),
          faultReported_(false) {
        bindLink();
    }

    BasicGatewayChildSession(
        Link& link,
        GatewayAccessStore& access,
        GatewayCredentialRenewalStore& renewal,
        IClock& clock,
        IRandom& random,
        IX25519AesGcmCryptoProvider& crypto,
        MutableByteSpan handshakeScratch,
        MutableByteSpan plaintextScratch,
        MutableByteSpan secureRecordScratch,
        const GatewayChildSessionConfig& config =
            GatewayChildSessionConfig())
        : link_(link), access_(access, renewal), clock_(clock), crypto_(crypto),
          config_(config), coordinator_(random, childConfig(config)),
          selector_(), credential_(),
          handshakeScratch_(handshakeScratch.data),
          plaintextScratch_(plaintextScratch.data),
          secureRecordScratch_(secureRecordScratch.data),
          handshakeCapacity_(boundedSize(handshakeScratch.size)),
          plaintextCapacity_(boundedSize(plaintextScratch.size)),
          secureRecordCapacity_(boundedSize(secureRecordScratch.size)),
          pendingSecureRecordSize_(0U),
          receiver_(nullptr), receiverContext_(nullptr),
          secureHandler_(nullptr), disconnectedHandler_(nullptr),
          sessionContext_(nullptr), faultHandler_(nullptr),
          faultContext_(nullptr), activeAttemptId_(0U),
          expiresAtUnixSeconds_(0U), credentialVersion_(0U),
          pendingError_(ErrorCode::Ok), lastError_(ErrorCode::Ok),
          state_(GatewayChildSessionState::Stopped),
          faultReported_(false) {
        bindLink();
    }

    ~BasicGatewayChildSession() { stop(); }

    Result start() {
        if (state_ != GatewayChildSessionState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (!validConfig()) {
            state_ = GatewayChildSessionState::Fault;
            lastError_ = ErrorCode::NotConfigured;
            return Result::failure(lastError_);
        }
        Result result = link_.start();
        if (!result) {
            state_ = GatewayChildSessionState::Fault;
            lastError_ = result.code();
            return result;
        }
        state_ = GatewayChildSessionState::Idle;
        lastError_ = ErrorCode::Ok;
        return Result::success();
    }

    void stop() {
        if (state_ == GatewayChildSessionState::Stopped) return;
        link_.stop();
        clearAttempt();
        access_.reset();
        state_ = GatewayChildSessionState::Stopped;
        lastError_ = ErrorCode::Ok;
    }

    void poll(uint32_t budgetMicros) {
        if (state_ == GatewayChildSessionState::Stopped ||
            state_ == GatewayChildSessionState::Fault) {
            return;
        }
        if (activeAttemptId_ != 0U &&
            (state_ == GatewayChildSessionState::Connecting ||
             state_ == GatewayChildSessionState::Authenticating ||
             state_ == GatewayChildSessionState::Secure) &&
            !accessTimeValid()) {
            pendingError_ = ErrorCode::AuthenticationRequired;
            processFailure();
            return;
        }
        link_.poll(budgetMicros);
        if (pendingError_ != ErrorCode::Ok) {
            processFailure();
            return;
        }
        if (state_ == GatewayChildSessionState::Secure &&
            pendingSecureRecordSize_ != 0U) {
            const Result result = link_.sendRecord(ByteView(
                secureRecordScratch_, pendingSecureRecordSize_));
            if (result) {
                secureZero(MutableByteSpan(
                    secureRecordScratch_, pendingSecureRecordSize_));
                pendingSecureRecordSize_ = 0U;
            } else if (result.code() != ErrorCode::WouldBlock) {
                pendingError_ = result.code();
                processFailure();
                return;
            }
        }
        if (state_ == GatewayChildSessionState::Authenticating) {
            Result result = coordinator_.poll(clock_.monotonicMillis());
            if (!result) {
                pendingError_ = result.code();
                processFailure();
            }
        }
    }

    Result connect() {
        if (state_ != GatewayChildSessionState::Idle) {
            return Result::failure(
                state_ == GatewayChildSessionState::Stopped
                    ? ErrorCode::NotConnected
                    : ErrorCode::AlreadyExists);
        }
        if (access_.protection() == StorageProtection::PlainFlash) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        uint64_t unixSeconds = 0U;
        if (!clock_.unixTime(unixSeconds)) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        GatewayAccessRecord access;
        Result result = access_.load(access);
        if (result) result = selector_.configure(access);
        if (result) {
            result = makeGatewayControllerCredential(
                access, unixSeconds, credential_);
        }
        if (result) expiresAtUnixSeconds_ = access.expiresAtUnixSeconds;
        if (result) credentialVersion_ = access.credentialVersion;
        clearGatewayAccessRecord(access);
        if (!result) {
            resetSecurity();
            lastError_ = result.code();
            return result;
        }
        result = selector_.connect(link_);
        if (!result) {
            resetSecurity();
            lastError_ = result.code();
            return result;
        }
        state_ = GatewayChildSessionState::Connecting;
        activeAttemptId_ = link_.attemptId();
        pendingError_ = ErrorCode::Ok;
        lastError_ = ErrorCode::Ok;
        faultReported_ = false;
        return Result::success();
    }

    Result disconnect() {
        if (state_ != GatewayChildSessionState::Connecting &&
            state_ != GatewayChildSessionState::Authenticating &&
            state_ != GatewayChildSessionState::Secure) {
            return Result::failure(ErrorCode::NotFound);
        }
        const GatewayChildSessionState previous = state_;
        // The link is allowed to report disconnection synchronously. Publish
        // the transitional state first so that callback-owned Idle is not
        // overwritten after disconnect() returns.
        state_ = GatewayChildSessionState::Disconnecting;
        Result result = link_.disconnect();
        if (!result && state_ == GatewayChildSessionState::Disconnecting) {
            state_ = previous;
        }
        return result;
    }

    Result sendFrame(ByteView frame) {
        if (state_ != GatewayChildSessionState::Secure ||
            !coordinator_.secure()) {
            return Result::failure(ErrorCode::AuthenticationRequired);
        }
        if (!accessTimeValid()) {
            pendingError_ = ErrorCode::AuthenticationRequired;
            return Result::failure(pendingError_);
        }
        if (pendingSecureRecordSize_ != 0U) {
            return Result::failure(ErrorCode::WouldBlock);
        }
        Result result = direct::validateRecord(frame);
        if (!result || frame.size > coordinator_.negotiatedMaxFrameSize()) {
            return result
                       ? Result::failure(ErrorCode::CapacityExceeded)
                       : result;
        }
        ByteView record;
        result = coordinator_.secureSession().seal(
            crypto_, frame,
            MutableByteSpan(
                secureRecordScratch_, secureRecordCapacity_),
            record);
        if (result) result = link_.sendRecord(record);
        if (result) {
            secureZero(MutableByteSpan(
                secureRecordScratch_, record.size));
        } else if (result.code() == ErrorCode::WouldBlock) {
            // seal() has already consumed a DirectSecure sequence. Retain the
            // exact ciphertext in the existing scratch slot and enqueue it
            // from poll(); re-sealing after queue pressure would create an
            // observable sequence gap at the child.
            pendingSecureRecordSize_ = static_cast<uint16_t>(record.size);
            return Result::success();
        } else {
            // A sealed sequence cannot be retried after an uncertain bearer
            // enqueue. Retire this session and let the cloud ledger retry on a
            // fresh authenticated connection.
            pendingError_ = result.code();
        }
        return result;
    }

    Result activateMatchedPending(uint32_t& storageRevision) {
        storageRevision = 0U;
        if (state_ != GatewayChildSessionState::Secure ||
            !selector_.matchedPending()) {
            return Result::failure(ErrorCode::StateConflict);
        }
        if (!accessTimeValid()) {
            pendingError_ = ErrorCode::AuthenticationRequired;
            return Result::failure(pendingError_);
        }
        return access_.activatePendingPresence(
            selector_.matchedPresenceVersion(), storageRevision);
    }

    GatewayChildSessionState state() const { return state_; }
    ErrorCode lastError() const { return lastError_; }
    bool secure() const {
        return state_ == GatewayChildSessionState::Secure &&
               coordinator_.secure();
    }
    uint32_t attemptId() const { return activeAttemptId_; }
    uint32_t matchedPresenceVersion() const {
        return selector_.matchedPresenceVersion();
    }
    // The durable Gateway credential version captured before Method 2 began.
    // A caller must not infer this fact from the mutable access store: Rotate
    // can promote that store while the old DirectSecure session is alive.
    uint32_t authenticatedCredentialVersion() const {
        return secure() ? credentialVersion_ : 0U;
    }
    bool matchedPendingPresence() const {
        return selector_.matchedPending();
    }
    uint32_t manifestRevision() const {
        return coordinator_.manifestRevision();
    }
    uint32_t negotiatedFeatures() const {
        return coordinator_.negotiatedFeatures();
    }
    uint16_t negotiatedMaxFrameSize() const {
        return coordinator_.negotiatedMaxFrameSize();
    }
    uint8_t remoteReliableReceiveWindow() const {
        return coordinator_.remoteReliableReceiveWindow();
    }
    ByteView manifestFingerprint() const {
        return coordinator_.manifestFingerprint();
    }

    void setReceiver(
        GatewayChildFrameReceiver receiver,
        void* context) {
        receiver_ = receiver;
        receiverContext_ = context;
    }
    void setSessionHandlers(
        GatewayChildSessionHandler secure,
        GatewayChildSessionHandler disconnected,
        void* context) {
        secureHandler_ = secure;
        disconnectedHandler_ = disconnected;
        sessionContext_ = context;
    }
    void setFaultHandler(
        GatewayChildSessionFaultHandler handler,
        void* context) {
        faultHandler_ = handler;
        faultContext_ = context;
    }

private:
    void bindLink() {
        link_.setReceiver(&BasicGatewayChildSession::recordThunk, this);
        link_.setSessionHandlers(
            &BasicGatewayChildSession::connectedThunk,
            &BasicGatewayChildSession::disconnectedThunk,
            this);
        link_.setFaultHandler(
            &BasicGatewayChildSession::faultThunk, this);
    }

    static uint16_t boundedSize(size_t value) {
        return value <= UINT16_MAX ? static_cast<uint16_t>(value) : 0U;
    }

    static ChildSessionConfig childConfig(
        const GatewayChildSessionConfig& config) {
        ChildSessionConfig result;
        // Gateway detach/rotate uses the authenticated child session's
        // private management side channel. Ordinary Direct clients do not
        // need to request this optional capability.
        result.features |= bbp2::FeatureControllerControl;
        result.maxFrameSize = config.maxFrameSize;
        result.authenticationTimeoutMillis =
            config.authenticationTimeoutMillis;
        return result;
    }

    bool validConfig() const {
        const size_t secureSize = security::directSecureRecordSize(
            config_.maxFrameSize);
        return config_.maxFrameSize >= kChildSessionMinimumFrameSize &&
               config_.authenticationTimeoutMillis != 0U &&
               handshakeScratch_ != nullptr &&
               handshakeCapacity_ >= config_.maxFrameSize &&
               plaintextScratch_ != nullptr &&
               plaintextCapacity_ >= config_.maxFrameSize &&
               secureRecordScratch_ != nullptr && secureSize != 0U &&
               secureRecordCapacity_ >= secureSize;
    }

    static void recordThunk(
        void* context,
        ByteView record,
        const RxContext&) {
        BasicGatewayChildSession* self =
            static_cast<BasicGatewayChildSession*>(context);
        if (self != nullptr) self->onRecord(record);
    }
    static void connectedThunk(void* context, const RxContext& rx) {
        BasicGatewayChildSession* self =
            static_cast<BasicGatewayChildSession*>(context);
        if (self != nullptr) self->onConnected(rx);
    }
    static void disconnectedThunk(void* context, const RxContext&) {
        BasicGatewayChildSession* self =
            static_cast<BasicGatewayChildSession*>(context);
        if (self != nullptr) self->onDisconnected();
    }
    static void faultThunk(
        void* context,
        ErrorCode error,
        uint32_t attemptId) {
        BasicGatewayChildSession* self =
            static_cast<BasicGatewayChildSession*>(context);
        if (self != nullptr &&
            (self->activeAttemptId_ == 0U ||
             attemptId == 0U ||
             attemptId == self->activeAttemptId_)) {
            self->pendingError_ = error == ErrorCode::Ok
                                      ? ErrorCode::InternalError
                                      : error;
        }
    }

    void onConnected(const RxContext& rx) {
        if (state_ != GatewayChildSessionState::Connecting ||
            rx.sessionId == 0U || rx.sessionId != activeAttemptId_) {
            pendingError_ = ErrorCode::ProtocolError;
            return;
        }
        ByteView hello;
        Result result = coordinator_.begin(
            credential_, clock_.monotonicMillis(),
            MutableByteSpan(handshakeScratch_, handshakeCapacity_),
            hello);
        clearControllerCredential(credential_);
        if (result) result = link_.sendRecord(hello);
        if (!result) {
            pendingError_ = result.code();
            return;
        }
        state_ = GatewayChildSessionState::Authenticating;
    }

    void onRecord(ByteView record) {
        if (state_ == GatewayChildSessionState::Authenticating) {
            ByteView response;
            Result result = coordinator_.handleFrame(
                record,
                clock_.monotonicMillis(),
                MutableByteSpan(handshakeScratch_, handshakeCapacity_),
                response);
            if (result && !response.empty()) {
                result = link_.sendRecord(response);
            }
            if (!result) {
                pendingError_ = result.code();
                return;
            }
            if (coordinator_.secure()) {
                if (!accessTimeValid()) {
                    pendingError_ = ErrorCode::AuthenticationRequired;
                    return;
                }
                state_ = GatewayChildSessionState::Secure;
                if (secureHandler_ != nullptr) {
                    secureHandler_(sessionContext_);
                }
            }
            return;
        }
        if (state_ != GatewayChildSessionState::Secure ||
            !coordinator_.secure()) {
            pendingError_ = ErrorCode::SequenceConflict;
            return;
        }
        ByteView frame;
        Result result = coordinator_.secureSession().open(
            crypto_, record,
            MutableByteSpan(plaintextScratch_, plaintextCapacity_),
            frame);
        if (result) result = direct::validateRecord(frame);
        if (!result || frame.size > coordinator_.negotiatedMaxFrameSize()) {
            pendingError_ = result
                                ? ErrorCode::CapacityExceeded
                                : result.code();
            return;
        }
        if (receiver_ != nullptr) receiver_(receiverContext_, frame);
    }

    void onDisconnected() {
        const bool notify = activeAttemptId_ != 0U;
        clearAttempt();
        if (state_ != GatewayChildSessionState::Stopped) {
            state_ = GatewayChildSessionState::Idle;
        }
        if (notify && disconnectedHandler_ != nullptr) {
            disconnectedHandler_(sessionContext_);
        }
    }

    void processFailure() {
        const ErrorCode error = pendingError_;
        pendingError_ = ErrorCode::Ok;
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        coordinator_.reset();
        clearControllerCredential(credential_);
        if (!faultReported_ && faultHandler_ != nullptr) {
            faultReported_ = true;
            faultHandler_(faultContext_, lastError_);
        }
        if (error == ErrorCode::AuthenticationRequired) {
            access_.authenticationFailed();
        }
        if (link_.connected() || link_.connecting()) {
            state_ = GatewayChildSessionState::Disconnecting;
            const Result result = link_.disconnect();
            if (result) {
                return;
            }
        }
        clearAttempt();
        state_ = GatewayChildSessionState::Idle;
    }

    void resetSecurity() {
        coordinator_.reset();
        selector_.clear();
        clearControllerCredential(credential_);
        expiresAtUnixSeconds_ = 0U;
        credentialVersion_ = 0U;
    }

    bool accessTimeValid() const {
        uint64_t unixSeconds = 0U;
        return expiresAtUnixSeconds_ != 0U &&
               clock_.unixTime(unixSeconds) && unixSeconds != 0U &&
               unixSeconds < expiresAtUnixSeconds_;
    }

    void clearAttempt() {
        resetSecurity();
        pendingSecureRecordSize_ = 0U;
        if (handshakeScratch_ != nullptr) {
            secureZero(MutableByteSpan(
                handshakeScratch_, handshakeCapacity_));
        }
        if (plaintextScratch_ != nullptr) {
            secureZero(MutableByteSpan(
                plaintextScratch_, plaintextCapacity_));
        }
        if (secureRecordScratch_ != nullptr) {
            secureZero(MutableByteSpan(
                secureRecordScratch_, secureRecordCapacity_));
        }
        activeAttemptId_ = 0U;
        pendingError_ = ErrorCode::Ok;
        faultReported_ = false;
    }

    Link& link_;
    GatewaySessionAccessSource access_;
    IClock& clock_;
    IX25519AesGcmCryptoProvider& crypto_;
    GatewayChildSessionConfig config_;
    ChildSessionCoordinator coordinator_;
    Selector selector_;
    ControllerCredential credential_;
    uint8_t* handshakeScratch_;
    uint8_t* plaintextScratch_;
    uint8_t* secureRecordScratch_;
    uint16_t handshakeCapacity_;
    uint16_t plaintextCapacity_;
    uint16_t secureRecordCapacity_;
    uint16_t pendingSecureRecordSize_;
    GatewayChildFrameReceiver receiver_;
    void* receiverContext_;
    GatewayChildSessionHandler secureHandler_;
    GatewayChildSessionHandler disconnectedHandler_;
    void* sessionContext_;
    GatewayChildSessionFaultHandler faultHandler_;
    void* faultContext_;
    uint32_t activeAttemptId_;
    uint64_t expiresAtUnixSeconds_;
    uint32_t credentialVersion_;
    ErrorCode pendingError_;
    ErrorCode lastError_;
    GatewayChildSessionState state_;
    bool faultReported_;
};

} // namespace blinker

#endif
