#ifndef BLINKER_TEST_WIO_TASK_STORAGE_PROBE_PORT_H
#define BLINKER_TEST_WIO_TASK_STORAGE_PROBE_PORT_H

#include "ProbeController.h"
#include "../PlatformStorageBackupGate/WioOwnedQspi.h"
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalTaskStorage.h>
#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/core/SecureMemory.h>
#include <string.h>

namespace wio_task_probe {
using namespace blinker;

// Long-lived maintenance workspace only; no product, networking or executor.
class TaskStorageProbePort final : public ProbePort {
public:
    enum : size_t { capacity = 4U, payloadSize = 644U, journalSize = 664U };
    enum : uint32_t { taskBase = WioTerminalQspiOwner::taskAddress,
                      identityBase = WioTerminalQspiOwner::identityAddress,
                      regionBytes = WioTerminalQspiOwner::regionSize };
    TaskStorageProbePort(WioTerminalQspiOwner& owner, WioTerminalQspiOwner::Now now,
                         const char* baselineHex)
        : owner_(owner), bank_(owner), tasks_(bank_, true, 1000U), access_(owner, now),
          baselineValid_(false), identityOpen_(false),
          identitySame_(false), snapshotAvailable_(false), rawToken_(0U), identityToken_(0U),
          baseline_(), capture_(), identityBefore_(), identityCurrent_(), snapshotDigest_(),
          chunk_(), expected_(), loaded_(), instance_(), header_(), records_(), hash_() {
        // The constructor initializes no hardware; digest workspace is fixed.
        baselineValid_ = decodeHex(baselineHex, baseline_);
        static_assert(WioTerminalTaskSnapshotBlobStore<capacity>::maximumSnapshotSize == payloadSize,
                      "Task codec size changed");
        static_assert(WioTerminalTaskSnapshotBlobStore<capacity>::journalScratchSize == journalSize,
                      "Task journal size changed");
    }
    bool healthy() const override { return !access_.failed() && !owner_.quarantined(); }
    bool identitySame() const { return identitySame_; }
    bool snapshotAvailable() const { return snapshotAvailable_; }
    const uint8_t* snapshotDigest() const { return snapshotDigest_; }
    Result captureBaseline() override {
        if (!baselineValid_) return Result::failure(ErrorCode::NotConfigured);
        uint32_t actualCapacity = 0U;
        if (!access_.begin(actualCapacity, true, false, 10000U)) return conflict();
        Result result = actualCapacity == WioTerminalQspiOwner::expectedCapacity
            ? Result::success() : conflict();
        // Two independent complete streaming reads must both match the exact
        // protected raw backup SHA. No raw credential bytes leave this port.
        for (uint8_t pass = 0U; result && pass < 2U; ++pass) {
            hash_.reset();
            for (uint32_t offset = 0U; result && offset < regionBytes * 2U; offset += sizeof(chunk_)) {
                if (!access_.read(taskBase + offset, chunk_, sizeof(chunk_))) result = conflict();
                else result = hash_.update(ByteView(chunk_, sizeof(chunk_)));
            }
            if (result) result = hash_.finish(MutableByteSpan(capture_, sizeof(capture_)));
            if (result && memcmp(capture_, baseline_, sizeof(capture_)) != 0) result = conflict();
        }
        const bool closed = access_.finish(); wipeChunk();
        if (result && !closed) result = conflict();
        if (!result) return result;
        result = bank_.begin();
        if (!result) return result;
        identityOpen_ = true;
        DeviceInstanceIdStore identity(bank_.deviceInstanceBlob());
        result = identity.load(instance_); // existing anchor only, NEVER loadOrCreate
        if (result) result = identityHash(identityBefore_);
        identitySame_ = static_cast<bool>(result);
        return result;
    }
    Result attestRegion() override {
        // REGION_ATTEST is a physical-USB declaration by the operator that the
        // whole task region is exclusively reserved and external FS disabled.
        // Neither its blank check nor tasks_.begin() proves that declaration.
        Result result = owner_.acquire(&rawToken_, WioTerminalQspiOwner::Region::Tasks);
        if (!result) return result;
        {
            WioTerminalQspiOwner::Operation operation(owner_, &rawToken_, 10000U);
            result = operation.result();
            for (uint32_t offset = 0U; result && offset < regionBytes; offset += sizeof(chunk_)) {
                result = owner_.read(&rawToken_, taskBase + offset, chunk_, sizeof(chunk_));
                if (result) for (size_t i = 0U; i < sizeof(chunk_); ++i)
                    if (chunk_[i] != 0xffU) { result = conflict(); break; }
            }
        }
        const Result released = owner_.release(&rawToken_); wipeChunk();
        if (!result) return result;
        if (!released) return released;
        result = tasks_.begin();
        if (result) result = checkIdentity();
        return result;
    }
    Result readFixture(bool present) override {
        snapshotAvailable_ = false;
        size_t written = 0U;
        Result result = tasks_.load(MutableByteSpan(loaded_, sizeof(loaded_)), written);
        if (!result && result.code() != ErrorCode::NotFound) return result;
        if (!present) {
            if (result.code() != ErrorCode::NotFound) return conflict();
            const Result unchanged = checkIdentity();
            return unchanged ? Result::failure(ErrorCode::NotFound) : unchanged;
        }
        if (!result || written != payloadSize) return conflict();
        result = fixture();
        if (result && memcmp(expected_, loaded_, payloadSize) != 0) result = conflict();
        if (result) result = sha256(ByteView(loaded_, written), MutableByteSpan(snapshotDigest_, sizeof(snapshotDigest_)));
        if (result) result = checkIdentity();
        snapshotAvailable_ = static_cast<bool>(result);
        secureZero(MutableByteSpan(loaded_, sizeof(loaded_)));
        return result;
    }
    Result replaceFixture() override {
        Result result = checkIdentity();
        if (result) result = fixture();
        if (result) result = tasks_.replace(ByteView(expected_, sizeof(expected_)));
        if (!result) return result; // unknown means NO read-back or replay
        return readFixture(true);
    }
    Result endIdentity() override {
        bank_.end(); identityOpen_ = false;
        if (!healthy()) return conflict();
        return readFixture(true); // the task lease alone must remain usable
    }
    Result clearFixture() override {
        Result result = checkIdentity();
        if (result) result = tasks_.clear();
        if (!result) return result;
        result = readFixture(false);
        return result.code() == ErrorCode::NotFound ? Result::success() : result;
    }
private:
    static Result conflict() { return Result::failure(ErrorCode::StateConflict); }
    static int nibble(char value) {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        return -1;
    }
    static bool decodeHex(const char* value, uint8_t* output) {
        if (value == nullptr || strlen(value) != kSha256Size * 2U) return false;
        for (size_t i = 0U; i < kSha256Size; ++i) {
            const int high = nibble(value[i * 2U]), low = nibble(value[i * 2U + 1U]);
            if (high < 0 || low < 0) return false;
            output[i] = static_cast<uint8_t>((high << 4) | low);
        }
        return true;
    }
    void wipeChunk() { secureZero(MutableByteSpan(chunk_, sizeof(chunk_))); }
    Result identityHash(uint8_t* output) {
        const void* token = identityOpen_ ? static_cast<const void*>(&bank_) : &identityToken_;
        Result result = Result::success();
        if (!identityOpen_) result = owner_.acquire(token, WioTerminalQspiOwner::Region::Identity);
        if (!result) return result;
        {
            WioTerminalQspiOwner::Operation operation(owner_, token, 10000U);
            result = operation.result(); hash_.reset();
            for (uint32_t offset = 0U; result && offset < regionBytes; offset += sizeof(chunk_)) {
                result = owner_.read(token, identityBase + offset, chunk_, sizeof(chunk_));
                if (result) result = hash_.update(ByteView(chunk_, sizeof(chunk_)));
            }
            if (result) result = hash_.finish(MutableByteSpan(output, kSha256Size));
        }
        if (!identityOpen_) {
            const Result released = owner_.release(token);
            if (result) result = released;
        }
        wipeChunk(); return result;
    }
    Result checkIdentity() {
        identitySame_ = false;
        Result result = identityHash(identityCurrent_);
        if (result && memcmp(identityBefore_, identityCurrent_, kSha256Size) != 0) result = conflict();
        identitySame_ = static_cast<bool>(result); return result;
    }
    Result fixture() {
        header_ = TaskSnapshotHeader();
        memcpy(header_.authority.deviceInstanceId, instance_.bytes, sizeof(instance_.bytes));
        memcpy(header_.authority.taskScope, "WioStorageProbe!", 16U);
        memset(header_.authority.rootDigest, 0xa5U, sizeof(header_.authority.rootDigest));
        header_.tableRevision = 1U; header_.nextTaskId = 5U;
        memcpy(header_.requestId, "WIO1", 4U); header_.requestId[15] = 1U;
        memset(header_.requestDigest, 0x31U, sizeof(header_.requestDigest));
        for (size_t i = 0U; i < capacity; ++i) {
            records_[i] = TaskSnapshotRecord(); records_[i].taskId = static_cast<uint32_t>(i + 1U);
            records_[i].definitionRevision = 1U; records_[i].timing.durationMillis = 1000U;
            records_[i].key[0] = 'p'; records_[i].key[1] = static_cast<uint8_t>('1' + i);
            records_[i].keyLength = 2U; records_[i].value[0] = 0xf4U; records_[i].valueLength = 1U;
            // Default disabled/Idle and zero Start sequence. There is NO executor.
        }
        size_t written = 0U;
        const Result result = TaskSnapshotCodec::encode(header_, records_, capacity,
            MutableByteSpan(expected_, sizeof(expected_)), written);
        return result && written != payloadSize ? conflict() : result;
    }
    WioTerminalQspiOwner& owner_;
    WioTerminalFlashBlobBank bank_;
    WioTerminalTaskSnapshotBlobStore<capacity> tasks_;
    storage_gate_qspi::OwnedQspiAccess access_;
    bool baselineValid_, identityOpen_, identitySame_, snapshotAvailable_;
    uint8_t rawToken_, identityToken_;
    uint8_t baseline_[kSha256Size], capture_[kSha256Size];
    uint8_t identityBefore_[kSha256Size], identityCurrent_[kSha256Size], snapshotDigest_[kSha256Size];
    uint8_t chunk_[256U], expected_[payloadSize], loaded_[payloadSize];
    DeviceInstanceId instance_;
    TaskSnapshotHeader header_;
    TaskSnapshotRecord records_[capacity];
    Sha256 hash_;
};
} // namespace wio_task_probe
#endif
