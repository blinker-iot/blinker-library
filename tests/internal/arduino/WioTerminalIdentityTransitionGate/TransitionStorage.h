#ifndef BLINKER_TEST_WIO_IDENTITY_TRANSITION_STORAGE_H
#define BLINKER_TEST_WIO_IDENTITY_TRANSITION_STORAGE_H

#include "TransitionController.h"
#include "../PlatformStorageBackupGate/WioOwnedQspi.h"
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalStorage.h>
#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/core/SecureMemory.h>
#include <string.h>

namespace wio_identity_transition {
using namespace blinker;
struct TransitionBinding {
    const char* sourceId;
    const char* originalSha256;
    const char* boardUid;
};

// Standalone test-only capability. Ordinary maintenance modes 0/2 still reject
// cross-instance restoration. This object can restore ONLY the compiled exact
// original 16 KiB file on the compiled physical UID, never an arbitrary image.
// Its serial adapter/upload gate MUST be ready before temporary preparation.
class TransitionStorage final : public TransitionPort {
public:
    enum : size_t {
        rawSize = WioTerminalQspiOwner::reservedSize,
        regionSize = WioTerminalQspiOwner::regionSize,
        sectorSize = WioTerminalQspiOwner::eraseBlockSize
    };
    TransitionStorage(WioTerminalQspiOwner& owner, WioTerminalQspiOwner::Now now,
                      IRandom& hardwareRandom, const TransitionBinding& binding, const char* actualUidRam)
        : owner_(owner), access_(owner, now), bank_(owner), random_(hardwareRandom), binding_(binding),
          actualUid_(actualUidRam), originalInstance_(), temporaryInstance_(), current_(), digest_(),
          expected_(), failed_(false), busy_(false), prepared_(false), restoreUsed_(false), captured_(false) {}
    bool healthy() const override { return !failed_ && !access_.failed() && !owner_.quarantined(); }
    ByteView capturedRaw() const { return captured_ ? ByteView(current_, sizeof(current_)) : ByteView(); }
    const DeviceInstanceId& temporaryInstance() const { return temporaryInstance_; }

    Result prepareTemporary() override {
        if (!healthy() || busy_ || prepared_ || restoreUsed_) return conflict();
        Busy active(busy_); prepared_ = true;
        Result result = validateBinding();
        if (result) result = capture();
        if (!result) return finish(result);
        result = sha256(ByteView(current_, sizeof(current_)), MutableByteSpan(digest_, sizeof(digest_)));
        if (!result || memcmp(digest_, expected_, sizeof(digest_)) != 0) return finish(conflict());
        for (size_t i = 0U; i < regionSize; ++i)
            if (current_[i] != 0xffU) return finish(conflict()); // never retire existing tasks by erasing them
        result = bank_.begin();
        DeviceInstanceIdStore identity(bank_.deviceInstanceBlob());
        if (result) result = identity.load(originalInstance_); // must already exist before first mutation
        if (!result) return finish(result);
        captured_ = false;
        IAtomicBlobStore* optional[4U] = {&bank_.wifiSelectorBlob(), &bank_.wifiSlot0Blob(),
                                        &bank_.wifiSlot1Blob(), &bank_.deviceAccessBlob()};
        for (size_t i = 0U; i < 4U; ++i) {
            result = optional[i]->clear();
            if (!result && result.code() != ErrorCode::NotFound) return finish(result);
        }
        result = bank_.deviceInstanceBlob().clear();
        if (!result) return finish(result);
        // A failed random source leaves no anchor. That state is deliberately
        // recoverable by the exact-raw path below, without relaxing modes 0/2.
        DistinctRandom random(random_, originalInstance_);
        result = identity.loadOrCreate(random, temporaryInstance_);
        if (!result) return finish(result);
        bank_.end(); // healthy only; uncertain paths return above without cleanup
        if (!healthy()) return finish(conflict());
        result = capture();
        if (result) for (size_t i = 0U; i < regionSize; ++i)
            if (current_[i] != 0xffU) { result = conflict(); break; }
        return finish(result);
    }

    Result captureCurrent() override {
        if (!healthy() || busy_) return conflict();
        Busy active(busy_);
        Result result = validateBinding();
        if (result) result = capture(); // no BDN2 requirement: protects no-anchor/torn transitions
        return finish(result);
    }

