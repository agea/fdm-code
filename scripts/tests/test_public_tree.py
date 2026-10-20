import importlib.util
import hashlib
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

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

    def test_powershell_source_allowed_but_binary_in_script_rejected(self):
        for content, allowed in ((b'Write-Host "Windows build"\n', True), (b'binary\0payload', False)):
            with self.subTest(allowed=allowed), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                (root / 'scripts').mkdir()
                (root / 'scripts/package-windows.ps1').write_bytes(content)
                self.git(root, 'add', '.')
                self.git(root, 'commit', '-m', 'Windows helper')
                if allowed:
                    self.assertEqual(module.verify(root)[0], 2)
                else:
                    with self.assertRaisesRegex(RuntimeError, 'binary/LFS'):
                        module.verify(root)

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

    def test_exact_branding_and_objective_c_source_are_allowed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            name = 'Content/Splash/Icon.png'
            target = root / name
            target.parent.mkdir(parents=True)
            target.write_bytes((module.ROOT / name).read_bytes())
            source = root / 'Source/fdm/Mac/ERBrandingMac.mm'
            source.parent.mkdir(parents=True)
            source.write_text('// Native application icon\n')
            self.git(root, 'add', '.')
            self.git(root, 'commit', '-m', 'Approved branding')
            self.assertEqual(module.verify(root)[0], 2)

    def test_branding_path_does_not_allow_different_bytes_or_lfs(self):
        for content in (b'private\0asset', b'version https://git-lfs.github.com/spec/v1\n'):
            with self.subTest(content=content), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                target = root / 'Branding/icon.png'
                target.parent.mkdir()
                target.write_bytes(content)
                self.git(root, 'add', '.')
                self.git(root, 'commit', '-m', 'Substituted branding')
                with self.assertRaisesRegex(RuntimeError, 'unapproved branding content'):
                    module.verify(root)

    def test_approved_menu_fonts_and_companion_text_are_allowed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            fonts = {name for name in module.PUBLIC_BRANDING if name.endswith('.ttf')}
            self.assertEqual(len(fonts), 3)
            for name in fonts | module.PUBLIC_BRANDING_TEXT:
                target = root / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes((module.ROOT / name).read_bytes())
            self.git(root, 'add', '.')
            self.git(root, 'commit', '-m', 'Approved fonts and credits')
            self.assertEqual(module.verify(root)[0], 2)

    def test_font_exception_rejects_replacement_or_additional_binary(self):
        for name, message in (('Branding/Fonts/Teko/Fonts/Teko-SemiBold.ttf', 'unapproved branding content'),
                              ('Branding/Fonts/Other.ttf', 'forbidden path')):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                target = root / name
                target.parent.mkdir(parents=True)
                target.write_bytes(b'unapproved\0font')
                self.git(root, 'add', '.')
                self.git(root, 'commit', '-m', 'Unapproved font')
                with self.assertRaisesRegex(RuntimeError, message):
                    module.verify(root)

    def test_approved_branding_text_still_rejects_binary_and_lfs(self):
        for content in (b'private\0data', b'version https://git-lfs.github.com/spec/v1\n'):
            with self.subTest(content=content), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                target = root / 'Branding/Credits.txt'
                target.parent.mkdir()
                target.write_bytes(content)
                self.git(root, 'add', '.')
                self.git(root, 'commit', '-m', 'Invalid credits')
                with self.assertRaisesRegex(RuntimeError, 'binary/LFS'):
                    module.verify(root)

    def test_reviewed_plist_history_remains_allowed_but_third_variant_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            name = 'Build/Mac/Resources/Info.Template.plist'
            target = root / name
            target.parent.mkdir(parents=True)
            old, current = b'<plist>old</plist>\n', b'<plist>current</plist>\n'
            with patch.dict(module.PUBLIC_BRANDING, {name: hashlib.sha256(current).hexdigest()}), \
                 patch.dict(module.PUBLIC_BRANDING_HISTORY, {name: {hashlib.sha256(old).hexdigest()}}):
                for contents in (old, current):
                    target.write_bytes(contents)
                    self.git(root, 'add', '.')
                    self.git(root, 'commit', '-m', 'Reviewed plist version')
                self.assertEqual(module.verify(root)[0], 3)
                target.write_bytes(b'<plist>unreviewed</plist>\n')
                self.git(root, 'add', '.')
                self.git(root, 'commit', '-m', 'Unreviewed plist version')
                with self.assertRaisesRegex(RuntimeError, 'unapproved branding content'):
                    module.verify(root)

    def test_other_images_and_branding_symlinks_remain_forbidden(self):
        for name, symlink in (('Branding/private.png', False), ('Branding/icon.png', True)):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                target = root / name
                target.parent.mkdir()
                if symlink:
                    target.symlink_to('../README.md')
                else:
                    target.write_bytes(b'private\0image')
                self.git(root, 'add', '.')
                self.git(root, 'commit', '-m', 'Unapproved asset')
                with self.assertRaisesRegex(RuntimeError, 'forbidden path'):
                    module.verify(root)


if __name__ == '__main__':
    unittest.main()
