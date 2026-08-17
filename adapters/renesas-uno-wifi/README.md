# Blinker Renesas UNO Wi-Fi adapter

`RenesasUnoWifiStation` implements `IWifiStation` for UNO R4 WiFi and the
Arduino `WiFiS3` library. While active it owns the global station lifecycle;
sketches must not call `WiFi.begin()` or `WiFi.disconnect()` concurrently.

WiFiS3 1.5.3 normally waits up to 10 seconds inside `WiFi.begin()`. The adapter
sets its connection wait timeout to zero before every start, so `begin()` only
sends the modem commands and association/DHCP progress is observed by `poll()`.
The zero setting intentionally remains in force because WiFiS3 exposes no
timeout getter and restoring an assumed value would overwrite application
state.

Each WiFiS3 command is still a synchronous UART transaction and can wait for
the connectivity module's internal command timeout if that module is
unresponsive. This is a platform SDK limitation, not a fully asynchronous
driver guarantee, and must be measured by the UNO R4 hardware release gate.

SSID and password are copied only into temporary NUL-terminated stack buffers;
WiFiS3 formats them into the `BEGINSTA` command synchronously and the adapter
wipes both buffers immediately after the command returns.

WiFiS3's high-level `begin()` cannot require a specific WPA3/transition
authentication mode and documents a 63-character passphrase limit. This
adapter therefore supports Open and WPA2-Personal profiles and returns
`UnsupportedFeature` for WPA3, mixed-mode declarations and 64-hex raw PSKs.
That is an explicit platform capability boundary, not a silent security
downgrade.
