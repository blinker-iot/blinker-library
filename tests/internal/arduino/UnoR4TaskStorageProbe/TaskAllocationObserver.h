#ifndef BLINKER_TEST_R4_TASK_ALLOCATION_OBSERVER_H
#define BLINKER_TEST_R4_TASK_ALLOCATION_OBSERVER_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#ifndef BLINKER_INTERNAL_TASK_ALLOCATION_OBSERVER
#define BLINKER_INTERNAL_TASK_ALLOCATION_OBSERVER 0
#endif

extern "C" void* _sbrk(ptrdiff_t increment);

namespace task_probe {
inline uintptr_t currentMsp() {
    uintptr_t value = 0U;
    __asm volatile("mrs %0, msp" : "=r"(value));
    return value;
}
struct AllocationObservation {
    uint32_t mallocCalls, reallocCalls, freeCalls, samples, invalidBreakSamples;
    uintptr_t minimumMsp;
    int32_t minimumGap;
    AllocationObservation() : mallocCalls(0U), reallocCalls(0U), freeCalls(0U), samples(0U), invalidBreakSamples(0U),
        minimumMsp(UINTPTR_MAX), minimumGap(INT32_MAX) {}
};
static AllocationObservation allocation;
static bool observing = false;
inline void sampleAllocationBoundary() {
    if (!observing) return;
    const uintptr_t msp = currentMsp();
    const uintptr_t heapBreak = reinterpret_cast<uintptr_t>(_sbrk(0));
    ++allocation.samples;
    if (msp < allocation.minimumMsp) allocation.minimumMsp = msp;
    if (heapBreak == UINTPTR_MAX || heapBreak == 0U) { ++allocation.invalidBreakSamples; return; }
    const int32_t gap = static_cast<int32_t>(msp - heapBreak);
    if (gap < allocation.minimumGap) allocation.minimumGap = gap;
}
inline void startObservation() {
    allocation = AllocationObservation(); observing = true; sampleAllocationBoundary();
}
inline void endObservation() { sampleAllocationBoundary(); observing = false; }
} // namespace task_probe

#if BLINKER_INTERNAL_TASK_ALLOCATION_OBSERVER
// Test-only GNU --wrap hooks. Delegate every call; never allocate, print,
// paint or scan memory here. Boundaries are NOT the complete call-depth peak.
struct _reent;
extern "C" void* __real__malloc_r(struct _reent*, size_t);
extern "C" void* __real__realloc_r(struct _reent*, void*, size_t);
extern "C" void __real__free_r(struct _reent*, void*);
extern "C" void* __wrap__malloc_r(struct _reent* context, size_t size) {
    if (task_probe::observing) ++task_probe::allocation.mallocCalls;
    task_probe::sampleAllocationBoundary();
    void* value = __real__malloc_r(context, size);
    task_probe::sampleAllocationBoundary();
    return value;
}
extern "C" void* __wrap__realloc_r(struct _reent* context, void* pointer, size_t size) {
    if (task_probe::observing) ++task_probe::allocation.reallocCalls;
    task_probe::sampleAllocationBoundary();
    void* value = __real__realloc_r(context, pointer, size);
    task_probe::sampleAllocationBoundary();
    return value;
}
extern "C" void __wrap__free_r(struct _reent* context, void* pointer) {
    if (task_probe::observing) ++task_probe::allocation.freeCalls;
    task_probe::sampleAllocationBoundary();
    __real__free_r(context, pointer);
    task_probe::sampleAllocationBoundary();
}
#endif
#endif
