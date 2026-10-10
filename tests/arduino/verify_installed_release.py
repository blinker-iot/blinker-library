#!/usr/bin/env python3
"""Install the public ZIP in an empty sketchbook and compile every product."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import zipfile


REPOSITORY = Path(__file__).resolve().parents[2]


@dataclass(frozen=True)
class Target:
    name: str
    fqbn: str
    example: str
    libraries: frozenset[str]
    time_enabled: bool = False
    tasks_enabled: bool = False


TARGETS = (
    Target(
        "esp32-wifi",
        "esp32:esp32:esp32",
        "WiFiQuickStart",
        frozenset(("BlinkerDeviceV2", "Network", "PubSubClient", "WiFi")),
    ),
    Target(
        "esp32-ble",
        "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB",
        "BleQuickStart",
        frozenset(("BlinkerDeviceV2",)),
    ),
    Target(
        "esp32-wifi-ble",
        "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB",
        "WiFiBleQuickStart",
        frozenset(
            (
                "BlinkerDeviceV2",
                "Network",
                "PubSubClient",
                "WiFi",
            )
        ),
    ),
    Target(
        "esp32-cloud-smoke",
        "esp32:esp32:esp32",
        "CloudSmokeTest",
        frozenset(("BlinkerDeviceV2", "Network", "PubSubClient", "WiFi")),
        time_enabled=True,
    ),
    Target(
        "uno-r4-wifi",
        "arduino:renesas_uno:unor4wifi",
        "WiFiQuickStart",
        frozenset(
            ("BlinkerDeviceV2", "Preferences", "PubSubClient", "WiFiS3")
        ),
    ),
    Target(
        "uno-r4-ble",
        "arduino:renesas_uno:unor4wifi",
        "BleQuickStart",
        frozenset(
            ("ArduinoBLE", "BlinkerDeviceV2", "Crypto", "Preferences", "WiFiS3")
        ),
    ),
    Target(
        "wio-terminal-wifi",
        "Seeeduino:samd:seeed_wio_terminal",
        "WiFiQuickStart",
        frozenset(
            (
                "BlinkerDeviceV2",
                "PubSubClient",
                "Seeed_Arduino_FreeRTOS",
                "Seeed_Arduino_FS",
                "Seeed_Arduino_mbedtls",
                "Seeed_Arduino_rpcUnified",
                "Seeed_Arduino_rpcWiFi",
            )
        ),
    ),
)

# Explicit maintainer opt-in only: not part of the ordinary release matrix.
TASK_PILOT_TARGET = Target(
    "esp32-wifi-tasks",
    "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB",
    "CompileTaskWiFi",
    frozenset(("BlinkerDeviceV2", "Network", "PubSubClient", "WiFi")),
    time_enabled=True, tasks_enabled=True,
)


def select_targets(names: list[str] | None) -> tuple[Target, ...]:
    if not names:
        return TARGETS
    available = {t.name: t for t in (*TARGETS, TASK_PILOT_TARGET)}
    if len(names) != len(set(names)) or any(n not in available for n in names):
        raise ValueError("unknown or duplicate installed target")
    return tuple(available[n] for n in names)

# compiled library directory -> (Library Manager install spec, reported name,
# pinned version). A single-target gate installs only that target's closure;
# the full matrix naturally takes the union of every target.
DEPENDENCIES = {
    "ArduinoBLE": ("ArduinoBLE@2.0.2", "ArduinoBLE", "2.0.2"),
    "Crypto": ("Crypto@0.4.0", "Crypto", "0.4.0"),
    "PubSubClient": ("PubSubClient@2.8", "PubSubClient", "2.8"),
    "Seeed_Arduino_FS": (
        "Seeed Arduino FS@2.1.3",
        "Seeed Arduino FS",
        "2.1.3",
    ),
    "Seeed_Arduino_mbedtls": (
        "Seeed_Arduino_mbedtls@3.0.2",
        "Seeed_Arduino_mbedtls",
        "3.0.2",
    ),
    "Seeed_Arduino_rpcUnified": (
        "Seeed Arduino rpcUnified@2.1.4",
        "Seeed Arduino rpcUnified",
        "2.1.4",
    ),
    "Seeed_Arduino_rpcWiFi": (
        "Seeed Arduino rpcWiFi@1.1.0",
        "Seeed Arduino rpcWiFi",
        "1.1.0",
    ),
}

BLINKER_ARCHIVE_MEMBER = re.compile(
    r"BlinkerDeviceV2\.a\(([^)]+\.o)\)"
)

PLATFORMS = ("esp32", "uno-r4", "wio-terminal")

REQUIRED_SYMBOLS = {
    "wifi": ("blinker::MqttFrameTransport::",),
    "ble": ("blinker::BleFrameTransport::",),
    "wifi-ble": (
        "blinker::MqttFrameTransport::",
        "blinker::BleFrameTransport::",
        "blinker::BleRecordTransport::",
    ),
}

FORBIDDEN_SYMBOLS = {
    "wifi": (
        "blinker::BleFrameTransport::",
        "blinker::BleLocalProvisioningEndpoint::",
        "blinker::BleModeCoordinator::",
        "blinker::BleNoiseModePreparation::",
        "blinker::BleNoiseProvisioningChannel::",
        "blinker::BleRecordTransport::",
        "blinker::BleSetupCompletion::",
        "blinker::ControllerHmacSha256Authorizer::",
        "blinker::security::NoiseNnSession::",
    ),
    "ble": (
        "blinker::ConfiguredDeviceKeySource::",
        "blinker::HttpDeviceKeySessionProvider::",
        "blinker::ManagedMqttTransport::",
        "blinker::MqttFrameTransport::",
        "blinker::Esp32WifiProvAdapter::",
        "blinker::WifiConnectionLifecycle::",
    ),
    "wifi-ble": (
        "blinker::BleLocalProvisioningEndpoint::",
        "blinker::BleModeCoordinator::",
        "blinker::BleNoiseModePreparation::",
        "blinker::BleNoiseProvisioningChannel::",
        "blinker::BleSetupCompletion::",
        "blinker::security::NoiseNnSession::",
    ),
}

UNSELECTED_EXTENSION_SYMBOLS = (
    "blinker::Esp32LocalWebSocket::",
    "blinker::Esp32LocalDiscovery::",
    "blinker::GatewayManagementClient::",
    "blinker::GatewayRouteClient::",
    "blinker::GattDirectChildLink::",
    "blinker::TimeSyncResponder::",
    "blinker::GatewayTimeResponder::",
)


def executable(explicit: str | None) -> str:
    if explicit:
        path = Path(explicit)
        if path.is_file():
            return str(path.resolve())
        raise SystemExit(f"arduino-cli executable not found: {path}")
    discovered = shutil.which("arduino-cli")
    if not discovered:
        raise SystemExit("arduino-cli was not found; pass --arduino-cli")
    return discovered


def run(
    command: list[str],
    environment: dict[str, str],
    *,
    echo_output: bool = True,
    log_path: Path | None = None,
) -> str:
    print("+ " + " ".join(command), flush=True)
    completed = subprocess.run(
        command,
        cwd=REPOSITORY,
        env=environment,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    output = completed.stdout or ""
    if log_path is not None:
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text(output, encoding="utf-8")
    if echo_output:
        print(output, end="" if output.endswith("\n") else "\n", flush=True)
    if completed.returncode != 0:
        raise SystemExit(f"command failed with exit code {completed.returncode}")
    return output


def installed_libraries(cli: str, environment: dict[str, str]) -> dict[str, dict]:
    output = run(
        [cli, "lib", "list", "--format", "json"],
        environment,
        echo_output=False,
    )
    payload = json.loads(output)
    return {
        item["library"]["name"]: item["library"]
        for item in payload.get("installed_libraries", ())
    }


def compiled_libraries(build: Path) -> frozenset[str]:
    paths = json.loads((build / "libraries.cache").read_text(encoding="utf-8"))
    return frozenset(
        Path(path).parent.name
        for path in paths
        if Path(path).name == "src" and "libraries" in Path(path).parts
    )


def verify_library_closure(target: Target, libraries: frozenset[str], config: str) -> None:
    # ESP32 3.x can expose its TLS socket as a separate SDK library. Older
    # cores may embed it. Neither case permits another user/transport library.
    optional = frozenset()
    if target.name.startswith("esp32-") and "WiFi" in target.libraries and re.search(
        r"^#define BLINKER_OFFICIAL_(?:MQTT|CLOUD_SESSION)_TLS 1$", config, re.MULTILINE
    ):
        optional = frozenset(("NetworkClientSecure",))
    if not target.libraries <= libraries or not libraries <= target.libraries | optional:
        raise SystemExit(
            f"{target.name} compiled libraries {sorted(libraries)}, "
            f"expected {sorted(target.libraries)}, optional SDK {sorted(optional)}"
        )


def verify_installed_bytes(archive: Path, installed_root: Path) -> None:
    """A marker is not proof: reject edits to an installed ZIP before compiling."""
    expected: set[Path] = set()
    roots: set[str] = set()
    with zipfile.ZipFile(archive) as bundle:
        for member in bundle.infolist():
            if member.is_dir():
                continue
            parts = member.filename.split("/")
            if len(parts) < 2 or any(
                part in ("", ".", "..") or "\\" in part or ":" in part
                for part in parts
            ):
                raise SystemExit("invalid archive member path")
            installed = installed_root.joinpath(*parts[1:])
            if installed in expected:
                raise SystemExit("duplicate archive member")
            expected.add(installed)
            roots.add(parts[0])
            if (
                installed.is_symlink()
                or not installed.is_file()
                or installed.read_bytes() != bundle.read(member)
            ):
                raise SystemExit(f"installed ZIP content changed: {'/'.join(parts[1:])}")
    if len(roots) != 1 or not expected:
        raise SystemExit("archive must contain one library root")
    actual = {path for path in installed_root.rglob("*") if path.is_file()}
    if actual != expected:
        raise SystemExit("unexpected files in installed ZIP")


def copy_dependencies(source: Path, sketchbook: Path, names: list[str]) -> None:
    """Optional offline cache; the normal version/path checks still apply."""
    for name in names:
        library = source / name
        if not (library / "library.properties").is_file():
            raise SystemExit(f"dependency cache is missing {name}")
        shutil.copytree(library, sketchbook / "libraries" / name)


def verify_provenance(build: Path, installed_root: Path) -> None:
    commands = json.loads(
        (build / "compile_commands.json").read_text(encoding="utf-8")
    )
    installed_source = (installed_root / "src").resolve()
    repository_source = (REPOSITORY / "src").resolve()
    compiled_blinker = 0
    for command in commands:
        source = Path(command["file"]).resolve()
        if source == repository_source or repository_source in source.parents:
            raise SystemExit(f"repository source leaked into ZIP gate: {source}")
        if source == installed_source or installed_source in source.parents:
            compiled_blinker += 1
        arguments = command.get("arguments", ())
        for index, argument in enumerate(arguments):
            include = ""
            if argument == "-I" and index + 1 < len(arguments):
                include = arguments[index + 1]
            elif argument.startswith("-I"):
                include = argument[2:]
            if include:
                header_root = Path(include).resolve()
                if header_root == repository_source or repository_source in header_root.parents:
                    raise SystemExit(f"repository headers leaked into ZIP gate: {include}")
    if compiled_blinker == 0:
        raise SystemExit("installed Blinker source was not compiled")


def toolchain_nm(build: Path) -> Path:
    commands = json.loads(
        (build / "compile_commands.json").read_text(encoding="utf-8")
    )
    compiler = Path(commands[0]["arguments"][0])
    name = compiler.name
    if name.lower().endswith(".exe"):
        name = name[:-4]
    if not name.endswith("g++"):
        raise SystemExit(f"unsupported compiler name for nm discovery: {name}")
    stem = name[:-3]
    for suffix in ("nm", "nm.exe"):
        candidate = compiler.parent / (stem + suffix)
        if candidate.is_file():
            return candidate
    raise SystemExit(f"nm not found beside compiler: {compiler}")


def defined_symbols(build: Path) -> str:
    elfs = tuple(build.glob("*.elf"))
    if len(elfs) != 1:
        raise SystemExit(f"build must produce exactly one ELF: {build}")
    completed = subprocess.run(
        [str(toolchain_nm(build)), "-C", "--defined-only", str(elfs[0])],
        cwd=REPOSITORY,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if completed.returncode != 0:
        raise SystemExit(f"nm failed for {elfs[0]}: {completed.stdout}")
    return completed.stdout or ""


def verify_time_closure(symbols: str, *, enabled: bool) -> None:
    for marker in (
        "blinker::TimeSyncService::",
        "blinker::TimeSyncCoordinator::",
        "blinker::WallClock::",
    ):
        if (marker in symbols) != enabled:
            raise SystemExit(f"optional business time link mismatch: enabled={enabled}, {marker}")


def verify_link_closure(
    target: Target, link_map: Path, symbols: str, *, time_enabled: bool | None = None
) -> None:
    verify_time_closure(
        symbols, enabled=target.time_enabled if time_enabled is None else time_enabled
    )
    for marker in ("blinker::TaskEngine::", "blinker::TaskManagementSession::",
                   "blinker::TaskPropertyTarget::"):
        if (marker in symbols) != target.tasks_enabled:
            raise SystemExit(f"optional task link mismatch: enabled={target.tasks_enabled}, {marker}")
    content = link_map.read_text(encoding="utf-8", errors="replace")
    members = frozenset(BLINKER_ARCHIVE_MEMBER.findall(content))
    if "BlinkerApi.cpp.o" not in members:
        raise SystemExit(f"{target.name} did not link the public facade")

    if target.example == "WiFiBleQuickStart":
        mode = "wifi-ble"
    elif target.example == "BleQuickStart":
        mode = "ble"
    else:
        mode = "wifi"
    required = ("BlinkerTransport.cpp.o",)
    missing_symbols = sorted(
        marker for marker in REQUIRED_SYMBOLS[mode] if marker not in symbols
    )
    if missing_symbols:
        raise SystemExit(
            f"{target.name} did not retain required symbols: {missing_symbols}"
        )
    missing = sorted(member for member in required if member not in members)
    if missing:
        raise SystemExit(
            f"{target.name} did not link required members: {missing}"
        )

    leaked_symbols = sorted(
        marker
        for marker in (*FORBIDDEN_SYMBOLS[mode], *UNSELECTED_EXTENSION_SYMBOLS)
        if marker in symbols
    )
    if leaked_symbols:
        raise SystemExit(
            f"{target.name} retained obsolete symbols: {leaked_symbols}"
        )

    platform = next(
        name
        for name in PLATFORMS
        if target.name.startswith(name + "-")
    )
    platform_leaks = sorted(
        marker
        for name, marker in (
            ("esp32", "blinker::Esp32"),
            ("uno-r4", "blinker::UnoR4"),
            ("wio-terminal", "blinker::WioTerminal"),
        )
        if name != platform and marker in symbols
    )
    if platform_leaks:
        raise SystemExit(
            f"{target.name} linked another platform: {platform_leaks}"
        )


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--arduino-cli")
    parser.add_argument("--zip-path", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, required=True)
    parser.add_argument("--data-dir", type=Path)
    parser.add_argument("--downloads-dir", type=Path)
    parser.add_argument(
        "--dependency-dir", type=Path,
        help="optional local library cache for offline installation",
    )
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument(
        "--reuse-work-dir",
        action="store_true",
        help=(
            "reuse an already isolated sketchbook after a completed ZIP and "
            "dependency install; installed versions are still verified"
        ),
    )
    parser.add_argument(
        "--target",
        action="append",
        choices=tuple(target.name for target in (*TARGETS, TASK_PILOT_TARGET)),
    )
    return parser.parse_args()


def main() -> int:
    gate_started = time.perf_counter()
    arguments = parse_arguments()
    archive = arguments.zip_path.resolve()
    work = arguments.work_dir.resolve()
    if not archive.is_file():
        raise SystemExit(f"release ZIP not found: {archive}")
    if (
        work.exists()
        and any(work.iterdir())
        and not arguments.reuse_work_dir
    ):
        raise SystemExit(f"work-dir must be absent or empty: {work}")
    work.mkdir(parents=True, exist_ok=True)
    archive_marker = work / ".blinker-release-sha256"
    archive_digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if arguments.reuse_work_dir:
        if (
            not archive_marker.is_file()
            or archive_marker.read_text(encoding="ascii").strip()
            != archive_digest
        ):
            raise SystemExit(
                "reused work-dir was not installed from this exact ZIP"
            )

    cli = executable(arguments.arduino_cli)
    sketchbook = work / "sketchbook"
    environment = os.environ.copy()
    environment["ARDUINO_DIRECTORIES_USER"] = str(sketchbook)
    environment["ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL"] = "true"
    if arguments.data_dir:
        environment["ARDUINO_DIRECTORIES_DATA"] = str(
            arguments.data_dir.resolve()
        )
    if arguments.downloads_dir:
        environment["ARDUINO_DIRECTORIES_DOWNLOADS"] = str(
            arguments.downloads_dir.resolve()
        )

    selected_targets = select_targets(arguments.target)
    if not arguments.reuse_work_dir:
        install_started = time.perf_counter()
        run([cli, "lib", "install", "--zip-path", str(archive)], environment)
        archive_marker.write_text(archive_digest + "\n", encoding="ascii")
        print(
            f"release ZIP installed in {time.perf_counter() - install_started:.1f}s",
            flush=True,
        )
    # Arduino CLI deliberately does not resolve library.properties `depends`
    # for a local --zip-path install, so the gate installs the pinned set in
    # the same empty sketchbook before compiling the installed archive.
    dependency_names = sorted(
        library
        for library in DEPENDENCIES
        if any(library in target.libraries for target in selected_targets)
    )
    if dependency_names and not arguments.reuse_work_dir:
        dependencies_started = time.perf_counter()
        if arguments.dependency_dir:
            copy_dependencies(
                arguments.dependency_dir.resolve(), sketchbook, dependency_names
            )
        else:
            run(
                [cli, "lib", "install", *(DEPENDENCIES[name][0] for name in dependency_names)],
                environment,
            )
        print(
            "release dependencies installed in "
            f"{time.perf_counter() - dependencies_started:.1f}s",
            flush=True,
        )
    libraries = installed_libraries(cli, environment)
    expected_versions = {"BlinkerDeviceV2": "0.1.0"}
    expected_versions.update(
        (DEPENDENCIES[name][1], DEPENDENCIES[name][2])
        for name in dependency_names
    )
    for name, version in expected_versions.items():
        actual = libraries.get(name, {}).get("version")
        if actual != version:
            raise SystemExit(
                f"installed {name} version is {actual!r}, expected {version!r}"
            )

    installed_root = Path(libraries["BlinkerDeviceV2"]["install_dir"]).resolve()
    expected_root = (sketchbook / "libraries" / "BlinkerDeviceV2").resolve()
    if installed_root != expected_root:
        raise SystemExit(f"Blinker was not installed in the empty sketchbook: {installed_root}")
    verify_installed_bytes(archive, installed_root)

    for target in selected_targets:
        build = work / "build" / target.name
        example = (REPOSITORY / "tests" / "arduino" if target.tasks_enabled
                   else installed_root / "examples") / target.example
        compile_started = time.perf_counter()
        output = run(
            [
                cli,
                "compile",
                "--fqbn",
                target.fqbn,
                "--clean",
                "--jobs",
                str(arguments.jobs),
                "--warnings",
                "none",
                "--build-path",
                str(build),
                str(example),
            ],
            environment,
            log_path=build / "compile.log",
        )
        if "Sketch uses" not in output:
            raise SystemExit(f"Arduino size summary missing: {target.name}")
        linked_libraries = compiled_libraries(build)
        verify_library_closure(
            target, linked_libraries,
            (installed_root / "src/BlinkerV2/arduino/config/OfficialConfig.h").read_text(encoding="utf-8"),
        )
        maps = tuple(build.glob("*.map"))
        if len(maps) != 1:
            raise SystemExit(f"{target.name} must produce exactly one link map")
        verify_link_closure(target, maps[0], defined_symbols(build))
        verify_provenance(build, installed_root)
        print(
            f"installed ZIP target passed: {target.name} "
            f"({time.perf_counter() - compile_started:.1f}s)",
            flush=True,
        )

    print(
        "installed Arduino ZIP gate passed: "
        + ", ".join(target.name for target in selected_targets),
        flush=True,
    )
    print(
        f"installed Arduino ZIP gate completed in "
        f"{time.perf_counter() - gate_started:.1f}s",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
