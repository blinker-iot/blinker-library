# Blinker ESP32 Storage Adapter

This optional package provides two layers:

- `Esp32NvsAtomicBlobStore` implements `IAtomicBlobStore` with one ESP-IDF
  NVS blob. It is intentionally record-agnostic; official composition uses it
  below `CloudCredentialStore`, `OwnershipRecordStore` and
  `ControllerCredentialStore`;
- `Esp32NvsWifiCredentialSink` composes three narrow blob stores with the
  portable `WifiCredentialStore` two-slot journal. It stores Wi-Fi credentials
  in a separate namespace and does not depend on `WiFi.h` or own the radio.

Important constraints:

- call `begin()` after the Arduino ESP32 core has initialized NVS;
- one `nvs_set_blob()`/`nvs_erase_key()` plus `nvs_commit()` is one logical
  replacement, so recovery exposes the previous complete value or the new
  complete value, never a partially decoded credential record;
- namespace and key are copied into the adapter and limited to 15 bytes;
- the default maximum blob size is 256 bytes; official composition explicitly
  selects 116 bytes for cloud, 112 bytes for ownership and 268 bytes for the
  four-controller table;
- the Wi-Fi journal uses `wifi_sel` (16 bytes), `wifi_0` (116 bytes) and
  `wifi_1` (116 bytes) in the default `bl_v2_net` namespace. It writes the
  inactive full profile before atomically switching the selector, so a reset
  exposes the old or new complete profile and retains one rollback target;
- `Esp32NvsWifiCredentialSink` rejects plain flash by default. An encrypted
  store now requires both `encrypted=true` and a non-null
  `encryptionVerifier`; `begin()` fails closed unless that callback attests the
  selected partition (`"nvs"` for the default partition). The callback must be
  backed by the product's secure NVS initialization/release evidence;
- neither flag nor callback enables encryption. They only prevent an
  unverified partition from being reported as `EncryptedFlash`.
  `allowPlainFlash=true` remains an explicit development-only opt-in;
- this is not `HardwareBackedSealed`: a cloud authentication secret is read into
  trusted RAM for TLS/login/HMAC use;
- real power-cut testing remains a hardware release gate. Host fault injection
  verifies adapter state/error behavior but cannot prove flash physics.

ESP-IDF documents NVS as providing sudden-power-loss protection through atomic
updates; an interrupted write may lose the new key-value pair but must not turn
the previous value into a partial record:
https://docs.espressif.com/projects/esp-idf/en/v5.3.3/esp32/api-reference/storage/nvs_flash.html

The generic blob default is `bl_v2_blob` / `record`. Official WiFi composition
explicitly uses `bl_v2_cloud` / `credential`; Wi-Fi uses `bl_v2_net` and the
three keys above. The ownership record uses `bl_v2_owner` / `binding`.
The controller table uses the independent `bl_v2_ctrl` / `table` blob.
Network reset clears only the Wi-Fi journal; authorized ownership reset writes
an Unclaimed generation tombstone instead of erasing the anti-rollback floor.
