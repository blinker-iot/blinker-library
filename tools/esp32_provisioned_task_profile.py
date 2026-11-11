"""Render an explicit internal ESP32/four-slot profile, defaulting to SoftAP.

This prepares source only. Public verification material is static compile input,
not evidence that a running Gateway signs with it or that any device is admitted.
"""
from __future__ import annotations

import argparse
from dataclasses import replace
import hashlib
import json
from pathlib import Path

import arduino_release_profile as release


PROFILE_ID = "esp32-softap-task-qa3-v1"
INTERNAL_MACRO = "BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS"
VERSION = "qa3-softap-task-link-20261031"
BLE_PROFILE_ID = "esp32-ble-task-da4-v1"
BLE_INTERNAL_MACRO = "BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS"
BLE_VERSION = "da4-ble-task-link-20261103"
PREFIX = "src/BlinkerV2/arduino/config/"


def checked_bearer(bearer: str) -> str:
    if bearer not in ("softap", "ble"):
        raise ValueError("internal provisioned tasks require an explicit supported bearer")
    return bearer


def internal_profile(transport: release.ReleaseProfile,
                     signing: release.ReleaseProfile,
                     bearer: str = "softap") -> release.ReleaseProfile:
    checked_bearer(bearer)
    if not transport.cloud_wifi_only or not transport.production:
        raise ValueError("internal provisioned tasks require the unchanged public Cloud transport profile")
    if (transport.control_port != 443 or not transport.cloud_session_tls or not transport.mqtt_tls
            or not signing.cloud_task_pilot or signing.production
            or len(signing.p256_keys) != 1 or signing.p256_keys[0].key_id != 1
            or signing.ed25519_keys
            or signing.p256_keys[0].public_key.hex() == release.TEST_P256_KEY):
        raise ValueError("internal provisioned tasks require fixed TLS and one non-fixture P256 keyId 1")
    # Reuse the existing reviewed storage policy/public signer projection. It is
    # no longer D4/manual and must never inherit that profile's graph guard.
    return replace(signing, channel="development",
                   firmware_version=BLE_VERSION if bearer == "ble" else VERSION,
                   control_host=transport.control_host, control_port=transport.control_port,
                   cloud_session_tls=transport.cloud_session_tls, mqtt_tls=transport.mqtt_tls,
                   cloud_session_ca_pem=transport.cloud_session_ca_pem,
                   mqtt_ca_pem=transport.mqtt_ca_pem, task_profile=None, product_profile=None)


def graph_guard(bearer: str = "softap") -> str:
    checked_bearer(bearer)
    label = "BLE" if bearer == "ble" else "SoftAP"
    lines = [f"// Internal QA-3: ESP32 {label} + four Cloud task slots; not admission.",
             "#if !defined(ARDUINO_ARCH_ESP32)",
             f'#error "QA3 {label} tasks support ESP32 only"', "#endif"]
    forbidden = ["BLINKER_PRODUCT_WIFI", "BLINKER_PRODUCT_BLE",
                 "BLINKER_ESP32_PROVISIONING_SOFTAP" if bearer == "ble" else "BLINKER_ESP32_PROVISIONING_BLE",
                 "BLINKER_V2_ARDUINO_ESP32_EDGEHUB_PRODUCT_H",
                 "BLINKER_V2_ARDUINO_ESP32_EDGEHUB_PLATFORM_H",
                 "BLINKER_V2_ARDUINO_UNO_R4_BLE_PLATFORM_H",
                 "BLINKER_V2_ARDUINO_WIO_TERMINAL_BLE_PLATFORM_H",
                 "BLINKER_UNO_R4_WIFI_TASK_EXTENSION_H",
                 "BLINKER_WIO_TERMINAL_WIFI_TASK_EXTENSION_H",
                 "BLINKER_OFFICIAL_PRODUCT_PROFILE"]
    if bearer == "softap":
        forbidden.append(BLE_INTERNAL_MACRO)
    for name in forbidden:
        lines += [f"#if defined({name})", '#error "QA3 profile forbids this product graph"', "#endif"]
    for name in ["BLINKER_INTERNAL_LAN_WEBSOCKET", "BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL",
                 "BLINKER_INTERNAL_LAN_ENCRYPTED", "BLINKER_INTERNAL_UNO_R4_TASKS",
                 "BLINKER_INTERNAL_WIO_TASKS"]:
        lines += [f"#if defined({name}) && {name} != 0",
                  '#error "QA3 profile forbids local and non-ESP32 task extensions"', "#endif"]
    supplied = [(INTERNAL_MACRO, 1), ("BLINKER_ENABLE_TASKS", 1),
                ("BLINKER_ESP32_PROVISIONING_" + bearer.upper(), 1)]
    if bearer == "ble":
        supplied.append((BLE_INTERNAL_MACRO, 1))
    supplied += [("BLINKER_TASK_" + key, value) for key, value in release.TASK_PILOT.items()]
    for name, value in supplied:
        lines += [f"#if defined({name}) && {name} != {value}U",
                  f'#error "QA3 requires its fixed {label} four-slot configuration"', "#endif",
                  f"#ifndef {name}", f"#define {name} {value}U", "#endif"]
    for name in ["BLINKER_OFFICIAL_CLOUD_SESSION_TLS", "BLINKER_OFFICIAL_MQTT_TLS"]:
        lines += [f"#if defined({name}) && {name} != 1",
                  '#error "QA3 requires HTTPS/MQTTS"', "#endif"]
    return "\n".join(lines) + "\n"


