#define BLINKER_ENABLE_WIO_BLE_PREVIEW 1

#include <BlinkerBLE.h>
#include <FreeRTOS.h>

extern "C" void* _sbrk(size_t increment);

BLINKER_PROPERTY(power, bool, blinker::readWrite());

namespace {

uintptr_t initialHeapEnd = 0U;
uint32_t nextReportAt = 0U;

void onPower(bool value) {
    power.report(value);
}

void printResources() {
    const uintptr_t heapEnd = reinterpret_cast<uintptr_t>(_sbrk(0U));
    const uintptr_t stackPointer = static_cast<uintptr_t>(__get_MSP());
    Serial.print(F("[resource-probe] freertos-free="));
    Serial.print(static_cast<unsigned long>(xPortGetFreeHeapSize()));
    Serial.print(F(" minimum="));
    Serial.println(static_cast<unsigned long>(xPortGetMinimumEverFreeHeapSize()));
    Serial.print(F("[resource-probe] newlib-growth="));
    Serial.print(static_cast<unsigned long>(heapEnd - initialHeapEnd));
    Serial.print(F(" heap-stack-headroom="));
    Serial.println(static_cast<unsigned long>(stackPointer - heapEnd));
}

}  // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t serialDeadline = millis() + 2000U;
    while (!Serial && static_cast<int32_t>(millis() - serialDeadline) < 0) {
    }

    initialHeapEnd = reinterpret_cast<uintptr_t>(_sbrk(0U));
    power.onWrite(onPower);
    Serial.print(F("[resource-probe] begin="));
    Serial.println(Blinker.begin(power));
    nextReportAt = millis() + 60000U;
}

void loop() {
    Blinker.run();
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextReportAt) >= 0) {
        printResources();
        nextReportAt = now + 5000U;
    }
}
