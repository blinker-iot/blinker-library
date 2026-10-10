#!/usr/bin/env python3
"""Fail if the P6 ESP32 image links the wrong provisioning/BLE graph."""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess


COMMON_REQUIRED = (
    "NimBLEDevice::",
    "DeviceKeySession",
    "PubSubClient",
    "blinker::bbp2::",
    "ControllerHmacSha256",
)
COMMON_FORBIDDEN = (
    " BLEDevice::",
    "esp_bluedroid_",
    "CloudCredentialStore",
    "OwnershipRecordStore",
    "LocalSetupSagaStore",
    "blinker::security::Noise",
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nm", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument(
        "--provisioning",
        choices=("ble", "softap"),
        default="softap",
    )
    parser.add_argument("--require-tls", action="store_true")
    args = parser.parse_args()

    symbols = subprocess.run(
        [str(args.nm), "-C", str(args.elf)],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    required_scheme = (
        "network_prov_scheme_ble"
        if args.provisioning == "ble"
        else "network_prov_scheme_softap"
    )
    forbidden_scheme = (
        "network_prov_scheme_softap"
        if args.provisioning == "ble"
        else "network_prov_scheme_ble"
    )
    missing = [value for value in COMMON_REQUIRED if value not in symbols]
    if required_scheme not in symbols:
        missing.append(required_scheme)
    if args.require_tls and "mbedtls_ssl_" not in symbols:
        missing.append("mbedtls_ssl_")
    leaked = [value for value in COMMON_FORBIDDEN if value in symbols]
    if forbidden_scheme in symbols:
        leaked.append(forbidden_scheme)
    if missing or leaked:
        if missing:
            print("missing required symbols: " + ", ".join(missing))
        if leaked:
            print("forbidden symbols linked: " + ", ".join(leaked))
        return 1
    print(f"ESP32 P6 {args.provisioning} scheme/host audit passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
