#include "../../../local/Esp32LocalGateway.h"
#include <BlinkerWiFiBLE.h>

#include <WiFi.h>
#include <esp_arduino_version.h>
#include <esp_heap_caps.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

namespace {

const uint32_t kStatusPeriodMs = 5000U;
const uint32_t kDiagnosticsPeriodMs = 30000U;
const UBaseType_t kTaskSnapshotCapacity = 32U;
uint32_t nextStatusMs = 0U;
uint32_t nextDiagnosticsMs = 0U;
bool currentPower = false;
TaskStatus_t taskSnapshot[kTaskSnapshotCapacity];

void onPower(bool value) {
    currentPower = value;
    power.report(currentPower);
}

void printIp(const IPAddress& address) {
    for (uint8_t index = 0U; index < 4U; ++index) {
        if (index != 0U) Serial.print('.');
        Serial.print(address[index]);
    }
}

void printStatus() {
    const blinker::ProductStatus status = Blinker.status();

    Serial.print(F("[probe] ms="));
    Serial.print(millis());
    Serial.print(F(" state="));
    Serial.print(static_cast<uint8_t>(status.state));
    Serial.print(F(" error="));
    Serial.print(static_cast<uint8_t>(status.lastError));
    Serial.print(F(" started="));
    Serial.print(status.started);
    Serial.print(F(" network="));
    Serial.print(status.networkReady);
    Serial.print(F(" local="));
    Serial.print(status.localReady);
    Serial.print(F(" cloud="));
    Serial.println(status.cloudReady);

    Serial.print(F("[probe] wifiMode="));
    Serial.print(static_cast<uint8_t>(WiFi.getMode()));
    Serial.print(F(" ip="));
    printIp(WiFi.localIP());
    Serial.print(F(" rssi="));
    Serial.print(WiFi.RSSI());
    Serial.println();

    Serial.print(F("[probe] heap free="));
    Serial.print(ESP.getFreeHeap());
    Serial.print(F(" min="));
    Serial.print(ESP.getMinFreeHeap());
    Serial.print(F(" largest="));
    Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Serial.print(F(" loopStackMinBytes="));
    Serial.println(uxTaskGetStackHighWaterMark(nullptr));
}

void printBuildInfo() {
    Serial.print(F("[probe] board="));
    Serial.print(ARDUINO_BOARD);
    Serial.print(F(" arduino="));
    Serial.print(ESP_ARDUINO_VERSION_STR);
    Serial.print(F(" idf="));
    Serial.println(esp_get_idf_version());
    Serial.print(F("[probe] ipcStackBytes="));
    Serial.print(CONFIG_ESP_IPC_TASK_STACK_SIZE);
    Serial.print(F(" canary="));
    Serial.print(CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY);
    Serial.print(F(" watchpoint="));
    Serial.println(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK);
}

void printTaskWatermarks(const char* phase) {
    const UBaseType_t expected = uxTaskGetNumberOfTasks();
    Serial.print(F("[probe] task snapshot phase="));
    Serial.println(phase);
    Serial.print(F("[probe] tasks expected="));
    Serial.print(expected);
    Serial.print(F(" capacity="));
    Serial.println(kTaskSnapshotCapacity);
    if (expected > kTaskSnapshotCapacity) {
        Serial.println(F("[probe] task snapshot skipped: capacity"));
        return;
    }

    configRUN_TIME_COUNTER_TYPE totalRunTime = 0U;
    const UBaseType_t count = uxTaskGetSystemState(
        taskSnapshot,
        kTaskSnapshotCapacity,
        &totalRunTime);
    if (count == 0U) {
        Serial.println(F("[probe] task snapshot failed"));
        return;
    }

    for (UBaseType_t index = 0U; index < count; ++index) {
        const TaskStatus_t& task = taskSnapshot[index];
        Serial.print(F("[probe] task name="));
        Serial.print(task.pcTaskName ? task.pcTaskName : "?");
        Serial.print(F(" stackMinBytes="));
        Serial.print(task.usStackHighWaterMark);
        Serial.print(F(" priority="));
        Serial.print(task.uxCurrentPriority);
#if configTASKLIST_INCLUDE_COREID == 1
        Serial.print(F(" core="));
        Serial.print(task.xCoreID);
#endif
        Serial.print(F(" state="));
        Serial.println(static_cast<uint8_t>(task.eCurrentState));
    }
}

void processSerialCommands() {
    while (Serial.available() > 0) {
        const char command = static_cast<char>(Serial.read());
        if (command == 'n') {
            Serial.print(F("[probe] resetNetwork="));
            Serial.println(Blinker.resetNetwork());
        } else if (command == 'd') {
            printBuildInfo();
            printStatus();
            printTaskWatermarks("command");
            Blinker.printDiagnostics(Serial);
        } else if (command == 'r') {
            Serial.println(F("[probe] restart"));
            Serial.flush();
            ESP.restart();
        }
    }
}

} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t serialDeadline = millis() + 3000U;
    while (!Serial &&
           static_cast<int32_t>(millis() - serialDeadline) < 0) {
        delay(10U);
    }

    Serial.println(F("[probe] ESP32 WiFi+BLE boot probe"));
    printBuildInfo();
    Serial.print(F("[probe] resetReason="));
    Serial.println(static_cast<uint8_t>(esp_reset_reason()));
    printTaskWatermarks("pre-begin");

    power.onWrite(onPower);
    Blinker.debug(Serial, blinker::LogLevel::Debug);
    const bool started = Blinker.begin(power);
    Serial.print(F("[probe] begin="));
    Serial.print(started);
    Serial.print(F(" lastError="));
    Serial.print(static_cast<uint8_t>(Blinker.lastError()));
    Serial.print(F(" text="));
    Serial.println(Blinker.lastErrorText());
    if (started) Blinker.report(power, currentPower);

    nextStatusMs = millis();
    nextDiagnosticsMs = millis() + kDiagnosticsPeriodMs;
}

void loop() {
    Blinker.run();
    processSerialCommands();

    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextStatusMs) >= 0) {
        printStatus();
        nextStatusMs = now + kStatusPeriodMs;
    }
    if (static_cast<int32_t>(now - nextDiagnosticsMs) >= 0) {
        printTaskWatermarks("periodic");
        Blinker.printDiagnostics(Serial);
        nextDiagnosticsMs = now + kDiagnosticsPeriodMs;
    }
}
