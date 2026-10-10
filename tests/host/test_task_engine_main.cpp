#include <cstdlib>
#include <cstring>
#include <iostream>
#include "BlinkerV2/runtime/TaskEngine.h"

#include "support/TaskAuthorityFixture.h"
#include "../arduino/Esp32WifiTaskLiveGate/TaskPilotPolicy.h"

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
const uint64_t base = 1790035200000ULL;
const size_t maxBlob = TaskSnapshotCodec::headerSize + TaskSnapshotCodec::trailerSize + 128U * 10U;

struct Environment : ITaskEnvironment {
    uint64_t utc = base;
    uint32_t now = 0U;
    TaskAuthorityBinding authority = testAuthority();
    TaskAuthorityState authorityState = TaskAuthorityState::Current;
    uint32_t revision = 1U;
    bool valid = true;
    void advance(uint32_t elapsed) { now += elapsed; utc += elapsed; }
    void observe(TaskObservation& output) override {
        output = TaskObservation();
        output.nowMillis = now;
        output.authority.binding = authority;
        output.authority.state = authorityState;
        if (valid) {
            output.time.utcMillis = utc;
            output.time.sampleRevision = revision;
            output.time.source = WallTimeSource::Server;
            output.time.state = WallClockState::Synchronized;
        }
    }
};

struct Flash : IAtomicBlobStore {
    enum class Failure { None, Before, After, Dirty };
    Environment& env;
    uint8_t durable[maxBlob] = {};
    uint8_t visible[maxBlob] = {};
    size_t durableSize = 0U;
    size_t visibleSize = 0U;
    unsigned writes = 0U;
    unsigned clears = 0U;
    unsigned loads = 0U;
    unsigned failAt = 0U;
    unsigned hookAt = 0U;
    Failure failure = Failure::None;
    uint32_t commitMillis = 0U;
    void (*hook)(Flash&) = nullptr;
    TaskEngine* engine = nullptr;
    explicit Flash(Environment& value) : env(value) {}
    Result load(MutableByteSpan output, size_t& written) override {
        ++loads;
        written = 0U;
        if (visibleSize == 0U) return Result::failure(ErrorCode::NotFound);
        CHECK(output.size >= visibleSize);
        memcpy(output.data, visible, visibleSize);
        written = visibleSize;
        return Result::success();
    }
    Result replace(ByteView value) override {
        ++writes;
        CHECK(value.size <= maxBlob);
        const Failure mode = writes == failAt ? failure : Failure::None;
        if (mode != Failure::Before) {
            memcpy(visible, value.data, value.size);
            visibleSize = value.size;
        }
        if (mode == Failure::None || mode == Failure::After) {
            memcpy(durable, value.data, value.size);
            durableSize = value.size;
        }
        env.advance(commitMillis);
        if (hook != nullptr && writes == hookAt) hook(*this);
        return mode == Failure::None ? Result::success() : Result::failure(ErrorCode::InternalError);
    }
    Result clear() override {
        ++clears;
        if (failure != Failure::Before) visibleSize = 0U;
        if (failure == Failure::None || failure == Failure::After) durableSize = 0U;
        return failure == Failure::None ? Result::success() : Result::failure(ErrorCode::InternalError);
    }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
    void reboot() {
        memcpy(visible, durable, durableSize);
        visibleSize = durableSize;
        failAt = hookAt = 0U;
        hook = nullptr;
        commitMillis = 0U;
    }
};

struct Target : ITaskTarget {
    Environment& env;
    Flash& flash;
    TaskEngine* engine = nullptr;
    uint32_t ids[20] = {};
    unsigned calls = 0U;
    unsigned validations = 0U;
    unsigned denyCall = 0U;
    uint32_t denyId = 0U;
    bool failApply = false;
    bool retireInApply = false;
    bool pendingInApply = false;
    explicit Target(Environment& value, Flash& storage) : env(value), flash(storage) {}
    Result validate(const TaskSnapshotRecord& task) override {
        ++validations;
        return task.taskId == denyId || validations == denyCall
            ? Result::failure(ErrorCode::InvalidArgument) : Result::success();
    }
    Result apply(const TaskSnapshotRecord& task) override {
        CHECK(calls < 20U);
        // Assert the actual callback sees a durable Claim, not staged RAM.
        TaskSnapshotHeader header;
        TaskSnapshotRecord records[10];
        size_t count = 0U;
        CHECK(TaskSnapshotCodec::decode(ByteView(flash.durable, flash.durableSize), header, records, 10U, count));
        bool claimed = false;
        for (size_t i = 0U; i < count; ++i)
            if (records[i].taskId == task.taskId) claimed = records[i].progress == TaskProgress::Claimed;
        CHECK(claimed);
        ids[calls++] = task.taskId;
        if (pendingInApply) env.authorityState = TaskAuthorityState::Pending;
        TaskPollReport nested;
        CHECK(engine->poll(nested).code() == ErrorCode::WouldBlock);
        TaskSnapshotRecord hidden;
        CHECK(engine->recordAt(0U, hidden).code() == ErrorCode::WouldBlock);
        TaskMutation request;
        TaskMutationReceipt receipt;
        CHECK(engine->mutateVerified(testAuthority(), request, receipt).code() == ErrorCode::WouldBlock);
        if (retireInApply) {
            engine->retire();
            CHECK(engine->clearRetired().code() == ErrorCode::WouldBlock);
            const uint8_t retiredId[16] = {77U};
            CHECK(engine->retireVerified(testAuthority(), ByteView(retiredId, 16U)).code() == ErrorCode::WouldBlock);
            CHECK(flash.clears == 0U); // No nested persistence from user callbacks.
        }
        return failApply ? Result::failure(ErrorCode::WouldBlock) : Result::success();
    }
};

TaskWritePolicy fastPolicy() {
    TaskWritePolicy policy;
    policy.windowMillis = 100U;
    policy.writesPerWindow = 100U;
    policy.minimumStartMillis = 10U;
    policy.maximumCommitMillis = 2000U;
    return policy;
}

struct Scene {
    Environment env;
    Flash flash;
    Target target;
    Scene() : env(), flash(env), target(env, flash) {}
};

template <size_t Capacity = 4U>
struct Boot {
    uint8_t workspace[TaskSnapshotCodec::headerSize + TaskSnapshotCodec::trailerSize + 128U * Capacity];
    TaskSnapshotRecord records[Capacity];
    TaskRuntimeSlot slots[Capacity];
    TaskSnapshotStore store;
    TaskEngine engine;
    Scene& scene;
    explicit Boot(Scene& value, const TaskWritePolicy& policy = fastPolicy())
        : workspace(), records(), slots(), store(value.flash, MutableByteSpan(workspace, sizeof(workspace))),
          engine(store, value.env, value.target, records, slots, Capacity, policy), scene(value) {
        scene.target.engine = &engine;
        scene.flash.engine = &engine;
    }
    void initialize() {
        CHECK(engine.begin().code() == ErrorCode::NotFound);
        CHECK(scene.flash.writes == 0U);
        CHECK(engine.initializeVerified(otherAuthority()).code() == ErrorCode::AuthenticationRequired);
        CHECK(engine.state() == TaskEngineState::Empty && scene.flash.writes == 0U);
        CHECK(engine.initializeVerified(testAuthority()).code() == ErrorCode::WouldBlock); // Cold bucket, no reboot refill.
        scene.env.advance(100U);
        CHECK(engine.initializeVerified(testAuthority()));
    }
    TaskSnapshotRecord at(size_t index = 0U) {
        TaskSnapshotRecord value;
        CHECK(engine.recordAt(index, value));
        return value;
    }
    TaskMutationReceipt change(const TaskMutation& request) {
        TaskMutationReceipt receipt;
        CHECK(engine.mutateVerified(testAuthority(), request, receipt));
        return receipt;
    }
};

