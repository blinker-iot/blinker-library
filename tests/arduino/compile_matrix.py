#!/usr/bin/env python3
"""Fail-fast Arduino compile matrix for the Device V2 package boundary."""

from __future__ import annotations

import argparse
from dataclasses import dataclass, replace
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


REPOSITORY = Path(__file__).resolve().parents[2]
ARDUINO_PORT = "src/BlinkerV2/arduino/ports/arduino/"
ESP32_PORT = "src/BlinkerV2/arduino/ports/esp32/"
UNO_R4_PORT = "src/BlinkerV2/arduino/ports/uno_r4/"

PORT_SOURCES = {
    "arduino-http": (ARDUINO_PORT + "ArduinoClientHttp.h",),
    "hardware-random": (
        ARDUINO_PORT + "HardwareRandom.h",
    ),
    "pubsub-mqtt": (ARDUINO_PORT + "PubSubMqtt.h",),
    "arduino-ble": (
        ARDUINO_PORT + "ArduinoBleLink.h",
        ARDUINO_PORT + "ArduinoBleLink.ipp",
    ),
    "portable-crypto": (
        ARDUINO_PORT + "PortableCrypto.h",
        ARDUINO_PORT + "PortableCrypto.ipp",
    ),
    "esp32-crypto": (
        ESP32_PORT + "Esp32Crypto.h",
    ),
    "esp32-storage": (
        ESP32_PORT + "Esp32Storage.h",
    ),
    "esp32-wifi": (
        ESP32_PORT + "Esp32WiFi.h",
        ESP32_PORT + "Esp32WiFi.ipp",
    ),
    "uno-r4-storage": (
        UNO_R4_PORT + "UnoR4Storage.h",
    ),
    "uno-r4-wifi": (
        UNO_R4_PORT + "UnoR4WiFi.h",
        UNO_R4_PORT + "UnoR4WiFi.ipp",
    ),
}

PORT_DEPENDENCIES = {
    identifier: () for identifier in PORT_SOURCES
}
PORT_DEPENDENCIES.update({
    "pubsub-mqtt": ("PubSubClient (>=2.8)",),
    "arduino-ble": ("ArduinoBLE (=2.0.2)",),
    "portable-crypto": ("Crypto (=0.4.0)",),
})


@dataclass(frozen=True)
class Target:
    name: str
    fqbn: str
    core: str
    core_version: str
    sketch: str
    libraries: tuple[str, ...]
    external_libraries: tuple[tuple[str, str], ...] = ()
    small_profile: bool = True
    definitions: tuple[str, ...] = ()
    build_properties: tuple[str, ...] = ()


