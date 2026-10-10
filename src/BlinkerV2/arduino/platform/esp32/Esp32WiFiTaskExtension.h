#ifndef BLINKER_ESP32_WIFI_TASK_EXTENSION_H
#define BLINKER_ESP32_WIFI_TASK_EXTENSION_H

#include "Esp32TaskModule.h"
#include "../../ports/esp32/Esp32Crypto.h"
#include "../../config/Esp32ServerKeys.h"
#include "../../../api/TaskProductLifecycle.h"
#include "../../../runtime/TaskTimeEnvironment.h"
#include "../../../runtime/TaskCloudTransport.h"
#include "../../config/TaskConfig.h"

namespace blinker {
namespace integration {
namespace official_detail {

// Closed manual-WiFi pilot: optional storage/authority/transport decoration,
// not a copy of the WiFi lifecycle or a second Client, clock or MQTT connection.
// Targets are an explicit property allowlist configured before begin.
template <typename Stack, size_t Capacity>
class Esp32WifiTaskExtension {
public:
    Esp32WifiTaskExtension(Stack& stack, IProductLifecycle& wifi)
        : authority_(), environment_(&TaskAuthorityCache::read, &authority_),
          target_(nullptr, 0U), tasks_(environment_, target_, policy()), crypto_(),
          verifier_(crypto_, official::serverSigningKeys, official::serverSigningKeysCount),
          roots_(roots(stack)), session_(tasks_.engine(), authority_, stack.random(), verifier_, stack.clock(), 15000U, 250U),
          adapter_(session_, authority_, tasks_.engine(), stack.deviceInstance(), roots_), workspace_(),
          carrier_(stack.cloudTransport(), stack.mqttTransport(), stack.clock(), adapter_, MutableByteSpan(workspace_, sizeof(workspace_))),
          lifecycle_(wifi, tasks_, target_, &adapter_) {}
    ~Esp32WifiTaskExtension() { lifecycle_.stop(); }

    template <typename... T>
    Result configure(Property<T>&... properties) { return configureTaskProperties(target_, properties...); }
    Result attach(Client&) { return Result::success(); } // TaskProductLifecycle attaches the same target.
    Result attachTime(ITimeSync& time) { return environment_.attach(time); }
    IFrameTransport& carrier(Stack&) { return carrier_; }
    IProductLifecycle& lifecycle(IProductLifecycle&) { return lifecycle_; }
    uint16_t capabilities() const { return 0U; }
    ErrorCode taskError() const { return lifecycle_.taskError(); }

private:
    static TaskRootSources roots(Stack& stack) {
        TaskRootSources value; value.deviceKey = &stack.deviceKeySource(); return value;
    }
    static TaskWritePolicy policy() {
        TaskWritePolicy value;
        value.windowMillis = BLINKER_TASK_WINDOW_MS;
        value.writesPerWindow = BLINKER_TASK_WRITES;
        value.minimumStartMillis = BLINKER_TASK_MIN_START_MS;
        value.maximumCommitMillis = BLINKER_TASK_MAX_COMMIT_MS;
        return value;
    }
    TaskAuthorityCache authority_;
    TaskTimeEnvironment environment_;
    TaskPropertyTarget target_;
    Esp32TaskModule<Capacity> tasks_;
    Esp32MbedTlsCryptoProvider crypto_;
    P256ServerKeyRingVerifier verifier_;
    TaskRootSources roots_;
    TaskManagementSession session_;
    TaskCommandAdapter adapter_;
    uint8_t workspace_[kTaskCarrierFrameMaxSize];
    TaskCloudTransport<typename Stack::CloudTransport> carrier_;
    TaskProductLifecycle<Esp32TaskModule<Capacity>> lifecycle_;
    Esp32WifiTaskExtension(const Esp32WifiTaskExtension&) = delete;
    Esp32WifiTaskExtension& operator=(const Esp32WifiTaskExtension&) = delete;
};

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
