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
sys.path.insert(0, str(REPOSITORY / "tools"))
from arduino_release_profile import CLOUD_WIFI_PROFILE_ID, TASK_PILOT, TASK_PILOT_ID


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

# Packaged in the same library, but intentionally not a stock-IDE compile
# target: its matching isolated Core/Modem/heap profile is verified separately.
# Do not silently add it to the ordinary seven task-disabled targets.
PROFILE_GATED_EXAMPLES = ("R4TasksQuickStart",)

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


def verify_installed_product_profile(config: str, targets: tuple[Target, ...]) -> str | None:
    values = re.findall(
        r"^[ \t]*#[ \t]*define[ \t]+BLINKER_OFFICIAL_PRODUCT_PROFILE\b([^\r\n]*)$",
        config, re.MULTILINE,
    )
    if not values:
        return None  # Existing full profiles keep their original target matrix.
    if [value.strip() for value in values] != [f'"{CLOUD_WIFI_PROFILE_ID}"']:
        raise SystemExit("installed product profile marker is invalid or duplicated")
    if any(target.name not in ("esp32-wifi", "esp32-cloud-smoke") or target.tasks_enabled
           for target in targets):
        raise SystemExit("cloud WiFi profile admits only esp32-wifi and esp32-cloud-smoke; "
                         "select them explicitly, not the full seven-target matrix")
    return CLOUD_WIFI_PROFILE_ID


def verify_installed_task_profile(config: str, targets: tuple[Target, ...]) -> str | None:
    """Bind the explicit task consumer to the installed development policy.

    Reading the tuple only identifies the existing ESP32 candidate; ordinary
    task-disabled links do not verify it or admit another platform's tasks.
    """
    definitions = re.findall(
        r"^[ \t]*#[ \t]*define[ \t]+BLINKER_TASK_([A-Z_]+)\b([^\r\n]*)$",
        config, re.MULTILINE,
    )
    profile = None
    if definitions:
        values = {}
        for name, raw in definitions:
            number = re.fullmatch(r"([0-9]+)[uU]?", raw.strip())
            if name not in TASK_PILOT or name in values or not number:
                raise SystemExit("installed task policy is not the supported development profile")
            values[name] = int(number[1])
        channels = re.findall(r'\breleaseChannel\[\]\s*=\s*"([^"]+)"\s*;', config)
        if values != TASK_PILOT or channels != ["development"]:
            raise SystemExit("installed task policy is not the supported development profile")
        profile = TASK_PILOT_ID
    task_targets = tuple(target for target in targets if target.tasks_enabled)
    if task_targets and (profile != TASK_PILOT_ID or task_targets != (TASK_PILOT_TARGET,)):
        raise SystemExit("explicit task target requires the installed ESP32 development task profile")
    return profile


def task_verification_scope(targets: tuple[Target, ...], profile: str | None) -> str:
    """Report only after every selected compile/link/provenance check passed."""
    boundary = ("ordinary installed R4/Wio task targets not verified by this gate; "
                "the separately admitted fixed R4 Plain candidate is outside this matrix")
    if any(target.tasks_enabled for target in targets):
        return f"installed task compile/link target verified: {TASK_PILOT_TARGET.name} ({profile}); {boundary}"
    return f"installed task targets not verified: none selected (ordinary task-disabled targets only); {boundary}"


def profile_gated_example_scope() -> str:
    return ("profile-gated reference packaged, not compiled by the ordinary installed matrix: "
            + ", ".join(PROFILE_GATED_EXAMPLES)
            + "; requires the separately verified isolated R4 Core/Modem/heap build profile")

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