TaskMutation command(TaskMutationKind kind, uint8_t id, uint32_t revision, uint32_t taskId) {
    TaskMutation request;
    request.kind = kind;
    request.requestId[0] = id;
    request.expectedRevision = revision;
    request.taskId = taskId;
    return request;
}

TaskMutation create(uint8_t id, uint32_t revision, uint64_t when, uint32_t duration = 0U) {
    auto request = command(TaskMutationKind::Create, id, revision, 0U);
    auto& definition = request.definition;
    definition.enabled = true;
    definition.keyLength = 5U;
    memcpy(definition.key, "power", 5U);
    definition.valueLength = 1U;
    definition.value[0] = 0xF5U;
    if (duration == 0U) {
        definition.timing.kind = TaskTimingKind::UtcOnce;
        definition.timing.utcMillis = when;
    } else {
        definition.timing.durationMillis = duration;
    }
    return request;
}

void managementAndReplay() {
    Scene scene;
    Boot<> boot(scene);
    boot.initialize();
    auto first = create(1U, 0U, scene.env.utc + 1000U);
    auto receipt = boot.change(first);
    CHECK(receipt.taskId == 1U && receipt.tableRevision == 1U && !receipt.replayed);
    const unsigned writes = scene.flash.writes;
    receipt = boot.change(first);
    CHECK(receipt.replayed && receipt.taskId == 1U && scene.flash.writes == writes);
    auto conflict = first;
    conflict.definition.value[0] = 0xF4U;
    CHECK(boot.engine.mutateVerified(testAuthority(), conflict, receipt).code() == ErrorCode::SequenceConflict);
    CHECK(receipt.taskId == 0U && scene.flash.writes == writes);
    CHECK(boot.engine.mutateVerified(TaskAuthorityBinding(), first, receipt).code() == ErrorCode::AuthenticationRequired);
    CHECK(boot.engine.mutateVerified(otherAuthority(), first, receipt).code() == ErrorCode::AuthenticationRequired);
    CHECK(boot.engine.state() == TaskEngineState::Ready);
    auto second = create(2U, 1U, scene.env.utc + 2000U);
    CHECK(boot.change(second).taskId == 2U);
    CHECK(boot.engine.mutateVerified(testAuthority(), first, receipt).code() == ErrorCode::SequenceConflict);
    auto disable = command(TaskMutationKind::SetEnabled, 3U, 2U, 1U);
    boot.change(disable);
    CHECK(!boot.at().enabled && boot.at().definitionRevision == 1U);
    auto replace = create(4U, 3U, scene.env.utc + 3000U);
    replace.kind = TaskMutationKind::Replace;
    replace.taskId = 1U;
    replace.definition.value[0] = 0xF4U;
    boot.change(replace);
    CHECK(boot.at().definitionRevision == 2U && boot.at().value[0] == 0xF4U);
    auto remove = command(TaskMutationKind::Remove, 5U, 4U, 1U);
    CHECK(boot.change(remove).taskId == 1U);
    CHECK(boot.engine.count() == 1U && boot.at().taskId == 2U);
    CHECK(boot.change(remove).replayed);
    boot.change(command(TaskMutationKind::Remove, 6U, 5U, 2U));
    CHECK(boot.engine.count() == 0U);
    CHECK(boot.change(create(7U, 6U, scene.env.utc + 4000U)).taskId == 3U); // No reuse after deletion.
}

void executeAndFailure() {
    for (bool fail : {false, true}) {
        Scene scene;
        Boot<> boot(scene);
        boot.initialize();
        boot.change(create(1U, 0U, scene.env.utc + 1000U));
        scene.target.failApply = fail;
        scene.env.advance(999U);
        TaskPollReport report;
        CHECK(boot.engine.poll(report) && report.taskId == 0U);
        const unsigned writes = scene.flash.writes;
        scene.env.advance(1U);
        CHECK(boot.engine.poll(report));
        CHECK(report.taskId == 1U && report.callbackInvoked);
        CHECK(report.progress == (fail ? TaskProgress::Failed : TaskProgress::Completed));
        CHECK(scene.target.calls == 1U && scene.flash.writes == writes + 2U);
        CHECK(boot.engine.tableRevision() == 1U);
        for (unsigned i = 0U; i < 5U; ++i) CHECK(boot.engine.poll(report) && report.taskId == 0U);
        CHECK(scene.target.calls == 1U && scene.flash.writes == writes + 2U);
    }
}

void countdownRecovery() {
    Scene scene;
    Boot<> first(scene);
    first.initialize();
    first.change(create(1U, 0U, 0U, 1000U));
    auto start = command(TaskMutationKind::Start, 2U, 1U, 1U);
    first.change(start);
    scene.env.advance(500U);
    CHECK(first.change(start).replayed);
    scene.env.advance(500U);
    TaskPollReport report;
    CHECK(first.engine.poll(report) && report.callbackInvoked && scene.target.calls == 1U);
    CHECK(first.change(start).replayed); // ACK again, never restart after completion.
    CHECK(first.engine.poll(report) && !report.callbackInvoked);
    auto restart = command(TaskMutationKind::Start, 3U, 2U, 1U);
    first.change(restart);
    scene.flash.reboot();
    scene.env.now = 0U;
    ++scene.env.revision;
    Boot<> recovered(scene);
    CHECK(recovered.engine.begin() && recovered.engine.state() == TaskEngineState::Recovering);
    CHECK(recovered.at().progress == TaskProgress::Interrupted);
    CHECK(recovered.change(restart).replayed); // Receipt survives normalization/boot.
    CHECK(recovered.engine.poll(report).code() == ErrorCode::WouldBlock);
    scene.env.advance(1U);
    CHECK(recovered.engine.poll(report) && recovered.engine.state() == TaskEngineState::Ready);
    CHECK(recovered.at().progress == TaskProgress::Interrupted && recovered.at().startSequence == 2U);
    scene.env.advance(2000U);
    CHECK(recovered.engine.poll(report) && scene.target.calls == 1U);
    recovered.change(command(TaskMutationKind::Start, 4U, 3U, 1U));
    CHECK(recovered.at().startSequence == 3U);
    scene.env.advance(1000U);
    CHECK(recovered.engine.poll(report) && scene.target.calls == 2U);
}

void enableDoesNotRestart() {
    Scene scene;
    Boot<> boot(scene);
    boot.initialize();
    boot.change(create(1U, 0U, 0U, 1000U));
    boot.change(command(TaskMutationKind::Start, 2U, 1U, 1U));
    auto enabled = command(TaskMutationKind::SetEnabled, 3U, 2U, 1U);
    enabled.enabled = true;
    scene.env.advance(500U);
    boot.change(enabled); // Same enabled value must preserve the live deadline.
    scene.env.advance(500U);
    TaskPollReport report;
    CHECK(boot.engine.poll(report) && report.callbackInvoked);
    boot.change(command(TaskMutationKind::Start, 4U, 3U, 1U));
    boot.change(command(TaskMutationKind::SetEnabled, 5U, 4U, 1U));
    CHECK(boot.at().progress == TaskProgress::Interrupted);
    enabled.requestId[0] = 6U;
    enabled.expectedRevision = 5U;
    boot.change(enabled);
    scene.env.advance(2000U);
    CHECK(boot.engine.poll(report) && scene.target.calls == 1U);
}

