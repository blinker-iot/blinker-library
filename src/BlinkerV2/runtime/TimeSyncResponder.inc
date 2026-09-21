#include "TimeSyncResponder.h"
#include "../core/Authorization.h"
#include "../protocol/bbp2/TimeSync.h"
#include "../protocol/bbp2/Messages.h"
#include "../protocol/bbp2/WireError.h"
#include <string.h>

namespace blinker {
TimeSyncResponder::TimeSyncResponder(IWallTimeSource& source)
    : source_(source), lastReplyMillis_(0U), replied_(false) {}

void TimeSyncResponder::reset() { lastReplyMillis_ = 0U; replied_ = false; }

Result TimeSyncResponder::reply(const TimeSyncPeer& peer, const bbp2::FrameView& request,
                               uint32_t nowMillis, MutableByteSpan output, ByteView& response) {
    response = ByteView();
    if (!peer.authenticated || peer.sessionId == 0U || peer.generation == 0U ||
        (peer.permissions & kAuthorizationPermissionControl) == 0U)
        return Result::failure(ErrorCode::AuthenticationRequired);
    if ((peer.features & bbp2::FeatureTimeSync) == 0U)
        return Result::failure(ErrorCode::UnsupportedFeature);
    if (request.header.version != bbp2::kVersion)
        return Result::failure(ErrorCode::UnsupportedVersion);
    if (request.header.kind != static_cast<uint8_t>(bbp2::MessageKind::TimeRequest) ||
        request.header.flags != bbp2::FlagNone || request.header.sequence == 0U)
        return Result::failure(ErrorCode::ProtocolError);
    if (output.data == nullptr || output.size < bbp2::kBaseHeaderSize + bbp2::kTimeResponseMaxSize)
        return Result::failure(ErrorCode::BufferTooSmall);
    ByteView nonce;
    const Result decoded = bbp2::decodeTimeRequest(request.body, nonce);
    if (!decoded) return decoded;
    if (replied_ && static_cast<uint32_t>(nowMillis - lastReplyMillis_) < 5000U)
        return Result::failure(ErrorCode::WouldBlock);
    lastReplyMillis_ = nowMillis; replied_ = true; // Error and failed sends consume the same quota.
    uint8_t echoed[bbp2::kTimeSyncNonceSize];
    memcpy(echoed, nonce.data, sizeof(echoed)); // Safe even when output aliases the request.
    const MutableByteSpan body(output.data + bbp2::kBaseHeaderSize, output.size - bbp2::kBaseHeaderSize);
    WallClockSnapshot snapshot;
    bbp2::TimeResponseView sample;
    sample.nonce = ByteView(echoed, sizeof(echoed));
    size_t written = 0U;
    Result result = Result::failure(ErrorCode::NotConfigured);
    if (source_.readTime(snapshot) && snapshot.state == WallClockState::Synchronized) {
        sample.utcAtReplyMillis = snapshot.utcMillis;
        sample.uncertaintyMillis = snapshot.uncertaintyMillis;
        result = bbp2::encodeTimeResponse(sample, body, written);
    }
    bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(bbp2::MessageKind::TimeResponse);
    header.flags = bbp2::FlagIsResponse;
    header.sequence = request.header.sequence;
    if (!result) {
        bbp2::ErrorBody error;
        error.errorCode = static_cast<uint16_t>(WireError::InternalError);
        error.relatedSequence = request.header.sequence;
        error.hasRelatedSequence = true;
        ByteView encoded;
        result = bbp2::encodeErrorBody(error, body, encoded);
        written = encoded.size;
        header.kind = static_cast<uint8_t>(bbp2::MessageKind::Error);
    }
    if (!result) return result;
    header.bodyLength = static_cast<uint16_t>(written);
    result = bbp2::encodeHeader(header, output);
    if (result) response = ByteView(output.data, bbp2::kBaseHeaderSize + written);
    return result;
}
}
