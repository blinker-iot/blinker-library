#ifndef BLINKER_TEST_R4_TASK_HEAP_BOUNDARY_H
#define BLINKER_TEST_R4_TASK_HEAP_BOUNDARY_H

#if defined(BLINKER_INTERNAL_R4_HEAP_BOUNDARY) && \
    BLINKER_INTERNAL_R4_HEAP_BOUNDARY != 0 && BLINKER_INTERNAL_R4_HEAP_BOUNDARY != 1
#error "R4 heap boundary must be explicitly disabled or enabled"
#endif

#if defined(BLINKER_INTERNAL_R4_HEAP_BOUNDARY) && BLINKER_INTERNAL_R4_HEAP_BOUNDARY == 1
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4HeapBoundary.h>
#include <errno.h>

extern "C" {
extern unsigned char __HeapBase, __HeapLimit, __StackTop;
// Required absolute final-link symbol; deliberately no weak/default value.
// An internal fixture can use --defsym=__blinker_r4_heap_limit=__StackTop-4096.
// That example is not a measured or supported production stack budget.
extern unsigned char __blinker_r4_heap_limit;
void* __real__sbrk(ptrdiff_t increment);
}

namespace task_heap_boundary {
// Shared by the thin linker entry and synthetic host tests. Original _sbrk
// owns all break state. No retry, allocation, logging, new-handler or recovery.
// One cooperative caller, no IRQ/RTOS allocator: query then delegate is not
// atomic. This does not add allocator locking or protect free-list reuse.
inline void* checkedSbrk(ptrdiff_t increment, uintptr_t heapBase,
    uintptr_t heapLimit, uintptr_t stackTop, uintptr_t cap) {
    void* const current = __real__sbrk(0);
    if (current == reinterpret_cast<void*>(UINTPTR_MAX)) return current;
    if (!blinker::unoR4HeapIncrementAllowed(heapBase, heapLimit, stackTop, cap,
            reinterpret_cast<uintptr_t>(current), increment)) {
        errno = ENOMEM;
        return reinterpret_cast<void*>(UINTPTR_MAX);
    }
    return increment == 0 ? current : __real__sbrk(increment);
}
} // namespace task_heap_boundary

// Include once in the internal sketch; --wrap=_sbrk applies at final link, not
// just to Sketch allocations. Actual libc call redirection needs ELF auditing.
extern "C" void* __wrap__sbrk(ptrdiff_t increment) {
    return task_heap_boundary::checkedSbrk(increment,
        reinterpret_cast<uintptr_t>(&__HeapBase), reinterpret_cast<uintptr_t>(&__HeapLimit),
        reinterpret_cast<uintptr_t>(&__StackTop), reinterpret_cast<uintptr_t>(&__blinker_r4_heap_limit));
}
#endif
#endif
