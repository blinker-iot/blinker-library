"""Limited Arduino delivery contracts; no SDK, runtime, credentials or network."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests/arduino"))
sys.path.insert(0, str(ROOT / "tests/release"))
import build_esp32_softap_tasks as delivery
import build_arduino_release as package
import arduino_release_profile as profile
import verify_installed_release as installed
from test_cloud_wifi_profile import preprocessor


class LimitedDelivery(unittest.TestCase):
    def test_cli_does_not_offer_an_unverifiable_custom_profile(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            result = subprocess.run([sys.executable, str(ROOT / "tools/build_esp32_softap_tasks.py"),
                                     "--stage-dir", str(root / "stage"), "--output-dir", str(root / "out"),
                                     "--profile", str(delivery.SPEC)], text=True, capture_output=True,
                                    timeout=20, check=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("unrecognized arguments: --profile", result.stderr)
            self.assertFalse((root / "stage").exists() or (root / "out").exists())

    def test_receipt_cannot_substitute_another_profile_or_archive(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "library.zip"
            archive.write_bytes(b"public test bytes")
            identity = root / "library" / profile.LIBRARY_IDENTITY_HEADER
            identity.parent.mkdir(parents=True)
            identity.write_bytes(b"public identity")
            receipt = {"schema": 1, "profileId": delivery.qa3.PROFILE_ID, "sdk": delivery.SDK,
                       "fqbn": delivery.FQBN, "example": delivery.EXAMPLE, "archive": archive.name,
                       "archiveSha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
                       "profileSha256": hashlib.sha256(delivery.SPEC.read_bytes()).hexdigest(),
                       "libraryIdentitySha256": hashlib.sha256(identity.read_bytes()).hexdigest()}
            manifest = root / "limited-profile.json"
            for key in ("archiveSha256", "profileSha256", "libraryIdentitySha256", "sdk", "fqbn", "profileId"):
                value = {**receipt, key: "mismatch"}
                manifest.write_text(json.dumps(value), encoding="utf-8")
                with self.subTest(key=key), self.assertRaisesRegex(ValueError, "receipt does not match"):
                    delivery.verify_package(archive, root / "library", manifest)

    def test_public_projection_matches_saved_signer_without_admitting_devices(self):
        transport, signer = delivery.load_inputs()
        self.assertTrue(transport.cloud_wifi_only and transport.production)
        self.assertEqual(len(signer.p256_keys), 1)
        self.assertEqual(signer.p256_keys[0].key_id, 1)
        self.assertEqual(signer.ed25519_keys, ())
        self.assertEqual(hashlib.sha256(delivery.P256_SPKI_PREFIX + signer.p256_keys[0].public_key).hexdigest(),
                         "02879e20a6ad6e021ed7e6c857f0a351db14b1568c467db519aac09920b6d061")
        self.assertNotIn("#define BLINKER_TASK_CAPACITY", profile.render_config(transport))
        # The special two-input composition must not enter the ordinary parser.
        with self.assertRaises(ValueError):
            profile.load_profile(delivery.SPEC)

    def test_spec_rejects_secret_fields_algorithm_key_and_digest_changes(self):
        original = json.loads(delivery.SPEC.read_text(encoding="utf-8"))
        mutations = [lambda p: p.update(private_key="must-not-be-published"),
                     lambda p: p.update(schema=True),
                     lambda p: p.update(transport_profile="../../private.json"),
                     lambda p: p["server_key"].update(algorithm=1),
                     lambda p: p["server_key"].update(key_id=True),
                     lambda p: p["server_key"].update(key_id=2),
                     lambda p: p["server_key"].update(pop="private"),
                     lambda p: p["server_key"].update(spki_sha256="0" * 64)]
        with tempfile.TemporaryDirectory() as directory:
            spec = Path(directory) / "input.json"
            for mutate in mutations:
                value = json.loads(json.dumps(original))
                mutate(value)
                spec.write_text(json.dumps(value), encoding="utf-8")
                with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                    delivery.load_inputs(spec)

    def test_deterministic_package_has_identity_and_never_claims_runtime_validation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            receipts = []
            for name in ("first", "second"):
                stage, output = root / name / "stage", root / name / "out"
                receipt = delivery.build(stage, output)
                archive = output / receipt["archive"]
                installed.verify_installed_bytes(archive, stage)
                delivery.verify_package(archive, stage, output / "limited-profile.json")
                self.assertEqual(receipt["taskTargets"], ["power:bool"])
                for flag in ("deployable", "runtimeSigningMatched", "formalAdmission",
                             "installationCompileVerified", "hardwareVerified"):
                    self.assertIs(receipt[flag], False)
                for header in ("OfficialConfig.h", "Esp32ServerKeys.h", "PortableServerKeys.h"):
                    self.assertTrue((stage / delivery.qa3.PREFIX / header).read_text(encoding="utf-8").startswith(delivery.BOARD_GUARD))
                receipts.append(receipt)
            self.assertEqual(receipts[0], receipts[1])
            library = root / "first/stage"
            header = library / "src/BlinkerV2/arduino/config/OfficialConfig.h"
            header.write_text(header.read_text(encoding="utf-8").replace(
                "BLINKER_TASK_CAPACITY 4U", "BLINKER_TASK_CAPACITY 5U"), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "configuration mismatch"):
                delivery.verify_library(library)

    def test_delivery_never_overwrites_nonempty_or_nested_outputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            occupied = root / "occupied"
            occupied.mkdir()
            marker = occupied / "user.txt"
            marker.write_text("keep", encoding="utf-8")
            for stage, out in [(occupied, root / "out"), (root / "stage", occupied),
                               (root / "same", root / "same"), (root / "outer", root / "outer/out"),
                               (ROOT / "src/generated-stage", root / "out")]:
                with self.subTest(stage=stage, out=out), self.assertRaises(ValueError):
                    delivery.build(stage, out)
            self.assertEqual(marker.read_text(encoding="utf-8"), "keep")

    def test_example_publishes_no_rendered_pop_or_extra_private_inputs(self):
        _, files = package.validate()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package.stage(root, files)
            with patch.object(package, "REPOSITORY", root):
                header = root / "examples" / delivery.EXAMPLE / "UserProvisioning.h"
                original = header.read_text(encoding="utf-8")
                header.write_text(original.replace("REPLACE_WITH_YOUR_DEVICE_POP", "private-pop"), encoding="utf-8")
                with self.assertRaisesRegex(ValueError, "public PoP placeholder"):
                    package.validate()
                header.write_text(original, encoding="utf-8")
                extra = header.parent / "PrivateIdentity.h"
                extra.write_text('// private\n', encoding="utf-8")
                with self.assertRaisesRegex(ValueError, "only the explicit"):
                    package.validate()

    def test_only_explicit_single_target_accepts_limited_config(self):
        config = delivery.configured_files(ROOT)[delivery.qa3.PREFIX + "OfficialConfig.h"].decode()
        selected = (installed.SOFTAP_TASK_TARGET,)
        self.assertNotIn(installed.SOFTAP_TASK_TARGET, installed.select_targets(None))
        self.assertEqual(len(installed.select_targets(None)), 7)
        self.assertEqual(installed.select_targets(["esp32-softap-tasks"]), selected)
        self.assertEqual(installed.verify_installed_task_profile(config, selected), delivery.qa3.PROFILE_ID)
        for targets in (installed.TARGETS, (installed.TASK_PILOT_TARGET,), selected + (installed.TARGETS[0],)):
            with self.subTest(targets=targets), self.assertRaises(SystemExit):
                installed.verify_installed_task_profile(config, targets)
        with self.assertRaises(SystemExit):
            installed.verify_installed_task_profile("// ordinary default zero\n", selected)
        with self.assertRaises(SystemExit):
            installed.verify_installed_task_profile(config.replace("CAPACITY 4U", "CAPACITY 5U"), selected)

    def test_all_translation_unit_headers_reject_other_boards(self):
        compiler = preprocessor()
        if not compiler:
            self.skipTest("local preprocessor unavailable")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "input.cpp"
            for header in ("OfficialConfig.h", "Esp32ServerKeys.h", "PortableServerKeys.h"):
                # The board prefix must reject before pulling any SDK/types.
                body = delivery.configured_files(ROOT)[delivery.qa3.PREFIX + header].decode()
                prefix = body.split("// Internal QA-3:", 1)[0]
                source.write_text(prefix + '\nint main() {}\n', encoding="utf-8")
                for board, accepted in (([], False), (["-DARDUINO_XIAO_ESP32S3"], True)):
                    result = subprocess.run([compiler, "-x", "c++", "-fsyntax-only", *board, str(source)],
                                            text=True, capture_output=True, timeout=20, check=False)
                    with self.subTest(header=header, board=board):
                        self.assertEqual(result.returncode == 0, accepted, result.stderr)


if __name__ == "__main__":
    unittest.main()
