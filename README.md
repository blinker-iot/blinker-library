# Blinker Device V2

Blinker Device V2 is one Arduino library for BBP/2 devices. A single install
contains the shared component/protocol runtime, ESP32 and UNO R4 WiFi/BLE,
and Wio Terminal WiFi implementations. Wio Terminal BLE is an explicit
experimental preview and is not part of the current release claim.

## WiFi QuickStart

```cpp
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void onPower(bool value) {
    power.report(value);
}

void setup() {
    power.onWrite(onPower);
    Blinker.begin(
        "DEVICE_KEY",
        "WIFI_SSID",
        "WIFI_PASSWORD",
        power);
}

void loop() {
    Blinker.run();
}
```

## BLE QuickStart

```cpp
#include <BlinkerBLE.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void onPower(bool value) {
    power.report(value);
}

void setup() {
    power.onWrite(onPower);
    Blinker.begin(power);
}

void loop() {
    Blinker.run();
}
```

## ESP32 WiFi+BLE QuickStart

```cpp
#include <BlinkerWiFiBLE.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void onPower(bool value) {
    power.report(value);
}

void setup() {
    power.onWrite(onPower);
    Blinker.begin(power);
}

void loop() {
    Blinker.run();
}
```

`Blinker.h` selects WiFi and `BlinkerWiFi.h` remains its explicit synonym.
`BlinkerBLE.h` and ESP32-only `BlinkerWiFiBLE.h` select the other products in
the same library; they are not separate packages. All selectors expose the same global `Blinker`, strongly
typed Property/Action/Event API, callbacks, atomic reporting and `run()` loop.
There is no public WiFi/BLE mode argument in `begin()`.

Realtime values use a lease-driven sampling callback; user code does not send
them from `loop()`:

```cpp
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));

uint32_t sampleUptimeSeconds() {
    return millis() / 1000U;
}

void setup() {
    uptimeSeconds.onSample(sampleUptimeSeconds);
    // Blinker.begin(..., uptimeSeconds);
}

void loop() {
    Blinker.run();
}
```

`onSample()` runs only when an authorized App/service lease is active and its
negotiated interval is due. Keep the callback short and non-blocking. For a slow
sensor, update a cached value with the sensor's own non-blocking state machine
and return that cache from the callback; do not perform network sends there.

Arduino IDE's **Include Library** action inserts `Blinker.h` by default.
For a BLE sketch, replace it with `BlinkerBLE.h`; do not include both headers.

## Optional business time (WiFi / BLE / ESP32 WiFi+BLE)

Call `Blinker.enableTime()` before `Blinker.begin(...)` to request UTC over the
existing MQTT connection, or from a Control-authorized phone on the existing BLE
Direct connection. Read it with `uint64_t utc; if (Blinker.time(utc)) { ... }`.
The value is UTC seconds; disabled, unsynchronized or expired time returns false
and clears the output. No second connection, NTP client or system clock is added.
The Broker must support BBP/2 TimeSync. Plain MQTT time is not tamper-resistant
and must not be used for credentials, certificates or security deadlines.

TimeSync is off in QuickStart examples and demonstrated in CloudSmokeTest.
Ordinary WiFi, pure BLE and ESP32 WiFi+BLE use the same optional clock API.
BLE needs a compatible App responder and Control permission; an Observer cannot
set time. Business UTC is not a security clock even when its carrier is encrypted.
Gateway time adapters remain internal/preview, not a public product API.
Do not treat missing time as a valid zero timestamp. Final-version hardware
acceptance is tracked separately for each declared platform and mode.

For diagnosis, `blinker::WallClockSnapshot clock; Blinker.timeStatus(clock)`
reads that same optional clock. The return value means valid UTC; on false,
`clock.state` distinguishes `Unsynchronized` from `Stale`. A valid snapshot also
includes source, sample age and uncertainty. Source is not an authorization or
security guarantee. This does not start another synchronization or change
`Blinker.lastError()`.

## Optional Sketch intervals and delays

```cpp
#include <BlinkerTimer.h>  // Same installed library; uses Arduino millis().
#include <Blinker.h>       // Or your existing BLE / WiFi+BLE selector.

blinker::Timer<4> timers;  // Explicit capacity, no heap or Flash writes.

void sampleSensor() { /* Update a local cache without blocking. */ }
void finishPulse() { /* Finish a local output pulse. */ }

void setup() {
    // Keep your usual component setup and Blinker.begin(...) unchanged.
    const auto sampling = timers.setInterval(1000U, sampleSensor);
    const auto delayed = timers.setTimeout(500U, finishPulse);
    if (!sampling.valid() || !delayed.valid()) { /* Invalid input or full. */ }
}
void loop() {
    Blinker.run();
    timers.run();
}
```