def verify_wio_qspi_closure(
    symbols: str, *, task_capacity: int = 0, region_reserved: bool = False
) -> None:
    """Check a known Wio link graph, not physical ownership or release admission.

    Task callers must separately attest the installed source/profile's tail
    reservation. The allocator symbols identify the reviewed internal closure,
    not hook behavior, concurrency safety or formal admission. SDK, actual macros
    and provenance remain compile-gate checks; symbols cannot detect arbitrary
    unknown/inline third-party QSPI drivers.
    """
    if type(task_capacity) is not int or task_capacity not in (0, 4):
        raise SystemExit("Wio task graph supports only capacity 0 or 4")
    if type(region_reserved) is not bool or (task_capacity and not region_reserved):
        raise SystemExit("Wio task graph requires explicit tail-region attestation")
    owners = [line for line in symbols.splitlines()
              if "guard variable for " not in line
              and re.search(r"\s[bBdD]\s+blinker::wioTerminalQspiOwner\(\)::owner$", line)]
    if len(owners) != 1:
        raise SystemExit("Wio link requires exactly one default Blinker QSPI owner")
    # Reuse the finite candidate's known conflict list; generic FS/SD interfaces
    # are permitted. Unrecognized physical layouts still need source attestation.
    conflicts = re.compile(
        r"sfud_|SFUD::|SPIFlash|Arduino_QSPI|QSPIFlash|Seeed_FS::(?:beginFlash|formatFlash)"
    )
    if conflicts.search(symbols):
        raise SystemExit("Wio link includes a conflicting onboard QSPI/Flash driver")
    capacities = {int(value) for value in re.findall(
        r"blinker::WioTerminalTaskSnapshotBlobStore<(\d+)[uUlL]*>", symbols
    )}
    task_markers = ("blinker::TaskEngine::", "blinker::TaskSnapshotStore::",
                    "blinker::TaskCloudTransport<")
    allocator_symbols = ("blinker_wio_newlib_lock_anchor", "__malloc_lock", "__malloc_unlock")
    defined = {name: [] for name in allocator_symbols}
    for line in symbols.splitlines():
        row = re.fullmatch(r"\s*[0-9A-Fa-f]+(?:\s+[0-9A-Fa-f]+)?\s+([A-Za-z?])\s+(.+?)\s*", line)
        if row and row[1] != "U" and row[2] in defined:
            defined[row[2]].append(row[1])
    if not task_capacity:
        if capacities or any(marker in symbols for marker in task_markers):
            raise SystemExit("Wio default link unexpectedly retains a task graph")
        # The unmodified SDK may contain its own empty malloc hooks. Only the
        # exact internal anchor proves accidental inclusion of our experiment.
        if defined[allocator_symbols[0]]:
            raise SystemExit("Wio default link retained the internal allocator anchor")
    elif (capacities != {4} or not all(marker in symbols for marker in task_markers)
          or "Ed25519::verify(" not in symbols):
        raise SystemExit("Wio task4 link requires the original store/engine/carrier/Ed25519 graph")
    elif any(types != ["T"] for types in defined.values()):
        raise SystemExit("Wio task4 link requires exactly one strong allocator anchor and each malloc hook")


def verify_ordinary_r4_closure(symbols: str) -> None:
    """Keep the internal Modem/UART and heap experiment out of ordinary links.

    Inspect actual ``nm --defined-only`` rows, not source/header names. This
    rejects an accidentally patched SDK/cache even when tasks are disabled;
    it does not certify the native SDK or admit an internal candidate.
    """
    forbidden = re.compile(
        r"(?:blinker_r4_rpc_[A-Za-z0-9_]+|__wrap__sbrk|__blinker_r4_heap_limit)"
        r"(?: \[clone [^\]]+\])?"
    )
    leaked = []
    for line in symbols.splitlines():
        row = re.fullmatch(r"\s*[0-9A-Fa-f]+\s+([A-Za-z?])\s+(.+?)\s*", line)
        if row and row[1] != "U" and forbidden.fullmatch(row[2]):
            leaked.append(row[2])
    if leaked:
        raise SystemExit("ordinary R4 link retained internal resource candidate: "
                         + ", ".join(sorted(set(leaked))))


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
    if platform == "wio-terminal":
        verify_wio_qspi_closure(symbols)
    elif platform == "uno-r4":
        verify_ordinary_r4_closure(symbols)
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
    config = (installed_root / "src/BlinkerV2/arduino/config/OfficialConfig.h").read_text(encoding="utf-8")
    verify_installed_product_profile(config, selected_targets)
    task_profile = verify_installed_task_profile(config, selected_targets)

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
            target, linked_libraries, config,
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
    print(task_verification_scope(selected_targets, task_profile), flush=True)
    print(profile_gated_example_scope(), flush=True)
    print(
        f"installed Arduino ZIP gate completed in "
        f"{time.perf_counter() - gate_started:.1f}s",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
