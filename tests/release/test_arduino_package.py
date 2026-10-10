"""Package and installed-source gates, without SDK/network/hardware access."""
import json
import hashlib
import re
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY / 'tools'))
sys.path.insert(0, str(REPOSITORY / 'tests/arduino'))
import build_arduino_release as release
import verify_installed_release as gate
import verify_esp32_ble_sdk as ble_sdk


class PackageBoundary(unittest.TestCase):
    def test_ci_covers_every_installed_target_once_with_separate_ble_sdk(self):
        workflow = (REPOSITORY / '.github/workflows/device-v2-ci.yml').read_text(encoding='utf-8')
        job = re.split(r'\n  [a-z][a-z0-9-]*:', workflow.split('  arduino-unified:', 1)[1])[0]
        targets = re.findall(r'--target ([a-z0-9-]+)', job)
        self.assertCountEqual(targets, [target.name for target in gate.TARGETS])
        self.assertEqual(job.count('--zip-path build/formal-release/artifacts/BlinkerDeviceV2-0.1.0.zip'), 2)
        self.assertLess(job.index('esp32:esp32@3.3.6'), job.index('--target esp32-wifi '))
        self.assertLess(job.index('--target wio-terminal-wifi'), job.index('esp32:esp32@3.3.11'))
        self.assertLess(job.index('esp32:esp32@3.3.11'), job.index('verify_esp32_ble_sdk.py'))
        self.assertLess(job.index('verify_esp32_ble_sdk.py'), job.index('--target esp32-ble '))
        self.assertIn('ARDUINO_DIRECTORIES_DATA: ${{ runner.temp }}/arduino-unified-data', job)
        self.assertIn('verify_esp32_ble_sdk.py --data-dir "$ARDUINO_DIRECTORIES_DATA"', job)

    def test_ble_sdk_pin_rejects_missing_or_changed_inputs(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            pins = {name: hashlib.sha256(name.encode()).hexdigest() for name in ble_sdk.PINNED_FILES}
            for name in pins:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(name.encode())
            with patch.object(ble_sdk, 'PINNED_FILES', pins):
                ble_sdk.verify_sdk(root)
                for name in pins:
                    path = root / name
                    path.write_bytes(b'changed')
                    with self.assertRaisesRegex(ValueError, 'input changed'):
                        ble_sdk.verify_sdk(root)
                    path.unlink()
                    with self.assertRaisesRegex(ValueError, 'input missing'):
                        ble_sdk.verify_sdk(root)
                    path.write_bytes(name.encode())

    def test_current_release_and_archive_are_deterministic(self):
        properties, files = release.validate()
        self.assertTrue(any(p.name == 'BlinkerEsp32LocalWebSocket.cpp' for p in files))
        self.assertFalse(any(p.relative_to(REPOSITORY).parts[0] in ('docs', 'tests', 'tools', 'build') for p in files))
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            release.stage(root / 'stage', files)
            a = release.archive(root / 'stage', root / 'a', properties)
            b = release.archive(root / 'stage', root / 'b', properties)
            self.assertEqual(a.read_bytes(), b.read_bytes())
            gate.verify_installed_bytes(a, root / 'stage')

    def test_build_artifacts_cannot_enter_public_source_or_examples(self):
        _, files = release.validate()
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            release.stage(root, files)
            with patch.object(release, 'REPOSITORY', root):
                for relative in ('src/leaked.bin', 'examples/WiFiQuickStart/build/firmware.elf'):
                    artifact = root / relative
                    artifact.parent.mkdir(parents=True, exist_ok=True)
                    artifact.write_bytes(b'not library source')
                    with self.assertRaisesRegex(ValueError, 'non-portable release file'):
                        release.validate()
                    artifact.unlink()

    def test_unreviewed_translation_unit_is_rejected(self):
        _, files = release.validate()
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            release.stage(root, files)
            (root / 'src/extra.cpp').write_text('// unexpected unit\n')
            with patch.object(release, 'REPOSITORY', root):
                with self.assertRaisesRegex(ValueError, 'implementation-unit boundary'):
                    release.validate()


class InstalledBoundary(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.library = self.root / 'library'
        self.library.mkdir()
        self.header = self.library / 'Blinker.h'
        self.header.write_bytes(b'original')
        self.archive = self.root / 'library.zip'
        with zipfile.ZipFile(self.archive, 'w') as bundle:
            bundle.writestr('BlinkerDeviceV2-0.1.0/Blinker.h', b'original')

    def test_identical_bytes_pass(self):
        gate.verify_installed_bytes(self.archive, self.library)

    def test_changed_or_missing_bytes_fail(self):
        self.header.write_bytes(b'changed')
        with self.assertRaisesRegex(SystemExit, 'content changed'):
            gate.verify_installed_bytes(self.archive, self.library)
        self.header.unlink()
        with self.assertRaisesRegex(SystemExit, 'content changed'):
            gate.verify_installed_bytes(self.archive, self.library)

    def test_extra_source_cannot_hide_behind_an_unchanged_zip_marker(self):
        (self.library / 'injected.h').write_bytes(b'extra')
        with self.assertRaisesRegex(SystemExit, 'unexpected files'):
            gate.verify_installed_bytes(self.archive, self.library)

    def test_unsafe_archive_paths_fail(self):
        for name in ('root/../escape', 'root/C:/escape', '/absolute', 'root//file'):
            with self.subTest(name=name):
                with zipfile.ZipFile(self.archive, 'w') as bundle:
                    bundle.writestr(name, b'bad')
                with self.assertRaisesRegex(SystemExit, 'invalid archive member'):
                    gate.verify_installed_bytes(self.archive, self.library)

    def commands(self, file, arguments=()):
        (self.root / 'compile_commands.json').write_text(json.dumps([{'file': str(file), 'arguments': arguments}]))

    def test_repository_cpp_is_not_installed_provenance(self):
        self.commands(REPOSITORY / 'src/BlinkerV2/implementation/BlinkerApi.cpp')
        with self.assertRaisesRegex(SystemExit, 'repository source leaked'):
            gate.verify_provenance(self.root, self.library)

    def test_repository_headers_are_not_installed_provenance(self):
        for arguments in (
            ['-I' + str(REPOSITORY / 'src')],
            ['-I', str(REPOSITORY / 'src')],
            ['-I' + str(REPOSITORY / 'src/BlinkerV2')],
        ):
            self.commands(self.library / 'src/BlinkerApi.cpp', arguments)
            with self.assertRaisesRegex(SystemExit, 'repository headers leaked'):
                gate.verify_provenance(self.root, self.library)

    def test_real_installed_source_is_required(self):
        self.commands(self.root / 'unrelated.cpp')
        with self.assertRaisesRegex(SystemExit, 'not compiled'):
            gate.verify_provenance(self.root, self.library)
        self.commands(self.library / 'src/BlinkerApi.cpp')
        gate.verify_provenance(self.root, self.library)

    def test_unselected_lan_or_gateway_fails_link_closure(self):
        link_map = self.root / 'sketch.map'
        link_map.write_text('BlinkerDeviceV2.a(BlinkerApi.cpp.o)\nBlinkerDeviceV2.a(BlinkerTransport.cpp.o)\n')
        target = next(t for t in gate.TARGETS if t.name == 'esp32-wifi')
        symbols = 'blinker::MqttFrameTransport::poll()'
        gate.verify_link_closure(target, link_map, symbols)
        for extra in ('blinker::Esp32LocalWebSocket::start()', 'blinker::GatewayRouteClient::start()',
                      'blinker::TimeSyncResponder::reply()', 'blinker::GatewayTimeResponder::reply()'):
            with self.assertRaisesRegex(SystemExit, 'retained obsolete symbols'):
                gate.verify_link_closure(target, link_map, symbols + extra)

    def test_default_time_does_not_link_and_opt_in_requires_complete_clock(self):
        markers = ('blinker::TimeSyncService::', 'blinker::TimeSyncCoordinator::', 'blinker::WallClock::')
        gate.verify_time_closure('', enabled=False)
        gate.verify_time_closure(''.join(markers), enabled=True)
        for marker in markers:
            with self.subTest(marker=marker):
                with self.assertRaisesRegex(SystemExit, 'optional business time link mismatch'):
                    gate.verify_time_closure(marker, enabled=False)
                with self.assertRaisesRegex(SystemExit, 'optional business time link mismatch'):
                    gate.verify_time_closure(''.join(m for m in markers if m != marker), enabled=True)

    def test_cloud_smoke_requires_the_time_it_explicitly_enables(self):
        link_map = self.root / 'smoke.map'
        link_map.write_text('BlinkerDeviceV2.a(BlinkerApi.cpp.o)\nBlinkerDeviceV2.a(BlinkerTransport.cpp.o)\n')
        smoke = next(t for t in gate.TARGETS if t.name == 'esp32-cloud-smoke')
        wifi = next(t for t in gate.TARGETS if t.name == 'esp32-wifi')
        base = 'blinker::MqttFrameTransport::poll()'
        clock = 'blinker::TimeSyncService::blinker::TimeSyncCoordinator::blinker::WallClock::'
        gate.verify_link_closure(smoke, link_map, base + clock)
        gate.verify_link_closure(wifi, link_map, base)
        for target, symbols in ((smoke, base), (wifi, base + clock)):
            with self.subTest(target=target.name):
                with self.assertRaisesRegex(SystemExit, 'optional business time link mismatch'):
                    gate.verify_link_closure(target, link_map, symbols)

    def test_matrix_time_selection_matches_public_examples(self):
        for target in gate.TARGETS:
            with self.subTest(target=target.name):
                source = (REPOSITORY / 'examples' / target.example / (target.example + '.ino')).read_text(encoding='utf-8')
                self.assertEqual(target.time_enabled, 'Blinker.enableTime()' in source)

    def test_task_pilot_is_explicit_and_uses_public_consumer(self):
        self.assertNotIn(gate.TASK_PILOT_TARGET, gate.select_targets(None))
        self.assertEqual(gate.select_targets(['esp32-wifi-tasks']), (gate.TASK_PILOT_TARGET,))
        for names in (['unknown'], ['esp32-wifi-tasks', 'esp32-wifi-tasks']):
            with self.assertRaises(ValueError):
                gate.select_targets(names)
        target = gate.TASK_PILOT_TARGET
        source = (REPOSITORY / 'tests/arduino' / target.example / (target.example + '.ino')).read_text()
        self.assertIn('#include <Blinker.h>', source)
        self.assertIn('Blinker.enableTasks(power)', source)
        self.assertNotIn('official_detail', source)

    def test_task_link_closure_rejects_missing_graph_and_default_leak(self):
        link_map = self.root / 'task.map'
        link_map.write_text('BlinkerDeviceV2.a(BlinkerApi.cpp.o)\nBlinkerDeviceV2.a(BlinkerTransport.cpp.o)\n')
        base = 'blinker::MqttFrameTransport::'
        clock = 'blinker::TimeSyncService::blinker::TimeSyncCoordinator::blinker::WallClock::'
        markers = ('blinker::TaskEngine::', 'blinker::TaskManagementSession::', 'blinker::TaskPropertyTarget::')
        gate.verify_link_closure(gate.TASK_PILOT_TARGET, link_map, base + clock + ''.join(markers))
        wifi = next(t for t in gate.TARGETS if t.name == 'esp32-wifi')
        for marker in markers:
            with self.assertRaisesRegex(SystemExit, 'optional task link mismatch'):
                gate.verify_link_closure(gate.TASK_PILOT_TARGET, link_map, base + clock + ''.join(m for m in markers if m != marker))
            with self.assertRaisesRegex(SystemExit, 'optional task link mismatch'):
                gate.verify_link_closure(wifi, link_map, base + marker)

    def test_tls_sdk_dependency_is_conditional_not_a_general_allowlist(self):
        wifi = next(t for t in gate.TARGETS if t.name == 'esp32-wifi')
        ble = next(t for t in gate.TARGETS if t.name == 'esp32-ble')
        r4 = next(t for t in gate.TARGETS if t.name == 'uno-r4-wifi')
        extra = frozenset(('NetworkClientSecure',))
        for config in ('#define BLINKER_OFFICIAL_MQTT_TLS 1\n', '#define BLINKER_OFFICIAL_CLOUD_SESSION_TLS 1\n'):
            gate.verify_library_closure(wifi, wifi.libraries, config)
            gate.verify_library_closure(wifi, wifi.libraries | extra, config)
            for target in (ble, r4):
                with self.assertRaisesRegex(SystemExit, 'compiled libraries'):
                    gate.verify_library_closure(target, target.libraries | extra, config)
            with self.assertRaisesRegex(SystemExit, 'compiled libraries'):
                gate.verify_library_closure(wifi, wifi.libraries | extra | frozenset(('ArduinoBLE',)), config)
            with self.assertRaisesRegex(SystemExit, 'compiled libraries'):
                gate.verify_library_closure(wifi, extra, config)
        with self.assertRaisesRegex(SystemExit, 'compiled libraries'):
            gate.verify_library_closure(wifi, wifi.libraries | extra, '#define BLINKER_OFFICIAL_MQTT_TLS 0\n')


if __name__ == '__main__':
    unittest.main()