void managementPowerFailures() {
    for (auto mode : {Flash::Failure::Before, Flash::Failure::After, Flash::Failure::Dirty}) {
        Scene scene;
        Boot<> first(scene);
        first.initialize();
        auto request = create(1U, 0U, scene.env.utc + 10000U);
        scene.flash.failAt = scene.flash.writes + 1U;
        scene.flash.failure = mode;
        TaskMutationReceipt receipt;
        CHECK(!first.engine.mutateVerified(testAuthority(), request, receipt));
        CHECK(receipt.taskId == 0U && first.engine.state() == TaskEngineState::Fault);
        TaskSnapshotRecord hidden;
        CHECK(!first.engine.recordAt(0U, hidden));
        scene.flash.reboot();
        Boot<> recovered(scene);
        CHECK(recovered.engine.begin());
        scene.env.advance(10U);
        receipt = recovered.change(request);
        CHECK(receipt.taskId == 1U && recovered.engine.count() == 1U);
        CHECK(receipt.replayed == (mode == Flash::Failure::After));
    }
}

void executionPowerFailures() {
    for (unsigned phase : {1U, 2U}) {
        for (auto mode : {Flash::Failure::Before, Flash::Failure::After, Flash::Failure::Dirty}) {
            Scene scene;
            Boot<> first(scene);
            first.initialize();
            const uint64_t due = scene.env.utc + 1000U;
            first.change(create(1U, 0U, due));
            scene.flash.failAt = scene.flash.writes + phase;
            scene.flash.failure = mode;
            scene.env.advance(1000U);
            TaskPollReport report;
            CHECK(!first.engine.poll(report));
            CHECK(first.engine.state() == TaskEngineState::Fault);
            CHECK(scene.target.calls == (phase == 2U ? 1U : 0U));
            CHECK(!first.engine.poll(report));
            scene.flash.reboot();
            scene.env.now = 0U;
            ++scene.env.revision;
            Boot<> recovered(scene);
            CHECK(recovered.engine.begin());
            const TaskProgress expected = phase == 1U
                ? (mode == Flash::Failure::After ? TaskProgress::Claimed : TaskProgress::None)
                : (mode == Flash::Failure::After ? TaskProgress::Completed : TaskProgress::Claimed);
            CHECK(recovered.at().progress == expected);
            scene.env.advance(100U);
            CHECK(recovered.engine.poll(report) && !report.callbackInvoked);
            CHECK(scene.target.calls == (phase == 2U ? 1U : 0U));
        }
    }
}

void commitRechecks() {
    for (unsigned fault = 0U; fault < 6U; ++fault) {
        Scene scene;
        Boot<> boot(scene);
        boot.initialize();
        boot.change(create(1U, 0U, scene.env.utc + 1000U));
        scene.flash.hookAt = scene.flash.writes + 1U;
        switch (fault) {
        case 0: scene.flash.commitMillis = 1001U; break;
        case 1: scene.flash.hook = [](Flash& f) { f.env.utc += 5000U; ++f.env.revision; }; break;
        case 2: scene.flash.hook = [](Flash& f) { f.env.utc -= 5000U; ++f.env.revision; }; break;
        case 3: scene.flash.hook = [](Flash& f) { f.env.valid = false; }; break;
        case 4: scene.flash.hook = [](Flash& f) { f.env.authority = otherAuthority(); }; break;
        default: scene.flash.hook = [](Flash& f) { f.engine->retire(); }; break;
        }
        scene.env.advance(1000U);
        TaskPollReport report;
        const auto result = boot.engine.poll(report);
        CHECK(scene.target.calls == 0U && !report.callbackInvoked);
        if (fault < 4U) CHECK(result && report.progress == TaskProgress::Skipped);
        else CHECK(!result && boot.engine.state() == TaskEngineState::Retired);
    }
    Scene scene;
    TaskWritePolicy policy = fastPolicy();
    policy.maximumCommitMillis = 5U;
    Boot<> boot(scene, policy);
    boot.initialize();
    boot.change(create(1U, 0U, scene.env.utc + 1000U));
    scene.flash.commitMillis = 6U;
    scene.env.advance(1000U);
    TaskPollReport report;
    CHECK(!boot.engine.poll(report) && boot.engine.state() == TaskEngineState::Fault && scene.target.calls == 0U);
}

void scopeAndTarget() {
    Scene scene;
    Boot<> boot(scene);
    boot.initialize();
    boot.change(create(1U, 0U, scene.env.utc + 1000U));
    scene.target.denyCall = scene.target.validations + 2U; // Changed after durable Claim.
    scene.env.advance(1000U);
    TaskPollReport report;
    CHECK(boot.engine.poll(report) && report.progress == TaskProgress::Failed && scene.target.calls == 0U);
    auto replacement = create(2U, 1U, scene.env.utc + 1000U);
    replacement.kind = TaskMutationKind::Replace;
    replacement.taskId = 1U;
    boot.change(replacement);
    scene.target.retireInApply = true;
    scene.env.advance(1000U);
    CHECK(!boot.engine.poll(report) && report.callbackInvoked && report.progress == TaskProgress::Claimed);
    CHECK(boot.engine.state() == TaskEngineState::Retired && scene.target.calls == 1U);
    CHECK(!boot.engine.poll(report));
    scene.flash.reboot();
    scene.env.authority = otherAuthority();
    Boot<> wrongOwner(scene);
    const unsigned writes = scene.flash.writes;
    CHECK(!wrongOwner.engine.begin() && wrongOwner.engine.state() == TaskEngineState::Retired);
    CHECK(scene.flash.writes == writes); // Never auto-clear old scope.
    scene.env.authority = testAuthority();
    CHECK(!wrongOwner.engine.poll(report)); // Latch even if old scope reappears.
}

void budgetAndOrder() {
    Scene scene;
    auto policy = fastPolicy();
    policy.writesPerWindow = 2U;
    Boot<> boot(scene, policy);
    boot.initialize(); // One of two credits remains.
    boot.change(create(1U, 0U, scene.env.utc + 10U)); // Zero credits.
    scene.env.advance(10U);
    TaskPollReport report;
    const unsigned writes = scene.flash.writes;
    CHECK(boot.engine.poll(report).code() == ErrorCode::WouldBlock);
    scene.env.advance(40U);
    CHECK(boot.engine.poll(report).code() == ErrorCode::WouldBlock); // One credit is not enough.
    CHECK(scene.flash.writes == writes && scene.target.calls == 0U);
    scene.env.advance(50U);
    CHECK(boot.engine.poll(report) && report.callbackInvoked && scene.flash.writes == writes + 2U);

    Scene multi;
    Boot<> many(multi);
    many.initialize();
    const uint64_t due = multi.env.utc + 1000U;
    for (uint8_t i = 1U; i <= 4U; ++i) many.change(create(i, i - 1U, due));
    TaskMutationReceipt receipt;
    CHECK(many.engine.mutateVerified(testAuthority(), create(5U, 4U, due), receipt).code() == ErrorCode::CapacityExceeded);
    multi.target.denyId = 1U;
    multi.env.advance(1000U);
    CHECK(many.engine.poll(report) && report.taskId == 1U && report.progress == TaskProgress::Failed);
    for (uint32_t i = 2U; i <= 4U; ++i) {
        CHECK(many.engine.poll(report) && report.taskId == i && report.callbackInvoked);
        CHECK(multi.target.ids[i - 2U] == i);
    }
    CHECK(multi.target.calls == 3U);
}

