#!/usr/bin/env python3
"""Prepare a fixed R4 task build stage; never build, upload or grant admission.

The ordinary ZIP/profile and installed Core remain unchanged. This separate
maintainer plan reuses the pinned SDK overlay, heap adapter and public profile
renderer. A prepared receipt is not a link/runtime/release certificate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

import prepare_r4_internal_candidate as candidate
from arduino_release_profile import (LIBRARY_IDENTITY_HEADER, configured_library_files,
                                     library_identity_words, load_profile)

ROOT = Path(__file__).resolve().parents[1]
TEMPLATES = Path(__file__).resolve().parent / "templates/r4_restricted"
FIXED = {
    "BLINKER_RESOURCE_PROFILE": 1, "BLINKER_ENABLE_TASKS": 1,
    "BLINKER_INTERNAL_UNO_R4_TASKS": 1, "BLINKER_TASK_CAPACITY": 4,
    "BLINKER_TASK_WINDOW_MS": 60000, "BLINKER_TASK_WRITES": 10,
    "BLINKER_TASK_MIN_START_MS": 30000, "BLINKER_TASK_MAX_COMMIT_MS": 1000,
    "BLINKER_OFFICIAL_CLOUD_SESSION_TLS": 0, "BLINKER_OFFICIAL_MQTT_TLS": 0,
}
CREDENTIAL_FIELDS = ("device_key", "wifi_ssid", "wifi_password")
PRIVATE = {"private/credentials.json", "private/UserCredentials.h"}
SDK_PREFIX = "sketchbook/hardware/arduino/renesas_uno/"
LIB_PREFIX = "sketchbook/libraries/BlinkerDeviceV2/"
BUILTINS = {"ctags": "5.8-arduino11", "serial-discovery": "1.5.2",
            "mdns-discovery": "1.1.0", "dfu-discovery": "0.1.2",
            "serial-monitor": "0.15.0"}
CORE_CONTRACT_HEADERS = (
    "arduino/ports/uno_r4/UnoR4ModemRpcBoundary.h",
    "arduino/ports/uno_r4/UnoR4ModemOwner.h",
    "arduino/ports/uno_r4/UnoR4ModemResponseBounds.h",
    "core/Result.h",
)


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def digest(value):
    if isinstance(value, bytes):
        return hashlib.sha256(value).hexdigest()
    result = hashlib.sha256()
    with value.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def tree(directory):
    result = {}
    for file in sorted(directory.rglob("*")):
        if file.is_symlink():
            raise ValueError("build inputs must not contain symbolic links")
        if file.is_file():
            result[file.relative_to(directory).as_posix()] = file
    return result


def credentials(path=None):
    value = (json.loads(path.read_text(encoding="utf-8")) if path else
             dict(zip(CREDENTIAL_FIELDS, ("REPLACE_DEVICE_KEY", "REPLACE_SSID", "REPLACE_PASSWORD"))))
    if (not isinstance(value, dict) or set(value) != set(CREDENTIAL_FIELDS) or
            any(not isinstance(value[key], str) or "\0" in value[key] for key in CREDENTIAL_FIELDS)):
        raise ValueError("private credentials require exactly three NUL-free strings")
    if any(len(value[key].encode("utf-8")) > limit for key, limit in
           zip(CREDENTIAL_FIELDS, (256, 32, 63))):
        raise ValueError("private credential string exceeds the fixed graph limit")
    header = ["// Private generated input. Never publish this file.", "#pragma once"]
    for key, macro in zip(CREDENTIAL_FIELDS, ("R4_DEVICE_KEY", "R4_WIFI_SSID", "R4_WIFI_PASSWORD")):
        # UTF-8 octal escapes avoid JSON's invalid C++ surrogate escapes and
        # preserve Chinese SSIDs without execution-character-set dependence.
        encoded = ''.join('\\%03o' % byte for byte in value[key].encode("utf-8"))
        header.append(f'#define {macro} "{encoded}"')
    return {"private/credentials.json": json_bytes(value),
            "private/UserCredentials.h": ("\n".join(header) + "\n").encode()}


def library_inputs(directory, name, version):
    properties = dict(line.split("=", 1) for line in
                      (directory / "library.properties").read_text(encoding="utf-8").splitlines()
                      if "=" in line and not line.startswith("#"))
    if properties.get("name") != name or properties.get("version") != version:
        raise ValueError("unsupported dependency name/version: " + name)
    files = {"library.properties": directory / "library.properties"}
    files.update({"src/" + key: value for key, value in tree(directory / "src").items()})
    if len(files) == 1:
        raise ValueError("dependency source is missing: " + name)
    return files


def restricted_profile(path):
    profile = load_profile(path)
    if (profile.channel != "development" or profile.task_profile or
            profile.cloud_session_tls or profile.mqtt_tls or
            profile.cloud_session_ca_pem or profile.mqtt_ca_pem or
            not profile.allow_plain_storage or len(profile.ed25519_keys) != 1):
        raise ValueError("restricted R4 profile requires development, Plain storage/transport and one Ed25519 key")
    return profile


def configured_library(profile):
    return configured_library_files(ROOT, profile)


def fixed_config(library, configured_files):
    definitions = []
    for macro, value in FIXED.items():
        definitions += [f"#if defined({macro}) && {macro} != {value}",
                        '#error "Conflicting restricted R4 build configuration"', "#endif",
                        f"#ifndef {macro}", f"#define {macro} {value}", "#endif"]
    fixed = (TEMPLATES / "Config.h").read_bytes().replace(
        b"@FIXED_DEFINES@", "\n".join(definitions).encode())
    header = (library / LIBRARY_IDENTITY_HEADER).resolve().as_posix()
    if any(character in header for character in ('"', "\r", "\n")):
        raise ValueError("library identity include path contains unsafe characters")
    checks = ["!defined(BLINKER_LIBRARY_BUILD_IDENTITY_VERSION)",
              "BLINKER_LIBRARY_BUILD_IDENTITY_VERSION != 1"]
    for index, word in enumerate(library_identity_words(configured_files)):
        macro = f"BLINKER_LIBRARY_BUILD_IDENTITY_{index}"
        checks += [f"!defined({macro})", f"{macro} != 0x{word:08X}UL"]
    binding = [f'#include "{header}"', "#if " + " || ".join(checks),
               '#error "R4 maintenance profile/library mismatch; verify and reinstall the matching profile"',
               "#endif", "#define BLINKER_INTERNAL_R4_LIBRARY_BINDING 1", ""]
    return fixed + "\n".join(binding).encode()


def expected(sdk, pubsub, crypto, compiler, builtin_tools, profile_path, output):
    sdk, pubsub, crypto, compiler, builtin_tools, output = (path.resolve() for path in
        (sdk, pubsub, crypto, compiler, builtin_tools, output))
    if any(output.is_relative_to(path) or path.is_relative_to(output) for path in
           (sdk, pubsub, crypto, compiler, builtin_tools, ROOT / "src")):
        raise ValueError("stage must be disjoint from SDK, dependency and source trees")
    if not all((compiler / (name + ".exe")).is_file() or (compiler / name).is_file()
               for name in ("arm-none-eabi-gcc", "arm-none-eabi-g++", "arm-none-eabi-ar")):
        raise ValueError("explicit original ARM compiler bin directory is required")
    if compiler.name != "bin" or compiler.parent.name != "7-2017q4":
        raise ValueError("restricted stage requires the original ARM 7-2017q4 compiler")
    if "version=1.5.3" not in (sdk / "platform.txt").read_text(encoding="utf-8").splitlines():
        raise ValueError("restricted stage requires original Renesas Core 1.5.3")
    profile = restricted_profile(profile_path)
    base, base_receipt = candidate.expected_candidate(
        sdk / "libraries/WiFiS3/src", output / "wiring", pubsub / "src")
    files = {SDK_PREFIX + key: value for key, value in tree(sdk).items()}
    overlay_names = {"Modem.cpp", "Modem.h", "WiFiCommands.h", "StringHelpers.h", "Serial.cpp", "Serial.h"}
    if {key.split("/", 1)[1] for key in base if key.startswith("sdk-overlay/") and key != "sdk-overlay/overlay.json"} != overlay_names:
        raise ValueError("reviewed overlay must replace exactly six files")
    for name in overlay_names:
        directory = "cores/arduino/" if name.startswith("Serial.") else "libraries/WiFiS3/src/"
        files[SDK_PREFIX + directory + name] = base["sdk-overlay/" + name]
    # The original Core recipe has -I<core>, not library-discovery includes.
    # Mirror only the existing four contract headers into this isolated copy;
    # no new owner logic, full library projection, implementation or .cpp TU.
    for name in CORE_CONTRACT_HEADERS:
        files[SDK_PREFIX + "cores/arduino/BlinkerV2/" + name] = ROOT / "src/BlinkerV2" / name
    library_files = configured_library(profile)
    files.update({LIB_PREFIX + key: value for key, value in library_files.items()})
    for name in ("CandidateConfig.h", "TaskHeapBoundary.h", "HeapBoundary.cpp"):
        # A direct Sketch TU guarantees the wrapper is present before libc is
        # resolved; do not rely on Arduino's library-archive extraction order.
        files["sketch/R4Restricted/" + name] = base[name]
    files["wiring/Config.h"] = fixed_config(output / LIB_PREFIX, library_files)
    files["wiring/CandidateConfig.h"] = base["CandidateConfig.h"]
    # One reviewed public graph, not a second maintainer-only Sketch. Do not
    # copy the adjacent placeholder header: private/UserCredentials.h is the
    # only credential include in the generated stage.
    reference = ROOT / "examples/R4TasksQuickStart"
    files["sketch/R4Restricted/R4Restricted.ino"] = reference / "R4TasksQuickStart.ino"
    inputs = {"sdk/" + key: digest(value) for key, value in tree(sdk).items()}
    inputs.update({key: digest(value) for key, value in candidate.source_paths().items()})
    for directory, name, version in ((pubsub, "PubSubClient", "2.8"), (crypto, "Crypto", "0.4.0")):
        for key, value in library_inputs(directory, name, version).items():
            files["sketchbook/libraries/" + name + "/" + key] = value
            inputs["dependencies/" + name + "/" + key] = digest(value)
    inputs["public-profile"] = digest(profile_path)
    inputs["restricted-generator"] = digest(Path(__file__))
    for key, value in tree(TEMPLATES).items():
        inputs["templates/" + key] = digest(value)
    for name in ("R4TasksQuickStart.ino", "UserCredentials.h", "README.md"):
        inputs["examples/R4TasksQuickStart/" + name] = digest(reference / name)
    for name in ("arm-none-eabi-gcc", "arm-none-eabi-g++", "arm-none-eabi-ar"):
        executable = compiler / (name + ".exe")
        if not executable.is_file():
            executable = compiler / name
        inputs["compiler/" + name] = digest(executable)
    # Preserve the installed CLI's exact preprocessor/discovery tools; an empty
    # data directory must not silently fall back to an unrelated installation.
    for name, version in BUILTINS.items():
        selected = tree(builtin_tools / name / version)
        if not selected:
            raise ValueError("missing pinned builtin tool: " + name + "/" + version)
        for key, value in selected.items():
            relative = name + "/" + version + "/" + key
            files["cli-data/packages/builtin/tools/" + relative] = value
            inputs["builtin/" + relative] = digest(value)
    files["cli-data/package_index.json"] = json_bytes({"packages": []})
    files["cli-data/library_index.json"] = json_bytes({"libraries": []})
    # Isolate installed library selection; tool binaries remain explicit,
    # read-only references. Preparation never invokes the CLI or its recipes.
    files["cli.yaml"] = ("directories:\n" + "\n".join(
        "  " + key + ": " + json.dumps(str(output / value)) for key, value in
        (("user", "sketchbook"), ("data", "cli-data"), ("downloads", "cli-downloads"))) +
        "\nbuild_cache:\n  path: " + json.dumps(str(output / "cache")) + "\n").encode()
    files[".gitignore"] = b"/private/\n/compile/\n/cache/\n/cli-data/\n/cli-downloads/\n"
    quote = lambda path: '"' + str(path).replace('\\', '/') + '"'
    # Emit static frame files only; this adds no runtime observer or callbacks.
    # Let CLI discover/compile Blinker from the unique sketchbook library.
    # A pre-injected -I<src> bypasses discovery and leaves its .cpp unlinked.
    flags = ("-fstack-usage -include " + quote(output / "wiring/Config.h") +
             " -I " + quote(output / "private"))
    argv = ["compile", "--config-file", str(output / "cli.yaml"), "--fqbn",
            "arduino:renesas_uno:unor4wifi", "--build-path", str(output / "compile"),
            "--jobs", "1", "--clean"]
    for key, value in (("build.compiler_path", str(compiler).replace('\\', '/') + "/"),
                       ("compiler.c.extra_flags", flags), ("compiler.cpp.extra_flags", flags),
                       # The original assembly recipe omits build.defines.
                       ("compiler.S.extra_flags", "-DNO_USB " + flags),
                       ("compiler.c.elf.extra_flags", " ".join(base_receipt["linkFlags"]))):
        argv += ["--build-property", key + "=" + value]
    argv += [str(output / "sketch/R4Restricted")]
    receipt = {
        "schema": "r4-restricted-release/1", "state": "prepared-not-linked",
        "target": "arduino:renesas_uno:unor4wifi", "core": "1.5.3",
        "linked": False, "formalTaskAdmission": False, "hardwareWritten": False,
        "ordinaryZipModified": False, "installedSdkModified": False,
        "compilerDirectory": str(compiler), "sdkInputDirectory": str(sdk),
        "builtinVersions": BUILTINS,
        "coreContractHeaders": list(CORE_CONTRACT_HEADERS),
        "sdkSelectedDirectory": str(output / SDK_PREFIX), "cliArguments": argv,
        "fixedGraph": FIXED, "inputSha256": inputs,
        "outputSha256": {key: digest(value) for key, value in files.items()},
        "privateInputs": "Three credential strings are excluded from this public receipt; verify with the same private JSON.",
        "requiredPostLink": [
            "CLI selected this stage hardware, all C/C++ consumers and PubSub saw Config.h; installed Core was not selected.",
            "Exactly one Modem/Serial/heap wrapper/owner; _sbrk_r redirects to wrapper then original _sbrk.",
            "Heap cap equals actual StackTop minus 6144; inspect actual HeapBase, static RAM and retained frames.",
            "No observer/fault gate/user callbacks; only the fixed power/duty/uptime task/control/time/telemetry graph.",
            "Gateway selects matching Ed25519 keyId/public key and only the explicitly admitted device scope.",
        ],
        "support": [
            "Non-safety-critical, cooperative manual WiFi, four tasks; no arbitrary Sketch, TLS, BLE, LAN or scan.",
            "Fault latches service closed; manual whole-board cold recovery and fresh read-only reconciliation, never UNKNOWN replay.",
            "Original SDK new/string OOM remains fatal; no ACK, RAII cleanup or automatic recovery is promised.",
        ],
    }
    return files, receipt, profile


def prepare(sdk, pubsub, crypto, compiler, builtin_tools, profile, output, credential_file=None):
    files, receipt, _ = expected(sdk, pubsub, crypto, compiler, builtin_tools, profile, output)
    files.update(credentials(credential_file))
    output.mkdir(parents=True, exist_ok=False)
    for key, value in files.items():
        target = output / key
        target.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(value, bytes):
            with target.open("xb") as stream:
                stream.write(value)
        else:
            shutil.copyfile(value, target)
    (output / "release-plan.json").write_bytes(json_bytes(receipt))
    return receipt


def verify(sdk, pubsub, crypto, compiler, builtin_tools, profile, output, credential_file=None):
    files, expected_receipt, _ = expected(sdk, pubsub, crypto, compiler, builtin_tools, profile, output)
    files.update(credentials(credential_file))
    actual = set(tree(output))
    # Compiler outputs are outside the prepared source receipt, never treated as
    # linked proof. All other extra/missing files or input mutations fail closed.
    actual = {key for key in actual if key.split("/", 1)[0] not in
              ("compile", "cache", "cli-downloads") and key != "cli-data/inventory.yaml"}
    if actual != set(files) | {"release-plan.json"}:
        raise ValueError("restricted stage has missing or unlisted source files")
    for key, value in files.items():
        if digest(output / key) != digest(value):
            raise ValueError("restricted stage input mismatch: " + ("private credentials" if key in PRIVATE else key))
    if json.loads((output / "release-plan.json").read_text(encoding="utf-8")) != expected_receipt:
        raise ValueError("restricted stage receipt/source mismatch")
    return expected_receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("sdk", "pubsub", "crypto", "compiler", "builtin-tools", "profile", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--credentials", type=Path, help="private JSON with exactly device_key, wifi_ssid, wifi_password")
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    operation = verify if args.verify_only else prepare
    result = operation(args.sdk, args.pubsub, args.crypto, args.compiler, args.builtin_tools,
                       args.profile, args.output, args.credentials)
    print(json.dumps({key: result[key] for key in ("schema", "state", "linked", "formalTaskAdmission", "hardwareWritten")}))


if __name__ == "__main__":
    main()
