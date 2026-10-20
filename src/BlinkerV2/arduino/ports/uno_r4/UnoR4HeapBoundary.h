#ifndef BLINKER_UNO_R4_HEAP_BOUNDARY_H
#define BLINKER_UNO_R4_HEAP_BOUNDARY_H

#include <stddef.h>
#include <stdint.h>

namespace blinker {

// Pure fixed-partition arithmetic, not a stack monitor or an allocator. The
// explicit cap reserves StackTop-cap bytes; no production reserve is selected
// here. Reusing allocated blocks and future stack/IRQ depth remain unprotected.
inline bool unoR4HeapIncrementAllowed(uintptr_t heapBase, uintptr_t heapLimit,
    uintptr_t stackTop, uintptr_t cap, uintptr_t currentBreak, ptrdiff_t increment) {
    if (heapBase == 0U || ((heapBase | heapLimit | stackTop | cap | currentBreak) & 3U) != 0U
        || heapBase > cap || cap > heapLimit || heapLimit >= stackTop
        || currentBreak < heapBase || currentBreak > heapLimit) return false;

    if (increment <= 0) {
        // Original bsp_sbrk rounds (increment+3)&~3: negative increments round
        // toward zero. Form the magnitude without negating PTRDIFF_MIN.
        const uintptr_t magnitude = increment == 0 ? 0U
            : static_cast<uintptr_t>(-(increment + 1)) + 1U;
        const uintptr_t shrink = magnitude & ~static_cast<uintptr_t>(3U);
        // Query/shrink may inspect or reduce an over-cap break, never grow it.
        return shrink <= currentBreak - heapBase;
    }
    const uintptr_t growth = static_cast<uintptr_t>(increment);
    if (growth > UINTPTR_MAX - 3U || currentBreak > cap) return false;
    const uintptr_t rounded = (growth + 3U) & ~static_cast<uintptr_t>(3U);
    return rounded <= cap - currentBreak;
}

} // namespace blinker
#endif
