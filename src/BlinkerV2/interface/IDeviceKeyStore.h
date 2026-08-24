#ifndef BLINKER_INTERFACE_IDEVICEKEYSTORE_H
#define BLINKER_INTERFACE_IDEVICEKEYSTORE_H

#include "IDeviceKeySource.h"

namespace blinker {

// Mutation is restricted to an already-authorized provisioning/reset flow.
// Transport/session authorization stays outside the persistence seam.
class IDeviceKeyStore : public IDeviceKeySource {
public:
    virtual Result replace(const DeviceKey& key) = 0;
    virtual Result clear() = 0;
};

} // namespace blinker

#endif
