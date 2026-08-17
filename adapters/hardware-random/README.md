# Blinker hardware random adapter

`PlatformHardwareRandom` implements the portable `IRandom` boundary without
using Arduino `random()`/`randomSeed()`:

- ESP32 uses ESP-IDF `esp_fill_random()`;
- UNO R4 uses the Renesas SCE hardware RNG.

Cloud session request IDs, BLE session IDs and provisioning nonces may share
this adapter. Keeping it independent from Wi-Fi, BLE and Noise prevents a
Wi-Fi-only product from importing a radio stack merely to obtain a CSPRNG.

The adapter is compile-tested against ESP32 Arduino Core 3.3.1 and Arduino
Renesas UNO Core 1.5.3. A new MCU must provide an equivalent cryptographic
generator; `rand()` and time-seeded PRNGs are not acceptable substitutes.
