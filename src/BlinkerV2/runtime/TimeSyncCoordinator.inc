#include "TimeSyncCoordinator.h"
#include <string.h>

namespace blinker {
namespace {
Result admitTimePeer(const TimeSyncPeer& peer) {
    if (peer.generation == 0U || (peer.source != WallTimeSource::Server &&
        peer.source != WallTimeSource::Controller && peer.source != WallTimeSource::Gateway))
        return Result::failure(ErrorCode::InvalidArgument);
    if (!peer.authenticated || (peer.permissions & kAuthorizationPermissionControl) == 0U)
        return Result::failure(ErrorCode::AuthenticationRequired);
    if ((peer.features & bbp2::FeatureTimeSync) == 0U)
        return Result::failure(ErrorCode::UnsupportedFeature);
    return Result::success();
}
}

TimeSyncCoordinator::TimeSyncCoordinator(WallClock& clock)
    : clock_(clock), sessionId_(0U), generation_(0U), sentAtMillis_(0U), nonce_(),
      sequence_(0U), transportId_(0U), source_(WallTimeSource::None), pending_(false) {}

Result TimeSyncCoordinator::begin(const TimeSyncPeer& peer, uint16_t sequence, uint32_t nowMillis,
                                 IRandom& random, MutableByteSpan body, size_t& written) {
    written = 0U;
    poll(nowMillis);
    if (pending_) return Result::failure(ErrorCode::AlreadyExists);
    Result result = admitTimePeer(peer);
    if (!result) return result;
    if (sequence == 0U) return Result::failure(ErrorCode::InvalidArgument);
    if (body.data == nullptr || body.size < bbp2::kTimeRequestSize)
        return Result::failure(ErrorCode::BufferTooSmall);
    uint8_t nonce[bbp2::kTimeSyncNonceSize] = {};
    result = random.fill(MutableByteSpan(nonce, sizeof(nonce)));
    if (!result) return result;
    if (!bbp2::validTimeSyncNonce(ByteView(nonce, sizeof(nonce))) || memcmp(nonce, nonce_, sizeof(nonce)) == 0)
        return Result::failure(ErrorCode::SequenceConflict);
    result = bbp2::encodeTimeRequest(ByteView(nonce, sizeof(nonce)), body, written);
    if (!result) return result;
    memcpy(nonce_, nonce, sizeof(nonce_));
    sessionId_ = peer.sessionId;
    generation_ = peer.generation;
    transportId_ = peer.transportId;
    source_ = peer.source;
    sequence_ = sequence;
    sentAtMillis_ = nowMillis;
    pending_ = true;
    return Result::success();
}

Result TimeSyncCoordinator::receive(const TimeSyncPeer& peer, const bbp2::FrameView& frame, uint32_t nowMillis) {
    poll(nowMillis);
    if (!pending_) return Result::failure(ErrorCode::NotFound);
    if (peer.sessionId != sessionId_ || peer.generation != generation_ ||
        peer.transportId != transportId_ || peer.source != source_)
        return Result::failure(ErrorCode::StateConflict);
    Result result = admitTimePeer(peer); // Recheck current authority, not the begin snapshot.
    if (!result) { cancel(); return result; }
    if (frame.header.version != bbp2::kVersion) return Result::failure(ErrorCode::UnsupportedVersion);
    if (frame.header.kind != static_cast<uint8_t>(bbp2::MessageKind::TimeResponse) ||
        (frame.header.flags & ~bbp2::FlagHasCrc32c) != bbp2::FlagIsResponse || frame.header.sequence != sequence_)
        return Result::failure(ErrorCode::SequenceConflict);
    bbp2::TimeResponseView response;
    result = bbp2::decodeTimeResponse(frame.body, response);
    if (!result) return result;
    if (memcmp(response.nonce.data, nonce_, sizeof(nonce_)) != 0)
        return Result::failure(ErrorCode::SequenceConflict);
    const uint32_t rtt = nowMillis - sentAtMillis_;
    WallClockSample sample;
    sample.utcMillis = response.utcAtReplyMillis + rtt / 2U;
    sample.uncertaintyMillis = response.uncertaintyMillis + (rtt + 1U) / 2U;
    sample.source = source_;
    cancel(); // A matched response is consumed even if the clock budget rejects it.
    return clock_.synchronize(sample, nowMillis);
}

void TimeSyncCoordinator::poll(uint32_t nowMillis) {
    clock_.poll(nowMillis);
    if (pending_ && nowMillis - sentAtMillis_ > bbp2::kTimeSyncMaxRttMillis) cancel();
}

void TimeSyncCoordinator::cancel() {
    pending_ = false;
    sessionId_ = generation_ = sentAtMillis_ = 0U;
    transportId_ = 0U;
    sequence_ = 0U;
    source_ = WallTimeSource::None;
    // Retain only the public last nonce to reject immediate RNG reuse. Global
    // uniqueness across boots depends on IRandom's secure-random contract.
}
} // namespace blinker
