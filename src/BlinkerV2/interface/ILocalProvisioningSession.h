#ifndef BLINKER_INTERFACE_ILOCALPROVISIONINGSESSION_H
#define BLINKER_INTERFACE_ILOCALPROVISIONINGSESSION_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

// Minimal lifecycle used by BLE composition. The concrete application owns
// enrollment state; radio and mode code do not need to know those details.
class ILocalProvisioningSession {
public:
    virtual ~ILocalProvisioningSession() {}

    // The 8-byte advertising locator is not the signed 16-byte setupSessionId.
    // A concrete coordinator owns the full id and exposes it only inside the
    // protected application handshake.
    virtual Result beginSession(ByteView setupSessionLocator) = 0;

    // Called exactly once after the responder handshake has been queued and
    // Noise has entered Transport. The borrowed 32-byte hash is the final
    // handshake transcript used to bind BleEnrollmentGrant to this channel.
    virtual Result secureSessionReady(ByteView setupTranscriptHash) = 0;
    virtual void endSession() = 0;
};

} // namespace blinker

#endif