CORE_LIBRARIES = (".",)
HTTP_LIBRARIES = (".", "port:arduino-http")
MQTT_LIBRARIES = (".", "port:pubsub-mqtt")
HARDWARE_RANDOM_LIBRARIES = (".", "port:hardware-random")
DEVICE_KEY_SESSION_LIBRARIES = (
    ".",
    "port:arduino-http",
    "port:hardware-random",
    "port:pubsub-mqtt",
)
ESP32_STORAGE_LIBRARIES = (".", "port:esp32-storage")
ESP32_WIFIPROV_LIBRARIES = (".",)
ESP32_CRYPTO_LIBRARIES = (
    ".",
    "port:hardware-random",
    "port:esp32-crypto",
)
ARDUINO_CRYPTO_LIBRARIES = (".", "port:portable-crypto")
ESP32_WIFI_LIBRARIES = (".", "port:esp32-wifi")
RENESAS_UNO_WIFI_LIBRARIES = (".", "port:uno-r4-wifi")
RENESAS_UNO_STORAGE_LIBRARIES = (".", "port:uno-r4-storage")
TARGETS = {
    target.name: target
    for target in (
        Target(
            "esp32-core",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompileV2",
            CORE_LIBRARIES,
        ),
        Target(
            "uno-r4-core",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/CompileV2",
            CORE_LIBRARIES,
        ),
        Target(
            "esp8266-core",
            "esp8266:esp8266:generic",
            "esp8266:esp8266",
            "3.1.2",
            "tests/arduino/CompileV2",
            CORE_LIBRARIES,
        ),
        Target(
            "wio-terminal-core",
            "Seeeduino:samd:seeed_wio_terminal",
            "Seeeduino:samd",
            "1.8.5",
            "tests/arduino/CompileV2",
            CORE_LIBRARIES,
        ),
        Target(
            "rp2040-core",
            "rp2040:rp2040:rpipico",
            "rp2040:rp2040",
            "5.1.0",
            "tests/arduino/CompileV2",
            CORE_LIBRARIES,
        ),
        Target(
            "nrf52840-core",
            "adafruit:nrf52:feather52840",
            "adafruit:nrf52",
            "1.7.0",
            "tests/arduino/CompileV2",
            CORE_LIBRARIES,
        ),
        Target(
            "uno-r4-size-probe",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/SizeProbe",
            CORE_LIBRARIES,
        ),
        Target(
            "uno-r4-size-probe-standard",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/SizeProbe",
            CORE_LIBRARIES,
            small_profile=False,
            definitions=("BLINKER_RESOURCE_PROFILE=2",),
        ),
        Target(
            "esp32-size-probe",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/SizeProbe",
            CORE_LIBRARIES,
        ),
        Target(
            "wio-terminal-size-probe",
            "Seeeduino:samd:seeed_wio_terminal",
            "Seeeduino:samd",
            "1.8.5",
            "tests/arduino/SizeProbe",
            CORE_LIBRARIES,
        ),
        # Keep the 32-bit ABI check without implying that UNO R4's 32 KB
        # SRAM can deploy the Large profile or link every probe object.
        Target(
            "esp32-size-probe-large",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/SizeProbe",
            CORE_LIBRARIES,
            small_profile=False,
            definitions=("BLINKER_RESOURCE_PROFILE=3",),
        ),
        Target(
            "esp32-random",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompileHardwareRandomAdapter",
            HARDWARE_RANDOM_LIBRARIES,
        ),
        Target(
            "uno-r4-random",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/CompileHardwareRandomAdapter",
            HARDWARE_RANDOM_LIBRARIES,
        ),
        Target(
            "esp32-wifi",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompileWifiStationAdapter",
            ESP32_WIFI_LIBRARIES,
        ),
        Target(
            "uno-r4-wifi",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/CompileWifiStationAdapter",
            RENESAS_UNO_WIFI_LIBRARIES,
        ),
        Target(
            "uno-r4-storage",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/CompileRenesasStorageAdapter",
            RENESAS_UNO_STORAGE_LIBRARIES,
        ),
        Target(
            "esp32-storage",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompileEsp32StorageAdapter",
            ESP32_STORAGE_LIBRARIES,
        ),
        Target(
            "esp32-wifiprov",
            "esp32:esp32:esp32:PartitionScheme=min_spiffs",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompileEsp32WifiProvAdapter",
            ESP32_WIFIPROV_LIBRARIES,
        ),
        Target(
            "esp32-crypto",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompileEsp32CryptoAdapter",
            ESP32_CRYPTO_LIBRARIES,
        ),
        Target(
            "esp32-http",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompileHttpAdapter",
            HTTP_LIBRARIES,
        ),
        Target(
            "uno-r4-http",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/CompileHttpAdapter",
            HTTP_LIBRARIES,
        ),
        Target(
            "esp8266-http",
            "esp8266:esp8266:generic",
            "esp8266:esp8266",
            "3.1.2",
            "tests/arduino/CompileHttpAdapter",
            HTTP_LIBRARIES,
        ),
        Target(
            "esp32-mqtt",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompilePubSubAdapter",
            MQTT_LIBRARIES,
            (("PubSubClient", "2.8"),),
        ),
        Target(
            "esp32-mqtt-tls",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompilePubSubTlsAdapter",
            MQTT_LIBRARIES,
            (("PubSubClient", "2.8"),),
        ),
        Target(
            "uno-r4-mqtt",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/CompilePubSubAdapter",
            MQTT_LIBRARIES,
            (("PubSubClient", "2.8"),),
        ),
        Target(
            "uno-r4-mqtt-tls",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/CompilePubSubTlsAdapter",
            MQTT_LIBRARIES,
            (("PubSubClient", "2.8"),),
        ),
        Target(
            "esp8266-mqtt",
            "esp8266:esp8266:generic",
            "esp8266:esp8266",
            "3.1.2",
            "tests/arduino/CompilePubSubAdapter",
            MQTT_LIBRARIES,
            (("PubSubClient", "2.8"),),
        ),
        Target(
            "esp8266-mqtt-tls",
            "esp8266:esp8266:generic",
            "esp8266:esp8266",
            "3.1.2",
            "tests/arduino/CompilePubSubTlsAdapter",
            MQTT_LIBRARIES,
            (("PubSubClient", "2.8"),),
        ),
        Target(
            "esp32-device-key-session",
            "esp32:esp32:esp32",
            "esp32:esp32",
            "3.3.6",
            "tests/arduino/CompileDeviceKeySession",
            DEVICE_KEY_SESSION_LIBRARIES,
            (("PubSubClient", "2.8"),),
        ),
        Target(
            "uno-r4-device-key-session",
            "arduino:renesas_uno:unor4wifi",
            "arduino:renesas_uno",
            "1.5.3",
            "tests/arduino/CompileDeviceKeySession",
            DEVICE_KEY_SESSION_LIBRARIES,
            (("PubSubClient", "2.8"),),
        ),
    )
}

