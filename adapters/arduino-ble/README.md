# Blinker ArduinoBLE Adapter

This optional library maps ArduinoBLE to `IBleLink`. Its first verified target
is UNO R4 WiFi/Renesas. It uses the same service and separate RX/TX
characteristics as the ESP32 adapter.

Important constraints:

- ArduinoBLE remains an external dependency; it is not copied into the Blinker
  core library;
- the default packet size is 20 bytes because ArduinoBLE does not expose the
  negotiated ATT MTU uniformly on supported boards;
- `BLE.paired()` is not used because ArduinoBLE 2.0.2 implements it as an
  any-peer query. Products that require encryption inject a platform security
  source for the exact connected central; without one, `start()` fails closed;
- application code cannot assign encrypted, bonded or authenticated flags.
  Pairing is link protection, while Controller Method 2 provides Device V2
  identity authorization;
- security state is reset on disconnect or encryption loss;
- one central is supported by ArduinoBLE's peripheral API.

The link also implements `IBleModeLink`. Configure a validated Provisioning or
Direct `ModeProfile` only while stopped; `start()` publishes the common Device
V2 service UUID plus the exact 13-byte mode Service Data in the 31-byte scan
response. The scan response has no room for a duplicate local name, so clients
must discover by UUID and strictly decode Service Data. Switching modes stops
the old endpoint and requires the central to scan and reconnect.
