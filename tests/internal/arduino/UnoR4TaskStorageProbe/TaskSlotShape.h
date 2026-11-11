#ifndef BLINKER_TEST_R4_TASK_SLOT_SHAPE_H
#define BLINKER_TEST_R4_TASK_SLOT_SHAPE_H

#include <stdint.h>

namespace task_probe {

// The journal chooses its slot. Presence/size admission must not assume ts0
// is first: phase 1 is one full record, phase 2 is two, phase 3 is a tombstone
// plus the previous full record. Snapshot/fixture validation stays separate.
inline bool matchesTaskSlotShape(uint8_t phase, const bool (&present)[2],
                                 const uint32_t (&sizes)[2], uint32_t recordSize) {
    if (recordSize <= 20U) return false;
    const bool absent0 = !present[0] && sizes[0] == 0U;
    const bool absent1 = !present[1] && sizes[1] == 0U;
    const bool full0 = present[0] && sizes[0] == recordSize;
    const bool full1 = present[1] && sizes[1] == recordSize;
    const bool tombstone0 = present[0] && sizes[0] == 20U;
    const bool tombstone1 = present[1] && sizes[1] == 20U;
    switch (phase) {
    case 0U: return absent0 && absent1;
    case 1U: return (full0 && absent1) || (absent0 && full1);
    case 2U: return full0 && full1;
    case 3U: return (tombstone0 && full1) || (full0 && tombstone1);
    default: return false;
    }
}

} // namespace task_probe
#endif
