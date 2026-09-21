#ifndef BLINKER_INTERFACE_ILOCALMESSAGELINK_H
#define BLINKER_INTERFACE_ILOCALMESSAGELINK_H

#include "ITransportLifecycle.h"
#include "../core/Span.h"

namespace blinker {

// Single physical binary-message connection, not an identity or authorization.
// A WebSocket port owns HTTP upgrade, masking/continuations/control frames,
// one bounded input message, physical network generation and socket deadlines.
// No Runtime callbacks/crypto in SDK tasks; these methods run in the product's
// loop. An SDK worker must keep borrowed RX stable and serialize its mailbox.
// Lifecycle is exclusively owned by one local transport; the object and its
// buffers outlive that transport. Do not share a physical link between owners.
class ILocalMessageLink : public ITransportLifecycle {
public:
    // Zero while disconnected. Each accepted connection gets a strictly
    // increasing nonzero ID for this object's lifetime (including stop/start).
    // Network change closes the old connection. Exhaustion fails closed.
    virtual uint32_t sessionId() const = 0;
    virtual size_t maxMessageSize() const = 0;
    // WouldBlock means no complete message. Success borrows the port's bounded
    // RX buffer until the next receive/poll/close; send never alters that view.
    virtual Result receive(ByteView& message) = 0;
    // Atomic consume/copy or WouldBlock with no bytes accepted. Must not block
    // for TCP completion or deliver callbacks; any queue has a fixed bound.
    virtual Result send(ByteView message) = 0;
    // Immediately invalidates sessionId and discards all old RX/TX, including
    // partially written records. No bytes may transfer to a subsequent peer.
    virtual void close() = 0;
};

} // namespace blinker
#endif
