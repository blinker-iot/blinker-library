# Blinker Device V2

This working tree contains the platform-independent Device V2 core. The legacy
implementation and legacy examples were removed from the working tree because
Arduino recursively compiles a library's complete `src` directory. The current
work is local and uncommitted on top of `origin/dev_edu`; no remote
`device_v2` branch is currently published.

Current scope:

- ordinary `<Blinker.h>` facade with global `Blinker`, one `begin()`, compile-time
  WiFi/BLE/WiFiBLE tags, void callbacks and atomic multi-Property reporting;
- Advanced owning `Device` with build-wide Small/Standard/Large storage profiles,
  plus caller-owned `ClientStorage`/`Client` for exceptional budgets;
- Property/Action/Event declarations that generate immutable capability
  descriptors and compose through a zero-copy `DeviceSchema`;
- scalar endpoint handles, typed command values, fixed-capacity state, and
  zero-JSON patch reporting;
- platform-independent result/span primitives;
- authentication-independent session-provider and MQTT dependency interfaces;
- complete-frame transport interface and fixed-capacity transport hub;
- MQTT frame transport with non-blocking reconnect/backoff;
- BBP/2 base header encoder/parser;
- bounded, definite-length CBOR codec with no DOM or heap allocation;
- typed HELLO/ACK/ERROR and versioned PATCH bodies;
- state snapshot/apply conflict handling and keyed COMMAND/EVENT visitors;
- fixed-capacity endpoint registry and static capability manifest;
- an existing `device-cloud-session/2`/CloudCredential software experiment that
  will be replaced in the Education composition by DeviceKey challenge-HMAC and
  versioned short-term MQTT credentials;
- managed MQTT login/data-plane composition and multi-transport device runtime;
- platform-independent device-instance identity and independent crash-atomic
  Ownership, Controller, network and experimental CloudCredential records;
- old Enrollment/1, ProductOrchestrator and stored permanent Setup-PSK source
  have been deleted. Optional P-256 developer identity remains independent of
  ordinary onboarding and is excluded from official WiFi compositions;
- explicit `ClientStatus` with separate local-ready and cloud-ready state;
- no Arduino, JSON, MQTT, WebSocket, BLE, or MCU SDK dependency.

Use `<Blinker.h>` for ordinary application code. It provides the global
`Blinker` facade, typed Property/Action/Event interactions and compile-time
WiFi/BLE/WiFiBLE profile tags. Internal probes, adapter authors and products
that deliberately own `Device`, `Client`, transports or buffers use
`<BlinkerV2Advanced.h>`.

The repository root remains the engineering source tree. `Blinker.begin(...)`
needs one linked platform composition. ESP32 and UNO R4 have staging WiFi software
graphs, not released onboarding products. The current priorities are a no-prefix
DeviceKey with hard session revocation, followed by ESP32 WiFiProv Security1.
UNO R4 currently has WiFiS3 and an encrypted-ArduinoBLE/portable-Noise internal
graph but still lacks hardware admission; Wio Terminal has no wireless product
adapter in this repository. See `docs/DEVICE_V2_DEVICE_KEY_CONTRACT.md` and
`docs/DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md`.

The public profile defaults are Small `24/256/192/192`, Standard
`32/1024/512/512`, and Large `64/4096/2048/2048` for
fields/frame/patch/total-state-arena. Select a non-default profile with a
build-wide `BLINKER_RESOURCE_PROFILE=1/2/3` definition, never a sketch-local
macro. A normal sketch declares a typed Property, registers a void callback, and
calls `Blinker.begin(profile, power, ...)`. A Sketch never asserts productId.
Headless WiFi-only targets `blinker::WiFi.onboard(ssid, password, deviceKey)`
with a long-lived, user-revealable DeviceKey. ESP32 App provisioning will install
the same DeviceKey through WiFiProv Security1 plus a short-lived EnrollmentTicket.
The current source parameter is still named `enrollmentKey` and is scheduled for
the DKEY cutover; it is not a compatibility contract. Explicit Schema,
`Device`, `Client`, `Product` and checked callbacks remain Advanced APIs.

