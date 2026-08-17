#ifndef BLINKER_INTERFACE_ILOCALPROVISIONINGAPPLICATION_H
#define BLINKER_INTERFACE_ILOCALPROVISIONINGAPPLICATION_H

#include "ILocalProvisioningSession.h"

namespace blinker {

// Allocation-free application seam above an authenticated local channel.
// Request is borrowed and response must point inside output on success.
class ILocalProvisioningApplication : public ILocalProvisioningSession {
public:
    virtual Result handle(
        ByteView request,
        MutableByteSpan operationWorkspace,
        MutableByteSpan output,
        ByteView& response) = 0;
};

} // namespace blinker

#endif
