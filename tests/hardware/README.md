# Device hardware release gates

Hardware gates live outside the Arduino release package. They exercise real
platform adapters and do not add code or dependencies to user sketches.

## Gateway execution demand (GATT §10.69)

Deploy the Broker with execution control support first, then the matching Hub
and App. The internal `Esp32EdgeHubDualBoard` sketch prints `executionWanted`
on changes and every 10 seconds. This is logical demand, **not physical BLE
release**: eager routing remains until the paired idle/Direct scheduler lands.

`edge_gateway_execution_gate.mjs` accepts the same JSON configuration as the
cold-start gate: `hub`, `child`, `tenant`, `instance`, `container`, `shard`,
`port`, `python`, and optional `cdp`. No credentials belong in that JSON.
Keep the signed-in debug App on the exact Hub enrollment page, with no pending
enrollment or Direct link. The default mode uses the production Session API:

```powershell
node tests/hardware/edge_gateway_execution_gate.mjs $gateConfig
```

It checks two owners, a fresh positive after partial release, total release,
abort before ACK, and actual session retirement without pre-releasing. Hub
serial, Broker owner count and exact MQTT control frames are independent
witnesses. No Action is issued; the device counter must remain unchanged.

For the separate killed-App/loss gate, set `fault:"execution-release-loss"`
and supply the exact `adb` executable path and `androidSerial`. This mode
**force-stops the installed debug App** while dropping only zero execution
Decisions on the exact Hub downlink. Hub MQTT and reachability stay active;
the Hub must expire its previous positive lease without receiving release.
The tool stops/cleans its bounded Inspector hooks on exit. Afterwards relaunch
the App, restore the WebView ADB forward and return to the same Hub page;
do not reset credentials or issue another Action to recover the test.

All Inspector hooks expire after 120 seconds. A failed run now captures only
a bounded state snapshot and whitelisted error code before cleanup; it is not
automatically retried. Gate wait/serial observations include polling latency
and must not be described as an exact physical disconnection deadline.

For the local HTTP/WS engineering environment, Capacitor copy requires the
existing explicit `BLINKER_LOCAL_CLEARTEXT=1` before Android assembly. Keep
the production HTTPS default unchanged, and use `adb install -r` to preserve
the current test account/Direct credentials. This setting is not a gateway
MQTTS exception. New tests run without hardware:

```sh
node --test tests/hardware/edge_gateway_execution_gate.test.mjs tests/hardware/edge_gateway_route_recovery_gate.test.cjs
```

## Wio Terminal BLE connection release

Prerequisites:

1. Flash `tests/arduino/WioBleReleaseGate` to a Wio Terminal running the
   frozen official RTL8720DN firmware.
2. Open the installed Android app's BLE debugger so its Capacitor WebView has
   Bluetooth permission, and keep that exact App/WebView in the foreground for
   the whole run. Android may suspend JavaScript timers in a background WebView.
3. Forward that WebView's DevTools socket to a local port with `adb forward`.
4. Use Node.js 22 or newer; the script uses its built-in WebSocket client.

Example:

```powershell
adb forward tcp:9222 localabstract:webview_devtools_remote_<app-pid>
node tests/hardware/wio_ble_release_gate.mjs `
  --cdp http://127.0.0.1:9222 `
  --device 2C:F7:F1:22:8B:7C
```

The default gate runs three connections without CCCD notification, three
connections with notification but without BBP/2 authentication, and 32 full
GATT connect/subscribe/read/disconnect cycles. Both unauthenticated cases must
be physically disconnected near the device's 15 second deadline. The address
is a test locator only and must not be stored as Blinker device identity.

This gate validates bounded connection release and resource reuse. It does not
replace the separate 24 hour authenticated DirectSecure soak required before
Wio BLE can leave opt-in preview.

## Edge Hub credential renewal repetition

`edge_gateway_renewal_gate.mjs` drives the internal-only active credential
renewal path without adding a test hook to device or service production code.
Each iteration waits for Service completion, replays the same create request,
and verifies the Broker delivery plus Prepare/Apply/Finalize ledgers. It stops
at the first non-exact version, storage revision, terminal failure, timeout, or
unexpected response field.

Capture the Edge Hub serial resource line before and after each stage. Run 30
iterations first and only promote the same firmware and service images to 100
when free/largest heap and task stack watermarks remain stable.

```powershell
$env:GATEWAY_ADMIN_TOKEN = '<local internal token>'
$env:BROKER_INTERNAL_TOKEN = '<local shard token>'
node tests/hardware/edge_gateway_renewal_gate.mjs `
  --topology <16-byte-base64url-topology-operation-id> `
  --count 30 `
  --expected-version 13 `
  --shard 1
```

The topology ID comes from the Service's authoritative active topology. Tokens
must be supplied through the environment and are never accepted as command-line
arguments or written to a result file.

## Edge Hub permit-join negative gate

`edge_gateway_permit_join_gate.mjs` checks the public owner HTTP path through a
signed-in Android WebView, a real Hub and a nearby provisioning child. It does
not enroll the child, write access credentials or create a topology. The phone
must not hold a Direct connection to that child during this gate.

```powershell
adb forward tcp:9222 localabstract:webview_devtools_remote_<app-pid>
node tests/hardware/edge_gateway_permit_join_gate.mjs `
  --hub <exact-edge-hub-logical-device-id>
```

Node.js 22+ is required. `--api` is the API URL reachable **from the phone**
(default `http://127.0.0.1:7000`, requiring the corresponding ADB reverse).
`--cdp` defaults to `http://127.0.0.1:9222`. `--protocol` can point to the
Service shared protocol module; by default the sibling `aily-blinker-services`
checkout is used. No token is exported from the WebView or printed.

The gate polls the operation GET until Hub Ready before using `/relay`; relay
GET does not reconcile the Service window with the Broker ledger. Every wait
is bounded, and even a failed assertion attempts to close the exact window.
It verifies:

- exact Open replay preserves the expiry; changed adapter with the same ID is 409;
- a one-bit mutation of a **real Hub candidate token** returns NotFound;
- exact Select replay returns identical result bytes in the same relay session;
- a changed Select after that result is 409 and leaves the result unchanged;
- Close reaches a terminal Service state.

Capture Hub serial state/resources and independently check Broker/PostgreSQL
terminal state, zero new enrollment intents and zero topology mutations. Token
tampering is a selection negative test, **not** a substitute for encrypted
grant/receipt/enrollment/attach tamper tests. Natural expiry is a separate gate.

