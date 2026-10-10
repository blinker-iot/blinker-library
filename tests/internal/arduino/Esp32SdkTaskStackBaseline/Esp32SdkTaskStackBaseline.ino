#include <Arduino.h>

#if __has_include(<esp32-hal-alloc-ble-mem.h>)
#include <esp32-hal-alloc-ble-mem.h>
#endif

#include <esp_arduino_version.h>
#include <esp_idf_version.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <host/ble_hs.h>
#include <host/ble_hs_adv.h>
#include <host/ble_hs_id.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>
#include <sdkconfig.h>
#include <WiFi.h>

namespace {

const UBaseType_t kTaskCapacity = 24U;
TaskStatus_t tasks[kTaskCapacity];

int onGapEvent(ble_gap_event*, void*) {
    return 0;
}

void onNimbleSync() {
    uint8_t addressType = 0U;
    if (ble_hs_util_ensure_addr(0) != 0
        || ble_hs_id_infer_auto(0, &addressType) != 0) {
        return;
    }
    static const char name[] = "SDK_PROBE";
    ble_hs_adv_fields fields = {};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = reinterpret_cast<const uint8_t*>(name);
    fields.name_len = sizeof(name) - 1U;
    fields.name_is_complete = 1U;
    if (ble_gap_adv_set_fields(&fields) != 0) return;

    ble_gap_adv_params parameters = {};
    parameters.conn_mode = BLE_GAP_CONN_MODE_UND;
    parameters.disc_mode = BLE_GAP_DISC_MODE_GEN;
    (void)ble_gap_adv_start(
        addressType,
        nullptr,
        BLE_HS_FOREVER,
        &parameters,
        onGapEvent,
        nullptr);
}

void nimbleHostTask(void*) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void printSnapshot(const char* phase) {
    const UBaseType_t expected = uxTaskGetNumberOfTasks();
    configRUN_TIME_COUNTER_TYPE totalRunTime = 0U;
    const UBaseType_t count = expected <= kTaskCapacity
        ? uxTaskGetSystemState(tasks, kTaskCapacity, &totalRunTime)
        : 0U;

    Serial.print(F("[sdk] task snapshot phase="));
    Serial.println(phase);
    Serial.print(F("[sdk] tasks expected="));
    Serial.print(expected);
    Serial.print(F(" captured="));
    Serial.println(count);
    for (UBaseType_t index = 0U; index < count; ++index) {
        Serial.print(F("[sdk] task name="));
        Serial.print(tasks[index].pcTaskName ? tasks[index].pcTaskName : "?");
        Serial.print(F(" stackMinBytes="));
        Serial.println(tasks[index].usStackHighWaterMark);
    }
}

} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t deadline = millis() + 3000U;
    while (!Serial && static_cast<int32_t>(millis() - deadline) < 0) {
        delay(10U);
    }

    Serial.print(F("[sdk] board="));
    Serial.println(ARDUINO_BOARD);
    Serial.print(F("[sdk] arduino="));
    Serial.println(ESP_ARDUINO_VERSION_STR);
    Serial.print(F("[sdk] idf="));
    Serial.println(esp_get_idf_version());
    Serial.print(F("[sdk] ipcStackBytes="));
    Serial.println(CONFIG_ESP_IPC_TASK_STACK_SIZE);
    Serial.print(F("[sdk] canary="));
    Serial.print(CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY);
    Serial.print(F(" watchpoint="));
    Serial.println(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK);
    printSnapshot("boot");
}

void loop() {
    const int command = Serial.read();
    if (command == 'd') {
        printSnapshot("command");
    } else if (command == 'w') {
        WiFi.persistent(false);
        WiFi.setAutoReconnect(false);
        const bool started = WiFi.mode(WIFI_STA);
        Serial.print(F("[sdk] wifi-sta result="));
        Serial.println(started ? 1 : 0);
        delay(500U);
        printSnapshot("wifi-sta");
    } else if (command == 'n') {
        const esp_err_t result = nimble_port_init();
        Serial.print(F("[sdk] nimble-init result="));
        Serial.println(static_cast<int>(result));
        delay(500U);
        printSnapshot("nimble-init");
    } else if (command == 'h') {
        const esp_err_t result = nimble_port_init();
        Serial.print(F("[sdk] nimble-host result="));
        Serial.println(static_cast<int>(result));
        if (result == ESP_OK) {
            ble_hs_cfg.sync_cb = onNimbleSync;
            ble_svc_gap_init();
            ble_svc_gatt_init();
            (void)ble_svc_gap_device_name_set("SDK_PROBE");
            nimble_port_freertos_init(nimbleHostTask);
        }
        delay(1000U);
        printSnapshot("nimble-host");
    } else if (command == 'r') {
        Serial.println(F("[sdk] restart"));
        Serial.flush();
        ESP.restart();
    }
    delay(10U);
}