    Result restoreOriginal(ByteView original) override {
        if (!healthy() || busy_ || restoreUsed_) return conflict();
        Busy active(busy_); restoreUsed_ = true;
        Result result = validateBinding();
        if (!result) return finish(result);
        if (original.data == nullptr || original.size != rawSize || overlapsCurrent(original))
            return finish(Result::failure(ErrorCode::InvalidArgument));
        result = sha256(original, MutableByteSpan(digest_, sizeof(digest_)));
        if (!result || memcmp(digest_, expected_, sizeof(digest_)) != 0) return finish(conflict());
        result = capture();
        if (!result) return finish(result);
        captured_ = false;
        // Two explicit sessions on the SAME owner: helper true writes tasks
        // only; helper false writes identity only. No new region/native seam.
        result = restoreRegion(original, true);
        if (result) result = restoreRegion(original, false);
        if (!result) return finish(result);
        result = capture();
        if (result && memcmp(current_, original.data, rawSize) != 0) result = conflict();
        return finish(result);
    }
private:
    class Busy {
    public:
        explicit Busy(bool& value) : value_(value) { value_ = true; }
        ~Busy() { value_ = false; }
    private:
        bool& value_;
    };
    class DistinctRandom final : public IRandom {
    public:
        DistinctRandom(IRandom& source, const DeviceInstanceId& original) : source_(source), original_(original) {}
        Result fill(MutableByteSpan output) override {
            Result result = source_.fill(output);
            if (result && output.size == sizeof(original_.bytes) &&
                memcmp(output.data, original_.bytes, sizeof(original_.bytes)) == 0)
                result = Result::failure(ErrorCode::StateConflict);
            return result;
        }
    private:
        IRandom& source_;
        const DeviceInstanceId& original_;
    };
    static Result conflict() { return Result::failure(ErrorCode::StateConflict); }
    static int nibble(char value) {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        return -1;
    }
    static bool hex(const char* value, size_t size) {
        if (value == nullptr || strlen(value) != size) return false;
        for (size_t i = 0U; i < size; ++i) if (nibble(value[i]) < 0) return false;
        return true;
    }
    Result validateBinding() {
        if (!hex(binding_.sourceId, 64U) || !hex(binding_.originalSha256, 64U) ||
            !hex(binding_.boardUid, 32U) || !hex(actualUid_, 32U)) return conflict();
        for (size_t i = 0U; i < 32U; ++i)
            if (nibble(binding_.boardUid[i]) != nibble(actualUid_[i])) return conflict();
        for (size_t i = 0U; i < sizeof(expected_); ++i)
            expected_[i] = static_cast<uint8_t>((nibble(binding_.originalSha256[i * 2U]) << 4) |
                                              nibble(binding_.originalSha256[i * 2U + 1U]));
        return Result::success();
    }
    bool overlapsCurrent(ByteView value) const {
        const uintptr_t a = reinterpret_cast<uintptr_t>(value.data);
        const uintptr_t b = reinterpret_cast<uintptr_t>(current_);
        return a >= b ? a - b < rawSize : b - a < value.size;
    }
    Result capture() {
        captured_ = false; uint32_t actualCapacity = 0U;
        if (!access_.begin(actualCapacity, true, false, 10000U) || actualCapacity != WioTerminalQspiOwner::expectedCapacity)
            return conflict();
        if (!access_.read(WioTerminalQspiOwner::taskAddress, current_, sizeof(current_)) ||
            !access_.matches(WioTerminalQspiOwner::taskAddress, current_, sizeof(current_))) return conflict();
        // end itself checks the whole-session deadline. A last native read
        // can be timely while the subsequent release is already too late.
        if (!access_.finish() || !healthy()) return conflict();
        captured_ = true; return Result::success();
    }
    Result restoreRegion(ByteView original, bool tasks) {
        uint32_t actualCapacity = 0U;
        if (!access_.begin(actualCapacity, tasks, tasks, 10000U) || actualCapacity != WioTerminalQspiOwner::expectedCapacity)
            return conflict();
        const size_t offset = tasks ? 0U : static_cast<size_t>(regionSize);
        const uint32_t address = tasks ? WioTerminalQspiOwner::taskAddress : WioTerminalQspiOwner::identityAddress;
        for (size_t slot = 0U; slot < 2U; ++slot) {
            const size_t within = slot * sectorSize;
            if (!access_.restoreSector(address + static_cast<uint32_t>(within),
                current_ + offset + within, original.data + offset + within)) return conflict();
        }
        return access_.finish() && healthy() ? Result::success() : conflict();
    }
    Result finish(Result result) {
        if (!result) { failed_ = true; captured_ = false; }
        secureZero(MutableByteSpan(digest_, sizeof(digest_)));
        return result;
    }

    WioTerminalQspiOwner& owner_;
    storage_gate_qspi::OwnedQspiAccess access_;
    WioTerminalFlashBlobBank bank_;
    IRandom& random_;
    TransitionBinding binding_;
    const char* actualUid_;
    DeviceInstanceId originalInstance_, temporaryInstance_;
    uint8_t current_[rawSize], digest_[kSha256Size], expected_[kSha256Size];
    bool failed_, busy_, prepared_, restoreUsed_, captured_;
};
} // namespace wio_identity_transition
#endif