def configured_files(root: Path, transport: release.ReleaseProfile,
                     signing: release.ReleaseProfile, bearer: str = "softap") -> dict:
    internal = internal_profile(transport, signing, bearer)
    files = release.configured_library_files(root, internal)
    guard = graph_guard(bearer)
    files[PREFIX + "OfficialConfig.h"] = (guard + release.render_config(internal)).encode()
    files[PREFIX + "Esp32ServerKeys.h"] = (guard + release.render_key_header(internal, p256=True)).encode()
    portable = release.render_key_header(internal, p256=False)
    # The ordinary renderer prefixes an empty portable ring with its manual
    # task guard. Replace only that exact generated prefix for this new graph;
    # do not change the public renderer or weaken the existing D4 guard.
    expected = release.cloud_task_guard()
    if not portable.startswith(expected):
        raise ValueError("portable null-ring renderer changed; review before preparation")
    files[PREFIX + "PortableServerKeys.h"] = (guard + portable[len(expected):]).encode()
    files[release.LIBRARY_IDENTITY_HEADER] = release.render_library_identity(files).encode()
    return files


def apply_internal_profile(library: Path, boundary: Path,
                           transport: release.ReleaseProfile,
                           signing: release.ReleaseProfile, bearer: str = "softap") -> dict:
    library, boundary = library.resolve(), boundary.resolve()
    if library == boundary or not library.is_relative_to(boundary) or not (library / "library.properties").is_file():
        raise ValueError("only an existing isolated internal library may be configured")
    if library.is_symlink() or boundary.is_symlink():
        raise ValueError("internal profile boundary must not be linked")
    files = configured_files(library, transport, signing, bearer)
    for name, content in files.items():
        if isinstance(content, bytes):
            target = library / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)
    return {"profileId": BLE_PROFILE_ID if bearer == "ble" else PROFILE_ID,
            "internalOnly": True, "notPublicRelease": True,
            "deployable": False, "runtimeSigningMatched": False, "formalAdmission": False,
            "bearer": bearer, "taskCapacity": 4, "signatureAlgorithm": 2,
            "p256KeyId": signing.p256_keys[0].key_id,
            "p256RawSha256": hashlib.sha256(signing.p256_keys[0].public_key).hexdigest(),
            "rootMode": "DeviceKey+Admin", "allowPlainStorage": signing.allow_plain_storage}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--boundary", type=Path, required=True)
    parser.add_argument("--transport-profile", type=Path, required=True)
    parser.add_argument("--signing-profile", type=Path, required=True)
    parser.add_argument("--bearer", choices=("softap", "ble"), default="softap")
    args = parser.parse_args()
    print(json.dumps(apply_internal_profile(args.library, args.boundary,
          release.load_profile(args.transport_profile), release.load_profile(args.signing_profile), args.bearer)))


if __name__ == "__main__":
    main()
