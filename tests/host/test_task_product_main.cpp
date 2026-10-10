#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "BlinkerV2/api/Product.h"
#include "BlinkerV2/api/TaskPropertyTarget.h"
#include "BlinkerV2/api/TaskProductLifecycle.h"
#include "BlinkerV2/identity/DeviceAccessStore.h"
#include "BlinkerV2/runtime/TaskTimeEnvironment.h"
#include "BlinkerV2/runtime/TimeSyncService.h"
#include "BlinkerV2/arduino/platform/esp32/Esp32TaskModule.h"

#include "support/TaskAuthorityFixture.h"

using namespace blinker;
using blinker::integration::official_detail::Esp32TaskModule;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
using Bytes = std::vector<uint8_t>;
struct Nvs {
    std::map<std::string, Bytes> durable, working;
    std::map<nvs_handle_t, std::string> handles;
    unsigned opens = 0U, sets = 0U, commits = 0U, erases = 0U;
    unsigned failCommit = 0U;
    bool commitThenFail = false;
    esp_err_t openError = ESP_OK;
    esp_err_t setError = ESP_OK;
    std::string lastNamespace, lastKey;
    void powerCycle() { working = durable; handles.clear(); failCommit = 0U; }
} nvs;

struct Tick : IClock {
    mutable uint32_t now = 0U, reads = 0U;
    uint32_t step = 0U;
    uint32_t monotonicMillis() const override { ++reads; const uint32_t value = now; now += step; return value; }
    bool unixTime(uint64_t& seconds) const override { seconds = 0U; return false; }
};
struct Random : IRandom {
    Result fill(MutableByteSpan bytes) override { memset(bytes.data, 3, bytes.size); return Result::success(); }
};
struct Lifecycle : IProductLifecycle {
    Client* client = nullptr;
    DeviceAccessStore* access = nullptr;
    TaskEngine* tasks = nullptr;
    TaskAuthorityState* retiredScope = nullptr;
    Product* resetDuringPoll = nullptr;
    ErrorCode nestedReset = ErrorCode::Ok;
    unsigned starts = 0U, polls = 0U, stops = 0U, accessResets = 0U, networkResets = 0U, timeAttaches = 0U;
    uint16_t flags = 0U;
    bool running = false, polling = false;
    Result attach(Client& value) override { client = &value; return Result::success(); }
    Result attachTime(Client&, ITimeSync&) override { ++timeAttaches; return Result::success(); }
    Result start() override { ++starts; running = true; return Result::success(); }
    void poll(uint32_t) override {
        CHECK(running && !polling); polling = true; ++polls;
        if (retiredScope != nullptr) *retiredScope = TaskAuthorityState::Retired;
        if (resetDuringPoll != nullptr) {
            Product* product = resetDuringPoll; resetDuringPoll = nullptr;
            nestedReset = product->resetAccess().code();
            CHECK(stops == 0U); // No transport teardown on its active callback stack.
        }
        polling = false;
    }
    void stop() override { CHECK(!polling); ++stops; running = false; }
    Result resetAccess() override {
        CHECK(!running && tasks != nullptr && tasks->state() == TaskEngineState::Cleared);
        CHECK(nvs.durable.count("nvs/bl_v2_tasks/snapshot") == 0U);
        ++accessResets;
        return access->eraseAccess(); // Real access-root implementation, fake SDK only.
    }
    Result resetNetwork() override { CHECK(!running); ++networkResets; return Result::success(); }
    ProductLifecycleStatus status() const override { return ProductLifecycleStatus(); }
    ProductCapabilities capabilities() const override { return ProductCapabilities(flags); }
};
struct Fixture;
Fixture* active = nullptr;
struct Fixture {
    Tick tick;
    Random random;
    TimeSyncService time;
    TaskAuthorityBinding authority = testAuthority();
    TaskAuthorityState authorityState = TaskAuthorityState::Current;
    TaskTimeEnvironment environment;
    PropertySpec<bool> powerSpec, readSpec;
    PropertySpec<int32_t> smallSpec;
    PropertySpec<double> ratioSpec;
    PropertySpec<StringView> textSpec;
    Property<bool> power, read;
    Property<int32_t> small;
    Property<double> ratio;
    Property<StringView> text;
    DeviceSchema<Property<bool>, Property<int32_t>, Property<double>, Property<bool>, Property<StringView>> schema;
    Device device;
    Lifecycle lifecycle;
    Product product;
    unsigned calls = 0U;
    bool reject = false, report = true, reenter = false;
    Product* resetFromWrite = nullptr;
    ErrorCode nestedReset = ErrorCode::Ok;
    TaskPropertyTarget* target = nullptr;
    TaskSnapshotRecord* nestedTask = nullptr;
    static void scope(void* context, TaskAuthorityObservation& output) {
        const Fixture& f = *static_cast<Fixture*>(context);
        output.binding = f.authority; output.state = f.authorityState;
    }
    static void powerWrite(bool value) {
        ++active->calls;
        if (active->reenter) CHECK(active->target->apply(*active->nestedTask).code() == ErrorCode::WouldBlock);
        if (active->resetFromWrite != nullptr) {
            const unsigned stops = active->lifecycle.stops;
            active->nestedReset = active->resetFromWrite->resetAccess().code();
            CHECK(active->lifecycle.stops == stops); // Stop is deferred until callback returns.
        }
        if (active->report) CHECK(active->power.report(value));
    }
    static Result smallWrite(int32_t value) {
        ++active->calls;
        return active->reject ? Result::failure(ErrorCode::WouldBlock) : active->small.report(value);
    }
    static void ratioWrite(double value) { ++active->calls; CHECK(active->ratio.report(value)); }
    explicit Fixture(bool begin = true) : environment(time, &scope, this),
        powerSpec("power", readWrite()), readSpec("read", readOnly()),
        smallSpec("small", readWrite()), ratioSpec("ratio", rangedProperty(PropertyMode::ReadWrite, 0.0, 2.0, 0.5)),
        textSpec("text", readWrite()), power(powerSpec), read(readSpec), small(smallSpec), ratio(ratioSpec), text(textSpec),
        schema(power, small, ratio, read, text), device(schema), lifecycle(), product(device, lifecycle) {
        active = this;
        CHECK(power.onWrite(&powerWrite)); CHECK(small.onWriteChecked(&smallWrite)); CHECK(ratio.onWrite(&ratioWrite));
        if (begin) CHECK(product.begin());
        CHECK(time.configure(tick, random));
    }
};
TaskSnapshotRecord value(const char* key, ValueType type, ByteView encoded) {
    TaskSnapshotRecord task;
    task.keyLength = static_cast<uint8_t>(strlen(key));
    memcpy(task.key, key, task.keyLength);
    task.valueLength = static_cast<uint8_t>(encoded.size);
    memcpy(task.value, encoded.data, encoded.size);
    task.valueType = type;
    return task;
}
TaskSnapshotRecord powerValue() { const uint8_t byte = 0xF5U; return value("power", ValueType::Boolean, ByteView(&byte, 1U)); }
TaskWritePolicy policy() {
    TaskWritePolicy result;
    result.windowMillis = 100U; result.writesPerWindow = 100U;
    result.minimumStartMillis = 10U; result.maximumCommitMillis = 100U;
    return result; // Fault-injection fixture only, NOT a measured production policy.
}
TaskMutation createPower() {
    TaskMutation request;
    request.requestId[0] = 1U; request.definition = powerValue();
    request.definition.enabled = true; request.definition.timing.durationMillis = 100U;
    return request;
}
void installCountdown(TaskEngine& engine) {
    TaskMutationReceipt receipt;
    CHECK(engine.mutateVerified(testAuthority(), createPower(), receipt));
    TaskMutation start;
    start.kind = TaskMutationKind::Start; start.taskId = 1U;
    start.requestId[0] = 2U; start.expectedRevision = 1U;
    CHECK(engine.mutateVerified(testAuthority(), start, receipt));
}

