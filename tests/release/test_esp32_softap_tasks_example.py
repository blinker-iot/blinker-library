"""Fixed SoftAP Tasks4 example contracts; no SDK, service or hardware execution."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/release'))
from test_cloud_wifi_profile import preprocessor

EXAMPLE = ROOT / 'examples/Esp32SoftApTasksQuickStart'
SKETCH = EXAMPLE / 'Esp32SoftApTasksQuickStart.ino'
PLACEHOLDER = 'REPLACE_WITH_YOUR_DEVICE_POP'
CONFIG = 'BlinkerV2/arduino/config/OfficialConfig.h'
FACADE = 'BlinkerV2/api/BlinkerFacade.h'
PRODUCT = 'BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h'
PROFILE = {
    'ARDUINO_ARCH_ESP32': 1, 'ARDUINO_XIAO_ESP32S3': 1,
    'BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS': 1, 'BLINKER_ENABLE_TASKS': 1,
    'BLINKER_TASK_CAPACITY': 4, 'BLINKER_ESP32_PROVISIONING_SOFTAP': 1,
    'BLINKER_OFFICIAL_CLOUD_SESSION_TLS': 1, 'BLINKER_OFFICIAL_MQTT_TLS': 1,
}


class ExampleSource(unittest.TestCase):
    def test_root_loads_library_profile_before_facade_and_product(self):
        source = (ROOT / 'src/BlinkerWiFiBLE.h').read_text(encoding='utf8')
        self.assertLess(source.index('#define BLINKER_PRODUCT_WIFI_BLE 1'), source.index(CONFIG))
        self.assertLess(source.index(CONFIG), source.index(FACADE))
        self.assertLess(source.index(FACADE), source.index(PRODUCT))

    def test_fixed_graph_uses_only_the_original_task_and_sampling_api(self):
        source = SKETCH.read_text(encoding='utf8')
        active = re.sub(r'//[^\n]*', '', source)
        includes = re.findall(r'^#include\s*[<"]([^>"\n]+)', active, re.MULTILINE)
        self.assertEqual(includes, ['UserProvisioning.h', 'BlinkerWiFiBLE.h', 'esp_arduino_version.h'])
        self.assertEqual(re.findall(r'BLINKER_PROPERTY\((\w+),', active), ['power', 'uptimeSeconds'])
        self.assertIn('blinker::realtime(1000U)', active)
        self.assertIn('void onPower(bool value) { currentPower = value; power.report(value); }', active)
        self.assertIn('return millis() / 1000U;', active)
        self.assertIn('power.onWrite(onPower);', active)
        self.assertIn('uptimeSeconds.onSample(sampleUptime);', active)
        self.assertIn('Blinker.enableTasks(power) && Blinker.begin(power, uptimeSeconds)', active)
        self.assertIn('if (begun) Blinker.run();', active)
        self.assertNotRegex(active, r'BLINKER_ACTION|\.start\s*\(|\.reset\w*\s*\(|\.initialize\w*\s*\(')
        self.assertEqual(re.findall(r'^#define\s+(BLINKER_\w+)', active, re.MULTILINE),
                         ['BLINKER_V2_WIFI_PROV_POP'])

    def test_only_one_public_pop_placeholder_and_explicit_install_boundary(self):
        header = (EXAMPLE / 'UserProvisioning.h').read_text(encoding='utf8')
        active = '\n'.join(line for line in header.splitlines()
                           if line.strip() and not line.lstrip().startswith('//'))
        self.assertEqual(active, '#pragma once\n#define ESP32_TASK_PROVISIONING_POP "' + PLACEHOLDER + '"')
        readme = (EXAMPLE / 'README.md').read_text(encoding='utf8')
        for marker in ('3.3.11', 'XIAO ESP32S3', 'Save As', '1-64 UTF-8 byte PoP',
                       'do not edit the SDK', 'default zero slots', 'cloud-beta.json',
                       'DeviceKey, SSID or WiFi password', 'does not open server-side admission',
                       'P256 keyId 1', 'DeviceKey+Admin', 'UNKNOWN'):
            self.assertIn(marker, readme)
        self.assertEqual({file.name for file in EXAMPLE.iterdir()},
                         {'Esp32SoftApTasksQuickStart.ino', 'UserProvisioning.h', 'README.md'})


class ExamplePrefix(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = preprocessor()
        if cls.compiler is None:
            raise unittest.SkipTest('a local C++ compiler is not installed')

    def compile_prefix(self, pop=PLACEHOLDER, *, changes=None, core=(3, 3, 11)):
        # Actual example prefix/static_assert, with explicit host-only header
        # stubs. This cannot prove the SDK graph or a complete firmware link.
        prefix = SKETCH.read_text(encoding='utf8').split('BLINKER_PROPERTY(power,', 1)[0]
        defines = {**PROFILE, **(changes or {})}
        with tempfile.TemporaryDirectory(prefix='softap-tasks-example-') as temporary:
            directory = Path(temporary)
            header = (EXAMPLE / 'UserProvisioning.h').read_text(encoding='utf8')
            (directory / 'UserProvisioning.h').write_text(header.replace(PLACEHOLDER, pop), encoding='utf8')
            (directory / 'BlinkerWiFiBLE.h').write_text('\n'.join(
                f'#define {key} {value}' for key, value in defines.items() if value is not None), encoding='utf8')
            (directory / 'esp_arduino_version.h').write_text('\n'.join(
                f'#define ESP_ARDUINO_VERSION_{name} {value}'
                for name, value in zip(('MAJOR', 'MINOR', 'PATCH'), core)), encoding='utf8')
            source = directory / 'prefix.cpp'
            source.write_text(prefix, encoding='utf8')
            return subprocess.run([self.compiler, '-std=gnu++11', '-x', 'c++', '-fsyntax-only',
                                   '-Wall', '-Wextra', '-Werror', '-I', str(directory), str(source)],
                                  capture_output=True, text=True, timeout=20, check=False)

    def test_configured_nonempty_pop_and_exact_profile_prefix_compile(self):
        result = self.compile_prefix('host-only-nonempty-pop')
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_placeholder_empty_and_overlong_pop_are_rejected(self):
        for pop in (PLACEHOLDER, '', 'x' * 65):
            with self.subTest(pop_length=len(pop)):
                result = self.compile_prefix(pop)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Set your own nonempty 1-64 byte PoP', result.stderr)

    def test_wrong_board_core_profile_bearer_capacity_or_tls_are_rejected(self):
        cases = [({'ARDUINO_XIAO_ESP32S3': None}, (3, 3, 11)),
                 ({}, (3, 3, 10)),
                 ({'BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS': None}, (3, 3, 11)),
                 ({'BLINKER_ENABLE_TASKS': 0}, (3, 3, 11)),
                 ({'BLINKER_TASK_CAPACITY': 0}, (3, 3, 11)),
                 ({'BLINKER_ESP32_PROVISIONING_SOFTAP': None}, (3, 3, 11)),
                 ({'BLINKER_ESP32_PROVISIONING_BLE': 1}, (3, 3, 11)),
                 ({'BLINKER_OFFICIAL_CLOUD_SESSION_TLS': 0}, (3, 3, 11)),
                 ({'BLINKER_OFFICIAL_MQTT_TLS': 0}, (3, 3, 11))]
        for changes, core in cases:
            with self.subTest(changes=changes, core=core):
                self.assertNotEqual(self.compile_prefix('host-only-nonempty-pop',
                                                       changes=changes, core=core).returncode, 0)

    def test_actual_root_exposes_generated_profile_before_api_consumer(self):
        # Preprocess the unchanged actual root against narrowly named include
        # stubs: missing/late OfficialConfig fails the Facade consumer below.
        with tempfile.TemporaryDirectory(prefix='softap-tasks-root-') as temporary:
            directory = Path(temporary)
            (directory / 'BlinkerWiFiBLE.h').write_bytes((ROOT / 'src/BlinkerWiFiBLE.h').read_bytes())
            files = {CONFIG: '#define TEST_LIBRARY_PROFILE_VISIBLE 1\n',
                     FACADE: '#ifndef TEST_LIBRARY_PROFILE_VISIBLE\n#error late library profile\n#endif\n',
                     PRODUCT: '#ifndef TEST_LIBRARY_PROFILE_VISIBLE\n#error missing library profile\n#endif\n'}
            for name, content in files.items():
                target = directory / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(content, encoding='utf8')
            source = directory / 'root.cpp'
            source.write_text('#include <BlinkerWiFiBLE.h>\n', encoding='utf8')
            result = subprocess.run([self.compiler, '-std=gnu++11', '-x', 'c++', '-E',
                                     '-DARDUINO_ARCH_ESP32=1', '-I', str(directory), str(source)],
                                    capture_output=True, text=True, timeout=20, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
