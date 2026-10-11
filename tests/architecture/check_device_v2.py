from __future__ import annotations

import hashlib
import re
import sys
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY / "tools"))
from build_arduino_release import EXPECTED_IMPLEMENTATION_UNITS

SOURCE_ROOT = REPOSITORY / "src"
PUBLIC_HEADER = SOURCE_ROOT / "Blinker.h"
WIFI_HEADER = SOURCE_ROOT / "BlinkerWiFi.h"
BLE_HEADER = SOURCE_ROOT / "BlinkerBLE.h"
WIFI_BLE_HEADER = SOURCE_ROOT / "BlinkerWiFiBLE.h"
ADVANCED_HEADER = SOURCE_ROOT / "BlinkerV2Advanced.h"
DIAGNOSTICS_HEADER = SOURCE_ROOT / "BlinkerV2" / "core" / "Diagnostics.h"
DIAGNOSTICS_SOURCE = SOURCE_ROOT / "BlinkerV2" / "core" / "Diagnostics.inc"
ARDUINO_DIAGNOSTICS = (
    SOURCE_ROOT / "BlinkerV2" / "arduino" / "ArduinoDiagnostics.inc"
)
RETIRED_LOGGER = SOURCE_ROOT / "BlinkerV2" / "interface" / "ILogger.h"
OUTBOUND_SCHEDULER_HEADER = (
    SOURCE_ROOT / "BlinkerV2" / "runtime" / "OutboundScheduler.h"
)
OUTBOUND_SCHEDULER_SOURCE = OUTBOUND_SCHEDULER_HEADER.with_suffix(".inc")
CLIENT_SOURCE = SOURCE_ROOT / "BlinkerV2" / "api" / "Client.inc"
DEVICE_RUNTIME_SOURCE = (
    SOURCE_ROOT / "BlinkerV2" / "runtime" / "DeviceRuntime.inc"
)
ARDUINO_ROOT = SOURCE_ROOT / "BlinkerV2" / "arduino"
PORT_ROOT = ARDUINO_ROOT / "ports"
ARDUINO_BLE_PORT = PORT_ROOT / "arduino" / "ArduinoBleLink.ipp"
ARDUINO_BLE_HEADER = ARDUINO_BLE_PORT.with_suffix(".h")
BLE_CONNECTION_SECURITY_SOURCE = (
    SOURCE_ROOT
    / "BlinkerV2"
    / "interface"
    / "IBleConnectionSecuritySource.h"
)
UNO_R4_BLE_PLATFORM = (
    ARDUINO_ROOT / "platform" / "uno_r4" / "UnoR4BLEPlatform.h"
)
ESP32_NIMBLE_HEADER = PORT_ROOT / "esp32" / "Esp32NimBle.h"
ESP32_NIMBLE_IMPLEMENTATION = PORT_ROOT / "esp32" / "Esp32NimBle.ipp"
ESP32_WIFIPROV = PORT_ROOT / "esp32"
ESP32_WIFIPROV_HEADER = ESP32_WIFIPROV / "Esp32WiFiProv.h"
ESP32_WIFIPROV_IMPLEMENTATION = ESP32_WIFIPROV / "Esp32WiFiProv.ipp"
RETIRED_ESP32_WIFIPROV_SOURCES = tuple(
    ESP32_WIFIPROV / name
    for name in (
        "Esp32WiFiProv.cpp",
        "Esp32BleWiFiProv.cpp",
        "Esp32SoftApWiFiProv.cpp",
    )
)
BLE_RECORD_TRANSPORT = (
    SOURCE_ROOT / "BlinkerV2" / "transport" / "BleRecordTransport.inc"
)
BLE_FRAME_TRANSPORT_HEADER = (
    SOURCE_ROOT / "BlinkerV2" / "transport" / "BleFrameTransport.h"
)
BLE_FRAME_TRANSPORT_SOURCE = BLE_FRAME_TRANSPORT_HEADER.with_suffix(".inc")
BLE_LINK_INTERFACE = (
    SOURCE_ROOT / "BlinkerV2" / "interface" / "IBleLink.h"
)
BLE_NOISE_CHANNEL = (
    SOURCE_ROOT
    / "BlinkerV2"
    / "provisioning"
    / "BleNoiseProvisioningChannel.inc"
)
WIO_RPC_BLE_HEADER = (
    PORT_ROOT / "wio_terminal" / "WioTerminalRpcBle.h"
)
WIO_RPC_BLE_IMPLEMENTATION = WIO_RPC_BLE_HEADER.with_suffix(".ipp")
BLE_MODE_COORDINATOR = (
    SOURCE_ROOT / "BlinkerV2" / "transport" / "BleModeCoordinator.inc"
)
BLE_SETUP_LIFECYCLE = (
    SOURCE_ROOT / "BlinkerV2" / "runtime" / "BleSetupLifecycle.h"
)
BLE_DIRECT_PROFILE_HEADER = (
    SOURCE_ROOT / "BlinkerV2" / "runtime" / "BleDirectProfile.h"
)
BLE_DIRECT_PROFILE_SOURCE = BLE_DIRECT_PROFILE_HEADER.with_suffix(".inc")
BLE_MODE_PROFILE = SOURCE_ROOT / "BlinkerV2" / "protocol" / "ble" / "Mode.inc"
IMPLEMENTATION_ROOT = SOURCE_ROOT / "BlinkerV2" / "implementation"
RETIRED_LOCAL_PAIRING = tuple(
    SOURCE_ROOT / "BlinkerV2" / "provisioning" / f"{name}{suffix}"
    for name in (
        "LocalControllerPairingApplication",
        "LocalControllerPairingApplicationContract",
        "LocalControllerPairingContract",
        "LocalControllerPairingCoordinator",
    )
    for suffix in (".h", ".cpp")
)
RETIRED_NONPRODUCT_SOURCES = tuple(
    SOURCE_ROOT / relative
    for relative in (
        "BlinkerV2/interface/IKeyValueStore.h",
        "BlinkerV2/interface/INetworkCredentialStore.h",
        "BlinkerV2/interface/IP256CryptoProvider.h",
        "BlinkerV2/manufacturing/FactoryCredentialBundle.h",
        "BlinkerV2/manufacturing/FactoryCredentialBundle.cpp",
        "BlinkerV2/provisioning/OwnershipReleaseCleanup.h",
        "BlinkerV2/provisioning/OwnershipReleaseCleanup.cpp",
        "BlinkerV2/provisioning/OwnershipReleaseCoordinator.h",
        "BlinkerV2/provisioning/OwnershipReleaseCoordinator.cpp",
        "BlinkerV2/provisioning/OwnershipReleaseVerifier.h",
        "BlinkerV2/provisioning/OwnershipReleaseVerifier.cpp",
        "BlinkerV2/interface/IOwnershipReleaseRecordStore.h",
        "BlinkerV2/provisioning/OwnershipReleaseContract.h",
        "BlinkerV2/provisioning/OwnershipReleaseContract.cpp",
        "BlinkerV2/provisioning/OwnershipReleaseRecord.h",
        "BlinkerV2/provisioning/OwnershipReleaseRecord.cpp",
        "BlinkerV2/arduino/ports/uno_r4/UnoR4BleSecurity.h",
        "BlinkerV2/arduino/ports/uno_r4/UnoR4BleSecurity.ipp",
        "BlinkerV2/arduino/ports/uno_r4/UnoR4BleSecurity.cpp",
        "BlinkerV2/provisioning/OwnershipReleaseRecordStore.h",
        "BlinkerV2/provisioning/OwnershipReleaseRecordStore.cpp",
        "BlinkerV2/provisioning/WifiCredentialResetAdapter.h",
        "BlinkerV2/runtime/DirectBleLifecycle.h",
        "BlinkerV2/runtime/DirectBleLifecycle.cpp",
        "BlinkerV2/security/CloudEnrollmentKeyRing.h",
        "BlinkerV2/security/CloudEnrollmentKeyRing.cpp",
        "BlinkerV2/identity/ControllerCredentialStore.h",
        "BlinkerV2/identity/ControllerCredentialStore.cpp",
        "BlinkerV2/identity/CloudCredential.h",
        "BlinkerV2/identity/CloudCredential.cpp",
        "BlinkerV2/identity/CloudCredentialStore.h",
        "BlinkerV2/identity/CloudCredentialStore.cpp",
        "BlinkerV2/identity/LogicalDeviceId.h",
        "BlinkerV2/identity/LogicalDeviceId.cpp",
        "BlinkerV2/identity/OwnershipRecord.h",
        "BlinkerV2/identity/OwnershipRecord.cpp",
        "BlinkerV2/identity/OwnershipRecordStore.h",
        "BlinkerV2/identity/OwnershipRecordStore.cpp",
        "BlinkerV2/provisioning/EnrollmentTicketContract.h",
        "BlinkerV2/provisioning/EnrollmentTicketContract.cpp",
        "BlinkerV2/provisioning/EnrollmentTicketVerifier.h",
        "BlinkerV2/provisioning/EnrollmentTicketVerifier.cpp",
        "BlinkerV2/provisioning/LocalSetupApplication.h",
        "BlinkerV2/provisioning/LocalSetupApplication.cpp",
        "BlinkerV2/provisioning/LocalSetupContract.h",
        "BlinkerV2/provisioning/LocalSetupContract.cpp",
        "BlinkerV2/provisioning/LocalSetupOperationContract.h",
        "BlinkerV2/provisioning/LocalSetupOperationContract.cpp",
        "BlinkerV2/provisioning/LocalSetupSaga.h",
        "BlinkerV2/provisioning/LocalSetupSaga.cpp",
        "BlinkerV2/provisioning/LocalSetupSagaRecord.h",
        "BlinkerV2/provisioning/LocalSetupSagaRecord.cpp",
        "BlinkerV2/provisioning/LocalSetupSagaStore.h",
        "BlinkerV2/provisioning/LocalSetupSagaStore.cpp",
        "BlinkerV2/provisioning/OwnershipClaimContract.h",
        "BlinkerV2/provisioning/OwnershipClaimContract.cpp",
        "BlinkerV2/provisioning/OwnershipClaimCoordinator.h",
        "BlinkerV2/provisioning/OwnershipClaimCoordinator.cpp",
        "BlinkerV2/provisioning/CloudCredentialEnrollmentContract.h",
        "BlinkerV2/provisioning/CloudCredentialEnrollmentContract.cpp",
        "BlinkerV2/provisioning/CloudCredentialEnrollmentCoordinator.h",
        "BlinkerV2/provisioning/CloudCredentialEnrollmentCoordinator.cpp",
        "BlinkerV2/provisioning/CloudCredentialEnrollmentGrantVerifier.h",
        "BlinkerV2/provisioning/CloudCredentialEnrollmentGrantVerifier.cpp",
        "BlinkerV2/provisioning/CloudCredentialEnvelopeSealer.h",
        "BlinkerV2/provisioning/CloudCredentialEnvelopeSealer.cpp",
        "BlinkerV2/provisioning/CloudEnrollmentRecord.h",
        "BlinkerV2/provisioning/CloudEnrollmentRecord.cpp",
        "BlinkerV2/provisioning/CloudEnrollmentRecordStore.h",
        "BlinkerV2/provisioning/CloudEnrollmentRecordStore.cpp",
    )
)
PRODUCT_ARDUINO = ARDUINO_ROOT
PRODUCT_COMMON = ARDUINO_ROOT / "internal"
PRODUCT_ESP32 = ARDUINO_ROOT / "platform" / "esp32"
PRODUCT_UNO_R4 = ARDUINO_ROOT / "platform" / "uno_r4"
PRODUCT_WIO_TERMINAL = ARDUINO_ROOT / "platform" / "wio_terminal"
OFFICIAL_BLE_PRODUCT = PRODUCT_COMMON / "BleProduct.h"
OFFICIAL_BLE_COMPOSITION = OFFICIAL_BLE_PRODUCT
OFFICIAL_ARDUINO_RUNTIME = PRODUCT_COMMON / "ArduinoRuntime.h"
OFFICIAL_DEVICE_KEY_WIFI_BLE = (
    PRODUCT_ESP32 / "Esp32WiFiBLEProduct.h"
)
LEGACY_OFFICIAL_ARDUINO = REPOSITORY / "integrations" / "official-arduino"
RETIRED_WIFI_BLE_GRAPH = tuple(
    LEGACY_OFFICIAL_ARDUINO / "src" / name
    for name in (
        "BlinkerOfficialWifiBleComposition.h",
        "BlinkerOfficialWifiCloudStack.h",
        "BlinkerOfficialEsp32WifiBle.h",
        "BlinkerOfficialRenesasUnoWifiBle.cpp",
        "BlinkerOfficialRenesasUnoWifiBle.h",
        "BlinkerOfficialEsp32WifiPlatform.h",
        "BlinkerOfficialRenesasUnoWifiPlatform.h",
    )
) + (
    SOURCE_ROOT / "BlinkerV2" / "runtime" / "WifiBleLifecycle.h",
)
OFFICIAL_BLE_WRAPPERS = (
    (PRODUCT_ESP32 / "Esp32BLEProduct.h", "PlatformBleOnlyComposition", True),
    (PRODUCT_UNO_R4 / "UnoR4BLEProduct.h", "PlatformBleOnlyComposition", True),
    (
        PRODUCT_WIO_TERMINAL / "WioTerminalBLEProduct.h",
        "PlatformBleOnlyComposition",
        True,
    ),
)
MODE_SELECTED_IMPLEMENTATIONS = (
    (
        PORT_ROOT / "esp32" / "Esp32NimBle.h",
        PORT_ROOT / "esp32" / "Esp32NimBle.ipp",
        PORT_ROOT / "esp32" / "Esp32NimBle.cpp",
    ),
    (
        PORT_ROOT / "esp32" / "Esp32WiFi.h",
        PORT_ROOT / "esp32" / "Esp32WiFi.ipp",
        PORT_ROOT / "esp32" / "Esp32WiFi.cpp",
    ),
    (
        PORT_ROOT / "arduino" / "ArduinoBleLink.h",
        PORT_ROOT / "arduino" / "ArduinoBleLink.ipp",
        PORT_ROOT / "arduino" / "ArduinoBleLink.cpp",
    ),
    (
        PORT_ROOT / "arduino" / "PortableCrypto.h",
        PORT_ROOT / "arduino" / "PortableCrypto.ipp",
        PORT_ROOT / "arduino" / "PortableCrypto.cpp",
    ),
    (
        PORT_ROOT / "uno_r4" / "UnoR4WiFi.h",
        PORT_ROOT / "uno_r4" / "UnoR4WiFi.ipp",
        PORT_ROOT / "uno_r4" / "UnoR4WiFi.cpp",
    ),
    (
        PORT_ROOT / "wio_terminal" / "WioTerminalWiFi.h",
        PORT_ROOT / "wio_terminal" / "WioTerminalWiFi.ipp",
        PORT_ROOT / "wio_terminal" / "WioTerminalWiFi.cpp",
    ),
    (
        PORT_ROOT / "wio_terminal" / "WioTerminalRpcBle.h",
        PORT_ROOT / "wio_terminal" / "WioTerminalRpcBle.ipp",
        PORT_ROOT / "wio_terminal" / "WioTerminalRpcBle.cpp",
    ),
)
MODE_SELECTED_PRODUCT_BINDINGS = (
    PRODUCT_ESP32 / "Esp32WiFiProduct",
    PRODUCT_ESP32 / "Esp32BLEProduct",
    PRODUCT_UNO_R4 / "UnoR4WiFiProduct",
    PRODUCT_UNO_R4 / "UnoR4BLEProduct",
    PRODUCT_WIO_TERMINAL / "WioTerminalWiFiProduct",
    PRODUCT_WIO_TERMINAL / "WioTerminalBLEProduct",
)
DEVICE_KEY_LOGIN_FREEZE = (
    REPOSITORY / "tests" / "freeze" / "device_key_cloud_login.sha256"
)
RELEASE_BUILDER = REPOSITORY / "tools" / "build_arduino_release.py"
RELEASE_PROFILE_TOOL = REPOSITORY / "tools" / "arduino_release_profile.py"
DEVELOPMENT_RELEASE_PROFILE = (
    REPOSITORY / "tests" / "release" / "development_profile.json"
)
RELEASE_PROFILE_TEST = (
    REPOSITORY / "tests" / "release" / "check_release_profile.py"
)
INSTALLED_RELEASE_GATE = (
    REPOSITORY / "tests" / "arduino" / "verify_installed_release.py"
)
ARDUINO_COMPILE_MATRIX = (
    REPOSITORY / "tests" / "arduino" / "compile_matrix.py"
)
PUBLIC_WIFI_COMPILE_PROBE = (
    REPOSITORY
    / "tests"
    / "arduino"
    / "CompilePublicWiFi"
    / "CompilePublicWiFi.ino"
)
CI_WORKFLOW = REPOSITORY / ".github" / "workflows" / "device-v2-ci.yml"
RETIRED_PACKAGE_PATHS = (
    REPOSITORY / "tools" / "build_official_arduino_package.py",
    REPOSITORY / "tools" / "build_device_v2_core_package.py",
    REPOSITORY / "release" / "device-v2-packages.json",
    REPOSITORY / "tests" / "official-package",
)
OFFICIAL_WIFI_PRODUCT = PRODUCT_COMMON / "WiFiProduct.h"
RETIRED_ENROLLMENT_GRAPH = tuple(
    REPOSITORY / path
    for path in (
        "src/BlinkerV2/control/CloudSessionContract.cpp",
        "src/BlinkerV2/control/CloudSessionContract.h",
        "src/BlinkerV2/control/HttpCloudSessionProvider.cpp",
        "src/BlinkerV2/control/HttpCloudSessionProvider.h",
        "src/BlinkerV2/provisioning/EnrollmentKey.cpp",
        "src/BlinkerV2/provisioning/EnrollmentKey.h",
        "src/BlinkerV2/provisioning/EnrollmentKeyHttpExchange.cpp",
        "src/BlinkerV2/provisioning/EnrollmentKeyHttpExchange.h",
        "src/BlinkerV2/provisioning/WifiOnboardingFlow.cpp",
        "src/BlinkerV2/provisioning/WifiOnboardingFlow.h",
        "src/BlinkerV2/provisioning/WifiOnboardingHttpContract.cpp",
        "src/BlinkerV2/provisioning/WifiOnboardingHttpContract.h",
        "src/BlinkerV2/runtime/WifiControlPlaneArena.h",
        "src/BlinkerV2/runtime/WifiOnboardingCloudLifecycle.h",
        "tests/host/test_cloud_session_main.cpp",
        "tests/host/test_enrollment_key_http_main.cpp",
        "tests/host/test_wifi_onboarding_flow_main.cpp",
    )
)

