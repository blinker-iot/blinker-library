#!/usr/bin/env python3
"""Link optional Timer/timeStatus APIs from an already isolated ordinary ZIP.

Uses the same installed group and fixed SDK as the ordinary product gate. No
install, SDK change, public example, product begin, network or hardware step.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys

import verify_installed_release as gate

REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY / "tools"))
from build_arduino_release import EXPECTED_IMPLEMENTATION_UNITS

PROBE = REPOSITORY / "tests/arduino/SketchTimerProbe/SketchTimerProbe.ino"
API_FILES = (
    "src/BlinkerTimer.h",
    "src/BlinkerV2/arduino/ArduinoSketchTimer.h",
    "src/BlinkerV2/core/SketchTimer.h",
    "src/BlinkerV2/api/BlinkerFacade.h",
)


@dataclass(frozen=True)
class Target:
    fqbn: str
    core: str
    libraries: frozenset[str]


TARGETS = {
    "esp32-sketch-timer": Target(
        "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB",
        "packages/esp32/hardware/esp32/3.3.11", frozenset(("BlinkerDeviceV2",))),
    # All 17 units are compiled. The existing R4 storage unit includes the
    # core's Preferences/WiFiS3 headers, even when its runtime is link-stripped.
    "uno-r4-sketch-timer": Target(
        "arduino:renesas_uno:unor4wifi", "packages/arduino/hardware/renesas_uno/1.5.3",
        frozenset(("BlinkerDeviceV2", "Preferences", "WiFiS3"))),
    "wio-terminal-sketch-timer": Target(
        "Seeeduino:samd:seeed_wio_terminal", "packages/Seeeduino/hardware/samd/1.8.5",
        frozenset(("BlinkerDeviceV2",))),
}
FORBIDDEN_SYMBOLS = (
    "blinker::TimeSyncService::", "blinker::TimeSyncCoordinator::",
    "blinker::TaskEngine::", "blinker::TaskManagementSession::",
    "blinker::TaskPropertyTarget::", "blinker::MqttFrameTransport::",
    "blinker::BleFrameTransport::", "blinker::BleRecordTransport::",
    "blinker::WifiConnectionLifecycle::", "blinker::Esp32", "blinker::UnoR4",
    "blinker::WioTerminal", *gate.UNSELECTED_EXTENSION_SYMBOLS,
)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_installation(archive: Path, work: Path) -> Path:
    marker = work / ".blinker-release-sha256"
    if not marker.is_file() or marker.read_text(encoding="ascii").strip() != digest(archive):
        raise SystemExit("group was not installed from this exact ZIP")
    installed = work / "sketchbook/libraries/BlinkerDeviceV2"
    gate.verify_installed_bytes(archive, installed)
    for name in API_FILES:
        if not (installed / name).is_file() or (installed / name).read_bytes() != (REPOSITORY / name).read_bytes():
            raise SystemExit(f"installed optional API does not match current source: {name}")
    return installed.resolve()


def verify_build(build: Path, installed: Path, target: Target, data: Path) -> dict:
    gate.verify_provenance(build, installed)
    commands = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
    source_root = (installed / "src").resolve()
    units = [Path(item["file"]).resolve().name for item in commands
             if source_root in Path(item["file"]).resolve().parents]
    if len(units) != 17 or set(units) != EXPECTED_IMPLEMENTATION_UNITS:
        raise SystemExit("optional API gate must compile all 17 installed implementation units once")
    options = json.loads((build / "build.options.json").read_text(encoding="utf-8"))
    core = (data / target.core).resolve()
    actual_cores = {Path(path).resolve() for path in options.get("hardwareFolders", "").split(",") if path}
    if options.get("fqbn") != target.fqbn or actual_cores != {core}:
        raise SystemExit("optional API gate used an unexpected FQBN or SDK")
    libraries = gate.compiled_libraries(build)
    if libraries != target.libraries:
        raise SystemExit(f"optional API compiled libraries {sorted(libraries)}, expected {sorted(target.libraries)}")
    if len(tuple(build.glob("*.map"))) != 1 or len(tuple(build.glob("*.elf"))) != 1:
        raise SystemExit("optional API gate requires one actual ELF and link map")
    symbols = gate.defined_symbols(build)
    for symbol in ("sample()", "delayed(void*)", "blinker::BlinkerClass::timeStatus("):
        if symbol not in symbols:
            raise SystemExit(f"optional API was not retained in the linked firmware: {symbol}")
    if any(symbol in symbols for symbol in FORBIDDEN_SYMBOLS):
        raise SystemExit("independent timer probe retained a product, UTC or persistent-task runtime")
    return {"fqbn": target.fqbn, "core": str(core), "implementationUnits": len(units),
            "compiledLibraries": sorted(libraries), "symbols": symbols}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--arduino-cli")
    parser.add_argument("--zip-path", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, required=True)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--downloads-dir", type=Path)
    parser.add_argument("--target", choices=tuple(TARGETS), required=True)
    args = parser.parse_args()
    archive, work, data = args.zip_path.resolve(), args.work_dir.resolve(), args.data_dir.resolve()
    installed = verify_installation(archive, work)
    target = TARGETS[args.target]
    if not (data / target.core / "platform.txt").is_file():
        raise SystemExit("required existing SDK is missing; this gate never installs or upgrades it")
    build = work / "build" / args.target
    sketch = work / "optional-api" / args.target / "SketchTimerProbe"
    if build.exists() or sketch.exists():
        raise SystemExit("choose a fresh optional API target evidence directory")
    environment = os.environ.copy()
    environment["ARDUINO_DIRECTORIES_USER"] = str(work / "sketchbook")
    environment["ARDUINO_DIRECTORIES_DATA"] = str(data)
    if args.downloads_dir:
        environment["ARDUINO_DIRECTORIES_DOWNLOADS"] = str(args.downloads_dir.resolve())
    cli = gate.executable(args.arduino_cli)
    library = gate.installed_libraries(cli, environment).get("BlinkerDeviceV2", {})
    if library.get("version") != "0.1.0" or Path(library.get("install_dir", "")).resolve() != installed:
        raise SystemExit("Arduino selected a different installed Blinker library")
    inputs = {name: digest(REPOSITORY / name) for name in API_FILES}
    inputs[str(PROBE.relative_to(REPOSITORY))] = digest(PROBE)
    for script in (Path(__file__).resolve(), Path(gate.__file__).resolve()):
        inputs[str(script.relative_to(REPOSITORY))] = digest(script)
    sketch.mkdir(parents=True)
    shutil.copyfile(PROBE, sketch / PROBE.name)
    command = [cli, "compile", "--fqbn", target.fqbn, "--clean", "--jobs", "1",
               "--warnings", "none", "--build-path", str(build), str(sketch)]
    output = gate.run(command, environment, log_path=build / "compile.log")
    if "Sketch uses" not in output:
        raise SystemExit("optional API compile size summary is missing")
    report = verify_build(build, installed, target, data)
    (build / "defined-symbols.log").write_text(report.pop("symbols"), encoding="utf-8")
    verify_installation(archive, work)
    if any(digest(REPOSITORY / name) != value for name, value in inputs.items()) or digest(sketch / PROBE.name) != inputs[str(PROBE.relative_to(REPOSITORY))]:
        raise SystemExit("optional API or gate inputs changed during compilation")
    outputs = {path.name: digest(path) for path in build.iterdir()
               if path.is_file() and (path.suffix in (".elf", ".map", ".log", ".bin", ".hex") or
                                     path.name in ("compile_commands.json", "build.options.json", "libraries.cache"))}
    report.update({"target": args.target, "archiveSha256": digest(archive), "inputs": inputs,
                   "command": command, "outputs": outputs, "passed": True,
                   "scope": "installed optional API complete link only; no product start or hardware"})
    (build / "timer-api.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"installed optional Timer/timeStatus link passed: {args.target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
