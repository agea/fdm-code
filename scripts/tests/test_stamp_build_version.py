import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('stamp', Path(__file__).parents[1] / 'stamp-build-version.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class StampTests(unittest.TestCase):
    def test_commit_date_survives_snapshot_and_dirty_tree_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            project = Path(tmp) / 'source'
            project.mkdir()
            def git(*args):
                return subprocess.check_output(['git', '-C', str(project), *args], text=True)
            git('init', '-q')
            git('-c', 'user.name=Test', '-c', 'user.email=test@example.com',
                'commit', '--allow-empty', '-qm', 'fixture',
                '--date=2020-01-01T00:00:00Z')
            module.ensure_clean(project)
            expected = module.source_date(project).strftime('%y.%m.%d.') + module.source_commit(project)[:8]
            snapshot = Path(tmp) / 'snapshot'
            (snapshot / 'Build').mkdir(parents=True)
            (snapshot / 'Build/SourceCommit.txt').write_text(module.source_commit(project))
            (snapshot / 'Build/SourceCommitDate.txt').write_text(module.source_date(project).isoformat())
            self.assertEqual(module.stamp(snapshot), expected)
            (project / 'untracked').write_text('dirty')
            with self.assertRaisesRegex(ValueError, 'dirty'):
                module.ensure_clean(project)

    def test_snapshot_preserves_commit_timezone(self):
        with tempfile.TemporaryDirectory() as tmp:
            project = Path(tmp)
            (project / 'Build').mkdir()
            (project / 'Build/SourceCommit.txt').write_text('a' * 40)
            (project / 'Build/SourceCommitDate.txt').write_text('2020-01-02T00:30:00+14:00')
            self.assertEqual(module.stamp(project), '20.01.02.aaaaaaaa')
