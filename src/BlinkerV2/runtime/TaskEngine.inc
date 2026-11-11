#include "TaskEngine.h"

#include <string.h>
#include "../core/Sha256.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {
namespace {

struct TaskCallGuard {
    bool& busy;
    explicit TaskCallGuard(bool& value) : busy(value) { busy = true; }
    ~TaskCallGuard() { busy = false; }
};

bool taskBytesNonzero(const uint8_t* bytes, size_t size) {
    for (size_t i = 0U; i < size; ++i) if (bytes[i] != 0U) return true;
    return false;
}

bool taskEngineActive(TaskEngineState state) {
    return state == TaskEngineState::Ready || state == TaskEngineState::Recovering;
}

} // namespace

Result TaskEngine::describeTargets(size_t index, TaskTargetView& output, size_t& total, MutableByteSpan fingerprint) {
    output = TaskTargetView(); total = 0U;
    if (fingerprint.data == nullptr || fingerprint.size != 32U) return Result::failure(ErrorCode::InvalidArgument);
    secureZero(fingerprint);
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (!taskEngineActive(state_)) return Result::failure(ErrorCode::NotConfigured);
    TaskCallGuard guard(busy_);
    TaskTargetView view; size_t count = 0U;
    Result result = target_.describe(UINT32_MAX, view, count);
    if (!result) return result;
    if (count > kTaskMaximumTargets || (index != UINT32_MAX && index >= count))
        return Result::failure(ErrorCode::ValueOutOfRange);
    Sha256 hash;
    cbor::Writer writer([](void* context, ByteView bytes) -> Result {
        return static_cast<Sha256*>(context)->update(bytes);
    }, &hash);
    result = writer.beginArray(3U);
    if (result) result = writer.writeText(StringView("blinker.task-targets.v1"));
    if (result) result = writer.writeUnsigned(capacity_);
    if (result) result = writer.beginArray(count);
    TaskTargetView selected;
    for (size_t i = 0U; result && i < count; ++i) {
        size_t observed = 0U;
        result = target_.describe(i, view, observed);
        if (result && (observed != count || view.key.data == nullptr || view.key.empty() ||
            view.key.size > kMaxEndpointKeyLength || view.valueType > ValueType::Float64))
            result = Result::failure(ErrorCode::InvalidArgument);
        if (result) result = writer.beginArray(2U);
        if (result) result = writer.writeText(view.key);
        if (result) result = writer.writeUnsigned(static_cast<uint8_t>(view.valueType));
        if (i == index) selected = view;
    }
    if (result) result = hash.finish(fingerprint);
    if (result && !taskEngineActive(state_)) result = Result::failure(ErrorCode::NotConfigured);
    if (result) { output = selected; total = count; }
    else secureZero(fingerprint);
    return result;
}

Result validateTaskMutation(const TaskMutation& request) {
    const bool definition = request.kind == TaskMutationKind::Create || request.kind == TaskMutationKind::Replace;
    const TaskSnapshotRecord& value = request.definition;
    if (request.kind > TaskMutationKind::Start || !taskBytesNonzero(request.requestId, sizeof(request.requestId)) ||
        (request.taskId == 0U) != (request.kind == TaskMutationKind::Create) ||
        (request.kind != TaskMutationKind::SetEnabled && request.enabled) ||
        value.taskId != 0U || value.definitionRevision != 0U || value.startSequence != 0U ||
        value.consumedUtcMillis != 0U || value.progress != TaskProgress::None)
        return Result::failure(ErrorCode::InvalidArgument);
    if (definition) {
        TaskSnapshotHeader header;
        header.tableRevision = 1U;
        header.nextTaskId = 2U;
        TaskSnapshotRecord record = value;
        record.taskId = 1U;
        record.definitionRevision = 1U;
        return TaskSnapshotCodec::validateRecord(record, header);
    }
    if (value.enabled || value.keyLength != 0U || value.valueLength != 0U ||
        value.valueType != ValueType::Boolean || value.timing.kind != TaskTimingKind::Countdown ||
        value.timing.utcMillis != 0U || value.timing.durationMillis != 0U ||
        value.timing.minuteOfDay != 0U || value.timing.utcOffsetMinutes != 0 || value.timing.weekdays != 0U)
        return Result::failure(ErrorCode::InvalidArgument);
    return Result::success();
}

