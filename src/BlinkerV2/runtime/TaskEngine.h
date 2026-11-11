#ifndef BLINKER_RUNTIME_TASKENGINE_H
#define BLINKER_RUNTIME_TASKENGINE_H

#include "TaskSnapshotStore.h"
#include "../core/TaskCalendarCursor.h"

namespace blinker {

// Internal execution seam, NOT a network owner proof. Production wiring must
// sample the same monotonic tick used to read the WallClock, and obtain scope
// from current verified device authority, never from a request body.
struct TaskObservation {
    WallClockSnapshot time;
    uint32_t nowMillis;
    TaskAuthorityObservation authority;
    TaskObservation() : time(), nowMillis(0U), authority() {}
};
class ITaskEnvironment {
public:
    virtual ~ITaskEnvironment() {}
    virtual void observe(TaskObservation& output) = 0;
};
struct TaskTargetView {
    StringView key;
    ValueType valueType;
    TaskTargetView() : key(), valueType(ValueType::Boolean) {}
};
enum : size_t { kTaskMaximumTargets = 32U };
class ITaskTarget {
public:
    virtual ~ITaskTarget() {}
    // validate is side-effect-free and enforces opt-in/key/type/constraints.
    virtual Result validate(const TaskSnapshotRecord& task) = 0;
    // Synchronous, bounded, at most one call. WouldBlock is a failure, not an
    // asynchronous acceptance. Success is not proof of physical actuation.
    virtual Result apply(const TaskSnapshotRecord& task) = 0;
    // Optional, side-effect-free catalog. UINT32_MAX queries count only.
    // Views borrow immutable descriptors, never callbacks or mutable state.
    virtual Result describe(size_t, TaskTargetView&, size_t&) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
};

struct TaskWritePolicy {
    uint32_t windowMillis;
    uint32_t minimumStartMillis;
    uint32_t maximumCommitMillis;
    uint16_t writesPerWindow;
    // No unmeasured product defaults. Platform admission supplies a policy.
    TaskWritePolicy() : windowMillis(0U), minimumStartMillis(0U),
        maximumCommitMillis(0U), writesPerWindow(0U) {}
};

enum class TaskMutationKind : uint8_t { Create, Replace, Remove, SetEnabled, Start };
// Only explicit pre-persistence admission decisions, never generic ErrorCode.
// None includes all uncertain/storage/context failures and successful replay.
enum class TaskMutationRejection : uint8_t { None, WriteBudget, StartCooldown };
struct TaskMutation {
    TaskSnapshotRecord definition; // Definition fields only for Create/Replace.
    uint8_t requestId[16];
    uint32_t expectedRevision;
    uint32_t taskId; // Zero only for Create.
    TaskMutationKind kind;
    bool enabled; // SetEnabled only; definition.enabled for Create/Replace.
    TaskMutation() : definition(), requestId(), expectedRevision(0U), taskId(0U),
        kind(TaskMutationKind::Create), enabled(false) {}
};
struct TaskMutationReceipt {
    uint32_t tableRevision;
    uint32_t taskId;
    bool replayed;
    TaskMutationReceipt() : tableRevision(0U), taskId(0U), replayed(false) {}
};
// Shared, side-effect-free definition validation for Engine and management.
Result validateTaskMutation(const TaskMutation& request);
struct TaskPollReport {
    uint32_t taskId;
    TaskProgress progress;
    bool callbackInvoked;
    TaskPollReport() : taskId(0U), progress(TaskProgress::None), callbackInvoked(false) {}
};
struct TaskRuntimeSlot {
    TaskCalendarCursor calendar;
    TaskCountdown countdown;
    bool targetBlocked;
    TaskRuntimeSlot() : calendar(), countdown(), targetBlocked(false) {}
};
enum class TaskEngineState : uint8_t { Uninitialized, Empty, Ready, Recovering, Retired, Fault, Cleared, AwaitingAuthority };

// Optional portable core. Caller owns disjoint tables and Store workspace;
// never mutate those buffers behind the engine. No facade, threads or heap.
class TaskEngine {
public:
    TaskEngine(TaskSnapshotStore& store, ITaskEnvironment& environment, ITaskTarget& target,
               TaskSnapshotRecord* records, TaskRuntimeSlot* slots, size_t capacity,
               const TaskWritePolicy& policy);