Before hardware runs, verify `build.options.json`, `compile_commands.json` and
`sdkconfig` against the admitted firmware baseline. In the current local setup,
the default Arduino CLI still selects Core 3.3.6; the admitted Hub uses isolated
Core 3.3.11 (`CONFIG_ESP_IPC_TASK_STACK_SIZE=2048`). Compile and upload with the
same explicit CLI config and build directory; do not silently fall back to the
default toolchain. This is test-baseline control, not a new public SDK-version
check or a required private SDK. Preserve the existing partition table/NVS.

## Edge Hub encrypted enrollment negatives (N3)

`edge_gateway_enrollment_negative_gate.mjs` runs the installed debug App's
production coordinator and WebCrypto over Service/Broker/Hub/GATT. It does not
install a production test API or export tokens/secrets. Open the exact Hub's
enrollment page, with no active discovery/recovery. Read the child instance ID
from known test inventory, and explicitly select its current provisioning
locator; never use the first scan result or a MAC as identity.

```powershell
node tests/hardware/edge_gateway_enrollment_negative_gate.mjs `
  --hub <exact-hub-logical-id> --instance <32-lowercase-hex-instance> `
  --locator <16-lowercase-hex-provisioning-locator> --fault signature
```

Run `signature`, `request-tag`, then `receipt-proof` on the same frozen APK,
Hub and child firmware. The first two alter respectively the signed grant's
last signature bit and the Noise request's last AEAD tag bit. The child must
disconnect without installing; no encrypted error response is required by the
current device contract. The App must consume the Hub's existing Disconnected
SelectResult instead of waiting for its 45 s receive deadline. The script
cancels only the newly rejected intent and its matching App recovery state.

`receipt-proof` changes the response's last proof bit **after real AEAD
decryption**. This tests the App's receipt validation, not on-air AEAD tag
validation: the real child has already installed. The script must observe
`BLE_DIRECT_RECEIPT_MISMATCH`, zero Method 2/commit calls, and keep the pending
credential/checkpoint. Use the normal App recovery entry afterwards to prove
Method 2 → commit → Gateway attach/proof → Active, without reset or new grant.

Before/after each case, independently audit child durable state and
PostgreSQL/Broker. The internal `Esp32BleAccessResetProbe` accepts `s` to open
only `bl_v2_access/root` read-only and print result/size/SHA-256, never raw
credentials. `4354` (`ESP_ERR_NVS_NOT_FOUND`) means no access root. An equal
digest proves unchanged persisted content, **not a physical flash-write
count**. The script alone cannot certify zero writes or full N3 completion.
`a` still explicitly resets access; never send it during pending recovery.

For an explicitly authorized reset/enrollment gate, the same internal sketch
can count access NVS API writes without changing production storage code:

```text
arduino-cli --config-file <core-config> compile --fqbn esp32:esp32:XIAO_ESP32S3 --library <library-root> --jobs 4 --build-path <witness-build> --build-property compiler.cpp.extra_flags=-DBLINKER_TEST_NVS_WITNESS=1 --build-property compiler.c.elf.extra_flags=-Wl,--wrap=nvs_open,--wrap=nvs_close,--wrap=nvs_set_blob,--wrap=nvs_commit,--wrap=nvs_erase_key tests/internal/arduino/Esp32BleAccessResetProbe
```

`AccessWriteWitness.h` wraps only public ESP-IDF NVS calls and forwards their
arguments/results untouched. It observes the single read/write handle for
`bl_v2_access`, counts `root` set/erase attempts or successes, and successful
namespace commits. Require `observed=1, ambiguous=0`; concurrent/multiple
read/write handles invalidate this single-loop probe. Read-only `s` does not
increment counts. Capture the baseline **after reset** and again after the
injection, before normal recovery installs the Gateway credential. Exactly
one set/commit across first receipt plus exact/changed requests proves one
NVS API installation, not one physical flash-page write. Counters restart at
boot; preserve an uptime witness. Afterwards restore a verified normal build
without the flag/wrappers, preserving partition table and NVS.

The same internal probe accepts `b` for a read-only native BLE witness:
uptime, Host sync/advertising/live-last-ACL, connection/disconnection counts,
indication failure count/status, last disconnect/advertise-complete reason,
and heap watermarks. A passive SDK GAP listener updates fixed counters under
a lock; it does not replace Blinker callbacks, log on the Host task, decode
packets or read credentials. Query before/after each gate, **before any reset**.
The values are observations at slightly different instants, not an atomic SDK
snapshot; `acl` tests only the last listener-observed handle, so `acl=0` with
zero observed connections does not prove there is no native ACL (the first
connection can precede listener registration). A live ACL with `adv=0` is normal.
Installing this probe resets runtime
and establishes a new baseline, so it cannot diagnose an earlier lost event.

Host `blinker_esp32_nimble_adapter_tests` compiles the real peripheral adapter
against a narrow SDK fake. It covers indication completion/failure, asynchronous
close, already-missing ACL, bounded busy termination, connection-event handoff,
and callback-before-advertising ordering. It is not an RF/SDK stability test.

Each run requires an exact child check before pending cancellation or HTTP
grant issuance, and restores its temporary App client hook in finally. A
180 s CDP deadline is a failure, not a pass: inspect the App checkpoint and
close/recover the exact operation before retrying. Any existing pending
credential for that child blocks a run. Keep the phone unlocked/foreground throughout.

## First enrollment-to-attachment handoff

Run this positive gate separately from N3 fault injection. Freeze the APK and
Hub/child firmware hashes. If preparing a previously attached test child, use
the normal owner detach workflow and wait for authoritative Detached before
resetting **only that exact child's access record**. Preserve its instance ID
and firmware. Never reset a pending enrollment to make recovery appear to pass.

1. In the exact Hub's enrollment page, scan and explicitly select the current
   provisioning locator. Check the known child instance before issuing a grant;
   do not choose the first advertisement or use MAC as identity.
2. Use the normal select/continue-waiting workflow. Waiting cloud delivery or
   proof may be polled; a failed phase ends the first-attempt gate. Do not retry
   a failure or create a replacement operation and call it first-attempt success.
3. With temporary debug observers, verify enrollment session closure precedes
   the first attach create; tracked links/sessions must then be zero. Verify
   child installation uses a fresh session through the checkpoint's exact Hub,
   not the old enrollment session or the phone's direct BLE controller.
4. Independently read PostgreSQL Active/version and Broker durable delivery ACK
   and Secure proof. Confirm App ready, no recovery checkpoint, no tracked
   links/sessions, and no active permit operation. Record elapsed time and Hub
   heap/stack watermarks, without mixing phone and server clocks for ordering.
