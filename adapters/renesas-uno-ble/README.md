# Blinker UNO R4 ArduinoBLE security source

This optional adapter is the narrow platform-specific seam required by the
UNO R4 WiFi official WiFi+BLE composition. It pins ArduinoBLE 2.0.2, resolves
the exact central to an ATT handle once in the connection callback, then reads
that handle's encryption bit without allocating in each `poll()`. It does not
use the unsafe any-peer `BLE.paired()` shortcut.

ArduinoBLE 2.0.2 does not expose an equivalent per-connection bond fact, so
the adapter reports `bonded=false`. Device V2 Direct BLE still requires the
real encrypted link plus Controller Method 2 authorization.

This adapter must be re-audited before changing the pinned ArduinoBLE version.
