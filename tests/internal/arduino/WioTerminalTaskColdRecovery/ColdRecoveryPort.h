#ifndef BLINKER_TEST_WIO_TASK_COLD_PORT_H
#define BLINKER_TEST_WIO_TASK_COLD_PORT_H

#include "ColdRecoveryController.h"
#include "../PlatformStorageBackupGate/WioOwnedQspi.h"
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalTaskStorage.h>
#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/core/SecureMemory.h>
#include <string.h>

namespace wio_task_cold {
using namespace blinker;

struct ColdRecoveryBinding {
    const char* sourceId;
    const char* baselineSha256; // ORIGINAL 16 KiB backup; NEVER replaced by current
    const char* identitySha256;
    const char* currentSha256;  // independently reconciled post-cold 16 KiB raw
    const char* intentSha256;   // original pre-send intent, including unknowns
    const char* boardUid;
};

class ColdRecoveryPort final : public ColdRecoveryReader {
public:
    enum : size_t { capacity = 4U, payloadSize = 644U };
    enum : uint32_t { taskBase = WioTerminalQspiOwner::taskAddress,
                      identityBase = WioTerminalQspiOwner::identityAddress,
                      regionBytes = WioTerminalQspiOwner::regionSize };
    ColdRecoveryPort(WioTerminalQspiOwner& owner, WioTerminalQspiOwner::Now now,
                     const ColdRecoveryBinding& binding, const char* actualUidRam)
        : owner_(owner), binding_(binding), actualUid_(actualUidRam), bank_(owner),
          tasks_(bank_, true, 1000U), readOnly_(tasks_), access_(owner, now), workspace_(),
          snapshot_(readOnly_, MutableByteSpan(workspace_, sizeof(workspace_))),
          attempted_(false), observed_(false), hasSnapshot_(false), identitySame_(false),
          expectedCurrent_(), expectedIdentity_(), currentDigest_(), identityDigest_(),
          payloadDigest_(), chunk_(), instance_(), header_(), records_(), fullHash_(), identityHash_() {
        static_assert(static_cast<size_t>(WioTerminalTaskSnapshotBlobStore<capacity>::maximumSnapshotSize) ==
                          static_cast<size_t>(payloadSize),
                      "Cold fixture task codec size changed");
    }
    bool observed() const override { return observed_; }
    bool hasSnapshot() const override { return hasSnapshot_; }
    bool identitySame() const { return identitySame_; }
    const uint8_t* payloadDigest() const { return payloadDigest_; }
    Result observe() override {
        if (attempted_) return conflict();
        attempted_ = true;
        if (!validBinding()) return finish(Result::failure(ErrorCode::InvalidArgument));
        Result result = captureCurrent(2U);
        if (!result) return finish(result);
        result = bank_.begin();
        if (!result) return finish(result);
        DeviceInstanceIdStore identity(bank_.deviceInstanceBlob());
        result = identity.load(instance_); // NEVER loadOrCreate or initialize
        if (!result) return finish(result.code() == ErrorCode::NotFound ? conflict() : result);
        result = tasks_.begin();
        if (!result) return finish(result);
        size_t count = 0U;
        // Existing snapshot store invokes the ORIGINAL codec; this fixture
        // neither parses journal bytes nor supplies another recovery decoder.
        result = snapshot_.load(header_, records_, capacity, count);
        if (!result && result.code() != ErrorCode::NotFound) return finish(result);
        const bool missing = result.code() == ErrorCode::NotFound;
        if (!missing) {
            if (count != capacity || !expectedFixture())
                return finish(Result::failure(ErrorCode::InvalidEncoding));
            result = sha256(ByteView(workspace_, payloadSize), MutableByteSpan(payloadDigest_, sizeof(payloadDigest_)));
            if (!result) return finish(result);
        }
        // Only a healthy, successfully decoded observation may end its own
        // views. Recheck the ENTIRE current region after the real store load,
        // so a late task-region change cannot be published under an old SHA.
        // Unknown/native failure paths above never reopen or do cleanup I/O.
        if (owner_.quarantined()) return finish(conflict());
        tasks_.end(); bank_.end();
        result = captureCurrent(1U);
        if (!result) return finish(result);
        identitySame_ = true;
        observed_ = true; hasSnapshot_ = !missing;
        return finish(missing ? Result::failure(ErrorCode::NotFound) : Result::success());
    }
private:
    // Additional capability boundary: even accidental snapshot-store mutation
    // cannot reach the physical port. No replace/clear entry is exposed above.
    class ReadOnlyBlob final : public IAtomicBlobStore {
    public:
        explicit ReadOnlyBlob(IAtomicBlobStore& storage) : storage_(storage) {}
        Result load(MutableByteSpan output, size_t& written) override { return storage_.load(output, written); }
        Result replace(ByteView) override { return Result::failure(ErrorCode::UnsupportedFeature); }
        Result clear() override { return Result::failure(ErrorCode::UnsupportedFeature); }
        StorageProtection protection() const override { return storage_.protection(); }
    private:
        IAtomicBlobStore& storage_;
    };
    static Result conflict() { return Result::failure(ErrorCode::StateConflict); }
    static int nibble(char value) {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        return -1;
    }
    static bool validHex(const char* value, size_t count) {
        if (value == nullptr || strlen(value) != count) return false;
        for (size_t i = 0U; i < count; ++i) if (nibble(value[i]) < 0) return false;
        return true;
    }
    static void decodeDigest(const char* value, uint8_t* output) {
        for (size_t i = 0U; i < kSha256Size; ++i)
            output[i] = static_cast<uint8_t>((nibble(value[i * 2U]) << 4) | nibble(value[i * 2U + 1U]));
    }
    bool validBinding() {
        if (!validHex(binding_.sourceId, 64U) || !validHex(binding_.baselineSha256, 64U) ||
            !validHex(binding_.identitySha256, 64U) || !validHex(binding_.currentSha256, 64U) ||
            !validHex(binding_.intentSha256, 64U) || !validHex(binding_.boardUid, 32U) ||
            !validHex(actualUid_, 32U)) return false;
        for (size_t i = 0U; i < 32U; ++i)
            if (nibble(actualUid_[i]) != nibble(binding_.boardUid[i])) return false;
        decodeDigest(binding_.currentSha256, expectedCurrent_);
        decodeDigest(binding_.identitySha256, expectedIdentity_);
        // Original backup and intent are immutable profile bindings verified
        // by the host against its protected files. This device cannot prove a
        // human power cycle or authenticate those files from public hashes.
        return true;
    }
    Result captureCurrent(uint8_t passes) {
        uint32_t actualCapacity = 0U;
        if (!access_.begin(actualCapacity, true, false, 10000U)) return conflict();
        Result result = actualCapacity == WioTerminalQspiOwner::expectedCapacity ? Result::success() : conflict();
        for (uint8_t pass = 0U; result && pass < passes; ++pass) {
            fullHash_.reset(); identityHash_.reset();
            for (uint32_t offset = 0U; result && offset < regionBytes * 2U; offset += sizeof(chunk_)) {
                if (!access_.read(taskBase + offset, chunk_, sizeof(chunk_))) result = conflict();
                else {
                    result = fullHash_.update(ByteView(chunk_, sizeof(chunk_)));
                    if (result && offset >= regionBytes) result = identityHash_.update(ByteView(chunk_, sizeof(chunk_)));
                }
            }
            if (result) result = fullHash_.finish(MutableByteSpan(currentDigest_, sizeof(currentDigest_)));
            if (result) result = identityHash_.finish(MutableByteSpan(identityDigest_, sizeof(identityDigest_)));
            if (result && (memcmp(currentDigest_, expectedCurrent_, kSha256Size) != 0 ||
                           memcmp(identityDigest_, expectedIdentity_, kSha256Size) != 0)) result = conflict();
        }
        const bool closed = access_.finish();
        return result && !closed ? conflict() : result;
    }
    static bool filled(const uint8_t* value, size_t size, uint8_t byte) {
        for (size_t i = 0U; i < size; ++i) if (value[i] != byte) return false;
        return true;
    }
    bool expectedFixture() const {
        if (header_.retired || header_.tableRevision != 1U || header_.nextTaskId != 5U ||
            memcmp(header_.authority.deviceInstanceId, instance_.bytes, sizeof(instance_.bytes)) != 0 ||
            memcmp(header_.authority.taskScope, "WioStorageProbe!", 16U) != 0 ||
            !filled(header_.authority.rootDigest, sizeof(header_.authority.rootDigest), 0xa5U) ||
            memcmp(header_.requestId, "WIO1", 4U) != 0 || header_.requestId[15] != 1U ||
            !filled(header_.requestId + 4U, 11U, 0U) ||
            !filled(header_.requestDigest, sizeof(header_.requestDigest), 0x31U)) return false;
        for (size_t i = 0U; i < capacity; ++i) {
            const TaskSnapshotRecord& record = records_[i];
            if (record.taskId != i + 1U || record.definitionRevision != 1U || record.enabled ||
                record.progress != TaskProgress::None || record.startSequence != 0U || record.consumedUtcMillis != 0U ||
                record.timing.kind != TaskTimingKind::Countdown || record.timing.durationMillis != 1000U ||
                record.timing.utcMillis != 0U || record.timing.minuteOfDay != 0U ||
                record.timing.utcOffsetMinutes != 0 || record.timing.weekdays != 0U ||
                record.keyLength != 2U || record.key[0] != 'p' || record.key[1] != static_cast<uint8_t>('1' + i) ||
                record.valueType != ValueType::Boolean || record.valueLength != 1U || record.value[0] != 0xf4U) return false;
        }
        return true;
    }
    Result finish(Result result) {
        secureZero(MutableByteSpan(chunk_, sizeof(chunk_)));
        secureZero(MutableByteSpan(workspace_, sizeof(workspace_)));
        fullHash_.reset(); identityHash_.reset();
        return result;
    }

    WioTerminalQspiOwner& owner_;
    ColdRecoveryBinding binding_;
    const char* actualUid_;
    WioTerminalFlashBlobBank bank_;
    WioTerminalTaskSnapshotBlobStore<capacity> tasks_;
    ReadOnlyBlob readOnly_;
    storage_gate_qspi::OwnedQspiAccess access_;
    uint8_t workspace_[payloadSize];
    TaskSnapshotStore snapshot_;
    bool attempted_, observed_, hasSnapshot_, identitySame_;
    uint8_t expectedCurrent_[kSha256Size], expectedIdentity_[kSha256Size];
    uint8_t currentDigest_[kSha256Size], identityDigest_[kSha256Size], payloadDigest_[kSha256Size];
    uint8_t chunk_[256U];
    DeviceInstanceId instance_;
    TaskSnapshotHeader header_;
    TaskSnapshotRecord records_[capacity];
    Sha256 fullHash_, identityHash_;
};
} // namespace wio_task_cold
#endif
