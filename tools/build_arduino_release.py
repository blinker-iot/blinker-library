#!/usr/bin/env python3
"""Validate and build the single public Blinker Arduino library ZIP."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import shutil
import sys
import zipfile

from arduino_release_profile import apply_profile, load_profile


REPOSITORY = Path(__file__).resolve().parents[1]
ROOT_FILES = (
    "library.properties",
    "library.json",
    "keywords.txt",
    "LICENSE",
    "README.md",
)
PUBLIC_EXAMPLES = (
    "WiFiQuickStart",
    "BleQuickStart",
    "WiFiBleQuickStart",
    "CloudSmokeTest",
)
EXPECTED_DEPENDENCIES = {
    "PubSubClient",
    "ArduinoBLE",
    "Crypto",
    "Seeed Arduino rpcWiFi",
    "Seeed Arduino rpcBLE",
    "Seeed Arduino rpcUnified",
}
EXPECTED_DEPENDS = (
    "PubSubClient (=2.8),ArduinoBLE (=2.0.2),Crypto (=0.4.0),"
    "Seeed Arduino rpcWiFi (=1.1.0),"
    "Seeed Arduino rpcBLE (=1.0.0),"
    "Seeed Arduino rpcUnified (=2.1.4)"
)
VERSION = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+$")
SAFE_NAME = re.compile(r"^[A-Za-z][A-Za-z0-9_.-]{0,62}$")
EXPECTED_IMPLEMENTATION_UNITS = {
    "BlinkerApi.cpp",
    "BlinkerArduino.cpp",
    "BlinkerControl.cpp",
    "BlinkerCore.cpp",
    "BlinkerEsp32.cpp",
    "BlinkerEsp32LocalWebSocket.cpp",
    "BlinkerIdentity.cpp",
    "BlinkerModel.cpp",
    "BlinkerProtocol.cpp",
    "BlinkerProvisioningBle.cpp",
    "BlinkerProvisioningController.cpp",
    "BlinkerProvisioningWifi.cpp",
    "BlinkerRuntime.cpp",
    "BlinkerSecurity.cpp",
    "BlinkerTransport.cpp",
    "BlinkerUnoR4.cpp",
    "BlinkerWioTerminal.cpp",
}


def read_properties() -> dict[str, str]:
    result: dict[str, str] = {}
    path = REPOSITORY / "library.properties"
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise ValueError(f"{path}:{number}: expected key=value")
        key, value = line.split("=", 1)
        key = key.strip()
        if key in result:
            raise ValueError(f"{path}:{number}: duplicate {key}")
        result[key] = value.strip()
    return result


def dependency_names(value: str) -> set[str]:
    return {
        re.sub(r"\s*\([^)]*\)\s*$", "", item).strip()
        for item in value.split(",")
        if item.strip()
    }


def validate() -> tuple[dict[str, str], tuple[Path, ...]]:
    properties = read_properties()
    required = {
        "name",
        "version",
        "architectures",
        "includes",
        "depends",
        "dot_a_linkage",
    }
    missing = required - properties.keys()
    if missing:
        raise ValueError("library.properties missing: " + ", ".join(sorted(missing)))
    if not SAFE_NAME.fullmatch(properties["name"]):
        raise ValueError("unsafe Arduino library name")
    if not VERSION.fullmatch(properties["version"]):
        raise ValueError("version must be semantic x.y.z")
    if properties["architectures"] != "esp32,renesas_uno,samd":
        raise ValueError(
            "public architectures must be esp32,renesas_uno,samd"
        )
    if properties["includes"] != "Blinker.h":
        raise ValueError("Arduino IDE must insert the default WiFi selector")
    if properties["dot_a_linkage"] != "true":
        raise ValueError(
            "unified library requires archive member dead-code extraction"
        )
    if (
        dependency_names(properties["depends"]) != EXPECTED_DEPENDENCIES
        or properties["depends"] != EXPECTED_DEPENDS
    ):
        raise ValueError("unified dependency set changed")

    source = REPOSITORY / "src"
    if not source.is_dir():
        raise ValueError("src directory is missing")
    for header in (
        "Blinker.h",
        "BlinkerWiFi.h",
        "BlinkerBLE.h",
        "BlinkerWiFiBLE.h",
    ):
        if not (source / header).is_file():
            raise ValueError(f"public header is missing: {header}")
    cpp_sources = list(source.rglob("*.cpp"))
    if {path.name for path in cpp_sources} != EXPECTED_IMPLEMENTATION_UNITS:
        raise ValueError("Arduino implementation-unit boundary changed")
    implementation = source / "BlinkerV2" / "implementation"
    if any(path.parent != implementation for path in cpp_sources):
        raise ValueError("ordinary .cpp must remain in BlinkerV2/implementation")
    object_sources = cpp_sources + list(source.rglob("*.c"))
    object_names: dict[str, Path] = {}
    for path in object_sources:
        member = path.with_suffix(".o").name.casefold()
        previous = object_names.get(member)
        if previous is not None:
            raise ValueError(
                "Arduino archive object-name collision: "
                f"{previous.relative_to(REPOSITORY)} and "
                f"{path.relative_to(REPOSITORY)}"
            )
        object_names[member] = path
    for retired in (REPOSITORY / "ports", REPOSITORY / "products"):
        if retired.exists() and any(path.is_file() for path in retired.rglob("*")):
            raise ValueError(f"retired public source tree is not empty: {retired}")
    example_root = REPOSITORY / "examples"
    actual_examples = {
        path.name for path in example_root.iterdir() if path.is_dir()
    }
    if actual_examples != set(PUBLIC_EXAMPLES):
        raise ValueError("public example set changed")
    for name in PUBLIC_EXAMPLES:
        if not (example_root / name / f"{name}.ino").is_file():
            raise ValueError(f"example sketch is missing: {name}")

    files: list[Path] = []
    for name in ROOT_FILES:
        path = REPOSITORY / name
        if not path.is_file():
            raise ValueError(f"release file is missing: {name}")
        files.append(path)
    files.extend(path for path in source.rglob("*") if path.is_file())
    for name in PUBLIC_EXAMPLES:
        files.extend(
            path for path in (example_root / name).rglob("*") if path.is_file()
        )
    root_files = {REPOSITORY / name for name in ROOT_FILES}
    source_suffixes = {".h", ".hpp", ".c", ".cpp", ".inc", ".ipp", ".ino"}
    invalid = [
        path for path in files
        if path.is_symlink() or "__pycache__" in path.parts
        or (path not in root_files and path.suffix not in source_suffixes)
    ]
    if invalid:
        raise ValueError(f"non-portable release file: {invalid[0]}")
    return properties, tuple(sorted(set(files)))


def stage(destination: Path, files: tuple[Path, ...]) -> None:
    if destination.exists() and any(destination.iterdir()):
        raise ValueError("stage-dir must be absent or empty")
    destination.mkdir(parents=True, exist_ok=True)
    for source in files:
        relative = source.relative_to(REPOSITORY)
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)


def archive(
    stage_root: Path,
    output: Path,
    properties: dict[str, str],
) -> Path:
    output.mkdir(parents=True, exist_ok=True)
    root = f"{properties['name']}-{properties['version']}"
    target = output / f"{root}.zip"
    with zipfile.ZipFile(target, "w") as bundle:
        for path in sorted(stage_root.rglob("*")):
            if not path.is_file():
                continue
            info = zipfile.ZipInfo(
                f"{root}/{path.relative_to(stage_root).as_posix()}",
                date_time=(1980, 1, 1, 0, 0, 0),
            )
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            bundle.writestr(info, path.read_bytes())
    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    (output / "SHA256SUMS").write_text(
        f"{digest}  {target.name}\n", encoding="ascii", newline="\n"
    )
    return target


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check-only", action="store_true")
    parser.add_argument("--stage-dir", type=Path)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--profile", type=Path)
    parser.add_argument(
        "--allow-development-profile",
        action="store_true",
        help="allow a non-production profile for CI compile gates",
    )
    args = parser.parse_args()
    properties, files = validate()
    print(f"single Arduino library validated: {len(files)} files")
    if args.check_only:
        if any((args.stage_dir, args.output_dir, args.profile)) or args.allow_development_profile:
            raise ValueError("--check-only cannot stage, configure or archive")
        return 0
    if args.stage_dir is None:
        raise ValueError("--stage-dir is required unless --check-only is used")
    if args.profile is None:
        raise ValueError("--profile is required for a staged release")
    profile = load_profile(args.profile.resolve())
    if not profile.production and not args.allow_development_profile:
        raise ValueError(
            "development release profile requires --allow-development-profile"
        )
    stage_root = args.stage_dir.resolve()
    stage(stage_root, files)
    apply_profile(stage_root, profile)
    print(f"Arduino release profile applied: {profile.channel}")
    print(f"single Arduino library staged: {stage_root}")
    if args.output_dir is not None:
        target = archive(stage_root, args.output_dir.resolve(), properties)
        print(f"single Arduino archive built: {target}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(1)
