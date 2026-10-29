"""R3 release boundaries only; no SDK build, archive, service or hardware access."""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tests/arduino'))
import arduino_release_profile as profile
import verify_installed_release as installed


class ReleaseSupportFreeze(unittest.TestCase):
    def test_seven_ordinary_targets_do_not_admit_preview_or_tasks(self):
        expected = {
            'esp32-wifi': ('esp32:esp32:esp32', 'WiFiQuickStart'),
            'esp32-ble': ('esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB', 'BleQuickStart'),
            'esp32-wifi-ble': ('esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB', 'WiFiBleQuickStart'),
            'esp32-cloud-smoke': ('esp32:esp32:esp32', 'CloudSmokeTest'),
            'uno-r4-wifi': ('arduino:renesas_uno:unor4wifi', 'WiFiQuickStart'),
            'uno-r4-ble': ('arduino:renesas_uno:unor4wifi', 'BleQuickStart'),
            'wio-terminal-wifi': ('Seeeduino:samd:seeed_wio_terminal', 'WiFiQuickStart'),
        }
        self.assertEqual({target.name: (target.fqbn, target.example)
                          for target in installed.TARGETS}, expected)
        self.assertTrue(all(not target.tasks_enabled for target in installed.TARGETS))

    def test_only_public_profile_keeps_matching_version_and_restricted_targets(self):
        self.assertEqual(sorted(path.name for path in (ROOT / 'profiles').glob('*.json')),
                         ['cloud-beta.json'])
        current = profile.load_profile(ROOT / 'profiles/cloud-beta.json')
        properties = dict(line.split('=', 1) for line in
                          (ROOT / 'library.properties').read_text().splitlines() if '=' in line)
        version = json.loads((ROOT / 'library.json').read_text())['version']
        self.assertEqual(properties['version'], version)
        self.assertEqual(current.firmware_version, f'device-v2-{version}')
        allowed = installed.select_targets(['esp32-wifi', 'esp32-cloud-smoke'])
        self.assertEqual(installed.verify_installed_product_profile(
            profile.render_config(current), allowed), profile.CLOUD_WIFI_PROFILE_ID)
        self.assertEqual(current.p256_keys, ())
        self.assertEqual(current.ed25519_keys, ())
        self.assertIsNone(current.task_profile)

    def test_key_rotation_preserves_the_first_signable_key_not_numeric_order(self):
        # Public fixture keys are development-only, never cloud trust material.
        fixture = json.loads((ROOT / 'tests/release/development_profile.json').read_text())
        for name, p256, key_type in (('p256', True, 'P256'), ('ed25519', False, 'Ed25519')):
            payload = copy.deepcopy(fixture)
            key = payload['server_keys'][name][0]['public_key']
            payload['server_keys'][name] = [{'id': 9, 'public_key': key},
                                            {'id': 2, 'public_key': key}]
            parsed = profile.parse_profile(payload)
            ring = parsed.p256_keys if p256 else parsed.ed25519_keys
            with self.subTest(algorithm=name):
                self.assertEqual([entry.key_id for entry in ring], [9, 2])
                rendered = profile.render_key_header(parsed, p256=p256)
                self.assertLess(rendered.index(f'{key_type}ServerPublicKey(9U'),
                                rendered.index(f'{key_type}ServerPublicKey(2U'))
                self.assertIn(f'{key_type}ServerPublicKey(9U, ByteView(serverSigningKey0Bytes', rendered)


if __name__ == '__main__':
    unittest.main()