struct TaskProductFixture {
    Fixture f;
    TaskPropertyBinding bindings[1];
    TaskPropertyTarget target;
    Esp32NvsAtomicBlobStore root;
    DeviceAccessStore access;
    Esp32TaskModule<4> module;
    TaskProductLifecycle<Esp32TaskModule<4>> lifecycle;
    Product product;
    static Esp32NvsBlobStoreConfig rootConfig() {
        Esp32NvsBlobStoreConfig config;
        config.namespaceName = "bl_v2_access";
        config.maximumSize = DeviceAccessStore::serializedSize;
        return config;
    }
    explicit TaskProductFixture(bool begin = true)
        : f(false), bindings{TaskPropertyBinding(f.power)}, target(bindings, 1U),
          root(rootConfig()), access(root), module(f.environment, target, policy()),
          lifecycle(f.lifecycle, module, target), product(f.device, lifecycle) {
        CHECK(root.begin());
        f.lifecycle.flags = ProductCapabilityAccessReset | ProductCapabilityNetworkReset;
        f.lifecycle.access = &access; f.lifecycle.tasks = &module.engine();
        if (begin) CHECK(product.begin(&f.time));
    }
    void install() {
        f.tick.now += 100U; CHECK(module.engine().initializeVerified(testAuthority()));
        installCountdown(module.engine());
    }
};

void lifecycleRunAndNetwork() {
    nvs = Nvs(); TaskProductFixture t;
    CHECK(t.lifecycle.taskError() == ErrorCode::Ok && t.f.lifecycle.timeAttaches == 1U);
    CHECK(t.target.validate(powerValue()));
    const unsigned opens = nvs.opens;
    t.install();
    const unsigned commits = nvs.commits;
    t.product.end(); t.product.run(); CHECK(t.f.lifecycle.polls == 0U);
    CHECK(t.product.begin());
    CHECK(t.product.resetNetwork() && t.product.status().started);
    CHECK(nvs.opens == opens && nvs.commits == commits && nvs.erases == 0U);
    CHECK(t.module.engine().state() == TaskEngineState::Ready && t.f.lifecycle.networkResets == 1U);
    t.f.tick.now += 100U; t.product.run();
    CHECK(t.f.calls == 1U && t.f.lifecycle.polls == 1U);
    CHECK(nvs.commits == commits + 2U); // One Claim/result, no second scheduler.
    t.product.run(); CHECK(t.f.calls == 1U && t.f.lifecycle.polls == 2U);
    static_assert(sizeof(Product) <= 3U * sizeof(void*) + 4U, "Default product facade grew");
#if UINTPTR_MAX <= UINT32_MAX
    static_assert(sizeof(TaskProductLifecycle<Esp32TaskModule<4>>) <= 28U, "Optional task lifecycle grew");
#endif
    std::cout << "optional task lifecycle bytes=" << sizeof(t.lifecycle) << '\n';
}

