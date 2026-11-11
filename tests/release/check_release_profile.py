#!/usr/bin/env python3
"""Exercise production release-profile validation without writing artifacts."""

from __future__ import annotations

import copy
import json
from pathlib import Path
import re
import sys


REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY / "tools"))

from arduino_release_profile import (
    parse_profile, render_config, render_key_header, TASK_PILOT_ID, TASK_PILOT,
    TEST_ED25519_KEY, MISENCODED_TEST_ED25519_KEY,
)


PROFILE = REPOSITORY / "tests/release/development_profile.json"


def rejected(payload: dict, expected: str) -> None:
    try:
        parse_profile(payload)
    except ValueError as error:
        if expected not in str(error):
            raise AssertionError(f"expected {expected!r}, got {error!r}") from error
        return
    raise AssertionError(f"profile unexpectedly accepted; expected {expected!r}")


def main() -> int:
    payload = json.loads(PROFILE.read_text(encoding="utf-8"))
    development = parse_profile(payload)
    assert not development.production
    portable_header = (REPOSITORY / "src/BlinkerV2/arduino/config/PortableServerKeys.h").read_text(encoding="utf-8")
    portable_array = re.search(r"serverSigningKeyBytes\[\]\s*=\s*\{([^}]+)\}", portable_header)
    assert portable_array, "portable test key declaration missing"
    portable_key = bytes(int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})U", portable_array[1]))
    assert portable_key.hex() == TEST_ED25519_KEY == payload["server_keys"]["ed25519"][0]["public_key"]
    assert development.ed25519_keys[0].public_key == portable_key
    assert "0xF3U" in render_key_header(development, p256=False)
    broken_development = copy.deepcopy(payload)
    broken_development["server_keys"]["ed25519"][0]["public_key"] = MISENCODED_TEST_ED25519_KEY
    rejected(broken_development, "misencoded Ed25519 test key")
    assert "BLINKER_OFFICIAL_MQTT_TLS 0" in render_config(development)
    assert "P256ServerPublicKey(1U" in render_key_header(development, p256=True)
    assert "BLINKER_TASK_CAPACITY" not in render_config(development)
    pilot = copy.deepcopy(payload)
    pilot["task_profile"] = TASK_PILOT_ID
    rendered = render_config(parse_profile(pilot))
    fixture = (REPOSITORY / "tests/arduino/Esp32WifiTaskLiveGate/TaskPilotPolicy.h").read_text(encoding="utf-8")
    for name, value in TASK_PILOT.items():
        assert f"#define BLINKER_TASK_{name} {value}U" in rendered
        assert f"#define BLINKER_TASK_{name} {value}" in fixture
    assert "BLINKER_ENABLE_TASKS" not in rendered  # Sketch still explicitly opts in.
    for invalid in (None, True, 4, {}, "unknown"):
        pilot["task_profile"] = invalid
        rejected(pilot, "task_profile admits only")
    pilot["task_profile"] = TASK_PILOT_ID
    pilot["channel"] = "production"
    rejected(pilot, "task_profile admits only")

    unknown = copy.deepcopy(payload)
    unknown["control"]["mqtt_security"] = "plain"
    rejected(unknown, "unknown=['mqtt_security']")

    production = copy.deepcopy(payload)
    production["channel"] = "production"
    rejected(production, "placeholder host")

    production["control"]["host"] = "iot.diandeng.tech"
    rejected(production, "P-256 test key")

    production["server_keys"]["p256"][0]["public_key"] = (
        "047cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978"
        "07775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1"
    )
    rejected(production, "Ed25519 test key")
    legacy = copy.deepcopy(production)
    legacy["server_keys"]["ed25519"][0]["public_key"] = MISENCODED_TEST_ED25519_KEY
    rejected(legacy, "Ed25519 test key")

    production["server_keys"]["ed25519"][0]["public_key"] = (
        "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c"
    )
    accepted = parse_profile(production)
    assert accepted.production

    tls_without_ca = copy.deepcopy(production)
    tls_without_ca["control"]["cloud_session_tls"] = True
    rejected(tls_without_ca, "PEM CA certificate")

    print("Arduino release-profile checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