# Public examples are separate targets instead of being hidden behind the
# comprehensive CompileV2 probe. This lets release CI prove that the code a
# user or coding agent will copy still compiles on every claimed Core target.
CORE_TARGET_NAMES = (
    "esp32-core",
    "uno-r4-core",
    "esp8266-core",
    "wio-terminal-core",
    "rp2040-core",
    "nrf52840-core",
)

FACADE_TARGET_NAMES = []
FACADE_PROFILES = {
    "esp32": (("wifi", 1), ("ble", 2)),
    "uno-r4": (("wifi", 1), ("ble", 2)),
    "wio-terminal": (("wifi", 1),),
}
for facade_name, facade_profiles in FACADE_PROFILES.items():
    core_target = TARGETS[facade_name + "-core"]
    for profile_name, profile_value in facade_profiles:
        target_name = facade_name + "-facade-" + profile_name
        FACADE_TARGET_NAMES.append(target_name)
        TARGETS[target_name] = Target(
            target_name,
            core_target.fqbn,
            core_target.core,
            core_target.core_version,
            "tests/arduino/CompileFacade",
            core_target.libraries,
            core_target.external_libraries,
            core_target.small_profile,
            core_target.definitions + (
                f"BLINKER_FACADE_PROFILE={profile_value}",
            ),
        )

DIAGNOSTIC_TARGET_NAMES = []
for facade_name in FACADE_PROFILES:
    core_target = TARGETS[facade_name + "-core"]
    for level_name, level_value in (("info", 1), ("debug", 2)):
        target_name = facade_name + "-diagnostics-" + level_name
        DIAGNOSTIC_TARGET_NAMES.append(target_name)
        TARGETS[target_name] = Target(
            target_name,
            core_target.fqbn,
            core_target.core,
            core_target.core_version,
            "tests/arduino/CompileFacade",
            core_target.libraries,
            core_target.external_libraries,
            core_target.small_profile,
            core_target.definitions + (
                "BLINKER_FACADE_PROFILE=1",
                f"BLINKER_FACADE_DIAGNOSTICS={level_value}",
            ),
        )

PUBLIC_WIFI_DEPENDENCIES = {
    "esp32": (("PubSubClient", "2.8"),),
    "uno-r4": (("PubSubClient", "2.8"),),
    "wio-terminal": (
        ("PubSubClient", "2.8"),
        ("Seeed Arduino rpcWiFi", "1.1.0"),
        ("Seeed Arduino rpcUnified", "2.1.4"),
    ),
}
PUBLIC_WIFI_TARGET_NAMES = []
for facade_name in FACADE_PROFILES:
    core_target = TARGETS[facade_name + "-core"]
    target_name = facade_name + "-product-wifi"
    PUBLIC_WIFI_TARGET_NAMES.append(target_name)
    TARGETS[target_name] = Target(
        target_name,
        core_target.fqbn,
        core_target.core,
        core_target.core_version,
        "tests/arduino/CompilePublicWiFi",
        core_target.libraries,
        PUBLIC_WIFI_DEPENDENCIES[facade_name],
        core_target.small_profile,
        core_target.definitions,
    )
    quickstart_name = facade_name + "-wifi-example"
    TARGETS[quickstart_name] = Target(
        quickstart_name,
        core_target.fqbn,
        core_target.core,
        core_target.core_version,
        "examples/WiFiQuickStart",
        core_target.libraries,
        PUBLIC_WIFI_DEPENDENCIES[facade_name],
        core_target.small_profile,
        core_target.definitions,
    )
    smoke_name = facade_name + "-cloud-smoke"
    TARGETS[smoke_name] = Target(
        smoke_name,
        core_target.fqbn,
        core_target.core,
        core_target.core_version,
        "examples/CloudSmokeTest",
        core_target.libraries,
        PUBLIC_WIFI_DEPENDENCIES[facade_name],
        core_target.small_profile,
        core_target.definitions,
    )

