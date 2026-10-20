#!/usr/bin/env python3
"""Prepare/verify internal R4 source wiring only; never build, install or admit.

Ordinary Arduino ZIPs keep the native SDK and task capacity zero. This tool
reuses the pinned overlay and existing heap adapter for a maintainer's separate
offline build. A matching receipt is NOT a link, runtime or release certificate.
"""
import argparse
import hashlib
import json
from pathlib import Path

import prepare_r4_modem_response_overlay as overlay
import r4_allocation_workset as allocation


ROOT = Path(__file__).resolve().parents[1]
HEAP_ADAPTER = "tests/internal/arduino/UnoR4WiFiTaskGate/TaskHeapBoundary.h"
LIMITS = {"MODEM_COMMAND_BYTES": 32, "MODEM_RESPONSE_BYTES": 1024,
          "MODEM_RPC_MS": 1000, "HEAP_BOUNDARY": 1}
# Freeze only the reviewed allocation/lifetime inputs, not a replacement SDK.
DEPENDENCY_SHA256 = {
    "sdk/libraries/WiFiS3/src/WiFiClient.cpp": "f0503d7ac267239b2d4a3931803f225f734a23408f7dd44cc5498dc9e84dd720",
    "sdk/libraries/WiFiS3/src/WiFiClient.h": "c5acb6074b928c9c5c083835f28a6396c3151a6c4520576934bfb974d070228f",
    "sdk/libraries/WiFiS3/src/WiFi.cpp": "14bac07e2cf61c81285e82090c86525e45ee00db24ae155fb11c7ab955268468",
    "sdk/libraries/WiFiS3/src/WiFi.h": "14ab2f0b9eac80fc81c49469865c9a5ae3747ec8d90eead77fcd21c6bff2cfc7",
    "sdk/cores/arduino/FifoBuffer.h": "1884ff375af17a433508ba45bd791081d1ee1f1f25436c7309b63df87b81e161",
    "sdk/cores/arduino/main.cpp": "6eb90a7cc9998d0ee8c6f95c9f361eb20151a9bce6b82a795eec1e4534e39c93",
    "sdk/cores/arduino/time.cpp": "66c37505448fab02188dc2636591a4d87904fb8c537cdfc892cf7d492566a633",
    "sdk/cores/arduino/FspTimer.cpp": "8dbaeb692bdaa15cef1b514fde683991e65613e9b7fb7b71c78a38866d81b920",
    "sdk/cores/arduino/FspTimer.h": "2f99fd218046c168d2ee41c407c0889bb2a48789257b5e76c05876755686e76f",
    "sdk/cores/arduino/api/String.cpp": "aa056fa2aa79f267739ae5403d99ce13bd6cff9c05bd7e7ff45ce2ca11c63da4",
    "sdk/cores/arduino/api/IPAddress.cpp": "8f72c8b5202a5e0df2512f22b61760feba1a40d457419aa0396f4fe45dfd43dd",
    "pubsub/PubSubClient.cpp": "c5ab036263d514791b1955fe44aacca103b2eea4ca07cd539bff25dd88cc4ede",
    "pubsub/PubSubClient.h": "376ddb9ecda5816dfeff344f8742253d487adc16272455ebe2dfa4c071cdd348",
}


