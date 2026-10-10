#ifndef BLINKER_TESTS_ESP32_BLE_API_PROBE_COMMON_H
#define BLINKER_TESTS_ESP32_BLE_API_PROBE_COMMON_H

#include <Arduino.h>
#include <esp_arduino_version.h>
#include <esp_heap_caps.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

namespace ble_api_probe {

constexpr UBaseType_t kTaskCapacity = 24U;
TaskStatus_t tasks[kTaskCapacity];

inline void printMemory(const char* phase) {
    Serial.printf(
        "[probe] memory phase=%s free=%u min=%u largest=%u\n",
        phase,
        ESP.getFreeHeap(),
        ESP.getMinFreeHeap(),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

inline void printTasks(const char* phase) {
    const UBaseType_t expected = uxTaskGetNumberOfTasks();
    configRUN_TIME_COUNTER_TYPE totalRunTime = 0U;
    const UBaseType_t count = expected <= kTaskCapacity
        ? uxTaskGetSystemState(tasks, kTaskCapacity, &totalRunTime)
        : 0U;

    Serial.printf(
        "[probe] tasks phase=%s expected=%u captured=%u\n",
        phase,
        static_cast<unsigned>(expected),
        static_cast<unsigned>(count));
    for (UBaseType_t index = 0U; index < count; ++index) {
        Serial.printf(
            "[probe] task name=%s stackMinBytes=%u\n",
            tasks[index].pcTaskName ? tasks[index].pcTaskName : "?",
            static_cast<unsigned>(tasks[index].usStackHighWaterMark));
    }
}

inline void begin(const char* backend) {
    Serial.begin(115200);
    const uint32_t deadline = millis() + 3000U;
    while (!Serial && static_cast<int32_t>(millis() - deadline) < 0) {
        delay(10U);
    }

    Serial.printf("[probe] backend=%s\n", backend);
    Serial.printf("[probe] board=%s\n", ARDUINO_BOARD);
    Serial.printf("[probe] arduino=%s\n", ESP_ARDUINO_VERSION_STR);
    Serial.printf("[probe] idf=%s\n", esp_get_idf_version());
    Serial.printf("[probe] resetReason=%u\n", static_cast<unsigned>(esp_reset_reason()));
    Serial.printf("[probe] ipcStackBytes=%u\n", CONFIG_ESP_IPC_TASK_STACK_SIZE);
    printMemory("before-init");
    printTasks("before-init");
    Serial.flush();
    delay(500U);
}

inline void initialized(bool result) {
    Serial.printf("[probe] init=%u\n", result ? 1U : 0U);
    printMemory("after-init");
    printTasks("after-init");
}

inline void advertising(bool result) {
    Serial.printf("[probe] advertising=%u\n", result ? 1U : 0U);
    delay(3000U);
    printMemory("settled");
    printTasks("settled");
}

inline void poll() {
    static uint32_t nextReportMs = 0U;
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextReportMs) < 0) return;
    nextReportMs = now + 10000U;
    printMemory("periodic");
    printTasks("periodic");
}

} // namespace ble_api_probe

#endif
