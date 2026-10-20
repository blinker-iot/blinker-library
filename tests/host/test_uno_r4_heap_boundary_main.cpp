// Fixed synthetic addresses only; 4096 B is a fixture, not a stack admission.
#define BLINKER_INTERNAL_R4_HEAP_BOUNDARY 1
#include "../internal/arduino/UnoR4WiFiTaskGate/TaskHeapBoundary.h"

#include <cstdlib>
#include <iostream>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

// Host placeholders link the hardware entry only. Its absolute-symbol binding
// and real newlib call graph are checked separately with the original SDK ELF.
extern "C" {
unsigned char __HeapBase = 0U, __HeapLimit = 0U, __StackTop = 0U;
unsigned char __blinker_r4_heap_limit = 0U;
}

namespace {
const uintptr_t base = 0x1000U, limit = 0x3c00U, top = 0x4000U, cap = top - 4096U;
uintptr_t fakeBreak = 0x2000U;
unsigned calls = 0U, changes = 0U;
bool failQuery = false, failChange = false;
ptrdiff_t lastIncrement = 0;
bool allowed(uintptr_t current, ptrdiff_t increment) {
    return blinker::unoR4HeapIncrementAllowed(base, limit, top, cap, current, increment);
}
void reset() {
    fakeBreak = 0x2000U; calls = 0U; changes = 0U;
    failQuery = false; failChange = false; lastIncrement = 0; errno = 0;
}
void* checked(ptrdiff_t increment) {
    return task_heap_boundary::checkedSbrk(increment, base, limit, top, cap);
}
void testBoundsAndAlignment() {
    CHECK(allowed(base, 0) && allowed(cap, 0) && !allowed(cap, 1));
    for (ptrdiff_t increment = 1; increment <= 4; ++increment) CHECK(allowed(cap - 4U, increment));
    CHECK(!allowed(cap - 4U, 5) && allowed(0x2000U, 0xfff) && !allowed(0x2000U, 0x1001));
    CHECK(!allowed(base - 4U, 0) && !allowed(limit + 4U, 0) && !allowed(base + 1U, 0));
    CHECK(allowed(cap + 4U, 0) && allowed(cap + 4U, -4) && !allowed(cap + 4U, 1));
    CHECK(!blinker::unoR4HeapIncrementAllowed(0U, limit, top, cap, base, 0));
    CHECK(!blinker::unoR4HeapIncrementAllowed(base + 1U, limit, top, cap, base, 0));
    CHECK(!blinker::unoR4HeapIncrementAllowed(base, limit + 1U, top, cap, base, 0));
    CHECK(!blinker::unoR4HeapIncrementAllowed(base, limit, top + 1U, cap, base, 0));
    CHECK(!blinker::unoR4HeapIncrementAllowed(base, limit, top, cap + 1U, base, 0));
    CHECK(!blinker::unoR4HeapIncrementAllowed(base, limit, top, base - 4U, base, 0));
    CHECK(!blinker::unoR4HeapIncrementAllowed(base, limit, top, limit + 4U, base, 0));
    CHECK(!blinker::unoR4HeapIncrementAllowed(base, top, top, cap, base, 0));
    CHECK(!blinker::unoR4HeapIncrementAllowed(base, limit, top, top, base, 0));
    CHECK(blinker::unoR4HeapIncrementAllowed(base, limit, top, base, base, 0));
}
void testNegativeAndOverflow() {
    for (ptrdiff_t increment = -1; increment >= -3; --increment) CHECK(allowed(base, increment));
    CHECK(allowed(base + 4U, -4) && allowed(base + 4U, -5) && allowed(base + 4U, -7));
    CHECK(!allowed(base + 4U, -8) && !allowed(base, -4));
    CHECK(!allowed(base, std::numeric_limits<ptrdiff_t>::min()));
    CHECK(!allowed(base, std::numeric_limits<ptrdiff_t>::max()));
    const uintptr_t high = UINTPTR_MAX & ~static_cast<uintptr_t>(3U);
    CHECK(!blinker::unoR4HeapIncrementAllowed(base, high - 4U, high, high - 4U, high - 8U, 8));
    CHECK(blinker::unoR4HeapIncrementAllowed(base, high - 4U, high, high - 4U, high - 8U, 4));
}
void testRestrictedProductPartition() {
    // Existing Core 1.5.3 ELF layout plus a restricted six-KiB engineering
    // candidate, NOT a production default or a whole-program stack proof.
    const uintptr_t r4Base = 0x200043f8U, r4Limit = 0x20007b00U, r4Top = 0x20007f00U;
    const uintptr_t r4Cap = r4Top - 6144U;
    static_assert(r4Cap == 0x20006700U && r4Cap - r4Base == 8968U, "Restricted R4 partition arithmetic");
    CHECK(blinker::unoR4HeapIncrementAllowed(r4Base, r4Limit, r4Top, r4Cap, r4Cap - 4U, 4));
    CHECK(!blinker::unoR4HeapIncrementAllowed(r4Base, r4Limit, r4Top, r4Cap, r4Cap - 4U, 5));
    CHECK(!blinker::unoR4HeapIncrementAllowed(r4Base, r4Limit, r4Top, r4Cap, r4Limit, 1));
}
void testDelegationAndFailure() {
    reset();
    CHECK(checked(0) == reinterpret_cast<void*>(0x2000U) && calls == 1U && changes == 0U);
    CHECK(checked(3) == reinterpret_cast<void*>(0x2000U) && calls == 3U && changes == 1U);
    CHECK(fakeBreak == 0x2004U && lastIncrement == 3);
    CHECK(checked(-5) == reinterpret_cast<void*>(0x2004U) && fakeBreak == 0x2000U && changes == 2U);
    CHECK(lastIncrement == -5); // Delegate the exact original increment, not the rounded value.
    fakeBreak = cap;
    const unsigned before = changes;
    errno = 0;
    CHECK(checked(1) == reinterpret_cast<void*>(UINTPTR_MAX) && errno == ENOMEM);
    CHECK(fakeBreak == cap && changes == before); // Rejected growth never reaches real change.
    reset(); failQuery = true; errno = 0;
    CHECK(checked(4) == reinterpret_cast<void*>(UINTPTR_MAX) && errno == EIO);
    CHECK(calls == 1U && changes == 0U);
    reset(); failChange = true; errno = 0;
    CHECK(checked(4) == reinterpret_cast<void*>(UINTPTR_MAX) && errno == EIO);
    CHECK(calls == 2U && changes == 1U && fakeBreak == 0x2000U);
    reset();
    CHECK(task_heap_boundary::checkedSbrk(4, base, limit, top, limit + 4U)
          == reinterpret_cast<void*>(UINTPTR_MAX));
    CHECK(errno == ENOMEM && calls == 1U && changes == 0U);
}
} // namespace

extern "C" void* __real__sbrk(ptrdiff_t increment) {
    ++calls;
    if (increment == 0) {
        if (failQuery) { errno = EIO; return reinterpret_cast<void*>(UINTPTR_MAX); }
        return reinterpret_cast<void*>(fakeBreak);
    }
    ++changes; lastIncrement = increment;
    if (failChange) { errno = EIO; return reinterpret_cast<void*>(UINTPTR_MAX); }
    void* const previous = reinterpret_cast<void*>(fakeBreak);
    if (increment > 0) fakeBreak += (static_cast<uintptr_t>(increment) + 3U) & ~static_cast<uintptr_t>(3U);
    else fakeBreak -= (static_cast<uintptr_t>(-(increment + 1)) + 1U) & ~static_cast<uintptr_t>(3U);
    return previous;
}

int main() {
    testBoundsAndAlignment(); testNegativeAndOverflow(); testRestrictedProductPartition(); testDelegationAndFailure();
    std::cout << "R4 fixed heap boundary finite model passed; no SDK stack/OOM admission\n";
}