void lifecycleResetTransactions() {
    // Clear before/after commit errors, followed by root before/after errors.
    for (unsigned mode = 0U; mode < 5U; ++mode) {
        nvs = Nvs();
        const Bytes original(32U, 0x5AU);
        nvs.durable["nvs/bl_v2_access/record"] = original;
        nvs.durable["nvs/bl_v2_net/profile"] = Bytes(20U, 0xA5U);
        nvs.working = nvs.durable;
        {
            TaskProductFixture t; t.install();
            if (mode != 0U) {
                nvs.failCommit = nvs.commits + (mode <= 2U ? 1U : 2U);
                nvs.commitThenFail = mode == 2U || mode == 4U;
            }
            CHECK(t.product.resetAccess().ok() == (mode == 0U));
            CHECK(t.product.status().started == (mode == 0U));
            CHECK(t.f.lifecycle.accessResets == (mode == 1U || mode == 2U ? 0U : 1U));
            if (mode == 0U) {
                // Product restarts after reset and may have provisioned a NEW
                // root. A second reset must erase it, not reuse an old receipt.
                nvs.working["nvs/bl_v2_access/record"] = nvs.durable["nvs/bl_v2_access/record"] = original;
                t.product.end();
                const unsigned taskErases = nvs.erases;
                CHECK(t.product.resetAccess() && t.f.lifecycle.accessResets == 2U);
                CHECK(nvs.erases == taskErases + 1U); // Root only; tasks already durably absent.
            }
            const unsigned commits = nvs.commits, erases = nvs.erases, starts = t.f.lifecycle.starts;
            CHECK(t.product.resetAccess().ok() == (mode == 0U));
            CHECK(nvs.commits == commits && nvs.erases == erases);
            if (mode != 0U) CHECK(!t.product.begin() && t.f.lifecycle.starts == starts);
            t.f.tick.now += 1000U; t.product.run(); CHECK(t.f.calls == 0U);
        }
        nvs.powerCycle();
        CHECK(nvs.durable.count("nvs/bl_v2_tasks/snapshot") == (mode == 1U ? 1U : 0U));
        CHECK(nvs.durable.count("nvs/bl_v2_access/record") == (mode == 0U || mode == 4U ? 0U : 1U));
        CHECK(nvs.durable["nvs/bl_v2_net/profile"] == Bytes(20U, 0xA5U));
    }
}

void lifecycleReentrantRetirement() {
    for (bool fromProperty : {false, true}) {
        nvs = Nvs(); TaskProductFixture t; t.install();
        if (fromProperty) t.f.resetFromWrite = &t.product;
        else t.f.lifecycle.resetDuringPoll = &t.product;
        const unsigned commits = nvs.commits;
        t.f.tick.now += 100U; t.product.run();
        CHECK((fromProperty ? t.f.nestedReset : t.f.lifecycle.nestedReset) == ErrorCode::WouldBlock);
        CHECK(!t.product.status().started && !t.f.lifecycle.running);
        CHECK(t.module.engine().state() == TaskEngineState::Retired);
        CHECK(t.f.calls == (fromProperty ? 1U : 0U));
        CHECK(nvs.commits == commits + (fromProperty ? 1U : 0U)); // Claim only, no stale result write.
        CHECK(nvs.erases == 0U && t.f.lifecycle.accessResets == 0U);
        CHECK(!t.product.begin()); // Cannot resume while retirement is pending.
        CHECK(t.product.resetAccess()); // Cooperative retry, after callback stack returned.
        CHECK(t.module.engine().state() == TaskEngineState::Cleared && t.f.lifecycle.accessResets == 1U);
        CHECK(t.product.begin()); t.product.run(); CHECK(t.f.calls == (fromProperty ? 1U : 0U));
    }
}

void lifecycleAdmissionFailures() {
    // Invalid Sketch selection must fail before opening the task namespace or
    // starting transport. This differs from a persistence failure after admission.
    for (bool missingCallback : {false, true}) {
        nvs = Nvs(); Fixture f(false);
        PropertySpec<bool> outsideSpec("outside", readWrite()); Property<bool> outside(outsideSpec);
        if (!missingCallback) CHECK(outside.onWrite(&Fixture::powerWrite));
        DeviceSchema<Property<bool>> outsideSchema(outside); Device outsideDevice(outsideSchema);
        const TaskPropertyBinding selected[] = {TaskPropertyBinding(outside)};
        TaskPropertyTarget target(selected, 1U);
        Esp32TaskModule<4> module(f.environment, target, policy());
        TaskProductLifecycle<Esp32TaskModule<4>> lifecycle(f.lifecycle, module, target);
        Product product(missingCallback ? outsideDevice : f.device, lifecycle);
        CHECK(!product.begin(&f.time));
        CHECK(nvs.opens == 0U && nvs.sets == 0U && f.lifecycle.starts == 0U && f.calls == 0U);
    }
    {
        nvs = Nvs(); TaskProductFixture t(false);
        CHECK(t.target.validate(powerValue()).code() == ErrorCode::NotConfigured);
        CHECK(t.product.resetAccess().code() == ErrorCode::NotConfigured && nvs.erases == 0U);
        CHECK(t.product.begin()); t.install();
        t.f.lifecycle.flags = 0U;
        CHECK(t.product.resetAccess().code() == ErrorCode::UnsupportedFeature);
        CHECK(t.product.status().started && t.f.lifecycle.stops == 0U);
        CHECK(t.module.engine().state() == TaskEngineState::Ready && nvs.erases == 0U);
        Fixture other;
        CHECK(t.target.attach(*other.lifecycle.client).code() == ErrorCode::StateConflict);
    }
    for (bool corrupt : {false, true}) {
        nvs = Nvs();
        if (corrupt) nvs.working["nvs/bl_v2_tasks/snapshot"] = nvs.durable["nvs/bl_v2_tasks/snapshot"] = Bytes(76U, 0xA5U);
        TaskProductFixture t(false);
        if (!corrupt) nvs.openError = ESP_ERR_NVS_NOT_ENOUGH_SPACE;
        CHECK(t.product.begin() && t.lifecycle.taskError() != ErrorCode::Ok);
        t.product.run(); CHECK(t.f.lifecycle.polls == 1U && t.f.calls == 0U);
        CHECK(!t.product.resetAccess() && t.f.lifecycle.accessResets == 0U && nvs.erases == 0U);
    }
    nvs = Nvs(); TaskProductFixture t(false); t.f.authorityState = TaskAuthorityState::Retired;
    CHECK(t.product.begin()); t.f.tick.now += 100U;
    CHECK(!t.module.engine().initializeVerified(testAuthority()));
    t.product.run(); CHECK(nvs.commits == 0U && t.f.calls == 0U);
}

