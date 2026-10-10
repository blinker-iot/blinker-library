// Isolated hardware fixture, NOT a public task API or an owner-auth adapter.
// No WiFi/BLE, credentials, GPIO or automatic task creation. Serial commands
// touch only bl_task_gate/snapshot; original firmware must be backed up first.
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/api/Product.h>
#include <BlinkerV2/api/TaskPropertyTarget.h>
#include <BlinkerV2/runtime/TaskTimeEnvironment.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32Storage.h>
#include <esp_system.h>

using namespace blinker;
namespace {
// Fixed 128-bit test authority. Never used by a production/network adapter.
TaskAuthorityBinding fixtureAuthority() {
    TaskAuthorityBinding value;
    for (size_t i = 0U; i < 16U; ++i) {
        value.deviceInstanceId[i] = static_cast<uint8_t>(i + 1U);
        value.taskScope[i] = static_cast<uint8_t>(i + 33U);
    }
    for (size_t i = 0U; i < 32U; ++i) value.rootDigest[i] = static_cast<uint8_t>(i + 65U);
    return value;
}
enum class Cut { None, Armed, BeforeClaim, Claimed, Applied };
Cut cut = Cut::None;
uint32_t callbacks = 0U;
bool ready = false;

void hold(const char* phase) {
    Serial.printf("{\"event\":\"cut\",\"phase\":\"%s\",\"callbacks\":%lu}\n", phase,
                  static_cast<unsigned long>(callbacks));
    Serial.flush();
    // Observable physical power-cut window. No Flash writes/retries here.
    // A software reset is a DIFFERENT test; runner must not call it power loss.
    for (;;) delay(50U);
}
struct Tick : IClock {
    uint32_t monotonicMillis() const override { return millis(); }
    bool unixTime(uint64_t& seconds) const override { seconds = 0U; return false; }
} tick;
struct NoTimeRandom : IRandom {
    Result fill(MutableByteSpan) override { return Result::failure(ErrorCode::NotConfigured); }
} randomSource; // This fixture never exchanges UTC samples or security nonces.
TimeSyncService timeService;
void isolatedScope(void*, TaskAuthorityObservation& output) {
    output.binding = fixtureAuthority(); output.state = TaskAuthorityState::Current;
}
TaskTimeEnvironment environment(timeService, isolatedScope, nullptr);

struct OfflineLifecycle : IProductLifecycle {
    blinker::Client* client = nullptr;
    Result attach(blinker::Client& value) override { client = &value; return Result::success(); }
    Result start() override { return Result::success(); }
    void poll(uint32_t) override {}
    void stop() override {}
    ProductLifecycleStatus status() const override { return ProductLifecycleStatus(); }
    ProductCapabilities capabilities() const override { return ProductCapabilities(); }
} lifecycle;
const PropertySpec<bool> powerSpec("power", readWrite());
Property<bool> power(powerSpec);
const DeviceSchema<Property<bool>> schema(power);
Device device(schema);
Product product(device, lifecycle);

Esp32NvsBlobStoreConfig storageConfig() {
    Esp32NvsBlobStoreConfig config;
    config.namespaceName = "bl_task_gate";
    config.key = "snapshot";
    config.maximumSize = TaskSnapshotCodec::encodedSize(10U);
    return config;
}
Esp32NvsAtomicBlobStore nvs(storageConfig());
struct MeasuredStore : IAtomicBlobStore {
    uint32_t writes = 0U, maxWriteUs = 0U;
    Result load(MutableByteSpan output, size_t& size) override { return nvs.load(output, size); }
    Result replace(ByteView value) override {
        // Fixed internal snapshot v2: progress is byte 22 of first record.
        const bool claim = value.size >= TaskSnapshotCodec::encodedSize(1U) &&
            value.data[TaskSnapshotCodec::headerSize + 22U] == static_cast<uint8_t>(TaskProgress::Claimed);
        if (claim && cut == Cut::BeforeClaim) hold("before_claim");
        const uint32_t before = micros();
        const Result result = nvs.replace(value);
        const uint32_t elapsed = micros() - before;
        ++writes; if (elapsed > maxWriteUs) maxWriteUs = elapsed;
        Serial.printf("{\"event\":\"write\",\"bytes\":%u,\"us\":%lu,\"code\":%u}\n",
            static_cast<unsigned>(value.size), static_cast<unsigned long>(elapsed), static_cast<unsigned>(result.code()));
        if (result && claim && cut == Cut::Claimed) hold("claimed");
        return result;
    }
    Result clear() override { return nvs.clear(); }
    StorageProtection protection() const override { return nvs.protection(); }
} measured;
uint8_t workspace[TaskSnapshotCodec::headerSize + 10U * TaskSnapshotCodec::recordSize + TaskSnapshotCodec::trailerSize];
TaskSnapshotRecord records[10];
TaskRuntimeSlot slots[10];
TaskSnapshotStore store(measured, MutableByteSpan(workspace, sizeof(workspace)));
const TaskPropertyBinding bindings[] = {TaskPropertyBinding(power)};
TaskWritePolicy fixturePolicy() {
    TaskWritePolicy policy;
    policy.windowMillis = 1000U; policy.writesPerWindow = 4U;
    policy.minimumStartMillis = 1000U; policy.maximumCommitMillis = 1000U;
    return policy; // Finite measurement fixture, not a production endurance policy.
}
TaskEngine& engine() {
    static TaskPropertyTarget target(*lifecycle.client, bindings, 1U);
    static TaskEngine value(store, environment, target, records, slots, 10U, fixturePolicy());
    return value;
}
void onPower(bool value) {
    ++callbacks;
    const Result result = power.report(value);
    Serial.printf("{\"event\":\"apply\",\"count\":%lu,\"value\":%u,\"code\":%u}\n",
        static_cast<unsigned long>(callbacks), value ? 1U : 0U, static_cast<unsigned>(result.code()));
    if (cut == Cut::Applied) hold("applied");
}
void status(const char* label) {
    nvs_stats_t stats = {};
    const esp_err_t statsResult = nvs_get_stats(nullptr, &stats);
    Serial.printf("{\"event\":\"status\",\"label\":\"%s\",\"state\":%u,\"count\":%u,\"revision\":%lu,"
        "\"callbacks\":%lu,\"writes\":%lu,\"maxWriteUs\":%lu,\"heap\":%lu,\"minHeap\":%lu,"
        "\"stackFree\":%u,\"nvsStatsCode\":%d,\"nvsUsed\":%u,\"nvsFree\":%u}\n", label,
        static_cast<unsigned>(engine().state()), static_cast<unsigned>(engine().count()),
        static_cast<unsigned long>(engine().tableRevision()), static_cast<unsigned long>(callbacks),
        static_cast<unsigned long>(measured.writes), static_cast<unsigned long>(measured.maxWriteUs),
        static_cast<unsigned long>(ESP.getFreeHeap()), static_cast<unsigned long>(ESP.getMinFreeHeap()),
        static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)), static_cast<int>(statsResult),
        static_cast<unsigned>(stats.used_entries), static_cast<unsigned>(stats.free_entries));
    for (size_t i = 0U; i < engine().count(); ++i) {
        TaskSnapshotRecord record;
        const Result result = engine().recordAt(i, record);
        Serial.printf("{\"event\":\"record\",\"index\":%u,\"code\":%u,\"id\":%lu,\"definition\":%lu,"
            "\"progress\":%u,\"start\":%lu,\"enabled\":%u}\n", static_cast<unsigned>(i),
            static_cast<unsigned>(result.code()), static_cast<unsigned long>(record.taskId),
            static_cast<unsigned long>(record.definitionRevision), static_cast<unsigned>(record.progress),
            static_cast<unsigned long>(record.startSequence), record.enabled ? 1U : 0U);
    }
}
void requestIdentity(TaskMutation& request) {
    request.expectedRevision = engine().tableRevision();
    // Test-only unique deterministic request ID per table revision.
    request.requestId[0] = 'G';
    for (size_t i = 0U; i < 4U; ++i)
        request.requestId[1U + i] = static_cast<uint8_t>((request.expectedRevision + 1U) >> (8U * i));
}
TaskMutation create() {
    TaskMutation request; requestIdentity(request);
    request.definition.keyLength = 5U; memcpy(request.definition.key, "power", 5U);
    request.definition.value[0] = 0xF5U; request.definition.valueLength = 1U;
    request.definition.enabled = true;
    request.definition.timing.durationMillis = 1500U;
    return request;
}
Result mutate(const TaskMutation& request, TaskMutationReceipt& receipt) {
    const uint32_t start = millis();
    Result result;
    do {
        result = engine().mutateVerified(fixtureAuthority(), request, receipt);
        if (result.code() != ErrorCode::WouldBlock) break;
        delay(10U);
    } while (millis() - start < 1500U);
    return result;
}
Result fill(size_t count) {
    if (engine().state() != TaskEngineState::Empty) return Result::failure(ErrorCode::StateConflict);
    delay(1000U); // Cold write bucket, never initialize automatically at boot.
    Result result = engine().initializeVerified(fixtureAuthority());
    for (size_t i = 0U; result && i < count; ++i) {
        TaskMutationReceipt receipt;
        result = mutate(create(), receipt);
    }
    return result;
}
Result run(Cut requestedCut) {
    TaskMutation request; requestIdentity(request);
    request.kind = TaskMutationKind::Start; request.taskId = 1U;
    TaskMutationReceipt receipt;
    Result result = mutate(request, receipt);
    if (!result) return result;
    const uint32_t writes = measured.writes;
    result = mutate(request, receipt);
    if (!result || !receipt.replayed || measured.writes != writes) return Result::failure(ErrorCode::StateConflict);
    cut = requestedCut;
    if (cut == Cut::Armed) hold("armed");
    const uint32_t start = millis();
    while (millis() - start < 4000U) {
        TaskPollReport report;
        const uint32_t before = micros();
        result = engine().poll(report);
        if (!result && result.code() != ErrorCode::WouldBlock) return result;
        if (report.taskId != 0U) {
            Serial.printf("{\"event\":\"poll\",\"us\":%lu,\"code\":%u,\"progress\":%u,\"invoked\":%u}\n",
                static_cast<unsigned long>(micros() - before), static_cast<unsigned>(result.code()),
                static_cast<unsigned>(report.progress), report.callbackInvoked ? 1U : 0U);
            return result;
        }
        delay(1U);
    }
    return Result::failure(ErrorCode::NotFound);
}
Result idle() {
    const uint32_t writes = measured.writes, calls = callbacks;
    for (size_t i = 0U; i < 100U; ++i) {
        TaskPollReport report;
        const Result result = engine().poll(report);
        if (!result) return result;
    }
    return measured.writes == writes && callbacks == calls ? Result::success() : Result::failure(ErrorCode::StateConflict);
}
Result replay() {
    if (engine().tableRevision() == 0U) return Result::failure(ErrorCode::StateConflict);
    TaskMutation request;
    request.expectedRevision = engine().tableRevision() - 1U;
    request.requestId[0] = 'G';
    for (size_t i = 0U; i < 4U; ++i)
        request.requestId[1U + i] = static_cast<uint8_t>(engine().tableRevision() >> (8U * i));
    request.kind = TaskMutationKind::Start; request.taskId = 1U;
    TaskMutationReceipt receipt;
    const uint32_t writes = measured.writes;
    const Result result = mutate(request, receipt);
    return result && receipt.replayed && measured.writes == writes ? Result::success()
        : Result::failure(ErrorCode::StateConflict);
}
Result full() {
    if (engine().count() != 10U) return Result::failure(ErrorCode::StateConflict);
    const uint32_t writes = measured.writes, revision = engine().tableRevision();
    TaskMutationReceipt receipt;
    const Result result = mutate(create(), receipt);
    return result.code() == ErrorCode::CapacityExceeded && measured.writes == writes &&
        engine().tableRevision() == revision ? Result::success() : Result::failure(ErrorCode::StateConflict);
}
Result edit() {
    TaskMutation request = create();
    request.kind = TaskMutationKind::Replace; request.taskId = 1U;
    TaskMutationReceipt receipt;
    Result result = mutate(request, receipt);
    if (!result) return result;
    request = TaskMutation(); requestIdentity(request);
    request.kind = TaskMutationKind::SetEnabled; request.taskId = 1U; request.enabled = false;
    result = mutate(request, receipt);
    if (!result) return result;
    requestIdentity(request); request.enabled = true;
    return mutate(request, receipt);
}
void restart() { Serial.flush(); delay(200U); ESP.restart(); }
void command(const char* input) {
    if (!ready) return;
    Result result = Result::failure(ErrorCode::InvalidArgument);
    if (strcmp(input, "inspect") == 0) result = Result::success();
    else if (strcmp(input, "fill4") == 0) result = fill(4U);
    else if (strcmp(input, "fill8") == 0) result = fill(8U);
    else if (strcmp(input, "fill10") == 0) result = fill(10U);
    else if (strcmp(input, "run") == 0) result = run(Cut::None);
    else if (strcmp(input, "arm") == 0) result = run(Cut::Armed);
    else if (strcmp(input, "beforeclaim") == 0) result = run(Cut::BeforeClaim);
    else if (strcmp(input, "claim") == 0) result = run(Cut::Claimed);
    else if (strcmp(input, "applied") == 0) result = run(Cut::Applied);
    else if (strcmp(input, "idle") == 0) result = idle();
    else if (strcmp(input, "edit") == 0) result = edit();
    else if (strcmp(input, "replay") == 0) result = replay();
    else if (strcmp(input, "full") == 0) result = full();
    else if (strcmp(input, "recover") == 0) {
        delay(1000U); TaskPollReport report; result = engine().poll(report);
    } else if (strcmp(input, "clear") == 0) {
        // Refuse clearing unknown data or other namespaces. Only this fixture
        // scope's successfully decoded table (or confirmed absence) is allowed.
        if (engine().state() == TaskEngineState::Ready || engine().state() == TaskEngineState::Recovering ||
            engine().state() == TaskEngineState::Empty) {
            engine().retire();
            result = engine().clearRetired();
            if (result) {
                Serial.println("{\"event\":\"cleared\"}"); restart(); return;
            }
        }
    } else if (strcmp(input, "reset") == 0) { restart(); return; }
    status(input);
    Serial.printf("{\"event\":\"done\",\"command\":\"%s\",\"code\":%u}\n", input, static_cast<unsigned>(result.code()));
}
} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t start = millis();
    while (!Serial && millis() - start < 5000U) delay(10U);
    Result result = power.onWrite(onPower);
    if (result) result = product.begin();
    if (result) result = timeService.configure(tick, randomSource);
    const uint32_t before = micros();
    if (result) result = nvs.begin();
    Serial.printf("{\"event\":\"open\",\"us\":%lu,\"code\":%u}\n",
        static_cast<unsigned long>(micros() - before), static_cast<unsigned>(result.code()));
    if (result) result = engine().begin();
    ready = result.ok() || result.code() == ErrorCode::NotFound;
    Serial.printf("{\"event\":\"boot\",\"code\":%u,\"ready\":%u,\"resetReason\":%u}\n",
        static_cast<unsigned>(result.code()), ready ? 1U : 0U, static_cast<unsigned>(esp_reset_reason()));
    if (ready) status("boot");
}
void loop() {
    static char input[16]; static size_t length = 0U; static bool overflow = false;
    while (Serial.available()) {
        const char value = static_cast<char>(Serial.read());
        if (value == '\r') continue;
        if (value == '\n') {
            input[length] = '\0';
            if (!overflow) command(input);
            length = 0U; overflow = false;
        } else if (length + 1U < sizeof(input)) input[length++] = value;
        else overflow = true;
    }
    delay(1U); // No autonomous task poll: all fixture actions are explicit.
}