void clockAbsenceAndWrap() {
    Scene scene;
    scene.env.now = 0xFFFFFFF0U;
    Boot<> boot(scene);
    boot.initialize(); // Ordinary tick wrap, not reboot.
    boot.change(create(1U, 0U, scene.env.utc + 1000U));
    boot.change(create(2U, 1U, 0U, 500U));
    boot.change(command(TaskMutationKind::Start, 3U, 2U, 2U));
    scene.env.valid = false;
    scene.env.advance(500U);
    TaskPollReport report;
    CHECK(boot.engine.poll(report) && report.taskId == 2U && report.callbackInvoked);
    scene.env.advance(500U);
    CHECK(boot.engine.poll(report) && report.taskId == 0U);
    scene.env.valid = true;
    ++scene.env.revision;
    CHECK(boot.engine.poll(report) && report.taskId == 0U); // No catch-up at recovered calendar deadline.
    --scene.env.now;
    CHECK(!boot.engine.poll(report) && boot.engine.state() == TaskEngineState::Fault);
}

void removalPreservesOtherDeadline() {
    Scene scene;
    Boot<> boot(scene);
    boot.initialize();
    boot.change(create(1U, 0U, scene.env.utc + 5000U));
    boot.change(create(2U, 1U, 0U, 1000U));
    boot.change(command(TaskMutationKind::Start, 3U, 2U, 2U));
    scene.env.advance(500U);
    boot.change(command(TaskMutationKind::Remove, 4U, 3U, 1U));
    CHECK(boot.at().taskId == 2U && boot.at().progress == TaskProgress::CountdownArmed);
    scene.env.advance(499U);
    TaskPollReport report;
    CHECK(boot.engine.poll(report) && !report.callbackInvoked);
    scene.env.advance(1U);
    CHECK(boot.engine.poll(report) && report.callbackInvoked && report.taskId == 2U);
}

void startCooldownIsGlobal() {
    Scene scene;
    auto policy = fastPolicy();
    policy.minimumStartMillis = 1000U;
    Boot<> boot(scene, policy);
    boot.initialize();
    boot.change(create(1U, 0U, 0U, 100U));
    auto start = command(TaskMutationKind::Start, 2U, 1U, 1U);
    TaskMutationReceipt receipt;
    CHECK(boot.engine.mutateVerified(testAuthority(), start, receipt).code() == ErrorCode::WouldBlock);
    scene.env.advance(900U);
    boot.change(start);
    CHECK(boot.change(start).replayed); // Duplicate bypasses cooldown, but never executes Start twice.
    boot.change(command(TaskMutationKind::Remove, 3U, 2U, 1U));
    boot.change(create(4U, 3U, 0U, 100U));
    start = command(TaskMutationKind::Start, 5U, 4U, 2U);
    CHECK(boot.engine.mutateVerified(testAuthority(), start, receipt).code() == ErrorCode::WouldBlock);
    scene.env.advance(999U);
    CHECK(boot.engine.mutateVerified(testAuthority(), start, receipt).code() == ErrorCode::WouldBlock);
    scene.env.advance(1U);
    boot.change(start);
    CHECK(boot.at().startSequence == 1U);
    scene.env.advance(100U);
    TaskPollReport report;
    CHECK(boot.engine.poll(report) && report.callbackInvoked);
    // Keep polling over more than a full uptime wrap since Start. An expired
    // cooldown stays expired, even when now-oldStart looks small again.
    for (unsigned i = 0U; i < 2U; ++i) {
        scene.env.advance(0x7FFFFFF0U);
        CHECK(boot.engine.poll(report) && !report.callbackInvoked);
    }
    boot.change(command(TaskMutationKind::Start, 6U, 5U, 2U));
    CHECK(boot.at().startSequence == 2U);
}

void weeklyRecovery() {
    Scene scene;
    scene.env.utc = base; // Midnight UTC; run daily at 00:01.
    Boot<> boot(scene);
    boot.initialize();
    auto request = create(1U, 0U, 0U);
    request.definition.timing.kind = TaskTimingKind::Weekly;
    request.definition.timing.minuteOfDay = 1U;
    request.definition.timing.weekdays = 0x7FU;
    boot.change(request);
    scene.env.advance(59900U);
    TaskPollReport report;
    CHECK(boot.engine.poll(report) && report.callbackInvoked);
    CHECK(boot.at().consumedUtcMillis == base + 60000U);
    scene.flash.reboot();
    scene.env.now = 0U;
    scene.env.utc -= 30000U; // Reboot plus backward correction cannot replay the consumed minute.
    ++scene.env.revision;
    Boot<> recovered(scene);
    CHECK(recovered.engine.begin());
    scene.env.advance(30000U);
    CHECK(recovered.engine.poll(report) && !report.callbackInvoked);
    scene.env.advance(86400000U);
    CHECK(recovered.engine.poll(report) && report.callbackInvoked && scene.target.calls == 2U);
    CHECK(recovered.at().consumedUtcMillis == base + 86460000U);
}

void exhaustedBudgetSkipsLate() {
    Scene scene;
    auto policy = fastPolicy();
    policy.windowMillis = 4000U;
    policy.writesPerWindow = 2U;
    Boot<> boot(scene, policy);
    CHECK(boot.engine.begin().code() == ErrorCode::NotFound);
    scene.env.advance(4000U);
    CHECK(boot.engine.initializeVerified(testAuthority()));
    boot.change(create(1U, 0U, scene.env.utc + 10U));
    scene.env.advance(10U);
    TaskPollReport report;
    CHECK(boot.engine.poll(report).code() == ErrorCode::WouldBlock);
    scene.env.advance(1990U); // Now late; one credit persists Skipped, never bypasses Claim.
    CHECK(boot.engine.poll(report) && report.progress == TaskProgress::Skipped && !report.callbackInvoked);
    CHECK(boot.at().progress == TaskProgress::Skipped && scene.flash.writes == 3U);
    scene.env.advance(10000U);
    CHECK(boot.engine.poll(report) && scene.target.calls == 0U && scene.flash.writes == 3U);
}

void managementScopeChangesDuringCommit() {
    for (bool retire : {false, true}) {
        Scene scene;
        Boot<> boot(scene);
        boot.initialize();
        scene.flash.hookAt = scene.flash.writes + 1U;
        scene.flash.hook = retire ? +[](Flash& f) { f.engine->retire(); }
                                 : +[](Flash& f) { f.env.authority = otherAuthority(); };
        TaskMutationReceipt receipt;
        CHECK(!boot.engine.mutateVerified(testAuthority(), create(1U, 0U, scene.env.utc + 100U), receipt));
        CHECK(receipt.tableRevision == 0U && boot.engine.state() == TaskEngineState::Retired);
        TaskSnapshotRecord hidden;
        CHECK(!boot.engine.recordAt(0U, hidden));
        TaskPollReport report;
        scene.env.advance(100U);
        CHECK(!boot.engine.poll(report) && scene.target.calls == 0U);
    }
}

void recoveryNormalizationFailure() {
    Scene scene;
    Boot<> first(scene);
    first.initialize();
    first.change(create(1U, 0U, 0U, 1000U));
    first.change(command(TaskMutationKind::Start, 2U, 1U, 1U));
    scene.flash.reboot();
    scene.env.now = 0U;
    Boot<> recovered(scene);
    CHECK(recovered.engine.begin());
    scene.flash.failure = Flash::Failure::Dirty;
    scene.flash.failAt = scene.flash.writes + 1U;
    scene.env.advance(1U);
    TaskPollReport report;
    CHECK(!recovered.engine.poll(report) && recovered.engine.state() == TaskEngineState::Fault);
    scene.env.advance(1000U);
    CHECK(!recovered.engine.poll(report) && scene.target.calls == 0U);
    scene.flash.reboot();
    Boot<> again(scene);
    CHECK(again.engine.begin() && again.at().progress == TaskProgress::Interrupted);
    scene.env.advance(1U);
    CHECK(again.engine.poll(report) && again.engine.state() == TaskEngineState::Ready);
}

