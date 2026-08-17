# Blinker ESP32 BLE Adapter

This optional library maps the ESP32 Arduino core BLE server to `IBleLink`.
It uses separate RX (write/write-without-response) and TX (notify) 128-bit
characteristics and leaves BBP/2 fragmentation to `BleFrameTransport`.

Important constraints:

- the adapter owns the ESP32 Arduino BLE singleton between `start()` and
  `stop()`;
- one active central is supported by this first adapter;
- ESP callbacks copy writes into a fixed queue; protocol callbacks run from
  `poll()`, not from the BLE task;
- the default queue accepts four packets of at most 244 bytes; both limits are
  compile-time macros;
- notification completion/backpressure is not exposed by the ESP32 Arduino BLE
  API, so use `BleFrameTransportConfig::maxPacketsPerPoll = 1` on hardware;
- LE Secure Connections encryption is mandatory and bonding is enabled by
  default; the adapter publishes no Device V2 session until encryption and
  notification subscription are both confirmed by the ESP32 BLE stack;
- `setSessionSecurity()` cannot change the stack-reported encryption or bond
  facts; it can only record later application authentication;
- security and application authentication reset on every disconnect.

The link implements `IBleModeLink` and publishes the same strict 13-byte
Provisioning/Direct Service Data used by the NimBLE and ArduinoBLE adapters.
Profiles can change only while stopped. Mode switching disconnects the current
central and restarts advertising. This Bluedroid path uses substantially more
ESP32 flash than NimBLE and is not selected by the official WiFi+BLE
composition; it must not run beside a second BLE-stack owner in one product.
