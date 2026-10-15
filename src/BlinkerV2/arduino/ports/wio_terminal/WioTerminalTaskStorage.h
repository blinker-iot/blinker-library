#ifndef BLINKER_WIO_TERMINAL_TASK_STORAGE_H
#define BLINKER_WIO_TERMINAL_TASK_STORAGE_H

#include "WioTerminalStorage.h"
#include <BlinkerV2/core/Crc32.h>
#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/runtime/TaskSnapshotStore.h>
#include <string.h>

namespace blinker {

// Optional storage only. No executor, authority creation or file-system claim.
// The maintenance profile must attest exclusive ownership of the whole task
// region. Blank flash is NOT that proof. Credentials retain their old BWB2
// bank and bytes; this view has an independent lease on the same QSPI owner.
template <size_t Capacity>
class WioTerminalTaskSnapshotBlobStore final : public IAtomicBlobStore {
public:
    enum : size_t {
        maximumSnapshotSize = TaskSnapshotCodec::headerSize +
            Capacity * TaskSnapshotCodec::recordSize + TaskSnapshotCodec::trailerSize,
        journalScratchSize = maximumSnapshotSize + 20U
    };

    explicit WioTerminalTaskSnapshotBlobStore(WioTerminalFlashBlobBank& bank,
        bool regionReserved = false, uint32_t budgetMillis = 1000U)
        : owner_(bank.qspiOwner()), budgetMillis_(budgetMillis),
          reserved_(regionReserved), opened_(false), failed_(false), encoded_(), verified_() {
        static_assert(Capacity > 0U && Capacity <= TaskSnapshotCodec::maxTasks,
                      "Invalid Wio task snapshot capacity");
        static_assert(journalScratchSize <= WioTerminalQspiOwner::eraseBlockSize,
                      "Wio task record exceeds its sector");
    }
    ~WioTerminalTaskSnapshotBlobStore() { end(); }

