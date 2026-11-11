// Fixed XIAO ESP32S3 SoftAP/TLS four-slot reference; see the adjacent README.
#include "UserProvisioning.h"
#define BLINKER_V2_WIFI_PROV_POP ESP32_TASK_PROVISIONING_POP
#include <BlinkerWiFiBLE.h>
#include <esp_arduino_version.h>

#if !defined(ARDUINO_ARCH_ESP32) || !defined(ARDUINO_XIAO_ESP32S3)
#error "Esp32SoftApTasksQuickStart requires the reviewed XIAO ESP32S3 target"
#endif
#if ESP_ARDUINO_VERSION_MAJOR != 3 || ESP_ARDUINO_VERSION_MINOR != 3 || ESP_ARDUINO_VERSION_PATCH != 11
#error "Esp32SoftApTasksQuickStart requires the original ESP32 Core 3.3.11"
#endif
#if !defined(BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS) || BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS != 1 || \
    !defined(BLINKER_ENABLE_TASKS) || BLINKER_ENABLE_TASKS != 1 || BLINKER_TASK_CAPACITY != 4 || \
    !defined(BLINKER_ESP32_PROVISIONING_SOFTAP) || BLINKER_ESP32_PROVISIONING_SOFTAP != 1 || \
    defined(BLINKER_ESP32_PROVISIONING_BLE) || \
    !defined(BLINKER_OFFICIAL_CLOUD_SESSION_TLS) || BLINKER_OFFICIAL_CLOUD_SESSION_TLS != 1 || \
    !defined(BLINKER_OFFICIAL_MQTT_TLS) || BLINKER_OFFICIAL_MQTT_TLS != 1
#error "Install the matching restricted SoftAP/TLS Tasks4 library profile; Sketch task macros are not sufficient"
#endif

namespace {
constexpr bool sameProvisioningText(const char* left, const char* right) {
    return *left == *right && (*left == '\0' || sameProvisioningText(left + 1, right + 1));
}
constexpr char provisioningPop[] = ESP32_TASK_PROVISIONING_POP;
static_assert(sizeof(provisioningPop) > 1U && sizeof(provisioningPop) <= 65U &&
              provisioningPop[0] != '\0' &&
              !sameProvisioningText(provisioningPop, "REPLACE_WITH_YOUR_DEVICE_POP"),
              "Set your own nonempty 1-64 byte PoP in the personal UserProvisioning.h copy");
}

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));

namespace {
bool begun = false;
bool currentPower = false;

void onPower(bool value) { currentPower = value; power.report(value); }
uint32_t sampleUptime() { return millis() / 1000U; }
}

void setup() {
    power.onWrite(onPower);
    uptimeSeconds.onSample(sampleUptime);
    begun = Blinker.enableTasks(power) && Blinker.begin(power, uptimeSeconds);
    if (begun) power.report(currentPower);
}

void loop() {
    if (begun) Blinker.run();
}
