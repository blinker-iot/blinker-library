"""Internal four-slot TLS graph only; no credentials, SDK build or hardware."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tests/release'))
import arduino_release_profile as profile
from test_cloud_wifi_profile import preprocessor

# Valid public 2G fixture, never used by an operational cloud candidate.
PUBLIC_FIXTURE = ('047cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978'
                  '07775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1')


def payload():
    value = json.loads((ROOT / 'profiles/cloud-beta.json').read_text(encoding='utf8'))
    del value['product_profile']
    value.update(channel='development', task_profile=profile.TASK_CLOUD_PILOT_ID)
    value['server_keys']['p256'] = [{'id': 1, 'public_key': PUBLIC_FIXTURE}]
    return value


class CloudTaskProfile(unittest.TestCase):
    def test_exact_p256_only_policy_and_typed_null_portable_ring(self):
        parsed = profile.parse_profile(payload())
        self.assertTrue(parsed.cloud_task_pilot)
        self.assertFalse(parsed.cloud_wifi_only)
        self.assertEqual(parsed.ed25519_keys, ())
        config = profile.render_config(parsed)
        for key, value in profile.TASK_PILOT.items():
            self.assertIn(f'#define BLINKER_TASK_{key} {value}U', config)
        self.assertIn('P256ServerPublicKey(1U', profile.render_key_header(parsed, p256=True))
        portable = profile.render_key_header(parsed, p256=False)
        self.assertIn('Ed25519ServerPublicKey* const serverSigningKeys = nullptr;', portable)
        self.assertNotIn('serverSigningKey0Bytes', portable)
        self.assertIn('BLINKER_OFFICIAL_HAS_SERVER_SIGNATURE 0', portable)

    def test_no_fake_ed_key_production_relabel_or_weaker_transport(self):
        mutations = [lambda p: p.update(channel='production'),
                     lambda p: p['control'].update(cloud_session_tls=False),
                     lambda p: p['control'].update(mqtt_tls=False),
                     lambda p: p['control'].update(port=80),
                     lambda p: p['control'].update(host='replace-with-control-host.example'),
                     lambda p: p['server_keys']['p256'][0].update(id=2),
                     lambda p: p['server_keys'].update(p256=[]),
                     lambda p: p['server_keys'].update(p256=[None]),
                     lambda p: p['server_keys']['p256'][0].update(public_key=profile.TEST_P256_KEY),
                     lambda p: p['server_keys'].update(ed25519=[{'id': 1, 'public_key': profile.TEST_ED25519_KEY}])]
        for mutate in mutations:
            value = payload()
            mutate(value)
            with self.subTest(value=value), self.assertRaises(ValueError):
                profile.parse_profile(value)

    def test_public_zero_slot_profile_remains_closed(self):
        value = json.loads((ROOT / 'profiles/cloud-beta.json').read_text(encoding='utf8'))
        parsed = profile.parse_profile(value)
        self.assertTrue(parsed.cloud_wifi_only)
        self.assertNotIn('#define BLINKER_TASK_CAPACITY 4U', profile.render_config(parsed))
        value['task_profile'] = profile.TASK_CLOUD_PILOT_ID
        with self.assertRaisesRegex(ValueError, 'without task_profile'):
            profile.parse_profile(value)

    def test_graph_and_policy_guard(self):
        compiler = preprocessor()
        if compiler is None:
            self.skipTest('C++ preprocessor is not installed')
        with tempfile.TemporaryDirectory(prefix='cloud-task-headers-') as directory:
            work = Path(directory)
            parsed = profile.parse_profile(payload())
            (work / 'OfficialConfig.h').write_text(profile.render_config(parsed), encoding='utf8')
            cases = [('manual', '#define BLINKER_PRODUCT_WIFI 1\n#define BLINKER_ENABLE_TASKS 1\n', True),
                     ('BLE', '#define BLINKER_PRODUCT_BLE 1\n', False),
                     ('gateway', '#define BLINKER_V2_ARDUINO_ESP32_EDGEHUB_PRODUCT_H 1\n', False),
                     ('LAN', '#define BLINKER_INTERNAL_LAN_WEBSOCKET 1\n', False),
                     ('slots', '#define BLINKER_TASK_CAPACITY 5\n', False),
                     ('policy', '#define BLINKER_TASK_MAX_COMMIT_MS 1000\n', False),
                     ('disabled', '#define BLINKER_ENABLE_TASKS 0\n', False)]
            for name, definitions, expected in cases:
                source = work / 'input.cpp'
                source.write_text(definitions + '#include "OfficialConfig.h"\n', encoding='utf8')
                result = subprocess.run([compiler, '-std=gnu++11', '-x', 'c++', '-E',
                                         '-DARDUINO_ARCH_ESP32=1', '-I', str(work), str(source)],
                                        capture_output=True, timeout=20)
                with self.subTest(name=name):
                    self.assertEqual(result.returncode == 0, expected, result.stderr.decode(errors='replace'))
            source.write_text('#include "OfficialConfig.h"\n', encoding='utf8')
            result = subprocess.run([compiler, '-std=gnu++11', '-x', 'c++', '-E', '-I', str(work), str(source)],
                                    capture_output=True, timeout=20)
            self.assertNotEqual(result.returncode, 0)


if __name__ == '__main__':
    unittest.main()