5. Restore all observers in finally and verify the original methods are back.
   Never export secrets, alter protocol bytes, or install production test APIs.

The 2026-09-05 evidence is in the GATT implementation plan §10.33: first-attempt
Active in 89.43 s, with normal pending polls and no failed/retry phase. This is
an end-to-end enrollment/attach measurement, not a BLE connection latency or
a replacement for encrypted negatives, process-kill recovery, or release soak.

## Gateway attachment receipt and device replay negatives (N3)

`edge_gateway_attachment_negative_gate.mjs` requires a signed-in debug App on
the exact Hub enrollment page, an existing PendingChildInstall topology and
its exact account-scoped checkpoint. It never detaches/resets/re-enrolls on
failure. To prepare a previously active test binding, perform normal owner
detach and wait for Detached, then create a new attachment using the existing
Admin credential; do not clear device access.

```powershell
node tests/hardware/edge_gateway_attachment_negative_gate.mjs `
  --hub <exact-hub-logical-id> --child <exact-child-logical-id> `
  --instance <32-lowercase-hex-instance> --operation <32-lowercase-hex-operation> `
  --fault receipt-proof
```

Run `receipt-proof`, then `request-replay` on the same pending operation and
frozen APK/firmware. Node.js 22+ and a forwarded debug WebView CDP endpoint are
required (`--cdp` defaults to `http://127.0.0.1:9222`). The script guards exact
topology/instance/checkpoint before opening control and restores all hooks.

- `receipt-proof`: mutate the proof after real DirectSecure decryption and
  receipt decoding. The child has already installed. App must refuse Gateway
  Method 2 and normal commit while preserving the checkpoint. Separately send
  the same bad receipt to the real Service: expect 409/receipt mismatch and
  unchanged PendingChildInstall/version/updatedAt. This intentional negative
  HTTP call is not the production relay's normal commit.
- `request-replay`: first repeat identical BBP/2 kind/flags/sequence/body,
  through the normal encryptor with a fresh DirectSecure counter; expect equal
  receipt bytes. Then change the secret under that same signed grant/sequence;
  expect wire AuthenticationRequired(2). Finally repeat the bad-receipt checks
  above to stop before activation and leave normal recovery available.

Record the child read-only NVS witness after its first install, then before
and after replays and recovery. Equality proves unchanged durable content,
not physical flash-write count. The script's `installs` counts successful
mutation API receipts, not writes. Complement it with Host DeviceAccessStore
`replaceCalls` assertions, including a full three-slot store reconstructed
before exact replay and a conflicting same-id/version secret.

Recover via the normal original checkpoint afterwards and independently check
PostgreSQL Active, Broker acknowledged delivery/Secure proof, no active permit
and zero App links/sessions/checkpoints. A connection timeout or test-harness
error is a failed attempt, never evidence of successful authentication
rejection. Existing GatewayHttpError uses `code/httpStatus`. Non-secret
SelectResult observations are included to distinguish connection failures.
Do not treat these tests as ciphertext replay, enrollment requestId replay,
Hub sealed-envelope validation, process-kill recovery or release soak.

## Exact-child permit connection regression

`edge_gateway_connection_gate.mjs` exercises the installed App's production
permit link, Method 2 and synchronization without any enrollment, detach,
credential mutation or access reset. Open the exact Hub enrollment page with
an existing authorized child and no active links/workflow. It matches the
child's stored instance and authorized rotating locator, never a MAC or the
first scan result.

```powershell
node tests/hardware/edge_gateway_connection_gate.mjs `
  --hub <exact-hub-logical-id> --child <exact-child-logical-id> `
  --instance <32-lowercase-hex-instance> --count 3
```

Node.js 22+ and the forwarded debug WebView are required. Count is 1..10;
the run stops on its first failure or leaked link/session/permit checkpoint. Record public
SelectResult bytes and discovery/connect/Method 2/cleanup timing, together
with the frozen APK/firmware hashes and independent serial/NVS/Service checks.
`gateway_app_gate.mjs` contains only the test CDP transport, not a production
API. Hooks are restored even if close fails. A CDP timeout does not cancel the
App operation; inspect and recover that exact window before retrying.

For a real child-initiated ACL rejection followed by fresh-window recovery,
add `--fault selected-acl --count 2`. After authenticating and synchronizing,
the first iteration sends one invalid application record through normal
fragmentation/relay. It must observe the authoritative Rejected window and
clean all links/sessions/checkpoints; the next iteration uses a different
operation and authenticates normally. This does not reset access or mutate
credentials/topology. It is not a radio power-loss or ciphertext/tag gate.

For exact ACK idempotency, use `--fault duplicate-ack --count 2`. Every Up ACK
is resubmitted once after its first HTTP request succeeds. Both iterations
must authenticate/synchronize/close and report nonzero `duplicateAcks`; verify
Hub relayError, authoritative windows, and unchanged credentials separately.
It only replays public relay ACKs, never encrypted application records.
For the distinct BBP/2 application ACK replay, use `--fault delayed-ack --count 1`.
It delays handling the first decoded ACK by 1300 ms, letting App's existing
reliable retry traverse the actual Broker/Hub/child. No response is fabricated.
The gate requires a real late ACK outside an ACK wait plus successful State
synchronization/cleanup. This controlled handling-delay gate is not a natural
radio latency measurement or an MQTT-outage continuity test.
On failure the gate keeps at most 32 public relay kind/direction/sequence/status
entries; opaque packet contents are neither parsed nor retained in this trace.
It also observes the authenticated session boundary and retains at most 32
BBP/2 kind/flags/sequence and pending-request metadata entries on failure.
No frame body, Noise material, credential or arbitrary error text is logged;
this is a bounded diagnostic, not a complete packet capture. Session/client
hooks are restored after cleanup and do not change the APK.

For a controlled MQTT-only outage, the internal `Esp32EdgeHubDualBoard` probe
accepts serial `c`. It arms one socket close for 30 seconds, triggering only
when the exact active permit window has an upstream batch awaiting ACK. It
does not stop WiFi, BLE, or the product, reset access, or erase NVS. Observe
`fault=mqtt_socket_closed_upstream_pending` and Broker connection events;
`fault=mqtt_socket_close_expired` means no injection occurred. The platform
subclass/socket pointer belongs only to this probe, not the published API.
Do not run a second operation until the first is confirmed terminal and all
App links/sessions/checkpoints are cleared. A production App HubOffline/503
failure is a failed continuity gate even if subsequent cleanup succeeds.

