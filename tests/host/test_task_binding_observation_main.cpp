#include <cstdlib>
#include <cstring>
#include <iostream>
#include "BlinkerV2/runtime/TaskSnapshotStore.h"
#include "support/TaskAuthorityFixture.h"

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
enum : size_t { bytes = TaskSnapshotCodec::headerSize + 4U * TaskSnapshotCodec::recordSize + TaskSnapshotCodec::trailerSize };

class Memory final : public IAtomicBlobStore {
public:
    Memory() : data(), size(0U), present(false), unknownWrite(false),
        readError(ErrorCode::Ok), reads(0U), writes(0U), clears(0U) {}
    Result load(MutableByteSpan output, size_t& written) override {
        ++reads; written = 0U;
        if (readError != ErrorCode::Ok) return Result::failure(readError);
        if (!present) return Result::failure(ErrorCode::NotFound);
        written = size;
        if (output.data == nullptr || output.size < size)
            return Result::failure(ErrorCode::BufferTooSmall);
        std::memcpy(output.data, data, size);
        return Result::success();
    }
    Result replace(ByteView value) override {
        ++writes;
        CHECK(value.size <= sizeof(data));
        std::memcpy(data, value.data, value.size); size = value.size; present = true;
        return unknownWrite ? Result::failure(ErrorCode::WouldBlock) : Result::success();
    }
    Result clear() override { ++clears; present = false; return Result::success(); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
    uint8_t data[bytes];
    size_t size;
    bool present, unknownWrite;
    ErrorCode readError;
    unsigned reads, writes, clears;
};

TaskSnapshotHeader binding(bool retired = false) {
    TaskSnapshotHeader header;
    header.authority = testAuthority();
    header.retired = retired;
    if (retired) {
        header.tableRevision = 1U;
        header.requestId[0] = 1U;
        header.requestDigest[0] = 2U;
    }
    return header;
}

TaskSnapshotRecord record() {
    TaskSnapshotRecord task;
    task.taskId = 1U; task.definitionRevision = 1U;
    task.timing.kind = TaskTimingKind::UtcOnce; task.timing.utcMillis = 1790035200000ULL;
    task.enabled = false; task.keyLength = 5U;
    std::memcpy(task.key, "power", 5U); task.valueLength = 1U; task.value[0] = 0xF4U;
    return task;
}

struct Fixture {
    Memory memory;
    uint8_t workspace[bytes];
    TaskSnapshotHeader header;
    TaskSnapshotRecord records[4U];
    TaskStoreBindingObservation observed;
    TaskSnapshotStore store;
    Fixture() : memory(), workspace(), header(), records(), observed(),
        store(memory, MutableByteSpan(workspace, sizeof(workspace))) {}
    void seed(const TaskSnapshotHeader& value, const TaskSnapshotRecord* tasks = nullptr, size_t count = 0U) {
        CHECK(TaskSnapshotCodec::encode(value, tasks, count,
            MutableByteSpan(memory.data, sizeof(memory.data)), memory.size));
        memory.present = true;
    }
    Result observe(size_t capacity = 4U) {
        return store.observeBinding(observed, header, records, capacity);
    }
    void readOnly(unsigned expectedReads = 1U) {
        CHECK(memory.reads == expectedReads && memory.writes == 0U && memory.clears == 0U);
    }
};

void missingIsTheOnlyUnboundObservation() {
    Fixture f;
    CHECK(f.observed.state == TaskStoreBindingState::Unknown);
    f.observed.state = TaskStoreBindingState::Bound;
    CHECK(f.observe());
    CHECK(f.observed.state == TaskStoreBindingState::Unbound);
    CHECK(!f.header.authority.valid());
    f.readOnly();
}

void activeEmptySnapshotRemainsBound() {
    Fixture f; f.seed(binding());
    CHECK(f.observe());
    CHECK(f.observed.state == TaskStoreBindingState::Bound);
    CHECK(f.header.authority == testAuthority() && f.header.tableRevision == 0U);
    f.readOnly();
}

void storedRecordRemainsBoundAndIsReadWithoutExecution() {
    Fixture f; TaskSnapshotHeader header = binding();
    header.tableRevision = 1U; header.nextTaskId = 2U;
    header.requestId[0] = 1U; header.requestDigest[0] = 2U;
    const TaskSnapshotRecord task = record(); f.seed(header, &task, 1U);
    CHECK(f.observe());
    CHECK(f.observed.state == TaskStoreBindingState::Bound);
    CHECK(f.records[0].taskId == 1U && !f.records[0].enabled);
    f.readOnly();
}

void retirementMarkerIsNotUnbound() {
    Fixture f; f.seed(binding(true));
    CHECK(f.observe());
    CHECK(f.observed.state == TaskStoreBindingState::Retired && f.header.retired);
    f.readOnly();
}

void corruptionStaysUnknownAndNeverReloadsTheFailedHandle() {
    Fixture f; f.seed(binding()); f.memory.data[0] ^= 1U;
    f.observed.state = TaskStoreBindingState::Unbound;
    CHECK(f.observe().code() == ErrorCode::InvalidEncoding);
    CHECK(f.observed.state == TaskStoreBindingState::Unknown && f.store.failed());
    f.memory.data[0] ^= 1U; // Even a now-readable same handle cannot unlock it.
    CHECK(f.observe().code() == ErrorCode::StateConflict);
    CHECK(f.observed.state == TaskStoreBindingState::Unknown);
    f.readOnly();
}

void unknownWriteCannotBeConfirmedFromTheAttemptedHandleView() {
    Fixture f; f.memory.unknownWrite = true;
    CHECK(f.store.replace(binding(), nullptr, 0U).code() == ErrorCode::WouldBlock);
    CHECK(f.memory.present && f.store.failed());
    f.observed.state = TaskStoreBindingState::Unbound;
    CHECK(f.observe().code() == ErrorCode::StateConflict);
    CHECK(f.observed.state == TaskStoreBindingState::Unknown);
    CHECK(f.memory.reads == 0U && f.memory.writes == 1U && f.memory.clears == 0U);
}

void readFailureReturnsOriginalErrorWithoutImplicitRetry() {
    Fixture f; f.memory.readError = ErrorCode::WouldBlock;
    CHECK(f.observe().code() == ErrorCode::WouldBlock);
    CHECK(f.observed.state == TaskStoreBindingState::Unknown);
    f.memory.readError = ErrorCode::Ok;
    CHECK(f.observe().code() == ErrorCode::StateConflict);
    CHECK(f.observed.state == TaskStoreBindingState::Unknown);
    f.readOnly();
}

void insufficientScratchCannotBeMistakenForUnbound() {
    Fixture f; TaskSnapshotHeader header = binding();
    header.tableRevision = 1U; header.nextTaskId = 2U;
    header.requestId[0] = 1U; header.requestDigest[0] = 2U;
    const TaskSnapshotRecord task = record(); f.seed(header, &task, 1U);
    CHECK(f.observe(0U).code() == ErrorCode::BufferTooSmall);
    CHECK(f.observed.state == TaskStoreBindingState::Unknown && f.store.failed());
    f.readOnly();
    Fixture small;
    TaskSnapshotStore tooSmall(small.memory, MutableByteSpan(small.workspace, TaskSnapshotCodec::encodedSize(0U) - 1U));
    CHECK(tooSmall.observeBinding(small.observed, small.header, small.records, 4U).code() == ErrorCode::BufferTooSmall);
    CHECK(small.observed.state == TaskStoreBindingState::Unknown);
    small.readOnly(0U);
}

void eachExplicitObservationReplacesThePreviousResult() {
    Fixture f;
    CHECK(f.observe() && f.observed.state == TaskStoreBindingState::Unbound);
    f.seed(binding());
    CHECK(f.observe() && f.observed.state == TaskStoreBindingState::Bound);
    f.seed(binding(true));
    CHECK(f.observe() && f.observed.state == TaskStoreBindingState::Retired);
    f.memory.data[0] ^= 1U;
    CHECK(f.observe().code() == ErrorCode::InvalidEncoding);
    CHECK(f.observed.state == TaskStoreBindingState::Unknown);
    CHECK(!f.header.authority.valid());
    f.readOnly(4U);
    Fixture missing;
    CHECK(missing.observe() && missing.observed.state == TaskStoreBindingState::Unbound);
    missing.memory.readError = ErrorCode::WouldBlock;
    CHECK(missing.observe().code() == ErrorCode::WouldBlock);
    CHECK(missing.observed.state == TaskStoreBindingState::Unknown);
    missing.readOnly(2U);
}
} // namespace

int main() {
    missingIsTheOnlyUnboundObservation();
    activeEmptySnapshotRemainsBound();
    storedRecordRemainsBoundAndIsReadWithoutExecution();
    retirementMarkerIsNotUnbound();
    corruptionStaysUnknownAndNeverReloadsTheFailedHandle();
    unknownWriteCannotBeConfirmedFromTheAttemptedHandleView();
    readFailureReturnsOriginalErrorWithoutImplicitRetry();
    insufficientScratchCannotBeMistakenForUnbound();
    eachExplicitObservationReplacesThePreviousResult();
    std::cout << "TaskStore binding observation: 9 read-only cases passed" << std::endl;
}