namespace {

Result taskDigestSink(void* context, ByteView bytes) {
    return static_cast<Sha256*>(context)->update(bytes);
}

// Local replay fingerprint, not a public wire or signature/proof format.
Result taskMutationDigest(const TaskAuthorityBinding& authority, const TaskMutation& request, uint8_t* digest) {
    Sha256 hash;
    cbor::Writer writer(taskDigestSink, &hash);
    Result result = writer.beginArray(7U);
    if (result) result = writer.writeText(StringView("BTM2"));
    if (result) result = writer.writeBytes(ByteView(authority.taskScope, sizeof(authority.taskScope)));
    if (result) result = writer.writeUnsigned(request.expectedRevision);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(request.kind));
    if (result) result = writer.writeUnsigned(request.taskId);
    if (result) result = writer.writeBool(request.enabled);
    if (request.kind == TaskMutationKind::Create || request.kind == TaskMutationKind::Replace) {
        const TaskSnapshotRecord& value = request.definition;
        if (result) result = writer.beginArray(10U);
        if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.timing.kind));
        if (result) result = writer.writeUnsigned(value.timing.utcMillis);
        if (result) result = writer.writeUnsigned(value.timing.durationMillis);
        if (result) result = writer.writeUnsigned(value.timing.minuteOfDay);
        if (result) result = writer.writeInt(value.timing.utcOffsetMinutes);
        if (result) result = writer.writeUnsigned(value.timing.weekdays);
        if (result) result = writer.writeBool(value.enabled);
        if (result) result = writer.writeText(StringView(reinterpret_cast<const char*>(value.key), value.keyLength));
        if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.valueType));
        if (result) result = writer.writeEncodedValue(ByteView(value.value, value.valueLength), cbor::Limits());
    } else if (result) {
        result = writer.writeNull();
    }
    return result ? hash.finish(MutableByteSpan(digest, kSha256Size)) : result;
}

bool taskStillDue(const TaskSnapshotRecord& task, const TaskObservation& before,
                  const TaskObservation& after, uint64_t occurrence, uint32_t deadline) {
    const uint32_t elapsed = after.nowMillis - before.nowMillis;
    if (elapsed > 0x7FFFFFFFU) return false;
    if (task.timing.kind == TaskTimingKind::Countdown)
        return after.nowMillis - deadline <= TaskCountdown::kLatenessMillis;
    return taskCalendarContinues(before.time.utcMillis, before.time.sampleRevision, elapsed, after.time) &&
        after.time.utcMillis >= occurrence && after.time.utcMillis - occurrence <= TaskCountdown::kLatenessMillis;
}

} // namespace

TaskEngine::TaskEngine(TaskSnapshotStore& store, ITaskEnvironment& environment, ITaskTarget& target,
                       TaskSnapshotRecord* records, TaskRuntimeSlot* slots, size_t capacity,
                       const TaskWritePolicy& policy)
    : store_(store), environment_(environment), target_(target), records_(records), slots_(slots),
      capacity_(capacity), count_(0U), header_(), policy_(policy), writeInterval_(0U),
      budgetTick_(0U), budgetRemainder_(0U), startCooldownMillis_(0U), credits_(0U),
      state_(TaskEngineState::Uninitialized), busy_(false) {}