During development, compile sketches with the repository root as the library.
The package name remains `BlinkerDeviceV2` until the App/server BBP/2 path,
device migration facade, and hardware release gates are complete; naming it
`Blinker` earlier would falsely advertise a production-compatible replacement.

Run `tools/build_device_v2_packages.py` from the repository root to validate
the engineering package boundary and create standalone ZIPs. Every package
listed in `release/device-v2-packages.json` is an independent standard Arduino
package used for adapter development and CI; it is not the final ordinary-user
distribution contract. External libraries such as PubSubClient, ArduinoBLE
and NimBLE-Arduino remain separate, pinned dependencies and their source is
never copied into this repository. ESP32 and UNO R4 WiFi
use their small platform packages under `adapters/esp32-wifi` and
`adapters/renesas-uno-wifi`; neither implementation is compiled for unrelated
MCUs. `examples/AdvancedCoreModel/AdvancedCoreModel.ino` is explicitly a
transport-independent Advanced model probe and does not claim cloud or BLE
connectivity. Product network QuickStarts are added only with their real static
platform composition. The ESP32 WiFi aggregate already ensures that
`<Blinker.h>` links the selected composition; other platform tags remain gated
until their equivalent package exists.

`BlinkerEsp32StorageAdapter` maps bounded credentials to committed ESP-IDF NVS
blobs without importing WiFiProv, BLE or MQTT. An encrypted store now requires
a product-owned `Esp32NvsEncryptionVerifier`; a boolean flag alone cannot make
plain NVS report `EncryptedFlash`. Identity, portable Noise provisioning and
radio composition remain separate packages so installing storage never owns a
radio implicitly.

The existing ESP32 pre-enrollment generator and CloudCredential records are
unreleased experiments. They may remain as Enhanced/factory evidence while the
DeviceKey composition is implemented, but are not the Education QuickStart or
the first App provisioning path.

The old `BlinkerEsp32IdentityAdapter` and its P-256-derived Developer identity
were removed from the active package set. Enhanced/manufactured products may
later inject an attestation provider, but ordinary WiFi/BLE/WiFiBLE firmware
uses the same public device-instance, ownership and credential contracts and
does not acquire an ESP32-specific identity API.

`BlinkerHardwareRandomAdapter` exposes ESP32 `esp_fill_random()` and UNO R4
Renesas SCE RNG through one portable `PlatformHardwareRandom`. It is separate
from BLE/Noise so a WiFi-only product can generate cloud-session request IDs
without importing a radio stack. The same injected `IRandom` seam is reused by
provisioning code; Arduino `random()` is not an acceptable security source.

The current `HttpCloudSessionProvider -> ManagedMqttTransport`, EnrollmentKey,
CloudCredential enrollment and portable Noise flows are software-tested staging
implementations. Education will replace their credential root with a 32-byte
DeviceKey, exact challenge-HMAC, `credentialVersion`, short SessionCredential and
broker hard disconnect on rotation. ESP32 provisioning will first use an internal
`network_prov_mgr` Security1 adapter and one bounded custom endpoint; Security2 and
Noise remain optional Enhanced profiles. Gateway/App production support, shared
DeviceKey vectors, Security1 final-composition resource probes and hardware E2E are
release blockers. Existing Ownership/Controller/BBP2 contracts remain reusable;
see `docs/DEVICE_V2_DEVICE_KEY_CONTRACT.md`,
`docs/DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md` and
`docs/DEVICE_V2_OWNERSHIP_RELEASE_CONTRACT.md`.

Properties use stateful `report()` backed by the bounded `EndpointStateStore`.
Writable Properties receive desired values through `onWrite()`, Actions receive
one-shot invocations through `onInvoke()`, and Events use `emit()` without
mutating state. App widgets, layout, style and bindings are deliberately absent
from the firmware and Manifest. See `docs/DEVICE_V2_CANONICAL_API.md`,
`docs/BBP2_NORMATIVE_WIRE_SPEC.md` and `docs/DEVICE_V2_REFACTOR_PROGRESS.md`.
