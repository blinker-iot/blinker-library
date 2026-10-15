#ifndef BLINKER_ARDUINO_PLATFORM_TASKMODULE_H
#define BLINKER_ARDUINO_PLATFORM_TASKMODULE_H

#include "../../runtime/TaskEngine.h"

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
          engine_(store_, environment, target, records_, slots_, Capacity, policy) {
        static_assert(Capacity > 0U && Capacity <= TaskSnapshotCodec::maxTasks, "Invalid task capacity");
    }
    // TaskEngine latches every initialization outcome (including load/policy
    // failure), so a second begin never reloads a potentially dirty store.
    Result begin() { return engine_.begin(); }
    TaskEngine& engine() { return engine_; }

private:
    uint8_t workspace_[TaskSnapshotCodec::headerSize + Capacity * TaskSnapshotCodec::recordSize +
                       TaskSnapshotCodec::trailerSize];
    TaskSnapshotRecord records_[Capacity];
    TaskRuntimeSlot slots_[Capacity];
    TaskSnapshotStore store_;
    TaskEngine engine_;
    TaskModule(const TaskModule&) = delete;
    TaskModule& operator=(const TaskModule&) = delete;
};

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
