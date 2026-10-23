"""Exercise destructive cleanup only inside temporary fixtures."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('build_cleanup', Path(__file__).parents[1] / 'build_cleanup.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class CleanupTests(unittest.TestCase):
    def fixture(self, root, name, status, release=True):
        run = root / name
        payload = run / ('build/project' if release else 'project')
        payload.mkdir(parents=True)
        (payload / 'asset').write_text('fixture')
        (run / ('release.json' if release else 'receipt.json')).write_text(json.dumps({'status': status}))
        (run / 'build.log').write_text('keep')
        return run, payload

    def test_only_completed_payloads_removed_and_receipts_retained(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            done, payload = self.fixture(root, 'done', 'uploaded')
            _, failed = self.fixture(root, 'failed', 'failed')
            _, running = self.fixture(root, 'running', 'building')
            _, ready = self.fixture(root, 'ready', 'ready')
            standalone, packaged = self.fixture(root, 'standalone', 'packaged', release=False)
            module.cleanup_old_builds([root], dry_run=True)
            self.assertTrue(payload.exists())
            module.cleanup_old_builds([root])
            self.assertFalse(payload.exists())
            self.assertFalse(packaged.exists())
            self.assertTrue(all(p.exists() for p in (failed, running, ready)))
            self.assertTrue((done / 'build.log').exists())
            self.assertIn('payload_removed_at', json.loads((done / 'release.json').read_text()))
            self.assertTrue(json.loads((standalone / 'receipt.json').read_text())['scratch_removed'])
            module.cleanup_old_builds([root])  # Idempotent.

    def test_symlinked_payload_and_run_are_preserved(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / 'builds'
            root.mkdir()
            outside = Path(tmp) / 'outside'
            outside.mkdir()
            (outside / 'important').write_text('keep')
            run, payload = self.fixture(root, 'done', 'uploaded')
            (payload / 'asset').unlink()
            payload.rmdir()
            payload.symlink_to(outside, target_is_directory=True)
            (root / 'linked-run').symlink_to(run, target_is_directory=True)
            module.cleanup_old_builds([root])
            self.assertTrue((outside / 'important').exists())
            self.assertTrue(payload.is_symlink())


if __name__ == '__main__':
    unittest.main()
