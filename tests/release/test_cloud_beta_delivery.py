"""Restricted candidate delivery contract; no SDK, network or firmware build."""
import hashlib
from pathlib import Path
import re
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import build_arduino_release as release

WORKFLOW = ROOT / '.github/workflows/device-v2-cloud-beta.yml'


class CloudBetaDelivery(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workflow = WORKFLOW.read_text(encoding='utf-8')

    def test_only_manual_or_reusable_candidate_build_with_read_only_permissions(self):
        trigger = self.workflow.split('\non:\n', 1)[1].split('\npermissions:', 1)[0]
        self.assertEqual(re.findall(r'^  ([a-z_]+):', trigger, re.MULTILINE),
                         ['workflow_dispatch', 'workflow_call'])
        permissions = self.workflow.split('\npermissions:\n', 1)[1].split('\njobs:', 1)[0]
        self.assertEqual(permissions.strip(), 'contents: read')
        self.assertNotRegex(self.workflow, r'secrets[.\[]|gh release|git push|\b(?:ssh|scp)\b')
        self.assertNotIn('always()', self.workflow)

    def test_original_cloud_toolchain_and_only_two_ordinary_installed_targets(self):
        self.assertIn('arduino/setup-arduino-cli@v2', self.workflow)
        self.assertIn('version: "1.5.1"', self.workflow)
        self.assertEqual(self.workflow.count('arduino-cli core install'), 1)
        self.assertIn('arduino-cli core install "esp32:esp32@3.3.11"', self.workflow)
        ordinary = self.workflow.split('      - name: Install and verify only the admitted cloud targets', 1)[1]
        ordinary = ordinary.split('      - name: Verify optional location input', 1)[0]
        self.assertEqual(re.findall(r'--target ([a-z0-9-]+)', ordinary),
                         ['esp32-wifi', 'esp32-cloud-smoke'])
        self.assertIn('tests/arduino/verify_installed_release.py', self.workflow)
        self.assertIn('--data-dir "$ARDUINO_DIRECTORIES_DATA"', self.workflow)
        self.assertIn('--work-dir "${RUNNER_TEMP}/cloud-beta-installed"', self.workflow)
        self.assertIn('--jobs 2', self.workflow)

    def test_isolated_directories_use_step_runtime_before_sdk_install_not_job_runner_context(self):
        quality = (ROOT / '.github/workflows/device-v2-ci.yml').read_text(encoding='utf-8')
        for name, workflow, directories in (
            ('cloud-beta', self.workflow, {
                'ARDUINO_DIRECTORIES_DATA': 'arduino-cloud-beta-data',
                'ARDUINO_DIRECTORIES_DOWNLOADS': 'arduino-cloud-beta-downloads',
            }),
            ('quality-unified', quality.split('\n  arduino-unified:\n', 1)[1], {
                'ARDUINO_DIRECTORIES_DATA': 'arduino-unified-data',
            }),
        ):
            with self.subTest(workflow=name):
                initialization = workflow.split('      - name: Initialize isolated Arduino directories\n', 1)[1]
                initialization = re.split(r'^      - ', initialization, maxsplit=1, flags=re.MULTILINE)[0]
                self.assertIn('        shell: bash', initialization)
                commands = [line.strip() for line in initialization.split('        run: |\n', 1)[1].splitlines()
                            if line.strip()]
                self.assertEqual(commands, [
                    f"printf '{variable}=%s\\n' \"$RUNNER_TEMP/{directory}\" >> \"$GITHUB_ENV\""
                    for variable, directory in directories.items()
                ])
                self.assertLess(workflow.index('Initialize isolated Arduino directories'),
                                workflow.index('arduino-cli core install'))
        for name, workflow in (('cloud-beta', self.workflow), ('quality', quality)):
            for job_environment in re.findall(r'^    env:\n((?:^ {6,}.*\n)*)', workflow, re.MULTILINE):
                with self.subTest(workflow=name, job_environment=job_environment):
                    self.assertNotRegex(job_environment, r'\$\{\{\s*runner(?:\.|\[)')

    def test_location_input_is_opt_in_and_reuses_the_same_zip_sdk_and_dependency_cache(self):
        trigger = self.workflow.split('\non:\n', 1)[1].split('\npermissions:', 1)[0]
        for event in ('workflow_dispatch', 'workflow_call'):
            inputs = trigger.split(f'  {event}:', 1)[1]
            inputs = re.split(r'^  [a-z_]+:', inputs, maxsplit=1, flags=re.MULTILINE)[0]
            with self.subTest(event=event):
                self.assertEqual(re.findall(r'^      ([a-z_]+):', inputs, re.MULTILINE),
                                 ['verify_location_input'])
                self.assertIn('        type: boolean', inputs)
                self.assertIn('        required: false', inputs)
                self.assertIn('        default: false', inputs)
        optional = self.workflow.split('      - name: Verify optional location input from the same ZIP', 1)[1]
        optional = optional.split('      - name: Retain the verified candidate', 1)[0]
        self.assertIn('if: ${{ inputs.verify_location_input }}', optional)
        self.assertIn('--zip-path build/cloud-beta/artifacts/BlinkerDeviceV2-0.1.0.zip', optional)
        self.assertIn('--work-dir "${RUNNER_TEMP}/cloud-beta-location-input-installed"', optional)
        self.assertIn('--data-dir "$ARDUINO_DIRECTORIES_DATA"', optional)
        self.assertIn('--dependency-dir "${RUNNER_TEMP}/cloud-beta-installed/sketchbook/libraries"', optional)
        self.assertEqual(re.findall(r'--target ([a-z0-9-]+)', optional), ['esp32-cloud-smoke'])
        self.assertIn('--location-input', optional)
        self.assertIn('--jobs 1', optional)
        self.assertNotIn('--reuse-work-dir', optional)
        self.assertNotIn('tools/build_arduino_release.py', optional)
        self.assertNotIn('core install', optional)
        self.assertNotIn('lib install', optional)

    def test_one_public_profile_build_and_checksum_before_installed_gate(self):
        self.assertEqual(self.workflow.count('tools/build_arduino_release.py'), 1)
        self.assertIn('--profile profiles/cloud-beta.json', self.workflow)
        self.assertNotIn('development_profile', self.workflow)
        self.assertNotIn('--allow-development-profile', self.workflow)
        self.assertIn('working-directory: build/cloud-beta/artifacts', self.workflow)
        checksum = self.workflow.index('sha256sum --check SHA256SUMS')
        self.assertLess(self.workflow.index('tools/build_arduino_release.py'), checksum)
        self.assertLess(checksum, self.workflow.index('tests/arduino/verify_installed_release.py'))

    def test_success_artifact_contains_only_matching_version_zip_and_checksum(self):
        properties = release.read_properties()
        archive = f"{properties['name']}-{properties['version']}.zip"
        self.assertIn(f'--zip-path build/cloud-beta/artifacts/{archive}', self.workflow)
        upload = self.workflow.split('uses: actions/upload-artifact@v4', 1)[1]
        self.assertIn('name: blinker-device-v2-cloud-beta-${{ github.sha }}', upload)
        paths = [line.strip() for line in upload.split('path: |', 1)[1].splitlines() if line.strip()]
        self.assertEqual(paths, [f'build/cloud-beta/artifacts/{archive}',
                                 'build/cloud-beta/artifacts/SHA256SUMS'])
        self.assertIn('if-no-files-found: error', upload)
        self.assertLess(self.workflow.index('tests/arduino/verify_installed_release.py'),
                        self.workflow.index('uses: actions/upload-artifact@v4'))
        self.assertLess(self.workflow.index('Verify optional location input from the same ZIP'),
                        self.workflow.index('uses: actions/upload-artifact@v4'))

    def test_public_profile_and_workflow_changes_reach_existing_quality_gates(self):
        quality = (ROOT / '.github/workflows/device-v2-ci.yml').read_text(encoding='utf-8')
        triggers = quality.split('\npermissions:', 1)[0]
        for event, end in (('push', 'pull_request'), ('pull_request', 'workflow_dispatch')):
            section = triggers.split(f'  {event}:', 1)[1].split(f'  {end}:', 1)[0]
            with self.subTest(event=event):
                self.assertIn('"profiles/**"', section)
                self.assertIn('".github/workflows/device-v2-cloud-beta.yml"', section)

    def test_quality_workflow_calls_same_commit_only_for_device_v2_push_after_quality_gates(self):
        quality = (ROOT / '.github/workflows/device-v2-ci.yml').read_text(encoding='utf-8')
        permissions = quality.split('\npermissions:\n', 1)[1].split('\njobs:', 1)[0]
        self.assertEqual(permissions.strip(), 'contents: read')
        caller = quality.split('\n  cloud-beta-candidate:\n', 1)[1]
        caller = re.split(r'^  [a-z0-9-]+:', caller, maxsplit=1, flags=re.MULTILINE)[0]
        self.assertEqual([line.strip() for line in caller.splitlines() if line.strip()], [
            'needs: [host-quality, package-boundary]',
            "if: ${{ github.event_name == 'push' && github.ref == 'refs/heads/device_v2' }}",
            'uses: ./.github/workflows/device-v2-cloud-beta.yml',
            'with:',
            'verify_location_input: true',
        ])
        self.assertEqual(quality.count('uses: ./.github/workflows/device-v2-cloud-beta.yml'), 1)
        self.assertNotIn('secrets:', caller)
        self.assertNotIn('always()', caller)

    def test_packaged_readme_is_usable_without_repo_docs_or_credentials(self):
        readme = (ROOT / 'README.md').read_text(encoding='utf-8')
        self.assertLess(readme.index('For the current cloud beta ZIP'), readme.index('## WiFi QuickStart'))
        install = readme.split('#### Install the cloud beta candidate', 1)[1].split('#### Maintainer build', 1)[0]
        for required in ('0.1.0', 'device-v2-0.1.0', 'SHA256SUMS', 'Get-FileHash',
                         '3.3.11', 'PubSubClient 2.8', 'esp32:esp32:esp32',
                         'WiFiQuickStart', 'CloudSmokeTest', 'Multiple libraries were found',
                         'ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL', 'flat multi-platform set'):
            with self.subTest(required=required):
                self.assertIn(required, install)
        self.assertIn('outside the Sketchbook `libraries` directory', install)
        self.assertIn('A checksum detects byte changes; it is not', install)
        self.assertNotRegex(install, r'\b[0-9a-f]{64}\b')  # no self-referential ZIP checksum

    def test_packaged_location_instructions_do_not_require_a_separate_zip_or_gps_driver(self):
        readme = (ROOT / 'README.md').read_text(encoding='utf-8')
        location = readme.split('### Optional device location input', 1)[1]
        for required in ('#include <Blinker.h>', 'BLINKER_LOCATION(position)',
                         'Blinker.location(position, latitude, longitude, sampledAtMillis)',
                         'power, position', 'Latitude comes first', 'boot-local',
                         'do not repeatedly restamp', '30 s', 'local snapshot only',
                         'Blinker.locationLost(position)', 'Blinker.clearLocation(position)',
                         'Manifest/owner admission', 'verify_location_input',
                         'not an additional ZIP', 'physical GPS test'):
            with self.subTest(required=required):
                self.assertIn(required, location)
        self.assertNotRegex(location, r'\b[0-9a-f]{64}\b')

    def test_existing_archive_writer_emits_the_digest_of_each_exact_zip(self):
        with tempfile.TemporaryDirectory(prefix='cloud-delivery-checksum-') as directory:
            work = Path(directory)
            stage = work / 'stage'
            stage.mkdir()
            readme = stage / 'README.md'
            properties = {'name': 'BlinkerDeviceV2', 'version': '0.1.0'}
            digests = []
            for index, text in enumerate(('first instructions', 'revised instructions')):
                readme.write_text(text, encoding='utf-8')
                artifact = release.archive(stage, work / str(index), properties)
                digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
                self.assertEqual((artifact.parent / 'SHA256SUMS').read_text(),
                                 f'{digest}  {artifact.name}\n')
                digests.append(digest)
            self.assertNotEqual(*digests)


if __name__ == '__main__':
    unittest.main()
