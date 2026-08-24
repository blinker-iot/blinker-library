#ifndef BLINKER_TRANSPORT_IFRAMETRANSPORT_H
#define BLINKER_TRANSPORT_IFRAMETRANSPORT_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../interface/ITransportLifecycle.h"

namespace blinker {

enum TransportFeature : uint16_t {
    TransportFeatureNone = 0,
    TransportFeatureBinary = 1U << 0,
    TransportFeatureLocal = 1U << 1,
    TransportFeatureCloud = 1U << 2,
    TransportFeatureAuthenticated = 1U << 3,
    TransportFeatureFragmented = 1U << 4
};

struct TransportCapabilities {
    uint16_t maxFrameSize;
    uint16_t features;

    TransportCapabilities() : maxFrameSize(0), features(TransportFeatureNone) {}
};

// Identifies only the source of the current synchronous callback. It is not a
// keepalive, authorization grant, or durable reply handle. Callers must not
// retain a pointer/reference to it after the callback returns.
struct RxContext {
    uint8_t transportId;
    uint32_t sessionId;
    bool encrypted;
    bool bonded;
    bool authenticated;

    RxContext()
        : transportId(0),
          sessionId(0),
          encrypted(false),
          bonded(false),
          authenticated(false) {}
};

enum class SendTargetKind : uint8_t {
    Broadcast = 0,
    Transport,
    Session
};

// A routing address only. Runtime must still verify that the peer exists,
// negotiated the feature, and is authorized before sending application data.
struct SendTarget {
    SendTargetKind kind;
    uint8_t transportId;
    uint32_t sessionId;

    SendTarget()
        : kind(SendTargetKind::Broadcast), transportId(0), sessionId(0) {}
};

typedef void (*FrameReceiver)(
    void* context,
    ByteView frame,
    const RxContext& rx);

typedef void (*FrameSessionHandler)(
    void* context,
    const RxContext& rx);

class IFrameTransport : public ITransportLifecycle {
public:
    virtual ~IFrameTransport() {}

    virtual Result start() = 0;
    virtual void stop() = 0;
    virtual void poll(uint32_t budgetMicros) = 0;
    virtual TransportState state() const = 0;
    virtual TransportCapabilities capabilities() const = 0;
    // The frame view is only valid for this call. Implementations must consume
    // it synchronously or copy it into adapter-owned storage before returning.
    // send() must not synchronously invoke the registered receive/session
    // callbacks; adapters defer such events until poll(). Runtime uses one
    // bounded transmit buffer and intentionally does not support send reentry.
    virtual Result send(ByteView frame, const SendTarget& target) = 0;
    virtual void setReceiver(FrameReceiver receiver, void* context) = 0;
    // Session-aware transports override this. Connectionless transports can
    // keep the default no-op and use sessionId 0.
    virtual void setSessionHandlers(
        FrameSessionHandler connected,
        FrameSessionHandler disconnected,
        void* context) {
        (void)connected;
        (void)disconnected;
        (void)context;
    }
};

} // namespace blinker

#endif
