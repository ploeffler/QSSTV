"""Offline release readiness regression tests. Run: python3 tests/test_unified_release.py

Requires PyYAML and jq; no GitHub access or release publishing.
"""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = yaml.safe_load((ROOT / '.github/workflows/experimental-unified-release.yml').read_text())
SCRIPT = WORKFLOW['jobs']['publish']['steps'][0]['run']


class ReadinessTests(unittest.TestCase):
    def run_check(self, runs, event='workflow_run', api_failure=False):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            mock = folder / 'gh'
            mock.write_text('#!/bin/bash\nif [[ "$API_FAILURE" == 1 ]]; then exit 1; fi\ncat "$FIXTURE"\n')
            mock.chmod(0o755)
            fixture = folder / 'runs.json'
            fixture.write_text(json.dumps(runs))
            output = folder / 'output'
            output.touch()
            env = dict(os.environ, PATH=directory + ':' + os.environ['PATH'],
                       FIXTURE=str(fixture), API_FAILURE=str(int(api_failure)),
                       TARGET_SHA='abc123', GH_REPO='example/repo',
                       TRIGGER_EVENT=event, GITHUB_OUTPUT=str(output))
            result = subprocess.run(['bash', '-e', '-o', 'pipefail', '-c', SCRIPT],
                                    env=env, capture_output=True, text=True)
            return result, output.read_text()

    def test_automatic_missing_builds_defer(self):
        result, output = self.run_check([])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output, 'ready=false\n')
        self.assertIn('Windows experimental build', result.stderr)
        self.assertIn('Linux DEB experimental', result.stderr)
        self.assertIn('Linux AppImage experimental', result.stderr)

    def test_manual_missing_builds_fail_visibly(self):
        result, output = self.run_check([], 'workflow_dispatch')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('::error::Required builds are missing', result.stdout)
        self.assertEqual(output, '')

    def test_only_latest_matching_success_is_selected(self):
        valid = dict(databaseId=10, headSha='abc123', headBranch='main',
                     event='push', conclusion='success')
        runs = [valid, dict(valid, databaseId=20, event='workflow_dispatch')]
        for field, value in [('headSha', 'other'), ('headBranch', 'feature/test'),
                             ('event', 'pull_request'), ('conclusion', 'failure')]:
            runs.append(dict(valid, databaseId=100, **{field: value}))
        result, output = self.run_check(runs)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output, 'ready=true\nwin=20\ndeb=20\napp=20\n')

    def test_api_failure_is_not_treated_as_pending(self):
        result, output = self.run_check([], api_failure=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('::error::Could not query builds', result.stderr)
        self.assertEqual(output, '')


if __name__ == '__main__':
    unittest.main()
