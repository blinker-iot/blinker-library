#ifndef BLINKER_PROTOCOL_BBP2_TIMESYNC_H
#define BLINKER_PROTOCOL_BBP2_TIMESYNC_H

#include "../../core/Span.h"
#include "../../core/WallClock.h"

namespace blinker { namespace bbp2 {

static const size_t kTimeSyncNonceSize = 16U;
static const size_t kTimeRequestSize = 18U;
static const size_t kTimeResponseMaxSize = 30U;
static const uint32_t kTimeSyncMaxRttMillis = 2000U;

struct TimeResponseView {
    ByteView nonce;
    uint64_t utcAtReplyMillis;
    uint32_t uncertaintyMillis;
    TimeResponseView() : nonce(), utcAtReplyMillis(0U), uncertaintyMillis(0U) {}
};

bool validTimeSyncNonce(ByteView nonce);
Result encodeTimeRequest(ByteView nonce, MutableByteSpan output, size_t& written);
Result decodeTimeRequest(ByteView body, ByteView& nonce);
Result encodeTimeResponse(const TimeResponseView& value, MutableByteSpan output, size_t& written);
Result decodeTimeResponse(ByteView body, TimeResponseView& value);

} } // namespace blinker::bbp2
#endif
