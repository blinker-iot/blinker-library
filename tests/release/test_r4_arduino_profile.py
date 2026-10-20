"""Finite side-by-side IDE profile tests; no SDK/compiler/hardware execution."""
import copy
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import install_r4_arduino_profile as installer
import prepare_r4_restricted_release as release


class ArduinoProfile(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="r4 IDE profile fixture ")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.sdk = self.root / "original SDK/1.5.3"
        self.compiler = self.root / "compiler/7-2017q4/bin"
        self.bossac = self.root / "uploader/1.9.1-arduino5"
        self.book = self.root / "user sketchbook with spaces"
        self.target = self.book / "hardware/blinker/renesas_uno"
        self.source = self.root / "repository source"
        self.profile = self.root / "public profile.json"
        self.payload = json.loads((ROOT / "tests/release/development_profile.json").read_text())
        self.profile.write_text(json.dumps(self.payload), encoding="utf-8")
        source_names = ["library.properties", "src/BlinkerWiFi.h",
                        "src/BlinkerV2/arduino/config/TaskConfig.h",
                        "src/BlinkerV2/arduino/config/OfficialConfig.h",
                        "src/BlinkerV2/arduino/config/Esp32ServerKeys.h",
                        "src/BlinkerV2/arduino/config/PortableServerKeys.h",
                        "src/BlinkerV2/arduino/config/LibraryBuildIdentity.h",
                        "src/BlinkerV2/arduino/ports/uno_r4/UnoR4HeapBoundary.h"]
        source_names += ["src/BlinkerV2/" + name for name in release.CORE_CONTRACT_HEADERS]
        source_names += ["examples/R4TasksQuickStart/" + name for name in
                         ("R4TasksQuickStart.ino", "UserCredentials.h", "README.md")]
        for name in source_names:
            self.write(self.source / name, (ROOT / name).read_bytes())
        self.write(self.sdk / "platform.txt", b"""name=Original Renesas boards
version=1.5.3
compiler.c.extra_flags=
compiler.cpp.extra_flags=
compiler.S.extra_flags=
compiler.c.elf.extra_flags=
tools.bossac.path={runtime.tools.bossac-1.9.1-arduino5.path}
recipe.cpp.o.pattern=original CPP compile recipe
recipe.c.combine.pattern=original link recipe
""")
        self.write(self.sdk / "boards.txt", b"""unor4wifi.name=Arduino UNO R4 WiFi
unor4wifi.build.core=arduino
unor4wifi.build.crossprefix=arm-none-eabi-
unor4wifi.build.compiler_path={runtime.tools.arm-none-eabi-gcc-7-2017q4.path}/bin/
unor4wifi.build.variant=UNOWIFIR4
unor4wifi.build.mcu=cortex-m4
unor4wifi.build.board=UNOWIFIR4
unor4wifi.build.defines=-DF_CPU=48000000 -DNO_USB -DARDUINO_UNOR4_WIFI
unor4wifi.upload.tool=bossac
unor4wifi.upload.tool.default=bossac
unor4wifi.upload.maximum_size=262144
unor4wifi.upload.maximum_data_size=32768
""")
        for name in ("Modem.cpp", "Modem.h", "WiFiCommands.h", "StringHelpers.h"):
            self.write(self.sdk / "libraries/WiFiS3/src" / name, ("original " + name).encode())
        for name in ("Serial.cpp", "Serial.h"):
            self.write(self.sdk / "cores/arduino" / name, ("original " + name).encode())
        self.write(self.sdk / "cores/arduino/main.cpp", b"original SDK main")
        self.write(self.sdk / "variants/UNOWIFIR4/libs/libfsp.a", b"original SDK archive")
        for name in ("arm-none-eabi-gcc", "arm-none-eabi-g++", "arm-none-eabi-ar"):
            self.write(self.compiler / (name + ".exe"), b"pinned compiler fixture")
        self.write(self.bossac / "bossac.exe", b"pinned uploader fixture")
        self.enterContext(patch.object(release, "ROOT", self.source))
        self.enterContext(patch.object(installer, "ROOT", self.source))
        self.enterContext(patch.object(release.candidate, "source_paths", side_effect=lambda: {
            path.relative_to(self.source).as_posix(): path
            for path in self.source.rglob("*") if path.is_file()}))
        self.base = {"sdk-overlay/" + name: ("reviewed overlay " + name).encode() for name in
                     ("Modem.cpp", "Modem.h", "WiFiCommands.h", "StringHelpers.h", "Serial.cpp", "Serial.h")}
        self.base.update({
            "CandidateConfig.h": b"// reviewed fixed candidate\n#define MQTT_MAX_PACKET_SIZE 361U\n",
            "HeapBoundary.cpp": b'#include "CandidateConfig.h"\n#include "TaskHeapBoundary.h"\n',
            "TaskHeapBoundary.h": (ROOT / release.candidate.HEAP_ADAPTER).read_bytes(),
        })
        self.enterContext(patch.object(release.candidate, "expected_candidate", return_value=(
            self.base, {"linkFlags": ["-Wl,--wrap=_sbrk", "-Wl,--defsym=__blinker_r4_heap_limit=__StackTop-6144"]})))
        self.library = self.book / "libraries/BlinkerDeviceV2"
        parsed = release.restricted_profile(self.profile)
        for name, value in release.configured_library(parsed).items():
            self.write(self.library / name, self.content(value))
        reference = "examples/R4TasksQuickStart/"
        for name in ("R4TasksQuickStart.ino", "UserCredentials.h", "README.md"):
            self.write(self.library / reference / name, (self.source / reference / name).read_bytes())
        # This is normal user input, not an installer-owned or receipt-bound file.
        self.credentials = self.library / reference / "UserCredentials.h"
        self.credentials.write_text('#define R4_DEVICE_KEY "PRIVATE_TEST_ONLY"\n', encoding="utf-8")
        for name, version in (("PubSubClient", "2.8"), ("Crypto", "0.4.0")):
            self.write(self.book / "libraries" / name / "library.properties",
                       f"name={name}\nversion={version}\n".encode())
            self.write(self.book / "libraries" / name / "src/Fixed.cpp", b"original dependency fixture")
        self.other_board = self.book / "hardware/personal/custom/boards.txt"
        self.user_library = self.book / "libraries/MyOwnLibrary/src/Personal.h"
        self.write(self.other_board, b"other user board, never replace")
        self.write(self.user_library, b"user library, never replace")
        self.inputs_before = self.snapshot_inputs()

    @staticmethod
    def content(value):
        return value if isinstance(value, bytes) else value.read_bytes()

    @staticmethod
    def write(path, content):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)

    def call(self, operation=installer.install):
        return operation(self.sdk, self.compiler, self.bossac, self.profile, self.book)

    def snapshot_inputs(self):
        directories = (self.sdk, self.compiler, self.bossac, self.source, self.book / "libraries")
        return {str(path): path.read_bytes() for directory in directories
                for path in directory.rglob("*") if path.is_file()}

    def configure_installed_library(self):
        parsed = release.restricted_profile(self.profile)
        for name, value in release.configured_library(parsed).items():
            self.write(self.library / name, self.content(value))
        return parsed

    def installed_profile_bytes(self):
        return {name: (self.target / name).read_bytes() for name in release.tree(self.target)}

    def binding_words(self, files):
        relative = "src/BlinkerV2/arduino/config/LibraryBuildIdentity.h"
        header = (self.library / relative).read_text(encoding="utf-8")
        words = [int(re.search(r"^#define BLINKER_LIBRARY_BUILD_IDENTITY_" + str(index)
                               + r" 0x([A-F0-9]{8})UL$", header, re.MULTILINE)[1], 16)
                 for index in range(8)]
        config = self.content(files["blinker/Config.h"]).decode()
        self.assertIn((self.library / relative).as_posix(), config)
        self.assertIn("#define BLINKER_INTERNAL_R4_LIBRARY_BINDING 1", config)
        for word in words:
            self.assertIn(f"0x{word:08X}UL", config)
        return words

    def test_new_binding_changes_for_host_or_key_without_source_changes(self):
        files, receipt = self.call(installer.expected)
        before = self.binding_words(files)
        source_before = {name: file.read_bytes() for name, file in release.tree(self.source).items()}
        for field in ("host", "key"):
            payload = copy.deepcopy(self.payload)
            if field == "host":
                payload["control"]["host"] = "other-device-v2-ci.invalid"
            else:
                payload["server_keys"]["ed25519"][0]["public_key"] = (
                    "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c")
            self.profile.write_text(json.dumps(payload), encoding="utf-8")
            self.configure_installed_library()
            changed_files, changed_receipt = self.call(installer.expected)
            with self.subTest(field=field):
                self.assertNotEqual(self.binding_words(changed_files), before)
                self.assertNotEqual(self.content(changed_files["blinker/Config.h"]),
                                    self.content(files["blinker/Config.h"]))
                self.assertNotEqual(changed_receipt, receipt)
                self.assertEqual({name: file.read_bytes() for name, file in release.tree(self.source).items()},
                                 source_before)
        self.assertFalse(self.target.exists())

    def test_new_explicit_verify_rejects_source_changed_without_restamping(self):
        self.call()
        before = self.installed_profile_bytes()
        identity = self.library / "src/BlinkerV2/arduino/config/LibraryBuildIdentity.h"
        identity_before = identity.read_bytes()
        file = self.library / "src/BlinkerWiFi.h"
        changed = file.read_bytes() + b"\n// User source edit without regenerating its identity.\n"
        file.write_bytes(changed)
        with self.assertRaises(ValueError):
            self.call(installer.verify)
        self.assertEqual(identity.read_bytes(), identity_before)
        self.assertEqual(file.read_bytes(), changed)
        self.assertEqual(self.installed_profile_bytes(), before)

    def test_new_ordered_upgrade_uninstalls_before_changing_source_library_profile(self):
        first = self.call()
        self.assertEqual(self.call(installer.verify), first)
        old_bytes = self.installed_profile_bytes()
        old_identity = (self.library / "src/BlinkerV2/arduino/config/LibraryBuildIdentity.h").read_bytes()
        self.call(installer.uninstall)
        backups = list((self.book / ".blinker-profile-backups").glob("*/renesas_uno"))
        self.assertEqual(len(backups), 1)
        self.assertFalse(self.target.exists())
        source = self.source / "src/BlinkerWiFi.h"
        source.write_bytes(source.read_bytes() + b"\n// Next maintained source revision.\n")
        payload = copy.deepcopy(self.payload)
        payload["control"]["host"] = "next-device-v2-ci.invalid"
        self.profile.write_text(json.dumps(payload), encoding="utf-8")
        self.configure_installed_library()
        second = self.call()
        self.assertEqual(self.call(installer.verify), second)
        self.assertNotEqual(first, second)
        self.assertNotEqual((self.library / "src/BlinkerV2/arduino/config/LibraryBuildIdentity.h").read_bytes(),
                            old_identity)
        self.assertEqual({name: (backups[0] / name).read_bytes() for name in release.tree(backups[0])}, old_bytes)
        for name, value in self.inputs_before.items():
            if Path(name).is_relative_to(self.sdk) or Path(name).is_relative_to(self.compiler) or Path(name).is_relative_to(self.bossac):
                self.assertEqual(Path(name).read_bytes(), value)
        self.assertIn("#define BLINKER_TASK_CAPACITY 0", (self.library / "src/BlinkerV2/arduino/config/TaskConfig.h").read_text())
        self.assertEqual(self.other_board.read_bytes(), b"other user board, never replace")
        self.assertEqual(self.user_library.read_bytes(), b"user library, never replace")
        self.assertIn("PRIVATE_TEST_ONLY", self.credentials.read_text())

    def test_new_update_before_uninstall_refuses_library_or_tool_drift(self):
        generator = self.source / "tools/prepare_r4_internal_candidate.py"
        self.write(generator, b"# matched original generator fixture\n")
        self.call()
        before = self.installed_profile_bytes()
        for drift in ("library-profile", "tool-input"):
            if drift == "library-profile":
                payload = copy.deepcopy(self.payload)
                payload["control"]["host"] = "changed-too-soon.invalid"
                self.profile.write_text(json.dumps(payload), encoding="utf-8")
                self.configure_installed_library()
            else:
                generator.write_bytes(b"# changed generator fixture\n")
            for operation in (installer.verify, installer.install, installer.uninstall):
                with self.subTest(drift=drift, operation=operation.__name__), self.assertRaises(ValueError):
                    self.call(operation)
                self.assertEqual(self.installed_profile_bytes(), before)
            self.profile.write_text(json.dumps(self.payload), encoding="utf-8")
            self.configure_installed_library()
            generator.write_bytes(b"# matched original generator fixture\n")
        self.assertEqual(self.other_board.read_bytes(), b"other user board, never replace")
        self.assertEqual(self.user_library.read_bytes(), b"user library, never replace")

    def test_matching_install_and_verify_are_idempotent_without_running_tools(self):
        with patch("subprocess.run") as run:
            receipt = self.call()
            first = {name: (self.target / name).stat().st_mtime_ns
                     for name in release.tree(self.target)}
            self.assertEqual(self.call(installer.verify), receipt)
            self.assertEqual(self.call(), receipt)
            self.assertEqual(first, {name: (self.target / name).stat().st_mtime_ns
                                     for name in release.tree(self.target)})
            run.assert_not_called()
        self.assertEqual(receipt["schema"], "r4-arduino-ide-profile/1")
        self.assertEqual(receipt["fqbn"], "blinker:renesas_uno:r4tasks")
        self.assertEqual(Path(receipt["target"]), self.target)
        for name in ("installedSdkModified", "libraryModified", "hardwareWritten"):
            self.assertIs(receipt[name], False)
        self.assertNotIn("PRIVATE_TEST_ONLY", json.dumps(receipt))

    def test_space_path_profile_wires_one_board_all_consumers_and_one_heap_tu(self):
        files, _ = self.call(installer.expected)
        self.assertIn(" ", str(self.book))
        boards = self.content(files["boards.txt"]).decode()
        self.assertIn("r4tasks.name=", boards)
        self.assertIn("r4tasks.build.core=arduino", boards)
        self.assertNotIn("unor4wifi.name=", boards)
        text = "\n".join(self.content(value).decode(errors="replace")
                         for name, value in files.items() if name.endswith(".txt"))
        for language in ("c", "cpp", "S"):
            self.assertIn("compiler." + language + ".extra_flags=", text)
        for token in ("-include", "Config.h", "--wrap=_sbrk", "__StackTop-6144"):
            self.assertIn(token, text)
        self.assertIn(str(self.compiler).replace("\\", "/"), text)
        self.assertIn(str(self.bossac).replace("\\", "/"), text)
        heap_tus = [name for name in files if name.startswith("cores/arduino/")
                    and name.endswith(".cpp") and "HeapBoundary" in name]
        self.assertEqual(len(heap_tus), 1)
        self.assertFalse(any(name.startswith("sketch/") or name.startswith("libraries/Blinker") for name in files))
        mirrored = {name.removeprefix("cores/arduino/BlinkerV2/") for name in files
                    if name.startswith("cores/arduino/BlinkerV2/")}
        self.assertEqual(mirrored, set(release.CORE_CONTRACT_HEADERS) |
                         {"arduino/ports/uno_r4/UnoR4HeapBoundary.h"})
        for name in ("Modem.cpp", "Modem.h", "WiFiCommands.h", "StringHelpers.h", "Serial.cpp", "Serial.h"):
            prefix = "cores/arduino/" if name.startswith("Serial.") else "libraries/WiFiS3/src/"
            self.assertEqual(self.content(files[prefix + name]), self.base["sdk-overlay/" + name])

    def test_original_sdk_libraries_credentials_and_default_zero_graph_are_unchanged(self):
        self.call()
        self.assertEqual(self.snapshot_inputs(), self.inputs_before)
        task_config = (self.library / "src/BlinkerV2/arduino/config/TaskConfig.h").read_text()
        self.assertIn("#define BLINKER_TASK_CAPACITY 0", task_config)
        ordinary = (self.library / "src/BlinkerV2/arduino/config/OfficialConfig.h").read_text()
        self.assertNotIn("#define BLINKER_ENABLE_TASKS", ordinary)
        self.assertNotIn("#define BLINKER_TASK_CAPACITY", ordinary)
        self.assertEqual(self.other_board.read_bytes(), b"other user board, never replace")
        self.assertEqual(self.user_library.read_bytes(), b"user library, never replace")

    def test_modified_owned_file_refuses_verify_reinstall_and_uninstall(self):
        self.call()
        file = self.target / "cores/arduino/Serial.cpp"
        file.write_bytes(file.read_bytes() + b"user change")
        before = file.read_bytes()
        for operation in (installer.verify, installer.install, installer.uninstall):
            with self.subTest(operation=operation.__name__), self.assertRaises(ValueError):
                self.call(operation)
            self.assertEqual(file.read_bytes(), before)

    def test_unlisted_file_refuses_all_mutations(self):
        self.call()
        file = self.target / "user-notes.txt"
        file.write_bytes(b"user owned")
        for operation in (installer.verify, installer.install, installer.uninstall):
            with self.subTest(operation=operation.__name__), self.assertRaises(ValueError):
                self.call(operation)
            self.assertEqual(file.read_bytes(), b"user owned")

    def test_receipt_edit_does_not_authorize_changed_or_unknown_outputs(self):
        self.call()
        file = self.target / "blinker-profile.json"
        payload = json.loads(file.read_text())
        payload["target"] = "arduino:renesas_uno:unor4wifi"
        file.write_text(json.dumps(payload), encoding="utf-8")
        for operation in (installer.verify, installer.install, installer.uninstall):
            with self.subTest(operation=operation.__name__), self.assertRaises(ValueError):
                self.call(operation)
        self.assertTrue(self.target.exists())

    def test_unknown_existing_target_is_never_adopted_or_overwritten(self):
        file = self.target / "boards.txt"
        self.write(file, b"unknown user platform")
        for operation in (installer.install, installer.uninstall):
            with self.subTest(operation=operation.__name__), self.assertRaises(ValueError):
                self.call(operation)
            self.assertEqual(file.read_bytes(), b"unknown user platform")
        self.assertFalse((self.target / "blinker-profile.json").exists())

    def test_wrong_profile_core_compiler_or_local_overrides_fail_before_install(self):
        cases = [(self.sdk / "platform.txt", b"version=1.5.4\n"),
                 (self.sdk / "platform.local.txt", b"compiler.cpp.extra_flags=unsafe\n"),
                 (self.sdk / "boards.local.txt", b"unor4wifi.build.core=other\n")]
        payload = copy.deepcopy(self.payload)
        payload["control"]["mqtt_tls"] = True
        cases.append((self.profile, json.dumps(payload).encode()))
        for file, bad_value in cases:
            existed, previous = file.exists(), file.read_bytes() if file.exists() else None
            file.write_bytes(bad_value)
            with self.subTest(path=file.name), self.assertRaises(ValueError):
                self.call()
            self.assertFalse(self.target.exists())
            if existed:
                file.write_bytes(previous)
            else:
                file.unlink()
        original = self.compiler
        self.compiler = self.root / "compiler/other-version/bin"
        shutil.copytree(original, self.compiler)
        with self.assertRaises(ValueError):
            self.call()
        self.assertFalse(self.target.exists())

    def test_duplicate_or_mismatched_installed_library_fails_before_install(self):
        duplicate = self.book / "libraries/AnotherBlinker"
        shutil.copytree(self.library, duplicate)
        with self.assertRaises(ValueError):
            self.call()
        # Fixture-only rename keeps test cleanup separate from product uninstall.
        duplicate.rename(self.root / "not-a-user-library")
        file = self.book / "libraries/PubSubClient/library.properties"
        file.write_text("name=PubSubClient\nversion=2.9\n", encoding="utf-8")
        with self.assertRaises(ValueError):
            self.call()
        file.write_text("name=PubSubClient\nversion=2.8\n", encoding="utf-8")
        file = self.library / "src/BlinkerWiFi.h"
        file.write_bytes(b"different library source")
        with self.assertRaises(ValueError):
            self.call()
        self.assertFalse(self.target.exists())

    def test_linked_target_or_input_is_not_followed(self):
        external = self.root / "external user platform"
        external.mkdir()
        self.write(external / "boards.txt", b"external user source")
        self.target.parent.mkdir(parents=True)
        def directory_link(link, destination):
            try:
                link.symlink_to(destination, target_is_directory=True)
            except OSError as error:
                if sys.platform != "win32":
                    raise error
                # Junctions need no elevated privilege on Windows. Exercise
                # the actual reparse-point boundary when symlink creation is
                # unavailable rather than skipping this safety regression.
                import _winapi
                _winapi.CreateJunction(str(destination), str(link))

        directory_link(self.target, external)
        for operation in (installer.install, installer.uninstall):
            with self.subTest(operation=operation.__name__), self.assertRaises(ValueError):
                self.call(operation)
        self.assertEqual((external / "boards.txt").read_bytes(), b"external user source")
        if self.target.is_symlink():
            self.target.unlink()
        else:
            self.target.rmdir()  # Remove only the fixture junction, not its target.
        link = self.sdk / "cores/arduino/linked"
        directory_link(link, self.book / "libraries/MyOwnLibrary")
        with self.assertRaises(ValueError):
            self.call()
        self.assertFalse(self.target.exists())

    def test_uninstall_preserves_exact_profile_as_backup_and_other_user_content(self):
        self.call()
        before = {name: (self.target / name).read_bytes() for name in release.tree(self.target)}
        self.call(installer.uninstall)
        self.assertFalse(self.target.exists())
        backups = list((self.book / ".blinker-profile-backups").glob("*/renesas_uno"))
        self.assertEqual(len(backups), 1)
        self.assertEqual({name: (backups[0] / name).read_bytes()
                          for name in release.tree(backups[0])}, before)
        self.assertEqual(self.snapshot_inputs(), self.inputs_before)
        self.assertEqual(self.other_board.read_bytes(), b"other user board, never replace")
        self.assertEqual(self.user_library.read_bytes(), b"user library, never replace")


if __name__ == "__main__":
    unittest.main()
