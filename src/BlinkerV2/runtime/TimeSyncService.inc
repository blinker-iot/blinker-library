#include "TimeSyncService.h"

namespace blinker {
TimeSyncService::TimeSyncService(const WallClockConfig& config)
    : clock_(config), coordinator_(clock_), tick_(nullptr), random_(nullptr),
      nextAttempt_(0U), generation_(1U), scheduled_(false) {}

Result TimeSyncService::configure(IClock& tick, IRandom& random) {
    if ((tick_ != nullptr && tick_ != &tick) || (random_ != nullptr && random_ != &random))
        return Result::failure(ErrorCode::AlreadyExists);
    tick_ = &tick;
    random_ = &random;
    return Result::success();
}

void TimeSyncService::reset(bool clearTime) {
    coordinator_.cancel();
    if (++generation_ == 0U) ++generation_;
    if (clearTime) {
        clock_.clear();
        scheduled_ = false;
    }
}

void TimeSyncService::sessionClosed(uint8_t transportId, uint32_t sessionId) {
    if (coordinator_.pendingFrom(transportId, sessionId)) reset(false);
}

bool TimeSyncService::poll(const TimeSyncPeer& selected) {
    if (tick_ == nullptr || random_ == nullptr) return false;
    const uint32_t now = tick_->monotonicMillis();
    coordinator_.poll(now);
    TimeSyncPeer peer = selected;
    peer.generation = generation_;
    if (coordinator_.pending() && (!coordinator_.pendingFrom(peer) || !peer.authenticated ||
        (peer.permissions & kAuthorizationPermissionControl) == 0U ||
        (peer.features & bbp2::FeatureTimeSync) == 0U)) reset(false);
    return !coordinator_.pending() &&
        (!scheduled_ || now - nextAttempt_ < 0x80000000U);
}

Result TimeSyncService::request(const TimeSyncPeer& source, uint16_t sequence,
                                MutableByteSpan body, size_t& written) {
    written = 0U;
    if (!poll(source)) return Result::failure(ErrorCode::WouldBlock);
    const uint32_t now = tick_->monotonicMillis();
    // Remote/uncertain failures retain this deadline. Local queue rejection
    // has a separate bounded delay because no request reached the peer.
    nextAttempt_ = now + 30000U;
    scheduled_ = true;
    TimeSyncPeer peer = source;
    peer.generation = generation_;
    return coordinator_.begin(peer, sequence, now, *random_, body, written);
}

void TimeSyncService::sendRejected(ErrorCode error) {
    const bool prepared = coordinator_.pending();
    reset(false); // Cancel that nonce/RTT anchor, never the valid UTC holdover.
    if (prepared && tick_ != nullptr && error == ErrorCode::WouldBlock)
        nextAttempt_ = tick_->monotonicMillis() + 1000U;
}

Result TimeSyncService::receive(const TimeSyncPeer& source, const bbp2::FrameView& frame) {
    if (tick_ == nullptr) return Result::failure(ErrorCode::NotConfigured);
    const uint32_t now = tick_->monotonicMillis();
    coordinator_.poll(now);
    TimeSyncPeer peer = source;
    peer.generation = generation_;
    if (frame.header.kind == static_cast<uint8_t>(bbp2::MessageKind::Error)) {
        bbp2::ErrorBody error;
        if (frame.header.flags != bbp2::FlagIsResponse || !peer.authenticated ||
            (peer.permissions & kAuthorizationPermissionControl) == 0U ||
            (peer.features & bbp2::FeatureTimeSync) == 0U)
            return Result::failure(ErrorCode::AuthenticationRequired);
        const Result decoded = bbp2::decodeErrorBody(frame.body, error);
        if (!decoded) return decoded;
        if (!error.hasRelatedSequence || !coordinator_.pendingFor(peer, error.relatedSequence))
            return Result::failure(ErrorCode::NotFound);
        reset(false);
        return Result::success();
    }
    const Result result = coordinator_.receive(peer, frame, now);
    if (result) nextAttempt_ = now + 300000U; // Refresh every five minutes.
    return result;
}

bool TimeSyncService::readTime(WallClockSnapshot& snapshot) {
    snapshot = WallClockSnapshot();
    if (tick_ == nullptr) return false;
    return clock_.read(tick_->monotonicMillis(), snapshot);
}
}
