# LAN business-time HIL probe

Internal ESP32 test only; not an Arduino example or release-package source.
It composes the existing WiFi product, LAN extension and `TimeSyncService`.
The network wrapper borrows the real MQTT client; it does not open another
socket, stop WiFi, change credentials or inject protocol replies.

Build from an installed development-profile ZIP with these **library-wide** flags:

```text
-DBLINKER_INTERNAL_LOCAL_ACCESS_CONTROL=1
-DBLINKER_INTERNAL_LAN_WEBSOCKET=1
-DBLINKER_INTERNAL_LAN_ENCRYPTED=0
```

Keep actual WiFi/DeviceKey values in an ignored generated Sketch, not this
template. Preserve the adjacent `support/ObservedTime.h` when relocating the
Sketch (or copy it into the Sketch and adjust that test-only include).
Before flashing, identify the exact board and back up its application.
Write only the application region; preserve NVS, access records and binding.
The debug App must contain the actual LocalAccess authorization/responder hookup.

Serial commands at 115200 baud (no periodic output):

| Command | Effect |
| --- | --- |
| `s` | Bounded JSON status: actual accepted sample counts/sources, UTC and transport state |
| `p` | Close only this board's MQTT TCP; refuse reconnect for 60 seconds, then automatically permit it |
| `r` | Permit the ordinary MQTT state machine to reconnect immediately |
| `c` | Once per boot, while MQTT is paused/offline, clear the **business** time anchor for an Observer negative test |

`c` is explicit fault injection, not evidence of natural expiry or holdover.
It never changes `IClock`, TLS, grant lifetime or server time. Do not use it for
the natural source-switch gate: obtain a real cloud sample, establish Control
LAN before the unchanged five-minute deadline, pause MQTT just before that
deadline, then verify the real 28/40-byte request/response and accepted local sample.

Use genuine server-issued permissions and the production client. An Observer
test may request a lower-privilege grant; never alter the returned grant/proof.
Bound App owners, inspect an uncertain Action instead of resending it, and
restore the original application and close test sessions after the gate.
Success does not cover encrypted LAN, all MCU platforms or the one-hour expiry.
