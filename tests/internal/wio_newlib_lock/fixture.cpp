#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalNewlibLock.h>
#include <Arduino.h>
#include <task.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

struct _reent;
extern "C" void __malloc_lock(struct _reent*);
extern "C" void __malloc_unlock(struct _reent*);

namespace {
bool schedulerStarted = false;
bool irq = false;
unsigned depth = 0;
unsigned suspends = 0;
unsigned resumes = 0;
unsigned stateReads = 0;
unsigned irqMask = 0;
unsigned irqDisables = 0;
unsigned allocations = 0;
bool watchAllocations = false;
bool fixtureFailure = false;

int require(bool condition, const char* message) {
    if (condition) return 0;
    std::fprintf(stderr, "%s\n", message);
    return 1;
}
void rejectIrqSchedulerCall() {
    if (irq) {
        std::fputs("scheduler-call-in-irq\n", stderr);
        std::exit(87);
    }
}
} // namespace

void* operator new(std::size_t size) {
    if (watchAllocations) ++allocations;
    void* value = std::malloc(size);
    if (!value) throw std::bad_alloc();
    return value;
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }

uint32_t __get_IPSR() {
    if (irq) {
        std::fputs("irq-context-entered\n", stdout);
        std::fflush(stdout);
    }
    return irq ? 16U : 0U;
}
void __disable_irq() {
    ++irqDisables;
    irqMask = 1U;
    if (irq) {
        std::fputs("irq-mask-disabled\n", stdout);
        std::fflush(stdout);
    }
}
void __NOP() {
    static bool announced = false;
    if (irq && !announced) {
        announced = true;
        std::fputs("irq-failstop-loop\n", stdout);
        std::fflush(stdout);
    }
}
BaseType_t xTaskGetSchedulerState() {
    rejectIrqSchedulerCall();
    ++stateReads;
    return !schedulerStarted ? taskSCHEDULER_NOT_STARTED :
        (depth ? taskSCHEDULER_SUSPENDED : taskSCHEDULER_RUNNING);
}
void vTaskSuspendAll() {
    rejectIrqSchedulerCall();
    ++suspends;
    ++depth;
    // Model the original cold FreeRTOS critical-nesting hazard. A cold hook
    // must never reach resume, which would leave the interrupt mask changed.
}
BaseType_t xTaskResumeAll() {
    rejectIrqSchedulerCall();
    ++resumes;
    if (!depth) fixtureFailure = true;
    else --depth;
    if (!schedulerStarted) irqMask = 0x80U;
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    blinker_wio_newlib_lock_anchor();
    const char* mode = argv[1];
    if (std::strcmp(mode, "irq-lock") == 0 || std::strcmp(mode, "irq-unlock") == 0) {
        irq = true;
        if (std::strcmp(mode, "irq-lock") == 0) __malloc_lock(nullptr);
        else __malloc_unlock(nullptr);
        return 88; // Returning from a void IRQ hook must never look successful.
    }
    schedulerStarted = std::strcmp(mode, "cold") != 0;
    // Cold hooks must not enter the SDK's uninitialized critical section.
    // Running tests assume the normal RTOS thread context, not a hand-written
    // BASEPRI mask: native FreeRTOS owns that port's critical-section semantics.
    const unsigned initialMask = schedulerStarted ? 0U : 0x20U;
    irqMask = initialMask;
    if (std::strcmp(mode, "suspended") == 0) depth = 2U;
    const unsigned initialDepth = depth;
    const bool nested = std::strcmp(mode, "nested") == 0;
    watchAllocations = true;
    __malloc_lock(nullptr);
    if (nested) __malloc_lock(nullptr);
    __malloc_unlock(nullptr);
    if (nested) __malloc_unlock(nullptr);
    watchAllocations = false;
    const unsigned expectedCalls = schedulerStarted ? (nested ? 2U : 1U) : 0U;
    return require(!fixtureFailure, "RTOS nesting underflow") |
        require(depth == initialDepth, "caller suspension nesting changed") |
        require(suspends == expectedCalls && resumes == expectedCalls,
            "unexpected suspend/resume count") |
        require(stateReads == (nested ? 4U : 2U), "scheduler state not checked for each hook") |
        require(irqMask == initialMask, "hook changed the fixture IRQ mask") |
        require(irqDisables == 0U, "ordinary path disabled IRQs") |
        require(allocations == 0U, "hook allocated dynamically");
}
