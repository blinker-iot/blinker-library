"""Finite public identity and native preprocessing checks; no firmware build."""
import copy
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import arduino_release_profile as profile
import prepare_r4_restricted_release as release

COMPILER = Path("C:/Users/LENOVO/AppData/Local/Arduino15/packages/arduino/tools/arm-none-eabi-gcc/7-2017q4/bin")
LANGUAGES = ("c", "c++", "assembler-with-cpp")
CONTRACT = "BlinkerV2/arduino/ports/uno_r4/UnoR4TaskBuildContract.h"
MISMATCH = "R4 maintenance profile/library mismatch"


class ProfileFixture(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="r4 identity with spaces ")
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.source = self.work / "repository"
        config = self.source / "src/BlinkerV2/arduino/config"
        config.mkdir(parents=True)
        (self.source / "library.properties").write_bytes(b"name=BlinkerDeviceV2\nversion=0.1.0\n")
        (self.source / "src/Example.h").write_bytes(b"#pragma once\n#define EXAMPLE_ABI 1\n")
        (config / "LibraryBuildIdentity.h").write_bytes(b"// Unconfigured source\n")
        for name in ("OfficialConfig.h", "Esp32ServerKeys.h", "PortableServerKeys.h"):
            (config / name).write_bytes(b"// Public placeholder\n")
        self.payload = json.loads((ROOT / "tests/release/development_profile.json").read_text())
        self.public_profile = profile.parse_profile(self.payload)

    def configured(self, root=None, payload=None):
        return profile.configured_library_files(root or self.source,
            profile.parse_profile(payload) if payload is not None else self.public_profile)

    @staticmethod
    def bytes(value):
        return value if isinstance(value, bytes) else value.read_bytes()


class PublicIdentity(ProfileFixture):
    def test_normal_zip_apply_and_restricted_source_map_share_exact_identity(self):
        desired = self.configured()
        stage = self.work / "normal single library stage"
        shutil.copytree(self.source, stage)
        profile.apply_profile(stage, self.public_profile)
        for name, value in desired.items():
            self.assertEqual((stage / name).read_bytes(), self.bytes(value), name)
        with patch.object(release, "ROOT", self.source):
            maintained = release.configured_library(self.public_profile)
        self.assertEqual({key: self.bytes(value) for key, value in maintained.items()},
                         {key: self.bytes(value) for key, value in desired.items()})
        header = (stage / profile.LIBRARY_IDENTITY_HEADER).read_text()
        self.assertIn("BLINKER_LIBRARY_BUILD_IDENTITY_VERSION 1", header)
        self.assertNotIn("namespace", header)
        self.assertNotIn("#include", header)

    def test_paths_credentials_and_examples_are_not_identity_inputs(self):
        before = profile.library_identity_words(self.configured())
        (self.source / "examples").mkdir()
        (self.source / "examples/Example.ino").write_bytes(b"// Different user graph\n")
        (self.source / "UserCredentials.h").write_bytes(b'// Fake test data, never real credentials\n#define TEST_KEY "fake-a"\n')
        renamed = self.work / "another machine layout"
        shutil.copytree(self.source, renamed)
        (renamed / "UserCredentials.h").write_bytes(b'// Different fake credential\n#define TEST_KEY "fake-b"\n')
        (renamed / "examples/Example.ino").write_bytes(b"// Changed non-input\n")
        self.assertEqual(profile.library_identity_words(self.configured()), before)
        self.assertEqual(profile.library_identity_words(self.configured(renamed)), before)

    def test_source_change_changes_identity(self):
        before = profile.library_identity_words(self.configured())
        (self.source / "src/Example.h").write_bytes(b"#pragma once\n#define EXAMPLE_ABI 2\n")
        self.assertNotEqual(profile.library_identity_words(self.configured()), before)

    def test_public_host_change_changes_identity(self):
        changed = copy.deepcopy(self.payload)
        changed["control"]["host"] = "another-public-control.invalid"
        self.assertNotEqual(profile.library_identity_words(self.configured(payload=changed)),
                            profile.library_identity_words(self.configured()))

    def test_public_ed25519_key_change_changes_identity(self):
        changed = copy.deepcopy(self.payload)
        original = changed["server_keys"]["ed25519"][0]["public_key"]
        changed["server_keys"]["ed25519"][0]["public_key"] = ("c7" if original[:2] != "c7" else "d7") + original[2:]
        self.assertNotEqual(profile.library_identity_words(self.configured(payload=changed)),
                            profile.library_identity_words(self.configured()))

    def test_identity_excludes_itself_and_is_independent_of_map_order(self):
        values = self.configured()
        expected = profile.library_identity_words(values)
        values[profile.LIBRARY_IDENTITY_HEADER] = b"arbitrary stale generated contents"
        self.assertEqual(profile.library_identity_words(values), expected)
        self.assertEqual(profile.library_identity_words(dict(reversed(tuple(values.items())))), expected)
        self.assertEqual(len(expected), 8)
        self.assertTrue(all(0 <= word <= 0xFFFFFFFF for word in expected))
        del values[profile.LIBRARY_IDENTITY_HEADER]
        self.assertEqual(profile.library_identity_words(values), expected)


