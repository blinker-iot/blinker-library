#ifndef BLINKER_INTERFACE_IBLELINK_H
#define BLINKER_INTERFACE_IBLELINK_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum class BleLinkState : uint8_t {
    Stopped = 0,
    Starting,
    Ready,
    Error
};

struct BleSessionInfo {
    uint32_t sessionId;
    uint16_t maxPacketSize;
    bool connected;
    bool notifyEnabled;
    bool encrypted;
    bool bonded;
    bool authenticated;

    BleSessionInfo()
        : sessionId(0),
          maxPacketSize(0),
          connected(false),
          notifyEnabled(false),
          encrypted(false),
          bonded(false),
          authenticated(false) {}
};

typedef void (*BlePacketReceiver)(
    void* context,
    const BleSessionInfo& session,
    ByteView packet);

typedef void (*BleSessionHandler)(
    void* context,
    const BleSessionInfo& session);

// Platform adapters expose raw characteristic packets and security metadata.
// sendPacket must consume/copy packet bytes before returning. WouldBlock means
// the caller should retry after poll; it must not partially consume a packet.
// A non-zero sessionId identifies exactly one physical connection: emit one
// connected callback, keep the ID stable, then emit its matching disconnected
// callback before reusing that numeric ID for any later peer.
class IBleLink {
public:
    virtual ~IBleLink() {}

    virtual Result start() = 0;
    virtual void stop() = 0;
    virtual void poll(uint32_t budgetMicros) = 0;
    virtual BleLinkState state() const = 0;
    virtual ErrorCode lastError() const = 0;
    virtual size_t sessionCount() const = 0;
    virtual Result sessionAt(size_t index, BleSessionInfo& session) const = 0;
    virtual Result sendPacket(uint32_t sessionId, ByteView packet) = 0;
    // Requests physical retirement of exactly one admitted connection. The
    // matching disconnected callback remains asynchronous and must still be
    // emitted once by the Port. Legacy/test links may report unsupported;
    // official BLE products implement this to bound unauthenticated peers.
    virtual Result disconnectSession(uint32_t sessionId) {
        (void)sessionId;
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    virtual void setPacketReceiver(BlePacketReceiver receiver, void* context) = 0;
    virtual void setSessionHandlers(
        BleSessionHandler connected,
        BleSessionHandler disconnected,
        void* context) = 0;
};

} // namespace blinker

#endif
