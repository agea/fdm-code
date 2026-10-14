import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import runpy

spec = importlib.util.spec_from_file_location('asset_setup', Path(__file__).resolve().parents[1] / 'setup-assets.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class AssetSetupTests(unittest.TestCase):
    def fixture(self, root):
        (root / 'assets.json').write_text(json.dumps({'repositories': {
            'characters': {'path': 'Assets/characters', 'mounts': ['Content/Characters']},
            'maps': {'path': 'Assets/maps', 'mounts': ['Content/EclipseRecoil/Maps']}}}))
        for folder in ('Assets/characters/Content/Characters', 'Assets/maps/Content/EclipseRecoil/Maps'):
            (root / folder).mkdir(parents=True)
            (root / folder / 'test.uasset').write_bytes(b'asset data')

    def test_preserves_bytes_and_relative_paths_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            self.assertEqual(module.mount(root), 2)
            self.assertEqual(module.mount(root), 2)
            self.assertEqual((root / 'Content/Characters/test.uasset').read_bytes(), b'asset data')
            self.assertTrue((root / 'Content/Characters').is_symlink())
            self.assertFalse((root / 'Content/Characters').readlink().is_absolute())
            module.validate(root, require_mounts=True)
            iterator = runpy.run_path(str(Path(__file__).resolve().parents[1] / 'asset_files.py'))['content_files']
            self.assertEqual({p.relative_to(root).as_posix() for p in iterator(root)},
                             {'Content/Characters/test.uasset', 'Content/EclipseRecoil/Maps/test.uasset'})

    def test_existing_content_is_never_overwritten_and_no_partial_mounts(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / 'Content/EclipseRecoil/Maps').mkdir(parents=True)
            marker = root / 'Content/EclipseRecoil/Maps/edited.umap'
            marker.write_bytes(b'edited')
            with self.assertRaisesRegex(RuntimeError, 'replace existing data'):
                module.mount(root)
            self.assertEqual(marker.read_bytes(), b'edited')
            self.assertFalse((root / 'Content/Characters').exists())

    def test_lfs_pointer_blocks_entire_plan(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / 'Assets/maps/Content/EclipseRecoil/Maps/test.uasset').write_bytes(module.LFS_HEADER + b'\n')
            with self.assertRaisesRegex(RuntimeError, 'Unresolved LFS'):
                module.mount(root)
            self.assertFalse((root / 'Content/Characters').exists())

    def test_conflicting_symlink_is_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / 'Content').mkdir()
            target = root / 'Assets/maps/Content/EclipseRecoil/Maps'
            (root / 'Content/Characters').symlink_to(target)
            with self.assertRaisesRegex(RuntimeError, 'Conflicting mount'):
                module.mount(root)
            self.assertEqual((root / 'Content/Characters').resolve(), target.resolve())

    def test_invalid_or_overlapping_paths_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for mounts in (['../secret'], ['Content', 'Content/Characters']):
                (root / 'assets.json').write_text(json.dumps({'repositories': {
                    'characters': {'path': 'Assets/characters', 'mounts': mounts}}}))
                with self.assertRaises(RuntimeError):
                    module.layout(root)


if __name__ == '__main__':
    unittest.main()
