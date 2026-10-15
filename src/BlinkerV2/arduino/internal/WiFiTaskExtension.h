#ifndef BLINKER_ARDUINO_WIFI_TASK_EXTENSION_H
#define BLINKER_ARDUINO_WIFI_TASK_EXTENSION_H

#include "../../api/TaskProductLifecycle.h"
#include "../../runtime/TaskTimeEnvironment.h"
#include "../../runtime/TaskCloudTransport.h"
#include "../config/TaskConfig.h"

namespace blinker {
namespace integration {
namespace official_detail {

// One optional task graph for all manual-WiFi products. The port supplies only
// its store-owning/borrowing module and trusted signature implementation.
// Identity, clock, Client, MQTT, authority and lifecycle remain the original ones.
template <typename Stack, typename Port>
class WifiTaskExtension {
public:
    WifiTaskExtension(Stack& stack, IProductLifecycle& wifi)
        : authority_(), environment_(&TaskAuthorityCache::read, &authority_),
          target_(nullptr, 0U),
          tasks_(environment_, target_, policy(), Port::storage(stack)), crypto_(),
          verifier_(crypto_, Port::keys(), Port::keyCount()), roots_(roots(stack)),
          session_(tasks_.engine(), authority_, stack.random(), verifier_, stack.clock(), 15000U, 250U),
          adapter_(session_, authority_, tasks_.engine(), stack.deviceInstance(), roots_), workspace_(),
          carrier_(stack.cloudTransport(), stack.mqttTransport(), stack.clock(), adapter_,
                   MutableByteSpan(workspace_, sizeof(workspace_))),
          lifecycle_(wifi, tasks_, target_, &adapter_) {}
    ~WifiTaskExtension() { lifecycle_.stop(); }

    template <typename... T>
    Result configure(Property<T>&... properties) { return configureTaskProperties(target_, properties...); }
    Result attach(Client&) { return Result::success(); }
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
    typedef typename Port::Module Module;
    TaskAuthorityCache authority_;
    TaskTimeEnvironment environment_;
    TaskPropertyTarget target_;
    Module tasks_;
    typename Port::Crypto crypto_;
    typename Port::Verifier verifier_;
    TaskRootSources roots_;
    TaskManagementSession session_;
    TaskCommandAdapter adapter_;
    uint8_t workspace_[kTaskCarrierFrameMaxSize];
    TaskCloudTransport<typename Stack::CloudTransport> carrier_;
    TaskProductLifecycle<Module> lifecycle_;
    WifiTaskExtension(const WifiTaskExtension&) = delete;
    WifiTaskExtension& operator=(const WifiTaskExtension&) = delete;
};

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
