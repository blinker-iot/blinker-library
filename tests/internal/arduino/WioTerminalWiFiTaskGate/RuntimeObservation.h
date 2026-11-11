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
enum : uint32_t { sampleIntervalMillis = 100U };
enum TaskRole : uint8_t { Other = 0U, Sketch = 1U, RpcServer = 2U, Idle = 3U, Timer = 4U };
struct TaskSnapshot {
    TaskStatus_t records[taskLimit];
    uint8_t roles[taskLimit];
    UBaseType_t total, captured, maximumTaskCount;
    uint32_t captures, incompleteCaptures, captureMicros, longestCaptureMicros;
    TaskHandle_t current;
    bool truncated;
    TaskSnapshot() : records(), roles(), total(0U), captured(0U), maximumTaskCount(0U),
        captures(0U), incompleteCaptures(0U), captureMicros(0U), longestCaptureMicros(0U),
        current(nullptr), truncated(false) {}
};
struct Resources {
    uint32_t samples, firstSampleMillis, lastSampleMillis, maximumSampleGapMillis,
        lowestSketchStackBytes;
    size_t lowestObservedRtosFree, maximumObservedNewlibUsed;
    bool newlibValid;
    Resources() : samples(0U), firstSampleMillis(0U), lastSampleMillis(0U),
        maximumSampleGapMillis(0U), lowestSketchStackBytes(UINT32_MAX),
        lowestObservedRtosFree(SIZE_MAX), maximumObservedNewlibUsed(0U), newlibValid(true) {}
};

inline uint32_t stackBytes(UBaseType_t words) {
    return static_cast<uint32_t>(words) * static_cast<uint32_t>(sizeof(StackType_t));
}
inline struct mallinfo newlibSnapshot() {
    // This fixture samples on runClient; nano newlib's malloc lock is empty.
    // Keep other RTOS tasks from mutating the list during this read. No Serial,
    // allocation or blocking SDK calls here; IRQ safety/latency is not promised.
    vTaskSuspendAll();
    const struct mallinfo value = mallinfo();
    (void)xTaskResumeAll();
    return value;
}
inline bool validNewlibSnapshot(const struct mallinfo& value) {
    // Basic unsigned counter bounds, not a fragmentation or heap-integrity test.
    return value.uordblks <= value.arena && value.fordblks <= value.arena;
}
inline void sample(Resources& value, bool force = false) {
    const uint32_t now = millis();
    const uint32_t gap = static_cast<uint32_t>(now - value.lastSampleMillis);
    if (!force && value.samples != 0U && gap < sampleIntervalMillis) return;
    if (value.samples == 0U) value.firstSampleMillis = now;
    else if (gap > value.maximumSampleGapMillis) value.maximumSampleGapMillis = gap;
    value.lastSampleMillis = now;
    if (value.samples != UINT32_MAX) ++value.samples;
    const uint32_t stack = stackBytes(uxTaskGetStackHighWaterMark(nullptr));
    if (stack < value.lowestSketchStackBytes) value.lowestSketchStackBytes = stack;
    const size_t rtos = xPortGetFreeHeapSize();
    if (rtos < value.lowestObservedRtosFree) value.lowestObservedRtosFree = rtos;
    const struct mallinfo heap = newlibSnapshot();
    if (!validNewlibSnapshot(heap)) value.newlibValid = false;
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
    const uint32_t started = micros();
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
    value.captureMicros = static_cast<uint32_t>(micros() - started);
    if (value.captureMicros > value.longestCaptureMicros) value.longestCaptureMicros = value.captureMicros;
    if (value.total > value.maximumTaskCount) value.maximumTaskCount = value.total;
    if (value.captures != UINT32_MAX) ++value.captures;
    if (value.truncated && value.incompleteCaptures != UINT32_MAX) ++value.incompleteCaptures;
    // If SDK returns zero when the eight-entry array is too small, report zero
    // captured plus the actual count/truncation. Never fabricate a partial list.
    // Elapsed time includes scheduler resume; watermarks cover only these live
    // tasks since their creation, not tasks deleted between explicit captures.
}

} // namespace wio_task_probe
#endif
