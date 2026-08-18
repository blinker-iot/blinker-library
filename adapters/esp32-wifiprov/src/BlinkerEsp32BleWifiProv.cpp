#include "BlinkerEsp32WifiProvAdapter.h"

#include <network_provisioning/scheme_ble.h>

namespace blinker {

namespace {

uint8_t kDefaultServiceUuid[16] = {
    0x9eU, 0x48U, 0x7dU, 0x6aU, 0x92U, 0x11U, 0x4fU, 0x4cU,
    0xa1U, 0x44U, 0x7bU, 0x4dU, 0x42U, 0x4bU, 0x02U, 0x01U};

esp_err_t prepareBle(const Esp32WifiProvConfig& config) {
    return network_prov_scheme_ble_set_service_uuid(
        config.serviceUuid != nullptr
            ? config.serviceUuid
            : kDefaultServiceUuid);
}

} // namespace

Esp32WifiProvConfig esp32BleWifiProvConfig(
    const char* serviceName,
    const char* proofOfPossession,
    uint8_t* serviceUuid) {
    Esp32WifiProvConfig config;
    config.scheme = &network_prov_scheme_ble;
    config.schemeEventHandler =
        NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BLE;
    config.prepare = &prepareBle;
    config.serviceName = serviceName;
    config.proofOfPossession = proofOfPossession;
    config.serviceUuid = serviceUuid;
    return config;
}

} // namespace blinker
