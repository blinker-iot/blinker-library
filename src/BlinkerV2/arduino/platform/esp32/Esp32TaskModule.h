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
        : nvs_(blobConfig(storage)), tasks_(nvs_, environment, target, policy), begun_(false) {}
    Result begin() {
        if (begun_) return Result::failure(ErrorCode::AlreadyExists);
        begun_ = true; // No retry against a potentially dirty persistence handle.
        Result result = nvs_.begin();
        return result ? tasks_.begin() : result;
    }
    TaskEngine& engine() { return tasks_.engine(); }

private:
    static Esp32NvsBlobStoreConfig blobConfig(const Esp32NvsBlobStoreConfig& input) {
        Esp32NvsBlobStoreConfig config = input;
        config.namespaceName = "bl_v2_tasks";
        config.key = "snapshot";
        config.maximumSize = TaskSnapshotCodec::encodedSize(Capacity);
        return config; // Preserve optional partition/encryption policy only.
    }
    Esp32NvsAtomicBlobStore nvs_;
    TaskModule<Capacity> tasks_;
    bool begun_;
    Esp32TaskModule(const Esp32TaskModule&) = delete;
    Esp32TaskModule& operator=(const Esp32TaskModule&) = delete;
};
} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
