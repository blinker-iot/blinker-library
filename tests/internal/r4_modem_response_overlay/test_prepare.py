"""Finite preparation checks; the caller supplies the untouched original SDK."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))
import prepare_r4_modem_response_overlay as overlay

SOURCE = None


class PrepareBoundary(unittest.TestCase):
    def test_exact_source_and_unchanged_header(self):
        originals = {name: (SOURCE / name).read_bytes()
                     for name in ("Modem.cpp", *overlay.COPIED_HEADERS)}
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "overlay"
            report = overlay.prepare(SOURCE, output)
            self.assertEqual(overlay.verify(SOURCE, output), report)
            self.assertTrue(report["originalHeaderUnchanged"])
            self.assertFalse(report["classLayoutChanged"])
            self.assertTrue(report["ownerOrProductIntegrated"])
            self.assertFalse(report["ordinaryProductEnabled"])
            self.assertTrue(report["txDeadlineImplemented"])
            self.assertFalse(report["hardwareStopValidated"])
            self.assertEqual(report["helperSha256"], overlay.sha256(
                (ROOT / "src" / overlay.HELPER_INCLUDE).read_bytes()))
            self.assertEqual(report["generatorSha256"], overlay.sha256(Path(overlay.__file__).read_bytes()))
            for name in overlay.COPIED_HEADERS:
                self.assertEqual((output / name).read_bytes(), originals[name])
            patched = (output / "Modem.cpp").read_text()
            self.assertEqual(patched.count("unoR4ModemAppendAllowed("), 6)
            self.assertNotIn("atoi(data_res.c_str())", patched)
            self.assertIn("#error \"Internal Modem overlay requires explicit", patched)
            self.assertIn("blinker_r4_parser_cleanup:\n   trim_results = true;", patched)
            # Preserve the complete original state graph, not a second parser.
            self.assertEqual(patched.count("case at_parse_state_t::"),
                             originals["Modem.cpp"].decode().count("case at_parse_state_t::"))
            with self.assertRaises(FileExistsError):
                overlay.prepare(SOURCE, output)
        self.assertEqual({name: (SOURCE / name).read_bytes() for name in originals}, originals)

    def test_source_mutation_and_sdk_output_refused(self):
        original = (SOURCE / "Modem.cpp").read_bytes()
        with self.assertRaisesRegex(ValueError, "SHA256"):
            overlay.patch_modem(original + b"\n// unreviewed source")
        with self.assertRaisesRegex(ValueError, "outside the original SDK"):
            overlay.prepare(SOURCE, SOURCE / "must-not-be-created")
        self.assertFalse((SOURCE / "must-not-be-created").exists())

    def test_header_mutation_refused_before_output(self):
        for changed in overlay.COPIED_HEADERS:
            with self.subTest(header=changed), tempfile.TemporaryDirectory() as temporary:
                source = Path(temporary) / "sdk/libraries/WiFiS3/src"
                source.mkdir(parents=True)
                for name in ("Modem.cpp", *overlay.COPIED_HEADERS):
                    value = (SOURCE / name).read_bytes()
                    if name == changed:
                        value += b"\n// unreviewed header"
                    (source / name).write_bytes(value)
                output = Path(temporary) / "overlay"
                with self.assertRaisesRegex(ValueError, changed.replace(".", r"\.") + " SHA256"):
                    overlay.prepare(source, output)
                self.assertFalse(output.exists())

    def test_post_prepare_content_mutations_refused(self):
        for changed in ("Modem.cpp", *overlay.COPIED_HEADERS, "Serial.cpp", "Serial.h"):
            with self.subTest(file=changed), tempfile.TemporaryDirectory() as temporary:
                output = Path(temporary) / "overlay"
                overlay.prepare(SOURCE, output)
                file = output / changed
                file.write_bytes(file.read_bytes() + b"\n// after prepare")
                with self.assertRaisesRegex(ValueError, "content mismatch"):
                    overlay.verify(SOURCE, output)

    def test_post_prepare_helper_mutation_refused(self):
        with tempfile.TemporaryDirectory() as temporary:
            helper = Path(temporary) / "helper.h"
            helper.write_bytes(overlay.helper_path().read_bytes())
            with patch.object(overlay, "helper_path", return_value=helper):
                output = Path(temporary) / "overlay"
                overlay.prepare(SOURCE, output)
                helper.write_bytes(helper.read_bytes() + b"\n// changed helper")
                with self.assertRaisesRegex(ValueError, "receipt/helper/generator mismatch"):
                    overlay.verify(SOURCE, output)

    def test_fake_receipt_cannot_admit_changed_source(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "overlay"
            receipt = overlay.prepare(SOURCE, output)
            file = output / "Modem.cpp"
            file.write_bytes(file.read_bytes() + b"\n// different parser")
            receipt["overlaySha256"]["Modem.cpp"] = overlay.sha256(file.read_bytes())
            (output / "overlay.json").write_text(json.dumps(receipt))
            with self.assertRaisesRegex(ValueError, "content mismatch"):
                overlay.verify(SOURCE, output)

    def test_receipt_mutation_and_missing_source_refused(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "overlay"
            receipt = overlay.prepare(SOURCE, output)
            receipt["helperSha256"] = "0" * 64
            (output / "overlay.json").write_text(json.dumps(receipt))
            with self.assertRaisesRegex(ValueError, "receipt/helper/generator mismatch"):
                overlay.verify(SOURCE, output)
            result = subprocess.run([sys.executable, overlay.__file__, "--verify-only",
                "--output", str(output)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn("--modem-dir", result.stderr)

    def test_uart_inputs_pinned_to_same_core(self):
        for changed in overlay.uart_overlay.SOURCE_SHA256:
            with self.subTest(file=changed), tempfile.TemporaryDirectory() as temporary:
                sdk = Path(temporary) / "sdk"
                source = sdk / "libraries/WiFiS3/src"
                core = sdk / "cores/arduino"
                source.mkdir(parents=True)
                core.mkdir(parents=True)
                for name in ("Modem.cpp", *overlay.COPIED_HEADERS):
                    (source / name).write_bytes((SOURCE / name).read_bytes())
                for name in overlay.uart_overlay.SOURCE_SHA256:
                    value = (SOURCE.parents[2] / "cores/arduino" / name).read_bytes()
                    if name == changed:
                        value += b"\n// unreviewed UART"
                    (core / name).write_bytes(value)
                output = Path(temporary) / "overlay"
                with self.assertRaisesRegex(ValueError, changed.replace(".", r"\.") + " SHA256"):
                    overlay.prepare(source, output)
                self.assertFalse(output.exists())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--modem-dir", type=Path, required=True)
    arguments, remaining = parser.parse_known_args()
    SOURCE = arguments.modem_dir.resolve()
    unittest.main(argv=[sys.argv[0], *remaining])
