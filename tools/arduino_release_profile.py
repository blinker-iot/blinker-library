"""Validate and apply the configuration embedded in a public Arduino ZIP."""

from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
import re


HOST = re.compile(r"^[A-Za-z0-9](?:[A-Za-z0-9.-]{0,251}[A-Za-z0-9])?$")
FIRMWARE_VERSION = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]{0,62}$")
HEX = re.compile(r"^[0-9A-Fa-f]+$")
TEST_P256_KEY = (
    "046b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296"
    "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5"
)
TEST_ED25519_KEY = (
    "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"
)
# Reject the previously misencoded fixture even in development profiles; it
# cannot verify the intended test signer. Historical frozen packages stay intact.
MISENCODED_TEST_ED25519_KEY = (
    "d75a980182b10ab7d54bfed3c964073a0ee1723fdaa62325af021a68f707511a"
)
TASK_PILOT_ID = "esp32-wifi-t2-lab-v1"
# Finite measured workload only; not an endurance/production admission.
TASK_PILOT = {"CAPACITY": 4, "WINDOW_MS": 60000, "WRITES": 10,
              "MIN_START_MS": 30000, "MAX_COMMIT_MS": 100}


@dataclass(frozen=True)
class ServerKey:
    key_id: int
    public_key: bytes


@dataclass(frozen=True)
class ReleaseProfile:
    channel: str
    firmware_version: str
    control_host: str
    control_port: int
    cloud_session_tls: bool
    mqtt_tls: bool
    cloud_session_ca_pem: str
    mqtt_ca_pem: str
    allow_plain_storage: bool
    p256_keys: tuple[ServerKey, ...]
    ed25519_keys: tuple[ServerKey, ...]
    task_profile: str | None = None

    @property
    def production(self) -> bool:
        return self.channel == "production"


def require_object(value: object, name: str) -> dict:
    if not isinstance(value, dict):
        raise ValueError(f"release profile {name} must be an object")
    return value


def require_string(value: object, name: str) -> str:
    if not isinstance(value, str):
        raise ValueError(f"release profile {name} must be a string")
    return value


def require_bool(value: object, name: str) -> bool:
    if not isinstance(value, bool):
        raise ValueError(f"release profile {name} must be a boolean")
    return value


def require_keys(value: dict, expected: set[str], name: str) -> None:
    actual = set(value)
    if actual != expected:
        missing = sorted(expected - actual)
        unknown = sorted(actual - expected)
        raise ValueError(
            f"release profile {name} fields changed; "
            f"missing={missing}, unknown={unknown}"
        )


def parse_keys(value: object, name: str, size: int) -> tuple[ServerKey, ...]:
    if not isinstance(value, list) or not 1 <= len(value) <= 4:
        raise ValueError(f"release profile {name} must contain 1 to 4 keys")
    result: list[ServerKey] = []
    identifiers: set[int] = set()
    for index, raw in enumerate(value):
        item = require_object(raw, f"{name}[{index}]")
        require_keys(item, {"id", "public_key"}, f"{name}[{index}]")
        key_id = item.get("id")
        if isinstance(key_id, bool) or not isinstance(key_id, int):
            raise ValueError(f"release profile {name}[{index}].id must be an integer")
        if not 1 <= key_id <= 0xFFFFFFFF or key_id in identifiers:
            raise ValueError(f"release profile {name} has an invalid or duplicate key id")
        encoded = require_string(item.get("public_key"), f"{name}[{index}].public_key")
        if len(encoded) != size * 2 or not HEX.fullmatch(encoded):
            raise ValueError(f"release profile {name}[{index}].public_key must be {size} bytes of hex")
        public_key = bytes.fromhex(encoded)
        if size == 65 and public_key[0] != 0x04:
            raise ValueError("P-256 public keys must use uncompressed SEC1 form")
        identifiers.add(key_id)
        result.append(ServerKey(key_id, public_key))
    return tuple(result)


