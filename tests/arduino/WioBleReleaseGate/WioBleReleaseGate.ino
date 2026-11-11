#define BLINKER_ENABLE_WIO_BLE_PREVIEW 1
#include <BlinkerBLE.h>

#include <FreeRTOS.h>
#include <erpc/erpc_port.h>

extern "C" void* _sbrk(size_t increment);

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);

namespace {

bool currentPower = false;
uint32_t pressCount = 0U;
uint32_t nextStatusAt = 0U;

void printRtlVersion() {
    char* version = rpc_system_version();
    Serial.print("RTL_VERSION=");
    Serial.println(version == nullptr ? "unavailable" : version);
    if (version != nullptr) erpc_free(version);
}

void printMemory() {
    Serial.print("FREERTOS_HEAP free=");
    Serial.print(static_cast<unsigned long>(xPortGetFreeHeapSize()));
    Serial.print(" minimum=");
    Serial.println(static_cast<unsigned long>(
        xPortGetMinimumEverFreeHeapSize()));

    const uintptr_t heapEnd = reinterpret_cast<uintptr_t>(_sbrk(0U));
    const uintptr_t stackPointer = static_cast<uintptr_t>(__get_MSP());
    Serial.print("NEWLIB_HEADROOM bytes=");
    Serial.println(static_cast<unsigned long>(stackPointer - heapEnd));
}

void onPower(bool value) {
    currentPower = value;
    power.report(currentPower);
    Serial.print("COMMAND power=");
    Serial.println(currentPower ? 1 : 0);
}

void onButton() {
    buttonPresses.report(++pressCount);
    Serial.print("ACTION button count=");
    Serial.println(pressCount);
}

} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t waitUntil = millis() + 5000U;
    while (!Serial && static_cast<int32_t>(millis() - waitUntil) < 0) {}

    Blinker.debug(Serial);
    printRtlVersion();
    power.onWrite(onPower);
    button.onInvoke(onButton);
    if (!Blinker.begin(power, buttonPresses, button)) {
        Serial.print("BEGIN_FAILED error=");
        Serial.println(Blinker.lastErrorText());
        return;
    }
    Blinker.report(power, currentPower, buttonPresses, pressCount);
    Serial.println("BEGIN_OK");
}

void loop() {
    Blinker.run();
    if (Serial.available()) {
        const int command = Serial.read();
        if (command == 'r') {
            Serial.println(Blinker.resetAccess()
                               ? "ACCESS_RESET_OK"
                               : "ACCESS_RESET_FAILED");
        } else if (command == 'v') {
            printRtlVersion();
        } else if (command == 'm') {
            printMemory();
        }
    }

    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextStatusAt) < 0) return;
    const blinker::ProductStatus status = Blinker.status();
    Serial.print("STATUS state=");
    Serial.print(static_cast<unsigned int>(status.state));
    Serial.print(" local=");
    Serial.print(status.localReady ? 1 : 0);
    T_GAP_CONN_INFO connection = {};
    const bool hasConnectionInfo = le_get_conn_info(0U, &connection);
    Serial.print(" rtlActive=");
    Serial.print(static_cast<unsigned int>(le_get_active_link_num()));
    Serial.print(" rtlInfo=");
    Serial.print(hasConnectionInfo ? 1 : 0);
    Serial.print(" rtlState=");
    Serial.print(hasConnectionInfo
                     ? static_cast<unsigned int>(connection.conn_state)
                     : 0xffU);
    Serial.print(" adv=");
    Serial.print(static_cast<unsigned int>(ble_gap_dev_state.gap_adv_state));
    Serial.print(" error=");
    Serial.println(static_cast<unsigned int>(status.lastError));
    nextStatusAt = now + 5000U;
}
