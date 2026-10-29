"""Finite driver-free input install/link gate checks; no SDK or network use."""
import contextlib
import copy
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("location_input_installed_gate", ROOT / "tests/arduino/verify_installed_release.py")
gate = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = gate
SPEC.loader.exec_module(gate)

CLOUD_MARKERS = (
    "blinker::CloudLocationBinding::prepare(", "blinker::CloudLocationBinding::write(",
    "blinker::Client::attachCloudSampleProvider(", "blinker::Client::telemetrySamplePreparerThunk(",
)
WRAPPERS = (
    "acceptUserLocation(double, double, unsigned int)",
    "acceptUserLocation(blinker::LocationFix const&, unsigned int)",
    "acceptFreshUserLocation(double, double)",
    "acceptFreshUserLocation(blinker::LocationFix const&)",
    "userLocationLost()", "userLocationClockReset()",
)


class LocationInputCandidateTests(unittest.TestCase):
    def test_only_single_original_sdk_target_and_no_business_time_or_matrix_growth(self):
        baseline = gate.TARGETS
        target, = gate.location_input_targets(gate.select_targets(["esp32-cloud-smoke"]))
        self.assertEqual(target.example, "CompileLocationInput")
        self.assertEqual(target.fqbn, "esp32:esp32:esp32")
        self.assertFalse(target.time_enabled)
        self.assertFalse(target.tasks_enabled)
        self.assertTrue(gate.TARGETS[3].time_enabled)
        self.assertEqual(gate.TARGETS, baseline)
        self.assertNotIn(target.example, [row.example for row in gate.TARGETS])
        for names in (None, ["esp32-wifi"], ["esp32-cloud-smoke", "esp32-wifi"], ["esp32-wifi-tasks"]):
            with self.subTest(names=names), self.assertRaises(SystemExit):
                gate.location_input_targets(gate.select_targets(names))

    def test_old_candidate_and_new_input_are_mutually_exclusive(self):
        args = ["gate", "--zip-path", "fixture.zip", "--work-dir", "fixture",
                "--location-input", "--location-candidate"]
        with patch.object(sys, "argv", args), contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            gate.parse_arguments()
        with patch.object(sys, "argv", args[:-1]):
            parsed = gate.parse_arguments()
        self.assertTrue(parsed.location_input)
        self.assertFalse(parsed.location_candidate)

    def test_link_requires_real_carrier_and_all_six_typed_wrapper_roots(self):
        markers = (*CLOUD_MARKERS, *WRAPPERS)
        symbols = "\n".join("00000001 T " + row for row in markers)
        gate.verify_location_input_link(symbols)
        # Short inline methods need not have a standalone ELF symbol.
        self.assertNotIn("blinker::Location::updateSnapshot", symbols)
        for missing in markers:
            with self.subTest(missing=missing), self.assertRaises(SystemExit):
                gate.verify_location_input_link("\n".join(row for row in markers if row != missing))

    def test_wrapper_signatures_do_not_accept_unrelated_overloads(self):
        symbols = "\n".join((*CLOUD_MARKERS, *WRAPPERS))
        gate.verify_location_input_link(symbols.replace("unsigned int", "unsigned long"))
        for replacement in ("acceptFreshUserLocation(double, float)", "otherAcceptFreshUserLocation(double, double)"):
            with self.subTest(replacement=replacement), self.assertRaises(SystemExit):
                gate.verify_location_input_link(symbols.replace("acceptFreshUserLocation(double, double)", replacement))

    def test_current_source_profile_and_probe_are_frozen_not_old_zip_metadata(self):
        frozen = gate.verify_location_input_inputs()
        self.assertEqual(frozen["sdk"], "3.3.11")
        self.assertEqual(frozen["productProfile"], gate.CLOUD_WIFI_PROFILE_ID)
        self.assertFalse(frozen["businessTimeEnabled"])
        self.assertEqual(frozen["taskCapacity"], 0)
        for name in ("src/BlinkerV2/location/Location.h", "src/BlinkerV2/api/BlinkerFacade.h",
                     "src/BlinkerV2/location/CloudLocationBinding.h"):
            self.assertIn(name, frozen["installedFiles"])
        self.assertIn("tests/arduino/CompileLocationInput/CompileLocationInput.ino", frozen["inputs"])
        changed = copy.deepcopy(frozen)
        changed["inputs"]["profiles/cloud-beta.json"] = "0" * 64
        with self.assertRaises(SystemExit):
            gate.verify_location_input_inputs(frozen=changed)

    def test_installed_source_and_rendered_profile_must_match_every_frozen_file(self):
        frozen = gate.verify_location_input_inputs()
        profile = gate.load_profile(ROOT / "profiles/cloud-beta.json")
        files = gate.configured_library_files(ROOT, profile)
        with tempfile.TemporaryDirectory(prefix="blinker-location-input-gate-") as directory:
            installed = Path(directory)
            for name, value in files.items():
                target = installed / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(value if isinstance(value, bytes) else value.read_bytes())
            self.assertEqual(gate.verify_location_input_inputs(installed, frozen=frozen), frozen)
            for name in ("src/BlinkerV2/location/Location.h", "src/BlinkerV2/api/Client.inc",
                         "src/BlinkerV2/arduino/config/OfficialConfig.h"):
                target = installed / name
                original = target.read_bytes()
                target.write_bytes(original + b"\n// stale fixture\n")
                with self.subTest(name=name), self.assertRaises(SystemExit):
                    gate.verify_location_input_inputs(installed, frozen=frozen)
                target.write_bytes(original)
            (installed / "src/BlinkerV2/location/Location.h").unlink()
            with self.assertRaises(SystemExit):
                gate.verify_location_input_inputs(installed, frozen=frozen)

    def test_repository_headers_cannot_satisfy_the_installed_input_gate(self):
        with tempfile.TemporaryDirectory(prefix="blinker-location-provenance-") as directory:
            build = Path(directory)
            installed = build / "sketchbook/libraries/BlinkerDeviceV2"
            rows = [{"file": str(installed / "src/BlinkerV2/implementation/BlinkerApi.cpp"),
                     "arguments": ["compiler", "-I" + str(ROOT / "src")]}]
            (build / "compile_commands.json").write_text(json.dumps(rows), encoding="utf-8")
            with self.assertRaises(SystemExit):
                gate.verify_provenance(build, installed)


if __name__ == "__main__":
    unittest.main()
