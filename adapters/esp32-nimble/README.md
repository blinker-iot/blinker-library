# Blinker ESP32 NimBLE adapter

This optional package implements `IBleLink` with NimBLE-Arduino 2.5.0. It is
the preferred ESP32 product candidate when BLE and WiFi/TLS/MQTT are linked in
the same firmware. The older `esp32-ble` package uses Arduino-ESP32's bundled
Bluedroid BLE API and remains available for compatibility and diagnostics.

The dependency is isolated in this adapter package; `BlinkerDeviceV2` does not
include or own a BLE stack. Keep NimBLE-Arduino pinned to 2.5.x until a newer
major/minor has passed the compile, resource and hardware-admission probes.

Like the Bluedroid adapter, callbacks only copy bounded events into fixed
queues. BBP/2 parsing and application callbacks run later from `poll()`.

The link implements `IBleModeLink`: a Provisioning or Direct profile is copied
only while stopped and is encoded as the common Device V2 UUID plus exact
13-byte Service Data. A mode change uses the same NimBLE adapter/host stack but
stops the old endpoint and disconnects the central before restart; App code
must rescan and reconnect. Provisioning and Direct are never two concurrent
callback owners and are not selected by payload magic.

The adapter enforces the BBP/2 security boundary instead of asking product
code to assert it manually:

- RX writes and TX subscription/read require an encrypted GATT link;
- the default is no-I/O LE Secure Connections with bonding, while BBP/2
  Method 2 supplies installed ControllerCredential identity authentication;
- `NimBLEConnInfo` is the source of truth for encrypted/bonded state;
- a session is not announced to `BleFrameTransport` until encryption and TX
  subscription have both completed; failed encryption disconnects the peer.

`setSessionSecurity()` cannot manufacture encryption or bonding. It accepts
those arguments only when they match the current NimBLE connection and may
then promote the separate application-authenticated bit.

For the original ESP32 dual WiFi/TLS/BLE product, use these as build-wide
compiler definitions (they must reach NimBLE's own translation units):

~~~text
CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED=1
CONFIG_BT_NIMBLE_ROLE_OBSERVER_DISABLED=1
CONFIG_BT_NIMBLE_CRYPTO_STACK_MBEDTLS=1
CONFIG_BT_NIMBLE_MAX_CONNECTIONS=1
CONFIG_BT_NIMBLE_MAX_CCCDS=3
CONFIG_BT_NIMBLE_LOG_LEVEL=5
CONFIG_NIMBLE_CPP_LOG_LEVEL=0
~~~

This profile does not remove peripheral, broadcaster, SMP, LE Secure
Connections or bonding. Definitions in an application header are too late:
Arduino builds the external NimBLE library in separate translation units.
The product probe carries the flags in `build_opt.h`; aily-builder 1.3.0 must
also receive them explicitly through repeated `--build-macros` due to its
prebuild/dependency-analysis race.
