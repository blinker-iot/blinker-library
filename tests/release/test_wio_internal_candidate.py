"""Bounded preparation tests; no compiler, SDK writes or runtime/device I/O."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import prepare_wio_internal_candidate as candidate


class WioInternalCandidate(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.sdk = self.root / "sdk"
        self.sdk.mkdir()
        self.pubsub = self.root / "PubSubClient/src"
        self.pubsub.mkdir(parents=True)
        self.inputs = {name: ("original temporary fixture " + name).encode()
                       for name in ("PubSubClient.h", "PubSubClient.cpp")}
        for name, content in self.inputs.items():
            (self.pubsub / name).write_bytes(content)
        self.enterContext(patch.object(candidate, "DEPENDENCY_SHA256", {
            name: hashlib.sha256(content).hexdigest() for name, content in self.inputs.items()
        }))
        self.output = self.root / "candidate"

    def prepare(self, output=None):
        return candidate.prepare(self.pubsub, output or self.output, self.sdk)

    def verify(self, output=None):
        return candidate.verify(self.pubsub, output or self.output, self.sdk)

    def assert_originals_untouched(self):
        self.assertEqual(list(self.sdk.rglob("*")), [])
        self.assertEqual({file.name: file.read_bytes() for file in self.pubsub.iterdir()}, self.inputs)

    def test_prepare_verify_is_only_two_offline_files_not_admission(self):
        receipt = self.prepare()
        self.assertEqual(self.verify(), receipt)
        self.assertEqual({file.name for file in self.output.iterdir()}, {"CandidateConfig.h", "candidate.json"})
        self.assertEqual(receipt["schema"], "internal-wio-candidate/1")
        self.assertEqual(receipt["state"], "internal-offline-only")
        self.assertEqual(receipt["requiredCore"], "1.8.5")
        for name in ("sdkVerified", "buildLinked", "ordinaryZipDependency", "formalTaskAdmission",
                     "hardwareStopValidated", "hardwareWritten", "oomRecoverable"):
            self.assertIs(receipt[name], False, name)
        self.assertEqual(receipt["compileFlags"], ["-include", str(self.output / "CandidateConfig.h")])
        self.assert_originals_untouched()

    def test_external_task_graph_is_required_not_enabled_by_header(self):
        self.prepare()
        config = (self.output / "CandidateConfig.h").read_text()
        for macro, value in (("BLINKER_ENABLE_TASKS", 1), ("BLINKER_INTERNAL_WIO_TASKS", 1),
                             ("BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED", 1), ("BLINKER_TASK_CAPACITY", 4)):
            self.assertIn(f"#if !defined({macro}) || {macro} != {value}", config)
            self.assertNotRegex(config, r"#\s*define\s+" + macro + r"\b")
        self.assertIn("!defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)", config)
        for macro in ("BLINKER_PRODUCT_BLE", "BLINKER_PRODUCT_WIFI_BLE", "BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL",
                      "BLINKER_INTERNAL_LAN_WEBSOCKET"):
            self.assertIn("defined(" + macro + ")", config)

    def test_small_frame_packet_and_plain_defaults_reject_conflicts(self):
        self.prepare()
        config = (self.output / "CandidateConfig.h").read_text()
        for macro, value in (("BLINKER_RESOURCE_PROFILE", "1"), ("BLINKER_DEVICE_FRAME_SIZE", "256"),
                             ("MQTT_MAX_PACKET_SIZE", "361U"), ("BLINKER_OFFICIAL_CLOUD_SESSION_TLS", "0"),
                             ("BLINKER_OFFICIAL_MQTT_TLS", "0")):
            self.assertIn(f"#if defined({macro}) && {macro} != {value}", config)
            self.assertIn(f"#ifndef {macro}\n#define {macro} {value}\n", config)
        profile = candidate.source_paths()["src/BlinkerV2/core/ResourceProfile.h"].read_text()
        self.assertIn("#define BLINKER_DEVICE_FRAME_SIZE 256\n", profile)

    def test_contract_requires_actual_all_consumers_and_constructor_not_zero_realloc(self):
        receipt = self.prepare()
        build = " ".join(receipt["requiredBuildSteps"])
        post = " ".join(receipt["requiredPostLinkEvidence"])
        for phrase in ("every actual .d consumer", "PubSubClient.cpp", "Sketch-only define is insufficient",
                       "missing, conflicting or wrong-graph", "verifies no SDK version"):
            self.assertIn(phrase, build)
        for phrase in ("constructor requests 361", "adapter requests the same 361", "no connection buffer growth",
                       "still calls native realloc", "does not mean zero realloc", "not physical proof"):
            self.assertIn(phrase, post)
        self.assertNotIn("GateConfig", " ".join(receipt["compileFlags"]))
        self.assert_originals_untouched()

    def test_dependency_reader_rejects_changed_native_source(self):
        self.assertEqual(candidate.dependency_inputs(self.pubsub), self.inputs)
        original_read = Path.read_bytes
        for name in self.inputs:
            def changed_read(file):
                content = original_read(file)
                return content + b"changed" if file == self.pubsub / name else content
            with self.subTest(name=name), patch.object(Path, "read_bytes", changed_read):
                with self.assertRaisesRegex(ValueError, "unreviewed PubSubClient"):
                    candidate.dependency_inputs(self.pubsub)
        self.assert_originals_untouched()

    def test_every_public_source_and_dependency_digest_is_bound(self):
        receipt = self.prepare()
        self.assertEqual(receipt["sourceSha256"], {
            name: candidate.sha256(file.read_bytes()) for name, file in candidate.source_paths().items()
        })
        self.assertEqual(receipt["dependencySha256"], {
            name: candidate.sha256(content) for name, content in self.inputs.items()
        })
        self.assertEqual(receipt["outputSha256"], {"CandidateConfig.h": candidate.sha256((self.output / "CandidateConfig.h").read_bytes())})

    def test_source_change_invalidates_receipt_without_mutating_public_source(self):
        files = candidate.source_paths()
        name = next(iter(files))
        substitute = self.root / "public-input-fixture"
        substitute.write_bytes(files[name].read_bytes())
        with patch.object(candidate, "source_paths", return_value={**files, name: substitute}):
            self.prepare()
            substitute.write_bytes(substitute.read_bytes() + b"changed")
            with self.assertRaisesRegex(ValueError, "receipt/source mismatch"):
                self.verify()

    def test_output_missing_changed_or_extra_is_rejected(self):
        for index, mode in enumerate(("missing", "changed", "extra")):
            output = self.root / ("candidate-" + str(index))
            self.prepare(output)
            if mode == "missing":
                (output / "CandidateConfig.h").unlink()
            elif mode == "changed":
                (output / "CandidateConfig.h").write_bytes(b"edited configuration")
            else:
                (output / "Unexpected.cpp").write_bytes(b"unreviewed extra translation unit")
            with self.subTest(mode=mode), self.assertRaises(ValueError):
                self.verify(output)

    def test_editable_receipt_cannot_admit_or_authorize_changed_header(self):
        for index, field in enumerate(("formalTaskAdmission", "sdkVerified", "buildLinked", "outputSha256")):
            output = self.root / ("receipt-" + str(index))
            receipt = self.prepare(output)
            if field == "outputSha256":
                content = b"modified header"
                (output / "CandidateConfig.h").write_bytes(content)
                receipt[field]["CandidateConfig.h"] = candidate.sha256(content)
            else:
                receipt[field] = True
            (output / "candidate.json").write_text(json.dumps(receipt))
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.verify(output)

    def test_existing_output_never_overwritten(self):
        self.prepare()
        original = {file.name: file.read_bytes() for file in self.output.iterdir()}
        with self.assertRaises(FileExistsError):
            self.prepare()
        self.assertEqual({file.name: file.read_bytes() for file in self.output.iterdir()}, original)

    def test_output_fences_resolve_dot_parent_and_reject_sdk_src_pubsub(self):
        for output in (self.sdk / "candidate", self.sdk / "core/../candidate", ROOT / "src" / "candidate",
                       self.pubsub / "candidate", self.pubsub.parent / "candidate"):
            with self.subTest(output=output), self.assertRaisesRegex(ValueError, "outside SDK"):
                self.prepare(output)
            self.assertFalse(output.exists())
        self.assert_originals_untouched()

    def test_verify_binds_dependency_directory_and_sdk_fence_without_claiming_sdk_verification(self):
        self.prepare()
        other_sdk = self.root / "other-sdk"
        other_sdk.mkdir()
        with self.assertRaisesRegex(ValueError, "receipt/source mismatch"):
            candidate.verify(self.pubsub, self.output, other_sdk)
        other_pubsub = self.root / "other-pubsub/src"
        other_pubsub.mkdir(parents=True)
        for name, content in self.inputs.items():
            (other_pubsub / name).write_bytes(content)
        with self.assertRaisesRegex(ValueError, "receipt/source mismatch"):
            candidate.verify(other_pubsub, self.output, self.sdk)


if __name__ == "__main__":
    unittest.main()
