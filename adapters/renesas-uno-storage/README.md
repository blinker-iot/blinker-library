# Blinker UNO R4 WiFi Storage Adapter

This optional adapter exposes six `IAtomicBlobStore` views from one WiFiS3
`Preferences` namespace: independent cloud, ownership and four-controller
table records plus the selector/two slots used by `WifiCredentialStore`.

The Arduino Renesas `Preferences` implementation sends commands to the UNO R4
WiFi ESP32-S3 connectivity module; it is not RA4M1 local Data Flash. The adapter
deliberately avoids RA4M1 `EEPROM`, whose public 8 KB address space has no
partition reservation mechanism and whose 1 KB erase blocks could collide with
an application.

Each logical blob alternates between two independent keys. Records contain a
store id, monotonic generation, state, payload length and CRC-32. Replacement
writes and reads back the inactive key before it can become newest; clear writes
a tombstone instead of erasing both copies. This supplies complete old-or-new
recovery assuming one failed connectivity-module key update does not corrupt a
different key.

Limits and release gates:

- storage is `PlainFlash`; it is not encrypted or hardware sealed;
- `Preferences` does not expose detailed read errors, so a transient modem read
  failure can appear as a missing/corrupt record and must fail device startup;
- the adapter is single-threaded and keeps one namespace open;
- ordinary boot loads the independent ownership, cloud and controller records;
  the controller table uses `ct0/ct1` and never shares a credential blob with
  either ownership or cloud state;
- the current pre-enrollment probe only installs cloud/network and is not yet an
  ownership-complete onboarding flow;
- real reset-during-write tests on the pinned connectivity firmware remain a
  release gate. Host fault injection verifies the journal algorithm, not flash
  physics or the co-processor firmware.