void retirementMarkerDurability() {
    const uint8_t id[16] = {91U}, otherId[16] = {92U};
    for (size_t populated : {0U, 1U}) {
        Scene scene; Boot<> boot(scene); boot.initialize();
        if (populated) boot.change(create(1U, 0U, scene.env.utc + 100U));
        const unsigned writes = scene.flash.writes;
        CHECK(!boot.engine.retireVerified(otherAuthority(), ByteView(id, 16U)));
        CHECK(!boot.engine.retireVerified(testAuthority(), ByteView()));
        uint8_t zero[16] = {};
        CHECK(!boot.engine.retireVerified(testAuthority(), ByteView(zero, 16U)));
        CHECK(boot.engine.state() == TaskEngineState::Ready && scene.flash.writes == writes);
        scene.flash.hookAt = writes + 1U;
        scene.flash.hook = [](Flash& f) {
            CHECK(f.engine->state() == TaskEngineState::Retired);
            TaskPollReport report; TaskSnapshotHeader record;
            CHECK(f.engine->poll(report).code() == ErrorCode::WouldBlock);
            CHECK(f.engine->retirementRecord(record).code() == ErrorCode::WouldBlock);
            CHECK(f.engine->clearRetired().code() == ErrorCode::WouldBlock);
        };
        CHECK(boot.engine.retireVerified(testAuthority(), ByteView(id, 16U)));
        CHECK(scene.flash.writes == writes + 1U && scene.flash.clears == 0U && scene.flash.durableSize == 132U);
        TaskSnapshotHeader marker;
        CHECK(boot.engine.retirementRecord(marker) && marker.retired && marker.authority == testAuthority());
        CHECK(marker.requestId[0] == 91U && marker.tableRevision == populated);
        CHECK(boot.engine.retireVerified(testAuthority(), ByteView(id, 16U)));
        CHECK(!boot.engine.retireVerified(testAuthority(), ByteView(otherId, 16U)));
        CHECK(!boot.engine.initializeVerified(testAuthority()));
        CHECK(!boot.engine.rebindVerified(testAuthority(), testAuthority()));
        CHECK(scene.flash.writes == writes + 1U);
        // Stale roots/current clock cannot resurrect the terminal snapshot.
        scene.flash.reboot(); scene.env.now = 0U; scene.env.valid = false;
        Boot<> restarted(scene);
        CHECK(restarted.engine.begin().code() == ErrorCode::AuthenticationRequired);
        CHECK(restarted.engine.state() == TaskEngineState::Retired && restarted.engine.count() == 0U);
        CHECK(restarted.engine.retirementRecord(marker));
        TaskAuthorityBinding hidden; CHECK(!restarted.engine.authorityBinding(hidden));
        CHECK(restarted.engine.retireVerified(testAuthority(), ByteView(id, 16U)));
        scene.env.valid = true; scene.env.advance(1000U);
        TaskPollReport report; CHECK(!restarted.engine.poll(report) && scene.target.calls == 0U);
        CHECK(scene.flash.writes == writes + 1U);
        // Only separate, explicitly trusted physical cleanup may erase it.
        CHECK(restarted.engine.clearRetired());
        CHECK(!restarted.engine.retirementRecord(marker));
    }
    for (auto mode : {Flash::Failure::Before, Flash::Failure::After, Flash::Failure::Dirty}) {
        Scene scene; Boot<> first(scene); first.initialize();
        first.change(create(1U, 0U, scene.env.utc + 1000U));
        scene.flash.failure = mode; scene.flash.failAt = scene.flash.writes + 1U;
        const unsigned reads = scene.flash.loads, writes = scene.flash.writes;
        CHECK(!first.engine.retireVerified(testAuthority(), ByteView(id, 16U)));
        CHECK(first.store.failed() && first.engine.state() == TaskEngineState::Retired);
        TaskSnapshotHeader marker; TaskPollReport report;
        CHECK(!first.engine.retirementRecord(marker));
        CHECK(!first.engine.retireVerified(testAuthority(), ByteView(id, 16U)));
        CHECK(!first.engine.poll(report) && scene.target.calls == 0U);
        CHECK(scene.flash.loads == reads && scene.flash.writes == writes + 1U);
        scene.flash.reboot(); scene.env.now = 0U;
        Boot<> again(scene); const Result recovered = again.engine.begin();
        if (mode == Flash::Failure::After) {
            CHECK(!recovered && again.engine.retirementRecord(marker));
            CHECK(again.engine.retireVerified(testAuthority(), ByteView(id, 16U)));
        } else {
            CHECK(recovered && !again.engine.retirementRecord(marker));
            // A failed retirement is UNKNOWN, not a durable stop guarantee.
            CHECK(again.engine.count() == 1U);
            CHECK(again.engine.retireVerified(testAuthority(), ByteView(id, 16U)));
        }
        CHECK(scene.target.calls == 0U);
    }
    Scene exhausted;
    auto policy = fastPolicy(); policy.writesPerWindow = 2U;
    Boot<> full(exhausted, policy); full.initialize();
    full.change(create(1U, 0U, exhausted.env.utc + 1000U));
    CHECK(full.engine.retireVerified(testAuthority(), ByteView(id, 16U))); // Terminal write bypasses editing quota.
}

void retirementDurability() {
    {
        Scene unopened;
        Boot<> neverRead(unopened);
        neverRead.engine.retire();
        CHECK(neverRead.engine.clearRetired().code() == ErrorCode::NotConfigured);
        CHECK(unopened.flash.clears == 0U); // Never delete an unvalidated snapshot.
    }
    Scene scene;
    Boot<> boot(scene);
    CHECK(boot.engine.clearRetired().code() == ErrorCode::StateConflict);
    boot.initialize();
    boot.change(create(1U, 0U, scene.env.utc + 100U));
    CHECK(boot.engine.clearRetired().code() == ErrorCode::StateConflict && scene.flash.clears == 0U);
    boot.engine.retire();
    CHECK(scene.flash.clears == 0U); // retire is safe in callbacks, but RAM-only.
    CHECK(boot.engine.clearRetired() && scene.flash.clears == 1U);
    CHECK(boot.engine.state() == TaskEngineState::Cleared && boot.engine.count() == 0U);
    boot.engine.retire();
    CHECK(boot.engine.clearRetired() && scene.flash.clears == 1U);
    CHECK(!boot.engine.initializeVerified(testAuthority()));
    TaskMutationReceipt receipt;
    CHECK(!boot.engine.mutateVerified(testAuthority(), create(2U, 0U, scene.env.utc + 100U), receipt));
    TaskSnapshotRecord hidden;
    CHECK(!boot.engine.recordAt(0U, hidden));
    scene.env.advance(100U);
    TaskPollReport report;
    CHECK(!boot.engine.poll(report) && scene.target.calls == 0U);
    scene.flash.reboot(); scene.env.now = 0U;
    Boot<> recovered(scene);
    CHECK(recovered.engine.begin().code() == ErrorCode::NotFound);
    CHECK(recovered.engine.state() == TaskEngineState::Empty && recovered.engine.count() == 0U);
    CHECK(!recovered.engine.poll(report) && scene.target.calls == 0U);

    for (auto failure : {Flash::Failure::Before, Flash::Failure::After, Flash::Failure::Dirty}) {
        Scene failed;
        Boot<> first(failed);
        first.initialize();
        first.change(create(1U, 0U, failed.env.utc + 1000U));
        failed.flash.failure = failure;
        first.engine.retire();
        const unsigned loads = failed.flash.loads;
        CHECK(!first.engine.clearRetired());
        CHECK(first.engine.state() == TaskEngineState::Retired && first.store.failed());
        CHECK(!first.engine.clearRetired());
        CHECK(failed.flash.clears == 1U && failed.flash.loads == loads); // No retry/read-back of dirty handle.
        CHECK(!first.engine.poll(report) && failed.target.calls == 0U);
        failed.flash.reboot(); failed.env.now = 0U;
        Boot<> again(failed);
        const Result result = again.engine.begin();
        if (failure == Flash::Failure::After) CHECK(result.code() == ErrorCode::NotFound);
        else CHECK(result && again.engine.count() == 1U); // Old authority/table may survive FAILED removal.
        CHECK(failed.target.calls == 0U); // No assertion that failure committed a durable retirement.
    }
}

