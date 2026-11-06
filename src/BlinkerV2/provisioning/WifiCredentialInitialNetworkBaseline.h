#ifndef BLINKER_PROVISIONING_WIFICREDENTIALINITIALNETWORKBASELINE_H
#define BLINKER_PROVISIONING_WIFICREDENTIALINITIALNETWORKBASELINE_H

#include "InitialCloudNetworkBaseline.h"
#include "WifiCredentialStore.h"

namespace blinker {

class WifiCredentialInitialNetworkBaseline : public IInitialCloudNetworkBaseline {
public:
    explicit WifiCredentialInitialNetworkBaseline(WifiCredentialStore& store)
        : store_(store) {}

    Result validateEmptyNetwork() override {
        return store_.verifyInitialNetworkEmpty();
    }

private:
    WifiCredentialStore& store_;
};

} // namespace blinker

#endif
