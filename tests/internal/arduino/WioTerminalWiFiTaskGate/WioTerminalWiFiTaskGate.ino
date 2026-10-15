// Internal finite Wio WiFi task budget gate, never a public Arduino example.
// GateConfig.h belongs only in an ignored build projection. Placeholder builds
// retain the real product graph but do NOT run begin()/run() if flashed by error.
#include "GateConfig.h"
#include <BlinkerWiFi.h>
#include "RuntimeObservation.h"

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "Wio WiFi task runtime gate requires Wio Terminal"
#endif
#ifndef GATE_COMPILE_ONLY
#error "Wio WiFi task gate requires an explicit compile-only or actual runtime profile"
#endif
#if GATE_COMPILE_ONLY != 0 && GATE_COMPILE_ONLY != 1
#error "Invalid Wio WiFi task gate runtime profile"
#endif

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(duty, uint32_t,
    blinker::rangedProperty(blinker::PropertyMode::ReadWrite, 0.0, 100.0, 5.0));
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));

namespace {
bool begun = false, currentPower = false;
uint32_t currentDuty = 0U, powerWrites = 0U, dutyWrites = 0U, samples = 0U;
uint32_t runCalls = 0U, longestRunMicros = 0U;
uint32_t signatureCalls = 0U, signaturePassed = 0U, longestSignatureMicros = 0U;
wio_task_probe::Resources resources;
wio_task_probe::TaskSnapshot taskSnapshot;

void onPower(bool value) { currentPower = value; ++powerWrites; power.report(value); }
void onDuty(uint32_t value) { currentDuty = value; ++dutyWrites; duty.report(value); }
uint32_t sampleUptime() { ++samples; return millis() / 1000U; }
void number(const char* key, unsigned long value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":"); Serial.print(value);
}
void observe() {
    wio_task_probe::sample(resources, true);
    wio_task_probe::capture(taskSnapshot);
    const struct mallinfo heap = mallinfo();
    blinker::WallClockSnapshot clock;
    const bool timeValid = Blinker.timeStatus(clock);
    Serial.print("{\"fixture\":\"wio-wifi-task-product\"");
    number("compileOnly", GATE_COMPILE_ONLY); number("begun", begun);
    number("runCalls", runCalls); number("longestRunMicros", longestRunMicros);
    number("power", currentPower); number("duty", currentDuty);
    number("powerWrites", powerWrites); number("dutyWrites", dutyWrites); number("samples", samples);
    number("timeValid", timeValid); number("timeSource", static_cast<unsigned>(clock.source));
    number("signatureCalls", signatureCalls); number("signaturePassed", signaturePassed);
    number("longestSignatureMicros", longestSignatureMicros);
    number("resourceSamples", resources.samples); number("sketchStackLowestBytes", resources.lowestSketchStackBytes);
    number("rtosFreeBytes", xPortGetFreeHeapSize()); number("rtosMinimumEverFreeBytes", xPortGetMinimumEverFreeHeapSize());
    number("rtosLowestObservedFreeBytes", resources.lowestObservedRtosFree);
    number("newlibValid", resources.newlibValid && heap.uordblks >= 0 && heap.fordblks >= 0 && heap.arena >= 0);
    number("newlibUsedBytes", heap.uordblks >= 0 ? static_cast<unsigned>(heap.uordblks) : 0U);
    number("newlibFreeBytes", heap.fordblks >= 0 ? static_cast<unsigned>(heap.fordblks) : 0U);
    number("newlibArenaBytes", heap.arena >= 0 ? static_cast<unsigned>(heap.arena) : 0U);
    number("newlibMaximumObservedUsedBytes", resources.maximumObservedNewlibUsed);
    number("taskLimit", wio_task_probe::taskLimit); number("taskCount", taskSnapshot.total);
    number("tasksCaptured", taskSnapshot.captured); number("tasksTruncated", taskSnapshot.truncated);
    number("traceWorkspaceBytes", sizeof(taskSnapshot)); number("stackWordBytes", sizeof(StackType_t));
    Serial.print(",\"tasks\":[");
    for (UBaseType_t i = 0U; i < taskSnapshot.captured && i < wio_task_probe::taskLimit; ++i) {
        const TaskStatus_t& task = taskSnapshot.records[i];
        if (i != 0U) Serial.print(',');
        Serial.print("{\"number\":"); Serial.print(task.xTaskNumber);
        number("role", taskSnapshot.roles[i]); number("current", task.xHandle == taskSnapshot.current);
        number("priority", task.uxCurrentPriority); number("state", task.eCurrentState);
        number("stackFreeBytes", wio_task_probe::stackBytes(task.usStackHighWaterMark)); Serial.print('}');
    }
    Serial.println("]}");
}
void startProduct() {
    power.onWrite(onPower); duty.onWrite(onDuty); uptimeSeconds.onSample(sampleUptime);
    bool ready = Blinker.enableTime();
#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
    ready = ready && Blinker.enableTasks(power, duty);
#endif
    begun = ready && Blinker.begin(GATE_DEVICE_KEY, GATE_WIFI_SSID, GATE_WIFI_PASSWORD,
                                  power, duty, uptimeSeconds);
    if (begun) Blinker.report(power, currentPower, duty, currentDuty);
}
void runProduct() {
    wio_task_probe::sample(resources);
    const uint32_t started = micros();
    if (begun) Blinker.run();
    const uint32_t elapsed = static_cast<uint32_t>(micros() - started);
    if (elapsed > longestRunMicros) longestRunMicros = elapsed;
    ++runCalls; wio_task_probe::sample(resources);
}
// Retain identical real product methods in a compile-only image without doing
// network/storage I/O. These pointers are diagnostic-gate overhead, not library RAM.
void (*volatile retainStart)() = &startProduct;
void (*volatile retainRun)() = &runProduct;
}

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
// Original SAMD GCC7/Crypto symbol: delegate exactly once with unchanged args
// and result. The build gate independently checks --wrap and the real callee.
extern "C" bool __real__ZN7Ed255196verifyEPKhS1_PKvj(const uint8_t*, const uint8_t*, const void*, unsigned);
extern "C" bool __wrap__ZN7Ed255196verifyEPKhS1_PKvj(
    const uint8_t* signature, const uint8_t* key, const void* message, unsigned size) {
    wio_task_probe::sample(resources, true);
    ++signatureCalls; const uint32_t started = micros();
    const bool accepted = __real__ZN7Ed255196verifyEPKhS1_PKvj(signature, key, message, size);
    const uint32_t elapsed = static_cast<uint32_t>(micros() - started);
    if (elapsed > longestSignatureMicros) longestSignatureMicros = elapsed;
    if (accepted) ++signaturePassed;
    wio_task_probe::sample(resources, true); return accepted;
}
#endif

void setup() {
    Serial.begin(115200U);
    (void)retainStart; (void)retainRun;
#if !GATE_COMPILE_ONLY
    startProduct();
#endif
}
void loop() {
#if !GATE_COMPILE_ONLY
    runProduct();
#endif
    if (Serial.available() <= 0) return;
    const int command = Serial.read();
    if (command == 'j') observe(); // Read-only metrics. No reset, arm, Initialize or Start command.
}
