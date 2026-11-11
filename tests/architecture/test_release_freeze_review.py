"""Check the reviewed nested freeze delta, without rebuilding or replaying tests."""
import hashlib
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
NESTED = 'tests/freeze/device_key_cloud_login.sha256'
CHANGED = {
    'src/BlinkerV2/api/BlinkerFacade.h',
    'src/BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h',
}


def digest_table(table):
    return hashlib.sha256(json.dumps(
        table, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


class ReviewedFreezeDelta(unittest.TestCase):
    def setUp(self):
        self.closure = json.loads((ROOT / 'tests/freeze/task_service_confirmation.json').read_text())
        self.review = self.closure['dependencyReviews'][-1]

    def test_only_the_reviewed_login_entries_changed(self):
        rows = {path: digest for digest, path in (
            line.split('  ', 1) for line in (ROOT / NESTED).read_text().splitlines()
            if line and not line.startswith('#'))}
        self.assertEqual(len(rows), 24)
        self.assertEqual(set(self.review['changedEntries']), CHANGED)
        self.assertEqual(len(self.review['changedEntries']), len(CHANGED))
        self.assertEqual(digest_table({p: h for p, h in rows.items() if p not in CHANGED}),
                         self.review['preservedLoginEntriesSha256'])

    def test_original_confirmation_review_and_eighteen_hashes_are_preserved(self):
        self.assertEqual(self.closure['contract'], 'task-service-confirmation-v1')
        self.assertEqual(self.closure['review'], 'QA-3 section 23 / 2026-11-01')
        self.assertEqual(self.closure['positiveRelease'],
                         'exact Service durable-confirmation MAC only; original deadlines and CAS unchanged')
        self.assertIs(self.closure['formalAdmission'], False)
        self.assertEqual(self.closure['algorithm'], 'sha256-lf')
        self.assertEqual(len(self.closure['files']), 19)
        self.assertEqual(digest_table({p: h for p, h in self.closure['files'].items() if p != NESTED}),
                         self.review['preservedSourceFilesSha256'])

    def test_nested_review_and_all_current_closure_files_match(self):
        self.assertEqual(self.review['path'], NESTED)
        self.assertEqual(self.review['review'], 'IP-4 section 14.7 / 2026-11-03')
        self.assertEqual(self.review['previousSha256'],
                         '8ee6a3b5a20a2cf6f4be88d99b1d67ac502ce7b9ce60f4300dbf0a76e50df72b')
        self.assertNotEqual(self.review['previousSha256'], self.review['sha256'])
        self.assertEqual(self.closure['files'][NESTED], self.review['sha256'])
        for path, expected in self.closure['files'].items():
            with self.subTest(path=path):
                content = (ROOT / path).read_text(encoding='utf8').replace('\r\n', '\n')
                self.assertEqual(hashlib.sha256(content.encode()).hexdigest(), expected)

    def test_quality_ci_runs_the_finite_contracts_before_builds(self):
        workflow = (ROOT / '.github/workflows/device-v2-ci.yml').read_text(encoding='utf8')
        command = "python3 -m unittest discover -s tests/architecture -p 'test_*.py'"
        self.assertEqual(workflow.count(command), 1)
        self.assertLess(workflow.index('python3 tests/architecture/check_device_v2.py'),
                        workflow.index(command))
        self.assertLess(workflow.index(command), workflow.index('Python syntax boundary'))


if __name__ == '__main__':
    unittest.main()
