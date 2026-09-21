#ifndef BLINKER_TRANSPORT_LOCALACCESSFRAMETRANSPORT_H
#define BLINKER_TRANSPORT_LOCALACCESSFRAMETRANSPORT_H

#include "IFrameTransport.h"
#include "../interface/IClock.h"
#include "../interface/ILocalMessageLink.h"
#include "../runtime/LocalAccessLease.h"

namespace blinker {

// First LAN profile: one caller, self only. Reuses the product authority and
// Runtime; knows no WiFi SDK, MQTT, discovery, fields or gateway child executor.
// Storage is caller-owned: one plaintext RX and one exact queued TX record.
// The link separately owns one bounded wire RX. No heap or pending-work list.
template <size_t LeaseCapacity, typename Records>
class LocalAccessFrameTransport final : public IFrameTransport {
public:
    LocalAccessFrameTransport(
        ILocalMessageLink& link, IClock& clock, IRandom& random,
        Records& records, LocalAccessAuthority<LeaseCapacity>& authority,
        MutableByteSpan plaintext, MutableByteSpan outgoing, uint16_t maxFrameSize = 400U)
        : link_(link), clock_(clock), authority_(authority), challenge_(authority, random, Records::profile),
          records_(records), plaintext_(plaintext), outgoing_(outgoing),
          maxFrameSize_(maxFrameSize), phase_(Phase::Idle), started_(false), announced_(false),
          session_(0U), lastSession_(0U), openedAt_(0U), admittedAt_(0U), lifetime_(0U),
          keyVersion_(0U), revision_(0U), permissions_(0U), authorization_(),
          queued_(0U), queuedAt_(0U), receiver_(nullptr), receiverContext_(nullptr),
          connected_(nullptr), disconnected_(nullptr), sessionContext_(nullptr) {}

    ~LocalAccessFrameTransport() { stop(); }

    Result start() override {
        if (started_) return Result::failure(ErrorCode::AlreadyExists);
        const size_t maximum = static_cast<size_t>(maxFrameSize_) + Records::overhead;
        if (maxFrameSize_ < 64U || maximum > Records::maximumMessage || plaintext_.data == nullptr ||
            outgoing_.data == nullptr || plaintext_.size < maxFrameSize_ ||
            outgoing_.size < maximum || outgoing_.size < local_access::kChallengeMaximumEncodedSize ||
            link_.maxMessageSize() < maximum ||
            link_.maxMessageSize() < local_access::kGrantMaximumEncodedSize ||
            overlaps(plaintext_, outgoing_)) return Result::failure(ErrorCode::InvalidArgument);
        Result result = link_.start();
        if (result) started_ = true;
        else link_.stop(); // Roll back a port that acquired only some resources.
        return result;
    }

    void stop() override {
        if (started_) { retire(); notifyClosed(); link_.stop(); }
        clearSecrets();
        started_ = false;
    }

    TransportState state() const override {
        return !started_ ? TransportState::Stopped : link_.state();
    }
    TransportCapabilities capabilities() const override {
        TransportCapabilities value;
        value.maxFrameSize = maxFrameSize_;
        value.features = TransportFeatureBinary | TransportFeatureLocal |
            TransportFeatureAuthenticated | TransportFeaturePeerInitiatesHello;
        return value;
    }
    uint32_t sessionPermissions(uint32_t id) const override {
        return id == session_ && link_.sessionId() == session_ && phase_ == Phase::Ready && leaseLive()
            ? permissions_ : 0U;
    }

    void poll(uint32_t budgetMicros) override {
        if (!started_) return;
        authority_.poll(clock_.monotonicMillis());
        challenge_.poll(clock_.monotonicMillis());
        // Expiry/revocation is checked before allowing a port to flush bytes.
        if (session_ != 0U && (link_.sessionId() != session_ || expired())) retire();
        notifyClosed();
        link_.poll(budgetMicros);
        if (session_ != 0U && link_.sessionId() != session_) retire();
        notifyClosed();
        if (phase_ == Phase::Idle && link_.sessionId() != 0U) open();
        if (phase_ == Phase::Closing) { notifyClosed(); return; }
        if (session_ == 0U) return;
        if (expired()) { retire(); notifyClosed(); return; }
        if (queued_ != 0U) {
            flush();
            if (queued_ != 0U || phase_ == Phase::Closing) return;
        }
        ByteView message;
        const Result result = link_.receive(message);
        if (result.code() == ErrorCode::WouldBlock) return;
        if (!result || message.data == nullptr || message.empty() ||
            message.size > link_.maxMessageSize()) { retire(); return; }
        // At most one input and one output operation per poll, no drain loop.
        handle(message);
    }