`setInterval` / `setTimeout` also accept a `void (*)(void*)` callback and its
context. Use `cancel(handle)`, `active(handle)`, `clear()` and `size()` to manage
callbacks; a valid handle is an issued token, not proof that it is still active.
Use integer milliseconds from 1 through `INT32_MAX`. No catch-up burst occurs
after a slow loop: an interval keeps its phase but runs at most once per call.
Callbacks may cancel or register timers; newly registered timers wait until a
later `run()`. Contexts must remain alive, and handles must not outlive their
scheduler. Use only from the same loop, not interrupts or other tasks.

This is boot-local `millis()` scheduling, not an App-editable persistent task.
Restart requires registration again. Network disconnect and `Blinker.end()` do
not cancel these independent timers. Keep callbacks short; continue polling at
gaps below `2^31 ms`, and clear/re-register after an unmeasured sleep or tick
reset. Real-time upload still uses `realtime()` / `onSample()` and its lease,
not an always-on report timer. The portable `SketchTimer<N>` core accepts a
monotonic clock function for non-Arduino use.

## Optional persistent tasks

Persistent tasks are disabled by default (capacity 0). The public development
profile and ordinary examples do not enable them. The closed ESP32 manual-WiFi
pilot requires the maintainer-built `esp32-wifi-t2-lab-v1` profile and a matching
Service device allowlist. Arduino users do not generate profiles, supply storage
budgets, or assemble internal objects.

In that candidate, define `BLINKER_ENABLE_TASKS 1` before `#include <Blinker.h>`.
Register the usual write callback, then call `Blinker.enableTasks(power)` before
the unchanged `Blinker.begin(key, ssid, password, power, ...)`. Check both results.
This selects the allowed scalar Properties and enables the existing business
clock; it does not initialize owner authority or create/start a task. The App
still requires explicit owner initialization and confirmed management results.

Targets must be global/long-lived, writable, have a write callback, and also be
passed to `begin()`. Ordinary writable Properties are not automatically allowed.
Repeated configuration is rejected, including after `end()`. Disabled products
return `UnsupportedFeature`; BLE and WiFi+BLE persistent tasks are not admitted.
Defining the opt-in alone cannot admit R4/Wio: their separate internal guards,
storage ownership and matching Ed25519 signer must also be satisfied. No task
graph is linked when the compile-time opt-in is absent.

R4 and Wio manual-WiFi four-slot candidates have passed finite internal task
acceptances using the same Store/Codec/Engine and App management chain. They
are not ordinary release profiles or a production/long-term resource admission.
R4 uses the shared modem owner and existing Preferences bank; synchronous SDK
calls are not preemptible by a wrapper deadline. Wio uses one onboard QSPI owner:
the final 16 KiB is reserved (8 KiB tasks followed by 8 KiB identity), even when
tasks are off. An independent onboard QSPI/filesystem driver cannot be added to
that product graph. A blank region does not establish ownership of an unknown
firmware's storage.

