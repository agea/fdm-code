"""Offline release workflow tests. Never invoke Apple services or upload a build."""
import importlib.util
import json
from pathlib import Path
import plistlib
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('release_macos', Path(__file__).parents[1] / 'release-macos.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
IDENTITY = 'Developer ID Application: Fixture (TESTTEAM)'
VERSION = '26.10.20.1234abcd'


class MacReleaseTests(unittest.TestCase):
    def args(self, root, **overrides):
        values = dict(engine=root / 'engine', workspace=root / 'releases', architecture='universal',
                      configuration='Development', identity=IDENTITY, keychain_profile='fixture',
                      butler='butler', butler_identity=None, init_assets=False, check=False,
                      no_upload=False, resume=None, upload_only=None, notarization_method='auto')
        values.update(overrides)
        return SimpleNamespace(**values)

    def fixture(self, root, signed=False):
        app = root / 'build/dist/RespawnAndRage.app'
        for path in ('Contents/UE/RespawnAndRage/Build/GameVersion.txt', 'Contents/Resources/AppIcon.icns',
                     'Contents/MacOS/RespawnAndRage'):
            file = app / path
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_text(VERSION if file.name == 'GameVersion.txt' else 'fixture')
        (app / 'Contents/Info.plist').write_bytes(plistlib.dumps({
            'NSHighResolutionCapable': True, 'CFBundleExecutable': 'RespawnAndRage'}))
        module.save(root / 'build/receipt.json', {'status': 'packaged', 'architecture': 'arm64+x64',
                    'game_version': VERSION, 'archive': str(root / 'build/dist')})
        if signed:
            module.save(root / 'build/RespawnAndRage-notarization.json',
                        {'status': 'accepted-and-gatekeeper-verified', 'app': str(app)})
        return app

    def runner(self, calls, fail=None):
        def fake(command, log=None, check=True):
            command = list(map(str, command))
            calls.append(command)
            stage = ('build' if any(p.endswith('/package-macos.py') for p in command) else
                     'notarize' if any(p.endswith('/notarize-macos.py') for p in command) else
                     'push' if 'push' in command else 'verify')
            if fail == stage or (fail == 'stapler' and command[:3] == ['xcrun', 'stapler', 'validate']):
                raise RuntimeError(stage + ' failed')
            if stage == 'build':
                run_dir = Path(command[command.index('--run-directory') + 1]).parent
                self.fixture(run_dir)
            if stage == 'notarize':
                self.assertIn(command[command.index('--method') + 1], ('notarytool', 'xcode'))
                app = Path(command[2])
                module.save(app.parent.parent / 'RespawnAndRage-notarization.json',
                            {'status': 'accepted-and-gatekeeper-verified', 'app': str(app)})
                scratch = Path(command[command.index('--scratch-directory') + 1])
                self.assertTrue(scratch.is_dir())
                self.assertEqual(scratch.parent, app.parents[2])
            return subprocess.CompletedProcess(command, 0, stdout='arm64 x86_64\n',
                stderr='Authority=' + IDENTITY + '\nflags=0x10000(runtime)\n')
        return fake

    def exercise(self, fail=None, no_upload=False):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            calls = []
            args = self.args(root, no_upload=no_upload)
            with patch.object(module, 'preflight', side_effect=lambda a: setattr(a, 'notarization_method', 'xcode')), patch.object(module, 'disk_space'), \
                    patch.object(module, 'run', side_effect=self.runner(calls, fail)):
                if fail:
                    with self.assertRaises(RuntimeError):
                        module.release(args)
                else:
                    module.release(args)
            receipt = json.loads(next(args.workspace.glob('*/release.json')).read_text())
            return calls, receipt

    def test_success_requires_signing_and_ticket_before_push_and_keeps_dist_clean(self):
        calls, receipt = self.exercise()
        self.assertEqual(receipt['status'], 'uploaded')
        push = calls[-1]
        self.assertIn(module.GAME + ':mac-universal', push)
        self.assertEqual(push[push.index('--userversion') + 1], VERSION)
        self.assertEqual(Path(push[2]).name, 'dist')
        self.assertTrue(any(c[:3] == ['xcrun', 'stapler', 'validate'] for c in calls[:-1]))
        self.assertTrue(any(c[0] == 'spctl' for c in calls[:-1]))

    def test_failed_build_never_signs_or_uploads(self):
        calls, receipt = self.exercise(fail='build')
        self.assertEqual(len(calls), 1)
        self.assertEqual(receipt['failed_at'], 'building')

    def test_rejected_notarization_and_failed_ticket_never_upload(self):
        for failure in ('notarize', 'stapler'):
            calls, receipt = self.exercise(fail=failure)
            self.assertFalse(any('push' in c for c in calls))
            self.assertEqual(receipt['status'], 'failed')

    def test_no_upload_still_notarizes_and_verifies(self):
        calls, receipt = self.exercise(no_upload=True)
        self.assertEqual(receipt['status'], 'ready')
        self.assertTrue(any(c[0] == 'spctl' for c in calls))
        self.assertFalse(any('push' in c for c in calls))

    def test_package_ignores_retained_xcode_app_but_rejects_multiple_delivery_apps(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            original = self.fixture(root)
            app = original.parent / 'Mac' / original.name
            app.parent.mkdir()
            original.rename(app)
            diagnostic = root / 'build/dist/RespawnAndRage.xcarchive/Products/Applications/RespawnAndRage.app'
            diagnostic.mkdir(parents=True)
            with patch.object(module, 'run', side_effect=self.runner([])):
                self.assertEqual(module.package(root)[0], app)
                (root / 'build/dist/Other/RespawnAndRage.app').mkdir(parents=True)
                with self.assertRaisesRegex(RuntimeError, 'Expected exactly one packaged'):
                    module.package(root)

    def test_resume_after_notarization_failure_verifies_without_rebuilding_or_uploading(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.fixture(root)
            module.save(root / 'release.json', {'status': 'failed', 'failed_at': 'notarizing',
                                               'error': 'previous Xcode authentication failure'})
            calls = []
            args = self.args(root, resume=root, no_upload=True)
            with patch.object(module, 'preflight', side_effect=lambda a: setattr(a, 'notarization_method', 'xcode')), \
                    patch.object(module, 'disk_space'), patch.object(module, 'run', side_effect=self.runner(calls)):
                module.release(args)
            receipt = json.loads((root / 'release.json').read_text())
            self.assertEqual(receipt['status'], 'ready')
            self.assertNotIn('error', receipt)
            self.assertNotIn('failed_at', receipt)
            self.assertTrue(any(any(p.endswith('/notarize-macos.py') for p in c) for c in calls))
            self.assertTrue(any(c[0] == 'spctl' for c in calls))
            self.assertFalse(any('push' in c or any(p.endswith('/package-macos.py') for p in c) for c in calls))

    def test_upload_retry_never_rebuilds_or_resigns_and_records_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            app = self.fixture(root, signed=True)
            calls = []
            args = self.args(root, upload_only=root)
            with patch.object(module, 'preflight'), patch.object(module, 'run', side_effect=self.runner(calls, 'push')):
                with self.assertRaises(RuntimeError):
                    module.release(args)
            self.assertEqual(json.loads((root / 'release.json').read_text())['failed_at'], 'uploading')
            self.assertFalse(any(c[1].endswith('.py') for c in calls if len(c) > 1))
            self.assertTrue((app / 'Contents/Info.plist').exists())

    def test_missing_or_unsigned_receipt_and_wrong_version_block_upload(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            app = self.fixture(root)
            with patch.object(module, 'run') as run:
                with self.assertRaises(OSError):
                    module.verify_distribution(app)
                run.assert_not_called()
            (app / 'Contents/UE/RespawnAndRage/Build/GameVersion.txt').write_text('26.10.20.deadbeef')
            with self.assertRaisesRegex(RuntimeError, 'version'):
                module.package(root)

    def test_missing_credentials_stop_before_creating_release(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            args = self.args(root)
            fake_helper = SimpleNamespace(ensure_idle=lambda: None,
                resolve_credentials=lambda *args: (_ for _ in ()).throw(RuntimeError('missing Keychain profile')))
            calls = []
            with patch.object(module.platform, 'system', return_value='Darwin'), \
                    patch.object(module.shutil, 'which', return_value='/fixture/tool'), \
                    patch.object(module, 'helper', return_value=fake_helper), patch.object(module, 'check_build'), \
                    patch.object(module, 'check_butler'), patch.object(module, 'run', side_effect=self.runner(calls)):
                with self.assertRaisesRegex(RuntimeError, 'Preflight failed'):
                    module.release(args)
            self.assertFalse(args.workspace.exists())
            self.assertFalse(any('push' in c or any(p.endswith('/package-macos.py') for p in c) for c in calls))

    def test_check_never_creates_workspace_or_runs_release_stages(self):
        with tempfile.TemporaryDirectory() as temporary:
            args = self.args(Path(temporary), check=True)
            with patch.object(module, 'preflight'), patch.object(module, 'run') as run:
                module.release(args)
                run.assert_not_called()
            self.assertFalse(args.workspace.exists())

    def test_notarization_space_is_checked_on_release_disk(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            app = self.fixture(root)
            with patch.object(module, 'disk_space', side_effect=RuntimeError('disk full')) as space:
                with self.assertRaisesRegex(RuntimeError, 'disk full'):
                    module.notarization_space(app, root)
                self.assertEqual(space.call_args.args[0], root)
                self.assertGreater(space.call_args.args[1], module.GIB)

    def test_discovers_existing_builder_cli_and_identity_without_reading_the_key(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            binary = root / 'unreal-builder/.work/butler/butler'
            binary.parent.mkdir(parents=True)
            binary.write_text('fixture executable')
            binary.chmod(0o755)
            identity = binary.parent.parent / 'itch-identity'
            identity.write_text('test fixture; never read')
            args = self.args(root)
            with patch.object(module, 'PROJECT', root / 'core'), patch.object(module.Path, 'home', return_value=root / 'home'), \
                    patch.object(module.shutil, 'which', return_value=None), patch.dict(module.os.environ, {}, clear=True):
                module.discover_butler(args)
            self.assertEqual(args.butler, str(binary.resolve()))
            self.assertEqual(args.butler_identity, identity.resolve())

    def test_explicit_butler_configuration_is_preserved(self):
        args = self.args(Path('/fixture'), butler='/explicit/butler', butler_identity=Path('/explicit/identity'))
        module.discover_butler(args)
        self.assertEqual(args.butler, '/explicit/butler')
        self.assertEqual(args.butler_identity, Path('/explicit/identity'))

    def test_installed_metal_component_is_refreshed_before_retrying_compiler(self):
        args = self.args(Path('/fixture'))
        results = [subprocess.CompletedProcess([], 1), subprocess.CompletedProcess([], 0),
                   subprocess.CompletedProcess([], 0)]
        with patch.object(module, 'run', side_effect=results) as run:
            module.check_build(args, lambda name, action: action() if name == 'Metal shader compiler' else None)
        self.assertEqual(run.call_args_list[1].args[0], ['xcodebuild', '-showComponent', 'MetalToolchain'])
        self.assertEqual(run.call_args_list[2].args[0], ['xcrun', '--sdk', 'macosx', 'metal', '-v'])

    def test_missing_metal_compiler_still_blocks_the_release(self):
        args = self.args(Path('/fixture'))
        with patch.object(module, 'run', return_value=subprocess.CompletedProcess([], 1)):
            with self.assertRaisesRegex(RuntimeError, 'Metal compiler unavailable'):
                module.check_build(args, lambda name, action: action() if name == 'Metal shader compiler' else None)


if __name__ == '__main__':
    unittest.main()
