"""QA-3 new internal profile only; no SDK, network or hardware operations."""
from dataclasses import replace
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests/release"))
import arduino_release_profile as release
import esp32_provisioned_task_profile as qa3
from test_cloud_task_profile import payload
from test_cloud_wifi_profile import preprocessor


class ProvisionedTaskProfile(unittest.TestCase):
    def setUp(self):
        self.transport = release.load_profile(ROOT / "profiles/cloud-beta.json")
        self.signing = release.parse_profile(payload())

    def test_new_internal_identity_keeps_public_transport_and_static_signer_only(self):
        internal = qa3.internal_profile(self.transport, self.signing)
        self.assertIsNone(internal.task_profile)
        self.assertIsNone(internal.product_profile)
        self.assertFalse(internal.cloud_task_pilot)
        self.assertFalse(internal.cloud_wifi_only)
        self.assertEqual((internal.control_host, internal.control_port), (self.transport.control_host, 443))
        self.assertTrue(internal.cloud_session_tls and internal.mqtt_tls)
        self.assertEqual(internal.p256_keys, self.signing.p256_keys)
        self.assertEqual(internal.ed25519_keys, ())
        self.assertEqual(internal.allow_plain_storage, self.signing.allow_plain_storage)

    def test_wrong_sources_or_transport_are_not_relabelled_as_provisioned(self):
        for transport, signing in [
                (replace(self.transport, product_profile=None), self.signing),
                (replace(self.transport, cloud_session_tls=False), self.signing),
                (replace(self.transport, mqtt_tls=False), self.signing),
                (replace(self.transport, control_port=80), self.signing),
                (self.transport, replace(self.signing, task_profile=None)),
                (self.transport, replace(self.signing, p256_keys=())),
                (self.transport, replace(self.signing, p256_keys=(release.ServerKey(2, self.signing.p256_keys[0].public_key),))),
                (self.transport, replace(self.signing, p256_keys=(release.ServerKey(1, bytes.fromhex(release.TEST_P256_KEY)),))),
                (self.transport, replace(self.signing, ed25519_keys=(release.ServerKey(1, bytes.fromhex(release.TEST_ED25519_KEY)),)))] :
            with self.subTest(transport=transport.control_port, signing=signing.task_profile), self.assertRaises(ValueError):
                qa3.internal_profile(transport, signing)

    def test_isolated_application_produces_exact_headers_and_regenerated_identity(self):
        with tempfile.TemporaryDirectory(prefix="qa3-profile-") as temporary:
            boundary = Path(temporary)
            library = boundary / "libraries" / "BlinkerDeviceV2"
            (library / "src").mkdir(parents=True)
            (library / "library.properties").write_text("name=fixture\nversion=1.0.0\n", encoding="utf8")
            (library / "src" / "fixture.h").write_text("// public fixture\n", encoding="utf8")
            result = qa3.apply_internal_profile(library, boundary, self.transport, self.signing)
            self.assertEqual(result["profileId"], qa3.PROFILE_ID)
            self.assertEqual(result["rootMode"], "DeviceKey+Admin")
            self.assertFalse(result["deployable"] or result["runtimeSigningMatched"] or result["formalAdmission"])
            for name in ["OfficialConfig.h", "Esp32ServerKeys.h", "PortableServerKeys.h"]:
                content = (library / qa3.PREFIX / name).read_text(encoding="utf8")
                self.assertTrue(content.startswith(qa3.graph_guard()))
                self.assertNotIn("Cloud Task pilot forbids BLE", content)
            config = (library / qa3.PREFIX / "OfficialConfig.h").read_text(encoding="utf8")
            self.assertIn("#define BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS 1U", config)
            self.assertIn("#define BLINKER_TASK_CAPACITY 4U", config)
            self.assertIn("#define BLINKER_ESP32_PROVISIONING_SOFTAP 1U", config)
            portable = (library / qa3.PREFIX / "PortableServerKeys.h").read_text(encoding="utf8")
            self.assertIn("Ed25519ServerPublicKey* const serverSigningKeys = nullptr", portable)
            files = release.configured_library_files(library, qa3.internal_profile(self.transport, self.signing))
            for name in ["OfficialConfig.h", "Esp32ServerKeys.h", "PortableServerKeys.h"]:
                files[qa3.PREFIX + name] = (library / qa3.PREFIX / name).read_bytes()
            self.assertEqual((library / release.LIBRARY_IDENTITY_HEADER).read_text(encoding="utf8"), release.render_library_identity(files))
            with self.assertRaises(ValueError):
                qa3.apply_internal_profile(library, library, self.transport, self.signing)

    def test_new_guard_rejects_conflicting_graph_and_policy_in_either_include_order(self):
        compiler = preprocessor()
        if compiler is None:
            self.skipTest("C++ preprocessor is not installed")
        with tempfile.TemporaryDirectory(prefix="qa3-guard-") as temporary:
            work = Path(temporary)
            (work / "OfficialConfig.h").write_text(qa3.graph_guard() + release.render_config(
                qa3.internal_profile(self.transport, self.signing)), encoding="utf8")
            cases = [("selected", "#define BLINKER_PRODUCT_WIFI_BLE 1\n", True),
                     ("translation-unit", "", True),
                     ("manual", "#define BLINKER_PRODUCT_WIFI 1\n", False),
                     ("BLE-product", "#define BLINKER_PRODUCT_BLE 1\n", False),
                     ("BLE-provisioner", "#define BLINKER_ESP32_PROVISIONING_BLE 1\n", False),
                     ("softap-disabled", "#define BLINKER_ESP32_PROVISIONING_SOFTAP 0\n", False),
                     ("internal-disabled", "#define BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS 0\n", False),
                     ("edge", "#define BLINKER_V2_ARDUINO_ESP32_EDGEHUB_PRODUCT_H 1\n", False),
                     ("LAN", "#define BLINKER_INTERNAL_LAN_WEBSOCKET 1\n", False),
                     ("R4", "#define BLINKER_INTERNAL_UNO_R4_TASKS 1\n", False),
                     ("slots", "#define BLINKER_TASK_CAPACITY 5\n", False),
                     ("disabled", "#define BLINKER_ENABLE_TASKS 0\n", False),
                     ("policy", "#define BLINKER_TASK_MAX_COMMIT_MS 1000\n", False),
                     ("plain", "#define BLINKER_OFFICIAL_MQTT_TLS 0\n", False)]
            for name, definitions, accepted in cases:
                for late in [False, True]:
                    source = work / "input.cpp"
                    text = '#include "OfficialConfig.h"\n' if late else ""
                    source.write_text(text + definitions + '#include "OfficialConfig.h"\n', encoding="utf8")
                    result = subprocess.run([compiler, "-std=gnu++11", "-x", "c++", "-E", "-DARDUINO_ARCH_ESP32=1",
                                             "-I", str(work), str(source)], capture_output=True, timeout=20)
                    with self.subTest(name=name, late=late):
                        self.assertEqual(result.returncode == 0, accepted, result.stderr.decode(errors="replace"))
            source.write_text('#include "OfficialConfig.h"\n', encoding="utf8")
            result = subprocess.run([compiler, "-x", "c++", "-E", "-I", str(work), str(source)], capture_output=True, timeout=20)
            self.assertNotEqual(result.returncode, 0)

    def test_public_profile_and_old_manual_guard_remain_unchanged(self):
        original = (ROOT / "profiles/cloud-beta.json").read_bytes()
        original_guard = release.cloud_task_guard()
        qa3.internal_profile(self.transport, self.signing)
        qa3.configured_files(ROOT, self.transport, self.signing)
        self.assertEqual((ROOT / "profiles/cloud-beta.json").read_bytes(), original)
        self.assertEqual(release.cloud_task_guard(), original_guard)
        self.assertIn("BLINKER_PRODUCT_WIFI_BLE", original_guard)
        self.assertIn("BLINKER_ESP32_PROVISIONING_SOFTAP", original_guard)
        self.assertFalse(self.transport.cloud_task_pilot)
        self.assertEqual(hashlib.sha256(original).hexdigest(), hashlib.sha256((ROOT / "profiles/cloud-beta.json").read_bytes()).hexdigest())

    def test_ble_is_explicit_and_keeps_the_same_transport_signer_and_isolated_boundary(self):
        internal = qa3.internal_profile(self.transport, self.signing, "ble")
        softap = qa3.internal_profile(self.transport, self.signing)
        self.assertEqual(internal, replace(softap, firmware_version=qa3.BLE_VERSION))
        with tempfile.TemporaryDirectory(prefix="da4-ble-profile-") as temporary:
            boundary = Path(temporary)
            library = boundary / "library"
            (library / "src").mkdir(parents=True)
            (library / "library.properties").write_text("name=fixture\nversion=1.0.0\n", encoding="utf8")
            result = qa3.apply_internal_profile(library, boundary, self.transport, self.signing, "ble")
            self.assertEqual(result["profileId"], qa3.BLE_PROFILE_ID)
            self.assertEqual(result["bearer"], "ble")
            self.assertEqual(result["rootMode"], "DeviceKey+Admin")
            self.assertEqual(result["p256RawSha256"], hashlib.sha256(self.signing.p256_keys[0].public_key).hexdigest())
            self.assertEqual((result["taskCapacity"], result["signatureAlgorithm"], result["p256KeyId"]), (4, 2, 1))
            for flag in ("deployable", "runtimeSigningMatched", "formalAdmission"):
                self.assertIs(result[flag], False)
            for name in ("OfficialConfig.h", "Esp32ServerKeys.h", "PortableServerKeys.h"):
                content = (library / qa3.PREFIX / name).read_text(encoding="utf8")
                self.assertTrue(content.startswith(qa3.graph_guard("ble")))
                self.assertIn("#define BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS 1U", content)
                self.assertIn("#define BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS 1U", content)
                self.assertIn("#define BLINKER_ESP32_PROVISIONING_BLE 1U", content)
                self.assertNotIn("#define BLINKER_ESP32_PROVISIONING_SOFTAP", content)
            files = qa3.configured_files(library, self.transport, self.signing, "ble")
            self.assertEqual((library / release.LIBRARY_IDENTITY_HEADER).read_text(encoding="utf8"),
                             release.render_library_identity(files))
            with self.assertRaises(ValueError):
                qa3.apply_internal_profile(library, library, self.transport, self.signing, "ble")
        for bearer in ("", "BLE", "both", "manual"):
            with self.subTest(bearer=bearer), self.assertRaises(ValueError):
                qa3.configured_files(ROOT, self.transport, self.signing, bearer)
        # The existing public/default caller remains SoftAP; no auto-selection.
        self.assertIn("#define BLINKER_ESP32_PROVISIONING_SOFTAP 1U", qa3.graph_guard())
        self.assertNotIn("#define " + qa3.BLE_INTERNAL_MACRO, qa3.graph_guard())

    def test_ble_guard_rejects_softap_or_unadmitted_selectors_in_both_include_orders(self):
        compiler = preprocessor()
        if compiler is None:
            self.skipTest("C++ preprocessor is not installed")
        with tempfile.TemporaryDirectory(prefix="da4-ble-guard-") as temporary:
            work = Path(temporary)
            (work / "OfficialConfig.h").write_text(qa3.graph_guard("ble") + release.render_config(
                qa3.internal_profile(self.transport, self.signing, "ble")), encoding="utf8")
            cases = [("selected", "#define BLINKER_PRODUCT_WIFI_BLE 1\n", True),
                     ("translation-unit", "", True),
                     ("softap", "#define BLINKER_ESP32_PROVISIONING_SOFTAP 1\n", False),
                     ("ble-disabled", "#define BLINKER_ESP32_PROVISIONING_BLE 0\n", False),
                     ("original-disabled", "#define BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS 0\n", False),
                     ("ble-internal-disabled", "#define BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS 0\n", False),
                     ("ble-internal-value", "#define BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS 2\n", False),
                     ("manual", "#define BLINKER_PRODUCT_WIFI 1\n", False),
                     ("BLE-only", "#define BLINKER_PRODUCT_BLE 1\n", False),
                     ("slots", "#define BLINKER_TASK_CAPACITY 5\n", False),
                     ("plain", "#define BLINKER_OFFICIAL_MQTT_TLS 0\n", False)]
            source = work / "input.cpp"
            for name, definitions, accepted in cases:
                for late in (False, True):
                    source.write_text(('#include "OfficialConfig.h"\n' if late else "")
                                      + definitions + '#include "OfficialConfig.h"\n', encoding="utf8")
                    result = subprocess.run([compiler, "-std=gnu++11", "-x", "c++", "-E", "-DARDUINO_ARCH_ESP32=1",
                                             "-I", str(work), str(source)], capture_output=True, timeout=20)
                    with self.subTest(name=name, late=late):
                        self.assertEqual(result.returncode == 0, accepted, result.stderr.decode(errors="replace"))
            # A BLE internal selector is also forbidden by the old SoftAP graph.
            (work / "OfficialConfig.h").write_text(qa3.graph_guard(), encoding="utf8")
            source.write_text("#define BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS 1\n"
                              '#include "OfficialConfig.h"\n', encoding="utf8")
            result = subprocess.run([compiler, "-x", "c++", "-E", "-DARDUINO_ARCH_ESP32=1",
                                     "-I", str(work), str(source)], capture_output=True, timeout=20)
            self.assertNotEqual(result.returncode, 0)

    def test_task_config_admits_ble_only_with_both_exact_internal_selectors(self):
        compiler = preprocessor()
        if compiler is None:
            self.skipTest("C++ preprocessor is not installed")
        header = ROOT / qa3.PREFIX / "TaskConfig.h"
        original = {"ARDUINO_ARCH_ESP32": 1, "BLINKER_PRODUCT_WIFI_BLE": 1,
                    qa3.INTERNAL_MACRO: 1, qa3.BLE_INTERNAL_MACRO: 1,
                    "BLINKER_ESP32_PROVISIONING_BLE": 1, "BLINKER_ENABLE_TASKS": 1,
                    "BLINKER_OFFICIAL_CLOUD_SESSION_TLS": 1, "BLINKER_OFFICIAL_MQTT_TLS": 1}
        original.update({"BLINKER_TASK_" + key: value for key, value in release.TASK_PILOT.items()})
        mutations = [("explicit-ble", {}, True),
                     ("missing-ble-selector", {qa3.BLE_INTERNAL_MACRO: None}, False),
                     ("missing-original-selector", {qa3.INTERNAL_MACRO: None}, False),
                     ("original-disabled", {qa3.INTERNAL_MACRO: 0}, False),
                     ("ble-selector-disabled", {qa3.BLE_INTERNAL_MACRO: 0}, False),
                     ("ble-selector-value", {qa3.BLE_INTERNAL_MACRO: 2}, False),
                     ("missing-ble", {"BLINKER_ESP32_PROVISIONING_BLE": None}, False),
                     ("both-bearers", {"BLINKER_ESP32_PROVISIONING_SOFTAP": 1}, False),
                     ("non-ESP32", {"ARDUINO_ARCH_ESP32": None}, False),
                     ("slots", {"BLINKER_TASK_CAPACITY": 5}, False),
                     ("no-TLS", {"BLINKER_OFFICIAL_MQTT_TLS": 0}, False),
                     ("legacy-softap", {qa3.BLE_INTERNAL_MACRO: None,
                                        "BLINKER_ESP32_PROVISIONING_BLE": None,
                                        "BLINKER_ESP32_PROVISIONING_SOFTAP": 1}, True)]
        with tempfile.TemporaryDirectory(prefix="da4-task-config-") as temporary:
            source = Path(temporary) / "input.cpp"
            for name, changes, accepted in mutations:
                definitions = {**original, **changes}
                source.write_text("".join(f"#define {key} {value}\n" for key, value in definitions.items() if value is not None)
                                  + f'#include "{header.as_posix()}"\n', encoding="utf8")
                result = subprocess.run([compiler, "-x", "c++", "-E", str(source)], capture_output=True, timeout=20)
                with self.subTest(name=name):
                    self.assertEqual(result.returncode == 0, accepted, result.stderr.decode(errors="replace"))
            source.write_text(f'#include "{header.as_posix()}"\n'
                              '#if BLINKER_TASK_CAPACITY != 0\n#error "ordinary default must stay closed"\n#endif\n', encoding="utf8")
            result = subprocess.run([compiler, "-x", "c++", "-E", str(source)], capture_output=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))

    def test_actual_facade_accepts_only_explicit_ble_or_original_admitted_graphs(self):
        compiler = preprocessor()
        if compiler is None:
            self.skipTest("C++ preprocessor is not installed")
        # The real portable Facade and its real dependencies, not a copied guard
        # or a fake header. No Arduino SDK, firmware link or hardware operation.
        header = ROOT / "src/BlinkerV2/api/BlinkerFacade.h"
        original = {"ARDUINO_ARCH_ESP32": 1, "BLINKER_PRODUCT_WIFI_BLE": 1,
                    "BLINKER_ENABLE_TASKS": 1, qa3.INTERNAL_MACRO: 1,
                    qa3.BLE_INTERNAL_MACRO: 1, "BLINKER_ESP32_PROVISIONING_BLE": 1}
        manual = {"ARDUINO_ARCH_ESP32": None, "BLINKER_PRODUCT_WIFI_BLE": None,
                  qa3.INTERNAL_MACRO: None, qa3.BLE_INTERNAL_MACRO: None,
                  "BLINKER_ESP32_PROVISIONING_BLE": None, "BLINKER_PRODUCT_WIFI": 1}
        cases = [("explicit-ble", {}, True),
                 ("original-softap", {qa3.BLE_INTERNAL_MACRO: None,
                                      "BLINKER_ESP32_PROVISIONING_BLE": None,
                                      "BLINKER_ESP32_PROVISIONING_SOFTAP": 1}, True),
                 ("ordinary-ble", {qa3.INTERNAL_MACRO: None, qa3.BLE_INTERNAL_MACRO: None}, False),
                 ("missing-ble-internal", {qa3.BLE_INTERNAL_MACRO: None}, False),
                 ("missing-original-internal", {qa3.INTERNAL_MACRO: None}, False),
                 ("original-zero", {qa3.INTERNAL_MACRO: 0}, False),
                 ("original-not-one", {qa3.INTERNAL_MACRO: 2}, False),
                 ("ble-internal-zero", {qa3.BLE_INTERNAL_MACRO: 0}, False),
                 ("ble-internal-not-one", {qa3.BLE_INTERNAL_MACRO: 2}, False),
                 ("ble-disabled", {"BLINKER_ESP32_PROVISIONING_BLE": 0}, False),
                 ("both-bearers", {"BLINKER_ESP32_PROVISIONING_SOFTAP": 1}, False),
                 ("BLE-only", {"BLINKER_PRODUCT_WIFI_BLE": None, "BLINKER_PRODUCT_BLE": 1}, False),
                 ("manual-ESP32", {**manual, "ARDUINO_ARCH_ESP32": 1}, True),
                 ("manual-unadmitted", manual, False),
                 ("R4-unadmitted", {**manual, "ARDUINO_ARCH_RENESAS_UNO": 1}, False),
                 ("R4-admitted", {**manual, "ARDUINO_ARCH_RENESAS_UNO": 1,
                                  "BLINKER_INTERNAL_UNO_R4_TASKS": 1}, True),
                 ("Wio-unadmitted", {**manual, "SEEED_WIO_TERMINAL": 1}, False),
                 ("Wio-admitted", {**manual, "SEEED_WIO_TERMINAL": 1,
                                   "BLINKER_INTERNAL_WIO_TASKS": 1}, True),
                 ("no-product-library-TU", {**manual, "BLINKER_PRODUCT_WIFI": None}, True)]
        with tempfile.TemporaryDirectory(prefix="da4-facade-profile-") as temporary:
            source = Path(temporary) / "input.cpp"
            for name, changes, accepted in cases:
                definitions = {**original, **changes}
                source.write_text("".join(f"#define {key} {value}\n" for key, value in definitions.items() if value is not None)
                                  + f'#include "{header.as_posix()}"\n', encoding="utf8")
                result = subprocess.run([compiler, "-std=gnu++11", "-x", "c++", "-E",
                                         "-I", str(ROOT / "src"), str(source)], capture_output=True, timeout=20)
                with self.subTest(name=name):
                    self.assertEqual(result.returncode == 0, accepted, result.stderr.decode(errors="replace"))

    def test_actual_wifible_root_propagates_generated_profile_before_real_facade(self):
        compiler = preprocessor()
        if compiler is None:
            self.skipTest("C++ preprocessor is not installed")
        # Actual root + generated profile + actual portable Facade/dependencies.
        # Only the native Product header is stubbed; do not predefine any product,
        # task or bearer macro on the command line to hide an entrypoint omission.
        root_header = ROOT / "src/BlinkerWiFiBLE.h"
        native_product = "BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h"
        with tempfile.TemporaryDirectory(prefix="da4-real-root-profile-") as temporary:
            work = Path(temporary)
            (work / "BlinkerWiFiBLE.h").write_bytes(root_header.read_bytes())
            product = work / native_product
            product.parent.mkdir(parents=True)
            product.write_text("// Host-only native SDK Product stub, no replacement Facade.\n"
                               "#ifndef BLINKER_PRODUCT_WIFI_BLE\n#error root must select WiFiBLE\n#endif\n",
                               encoding="utf8")
            config = work / "BlinkerV2/arduino/config/OfficialConfig.h"
            config.parent.mkdir(parents=True)
            source = work / "input.cpp"
            for bearer in ("ble", "softap"):
                files = qa3.configured_files(ROOT, self.transport, self.signing, bearer)
                config.write_bytes(files[qa3.PREFIX + "OfficialConfig.h"])
                checks = ("#if !defined(BLINKER_PRODUCT_WIFI_BLE) || BLINKER_PRODUCT_WIFI_BLE != 1\n"
                          '#error "actual root product selector missing"\n#endif\n'
                          "#if !defined(BLINKER_ENABLE_TASKS) || BLINKER_ENABLE_TASKS != 1 || "
                          "!defined(BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS) || BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS != 1\n"
                          '#error "generated task profile did not propagate"\n#endif\n')
                if bearer == "ble":
                    checks += ("#if !defined(BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS) || "
                               "BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS != 1 || "
                               "!defined(BLINKER_ESP32_PROVISIONING_BLE) || BLINKER_ESP32_PROVISIONING_BLE != 1 || "
                               "defined(BLINKER_ESP32_PROVISIONING_SOFTAP)\n"
                               '#error "explicit generated BLE profile missing"\n#endif\n')
                else:
                    checks += ("#if !defined(BLINKER_ESP32_PROVISIONING_SOFTAP) || BLINKER_ESP32_PROVISIONING_SOFTAP != 1 || "
                               "defined(BLINKER_ESP32_PROVISIONING_BLE) || defined(BLINKER_INTERNAL_ESP32_BLE_PROVISIONED_TASKS)\n"
                               '#error "original generated SoftAP profile changed"\n#endif\n')
                source.write_text('#include <BlinkerWiFiBLE.h>\n' + checks, encoding="utf8")
                result = subprocess.run([compiler, "-std=gnu++11", "-x", "c++", "-E",
                                         "-DARDUINO_ARCH_ESP32=1", "-I", str(work),
                                         "-I", str(ROOT / "src"), str(source)], capture_output=True, timeout=20)
                with self.subTest(bearer=bearer):
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    output = result.stdout.decode(errors="replace")
                    # Real emitted definitions prove profile-before-Facade order,
                    # including when no tasks were defined before the root include.
                    self.assertLess(output.index("static const char firmwareVersion[]"),
                                    output.index("struct BleProfile {}"))


if __name__ == "__main__":
    unittest.main()