An established or selected ACL that fails ends its permit window. WouldBlock
may retry only on the same ACL; opaque Noise/DirectSecure fragments must never
be resumed on a new ACL. Short positive cycles do not prove the native cause
of an earlier timeout, forced-disconnect recovery, N3 completion, or release
soak. Remove temporary native traces before the final firmware gate.

### Enrollment requestId replay gate

`edge_gateway_enrollment_negative_gate.mjs --fault request-replay` extends the
existing signature/tag/receipt gate; CDP transport is shared with
`gateway_app_gate.mjs`. It requires an exact **unprovisioned** child instance
and current setup locator, an idle App, and no outstanding recovery/window.
Do not reset a currently Active child without explicit authorization covering
credential removal, re-enrollment and normal Gateway attachment recovery.

```text
node tests/hardware/edge_gateway_enrollment_negative_gate.mjs --hub <exact-hub-id> --instance <32-hex-child-instance> --locator <16-hex-current-setup-locator> --fault request-replay
node --test tests/hardware/edge_gateway_enrollment_negative_gate.test.mjs
```

The first command is a real transaction, not a dry run: the child installs
Admin/Presence credentials and the App saves its pending recovery. The gate
checks the complete first canonical response against the HMAC receipt saved
by the production client. It then sends identical Enrollment plaintext with
requestId **2** at fresh Noise TX counter **1**, validates the identical full
response at RX counter **1**, and changes only PresenceKey content at TX
counter **2**. The original request/response used counter **0**. Cloud
intentId, Enrollment requestId and AEAD counters are independent identifiers.
No ciphertext/nonce is reused, and no new production retry API is introduced.

After rejection the gate must abort this Noise session; its injected counters
must not be handed back to the original client. All hooks and copied secrets
are restored/cleared even on cleanup failure. It blocks Method 2 and commit,
closes only the permit window, and **keeps** the installed original pending
credential/enrollment checkpoint. Use the production `finish_enrollment`
recovery to authenticate with the original Admin credential, commit, and
attach. Do not cancel/remove this installed pending intent or create a new
one. If close fails or CDP times out, inspect/recover that exact window first;
do not rerun the injector over an uncertain transaction.

Passing script assertions alone does not prove MCU rejection or zero NVS
writes. Also record actual child error/ACL evidence, first-install vs replay
access content and write attempts, the Service intent/terminal permit state,
and subsequent normal Method 2/commit/attach recovery. Generic transport
failure is not authentication-rejection evidence. A changed request that
receives any response fails this gate, even if subsequent disconnect succeeds.
The terminal window can overtake the last relay ACK/SelectResult. In that
case `RELAY_NOT_READY` is insufficient: before closing the window, the gate
uses its normal account-bound GET and requires the exact operation, Hub,
adapter and authoritative `rejected` state. Ready/expired, wrong binding or
failed GET must fail. `changedWriteCompleted=false` records that the terminal
state overtook HTTP write completion; it must not be reported as a received
final write ACK. This observation does not weaken production relay validation
or distinguish every possible reason for a selected ACL rejection; combine
it with the exact receipt/counter, MCU and NVS evidence above.
Self-tests use modeled App/link/storage boundaries and actual AES-GCM, not
a real Noise responder, Service signature, MCU or flash. The separate C++
Host gate checks conflict/replay/storage semantics; neither replaces hardware.

### Isolated ESP32 envelope / NVS gate

`tests/internal/arduino/Esp32GatewayEnvelopeProbe` uses the production
`GatewayAccessDeliveryProcessor`, `GatewayAccessStore`, ESP32 mbedTLS crypto
and NVS adapter, but **synthetic public credentials** and its own
`nvs:bl_n3_env/record`. It has no network, App or MQTT receiver. It is not a
product example, a four-end transport fault injection, or evidence of D4b
encrypted-at-rest admission. Its protection override is test-only.

Verify `EnvelopeVectors.h` against the actual Service sealer with:

```text
node tests/golden/server_gateway_envelope_probe.cjs
```

The generator's vault stub is not a KMS test. `--header` emits regenerated
public fixtures to stdout; review any fixture change. No real account keys
are accepted by this gate.

Run only when the exact Hub has no active permit/renewal/revocation. Record
the current firmware hash and restore artifact first; verify MCU/port and
partition layout before flashing **only the app partition**. Never erase
NVS, modify a completed delivery ledger, or change DeviceKey/ownership to
manufacture a test result. Serial commands at 115200:

- `s`: optional read-only size/hash of `bl_eh_access/child`, never raw bytes.
- `e`: empty/stored admission matrix, failed write, exact replay, valid
  rewrap and material conflict; leaves the original synthetic fixture only.
- `r`: explicit software restart (not physical power-loss).
- `v`: require the persisted exact fixture, return the frozen ACK with
  zero replace attempts. Run after `r`; missing/different content fails.
- `x`: verify the exact fixture first, then erase only its test record.
  The empty namespace can remain; do not erase the NVS partition.

Restore the frozen Hub app afterward and verify management subscription,
normal App authentication/synchronization/cleanup, and unchanged topology /
child credential content. `checks` counts assertions, not independent cases;
`attempts` counts calls to replace, `writes` successful actual NVS adapter
replaces (not physical flash-page program operations). A rejected frame
must leave both attempts and the stored-content hash unchanged. Report
simulated clock admission separately from real SNTP/expiry on the live Hub.

## ESP32 child GAP witness across Host restarts

`tests/internal/arduino/Esp32BleAccessResetProbe/BleGapWitness.h` is an optional
observer, not a product SDK hook. Compile that sketch with:

```text
--build-property compiler.cpp.extra_flags=-DBLINKER_TEST_GAP_WITNESS
--build-property compiler.c.elf.extra_flags=-Wl,--wrap=ble_gap_init
```

Do not replace `build.extra_flags`: it carries the board USB/core definitions.
Verify the map contains the wrapper and flash only the confirmed child app
partition, with the existing restore artifact and matching partition layout.
`ble_gap_init` clears the SDK listener list, so a setup-only registration loses
all observations after resetAccess or Provisioning/Direct Host restart. The
wrapper registers after successful init, before the Host task starts, preserving
the original init result. It adds no production callback, SDK fork or timer.

Serial `b` reads public metadata only. Require `observed=1`, `init=0`, `listener=0`
and the expected `hostStarts` transition before using cumulative connect/error
counts. Last connection/reason metadata resets per Host generation; totals are
since boot, not since the last `b`. No secret, address or packet is recorded.
Without these flags the sketch prints `observed=0` and no listener counters.
Serial `a` is destructive access reset and requires explicit user authorization;
it is **not** needed to attach a normally enrolled Direct device to a gateway.

