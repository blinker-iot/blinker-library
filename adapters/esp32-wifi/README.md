# Blinker ESP32 Wi-Fi adapter

`Esp32WifiStation` implements the portable `IWifiStation` boundary with the
Arduino ESP32 `WiFi` singleton. The adapter owns station connection lifecycle
while it is active; sketches must not call `WiFi.begin()` or
`WiFi.disconnect()` concurrently.

The ESP32 core copies SSID and credentials into `wifi_config_t` before
`WiFi.begin()` returns. This adapter therefore uses short-lived, NUL-terminated
stack buffers and wipes them immediately instead of retaining another password
in the adapter object.

`WiFi.begin()` can wait up to about one second for the ESP network interface to
start, but it does not wait for association or DHCP. Those states are observed
by `poll()`. Terminal connect failure and missing-radio states are surfaced as
`Failed`; the portable `WifiConnectionLifecycle` owns timeout and retry policy.

The adapter applies an Open, WPA2 or WPA3 minimum-security floor before
starting. `Wpa2Wpa3Personal` uses the WPA2 floor so both WPA2 and transition
networks remain usable; it never lowers WPA3-only configuration to WPA2.
