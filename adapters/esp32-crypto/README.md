# Blinker ESP32 Crypto Adapter

This optional package supplies the ESP32-native primitives required by
portable onboarding and the shared `NoiseNnSession` (NN/NNpsk0):

- X25519 public-key derivation and Diffie-Hellman through ESP-IDF mbedTLS;
- AES-256-GCM with a 16-byte authentication tag;
- raw low-S P-256/SHA-256 server signature verification;
- `esp_fill_random()` through the common `IRandom` seam.

The package does not own NimBLE, GATT, Wi-Fi credentials, claim state, NVS,
MQTT or the provisioning lifecycle. Those remain separate composition layers.
It requires Arduino-ESP32 3.3.1/ESP-IDF 5.5 features
`MBEDTLS_ECP_DP_CURVE25519_ENABLED` and `MBEDTLS_GCM_C`.

`CompileEsp32CryptoAdapter` links RFC 7748 X25519, NIST AES-256-GCM and P-256
verification self-checks into the firmware. A successful compile proves SDK
availability and resource size only; the self-check must still run on real
hardware before release. Authentication failure zeroes provisional plaintext.
