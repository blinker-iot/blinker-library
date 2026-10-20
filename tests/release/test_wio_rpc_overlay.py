"""Finite generator contract tests; temporary fragments are not SDK/Host proof."""
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
import prepare_wio_rpc_overlay as overlay


class WioRpcOverlayPreparation(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.rpc = self.root / "rpcUnified"
        self.rpc.mkdir()
        self.sdk = self.root / "sdk"
        self.sdk.mkdir()
        self.output = self.root / "overlay"
        # Exercise the real exact-once replacement machinery, not a duplicate
        # pending algorithm. Real SDK CPP/headers are tested by another gate.
        self.original = {"library.properties": b"name=Seeed Arduino rpcUnified\nversion=2.1.4\n"}
        self.original.update({name: ("\n// temporary source fragment separator\n".join(
            before for before, _after in replacements)).encode("utf-8")
            for name, replacements in overlay.REPLACEMENTS.items()})
        for name, content in self.original.items():
            file = self.rpc / name
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(content)
        self.pins = {name: self.digest(content) for name, content in self.original.items()}
        self.enterContext(patch.object(overlay, "DEPENDENCY_SHA256", self.pins))

    @staticmethod
    def digest(content):
        return hashlib.sha256(content).hexdigest()

    def prepare(self, output=None):
        return overlay.prepare(self.rpc, output or self.output, self.sdk)

    def verify(self, output=None):
        return overlay.verify(self.rpc, output or self.output, self.sdk)

    def assert_originals_untouched(self):
        self.assertEqual({file.relative_to(self.rpc).as_posix(): file.read_bytes()
                          for file in self.rpc.rglob("*") if file.is_file()}, self.original)
        self.assertEqual(list(self.sdk.rglob("*")), [])

    def read_changed_input(self, name, content):
        original_read = Path.read_bytes

        def read(file):
            return content if file == self.rpc / name else original_read(file)

        return patch.object(Path, "read_bytes", read)

    def test_fixture_fragments_are_independent_and_prepare_verify_is_offline(self):
        for name, replacements in overlay.REPLACEMENTS.items():
            for before, _after in replacements:
                self.assertEqual(self.original[name].decode().count(before), 1)
        with patch("subprocess.run") as process:
            receipt = self.prepare()
            self.assertEqual(self.verify(), receipt)
            process.assert_not_called()
        self.assertEqual(receipt["schema"], "internal-wio-rpc-pending/1")
        self.assertEqual(receipt["scope"], "pending-lifecycle-only")
        self.assertEqual(receipt["state"], "internal-offline-only")
        for field in ("sdkVerified", "buildLinked", "wholeRpcDeadlineComplete", "transportQuarantineComplete",
                      "formalTaskAdmission", "hardwareWritten", "ordinarySdkDependency", "crossThreadCancellationSupported"):
            self.assertIs(receipt[field], False, field)
        self.assertIs(receipt["originalWaitForeverUnchanged"], True)
        self.assertEqual(receipt["requiredRpcUnified"], "2.1.4")
        self.assertEqual(receipt["requiredCore"], "1.8.5")
        self.assert_originals_untouched()

    def test_output_set_and_digests_bind_exactly_three_generated_files(self):
        receipt = self.prepare()
        files, expected = overlay.expected_overlay(self.rpc, self.output, self.sdk)
        self.assertEqual(receipt, expected)
        self.assertEqual(set(files), {name.removeprefix("src/") for name in overlay.REPLACEMENTS})
        self.assertEqual({file.relative_to(self.output).as_posix() for file in self.output.rglob("*") if file.is_file()}, set(files) | {"overlay.json"})
        self.assertEqual(receipt["dependencySha256"], self.pins)
        self.assertEqual(receipt["operatorSha256"], self.digest(Path(overlay.__file__).read_bytes()))
        self.assertEqual(receipt["outputSha256"], {name: self.digest(content) for name, content in files.items()})
        for name, content in files.items():
            self.assertEqual((self.output / name).read_bytes(), content)
        header = files["erpc/erpc_transport_arbitrator.h"].decode()
        for macro in ("BLINKER_ENABLE_TASKS", "BLINKER_INTERNAL_WIO_TASKS",
                      "BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED", "BLINKER_TASK_CAPACITY"):
            self.assertNotRegex(header, r"#\s*define\s+" + macro + r"\b")

    def test_each_changed_pinned_input_rejected_before_output_creation(self):
        for name, content in self.original.items():
            with self.subTest(name=name), self.read_changed_input(name, content + b"changed"):
                with self.assertRaisesRegex(ValueError, "unreviewed rpcUnified"):
                    self.prepare()
            self.assertFalse(self.output.exists())
        self.assert_originals_untouched()

    def test_metadata_version_is_checked_even_after_fixture_digest_matches(self):
        content = self.original["library.properties"].replace(b"version=2.1.4", b"version=2.1.3")
        with self.read_changed_input("library.properties", content), patch.object(overlay, "DEPENDENCY_SHA256", {
                **self.pins, "library.properties": self.digest(content)}):
            with self.assertRaisesRegex(ValueError, "metadata required"):
                self.prepare()
        self.assertFalse(self.output.exists())

    def test_missing_or_duplicate_patch_anchor_fails_even_with_matching_digest(self):
        for name, replacements in overlay.REPLACEMENTS.items():
            for before, _after in replacements:
                for changed in (self.original[name].replace(before.encode(), b"REMOVED_ANCHOR", 1),
                                self.original[name] + b"\n" + before.encode()):
                    with self.subTest(name=name, anchor=before[:40]), self.read_changed_input(name, changed):
                        with patch.object(overlay, "DEPENDENCY_SHA256", {**self.pins, name: self.digest(changed)}):
                            with self.assertRaisesRegex(ValueError, "anchor must occur exactly once"):
                                self.prepare()
                    self.assertFalse(self.output.exists())
        self.assert_originals_untouched()

    def test_verify_rechecks_original_bytes_not_receipt_dependency_hash(self):
        receipt = self.prepare()
        name = "src/erpc/erpc_transport_arbitrator.cpp"
        changed = self.original[name] + b"changed"
        receipt["dependencySha256"][name] = self.digest(changed)
        (self.output / "overlay.json").write_text(json.dumps(receipt))
        with self.read_changed_input(name, changed), self.assertRaisesRegex(ValueError, "unreviewed rpcUnified"):
            self.verify()
        self.assert_originals_untouched()

    def test_missing_extra_or_modified_output_is_rejected(self):
        for index, mode in enumerate(("missing", "extra", "modified")):
            output = self.root / ("output-" + str(index))
            receipt = self.prepare(output)
            first = next(iter(receipt["outputSha256"]))
            if mode == "missing":
                (output / first).unlink()
            elif mode == "extra":
                (output / "Unexpected.cpp").write_bytes(b"unreviewed translation unit")
            else:
                (output / first).write_bytes(b"edited generated content")
            with self.subTest(mode=mode), self.assertRaises(ValueError):
                self.verify(output)

    def test_receipt_output_self_hash_cannot_authorize_modified_overlay(self):
        receipt = self.prepare()
        name = next(iter(receipt["outputSha256"]))
        content = b"different pending implementation"
        (self.output / name).write_bytes(content)
        receipt["outputSha256"][name] = self.digest(content)
        (self.output / "overlay.json").write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "output mismatch"):
            self.verify()

    def test_receipt_cannot_claim_deadline_quarantine_or_admission(self):
        for index, field in enumerate(("wholeRpcDeadlineComplete", "transportQuarantineComplete", "formalTaskAdmission",
                                       "sdkVerified", "buildLinked", "crossThreadCancellationSupported")):
            output = self.root / ("claim-" + str(index))
            receipt = self.prepare(output)
            receipt[field] = True
            (output / "overlay.json").write_text(json.dumps(receipt))
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "receipt/source mismatch"):
                self.verify(output)

    def test_operator_change_invalidates_verify_without_editing_real_tool(self):
        self.prepare()
        operator = Path(overlay.__file__)
        original_read = Path.read_bytes
        original = operator.read_bytes()

        def read(file):
            content = original_read(file)
            return content + b"changed operator" if file == operator else content

        with patch.object(Path, "read_bytes", read), self.assertRaisesRegex(ValueError, "receipt/source mismatch"):
            self.verify()
        self.assertEqual(operator.read_bytes(), original)

    def test_existing_output_never_overwritten(self):
        self.prepare()
        original = {file.relative_to(self.output).as_posix(): file.read_bytes()
                    for file in self.output.rglob("*") if file.is_file()}
        with self.assertRaises(FileExistsError):
            self.prepare()
        self.assertEqual({file.relative_to(self.output).as_posix(): file.read_bytes()
                          for file in self.output.rglob("*") if file.is_file()}, original)
        self.assert_originals_untouched()

    def test_output_fences_reject_sdk_src_rpc_and_resolved_parent(self):
        for output in (self.sdk / "overlay", self.sdk / "core/../overlay", ROOT / "src/overlay", self.rpc / "overlay"):
            with self.subTest(output=output), self.assertRaisesRegex(ValueError, "outside SDK"):
                self.prepare(output)
            self.assertFalse(output.exists())
        original_resolve = Path.resolve

        def resolve(file, *args, **kwargs):
            # Model a pre-existing symlink parent resolving into the SDK; the
            # test does not require Windows permission to create real symlinks.
            return self.sdk / "overlay" if file == self.output else original_resolve(file, *args, **kwargs)

        with patch.object(Path, "resolve", resolve), self.assertRaisesRegex(ValueError, "outside SDK"):
            self.prepare()
        self.assertFalse(self.output.exists())
        self.assert_originals_untouched()

    def test_verify_binds_both_input_directory_and_sdk_fence(self):
        self.prepare()
        sdk = self.root / "other-sdk"
        sdk.mkdir()
        with self.assertRaisesRegex(ValueError, "receipt/source mismatch"):
            overlay.verify(self.rpc, self.output, sdk)
        rpc = self.root / "other-rpc"
        for name, content in self.original.items():
            file = rpc / name
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(content)
        with self.assertRaisesRegex(ValueError, "receipt/source mismatch"):
            overlay.verify(rpc, self.output, self.sdk)

    def test_build_contract_retains_complete_projection_and_excludes_recovery(self):
        receipt = self.prepare()
        build = " ".join(receipt["requiredBuildSteps"])
        unsupported = " ".join(receipt["unsupported"])
        for phrase in ("complete pinned rpcUnified source tree", "every actual header consumer", "erpc_Unified_init.cpp",
                       "quoted includes", "token once", "pre-receive failure", "must not be reused"):
            self.assertIn(phrase, build)
        for phrase in ("Whole-RPC deadline", "UART stop", "lock acquisition deadline", "global recovery", "replay"):
            self.assertIn(phrase, unsupported)

    def test_cli_rejects_missing_inputs_and_timeout_override(self):
        for arguments in (("--output", str(self.output)),
                          ("--rpc-dir", str(self.rpc), "--sdk-root", str(self.sdk), "--output", str(self.output), "--timeout-ms", "1")):
            with self.subTest(arguments=arguments):
                result = subprocess.run([sys.executable, overlay.__file__, *arguments], capture_output=True, text=True, timeout=10)
                self.assertEqual(result.returncode, 2)
                self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