def parse_profile(payload: object) -> ReleaseProfile:
    payload = require_object(payload, "root")
    require_keys(
        payload,
        {
            "schema",
            "channel",
            "firmware_version",
            "control",
            "allow_plain_storage",
            "server_keys",
        } | ({"task_profile"} if "task_profile" in payload else set()),
        "root",
    )
    if payload.get("schema") != 1:
        raise ValueError("release profile schema must be 1")
    channel = require_string(payload.get("channel"), "channel")
    if channel not in ("development", "production"):
        raise ValueError("release profile channel must be development or production")
    task_profile = payload.get("task_profile")
    if "task_profile" in payload:
        if task_profile != TASK_PILOT_ID or channel != "development":
            raise ValueError("task_profile admits only esp32-wifi-t2-lab-v1 in development")
    firmware_version = require_string(payload.get("firmware_version"), "firmware_version")
    if not FIRMWARE_VERSION.fullmatch(firmware_version):
        raise ValueError("release profile firmware_version is invalid")
    control = require_object(payload.get("control"), "control")
    require_keys(
        control,
        {
            "host",
            "port",
            "cloud_session_tls",
            "mqtt_tls",
            "cloud_session_ca_pem",
            "mqtt_ca_pem",
        },
        "control",
    )
    host = require_string(control.get("host"), "control.host")
    if not HOST.fullmatch(host):
        raise ValueError("release profile control.host is invalid")
    port = control.get("port")
    if isinstance(port, bool) or not isinstance(port, int) or not 1 <= port <= 65535:
        raise ValueError("release profile control.port must be between 1 and 65535")
    cloud_tls = require_bool(control.get("cloud_session_tls"), "control.cloud_session_tls")
    mqtt_tls = require_bool(control.get("mqtt_tls"), "control.mqtt_tls")
    cloud_ca = require_string(control.get("cloud_session_ca_pem"), "control.cloud_session_ca_pem")
    mqtt_ca = require_string(control.get("mqtt_ca_pem"), "control.mqtt_ca_pem")
    if cloud_tls and "-----BEGIN CERTIFICATE-----" not in cloud_ca:
        raise ValueError("TLS cloud session profile requires a PEM CA certificate")
    if mqtt_tls and "-----BEGIN CERTIFICATE-----" not in mqtt_ca:
        raise ValueError("TLS MQTT profile requires a PEM CA certificate")
    if len(cloud_ca.encode("utf-8")) > 8192 or len(mqtt_ca.encode("utf-8")) > 8192:
        raise ValueError("release profile CA certificate exceeds 8192 UTF-8 bytes")
    keys = require_object(payload.get("server_keys"), "server_keys")
    require_keys(keys, {"p256", "ed25519"}, "server_keys")
    profile = ReleaseProfile(
        channel,
        firmware_version,
        host,
        port,
        cloud_tls,
        mqtt_tls,
        cloud_ca,
        mqtt_ca,
        require_bool(payload.get("allow_plain_storage"), "allow_plain_storage"),
        parse_keys(keys.get("p256"), "server_keys.p256", 65),
        parse_keys(keys.get("ed25519"), "server_keys.ed25519", 32),
        task_profile,
    )
    if any(key.public_key.hex() == MISENCODED_TEST_ED25519_KEY
           for key in profile.ed25519_keys):
        raise ValueError("release profile contains the misencoded Ed25519 test key")
    if profile.production:
        if host.endswith((".example", ".invalid")) or "replace-with" in host:
            raise ValueError("production release profile contains a placeholder host")
        if any(key.public_key.hex() == TEST_P256_KEY for key in profile.p256_keys):
            raise ValueError("production release profile contains the P-256 test key")
        if any(key.public_key.hex() == TEST_ED25519_KEY for key in profile.ed25519_keys):
            raise ValueError("production release profile contains the Ed25519 test key")
    return profile


def load_profile(path: Path) -> ReleaseProfile:
    return parse_profile(json.loads(path.read_text(encoding="utf-8")))


