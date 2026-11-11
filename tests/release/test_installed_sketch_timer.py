"""Narrow optional-API gate regressions, without SDK/network/hardware access."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY / "tests/arduino"))
import verify_installed_sketch_timer as timer


class InstalledTimerGate(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.work = self.root / "group"
        self.installed = self.work / "sketchbook/libraries/BlinkerDeviceV2"
        self.archive = self.root / "library.zip"
        with zipfile.ZipFile(self.archive, "w") as bundle:
            for name in timer.API_FILES:
                payload = (REPOSITORY / name).read_bytes()
                bundle.writestr("BlinkerDeviceV2/" + name, payload)
                path = self.installed / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(payload)
        (self.work / ".blinker-release-sha256").write_text(timer.digest(self.archive), encoding="ascii")
        self.target = timer.TARGETS["esp32-sketch-timer"]
        self.data = self.root / "sdk"
        self.build = self.work / "build/probe"
        self.build.mkdir(parents=True)
        self.commands = [{"file": str(self.installed / "src/BlinkerV2/implementation" / name), "arguments": []}
                         for name in sorted(timer.EXPECTED_IMPLEMENTATION_UNITS)]
        self.save_commands()
        (self.build / "build.options.json").write_text(json.dumps({
            "fqbn": self.target.fqbn, "hardwareFolders": str(self.data / self.target.core)}))
        self.save_libraries(self.target.libraries)
        (self.build / "probe.elf").write_bytes(b"fixture ELF, never hardware evidence")
        (self.build / "probe.map").write_text("fixture map")
        self.symbols = "sample()\ndelayed(void*)\nblinker::BlinkerClass::timeStatus(blinker::WallClockSnapshot&)\n"

    def save_commands(self):
        (self.build / "compile_commands.json").write_text(json.dumps(self.commands))

    def save_libraries(self, names):
        (self.build / "libraries.cache").write_text(json.dumps([
            str(self.work / "sketchbook/libraries" / name / "src") for name in names]))

    def verify(self):
        with patch.object(timer.gate, "defined_symbols", return_value=self.symbols):
            return timer.verify_build(self.build, self.installed, self.target, self.data)

    def test_exact_zip_current_api_and_build_admitted(self):
        self.assertEqual(timer.verify_installation(self.archive, self.work), self.installed.resolve())
        self.assertEqual(self.verify()["implementationUnits"], 17)

    def test_marker_does_not_substitute_for_installed_bytes(self):
        (self.installed / timer.API_FILES[0]).write_text("edited")
        with self.assertRaisesRegex(SystemExit, "content changed"):
            timer.verify_installation(self.archive, self.work)

    def test_other_zip_and_old_api_are_rejected(self):
        (self.work / ".blinker-release-sha256").write_text("old ZIP")
        with self.assertRaisesRegex(SystemExit, "exact ZIP"):
            timer.verify_installation(self.archive, self.work)
        (self.work / ".blinker-release-sha256").write_text(timer.digest(self.archive))
        with patch.object(timer, "API_FILES", ("src/missing-new-api.h",)):
            with self.assertRaisesRegex(SystemExit, "current source"):
                timer.verify_installation(self.archive, self.work)

    def test_all_installed_units_exactly_once(self):
        self.commands.pop()
        self.save_commands()
        with self.assertRaisesRegex(SystemExit, "17 installed"):
            self.verify()
        self.commands.append(self.commands[0])
        self.save_commands()
        with self.assertRaisesRegex(SystemExit, "17 installed"):
            self.verify()

    def test_repository_source_and_include_are_rejected(self):
        self.commands[0]["arguments"] = ["-I" + str(REPOSITORY / "src")]
        self.save_commands()
        with self.assertRaisesRegex(SystemExit, "repository headers"):
            self.verify()
        self.commands[0] = {"file": str(REPOSITORY / "src/BlinkerTimer.h"), "arguments": []}
        self.save_commands()
        with self.assertRaisesRegex(SystemExit, "repository source"):
            self.verify()

    def test_wrong_sdk_and_fqbn_are_rejected(self):
        for options in ({"fqbn": "different", "hardwareFolders": str(self.data / self.target.core)},
                        {"fqbn": self.target.fqbn, "hardwareFolders": str(self.data / "new-SDK")}):
            (self.build / "build.options.json").write_text(json.dumps(options))
            with self.assertRaisesRegex(SystemExit, "unexpected FQBN or SDK"):
                self.verify()

    def test_unselected_library_and_runtime_are_rejected(self):
        self.save_libraries(self.target.libraries | {"PubSubClient"})
        with self.assertRaisesRegex(SystemExit, "compiled libraries"):
            self.verify()
        self.save_libraries(self.target.libraries)
        self.symbols += "blinker::TimeSyncService::poll()"
        with self.assertRaisesRegex(SystemExit, "retained a product"):
            self.verify()

    def test_real_callback_api_and_link_outputs_are_required(self):
        for symbol in ("sample()", "delayed(void*)", "blinker::BlinkerClass::timeStatus("):
            original = self.symbols
            self.symbols = original.replace(symbol, "optimized out")
            with self.assertRaisesRegex(SystemExit, "not retained"):
                self.verify()
            self.symbols = original
        (self.build / "probe.elf").unlink()
        with self.assertRaisesRegex(SystemExit, "one actual ELF"):
            self.verify()

    def test_only_r4_has_exact_sdk_storage_compile_dependencies(self):
        self.assertEqual(timer.TARGETS["uno-r4-sketch-timer"].libraries,
                         frozenset(("BlinkerDeviceV2", "Preferences", "WiFiS3")))
        for name in ("esp32-sketch-timer", "wio-terminal-sketch-timer"):
            self.assertEqual(timer.TARGETS[name].libraries, frozenset(("BlinkerDeviceV2",)))

    def test_main_only_compiles_clean_serial_probe_in_existing_group(self):
        core = self.data / self.target.core
        core.mkdir(parents=True)
        (core / "platform.txt").write_text("existing fixture SDK")
        commands = []

        def compile_fixture(command, environment, *, log_path):
            commands.append(command)
            self.assertEqual(command[1], "compile")
            self.assertIn("--clean", command)
            self.assertEqual(command[command.index("--jobs") + 1], "1")
            self.assertEqual(environment["ARDUINO_DIRECTORIES_USER"], str(self.work / "sketchbook"))
            self.assertEqual(environment["ARDUINO_DIRECTORIES_DATA"], str(self.data))
            log_path.parent.mkdir(parents=True)
            log_path.write_text("fixture compile, no SDK process")
            return "Sketch uses fixture bytes"

        argv = ["verify_installed_sketch_timer.py", "--zip-path", str(self.archive),
                "--work-dir", str(self.work), "--data-dir", str(self.data),
                "--target", "esp32-sketch-timer"]
        with patch.object(sys, "argv", argv), patch.object(timer.gate, "executable", return_value="fixture-cli"), \
             patch.object(timer.gate, "installed_libraries", return_value={"BlinkerDeviceV2": {
                 "version": "0.1.0", "install_dir": str(self.installed)}}), \
             patch.object(timer.gate, "run", side_effect=compile_fixture), \
             patch.object(timer, "verify_build", return_value={"symbols": self.symbols, "implementationUnits": 17}), \
             patch("builtins.print"):
            self.assertEqual(timer.main(), 0)
        self.assertEqual(len(commands), 1)  # No lib/core install, upload or hardware command.
        report = json.loads((self.work / "build/esp32-sketch-timer/timer-api.json").read_text())
        self.assertEqual(report["archiveSha256"], timer.digest(self.archive))
        self.assertEqual(report["inputs"][str(timer.PROBE.relative_to(REPOSITORY))], timer.digest(timer.PROBE))
        self.assertIn("complete link only", report["scope"])


if __name__ == "__main__":
    unittest.main()
