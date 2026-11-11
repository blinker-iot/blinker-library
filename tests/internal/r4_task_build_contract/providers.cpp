#include "Config.h"
#include <stddef.h>
#include <stdint.h>

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
bool fixtureFaulted = false, fixtureHeapUnavailable = false;
unsigned fixtureRpcCalls = 0U, fixtureHeapCalls = 0U;
ptrdiff_t fixtureIncrement = 1;

#ifndef FIXTURE_OMIT_RPC
extern "C" bool blinker_r4_rpc_faulted() { ++fixtureRpcCalls; return fixtureFaulted; }
#endif
#ifndef FIXTURE_OMIT_HEAP
extern "C" void* __wrap__sbrk(ptrdiff_t increment) {
    ++fixtureHeapCalls;
    fixtureIncrement = increment;
    return reinterpret_cast<void*>(fixtureHeapUnavailable ? UINTPTR_MAX : uintptr_t(4096U));
}
#endif
#endif
