#ifndef BLINKER_TEST_UTC_SNAPSHOT_SERIAL_H
#define BLINKER_TEST_UTC_SNAPSHOT_SERIAL_H

#include <BlinkerV2/core/WallClock.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

namespace blinker_test {
// Test-only observation of the original business clock. No clock, transport,
// sampling schedule or task operation is owned here. The caller explicitly
// captures at most twice per boot and drains only from its cooperative loop.
class UtcSnapshotSerial {
public:
    UtcSnapshotSerial()
        : snapshot_(), before_(0U), after_(0U), captures_(0U), line_(0U), valid_(false) {}

    bool capture(bool valid, const blinker::WallClockSnapshot& snapshot,
                 uint32_t before, uint32_t after) {
        if (pending() || captures_ >= 2U) return false;
        snapshot_ = snapshot; before_ = before; after_ = after; valid_ = valid;
        ++captures_; line_ = 1U;
        return true;
    }

    // Includes the terminating newline in written, never the NUL. Formatting
    // is repeatable and does not consume a line, including on a small buffer.
    bool format(char* output, size_t capacity, size_t& written) const {
        written = 0U;
        if (output == nullptr || capacity == 0U) return false;
        output[0] = '\0';
        if (!pending()) return false;
        const int size = line_ == 1U
            ? snprintf(output, capacity, "UTC1,%u,%u,%u,%u,%lu,%lu,%lu\n",
                       static_cast<unsigned>(captures_), valid_ ? 1U : 0U,
                       static_cast<unsigned>(snapshot_.state), static_cast<unsigned>(snapshot_.source),
                       static_cast<unsigned long>(snapshot_.sampleRevision),
                       static_cast<unsigned long>(before_), static_cast<unsigned long>(after_))
            : snprintf(output, capacity, "UTC2,%u,%llu,%lu,%lu\n",
                       static_cast<unsigned>(captures_), static_cast<unsigned long long>(snapshot_.utcMillis),
                       static_cast<unsigned long>(snapshot_.ageMillis),
                       static_cast<unsigned long>(snapshot_.uncertaintyMillis));
        if (size <= 0 || size > 63 || static_cast<size_t>(size) >= capacity) {
            output[0] = '\0'; return false;
        }
        written = static_cast<size_t>(size);
        return true;
    }

    bool pending() const { return line_ != 0U; }
    // Call only after availableForWrite() >= written and a complete successful
    // write. A short/blocked output must not discard or replace the snapshot.
    void consume() { if (line_ == 1U) line_ = 2U; else if (line_ == 2U) line_ = 0U; }

private:
    blinker::WallClockSnapshot snapshot_;
    uint32_t before_, after_;
    uint8_t captures_, line_;
    bool valid_;
};
static_assert(sizeof(UtcSnapshotSerial) <= 64U, "Bounded test-only UTC observation");
} // namespace blinker_test
#endif
