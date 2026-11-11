// Internal single-self wireless gate, not a public installation example.
// Force-include an ignored local configuration for every translation unit.
#include <BlinkerWiFi.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>
#include <string.h>
#include <sys/time.h>
#if defined(BLINKER_INTERNAL_LAN_ENCRYPTED) && BLINKER_INTERNAL_LAN_ENCRYPTED
#include "EmptyGcmGate.h"
constexpr unsigned kLanSecurityProfile = 1U;
#else
constexpr unsigned kLanSecurityProfile = 2U;
#endif

#if !BLINKER_INTERNAL_LAN_WEBSOCKET || !BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL
#error "This gate requires the internal LAN and cloud-authority build flags"
#endif
#ifndef BLINKER_GATE_BUILD_TIME
#error "Provide test credentials, CA and a current build timestamp privately"
#endif

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);

uint32_t pressCount = 0;
// Test-only static snapshot: avoids allocating from the heap being measured.
// Report this workspace separately from public-product static RAM figures.
TaskStatus_t taskSnapshot[24];
void printTaskSnapshot() {
    wifi_config_t native = {};
    if (esp_wifi_get_config(WIFI_IF_STA, &native) == ESP_OK) {
        Serial.printf("gate wifiConfig ssidMatches=%u credentialMatches=%u channel=%u pinnedBssid=%u\n",
            strncmp(reinterpret_cast<const char*>(native.sta.ssid), BLINKER_WIFI_SSID, sizeof(native.sta.ssid)) == 0,
            strncmp(reinterpret_cast<const char*>(native.sta.password), BLINKER_WIFI_PASSWORD, sizeof(native.sta.password)) == 0,
            static_cast<unsigned>(native.sta.channel), static_cast<unsigned>(native.sta.bssid_set));
    }
    // Diagnose only equality/selection flags; never emit or retain a key/password copy.
    volatile uint8_t* bytes = reinterpret_cast<volatile uint8_t*>(&native);
    for (size_t index = 0; index < sizeof(native); ++index) bytes[index] = 0U;
    const UBaseType_t expected = uxTaskGetNumberOfTasks();
    configRUN_TIME_COUNTER_TYPE runtime = 0;
    const UBaseType_t captured = expected <= 24U
        ? uxTaskGetSystemState(taskSnapshot, 24U, &runtime) : 0U;
    Serial.printf("gate tasks expected=%u captured=%u workspace=%u\n",
        static_cast<unsigned>(expected), static_cast<unsigned>(captured),
        static_cast<unsigned>(sizeof(taskSnapshot)));
    for (UBaseType_t index = 0; index < captured; ++index)
        Serial.printf("gate task=%s stackMin=%u\n", taskSnapshot[index].pcTaskName,
            static_cast<unsigned>(taskSnapshot[index].usStackHighWaterMark));
}
void onPower(bool value) {
    power.report(value);
    Serial.printf("gate power=%u\n", value ? 1U : 0U);
}
void onButton() {
    buttonPresses.report(++pressCount);
    Serial.printf("gate buttonPresses=%lu\n", static_cast<unsigned long>(pressCount));
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0); // Test logging cannot block MQTT/LAN when no monitor drains USB.
    WiFi.onEvent([](arduino_event_id_t, arduino_event_info_t info) {
        Serial.printf("gate wifiDisconnect reason=%u\n", static_cast<unsigned>(info.wifi_sta_disconnected.reason));
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#if defined(BLINKER_INTERNAL_LAN_ENCRYPTED) && BLINKER_INTERNAL_LAN_ENCRYPTED
    const bool emptyGcmPassed = emptyGcmGate();
    Serial.printf("gate emptyGcm=%u\n", emptyGcmPassed ? 1U : 0U);
    if (!emptyGcmPassed) return;
#endif
    // A build-time clock bootstrap is only for this short-lived CA hardware
    // gate; it is not a production time source or a certificate bypass.
    const timeval now = { BLINKER_GATE_BUILD_TIME, 0 };
    settimeofday(&now, nullptr);
    Blinker.debug(Serial);
    power.onWrite(onPower);
    button.onInvoke(onButton);
    const bool started = Blinker.begin(BLINKER_DEVICE_KEY, BLINKER_WIFI_SSID,
        BLINKER_WIFI_PASSWORD, power, buttonPresses, button);
    Serial.printf("gate begin=%u\n", started ? 1U : 0U);
    if (started) Blinker.report(power, false, buttonPresses, pressCount);
}

void loop() {
    Blinker.run();
    if (Serial.available() && Serial.read() == 'r') printTaskSnapshot();
    static uint32_t last = 0U;
    if (millis() - last >= 10000U) {
        last = millis();
        Blinker.printDiagnostics(Serial);
        Serial.printf("gate profile=%u wifi=%u heap=%lu minHeap=%lu largest=%lu stack=%lu ip=%s\n", kLanSecurityProfile, WiFi.status() == WL_CONNECTED,
            static_cast<unsigned long>(ESP.getFreeHeap()),
            static_cast<unsigned long>(ESP.getMinFreeHeap()),
            static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)),
            static_cast<unsigned long>(uxTaskGetStackHighWaterMark(nullptr)), WiFi.localIP().toString().c_str());
    }
}
