// Observer-only software model. This does NOT prove SDK timeout or media durability.
#define BLINKER_INTERNAL_RPC_FAULT_GATE 1
#include "../internal/arduino/UnoR4WiFiTaskGate/TaskRpcFaultObserver.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
ModemClass modem;
uint32_t clockMicros = 0U, payloadMicros = 17U;
unsigned writeCalls = 0U, nowaitCalls = 0U, passthroughCalls = 0U;
bool writeAccepted = true, payloadAccepted = false;
const uint8_t* payloadPointer = nullptr;
size_t payloadSize = 0U;
std::string formattedCommand;
uint32_t nowMillis() { return clockMicros / 1000U; }
void timeout(uint32_t value) { modem.timeout(value); }
void capture(const char* format, va_list arguments) {
    char command[MAX_BUFF_SIZE];
    (void)vsnprintf(command, sizeof(command), format, arguments);
    formattedCommand = command;
}
void header(const char* key) {
    std::string prompt, response;
    __wrap__ZN10ModemClass12write_nowaitERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
        &modem, prompt, response, "%s%s,%d,%d\r\n", CMD_WRITE(_PREF_PUT), key, 9, 20);
}
bool payload(const uint8_t* bytes, size_t size) {
    return __wrap__ZN10ModemClass11passthroughEPKhj(&modem, bytes, size);
}
}

uint32_t micros() { return clockMicros; }
namespace blinker {
RenesasUnoModemOwner& renesasUnoModemOwner() {
    static RenesasUnoModemOwner owner(nowMillis, timeout, 10000U);
    return owner;
}
}

extern "C" bool __real__ZN10ModemClass5writeERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
    ModemClass* self, const std::string&, std::string& response, const char* format, ...) {
    CHECK(self == &modem);
    va_list arguments; va_start(arguments, format); capture(format, arguments); va_end(arguments);
    ++writeCalls; response = "real-response"; return writeAccepted;
}
extern "C" void __real__ZN10ModemClass12write_nowaitERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
    ModemClass* self, const std::string&, std::string&, const char* format, ...) {
    CHECK(self == &modem);
    va_list arguments; va_start(arguments, format); capture(format, arguments); va_end(arguments);
    ++nowaitCalls;
}
extern "C" bool __real__ZN10ModemClass11passthroughEPKhj(
    ModemClass* self, const uint8_t* bytes, size_t size) {
    CHECK(self == &modem);
    ++passthroughCalls; payloadPointer = bytes; payloadSize = size;
    clockMicros += payloadMicros; return payloadAccepted;
}

int main() {
    using task_rpc_probe::observation;
    uint8_t task[20] = {'B', 'R', 'J', '2', 1U, 5U};
    uint8_t credential[20] = {'B', 'R', 'J', '2', 1U, 4U};
    CHECK(!observation.armed && !observation.armUsed && !observation.injectedReplyValid);
    header("ts0");
    CHECK(!payload(task, sizeof(task)) && modem.timeoutCalls == 0U);
    CHECK(observation.taskPutCalls == 1U && observation.taskPayloadCalls == 1U);
    CHECK(task_rpc_probe::arm() && !task_rpc_probe::arm());
    header("ac0");
    CHECK(!payload(credential, sizeof(credential)) && observation.armed && modem.timeoutCalls == 0U);
    header("ts0");
    CHECK(!payload(credential, sizeof(credential)) && observation.armed && modem.timeoutCalls == 0U);
    header("ts1");
    CHECK(formattedCommand == "AT+PREFPUT=ts1,9,20\r\n");
    clockMicros = UINT32_MAX - 5U; payloadMicros = 17U;
    CHECK(!payload(task, sizeof(task)));
    CHECK(payloadPointer == task && payloadSize == sizeof(task));
    CHECK(modem.timeoutCalls == 1U && modem.timeoutMillis == 0U);
    CHECK(!observation.armed && observation.armUsed && !task_rpc_probe::arm());
    CHECK(observation.injections == 1U && observation.injectedReplyValid && !observation.injectedAccepted);
    CHECK(observation.injectedElapsedMicros == 17U);
    payloadAccepted = true;
    header("ts0"); CHECK(payload(task, sizeof(task)) && modem.timeoutCalls == 1U);
    CHECK(observation.injections == 1U && !observation.injectedAccepted); // Preserve injected reply.

    std::string prompt = "+REAL:", response;
    const std::string longText(160U, 'a');
    writeAccepted = false;
    CHECK(!__wrap__ZN10ModemClass5writeERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERS5_PKcz(
        &modem, prompt, response, "%s:%u", longText.c_str(), 8U));
    CHECK(response == "real-response" && formattedCommand.size() == MAX_BUFF_SIZE - 1U);
    CHECK(observation.writeCalls == writeCalls && observation.writeNowaitCalls == nowaitCalls);
    CHECK(observation.passthroughCalls == passthroughCalls);

    // Reset only the test's simulated MCU RAM, not the production owner.
    observation = task_rpc_probe::Observation();
    CHECK(!observation.armed && !observation.armUsed && task_rpc_probe::arm());
    header("ts0");
    CHECK(payload(task, sizeof(task)) && observation.injectedAccepted && observation.injections == 1U);
    CHECK(!task_rpc_probe::arm()); // A true real result is never changed to failure.
    std::cout << "Task RPC observer finite model passed; not a hardware timeout proof\n";
}