    Result send(ByteView frame, const SendTarget& target) override {
        if (!started_ || phase_ != Phase::Ready || !leaseLive() ||
            link_.sessionId() != session_) {
            if (session_ != 0U) retire();
            return Result::failure(ErrorCode::NotConnected);
        }
        if (target.kind == SendTargetKind::Session && target.sessionId != session_)
            return Result::failure(ErrorCode::NotFound);
        if (target.kind != SendTargetKind::Session && target.kind != SendTargetKind::Transport &&
            target.kind != SendTargetKind::Broadcast) return Result::failure(ErrorCode::InvalidArgument);
        if (frame.data == nullptr || frame.empty()) return Result::failure(ErrorCode::InvalidArgument);
        if (frame.size > maxFrameSize_) return Result::failure(ErrorCode::CapacityExceeded);
        if (queued_ != 0U) return Result::failure(ErrorCode::WouldBlock);
        size_t written = 0U;
        Result result = records_.encode(frame, outgoing_, written);
        if (result) { queued_ = written; queuedAt_ = clock_.monotonicMillis(); }
        if (!result) retire(); // Retire on record failure; never retry or change the profile.
        return result;
    }

    Result disconnectSession(uint32_t id) override {
        if (id == 0U || id != session_) return Result::failure(ErrorCode::NotFound);
        retire(); // Callbacks are deferred to poll, including when called by send.
        return Result::success();
    }
    void setReceiver(FrameReceiver receiver, void* context) override {
        receiver_ = receiver; receiverContext_ = context;
    }
    void setSessionHandlers(FrameSessionHandler connected, FrameSessionHandler disconnected,
                            void* context) override {
        connected_ = connected; disconnected_ = disconnected; sessionContext_ = context;
    }

private:
    enum class Phase : uint8_t { Idle, Grant, Handshake, Response, Ready, Closing };
    enum : uint32_t { kWriteDeadlineMillis = 3000U };

