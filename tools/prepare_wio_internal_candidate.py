#!/usr/bin/env python3
"""Prepare/verify Wio internal preprocessing only; never build, install or admit.

Only public source hashes and the pinned PubSubClient 2.8 inputs are read. The
SDK root is a path fence, not an SDK version check or an input to an overlay.
"""
import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEPENDENCY_SHA256 = {
    "PubSubClient.cpp": "c5ab036263d514791b1955fe44aacca103b2eea4ca07cd539bff25dd88cc4ede",
    "PubSubClient.h": "376ddb9ecda5816dfeff344f8742253d487adc16272455ebe2dfa4c071cdd348",
}


def sha256(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def json_bytes(value: dict) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def source_paths() -> dict[str, Path]:
    files = [ROOT / "library.properties", Path(__file__).resolve()]
    files.extend(sorted(file for file in (ROOT / "src").rglob("*") if file.is_file()))
    if any(file.is_symlink() for file in files):
        raise ValueError("internal candidate inputs must be public regular files")
    return {file.relative_to(ROOT).as_posix(): file for file in files}


def dependency_inputs(pubsub_dir: Path) -> dict[str, bytes]:
    files = {}
    for name, digest in DEPENDENCY_SHA256.items():
        file = pubsub_dir / name
        if file.is_symlink():
            raise ValueError("PubSub inputs must be original regular files")
        content = file.read_bytes()
        if sha256(content) != digest:
            raise ValueError("unreviewed PubSubClient 2.8 input: " + name)
        files[name] = content
    return files


def expected_candidate(pubsub_dir: Path, output: Path, sdk_root: Path) -> tuple[dict, dict]:
    pubsub_dir, output, sdk_root = pubsub_dir.resolve(), output.resolve(), sdk_root.resolve()
    if not pubsub_dir.is_dir() or not sdk_root.is_dir():
        raise ValueError("original PubSub directory and SDK path fence must exist")
    if any(output.is_relative_to(root) for root in (sdk_root, ROOT / "src", pubsub_dir.parent)):
        raise ValueError("candidate output must be outside SDK, library src and PubSub trees")
    sources = {name: file.read_bytes() for name, file in source_paths().items()}
    dependencies = dependency_inputs(pubsub_dir)
    config = ["// Internal preprocessing only; does not enable tasks or grant admission.",
              "#ifndef BLINKER_INTERNAL_WIO_CANDIDATE_CONFIG_H",
              "#define BLINKER_INTERNAL_WIO_CANDIDATE_CONFIG_H",
              "#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)",
              '#error "Internal candidate requires the reviewed Wio Terminal target"', "#endif"]
    # External graph selection is required even in PubSubClient.cpp, which does
    # not include Blinker headers. This header never invents task authorization.
    for name, value in (("BLINKER_ENABLE_TASKS", 1), ("BLINKER_INTERNAL_WIO_TASKS", 1),
                        ("BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED", 1), ("BLINKER_TASK_CAPACITY", 4)):
        config.extend([f"#if !defined({name}) || {name} != {value}",
                       '#error "Wio candidate requires the explicit internal four-slot task graph"', "#endif"])
    config.extend(["#if defined(BLINKER_PRODUCT_BLE) || defined(BLINKER_PRODUCT_WIFI_BLE) || \\",
                   "    (defined(BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) && BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) || \\",
                   "    (defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET)",
                   '#error "Wio candidate admits the existing manual WiFi graph only"', "#endif"])
    for name, value in (("BLINKER_RESOURCE_PROFILE", "1"), ("BLINKER_DEVICE_FRAME_SIZE", "256"),
                        ("MQTT_MAX_PACKET_SIZE", "361U"), ("BLINKER_OFFICIAL_CLOUD_SESSION_TLS", "0"),
                        ("BLINKER_OFFICIAL_MQTT_TLS", "0")):
        config.extend([f"#if defined({name}) && {name} != {value}",
                       '#error "Conflicting Wio Small256/plain/MQTT361 candidate configuration"', "#endif",
                       f"#ifndef {name}", f"#define {name} {value}", "#endif"])
    config.extend(["#endif", ""])
    files = {"CandidateConfig.h": "\n".join(config).encode("utf-8")}
    receipt = {
        "schema": "internal-wio-candidate/1", "state": "internal-offline-only",
        "requiredCore": "1.8.5", "requiredTarget": "Seeeduino:samd:seeed_wio_terminal",
        "sdkPathFence": str(sdk_root), "sdkVerified": False, "buildLinked": False,
        "ordinaryZipDependency": False, "formalTaskAdmission": False,
        "hardwareStopValidated": False, "hardwareWritten": False, "oomRecoverable": False,
        "compileFlags": ["-include", str(output / "CandidateConfig.h")],
        "sourceSha256": {name: sha256(content) for name, content in sources.items()},
        "dependencyDirectory": str(pubsub_dir),
        "dependencySha256": {name: sha256(content) for name, content in dependencies.items()},
        "outputSha256": {name: sha256(content) for name, content in files.items()},
        "requiredBuildSteps": [
            "Verify the original Seeed SAMD 1.8.5 SDK, compiler and complete reviewed public source/profile projection separately; this tool verifies no SDK version.",
            "Supply the explicit Wio task/internal/reserved/capacity macros library-wide; the forced header does not enable tasks.",
            "Force-include CandidateConfig.h in pinned PubSubClient.cpp AND every actual .d consumer; a Sketch-only define is insufficient.",
            "Rebuild every actual owner/header consumer with the same Small1/frame256/MQTT361 configuration; reject missing, conflicting or wrong-graph consumers.",
            "Preserve reviewed profile/callbacks and storage region ownership; never resolve a private projection to public placeholders.",
        ],
        "requiredPostLinkEvidence": [
            "Bind actual compile commands, .d consumers, source/profile and archive member hashes to one complete graph.",
            "Retain the unique Wio allocator anchor and native hooks, original TaskStore/Codec/Engine and Ed25519 graph; symbols are not physical proof.",
            "Prove the real pinned PubSubClient constructor requests 361 and the adapter requests the same 361; no connection buffer growth.",
            "Same-size setBufferSize still calls native realloc; no-growth does not mean zero realloc or an allocator/fragmentation fix.",
        ],
        "unsupported": [
            "Ordinary ZIP profile, production release or automatic task admission.",
            "BLE, WiFi+BLE, TLS, LAN or arbitrary SDK/profile/callback changes outside the reviewed graph.",
            "SDK or hardware verification, complete heap/stack proof, offline UTC or RPC deadline guarantees.",
        ],
    }
    return files, receipt


def prepare(pubsub_dir: Path, output: Path, sdk_root: Path) -> dict:
    files, receipt = expected_candidate(pubsub_dir, output, sdk_root)
    output.mkdir(parents=True, exist_ok=False)
    for name, content in {**files, "candidate.json": json_bytes(receipt)}.items():
        with (output / name).open("xb") as stream:
            stream.write(content)
    return receipt


def verify(pubsub_dir: Path, output: Path, sdk_root: Path) -> dict:
    files, receipt = expected_candidate(pubsub_dir, output, sdk_root)
    actual = {file.relative_to(output).as_posix() for file in output.rglob("*") if file.is_file()}
    if actual != set(files) | {"candidate.json"}:
        raise ValueError("internal candidate has missing or unlisted output files")
    if any(file.is_symlink() for file in output.rglob("*")):
        raise ValueError("internal candidate outputs must be regular files")
    for name, content in files.items():
        if (output / name).read_bytes() != content:
            raise ValueError("internal candidate output mismatch: " + name)
    if json.loads((output / "candidate.json").read_text(encoding="utf-8")) != receipt:
        raise ValueError("internal candidate receipt/source mismatch")
    return receipt


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pubsub-dir", type=Path, required=True)
    parser.add_argument("--sdk-root", type=Path, required=True, help="existing SDK root used only as an output path fence")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    receipt = (verify if args.verify_only else prepare)(args.pubsub_dir, args.output, args.sdk_root)
    print(json.dumps({name: receipt[name] for name in (
        "schema", "state", "sdkVerified", "buildLinked", "formalTaskAdmission", "hardwareWritten")}))


if __name__ == "__main__":
    main()
