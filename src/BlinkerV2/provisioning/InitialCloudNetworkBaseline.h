#ifndef BLINKER_PROVISIONING_INITIALCLOUDNETWORKBASELINE_H
#define BLINKER_PROVISIONING_INITIALCLOUDNETWORKBASELINE_H

#include "../core/Result.h"

namespace blinker {

// Read-only, repeatable admission check. The caller owns serialization with
// network writes and must not use a failed write handle as durability proof.
class IInitialCloudNetworkBaseline {
public:
    virtual ~IInitialCloudNetworkBaseline() {}
    virtual Result validateEmptyNetwork() = 0;
};

} // namespace blinker

#endif
