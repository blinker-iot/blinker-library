"""Public WiFiProv setup contracts; no SDK link, service, credentials or hardware."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tests/release'))
import build_arduino_release as release
from test_cloud_wifi_profile import preprocessor

EXAMPLE = ROOT / 'examples/WiFiBleQuickStart/WiFiBleQuickStart.ino'
OPTIONS = 'BlinkerV2/arduino/config/Esp32WiFiProvOptions.h'


class PublicSetup(unittest.TestCase):
    def test_example_keeps_default_and_places_optional_private_config_first(self):
        source = EXAMPLE.read_text(encoding='utf8')
        self.assertLess(source.index('// #include "PrivateProvisioningConfig.h"'),
                        source.index('#include <BlinkerWiFiBLE.h>'))
        active = re.sub(r'//[^\n]*', '', source)
        self.assertNotRegex(active, r'#\s*define\s+BLINKER_(?:V2_WIFI_PROV_POP|ESP32_PROVISIONING_)')
        self.assertNotIn('PrivateProvisioningConfig.h', active)
        self.assertIn('Blinker.begin(power, buttonPresses, button)', active)
        self.assertIn('Blinker.run()', active)

    def test_readme_explains_distinct_secrets_and_restricted_release_scope(self):
        source = (ROOT / 'README.md').read_text(encoding='utf8')
        self.assertTrue('## ESP32 WiFi+BLE provisioning' in source,
                        'public README must include provisioning setup instructions')
        section = source.split('## ESP32 WiFi+BLE provisioning', 1)[1].split('\n## ', 1)[0]
        for marker in ('PrivateProvisioningConfig.h', 'BLINKER_ESP32_PROVISIONING_SOFTAP',
                       'BLINKER_V2_WIFI_PROV_POP', '1–64 UTF-8 bytes', 'PoP', 'DeviceKey',
                       'hotspot password', 'cloud-beta.json', 'manual WiFi', 'Save As'):
            self.assertIn(marker, section)
        self.assertIn('does not enable WiFi+BLE', section)
        self.assertIn('not a shared default', section)


class SetupSelector(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = preprocessor()
        if cls.compiler is None:
            raise unittest.SkipTest('a local C++ preprocessor is not installed')
        source = (ROOT / 'src/BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h').read_text(encoding='utf8')
        cls.selector = source.split('#include <sdkconfig.h>', 1)[1].split('#include "Esp32WiFiPlatform.h"', 1)[0]

    def check(self, definitions, assertions='', *, accepted=True):
        with tempfile.TemporaryDirectory(prefix='wifiprov-public-') as directory:
            file = Path(directory) / 'input.cpp'
            file.write_text(definitions + self.selector + '#include <' + OPTIONS + '>\n' + assertions, encoding='utf8')
            result = subprocess.run([self.compiler, '-std=gnu++11', '-x', 'c++', '-fsyntax-only',
                                     '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'src'), str(file)],
                                    capture_output=True, text=True, timeout=20, check=False)
            if accepted:
                self.assertEqual(result.returncode, 0, result.stderr)
            else:
                self.assertNotEqual(result.returncode, 0)
            return result

    def test_default_ble_keeps_explicit_pop_before_product(self):
        self.check('#define CONFIG_BT_NIMBLE_ENABLED 1\n#define BLINKER_V2_WIFI_PROV_POP "fixture-pop"\n',
                   '#if !defined(BLINKER_ESP32_PROVISIONING_BLE) || defined(BLINKER_ESP32_PROVISIONING_SOFTAP)\n#error wrong bearer\n#endif\n'
                   'static_assert(blinker::official::wifiProvProofOfPossession()[0] == \'f\', "same pop");\n')

    def test_explicit_softap_does_not_silently_select_ble(self):
        self.check('#define BLINKER_ESP32_PROVISIONING_SOFTAP 1\n#define BLINKER_V2_WIFI_PROV_POP "fixture-pop"\n',
                   '#ifdef BLINKER_ESP32_PROVISIONING_BLE\n#error unexpected BLE selection\n#endif\n'
                   'static_assert(blinker::official::wifiProvProofOfPossession()[0] == \'f\', "same pop");\n')

    def test_two_bearers_are_rejected(self):
        result = self.check('#define CONFIG_BT_NIMBLE_ENABLED 1\n#define BLINKER_ESP32_PROVISIONING_BLE 1\n'
                            '#define BLINKER_ESP32_PROVISIONING_SOFTAP 1\n', accepted=False)
        self.assertIn('Select only one ESP32 provisioning transport', result.stderr)

    def test_default_ble_never_falls_back_when_nimble_is_missing(self):
        result = self.check('', accepted=False)
        self.assertIn('requires an ESP32 Arduino core built with IDF NimBLE', result.stderr)


class PrivateConfigurationBoundary(unittest.TestCase):
    def test_public_library_remains_valid_and_private_headers_are_not_shipped(self):
        _, files = release.validate()
        self.assertIn(EXAMPLE, files)
        self.assertFalse(any(file.name.casefold() == 'privateprovisioningconfig.h' for file in files))

    def test_release_rejects_private_header_without_removing_it(self):
        _, files = release.validate()
        with tempfile.TemporaryDirectory(prefix='wifiprov-release-') as directory:
            stage = Path(directory) / 'library'
            release.stage(stage, files)
            for name in ('PrivateProvisioningConfig.h', 'privateprovisioningconfig.h', 'PRIVATEPROVISIONINGCONFIG.H'):
                with self.subTest(name=name):
                    private = stage / 'examples/WiFiBleQuickStart' / name
                    private.write_text('#define BLINKER_V2_WIFI_PROV_POP "fixture-private-only"\n', encoding='utf8')
                    arguments = ['build_arduino_release.py', '--stage-dir', str(Path(directory) / 'candidate'),
                                 '--output-dir', str(Path(directory) / 'artifacts'),
                                 '--profile', str(ROOT / 'profiles/cloud-beta.json')]
                    with patch.object(release, 'REPOSITORY', stage), \
                            patch.object(sys, 'argv', arguments), \
                            patch.object(release, 'stage') as staging, \
                            patch.object(release, 'archive') as archive:
                        with self.assertRaisesRegex(ValueError, 'private provisioning config'):
                            release.main()
                        staging.assert_not_called()
                        archive.assert_not_called()
                    self.assertIn('fixture-private-only', private.read_text(encoding='utf8'))
                    self.assertEqual(list(Path(directory).rglob('*.zip')), [])
                    private.unlink()


if __name__ == '__main__':
    unittest.main()
