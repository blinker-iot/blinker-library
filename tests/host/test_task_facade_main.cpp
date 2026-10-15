#include "BlinkerWiFi.h"
#include "BlinkerV2/api/TaskPropertyTarget.h"
#include <cstdlib>
#include <iostream>

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
TaskPropertyTarget target(nullptr, 0U);
unsigned configurations = 0U, starts = 0U, timeAttaches = 0U, writes = 0U;
struct Tick : IClock {
    uint32_t now = 100U;
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t& seconds) const override { seconds = 0U; return false; }
} tick;
struct Random : IRandom {
    uint8_t next = 1U;
    Result fill(MutableByteSpan output) override {
        for (size_t i = 0U; i < output.size; ++i) output.data[i] = next;
        ++next;
        return Result::success();
    }
} randomSource;
ITimeSync* businessTime = nullptr;
struct Lifecycle : IProductLifecycle {
    Result attach(Client& client) override { return target.attach(client); }
    Result attachTime(Client&, ITimeSync& time) override {
        ++timeAttaches; businessTime = &time;
        return time.configure(tick, randomSource);
    }
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

void checkTimeObservation(BlinkerClass& facade) {
    WallClockSnapshot snapshot;
    CHECK(!facade.timeStatus(snapshot) && snapshot.state == WallClockState::Unsynchronized);
    TimeSyncPeer peer;
    peer.authenticated = true;
    peer.transportId = 1U; peer.sessionId = 1U;
    peer.permissions = kAuthorizationPermissionControl;
    peer.features = bbp2::FeatureTimeSync;
    peer.source = WallTimeSource::Server;
    const auto synchronize = [&]() {
        uint8_t request[64], response[64]; size_t requestSize = 0U, responseSize = 0U;
        CHECK(businessTime->request(peer, 7U, MutableByteSpan(request, sizeof(request)), requestSize));
        bbp2::TimeResponseView sample;
        CHECK(bbp2::decodeTimeRequest(ByteView(request, requestSize), sample.nonce));
        sample.utcAtReplyMillis = 1789260000000ULL; sample.uncertaintyMillis = 1000U;
        CHECK(bbp2::encodeTimeResponse(sample, MutableByteSpan(response, sizeof(response)), responseSize));
        bbp2::FrameView reply;
        reply.header.kind = static_cast<uint8_t>(bbp2::MessageKind::TimeResponse);
        reply.header.flags = bbp2::FlagIsResponse; reply.header.sequence = 7U;
        reply.body = ByteView(response, responseSize);
        CHECK(businessTime->receive(peer, reply));
    };
    synchronize();
    CHECK(facade.timeStatus(snapshot) && snapshot.state == WallClockState::Synchronized &&
          snapshot.source == WallTimeSource::Server && snapshot.ageMillis == 0U);
    uint64_t seconds = 0U;
    CHECK(facade.time(seconds) && seconds == snapshot.utcMillis / 1000U);
    tick.now += 1001U;
    CHECK(facade.timeStatus(snapshot) && snapshot.ageMillis == 1001U);
    tick.now += 3600000U;
    CHECK(!facade.timeStatus(snapshot) && snapshot.state == WallClockState::Stale &&
          snapshot.utcMillis == 0U && snapshot.source == WallTimeSource::None);
    synchronize();
    CHECK(facade.timeStatus(snapshot) && snapshot.state == WallClockState::Synchronized);
    businessTime->reset(true);
    CHECK(!facade.timeStatus(snapshot) && snapshot.state == WallClockState::Unsynchronized &&
          snapshot.utcMillis == 0U && facade.lastError() == ErrorCode::Ok);
}
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
    checkTimeObservation(facade);
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
