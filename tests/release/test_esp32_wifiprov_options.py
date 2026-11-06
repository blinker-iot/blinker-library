"""Optional PoP source/header contracts; no SDK build, network or hardware."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/release'))
from test_cloud_wifi_profile import preprocessor


HEADER = 'BlinkerV2/arduino/config/Esp32WiFiProvOptions.h'
PRODUCTS = ('Esp32WiFiBLEProduct.h', 'Esp32EdgeHubProduct.h')


class WiFiProvOptionsSource(unittest.TestCase):
    def test_both_products_share_the_same_pop_for_both_bearers(self):
        for name in PRODUCTS:
            source = (ROOT / 'src/BlinkerV2/arduino/platform/esp32' / name).read_text(encoding='utf8')
            with self.subTest(product=name):
                self.assertIn('#include <' + HEADER + '>', source)
                self.assertRegex(source, r'esp32SoftApWifiProvConfig\(\s*serviceName,\s*'
                                 r'official::wifiProvProofOfPossession\(\),\s*nullptr\)')
                self.assertRegex(source, r'esp32BleWifiProvConfig\(\s*serviceName,\s*'
                                 r'official::wifiProvProofOfPossession\(\)\)')
                self.assertEqual(source.count('official::wifiProvProofOfPossession()'), 2)
                self.assertNotIn('BLINKER_V2_WIFI_PROV_POP', source)

    def test_adapter_keeps_explicit_empty_and_length_validation_and_security1(self):
        source = (ROOT / 'src/BlinkerV2/arduino/ports/esp32/Esp32WiFiProv.ipp').read_text(encoding='utf8')
        self.assertRegex(source, r'config\.proofOfPossession != nullptr &&\s*'
                         r'!validText\(config\.proofOfPossession, 1U, 64U\)')
        self.assertRegex(source, r'network_prov_mgr_start_provisioning\(\s*'
                         r'NETWORK_PROV_SECURITY_1,\s*config\.proofOfPossession,')
        self.assertEqual(source.count('config.proofOfPossession = proofOfPossession;'), 2)

    def test_shared_option_is_dependency_free_and_default_null(self):
        source = (ROOT / 'src' / HEADER).read_text(encoding='utf8')
        self.assertRegex(source, r'#ifndef BLINKER_V2_WIFI_PROV_POP\s*'
                         r'#define BLINKER_V2_WIFI_PROV_POP nullptr\s*#endif')
        self.assertEqual(re.findall(r'^\s*#\s*include\b', source, re.MULTILINE), [])
        self.assertIn('return BLINKER_V2_WIFI_PROV_POP;', source)
        self.assertNotRegex(source, r'\b(?:strlen|strcmp|Serial|printf)\s*\(')


class WiFiProvOptionsHeader(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = preprocessor()
        if cls.compiler is None:
            raise unittest.SkipTest('a local C++ compiler is not installed')

    def check(self, definitions, assertions):
        # Compile only the actual dependency-free header, never SDK/product code.
        with tempfile.TemporaryDirectory(prefix='wifiprov-options-') as directory:
            source = Path(directory) / 'input.cpp'
            source.write_text(definitions + '#include <' + HEADER + '>\n'
                              + assertions, encoding='utf8')
            result = subprocess.run([self.compiler, '-std=gnu++11', '-x', 'c++', '-fsyntax-only',
                                     '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'src'), str(source)],
                                    capture_output=True, text=True, timeout=20, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_ordinary_default_is_null(self):
        self.check('', 'static_assert(blinker::official::wifiProvProofOfPossession() == nullptr, "default");\n')

    def test_configured_text_is_preserved_exactly(self):
        self.check('#define BLINKER_V2_WIFI_PROV_POP "fixture-only-pop"\n',
                   'static_assert(blinker::official::wifiProvProofOfPossession()[0] == \'f\', "first");\n'
                   'static_assert(blinker::official::wifiProvProofOfPossession()[16] == \'\\0\', "length");\n')

    def test_explicit_empty_is_not_changed_to_null(self):
        self.check('#define BLINKER_V2_WIFI_PROV_POP ""\n',
                   'static_assert(blinker::official::wifiProvProofOfPossession() != nullptr, "not null");\n'
                   'static_assert(blinker::official::wifiProvProofOfPossession()[0] == \'\\0\', "empty");\n')


if __name__ == '__main__':
    unittest.main()