void authorityWaitingAndRecovery() {
    Scene scene; Boot<> first(scene); first.initialize();
    first.change(create(1U, 0U, 0U, 100U));
    first.change(command(TaskMutationKind::Start, 2U, 1U, 1U));
    scene.flash.reboot(); scene.env.now = 0U;
    scene.env.authorityState = TaskAuthorityState::Pending;
    Boot<> boot(scene);
    CHECK(boot.engine.begin().code() == ErrorCode::WouldBlock);
    CHECK(boot.engine.state() == TaskEngineState::AwaitingAuthority);
    const unsigned writes = scene.flash.writes;
    TaskSnapshotRecord hidden; TaskPollReport report; TaskMutationReceipt receipt;
    CHECK(!boot.engine.recordAt(0U, hidden));
    CHECK(!boot.engine.mutateVerified(testAuthority(), create(3U, 2U, scene.env.utc + 100U), receipt));
    scene.env.advance(1000U);
    CHECK(boot.engine.poll(report).code() == ErrorCode::WouldBlock);
    CHECK(scene.flash.writes == writes && scene.target.calls == 0U);
    scene.env.authorityState = TaskAuthorityState::Current;
    CHECK(boot.engine.poll(report));
    CHECK(boot.engine.state() == TaskEngineState::Ready);
    CHECK(boot.at().progress == TaskProgress::Interrupted);
    CHECK(scene.flash.writes == writes + 1U && scene.target.calls == 0U);
    // Missing proof is resumable. Explicit retirement is terminal.
    scene.env.authorityState = TaskAuthorityState::Pending;
    CHECK(!boot.engine.poll(report));
    scene.env.authorityState = TaskAuthorityState::Retired;
    CHECK(!boot.engine.poll(report) && boot.engine.state() == TaskEngineState::Retired);
    scene.env.authorityState = TaskAuthorityState::Current;
    CHECK(!boot.engine.poll(report));
    CHECK(!boot.engine.rebindVerified(testAuthority(), testAuthority()));

    Scene calendar; Boot<> online(calendar); online.initialize();
    online.change(create(1U, 0U, calendar.env.utc + 100U));
    calendar.env.authorityState = TaskAuthorityState::Pending;
    CHECK(!online.engine.poll(report));
    calendar.env.advance(100U); calendar.env.authorityState = TaskAuthorityState::Current;
    const unsigned before = calendar.flash.writes;
    CHECK(online.engine.poll(report)); // Due during the gap is not replayed, even within grace.
    CHECK(!report.callbackInvoked && calendar.flash.writes == before && calendar.target.calls == 0U);
}

void pendingDuringCommit() {
    for (TaskMutationKind kind : {TaskMutationKind::Create, TaskMutationKind::Replace,
         TaskMutationKind::Remove, TaskMutationKind::SetEnabled, TaskMutationKind::Start}) {
        Scene scene; Boot<> boot(scene); boot.initialize();
        boot.change(create(1U, 0U, 0U, 1000U));
        boot.change(create(2U, 1U, 0U, 1000U));
        auto request = command(kind, 3U, 2U, kind == TaskMutationKind::Create ? 0U : 1U);
        if (kind == TaskMutationKind::Create || kind == TaskMutationKind::Replace)
            request.definition = create(0U, 0U, 0U, 2000U).definition;
        scene.flash.hookAt = scene.flash.writes + 1U;
        scene.flash.hook = [](Flash& f) { f.env.authorityState = TaskAuthorityState::Pending; };
        TaskMutationReceipt receipt;
        CHECK(boot.engine.mutateVerified(testAuthority(), request, receipt).code() == ErrorCode::WouldBlock);
        CHECK(boot.engine.state() == TaskEngineState::AwaitingAuthority && receipt.tableRevision == 0U);
        const size_t count = kind == TaskMutationKind::Create ? 3U : (kind == TaskMutationKind::Remove ? 1U : 2U);
        CHECK(boot.engine.count() == count && boot.engine.tableRevision() == 3U);
        TaskSnapshotHeader durable; TaskSnapshotRecord tasks[4]; size_t loaded = 0U;
        CHECK(TaskSnapshotCodec::decode(ByteView(scene.flash.durable, scene.flash.durableSize), durable, tasks, 4U, loaded));
        CHECK(loaded == count && durable.tableRevision == 3U);
        TaskSnapshotRecord hidden;
        CHECK(!boot.engine.recordAt(0U, hidden));
        scene.env.advance(100U); scene.env.authorityState = TaskAuthorityState::Current;
        TaskPollReport report; CHECK(boot.engine.poll(report));
        CHECK(scene.target.calls == 0U);
        const unsigned writes = scene.flash.writes;
        CHECK(boot.engine.mutateVerified(testAuthority(), request, receipt) && receipt.replayed);
        CHECK(receipt.tableRevision == 3U && scene.flash.writes == writes);
        for (size_t i = 0U; i < count; ++i) {
            const auto task = boot.at(i);
            CHECK(task.taskId == tasks[i].taskId && task.enabled == tasks[i].enabled);
            CHECK(task.timing.durationMillis == tasks[i].timing.durationMillis);
            if (tasks[i].progress == TaskProgress::CountdownArmed) CHECK(task.progress == TaskProgress::Interrupted);
        }
    }
    // Initialization committed before proof became unavailable: never format again.
    Scene initial; Boot<> empty(initial); CHECK(empty.engine.begin().code() == ErrorCode::NotFound);
    initial.env.advance(100U); initial.flash.hookAt = 1U;
    initial.flash.hook = [](Flash& f) { f.env.authorityState = TaskAuthorityState::Pending; };
    CHECK(empty.engine.initializeVerified(testAuthority()).code() == ErrorCode::WouldBlock);
    CHECK(empty.engine.state() == TaskEngineState::AwaitingAuthority);
    initial.env.authorityState = TaskAuthorityState::Current;
    TaskPollReport report; CHECK(empty.engine.poll(report));
    CHECK(initial.flash.writes == 1U && empty.engine.count() == 0U);
    CHECK(empty.engine.initializeVerified(testAuthority()).code() == ErrorCode::StateConflict);

    Scene slow; Boot<> slowBoot(slow); slowBoot.initialize();
    slow.flash.commitMillis = fastPolicy().maximumCommitMillis + 1U;
    slow.flash.hookAt = slow.flash.writes + 1U;
    slow.flash.hook = [](Flash& f) { f.env.authorityState = TaskAuthorityState::Pending; };
    TaskMutationReceipt receipt;
    CHECK(slowBoot.engine.mutateVerified(testAuthority(), create(1U, 0U, slow.env.utc + 10000U), receipt).code()
          == ErrorCode::StateConflict);
    CHECK(slowBoot.engine.state() == TaskEngineState::Fault); // Pending cannot hide an over-budget write.
    slow.env.authorityState = TaskAuthorityState::Current;
    CHECK(!slowBoot.engine.poll(report) && slow.target.calls == 0U);
}

