#include "Arduino.h"
namespace host_rpc {
uint32_t clockMs = 0, primask = 0, ipsr = 0, basepri = 0;
int schedulerState = taskSCHEDULER_RUNNING;
uintptr_t currentId = 2;
unsigned pauseCalls = 0, tryLockCalls = 0, heldLocks = 0, unsafeDriverCalls = 0;
bool failAllTryLocks = false, observeHardware = false, dre = true, uartError = false, frameError = false;
std::function<void()> onPause;
std::function<void(uint32_t)> onSemaphoreWait;
std::function<void()> afterSignal;
std::vector<std::string> hardwareEvents;
std::vector<uint8_t> transmitted;
std::deque<uint8_t> incoming;
Register& Register::operator=(uint32_t next) {
    value = next;
    if (observeHardware) {
        if (primask != 1U) throw std::runtime_error("unguarded hardware mutation");
        hardwareEvents.push_back(name);
        if (std::string(name) == "data") transmitted.push_back(static_cast<uint8_t>(next));
    }
    return *this;
}
}
Sercom hostSercom0;
FakePort hostPort;
bool hostIrqEnabled[4] = {true, true, true, true};
bool hostIrqCleared[4] = {false, false, false, false};
PinDescription g_APinDescription[86] = {};
void NVIC_DisableIRQ(IRQn_Type irq) {
    if (host_rpc::primask != 1U) throw std::runtime_error("unguarded IRQ disable");
    hostIrqEnabled[int(irq) - 46] = false;
    host_rpc::hardwareEvents.push_back("disable-" + std::to_string(int(irq)));
}
void NVIC_ClearPendingIRQ(IRQn_Type irq) {
    if (host_rpc::primask != 1U) throw std::runtime_error("unguarded IRQ pending clear");
    hostIrqCleared[int(irq) - 46] = true;
    host_rpc::hardwareEvents.push_back("clear-" + std::to_string(int(irq)));
}
void delay(uint32_t value) {
    if (host_rpc::primask != 0U) throw std::runtime_error("pause while IRQs held");
    host_rpc::clockMs += value;
    ++host_rpc::pauseCalls;
    if (host_rpc::onPause) host_rpc::onPause();
}
