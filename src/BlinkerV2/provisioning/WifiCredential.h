#ifndef BLINKER_PROVISIONING_WIFICREDENTIAL_H
#define BLINKER_PROVISIONING_WIFICREDENTIAL_H

#include "../interface/IWifiCredentialSink.h"

namespace blinker {

enum : size_t {
    kWifiSsidMaxSize = 32U,
    kWifiCredentialMaxSize = 64U
};

Result validateWifiNetworkConfig(const WifiNetworkConfig& config);

} // namespace blinker

#endif