void lifecycleScopeObservation() {
    nvs = Nvs(); TaskProductFixture t; t.install();
    t.f.lifecycle.retiredScope = &t.f.authorityState;
    const unsigned commits = nvs.commits;
    t.f.tick.now += 100U; t.product.run();
    CHECK(t.f.calls == 0U && nvs.commits == commits && nvs.erases == 0U);
    CHECK(t.module.engine().state() == TaskEngineState::Retired);
    CHECK(t.lifecycle.taskError() == ErrorCode::AuthenticationRequired);
    CHECK(t.product.status().started); // Retirement is not a network teardown.
    t.product.run(); CHECK(t.f.lifecycle.polls == 2U);
}

void lifecycleTerminalSnapshot() {
    nvs = Nvs();
    {
        TaskProductFixture t; t.install();
        const uint8_t id[16] = {91U};
        CHECK(t.module.engine().retireVerified(testAuthority(), ByteView(id, 16U)));
        const unsigned commits = nvs.commits;
        t.f.tick.now += 1000U; t.product.run();
        CHECK(t.product.status().started && t.f.lifecycle.polls == 1U && t.f.calls == 0U);
        CHECK(t.f.power.report(true)); // Ordinary property API remains usable.
        CHECK(nvs.commits == commits && nvs.erases == 0U);
        t.product.end();
    }
    nvs.powerCycle();
    TaskProductFixture reboot;
    CHECK(reboot.product.status().started && reboot.f.lifecycle.running);
    CHECK(reboot.module.engine().state() == TaskEngineState::Retired);
    CHECK(reboot.lifecycle.taskError() == ErrorCode::AuthenticationRequired);
    const unsigned commits = nvs.commits, opens = nvs.opens;
    reboot.product.run(); reboot.product.end(); CHECK(reboot.product.begin());
    reboot.f.tick.now += 1000U; reboot.product.run();
    CHECK(reboot.f.lifecycle.polls == 2U && reboot.f.calls == 0U);
    CHECK(reboot.module.engine().state() == TaskEngineState::Retired);
    CHECK(reboot.f.power.report(false));
    CHECK(nvs.commits == commits && nvs.opens == opens && nvs.erases == 0U);
}

void lifecycleAuthorityResume() {
    nvs = Nvs(); TaskProductFixture t; t.install();
    t.f.authorityState = TaskAuthorityState::Pending;
    const unsigned commits = nvs.commits;
    t.product.run();
    CHECK(t.module.engine().state() == TaskEngineState::AwaitingAuthority);
    CHECK(t.lifecycle.taskError() == ErrorCode::WouldBlock && t.product.status().started);
    CHECK(nvs.commits == commits && t.f.calls == 0U);
    t.f.tick.now += 1000U; t.f.authorityState = TaskAuthorityState::Current;
    t.product.run(); // Existing Product must continue polling a waiting engine.
    CHECK(t.module.engine().state() == TaskEngineState::Ready && t.f.calls == 0U);
    TaskSnapshotRecord task; CHECK(t.module.engine().recordAt(0U, task));
    CHECK(task.progress == TaskProgress::Interrupted && nvs.commits == commits + 1U);
    CHECK(t.lifecycle.taskError() == ErrorCode::Ok);
    t.f.authorityState = TaskAuthorityState::Pending;
    t.product.run(); CHECK(t.module.engine().state() == TaskEngineState::AwaitingAuthority);
    CHECK(t.product.resetAccess()); // A waiting engine is still explicitly erasable.
    CHECK(t.module.engine().state() == TaskEngineState::Cleared);
}

