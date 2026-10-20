# UNO R4 four-slot task reference

This is the fixed manual-Plain-WiFi graph admitted under the maintained R4
board profile, not an unrestricted Arduino Sketch. The same installed
Blinker library still defaults to **zero persistent task slots**. Sketch
periodic/delay Timer, realtime sampling and Cloud history are independent.

The existing API is unchanged: configure the short write callbacks, call
`Blinker.enableTasks(power, duty)` before `Blinker.begin(...)`, then poll
`Blinker.run()`. `enableTasks()` also enables business time; it does not claim
ownership, initialize a task scope, create a definition or Start a task. Check
the returned results. Only the registered writable Properties are task targets.

The maintained profile must supply the original Core 1.5.3 SDK overlay,
all-consumer configuration, heap wrapper and link flags, matching Ed25519
verification key and explicit Service device-ID admission. This example rejects
an unmatched build rather than silently running with zero slots. Adding
`BLINKER_ENABLE_TASKS` inside a Sketch cannot install that profile.

For Arduino IDE, a maintainer runs `tools/install_r4_arduino_profile.py` from
the repository once, after installing the same matching-profile library ZIP and
its pinned dependencies. It adds a separate local board without modifying the
official SDK or library. Restart IDE and select
`Blinker UNO R4 WiFi - Tasks4 Plain (Core 1.5.3)` explicitly. Then open this
example, save a personal copy, fill the three strings in `UserCredentials.h`,
and use the normal Verify/Upload buttons. No Sketch adapter or extra compiler
flags are needed. Selecting the official UNO R4 WiFi board returns ordinary
Sketches to default-zero tasks; this task reference rejects that selection.

The normal library release carries a generated pure-preprocessor identity.
The maintained board checks that identity in C, C++ and assembly; a replaced
library or public profile must not silently use the old board. Before updating,
use the matching old tool and inputs to verify/uninstall the old board, then
update and install the new matching pair. If already mismatched, stop and
restore the old matching inputs before uninstalling; never edit the identity,
force-overwrite the board, clear credentials or replay an UNKNOWN request.
No Python or full-source hashing runs during normal IDE compilation. This is
release compatibility checking, not detection of arbitrary manual source,
dependency or tool-binary edits; explicit verification remains required.

The alternative maintainer-only `tools/prepare_r4_restricted_release.py` uses
this exact `.ino` with a separate private credential include, not the adjacent
public placeholder header. Keep real DeviceKey, SSID and password out of Git
and shared packages. The IDE profile installer never asks for those secrets.

Supported graph: `power` (bool), `duty` (integer 0..100, step 5), and
`uptimeSeconds` (lease-driven realtime at 1000 ms); four persistent task slots,
minimum Start 30 seconds. No TLS, BLE, LAN, extra callbacks, boot-time Start or
automatic recovery is admitted by this reference. After a latched modem fault,
stop service and use whole-board cold recovery plus fresh read-only
reconciliation; never replay an UNKNOWN write.

The complete support/build/recovery contract is in
`docs/DEVICE_V2_R4_RESTRICTED_TASK_ADMISSION.md` in the source repository.
Installation and selection steps are in
`docs/DEVICE_V2_R4_ARDUINO_PROFILE_INSTALLATION.md` in the source repository.
This local Windows-maintained profile is not a public Boards Manager release
or a promise of task support for arbitrary Sketches. It requires no second
library ZIP. The separately recorded §11.55 test passed matching-board startup,
fresh empty Catalog and one control ACK/State on COM9, followed by restoration.
The later §11.57 record in
`docs/DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md` (source repository)
also covers the new identity-guarded artifact: original-SDK link/postverify,
startup, fresh empty Catalog, control ACK/State and restoration. It does not
reuse the older §11.55 binary as evidence for the new guard. The current
closeout and deferred scope are recorded in §11.58; these historical gates
are not a recurring checklist.
CLI installation/compile evidence alone does not certify GUI interaction or
server-side admission; arbitrary Sketches and online deployment remain outside
this fixed reference's support contract.
