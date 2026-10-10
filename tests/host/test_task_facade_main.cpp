#define BLINKER_PRODUCT_WIFI 1
#include "BlinkerV2/api/BlinkerFacade.h"
#include "BlinkerV2/api/TaskPropertyTarget.h"
#include <cstdlib>
#include <iostream>

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
TaskPropertyTarget target(nullptr, 0U);
unsigned configurations = 0U, starts = 0U, timeAttaches = 0U, writes = 0U;
struct Lifecycle : IProductLifecycle {
    Result attach(Client& client) override { return target.attach(client); }
    Result attachTime(Client&, ITimeSync&) override { ++timeAttaches; return Result::success(); }
    Result start() override {
        TaskTargetView view; size_t total = 0U;
        const Result result = target.describe(UINT32_MAX, view, total);
        if (result) ++starts;
        return result;
    }
    void poll(uint32_t) override {}
    void stop() override {}
    ProductLifecycleStatus status() const override { return ProductLifecycleStatus(); }
    ProductCapabilities capabilities() const override { return ProductCapabilities(); }
} testLifecycle;
BLINKER_PROPERTY(power, bool, readWrite());
BLINKER_PROPERTY(other, bool, readWrite());
BLINKER_PROPERTY(read, bool, readOnly());
BLINKER_PROPERTY(text, StringView, readWrite());
void onPower(bool v) { ++writes; CHECK(power.report(v)); }
}

namespace blinker { namespace integration {
template <> struct TaskSupport<WifiDeviceKeyProfile> {
    template <typename... T> static Result configure(Property<T>&... properties) {
        ++configurations;
        return configureTaskProperties(target, properties...);
    }
};
IProductLifecycle& lifecycle(const WifiDeviceKeyProfile&) { return testLifecycle; }
} }

int main() {
    BlinkerClass facade;
    // No opt-in implementation for BLE: no target storage or fallback route.
    CHECK(integration::TaskSupport<facade_detail::BleProfile>::configure(power).code() == ErrorCode::UnsupportedFeature);
    CHECK(!facade.enableTasks(read) && facade.lastError() == ErrorCode::UnsupportedFeature);
    CHECK(!facade.enableTasks(text) && facade.lastError() == ErrorCode::UnsupportedFeature);
    CHECK(!facade.enableTasks(power, power) && facade.lastError() == ErrorCode::AlreadyExists);
    CHECK(power.onWrite(onPower));
    // Same bool signature after a failed first attempt must use power, not read.
    CHECK(facade.enableTasks(power));
    CHECK(!facade.enableTasks(other) && facade.lastError() == ErrorCode::AlreadyExists);
    TaskPropertyTarget different(nullptr, 0U);
    CHECK(configureTaskProperties(different, other).code() == ErrorCode::AlreadyExists);
    CHECK(facade.begin("key", "ssid", "password", power, other));
    CHECK(starts == 1U && timeAttaches == 1U && writes == 0U);
    TaskTargetView view; size_t total = 0U;
    CHECK(target.describe(0U, view, total) && total == 1U && view.key.data == power.fieldAt(0)->key.data);
    const unsigned configured = configurations;
    CHECK(!facade.enableTasks(power) && facade.lastError() == ErrorCode::AlreadyExists);
    CHECK(configurations == configured);
    facade.end();
    CHECK(!facade.enableTasks(other)); // end is not a reconfiguration/authority reset.
    CHECK(facade.begin("key", "ssid", "password", power, other));
    CHECK(starts == 2U && timeAttaches == 2U && writes == 0U);
    facade.end();
#if UINTPTR_MAX <= UINT32_MAX
    static_assert(sizeof(BlinkerClass) == 12U, "Task opt-in grew every facade");
#endif
    std::cout << "task facade: typed selection, immutable bindings, time/lifecycle and disabled mode passed\n";
}
