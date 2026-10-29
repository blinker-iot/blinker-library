# Blinker Device V2

Blinker Device V2 is one Arduino library for BBP/2 devices. A single install
contains the shared component/protocol runtime, ESP32 and UNO R4 WiFi/BLE,
and Wio Terminal WiFi implementations. Wio Terminal BLE is an explicit
experimental preview and is not part of the current release claim.

Release preparation and current Aily Builder validation are tracked in the
[library release execution plan](docs/DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md).

For the current cloud beta ZIP, start with [its installation steps below](#install-the-cloud-beta-candidate):
**ESP32 manual WiFi, TLS, zero persistent-task slots only**. The generic BLE,
R4/Wio and task examples in this single library do not enable those modes in
that restricted profile. A GitHub source ZIP or a bare clone contains development
configuration and is not the configured cloud candidate.

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
profile and ordinary communication examples do not enable them. The closed ESP32 manual-WiFi
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
Defining the opt-in alone cannot admit R4/Wio: their separate build guards,
storage ownership and matching Ed25519 signer must also be satisfied. No task
graph is linked when the compile-time opt-in is absent.

The maintained UNO R4 manual-Plain-WiFi four-slot **fixed graph** has passed
limited admission using the same Store/Codec/Engine and App management chain.
[R4TasksQuickStart](examples/R4TasksQuickStart/README.md) is its single public
reference: `BlinkerWiFi.h`, `enableTasks(power, duty)` and the unchanged
`begin()` / `run()`. It requires the protected build profile and exact Service
device-ID admission; an ordinary IDE build rejects this reference with an
explanation instead of silently running with zero slots. The ordinary library
still defaults to zero, and no second library ZIP is needed.

Maintainers use the [fixed R4 build/support contract](docs/DEVICE_V2_R4_RESTRICTED_TASK_ADMISSION.md).
The generator consumes that same reference and keeps credentials private. The
profile binds the original SDK overlay, all-consumer configuration, heap adapter
and link flags; a Sketch macro cannot supply them. The maintainer's
[IDE profile installer](docs/DEVICE_V2_R4_ARDUINO_PROFILE_INSTALLATION.md) adds a
separate local Tasks4 Plain board alongside the official board, without editing
the installed SDK or this library. Select it explicitly for this reference;
select the official board for ordinary default-zero tasks. This is not task
support for arbitrary Sketches. The shared modem owner latches faults and stops
service; its 1000 ms RPC budget is not an arbitrary-callback hard-return SLA.
Manual cold recovery and fresh reconciliation, not automatic retry, are required.

Wio remains an internal manual-WiFi four-slot candidate, with further admission
paused. It uses one onboard QSPI owner:
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
| ESP32 | device API and hardware E2E complete; restricted cloud WiFi profile available below | App/Service/device hardware E2E passed; full public profile pending | BLE WiFiProv and Direct hardware E2E passed; release profile pending |
| UNO R4 | device API and hardware E2E complete; public profile pending | App/Service/device hardware E2E passed; public profile pending | not planned for the current release |
| Wio Terminal | device/cloud hardware E2E passed; public profile pending | experimental opt-in; hardware E2E passed, not released | not planned for the current release |

The first-release source scope is frozen as six ordinary modes / seven compile
targets, not every board covered by the architecture tags. SDK baselines,
tool-specific evidence and the narrower publicly configured environment are
separate in [release plan R3](docs/DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#9-r3-首发边界与公开配置冻结).
The only current public cloud profile is ESP32 manual WiFi with zero persistent
task slots; it does not enable R4/Wio cloud release or BLE enrollment.

The following capabilities share the existing API; they are not separate
Arduino packages:

| Capability | ESP32 | UNO R4 WiFi | Wio Terminal |
|---|---|---|---|
| Lease-driven realtime sampling | WiFi/BLE finite hardware acceptance | WiFi/BLE finite hardware acceptance | WiFi and experimental BLE finite hardware acceptance |
| Cloud H1 state-change history | existing Cloud State chain | ordinary WiFi finite hardware acceptance | ordinary WiFi finite hardware acceptance |
| Sketch periodic/delay Timer | optional, RAM-only | optional, RAM-only | optional, RAM-only |
| App-managed persistent tasks | closed manual-WiFi four-slot pilot | maintained fixed Plain four-slot profile only; ordinary default 0 | internal manual-WiFi four-slot candidate with reserved QSPI region; paused |

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
examples/                      four ordinary examples + profile-gated R4 task reference
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

### Restricted cloud beta profile

Profile selection, current environment and key-role boundaries are documented in
[the public profile index](profiles/README.md).

#### Install the cloud beta candidate

Library version: **0.1.0**; embedded firmware label: `device-v2-0.1.0`.
Use the candidate's `BlinkerDeviceV2-0.1.0.zip` together with its own `SHA256SUMS`.
Check the ZIP before installing: run `sha256sum --check SHA256SUMS` in the download
directory on Linux, or compare `Get-FileHash .\BlinkerDeviceV2-0.1.0.zip -Algorithm SHA256`
with the supplied file in PowerShell. A checksum detects byte changes; it is not
a signature. Do not compare a new build with an older candidate's checksum.

1. Keep existing Sketches and credentials. Move older Blinker, split Core/Port
   packages or duplicate V2 copies **outside the Sketchbook `libraries` directory**;
   renaming them inside that directory does not remove the `Blinker.h` conflict.
   Do not delete your sketches, erase device Flash or reset authorization.
2. Use the already admitted **Espressif Arduino-ESP32 3.3.11** core and install
   **PubSubClient 2.8**. `WiFi`, `Network` and `NetworkClientSecure` are supplied
   by this core; do not install third-party replacements with those names.
   No NimBLE-Arduino, ArduinoBLE, Crypto or Seeed library is required for this
   restricted WiFi path. This cloud baseline is separate from the ordinary
   Plain WiFi 3.3.6 baseline; do not switch SDKs to reuse unrelated evidence.
3. In Arduino IDE, select **Sketch > Include Library > Add .ZIP Library** and
   install only this configured ZIP. Select **ESP32 Dev Module**
   (`esp32:esp32:esp32`, default 4 MB flash / default partition).
4. Open **File > Examples > BlinkerDeviceV2 > WiFiQuickStart** (or
   **CloudSmokeTest**). Use a WiFi device's DeviceKey from the matching Blinker
   beta environment, and fill only that key, SSID and password in your private
   Sketch. The device needs a reachable 2.4 GHz network. Do not change internal
   service hosts, CA headers or key rings; keep calling `Blinker.run()` in `loop()`.
5. Enable verbose compilation and confirm the selected `Blinker.h` comes from
   this V2 installation. If Arduino reports **Multiple libraries were found**,
   inspect the **Used / Not used** paths and resolve duplicates before uploading.
   Compile first; changing this library does not require clearing device storage.

The single library's `depends` metadata is a **flat multi-platform set**, not a
per-mode installer. A Library Manager dependency prompt may offer libraries used
by other modes; that is not additional cloud support. Manual ZIP/CLI installation
does not guarantee dependency resolution. For this candidate, install the explicit
WiFi dependency above rather than relying on automatic installation of the whole set.

Equivalent CLI installation (the ESP32 core must already be installed):

```sh
arduino-cli lib install "PubSubClient@2.8"
arduino-cli lib install --zip-path BlinkerDeviceV2-0.1.0.zip
arduino-cli compile --fqbn esp32:esp32:esp32 /path/to/your/WiFiQuickStart
```

CLI may require `ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL=true` for a local ZIP;
enable it only for the checksum-verified artifact. No account or device Key is
needed to compile the unchanged placeholder examples. Detailed Chinese guidance
is in [the installation guide](docs/DEVICE_V2_ESP32_CLOUD_BETA_INSTALLATION.md).

#### Maintainer build and acceptance scope

Maintainers can build the **same single library** for the currently admitted
ESP32 manual-WiFi cloud path without copying a private stage or inventing
unused BLE/task signing keys:

```sh
python -B tools/build_arduino_release.py --profile profiles/cloud-beta.json --stage-dir build/cloud-beta/stage --output-dir build/cloud-beta/artifacts
```

Use fresh/empty output locations; this command does not overwrite the repository
source defaults. Install the generated ZIP once, then use `WiFiQuickStart` or
`CloudSmokeTest` with that environment's DeviceKey and your WiFi credentials.
The public profile selects HTTPS `iot.yiyu.pro:443` and MQTTS with the accepted
public Sectigo E46 root; MQTT host/port still come from the authenticated cloud
session (`mqtt.yiyu.pro:8883` in this deployment). It contains no account,
DeviceKey, WiFi credentials, private keys or RFC fixture signing keys.

This is an internal **ESP32 WiFi, zero-task-slot** profile, not a general
multi-platform cloud release. BLE/WiFiBLE, provisioning, LAN, gateway and tasks
fail closed; ordinary self-hosted profiles and the R4 fixed Plain profile are
unchanged. `channel: production` applies the existing strict configuration
validator; it does not claim production release, new firmware runtime acceptance
or R4/Wio TLS admission. The seven-target baseline remains a separate full-profile
gate; this profile's installed targets are only `esp32-wifi` and
`esp32-cloud-smoke`. Exact package evidence, version/checksum records and the
manual candidate CI boundary are in the
[library release plan](docs/DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md).

### Optional device location input (current source)

User code owns the GPS/other device driver and parsing; no GPS dependency is
installed by Blinker. Use the same configured library, not an additional ZIP:

```cpp
#include <Blinker.h>
BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_LOCATION(position);

void setup() {
    Blinker.begin("YOUR_DEVICE_KEY", "YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD",
                  power, position);
}

// Call from your parser only for a new WGS84 observation, after begin succeeds.
bool onNewPosition(double latitude, double longitude, uint32_t sampledAtMillis) {
    return Blinker.location(position, latitude, longitude, sampledAtMillis);
}

void loop() { Blinker.run(); } // Advance your nonblocking parser here as well.
```

Latitude comes first, longitude second, in degrees. `sampledAtMillis` is the
original sample tick from this device's boot-local `millis()` clock, not GPS UTC
or a peripheral clock. The overload without a tick means a new observation now;
do not repeatedly restamp an old point. The default maximum sample age is 30 s
and device-side interval floor is 1 s; the authorized Cloud lease controls sends.

A successful call accepts the local snapshot only, not an upload/delivery ACK.
Read `Blinker.lastError()` on failure. Invalid values do not overwrite a fix;
`(0,0)` is a valid coordinate, not a loss marker. `Blinker.locationLost(position)`
suppresses new sampling; `Blinker.clearLocation(position)` also clears local
sample ordering. Neither deletes the cloud's last position.

`blinker::LocationFix` overloads retain E7 coordinates and optional accuracy/UTC.
Unknown accuracy/time remain unknown; they never synchronize a clock. Keep
Location/Spec alive for the session and call inputs serially with `Blinker.run()`,
not from an IRQ. The existing exact Manifest/owner admission must match before
Cloud location can be used: declaration or successful compilation is not a grant.

For advanced metadata examples, a source checkout also contains
`docs/DEVICE_V2_LOCATION_INPUT_API.md` (not bundled in the ZIP). The instructions
above work without that document.
This optional source API does not add a GPS dependency, change the frozen R5 ZIP,
or open new BLE/LAN/platform/location-sharing support.

The manual cloud-candidate workflow keeps its two ordinary targets by default.
Maintainers claiming location-input validation must also select
`verify_location_input`; it verifies the same ZIP in a separate sketchbook.
That option does not enable location on a device or run a physical GPS test.
The existing `device_v2` push quality workflow also calls this same candidate
workflow with location-input verification enabled, after its host/package gates
pass. Both workflows use the same commit; no default-branch update is required.
