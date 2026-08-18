# Blinker ESP32 WiFiProv adapter

This optional ESP32-only package wraps the low-level Espressif Network
Provisioning manager with BLE Security1. It exposes one bounded
`blinker-config` endpoint for the portable DeviceKey provisioning wire and
copies received Wi-Fi credentials into an injected Blinker credential journal.

The adapter deliberately does not implement cloud registration, MQTT or
BBP/2. Product composition supplies those independent layers after local
provisioning completes.

Constraints:

- the proof-of-possession string is required;
- requests are fixed at 2 or 45 bytes and responses at 3 or 20 bytes;
- the only required allocation is Espressif's response buffer, capped at
  20 bytes and released by Protocomm;
- Arduino Wi-Fi persistence is disabled so the Blinker journal remains the
  only durable network credential source;
- BLE memory release is selected by the product: WiFi-only can release it,
  while WiFi+BLE must retain the host;
- Security2 and Noise remain optional enhanced profiles, not hidden
  dependencies of this adapter.

The package requires Arduino-ESP32 with native Wi-Fi, Network Provisioning
Wi-Fi support and Protocomm Security1 enabled.
