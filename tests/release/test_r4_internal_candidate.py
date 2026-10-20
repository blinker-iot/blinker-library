"""Finite internal R4 preparation checks; no compiler, SDK writes or hardware."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import prepare_r4_internal_candidate as candidate


def digest(content):
    return hashlib.sha256(content).hexdigest()


class InternalCandidatePreparation(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.sdk = self.root / "sdk"
        self.modem = self.sdk / "libraries/WiFiS3/src"
        self.core = self.sdk / "cores/arduino"
        self.pubsub = self.root / "PubSubClient/src"
        self.modem.mkdir(parents=True)
        self.core.mkdir(parents=True)
        self.pubsub.mkdir(parents=True)
        self.original = {}
        for name in ("Modem.cpp", "Modem.h", "WiFiCommands.h", "StringHelpers.h",
                     "Serial.cpp", "Serial.h"):
            path = (self.core if name.startswith("Serial.") else self.modem) / name
            path.write_bytes(("untouched fixture " + name).encode())
            self.original[path] = path.read_bytes()
        self.pubsub_original = {}
        for name in ("PubSubClient.h", "PubSubClient.cpp"):
            path = self.pubsub / name
            path.write_bytes(("untouched fixture " + name).encode())
            self.pubsub_original[path] = path.read_bytes()
        self.generated_overlay = {path.name: b"generated fixture " + path.name.encode()
                                  for path in self.original}
        self.overlay_receipt = {
            "schema": "internal-r4-modem-response-overlay/2", "core": "1.5.3",
            "sourceSha256": {path.name: digest(value)
                             for path, value in self.original.items()},
            "overlaySha256": {name: digest(value)
                              for name, value in self.generated_overlay.items()},
            "ordinaryProductEnabled": False, "hardwareStopValidated": False,
            "oomRecoverable": False, "ordinaryZipDependency": False,
        }

        def expected_overlay(modem, output):
            self.assertEqual(modem.resolve(), self.modem.resolve())
            self.assertEqual(output.name, "sdk-overlay")
            # This fixture replaces only the already separately tested pinned
            # overlay generator, not the candidate's generated-input verifier.
            return dict(self.generated_overlay), dict(self.overlay_receipt)

        self.expected = self.enterContext(patch.object(
            candidate.overlay, "expected_overlay", side_effect=expected_overlay))

        def dependency_inputs(modem, pubsub):
            self.assertEqual(modem.resolve(), self.modem.resolve())
            self.assertEqual(pubsub.resolve(), self.pubsub.resolve())
            # Dependency validation is separately pinned by the real tool.
            # Only temporary fixture bytes stand in for those read-only inputs.
            return dict({"sdk/" + path.relative_to(self.sdk).as_posix(): path.read_bytes()
                         for path in self.original},
                        **{"pubsub/" + path.name: path.read_bytes()
                           for path in self.pubsub_original})

        self.read_dependencies = candidate.dependency_inputs
        self.dependencies = self.enterContext(patch.object(
            candidate, "dependency_inputs", side_effect=dependency_inputs))
        self.output = self.root / "candidate"

    def assert_original_sdk_untouched(self):
        self.assertEqual({path: path.read_bytes() for path in self.original}, self.original)
        self.assertEqual({path for path in self.sdk.rglob("*") if path.is_file()},
                         set(self.original))
        self.assertEqual({path: path.read_bytes() for path in self.pubsub_original},
                         self.pubsub_original)
        self.assertEqual({path for path in self.pubsub.rglob("*") if path.is_file()},
                         set(self.pubsub_original))

    def test_prepare_verify_is_internal_not_release_or_runtime_admission(self):
        with patch("subprocess.run") as process:
            receipt = candidate.prepare(self.modem, self.output, self.pubsub)
            self.assertEqual(candidate.verify(self.modem, self.output, self.pubsub), receipt)
            process.assert_not_called()
        self.assertEqual(receipt["schema"], "internal-r4-candidate/2")
        self.assertEqual(receipt["state"], "internal-offline-only")
        for field in ("ordinaryZipDependency", "formalTaskAdmission", "hardwareStopValidated",
                      "oomRecoverable", "linked", "hardwareWritten"):
            self.assertIs(receipt[field], False, field)
        self.assertEqual(json.loads((self.output / "candidate.json").read_text()), receipt)
        self.assert_original_sdk_untouched()

    def test_fixed_config_and_heap_wrapper_do_not_enable_tasks(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        config = (self.output / "CandidateConfig.h").read_text()
        for name, value in (("MODEM_COMMAND_BYTES", 32), ("MODEM_RESPONSE_BYTES", 1024),
                            ("MODEM_RPC_MS", 1000), ("HEAP_BOUNDARY", 1)):
            self.assertRegex(config, r"#define BLINKER_INTERNAL_R4_" + name + r"\s+" + str(value))
        self.assertNotIn("BLINKER_ENABLE_TASKS", config)
        self.assertNotIn("TASK_CAPACITY", config)
        self.assertNotIn("GateConfig.h", config)
        wrapper = (self.output / "HeapBoundary.cpp").read_text()
        self.assertIn('"TaskHeapBoundary.h"', wrapper)
        self.assertNotIn("__wrap__sbrk", wrapper)  # Reuse the original implementation.
        self.assertTrue((self.output / "TaskHeapBoundary.h").is_file())
        self.assertIn("-include", receipt["compileFlags"])
        self.assertIn(str(self.output / "CandidateConfig.h"), receipt["compileFlags"])
        self.assertIn("-Wl,--wrap=_sbrk", receipt["linkFlags"])
        self.assertIn("-Wl,--defsym=__blinker_r4_heap_limit=__StackTop-6144",
                      receipt["linkFlags"])
        self.assertNotIn("TASK_CAPACITY", " ".join(receipt["compileFlags"]))
        self.assert_original_sdk_untouched()

    def test_candidate_frame_guard_freezes_reviewed_mqtt_workset(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        config = (self.output / "CandidateConfig.h").read_text()
        self.assertIn("#if defined(BLINKER_DEVICE_FRAME_SIZE) && BLINKER_DEVICE_FRAME_SIZE != 256", config)
        self.assertIn('#error "R4 candidate workset requires the reviewed frame size"', config)
        self.assertIn("#ifndef BLINKER_DEVICE_FRAME_SIZE\n#define BLINKER_DEVICE_FRAME_SIZE 256\n", config)
        profile = receipt["allocationWorkset"]["profile"]
        self.assertEqual(profile["frameBytes"], 256)
        self.assertEqual(profile["initialMqttBytes"], 361)
        self.assertTrue(receipt["allocationWorkset"]["nativeReallocGrowthAvoided"])
        self.assertNotIn("BLINKER_ENABLE_TASKS", config)
        self.assertNotIn("TASK_CAPACITY", config)
        self.assertFalse(receipt["formalTaskAdmission"])
        self.assert_original_sdk_untouched()

    def test_candidate_frame_token_matches_bound_small_profile(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        name = "src/BlinkerV2/core/ResourceProfile.h"
        source = candidate.source_paths()[name].read_bytes()
        self.assertEqual(receipt["sourceSha256"][name], digest(source))
        small_profile = source.decode().split(
            "#if BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_SMALL", 1)[1].split(
            "#elif", 1)[0]
        pattern = r"^#define\s+BLINKER_DEVICE_FRAME_SIZE\s+(\S+)\s*$"
        profile_token = re.search(pattern, small_profile, re.MULTILINE).group(1)
        config = (self.output / "CandidateConfig.h").read_text()
        candidate_token = re.search(pattern, config, re.MULTILINE).group(1)
        self.assertEqual(profile_token, "256")
        self.assertEqual(candidate_token, profile_token)
        self.assert_original_sdk_untouched()

    def test_compile_flags_do_not_shadow_reviewed_source_projection(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        self.assertEqual(receipt["compileFlags"], [
            "-include", str(self.output / "CandidateConfig.h"),
            "-I" + str(self.output / "sdk-overlay")])
        self.assertNotIn("-I" + str(candidate.ROOT / "src"), receipt["compileFlags"])
        self.assertFalse(receipt["linked"])
        self.assertFalse(receipt["formalTaskAdmission"])
        self.assert_original_sdk_untouched()

    def test_build_requires_explicit_complete_reviewed_projection_and_consumers(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        steps = "\n".join(receipt["requiredBuildSteps"])
        for requirement in ("complete reviewed source projection/src",
                            "prioritize its include path",
                            "does not select or supply that projection",
                            "private OfficialConfig.h overrides",
                            "include missing UnoR4HeapBoundary.h helpers",
                            "without replacing its private configuration",
                            "every actual .d consumer", "not only Sketch",
                            "public placeholder"):
            self.assertIn(requirement, steps)
        self.assertFalse(receipt["linked"])
        self.assertFalse(receipt["formalTaskAdmission"])
        self.assert_original_sdk_untouched()

    def test_heap_wrapper_statically_binds_actual_topic_and_frame_budget(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        wrapper = (self.output / "HeapBoundary.cpp").read_text()
        self.assertIn("#include <BlinkerV2/core/ResourceProfile.h>", wrapper)
        self.assertIn("#include <BlinkerV2/control/DeviceKeySessionContract.h>", wrapper)
        self.assertIn("static_assert(BLINKER_DEVICE_FRAME_SIZE + blinker::kDeviceKeyMaximumTopicSize + 9U == MQTT_MAX_PACKET_SIZE,", wrapper)
        self.assertIn('"R4 initial MQTT buffer must match the reviewed packet budget"', wrapper)
        profile = receipt["allocationWorkset"]["profile"]
        self.assertEqual(profile["frameBytes"] + profile["topicBytes"] + profile["mqttHeaderBytes"],
                         receipt["allocationWorkset"]["initialMqttPacketBytes"])
        self.assertNotIn("BLINKER_ENABLE_TASKS", wrapper)
        self.assertNotIn("TASK_CAPACITY", wrapper)
        self.assertNotIn("__wrap__sbrk", wrapper)  # Still only includes the original adapter.
        self.assert_original_sdk_untouched()

    def test_exact_overlay_and_every_generated_file_are_bound(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        files, expected = candidate.expected_candidate(self.modem, self.output, self.pubsub)
        self.assertEqual(receipt, expected)
        self.assertEqual(set(receipt["outputSha256"]), set(files))
        for name, content in files.items():
            self.assertEqual((self.output / name).read_bytes(), content)
            self.assertEqual(receipt["outputSha256"][name], digest(content))
        for name, content in self.generated_overlay.items():
            self.assertEqual(files["sdk-overlay/" + name], content)
        self.assert_original_sdk_untouched()

    def test_pinned_dependencies_are_bound_but_not_copied(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        inputs = self.dependencies(self.modem, self.pubsub)
        self.assertEqual(receipt["dependencySha256"],
                         {name: digest(content) for name, content in inputs.items()})
        self.assertEqual(receipt["dependencyDirectory"], str(self.pubsub.resolve()))
        self.assertTrue(receipt["allocationWorkset"]["nativeReallocGrowthAvoided"])
        self.assertEqual(receipt["allocationWorkset"]["initialMqttPacketBytes"], 361)
        files, _ = candidate.expected_candidate(self.modem, self.output, self.pubsub)
        self.assertFalse(any(Path(name).name.startswith("PubSubClient.") for name in files))
        self.assertTrue(any("PubSubClient.cpp" in step for step in receipt["requiredBuildSteps"]))
        self.assertTrue(any("User buffer resizing" in scope for scope in receipt["unsupported"]))
        self.assert_original_sdk_untouched()

    def test_dependency_reader_rejects_changed_bytes_without_real_sdk_edits(self):
        inputs = self.dependencies(self.modem, self.pubsub)
        expected = {name: digest(content) for name, content in inputs.items()}
        with patch.object(candidate, "DEPENDENCY_SHA256", expected):
            self.assertEqual(self.read_dependencies(self.modem, self.pubsub), inputs)
            # Mock reads only; neither temporary fixtures nor installed SDK
            # bytes are changed while exercising the real pinned-input reader.
            for name in inputs:
                with self.subTest(name=name):
                    path = (self.sdk / name[4:] if name.startswith("sdk/")
                            else self.pubsub / name[7:])
                    original_read = Path.read_bytes

                    def read_bytes(file):
                        content = original_read(file)
                        return content + b" changed dependency" if file == path else content

                    with patch.object(Path, "read_bytes", read_bytes), self.assertRaises(ValueError):
                        self.read_dependencies(self.modem, self.pubsub)
        self.assert_original_sdk_untouched()

    def test_dependency_directory_cannot_be_changed_after_preparation(self):
        candidate.prepare(self.modem, self.output, self.pubsub)
        inputs = self.dependencies(self.modem, self.pubsub)
        other = self.root / "other/PubSubClient/src"
        with patch.object(candidate, "dependency_inputs", return_value=inputs), self.assertRaises(ValueError):
            candidate.verify(self.modem, self.output, other)
        self.assert_original_sdk_untouched()

    def test_dependency_input_mutations_invalidate_without_real_sdk_edits(self):
        candidate.prepare(self.modem, self.output, self.pubsub)
        inputs = self.dependencies(self.modem, self.pubsub)
        for name in inputs:
            with self.subTest(name=name), patch.object(candidate, "dependency_inputs", return_value={
                    **inputs, name: inputs[name] + b"\nchanged temporary dependency input"}):
                with self.assertRaises(ValueError):
                    candidate.verify(self.modem, self.output, self.pubsub)
        self.assert_original_sdk_untouched()

    def test_editable_dependency_receipt_cannot_replace_pinned_inputs(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        name = next(iter(receipt["dependencySha256"]))
        receipt["dependencySha256"][name] = digest(b"self-declared different dependency")
        (self.output / "candidate.json").write_text(json.dumps(receipt))
        with self.assertRaises(ValueError):
            candidate.verify(self.modem, self.output, self.pubsub)
        self.assert_original_sdk_untouched()

    def test_reusing_an_output_never_overwrites_existing_content(self):
        candidate.prepare(self.modem, self.output, self.pubsub)
        files = {path: path.read_bytes() for path in self.output.rglob("*") if path.is_file()}
        with self.assertRaises(FileExistsError):
            candidate.prepare(self.modem, self.output, self.pubsub)
        self.assertEqual({path: path.read_bytes() for path in files}, files)
        self.assert_original_sdk_untouched()

    def test_output_inside_original_sdk_is_rejected_before_creation(self):
        for output in (self.modem / "candidate", self.core / "candidate", self.sdk / "candidate",
                       self.pubsub / "candidate"):
            with self.subTest(output=output), self.assertRaises(ValueError):
                candidate.prepare(self.modem, output, self.pubsub)
            self.assertFalse(output.exists())
        self.assert_original_sdk_untouched()

    def test_post_prepare_output_mutation_is_rejected(self):
        files, _ = candidate.expected_candidate(self.modem, self.output, self.pubsub)
        for index, name in enumerate(files):
            with self.subTest(name=name):
                output = self.root / ("mutation-" + str(index))
                candidate.prepare(self.modem, output, self.pubsub)
                path = output / name
                path.write_bytes(path.read_bytes() + b"\nchanged generated input")
                with self.assertRaises(ValueError):
                    candidate.verify(self.modem, output, self.pubsub)
        self.assert_original_sdk_untouched()

    def test_editable_receipt_cannot_authorize_changed_output(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        name = "sdk-overlay/Modem.cpp"
        output = self.output / name
        output.write_bytes(b"different generated parser")
        receipt["outputSha256"][name] = digest(output.read_bytes())
        (self.output / "candidate.json").write_text(json.dumps(receipt))
        with self.assertRaises(ValueError):
            candidate.verify(self.modem, self.output, self.pubsub)

    def test_receipt_cannot_claim_formal_support(self):
        receipt = candidate.prepare(self.modem, self.output, self.pubsub)
        receipt["formalTaskAdmission"] = True
        (self.output / "candidate.json").write_text(json.dumps(receipt))
        with self.assertRaises(ValueError):
            candidate.verify(self.modem, self.output, self.pubsub)

    def test_bound_source_changes_invalidate_candidate_without_real_source_edits(self):
        sources = candidate.source_paths()
        self.assertTrue(sources)
        name = next(iter(sources))
        substitute = self.root / "isolated-source"
        substitute.write_bytes(sources[name].read_bytes())
        with patch.object(candidate, "source_paths", return_value=dict(sources, **{name: substitute})):
            candidate.prepare(self.modem, self.output, self.pubsub)
            substitute.write_bytes(substitute.read_bytes() + b"\nchanged bound source")
            with self.assertRaises(ValueError):
                candidate.verify(self.modem, self.output, self.pubsub)
        self.assert_original_sdk_untouched()

    def test_cli_has_no_limit_override_and_requires_original_sdk(self):
        for arguments in (("--output", str(self.output)),
                          ("--modem-dir", str(self.modem), "--output", str(self.output)),
                          ("--modem-dir", str(self.modem), "--output", str(self.output),
                           "--pubsub-dir", str(self.pubsub),
                           "--response-bytes", "2048")):
            with self.subTest(arguments=arguments):
                result = subprocess.run([sys.executable, candidate.__file__, *arguments],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 2)
                self.assertFalse(self.output.exists())
        self.assert_original_sdk_untouched()

    def test_config_rejects_conflicting_defines_without_hidden_task_enable(self):
        candidate.prepare(self.modem, self.output, self.pubsub)
        config = (self.output / "CandidateConfig.h").read_text()
        for name, value in (("MODEM_COMMAND_BYTES", 32), ("MODEM_RESPONSE_BYTES", 1024),
                            ("MODEM_RPC_MS", 1000), ("HEAP_BOUNDARY", 1)):
            macro = "BLINKER_INTERNAL_R4_" + name
            self.assertIn(f"#if defined({macro}) && {macro} != {value}", config)
            self.assertIn(f"#ifndef {macro}\n#define {macro} {value}U", config)
        self.assertIn("#if defined(MQTT_MAX_PACKET_SIZE) && MQTT_MAX_PACKET_SIZE != 361", config)
        self.assertIn("#ifndef MQTT_MAX_PACKET_SIZE\n#define MQTT_MAX_PACKET_SIZE 361U", config)
        self.assertEqual(config.count('#error "Conflicting internal R4 candidate configuration"'), 4)
        self.assertIn('#error "R4 candidate requires consistent initial MQTT packet size"', config)
        for target in ("ARDUINO_UNOWIFIR4", "ARDUINO_ARCH_RENESAS_UNO", "NO_USB"):
            self.assertIn("!defined(" + target + ")", config)

    def test_unlisted_output_sources_cannot_become_candidate_inputs(self):
        for index, name in enumerate(("Unexpected.cpp", "sdk-overlay/OldSerial.cpp")):
            with self.subTest(name=name):
                output = self.root / ("extra-" + str(index))
                candidate.prepare(self.modem, output, self.pubsub)
                (output / name).write_bytes(b"unreviewed extra translation unit")
                with self.assertRaises(ValueError):
                    candidate.verify(self.modem, output, self.pubsub)
        self.assert_original_sdk_untouched()


if __name__ == "__main__":
    unittest.main()