class OriginalCompilerBinding(ProfileFixture):
    @classmethod
    def setUpClass(cls):
        for name in ("arm-none-eabi-gcc.exe", "arm-none-eabi-g++.exe"):
            if not (COMPILER / name).is_file():
                raise unittest.SkipTest("original ARM GCC 7-2017q4 is not installed")

    def setUp(self):
        super().setUp()
        self.library = self.work / "single installed library"
        shutil.copytree(self.source, self.library)
        self.values = self.configured()
        profile.apply_profile(self.library, self.public_profile)
        self.wiring = self.work / "maintenance board"
        self.wiring.mkdir()
        # Only public preprocessing prerequisites; no SDK owner or runtime is
        # fabricated, compiled, linked or executed by these -E checks.
        candidate = ["#pragma once", "#define MQTT_MAX_PACKET_SIZE 361U",
                     "#define BLINKER_DEVICE_FRAME_SIZE 256"]
        for name, value in release.candidate.LIMITS.items():
            candidate.append(f"#define BLINKER_INTERNAL_R4_{name} {value}U")
        (self.wiring / "CandidateConfig.h").write_text("\n".join(candidate) + "\n")
        self.config = self.wiring / "Config.h"
        self.config.write_bytes(release.fixed_config(self.library, self.values))

    def preprocess(self, language, body, config=True):
        source = self.work / "preprocess-input.inc"
        source.write_text(body)
        executable = "arm-none-eabi-g++.exe" if language == "c++" else "arm-none-eabi-gcc.exe"
        arguments = [str(COMPILER / executable), "-E", "-P", "-x", language,
                     "-DARDUINO_UNOWIFIR4=1", "-DARDUINO_ARCH_RENESAS_UNO=1", "-DNO_USB",
                     "-I", str(ROOT / "src")]
        if config:
            arguments += ["-include", str(self.config)]
        arguments += [str(source)]
        return subprocess.run(arguments, capture_output=True, text=True, timeout=30, check=False)

    def test_matching_pure_binding_passes_c_cpp_and_assembly(self):
        for language in LANGUAGES:
            with self.subTest(language=language):
                contract = f"#include <{CONTRACT}>\n" if language == "c++" else ""
                result = self.preprocess(language, contract +
                    "#if BLINKER_INTERNAL_R4_LIBRARY_BINDING != 1\n#error Missing binding marker\n#endif\nBINDING_MATCHED\n")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("BINDING_MATCHED", result.stdout)

    def test_missing_identity_header_rejects_c_cpp_and_assembly(self):
        (self.library / profile.LIBRARY_IDENTITY_HEADER).unlink()
        for language in LANGUAGES:
            with self.subTest(language=language):
                result = self.preprocess(language, "SHOULD_NOT_PASS\n")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("LibraryBuildIdentity.h", result.stderr)

    def test_old_unconfigured_header_rejects_c_cpp_and_assembly(self):
        (self.library / profile.LIBRARY_IDENTITY_HEADER).write_text(
            "#pragma once\n#define BLINKER_LIBRARY_BUILD_IDENTITY_VERSION 0\n")
        for language in LANGUAGES:
            with self.subTest(language=language):
                result = self.preprocess(language, "SHOULD_NOT_PASS\n")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(MISMATCH, result.stderr)

    def test_changed_generated_identity_rejects_c_cpp_and_assembly(self):
        changed = dict(self.values)
        changed["src/Example.h"] = b"#pragma once\n#define EXAMPLE_ABI 999\n"
        (self.library / profile.LIBRARY_IDENTITY_HEADER).write_text(profile.render_library_identity(changed))
        for language in LANGUAGES:
            with self.subTest(language=language):
                result = self.preprocess(language, "SHOULD_NOT_PASS\n")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(MISMATCH, result.stderr)

    def test_real_task_contract_rejects_old_board_without_binding_marker(self):
        include = ('#include "' + (self.library / profile.LIBRARY_IDENTITY_HEADER).resolve().as_posix() + '"').encode()
        old_recipe = self.config.read_bytes().split(include, 1)
        self.assertEqual(len(old_recipe), 2)
        self.config.write_bytes(old_recipe[0])
        result = self.preprocess("c++", f"#include <{CONTRACT}>\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("matching maintained library/profile binding", result.stderr)

    def test_default_zero_slots_need_no_binding_provider(self):
        (self.library / profile.LIBRARY_IDENTITY_HEADER).unlink()
        for language in LANGUAGES:
            with self.subTest(language=language):
                result = self.preprocess(language, f"#include <{CONTRACT}>\n"
                    "#if BLINKER_TASK_CAPACITY != 0\n#error Ordinary task capacity changed\n#endif\n"
                    "#ifdef BLINKER_INTERNAL_R4_LIBRARY_BINDING\n#error Unexpected binding provider\n#endif\n"
                    "ORDINARY_ZERO_MATCHED\n", config=False)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("ORDINARY_ZERO_MATCHED", result.stdout)


if __name__ == "__main__":
    unittest.main()
