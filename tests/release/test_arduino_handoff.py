"""Deterministic maintainer source closure; no SDK, network, Git or hardware."""
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY / 'tools'))
import build_arduino_handoff as handoff
import build_arduino_release as release
from arduino_release_profile import apply_profile, load_profile, TASK_PILOT_ID


class SourceHandoff(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        cls.root = Path(cls.temp.name)
        profile = json.loads((REPOSITORY / 'tests/release/development_profile.json').read_text())
        profile['task_profile'] = TASK_PILOT_ID
        cls.profile = cls.root / 'profile.json'
        cls.profile.write_text(json.dumps(profile))
        properties, files = release.validate()
        stage = cls.root / 'stage'
        release.stage(stage, files)
        apply_profile(stage, load_profile(cls.profile))
        cls.reference = release.archive(stage, cls.root / 'reference', properties)
        cls.dependency = cls.root / 'PubSubClient'
        (cls.dependency / 'src').mkdir(parents=True)
        for name in handoff.DEPENDENCY_FILES:
            (cls.dependency / name).write_text('fixture only')
        (cls.dependency / 'library.properties').write_text('name=PubSubClient\nversion=2.8\n')
        (cls.dependency / 'src/PubSubClient.h').write_text('// fixture\n')
        cls.bundle = cls.root / 'bundle'
        handoff.export(cls.profile, cls.reference, cls.dependency, cls.bundle)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def copy(self):
        return Path(shutil.copytree(self.bundle, self.root / 'copy'))

    def test_exact_source_profile_rebuild_without_original_working_tree(self):
        bundle = self.copy()
        artifact = handoff.rebuild(bundle, self.root / 'rebuild')
        self.assertEqual(artifact.read_bytes(), self.reference.read_bytes())
        self.assertFalse((bundle / 'source/tools/__pycache__').exists())

    def test_bundle_is_an_allowlist_not_a_workspace_copy(self):
        manifest = handoff.verify(self.bundle)
        names = manifest['files']
        self.assertIn('source/tools/build_arduino_release.py', names)
        self.assertIn('source/tests/arduino/CompileTaskWiFi/CompileTaskWiFi.ino', names)
        self.assertIn('dependencies/PubSubClient/LICENSE.txt', names)
        self.assertFalse(any('/build/' in n or '/.git/' in n or '/tests/hardware/' in n for n in names))
        self.assertNotIn('source/.github/workflows/device-v2-ci.yml', names)
        self.assertEqual(json.loads((self.bundle / 'requirements.json').read_text())['libraries'], {'PubSubClient': '2.8'})

    def test_missing_changed_or_extra_input_rejected(self):
        bundle = self.copy()
        file = bundle / 'source/src/Blinker.h'
        original = file.read_bytes()
        for payload in (None, b'changed'):
            if payload is None:
                file.unlink()
            else:
                file.write_bytes(payload)
            with self.assertRaisesRegex(ValueError, 'bytes missing, modified or unexpected'):
                handoff.verify(bundle)
            file.write_bytes(original)
        (bundle / 'extra.txt').write_text('unexpected')
        with self.assertRaises(ValueError):
            handoff.verify(bundle)

    def test_manifest_cannot_escape_root(self):
        bundle = self.copy()
        path = bundle / 'manifest.json'
        manifest = json.loads(path.read_text())
        for unsafe in ('../outside', '/absolute', 'C:/outside', 'a\\b', 'a//b', './a'):
            with self.subTest(unsafe=unsafe):
                bad = {**manifest, 'files': {unsafe: '0' * 64}}
                path.write_text(json.dumps(bad))
                with self.assertRaisesRegex(ValueError, 'unsafe manifest path'):
                    handoff.verify(bundle)

    def test_profile_changes_and_reference_changes_rejected(self):
        profile = json.loads(self.profile.read_text())
        profile['control']['port'] += 1
        changed = self.root / 'changed.json'
        changed.write_text(json.dumps(profile))
        with self.assertRaisesRegex(ValueError, 'differs from reference ZIP'):
            handoff.export(changed, self.reference, self.dependency, self.root / 'bundle')
        self.assertFalse((self.root / 'bundle').exists())

    def test_task_reference_cannot_be_relabelled_plain_or_contain_secret(self):
        profile = json.loads(self.profile.read_text())
        changed = self.root / 'changed.json'
        for payload in ({k: v for k, v in profile.items() if k != 'task_profile'}, {**profile, 'deviceKey': 'secret'}):
            changed.write_text(json.dumps(payload))
            with self.assertRaises(ValueError):
                handoff.export(changed, self.reference, self.dependency, self.root / 'bundle')

    def test_plain_wifi_reference_rebuilds_without_implicitly_admitting_tasks(self):
        profile = json.loads(self.profile.read_text())
        del profile['task_profile']
        plain = self.root / 'plain.json'
        plain.write_text(json.dumps(profile))
        properties, files = release.validate()
        stage = self.root / 'plain-stage'
        release.stage(stage, files)
        apply_profile(stage, load_profile(plain))
        reference = release.archive(stage, self.root / 'plain-reference', properties)
        bundle = self.root / 'plain-bundle'
        handoff.export(plain, reference, self.dependency, bundle)
        self.assertIsNone(load_profile(bundle / 'profile.json').task_profile)
        artifact = handoff.rebuild(bundle, self.root / 'plain-rebuilt')
        self.assertEqual(artifact.read_bytes(), reference.read_bytes())

    def test_old_task_manifest_still_verified_but_requires_task_profile(self):
        bundle = self.copy()
        file = bundle / 'manifest.json'
        manifest = json.loads(file.read_text())
        manifest['kind'] = 'blinker-task-pilot-source'
        file.write_text(json.dumps(manifest))
        handoff.verify(bundle)
        # A signed/known reference digest is still checked separately at rebuild.
        profile_file = bundle / 'profile.json'
        profile = json.loads(profile_file.read_text())
        del profile['task_profile']
        profile_file.write_text(json.dumps(profile))
        manifest['files']['profile.json'] = handoff.digest(profile_file)
        file.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'old task handoff'):
            handoff.verify(bundle)

    def test_rebuild_does_not_overwrite_or_modify_its_bundle(self):
        bundle = self.copy()
        for work in (bundle, bundle / 'new-build', self.root):
            with self.assertRaisesRegex(ValueError, 'work must be a new directory'):
                handoff.rebuild(bundle, work)
        handoff.verify(bundle)

    def test_changed_expected_digest_does_not_falsely_pass_rebuild(self):
        bundle = self.copy()
        path = bundle / 'manifest.json'
        manifest = json.loads(path.read_text())
        manifest['expected_zip']['sha256'] = '0' * 64
        path.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'differs from tested reference'):
            handoff.rebuild(bundle, self.root / 'rebuild')

    def test_dependency_must_have_matching_version(self):
        dependency = Path(shutil.copytree(self.dependency, self.root / 'dependency'))
        (dependency / 'library.properties').write_text('name=PubSubClient\nversion=99\n')
        with self.assertRaisesRegex(ValueError, 'PubSubClient 2.8'):
            handoff.export(self.profile, self.reference, dependency, self.root / 'bundle')


if __name__ == '__main__':
    unittest.main()
