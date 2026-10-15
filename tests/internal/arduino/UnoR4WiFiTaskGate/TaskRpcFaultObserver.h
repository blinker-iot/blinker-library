#ifndef BLINKER_TEST_TASK_RPC_FAULT_OBSERVER_H
#define BLINKER_TEST_TASK_RPC_FAULT_OBSERVER_H

// Linker-wrap observer for the finite R4 hardware gate only. No production hook,
// fake Result, skipped payload, UART flushing, automatic write or fault unlock.
// --wrap covers external references, not necessarily same-TU SDK calls.
#if defined(BLINKER_INTERNAL_RPC_FAULT_GATE) && BLINKER_INTERNAL_RPC_FAULT_GATE

#include <Modem.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemOwner.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace task_rpc_probe {

struct Observation {
    uint32_t writeCalls, writeNowaitCalls, passthroughCalls;
    uint32_t taskPutCalls, taskPayloadCalls, injections, injectedElapsedMicros;
    bool armUsed, armed, pendingTaskHeader, injectedReplyValid, injectedAccepted;
    Observation()
        : writeCalls(0U), writeNowaitCalls(0U), passthroughCalls(0U),
          taskPutCalls(0U), taskPayloadCalls(0U), injections(0U), injectedElapsedMicros(0U),
          armUsed(false), armed(false), pendingTaskHeader(false),
          injectedReplyValid(false), injectedAccepted(false) {}
};

static Observation observation;

inline bool arm() {
    if (observation.armUsed || !blinker::renesasUnoModemOwner().networkAllowed()) return false;
    observation.armUsed = true;
    observation.armed = true;
    return true; // Only arm the next task PUT; never issue a modem command here.
}

inline bool taskPutHeader(const char* command) {
    static const char prefix[] = CMD_WRITE(_PREF_PUT);
    const size_t prefixSize = sizeof(prefix) - 1U;
    if (strncmp(command, prefix, prefixSize) != 0) return false;
    const char* key = command + prefixSize;
    return strncmp(key, "ts0,", 4U) == 0 || strncmp(key, "ts1,", 4U) == 0;
}

inline bool taskJournalPayload(const uint8_t* data, size_t size) {
    return data != nullptr && size >= 20U && memcmp(data, "BRJ2", 4U) == 0 &&
           data[4] == 1U && data[5] == 5U;
}

} // namespace task_rpc_probe

// The explicit self argument is the original C++ method's hidden this pointer.
// Symbols are frozen to the existing Renesas UNO 1.5.3 GCC ABI, not a new SDK.
extern "C" bool __real__ZN10ModemClass5writeERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
    ModemClass*, const std::string&, std::string&, const char*, ...);
extern "C" void __real__ZN10ModemClass12write_nowaitERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
    ModemClass*, const std::string&, std::string&, const char*, ...);
extern "C" bool __real__ZN10ModemClass11passthroughEPKhj(ModemClass*, const uint8_t*, size_t);

extern "C" bool __wrap__ZN10ModemClass5writeERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
    ModemClass* self, const std::string& prompt, std::string& response, const char* format, ...) {
    // Same 128-byte bounded formatting as the SDK, forwarded once. Never print
    // this buffer: it can contain credentials or private protocol material.
    char command[MAX_BUFF_SIZE];
    va_list arguments;
    va_start(arguments, format);
    (void)vsnprintf(command, sizeof(command), format, arguments);
    va_end(arguments);
    ++task_rpc_probe::observation.writeCalls;
    return __real__ZN10ModemClass5writeERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
        self, prompt, response, "%s", command);
}

extern "C" void __wrap__ZN10ModemClass12write_nowaitERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
    ModemClass* self, const std::string& prompt, std::string& response, const char* format, ...) {
    char command[MAX_BUFF_SIZE];
    va_list arguments;
    va_start(arguments, format);
    (void)vsnprintf(command, sizeof(command), format, arguments);
    va_end(arguments);
    task_rpc_probe::Observation& observed = task_rpc_probe::observation;
    ++observed.writeNowaitCalls;
    observed.pendingTaskHeader = task_rpc_probe::taskPutHeader(command);
    if (observed.pendingTaskHeader) ++observed.taskPutCalls;
    __real__ZN10ModemClass12write_nowaitERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
        self, prompt, response, "%s", command);
}

extern "C" bool __wrap__ZN10ModemClass11passthroughEPKhj(
    ModemClass* self, const uint8_t* data, size_t size) {
    task_rpc_probe::Observation& observed = task_rpc_probe::observation;
    ++observed.passthroughCalls;
    const bool taskPayload = observed.pendingTaskHeader && task_rpc_probe::taskJournalPayload(data, size);
    observed.pendingTaskHeader = false;
    if (taskPayload) ++observed.taskPayloadCalls;
    const bool inject = taskPayload && observed.armed;
    if (inject) {
        observed.armed = false; // Consume before the one REAL payload/response call.
        ++observed.injections;
        // Test-only short response allowance. UART sends the entire promised
        // payload first. A true SDK reply stays true; no forced failure/retry.
        self->timeout(0U);
    }
    const uint32_t started = static_cast<uint32_t>(micros());
    const bool accepted = __real__ZN10ModemClass11passthroughEPKhj(self, data, size);
    const uint32_t elapsed = static_cast<uint32_t>(micros()) - started;
    if (inject) {
        observed.injectedReplyValid = true;
        observed.injectedAccepted = accepted;
        observed.injectedElapsedMicros = elapsed;
    }
    return accepted;
}

#endif // BLINKER_INTERNAL_RPC_FAULT_GATE
#endif
