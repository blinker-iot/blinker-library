"""Finite workflow contracts only; no SDK, packaging, network or hardware."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = '.github/workflows/device-v2-softap-tasks.yml'
ARTIFACTS = 'build/softap-tasks/artifacts'


def caller_job(source):
    body = source.split('\n  softap-tasks-candidate:\n', 1)[1]
    return re.split(r'\n  [\w-]+:\n', body, maxsplit=1)[0]


class SoftApTasksCiContract(unittest.TestCase):
    def workflow(self):
        return (ROOT / WORKFLOW).read_text(encoding='utf8')

    def test_reusable_only_and_read_only_without_secret_or_retry_bypass(self):
        source = self.workflow()
        events = source.split('\non:\n', 1)[1].split('\npermissions:', 1)[0]
        self.assertEqual(events.strip(), 'workflow_call:')
        self.assertIn('permissions:\n  contents: read\n', source)
        for forbidden in ('workflow_dispatch:', 'secrets:', '${{ secrets.',
                          'contents: write', 'continue-on-error:', 'arduino-cli upload'):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, source)

    def test_same_commit_caller_keeps_both_original_quality_dependencies(self):
        source = (ROOT / '.github/workflows/device-v2-ci.yml').read_text(encoding='utf8')
        job = caller_job(source)
        self.assertIn('needs: [host-quality, package-boundary]', job)
        self.assertIn("if: ${{ github.event_name == 'push' && github.ref == 'refs/heads/device_v2' }}", job)
        self.assertIn('uses: ./' + WORKFLOW, job)
        self.assertNotIn('always()', job)
        self.assertNotIn('secrets:', job)
        self.assertEqual(source.count('"' + WORKFLOW + '"'), 2)
        self.assertIn('Cross-language exact golden vectors', source)
        self.assertIn('tests/golden/run_cross_language_golden.ps1', source)

    def test_original_sdk_cli_and_checkout_are_pinned(self):
        source = self.workflow()
        self.assertIn('uses: actions/checkout@v6', source)
        self.assertIn('ref: ${{ github.sha }}', source)
        self.assertIn('persist-credentials: false', source)
        self.assertIn('uses: arduino/setup-arduino-cli@v2', source)
        self.assertIn('version: "1.5.1"', source)
        self.assertIn('arduino-cli core install "esp32:esp32@3.3.11"', source)
        self.assertIn('"$RUNNER_TEMP/arduino-softap-tasks-data"', source)
        self.assertIn('"$RUNNER_TEMP/arduino-softap-tasks-downloads"', source)

    def test_uses_existing_builder_and_only_explicit_limited_install_target(self):
        source = self.workflow()
        self.assertEqual(source.count('python3 tools/build_esp32_softap_tasks.py'), 1)
        self.assertIn('--stage-dir build/softap-tasks/library', source)
        self.assertIn('--output-dir ' + ARTIFACTS, source)
        self.assertEqual(source.count('python3 tests/arduino/verify_installed_release.py'), 1)
        self.assertEqual(re.findall(r'--target\s+(\S+)', source), ['esp32-softap-tasks'])
        self.assertIn('--softap-task-manifest ' + ARTIFACTS + '/limited-profile.json', source)
        self.assertIn('--zip-path ' + ARTIFACTS + '/BlinkerDeviceV2-0.1.0.zip', source)
        self.assertIn('--jobs 1', source)
        self.assertNotIn('--reuse-work-dir', source)

    def test_checksum_precedes_complete_install_and_upload(self):
        source = self.workflow()
        self.assertIn('working-directory: ' + ARTIFACTS, source)
        self.assertLess(source.index('sha256sum --check SHA256SUMS'),
                        source.index('python3 tests/arduino/verify_installed_release.py'))
        self.assertLess(source.index('python3 tests/arduino/verify_installed_release.py'),
                        source.index('uses: actions/upload-artifact@v4'))

    def test_only_zip_checksum_and_public_limited_receipt_are_uploaded(self):
        source = self.workflow()
        self.assertIn('name: blinker-device-v2-softap-tasks-${{ github.sha }}', source)
        self.assertIn('if-no-files-found: error', source)
        paths = re.findall(r'path: \|\n((?: {12}\S[^\n]*\n?)+)', source)
        self.assertEqual(len(paths), 1)
        self.assertEqual([line.strip() for line in paths[0].splitlines()], [
            ARTIFACTS + '/BlinkerDeviceV2-0.1.0.zip',
            ARTIFACTS + '/SHA256SUMS',
            ARTIFACTS + '/limited-profile.json',
        ])
        self.assertIn('No deployment, formal admission or hardware verification', source)


if __name__ == '__main__':
    unittest.main()
