#ifndef BLINKER_RUNTIME_TASKSNAPSHOTSTORE_H
#define BLINKER_RUNTIME_TASKSNAPSHOTSTORE_H

#include "../core/TaskTiming.h"
#include "../interface/IAtomicBlobStore.h"
#include "../model/Endpoint.h"
#include "TaskAuthority.h"

namespace blinker {

// Internal storage candidate only. No task wire/facade or ownership proof.
// A recovered Claimed result is Unknown to the user, never permission to replay.
enum class TaskProgress : uint8_t {
    None, CountdownArmed, Claimed, Completed, Failed, Skipped, Interrupted
};

struct TaskSnapshotHeader {
    TaskAuthorityBinding authority;
    uint32_t tableRevision;
    uint32_t nextTaskId;
    uint8_t requestId[16];
    uint8_t requestDigest[32];
    bool retired; // Terminal marker, never a zero-task active table.

    TaskSnapshotHeader()
        : authority(), tableRevision(0U), nextTaskId(1U), requestId(), requestDigest(), retired(false) {}
};

struct TaskSnapshotRecord {
    TaskTiming timing;
    uint64_t consumedUtcMillis;
    uint32_t taskId;
    uint32_t definitionRevision;
    uint32_t startSequence;
    uint8_t key[kMaxEndpointKeyLength];
    // One owned canonical scalar CBOR item, never a borrowed session buffer.
    uint8_t value[9];
    uint8_t keyLength;
    uint8_t valueLength;
    ValueType valueType;
    TaskProgress progress;
    bool enabled;

    TaskSnapshotRecord()
        : timing(), consumedUtcMillis(0U), taskId(0U), definitionRevision(0U),
          startSequence(0U), key(), value(), keyLength(0U), valueLength(0U),
          valueType(ValueType::Boolean), progress(TaskProgress::None), enabled(false) {}
};

// Explicit big-endian disk encoding, not memcpy(struct) or a BBP/2 frame.
// Schema 2 stores instance/scope/root binding atomically with the table.
// Schema 3 is ONLY a terminal retirement marker (same header, zero records).
// Old schema-2 readers reject it; active tables retain byte-identical schema 2.
// Schema 1 (u32 epoch) is rejected, never auto-migrated or auto-formatted.
// No heap. Callers own non-overlapping record arrays and byte workspace.
// Definitions must be ordered by increasing taskId. Decode validates the whole
// snapshot before publishing any header/record; count is zero on failure.
class TaskSnapshotCodec {
public:
    enum : size_t { headerSize = 128U, recordSize = 128U, trailerSize = 4U, maxTasks = 10U };
    static size_t encodedSize(size_t count);
    static Result validateRecord(const TaskSnapshotRecord& record,
                                 const TaskSnapshotHeader& header, uint32_t previousId = 0U);
    static Result encode(const TaskSnapshotHeader& header,
                         const TaskSnapshotRecord* records, size_t count,
                         MutableByteSpan output, size_t& written);
    static Result decode(ByteView input, TaskSnapshotHeader& header,
                         TaskSnapshotRecord* records, size_t capacity, size_t& count);
};

// Persistence boundary only: TaskEngine owns verified scope checks, CAS,
// request deduplication, write quotas, and Claim -> callback -> completion.
// A storage/decode error latches this instance unusable. There is deliberately
// no retry/unlock API: reconstruct ONLY after the port's durability is known
// again (e.g. real reboot), not merely after rereading a dirty NVS handle.
class TaskSnapshotStore {
public:
    TaskSnapshotStore(IAtomicBlobStore& storage, MutableByteSpan workspace)
        : storage_(storage), workspace_(workspace), state_(State::Unloaded) {}
    Result load(TaskSnapshotHeader& header, TaskSnapshotRecord* records,
                size_t capacity, size_t& count);
    Result replace(const TaskSnapshotHeader& header,
                   const TaskSnapshotRecord* records, size_t count);
    // Trusted retirement only, after a validated load (including missing) or
    // committed replacement. Failure locks this handle just like replace;
    // never infer durable deletion from a read on the failed handle.
    Result clear();
    bool failed() const { return state_ == State::Failed; }
    bool hasCapacity(size_t count) const {
        const size_t size = TaskSnapshotCodec::encodedSize(count);
        return size != 0U && workspace_.data != nullptr && workspace_.size >= size;
    }

private:
    enum class State : uint8_t { Unloaded, Ready, Failed };
    IAtomicBlobStore& storage_;
    MutableByteSpan workspace_;
    State state_;
    TaskSnapshotStore(const TaskSnapshotStore&);
    TaskSnapshotStore& operator=(const TaskSnapshotStore&);
};

} // namespace blinker

#endif