## Edge Hub Admission V2 / route recovery

`edge_gateway_route_recovery_gate.cjs` is an **internal local Broker Inspector**
tool, not a device API or production service endpoint. Use only with an exact
test Hub/child whose binding is already Active and route fully ready. Preserve
both boards' access records and the Service/Broker volumes. Deploy paired Hub
and Broker Admission V2 builds first; do not change the child or App protocol.

The tool runs inside the disposable Broker container, in its Broker working
directory, using its installed codecs and `ws`. Enable Node Inspector with
SIGUSR1 **on container loopback only**; never publish port 9229 or enable this
on an online production deployment. Copy the script to `/tmp`, not into `dist`.

```text
node /tmp/edge_gateway_route_recovery_gate.cjs start '{"fault":"admission","hub":"<exact hub>","child":"<exact child>","tenant":"<exact tenant>","shard":1}'
node /tmp/edge_gateway_route_recovery_gate.cjs recover
node /tmp/edge_gateway_route_recovery_gate.cjs status
node /tmp/edge_gateway_route_recovery_gate.cjs close
node /tmp/edge_gateway_route_recovery_gate.cjs disable-inspector
```

Inspect `status`, not just the process exit code: require `sameMqtt=true`, a new
ready protocol session, the expected drop/retry count and equal byte digests
on exact retries. Independently verify PostgreSQL topology, command ledger,
App counter and child access hash. The script prints bounded metadata, never
credentials or arbitrary exception bodies. On a tool error inspect the current
state before retrying. The 120-second hook deadline restores originals, but
does not cancel an already dispatched production recovery operation.

Modes:

- `observe`: no message loss; use the internal Hub's `d` for one physical cold
  disconnect, with MQTT and credentials preserved.
- `closure-receipt`: requires the paired closure-capable Hub/Broker. Drop only
  the first route closure receipt, observe exact notice copies and old projection
  offline before fresh bootstrap. `replayClosure` after new ready injects the
  previous notice and receipt: the new route must remain ready. The cold runner
  then submits exactly one App Action/readback; no credential or topology reset.
  A fast new Hello may supersede the notice before a retry is due: use the trace
  count to distinguish that case from actual retransmission. This is not an
  all-notices-lost or long-soak guarantee.
- `admission` / `hello` / `manifest`: `recover` retires only the transient route
  and invokes the normal authority-checked admission; drop its first matching
  admission / Hello / ManifestRequest, respectively.
- `bootstrap-exhaustion`: drop every copy of the first exact ManifestRequest,
  allow a new generation. Only invoke `recover` once; the second admission must
  come from production timeout recovery. A successful sample does not prove
  the separate two-failures-remain-offline case.
- `bootstrap-terminal`: drop ManifestRequests in both generations. Invoke
  `recover` once, then observe for 45 seconds. Require exactly two requests
  (four identical copies each), no third admission, no Command and offline
  App readiness rejection. After cleanup, explicitly reconnect the Hub MQTT
  session to restore service; closing the hook must not itself re-admit it.
- `unknown-action`: do **not** invoke `recover`. Send exactly one intentional
  counter-only `button` Action via the real signed-in App account path. Drop
  the matching real ACK, then retire/recover. Require an `outcome_unknown`
  ledger and one increment; after ready, `replayOld` injects the old ACK and
  old downlink envelope. Require no additional increment, then normal control.
- `runtime-state`: keep the same BLE/MQTT/protocol session. Drop the first
  counter Patch, then the first runtime StateRequest and the second Action's
  first ACK. Only two intentional counter Actions are submitted. `waitPatch`
  must observe the first dropped report before the second Action; otherwise
  device report coalescing can prevent the intended revision gap. `waitReady`
  requires all three faults, the same protocol session and counter +2.

`close` restores both hooks and clears retained frame buffers. At the end run
`disable-inspector` and verify loopback 9229 refuses connections. Do not leave
an Inspector listener behind. All injection is at Broker packet boundaries,
not a claim of RF loss or a mock of the physical child's execution.

The independent `.test.cjs` uses codec stubs to test targeting, ownership,
one-shot loss, exact-request exhaustion and cleanup. It needs no sibling
Service checkout; wire golden and Broker lifecycle tests remain separate.
Measure timing within one observer: Android, Windows and Docker wall clocks
can differ, so do not subtract their absolute timestamps for cold latency.

### Cold-route timing gate (L2b-4a)

The internal ESP32 Hub probe also accepts read-only `g`: it reports the active
ACL connection interval (1.25 ms units), latency, supervision timeout (10 ms
units) and ATT MTU, never addresses or secrets. The production Central port
now requests 12 units / 15 ms by default; verify the negotiated value rather
than inferring it from the SDK version. No public Sketch option is required.
The temporary live interval-changing probe has been removed. Route observation
metadata includes `frameBytes`, not frame contents; computed fragment counts
are not a radio trace or measured throughput.

`edge_gateway_cold_start_gate.mjs` reuses the route recovery gate and signed-in
Android App account client. Run only on the exact, idle Hub enrollment debug
page with an Active child, no Direct connection or pending recovery/permit.
The child fixture must expose a harmless `button` Action and `buttonPresses`
counter; the Hub internal probe must implement `d` as ordinary BLE disconnect.
The App must include the known-offline readiness guard from GATT §10.57.

With `readyWait:true`, require the App implementation from GATT §10.62.
On the actual offline notification the helper starts two production
`waitUntilReady` consumers and cancels one. Before any Action it verifies that
the other reached ready within the original 30 s budget. `fault:"bootstrap-terminal"`
instead requires `DEVICE_V2_READY_TIMEOUT` near 30 s, keeps observing Broker for
45 s (two exact generations only), and must never submit an Action.
The helper prints only fixed failure categories and timing, not exception bodies.
Restore normal connectivity after a terminal negative test; do not reset Access
or blindly rerun a failed Action. Page cancellation never owns account teardown;
the helper closes its test account session explicitly during cleanup.

```powershell
node tests/hardware/edge_gateway_cold_start_gate.mjs '{"hub":"<logical hub>","child":"<logical child>","instance":"<32 hex>","tenant":"<tenant>","shard":1,"port":"<Hub COM>","python":"<python executable>","container":"<broker container>","count":1}'
```

