#!/usr/bin/env python3
"""Install the fixed R4 Tasks4 Plain board without modifying SDKs or libraries.

This Windows-maintained profile is a separate sketchbook hardware platform,
not another Blinker library. Installation never compiles, uploads or grants a
device's server-side task admission. Unknown or modified installations are
never overwritten; uninstall preserves the verified platform by moving it.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import stat
import tempfile
import uuid

import prepare_r4_restricted_release as release

ROOT = Path(__file__).resolve().parents[1]
TARGET = Path("hardware/blinker/renesas_uno")
FQBN = "blinker:renesas_uno:r4tasks"
MANIFEST = "blinker-profile.json"
BOARD_NAME = "Blinker UNO R4 WiFi - Tasks4 Plain (Core 1.5.3)"
HEAP_HEADER = "arduino/ports/uno_r4/UnoR4HeapBoundary.h"
OVERLAYS = {"Modem.cpp", "Modem.h", "WiFiCommands.h", "StringHelpers.h",
            "Serial.cpp", "Serial.h"}


def checked_path(value):
    """Check the lexical path before resolving, including junction ancestors."""
    path = Path(os.path.abspath(value))
    if any(character in str(path) for character in ('"', "\r", "\n", "\t", "{", "}")):
        raise ValueError("profile paths cannot contain property metacharacters")
    for entry in (path, *path.parents):
        try:
            metadata = entry.lstat()
        except FileNotFoundError:
            continue
        if (stat.S_ISLNK(metadata.st_mode) or
                getattr(metadata, "st_file_attributes", 0) &
                getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)):
            raise ValueError("profile paths cannot contain symbolic links or junctions")
    return path.resolve()


def tree(directory):
    directory = checked_path(directory)
    if not directory.is_dir():
        raise ValueError("required profile input directory is missing")
    files = {}
    for parent, directories, names in os.walk(directory, followlinks=False):
        for name in directories + names:
            file = checked_path(Path(parent) / name)
            if file.is_file():
                files[file.relative_to(directory).as_posix()] = file
    return dict(sorted(files.items()))


def properties(file):
    return dict(line.split("=", 1) for line in
                file.read_text(encoding="utf-8").splitlines()
                if "=" in line and not line.startswith("#"))


def find_library(book, name):
    libraries = checked_path(book / "libraries")
    if not libraries.is_dir():
        raise ValueError("install the single Blinker library and dependencies first")
    matches = []
    for directory in sorted(libraries.iterdir()):
        directory = checked_path(directory)
        metadata = directory / "library.properties"
        if directory.is_dir() and metadata.is_file() and properties(checked_path(metadata)).get("name") == name:
            matches.append(directory)
    if len(matches) != 1:
        raise ValueError("exactly one sketchbook library is required: " + name)
    return matches[0]


def require_same(actual, desired, message):
    if set(actual) != set(desired) or any(
            release.digest(actual[name]) != release.digest(value)
            for name, value in desired.items()):
        raise ValueError(message)


def within_book(book, path):
    path = checked_path(path)
    if path == book or not path.is_relative_to(book):
        raise ValueError("profile operation must remain inside the sketchbook")
    return path


def expected(sdk, compiler, bossac, profile_path, sketchbook):
    sdk, compiler, bossac, profile_path, book = (
        checked_path(path) for path in (sdk, compiler, bossac, profile_path, sketchbook))
    target = within_book(book, book / TARGET)
    if bossac.is_file():
        bossac = bossac.parent
    if compiler.name != "bin" or compiler.parent.name != "7-2017q4":
        raise ValueError("profile requires the original ARM compiler 7-2017q4/bin")
    if bossac.name != "1.9.1-arduino5":
        raise ValueError("profile requires original bossac 1.9.1-arduino5")
    for directory, names in ((compiler, ("arm-none-eabi-gcc", "arm-none-eabi-g++", "arm-none-eabi-ar")),
                             (bossac, ("bossac",))):
        for name in names:
            if not any((directory / (name + suffix)).is_file() for suffix in (".exe", "")):
                raise ValueError("required original tool is missing: " + name)
    if "version=1.5.3" not in checked_path(sdk / "platform.txt").read_text(encoding="utf-8").splitlines():
        raise ValueError("profile requires original Renesas Core 1.5.3")
    if any((sdk / name).exists() for name in
           ("platform.local.txt", "boards.local.txt", "programmers.local.txt")):
        raise ValueError("original SDK local overrides are not supported")
    if any((sdk / name).exists() for name in
           ("blinker", "cores/arduino/BlinkerV2", "cores/arduino/BlinkerR4HeapBoundary.cpp", MANIFEST)):
        raise ValueError("SDK already contains maintenance profile wiring")
    for path in (sdk, compiler.parent, bossac, profile_path, ROOT / "src"):
        if target.is_relative_to(path) or path.is_relative_to(target):
            raise ValueError("profile target must be disjoint from its inputs")

    profile = release.restricted_profile(profile_path)
    blinker = find_library(book, "BlinkerDeviceV2")
    pubsub = find_library(book, "PubSubClient")
    crypto = find_library(book, "Crypto")
    library = {"library.properties": blinker / "library.properties"}
    library.update({"src/" + key: value for key, value in tree(blinker / "src").items()})
    configured_library = release.configured_library(profile)
    require_same(library, configured_library,
                 "single Blinker library source/public profile does not match this maintenance profile")
    reference = "examples/R4TasksQuickStart/R4TasksQuickStart.ino"
    if release.digest(checked_path(blinker / reference)) != release.digest(ROOT / reference):
        raise ValueError("single Blinker library R4 reference example does not match")
    dependencies = {}
    for directory, name, version in ((pubsub, "PubSubClient", "2.8"), (crypto, "Crypto", "0.4.0")):
        # Validate links before the shared dependency reader inspects any files.
        tree(directory)
        dependencies[name] = release.library_inputs(directory, name, version)

    base, base_receipt = release.candidate.expected_candidate(
        sdk / "libraries/WiFiS3/src", target / "blinker", pubsub / "src")
    names = {key[len("sdk-overlay/"):] for key in base
             if key.startswith("sdk-overlay/") and key != "sdk-overlay/overlay.json"}
    if names != OVERLAYS:
        raise ValueError("maintenance profile must replace exactly the reviewed six SDK files")
    sdk_files = tree(sdk)
    files = dict(sdk_files)
    for name in OVERLAYS:
        prefix = "cores/arduino/" if name.startswith("Serial.") else "libraries/WiFiS3/src/"
        files[prefix + name] = base["sdk-overlay/" + name]
    core_headers = (*release.CORE_CONTRACT_HEADERS, HEAP_HEADER)
    for name in core_headers:
        files["cores/arduino/BlinkerV2/" + name] = ROOT / "src/BlinkerV2" / name
    files["blinker/Config.h"] = release.fixed_config(blinker, configured_library)
    files["blinker/CandidateConfig.h"] = base["CandidateConfig.h"]
    files["blinker/TaskHeapBoundary.h"] = base["TaskHeapBoundary.h"]
    # Whole-archive in the pinned Core recipe retains exactly this wrapper.
    # Its only library helper is the pure arithmetic header mirrored above.
    files["cores/arduino/BlinkerR4HeapBoundary.cpp"] = (
        b'// Maintained Core entry; no duplicate Sketch heap adapter.\n'
        b'#include "../../blinker/TaskHeapBoundary.h"\n')

    board = [line.replace("unor4wifi.", "r4tasks.", 1) for line in
             checked_path(sdk / "boards.txt").read_text(encoding="utf-8").splitlines()
             if line.startswith("unor4wifi.")]
    if not board or "r4tasks.build.core=arduino" not in board:
        raise ValueError("original UNO R4 WiFi board/core definition is missing")
    board = [line for line in board if not line.startswith(
        ("r4tasks.name=", "r4tasks.build.compiler_path="))]
    board += ["r4tasks.name=" + BOARD_NAME,
              "r4tasks.build.compiler_path=" + compiler.as_posix() + "/"]
    files["boards.txt"] = ("\n".join(board) + "\n").encode()
    flags = '-fstack-usage -include "{runtime.platform.path}/blinker/Config.h"'
    files["platform.local.txt"] = ("\n".join((
        "name=Blinker maintained R4 Tasks4 Plain",
        "compiler.c.extra_flags=" + flags,
        "compiler.cpp.extra_flags=" + flags,
        "compiler.S.extra_flags=-DNO_USB " + flags,
        "compiler.c.elf.extra_flags=" + " ".join(base_receipt["linkFlags"]),
        "tools.bossac.path=" + bossac.as_posix(),
    )) + "\n").encode()

    inputs = {"sdk/" + key: release.digest(value) for key, value in sdk_files.items()}
    inputs.update({"source/" + key: release.digest(value)
                   for key, value in release.candidate.source_paths().items()})
    for name, values in (("compiler", tree(compiler.parent)), ("bossac", tree(bossac)),
                         ("single-library", library)):
        inputs.update({name + "/" + key: release.digest(value) for key, value in values.items()})
    for name, values in dependencies.items():
        inputs.update({"dependencies/" + name + "/" + key: release.digest(value)
                       for key, value in values.items()})
    inputs.update({"public-profile": release.digest(profile_path),
                   "installer": release.digest(Path(__file__)),
                   "restricted-generator": release.digest(Path(release.__file__)),
                   "fixed-example": release.digest(ROOT / reference)})
    receipt = {
        "schema": "r4-arduino-ide-profile/1", "state": "prepared-profile-not-linked",
        "target": str(target), "fqbn": FQBN, "core": "1.5.3",
        "compilerDirectory": str(compiler), "bossacDirectory": str(bossac),
        "sdkInputDirectory": str(sdk), "singleLibraryDirectory": str(blinker),
        "installedSdkModified": False, "libraryModified": False,
        "hardwareWritten": False, "linked": False, "formalTaskAdmission": False,
        "fixedGraph": release.FIXED, "coreContractHeaders": list(core_headers),
        "inputSha256": inputs,
        "outputSha256": {key: release.digest(value) for key, value in files.items()},
        "selection": "Restart Arduino IDE, choose the separate Blinker Tasks4 Plain board; select the official UNO R4 WiFi board to return to ordinary default zero slots.",
        "support": "Only the reviewed R4TasksQuickStart graph, Plain transport and matching server-side admission; not arbitrary Sketch admission.",
    }
    return files, receipt


def verify(sdk, compiler, bossac, profile_path, sketchbook):
    files, receipt = expected(sdk, compiler, bossac, profile_path, sketchbook)
    target = checked_path(receipt["target"])
    actual = tree(target)
    if set(actual) != set(files) | {MANIFEST}:
        raise ValueError("installed profile has missing or unlisted files")
    require_same({key: value for key, value in actual.items() if key != MANIFEST},
                 files, "installed maintenance profile was modified")
    if json.loads((target / MANIFEST).read_text(encoding="utf-8")) != receipt:
        raise ValueError("installed profile manifest/input mismatch")
    return receipt


def install(sdk, compiler, bossac, profile_path, sketchbook):
    files, receipt = expected(sdk, compiler, bossac, profile_path, sketchbook)
    book, target = checked_path(sketchbook), checked_path(receipt["target"])
    if target.exists():
        return verify(sdk, compiler, bossac, profile_path, sketchbook)
    # A failed stage is preserved for diagnosis. Nothing in hardware is made
    # visible until every file and the manifest have been written and checked.
    stage = within_book(book, Path(tempfile.mkdtemp(prefix=".blinker-r4-stage-", dir=book)))
    for key, value in files.items():
        file = within_book(stage, stage / key)
        file.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(value, bytes):
            with file.open("xb") as stream:
                stream.write(value)
        else:
            shutil.copyfile(value, file)
    (stage / MANIFEST).write_bytes(release.json_bytes(receipt))
    require_same({key: value for key, value in tree(stage).items() if key != MANIFEST},
                 files, "prepared profile changed before installation")
    target.parent.mkdir(parents=True, exist_ok=True)
    within_book(book, target.parent)
    if target.exists():
        raise ValueError("profile target appeared during installation; stage preserved")
    stage.rename(target)
    return verify(sdk, compiler, bossac, profile_path, sketchbook)


def uninstall(sdk, compiler, bossac, profile_path, sketchbook):
    receipt = verify(sdk, compiler, bossac, profile_path, sketchbook)
    book = checked_path(sketchbook)
    target = within_book(book, Path(receipt["target"]))
    archive = within_book(book, book / ".blinker-profile-backups" / uuid.uuid4().hex / "renesas_uno")
    archive.parent.mkdir(parents=True, exist_ok=False)
    within_book(book, archive.parent)
    target.rename(archive)
    return {**receipt, "state": "uninstalled-preserved", "archivedTo": str(archive)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("sdk", "compiler", "bossac", "profile", "sketchbook"):
        parser.add_argument("--" + name, type=Path, required=True)
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--verify-only", action="store_true")
    action.add_argument("--uninstall", action="store_true")
    arguments = parser.parse_args()
    operation = uninstall if arguments.uninstall else verify if arguments.verify_only else install
    result = operation(arguments.sdk, arguments.compiler, arguments.bossac,
                       arguments.profile, arguments.sketchbook)
    print(json.dumps({key: result[key] for key in
                     ("schema", "state", "target", "fqbn", "installedSdkModified", "libraryModified")}
                     | ({"archivedTo": result["archivedTo"]} if "archivedTo" in result else {})))


if __name__ == "__main__":
    main()
