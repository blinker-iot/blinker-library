#include <Arduino.h>
#include <WiFi.h>
#include <network_provisioning/manager.h>
#include <network_provisioning/scheme_ble.h>

namespace {

void onEvent(void*, network_prov_cb_event_t event, void*) {
    Serial.print("event=");
    Serial.println(static_cast<unsigned>(event));
}

} // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("native WiFiProv probe starting");
    WiFi.persistent(false);
    (void)WiFi.STA.begin(false);

    network_prov_mgr_config_t config = {};
    config.scheme = network_prov_scheme_ble;
    config.app_event_handler.event_cb = onEvent;

    esp_err_t error = network_prov_mgr_init(config);
    Serial.print("init=");
    Serial.println(error);
    if (error == ESP_OK) {
        error = network_prov_mgr_start_provisioning(
            NETWORK_PROV_SECURITY_1,
            nullptr,
            "BLINKER_NATIVE",
            nullptr);
    }
    Serial.print("start=");
    Serial.println(error);
}

void loop() {
    delay(1000);
}