    static bool overlaps(MutableByteSpan a, MutableByteSpan b) {
        const uintptr_t first = reinterpret_cast<uintptr_t>(a.data);
        const uintptr_t second = reinterpret_cast<uintptr_t>(b.data);
        return first <= second ? second - first < a.size : first - second < b.size;
    }
    bool leaseLive() const {
        return permissions_ != 0U && authority_.deviceKeyVersion() == keyVersion_ &&
            authority_.authorityRevision() == revision_ &&
            static_cast<uint32_t>(clock_.monotonicMillis() - admittedAt_) < lifetime_ &&
            authority_.permits(ByteView(authorization_, sizeof(authorization_)),
                permissions_, clock_.monotonicMillis());
    }
    bool expired() const {
        if (phase_ == Phase::Closing) return true;
        if (phase_ == Phase::Ready) return !leaseLive();
        return !authority_.acceptingGrants() ||
            static_cast<uint32_t>(clock_.monotonicMillis() - openedAt_) >=
                local_access::kChallengeResponseMillis ||
            ((phase_ == Phase::Handshake || phase_ == Phase::Response) && !leaseLive());
    }
    RxContext rx() const {
        RxContext value;
        value.sessionId = session_; value.encrypted = Records::encrypted; value.authenticated = true;
        return value;
    }
    void open() {
        const uint32_t id = link_.sessionId();
        if (id <= lastSession_ || !authority_.acceptingGrants()) { link_.close(); return; }
        lastSession_ = id; session_ = id; openedAt_ = clock_.monotonicMillis();
        size_t written = 0U;
        const Result result = challenge_.issueChallenge(openedAt_, outgoing_, written);
        if (!result) { retire(); return; }
        phase_ = Phase::Grant; queued_ = written; queuedAt_ = openedAt_;
    }
    void flush() {
        if (static_cast<uint32_t>(clock_.monotonicMillis() - queuedAt_) >= kWriteDeadlineMillis) {
            retire(); return;
        }
        const Result result = link_.send(ByteView(outgoing_.data, queued_));
        if (result.code() == ErrorCode::WouldBlock) return;
        if (!result) { retire(); return; }
        secureZero(outgoing_); queued_ = 0U;
        if (phase_ == Phase::Response) {
            phase_ = Phase::Ready; announced_ = true;
            if (connected_ != nullptr) connected_(sessionContext_, rx());
        }
    }
    void handle(ByteView message) {
        if (phase_ == Phase::Grant) {
            local_access::GrantView grant;
            Result result = local_access::decodeGrant(message, grant);
            if (!result || grant.targetKind != local_access::TargetKind::Self ||
                grant.securityProfile != Records::profile) { retire(); return; }
            LocalAccessAdmission admission;
            result = challenge_.acceptGrant(message, clock_.monotonicMillis(), admission);
            if (result) {
                memcpy(authorization_, admission.authorizationId, sizeof(authorization_));
                permissions_ = admission.permissions; lifetime_ = admission.remainingMillis;
                admittedAt_ = clock_.monotonicMillis(); keyVersion_ = authority_.deviceKeyVersion();
                revision_ = admission.authorityRevision;
                result = records_.begin(ByteView(admission.sessionKey, sizeof(admission.sessionKey)), grant.authenticator);
            }
            if (!result) { retire(); return; }
            phase_ = Phase::Handshake;
            return;
        }
        Result result;
        ByteView plaintext;
        if (phase_ == Phase::Handshake) {
            size_t written = 0U;
            result = records_.reply(message, plaintext_, outgoing_, written);
            if (result) { queued_ = written; queuedAt_ = clock_.monotonicMillis(); phase_ = Phase::Response; }
        } else if (phase_ == Phase::Ready && message.size <= static_cast<size_t>(maxFrameSize_) + Records::overhead) {
            result = records_.decode(message, plaintext_, plaintext);
            if (result && leaseLive() && receiver_ != nullptr) receiver_(receiverContext_, plaintext, rx());
            if (!leaseLive()) result = Result::failure(ErrorCode::AuthenticationRequired);
        } else {
            result = Result::failure(ErrorCode::ProtocolError);
        }
        secureZero(plaintext_);
        if (!result) retire();
    }
    void clearSecrets() {
        records_.clear(); challenge_.clear(); secureZero(plaintext_); secureZero(outgoing_);
        secureZero(MutableByteSpan(authorization_, sizeof(authorization_)));
        permissions_ = 0U; lifetime_ = 0U; queued_ = 0U;
    }
    void retire() {
        // A port may already expose a newer physical generation. Retire our
        // old context without closing that unrelated newly accepted socket.
        if (session_ == 0U || link_.sessionId() == session_) link_.close();
        clearSecrets(); phase_ = Phase::Closing;
        // Closing a socket does not retire its product-owned authorization or
        // renew its deadline. Reconnection needs a fresh challenge + grant.
    }
    void notifyClosed() {
        if (phase_ != Phase::Closing) return;
        const RxContext previous = rx();
        const bool notify = announced_;
        announced_ = false; session_ = 0U; phase_ = Phase::Idle;
        if (notify && disconnected_ != nullptr) disconnected_(sessionContext_, previous);
    }

    ILocalMessageLink& link_;
    IClock& clock_;
    LocalAccessAuthority<LeaseCapacity>& authority_;
    LocalAccessChallengeSession<LeaseCapacity> challenge_;
    Records& records_;
    MutableByteSpan plaintext_, outgoing_;
    uint16_t maxFrameSize_;
    Phase phase_;
    bool started_, announced_;
    uint32_t session_, lastSession_, openedAt_, admittedAt_, lifetime_;
    uint32_t keyVersion_, revision_, permissions_;
    uint8_t authorization_[local_access::kIdSize];
    size_t queued_;
    uint32_t queuedAt_;
    FrameReceiver receiver_;
    void* receiverContext_;
    FrameSessionHandler connected_, disconnected_;
    void* sessionContext_;

    LocalAccessFrameTransport(const LocalAccessFrameTransport&);
    LocalAccessFrameTransport& operator=(const LocalAccessFrameTransport&);
};

} // namespace blinker
#endif
