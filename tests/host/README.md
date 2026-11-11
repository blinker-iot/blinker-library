# Device V2 host quality gates

The host suite builds the platform-independent core with strict warnings and
runs functional, cross-language golden and protocol robustness checks. The
deterministic fuzz target feeds bounded arbitrary input to:

- the BBP/2 header and exact-frame parser;
- the restricted canonical CBOR validator, typed readers and copy writer;
- the BLE fragment codec and the reassembly state machine, including timeout,
  disconnect/reconnect, packet-size changes and outbound fragmentation;
- the strict 13-byte BLE mode Service Data profile, including unknown version,
  mode/capability mismatch and exact decode/re-encode equality.
- the fixed 116-byte Wi-Fi credential profile record, including CRC,
  canonical zero padding, semantic validation and exact decode/re-encode.
- `device-key-session/1` challenge/session/error bodies, strict 43-character
  DeviceKey decoding and canonical re-encode equality; dedicated tests cover
  streamed proof HMAC, exact retry, challenge expiry, credential rotation,
  refresh lifetime and raw-key non-disclosure. The same target also runs the
  functional DeviceKey -> SessionCredential -> MQTT CONNECT/SUBSCRIBE ->
  BBP/2 publish/receive chain with real device-side components and fake I/O.
- the Arduino Client HTTP adapter stages network work until `poll()`, parses a
  bounded Content-Length response and terminates a stalled request at the
  caller-supplied `timeoutMillis` using an injected monotonic clock.
- EnrollmentTicket and LocalSetup request envelopes, including strict profile,
  version, map shape, operation and body-size boundaries. Dedicated state tests
  cover final Noise-hash binding, sequence gaps, exact replay, expiry and
  setup-window ID reuse/rotation.

The separate `blinker_diagnostics_output_tests` target builds the real core and
Arduino formatter against a minimal Print/IDF driver fake. It checks single
attempts, rejected/short outputs, summary scheduling, callback reentry and
reconfiguration, millis wrap, fixed-line overflow and zero-wait driver calls.
Win32 also checks the 92 B recorder ABI. It does not prove arbitrary Print or
USB hardware non-blocking; the ESP32-S3 `DiagnosticOutputGate` owns an IDF USB
console explicitly and is separate from the ordinary Arduino examples.

Run the sanitizer gate without creating build artifacts in the repository:

~~~powershell
python run_sanitized_tests.py --mode asan --iterations 200000
~~~

On Windows this uses MSVC AddressSanitizer and automatically exposes the
matching Visual Studio ASan runtime DLL. MSVC does not implement UBSan; a
Linux GCC/Clang job is therefore mandatory for the combined gate:

~~~sh
python3 run_sanitized_tests.py --mode asan-ubsan --iterations 200000
~~~

`BLINKER_USE_LIBFUZZER=ON` is also available for a Clang coverage-guided job.
It is intentionally separate from the deterministic smoke target: the smoke
target is a reproducible per-change gate, while a time-bounded libFuzzer corpus
job should retain crashing inputs and corpus growth as CI artifacts.

CI restores a persistent corpus cache, runs `seed_fuzz_corpus.py` so the exact
BBP/2, CBOR and BLE baseline inputs remain present, and then lets libFuzzer add
minimized mutations. A sanitizer/fuzzer failure must be copied into a reviewed
regression corpus before it is considered fixed; the mutable CI cache is
discovery state, not a normative protocol fixture.

## Capacity-only ABI gate

`blinker_resource_profile_tests` links the real portable core. Separate CMake
build trees can vary the transport/peer tables without selecting larger frames:

~~~powershell
cmake -S tests/host -B build/lan-small-win32-3 -G "Visual Studio 17 2022" -A Win32 -DBLINKER_HOST_RESOURCE_PROFILE=1 -DBLINKER_HOST_MAX_TRANSPORTS=3 -DBLINKER_HOST_MAX_PEER_SESSIONS=3
cmake --build build/lan-small-win32-3 --config Release --target blinker_resource_profile_tests
ctest --test-dir build/lan-small-win32-3 -C Release -R "^blinker_resource_profile_tests$" --output-on-failure
~~~

Run the same gate in another tree with both capacities set to 2 for comparison.
The overrides apply to every core/test translation unit; never put them only in
a Sketch. The full integration suite still defaults to Standard and has tests
that intentionally need its capacities. The capacity-only variant does not
claim that full suite ran at Small, or that a third LAN transport/peer handshake
exists. Published Arduino defaults remain unchanged.

S3a-1b stores two access-service pointers in each existing transport slot and
removes Runtime's two global pointers. Current Win32 Small Device sizes are
1,672 B (2/2) and 1,760 B (3/3); capacities and frame/schema defaults are unchanged,
but the object-size ceilings were explicitly revised. For default Standard and
Large ABI probes, use separate trees with BLINKER_HOST_RESOURCE_PROFILE=2/3 and
omit both capacity overrides (measured Device 3,460/11,004 B).

The standard integration suite also exercises three actual Runtime/Hub paths
with fake access services: per-path Hello/auth/control/PresenceKey dispatch,
same-session-ID isolation, native/non-native close, reindex/reuse, stopped-only
registration, start rollback and BLE lifecycle destruction. This is a dispatch
gate, not a production LAN transport, cryptographic handshake or hardware gate.

S3a-1c additionally exercises the production child executor boundary. Two
independent upstream owners may contribute demand, but only one southbound
request is in flight. A busy second owner cannot consume a sequence or replace
the first response callback; ACK is returned to the originating owner, recent
replay keeps its child sequence, and the next accepted request advances the
single allocator. The Cloud adapter test also retains admission, projection,
MQTT-generation and exact-frame replay coverage. This is not a WebSocket or
local-authorization test.

