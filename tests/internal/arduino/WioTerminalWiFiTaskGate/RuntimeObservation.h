#ifndef BLINKER_TEST_WIO_TASK_RUNTIME_OBSERVATION_H
#define BLINKER_TEST_WIO_TASK_RUNTIME_OBSERVATION_H

#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <malloc.h>
#include <limits.h>
#include <string.h>

#if configUSE_TRACE_FACILITY != 1 || INCLUDE_uxTaskGetStackHighWaterMark != 1
#error "Wio runtime gate requires the existing SDK task trace and stack watermark APIs"
#endif

namespace wio_task_probe {

// Measurement-only fixed workspace. It does not create a thread or change the
// SDK's 48 KiB RTOS heap, runClient/runServer stacks, priorities or timeouts.
enum : UBaseType_t { taskLimit = 8U };
enum TaskRole : uint8_t { Other = 0U, Sketch = 1U, RpcServer = 2U, Idle = 3U, Timer = 4U };
struct TaskSnapshot {
    TaskStatus_t records[taskLimit];
    uint8_t roles[taskLimit];
    UBaseType_t total, captured;
    TaskHandle_t current;
    bool truncated;
    TaskSnapshot() : records(), roles(), total(0U), captured(0U), current(nullptr), truncated(false) {}
};
struct Resources {
    uint32_t samples, lastSampleMillis, lowestSketchStackBytes;
    size_t lowestObservedRtosFree, maximumObservedNewlibUsed;
    bool newlibValid;
    Resources() : samples(0U), lastSampleMillis(0U), lowestSketchStackBytes(UINT32_MAX),
        lowestObservedRtosFree(SIZE_MAX), maximumObservedNewlibUsed(0U), newlibValid(true) {}
};

inline uint32_t stackBytes(UBaseType_t words) {
    return static_cast<uint32_t>(words) * static_cast<uint32_t>(sizeof(StackType_t));
}
inline void sample(Resources& value, bool force = false) {
    const uint32_t now = millis();
    if (!force && value.samples != 0U && static_cast<uint32_t>(now - value.lastSampleMillis) < 100U) return;
    value.lastSampleMillis = now; ++value.samples;
    const uint32_t stack = stackBytes(uxTaskGetStackHighWaterMark(nullptr));
    if (stack < value.lowestSketchStackBytes) value.lowestSketchStackBytes = stack;
    const size_t rtos = xPortGetFreeHeapSize();
    if (rtos < value.lowestObservedRtosFree) value.lowestObservedRtosFree = rtos;
    const struct mallinfo heap = mallinfo();
    if (heap.uordblks < 0 || heap.fordblks < 0 || heap.arena < 0) value.newlibValid = false;
    else if (static_cast<size_t>(heap.uordblks) > value.maximumObservedNewlibUsed)
        value.maximumObservedNewlibUsed = static_cast<size_t>(heap.uordblks);
}
inline uint8_t taskRole(const char* name) {
    if (name == nullptr) return Other;
    if (strcmp(name, "runClient") == 0) return Sketch;
    if (strcmp(name, "runServer") == 0) return RpcServer;
    if (strcmp(name, "IDLE") == 0) return Idle;
    if (strcmp(name, "Tmr Svc") == 0) return Timer;
    return Other; // Do not export arbitrary user or SDK task-name strings.
}
inline void capture(TaskSnapshot& value) {
    // No Serial, allocation or blocking SDK calls while scheduler is held.
    // uxTaskGetSystemState's nested suspend/resume is supported by this SDK.
    vTaskSuspendAll();
    value.current = xTaskGetCurrentTaskHandle();
    value.total = uxTaskGetNumberOfTasks();
    value.captured = uxTaskGetSystemState(value.records, taskLimit, nullptr);
    value.truncated = value.total > taskLimit || value.captured != value.total;
    for (UBaseType_t i = 0U; i < value.captured && i < taskLimit; ++i)
        value.roles[i] = taskRole(value.records[i].pcTaskName);
    (void)xTaskResumeAll();
    // If SDK returns zero when the eight-entry array is too small, report zero
    // captured plus the actual count/truncation. Never fabricate a partial list.
}

} // namespace wio_task_probe
#endif