Unknown writes are queried/reconciled, not automatically replayed. Restore
identity, authorization and task scope consistently; restoring empty local
storage alone does not retire a cloud scope. Maintainer admission and recovery
steps are in [the task execution plan](docs/DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md#11-r4-与-wio-新能力适配). None of these pilots guarantees offline UTC after a reboot.

## Current platform matrix

| Board family | WiFi | BLE | WiFi+BLE provisioning |
|---|---|---|---|
| ESP32 | device API and hardware E2E complete; production config pending | App/Service/device hardware E2E passed | BLE WiFiProv and Direct hardware E2E passed; release profile pending |
| UNO R4 | device API and hardware E2E complete; production config pending | App/Service/device hardware E2E passed | not planned for the current release |
| Wio Terminal | device/cloud hardware E2E passed | experimental opt-in; hardware E2E passed, not released | not planned for the current release |

The following capabilities share the existing API; they are not separate
Arduino packages:

| Capability | ESP32 | UNO R4 WiFi | Wio Terminal |
|---|---|---|---|
| Lease-driven realtime sampling | WiFi/BLE finite hardware acceptance | WiFi/BLE finite hardware acceptance | WiFi and experimental BLE finite hardware acceptance |
| Cloud H1 state-change history | existing Cloud State chain | ordinary WiFi finite hardware acceptance | ordinary WiFi finite hardware acceptance |
| Sketch periodic/delay Timer | optional, RAM-only | optional, RAM-only | optional, RAM-only |
| App-managed persistent tasks | closed manual-WiFi four-slot pilot | internal manual-WiFi four-slot candidate | internal manual-WiFi four-slot candidate with reserved QSPI region |

H1 stores state changes received by the cloud, not all realtime samples or
offline device history. Pure BLE direct operation does not upload Cloud H1 by
itself. All ordinary products retain persistent-task capacity 0. Finite
acceptance covers the stated workload, not full transient resource peaks or
every third-party peripheral combination.

Wio Terminal BLE development builds must define
`BLINKER_ENABLE_WIO_BLE_PREVIEW=1` before including `BlinkerBLE.h`. The opt-in
does not imply production support. Enrollment, DirectSecure, controls, same-image
reflash and a real USB power cycle have passed; long-running reconnect and
single-connection denial-of-service hardening remain before the macro is removed.

WiFi uses DeviceKey challenge-HMAC, a short SessionCredential, PubSubClient
and BBP/2. Plain HTTP/MQTT is the constrained-device default; TLS remains an
explicit release configuration. BLE uses the bounded BLE/Noise/Direct runtime.
ESP32 WiFi+BLE uses BLE WiFiProv Security1 for first setup, then runs WiFi
cloud and ESP-IDF NimBLE Direct against one Manifest/state model. The education setup
window has no unique PoP and therefore does not claim active-MITM resistance.

## Repository structure

```text
src/BlinkerV2/                 platform-independent Core
src/BlinkerV2/arduino/internal Product composition
src/BlinkerV2/arduino/ports/   SDK/external-library Ports
src/BlinkerV2/arduino/platform platform binding
examples/                      four public Arduino examples
tests/internal/                unreleased probes only
```

Mode-specific SDK Port implementations use private `.ipp` files included only
by the selected product header. This prevents an unselected third-party WiFi or
BLE SDK from entering dependency discovery. Arduino still compiles every
ordinary `.cpp` below `src`, so the shared Core translation-unit count remains
a release-time compile-cost issue; link-map/ELF checks prove firmware closure,
not compile-time closure.

The repository root is the complete Arduino library. Public releases contain
one `library.properties`, one `src/` and one `examples/`; users never install
Core, Port or platform packages separately.

The current external dependency set is PubSubClient 2.8, ArduinoBLE 2.0.2,
Crypto 0.4.0 and the pinned Seeed rpcWiFi/rpcBLE/rpcUnified libraries. ESP32
BLE uses the NimBLE Host already supplied by ESP-IDF. Arduino has no conditional
dependency field, so the unified library declares the remaining flat set while
archive linkage prevents the unselected product graph from entering firmware.

When installing a ZIP locally, Arduino CLI does not resolve `depends` for you.
Install the required pinned libraries once through Library Manager; do not copy
additional Blinker Core/Port/product ZIPs. ESP32 BLE and WiFi+BLE also require an
Arduino core built with ESP-IDF NimBLE enabled and the admitted SDK configuration.
A stock core using Bluedroid is not equivalent, and installing NimBLE-Arduino
does not replace that SDK requirement. Ordinary ESP32 WiFi does not require BLE.

The current ESP32S3 BLE/WiFi+BLE baseline is the **official Arduino-ESP32
3.3.11 (ESP-IDF 5.5.5)** Board Manager package, not a private SDK or an additional
NimBLE-Arduino install. Its S3 SDK enables NimBLE with a 2048-byte IPC stack and
5120-byte Host stack. CI checks that SDK's configuration/binary digests, then
compiles both BLE products from the same ZIP used for the five WiFi/portable
targets (ordinary ESP32 WiFi retains the stock 3.3.6 baseline). This is a tested
baseline, not an assertion that every earlier SDK fails or every ESP32 variant
has passed hardware acceptance. See the
[official release](https://github.com/espressif/arduino-esp32/releases/tag/3.3.11)
and `docs/DEVICE_V2_C4_REPRODUCIBLE_DEPLOYMENT_PLAN.md` for the remaining gates.

BLE packet negotiation, integer-step validation, Sketch Timer and finite
platform acceptances are recorded in
[the current task and platform execution plan](docs/DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md).
Source/install compile gates and production admission remain separate. Restored
test boards do not necessarily retain the accepted candidate firmware.
Ordinary LAN/P1 and unadmitted persistent-task modes stay disabled.

Production publication separately requires the declared build/reliability gates,
real service configuration and matching public server key rings. The repository
defaults are development templates, not production trust material. See
`docs/DEVICE_V2_STANDARD_ARDUINO_LAYOUT.md`,
`docs/DEVICE_V2_CANONICAL_API.md`,
`docs/DEVICE_V2_DEVICE_KEY_CONTRACT.md`, and
`docs/DEVICE_V2_ARCHITECTURE_CLOSURE_AUDIT.md`.