void propertyAdmissionAndCallbacks() {
    Fixture f;
    const TaskPropertyBinding bindings[] = { TaskPropertyBinding(f.power), TaskPropertyBinding(f.small),
        TaskPropertyBinding(f.ratio), TaskPropertyBinding(f.read), TaskPropertyBinding(f.text) };
    TaskPropertyTarget target(*f.lifecycle.client, bindings, 5U);
    TaskTargetView described; size_t targets = 0U;
    CHECK(!target.describe(UINT32_MAX, described, targets)); // Read-only/text are not task targets.
    TaskPropertyTarget catalog(*f.lifecycle.client, bindings, 3U);
    CHECK(catalog.describe(UINT32_MAX, described, targets) && targets == 3U && described.key.empty());
    for (size_t i = 0U; i < 3U; ++i) {
        CHECK(catalog.describe(i, described, targets) && targets == 3U);
        CHECK(described.key.data == bindings[i].field()->key.data);
        CHECK(described.valueType == bindings[i].field()->type);
    }
    CHECK(!catalog.describe(3U, described, targets) && described.key.empty() && targets == 0U);
    CHECK(f.calls == 0U); // Discovery never probes by applying a candidate value.
    auto task = powerValue();
    CHECK(target.validate(task) && f.calls == 0U);
    f.target = &target; f.nestedTask = &task; f.reenter = true;
    CHECK(target.apply(task) && f.calls == 1U);
    bool current = false; CHECK(f.power.current(current) && current);
    f.report = false; task.value[0] = 0xF4U;
    CHECK(target.apply(task) && f.calls == 2U);
    CHECK(f.power.current(current) && current); // No implicit state update/pretend GPIO.
    const uint8_t tooLarge[] = {0x1AU, 0x80U, 0U, 0U, 0U};
    task = value("small", ValueType::SignedInteger, ByteView(tooLarge, sizeof(tooLarge)));
    CHECK(!target.validate(task) && f.calls == 2U); // Typed narrowing checked BEFORE Claim.
    task.value[0] = 0x18U; task.value[1] = 127U; task.valueLength = 2U;
    CHECK(target.validate(task)); f.reject = true;
    CHECK(target.apply(task).code() == ErrorCode::WouldBlock);
    int32_t small = 0; CHECK(!f.small.current(small));
    f.reject = false; CHECK(target.apply(task)); CHECK(f.small.current(small) && small == 127);
    uint8_t encoded[9]; cbor::Writer writer(MutableByteSpan(encoded, sizeof(encoded)));
    CHECK(writer.writeFloat64(1.25));
    task = value("ratio", ValueType::Float64, ByteView(encoded, writer.size()));
    CHECK(!target.validate(task)); // Step constraint.
    cbor::Writer good(MutableByteSpan(encoded, sizeof(encoded))); CHECK(good.writeFloat64(1.5));
    task = value("ratio", ValueType::Float64, ByteView(encoded, good.size()));
    CHECK(target.apply(task)); double ratio = 0.0; CHECK(f.ratio.current(ratio) && ratio == 1.5);
    task = powerValue(); memcpy(task.key, "read", 4U); task.keyLength = 4U;
    CHECK(!target.apply(task));
    task = powerValue(); task.valueType = ValueType::UnsignedInteger;
    CHECK(!target.apply(task));
    const uint8_t text[] = {0x61U, 'x'};
    task = value("text", ValueType::Text, ByteView(text, sizeof(text)));
    CHECK(!target.apply(task));
    task = powerValue(); memcpy(task.key, "other", 5U);
    CHECK(target.validate(task).code() == ErrorCode::NotFound);
    task = powerValue(); task.valueLength = 2U; task.value[1] = 0xF4U;
    CHECK(!target.apply(task));
    const TaskPropertyBinding duplicate[] = {TaskPropertyBinding(f.power), TaskPropertyBinding(f.power)};
    TaskPropertyTarget invalid(*f.lifecycle.client, duplicate, 2U);
    CHECK(invalid.validate(powerValue()).code() == ErrorCode::AlreadyExists);
    CHECK(invalid.describe(UINT32_MAX, described, targets).code() == ErrorCode::AlreadyExists);
    TaskPropertyTarget empty(*f.lifecycle.client, nullptr, 0U); CHECK(!empty.apply(powerValue()));
    Property<bool> unbound(f.powerSpec);
    const TaskPropertyBinding wrong[] = {TaskPropertyBinding(unbound)};
    TaskPropertyTarget unboundTarget(*f.lifecycle.client, wrong, 1U); CHECK(!unboundTarget.apply(powerValue()));
    CHECK(!unboundTarget.describe(UINT32_MAX, described, targets));
    CHECK(f.device.endpoints().find("power").onCommand(
        [](void*, const EndpointHandle&, const EndpointValueView&, const RxContext&) { CHECK(false); return Result::success(); }, nullptr));
    CHECK(!target.validate(powerValue())); // Low-level override must never retain stale local permission.
    CHECK(!catalog.describe(UINT32_MAX, described, targets));
    CHECK(f.power.onWrite(&Fixture::powerWrite)); f.reenter = false;
    CHECK(target.validate(powerValue()));
    CHECK(catalog.describe(UINT32_MAX, described, targets));
    Fixture other;
    TaskPropertyTarget wrongClient(*other.lifecycle.client, bindings, 5U);
    CHECK(!wrongClient.apply(powerValue()));
}

void coherentTime() {
    Fixture f;
    TaskTimeEnvironment late(&Fixture::scope, &f);
    TaskObservation unopened;
    late.observe(unopened); CHECK(unopened.authority.state == TaskAuthorityState::Pending);
    CHECK(late.attach(f.time)); CHECK(late.attach(f.time));
    TimeSyncService otherTime; CHECK(late.attach(otherTime).code() == ErrorCode::AlreadyExists);
    late.observe(unopened); CHECK(unopened.authority.state == TaskAuthorityState::Current);
    const TaskPropertyBinding selected[] = {TaskPropertyBinding(f.power)};
    TaskPropertyTarget selectedTarget(nullptr, 0U);
    CHECK(!selectedTarget.configure(nullptr, 1U)); CHECK(!selectedTarget.configure(selected, 0U));
    CHECK(selectedTarget.configure(selected, 1U)); CHECK(selectedTarget.attach(*f.lifecycle.client));
    CHECK(selectedTarget.validate(powerValue()));
    CHECK(selectedTarget.configure(selected, 1U).code() == ErrorCode::AlreadyExists);
    TimeSyncService unconfigured;
    TaskTimeEnvironment absent(unconfigured, &Fixture::scope, &f);
    TaskObservation observation;
    absent.observe(observation); CHECK(observation.authority.state != TaskAuthorityState::Current);
    f.tick.step = 1U;
    const unsigned before = f.tick.reads;
    f.environment.observe(observation);
    CHECK(f.tick.reads == before + 1U && observation.authority.state == TaskAuthorityState::Current && observation.authority.binding == testAuthority());
    CHECK(observation.time.state == WallClockState::Unsynchronized);
    f.tick.step = 0U;
    TimeSyncPeer peer; peer.authenticated = true; peer.permissions = kAuthorizationPermissionControl;
    peer.features = bbp2::FeatureTimeSync; peer.source = WallTimeSource::Server; peer.sessionId = 1U;
    uint8_t request[32]; size_t size = 0U;
    CHECK(f.time.request(peer, 1U, MutableByteSpan(request, sizeof(request)), size));
    bbp2::TimeResponseView sample;
    CHECK(bbp2::decodeTimeRequest(ByteView(request, size), sample.nonce));
    sample.utcAtReplyMillis = 1790035200000ULL; sample.uncertaintyMillis = 100U;
    uint8_t reply[32]; CHECK(bbp2::encodeTimeResponse(sample, MutableByteSpan(reply, sizeof(reply)), size));
    bbp2::FrameView frame; frame.header.kind = static_cast<uint8_t>(bbp2::MessageKind::TimeResponse);
    frame.header.sequence = 1U; frame.header.flags = bbp2::FlagIsResponse; frame.body = ByteView(reply, size);
    CHECK(f.time.receive(peer, frame));
    const uint32_t anchor = f.tick.now;
    f.tick.now += 50U; f.tick.step = 1U;
    f.environment.observe(observation);
    CHECK(observation.nowMillis == anchor + 50U && observation.time.utcMillis == sample.utcAtReplyMillis + 50U);
    f.tick.step = 0U; f.tick.now += 3600000U;
    f.environment.observe(observation); CHECK(observation.time.state == WallClockState::Stale && observation.authority.state == TaskAuthorityState::Current && observation.authority.binding == testAuthority());
    f.authorityState = TaskAuthorityState::Retired; f.environment.observe(observation); CHECK(observation.authority.state != TaskAuthorityState::Current);
}