`count` is 1..3. Optional `fault:"manifest"` drops only the first exact
ManifestRequest. No credentials belong in the arguments. All end-to-end marks
use one host monotonic clock, including USB/CDP/Docker scheduling overhead.
App and Broker internal durations are separate observations, not clock offsets.
`waitReady` observes for at most 45 seconds; same MQTT and a **new** ready child
session are required. During the real offline notification the App performs a
readiness-only check and must reject with `DEVICE_V2_TARGET_UNREACHABLE`. It
never submits a control while offline. After ready it issues one Action and
requires exactly one increment. Timeout/unknown/error stops the run; do not
rerun blindly before checking the physical counter and ledger.

Serial uses DTR/RTS=false and writes only `d`, never reboot/resetAccess. Cleanup
closes observers, the App account test session, Broker hooks and Inspector;
verify 9229 refuses connections afterwards. Topology/credentials are preserved.
These short samples do not replace failure-terminal, radio-loss or soak gates.
The five hardware helper suites now contain 62 tests and run without physical
hardware or a sibling Service checkout in CI.

### Gateway reachability lease gate (L2c)

Use `edge_gateway_lease_gate.mjs` after deploying a matching Hub and Broker.
The debug App must be signed in on the exact Hub enrollment page; no pending
enrollment, Direct link or credential recovery may be active. Provide the same
`hub`, `child`, `instance`, `tenant`, `container`, `shard` fields as the cold-start
gate, plus `port` and `python` for `lease-loss-all`. No DeviceKey is supplied.

Run one `fault` per invocation:

- `observe`: three distinct challenge/proof cycles, same MQTT/child session.
- `lease-request`: drop the first challenge, require an exact retry near 5 s.
- `lease-proof`: drop the first proof, require the same challenge and proof retry.
- `lease-loss-all`: drop all route uplink (including proof, closure and Hello),
  write only normal serial `d`, require the old projection to expire while MQTT
  stays online, and require the App's read-only readiness check to reject.
  Replay the captured proof while offline, observe another 5 s without automatic
  admission, then explicitly lift injection/re-admit the volatile route and
  replay the old proof against the new session. Active/access storage is unchanged.

All modes compare the App's counter before/after and issue **no Actions**.
The loss-all gate does not reset/re-enroll or flash the child. `recover` on that
fault explicitly ends its injector; this is an engineering recovery operation,
not an implemented user-facing demand/wake API. Each Broker hook has a 120 s
cap, stores at most one proof and allows at most two explicit replays.
Cleanup closes the observer/account test session, hooks and Inspector; verify
port 9229 refuses connections afterwards. Failure does not trigger another test
Action. If failure occurs before explicit recovery, inspect/recover the projection
before restarting a gate, which requires an initially ready child.

`edge_gateway_route_recovery_gate.cjs preflight <config>` reads only Hub/child
readiness, revision and counter. It exports no session credential. Event
`accepted` means the production handler returned; only business frames have a
`bbpAccepted` field. Lease validity is demonstrated by continuing fresh challenges
and readiness, not by the handler's deliberately null BBP lane result.

The five software suites run on Windows and in an isolated Linux Node image:

```sh
node --test tests/hardware/edge_gateway_route_recovery_gate.test.cjs tests/hardware/edge_gateway_enrollment_negative_gate.test.mjs tests/hardware/edge_gateway_connection_gate.test.mjs tests/hardware/edge_gateway_cold_start_gate.test.mjs tests/hardware/edge_gateway_lease_gate.test.mjs
```

With `fault:"runtime-state"`, `count` must be 1 and no serial disconnect is
performed. The same runner uses two account Commands and then an App State
readback; failures never automatically replay an Action. Require exact State
request digests, a correlated command ACK accepted while State is pending,
and +2 in both Broker and App.
Fixed phase names are printed before each App/Broker operation to locate
precondition/cleanup failures without printing arbitrary exception bodies.

With `fault:"ingress"`, `count` must be 1. First compile the **internal**
`Esp32EdgeHubDualBoard` sketch with `BLINKER_TEST_INGRESS_CAPACITY=1`; the
normal build must not accept `q`. Its injected RouteClient wraps only the
receiver: real MQTT, the production 1000 ms slot, expiry and clock still run.
The wrapper returns WouldBlock before accepting Commands for 6 seconds after
the first one; other request kinds pass through, and it never prints bodies.
No public firmware API, MQTT endpoint or SDK memory patch is involved.

The runner writes only `q`, submits one intentional counter Action, waits for
the probe to release, and checks one or more actual slot expiry witnesses,
the same MQTT/protocol session and an unchanged counter. Exact Broker retries
are expected, not extra Actions. Read the ledger separately: it must remain
`outcome_unknown`, not a false confirmed rejection. A probe knows it blocked
the request, but the production Broker has no authenticated negative receipt.
Only inspect bounded error codes, not arbitrary exception payloads.

The serial `heldMs` starts at the first receiver callback, which may be a
millisecond later than MQTT receipt; it does not change the production clock.
This is controlled receiver capacity pressure on a real MCU, not proof of
natural RF congestion or every overlapping GATT response. Host coverage also
checks no late execution, non-Command passthrough and automatic release.
Restore and verify the normal application-only image after this gate; preserve
all credential/NVS partitions. Do not leave fault firmware on the test Hub.

## Isolated Direct phase timing (not the Gateway fairness gate)

`edge_gateway_direct_timing_gate.mjs` observes the installed debug App's real
discovery, native GATT, Hello/Method 2, initial Manifest/State and native close.
It sends no Actions and never exports credentials, locators, addresses or State
values. The signed-in App must be foreground on `/home/device`, with no page,
read, enrollment or BLE work active. The exact child must already have its
active Direct credential; do not reset/re-enroll to prepare this measurement.

Use an ignored local JSON config containing `child`, its 32-character lowercase
hex `instance`, `count` (1–10), `mode: "isolated-baseline"`, `hubIsolated: true`,
and optionally `cdp` (default `http://127.0.0.1:9222`). The operator must separately
verify that the Hub is not acquiring/holding this child's radio throughout the
run, e.g. serial `Stopped / secure=0 / executionWanted=0` plus no management or
permit work. The boolean is an explicit assertion, not automatic proof of isolation.

```sh
node tests/hardware/edge_gateway_direct_timing_gate.mjs build/direct-timing.config.json
node --test tests/hardware/edge_gateway_direct_timing_gate.test.mjs
```

The observer uses neutral foreground/account cancellation checks only in this
isolated diagnostic; it does **not** create Broker Ready or exercise the normal
managed-child selector. It drains observed native closing before another
iteration, keeps late completions in the original row, and restores all hooks
even on cleanup rejection. One failed phase stops the batch; no implicit retry,
Action, reset, pairing change or quota-avoidance sleep is allowed. Its 60 s abort
and 75 s CDP deadline do not prove native cleanup; after timeout inspect links and
work before another run. `nativeAdmissionBuild` distinguishes the newer App API,
not a certification of Android's native implementation.

