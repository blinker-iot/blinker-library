// Internal finite product gate. Private configuration is generated in build/.
// Uses the original public task/control/realtime chain; no boot-time Start.
#include "GateConfig.h"
#define BLINKER_ENABLE_TASKS 1
#include <BlinkerWiFi.h>
#include "TaskAllocationObserver.h"
#if defined(BLINKER_INTERNAL_RPC_FAULT_GATE) && BLINKER_INTERNAL_RPC_FAULT_GATE
#include "TaskRpcFaultObserver.h"
#endif
#include <malloc.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(duty, uint32_t,
    blinker::rangedProperty(blinker::PropertyMode::ReadWrite, 0.0, 100.0, 5.0));
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));

namespace {
bool begun = false, currentPower = false;
uint32_t currentDuty = 0U, powerWrites = 0U, dutyWrites = 0U, samples = 0U;
uint32_t loops = 0U, longestRunMicros = 0U;
uint32_t signatureCalls = 0U, signaturePassed = 0U, longestSignatureMicros = 0U;
int32_t minimumSignatureEntryGap = INT32_MAX;
void onPower(bool value) { currentPower = value; ++powerWrites; power.report(value); }
void onDuty(uint32_t value) { currentDuty = value; ++dutyWrites; duty.report(value); }
uint32_t sampleUptime() { ++samples; return millis() / 1000U; }
void number(const char* key, unsigned long value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":"); Serial.print(value);
}
void observe() {
    task_probe::observing = false; // Serial formatting is not a product sample.
    const task_probe::AllocationObservation value = task_probe::allocation;
    const struct mallinfo heap = mallinfo();
    blinker::WallClockSnapshot time;
    const bool valid = Blinker.timeStatus(time);
    Serial.print("{\"fixture\":\"r4-wifi-task-product\"");
    number("begun", begun); number("loops", loops); number("power", currentPower);
    number("duty", currentDuty); number("powerWrites", powerWrites); number("dutyWrites", dutyWrites);
    number("samples", samples); number("timeValid", valid); number("timeSource", static_cast<unsigned>(time.source));
    number("longestRunMicros", longestRunMicros); number("signatureCalls", signatureCalls);
    number("signaturePassed", signaturePassed); number("longestSignatureMicros", longestSignatureMicros);
    number("mallocCalls", value.mallocCalls); number("reallocCalls", value.reallocCalls);
    number("freeCalls", value.freeCalls); number("allocationSamples", value.samples);
    number("invalidBreakSamples", value.invalidBreakSamples);
    Serial.print(",\"minimumAllocationGap\":"); Serial.print(value.minimumGap);
    Serial.print(",\"minimumSignatureEntryGap\":"); Serial.print(minimumSignatureEntryGap);
    number("heapUsed", heap.uordblks); number("heapFree", heap.fordblks); number("heapArena", heap.arena);
#if defined(BLINKER_INTERNAL_RPC_FAULT_GATE) && BLINKER_INTERNAL_RPC_FAULT_GATE
    const task_rpc_probe::Observation& rpc = task_rpc_probe::observation;
    const blinker::RenesasUnoModemOwner& owner = blinker::renesasUnoModemOwner();
    number("rpcWriteCalls", rpc.writeCalls); number("rpcWriteNowaitCalls", rpc.writeNowaitCalls);
    number("rpcPassthroughCalls", rpc.passthroughCalls); number("rpcTaskPutCalls", rpc.taskPutCalls);
    number("rpcTaskPayloadCalls", rpc.taskPayloadCalls); number("rpcArmUsed", rpc.armUsed);
    number("rpcArmed", rpc.armed); number("rpcInjections", rpc.injections);
    number("rpcInjectedReplyValid", rpc.injectedReplyValid); number("rpcInjectedAccepted", rpc.injectedAccepted);
    number("rpcInjectedElapsedMicros", rpc.injectedElapsedMicros);
    number("modemQuarantined", owner.quarantined()); number("modemOutcome", owner.outcome());
    number("modemNetworkAllowed", owner.networkAllowed());
#endif
    Serial.println('}');
    task_probe::observing = true;
}
}

// The selected Core/Crypto mangled symbol is checked by the finite build gate.
// This test-only wrapper delegates exactly once, without changing verification.
extern "C" bool __real__ZN7Ed255196verifyEPKhS1_PKvj(const uint8_t*, const uint8_t*, const void*, unsigned);
extern "C" bool __wrap__ZN7Ed255196verifyEPKhS1_PKvj(
    const uint8_t* signature, const uint8_t* key, const void* message, unsigned size) {
    task_probe::sampleAllocationBoundary();
    const uintptr_t heapBreak = reinterpret_cast<uintptr_t>(_sbrk(0));
    if (heapBreak != 0U && heapBreak != UINTPTR_MAX) {
        const int32_t gap = static_cast<int32_t>(task_probe::currentMsp() - heapBreak);
        if (gap < minimumSignatureEntryGap) minimumSignatureEntryGap = gap;
    }
    ++signatureCalls;
    const uint32_t started = micros();
    const bool accepted = __real__ZN7Ed255196verifyEPKhS1_PKvj(signature, key, message, size);
    const uint32_t elapsed = static_cast<uint32_t>(micros()) - started;
    if (elapsed > longestSignatureMicros) longestSignatureMicros = elapsed;
    if (accepted) ++signaturePassed;
    task_probe::sampleAllocationBoundary();
    return accepted;
}

void setup() {
    Serial.begin(115200);
    task_probe::startObservation();
    power.onWrite(onPower); duty.onWrite(onDuty); uptimeSeconds.onSample(sampleUptime);
    begun = Blinker.enableTime() && Blinker.enableTasks(power, duty) &&
        Blinker.begin(GATE_DEVICE_KEY, GATE_WIFI_SSID, GATE_WIFI_PASSWORD, power, duty, uptimeSeconds);
    if (begun) Blinker.report(power, currentPower, duty, currentDuty);
}
void loop() {
    task_probe::sampleAllocationBoundary();
    const uint32_t started = micros();
    if (begun) Blinker.run();
    const uint32_t elapsed = static_cast<uint32_t>(micros()) - started;
    if (elapsed > longestRunMicros) longestRunMicros = elapsed;
    ++loops;
    if (Serial.available() <= 0) return;
    const int command = Serial.read();
    if (command == 'j') observe();
#if defined(BLINKER_INTERNAL_RPC_FAULT_GATE) && BLINKER_INTERNAL_RPC_FAULT_GATE
    else if (command == 'f') {
        task_probe::observing = false;
        Serial.print("{\"event\":\"rpc_fault_arm\",\"accepted\":");
        Serial.print(task_rpc_probe::arm() ? 1U : 0U);
        Serial.println(",\"automaticWrite\":false}");
        task_probe::observing = true;
    }
#endif
    else if (command == 'd') {
        task_probe::observing = false; Blinker.printDiagnostics(Serial); task_probe::observing = true;
    } else if (command == 'r') NVIC_SystemReset();
}
