"""Exercise cleanup on disposable projects, including protected data and links."""
import contextlib
import importlib.util
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('cleanup', Path(__file__).resolve().parents[1] / 'clean-project.py')
cleanup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cleanup)

class CleanupTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve() / 'project'
        self.root.mkdir()
        subprocess.run(['git', 'init', '-q', str(self.root)], check=True)
        for name in ('RespawnAndRage.uproject', 'Content/keep.uasset', 'SourceAssets/keep.fbx',
                     'Saved/Config/settings.ini', 'Saved/Autosaves/keep.umap',
                     'Saved/AssetAudit/report.json', 'Saved/Testing/test.log',
                     'Saved/Testing/reference.png', 'Intermediate/Build/generated.o'):
            p = self.root / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text('preserve or generated')

    def run_cleanup(self, *args):
        with patch.object(cleanup, 'PROJECT', self.root), patch('sys.argv', ['clean-project.py', *args]), contextlib.redirect_stdout(io.StringIO()):
            cleanup.main()

    def test_default_preview_never_deletes(self):
        self.run_cleanup()
        self.assertTrue((self.root / 'Saved/Testing/test.log').exists())

    def test_log_cleanup_preserves_assets_settings_evidence_images(self):
        with patch.object(cleanup, 'ensure_idle'):
            self.run_cleanup('--apply')
        self.assertFalse((self.root / 'Saved/Testing/test.log').exists())
        for name in ('Content/keep.uasset', 'SourceAssets/keep.fbx', 'Saved/Config/settings.ini',
                     'Saved/Autosaves/keep.umap', 'Saved/AssetAudit/report.json', 'Saved/Testing/reference.png',
                     'Intermediate/Build/generated.o'):
            self.assertTrue((self.root / name).exists(), name)

    def test_indexed_generated_file_prevents_any_deletion(self):
        subprocess.run(['git', '-C', str(self.root), 'add', 'Saved/Testing/test.log'], check=True)
        with self.assertRaisesRegex(RuntimeError, 'tracked/indexed'):
            self.run_cleanup('--apply')
        self.assertTrue((self.root / 'Saved/Testing/test.log').exists())

    def test_symlinked_directory_and_file_are_not_followed(self):
        outside = Path(self.temp.name) / 'outside'
        outside.mkdir()
        target = outside / 'precious.log'
        target.write_text('keep')
        (self.root / 'Saved/Testing/link').symlink_to(outside, target_is_directory=True)
        (self.root / 'Saved/Testing/link.log').symlink_to(target)
        with patch.object(cleanup, 'ensure_idle'):
            self.run_cleanup('--apply')
        self.assertTrue(target.exists())
        self.assertTrue((self.root / 'Saved/Testing/link.log').is_symlink())

    def test_active_unreal_prevents_deletion(self):
        process = subprocess.CompletedProcess([], 0, stdout='UnrealEditor\n')
        with patch.object(cleanup, 'tracked_paths', return_value=set()), patch.object(cleanup.subprocess, 'run', return_value=process):
            with self.assertRaisesRegex(RuntimeError, 'Close Unreal'):
                self.run_cleanup('--apply')
        self.assertTrue((self.root / 'Saved/Testing/test.log').exists())

    def test_finder_editor_service_does_not_block_cleanup(self):
        process = subprocess.CompletedProcess([], 0, stdout='/Applications/UnrealEditorServices.app/Contents/MacOS/UnrealEditorServices\n')
        with patch.object(cleanup, 'tracked_paths', return_value=set()), patch.object(cleanup.subprocess, 'run', return_value=process):
            self.run_cleanup('--apply')
        self.assertFalse((self.root / 'Saved/Testing/test.log').exists())

if __name__ == '__main__':
    unittest.main()
