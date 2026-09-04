#ifndef BLINKER_INTERFACE_IGATEWAYPERMITJOINRELAYADAPTER_H
#define BLINKER_INTERFACE_IGATEWAYPERMITJOINRELAYADAPTER_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../protocol/gateway/PermitJoinRelay.h"

namespace blinker {

struct GatewayPermitJoinCandidate {
    uint8_t token[gateway::kPermitJoinCandidateTokenMaximumSize];
    uint8_t tokenSize;
    uint8_t wireVersion;
    uint16_t capabilities;
    uint8_t signalQuality;

    GatewayPermitJoinCandidate()
        : token(), tokenSize(0U), wireVersion(0U), capabilities(0U),
          signalQuality(0U) {}
};

// Opaque southbound enrollment relay. Cloud and management layers never see
// radio addresses and never parse Noise, grants, receipts or BBP/2.
class IGatewayPermitJoinRelayAdapter {
public:
    virtual ~IGatewayPermitJoinRelayAdapter() {}
    virtual size_t relayCandidateCount() const = 0;
    virtual Result readRelayCandidate(
        size_t index,
        GatewayPermitJoinCandidate& output) const = 0;
    virtual Result selectRelayCandidate(ByteView token) = 0;
    virtual void clearRelaySelection() = 0;
    // Success means the southbound bearer reliably accepted this packet.
    // WouldBlock or NotConnected means no packet was accepted and the relay
    // must withhold its cloud ACK so the exact fragment can be retried later.
    virtual Result queueRelayPacket(ByteView packet) = 0;
    virtual Result takeRelayPacket(
        MutableByteSpan output,
        size_t& written) = 0;
    virtual size_t relayPacketCount() const = 0;
    virtual bool relayConnected() const = 0;
    virtual uint16_t relayPacketSize() const = 0;
};

} // namespace blinker

#endif
