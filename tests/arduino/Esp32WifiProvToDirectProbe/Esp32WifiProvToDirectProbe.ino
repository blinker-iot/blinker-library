#include <Arduino.h>
#include <WiFi.h>
#include <network_provisioning/manager.h>
#include <network_provisioning/scheme_ble.h>

#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32NimBle.h>

namespace {

blinker::Esp32NimBleLink directBle;
uint32_t transitionAt = 0U;
bool transitioned = false;
bool directAttempted = false;
volatile bool provisioningEnded = false;

void onEvent(void*, network_prov_cb_event_t event, void*) {
    Serial.print("wifiprov-event=");
    Serial.println(static_cast<unsigned>(event));
    if (event == NETWORK_PROV_END) provisioningEnded = true;
}

} // namespace

void setup() {
    Serial.begin(115200);
    delay(1000U);
    Serial.println("WiFiProv to Direct probe starting");
    WiFi.persistent(false);
    (void)WiFi.STA.begin(false);

    network_prov_mgr_config_t config = {};
    config.scheme = network_prov_scheme_ble;
    config.app_event_handler.event_cb = onEvent;

    esp_err_t error = network_prov_mgr_init(config);
    if (error == ESP_OK) {
        error = network_prov_mgr_start_provisioning(
            NETWORK_PROV_SECURITY_1,
            nullptr,
            "BLINKER_TRANSITION",
            nullptr);
    }
    Serial.print("wifiprov-start=");
    Serial.println(error);
    transitionAt = millis() + 3000U;
}

void loop() {
    if (!transitioned &&
        static_cast<int32_t>(millis() - transitionAt) >= 0) {
        transitioned = true;
        Serial.println("wifiprov-stop");
        network_prov_mgr_stop_provisioning();
    }
    if (transitioned && provisioningEnded && !directAttempted) {
        directAttempted = true;
        network_prov_mgr_deinit();
        const blinker::Result result = directBle.start();
        Serial.print("direct-start=");
        Serial.println(result.ok());
        Serial.print("direct-error=");
        Serial.println(static_cast<unsigned>(directBle.lastError()));
    }
    directBle.poll(1000U);
    delay(1U);
}