PUBLIC_BLE_DEPENDENCIES = {
    "esp32": (),
    "uno-r4": (
        ("ArduinoBLE", "2.0.2"),
        ("Crypto", "0.4.0"),
    ),
}
PUBLIC_BLE_TARGET_NAMES = []
for facade_name in PUBLIC_BLE_DEPENDENCIES:
    core_target = TARGETS[facade_name + "-core"]
    target_name = facade_name + "-product-ble"
    PUBLIC_BLE_TARGET_NAMES.append(target_name)
    TARGETS[target_name] = Target(
        target_name,
        (
            "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB"
            if facade_name == "esp32"
            else core_target.fqbn
        ),
        core_target.core,
        core_target.core_version,
        "examples/BleQuickStart",
        core_target.libraries,
        PUBLIC_BLE_DEPENDENCIES[facade_name],
        core_target.small_profile,
        core_target.definitions,
    )

TARGETS["esp32-product-wifi-ble"] = Target(
    "esp32-product-wifi-ble",
    "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB",
    TARGETS["esp32-core"].core,
    TARGETS["esp32-core"].core_version,
    "examples/WiFiBleQuickStart",
    TARGETS["esp32-core"].libraries,
    (("PubSubClient", "2.8"),),
)
TARGETS["esp32-edge-hub-internal"] = Target(
    "esp32-edge-hub-internal",
    "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB",
    TARGETS["esp32-core"].core,
    TARGETS["esp32-core"].core_version,
    "tests/arduino/CompileEsp32EdgeHubInternal",
    TARGETS["esp32-core"].libraries,
    (("PubSubClient", "2.8"),),
)
TARGETS["esp32-edge-hub-security-preflight"] = Target(
    "esp32-edge-hub-security-preflight",
    "esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB",
    TARGETS["esp32-core"].core,
    TARGETS["esp32-core"].core_version,
    "tests/internal/arduino/Esp32EdgeHubSecurityPreflight",
    TARGETS["esp32-core"].libraries,
)
TARGETS["wio-terminal-preview-ble"] = Target(
    "wio-terminal-preview-ble",
    TARGETS["wio-terminal-core"].fqbn,
    TARGETS["wio-terminal-core"].core,
    TARGETS["wio-terminal-core"].core_version,
    "examples/BleQuickStart",
    TARGETS["wio-terminal-core"].libraries,
    (
        ("Crypto", "0.4.0"),
        ("Seeed Arduino rpcBLE", "1.0.0"),
        ("Seeed Arduino rpcUnified", "2.1.4"),
    ),
    definitions=("BLINKER_ENABLE_WIO_BLE_PREVIEW=1",),
)
TARGETS["wio-terminal-preview-release-gate"] = Target(
    "wio-terminal-preview-release-gate",
    TARGETS["wio-terminal-core"].fqbn,
    TARGETS["wio-terminal-core"].core,
    TARGETS["wio-terminal-core"].core_version,
    "tests/arduino/WioBleReleaseGate",
    TARGETS["wio-terminal-core"].libraries,
    (
        ("Crypto", "0.4.0"),
        ("Seeed Arduino rpcBLE", "1.0.0"),
        ("Seeed Arduino rpcUnified", "2.1.4"),
    ),
)

for core_target_name in CORE_TARGET_NAMES:
    core_target = TARGETS[core_target_name]
    crypto_name = core_target_name + "-crypto"
    TARGETS[crypto_name] = Target(
        crypto_name,
        core_target.fqbn,
        core_target.core,
        core_target.core_version,
        "tests/arduino/CompileArduinoCryptoAdapter",
        ARDUINO_CRYPTO_LIBRARIES,
        (("Crypto", "0.4.0"),),
        core_target.small_profile,
        core_target.definitions,
    )