    // Once per instance. Missing data returns NotFound + Empty, never formats.
    Result begin();
    // Trusted management adapter only, after actual owner verification.
    Result initializeVerified(const TaskAuthorityBinding& verifiedAuthority);
    // Internal CAS seam for a separately verified same-owner root transition.
    // No scope/instance changes, no tableRevision or replay-receipt changes.
    Result rebindVerified(const TaskAuthorityBinding& expected, const TaskAuthorityBinding& replacement);
    Result mutateVerified(const TaskAuthorityBinding& verifiedAuthority, const TaskMutation& request,
                          TaskMutationReceipt& receipt, TaskMutationRejection* rejection = nullptr);
    Result poll(TaskPollReport& report);
    Result recordAt(size_t index, TaskSnapshotRecord& output);
    // Internal lifecycle metadata only, NOT network read access or owner proof.
    // Available after a validated load even while awaiting independent roots.
    Result authorityBinding(TaskAuthorityBinding& output) const;
    TaskEngineState state() const { return state_; }
    uint32_t tableRevision() const { return header_.tableRevision; }
    size_t count() const { return count_; }
    size_t capacity() const { return capacity_; }
    // Internal read seam; the authenticated management session owns admission.
    // Hashes actual capacity and ordered targets without a full catalog buffer.
    Result describeTargets(size_t index, TaskTargetView& output, size_t& total, MutableByteSpan fingerprint);
    // Authority retirement is immediate and terminal, even during a callback.
    // It stops execution, not physical storage deletion or credential handling.
    void retire() { if (state_ != TaskEngineState::Cleared) state_ = TaskEngineState::Retired; }
    // Trusted lifecycle only, AFTER a fresh authorized retirement has been
    // verified against this exact stored binding. Never call on MQTT errors.
    // Atomically replace the table with a terminal marker; success survives
    // reboot. Exact request retries do not write. Failed writes remain unknown
    // and latch the store; no erase/read-back/retry on that dirty handle.
    Result retireVerified(const TaskAuthorityBinding& expected, ByteView requestId);
    // Internal validated metadata, NOT an authenticated network response.
    // Only exposes a KNOWN durable marker, including after a fresh boot.
    Result retirementRecord(TaskSnapshotHeader& output) const;
    // Trusted lifecycle, AFTER retire(), outside receive/property callbacks.
    // Success permits the caller to continue clearing authority. Failure must
    // stop that transition; no dirty-handle retry. Cleared stays terminal and
    // idempotent: it is NOT an empty table that can be initialized in place.
    Result clearRetired();

private:
    Result observe(TaskObservation& output, const TaskAuthorityBinding& expected);
    void resume(const TaskObservation& observation);
    Result persist(const TaskSnapshotHeader& header, size_t count, TaskObservation& after);
    void prime(size_t index, const TaskObservation& observation, bool preserveWatermark = false);
    Result runOne(size_t index, TaskRuntimeSlot candidate, const TaskObservation& before,
                  uint64_t occurrence, uint32_t deadline, TaskProgress decision, TaskPollReport& report);

    TaskSnapshotStore& store_;
    ITaskEnvironment& environment_;
    ITaskTarget& target_;
    TaskSnapshotRecord* records_;
    TaskRuntimeSlot* slots_;
    size_t capacity_;
    size_t count_;
    TaskSnapshotHeader header_;
    TaskWritePolicy policy_;
    uint32_t writeInterval_;
    uint32_t budgetTick_;
    uint32_t budgetRemainder_;
    uint32_t startCooldownMillis_;
    uint16_t credits_;
    TaskEngineState state_;
    bool busy_;
    TaskEngine(const TaskEngine&);
    TaskEngine& operator=(const TaskEngine&);
};

} // namespace blinker
#endif
