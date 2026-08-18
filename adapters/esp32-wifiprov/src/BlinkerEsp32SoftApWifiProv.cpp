#include "BlinkerEsp32WifiProvAdapter.h"

#include <network_provisioning/scheme_softap.h>

namespace blinker {

Esp32WifiProvConfig esp32SoftApWifiProvConfig(
    const char* serviceName,
    const char* proofOfPossession,
    const char* serviceKey) {
    Esp32WifiProvConfig config;
    config.scheme = &network_prov_scheme_softap;
    config.serviceName = serviceName;
    config.proofOfPossession = proofOfPossession;
    config.serviceKey = serviceKey;
    return config;
}

} // namespace blinker