void authorityGapAroundCallback() {
    for (unsigned window = 0U; window < 3U; ++window) {
        Scene scene; Boot<> boot(scene); boot.initialize();
        boot.change(create(1U, 0U, scene.env.utc + 100U));
        scene.env.advance(100U);
        if (window == 1U) scene.target.pendingInApply = true;
        else {
            scene.flash.hookAt = scene.flash.writes + (window == 0U ? 1U : 2U);
            scene.flash.hook = [](Flash& f) { f.env.authorityState = TaskAuthorityState::Pending; };
        }
        TaskPollReport report; CHECK(!boot.engine.poll(report));
        CHECK(boot.engine.state() == TaskEngineState::AwaitingAuthority);
        const unsigned callbacks = window == 0U ? 0U : 1U;
        CHECK(scene.target.calls == callbacks);
        const unsigned writes = scene.flash.writes;
        scene.env.advance(100U); scene.env.authorityState = TaskAuthorityState::Current;
        CHECK(boot.engine.poll(report) && !report.callbackInvoked);
        CHECK(scene.flash.writes == writes && scene.target.calls == callbacks);
        CHECK(boot.at().progress == (window == 2U ? TaskProgress::Completed : TaskProgress::Claimed));
    }
}

void rootContinuity() {
    Scene scene; Boot<> boot(scene); boot.initialize();
    const auto original = create(1U, 0U, scene.env.utc + 10000U);
    boot.change(original);
    const TaskAuthorityBinding before = testAuthority();
    TaskAuthorityBinding after = before; after.rootDigest[31] ^= 1U;
    scene.env.authority = after;
    TaskPollReport report; TaskMutationReceipt receipt; TaskSnapshotRecord hidden;
    const unsigned writes = scene.flash.writes;
    CHECK(boot.engine.poll(report).code() == ErrorCode::AuthenticationRequired);
    CHECK(boot.engine.state() == TaskEngineState::AwaitingAuthority && scene.flash.writes == writes);
    CHECK(!boot.engine.recordAt(0U, hidden));
    CHECK(!boot.engine.rebindVerified(otherAuthority(), after));
    TaskAuthorityBinding wrong = after; wrong.deviceInstanceId[15] ^= 1U;
    CHECK(!boot.engine.rebindVerified(before, wrong));
    wrong = after; wrong.rootDigest[0] ^= 1U;
    CHECK(!boot.engine.rebindVerified(before, wrong)); // Replacement must be the observed actual root.
    CHECK(scene.flash.writes == writes);
    CHECK(boot.engine.rebindVerified(before, after));
    CHECK(boot.engine.tableRevision() == 1U && boot.engine.count() == 1U);
    CHECK(scene.flash.writes == writes + 1U);
    CHECK(boot.engine.rebindVerified(before, after) && scene.flash.writes == writes + 1U);
    CHECK(!boot.engine.mutateVerified(before, original, receipt));
    CHECK(boot.engine.mutateVerified(after, original, receipt) && receipt.replayed);
    CHECK(scene.flash.writes == writes + 1U); // BTM2 uses scope, not the rotated root.
    scene.flash.reboot(); scene.env.now = 0U;
    Boot<> reboot(scene); CHECK(reboot.engine.begin());
    CHECK(reboot.at().taskId == 1U && scene.target.calls == 0U);
    scene.env.authority.taskScope[15] ^= 1U;
    CHECK(!reboot.engine.poll(report) && reboot.engine.state() == TaskEngineState::Retired);
    scene.env.authority = after;
    CHECK(!reboot.engine.poll(report)); // A proven new owner cannot restore the retired instance.
}

void rootRebindPreservesStartReplay() {
    for (bool pendingAfterCommit : {false, true}) {
        Scene scene; Boot<> boot(scene); boot.initialize();
        boot.change(create(1U, 0U, 0U, 1000U));
        const auto start = command(TaskMutationKind::Start, 2U, 1U, 1U);
        boot.change(start);
        auto replacement = testAuthority(); replacement.rootDigest[31] ^= 1U;
        scene.env.authority = replacement;
        if (pendingAfterCommit) {
            scene.flash.hookAt = scene.flash.writes + 1U;
            scene.flash.hook = [](Flash& f) { f.env.authorityState = TaskAuthorityState::Pending; };
        }
        const unsigned before = scene.flash.writes;
        const Result rebound = boot.engine.rebindVerified(testAuthority(), replacement);
        CHECK(rebound.code() == (pendingAfterCommit ? ErrorCode::WouldBlock : ErrorCode::Ok));
        CHECK(scene.flash.writes == before + 1U && boot.engine.tableRevision() == 2U);
        scene.env.authorityState = TaskAuthorityState::Current;
        CHECK(boot.engine.rebindVerified(testAuthority(), replacement)); // Exact durable CAS retry.
        CHECK(scene.flash.writes == before + 1U);
        TaskPollReport report; CHECK(boot.engine.poll(report));
        CHECK(boot.engine.state() == TaskEngineState::Ready && !report.callbackInvoked);
        CHECK(boot.at().progress == TaskProgress::Interrupted && boot.at().startSequence == 1U);
        const unsigned writes = scene.flash.writes;
        TaskMutationReceipt receipt;
        CHECK(boot.engine.mutateVerified(replacement, start, receipt) && receipt.replayed);
        CHECK(receipt.tableRevision == 2U && receipt.taskId == 1U && scene.flash.writes == writes);
        scene.env.advance(2000U); CHECK(boot.engine.poll(report) && !report.callbackInvoked);
        scene.flash.reboot(); scene.env.now = 0U;
        Boot<> reboot(scene); CHECK(reboot.engine.begin());
        CHECK(reboot.engine.mutateVerified(replacement, start, receipt) && receipt.replayed);
        scene.env.advance(2000U); CHECK(reboot.engine.poll(report));
        CHECK(scene.target.calls == 0U && scene.flash.writes == writes);
    }
}

void rootRebindStorageFailures() {
    for (auto failure : {Flash::Failure::Before, Flash::Failure::After, Flash::Failure::Dirty}) {
        Scene scene; Boot<> boot(scene); boot.initialize();
        boot.change(create(1U, 0U, scene.env.utc + 10000U));
        auto after = testAuthority(); after.rootDigest[31] ^= 1U;
        scene.env.authority = after;
        scene.flash.failAt = scene.flash.writes + 1U; scene.flash.failure = failure;
        CHECK(!boot.engine.rebindVerified(testAuthority(), after));
        CHECK(boot.engine.state() == TaskEngineState::Fault && boot.store.failed());
        const unsigned writes = scene.flash.writes;
        CHECK(!boot.engine.rebindVerified(testAuthority(), after) && scene.flash.writes == writes);
        scene.flash.reboot(); scene.env.now = 0U;
        Boot<> reboot(scene);
        const Result loaded = reboot.engine.begin();
        CHECK(scene.target.calls == 0U && scene.flash.writes == writes);
        if (failure == Flash::Failure::After) CHECK(loaded && reboot.engine.state() == TaskEngineState::Ready);
        else CHECK(!loaded && reboot.engine.state() == TaskEngineState::AwaitingAuthority);
    }
}

