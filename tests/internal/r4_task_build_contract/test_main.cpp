#include "Config.h"
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4TaskBuildContract.h>
#include <stdio.h>
#include <string.h>

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
extern bool fixtureFaulted, fixtureHeapUnavailable;
extern unsigned fixtureRpcCalls, fixtureHeapCalls;
extern ptrdiff_t fixtureIncrement;

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    fixtureFaulted = strcmp(argv[1], "rpc-fault") == 0;
    fixtureHeapUnavailable = strcmp(argv[1], "heap-unavailable") == 0;
    const blinker::Result value = blinker::integration::official_detail::renesasUnoTaskBuildReady();
    const blinker::ErrorCode expected = fixtureFaulted ? blinker::ErrorCode::StateConflict :
        fixtureHeapUnavailable ? blinker::ErrorCode::NotConfigured : blinker::ErrorCode::Ok;
    if (value.code() != expected || fixtureRpcCalls != 1U ||
        fixtureHeapCalls != (fixtureFaulted ? 0U : 1U) ||
        (!fixtureFaulted && fixtureIncrement != 0)) return 2;
    puts(argv[1]);
    return 0;
}
#else
static_assert(BLINKER_TASK_CAPACITY == 0, "Ordinary task-disabled capacity stays zero");
int main() { puts("disabled: no protected SDK/heap entries linked"); return 0; }
#endif
