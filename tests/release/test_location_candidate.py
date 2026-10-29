"""Finite LOC-2 install-gate contract checks; no SDK compilation or network."""
import importlib.util
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("location_installed_gate", ROOT / "tests/arduino/verify_installed_release.py")
gate = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = gate
SPEC.loader.exec_module(gate)


class LocationCandidateTests(unittest.TestCase):
    def test_frozen_inputs_and_canonical_manifest_hash_match(self):
        frozen = gate.verify_location_inputs()
        self.assertEqual(frozen["manifestRevision"], 1)
        self.assertEqual(frozen["fieldCount"], 3)
        self.assertEqual(frozen["locationEndpointId"], 3)
        self.assertEqual(frozen["locationCodec"], "device-gnss-v1")

    def test_explicit_single_consumer_preserves_public_matrix(self):
        before = gate.TARGETS
        target = gate.location_candidate_targets(gate.select_targets(["esp32-cloud-smoke"]))[0]
        self.assertEqual(target.example, "CompileCloudLocation")
        self.assertEqual(target.fqbn, "esp32:esp32:esp32")
        self.assertTrue(target.time_enabled)
        self.assertFalse(target.tasks_enabled)
        self.assertEqual(gate.TARGETS, before)
        self.assertNotIn("CompileCloudLocation", [item.example for item in gate.TARGETS])
        for names in (None, ["esp32-wifi"], ["esp32-cloud-smoke", "esp32-wifi"], ["esp32-wifi-tasks"]):
            with self.assertRaises(SystemExit):
                gate.location_candidate_targets(gate.select_targets(names))

    def test_link_requires_real_binding_and_preparer_not_only_wifi_symbols(self):
        markers = ["blinker::CloudLocationBinding::prepare(", "blinker::CloudLocationBinding::write(",
                   "blinker::Client::attachCloudSampleProvider(", "blinker::Client::telemetrySamplePreparerThunk("]
        gate.verify_location_link("\n".join(markers))
        for missing in markers:
            with self.assertRaises(SystemExit):
                gate.verify_location_link("\n".join(marker for marker in markers if marker != missing))

    def test_shared_schema_is_not_an_extra_public_example_or_fake_fix(self):
        sketch = (ROOT / "tests/arduino/CompileCloudLocation/CompileCloudLocation.ino").read_text(encoding="utf-8")
        self.assertIn('#include "Schema.h"', sketch)
        self.assertIn("locationBinding.attach(device,", sketch)
        self.assertIn("product.begin(&businessTime)", sketch)
        self.assertNotIn("locationSnapshot.update(", sketch.split("void setup()", 1)[1])
        self.assertNotIn("position.set", sketch)
        self.assertFalse((ROOT / "examples/CompileCloudLocation").exists())


if __name__ == "__main__":
    unittest.main()