# Same product sources and dependencies; the opt-in control carrier must not
# turn into a separate library/package or pull gateway code into plain WiFi.
LOCAL_CONTROL_TARGET_NAMES = []
for base_name in (*PUBLIC_WIFI_TARGET_NAMES, "esp32-product-wifi-ble", "esp32-edge-hub-internal"):
    base = TARGETS[base_name]
    name = base_name + "-local-control"
    LOCAL_CONTROL_TARGET_NAMES.append(name)
    TARGETS[name] = replace(base, name=name,
        definitions=base.definitions + ("BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL=1",))

GROUPS = {
    "local-control": tuple(LOCAL_CONTROL_TARGET_NAMES),
    "core": CORE_TARGET_NAMES,
    "facade": tuple(FACADE_TARGET_NAMES),
    "diagnostics": tuple(DIAGNOSTIC_TARGET_NAMES),
    "products": tuple(
        PUBLIC_WIFI_TARGET_NAMES
        + PUBLIC_BLE_TARGET_NAMES
        + [
            "esp32-product-wifi-ble",
            "esp32-edge-hub-internal",
        ]
    ),
    "examples": tuple(
        [name + "-wifi-example" for name in FACADE_PROFILES]
        + [name + "-cloud-smoke" for name in FACADE_PROFILES]
        + PUBLIC_BLE_TARGET_NAMES
        + ["esp32-product-wifi-ble"]
    ),
    "portable-crypto": tuple(name + "-crypto" for name in CORE_TARGET_NAMES),
    "previews": (
        "wio-terminal-preview-ble",
        "wio-terminal-preview-release-gate",
    ),
    "size-probes": (
        "esp32-size-probe",
        "uno-r4-size-probe",
        "wio-terminal-size-probe",
    ),
    "cloud": (
        "esp32-random",
        "uno-r4-random",
        "esp32-wifi",
        "esp32-storage",
        "esp32-crypto",
        "esp32-http",
        "esp32-mqtt",
        "esp32-mqtt-tls",
        "uno-r4-http",
        "uno-r4-wifi",
        "uno-r4-storage",
        "uno-r4-mqtt",
        "uno-r4-mqtt-tls",
        "esp8266-http",
        "esp8266-mqtt",
        "esp8266-mqtt-tls",
        "esp32-device-key-session",
        "uno-r4-device-key-session",
    ),
}


def executable(explicit: str | None) -> str:
    if explicit:
        path = Path(explicit)
        if path.is_file():
            return str(path)
        raise SystemExit(f"arduino-cli executable not found: {path}")
    discovered = shutil.which("arduino-cli")
    if not discovered:
        raise SystemExit("arduino-cli was not found; pass --arduino-cli")
    return discovered


