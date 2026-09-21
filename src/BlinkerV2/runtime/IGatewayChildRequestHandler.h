#ifndef BLINKER_RUNTIME_IGATEWAYCHILDREQUESTHANDLER_H
#define BLINKER_RUNTIME_IGATEWAYCHILDREQUESTHANDLER_H

#include "../protocol/bbp2/Frame.h"

namespace blinker {
// Trusted, invocation-local session facts supplied by the sole executor.
struct GatewayChildRequestContext {
    uint32_t attemptId;
    uint32_t permissions;
    uint32_t features;
    uint32_t nowMillis;
};

// Optional borrowed protocol module, not an upstream router or connection owner.
// One module instance per child; its lifetime must cover the executor binding.
// reply only encodes into borrowed scratch. Executor alone submits to the live
// session and owns sequence/cancellation rules; no asynchronous work is allowed.
class IGatewayChildRequestHandler {
public:
    virtual ~IGatewayChildRequestHandler() {}
    virtual uint32_t features() const = 0;
    virtual bool handles(uint8_t kind) const = 0;
    virtual Result reply(const GatewayChildRequestContext& peer,
                         const bbp2::FrameView& request,
                         MutableByteSpan output, ByteView& response) = 0;
    virtual void reset() = 0;
};
}
#endif
