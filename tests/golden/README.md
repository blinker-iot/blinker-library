# Device V2 cross-language golden vectors

This directory freezes the wire contracts that are already shared by the
device library, App and service implementations. Each checked-in fixture is
generated independently by production C++, browser-safe TypeScript and Node
where all three implementations exist.

Current cross-language gates cover:

- BBP/2 frames, CBOR bodies, Manifest pages and 4-byte BLE fragmentation;
- Noise NNpsk0 handshake, records and transport keys;
- Controller HMAC authentication;
- DirectSecureRecord KDF, bidirectional AES-GCM records and ATT20 fragments;
- Controller install/rotate/revoke grants and receipts;
- BLE-only EnrollmentGrant v2 on device and service, including the signed
  security profile and Ed25519/P-256 signatures;
- DeviceKey cloud-session request and response;
- the server-side P-256 identity fixture.

The old EnrollmentTicket/Ownership/LocalSetup/CloudEnrollment aggregate graph
is retired. `BleEnrollmentGrant` v2 plus the two-message BLE enrollment
application now has exact device/service vectors. The App case remains outside
this gate until its production BLE enrollment client is implemented; it must
consume this same fixture rather than introduce a second wire model.

The BBP/2 fixture includes values, Keyed/Field-ID maps, State and Manifest
pagination, complete frames, BLE packets, and malformed inputs. Its canonical
Manifest profile fixes numeric constraints at CBOR binary64; integers and
lengths still use their shortest encoding.

The portable Noise fixture is checked against real OpenSSL X25519 and
AES-256-GCM plus the public Cacophony vector. Replay providers in this folder
are test harnesses, not production cryptography.

The DirectSecureRecord fixture is generated and verified with Node/OpenSSL
AES-256-GCM. Device C++ and browser-safe TypeScript independently derive the
Method 2 keys, nonce, AAD, direction and sequence, then replay only the frozen
AES primitive outputs. It also freezes representative Command/Ack/Patch sizes
with the 4-byte fragment header at the minimum 20-byte ATT payload.

No verifier calls another verifier or uses the Python size benchmark as an
oracle. C++ `--emit` modes are reserved for intentional, reviewed wire-version
changes; normal CI only compares against checked-in expected values.

## Run

Build the Host CMake targets, then run:

```powershell
powershell -ExecutionPolicy Bypass -File tests/golden/run_cross_language_golden.ps1
```

Pass the corresponding `-*CppExecutable` argument when a target is outside
`tests/host/build`, and `-TypeScriptCompiler` to pin the App compiler. CI uses
TypeScript 4.3.5. These small implementations are conformance references, not
the production App or service architecture.

## Task management grant (internal candidate)

The task grant v1 fixture is shared by `blinker_task_management_grant_tests`
(run by CTest) and the independent Node oracle:

```powershell
node tests/golden/task_management_grant_oracle.cjs tests/golden/task_management_grant_vectors.properties
```

C++ checks the real codec, streaming SHA-256 and expected-binding verification
with an explicitly fake signature boundary. Node checks the same fixture using
real Ed25519 and raw P-256 signatures plus transcript/signature tampering.
The fixture contains test-only public keys, never production trust roots.
This is not a deployed Service signer, an App implementation, or proof that
nonce consumption, scope bootstrap or a public task BBP kind is implemented.
