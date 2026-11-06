#ifndef BLINKER_RUNTIME_TASKROOTOWNERGATE_H
#define BLINKER_RUNTIME_TASKROOTOWNERGATE_H

#include <atomic>
#include "../interface/IDeviceAccessChangeObserver.h"

namespace blinker {

// The Store serializes its own callers. This gate additionally excludes an
// admitted native ROOT COMMIT from the cooperative task/management cycle.
// No callback performs root reads, cache operations, I/O or waiting.
class TaskRootOwnerGate final : public IDeviceAccessChangeObserver {
public:
    // C++11 guarantees atomic_flag itself is lock-free. Do not assume the
    // platform's general 32-bit atomic is always lock-free (Xtensa reports 1).
    TaskRootOwnerGate() : uncertain_(false), ownerHeld_(false), cooperativeBorrowed_(false) {}
    Result beginDeviceAccessChange() override {
        if (busy_.test_and_set(std::memory_order_acquire))
            return Result::failure(ErrorCode::WouldBlock);
        if (uncertain_) {
            busy_.clear(std::memory_order_release);
            return Result::failure(ErrorCode::StateConflict);
        }
        clean_.clear(std::memory_order_release);
        return Result::success();
    }
    void endDeviceAccessChange(Result outcome) override {
        // A failed commit may expose an uncommitted NVS handle view. Do not
        // reopen authority by reading that same handle on the next tick.
        if (!outcome) uncertain_ = true; // Protected by the held busy_ flag.
        clean_.clear(std::memory_order_release);
        busy_.clear(std::memory_order_release);
    }
    // Only the explicit cooperative Presence port may borrow the already-held
    // owner. Ordinary native writers still fail on busy_; this never unlocks it.
    Result beginCooperativePresenceChange() override {
        return beginBorrowedChange();
    }
    void endCooperativePresenceChange(Result outcome) override {
        endBorrowedChange(outcome);
    }
    Result beginCooperativeInitialCloudKeyChange() override {
        return beginBorrowedChange();
    }
    void endCooperativeInitialCloudKeyChange(Result outcome) override {
        endBorrowedChange(outcome);
    }
    bool enterOwner() {
        if (busy_.test_and_set(std::memory_order_acquire)) return false;
        ownerHeld_ = true;
        return true;
    }
    void leaveOwner() {
        if (cooperativeBorrowed_) return; // An unbalanced borrowed scope fails closed.
        ownerHeld_ = false;
        busy_.clear(std::memory_order_release);
    }
    void markDirty() { clean_.clear(std::memory_order_release); }
    // Only the exclusive owner consumes dirty or reads uncertain. Quiescent
    // test/destruction callers may inspect too; these are not authorization.
    bool takeDirty() { return !clean_.test_and_set(std::memory_order_acquire); }
    bool uncertain() const { return uncertain_; }
    bool idle() const {
        if (busy_.test_and_set(std::memory_order_acquire)) return false;
        busy_.clear(std::memory_order_release);
        return true;
    }

private:
    // Shared exclusion for the two named ports only, not a public reentrant API.
    Result beginBorrowedChange() {
        if (!ownerHeld_) return Result::failure(ErrorCode::NotConfigured);
        if (uncertain_) return Result::failure(ErrorCode::StateConflict);
        if (cooperativeBorrowed_) return Result::failure(ErrorCode::WouldBlock);
        cooperativeBorrowed_ = true;
        markDirty();
        return Result::success();
    }
    void endBorrowedChange(Result outcome) {
        if (!cooperativeBorrowed_) return;
        if (!outcome) uncertain_ = true;
        cooperativeBorrowed_ = false;
        markDirty();
    }
    mutable std::atomic_flag busy_ = ATOMIC_FLAG_INIT;
    std::atomic_flag clean_ = ATOMIC_FLAG_INIT; // Clear means pending refresh.
    bool uncertain_; // Never reset by stop/start or a successful handle read.
    bool ownerHeld_, cooperativeBorrowed_; // Cooperative-owner-only, never read by native writers.
    TaskRootOwnerGate(const TaskRootOwnerGate&) = delete;
    TaskRootOwnerGate& operator=(const TaskRootOwnerGate&) = delete;
};

} // namespace blinker
#endif
