"""Finite preparation-only release-wiring checks; no compiler/SDK/hardware writes."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import prepare_r4_restricted_release as release


class RestrictedRelease(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.sdk = self.root / "sdk/1.5.3"
        self.compiler = self.root / "compiler/7-2017q4/bin"
        self.builtins = self.root / "builtin-tools"
        self.pubsub, self.crypto = self.root / "PubSubClient", self.root / "Crypto"
        self.source = self.root / "repository"
        self.output = self.root / "prepared"
        inputs = {
            self.sdk / "platform.txt": b"version=1.5.3\noriginal recipe\n",
            self.sdk / "boards.txt": b"unor4wifi.build.defines=-DNO_USB\n",
            self.sdk / "cores/arduino/Serial.cpp": b"original serial",
            self.sdk / "variants/UNOWIFIR4/libs/libfsp.a": b"original archive",
            self.source / "library.properties": b"name=BlinkerDeviceV2\nversion=0.1.0\n",
            self.source / "src/BlinkerWiFi.h": b"original public selector",
            self.source / "src/BlinkerV2/arduino/config/OfficialConfig.h": b"ordinary capacity zero",
        }
        for name in ("arm-none-eabi-gcc", "arm-none-eabi-g++", "arm-none-eabi-ar"):
            inputs[self.compiler / (name + ".exe")] = b"original compiler fixture"
        for name in release.CORE_CONTRACT_HEADERS:
            inputs[self.source / "src/BlinkerV2" / name] = ("original contract " + name).encode()
        for name in ("R4TasksQuickStart.ino", "UserCredentials.h", "README.md"):
            relative = Path("examples/R4TasksQuickStart") / name
            inputs[self.source / relative] = (ROOT / relative).read_bytes()
        for name, version in release.BUILTINS.items():
            inputs[self.builtins / name / version / (name + ".exe")] = b"original builtin fixture"
        for directory, name, version in ((self.pubsub, "PubSubClient", "2.8"), (self.crypto, "Crypto", "0.4.0")):
            inputs[directory / "library.properties"] = f"name={name}\nversion={version}\n".encode()
            inputs[directory / "src/Fixed.cpp"] = b"original dependency"
        for file, value in inputs.items():
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(value)
        self.inputs = inputs
        self.profile = self.root / "public.json"
        self.payload = json.loads((ROOT / "tests/release/development_profile.json").read_text())
        self.profile.write_text(json.dumps(self.payload))
        self.enterContext(patch.object(release, "ROOT", self.source))
        self.enterContext(patch.object(release.candidate, "source_paths", return_value={
            file.relative_to(self.source).as_posix(): file for file in inputs if file.is_relative_to(self.source)}))
        self.base = {"sdk-overlay/" + name: ("overlay " + name).encode() for name in
                     ("Modem.cpp", "Modem.h", "WiFiCommands.h", "StringHelpers.h", "Serial.cpp", "Serial.h")}
        self.base.update({"CandidateConfig.h": b"// reviewed candidate guards\n",
                          "HeapBoundary.cpp": b'#include "TaskHeapBoundary.h"\n',
                          "TaskHeapBoundary.h": b"// original thin heap adapter\n"})
        self.enterContext(patch.object(release.candidate, "expected_candidate", return_value=(
            self.base, {"linkFlags": ["-Wl,--wrap=_sbrk", "-Wl,--defsym=__blinker_r4_heap_limit=__StackTop-6144"]})))

    def call(self, operation=release.prepare, private=None):
        return operation(self.sdk, self.pubsub, self.crypto, self.compiler,
                         self.builtins, self.profile, self.output, private)

    def test_new_release_projects_the_shared_identity_and_exact_library_binding(self):
        receipt = self.call()
        library = release.configured_library(release.restricted_profile(self.profile))
        identity = release.LIB_PREFIX + release.LIBRARY_IDENTITY_HEADER
        self.assertEqual((self.output / identity).read_bytes(), library[release.LIBRARY_IDENTITY_HEADER])
        config = (self.output / "wiring/Config.h").read_text(encoding="utf-8")
        self.assertIn((self.output / identity).resolve().as_posix(), config)
        self.assertIn("#define BLINKER_INTERNAL_R4_LIBRARY_BINDING 1", config)
        for word in release.library_identity_words(library):
            self.assertIn(f"0x{word:08X}UL", config)
        self.assertEqual(self.call(release.verify), receipt)
        self.assertFalse(receipt["installedSdkModified"])
        self.assertFalse(receipt["hardwareWritten"])
        self.assertFalse(receipt["formalTaskAdmission"])

    def test_builtin_tools_are_explicit_pinned_copied_and_bound(self):
        file = self.builtins / "ctags/5.8-arduino11/ctags.exe"
        original = file.read_bytes()
        file.unlink()
        with self.assertRaisesRegex(ValueError, "missing pinned builtin tool"):
            self.call()
        self.assertFalse(self.output.exists())
        file.write_bytes(original)
        receipt = self.call()
        self.assertEqual(receipt["builtinVersions"], release.BUILTINS)
        self.assertEqual(self.call(release.verify), receipt)
        for name, version in release.BUILTINS.items():
            key = f"cli-data/packages/builtin/tools/{name}/{version}/{name}.exe"
            self.assertEqual((self.output / key).read_bytes(), b"original builtin fixture")
            self.assertIn(key, receipt["outputSha256"])
        copied = self.output / "cli-data/packages/builtin/tools/ctags/5.8-arduino11/ctags.exe"
        copied.write_bytes(b"altered builtin")
        with self.assertRaisesRegex(ValueError, "restricted stage input mismatch"):
            self.call(release.verify)
        self.assertEqual(file.read_bytes(), original)

    def test_include_options_keep_quoted_paths_as_separate_cli_tokens(self):
        self.output = self.root / "prepared stage with spaces"
        receipt = self.call()
        for language in ("c", "cpp", "S"):
            argument = next(value for value in receipt["cliArguments"]
                            if value.startswith(f"compiler.{language}.extra_flags="))
            self.assertNotIn('-I"', argument)
            quoted = '"' + str(self.output / "private").replace('\\', '/') + '"'
            # Arduino CLI removes standalone quote pairs, but does not
            # consistently unquote a path concatenated to its -I option.
            self.assertIn("-I " + quoted, argument)

    def test_projection_includes_are_left_to_cli_library_discovery(self):
        receipt = self.call()
        projection = str(self.output / (release.LIB_PREFIX + "src")).replace('\\', '/')
        for language in ("c", "cpp", "S"):
            argument = next(value for value in receipt["cliArguments"]
                            if value.startswith(f"compiler.{language}.extra_flags="))
            self.assertNotIn(projection, argument)
            self.assertEqual(argument.count("-I "), 1)  # Private credentials only.
            self.assertIn("-include", argument)
        self.assertTrue((self.output / (release.LIB_PREFIX + "library.properties")).is_file())
        self.assertTrue((self.output / (release.LIB_PREFIX + "src/BlinkerWiFi.h")).is_file())
        self.assertIn("#include <BlinkerWiFi.h>", (self.output / "sketch/R4Restricted/R4Restricted.ino").read_text())
        self.assertEqual((self.output / "sketch/R4Restricted/HeapBoundary.cpp").read_bytes(), self.base["HeapBoundary.cpp"])

    def test_core_mirrors_only_four_original_contract_headers(self):
        receipt = self.call()
        prefix = release.SDK_PREFIX + "cores/arduino/BlinkerV2/"
        mirrored = {key[len(prefix):] for key in receipt["outputSha256"] if key.startswith(prefix)}
        self.assertEqual(mirrored, set(release.CORE_CONTRACT_HEADERS))
        self.assertEqual(receipt["coreContractHeaders"], list(release.CORE_CONTRACT_HEADERS))
        self.assertFalse(any(name.endswith(".cpp") or name == "BlinkerWiFi.h" for name in mirrored))
        for name in mirrored:
            self.assertEqual((self.output / (prefix + name)).read_bytes(),
                             (self.source / "src/BlinkerV2" / name).read_bytes())
        flags = [value for value in receipt["cliArguments"] if ".extra_flags=" in value]
        self.assertFalse(any("cores/arduino/BlinkerV2" in value for value in flags))

    def test_public_reference_is_the_single_graph_and_cannot_shadow_private_credentials(self):
        receipt = self.call()
        relative = "examples/R4TasksQuickStart/"
        reference = self.source / relative
        self.assertEqual((self.output / "sketch/R4Restricted/R4Restricted.ino").read_bytes(),
                         (reference / "R4TasksQuickStart.ino").read_bytes())
        self.assertFalse((self.output / "sketch/R4Restricted/UserCredentials.h").exists())
        for name in ("R4TasksQuickStart.ino", "UserCredentials.h", "README.md"):
            self.assertEqual(receipt["inputSha256"][relative + name], release.digest(reference / name))
        self.assertFalse((release.TEMPLATES / "R4Restricted.ino").exists())
        self.assertEqual((self.output / "private/UserCredentials.h").read_bytes(),
                         release.credentials()["private/UserCredentials.h"])
        # Changes to the user-facing contract must invalidate an old receipt,
        # even when the matching generated Sketch has not been changed.
        (reference / "README.md").write_bytes(b"changed support contract")
        with self.assertRaisesRegex(ValueError, "receipt/source mismatch"):
            self.call(release.verify)

    def test_fixed_stage_uses_existing_renderers_overlay_and_heap_without_running_build(self):
        with patch("subprocess.run") as run:
            receipt = self.call()
            self.assertEqual(self.call(release.verify), receipt)
            run.assert_not_called()
        self.assertEqual(receipt["state"], "prepared-not-linked")
        for key in ("linked", "formalTaskAdmission", "hardwareWritten", "ordinaryZipModified", "installedSdkModified"):
            self.assertIs(receipt[key], False)
        for file, value in self.inputs.items():
            self.assertEqual(file.read_bytes(), value)
        self.assertEqual((self.output / (release.SDK_PREFIX + "cores/arduino/Serial.cpp")).read_bytes(), self.base["sdk-overlay/Serial.cpp"])
        self.assertEqual((self.output / (release.SDK_PREFIX + "variants/UNOWIFIR4/libs/libfsp.a")).read_bytes(), b"original archive")
        self.assertEqual((self.output / "sketch/R4Restricted/HeapBoundary.cpp").read_bytes(), self.base["HeapBoundary.cpp"])
        self.assertEqual(sum(Path(key).name == "HeapBoundary.cpp" for key in receipt["outputSha256"]), 1)
        self.assertFalse((self.output / (release.LIB_PREFIX + "src/restricted")).exists())
        config = (self.output / "wiring/Config.h").read_text()
        for macro, value in release.FIXED.items():
            self.assertIn(f"#define {macro} {value}\n", config)
        args = receipt["cliArguments"]
        for language in ("c", "cpp", "S"):
            flags = next(arg for arg in args if arg.startswith(f"compiler.{language}.extra_flags="))
            self.assertIn("-include", flags)
            self.assertIn("wiring/Config.h", flags)
            self.assertIn("private", flags)
        self.assertIn("__StackTop-6144", " ".join(args))
        self.assertEqual(args.count("--jobs"), 1)
        self.assertIn("sketchbook", (self.output / "cli.yaml").read_text())
        sketch = (self.output / "sketch/R4Restricted/R4Restricted.ino").read_text()
        self.assertIn("Blinker.enableTasks(power, duty)", sketch)
        for excluded in ("Serial.", "Observer", "FAULT_GATE", "startTask", "taskStart"):
            self.assertNotIn(excluded, sketch)

    def test_private_json_is_data_only_and_never_in_public_receipt(self):
        path = self.root / "secret.json"
        values = {"device_key": "SECRET\"\\KEY", "wifi_ssid": "带翅膀的蜗牛", "wifi_password": "PRIVATE_PASSWORD"}
        path.write_text(json.dumps(values))
        receipt = self.call(private=path)
        public = json.dumps(receipt)
        for secret in (*values.values(), str(path)):
            self.assertNotIn(secret, public)
        self.assertFalse(any(key.startswith("private/") for key in receipt["outputSha256"]))
        self.assertEqual(self.call(release.verify, path), receipt)
        header = (self.output / "private/UserCredentials.h").read_text()
        self.assertIn("\\345\\270", header)
        self.assertNotIn("SECRET", header)
        values["callback"] = "arbitrary code"
        path.write_text(json.dumps(values))
        with self.assertRaises(ValueError):
            self.call(release.verify, path)

    def test_every_prepared_public_file_is_bound_and_extra_source_is_rejected(self):
        self.call()
        for name in ("wiring/Config.h", release.SDK_PREFIX + "cores/arduino/Serial.cpp", ".gitignore",
                     "sketch/R4Restricted/R4Restricted.ino", "release-plan.json"):
            path = self.output / name
            original = path.read_bytes()
            path.write_bytes(original + b"changed")
            with self.assertRaises((ValueError, json.JSONDecodeError)):
                self.call(release.verify)
            path.write_bytes(original)
        extra = self.output / "sketch/Injected.cpp"
        extra.write_text("void injected() {}")
        with self.assertRaises(ValueError):
            self.call(release.verify)

    def test_unknown_tls_task_or_private_profile_fields_reject_before_output(self):
        changed = []
        for field, value in (("task_profile", "esp32-wifi-t2-lab-v1"), ("device_key", "private")):
            payload = copy.deepcopy(self.payload)
            payload[field] = value
            changed.append(payload)
        payload = copy.deepcopy(self.payload)
        payload["control"]["cloud_session_tls"] = True
        payload["control"]["cloud_session_ca_pem"] = "-----BEGIN CERTIFICATE-----"
        changed.append(payload)
        for payload in changed:
            self.profile.write_text(json.dumps(payload))
            with self.assertRaises(ValueError):
                self.call()
            self.assertFalse(self.output.exists())

    def test_wrong_core_dependency_and_output_overlap_fail_closed(self):
        file = self.sdk / "platform.txt"
        file.write_text("version=1.5.4\n")
        with self.assertRaises(ValueError):
            self.call()
        file.write_bytes(self.inputs[file])
        file = self.pubsub / "library.properties"
        file.write_text("name=PubSubClient\nversion=2.9\n")
        with self.assertRaises(ValueError):
            self.call()
        file.write_bytes(self.inputs[file])
        self.output = self.sdk / "candidate"
        with self.assertRaises(ValueError):
            self.call()
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
