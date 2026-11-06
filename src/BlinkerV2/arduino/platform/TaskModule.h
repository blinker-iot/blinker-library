#ifndef BLINKER_ARDUINO_PLATFORM_TASKMODULE_H
#define BLINKER_ARDUINO_PLATFORM_TASKMODULE_H

#include "../../runtime/TaskEngine.h"
#include "../../interface/IDeviceAccessStore.h"

namespace blinker {
namespace integration {
namespace official_detail {

// Internal optional composition over a store already opened by its platform
// owner. It owns only task tables/workspace, never storage initialization,
// handles, namespaces or transport. Scope/time and execution remain injected.
template <size_t Capacity>
class TaskModule {
public:
    TaskModule(IAtomicBlobStore& storage, ITaskEnvironment& environment,
               ITaskTarget& target, const TaskWritePolicy& policy)
        : workspace_(), records_(), slots_(),
          store_(storage, MutableByteSpan(workspace_, sizeof(workspace_))),
          engine_(store_, environment, target, records_, slots_, Capacity, policy), initialCloudKeyCommit_(false) {
        static_assert(Capacity > 0U && Capacity <= TaskSnapshotCodec::maxTasks, "Invalid task capacity");
    }
    // TaskEngine latches every initialization outcome (including load/policy
    // failure), so a second begin never reloads a potentially dirty store.
    Result begin() { return initialCloudKeyCommit_
        ? Result::failure(ErrorCode::WouldBlock) : engine_.begin(); }
    TaskEngine& engine() { return engine_; }
    bool initialCloudKeyCommitActive() const { return initialCloudKeyCommit_; }
    bool taskStorageFailed() const { return store_.failed(); }

    // Trusted product transaction only, before Engine owns these buffers.
    // Reuse the SAME Store/Codec and fixed records/workspace: no second handle,
    // initialization, formatting, scope selection or task-state migration.
    Result installInitialCloudKeyVerified(IDeviceAccessStore& access,
        const DeviceKey& candidate, uint32_t expectedEpoch,
        const ControllerCredential& expectedAdmin,
        IInitialCloudKeyCommitGuard* guard = nullptr) {
        if (initialCloudKeyCommit_) return Result::failure(ErrorCode::WouldBlock);
        if (engine_.state() != TaskEngineState::Uninitialized)
            return Result::failure(ErrorCode::NotConfigured);
        TaskStoreBindingObservation observation;
        TaskSnapshotHeader header;
        const Result result = store_.observeBinding(observation, header, records_, Capacity);
        if (!result) return result;
        if (observation.state != TaskStoreBindingState::Unbound)
            return Result::failure(ErrorCode::StateConflict);
        // Only this exact post-observation scope may arm the product observer.
        // A native callback during load must not borrow a first-key window.
        initialCloudKeyCommit_ = true;
        const Result committed = guard == nullptr
            ? access.installInitialCloudKeyVerified(candidate, expectedEpoch, expectedAdmin)
            : access.installInitialCloudKeyAuthorized(candidate, expectedEpoch, expectedAdmin, *guard);
        initialCloudKeyCommit_ = false;
        return committed;
    }

private:
    uint8_t workspace_[TaskSnapshotCodec::headerSize + Capacity * TaskSnapshotCodec::recordSize +
                       TaskSnapshotCodec::trailerSize];
    TaskSnapshotRecord records_[Capacity];
    TaskRuntimeSlot slots_[Capacity];
    TaskSnapshotStore store_;
    TaskEngine engine_;
    bool initialCloudKeyCommit_;
    TaskModule(const TaskModule&) = delete;
    TaskModule& operator=(const TaskModule&) = delete;
};

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
