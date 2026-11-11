#ifndef BLINKER_WIO_TERMINAL_NEWLIB_LOCK_INC
#define BLINKER_WIO_TERMINAL_NEWLIB_LOCK_INC

#if (defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)) && \
    defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS

#if BLINKER_ENABLE_TASKS != 1 || \
    !defined(BLINKER_INTERNAL_WIO_TASKS) || BLINKER_INTERNAL_WIO_TASKS != 1 || \
    !defined(BLINKER_TASK_CAPACITY) || BLINKER_TASK_CAPACITY != 4 || \
    !defined(BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED) || \
    BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED != 1
#error "Wio newlib locking requires the complete internal four-slot task graph"
#endif

#include "WioTerminalNewlibLock.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

#if INCLUDE_xTaskGetSchedulerState != 1 && configUSE_TIMERS != 1
#error "Wio newlib locking requires the original scheduler-state API"
#endif

struct _reent;

namespace {

[[noreturn]] void wioAllocatorContextFault() {
    // Allocating from an IRQ cannot safely wait or report an allocator failure.
    __disable_irq();
    for (;;) { __NOP(); }
}

bool wioAllocatorSchedulerStarted() {
    if (__get_IPSR() != 0U) wioAllocatorContextFault();
    const BaseType_t state = xTaskGetSchedulerState();
    if (state == taskSCHEDULER_NOT_STARTED) return false;
    if (state != taskSCHEDULER_RUNNING && state != taskSCHEDULER_SUSPENDED)
        wioAllocatorContextFault();
    return true;
}

} // namespace

// Only newlib heap metadata is serialized, not errno or all newlib services.
// Native scheduler nesting preserves recursive and already-suspended callers.
extern "C" void __malloc_lock(struct _reent*) {
    if (wioAllocatorSchedulerStarted()) vTaskSuspendAll();
}

extern "C" void __malloc_unlock(struct _reent*) {
    if (wioAllocatorSchedulerStarted()) (void)xTaskResumeAll();
}

extern "C" void blinker_wio_newlib_lock_anchor() {}

#endif
#endif