template <size_t Capacity>
void moduleAndRecovery() {
    nvs = Nvs();
    nvs.durable["nvs/bl_v2_net/profile"] = Bytes(20U, 0xA5U);
    nvs.durable["nvs/bl_v2_access/record"] = Bytes(32U, 0x5AU);
    nvs.working = nvs.durable;
    Fixture f;
    const TaskPropertyBinding bindings[] = {TaskPropertyBinding(f.power)};
    TaskPropertyTarget target(*f.lifecycle.client, bindings, 1U);
    {
        Esp32NvsBlobStoreConfig input;
        input.namespaceName = "bl_v2_access"; input.key = "record"; // Composition fixes task namespace/key/size.
        Esp32TaskModule<Capacity> module(f.environment, target, policy(), input);
        CHECK(nvs.opens == 0U && nvs.sets == 0U);
        CHECK(module.begin().code() == ErrorCode::NotFound);
        CHECK(nvs.lastNamespace == "nvs/bl_v2_tasks" && nvs.lastKey == "snapshot");
        CHECK(nvs.sets == 0U && module.engine().state() == TaskEngineState::Empty);
        f.tick.now += 100U; CHECK(module.engine().initializeVerified(testAuthority()));
        installCountdown(module.engine());
        f.tick.now += 100U;
        TaskPollReport report; CHECK(module.engine().poll(report) && report.callbackInvoked && f.calls == 1U);
        CHECK(report.progress == TaskProgress::Completed && nvs.commits == 5U);
        for (unsigned i = 0U; i < 100U; ++i) CHECK(module.engine().poll(report));
        CHECK(nvs.commits == 5U && nvs.erases == 0U);
        for (size_t i = 1U; i < Capacity; ++i) {
            TaskMutation request = createPower();
            request.requestId[0] = static_cast<uint8_t>(i + 2U);
            request.expectedRevision = static_cast<uint32_t>(i + 1U);
            request.definition.enabled = false;
            TaskMutationReceipt receipt;
            CHECK(module.engine().mutateVerified(testAuthority(), request, receipt));
        }
        CHECK(nvs.durable["nvs/bl_v2_tasks/snapshot"].size() == TaskSnapshotCodec::encodedSize(Capacity));
        std::cout << Capacity << " tasks: ESP32 module sizeof=" << sizeof(module)
                  << ", binding=" << sizeof(TaskPropertyBinding) << ", target=" << sizeof(target) << '\n';
    }
    nvs.powerCycle(); f.tick.now = 0U;
    {
        Esp32TaskModule<Capacity> recovered(f.environment, target, policy());
        CHECK(recovered.begin());
        CHECK(recovered.engine().count() == Capacity);
        TaskSnapshotRecord task; CHECK(recovered.engine().recordAt(0U, task));
        CHECK(task.progress == TaskProgress::Completed);
        f.tick.now += 1000U; TaskPollReport report;
        CHECK(recovered.engine().poll(report) && !report.callbackInvoked && f.calls == 1U);
        f.authorityState = TaskAuthorityState::Retired;
        CHECK(!recovered.engine().poll(report) && recovered.engine().state() == TaskEngineState::Retired);
    }
    CHECK(nvs.durable["nvs/bl_v2_net/profile"] == Bytes(20U, 0xA5U));
    CHECK(nvs.durable["nvs/bl_v2_access/record"] == Bytes(32U, 0x5AU));
}

