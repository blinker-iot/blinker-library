#ifndef BLINKER_ARDUINO_PLATFORM_ESP32_TASKMODULE_H
#define BLINKER_ARDUINO_PLATFORM_ESP32_TASKMODULE_H

#include "../../ports/esp32/Esp32Storage.h"
#include "../TaskModule.h"

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
        : nvs_(blobConfig(storage)), tasks_(nvs_, environment, target, policy), begun_(false),
          storageBegun_(false), storageError_(ErrorCode::NotConfigured) {}
    Result begin() {
        if (tasks_.initialCloudKeyCommitActive()) return Result::failure(ErrorCode::WouldBlock);
        if (begun_) return Result::failure(ErrorCode::AlreadyExists);
        begun_ = true; // No retry against a potentially dirty persistence handle.
        const Result result = beginStorage();
        return result ? tasks_.begin() : result;
    }
    TaskEngine& engine() { return tasks_.engine(); }
    bool initialCloudKeyCommitActive() const { return tasks_.initialCloudKeyCommitActive(); }
    bool taskStorageFailed() const {
        return (storageBegun_ && storageError_ != ErrorCode::Ok) || tasks_.taskStorageFailed();
    }

    // Storage may be observed before Engine begins, but is opened only once.
    // A later normal begin() borrows this same handle, never a second view.
    Result installInitialCloudKeyVerified(IDeviceAccessStore& access,
        const DeviceKey& candidate, uint32_t expectedEpoch,
        const ControllerCredential& expectedAdmin,
        IInitialCloudKeyCommitGuard* guard = nullptr) {
        if (begun_ || tasks_.engine().state() != TaskEngineState::Uninitialized)
            return Result::failure(ErrorCode::NotConfigured);
        const Result result = beginStorage();
        return result ? tasks_.installInitialCloudKeyVerified(
            access, candidate, expectedEpoch, expectedAdmin, guard) : result;
    }

private:
    Result beginStorage() {
        if (storageBegun_) return storageError_ == ErrorCode::Ok
            ? Result::success() : Result::failure(ErrorCode::StateConflict);
        storageBegun_ = true; // Opening failure never retries this module.
        const Result result = nvs_.begin();
        storageError_ = result.code();
        return result;
    }
    static Esp32NvsBlobStoreConfig blobConfig(const Esp32NvsBlobStoreConfig& input) {
        Esp32NvsBlobStoreConfig config = input;
        config.namespaceName = "bl_v2_tasks";
        config.key = "snapshot";
        config.maximumSize = TaskSnapshotCodec::encodedSize(Capacity);
        return config; // Preserve optional partition/encryption policy only.
    }
    Esp32NvsAtomicBlobStore nvs_;
    TaskModule<Capacity> tasks_;
    bool begun_, storageBegun_;
    ErrorCode storageError_;
    Esp32TaskModule(const Esp32TaskModule&) = delete;
    Esp32TaskModule& operator=(const Esp32TaskModule&) = delete;
};
} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
