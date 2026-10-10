#ifndef BLINKER_TEST_INGRESS_CAPACITY_PROBE_H
#define BLINKER_TEST_INGRESS_CAPACITY_PROBE_H

#include <BlinkerV2/runtime/GatewayRouteClient.h>
#include <BlinkerV2/protocol/bbp2/Frame.h>
#include <BlinkerV2/protocol/bbp2/Messages.h>

// Test-only receiver backpressure. The real MQTT subscription, bounded slot,
// expiry stay in GatewayRouteEndpoint; MQTT stays in GatewayRouteClient. No frame or secret logging.
class IngressCapacityProbe : public blinker::GatewayRouteEndpoint {
public:
    IngressCapacityProbe(blinker::GatewayRouteClient& client, blinker::GatewayRouteMessageMatcher matcher, void* context)
        : GatewayRouteEndpoint(client, matcher, context) { instance = this; }
    ~IngressCapacityProbe() { if (instance == this) instance = nullptr; }

    void setReceiver(blinker::GatewayRouteMessageReceiver receiver, void* context) {
        receiver_ = receiver;
        context_ = context;
        GatewayRouteEndpoint::setReceiver(receiver ? receive : nullptr, this);
    }

    void arm() {
        if (armed_ || hasPendingEnvelope()) {
            Serial.println("ingress=busy");
            return;
        }
        armed_ = true;
        started_ = false;
        slotBlocked_ = false;
        expirations_ = 0;
        armedAt_ = monotonicMillis();
        Serial.println("ingress=armed");
    }

    // Sketch/test calls this after the shared client poll; no production hook.
    void observe() {
        if (slotBlocked_ && !hasPendingEnvelope()) {
            if (lastError() == blinker::ErrorCode::WouldBlock) {
                ++expirations_;
                Serial.printf("ingress=expired count=%u heldMs=%lu\n", expirations_,
                    static_cast<unsigned long>(monotonicMillis() - slotAt_));
            }
            slotBlocked_ = false;
        }
        // Longer than the 3750 ms command budget plus the 1000 ms ingress
        // bound: never release a queued copy into execution after its timeout.
        if (armed_ && !hasPendingEnvelope() &&
            monotonicMillis() - (started_ ? startedAt_ : armedAt_) >=
                (started_ ? 6000U : 15000U)) {
            armed_ = false;
            Serial.printf("ingress=released count=%u\n", expirations_);
        }
    }

    static IngressCapacityProbe* instance;

private:
    static blinker::Result receive(void* context, blinker::ByteView bytes) {
        auto& self = *static_cast<IngressCapacityProbe*>(context);
        blinker::gateway::ChildRouteEnvelopeView envelope;
        blinker::bbp2::FrameView frame;
        if (self.armed_ && blinker::gateway::decodeChildRouteEnvelope(bytes, envelope) &&
            blinker::bbp2::parseFrame(envelope.innerBbp2Frame, frame) &&
            frame.header.kind == static_cast<uint8_t>(blinker::bbp2::MessageKind::Command)) {
            const uint32_t now = self.monotonicMillis();
            if (!self.started_) { self.started_ = true; self.startedAt_ = now; }
            if (!self.slotBlocked_) {
                self.slotBlocked_ = true;
                self.slotAt_ = now;
                Serial.printf("ingress=blocked sequence=%u\n", frame.header.sequence);
            }
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        return self.receiver_(self.context_, bytes);
    }

    blinker::GatewayRouteMessageReceiver receiver_ = nullptr;
    void* context_ = nullptr;
    uint32_t armedAt_ = 0, startedAt_ = 0, slotAt_ = 0;
    unsigned int expirations_ = 0;
    bool armed_ = false, started_ = false, slotBlocked_ = false;
};

IngressCapacityProbe* IngressCapacityProbe::instance = nullptr;
#endif
