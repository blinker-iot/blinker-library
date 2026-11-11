#!/usr/bin/env python3
"""Compile-only ESP32 task resource gate; never uploads or contacts a device."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SKETCH = ROOT / "tests/arduino/Esp32TaskResourceProbe"
PRODUCT_SKETCH = ROOT / "tests/arduino/Esp32WifiTaskProductProbe"
FQBN = "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB"
CAPACITIES = (0, 4, 8, 10)
COMMON_METHODS = (
    "blinker::TaskEngine::begin(", "blinker::TaskEngine::initializeVerified(",
    "blinker::TaskEngine::mutateVerified(", "blinker::TaskEngine::rebindVerified(", "blinker::TaskEngine::poll(",
    "blinker::TaskEngine::recordAt(", "blinker::TaskPropertyTarget::dispatch(",
    "blinker::TaskSnapshotStore::replace(", "blinker::TaskEngine::clearRetired(",
    "blinker::TaskSnapshotStore::clear(", "blinker::TaskProductLifecycle<",
)
# Both graphs retain the persistent retirement seam; only the product carries it online.
REQUIRED_METHODS = COMMON_METHODS + (
    "blinker::TaskEngine::retireVerified(", "blinker::TaskEngine::retirementRecord(",
)
PRODUCT_METHODS = REQUIRED_METHODS + (
    "blinker::TaskManagementSession::", "blinker::TaskCommandAdapter::",
    "blinker::TaskCloudTransport<", "blinker::verifyTaskManagementGrant(",
    "blinker::integration::official_detail::Esp32WifiTaskExtension<",
)
TASK_SYMBOL = re.compile(r"blinker::(?:TaskEngine|TaskSnapshot|TaskCalendar|TaskProperty|TaskProductLifecycle|TaskTimeEnvironment|TaskCountdown|TaskAuthority|TaskManagement|TaskCommand|TaskCloudTransport|nextTaskUtc|validateTaskTiming|taskCalendarContinues|taskAuthority|taskManagement|taskMutationPrepare|taskReadPrepare|encodeTask|decodeTask|verifyTask|task_(?:authority|contract|session|adapter)_detail)")


def run(command: list[str]) -> str:
    process = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, encoding="utf-8", errors="replace")
    if process.returncode:
        raise RuntimeError(f"command failed ({process.returncode}): {process.stdout[-6000:]}")
    return process.stdout


def resource_summary(log: str) -> dict[str, int]:
    flash = re.search(r"Sketch uses (\d+) bytes", log)
    ram = re.search(r"Global variables use (\d+) bytes", log)
    if not flash or not ram:
        raise ValueError("missing Arduino Flash/static RAM summary (use English CLI output)")
    return {"flash_bytes": int(flash[1]), "static_ram_bytes": int(ram[1])}


def check_symbols(symbols: str, capacity: int, product: bool = False) -> int:
    if capacity == 0:
        if TASK_SYMBOL.search(symbols):
            raise ValueError("disabled build retains task code/state")
        if not product:
            return 0
    else:
        required = PRODUCT_METHODS if product else REQUIRED_METHODS
        missing = [name for name in required if name not in symbols]
        if missing:
            raise ValueError("task graph was not linked: " + ", ".join(missing))
    name = (r"blinker::integration::official_detail::esp32WifiProduct\(\)::instance"
            if product else "tasks")
    storage_kinds = "bBdDVv" if product else "bBdD"
    matches = re.findall(r"^\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+[" + storage_kinds + r"]\s+" + name + r"$",
                         symbols, re.MULTILINE)
    if len(matches) != 1:
        raise ValueError("missing or ambiguous real product/module storage symbol")
    return int(matches[0], 16)


def digest(path: Path) -> str:
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def source_digest(sketch: Path = SKETCH) -> str:
    hasher = hashlib.sha256()
    paths = sorted((ROOT / "src").rglob("*")) + sorted(sketch.rglob("*"))
    paths += [ROOT / "library.properties", Path(__file__).resolve()]
    for path in paths:
        if path.is_file():
            hasher.update(path.relative_to(ROOT).as_posix().encode("utf-8") + b"\0")
            hasher.update(bytes.fromhex(digest(path)))
    return hasher.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--arduino-cli", default=shutil.which("arduino-cli"))
    parser.add_argument("--config-file", type=Path, required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--product", action="store_true", help="measure the actual optional manual WiFi product")
    args = parser.parse_args()
    source_sketch = PRODUCT_SKETCH if args.product else SKETCH
    if not args.arduino_cli or not args.config_file.is_file():
        parser.error("Arduino CLI and an existing config file are required")
    destination = args.build_root.resolve()
    if not destination.is_relative_to(ROOT / "build") or destination == ROOT / "build":
        parser.error("use a dedicated subdirectory of this repository's build directory")
    destination.mkdir(parents=True, exist_ok=True)
    if (destination / "resource-report.json").exists():
        parser.error("a completed report already exists; use a new evidence directory")
    cli = [args.arduino_cli, "--no-color", "--config-file", str(args.config_file.resolve())]
    common = ["compile", "--fqbn", FQBN, "--library", str(ROOT), "--jobs", "4", "--warnings", "all"]
    properties_text = run(cli + common + ["--show-properties", str(source_sketch)])
    properties = dict(line.split("=", 1) for line in properties_text.splitlines() if "=" in line)
    platform_path = Path(properties["runtime.platform.path"])
    if platform_path.name != "3.3.11":
        raise ValueError("this evidence baseline requires the installed stock ESP32 3.3.11 core")
    nm = Path(properties["compiler.path"]) / (properties["compiler.prefix"] + "nm" + (".exe" if os.name == "nt" else ""))
    if not nm.is_file():
        raise ValueError("configured compiler's nm is missing")
    before = source_digest(source_sketch)
    rows = []
    work = destination / "work"
    for capacity in CAPACITIES:
        folder = destination / f"capacity-{capacity}"
        folder.mkdir(exist_ok=True)
        # Capacity changes only this sketch's template, never library ABI.
        # Reuse one build cache without changing flags for all SDK sources.
        sketch = destination / "sketch" / source_sketch.name
        sketch.mkdir(parents=True, exist_ok=True)
        macro = "BLINKER_TASK_CAPACITY" if args.product else "BLINKER_TASK_PROBE_CAPACITY"
        (sketch / (source_sketch.name + ".ino")).write_text(
            f"#define {macro} {capacity}\n" +
            (source_sketch / (source_sketch.name + ".ino")).read_text(encoding="utf-8"), encoding="utf-8")
        command = cli + common + ["--build-path", str(work), str(sketch)]
        print(f"Compiling task capacity={capacity} (no upload)", flush=True)
        # Preserve failing build evidence too, without capturing credentials.
        process = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 encoding="utf-8", errors="replace")
        (folder / "compile.log").write_text(process.stdout, encoding="utf-8")
        if process.returncode:
            raise RuntimeError(f"compile failed; see {folder / 'compile.log'}\n{process.stdout[-4000:]}")
        for name in (source_sketch.name + ".ino.elf", source_sketch.name + ".ino.bin", "sdkconfig", "build.options.json"):
            shutil.copyfile(work / name, folder / name)
        elf = folder / (source_sketch.name + ".ino.elf")
        binary = folder / (source_sketch.name + ".ino.bin")
        symbols = run([str(nm), "-S", "--defined-only", "--demangle", str(elf)])
        (folder / "symbols.txt").write_text(symbols, encoding="utf-8")
        row = {"capacity": capacity, **resource_summary(process.stdout),
               ("product_bytes" if args.product else "module_bytes"): check_symbols(symbols, capacity, args.product),
               "elf_sha256": digest(elf), "binary_sha256": digest(binary)}
        if rows:
            row["flash_delta"] = row["flash_bytes"] - rows[0]["flash_bytes"]
            row["static_ram_delta"] = row["static_ram_bytes"] - rows[0]["static_ram_bytes"]
        rows.append(row)
        print(json.dumps(row), flush=True)
    if before != source_digest(source_sketch):
        raise ValueError("sources changed during compilation; repeat the matrix")
    report = {"scope": "compile/link only; no boot, heap/stack peaks, NVS latency or power-loss evidence",
              "baseline": "same WiFi/time graph; empty and full tables share fixed-capacity allocation",
              "mode": "manual-wifi-product" if args.product else "task-module",
              "fqbn": FQBN, "core_version": platform_path.name,
              "cli_version": run(cli + ["version"]).strip(),
              "compiler_version": run([str(nm.with_name(properties["compiler.prefix"] + "g++" + nm.suffix)), "--version"]).splitlines()[0],
              "source_sha256": before, "variants": rows}
    (destination / "resource-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("ESP32 task firmware resource gate passed (hardware gates remain pending)", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
