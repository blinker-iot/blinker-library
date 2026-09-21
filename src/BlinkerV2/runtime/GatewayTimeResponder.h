#ifndef BLINKER_RUNTIME_GATEWAYTIMERESPONDER_H
#define BLINKER_RUNTIME_GATEWAYTIMERESPONDER_H

#include "IGatewayChildRequestHandler.h"
#include "TimeSyncResponder.h"
#include "../protocol/bbp2/Messages.h"

namespace blinker {
// An opt-in module. Default Hub/executor only reference the abstract seam, so
// they do not link this policy or create a clock. All children borrow ONE source.
class GatewayTimeResponder final : public IGatewayChildRequestHandler {
public:
    explicit GatewayTimeResponder(IWallTimeSource& source) : responder_(source) {}
    uint32_t features() const override { return bbp2::FeatureTimeSync; }
    bool handles(uint8_t kind) const override {
        return kind == static_cast<uint8_t>(bbp2::MessageKind::TimeRequest) ||
               kind == static_cast<uint8_t>(bbp2::MessageKind::TimeResponse);
    }
    Result reply(const GatewayChildRequestContext& context, const bbp2::FrameView& request,
                 MutableByteSpan output, ByteView& response) override {
        TimeSyncPeer peer;
        peer.sessionId = peer.generation = context.attemptId;
        peer.permissions = context.permissions;
        peer.features = context.features;
        peer.authenticated = context.permissions != 0U;
        return responder_.reply(peer, request, context.nowMillis, output, response);
    }
    void reset() override { responder_.reset(); }
private:
    TimeSyncResponder responder_;
};
}
#endif
