#ifndef BLINKER_INTERFACE_INETWORKCREDENTIALSTORE_H
#define BLINKER_INTERFACE_INETWORKCREDENTIALSTORE_H

#include "../core/Result.h"

namespace blinker {

// Optional mutation boundary used by authorized ownership release. Network
// credentials are preserved unless the signed release policy requires clear.
class INetworkCredentialStore {
public:
    virtual ~INetworkCredentialStore() {}
    virtual Result clear() = 0;
};

} // namespace blinker

#endif
