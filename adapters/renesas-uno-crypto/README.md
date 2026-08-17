# Blinker UNO R4 crypto adapter

`RenesasUnoP256Verifier` implements only the P-256 digest verification needed
for pinned server grants. It delegates verification to the `SoftwareATSE`
service supplied by Arduino Renesas UNO Core 1.5.3 and strips the SEC1 `0x04`
prefix before passing the 64-byte public-key body to that service.

Call `begin()` after the connectivity module is available and `end()` before
teardown. The adapter deliberately does not implement `IP256CryptoProvider`:
SoftwareATSE keeps generated private keys inside its own store and cannot
satisfy that raw-private-key interface safely.
