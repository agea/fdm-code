import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('public_tree', Path(__file__).resolve().parents[1] / 'verify-public-tree.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class PublicTreeTests(unittest.TestCase):
    def git(self, root, *args):
        return subprocess.check_output(['git', *args], cwd=root, stderr=subprocess.STDOUT)

    def fixture(self, root):
        self.git(root, 'init', '-b', 'main')
        self.git(root, 'config', 'user.name', 'Test')
        self.git(root, 'config', 'user.email', 'test@example.invalid')
        # Avoid inherited LFS filters: the public parent owns only text.
        (root / '.gitattributes').write_text('* text=auto\n')
        (root / 'README.md').write_text('Public code\n')
        self.git(root, 'add', '.')
        self.git(root, 'commit', '-m', 'Code')

    def test_text_and_gitlink_are_allowed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            oid = self.git(root, 'rev-parse', 'HEAD').decode().strip()
            self.git(root, 'update-index', '--add', '--cacheinfo', '160000', oid, 'Assets/characters')
            self.git(root, 'commit', '-m', 'Asset link only')
            self.assertEqual(module.verify(root)[0], 2)

    def test_removed_asset_still_blocks_publication_of_history(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / 'Content').mkdir()
            (root / 'Content/private.uasset').write_bytes(b'private data')
            self.git(root, 'add', '.')
            self.git(root, 'commit', '-m', 'Asset leak')
            self.git(root, 'rm', 'Content/private.uasset')
            self.git(root, 'commit', '-m', 'Remove from current tree')
            with self.assertRaisesRegex(RuntimeError, 'forbidden path'):
                module.verify(root)

    def test_binary_blob_disguised_as_source_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / 'Source').mkdir()
            (root / 'Source/disguised.cpp').write_bytes(b'binary\0data')
            self.git(root, 'add', '.')
            self.git(root, 'commit', '-m', 'Invalid source')
            with self.assertRaisesRegex(RuntimeError, 'binary/LFS'):
                module.verify(root)


if __name__ == '__main__':
    unittest.main()
