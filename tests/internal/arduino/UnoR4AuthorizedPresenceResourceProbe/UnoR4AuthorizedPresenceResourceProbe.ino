#include <AES.h>
#include <BlinkerBLE.h>

namespace {

constexpr uint8_t kStackMarker = 0xA5U;

extern "C" uint8_t __StackLimit;
extern "C" uint8_t __StackTop;
extern "C" void *_sbrk(ptrdiff_t increment);

uint8_t *markedHeapEnd = nullptr;
uint8_t *markedStackTop = nullptr;
bool stackMarked = false;
bool productStarted = false;
bool measurementReady = false;
size_t measuredHeapGrowth = 0U;
size_t measuredHeapStackGap = 0U;
size_t measuredStackDepth = 0U;
uint32_t lastReport = 0U;

__attribute__((noinline)) uintptr_t stackPointer() {
    uintptr_t value = 0U;
    __asm volatile("mov %0, sp" : "=r"(value));
    return value;
}

__attribute__((noinline)) bool markUnusedStack() {
    uint8_t *const limit = &__StackLimit;
    uint8_t *const top = &__StackTop;
    uint8_t *const heapEnd = static_cast<uint8_t *>(_sbrk(0));
    const uintptr_t current = stackPointer();
    if (heapEnd == nullptr || heapEnd >= limit ||
        current <= reinterpret_cast<uintptr_t>(limit) ||
        current > reinterpret_cast<uintptr_t>(top)) {
        return false;
    }

    markedHeapEnd = heapEnd;
    markedStackTop = reinterpret_cast<uint8_t *>(current & ~uintptr_t(3U));
    noInterrupts();
    for (volatile uint8_t *cursor = markedHeapEnd;
         cursor < markedStackTop;
         ++cursor) {
        *cursor = kStackMarker;
    }
    interrupts();
    return true;
}

size_t heapStackGap(uint8_t *currentHeapEnd) {
    if (markedStackTop == nullptr || currentHeapEnd == nullptr ||
        currentHeapEnd >= markedStackTop) {
        return 0U;
    }

    uint8_t *cursor = currentHeapEnd;
    while (cursor < markedStackTop && *cursor == kStackMarker) {
        ++cursor;
    }
    return static_cast<size_t>(cursor - currentHeapEnd);
}

void printStackWatermark() {
    const size_t nominalCapacity =
        static_cast<size_t>(&__StackTop - &__StackLimit);
    Serial.print(F("[resource-probe] stack-marker="));
    Serial.println(stackMarked ? F("ready") : F("failed"));
    Serial.print(F("[resource-probe] begin="));
    Serial.println(productStarted);
    Serial.print(F("[resource-probe] nominal-stack="));
    Serial.print(nominalCapacity);
    Serial.print(F(" boundary-crossed="));
    Serial.println(measuredStackDepth > nominalCapacity);
    Serial.print(F("[resource-probe] heap-growth="));
    Serial.print(measuredHeapGrowth);
    Serial.print(F(" heap-stack-gap="));
    Serial.print(measuredHeapStackGap);
    Serial.print(F(" observed-stack-depth="));
    Serial.println(measuredStackDepth);
}

}  // namespace

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void onPower(bool value) {
    power.report(value);
}

void setup() {
    Serial.begin(115200);
    const uint32_t serialDeadline = millis() + 2000U;
    while (!Serial && static_cast<int32_t>(millis() - serialDeadline) < 0) {
    }

    power.onWrite(onPower);
    stackMarked = markUnusedStack();
    productStarted = Blinker.begin(power);
}

void loop() {
    Blinker.run();

    const uint32_t now = millis();
    if (!measurementReady && now >= 60000U) {
        uint8_t *const currentHeapEnd = static_cast<uint8_t *>(_sbrk(0));
        measuredHeapGrowth = static_cast<size_t>(currentHeapEnd - markedHeapEnd);
        measuredHeapStackGap = heapStackGap(currentHeapEnd);
        measuredStackDepth = static_cast<size_t>(
            &__StackTop - (currentHeapEnd + measuredHeapStackGap));
        measurementReady = true;
        lastReport = now;
    }
    if (measurementReady &&
        static_cast<uint32_t>(now - lastReport) >= 5000U) {
        lastReport = now;
        printStackWatermark();
    }
}
