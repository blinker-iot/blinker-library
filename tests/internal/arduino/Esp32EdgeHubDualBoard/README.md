# Internal Edge Hub hardware bench

This existing bench explicitly substitutes **unprotected test storage** for the
production encrypted-NVS requirement. Never publish this platform alias or claim
its success proves the eFuse/protected-storage release gate. It retains actual
TLS, SNTP, gateway management, Method 2 and DirectSecure; no fake auth/time source.

`BLINKER_TEST_ENABLE_TIME` installs one observed real `TimeSyncService` for the Hub
and two optional `GatewayTimeResponder` objects borrowing that source. The normal
executor owns negotiation, permissions and the existing bounded output queue.
Without the macro, neither time service nor responders are created. `u` only
queries accepted sample counts, source, age and uncertainty. It does not reset
time, grant access or send a synthetic child request. `y` queries bounded real
handler request/reply counts, permissions, sequence, result and encoded size;
it does not imply that the reply was enqueued or accepted by the child.

For reproducible installed-ZIP builds, use a generated ignored Sketch with a
current TLS profile and copy `support/ObservedTime.h` beside it; replace only the
test-local includes. Do not carry an old host/CA header into a new release profile.
The TLS control port is not the plain HTTP port. Preserve persistent Hub identity,
Key and child records; verify board/app hashes before changing any firmware.

This bench also retains earlier explicit fault commands used by other gates.
For the time gate use only `u`/`y`, slot selection `0`/`1`, normal disconnect `d` and snapshots;
do not arm access-reset or revocation/renewal faults incidentally. A compiled
responder is not evidence of a real child TimeRequest/TimeResponse exchange.

With both `BLINKER_TEST_ENABLE_TIME` and `BLINKER_TEST_TIME_SOURCE_OUTAGE`, `v`
once per boot makes only the child time provider unavailable for 45 seconds.
The Hub's real business service, security/SNTP clock and control paths keep running.
The actual responder must produce correlated Error replies and later recover;
there is no synthetic response, access reset or default-hour expiry claim.
