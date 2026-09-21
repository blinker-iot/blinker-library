#ifndef BLINKER_INTERFACE_IGATTPERMITJOINPORTLEASE_H
#define BLINKER_INTERFACE_IGATTPERMITJOINPORTLEASE_H
#include "IBleCentralPort.h"
namespace blinker {
// One bounded Native GATT window borrows a concrete idle port. The generic
// permit coordinator knows neither this lease nor BLE discovery/identity.
class IGattPermitJoinPortLease {
public:
    virtual ~IGattPermitJoinPortLease() {}
    virtual Result acquirePermitJoinPort(IBleCentralPort*& port) = 0;
    virtual void releasePermitJoinPort(IBleCentralPort& port) = 0;
    virtual bool allowPermitJoinScan(IBleCentralPort& port) = 0;
    virtual bool allowPermitJoinTarget(const BleCentralAdvertisement& target) = 0;
};
} // namespace blinker
#endif