FORBIDDEN_INCLUDES = re.compile(
    r"#\s*include\s*[<\"](?:"
    r"Arduino(?:Json)?\.h|WProgram\.h|WiFi[^/>\"]*\.h|"
    r"BLE[^/>\"]*\.h|ArduinoBLE\.h|PubSubClient\.h|"
    r"WebSockets[^/>\"]*\.h|HTTPClient\.h|AsyncTCP\.h|"
    r"ESPAsyncTCP\.h|EEPROM\.h|NTPClient\.h"
    r")[>\"]"
)

NO_HEAP_FOLDERS = (
    SOURCE_ROOT / "BlinkerV2" / "api",
    SOURCE_ROOT / "BlinkerV2" / "core",
    SOURCE_ROOT / "BlinkerV2" / "control",
    SOURCE_ROOT / "BlinkerV2" / "identity",
    SOURCE_ROOT / "BlinkerV2" / "model",
    SOURCE_ROOT / "BlinkerV2" / "protocol",
    SOURCE_ROOT / "BlinkerV2" / "provisioning",
    SOURCE_ROOT / "BlinkerV2" / "runtime",
    SOURCE_ROOT / "BlinkerV2" / "security",
    SOURCE_ROOT / "BlinkerV2" / "transport",
)

FORBIDDEN_NO_HEAP_TOKENS = {
    "new": re.compile(r"\bnew\b"),
    "delete": re.compile(r"\bdelete\b"),
    "malloc": re.compile(r"\bmalloc\s*\("),
    "calloc": re.compile(r"\bcalloc\s*\("),
    "realloc": re.compile(r"\brealloc\s*\("),
    "free": re.compile(r"\bfree\s*\("),
    "Arduino String": re.compile(r"\bString\b"),
    "std::string": re.compile(r"\bstd::string\b"),
    "std::vector": re.compile(r"\bstd::vector\b"),
    "std::function": re.compile(r"\bstd::function\b"),
}