void nvsFailureBoundaries() {
    for (bool after : {false, true}) {
        nvs = Nvs(); Fixture f;
        const TaskPropertyBinding bindings[] = {TaskPropertyBinding(f.power)};
        TaskPropertyTarget target(*f.lifecycle.client, bindings, 1U);
        {
            Esp32TaskModule<4> module(f.environment, target, policy());
            CHECK(module.begin().code() == ErrorCode::NotFound);
            f.tick.now += 100U; CHECK(module.engine().initializeVerified(testAuthority())); installCountdown(module.engine());
            nvs.failCommit = nvs.commits + 1U; nvs.commitThenFail = after;
            f.tick.now += 100U; TaskPollReport report;
            CHECK(!module.engine().poll(report) && f.calls == 0U && module.engine().state() == TaskEngineState::Fault);
            const unsigned sets = nvs.sets;
            CHECK(!module.engine().poll(report) && nvs.sets == sets);
            CHECK(module.begin().code() == ErrorCode::AlreadyExists);
        }
        nvs.powerCycle(); f.tick.now = 0U;
        Esp32TaskModule<4> recovered(f.environment, target, policy()); CHECK(recovered.begin());
        TaskSnapshotRecord task; CHECK(recovered.engine().recordAt(0U, task));
        CHECK(task.progress == (after ? TaskProgress::Claimed : TaskProgress::Interrupted));
        f.tick.now += 1000U; TaskPollReport report;
        CHECK(recovered.engine().poll(report) && !report.callbackInvoked && f.calls == 0U);
    }
    for (bool encrypted : {false, true}) {
        nvs = Nvs(); Fixture f;
        const TaskPropertyBinding bindings[] = {TaskPropertyBinding(f.power)};
        TaskPropertyTarget target(*f.lifecycle.client, bindings, 1U);
        Esp32NvsBlobStoreConfig config; config.encrypted = encrypted;
        if (!encrypted) nvs.openError = ESP_ERR_NVS_NOT_ENOUGH_SPACE;
        Esp32TaskModule<4> failed(f.environment, target, policy(), config);
        CHECK(!failed.begin() && nvs.sets == 0U && nvs.erases == 0U);
        CHECK(failed.begin().code() == ErrorCode::AlreadyExists);
    }
    nvs = Nvs(); Fixture f;
    const TaskPropertyBinding bindings[] = {TaskPropertyBinding(f.power)};
    TaskPropertyTarget target(*f.lifecycle.client, bindings, 1U);
    Esp32TaskModule<4> full(f.environment, target, policy());
    CHECK(full.begin().code() == ErrorCode::NotFound);
    f.tick.now += 100U; CHECK(full.engine().initializeVerified(testAuthority()));
    nvs.setError = ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    const unsigned commits = nvs.commits;
    TaskMutationReceipt receipt;
    CHECK(!full.engine().mutateVerified(testAuthority(), createPower(), receipt));
    CHECK(full.engine().state() == TaskEngineState::Fault && nvs.commits == commits && receipt.taskId == 0U);
}

void nvsRetirementMarker() {
    const uint8_t requestId[16] = {81U};
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        nvs = Nvs();
        nvs.durable["nvs/bl_v2_net/profile"] = Bytes(20U, 0xA5U);
        nvs.durable["nvs/bl_v2_access/record"] = Bytes(32U, 0x5AU);
        nvs.working = nvs.durable;
        Fixture f;
        const TaskPropertyBinding bindings[] = {TaskPropertyBinding(f.power)};
        TaskPropertyTarget target(*f.lifecycle.client, bindings, 1U);
        {
            Esp32TaskModule<4> module(f.environment, target, policy());
            CHECK(module.begin().code() == ErrorCode::NotFound);
            f.tick.now += 100U; CHECK(module.engine().initializeVerified(testAuthority()));
            installCountdown(module.engine());
            if (mode != 0U) { nvs.failCommit = nvs.commits + 1U; nvs.commitThenFail = mode == 2U; }
            CHECK(module.engine().retireVerified(testAuthority(), ByteView(requestId, 16U)).ok() == (mode == 0U));
            TaskSnapshotHeader marker;
            CHECK(module.engine().retirementRecord(marker).ok() == (mode == 0U));
            const unsigned writes = nvs.commits;
            CHECK(module.engine().retireVerified(testAuthority(), ByteView(requestId, 16U)).ok() == (mode == 0U));
            CHECK(nvs.erases == 0U && nvs.commits == writes);
        }
        CHECK(nvs.durable["nvs/bl_v2_net/profile"] == Bytes(20U, 0xA5U));
        CHECK(nvs.durable["nvs/bl_v2_access/record"] == Bytes(32U, 0x5AU));
        nvs.powerCycle(); f.tick.now = 0U;
        Esp32TaskModule<4> recovered(f.environment, target, policy());
        const Result result = recovered.begin();
        TaskSnapshotHeader marker;
        if (mode == 1U) {
            CHECK(result && recovered.engine().state() == TaskEngineState::Recovering);
            CHECK(!recovered.engine().retirementRecord(marker));
        } else {
            CHECK(result.code() == ErrorCode::AuthenticationRequired && recovered.engine().state() == TaskEngineState::Retired);
            CHECK(recovered.engine().retirementRecord(marker) && marker.requestId[0] == 81U);
            CHECK(nvs.durable["nvs/bl_v2_tasks/snapshot"].size() == 132U);
            CHECK(recovered.engine().retireVerified(testAuthority(), ByteView(requestId, 16U)));
            f.tick.now += 1000U; TaskPollReport report;
            CHECK(!recovered.engine().poll(report) && f.calls == 0U);
        }
    }
}