void limitsAndValidation() {
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        Scene scene;
        TaskSnapshotHeader header;
        header.authority = testAuthority();
        header.tableRevision = mode == 0U ? UINT32_MAX : 1U;
        header.nextTaskId = mode == 1U ? UINT32_MAX : 2U;
        header.requestId[0] = 99U;
        header.requestDigest[0] = 1U;
        auto task = create(1U, 0U, 0U, 1000U).definition;
        task.taskId = task.definitionRevision = 1U;
        task.progress = TaskProgress::Interrupted;
        task.startSequence = mode == 2U ? UINT32_MAX : 1U;
        CHECK(TaskSnapshotCodec::encode(header, &task, 1U,
              MutableByteSpan(scene.flash.durable, maxBlob), scene.flash.durableSize));
        scene.flash.reboot();
        Boot<> boot(scene);
        CHECK(boot.engine.begin());
        scene.env.advance(100U);
        auto request = mode == 2U ? command(TaskMutationKind::Start, 1U, header.tableRevision, 1U)
                                 : create(1U, header.tableRevision, scene.env.utc + 1000U);
        TaskMutationReceipt receipt;
        CHECK(boot.engine.mutateVerified(testAuthority(), request, receipt).code() == ErrorCode::SequenceConflict);
        CHECK(scene.flash.writes == 0U);
    }
    Scene scene;
    Boot<> boot(scene);
    boot.initialize();
    for (unsigned mode = 0U; mode < 5U; ++mode) {
        auto invalid = create(1U, 0U, scene.env.utc + 1000U);
        switch (mode) {
        case 0: invalid.requestId[0] = 0U; break;
        case 1: invalid.definition.taskId = 1U; break;
        case 2: invalid.definition.keyLength = 65U; break;
        case 3: invalid.definition.value[0] = 0U; break;
        default: invalid.enabled = true; break;
        }
        TaskMutationReceipt receipt;
        CHECK(!boot.engine.mutateVerified(testAuthority(), invalid, receipt));
        CHECK(scene.flash.writes == 1U && boot.engine.count() == 0U && boot.engine.state() == TaskEngineState::Ready);
    }
    Scene bad;
    Boot<> badPolicy(bad, TaskWritePolicy());
    CHECK(!badPolicy.engine.begin() && bad.flash.writes == 0U);
}

// Same explicit policy as the real internal T2 sketch, not an invented fast
// test policy. Measured Flash latency is simulated here; this is not hardware.
void closedPilotPolicy() {
    TaskWritePolicy policy;
    policy.windowMillis = BLINKER_TASK_WINDOW_MS;
    policy.writesPerWindow = BLINKER_TASK_WRITES;
    policy.minimumStartMillis = BLINKER_TASK_MIN_START_MS;
    policy.maximumCommitMillis = BLINKER_TASK_MAX_COMMIT_MS;
    Scene scene; Boot<> boot(scene, policy);
    CHECK(boot.engine.begin().code() == ErrorCode::NotFound);
    scene.env.advance(5999U);
    CHECK(boot.engine.initializeVerified(testAuthority()).code() == ErrorCode::WouldBlock);
    scene.env.advance(1U); CHECK(boot.engine.initializeVerified(testAuthority()));
    scene.env.advance(6000U); scene.flash.commitMillis = 47U;
    boot.change(create(1U, 0U, 0U, 1000U));
    const auto start = command(TaskMutationKind::Start, 2U, 1U, 1U);
    TaskMutationReceipt receipt;
    CHECK(boot.engine.mutateVerified(testAuthority(), start, receipt).code() == ErrorCode::WouldBlock);
    scene.env.advance(30000U - scene.env.now);
    boot.change(start); scene.env.advance(1000U);
    TaskPollReport report;
    CHECK(boot.engine.poll(report) && report.callbackInvoked && scene.target.calls == 1U);
    CHECK(scene.flash.writes == 5U); // Initialize, Create, Start, Claim, completion.
    CHECK(boot.engine.mutateVerified(testAuthority(), start, receipt) && receipt.replayed);
    CHECK(scene.flash.writes == 5U);
    const auto next = command(TaskMutationKind::Start, 3U, 2U, 1U);
    CHECK(boot.engine.mutateVerified(testAuthority(), next, receipt).code() == ErrorCode::WouldBlock);
    scene.flash.reboot(); scene.env.now = 0U;
    Boot<> reboot(scene, policy); CHECK(reboot.engine.begin());
    CHECK(reboot.engine.mutateVerified(testAuthority(), next, receipt).code() == ErrorCode::WouldBlock);
    CHECK(scene.flash.writes == 5U); // Reboot does not refill credits/cooldown.

    Scene burst; Boot<> four(burst, policy);
    CHECK(four.engine.begin().code() == ErrorCode::NotFound);
    burst.env.advance(60000U); CHECK(four.engine.initializeVerified(testAuthority()));
    burst.flash.commitMillis = 47U;
    const uint64_t due = burst.env.utc + 120000U;
    for (uint8_t id = 1U; id <= 4U; ++id) four.change(create(id, id - 1U, due));
    burst.env.advance(static_cast<uint32_t>(due - burst.env.utc)); // Refill before this admitted burst.
    for (unsigned i = 0U; i < 4U; ++i) CHECK(four.engine.poll(report) && report.callbackInvoked);
    CHECK(burst.target.calls == 4U && burst.flash.writes == 13U);

    Scene slow; Boot<> overBudget(slow, policy);
    CHECK(overBudget.engine.begin().code() == ErrorCode::NotFound);
    slow.env.advance(12000U); CHECK(overBudget.engine.initializeVerified(testAuthority()));
    slow.flash.commitMillis = 101U;
    CHECK(overBudget.engine.mutateVerified(testAuthority(), create(1U, 0U, slow.env.utc + 1000U), receipt)
          .code() == ErrorCode::StateConflict);
    CHECK(overBudget.engine.state() == TaskEngineState::Fault && slow.target.calls == 0U && slow.flash.writes == 2U);
    slow.env.advance(2000U); CHECK(!overBudget.engine.poll(report) && slow.target.calls == 0U);
}

template <size_t Capacity>
void capacityResources() {
    Scene scene;
    Boot<Capacity> boot(scene);
    boot.initialize();
    for (uint8_t i = 1U; i <= Capacity; ++i)
        boot.change(create(i, i - 1U, scene.env.utc + 1000U));
    CHECK(boot.engine.count() == Capacity);
    scene.env.advance(1000U);
    TaskPollReport report;
    for (size_t i = 0U; i < Capacity; ++i) CHECK(boot.engine.poll(report) && report.callbackInvoked);
    CHECK(scene.target.calls == Capacity);
    const size_t ram = sizeof(TaskEngine) + sizeof(TaskSnapshotStore) +
        sizeof(boot.records) + sizeof(boot.slots) + sizeof(boot.workspace);
    std::cout << Capacity << " tasks: engine/store/tables/scratch=" << ram << " B\n";
}
} // namespace

int main() {
    managementAndReplay(); executeAndFailure(); countdownRecovery(); enableDoesNotRestart();
    managementPowerFailures(); executionPowerFailures(); commitRechecks(); scopeAndTarget();
    budgetAndOrder(); clockAbsenceAndWrap(); limitsAndValidation();
    removalPreservesOtherDeadline(); startCooldownIsGlobal(); weeklyRecovery(); exhaustedBudgetSkipsLate();
    managementScopeChangesDuringCommit(); recoveryNormalizationFailure(); retirementDurability();
    retirementMarkerDurability();
    authorityWaitingAndRecovery(); pendingDuringCommit(); authorityGapAroundCallback();
    rootContinuity(); rootRebindPreservesStartReplay(); rootRebindStorageFailures();
    closedPilotPolicy();
    capacityResources<4U>(); capacityResources<8U>(); capacityResources<10U>();
    std::cout << "task engine behavior/authority groups + 3 capacities passed; engine=" << sizeof(TaskEngine)
              << ", slot=" << sizeof(TaskRuntimeSlot) << '\n';
}