RETIRED_AUTHORIZED_PRESENCE_SWITCH = (
    "BLINKER_ENABLE_AUTHORIZED_" + "PRESENCE_PREVIEW"
)


def source_files(root: Path):
    for suffix in ("*.h", "*.hpp", "*.c", "*.cpp", "*.inc"):
        yield from root.rglob(suffix)


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//.*", "", text)


def ble_packet_boundary_failures(
    port_header: str, port_source: str, frame_header: str,
    frame_source: str, record_source: str, noise_source: str,
    runtime_source: str,
) -> list[str]:
    """Check N3 layer boundaries; behavior remains covered by host tests."""
    port_header, port_source, frame_header, frame_source, record_source, noise_source, runtime_source = (
        strip_comments(value) for value in (
            port_header, port_source, frame_header, frame_source,
            record_source, noise_source, runtime_source,
        )
    )

    def section(source: str, start: str, end: str) -> str:
        return source.split(start, 1)[1].split(end, 1)[0] if start in source else ""

    def contains(source: str, *tokens: str) -> bool:
        return all(token in source for token in tokens)

    capacity = section(port_source, "Esp32NimBleLink::packetCapacity(", "Esp32NimBleLink::sendPacket(")
    negotiate = section(frame_source, "BleFrameTransport::negotiatePacketSize(", "BleFrameTransport::disconnectSession(")
    record_negotiate = section(record_source, "BleRecordTransport::negotiatePacketSize(", "BleRecordTransport::disconnectSession(")
    flush = section(record_source, "BleRecordTransport::flushTx(", "BleRecordTransport::expireReassembly(")
    send = section(port_source, "Esp32NimBleLink::sendPacket(", "Esp32NimBleLink::disconnectSession(")
    faults = section(frame_source, "BleFrameTransport::onFault(", "BleFrameTransport::expireUnauthenticatedSessions(")
    hello = section(runtime_source, "DeviceRuntime::handleHello(", "DeviceRuntime::handleCommand(")
    failures = []
    if not (
        contains(port_header, "#define BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE 244", ">= 20", "<= 244")
        and contains(capacity, "ble_att_mtu(handle)", "mtu - 3U", "BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE")
        and "session.maxPacketSize = packetCapacity(connectionHandle_);" in port_source
        and "negotiatePacketSize" not in port_source
    ):
        failures.append("BLE physical ATT capacity must be bounded without selecting protocol stride")
    if not (
        "initialPacketSize(ble::kMinimumPacketSize)" in frame_header
        and "records.initialPacketSize = ble::kMinimumPacketSize;" in noise_source
        and contains(negotiate, "!rx.encrypted", "slot == nullptr", "!slot->active()",
                     "requested < ble::kMinimumPacketSize", "requested > 244U",
                     "records_.negotiatePacketSize(rx.sessionId, requested)")
        and contains(frame_source, "bbp2::MessageKind::Hello", "? ble::kMinimumPacketSize : 0U")
        and contains(hello, "(frame.header.flags & bbp2::FlagIsResponse) == 0U",
                     "hello.maxBlePacketSize != 0U", "transport->negotiatePacketSize(")
    ):
        failures.append("BLE bootstrap must stay ATT20 and packet negotiation must require active Direct authentication")
    if not (
        contains(record_negotiate, "physical.maxPacketSize", "packetScratch_.size", "session->packetFallback")
        and contains(flush, "availablePayloadCapacity < item.fragmentPayloadCapacity",
                     "if (result.code() == ErrorCode::WouldBlock) return;",
                     "result.code() == ErrorCode::CapacityExceeded",
                     "sessionState->packetLimit = ble::kMinimumPacketSize;",
                     "sessionState->packetFallback = true;",
                     "item.recordId = nextRecordId(*sessionState);",
                     "item.offset = 0U;", "item.fragmentIndex = 0U;", "txBuffer(txHead_)")
    ):
        failures.append("BLE queued record stride and same-record ATT20 fallback must be preserved")
    if not (
        contains(send, "packet.size > capacity", "ErrorCode::CapacityExceeded", "ErrorCode::ProtocolError")
        and contains(faults, "slot->session.ready()", "retire(rx.sessionId, error)",
                     "records_.disconnectSession(sessionId, error)")
    ):
        failures.append("BLE unknown native send errors must retire Direct sessions, not trigger capacity fallback")
    return failures


def login_freeze_digest(path: Path) -> str:
    """Hash implementation text without build-layout-only include paths."""
    text = path.read_text(encoding="utf-8")
    text = re.sub(
        r"^\s*#\s*include\s*[<\"][^>\"]+[>\"]\s*$",
        "",
        text,
        flags=re.MULTILINE,
    )
    text = re.sub(
        r"^\s*#\s*if\s+defined\(ARDUINO_ARCH_[^)]+\)"
        r"(?:\s*\|\|\s*defined\(ARDUINO_ARCH_[^)]+\))?\s*$",
        "",
        text,
        flags=re.MULTILINE,
    )
    text = re.sub(
        r"^\s*#\s*endif\s*//\s*ARDUINO_ARCH_.*$",
        "",
        text,
        flags=re.MULTILINE,
    )
    normalized = "\n".join(
        line.rstrip() for line in text.splitlines() if line.strip()
    ) + "\n"
    return hashlib.sha256(normalized.encode("utf-8")).hexdigest()


