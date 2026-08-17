#ifndef BLINKER_PROVISIONING_WIFICREDENTIALRESETADAPTER_H
#define BLINKER_PROVISIONING_WIFICREDENTIALRESETADAPTER_H

#include "../interface/INetworkCredentialStore.h"
#include "WifiCredentialStore.h"

namespace blinker {

// Keeps the normal WiFi store ABI unchanged; ownership release pays one
// reference only when network clearing is actually composed.
class WifiCredentialResetAdapter final : public INetworkCredentialStore {
public:
    explicit WifiCredentialResetAdapter(WifiCredentialStore& store)
        : store_(store) {}

    Result clear() override { return store_.clear(); }

private:
    WifiCredentialStore& store_;
};

} // namespace blinker

#endif