Result TaskEngine::observe(TaskObservation& output, const TaskAuthorityBinding& expected) {
    if (state_ == TaskEngineState::Retired) return Result::failure(ErrorCode::AuthenticationRequired);
    if (state_ == TaskEngineState::Fault) return Result::failure(ErrorCode::StateConflict);
    output = TaskObservation();
    environment_.observe(output);
    if (state_ == TaskEngineState::Retired || output.authority.state == TaskAuthorityState::Retired) {
        retire();
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    const uint32_t elapsed = output.nowMillis - budgetTick_;
    if (elapsed > 0x7FFFFFFFU) {
        state_ = TaskEngineState::Fault;
        return Result::failure(ErrorCode::StateConflict);
    }
    budgetTick_ = output.nowMillis;
    // Decrement while polling instead of retaining an old Start tick forever:
    // a full uint32 uptime wrap must not resurrect an expired cooldown.
    startCooldownMillis_ = elapsed >= startCooldownMillis_ ? 0U : startCooldownMillis_ - elapsed;
    const uint64_t total = static_cast<uint64_t>(budgetRemainder_) + elapsed;
    const uint64_t available = credits_ + total / writeInterval_;
    credits_ = static_cast<uint16_t>(available > policy_.writesPerWindow ? policy_.writesPerWindow : available);
    budgetRemainder_ = credits_ == policy_.writesPerWindow ? 0U : static_cast<uint32_t>(total % writeInterval_);
    if (output.authority.state != TaskAuthorityState::Current || !output.authority.binding.valid()) {
        if (taskEngineActive(state_)) state_ = TaskEngineState::AwaitingAuthority;
        return Result::failure(ErrorCode::WouldBlock);
    }
    if (!expected.valid() || !output.authority.binding.sameOwner(expected)) {
        if (state_ != TaskEngineState::Empty) retire();
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (output.authority.binding != expected) {
        if (taskEngineActive(state_)) state_ = TaskEngineState::AwaitingAuthority;
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    return Result::success();
}

void TaskEngine::resume(const TaskObservation& observation) {
    state_ = TaskEngineState::Ready;
    for (size_t i = 0U; i < count_; ++i) {
        // No catch-up or new countdown deadline after an authority gap.
        prime(i, observation, true);
        if (records_[i].progress == TaskProgress::CountdownArmed) state_ = TaskEngineState::Recovering;
    }
}

void TaskEngine::prime(size_t index, const TaskObservation& observation, bool preserveWatermark) {
    uint64_t consumed = records_[index].consumedUtcMillis;
    if (preserveWatermark && slots_[index].calendar.watermark() > consumed)
        consumed = slots_[index].calendar.watermark();
    slots_[index] = TaskRuntimeSlot();
    slots_[index].calendar.reset(consumed);
    if (records_[index].enabled && records_[index].timing.kind != TaskTimingKind::Countdown) {
        TaskCalendarDecision ignored;
        slots_[index].calendar.poll(records_[index].timing, observation.time, observation.nowMillis, ignored);
    }
}

Result TaskEngine::begin() {
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (state_ != TaskEngineState::Uninitialized) return Result::failure(ErrorCode::AlreadyExists);
    TaskCallGuard guard(busy_);
    if (records_ == nullptr || slots_ == nullptr || capacity_ == 0U || capacity_ > TaskSnapshotCodec::maxTasks ||
        !store_.hasCapacity(capacity_) || policy_.writesPerWindow < 2U || policy_.windowMillis == 0U ||
        policy_.windowMillis > 0x7FFFFFFFU || policy_.minimumStartMillis == 0U ||
        policy_.minimumStartMillis > 0x7FFFFFFFU || policy_.maximumCommitMillis == 0U ||
        policy_.maximumCommitMillis > 0x7FFFFFFFU) {
        state_ = TaskEngineState::Fault;
        return Result::failure(ErrorCode::InvalidArgument);
    }
    writeInterval_ = static_cast<uint32_t>((static_cast<uint64_t>(policy_.windowMillis) +
        policy_.writesPerWindow - 1U) / policy_.writesPerWindow);
    TaskObservation observation;
    environment_.observe(observation);
    if (state_ == TaskEngineState::Retired) return Result::failure(ErrorCode::AuthenticationRequired);
    budgetTick_ = observation.nowMillis;
    startCooldownMillis_ = policy_.minimumStartMillis;
    Result result = store_.load(header_, records_, capacity_, count_);
    if (state_ == TaskEngineState::Retired) return Result::failure(ErrorCode::AuthenticationRequired);
    if (result.code() == ErrorCode::NotFound) {
        state_ = TaskEngineState::Empty;
        return result;
    }
    if (!result) { state_ = TaskEngineState::Fault; return result; }
    if (header_.retired) {
        retire(); // Before scope restoration, normalization, targets or any writes.
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    state_ = TaskEngineState::AwaitingAuthority;
    for (size_t i = 0U; i < count_; ++i) slots_[i] = TaskRuntimeSlot();
    result = observe(observation, header_.authority);
    if (!result) return result;
    resume(observation);
    return Result::success();
}

Result TaskEngine::persist(const TaskSnapshotHeader& header, size_t count, TaskObservation& after) {
    if (credits_ == 0U) return Result::failure(ErrorCode::WouldBlock);
    --credits_; // Charge attempted writes as well as successful writes.
    const uint32_t start = budgetTick_;
    Result result = store_.replace(header, records_, count);
    if (!result) { state_ = TaskEngineState::Fault; return result; }
    // Durability is known, even when authority disappears during the write.
    // Keep staged records/header/count consistent before a resumable pause.
    header_ = header;
    count_ = count;
    if (state_ == TaskEngineState::Empty) state_ = TaskEngineState::AwaitingAuthority;
    result = observe(after, header.authority);
    if (state_ != TaskEngineState::Retired && after.nowMillis - start > policy_.maximumCommitMillis) {
        state_ = TaskEngineState::Fault; // Durable may be new; never apply now.
        return Result::failure(ErrorCode::StateConflict);
    }
    if (!result) return result;
    return Result::success();
}

Result TaskEngine::retireVerified(const TaskAuthorityBinding& expected, ByteView requestId) {
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if ((!taskEngineActive(state_) && state_ != TaskEngineState::AwaitingAuthority &&
         state_ != TaskEngineState::Retired) || store_.failed())
        return Result::failure(ErrorCode::StateConflict);
    if (!expected.valid() || expected != header_.authority)
        return Result::failure(ErrorCode::AuthenticationRequired);
    if (requestId.data == nullptr || requestId.size != 16U || !taskBytesNonzero(requestId.data, requestId.size))
        return Result::failure(ErrorCode::InvalidArgument);
    TaskCallGuard guard(busy_);
    TaskSnapshotHeader next = header_;
    next.retired = true;
    memcpy(next.requestId, requestId.data, 16U);
    // Local idempotency fingerprint only, not the eventual signed wire digest.
    Sha256 hash;
    cbor::Writer writer(taskDigestSink, &hash);
    Result result = writer.beginArray(5U);
    if (result) result = writer.writeText(StringView("BTR1"));
    if (result) result = writer.writeBytes(ByteView(expected.deviceInstanceId, 16U));
    if (result) result = writer.writeBytes(ByteView(expected.taskScope, 16U));
    if (result) result = writer.writeBytes(ByteView(expected.rootDigest, 32U));
    if (result) result = writer.writeBytes(requestId);
    if (result) result = hash.finish(MutableByteSpan(next.requestDigest, 32U));
    if (!result) return result;
    if (header_.retired) {
        return memcmp(header_.requestId, next.requestId, 16U) == 0 &&
               memcmp(header_.requestDigest, next.requestDigest, 32U) == 0
            ? Result::success() : Result::failure(ErrorCode::StateConflict);
    }
    retire(); // Stop before storage; no callback can execute or re-enter Flash.
    // One terminal write is not blocked by a spent ordinary editing budget.
    // Any failure locks Store; success makes future retries read-only.
    result = store_.replace(next, nullptr, 0U);
    if (!result) return result;
    header_ = next;
    count_ = 0U;
    for (size_t i = 0U; i < capacity_; ++i) {
        records_[i] = TaskSnapshotRecord(); slots_[i] = TaskRuntimeSlot();
    }
    return Result::success();
}

Result TaskEngine::retirementRecord(TaskSnapshotHeader& output) const {
    output = TaskSnapshotHeader();
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (state_ != TaskEngineState::Retired || !header_.retired || store_.failed())
        return Result::failure(ErrorCode::StateConflict);
    output = header_;
    return Result::success();
}

Result TaskEngine::initializeVerified(const TaskAuthorityBinding& verifiedAuthority) {
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (state_ != TaskEngineState::Empty) return Result::failure(ErrorCode::StateConflict);
    if (!verifiedAuthority.valid()) return Result::failure(ErrorCode::AuthenticationRequired);
    TaskCallGuard guard(busy_);
    TaskObservation observation;
    Result result = observe(observation, verifiedAuthority);
    if (!result) return result;
    TaskSnapshotHeader initial;
    initial.authority = verifiedAuthority;
    result = persist(initial, 0U, observation);
    if (result) state_ = TaskEngineState::Ready;
    return result;
}

Result TaskEngine::rebindVerified(const TaskAuthorityBinding& expected, const TaskAuthorityBinding& replacement) {
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (!taskEngineActive(state_) && state_ != TaskEngineState::AwaitingAuthority)
        return Result::failure(ErrorCode::StateConflict);
    if (!expected.valid() || !replacement.valid() || !expected.sameOwner(replacement) ||
        (header_.authority != expected && header_.authority != replacement))
        return Result::failure(ErrorCode::AuthenticationRequired);
    TaskCallGuard guard(busy_);
    TaskObservation observation;
    Result result = observe(observation, replacement);
    if (!result) return result;
    if (header_.authority == replacement) return Result::success(); // Durable CAS retry, no write.
    TaskSnapshotHeader next = header_;
    next.authority = replacement;
    result = persist(next, count_, observation);
    if (result) resume(observation);
    return result;
}

Result TaskEngine::mutateVerified(const TaskAuthorityBinding& verifiedAuthority, const TaskMutation& request,
                                  TaskMutationReceipt& receipt, TaskMutationRejection* rejection) {
    receipt = TaskMutationReceipt();
    if (rejection != nullptr) *rejection = TaskMutationRejection::None;
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (!taskEngineActive(state_)) return Result::failure(ErrorCode::StateConflict);
    if (!verifiedAuthority.valid() || verifiedAuthority != header_.authority)
        return Result::failure(ErrorCode::AuthenticationRequired);
    TaskCallGuard guard(busy_);
    TaskObservation observation;
    Result result = observe(observation, header_.authority);
    if (!result) return result;
    result = validateTaskMutation(request);
    if (!result) return result;
    uint8_t digest[kSha256Size];
    result = taskMutationDigest(verifiedAuthority, request, digest);
    if (!result) return result;
    if (header_.tableRevision != 0U && memcmp(header_.requestId, request.requestId, sizeof(request.requestId)) == 0) {
        if (memcmp(header_.requestDigest, digest, sizeof(digest)) != 0)
            return Result::failure(ErrorCode::SequenceConflict);
        receipt.tableRevision = header_.tableRevision;
        receipt.taskId = request.kind == TaskMutationKind::Create ? header_.nextTaskId - 1U : request.taskId;
        receipt.replayed = true;
        return Result::success();
    }
    if (request.expectedRevision != header_.tableRevision || header_.tableRevision == UINT32_MAX)
        return Result::failure(ErrorCode::SequenceConflict);
    if (state_ == TaskEngineState::Recovering) return Result::failure(ErrorCode::WouldBlock);
    size_t index = 0U;
    while (index < count_ && records_[index].taskId != request.taskId) ++index;
    const bool create = request.kind == TaskMutationKind::Create;
    if (create && count_ == capacity_) return Result::failure(ErrorCode::CapacityExceeded);
    if (create && header_.nextTaskId == UINT32_MAX) return Result::failure(ErrorCode::SequenceConflict);
    if (!create && index == count_) return Result::failure(ErrorCode::NotFound);
    TaskSnapshotRecord candidate = create ? request.definition : records_[index];
    TaskSnapshotHeader next = header_;
    ++next.tableRevision;
    bool rearm = create;
    if (create) {
        candidate.taskId = next.nextTaskId++;
        candidate.definitionRevision = 1U;
    } else if (request.kind == TaskMutationKind::Replace) {
        if (candidate.definitionRevision == UINT32_MAX) return Result::failure(ErrorCode::SequenceConflict);
        const uint32_t definition = candidate.definitionRevision + 1U;
        candidate = request.definition;
        candidate.taskId = request.taskId;
        candidate.definitionRevision = definition;
        rearm = true;
    } else if (request.kind == TaskMutationKind::SetEnabled) {
        rearm = candidate.enabled != request.enabled || slots_[index].targetBlocked;
        candidate.enabled = request.enabled;
        if (!candidate.enabled && candidate.progress == TaskProgress::CountdownArmed)
            candidate.progress = TaskProgress::Interrupted;
    } else if (request.kind == TaskMutationKind::Start) {
        if (!candidate.enabled || candidate.timing.kind != TaskTimingKind::Countdown)
            return Result::failure(ErrorCode::InvalidArgument);
        if (slots_[index].countdown.active()) return Result::failure(ErrorCode::AlreadyExists);
        if (candidate.startSequence == UINT32_MAX) return Result::failure(ErrorCode::SequenceConflict);
        if (startCooldownMillis_ != 0U) {
            if (rejection != nullptr) *rejection = TaskMutationRejection::StartCooldown;
            return Result::failure(ErrorCode::WouldBlock);
        }
        ++candidate.startSequence;
        candidate.progress = TaskProgress::CountdownArmed;
    }
    if (request.kind != TaskMutationKind::Remove) {
        result = TaskSnapshotCodec::validateRecord(candidate, next);
        if (!result) return result;
        if (candidate.enabled) {
            result = target_.validate(candidate);
            if (!result) return result;
        }
    }
    result = observe(observation, header_.authority);
    if (!result) return result;
    if (credits_ == 0U) {
        if (rejection != nullptr) *rejection = TaskMutationRejection::WriteBudget;
        return Result::failure(ErrorCode::WouldBlock);
    }
    memcpy(next.requestId, request.requestId, sizeof(next.requestId));
    memcpy(next.requestDigest, digest, sizeof(next.requestDigest));
    const bool remove = request.kind == TaskMutationKind::Remove;
    const size_t nextCount = create ? count_ + 1U : (remove ? count_ - 1U : count_);
    // One table plus Store scratch. Reads/reentry are blocked while staged;
    // any persistence error faults the engine, so staged RAM is never exposed.
    if (remove) {
        for (size_t i = index; i + 1U < count_; ++i) {
            records_[i] = records_[i + 1U];
            slots_[i] = slots_[i + 1U];
        }
    } else {
        records_[index] = candidate;
        if (create || request.kind == TaskMutationKind::Replace) slots_[index] = TaskRuntimeSlot();
    }
    result = persist(next, nextCount, observation);
    if (!result) return result;
    if (remove) {
        records_[count_] = TaskSnapshotRecord();
        slots_[count_] = TaskRuntimeSlot();
    } else if (rearm) {
        prime(index, observation, request.kind == TaskMutationKind::SetEnabled);
    }
    if (request.kind == TaskMutationKind::Start) {
        slots_[index].targetBlocked = false;
        slots_[index].countdown.start(candidate.timing.durationMillis, observation.nowMillis);
        startCooldownMillis_ = policy_.minimumStartMillis;
    }
    receipt.tableRevision = header_.tableRevision;
    receipt.taskId = candidate.taskId;
    return Result::success();
}

Result TaskEngine::authorityBinding(TaskAuthorityBinding& output) const {
    output = TaskAuthorityBinding();
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (state_ == TaskEngineState::Empty) return Result::failure(ErrorCode::NotFound);
    if (!taskEngineActive(state_) && state_ != TaskEngineState::AwaitingAuthority)
        return Result::failure(ErrorCode::StateConflict);
    output = header_.authority;
    return Result::success();
}

Result TaskEngine::recordAt(size_t index, TaskSnapshotRecord& output) {
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (!taskEngineActive(state_)) return Result::failure(ErrorCode::StateConflict);
    TaskCallGuard guard(busy_);
    TaskObservation observation;
    Result result = observe(observation, header_.authority);
    if (!result) return result;
    if (index >= count_) return Result::failure(ErrorCode::NotFound);
    output = records_[index];
    if (state_ == TaskEngineState::Recovering && output.progress == TaskProgress::CountdownArmed)
        output.progress = TaskProgress::Interrupted;
    return Result::success();
}

Result TaskEngine::clearRetired() {
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (state_ == TaskEngineState::Cleared) return Result::success();
    if (state_ != TaskEngineState::Retired) return Result::failure(ErrorCode::StateConflict);
    TaskCallGuard guard(busy_);
    const Result result = store_.clear();
    if (!result) return result; // Remain stopped; Store latches uncertain durability.
    for (size_t i = 0U; i < count_; ++i) {
        records_[i] = TaskSnapshotRecord();
        slots_[i] = TaskRuntimeSlot();
    }
    count_ = 0U;
    header_ = TaskSnapshotHeader();
    state_ = TaskEngineState::Cleared;
    return Result::success();
}

Result TaskEngine::runOne(size_t index, TaskRuntimeSlot candidate, const TaskObservation& before,
                          uint64_t occurrence, uint32_t deadline, TaskProgress decision, TaskPollReport& report) {
    report.taskId = records_[index].taskId;
    const bool due = decision == TaskProgress::Claimed;
    if (credits_ < (due ? 2U : 1U)) return Result::failure(ErrorCode::WouldBlock);
    TaskObservation current = before;
    if (due) {
        const Result allowed = target_.validate(records_[index]);
        Result result = observe(current, header_.authority);
        if (!result) return result;
        if (!allowed) { decision = TaskProgress::Failed; candidate.targetBlocked = true; }
        else if (!taskStillDue(records_[index], before, current, occurrence, deadline)) decision = TaskProgress::Skipped;
    }
    records_[index].consumedUtcMillis = occurrence;
    records_[index].progress = decision;
    Result result = persist(header_, count_, current);
    if (!result) return result;
    slots_[index] = candidate;
    report.progress = decision;
    if (decision != TaskProgress::Claimed) return Result::success();

    const Result allowed = target_.validate(records_[index]);
    result = observe(current, header_.authority);
    if (!result) return result;
    if (!allowed) {
        decision = TaskProgress::Failed;
        slots_[index].targetBlocked = true;
    } else if (!taskStillDue(records_[index], before, current, occurrence, deadline)) {
        decision = TaskProgress::Skipped;
    } else {
        report.callbackInvoked = true;
        decision = target_.apply(records_[index]) ? TaskProgress::Completed : TaskProgress::Failed;
    }
    // A callback can retire authority. Do not write an old-scope result then.
    result = observe(current, header_.authority);
    if (!result) return result;
    records_[index].progress = decision;
    result = persist(header_, count_, current);
    if (result) report.progress = decision;
    return result;
}

Result TaskEngine::poll(TaskPollReport& report) {
    report = TaskPollReport();
    if (busy_) return Result::failure(ErrorCode::WouldBlock);
    if (!taskEngineActive(state_) && state_ != TaskEngineState::AwaitingAuthority)
        return Result::failure(ErrorCode::StateConflict);
    TaskCallGuard guard(busy_);
    TaskObservation observation;
    Result result = observe(observation, header_.authority);
    if (!result) return result;
    if (state_ == TaskEngineState::AwaitingAuthority) resume(observation);
    if (state_ == TaskEngineState::Recovering) {
        if (credits_ == 0U) return Result::failure(ErrorCode::WouldBlock);
        for (size_t i = 0U; i < count_; ++i)
            if (records_[i].progress == TaskProgress::CountdownArmed) records_[i].progress = TaskProgress::Interrupted;
        result = persist(header_, count_, observation);
        if (result) state_ = TaskEngineState::Ready;
        return result;
    }
    for (size_t i = 0U; i < count_; ++i) {
        if (!records_[i].enabled || slots_[i].targetBlocked) continue;
        TaskRuntimeSlot candidate = slots_[i];
        if (records_[i].timing.kind == TaskTimingKind::Countdown) {
            if (records_[i].progress != TaskProgress::CountdownArmed) continue;
            const uint32_t deadline = candidate.countdown.deadlineMillis();
            const CountdownPoll state = candidate.countdown.poll(observation.nowMillis);
            if (state == CountdownPoll::Due || state == CountdownPoll::Skipped || state == CountdownPoll::Interrupted) {
                const TaskProgress decision = state == CountdownPoll::Due ? TaskProgress::Claimed
                    : (state == CountdownPoll::Skipped ? TaskProgress::Skipped : TaskProgress::Interrupted);
                return runOne(i, candidate, observation, 0U, deadline, decision, report);
            }
        } else {
            TaskCalendarDecision decision;
            result = candidate.calendar.poll(records_[i].timing, observation.time, observation.nowMillis, decision);
            if (!result) { state_ = TaskEngineState::Fault; return result; }
            if (decision.state == CalendarPoll::Due || decision.state == CalendarPoll::Skipped)
                return runOne(i, candidate, observation, decision.occurrenceUtcMillis, 0U,
                    decision.state == CalendarPoll::Due ? TaskProgress::Claimed : TaskProgress::Skipped, report);
        }
        slots_[i] = candidate;
    }
    return Result::success();
}

} // namespace blinker
