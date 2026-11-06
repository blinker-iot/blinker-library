# XIAO ESP32S3 SoftAP four-slot task reference

This is the fixed restricted profile, not task support for arbitrary ESP32
Sketches. It requires the original ESP32 Core **3.3.11**, the **XIAO ESP32S3**
board with its reviewed 8 MiB partition, and the matching maintainer-generated
SoftAP/TLS Tasks4 library. Use the normal Arduino IDE Verify/Upload buttons;
do not edit the SDK or add Sketch compiler flags. The ordinary library and
`cloud-beta.json` remain **default zero slots**. Install only one Blinker library,
not normal and restricted copies side by side.

1. Install the exact restricted library ZIP and pinned dependencies identified
   by its delivery instructions and SHA256SUMS. A source ZIP or ordinary
   cloud-beta ZIP is not this profile; this example rejects those selections.
2. Open this example and **Save As** a personal Sketch before editing it.
3. In that personal `UserProvisioning.h`, replace the only public placeholder
   with the device's own **nonempty 1-64 UTF-8 byte PoP**. The unchanged
   placeholder, empty text and overlong text are rejected at compile time.
   Do not reuse a shared default, DeviceKey, WiFi password or hotspot password
   as PoP, and do not commit or distribute your personal header.
4. Keep `UserProvisioning.h` before the product header. The root library entry
   loads the generated library-wide profile before the API and bearer selection;
   defining task macros in a Sketch does not configure all library consumers.
5. Use the real App's existing SoftAP provisioning and identity flow, supplying
   the matching PoP when requested. **Do not put DeviceKey, SSID or WiFi password
   in this Sketch.** They remain part of the existing App/device access flow.
   This example never clears an installed root or resets the task store.

The fixed graph is `power` (writable boolean) plus `uptimeSeconds` (lease-driven
realtime sampling at 1000 ms). Only `power` is registered by
`Blinker.enableTasks(power)`. That existing API also enables business time;
then `Blinker.begin(power, uptimeSeconds)` and cooperative `Blinker.run()` use
the original transport, TaskStore and executor. The short write callback reports
the accepted value. No GPIO is selected, and this is not proof of physical output.

Four persistent slots use the matching HTTPS/MQTTS, P256 keyId 1 and
DeviceKey+Admin authority profile. A maintainer must independently verify the
running signer/public key and explicitly admit the device ID on the Service.
**Installing the ZIP does not open server-side admission.** SDK link success
does not establish that the service, identity or task scope matches.
`enableTasks()` does not claim ownership, initialize a scope, create a task,
Start a countdown or replay an UNKNOWN request. Those remain explicit App
operations under the original authentication, CAS and read-only reconciliation.

Supported scope is the reviewed XIAO ESP32S3 SoftAP-after-real-deinit then Cloud
task path, four slots and the original time/storage/fault policy. Calendar tasks
need valid device UTC and use fixed offsets, not automatic DST. No arbitrary
Sketch, BLE/LAN task management, numeric target, additional callback, offline
forever guarantee, GPIO timing or next-week execution is admitted here. Failed
root writes remain fail-closed until cold recovery; never clear a root, restore
an old NVS image or replay an UNKNOWN write as an automatic recovery shortcut.

Current evidence and the limited support decision are recorded in
`docs/DEVICE_V2_TEST_REPORT_RECONCILIATION_AND_CORE_PLAN.md` in the source
repository, especially sections 27 and 30. Internal BIN validation is not public
Arduino-package release approval.
