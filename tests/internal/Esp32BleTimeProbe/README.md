# Ordinary BLE / Hub child time observation

Uses the same `Esp32PlatformBleProduct` as `BlinkerBLE.h`, with one observed real
time service. It does not implement another child protocol or install credentials.
At 115200, `s` prints a compact clock/control snapshot and `t` prints the wire
trace; query them separately so nonblocking USB output is not truncated. No
periodic output, reset or grant command. The optional trace compares accepted sequence/nonce and
counts BBP bytes (not BLE fragmentation/encryption/ATT overhead).
`requests` counts successful encoding, not delivery; `rejected`/`sendError`
record actual local enqueue failures. Only `samples` counts accepted time.

Build a generated ignored copy against an installed release ZIP, preserving or
copying `support/ObservedTime.h`. Back up and identify the exact application before
flashing; preserve the access root. Verify instance/logical identity and the real
Hub topology before using App demand to connect. Do not have the phone own a Direct
session during the southbound-only gate. A `Controller` source tag includes a Hub
controller; only real session evidence identifies the responder.

The shared trace is test-only and borrowed for the full observer lifetime. Earlier
probes without it still only count accepted samples; their historical measured
binary sizes must not be presented as measurements of a newly built observer.