The 2026-09-07 requested 10-cycle run passed five cycles and stopped on cycle six:
ADB reported Android scanning too frequently while the App reported scan timeout.
This is a failed 10-cycle gate, not five acceptable substitutes or a fairness
window measurement. See GATT plan §10.84 / demand contract 6.22 for the required
shared scan lifecycle/error handling work before repeating it. Do not silently
change the test pacing to hide the production discovery limitation.

After §10.85, the same rapid run explicitly returns `BLE_SCAN_THROTTLED` rather
than a false device timeout. The count-10 runner still fails unless all ten
connections complete; an expected quota refusal is a separate negative gate,
not a successful connection. A later independent recovery must preserve the
same credential/binding. Android quotas limit scan starts, not connected traffic.

`node tests/hardware/ble_scan_resource_gate.mjs` is an Android-only, scan-only
test on the signed-in `/home/device` page with no other BLE/provisioning work.
It verifies Direct/WiFiProv mutual exclusion, non-owner/stale stop isolation and
release using the installed native plugins. It never selects a peripheral,
connects GATT, provisions, reads a Key or sends an Action. An unavailable quota
is a precondition failure; inspect cleanup and retry explicitly later, not in a
hidden loop. It does not certify a complete WiFiProv enrollment or the second
name/QR search path. Unit coverage is in `ble_scan_resource_gate.test.mjs`.

## WiFiProv lookup cancellation and Android process quota

`node tests/hardware/ble_scan_lookup_gate.mjs` uses a random nonexistent name
for name/QR lookup. It checks overlap rejection, stop/clearState cancellation,
Direct scan handoff and the native 10 s not-found deadline. It refuses to clear
any existing provisioning selection. No GATT, SSID/Key or enrollment operation
is involved. Five actual starts may need an available budget: startup presence
also consumes that budget. Keep quota rejections as evidence, not as a pass or
an exclusivity failure; the gate does not wait/retry to manufacture success.

After GATT §10.86, a tiny Android monotonic/boot ledger survives process death.
This covers our own starts; some vendor SDKs can still silently reject stricter
limits. The process gate has explicit phases:

```sh
node tests/hardware/ble_scan_restart_gate.mjs prepare
# Confirm the exact DEBUG package, then force-stop/relaunch it WITHOUT pm clear.
# Re-forward CDP to the new WebView PID; do not change Bluetooth or the boards.
node tests/hardware/ble_scan_restart_gate.mjs after-restart
# Later, after the recorded cooldown, explicitly request recovery:
node tests/hardware/ble_scan_restart_gate.mjs recover
```

Prepare spends at most six attempts, stopping on the first failure. After-restart
must run before the original window expires and observe a pre-SDK THROTTLED
rejection, not a refunded successful scan. Recovery is a separate call requiring
an actual advertisement. No automatic pacing or force-stop is built into the
tool. Preserve all phase output and elapsed intervals. Both tools require the
signed-in idle debug `/home/device` page; leave bindings, credentials and state
unchanged. Their model tests do not certify a physical SDK stop exception.

## Paired offline phone and continuous Cloud caller (F3)

`node tests/hardware/edge_gateway_fairness_gate.mjs <local-config.json>` runs
the ordinary phone page against a single-connection child while an independent
account-side MQTT caller retains one real execution demand. The config contains
`child`, `instance`, `expectedCounter`, `appRoot`, `adb`, `phone`, `python`, and
the verified Hub serial `port`; it must not contain credentials. Both boards
must already have their original binding and the intended candidate firmware.

The tool bundles the existing App account SDK, obtains a NEW account session
from the signed-in idle debug App, and keeps the issued credential only in
process memory. It disables phone WiFi/mobile data, removes the two Cloud USB
reverse mappings, and restarts the same package without clearing App data.
The phone must stay unlocked. The original radio switches and 7000→17001 /
8884→8884 mappings are restored independently in `finally`.

Evidence includes normal-page fresh state, native disconnect and OS-observed
absence plus a full quiet interval, continuous wanted=1, Cloud loss/recovery,
and at least one COMPLETE recovered → yielded useful window before the next
recovery and cleanup. It performs no Action, reset, enrollment or firmware
write. Failures are fail-stop, not silently retried. If USB disappears, final
restoration is unconfirmed and must be checked after reconnection. These short
measurements do not establish release-level fairness or 10→30→100 stability.

## Online normal-page Direct yield (F4)

`node tests/hardware/edge_gateway_online_page_gate.mjs <local-config.json>`
uses the same non-secret config and independent production account caller.
It requires a verified engineering Hub advertising outer-Hello bit18 and a
Broker that negotiates it only for authenticated Edge Hubs. Ordinary devices
and synthetic child Hello must not acquire this capability by default.

Keep the phone online on the idle home page. The gate opens the ordinary child
page twice, observes accepted → actual physical Ready → native BLE connection,
checks fresh state without sending Actions, leaves normally and verifies native
close/OS release and Cloud recovery with the same caller owner. A third visit
cancels before Ready and must never start a native connection. Wrappers only
observe real promises/calls; they do not inject Ready or choose another route.
No reset, firmware write, network change, credential mutation or silent retry
is performed. Capture Hub diagnostics separately without resetting the board.
Failure stops the batch; inspect cleanup before retrying.

`node tests/hardware/edge_gateway_management_yield_gate.mjs <local-config.json>`
adds the exact `hub` ID to that config. It opens two original owner permit-join
windows (no candidate select, enrollment, attach, revoke or access mutation).
With one continuous Cloud caller, the first ordinary page must wait until the
window's own Close request before physical Ready/native connect. The second
page cancels while management stays Ready; the observer remains active for
7 seconds across the original owner's later Close to reject late acquisition.
Capture Hub `permitState=1/relayActive=1` and actual Direct wire alongside the
tool; HTTP subscription health alone is not management contention evidence.
The exact window is closed in `finally`, including after a lost HTTP response.

After that short negative gate passes, the online page gate accepts an explicit
`rounds: 10`, then separately `30`, then `100`. Default remains two visits. Each
stage keeps ONE real Cloud owner, checks every native close and Cloud recovery,
then performs a final cancel-before-Ready negative visit. Stop and review failures,
resource floors and cleanup before starting the next stage; no automatic stage
promotion or retry. Do not use loop count as a substitute for management/negative
coverage or release soak. These tests do not change firmware, keys or bindings.
The Cloud observer logs only whitelisted readiness facts and known error codes,
not raw Store values or exception messages. `run-start.unixMillis` anchors each
runner's elapsed clock; correlate serial/Broker capture origins before assigning
a recovery failure to Presence, manifest/state transfer or radio scheduling.

