// Internal hardware gate, never a public example. GateConfig.h is generated in
// an ignored build directory by the isolated Service fixture. No credentials
// belong in this source tree. Requires linker --wrap=nvs_set_blob/nvs_commit.
#include "GateConfig.h"
#define BLINKER_ENABLE_TASKS 1
#include "TaskPilotPolicy.h"
#include <BlinkerWiFi.h>
#include <esp_timer.h>
#include <cstring>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);
static uint32_t callbacks = 0, presses = 0, loops = 0;
static uint32_t commits = 0, snapshots = 0, maxCommitUs = 0, maxSnapshotUs = 0, maxLoopUs = 0;
static bool value = false, begun = false;
extern "C" esp_err_t __real_nvs_commit(nvs_handle_t);
extern "C" esp_err_t __real_nvs_set_blob(nvs_handle_t, const char*, const void*, size_t);
extern "C" esp_err_t __wrap_nvs_commit(nvs_handle_t handle) {
    const int64_t start = esp_timer_get_time();
    const esp_err_t result = __real_nvs_commit(handle);
    const uint32_t elapsed = static_cast<uint32_t>(esp_timer_get_time() - start);
    ++commits; if (elapsed > maxCommitUs) maxCommitUs = elapsed;
    return result;
}
extern "C" esp_err_t __wrap_nvs_set_blob(nvs_handle_t handle, const char* key, const void* data, size_t size) {
    const int64_t start = esp_timer_get_time();
    const esp_err_t result = __real_nvs_set_blob(handle, key, data, size);
    if (std::strcmp(key, "snapshot") == 0) {
        const uint32_t elapsed = static_cast<uint32_t>(esp_timer_get_time() - start);
        ++snapshots; if (elapsed > maxSnapshotUs) maxSnapshotUs = elapsed;
    }
    return result;
}
static void onPower(bool requested) { value = requested; ++callbacks; power.report(value); }
static void onButton() { buttonPresses.report(++presses); }
void setup() {
    Serial.begin(115200);
    power.onWrite(onPower); button.onInvoke(onButton);
    begun = Blinker.enableTasks(power) && Blinker.begin(GATE_DEVICE_KEY, GATE_WIFI_SSID, GATE_WIFI_PASSWORD,
                                                power, buttonPresses, button);
    if (begun) { power.report(false); buttonPresses.report(0U); }
}
void loop() {
    const int64_t start = esp_timer_get_time();
    if (begun) Blinker.run();
    const uint32_t elapsed = static_cast<uint32_t>(esp_timer_get_time() - start);
    ++loops; if (elapsed > maxLoopUs) maxLoopUs = elapsed;
    if (!Serial.available()) return;
    const int command = Serial.read();
    if (command == 'r') { ESP.restart(); return; } // Software reboot, never power-loss evidence.
    if (command == 'm') { loops = commits = snapshots = maxCommitUs = maxSnapshotUs = maxLoopUs = 0U; return; }
    if (command == 'd') { Blinker.printDiagnostics(Serial); return; }
    if (command != 'j') return;
    uint64_t utc = 0; const bool synchronized = Blinker.time(utc);
    Serial.printf("{\"fixture\":\"task-product\",\"begun\":%u,\"wifi\":%u,\"power\":%u,\"callbacks\":%lu,"
        "\"presses\":%lu,\"loops\":%lu,\"commits\":%lu,\"snapshots\":%lu,\"maxCommitUs\":%lu,"
        "\"maxSnapshotUs\":%lu,\"maxLoopUs\":%lu,\"heap\":%lu,\"minHeap\":%lu,\"stackFree\":%lu,"
        "\"timeValid\":%u,\"utc\":%lu,\"taskError\":%u}\n",
        begun, WiFi.status(), value, static_cast<unsigned long>(callbacks), static_cast<unsigned long>(presses),
        static_cast<unsigned long>(loops), static_cast<unsigned long>(commits), static_cast<unsigned long>(snapshots),
        static_cast<unsigned long>(maxCommitUs), static_cast<unsigned long>(maxSnapshotUs), static_cast<unsigned long>(maxLoopUs),
        static_cast<unsigned long>(ESP.getFreeHeap()), static_cast<unsigned long>(ESP.getMinFreeHeap()),
        static_cast<unsigned long>(uxTaskGetStackHighWaterMark(nullptr)), synchronized, static_cast<unsigned long>(utc),
        static_cast<unsigned>(blinker::integration::official_detail::esp32WifiProduct().extension().taskError()));
}
