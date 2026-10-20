"""Preparation contracts only; mocked transforms are not native SDK evidence."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import prepare_wio_rpc_deadline_overlay as overlay


class WioRpcDeadlinePreparation(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.rpc, self.wifi, self.sdk = [self.root / name for name in ("rpc", "wifi", "sdk")]
        self.output = self.root / "output"
        for directory in (self.rpc, self.wifi, self.sdk):
            directory.mkdir()
        self.originals = {}
        for directory, attribute in ((self.rpc, "RPC_SHA256"), (self.wifi, "WIFI_SHA256")):
            pins = {}
            for name in getattr(overlay, attribute):
                content = (name + "\n// temporary preparation fixture\n").encode()
                file = directory / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_bytes(content)
                self.originals[file] = content
                pins[name] = overlay.digest(content)
            self.enterContext(patch.object(overlay, attribute, pins))
        self.enterContext(patch.object(overlay.pending, "expected_overlay", return_value=(
            {"erpc/erpc_transport_arbitrator.cpp": b"pending stage"}, {})))
        self.enterContext(patch.object(overlay, "pending_deadline", side_effect=lambda inputs: {
            "erpc/erpc_transport_arbitrator.cpp": inputs["erpc/erpc_transport_arbitrator.cpp"] + "\ndeadline stage"}))
        self.enterContext(patch.object(overlay.client, "transform", side_effect=lambda inputs: {
            "erpc/erpc_client_manager.cpp": inputs["erpc/erpc_client_manager.cpp"] + "client stage"}))
        self.enterContext(patch.object(overlay.transport, "transform", side_effect=lambda inputs, port: {
            "erpc/erpc_arduino_uart_transport.cpp": inputs["erpc/erpc_arduino_uart_transport.cpp"] + "transport stage",
            "erpc/blinker_wio_rpc_port.inc": port}))

    def prepare(self, output=None):
        return overlay.prepare(self.rpc, self.wifi, output or self.output, self.sdk)

    def verify(self, output=None):
        return overlay.verify(self.rpc, self.wifi, output or self.output, self.sdk)

    def originals_untouched(self):
        for file, content in self.originals.items():
            self.assertEqual(file.read_bytes(), content)
        self.assertEqual(list(self.sdk.rglob("*")), [])

    def test_prepare_verify_binds_composed_output_without_running_tools(self):
        with patch("subprocess.run") as process:
            receipt = self.prepare()
            self.assertEqual(self.verify(), receipt)
            process.assert_not_called()
        files, expected = overlay.expected_overlay(self.rpc, self.wifi, self.output, self.sdk)
        self.assertEqual(expected, receipt)
        self.assertEqual(files["erpc/erpc_transport_arbitrator.cpp"], b"pending stage\ndeadline stage")
        self.assertIn(b"client stage", files["erpc/erpc_client_manager.cpp"])
        self.assertIn(b"transport stage", files["erpc/erpc_arduino_uart_transport.cpp"])
        self.assertEqual(receipt["budgetMillis"], 1000)
        self.assertEqual(receipt["scope"], "fixed-sta-plain-single-generated-rpc")
        self.assertEqual(len(receipt["operatorSha256"]), 6)
        self.assertEqual(receipt["outputSha256"], {name: overlay.digest(data) for name, data in files.items()})
        self.originals_untouched()

    def test_each_pinned_rpc_input_is_checked_before_any_output(self):
        for file, original in self.originals.items():
            if not file.is_relative_to(self.rpc):
                continue
            with self.subTest(name=file.name):
                file.write_bytes(original + b"changed")
                with self.assertRaisesRegex(ValueError, "Unreviewed Wio RPC dependency"):
                    self.prepare()
                file.write_bytes(original)
                self.assertFalse(self.output.exists())
        self.originals_untouched()

    def test_each_pinned_wifi_scope_input_is_checked(self):
        for file, original in self.originals.items():
            if file.is_relative_to(self.wifi):
                file.write_bytes(original + b"changed")
                with self.assertRaises(ValueError):
                    self.prepare()
                file.write_bytes(original)
                self.assertFalse(self.output.exists())
        self.originals_untouched()

    def test_missing_extra_and_modified_files_fail_verification(self):
        for index, mode in enumerate(("missing", "extra", "modified")):
            output = self.root / ("output" + str(index))
            receipt = self.prepare(output)
            first = output / next(iter(receipt["outputSha256"]))
            if mode == "missing":
                first.unlink()
            elif mode == "extra":
                (output / "Unexpected.cpp").write_bytes(b"extra")
            else:
                first.write_bytes(b"changed")
            with self.subTest(mode=mode), self.assertRaises(ValueError):
                self.verify(output)

    def test_receipt_self_hash_cannot_authorize_changed_code(self):
        receipt = self.prepare()
        first = next(iter(receipt["outputSha256"]))
        changed = b"changed code"
        (self.output / first).write_bytes(changed)
        receipt["outputSha256"][first] = overlay.digest(changed)
        (self.output / "overlay.json").write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "content mismatch"):
            self.verify()

    def test_receipt_cannot_claim_sdk_physical_or_task_admission(self):
        fields = ("sdkVerified", "buildLinked", "hardwareWritten", "wholeRpcDeadlineComplete",
                  "physicalIsolationVerified", "transportQuarantineComplete", "formalTaskAdmission", "ordinarySdkDependency")
        for index, field in enumerate(fields):
            output = self.root / ("claim" + str(index))
            receipt = self.prepare(output)
            self.assertIs(receipt[field], False)
            receipt[field] = True
            (output / "overlay.json").write_text(json.dumps(receipt))
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "receipt/operator"):
                self.verify(output)

    def test_changed_operator_invalidates_existing_receipt(self):
        self.prepare()
        target = Path(overlay.client.__file__)
        read = Path.read_bytes
        with patch.object(Path, "read_bytes", lambda file: read(file) + (b"changed" if file == target else b"")):
            with self.assertRaisesRegex(ValueError, "receipt/operator"):
                self.verify()

    def test_budget_is_not_silently_changed_by_editing_the_header(self):
        read = Path.read_bytes
        with patch.object(Path, "read_bytes", lambda file: read(file).replace(b"kBudgetMillis = 1000U;", b"kBudgetMillis = 1U;") if file == overlay.BOUNDARY else read(file)):
            with self.assertRaisesRegex(ValueError, "observation budget"):
                self.prepare()
        self.assertFalse(self.output.exists())

    def test_existing_output_is_never_overwritten(self):
        self.prepare()
        original = {p.relative_to(self.output).as_posix(): p.read_bytes() for p in self.output.rglob("*") if p.is_file()}
        with self.assertRaises(FileExistsError):
            self.prepare()
        self.assertEqual({p.relative_to(self.output).as_posix(): p.read_bytes() for p in self.output.rglob("*") if p.is_file()}, original)

    def test_output_path_fences_cover_both_dependencies_sdk_and_src(self):
        for output in (self.rpc / "overlay", self.wifi / "overlay", self.sdk / "x/../overlay", ROOT / "src/overlay"):
            with self.subTest(output=output), self.assertRaisesRegex(ValueError, "outside original"):
                self.prepare(output)
            self.assertFalse(output.exists())
        self.originals_untouched()

    def test_verify_binds_original_directories_and_output_set(self):
        self.prepare()
        other = self.root / "other-sdk"
        other.mkdir()
        with self.assertRaisesRegex(ValueError, "receipt/operator"):
            overlay.verify(self.rpc, self.wifi, self.output, other)
        receipt = json.loads((self.output / "overlay.json").read_text())
        self.assertIn("erpc/blinker_wio_rpc_port.inc", receipt["outputSha256"])
        self.assertIn("erpc/blinker_wio_rpc_boundary.h", receipt["outputSha256"])

    def test_exact_function_edits_reject_missing_duplicate_or_reordered_anchors(self):
        for text in ("first", "first first last", "last first"):
            with self.assertRaises(ValueError):
                overlay.replace_function(text, "first", "last", "new")
        self.assertEqual(overlay.replace_function("head first body last tail", "first", "last", "new"), "head new\n\nlast tail")
        with self.assertRaises(ValueError):
            overlay.replace_once("duplicate duplicate", "duplicate", "new")

    def test_build_contract_keeps_scope_and_no_recovery_boundaries(self):
        receipt = self.prepare()
        text = " ".join(receipt["requiredBuildSteps"] + receipt["unsupported"])
        for phrase in ("complete pinned", "every actual consumer", "Small256/MQTT361/newlib/profile", "runClient-only",
                       "no AP/scan/BLE", "replay after fault", "Multi-RPC", "latency guarantee", "physical stop"):
            self.assertIn(phrase, text)

    def test_cli_rejects_runtime_actions_and_timeout_overrides(self):
        for flag in ("--install", "--rpc-budget-ms"):
            result = subprocess.run([sys.executable, overlay.__file__, "--rpc-dir", str(self.rpc),
                "--wifi-dir", str(self.wifi), "--sdk-root", str(self.sdk), "--output", str(self.output), flag, "1"],
                capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 2)
            self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