def cpp_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=True)


def render_config(profile: ReleaseProfile) -> str:
    cloud_tls = "true" if profile.cloud_session_tls else "false"
    mqtt_tls = "true" if profile.mqtt_tls else "false"
    plain_storage = "true" if profile.allow_plain_storage else "false"
    task_config = "\n".join(f"#define BLINKER_TASK_{key} {value}U"
                            for key, value in TASK_PILOT.items()) if profile.task_profile else ""
    return f"""#ifndef BLINKER_V2_ARDUINO_OFFICIAL_CONFIG_H
#define BLINKER_V2_ARDUINO_OFFICIAL_CONFIG_H

#include <stdint.h>

namespace blinker {{
namespace official {{

static const char releaseChannel[] = {cpp_string(profile.channel)};
static const char controlHost[] = {cpp_string(profile.control_host)};
static const uint16_t controlPort = {profile.control_port}U;
static const char firmwareVersion[] = {cpp_string(profile.firmware_version)};
static const char cloudSessionCaPem[] = {cpp_string(profile.cloud_session_ca_pem)};
static const char mqttCaPem[] = {cpp_string(profile.mqtt_ca_pem)};
static const bool cloudSessionTls = {cloud_tls};
static const bool mqttTls = {mqtt_tls};
static const bool allowPlainStorage = {plain_storage};

}} // namespace official
}} // namespace blinker

#define BLINKER_OFFICIAL_CLOUD_SESSION_TLS {int(profile.cloud_session_tls)}
#define BLINKER_OFFICIAL_MQTT_TLS {int(profile.mqtt_tls)}
{task_config}

#endif
"""


def render_key_header(profile: ReleaseProfile, *, p256: bool) -> str:
    keys = profile.p256_keys if p256 else profile.ed25519_keys
    guard = "BLINKER_V2_ARDUINO_ESP32_SERVER_KEYS_H" if p256 else "BLINKER_V2_ARDUINO_PORTABLE_SERVER_KEYS_H"
    include = "BlinkerV2/security/P256ServerKeyRingVerifier.h" if p256 else "BlinkerV2/security/Ed25519ServerKeyRingVerifier.h"
    key_type = "P256ServerPublicKey" if p256 else "Ed25519ServerPublicKey"
    arrays: list[str] = []
    descriptors: list[str] = []
    for index, key in enumerate(keys):
        values = [f"0x{byte:02X}U" for byte in key.public_key]
        lines = ["    " + ", ".join(values[offset:offset + 8]) for offset in range(0, len(values), 8)]
        arrays.append(f"static const uint8_t serverSigningKey{index}Bytes[] = {{\n" + ",\n".join(lines) + "\n};")
        descriptors.append(
            f"    {key_type}({key.key_id}U, ByteView(serverSigningKey{index}Bytes, sizeof(serverSigningKey{index}Bytes)))"
        )
    descriptor_body = ",\n".join(descriptors)
    return f"""#ifndef {guard}
#define {guard}

#include <{include}>

#define BLINKER_OFFICIAL_HAS_SERVER_SIGNATURE 1

namespace blinker {{
namespace official {{

{chr(10).join(arrays)}
static const {key_type} serverSigningKeys[] = {{
{descriptor_body}
}};
static const size_t serverSigningKeysCount =
    sizeof(serverSigningKeys) / sizeof(serverSigningKeys[0]);

}} // namespace official
}} // namespace blinker

#endif
"""


def apply_profile(stage_root: Path, profile: ReleaseProfile) -> None:
    config = stage_root / "src/BlinkerV2/arduino/config"
    (config / "OfficialConfig.h").write_text(render_config(profile), encoding="utf-8", newline="\n")
    (config / "Esp32ServerKeys.h").write_text(render_key_header(profile, p256=True), encoding="utf-8", newline="\n")
    (config / "PortableServerKeys.h").write_text(render_key_header(profile, p256=False), encoding="utf-8", newline="\n")
