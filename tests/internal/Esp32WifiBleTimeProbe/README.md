# WiFi+BLE business time hardware gate

Internal test, excluded from the Arduino release. Build against a fresh installed
development-profile ZIP, preserving the adjacent `support/ObservedTime.h` path.
This is the same `Esp32OfficialWifiBleProduct` selected by `BlinkerWiFiBLE.h`,
not a substitute manual-WiFi graph. It owns one real `TimeSyncService`; the shared
test observer only counts accepted samples. No new socket, grant or time injection.

The probe loads the existing access root and WiFi profile. Without a DeviceKey it
starts normal BLE WiFiProv: use the signed-in App and exact instance/advertisement.
An existing BLE identity must use the normal confirmed cloud-enable flow, not a
new identity, fake credential, or silent NVS reset. Provisioning writes credentials
through the real product. Record those intended changes separately from flashing.

Before flashing, verify MAC, back up and hash the current application. Write only
the application partition. `s` at 115200 returns bounded status on demand; there
are no erase, reset-access, or periodic logging commands. The normal product
loop, Hello, Method 2 and DirectSecure remain unchanged.

For the checkpoint-selection gate only, `BLINKER_TEST_NETWORK_RESET` enables one
`n` command per boot. It calls real `Product::resetNetwork()` to re-enter WiFiProv
while preserving DeviceKey and Direct access. It intentionally clears the WiFi
profile; use the signed-in App to restore the known network. Do not send it during
a command/authorization transaction. It is absent in the default probe.

Verify accepted cloud time while BLE Property/Action remains usable, then close
BLE and verify cloud control on the original account MQTT. Restore the exact
original application and verify it after testing. Report real hardware separately
from compilation; this is not a Hub/child, protected-storage, expiry or all-MCU gate.