def sha256(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def json_bytes(value: dict) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def source_paths() -> dict[str, Path]:
    # Public source only: never inspect private profiles, accounts or backups.
    files = [ROOT / "library.properties", ROOT / HEAP_ADAPTER,
             Path(__file__).resolve(), Path(overlay.__file__).resolve(),
             Path(overlay.uart_overlay.__file__).resolve(), Path(allocation.__file__).resolve()]
    files.extend(sorted((ROOT / "src").rglob("*")))
    return {file.relative_to(ROOT).as_posix(): file for file in files if file.is_file()}


def dependency_inputs(modem_dir: Path, pubsub_dir: Path) -> dict[str, bytes]:
    files = {}
    for name, digest in DEPENDENCY_SHA256.items():
        path = (modem_dir.parents[2] / name[4:] if name.startswith("sdk/")
                else pubsub_dir / name[7:])
        content = path.read_bytes()
        if sha256(content) != digest:
            raise ValueError("unreviewed R4 allocation dependency: " + name)
        files[name] = content
    return files


def expected_candidate(modem_dir: Path, output: Path, pubsub_dir: Path) -> tuple[dict, dict]:
    modem_dir, output, pubsub_dir = modem_dir.resolve(), output.resolve(), pubsub_dir.resolve()
    if modem_dir.parts[-3:] != ("libraries", "WiFiS3", "src"):
        raise ValueError("source must be the original SDK libraries/WiFiS3/src directory")
    if (output.is_relative_to(modem_dir.parents[2]) or output.is_relative_to(ROOT / "src")
            or output.is_relative_to(pubsub_dir.parent)):
        raise ValueError("candidate output must be outside SDK and library source trees")
    sources = source_paths()
    # Bind sources that feed copies, not a receipt's editable self-declared SHA.
    original = {name: file.read_bytes() for name, file in sources.items()}
    dependencies = dependency_inputs(modem_dir, pubsub_dir)
    workset = allocation.restricted_workset(361)
    generated_sdk, sdk_receipt = overlay.expected_overlay(modem_dir, output / "sdk-overlay")
    files = {"sdk-overlay/" + name: value for name, value in generated_sdk.items()}
    files["sdk-overlay/overlay.json"] = json_bytes(sdk_receipt)
    config = ["// Internal offline wiring only; does not enable tasks or grant admission.",
              "#ifndef BLINKER_INTERNAL_R4_CANDIDATE_CONFIG_H",
              "#define BLINKER_INTERNAL_R4_CANDIDATE_CONFIG_H",
              "#if !defined(ARDUINO_UNOWIFIR4) || !defined(ARDUINO_ARCH_RENESAS_UNO) || !defined(NO_USB)",
              '#error "Internal candidate requires the reviewed UNO R4 NO_USB target"', "#endif"]
    for name, value in LIMITS.items():
        macro = "BLINKER_INTERNAL_R4_" + name
        config.extend([f"#if defined({macro}) && {macro} != {value}",
                       '#error "Conflicting internal R4 candidate configuration"', "#endif",
                       f"#ifndef {macro}", f"#define {macro} {value}U", "#endif"])
    # Pinned nano realloc can over-read an old allocation when growing. Avoid
    # this graph's 256->361 growth, rather than replacing libc or the allocator.
    config.extend(["#if defined(MQTT_MAX_PACKET_SIZE) && MQTT_MAX_PACKET_SIZE != 361",
                   '#error "R4 candidate requires consistent initial MQTT packet size"',
                   "#endif", "#ifndef MQTT_MAX_PACKET_SIZE", "#define MQTT_MAX_PACKET_SIZE 361U", "#endif"])
    # Preserve the Small profile's exact token across retained consumer TUs.
    config.extend(["#if defined(BLINKER_DEVICE_FRAME_SIZE) && BLINKER_DEVICE_FRAME_SIZE != 256",
                   '#error "R4 candidate workset requires the reviewed frame size"',
                   "#endif", "#ifndef BLINKER_DEVICE_FRAME_SIZE", "#define BLINKER_DEVICE_FRAME_SIZE 256", "#endif"])
    config.extend(["#endif", ""])
    files["CandidateConfig.h"] = "\n".join(config).encode("utf-8")
    files["TaskHeapBoundary.h"] = original[HEAP_ADAPTER]
    files["HeapBoundary.cpp"] = b'''#include "CandidateConfig.h"
#include <BlinkerV2/core/ResourceProfile.h>
#include <BlinkerV2/control/DeviceKeySessionContract.h>
static_assert(BLINKER_DEVICE_FRAME_SIZE + blinker::kDeviceKeyMaximumTopicSize + 9U == MQTT_MAX_PACKET_SIZE,
              "R4 initial MQTT buffer must match the reviewed packet budget");
#include "TaskHeapBoundary.h"
'''
    receipt = {
        "schema": "internal-r4-candidate/2", "state": "internal-offline-only",
        "core": "1.5.3", "target": "arduino:renesas_uno:unor4wifi",
        "ordinaryZipDependency": False, "formalTaskAdmission": False,
        "hardwareStopValidated": False, "oomRecoverable": False,
        "linked": False, "hardwareWritten": False,
        "compileFlags": ["-include", str(output / "CandidateConfig.h"),
                         "-I" + str(output / "sdk-overlay")],
        "linkFlags": ["-Wl,--wrap=_sbrk",
                      "-Wl,--defsym=__blinker_r4_heap_limit=__StackTop-6144"],
        "sourceSha256": {name: sha256(value) for name, value in original.items()},
        "dependencyDirectory": str(pubsub_dir),
        "dependencySha256": {name: sha256(value) for name, value in dependencies.items()},
        "allocationWorkset": workset,
        "sdkOverlay": sdk_receipt,
        "outputSha256": {name: sha256(value) for name, value in files.items()},
        "requiredBuildSteps": [
            "Force-include CandidateConfig.h consistently in every rebuilt owner/SDK consumer TU.",
            "Provide a complete reviewed source projection/src and prioritize its include path; this tool does not select or supply that projection.",
            "Preserve reviewed private OfficialConfig.h overrides, profile and callbacks; include missing UnoR4HeapBoundary.h helpers in the projection without replacing its private configuration.",
            "Inspect every actual .d consumer and recompile all owner/header consumers, not only Sketch; reject a dependency graph that resolves the reviewed private profile to a public placeholder.",
            "Compile both pinned Modem.cpp and Serial.cpp and the single HeapBoundary.cpp TU.",
            "Recompile pinned PubSubClient.cpp AND all consumers with MQTT_MAX_PACKET_SIZE=361; a Sketch-only define is insufficient.",
            "Replace the unique Serial.cpp.o member in a COPY of core.a; never append a second implementation.",
            "Replace affected library archive members; retain input/member hashes and original link order.",
        ],
        "requiredPostLinkEvidence": [
            "Actual compiler, flags, SDK/FSP/archive, source projection and profile hashes.",
            "All owner consumers rebuilt; no retained native/duplicate Modem or Serial implementation.",
            "One owner/boundary; actual _sbrk_r -> wrapper -> original _sbrk, not symbol presence alone.",
            "Absolute heap cap equals actual StackTop minus 6144; recompute span from new HeapBase.",
            "Actual UART stop/fail-stop calls and resource frames; no claim of physical or IRQ proof.",
            "Actual PubSub constructor requests 361 and adapter requests the same size; no native non-null realloc growth in the admitted graph.",
        ],
        "unsupported": [
            "Ordinary ZIP installation, production release or automatic R4 task admission.",
            "Any Sketch/callback, BLE/TLS/LAN/scan, SDK debug or DMA/DTC outside the reviewed graph.",
            "IRQ/RTOS allocator, disabled timebase, recoverable string/new OOM or hard hardware WCET.",
            "User buffer resizing, native WiFi calls, String concatenation or dynamic stdio outside the reviewed allocation lifetimes.",
        ],
    }
    return files, receipt


def prepare(modem_dir: Path, output: Path, pubsub_dir: Path) -> dict:
    files, receipt = expected_candidate(modem_dir, output, pubsub_dir)
    output.mkdir(parents=True, exist_ok=False)
    for name, content in files.items():
        file = output / name
        file.parent.mkdir(parents=True, exist_ok=True)
        with file.open("xb") as stream:
            stream.write(content)
    with (output / "candidate.json").open("xb") as stream:
        stream.write(json_bytes(receipt))
    return receipt


def verify(modem_dir: Path, output: Path, pubsub_dir: Path) -> dict:
    files, expected = expected_candidate(modem_dir, output, pubsub_dir)
    actual = {file.relative_to(output).as_posix() for file in output.rglob("*") if file.is_file()}
    if actual != set(files) | {"candidate.json"}:
        raise ValueError("internal candidate has missing or unlisted output files")
    for name, content in files.items():
        if (output / name).read_bytes() != content:
            raise ValueError("internal candidate output mismatch: " + name)
    receipt = json.loads((output / "candidate.json").read_text(encoding="utf-8"))
    if receipt != expected:
        raise ValueError("internal candidate receipt/source mismatch")
    return expected


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--modem-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--pubsub-dir", type=Path, required=True,
                        help="original pinned PubSubClient 2.8 src directory; never modified")
    parser.add_argument("--verify-only", action="store_true")
    arguments = parser.parse_args()
    receipt = (verify if arguments.verify_only else prepare)(arguments.modem_dir, arguments.output, arguments.pubsub_dir)
    print(json.dumps({name: receipt[name] for name in (
        "schema", "state", "linked", "formalTaskAdmission", "hardwareWritten")}))


if __name__ == "__main__":
    main()