def run_listing(cli: str, arguments: list[str]) -> str:
    completed = subprocess.run(
        [cli, *arguments],
        cwd=REPOSITORY,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    # Some older/local CLI installations return non-zero for unrelated,
    # malformed board-package directories while still returning a usable
    # listing. Required entries below remain the release gate.
    return completed.stdout


def require_toolchain(cli: str, target: Target, core_listing: str) -> None:
    pattern = re.compile(
        rf"^{re.escape(target.core)}\s+{re.escape(target.core_version)}(?:\s|$)",
        flags=re.MULTILINE,
    )
    if not pattern.search(core_listing):
        raise SystemExit(
            f"{target.name} requires {target.core}@{target.core_version}"
        )


def require_libraries(
    target: Target,
    library_listing: str,
) -> None:
    for name, version in target.external_libraries:
        pattern = re.compile(
            rf"^{re.escape(name)}\s+{re.escape(version)}(?:\s|$)",
            flags=re.MULTILINE,
        )
        if not pattern.search(library_listing):
            raise SystemExit(f"{target.name} requires {name}@{version}")


def compile_target(cli: str, target: Target, build_directory: Path) -> None:
    sketch = REPOSITORY / target.sketch
    if not sketch.is_dir():
        raise SystemExit(f"sketch directory is missing: {sketch}")
    build_directory.mkdir(parents=True, exist_ok=True)
    command = [
        cli,
        "compile",
        "--fqbn",
        target.fqbn,
        "--clean",
        "--jobs",
        "4",
        "--warnings",
        "all",
        "--build-path",
        str(build_directory),
    ]
    port_ids = [
        relative.removeprefix("port:")
        for relative in target.libraries
        if relative.startswith("port:")
    ]
    for relative in target.libraries:
        if relative.startswith("port:"):
            continue
        library = REPOSITORY / relative
        if not (library / "library.properties").is_file():
            raise SystemExit(f"Arduino library root is invalid: {library}")
        command.extend(("--library", str(library)))
    port_directory: tempfile.TemporaryDirectory[str] | None = None
    if port_ids:
        unknown = set(port_ids) - PORT_SOURCES.keys()
        if unknown:
            raise SystemExit("unknown test ports: " + ", ".join(sorted(unknown)))
        port_directory = tempfile.TemporaryDirectory(
            prefix=f"blinker-{target.name}-ports-",
            dir=build_directory.parent,
        )
        port_library = Path(port_directory.name)
        port_source = port_library / "src"
        port_source.mkdir()
        copied: set[str] = set()
        dependencies = ["BlinkerDeviceV2"]
        for port_id in port_ids:
            for dependency in PORT_DEPENDENCIES[port_id]:
                if dependency not in dependencies:
                    dependencies.append(dependency)
            for relative in PORT_SOURCES[port_id]:
                source = REPOSITORY / relative
                if source.name in copied:
                    raise SystemExit(f"test port filename collision: {source.name}")
                copied.add(source.name)
                shutil.copyfile(source, port_source / source.name)
        (port_library / "library.properties").write_text(
            "name=BlinkerPortProbe\n"
            "version=0.0.0\n"
            "author=Blinker\n"
            "maintainer=Blinker\n"
            "sentence=Generated Device V2 port compile probe.\n"
            "paragraph=Generated only for repository compile tests.\n"
            "category=Communication\n"
            "url=https://diandeng.tech/\n"
            "architectures=*\n"
            "depends=" + ",".join(dependencies) + "\n",
            encoding="utf-8",
            newline="\n",
        )
        command.extend(("--library", str(port_library)))
    for build_property in target.build_properties:
        command.extend(("--build-property", build_property))
    definitions = list(target.definitions)
    if target.small_profile:
        definitions.append("BLINKER_RESOURCE_PROFILE=1")
    if definitions:
        command.extend(
            (
                "--build-property",
                "compiler.cpp.extra_flags="
                + " ".join(f"-D{definition}" for definition in definitions),
            )
        )
    command.append(str(sketch))

    print("+", subprocess.list2cmdline(command), flush=True)
    try:
        completed = subprocess.run(
            command,
            cwd=REPOSITORY,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            check=False,
        )
    finally:
        if port_directory is not None:
            port_directory.cleanup()
    output = completed.stdout
    (build_directory / "compile.log").write_text(output, encoding="utf-8")
    print(output, end="" if output.endswith("\n") else "\n")
    if completed.returncode != 0:
        raise SystemExit(f"Arduino compile failed: {target.name}")
    size_markers = (
        "Sketch uses",
        ". Code in flash",
        "Program:",
    )
    if not any(marker in output for marker in size_markers):
        raise SystemExit(f"Arduino size summary is missing: {target.name}")
    print(f"arduino matrix target passed: {target.name}", flush=True)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--arduino-cli")
    parser.add_argument("--target", action="append", choices=sorted(TARGETS))
    parser.add_argument("--group", action="append", choices=sorted(GROUPS))
    parser.add_argument("--build-root", type=Path)
    parser.add_argument("--list", action="store_true")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.list:
        for name in sorted(TARGETS):
            print(name)
        return 0

    names: list[str] = []
    for name in arguments.target or ():
        if name not in names:
            names.append(name)
    for group in arguments.group or ():
        for name in GROUPS[group]:
            if name not in names:
                names.append(name)
    if not names:
        raise SystemExit("select at least one --target or --group")

    cli = executable(arguments.arduino_cli)
    core_listing = run_listing(cli, ["core", "list"])
    library_listing = run_listing(cli, ["lib", "list"])
    for name in names:
        target = TARGETS[name]
        require_toolchain(cli, target, core_listing)
        require_libraries(target, library_listing)

    temporary: tempfile.TemporaryDirectory[str] | None = None
    if arguments.build_root:
        build_root = arguments.build_root.resolve()
        build_root.mkdir(parents=True, exist_ok=True)
    else:
        temporary = tempfile.TemporaryDirectory(prefix="blinker-arduino-matrix-")
        build_root = Path(temporary.name)

    try:
        for name in names:
            compile_target(cli, TARGETS[name], build_root / name)
    finally:
        if temporary is not None:
            temporary.cleanup()

    print("Arduino Device V2 matrix passed: " + ", ".join(names))
    return 0


if __name__ == "__main__":
    sys.exit(main())
