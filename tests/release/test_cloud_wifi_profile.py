"""Restricted Cloud WiFi release; no network, credentials, SDK build or hardware."""
import copy
import hashlib
import json
from pathlib import Path
import shutil
import ssl
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tests/arduino'))
import arduino_release_profile as profile
import build_arduino_release as release
import verify_installed_release as installed

PUBLIC_PROFILE = ROOT / 'profiles/cloud-beta.json'
E46_DER_SHA256 = 'c90f26f0fb1b4018b22227519b5ca2b53e2ca5b3be5cf18efe1bef47380c5383'
CONFIG = 'src/BlinkerV2/arduino/config/'


class CloudProfile(unittest.TestCase):
    def setUp(self):
        self.payload = json.loads(PUBLIC_PROFILE.read_text(encoding='utf-8'))
        self.parsed = profile.parse_profile(self.payload)

    def test_public_profile_matches_accepted_transport_and_no_business_signer(self):
        self.assertEqual(self.parsed.product_profile, profile.CLOUD_WIFI_PROFILE_ID)
        self.assertTrue(self.parsed.production)
        self.assertEqual((self.parsed.control_host, self.parsed.control_port), ('iot.yiyu.pro', 443))
        self.assertTrue(self.parsed.cloud_session_tls and self.parsed.mqtt_tls)
        self.assertEqual((self.parsed.p256_keys, self.parsed.ed25519_keys), ((), ()))
        self.assertIsNone(self.parsed.task_profile)
        for ca in (self.parsed.cloud_session_ca_pem, self.parsed.mqtt_ca_pem):
            self.assertEqual(hashlib.sha256(ssl.PEM_cert_to_DER_cert(ca)).hexdigest(), E46_DER_SHA256)

    def test_scope_channel_and_task_profile_cannot_be_relabelled(self):
        for field, value in (('product_profile', None), ('product_profile', 'ordinary'),
                             ('channel', 'development'), ('task_profile', None),
                             ('task_profile', profile.TASK_PILOT_ID)):
            payload = {**self.payload, field: value}
            with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                profile.parse_profile(payload)

    def test_plain_wrong_port_or_missing_ca_are_rejected(self):
        for field, value in (('cloud_session_tls', False), ('mqtt_tls', False),
                             ('port', 80), ('port', 8883),
                             ('cloud_session_ca_pem', ''), ('mqtt_ca_pem', '')):
            payload = copy.deepcopy(self.payload)
            payload['control'][field] = value
            with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                profile.parse_profile(payload)

    def test_signing_keys_cannot_be_smuggled_into_restricted_graph(self):
        fixture = json.loads((ROOT / 'tests/release/development_profile.json').read_text())
        for algorithm in ('p256', 'ed25519'):
            payload = copy.deepcopy(self.payload)
            payload['server_keys'][algorithm] = fixture['server_keys'][algorithm]
            with self.subTest(algorithm=algorithm), self.assertRaisesRegex(ValueError, 'empty business signing rings'):
                profile.parse_profile(payload)

    def test_empty_rings_are_not_implicitly_admitted_for_existing_profiles(self):
        payload = copy.deepcopy(self.payload)
        del payload['product_profile']
        with self.assertRaisesRegex(ValueError, 'must contain 1 to 4 keys'):
            profile.parse_profile(payload)

    def test_private_unknown_fields_and_placeholder_host_are_rejected(self):
        for field in ('deviceKey', 'password', 'ssid', 'private_key', 'credentials'):
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, 'unknown='):
                profile.parse_profile({**self.payload, field: 'fixture-not-a-secret'})
        payload = copy.deepcopy(self.payload)
        payload['control']['host'] = 'replace-with-control-host.example'
        with self.assertRaisesRegex(ValueError, 'placeholder host'):
            profile.parse_profile(payload)

    def test_empty_headers_use_typed_null_and_no_key_bytes(self):
        for p256, name in ((True, 'P256'), (False, 'Ed25519')):
            rendered = profile.render_key_header(self.parsed, p256=p256)
            with self.subTest(p256=p256):
                self.assertIn(f'const {name}ServerPublicKey* const serverSigningKeys = nullptr;', rendered)
                self.assertIn('serverSigningKeysCount = 0U;', rendered)
                self.assertIn('#define BLINKER_OFFICIAL_HAS_SERVER_SIGNATURE 0', rendered)
                self.assertNotIn('serverSigningKey0Bytes', rendered)
                self.assertNotIn('serverSigningKeys[]', rendered)

    def test_existing_full_profile_keeps_nonempty_keys_and_no_restricted_guard(self):
        fixture = profile.load_profile(ROOT / 'tests/release/development_profile.json')
        self.assertIsNone(fixture.product_profile)
        self.assertFalse(fixture.cloud_wifi_only)
        self.assertIn('serverSigningKey0Bytes[]', profile.render_key_header(fixture, p256=True))
        self.assertNotIn('Cloud WiFi profile', profile.render_config(fixture))
        self.assertNotIn('BLINKER_OFFICIAL_PRODUCT_PROFILE', profile.render_config(fixture))

    def test_installed_target_scope_does_not_expand_to_ble_r4_wio_or_tasks(self):
        config = profile.render_config(self.parsed)
        allowed = installed.select_targets(['esp32-wifi', 'esp32-cloud-smoke'])
        self.assertEqual(installed.verify_installed_product_profile(config, allowed), profile.CLOUD_WIFI_PROFILE_ID)
        self.assertIsNone(installed.verify_installed_task_profile(config, allowed))
        for target in (*installed.TARGETS, installed.TASK_PILOT_TARGET):
            if target in allowed:
                continue
            with self.subTest(target=target.name), self.assertRaisesRegex(SystemExit, 'cloud WiFi profile admits only'):
                installed.verify_installed_product_profile(config, (target,))
        with self.assertRaisesRegex(SystemExit, 'select them explicitly'):
            installed.verify_installed_product_profile(config, installed.select_targets(None))
        legacy = profile.render_config(profile.load_profile(ROOT / 'tests/release/development_profile.json'))
        self.assertIsNone(installed.verify_installed_product_profile(legacy, installed.TARGETS))

    def test_installed_product_marker_cannot_be_changed_or_duplicated(self):
        config = profile.render_config(self.parsed)
        marker = f'#define BLINKER_OFFICIAL_PRODUCT_PROFILE "{profile.CLOUD_WIFI_PROFILE_ID}"'
        for changed in (config.replace(marker, marker.replace(profile.CLOUD_WIFI_PROFILE_ID, 'unknown')),
                        config + marker + '\n'):
            with self.assertRaisesRegex(SystemExit, 'invalid or duplicated'):
                installed.verify_installed_product_profile(changed, installed.select_targets(['esp32-wifi']))

    def test_one_normal_library_archive_is_reproducible_and_has_only_four_derived_headers(self):
        properties, files = release.validate()
        derived = profile.configured_library_files(ROOT, self.parsed)
        self.assertEqual({name for name, value in derived.items() if isinstance(value, bytes)},
                         {CONFIG + name for name in ('OfficialConfig.h', 'Esp32ServerKeys.h',
                                                     'PortableServerKeys.h', 'LibraryBuildIdentity.h')})
        with tempfile.TemporaryDirectory(prefix='cloud-wifi-release-') as directory:
            work = Path(directory)
            archives = []
            for suffix in ('a', 'b'):
                stage = work / suffix / 'stage'
                release.stage(stage, files)
                profile.apply_profile(stage, self.parsed)
                archives.append(release.archive(stage, work / suffix / 'artifacts', properties))
            self.assertEqual(archives[0].read_bytes(), archives[1].read_bytes())
            self.assertEqual(archives[0].name, 'BlinkerDeviceV2-0.1.0.zip')
            installed.verify_installed_bytes(archives[0], work / 'a/stage')
            for name, value in derived.items():
                expected = value if isinstance(value, bytes) else value.read_bytes()
                self.assertEqual((work / 'a/stage' / name).read_bytes(), expected, name)

    def test_public_profile_uses_existing_release_cli_without_development_override(self):
        with tempfile.TemporaryDirectory(prefix='cloud-wifi-cli-') as directory:
            work = Path(directory)
            result = subprocess.run(
                [sys.executable, '-B', str(ROOT / 'tools/build_arduino_release.py'),
                 '--profile', str(PUBLIC_PROFILE), '--stage-dir', str(work / 'stage'),
                 '--output-dir', str(work / 'artifacts')],
                cwd=ROOT, capture_output=True, text=True, timeout=30,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('Arduino release profile applied: production', result.stdout)
            installed.verify_installed_bytes(work / 'artifacts/BlinkerDeviceV2-0.1.0.zip', work / 'stage')


def preprocessor():
    # A finite header check, not an ESP32 SDK build/link or runtime admission.
    for candidate in (shutil.which('g++'), shutil.which('clang++'),
                      'C:/Users/LENOVO/AppData/Local/Arduino15/packages/arduino/tools/'
                      'arm-none-eabi-gcc/7-2017q4/bin/arm-none-eabi-g++.exe'):
        if candidate and Path(candidate).is_file():
            return candidate
    return None


class CloudGraphGuard(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = preprocessor()
        if cls.compiler is None:
            raise unittest.SkipTest('a local C++ preprocessor is not installed')

    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='cloud-profile-headers-')
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.parsed = profile.load_profile(PUBLIC_PROFILE)
        self.headers = {'OfficialConfig.h': profile.render_config(self.parsed),
                        'Esp32ServerKeys.h': profile.render_key_header(self.parsed, p256=True),
                        'PortableServerKeys.h': profile.render_key_header(self.parsed, p256=False)}
        for name, source in self.headers.items():
            (self.work / name).write_text(source, encoding='utf-8')

    def check(self, source, *, esp32=True, syntax=False):
        file = self.work / 'input.cpp'
        file.write_text(source, encoding='utf-8')
        command = [self.compiler, '-std=gnu++11', '-x', 'c++',
                   '-fsyntax-only' if syntax else '-E', '-I', str(self.work),
                   '-I', str(ROOT / 'src')]
        if esp32:
            command.append('-DARDUINO_ARCH_ESP32=1')
        return subprocess.run([*command, str(file)], capture_output=True, text=True, timeout=20)

    def test_manual_wifi_and_zero_extensions_pass(self):
        source = '#define BLINKER_PRODUCT_WIFI 1\n'
        source += ''.join(f'#define {name} 0\n' for name in profile.CLOUD_WIFI_ZERO_FEATURES)
        result = self.check(source + '#include "OfficialConfig.h"\n', syntax=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_non_esp32_architecture_is_rejected(self):
        result = self.check('#include "OfficialConfig.h"\n', esp32=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Cloud WiFi profile supports ESP32 only', result.stderr)

    def test_every_forbidden_graph_is_rejected_before_or_after_first_include(self):
        for header in self.headers:
            for graph in profile.CLOUD_WIFI_FORBIDDEN_GRAPHS:
                for late in (False, True):
                    with self.subTest(header=header, graph=graph, late=late):
                        first = f'#include "{header}"\n' if late else ''
                        result = self.check(first + f'#define {graph} 1\n#include "{header}"\n')
                        self.assertNotEqual(result.returncode, 0)
                        self.assertIn('Cloud WiFi profile forbids', result.stderr)

    def test_positive_task_capacity_and_extensions_are_rejected(self):
        for feature in profile.CLOUD_WIFI_ZERO_FEATURES:
            for value in (1, 4):
                with self.subTest(feature=feature, value=value):
                    result = self.check(f'#define {feature} {value}\n#include "OfficialConfig.h"\n')
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn('Cloud WiFi profile requires zero tasks', result.stderr)

    def test_typed_null_rings_compile_independently(self):
        for header in ('Esp32ServerKeys.h', 'PortableServerKeys.h'):
            with self.subTest(header=header):
                result = self.check(f'#include "{header}"\n'
                                    'static_assert(blinker::official::serverSigningKeysCount == 0U, "zero ring");\n'
                                    '#if BLINKER_OFFICIAL_HAS_SERVER_SIGNATURE != 0\n#error signer exposed\n#endif\n',
                                    syntax=True)
                self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
