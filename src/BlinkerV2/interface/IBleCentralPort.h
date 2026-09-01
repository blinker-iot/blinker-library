#ifndef BLINKER_INTERFACE_IBLECENTRALPORT_H
#define BLINKER_INTERFACE_IBLECENTRALPORT_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum class BleCentralPortState : uint8_t {
    Stopped = 0U,
    Idle,
    Scanning,
    Connecting,
    Discovering,
    Ready,
    Disconnecting,
    Error
};

// A view over one scan result for the Blinker service UUID. Native addresses
// are deliberately absent: MAC/iOS identifiers are bearer-local hints, not
// child identity. Views are valid only for the matcher call.
struct BleCentralAdvertisement {
    ByteView modeServiceData;
    int16_t rssi;

    BleCentralAdvertisement() : modeServiceData(), rssi(0) {}
};

typedef bool (*BleCentralAdvertisementMatcher)(
    void* context,
    const BleCentralAdvertisement& advertisement);

struct BleCentralConnectRequest {
    uint32_t attemptId;
    BleCentralAdvertisementMatcher matcher;
    void* matcherContext;

    BleCentralConnectRequest()
        : attemptId(0U), matcher(nullptr), matcherContext(nullptr) {}
};

struct BleCentralConnectionInfo {
    uint32_t attemptId;
    uint16_t maxPacketSize;
    bool connected;
    bool notifyEnabled;
    bool encrypted;
    bool bonded;

    BleCentralConnectionInfo()
        : attemptId(0U), maxPacketSize(0U), connected(false),
          notifyEnabled(false), encrypted(false), bonded(false) {}
};

typedef void (*BleCentralPacketReceiver)(
    void* context,
    const BleCentralConnectionInfo& connection,
    ByteView packet);

typedef void (*BleCentralConnectionHandler)(
    void* context,
    const BleCentralConnectionInfo& connection);

// Single-outbound-connection BLE Central port. connect() scans only the
// Blinker service and invokes matcher from poll() for exact mode service-data.
// The native peer address stays inside the Port. A connected callback is legal
// only after service/characteristic discovery and TX notification enablement.
// Callbacks must never be invoked synchronously from start/connect/send/cancel.
class IBleCentralPort {
public:
    virtual ~IBleCentralPort() {}

    virtual Result start() = 0;
    virtual void stop() = 0;
    virtual void poll(uint32_t budgetMicros) = 0;
    virtual BleCentralPortState state() const = 0;
    virtual ErrorCode lastError() const = 0;

    virtual Result connect(const BleCentralConnectRequest& request) = 0;
    // Cancels scanning/connecting or retires the ready connection for exactly
    // one attempt generation. Completion remains asynchronous.
    virtual Result cancel(uint32_t attemptId) = 0;
    virtual Result connection(BleCentralConnectionInfo& output) const = 0;
    virtual Result sendPacket(uint32_t attemptId, ByteView packet) = 0;

    virtual void setPacketReceiver(
        BleCentralPacketReceiver receiver,
        void* context) = 0;
    virtual void setConnectionHandlers(
        BleCentralConnectionHandler connected,
        BleCentralConnectionHandler disconnected,
        void* context) = 0;
};

} // namespace blinker

#endif
