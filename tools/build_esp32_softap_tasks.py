#!/usr/bin/env python3
"""Explicit maintainer packaging of the fixed QA-3 graph, never admission.

Reuse the single-library file boundary, profile composer and deterministic ZIP.
No credentials, SDK edits, device writes or runtime signing attestation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import arduino_release_profile as release
import build_arduino_release as package
import esp32_provisioned_task_profile as qa3


ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / "profiles/esp32-softap-tasks-limited.json"
EXAMPLE = "Esp32SoftApTasksQuickStart"
SDK = "3.3.11"
FQBN = "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB"
BOARD_GUARD = ('// Limited Arduino delivery: only the reviewed XIAO ESP32S3 board.\n'
               '#if !defined(ARDUINO_XIAO_ESP32S3)\n'
               '#error "Limited SoftAP task delivery requires XIAO ESP32S3"\n'
               '#endif\n')
# DER SubjectPublicKeyInfo for id-ecPublicKey / prime256v1 / uncompressed SEC1.
P256_SPKI_PREFIX = bytes.fromhex("3059301306072a8648ce3d020106082a8648ce3d030107034200")


def load_inputs(path: Path = SPEC) -> tuple[release.ReleaseProfile, release.ReleaseProfile]:
    spec = json.loads(path.read_text(encoding="utf-8"))
    if (not isinstance(spec, dict) or set(spec) != {"schema", "profile_id", "transport_profile", "server_key"}
            or type(spec["schema"]) is not int or spec["schema"] != 1
            or spec["profile_id"] != qa3.PROFILE_ID or spec["transport_profile"] != "cloud-beta.json"):
        raise ValueError("unsupported limited SoftAP task specification")
    key = spec["server_key"]
    if (not isinstance(key, dict) or set(key) != {"algorithm", "key_id", "public_key_hex", "spki_sha256"}
            or type(key["algorithm"]) is not int or key["algorithm"] != 2
            or type(key["key_id"]) is not int or key["key_id"] != 1
            or not isinstance(key["public_key_hex"], str)):
        raise ValueError("limited tasks require one public P256 keyId 1 only")
    # The strict existing parser validates public key encoding and HTTPS/MQTTS.
    payload = json.loads((ROOT / "profiles" / spec["transport_profile"]).read_text(encoding="utf-8"))
    transport = release.parse_profile(payload)
    payload.pop("product_profile")
    payload.update(channel="development", task_profile=release.TASK_CLOUD_PILOT_ID,
                   server_keys={"p256": [{"id": key["key_id"], "public_key": key["public_key_hex"]}],
                                "ed25519": []})
    signing = release.parse_profile(payload)
    actual = hashlib.sha256(P256_SPKI_PREFIX + signing.p256_keys[0].public_key).hexdigest()
    if key["spki_sha256"] != actual:
        raise ValueError("limited task public SPKI digest mismatch")
    qa3.internal_profile(transport, signing)
    return transport, signing


def configured_files(library: Path, spec: Path = SPEC) -> dict:
    transport, signing = load_inputs(spec)
    files = qa3.configured_files(library, transport, signing)
    for name in ("OfficialConfig.h", "Esp32ServerKeys.h", "PortableServerKeys.h"):
        key = qa3.PREFIX + name
        files[key] = BOARD_GUARD.encode() + files[key]
    files[release.LIBRARY_IDENTITY_HEADER] = release.render_library_identity(files).encode()
    return files


def verify_library(library: Path, spec: Path = SPEC) -> None:
    """Compare the complete generated source/identity, not just a ready marker."""
    for name, content in configured_files(library, spec).items():
        path = library / name
        expected = content if isinstance(content, bytes) else content.read_bytes()
        if not path.is_file() or path.is_symlink() or path.read_bytes() != expected:
            raise ValueError(f"limited installed library configuration mismatch: {name}")


def verify_package(archive: Path, library: Path, manifest: Path) -> None:
    receipt = json.loads(manifest.read_text(encoding="utf-8"))
    expected = {"schema": 1, "profileId": qa3.PROFILE_ID, "sdk": SDK, "fqbn": FQBN,
                "example": EXAMPLE, "archive": archive.name,
                "archiveSha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
                "profileSha256": hashlib.sha256(SPEC.read_bytes()).hexdigest(),
                "libraryIdentitySha256": hashlib.sha256((library / release.LIBRARY_IDENTITY_HEADER).read_bytes()).hexdigest()}
    if any(receipt.get(name) != value for name, value in expected.items()):
        raise ValueError("limited package receipt does not match ZIP/profile/library")
    verify_library(library)


def build(stage: Path, output: Path) -> dict:
    # Fail closed before copying or overwriting any stage/output.
    spec = SPEC  # One fixed maintainer delivery; not a generic profile API.
    transport, signing = load_inputs(spec)
    for target in (stage, output):
        if target.is_symlink() or (target.exists() and (not target.is_dir() or any(target.iterdir()))):
            raise ValueError("limited stage/output must be absent or empty, never linked")
    stage, output = stage.resolve(), output.resolve()
    if stage == output or stage.is_relative_to(output) or output.is_relative_to(stage):
        raise ValueError("limited stage and output must be separate directories")
    repository = package.REPOSITORY.resolve()
    # Generated files may only live in ignored build output or outside the repo.
    for target in (stage, output):
        if target == repository or (target.is_relative_to(repository)
                                     and not target.is_relative_to(repository / "build")):
            raise ValueError("limited outputs must not overwrite repository sources")
    properties, files = package.validate()
    package.stage(stage, files)
    for name, content in configured_files(stage, spec).items():
        if isinstance(content, bytes):
            (stage / name).write_bytes(content)
    verify_library(stage, spec)
    archive = package.archive(stage, output, properties)
    receipt = {
        "schema": 1, "profileId": qa3.PROFILE_ID, "limitedDelivery": True,
        "deployable": False, "runtimeSigningMatched": False, "formalAdmission": False,
        "archive": archive.name, "archiveSha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
        "profileSha256": hashlib.sha256(spec.read_bytes()).hexdigest(),
        "libraryIdentitySha256": hashlib.sha256((stage / release.LIBRARY_IDENTITY_HEADER).read_bytes()).hexdigest(),
        "sdk": SDK, "fqbn": FQBN, "example": EXAMPLE,
        "bearer": "softap", "cloudTls": True, "mqttTls": True, "taskCapacity": 4,
        "taskTargets": ["power:bool"], "rootMode": "DeviceKey+Admin",
        "signatureAlgorithm": 2, "keyId": 1,
        "publicKeySpkiSha256": hashlib.sha256(P256_SPKI_PREFIX + signing.p256_keys[0].public_key).hexdigest(),
        "controlHost": transport.control_host,
        "installationCompileVerified": False, "hardwareVerified": False,
    }
    (output / "limited-profile.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return receipt


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.stage_dir, args.output_dir), indent=2))


if __name__ == "__main__":
    main()