def verify_device_key_login_freeze(failures: list[str]) -> None:
    seen: set[Path] = set()
    for line_number, raw_line in enumerate(
        DEVICE_KEY_LOGIN_FREEZE.read_text(encoding="utf-8").splitlines(),
        start=1,
    ):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        match = re.fullmatch(r"([0-9a-f]{64})  ([^\\]+)", line)
        if match is None:
            failures.append(
                f"{DEVICE_KEY_LOGIN_FREEZE}:{line_number}: invalid freeze entry"
            )
            continue
        expected, relative = match.groups()
        path = REPOSITORY / Path(relative)
        if path in seen:
            failures.append(
                f"{DEVICE_KEY_LOGIN_FREEZE}:{line_number}: duplicate path {relative}"
            )
            continue
        seen.add(path)
        if not path.is_file():
            failures.append(f"{DEVICE_KEY_LOGIN_FREEZE}: missing {relative}")
            continue
        actual = login_freeze_digest(path)
        if actual != expected:
            failures.append(
                f"{relative}: DeviceKey login freeze changed; expected {expected}, got {actual}"
            )

    if not seen:
        failures.append(f"{DEVICE_KEY_LOGIN_FREEZE}: empty freeze manifest")


def main() -> int:
    failures: list[str] = []

    implementation_units = {
        path.name for path in SOURCE_ROOT.rglob("*.cpp")
    }
    if implementation_units != EXPECTED_IMPLEMENTATION_UNITS:
        failures.append(
            "Arduino implementation units changed: "
            f"expected {len(EXPECTED_IMPLEMENTATION_UNITS)}, "
            f"found {sorted(implementation_units)}"
        )
    if any(path.parent != IMPLEMENTATION_ROOT
           for path in SOURCE_ROOT.rglob("*.cpp")):
        failures.append(
            "ordinary .cpp files must remain in BlinkerV2/implementation"
        )

    verify_device_key_login_freeze(failures)
    # Protocol authority is not ownership of native connection mechanics.
    for relative, pattern in (
        ("runtime/GatewayChildRouteBridge.h", r"\bchild_\.(?:start|connect|disconnect|stop)\s*\("),
        ("runtime/GatewayProofCoordinator.h", r"\bchild_\.(?:start|connect|disconnect|stop)\s*\("),
        ("arduino/platform/esp32/Esp32EdgeHubProduct.h", r"\bchild_\.session\.(?:start|connect|disconnect)\s*\("),
        ("runtime/GatewayGattChildContext.h", r"\b(?:session|connection)\.(?:start|connect|disconnect|allowAcquisition|ensureConnected|retire)\s*\("),
        ("runtime/GatewayGattRadio.h", r"\bchild_\.(?:start|connect|disconnect)\s*\("),
    ):
        path = SOURCE_ROOT / "BlinkerV2" / relative
        if re.search(pattern, strip_comments(path.read_text(encoding="utf-8"))):
            failures.append(f"{relative}: native acquisition/retirement bypasses shared connection owner")
    edge_product = strip_comments((SOURCE_ROOT / "BlinkerV2/arduino/platform/esp32/Esp32EdgeHubProduct.h").read_text(encoding="utf-8"))
    central_header = strip_comments((PORT_ROOT / "esp32/Esp32NimBleCentralPort.h").read_text(encoding="utf-8"))
    central_port = strip_comments((PORT_ROOT / "esp32/Esp32NimBleCentralPort.ipp").read_text(encoding="utf-8"))
    central_host = strip_comments((PORT_ROOT / "esp32/Esp32NimBleCentralHost.h").read_text(encoding="utf-8"))
    if re.search(r"activePort|nimble_port_(?:init|stop|deinit|freertos_init)|ble_hs_cfg|esp32_nimble_detail::(?:claim|release)", central_header + central_port):
        failures.append("Central Port must borrow the product Host, not own SDK lifecycle or global callbacks")
    if not re.search(r"Esp32NimBleCentralHost centralHost_;\s*Esp32NimBleCentralPort childPorts_\[activeChildCapacity\];", edge_product) or edge_product.count("Esp32NimBleCentralPort(centralHost_)") != 2:
        failures.append("Edge Hub must own the Central Host before its borrowed Ports")
    if central_port.count("Esp32NimBleCentralHost::Callback callback(context);") != 7 or re.search(r"&Esp32NimBleCentralPort::on\w+,\s*(?:this|self)\s*\)", central_port):
        failures.append("all Central GAP/GATT callbacks must resolve an opaque lifetime token, never raw Port memory")
    if any(word in central_host for word in ("DeviceKey", "GatewayAccess", "Mqtt", "Credential", "ClientStorage")):
        failures.append("Central Host must not own cloud identity, child credentials or business workspaces")
    if "activity_" in central_host or "CONFIG_BT_NIMBLE_MAX_CONNECTIONS" not in central_host or "host_.finishInitiation(this)" not in central_port:
        failures.append("Central must separate the bounded per-Port SDK slot from the single scan/initiation owner")
    if "host_.ownsInitiation(this)" not in central_port or "port.token = 0U" not in central_host:
        failures.append("Central close/reset must preserve other Ports and invalidate pre-reset callbacks")
    if re.search(r"child_\.connection\.(?:allowAcquisition|ensureConnected|retire)\s*\(", edge_product):
        failures.append("ESP32 Edge Hub must delegate all GATT decisions to the shared radio policy")
    child_context = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/GatewayGattChildContext.h").read_text(encoding="utf-8"))
    if re.search(r"Esp32|Arduino\.h|MqttFrameTransport|DeviceKeyWifiStack|NimBle", child_context):
        failures.append("GATT child context must not own an MCU SDK, WiFi product or MQTT transport")
    if "ChildContext children_[activeChildCapacity];" not in edge_product or "childRx_" in edge_product or "routeBridge_;" in edge_product:
        failures.append("ESP32 Edge Hub must consume the single per-child context, not duplicate its old graph")
    if "management_(cloudMux_, directory_, managementRouter_)" not in edge_product:
        failures.append("ESP32 management must use directory delivery and child-aware control routing")
    if not re.search(r"if \(result\) result = platform_\.gatewayStorage\(\)\.clear\(\);\s*if \(result\) result = deviceKey_\.clear\(\);", edge_product) or "GatewayChildRecords childRecords_;" in edge_product:
        failures.append("Edge Hub reset must clear the entire durable bank, not keep a private slot-zero blob")
    if "GatewayChildBinding bindings_[Platform::GatewayStorage::capacity]" not in edge_product:
        failures.append("durable bindings must be independent of native GATT workspace capacity")
    if re.search(r"Gateway(?:Access|CredentialRenewal)DeliveryProcessor|IDeviceKeySource", child_context):
        failures.append("active GATT context must not own durable credential ingress or the Hub root source")
    child_executor = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/GatewayChildExecutor.h").read_text(encoding="utf-8"))
    child_bridge = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/GatewayChildRouteBridge.h").read_text(encoding="utf-8"))
    if any(token in child_bridge for token in ("child_.setReceiver", "child_.setSessionHandlers", "nextChildSequence_")):
        failures.append("Cloud child adapter must not own callbacks or the southbound sequence allocator")
    if child_executor.count("child_.setReceiver") != 2 or child_executor.count("child_.setSessionHandlers") != 2 or "nextSequence_" not in child_executor:
        failures.append("the unique child executor must install and retire the sole child callbacks and sequence allocator")
    time_source = strip_comments((SOURCE_ROOT / "BlinkerV2/interface/IWallTimeSource.h").read_text(encoding="utf-8"))
    time_service = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/TimeSyncService.inc").read_text(encoding="utf-8"))
    time_responder = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/TimeSyncResponder.inc").read_text(encoding="utf-8"))
    if "readTime(WallClockSnapshot&" not in time_source or re.search(r"\b(?:IClock|synchronize|reset|WallClock)\s*[&*(]", time_source):
        failures.append("shared business time must expose only a read-only snapshot, never a mutable/security clock")
    if re.search(r"\b(?:unixTime|settimeofday|configTime)\s*\(", time_service + time_responder):
        failures.append("business time service/responder must not read or change the security/system clock")
    if "DeviceRuntime.h" in time_responder or "source_.readTime(snapshot)" not in time_responder:
        failures.append("time responder must borrow the shared snapshot, not depend on DeviceRuntime")
    if any(token in child_executor + child_bridge + edge_product for token in (
        'GatewayTimeResponder', 'TimeSyncResponder', 'TimeSyncService',
    )):
        failures.append("default gateway composition/executor must not own or link concrete business-time modules")
    if 'IGatewayChildRequestHandler* requestHandler_' not in child_executor:
        failures.append("optional child requests must borrow one module through the unique executor")
    local_contract = strip_comments((SOURCE_ROOT / "BlinkerV2/protocol/local/Access.h").read_text(encoding="utf-8"))
    local_security = strip_comments((SOURCE_ROOT / "BlinkerV2/security/LocalAccessSecurity.inc").read_text(encoding="utf-8"))
    local_lease = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/LocalAccessLease.h").read_text(encoding="utf-8"))
    local_record = strip_comments((SOURCE_ROOT / "BlinkerV2/protocol/noise/LocalSecureRecord.inc").read_text(encoding="utf-8"))
    if "ControllerGrant" in local_contract + local_security + local_lease:
        failures.append("LAN access must not reuse the durable ControllerGrant mutation contract")
    if any(secret in local_contract for secret in ("noisePsk", "sessionKey", "DeviceKey")):
        failures.append("LAN wire grants must contain neither a PSK nor a DeviceKey")
    if local_contract.count("NoiseNnPsk0AesGcmSha256") != 1 or local_contract.count("PlainHmacSha256") != 1 or "kMaximumLifetimeMillis = 300000U" not in local_contract:
        failures.append("LAN must expose explicit Plain/Noise profiles with bounded lifetime")
    local_transport = strip_comments((SOURCE_ROOT / "BlinkerV2/transport/LocalAccessFrameTransport.h").read_text(encoding="utf-8"))
    plain_session = strip_comments((SOURCE_ROOT / "BlinkerV2/security/LocalAccessPlainSession.h").read_text(encoding="utf-8"))
    if any(dependency in local_transport + plain_session for dependency in ("NoiseNn.h", "NoiseNnSession", "LocalSecureRecord.h", "Esp32Crypto")):
        failures.append("shared LAN transport and Plain session must not depend on Noise/platform crypto")
    if "streamGrantTranscript" not in local_security or "grant.authenticator" not in local_security:
        failures.append("LAN grant authentication must stream the canonical transcript and bind session material")
    if "unixTime" in local_lease or "expiresAtUnix" in local_lease:
        failures.append("LAN challenge and lease expiry must use receiver monotonic time only")
    if (
        "kLocalAccessNoiseNnPsk0Prologue" not in local_record
        or local_record.count("localAccessNoisePrologue") != 1
    ):
        failures.append("LAN Noise must use a distinct fixed domain from provisioning")
    if not re.search(r"BasicGatewayChildConnection<GatewayGattChildSession> connection;\s*Executor executor;\s*Proof proof;\s*Bridge bridge;", child_context):
        failures.append("GATT child context must own one executor before all adapters and management coordinators")
    if not re.search(r"executor\.setControllerControlReceiver\(\s*&IGatewayManagementControl::controllerControlResponseThunk,\s*&management\)", child_context) or any(f"&children_[{i}U].management}}" not in edge_product for i in range(2)):
        failures.append("child control responses must return to their originating mux, not a mutable directory owner")
    child_mux = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/GatewayManagementControlMux.h").read_text(encoding="utf-8"))
    if "PermitJoin" in child_mux or "permit_" in child_mux or "managementControl_" in edge_product:
        failures.append("child mux must own only proof/revoke/renew; no duplicate Hub permit or Product mux")
    if "managementRouter_.setPermitAdmission(gattPool_)" not in edge_product or "permitJoinAdapter_(stack_.clock(), gattPool_)" not in edge_product:
        failures.append("child radio admission must not overwrite Hub-wide management admission")
    gatt_pool = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/GatewayGattPool.h").read_text(encoding="utf-8"))
    gatt_radio = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/GatewayGattRadio.h").read_text(encoding="utf-8"))
    if "permit_." in gatt_radio or "permitJoin_" in gatt_radio or "management_.poll()" in gatt_radio:
        failures.append("per-child radio must not own or poll Hub-wide permit/management")
    if any(word in gatt_pool for word in ("Esp32", "NimBle", "Mqtt", "DeviceKey")) or "GatewayPresenceMatcher matcher;" not in gatt_pool:
        failures.append("GATT pool must use existing presence matching, not SDK/cloud authority or MAC identity")
    management_router = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/GatewayChildManagementRouter.inc").read_text(encoding="utf-8"))
    if re.search(r"\.commit\s*\(|\.replace\s*\(|\.clear\s*\(|\b(?:Esp32|NimBLE|WiFiClient)\w*\b", management_router):
        failures.append("management routing must not mutate authority records or depend on a native platform")
    route_binding = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/GatewayRouteBinding.h").read_text(encoding="utf-8"))
    if "GatewayAccessRecord" in route_binding or ".load(" in route_binding:
        failures.append("route selection must use invalidated non-secret identity, not read full Flash records per frame")
    directory_source = strip_comments((SOURCE_ROOT / "BlinkerV2/runtime/GatewayChildDirectory.inc").read_text(encoding="utf-8"))
    if re.search(r"\.commit\s*\(|\.replace\s*\(|\.clear\s*\(", directory_source):
        failures.append("child directory must delegate authority mutations to existing authenticated processors")
    retired_nonproduct = [
        str(path) for path in RETIRED_NONPRODUCT_SOURCES if path.exists()
    ]
    if retired_nonproduct:
        failures.append(
            "retired non-product source remains: "
            + ", ".join(retired_nonproduct)
        )
    if RETIRED_LOGGER.exists():
        failures.append("retired string ILogger interface remains")
    for path in (DIAGNOSTICS_HEADER, DIAGNOSTICS_SOURCE, ARDUINO_DIAGNOSTICS):
        if not path.is_file():
            failures.append(f"diagnostics implementation is missing: {path}")
    for path in (OUTBOUND_SCHEDULER_HEADER, OUTBOUND_SCHEDULER_SOURCE):
        if not path.is_file():
            failures.append(f"outbound scheduler is missing: {path}")
    if OUTBOUND_SCHEDULER_SOURCE.is_file():
        scheduler = strip_comments(
            OUTBOUND_SCHEDULER_SOURCE.read_text(encoding="utf-8")
        )
        if any(
            required not in scheduler
            for required in (
                "markStateDirty",
                "stateFlushDue",
                "commitStateSelection",
                "admitEvent",
                "eventTokensPerSecond",
            )
        ):
            failures.append(
                f"{OUTBOUND_SCHEDULER_SOURCE}: state/event scheduling gate missing"
            )
    if CLIENT_SOURCE.is_file() and DEVICE_RUNTIME_SOURCE.is_file():
        client = strip_comments(CLIENT_SOURCE.read_text(encoding="utf-8"))
        runtime = strip_comments(
            DEVICE_RUNTIME_SOURCE.read_text(encoding="utf-8")
        )
        if any(
            required not in client
            for required in (
                "outbound_.markStateDirty",
                "outbound_.admitEvent",
                "flushPendingState(true, true)",
                "setStateSettlementHandler",
            )
        ) or "stateSettlementHandler_" not in runtime:
            failures.append("Client/Runtime outbound settlement gate missing")
    limiter_in_ports = tuple(
        path
        for path in source_files(PORT_ROOT)
        if "OutboundScheduler" in path.read_text(
            encoding="utf-8", errors="ignore"
        )
    )
    if limiter_in_ports:
        failures.append("platform port contains duplicated outbound scheduler")
    if ARDUINO_DIAGNOSTICS.is_file():
        formatter = ARDUINO_DIAGNOSTICS.read_text(encoding="utf-8")
        leaked_secret_terms = tuple(
            term
            for term in (
                "deviceKey",
                "password",
                "credential",
                "token",
                "proofOfPossession",
                "authorization",
            )
            if term in formatter
        )
        if leaked_secret_terms:
            failures.append(
                "Arduino diagnostics formatter names secret-bearing data: "
                + ", ".join(leaked_secret_terms)
            )
    if not PUBLIC_WIFI_COMPILE_PROBE.is_file():
        failures.append("public WiFi product compile probe is missing")
    if ARDUINO_COMPILE_MATRIX.is_file():
        compile_matrix = ARDUINO_COMPILE_MATRIX.read_text(encoding="utf-8")
        if (
            'target_name = facade_name + "-product-wifi"'
            not in compile_matrix
            or '"tests/arduino/CompilePublicWiFi"' not in compile_matrix
            or 'target_name = facade_name + "-product-ble"'
            not in compile_matrix
            or '"examples/BleQuickStart"' not in compile_matrix
            or 'TARGETS["esp32-product-wifi-ble"]' not in compile_matrix
            or '"examples/WiFiBleQuickStart"' not in compile_matrix
            or '"products": tuple(' not in compile_matrix
        ):
            failures.append(
                f"{ARDUINO_COMPILE_MATRIX}: actual WiFi/BLE product gate missing"
            )
        if (
            RETIRED_AUTHORIZED_PRESENCE_SWITCH in compile_matrix
            or "authorized-presence-preview" in compile_matrix
        ):
            failures.append(
                f"{ARDUINO_COMPILE_MATRIX}: retired Presence preview target remains"
            )
    else:
        failures.append("Arduino compile matrix is missing")

    retired_presence_switch_files = []
    for root in (
        SOURCE_ROOT,
        REPOSITORY / "examples",
        REPOSITORY / "tests" / "arduino",
        REPOSITORY / "tests" / "internal" / "arduino",
    ):
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if (
                path.is_file()
                and path.suffix in {".h", ".inc", ".cpp", ".ino", ".py"}
                and RETIRED_AUTHORIZED_PRESENCE_SWITCH
                in path.read_text(encoding="utf-8", errors="ignore")
            ):
                retired_presence_switch_files.append(
                    str(path.relative_to(REPOSITORY))
                )
    if retired_presence_switch_files:
        failures.append(
            "retired authorized Presence build switch remains: "
            + ", ".join(retired_presence_switch_files)
        )

    direct_profile_text = strip_comments(
        BLE_DIRECT_PROFILE_HEADER.read_text(encoding="utf-8")
        + BLE_DIRECT_PROFILE_SOURCE.read_text(encoding="utf-8")
    )
    if (
        "tryMakeAuthorized(profile)" not in direct_profile_text
        or "authorizedProfile_" not in direct_profile_text
        or "authorizedPresence" in direct_profile_text
    ):
        failures.append(
            "BLE Direct profile must select v2/v3 from durable Presence state"
        )
    if (REPOSITORY / "adapters").exists():
        failures.append("retired adapters package tree remains")
    if (REPOSITORY / "ports").exists() or (REPOSITORY / "products").exists():
        failures.append("retired root Port/Product source tree remains")
    retired_developer_identity = (
        SOURCE_ROOT / "BlinkerV2" / "identity" / "DeveloperDeviceId.cpp",
        SOURCE_ROOT / "BlinkerV2" / "identity" / "DeveloperDeviceId.h",
        SOURCE_ROOT / "BlinkerV2" / "identity" / "P256DeveloperIdentity.cpp",
        SOURCE_ROOT / "BlinkerV2" / "identity" / "P256DeveloperIdentity.h",
        SOURCE_ROOT
        / "BlinkerV2"
        / "identity"
        / "P256DeveloperIdentityRecord.cpp",
        SOURCE_ROOT
        / "BlinkerV2"
        / "identity"
        / "P256DeveloperIdentityRecord.h",
        SOURCE_ROOT / "BlinkerV2" / "interface" / "IFactoryIdentity.h",
    )
    if any(path.exists() for path in retired_developer_identity):
        failures.append("retired P-256 Developer Identity graph remains")

    if any(path.exists() for path in RETIRED_ENROLLMENT_GRAPH):
        failures.append("retired EnrollmentKey/CloudSession onboarding graph remains")

    gitignore = (REPOSITORY / ".gitignore").read_text(encoding="utf-8")
    if re.search(r"(?m)^/release/?$", gitignore):
        failures.append("release manifest is hidden by the root .gitignore")
    if not RELEASE_BUILDER.is_file():
        failures.append("single Arduino release builder is missing")
    for required_release_file in (
        RELEASE_PROFILE_TOOL,
        DEVELOPMENT_RELEASE_PROFILE,
        RELEASE_PROFILE_TEST,
    ):
        if not required_release_file.is_file():
            failures.append(
                "release-profile boundary file is missing: "
                + str(required_release_file.relative_to(REPOSITORY))
            )
    if not INSTALLED_RELEASE_GATE.is_file():
        failures.append("installed Arduino release gate is missing")
    if any(path.exists() for path in RETIRED_PACKAGE_PATHS):
        failures.append("retired per-product packaging path remains")
    if (REPOSITORY / "ISSUE_TEMPLATE.md").exists():
        failures.append("retired root issue template remains")

    workflow = CI_WORKFLOW.read_text(encoding="utf-8")
    if (
        '      - "products/**"' in workflow
        or '      - "ports/**"' in workflow
        or '      - "integrations/**"' in workflow
        or "build_official_arduino_package.py" in workflow
        or "build_device_v2_core_package.py" in workflow
        or "tests/official-package" in workflow
        or "device-v2-packages.json" in workflow
        or re.search(r"(?m)^\s{14}targets:", workflow)
    ):
        failures.append(f"{CI_WORKFLOW}: stale path or malformed matrix entry")
    if (
        "tests/arduino/verify_installed_release.py" not in workflow
        or "--zip-path build/formal-release/artifacts/" not in workflow
        or "--jobs 4" not in workflow
    ):
        failures.append(f"{CI_WORKFLOW}: installed ZIP gate missing")
    if (
        "tests/release/check_release_profile.py" not in workflow
        or workflow.count("--profile tests/release/development_profile.json") != 3
        or workflow.count("--allow-development-profile") != 3
    ):
        failures.append(f"{CI_WORKFLOW}: release-profile gate missing")

    keywords = (REPOSITORY / "keywords.txt").read_text(encoding="utf-8")
    if any(
        re.search(rf"(?m)^{re.escape(name)}\s", keywords)
        for name in (
            "WiFi",
            "BLE",
            "WiFiBLE",
            "WifiOnboardingProfile",
            "onboard",
            "onboardOpen",
            "ClientStorage",
            "FieldSpec",
        )
    ):
        failures.append("keywords.txt exposes retired mode/onboarding/internal API")

    retired_uno_crypto = "renesas-uno-" + "crypto"
    retired_uno_crypto_type = "BlinkerRenesasUno" + "Crypto"
    retired_uno_crypto_target = "blinker_renesas_" + "crypto"
    host_build_text = "\n".join(
        path.read_text(encoding="utf-8", errors="ignore")
        for path in (REPOSITORY / "tests" / "host").rglob("*")
        if path.is_file()
        and path.suffix in {".txt", ".cpp", ".h"}
        and not any(
            part == "build" or part.startswith("build-")
            for part in path.parts
        )
    )
    if any(
        retired in host_build_text
        for retired in (
            "DeveloperDeviceId",
            "P256DeveloperIdentity",
            "IFactoryIdentity",
        )
    ):
        failures.append("retired Developer Identity host test graph remains")
    if any(
        retired in host_build_text
        for retired in (
            "EnrollmentKeyHttpExchange",
            "WifiOnboardingFlow",
            "WifiOnboardingCloudLifecycle",
            "HttpCloudSessionProvider",
            "CloudSessionContract",
        )
    ):
        failures.append("retired onboarding/session host test graph remains")
    if (
        (PORT_ROOT / "uno_r4" / "UnoR4Crypto.cpp").exists()
        or retired_uno_crypto in host_build_text
        or retired_uno_crypto_type in host_build_text
        or retired_uno_crypto_target in host_build_text
    ):
        failures.append("retired UNO R4 crypto adapter/test graph remains")

    wifiprov_header = strip_comments(
        ESP32_WIFIPROV_HEADER.read_text(encoding="utf-8")
    )
    wifiprov_implementation = strip_comments(
        ESP32_WIFIPROV_IMPLEMENTATION.read_text(encoding="utf-8")
    )
    if (
        '#include "Esp32WiFiProv.ipp"' not in wifiprov_header
        or "BLINKER_ESP32_PROVISIONING_SOFTAP" not in wifiprov_implementation
        or "network_prov_scheme_softap" not in wifiprov_implementation
        or "network_prov_scheme_ble" not in wifiprov_implementation
        or "inline Result Esp32WifiProvAdapter::begin" not in wifiprov_implementation
        or "DeviceKeyProvisioningOperation::GetInfo" not in wifiprov_implementation
        or "kDeviceKeyProvisioningHasDeviceKey" not in wifiprov_implementation
        or any(path.exists() for path in RETIRED_ESP32_WIFIPROV_SOURCES)
    ):
        failures.append(
            "ESP32 WiFiProv must remain selector-reachable .ipp only"
        )

    public_includes = re.findall(
        r'#\s*include\s*[<"]([^>"]+)[>"]',
        PUBLIC_HEADER.read_text(encoding="utf-8"),
    )
    if public_includes != ["BlinkerWiFi.h"]:
        failures.append(
            f"{PUBLIC_HEADER}: default aggregate must contain only BlinkerWiFi.h"
        )
    selector_contracts = (
        (
            WIFI_HEADER,
            "BLINKER_PRODUCT_WIFI",
            "BLINKER_PRODUCT_BLE",
            (
                "BlinkerV2/arduino/platform/esp32/Esp32WiFiProduct.h",
                "BlinkerV2/arduino/platform/uno_r4/UnoR4WiFiProduct.h",
                "BlinkerV2/arduino/platform/wio_terminal/WioTerminalWiFiProduct.h",
            ),
        ),
        (
            BLE_HEADER,
            "BLINKER_PRODUCT_BLE",
            "BLINKER_PRODUCT_WIFI",
            (
                "BlinkerV2/arduino/platform/esp32/Esp32BLEProduct.h",
                "BlinkerV2/arduino/platform/uno_r4/UnoR4BLEProduct.h",
                "BlinkerV2/arduino/platform/wio_terminal/WioTerminalBLEProduct.h",
            ),
        ),
        (
            WIFI_BLE_HEADER,
            "BLINKER_PRODUCT_WIFI_BLE",
            "BLINKER_PRODUCT_WIFI",
            (
                "BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h",
            ),
        ),
    )
    for header, selected, conflicting, platform_products in selector_contracts:
        selector_text = header.read_text(encoding="utf-8")
        selector_includes = re.findall(
            r'#\s*include\s*[<"]([^>"]+)[>"]', selector_text
        )
        if (
            not selector_includes
            or selector_includes[0] != "BlinkerV2/api/BlinkerFacade.h"
            or any(
                product not in selector_includes
                for product in platform_products
            )
            or f"#define {selected} 1" not in selector_text
            or f"defined({conflicting})" not in selector_text
        ):
            failures.append(f"{header}: product selector contract regression")
    for header, implementation, retired_source in MODE_SELECTED_IMPLEMENTATIONS:
        include = f'#include "{implementation.name}"'
        if (
            not header.is_file()
            or not implementation.is_file()
            or include not in header.read_text(encoding="utf-8")
            or retired_source.exists()
        ):
            failures.append(
                f"{header}: per-sketch implementation selection regression"
            )
    for binding in MODE_SELECTED_PRODUCT_BINDINGS:
        if not binding.with_suffix(".h").is_file() or binding.with_suffix(
            ".cpp"
        ).exists():
            failures.append(
                f"{binding}: Product binding must be selector-reachable header only"
            )
    facade_text = PUBLIC_HEADER.read_text(encoding="utf-8") + (
        SOURCE_ROOT / "BlinkerV2" / "api" / "BlinkerFacade.h"
    ).read_text(encoding="utf-8")
    if any(
        obsolete in facade_text
        for obsolete in (
            "WiFiTag",
            "BleTag",
            "WiFiBleTag",
            "constexpr WiFi",
            "constexpr BLE",
            "constexpr WiFiBLE",
        )
    ):
        failures.append("public facade still exposes a redundant product tag")
    if (
        "IsInteraction" in facade_text
        or "#include <type_traits>" in facade_text
        or "#if defined(BLINKER_PRODUCT_WIFI)" not in facade_text
        or "#elif defined(BLINKER_PRODUCT_BLE)" not in facade_text
        or "#elif defined(BLINKER_PRODUCT_WIFI_BLE)" not in facade_text
    ):
        failures.append("public facade product overload boundary regression")
    lifecycle_api_text = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (
            SOURCE_ROOT / "BlinkerV2" / "api" / "BlinkerFacade.h",
            SOURCE_ROOT / "BlinkerV2" / "api" / "Product.h",
            SOURCE_ROOT / "BlinkerV2" / "interface" / "IProductLifecycle.h",
        )
    )
    if (
        "resetOwnership" in lifecycle_api_text
        or "ProductCapabilityOwnershipReset" in lifecycle_api_text
    ):
        failures.append("unimplemented ownership reset remains public")
    if any(
        required not in lifecycle_api_text
        for required in (
            "resetNetwork",
            "ProductCapabilityNetworkReset",
        )
    ):
        failures.append("network reset lifecycle contract is incomplete")
    advanced_text = ADVANCED_HEADER.read_text(encoding="utf-8")
    if (
        not re.search(
            r'#\s*include\s*[<"]BlinkerV2/api/BlinkerFacade\.h[>"]',
            advanced_text,
        )
        or re.search(r'#\s*include\s*[<"]Blinker\.h[>"]', advanced_text)
    ):
        failures.append(
            f"{ADVANCED_HEADER}: Advanced entry must directly include the facade"
        )

    arduino_ble_text = strip_comments(
        ARDUINO_BLE_PORT.read_text(encoding="utf-8")
        + ARDUINO_BLE_HEADER.read_text(encoding="utf-8")
    )
    if "BLE.paired(" in arduino_ble_text:
        failures.append(
            f"{ARDUINO_BLE_PORT}: any-peer BLE.paired() is forbidden"
        )
    if "setSessionSecurity" in arduino_ble_text:
        failures.append(
            f"{ARDUINO_BLE_PORT}: application-writable security is forbidden"
        )
    if (
        "IBleConnectionSecuritySource" not in arduino_ble_text
        or "StringView(address.c_str(), address.length())" not in arduino_ble_text
        or "IArduinoBleSecuritySource" in arduino_ble_text
        or not BLE_CONNECTION_SECURITY_SOURCE.is_file()
    ):
        failures.append(
            "BLE exact-peer security must use the Core address-view contract"
        )
    uno_r4_ble_text = strip_comments(
        UNO_R4_BLE_PLATFORM.read_text(encoding="utf-8")
    )
    if (
        "config.requireEncryption = false" not in uno_r4_ble_text
        or "UnoR4BleSecurity" in uno_r4_ble_text
    ):
        failures.append(
            f"{UNO_R4_BLE_PLATFORM}: portable Direct security policy missing"
        )

    failures.extend(ble_packet_boundary_failures(
        ESP32_NIMBLE_HEADER.read_text(encoding="utf-8"),
        ESP32_NIMBLE_IMPLEMENTATION.read_text(encoding="utf-8"),
        BLE_FRAME_TRANSPORT_HEADER.read_text(encoding="utf-8"),
        BLE_FRAME_TRANSPORT_SOURCE.read_text(encoding="utf-8"),
        BLE_RECORD_TRANSPORT.read_text(encoding="utf-8"),
        BLE_NOISE_CHANNEL.read_text(encoding="utf-8"),
        DEVICE_RUNTIME_SOURCE.read_text(encoding="utf-8"),
    ))

    ble_record_text = strip_comments(
        BLE_RECORD_TRANSPORT.read_text(encoding="utf-8")
    )
    on_packet = ble_record_text.split(
        "void BleRecordTransport::onPacket", 1
    )[-1].split("void BleRecordTransport::reconcileSessions", 1)[0]
    if (
        "reconcileSessions();" not in ble_record_text
        or "securityDowngraded(" not in ble_record_text
        or "retireSession(" not in ble_record_text
        or "findRx(info.sessionId, true)" in on_packet
    ):
        failures.append(
            f"{BLE_RECORD_TRANSPORT}: BLE session fault gate missing"
        )

    ble_link_text = strip_comments(
        BLE_LINK_INTERFACE.read_text(encoding="utf-8")
    )
    ble_frame_text = strip_comments(
        BLE_FRAME_TRANSPORT_HEADER.read_text(encoding="utf-8")
        + BLE_FRAME_TRANSPORT_SOURCE.read_text(encoding="utf-8")
    )
    ble_noise_text = strip_comments(
        BLE_NOISE_CHANNEL.read_text(encoding="utf-8")
    )
    if (
        "virtual Result disconnectSession(uint32_t sessionId)" not in ble_link_text
        or "link_.disconnectSession(sessionId)" not in ble_record_text
        or "authenticationTimeoutMillis" not in ble_frame_text
        or "expireUnauthenticatedSessions" not in ble_frame_text
        or "records_.disconnectSession(rx.sessionId, error)" not in ble_noise_text
    ):
        failures.append(
            "BLE active retirement/authentication deadline contract is missing"
        )

    for port_header, port_source in (
        (ARDUINO_BLE_HEADER, ARDUINO_BLE_PORT),
        (ESP32_NIMBLE_HEADER, ESP32_NIMBLE_IMPLEMENTATION),
        (WIO_RPC_BLE_HEADER, WIO_RPC_BLE_IMPLEMENTATION),
    ):
        port_text = strip_comments(
            port_header.read_text(encoding="utf-8")
            + port_source.read_text(encoding="utf-8")
        )
        if (
            "sessionReadyTimeoutMillis" not in port_text
            or "disconnectSession(uint32_t sessionId)" not in port_text
        ):
            failures.append(
                f"{port_header}: BLE readiness timeout/active disconnect missing"
            )

    ble_mode_text = strip_comments(
        BLE_MODE_COORDINATOR.read_text(encoding="utf-8")
    )
    if (
        "activeProfile_.mode == ble::ApplicationMode::Direct"
        not in ble_mode_text
        or "activeEndpoint_->state() == TransportState::Error"
        not in ble_mode_text
    ):
        failures.append(
            f"{BLE_MODE_COORDINATOR}: direct-link fault propagation missing"
        )

    ble_setup_text = strip_comments(
        BLE_SETUP_LIFECYCLE.read_text(encoding="utf-8")
    )
    if (
        "completion_.load(complete)" not in ble_setup_text
        or "setupComplete_ = complete" not in ble_setup_text
        or "provisioning_.activeSessionId() == 0U" not in ble_setup_text
        or "ble_.restartActive()" not in ble_setup_text
        or "LocalControllerPairing" in ble_setup_text
    ):
        failures.append(
            f"{BLE_SETUP_LIFECYCLE}: shared BLE setup lifecycle gate missing"
        )

    ble_profile_text = strip_comments(
        BLE_MODE_PROFILE.read_text(encoding="utf-8")
    )
    if "ModeCapabilityLocalControllerPairingV1" in ble_profile_text:
        failures.append(
            f"{BLE_MODE_PROFILE}: retired local pairing capability remains"
        )
    if any(path.exists() for path in RETIRED_LOCAL_PAIRING):
        failures.append("retired LocalControllerPairing source remains")

    official_ble_text = strip_comments(
        OFFICIAL_BLE_COMPOSITION.read_text(encoding="utf-8")
    )
    if (
        "class PlatformBleOnlyComposition" not in official_ble_text
        or "BleEnrollmentApplication" not in official_ble_text
        or "BleEnrollmentGrantVerifier" not in official_ble_text
        or "DeviceAccessStore" not in official_ble_text
        or "DeviceAccessSetupCompletion" not in official_ble_text
        or "serverSignatureVerifier()" not in official_ble_text
        or "ControllerAuthTransportPolicy::EstablishDirectSecure"
        not in official_ble_text
        or "ControllerCredentialDomain::Local" in official_ble_text
        or any(
            forbidden in official_ble_text
            for forbidden in (
                "WifiLifecycle",
                "WifiCredentials",
                "PubSubClient",
                "ManagedMqtt",
                "CloudCredentialEnrollment",
                "ControllerCredentialStore",
                "EnrollmentTicket",
                "LocalSetup",
                "Ownership",
            )
        )
    ):
        failures.append(
            f"{OFFICIAL_BLE_COMPOSITION}: platform/local BLE composition isolation missing"
        )

    official_workspace_text = strip_comments(
        OFFICIAL_BLE_PRODUCT.read_text(encoding="utf-8")
    )
    if (
        official_workspace_text.count("uint8_t recordRx_") != 1
        or official_workspace_text.count("uint8_t recordTx_") != 1
        or "MutableByteSpan noiseRx()" not in official_workspace_text
        or "MutableByteSpan directRx()" not in official_workspace_text
    ):
        failures.append(
            f"{OFFICIAL_BLE_PRODUCT}: shared BLE workspace regression"
        )

    official_sources_text = "\n".join(
        strip_comments(path.read_text(encoding="utf-8"))
        for path in source_files(PRODUCT_ARDUINO)
    )
    if (
        official_sources_text.count("class ArduinoClock") != 1
        or official_sources_text.count(
            "inline BleFrameTransportConfig directBleTransportConfig"
        ) != 1
        or official_sources_text.count(
            "inline Esp32NvsBlobStoreConfig esp32BlobStorageConfig"
        ) != 1
    ):
        failures.append(
            f"{OFFICIAL_ARDUINO_RUNTIME}: duplicate official runtime helper"
        )

    official_wifi_text = strip_comments(
        OFFICIAL_WIFI_PRODUCT.read_text(encoding="utf-8")
    )
    if "BlinkerV2Advanced.h" in official_wifi_text:
        failures.append(
            f"{OFFICIAL_WIFI_PRODUCT}: Product must include exact Core contracts"
        )

    device_key_wifi_ble_text = strip_comments(
        OFFICIAL_DEVICE_KEY_WIFI_BLE.read_text(encoding="utf-8")
    )
    if any(
        required not in device_key_wifi_ble_text
        for required in (
            "DeviceAccessStore",
            "DirectBleExtension",
            "Esp32WifiBleProduct",
            "esp32BleWifiProvConfig",
            "esp32SoftApWifiProvConfig",
            "BLINKER_ESP32_PROVISIONING_BLE",
            "BLINKER_ESP32_PROVISIONING_SOFTAP",
            "ProductCapabilityNetworkReset",
            "platform_.wifiCredentials().clear()",
        )
    ):
        failures.append(
            f"{OFFICIAL_DEVICE_KEY_WIFI_BLE}: canonical DeviceKey WiFi+BLE graph missing"
        )
    retired = [str(path) for path in RETIRED_WIFI_BLE_GRAPH if path.exists()]
    if LEGACY_OFFICIAL_ARDUINO.exists():
        retired.extend(
            str(path)
            for path in LEGACY_OFFICIAL_ARDUINO.rglob("*")
            if path.is_file()
        )
    if retired:
        failures.append(
            "retired Ownership/CloudCredential WiFi+BLE graph remains: "
            + ", ".join(retired)
        )

    for wrapper, expected, is_public in OFFICIAL_BLE_WRAPPERS:
        wrapper_text = strip_comments(wrapper.read_text(encoding="utf-8"))
        forbidden = (
            "LocalBleComposition"
            if expected == "PlatformBleOnlyComposition"
            else "PlatformBleOnlyComposition"
        )
        has_public_facade = "lifecycle(facade_detail::BleProfile" in wrapper_text
        if (
            expected not in wrapper_text
            or forbidden in wrapper_text
            or has_public_facade != is_public
        ):
            failures.append(
                f"{wrapper}: BLE product wrapper boundary regression"
            )

    for path in (REPOSITORY / "examples").rglob("*.ino"):
        text = path.read_text(encoding="utf-8")
        if (
            re.search(r'#\s*include\s*<BlinkerV2Advanced\.h>', text)
            and not path.parent.name.startswith("Advanced")
        ):
            failures.append(f"{path}: public example includes Advanced entry")
    public_product_examples = {
        "WiFiQuickStart": "Blinker.h",
        "CloudSmokeTest": "Blinker.h",
        "BleQuickStart": "BlinkerBLE.h",
        "WiFiBleQuickStart": "BlinkerWiFiBLE.h",
    }
    for name, entry in public_product_examples.items():
        path = REPOSITORY / "examples" / name / f"{name}.ino"
        text = path.read_text(encoding="utf-8")
        if (
            f"#include <{entry}>" not in text
            or (entry != "Blinker.h" and "#include <Blinker.h>" in text)
            or "blinker::WiFi" in text
            or "blinker::BLE" in text
        ):
            failures.append(f"{path}: public selector/tag-free API regression")
    for path in (REPOSITORY / "tests").rglob("*.ino"):
        relative_parts = path.relative_to(REPOSITORY / "tests").parts
        if any(
            part == "build"
            or part.startswith("build-")
            or part.startswith(".build-")
            for part in relative_parts
        ):
            continue
        text = path.read_text(encoding="utf-8")
        if (
            re.search(r'#\s*include\s*<Blinker\.h>', text)
            and path.parent.name not in ("CompileFacade", "CompileTaskWiFi")
        ):
            failures.append(f"{path}: internal probe must select Advanced entry")

    for path in source_files(SOURCE_ROOT):
        if ARDUINO_ROOT in path.parents:
            continue
        text = path.read_text(encoding="utf-8")
        if FORBIDDEN_INCLUDES.search(text):
            failures.append(f"{path}: platform/third-party include in v2 core")

    for folder in NO_HEAP_FOLDERS:
        for path in source_files(folder):
            code = strip_comments(path.read_text(encoding="utf-8"))
            # C++11 deleted special members prevent accidental secret copies;
            # they are declarations, not heap deallocation expressions.
            code = re.sub(r"=\s*delete\s*;", "", code)
            for label, pattern in FORBIDDEN_NO_HEAP_TOKENS.items():
                if pattern.search(code):
                    failures.append(f"{path}: forbidden {label} in no-heap layer")

    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1

    print("device-v2 architecture checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