## Same-account ordinary WiFi witness (R4 / B1)

Import `runCloudWitnessAppGate(mode, logicalDeviceId)` from
`cloud_witness_app_gate.mjs` with the signed-in debug App on `/home/device`
and its CDP endpoint forwarded to localhost:9222. Use an authorized ordinary
WiFiQuickStart test device (`power`, `button`, `buttonPresses`). Modes are
`open`, `inspect`, `sample`, `action`, `start`, `close`.

This uses the App's EXISTING account MQTT/State interest, not the separate
account caller used by some Hub gates. `open` awaits interest acceptance and
fresh Manifest/State; `start` sends sequential alternating Property writes
with a 1 s gap, at most 24 and a 120 s lifetime. Action is latched once before
sending and never retried after an uncertain result. Changed account sessions
cannot send; close/deadline/observation loss releases only this witness.
Only the three standard test endpoints are used; output State includes just
power/buttonPresses, not credentials or arbitrary device fields.

Run concurrently with `lan_self_app_gate.mjs` on a DIFFERENT physical target,
revoke that target through the original authorized API, and correlate exact
MCU Retire ACK, native close, unchanged witness grant, continued real State
and ACKs, and unchanged account Session. Gateway AND both participating Brokers
must enable cross-shard requests before capturing the baseline. Mere HTTP
login/session rows, unchanged permission rows or an open MQTT socket are not
device communication evidence. Always `close` both gates in `finally`, restore
the original owner/runtime flags, and keep private account/firmware artifacts
outside version control. See the central R4 hardware report for evidence limits.

`node --test tests/hardware/cloud_witness_app_gate.test.mjs` covers the isolated
observer lifecycle; it does not replace the physical two-device test.

## Bounded BLE native timing (P1)

Serialize `bleNativeTimingGate` through `runGatewayAppGate`: `start` before a
normal-page measurement, `read` when needed, `stop` after closing that page.
It observes native BluetoothLe calls without changing their arguments or
returned promises. Output is bounded (512 events by default) and contains only
method, call sequence, settlement and elapsed time, never payloads or secrets.
Always stop in cleanup; do not stack observers or remove another wrapper.

`firstConnectAppGate` still defaults to the original policy. Explicit
`measureBlePhases: true, expectedTransport: 'ble', testConnectionPriority: 1`
requests Android HIGH once after GATT setup for an A/B experiment only. It is
not a production policy or proof that the requested interval took effect;
capture the actual Android GATT update and report unsupported/failed requests.
Keep packet size, authentication, cache ACK and inter-packet waits unchanged.

If Cloud is unavailable, `useCachedAccountForBle: true` permits measured BLE
only: it skips an extra account refresh, not exact owner/credential/device
admission. Record the offline boundary; it cannot pass Cloud comparison or a
whole-page control gate. Keep local IDs/results outside version control. Close
the page, restore both observers and verify native/JS resources after each visit.

`node --test tests/hardware/ble_native_timing_gate.test.mjs tests/hardware/first_connect_app_gate.test.mjs`
checks observer restoration and experiment boundaries, not physical performance.

## Optional WiFi task product (D-T3b-3, internal only)

`tests/arduino/Esp32WifiTaskLiveGate` uses the real four-slot product, DeviceKey
login, MQTT, time source and property callbacks. It is not a user example or a
release profile. Generate its private `GateConfig.h` in an ignored build
directory with the adjacent Service tool `tools/device-v2-task-product-live-gate.js`.
That tool requires an explicit disposable loopback PostgreSQL database named
`task_gate`, `TASK_GATE_HOST`, `TASK_GATE_OUTPUT`, `TASK_GATE_SSID` and
`TASK_GATE_PASSWORD`. It uses the existing build/test signing anchor, never a
production key or a forged user bearer token. Its isolated principal does not
constitute an App/account end-to-end test. Do not deploy its test HTTP/MQTT ports.

Compile the generated sketch with the library and linker property
`compiler.c.elf.extra_flags=-Wl,--wrap=nvs_set_blob -Wl,--wrap=nvs_commit`.
Back up and verify the dedicated board's whole Flash **before** uploading.
`task_product_gate.py --port <port> --serial <USB-serial> --out-dir <build/subdir>`
only accepts that exact board and this fixture's structured metrics. `--command
measure` resets counters; `--command reboot` is a software restart, not power
loss. Output is the sample **before** the command; inspect again for the result.
COM11/COM12 are protected witness ports. No auto-selection, erase or access reset.

The Service tool accepts bounded JSON lines on interactive stdin (`context`,
`manifest`, `state`, `control`, `task`, `retire`, `delete`); `quit` closes its
servers/schema. Task `operation` and typed bodies use the existing contract.
Intent IDs are recorded before I/O; an unknown Start must retain the original ID
and body. `drop-next-result` discards exactly one real Commit response before
Service consumes it; this is controlled receipt-loss injection, not an RF fault.
No MQTT retry queue, public task route or App code is introduced.

Record authenticated installation/query, actual callback/State, ordinary
Property/Action ACK, UTC and reconnect, then retirement/reboot/fresh proof.
Report `snapshots`, all-NVS `commits`, write latency, loop call duration and heap/
loop-task stack separately. They are finite measurements, not wear-life, all-SDK
stack admission or a maximum end-to-end latency guarantee. Restore the original
whole Flash and verify it before normal boot; stop only the test-owned database.
Private firmware, keys and raw account artifacts remain outside Git.

The current closed T2 fixture includes `TaskPilotPolicy.h`; the generator copies
it beside the sketch, and the host Engine regression consumes the same header.
It uses a capacity-10 token bucket refilling one write credit every 6 seconds,
empty at boot; global Start cooldown is 30 seconds, commit fault threshold 100 ms.
This is not a rolling-window limit or a production wear-life guarantee. Wait for
admission before Initialize/Start; a one-second countdown definition is still
valid after the Start cooldown. A full four-task due burst consumes eight writes;
do not exhaust credits with edits around that burst. Late work may be skipped,
never automatically restarted. A slow commit may already be durable when Fault
is reported; the threshold cannot interrupt Flash writes.

Batch 25's hardware sample used the former 16-credit / 1-second Start / 1-second
commit fixture. Keep that evidence unchanged. Batch 26 only validates the new
policy in host tests; compile and hardware admission of this profile belong to
the subsequent owner/App/MCU T2 gate, not to the earlier measurements.
