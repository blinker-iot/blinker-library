// COMPILE/LINK ONLY. Not a public task example or a hardware storage test.
// No credentials, invented owner scope, automatic initialization or GPIO.
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/platform/esp32/Esp32WiFiProduct.h>
#include <BlinkerV2/runtime/TimeSyncService.h>

#ifndef BLINKER_TASK_PROBE_CAPACITY
#define BLINKER_TASK_PROBE_CAPACITY 0
#endif
#if BLINKER_TASK_PROBE_CAPACITY != 0 && BLINKER_TASK_PROBE_CAPACITY != 4 && \
    BLINKER_TASK_PROBE_CAPACITY != 8 && BLINKER_TASK_PROBE_CAPACITY != 10
#error "Probe capacity must be 0 (disabled), 4, 8 or 10"
#endif
#if BLINKER_TASK_PROBE_CAPACITY
#include <BlinkerV2/api/TaskProductLifecycle.h>
#include <BlinkerV2/runtime/TaskTimeEnvironment.h>
#include <BlinkerV2/arduino/platform/esp32/Esp32TaskModule.h>
#endif

using namespace blinker;
using namespace blinker::integration::official_detail;

static const PropertySpec<bool> powerSpec("power", readWrite());
static Property<bool> power(powerSpec);
static const DeviceSchema<Property<bool>> schema(power);
static Device device(schema);
static DeviceKeyWifiProduct<Esp32WifiNetworkPlatform> wifi;
static TimeSyncService timeService; // Same time-enabled baseline in every variant.

// Volatile guard retains the real product graph for the linker, but this
// compile-only firmware does not start networking or open NVS if uploaded.
volatile bool blinkerTaskProbeArmed = false;
volatile uint8_t blinkerTaskProbeOperation = 0U;
volatile uint16_t blinkerTaskProbeResult = 0U;

#if BLINKER_TASK_PROBE_CAPACITY
static void noOwnerScope(void*, TaskAuthorityObservation&) {} // Pending, never fake owner.
static TaskTimeEnvironment taskEnvironment(timeService, noOwnerScope, nullptr);
static const TaskPropertyBinding bindings[] = {TaskPropertyBinding(power)};
static TaskWritePolicy compilePolicy() {
    TaskWritePolicy policy;
    policy.windowMillis = 60000U;
    policy.writesPerWindow = 2U;
    policy.minimumStartMillis = 60000U;
    policy.maximumCommitMillis = 100U;
    return policy; // Link fixture only; NOT a measured production write policy.
}
static TaskPropertyTarget target(bindings, 1U);
static Esp32TaskModule<BLINKER_TASK_PROBE_CAPACITY> tasks(taskEnvironment, target, compilePolicy());
static TaskProductLifecycle<Esp32TaskModule<BLINKER_TASK_PROBE_CAPACITY>> lifecycle(wifi, tasks, target);
static Product product(device, lifecycle);

// Retain all internal management paths as well as poll. Empty authority prevents
// these internal calls from granting authority. No new BBP kind is advertised.
static Result linkManagement() {
    TaskEngine& engine = tasks.engine();
    switch (blinkerTaskProbeOperation) {
        case 1U: return engine.initializeVerified(TaskAuthorityBinding());
        case 2U: {
            TaskMutation request;
            TaskMutationReceipt receipt;
            return engine.mutateVerified(TaskAuthorityBinding(), request, receipt);
        }
        case 3U: {
            TaskSnapshotRecord record;
            return engine.recordAt(0U, record);
        }
        case 4U: return product.resetAccess(); // WiFi-only keeps UnsupportedFeature.
        case 5U: return engine.rebindVerified(TaskAuthorityBinding(), TaskAuthorityBinding());
        case 6U: return engine.retireVerified(TaskAuthorityBinding(), ByteView());
        case 7U: {
            TaskSnapshotHeader marker;
            return engine.retirementRecord(marker);
        }
        default: return Result::success(); // Product is the single poll owner.
    }
}
#else
static Product product(device, wifi);
#endif

static void onPower(bool value) { (void)power.report(value); }

void setup() {
    if (!blinkerTaskProbeArmed) return;
    Result result = power.onWrite(onPower);
    // Public test-vector key and deliberately unusable network, not secrets.
    if (result) result = wifi.configure(StringView("compile-only.invalid"), StringView(),
        StringView("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8"));
    if (result) result = product.begin(&timeService);
    blinkerTaskProbeResult = static_cast<uint16_t>(result.code());
}

void loop() {
    if (!blinkerTaskProbeArmed || !product.status().started) return;
    product.run(1000U);
#if BLINKER_TASK_PROBE_CAPACITY
    blinkerTaskProbeResult = static_cast<uint16_t>(linkManagement().code());
#endif
}