    Result begin() {
        if (failed_ || owner_.quarantined()) return Result::failure(ErrorCode::StateConflict);
        if (opened_) return Result::success();
        if (!reserved_) return Result::failure(ErrorCode::NotConfigured);
        if (budgetMillis_ == 0U || budgetMillis_ > INT32_MAX)
            return Result::failure(ErrorCode::InvalidArgument);
        Result result = owner_.acquire(this, WioTerminalQspiOwner::Region::Tasks);
        if (!result) return result;
        {
            WioTerminalQspiOwner::Operation operation(owner_, this, budgetMillis_);
            result = operation.result();
            if (result) result = regionAvailable();
        }
        if (!result) { failed_ = true; (void)owner_.release(this); }
        else opened_ = true;
        wipe();
        return result;
    }
    void end() {
        if (opened_ && owner_.release(this)) opened_ = false;
        // end/begin is never a durability or fault-unlock operation.
        if (!opened_) wipe();
    }
    Result load(MutableByteSpan output, size_t& written) override {
        written = 0U;
        const Result ready = usable();
        if (!ready) return ready;
        WioTerminalQspiOwner::Operation operation(owner_, this, budgetMillis_);
        if (!operation.result()) return operation.result();
        Record selected;
        Result result = newest(selected);
        if (result && (!selected.valid || selected.state == tombstone))
            result = Result::failure(ErrorCode::NotFound);
        if (result) {
            written = selected.size;
            if (output.data == nullptr || output.size < written)
                result = Result::failure(ErrorCode::BufferTooSmall);
            else memcpy(output.data, encoded_ + headerSize, written);
        }
        return finish(result);
    }
    Result replace(ByteView value) override {
        if (value.data == nullptr || value.empty()) return Result::failure(ErrorCode::InvalidArgument);
        if (value.size > maximumSnapshotSize) return Result::failure(ErrorCode::CapacityExceeded);
        return update(value, active);
    }
    Result clear() override { return update(ByteView(), tombstone); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }

private:
    enum : uint8_t { active = 1U, tombstone = 2U };
    enum : size_t { headerSize = 16U, crcSize = 4U, pageSize = 256U };
    struct Record {
        uint32_t generation;
        uint16_t size;
        uint8_t slot, state;
        bool valid;
        Record() : generation(0U), size(0U), slot(1U), state(0U), valid(false) {}
    };
    static uint16_t u16(const uint8_t* p) {
        return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8U) | p[1]);
    }
    static uint32_t u32(const uint8_t* p) {
        return (static_cast<uint32_t>(p[0]) << 24U) | (static_cast<uint32_t>(p[1]) << 16U) |
               (static_cast<uint32_t>(p[2]) << 8U) | static_cast<uint32_t>(p[3]);
    }
    static void put32(uint8_t* p, uint32_t value) {
        p[0] = static_cast<uint8_t>(value >> 24U); p[1] = static_cast<uint8_t>(value >> 16U);
        p[2] = static_cast<uint8_t>(value >> 8U); p[3] = static_cast<uint8_t>(value);
    }
    static bool hasMagic(const uint8_t* p) { return memcmp(p, "BWT2", 4U) == 0; }
    static bool erased(const uint8_t* p, size_t size) {
        for (size_t i = 0U; i < size; ++i) if (p[i] != 0xffU) return false;
        return true;
    }
    static Record inspect(const uint8_t* p, uint8_t slot) {
        Record r;
        r.generation = u32(p + 8U); r.size = u16(p + 12U); r.slot = slot; r.state = p[6];
        r.valid = hasMagic(p) && p[4] == 1U && p[5] == 5U && p[7] == 0U &&
            p[14] == 0U && p[15] == 0U && r.generation != 0U && r.size <= maximumSnapshotSize &&
            ((r.state == active && r.size != 0U) || (r.state == tombstone && r.size == 0U));
        if (r.valid) r.valid = u32(p + headerSize + r.size) ==
            computeCrc32(ByteView(p, headerSize + r.size));
        return r;
    }
    uint32_t address(uint8_t slot) const {
        return owner_.regionAddress(WioTerminalQspiOwner::Region::Tasks) +
               static_cast<uint32_t>(slot) * WioTerminalQspiOwner::eraseBlockSize;
    }
    Result usable() const {
        if (failed_ || owner_.quarantined()) return Result::failure(ErrorCode::StateConflict);
        return opened_ ? Result::success() : Result::failure(ErrorCode::NotConfigured);
    }
    void wipe() { secureZero(MutableByteSpan(encoded_, sizeof(encoded_))); secureZero(MutableByteSpan(verified_, sizeof(verified_))); }
    Result finish(Result result) {
        if (!result && result.code() != ErrorCode::NotFound && result.code() != ErrorCode::BufferTooSmall)
            failed_ = true;
        wipe();
        return result;
    }
    Result regionAvailable() {
        for (uint8_t slot = 0U; slot < 2U; ++slot) {
            Result result = owner_.read(this, address(slot), verified_, sizeof(verified_));
            if (!result) return result;
            // Recognizable interrupted records are owned, but not valid data.
            if (inspect(verified_, slot).valid || hasMagic(verified_)) continue;
            if (!erased(verified_, sizeof(verified_))) return Result::failure(ErrorCode::StateConflict);
            for (uint32_t offset = static_cast<uint32_t>(sizeof(verified_));
                 offset < WioTerminalQspiOwner::eraseBlockSize;) {
                const size_t remaining = WioTerminalQspiOwner::eraseBlockSize - offset;
                const size_t count = remaining < sizeof(verified_) ? remaining : sizeof(verified_);
                result = owner_.read(this, address(slot) + offset, verified_, count);
                if (!result) return result;
                if (!erased(verified_, count)) return Result::failure(ErrorCode::StateConflict);
                offset += static_cast<uint32_t>(count);
            }
        }
        return Result::success();
    }
    // Codec/root verification remains in the existing TaskSnapshotStore/Engine.
    // This layer selects an intact journal or fails; corrupt data is not Empty.
    Result newest(Record& selected) {
        bool present = false;
        for (uint8_t slot = 0U; slot < 2U; ++slot) {
            const Result result = owner_.read(this, address(slot), verified_, sizeof(verified_));
            if (!result) return result;
            present = present || !erased(verified_, sizeof(verified_));
            // Never roll back over an unsupported/oversized owned record and
            // then erase it as if it were merely a torn current-format write.
            if (hasMagic(verified_)) {
                if (verified_[4] != 1U || verified_[5] != 5U)
                    return Result::failure(ErrorCode::UnsupportedVersion);
                if (u16(verified_ + 12U) > maximumSnapshotSize)
                    return Result::failure(ErrorCode::CapacityExceeded);
            }
            const Record candidate = inspect(verified_, slot);
            if (!candidate.valid) continue;
            if (selected.valid && candidate.generation == selected.generation)
                return Result::failure(ErrorCode::SequenceConflict);
            if (!selected.valid || candidate.generation > selected.generation) {
                selected = candidate; memcpy(encoded_, verified_, sizeof(encoded_));
            }
        }
        return selected.valid || !present ? Result::success() : Result::failure(ErrorCode::InvalidEncoding);
    }
    Result update(ByteView value, uint8_t state) {
        const Result ready = usable();
        if (!ready) return ready;
        WioTerminalQspiOwner::Operation operation(owner_, this, budgetMillis_);
        if (!operation.result()) return operation.result();
        Record selected;
        Result result = newest(selected);
        if (!result) return finish(result);
        if (selected.generation == UINT32_MAX) return finish(Result::failure(ErrorCode::SequenceConflict));
        if (state == tombstone && selected.valid && selected.state == tombstone) return finish(Result::success());
        memset(encoded_, 0U, sizeof(encoded_));
        memcpy(encoded_, "BWT2", 4U); encoded_[4] = 1U; encoded_[5] = 5U; encoded_[6] = state;
        put32(encoded_ + 8U, selected.generation + 1U);
        encoded_[12] = static_cast<uint8_t>(value.size >> 8U); encoded_[13] = static_cast<uint8_t>(value.size);
        if (!value.empty()) memcpy(encoded_ + headerSize, value.data, value.size);
        const size_t size = headerSize + value.size + crcSize;
        put32(encoded_ + headerSize + value.size, computeCrc32(ByteView(encoded_, headerSize + value.size)));
        uint32_t target = address(static_cast<uint8_t>(selected.slot ^ 1U));
        result = owner_.erase(this, target);
        for (size_t offset = 0U; result && offset < size;) {
            const size_t pageRemaining = pageSize - ((target + offset) % pageSize);
            const size_t count = size - offset < pageRemaining ? size - offset : pageRemaining;
            result = owner_.program(this, target + static_cast<uint32_t>(offset), encoded_ + offset, count);
            offset += count;
        }
        // An unconfirmed/late erase or page program must not be read back/replayed.
        if (result) result = owner_.read(this, target, verified_, size);
        if (result && memcmp(encoded_, verified_, size) != 0) result = owner_.verificationFailed(this);
        return finish(result);
    }

    WioTerminalQspiOwner& owner_;
    uint32_t budgetMillis_;
    bool reserved_, opened_, failed_;
    uint8_t encoded_[journalScratchSize], verified_[journalScratchSize];
    WioTerminalTaskSnapshotBlobStore(const WioTerminalTaskSnapshotBlobStore&) = delete;
    WioTerminalTaskSnapshotBlobStore& operator=(const WioTerminalTaskSnapshotBlobStore&) = delete;
};

} // namespace blinker
#endif
