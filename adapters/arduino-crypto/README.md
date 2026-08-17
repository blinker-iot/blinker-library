# Blinker Arduino crypto adapter

`ArduinoCryptoProvider` is the portable fallback for
`INoiseCryptoProvider`, `IX25519AesGcmCryptoProvider` and the verification-only
`IEd25519DigestVerifier`. It uses pinned MIT-licensed `Crypto@0.4.0`, so one
provider can protect BLE Noise records, cloud credential envelopes and verify
pinned server grants on MCUs without a suitable native crypto backend.

The adapter does not use `Crypto`'s RNG or Ed25519 signing API. Entropy stays
behind Blinker `IRandom`; Ed25519 inputs pass Blinker canonical encoding checks
before verification. This avoids the dependency's independently supplied
public/private signing-key API entirely. Native MCU backends remain preferable
when they pass the same vectors and resource gates.

This adapter owns no long-lived key material and performs no heap allocation.
Authentication failure clears the provisional plaintext before returning.
