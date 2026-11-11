#ifndef BLINKER_V2_ARDUINO_PORTS_UNO_R4_TASK_STORAGE_H
#define BLINKER_V2_ARDUINO_PORTS_UNO_R4_TASK_STORAGE_H

#include "UnoR4Storage.h"
#include "../../../runtime/TaskSnapshotStore.h"

namespace blinker {

// Optional non-owning view of the existing bank. It opens/closes no Preferences
// handle and owns no executor. The bank must outlive this single-threaded view;
// end() on that owner makes every operation explicitly NotConfigured.
// Credentials keep their 260 B payload bound. Only this opt-in view owns task
// scratch: two fixed journal buffers (1328 B at four slots), not per-call stack
// arrays or a shared global/heap allocation. Keep the view in the persistent
// product graph, not a loop-local variable on the nominal 1 KiB R4 stack. Use
// one cooperative, non-reentrant caller; journal/CRC/readback stay shared.
template <size_t Capacity>
class RenesasUnoTaskSnapshotBlobStore final : public IAtomicBlobStore {
public:
    enum : size_t {
        maximumSnapshotSize = TaskSnapshotCodec::headerSize +
            Capacity * TaskSnapshotCodec::recordSize + TaskSnapshotCodec::trailerSize,
        journalScratchSize = maximumSnapshotSize + 20U
    };

    explicit RenesasUnoTaskSnapshotBlobStore(RenesasUnoPreferencesBlobBank& bank,
                                            uint32_t budgetMillis = 1000U)
        : bank_(bank), budgetMillis_(budgetMillis), encoded_(), verified_() {
        static_assert(Capacity > 0U && Capacity <= TaskSnapshotCodec::maxTasks,
                      "Invalid task snapshot capacity");
    }

    Result load(MutableByteSpan output, size_t& written) override {
        return bank_.loadBounded(RenesasUnoPreferencesBlobBank::BlobTaskSnapshot,
            maximumSnapshotSize, MutableByteSpan(encoded_, sizeof(encoded_)),
            output, written, budgetMillis_);
    }

    Result replace(ByteView value) override {
        return bank_.replaceBounded(RenesasUnoPreferencesBlobBank::BlobTaskSnapshot,
            maximumSnapshotSize, MutableByteSpan(encoded_, sizeof(encoded_)),
            MutableByteSpan(verified_, sizeof(verified_)), value, budgetMillis_);
    }

    Result clear() override {
        return bank_.clearBounded(RenesasUnoPreferencesBlobBank::BlobTaskSnapshot,
            maximumSnapshotSize, MutableByteSpan(encoded_, sizeof(encoded_)),
            MutableByteSpan(verified_, sizeof(verified_)), budgetMillis_);
    }

    StorageProtection protection() const override {
        return StorageProtection::PlainFlash;
    }

private:
    RenesasUnoPreferencesBlobBank& bank_;
    uint32_t budgetMillis_;
    uint8_t encoded_[journalScratchSize];
    uint8_t verified_[journalScratchSize];
    RenesasUnoTaskSnapshotBlobStore(const RenesasUnoTaskSnapshotBlobStore&) = delete;
    RenesasUnoTaskSnapshotBlobStore& operator=(const RenesasUnoTaskSnapshotBlobStore&) = delete;
};

} // namespace blinker

#endif
