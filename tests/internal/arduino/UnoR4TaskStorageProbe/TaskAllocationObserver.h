#ifndef BLINKER_TEST_R4_TASK_ALLOCATION_OBSERVER_H
#define BLINKER_TEST_R4_TASK_ALLOCATION_OBSERVER_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <malloc.h>

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
    uint32_t heapSamples, invalidHeapSamples, nestedSamples, handlerSamples, threadPspSamples;
    size_t maximumObservedHeapUsed, maximumObservedHeapArena;
    uintptr_t minimumMsp, minimumHandlerMsp, maximumObservedHeapBreak;
    int32_t minimumGap;
    AllocationObservation() : mallocCalls(0U), reallocCalls(0U), freeCalls(0U), samples(0U), invalidBreakSamples(0U),
        heapSamples(0U), invalidHeapSamples(0U), nestedSamples(0U), handlerSamples(0U), threadPspSamples(0U),
        maximumObservedHeapUsed(0U), maximumObservedHeapArena(0U), minimumMsp(UINTPTR_MAX),
        minimumHandlerMsp(UINTPTR_MAX), maximumObservedHeapBreak(0U), minimumGap(INT32_MAX) {}
};
static AllocationObservation allocation;
static bool observing = false;
static bool sampling = false;
static uint32_t allocatorDepth = 0U;
inline void sampleAllocationBoundary() {
    if (!observing) return;
    const uintptr_t msp = currentMsp();
    // Do not inspect newlib's free-list from an interrupt. No IRQ is hooked:
    // these counts describe only handlers that happen to reach this observer.
    if (__get_IPSR() != 0U) {
        ++allocation.handlerSamples;
        if (msp < allocation.minimumHandlerMsp) allocation.minimumHandlerMsp = msp;
        return;
    }
    ++allocation.samples;
    if ((__get_CONTROL() & 2U) != 0U) { ++allocation.threadPspSamples; return; }
    if (msp < allocation.minimumMsp) allocation.minimumMsp = msp;
    // realloc may call malloc/free while its allocator state is transitional.
    // Only the outer settled boundaries may inspect that state.
    if (allocatorDepth != 0U || sampling) { ++allocation.nestedSamples; return; }
    sampling = true;
    const uintptr_t heapBreak = reinterpret_cast<uintptr_t>(_sbrk(0));
    if (heapBreak == UINTPTR_MAX || heapBreak == 0U) {
        ++allocation.invalidBreakSamples; sampling = false; return;
    }
    if (heapBreak > allocation.maximumObservedHeapBreak) allocation.maximumObservedHeapBreak = heapBreak;
    const uintptr_t distance = msp >= heapBreak ? msp - heapBreak : heapBreak - msp;
    if (distance > static_cast<uintptr_t>(INT32_MAX)) {
        ++allocation.invalidBreakSamples; sampling = false; return;
    }
    const int32_t gap = msp >= heapBreak ? static_cast<int32_t>(distance) : -static_cast<int32_t>(distance);
    if (gap < allocation.minimumGap) allocation.minimumGap = gap;
    // Core 1.5.3 nano mallinfo does not allocate; its own known free-list walk
    // is not a hard-time-bounded probe. Values exclude transient nested peaks,
    // allocator-internal/IRQ stack depth and the largest contiguous free block.
    const struct mallinfo heap = mallinfo();
    ++allocation.heapSamples;
    // Core 1.5.3 declares these fields size_t (unsigned), not signed int.
    // Reject out-of-arena counters; this is not fragmentation detection.
    if (heap.uordblks > heap.arena || heap.fordblks > heap.arena) {
        ++allocation.invalidHeapSamples;
    } else {
        if (heap.uordblks > allocation.maximumObservedHeapUsed) allocation.maximumObservedHeapUsed = heap.uordblks;
        if (heap.arena > allocation.maximumObservedHeapArena) allocation.maximumObservedHeapArena = heap.arena;
    }
    sampling = false;
}
inline void startObservation() {
    // Restart is permitted only outside an allocator/probe call (setup here).
    if (sampling || allocatorDepth != 0U) return;
    allocation = AllocationObservation(); sampling = false; allocatorDepth = 0U;
    observing = true; sampleAllocationBoundary();
}
inline void endObservation() { sampleAllocationBoundary(); observing = false; }
} // namespace task_probe

#if BLINKER_INTERNAL_TASK_ALLOCATION_OBSERVER
// Test-only GNU --wrap hooks. Delegate every call exactly once; never allocate,
// print, paint or scan arbitrary memory. Boundaries are NOT full stack/heap peaks.
struct _reent;
extern "C" void* __real__malloc_r(struct _reent*, size_t);
extern "C" void* __real__realloc_r(struct _reent*, void*, size_t);
extern "C" void __real__free_r(struct _reent*, void*);
extern "C" void* __wrap__malloc_r(struct _reent* context, size_t size) {
    const bool tracking = task_probe::observing && __get_IPSR() == 0U;
    if (tracking) ++task_probe::allocation.mallocCalls;
    task_probe::sampleAllocationBoundary();
    if (tracking) ++task_probe::allocatorDepth;
    void* value = __real__malloc_r(context, size);
    if (tracking) --task_probe::allocatorDepth;
    task_probe::sampleAllocationBoundary();
    return value;
}
extern "C" void* __wrap__realloc_r(struct _reent* context, void* pointer, size_t size) {
    const bool tracking = task_probe::observing && __get_IPSR() == 0U;
    if (tracking) ++task_probe::allocation.reallocCalls;
    task_probe::sampleAllocationBoundary();
    if (tracking) ++task_probe::allocatorDepth;
    void* value = __real__realloc_r(context, pointer, size);
    if (tracking) --task_probe::allocatorDepth;
    task_probe::sampleAllocationBoundary();
    return value;
}
extern "C" void __wrap__free_r(struct _reent* context, void* pointer) {
    const bool tracking = task_probe::observing && __get_IPSR() == 0U;
    if (tracking) ++task_probe::allocation.freeCalls;
    task_probe::sampleAllocationBoundary();
    if (tracking) ++task_probe::allocatorDepth;
    __real__free_r(context, pointer);
    if (tracking) --task_probe::allocatorDepth;
    task_probe::sampleAllocationBoundary();
}
#endif
#endif
