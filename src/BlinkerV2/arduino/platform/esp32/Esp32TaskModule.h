#ifndef BLINKER_ARDUINO_PLATFORM_ESP32_TASKMODULE_H
#define BLINKER_ARDUINO_PLATFORM_ESP32_TASKMODULE_H

#include "../../ports/esp32/Esp32Storage.h"
#include "../../../runtime/TaskEngine.h"

namespace blinker {
namespace integration {
namespace official_detail {

// Internal optional composition, not automatically enabled by begin(). It
// opens only its own task namespace and never initializes or erases NVS itself.
// Scope/time and property execution stay injected, independent of WiFi/BLE.
template <size_t Capacity>
class Esp32TaskModule {
public:
    Esp32TaskModule(ITaskEnvironment& environment, ITaskTarget& target,
                    const TaskWritePolicy& policy,
                    const Esp32NvsBlobStoreConfig& storage = Esp32NvsBlobStoreConfig())
        : nvs_(blobConfig(storage)), workspace_(), records_(), slots_(),
          store_(nvs_, MutableByteSpan(workspace_, sizeof(workspace_))),
          engine_(store_, environment, target, records_, slots_, Capacity, policy), begun_(false) {
        static_assert(Capacity > 0U && Capacity <= TaskSnapshotCodec::maxTasks, "Invalid task capacity");
    }
    Result begin() {
        if (begun_) return Result::failure(ErrorCode::AlreadyExists);
        begun_ = true; // No retry against a potentially dirty persistence handle.
        Result result = nvs_.begin();
        return result ? engine_.begin() : result;
    }
    TaskEngine& engine() { return engine_; }

private:
    static Esp32NvsBlobStoreConfig blobConfig(const Esp32NvsBlobStoreConfig& input) {
        Esp32NvsBlobStoreConfig config = input;
        config.namespaceName = "bl_v2_tasks";
        config.key = "snapshot";
        config.maximumSize = TaskSnapshotCodec::encodedSize(Capacity);
        return config; // Preserve optional partition/encryption policy only.
    }
    Esp32NvsAtomicBlobStore nvs_;
    uint8_t workspace_[TaskSnapshotCodec::headerSize + Capacity * TaskSnapshotCodec::recordSize +
                       TaskSnapshotCodec::trailerSize];
    TaskSnapshotRecord records_[Capacity];
    TaskRuntimeSlot slots_[Capacity];
    TaskSnapshotStore store_;
    TaskEngine engine_;
    bool begun_;
    Esp32TaskModule(const Esp32TaskModule&) = delete;
    Esp32TaskModule& operator=(const Esp32TaskModule&) = delete;
};
} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