void nvsRetirement() {
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        nvs = Nvs();
        nvs.durable["nvs/bl_v2_net/profile"] = Bytes(20U, 0xA5U);
        nvs.durable["nvs/bl_v2_access/record"] = Bytes(32U, 0x5AU);
        nvs.working = nvs.durable;
        Fixture f;
        const TaskPropertyBinding bindings[] = {TaskPropertyBinding(f.power)};
        TaskPropertyTarget target(*f.lifecycle.client, bindings, 1U);
        {
            Esp32TaskModule<4> module(f.environment, target, policy());
            CHECK(module.begin().code() == ErrorCode::NotFound);
            f.tick.now += 100U; CHECK(module.engine().initializeVerified(testAuthority()));
            installCountdown(module.engine());
            if (mode != 0U) { nvs.failCommit = nvs.commits + 1U; nvs.commitThenFail = mode == 2U; }
            module.engine().retire();
            CHECK(module.engine().clearRetired().ok() == (mode == 0U));
            CHECK(nvs.lastNamespace == "nvs/bl_v2_tasks" && nvs.lastKey == "snapshot");
            const unsigned commits = nvs.commits;
            CHECK(module.engine().clearRetired().ok() == (mode == 0U));
            CHECK(nvs.erases == 1U && nvs.commits == commits);
            f.tick.now += 1000U; TaskPollReport report;
            CHECK(!module.engine().poll(report) && f.calls == 0U);
        }
        CHECK(nvs.durable["nvs/bl_v2_net/profile"] == Bytes(20U, 0xA5U));
        CHECK(nvs.durable["nvs/bl_v2_access/record"] == Bytes(32U, 0x5AU));
        nvs.powerCycle(); f.tick.now = 0U;
        Esp32TaskModule<4> recovered(f.environment, target, policy());
        const Result result = recovered.begin();
        if (mode == 1U) {
            CHECK(result && recovered.engine().state() == TaskEngineState::Recovering);
            TaskSnapshotRecord task; CHECK(recovered.engine().recordAt(0U, task));
            CHECK(task.progress == TaskProgress::Interrupted);
        } else CHECK(result.code() == ErrorCode::NotFound && recovered.engine().count() == 0U);
    }
    for (bool fail : {false, true}) {
        nvs = Nvs(); Fixture f;
        const TaskPropertyBinding bindings[] = {TaskPropertyBinding(f.power)};
        TaskPropertyTarget target(*f.lifecycle.client, bindings, 1U);
        Esp32TaskModule<4> empty(f.environment, target, policy());
        CHECK(empty.begin().code() == ErrorCode::NotFound);
        empty.engine().retire();
        if (fail) nvs.failCommit = 1U;
        CHECK(empty.engine().clearRetired().ok() == !fail);
        CHECK(nvs.erases == 1U && nvs.commits == 1U);
        CHECK(empty.engine().clearRetired().ok() == !fail);
        CHECK(nvs.erases == 1U && nvs.commits == 1U); // NotFound cannot conceal failed commit.
    }
}
} // namespace

extern "C" esp_err_t nvs_open_from_partition(const char* partition, const char* name,
                                              nvs_open_mode_t, nvs_handle_t* out) {
    if (nvs.openError != ESP_OK) return nvs.openError;
    nvs.lastNamespace = std::string(partition) + "/" + name;
    *out = ++nvs.opens; nvs.handles[*out] = nvs.lastNamespace; return ESP_OK;
}
extern "C" esp_err_t nvs_open(const char* name, nvs_open_mode_t mode, nvs_handle_t* out) {
    return nvs_open_from_partition("nvs", name, mode, out);
}
extern "C" void nvs_close(nvs_handle_t handle) { nvs.handles.erase(handle); }
extern "C" esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* out, size_t* length) {
    if (nvs.handles.count(handle) == 0U) return ESP_ERR_NVS_INVALID_HANDLE;
    nvs.lastKey = key; const auto found = nvs.working.find(nvs.handles[handle] + "/" + key);
    if (found == nvs.working.end()) return ESP_ERR_NVS_NOT_FOUND;
    if (out != nullptr) {
        if (*length < found->second.size()) return ESP_ERR_NVS_INVALID_LENGTH;
        memcpy(out, found->second.data(), found->second.size());
    }
    *length = found->second.size(); return ESP_OK;
}
extern "C" esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key, const void* value, size_t size) {
    if (nvs.handles.count(handle) == 0U) return ESP_ERR_NVS_INVALID_HANDLE;
    ++nvs.sets; nvs.lastKey = key;
    if (nvs.setError != ESP_OK) return nvs.setError;
    const uint8_t* bytes = static_cast<const uint8_t*>(value);
    nvs.working[nvs.handles[handle] + "/" + key] = Bytes(bytes, bytes + size); return ESP_OK;
}
extern "C" esp_err_t nvs_erase_key(nvs_handle_t handle, const char* key) {
    if (nvs.handles.count(handle) == 0U) return ESP_ERR_NVS_INVALID_HANDLE;
    ++nvs.erases; nvs.lastKey = key;
    return nvs.working.erase(nvs.handles[handle] + "/" + key) != 0U ? ESP_OK : ESP_ERR_NVS_NOT_FOUND;
}
extern "C" esp_err_t nvs_commit(nvs_handle_t handle) {
    if (nvs.handles.count(handle) == 0U) return ESP_ERR_NVS_INVALID_HANDLE;
    const bool fail = ++nvs.commits == nvs.failCommit;
    if (!fail || nvs.commitThenFail) {
        // A commit on one namespace must not flush another dirty handle.
        const std::string prefix = nvs.handles[handle] + "/";
        for (auto it = nvs.durable.begin(); it != nvs.durable.end();) {
            if (it->first.compare(0U, prefix.size(), prefix) == 0) it = nvs.durable.erase(it);
            else ++it;
        }
        for (const auto& item : nvs.working)
            if (item.first.compare(0U, prefix.size(), prefix) == 0) nvs.durable[item.first] = item.second;
    }
    return fail ? ESP_ERR_NVS_NOT_ENOUGH_SPACE : ESP_OK;
}

int main() {
    propertyAdmissionAndCallbacks(); coherentTime();
    moduleAndRecovery<4>(); moduleAndRecovery<8>(); moduleAndRecovery<10>(); nvsFailureBoundaries(); nvsRetirement();
    nvsRetirementMarker();
    lifecycleRunAndNetwork(); lifecycleResetTransactions(); lifecycleReentrantRetirement(); lifecycleAdmissionFailures();
    lifecycleScopeObservation(); lifecycleTerminalSnapshot(); lifecycleAuthorityResume();
    std::cout << "task product: property/time/NVS composition gates passed (host fake NVS, no hardware)\n";
}
