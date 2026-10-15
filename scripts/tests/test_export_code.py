import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('code_export', Path(__file__).resolve().parents[1] / 'export-code.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class CodeExportTests(unittest.TestCase):
    def test_export_excludes_assets_git_archives_and_symlinks(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve() / 'project'
            for name in ('RespawnAndRage.uproject', '.gitignore', 'Source/fdm/main.cpp', 'Config/DefaultEngine.ini',
                         'Content/private.uasset', 'SourceAssets/private.fbx', '.git/secret',
                         'scripts/private.zip', 'docs/private.png'):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(name)
            outside = Path(directory).resolve() / 'private.py'
            outside.write_text('private')
            (root / 'scripts/link.py').symlink_to(outside)
            output = root / 'BuildArtifacts/export'
            module.export(root, output)
            manifest = json.loads((output / 'CODE_EXPORT.json').read_text())
            self.assertEqual({f['path'] for f in manifest['files']},
                             {'RespawnAndRage.uproject', '.gitignore', 'Source/fdm/main.cpp', 'Config/DefaultEngine.ini'})
            self.assertIn('/Content/', (output / '.gitignore').read_text())
            self.assertNotIn('/Content/', (root / '.gitignore').read_text())
            self.assertFalse((output / '.git').exists())
            self.assertFalse((output / 'Content').exists())
            self.assertTrue((root / 'Content/private.uasset').exists())
            with self.assertRaisesRegex(RuntimeError, 'must not already exist'):
                module.export(root, output)

if __name__ == '__main__':
    unittest.main()