The probe prints pointer width, frame/table capacities and object sizes. A
32-bit MSVC result is an ABI comparison, not ESP32/Renesas/SAMD runtime heap,
TLS stack, modem-socket or radio admission evidence.

S3b-2b2a extends the existing LocalAccess test with authenticated control Hello,
authority Enable/Disable, precise retirement and exact ACK/replay. It covers
an installed A and an in-flight never-installed A grant, preserving established
B permissions/deadline, retrying other pending challenges without a fixed wait,
ACK loss/retransmission, sequence gaps/conflicts, reflection, tampering, small or
overlapping buffers and a reboot nonce. Protocol fuzz now also round-trips Control.
The wire goldens remain in the shared `tests/golden/local_access_golden_vectors.properties`.
Win32 sizes are lease144/admission168/authority328/challenge52/control-receiver84 B;
the last is per product, not per child or MQTT session. Sender persistence,
Broker routing, product wiring and actual MCU revocation are not covered by this
software gate. Existing natural-expiry tombstones are not removed by this change.

LAN section 20 extends `blinker_http_device_key_session_tests` with the optional
`LocalAccessCloudTransport`: real C++ login/ManagedMQTT/control codecs, fake network
ports, one bounded inbox, deferred ACK, retransmission and reconnect/Key isolation.
LAN section 21 replaces the product's old Hello/bootstrap timer with one fresh
progress query round (four attempts, 10-second deadline, at most one new round
per minute). Signed Synchronized/Pending/Refresh bind proof, boot and nonce;
status is not an ACK. Tests cover exact replay, consumed/expired/wrong-nonce and
old-proof rejection, configured silence/stalled Pending recovery, and the independent
300-second proof-refresh limit. This is not a physical Broker/MCU loop. Run through
`run_sanitized_tests.py`; the Win32 Release target prints provider296 and
single-slot wrapper488 B (+36 from section 20). Control receiver remains84 B.
The protocol golden and deterministic fuzz corpus include Query/Status.
LocalAccess tests cover no-traffic expiry latching and
challenge expiry across a uint32 clock cycle; the original 15-second tombstone
window remains. No ordinary product allocates the wrapper unless the internal
library-wide `BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL=1` gate is enabled.

Sanitizer compilation alone is not a pass. CTest and the requested fuzz
iterations must exit successfully with `halt_on_error=1`; on Linux the gate
also enables LeakSanitizer and stops on the first undefined behavior report.
The MSVC runtime does not support `detect_leaks`, so the Windows gate disables
that ASan option explicitly instead of failing before any test executes.

LAN section 23 adds `blinker_local_access_transport_tests`: a real authority,
Noise state machine, single-self frame transport and Runtime over a fake bounded
message link. Shared `support/DeterministicNoiseCrypto.h` is deliberately NOT
AES-GCM/X25519; golden/provider tests remain independent. Covers handshake/order,
wrong domain/PSK/MAC, exact ciphertext backpressure, replay, expiry/revoke/Key,
socket replacement and callback ownership, reconnect deadlines, Observe-only
Manifest versus Control-only Command and outbound Patch permission checks.
Run with the standard CMake/CTest or sanitized runner above. It is not a native
WebSocket, mDNS, actual MCU crypto or App-to-MCU interoperability gate.
The historical section-23 Win32 transport embedded Noise (448 B). Section 30
injects separate Plain/Noise record sessions into the same transport. Its
sizeof alone is no longer the complete connection-state cost; count the
selected record object, external RX/TX buffers, authority and native port too.

LAN section 24 adds `blinker_websocket_ingress_tests` (all header split points,
mask/RSV/opcodes/canonical length, bounded continuation) and
`blinker_esp32_local_websocket_tests` (actual ESP32 port source, mock HTTPD/lwIP,
clock/events/mDNS). The latter checks short writes, pending-output retirement,
mailbox backpressure, stable borrowed RX during network callbacks, HTTP total
deadline/cap/read-ahead rejection, fragmented-message deadline across Ping,
Close validation, session generation and discovery ownership/first initialization.
The complete suite is now 57 tests plus 20,000 deterministic fuzz iterations.
This mock is NOT an HTTP/WS parser, a FreeRTOS scheduler or a radio emulator;
real SDK concurrency, stack/heap peaks, socket failures and App-to-MCU traffic
remain hardware gates. Native RX mailbox and borrowed RX are separate 420 B
buffers; do not reuse the core transport sizeof as a full product RAM estimate.

LAN section 30 runs the same transport/Runtime permissions and finite-lease
checks for profile 1 (Noise) and profile 2 (handshake proof, then plain BBP).
The Plain tests cover shared golden proofs, direction/reflection, exact mode
binding, pre-authentication input, backpressure and key/revocation/expiry. Run
`blinker_device_v2_tests` too: an untrusted local transport cannot gain access
by setting the Rx authenticated flag without explicit authentication capability.
Host-64 record objects are 65 B Plain / 280 B Noise; shared transport is 248 B.
These are host ABI measurements, not MCU heap/stack or full product footprints.
The public WiFiQuickStart three-build comparison (LAN off / Plain / Noise) uses
the same Arduino ESP32 SDK/FQBN and placeholder credentials. The private
Esp32LanSelfGate runs its SDK GCM known-answer test only in Noise mode; it must
not pull GCM test code into a Plain resource measurement or report a skipped
crypto test as passed. Internal flags apply to every compilation unit.
