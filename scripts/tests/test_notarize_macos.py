"""Offline checks of the release gate; never invoke signing or Apple services."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('notarize_macos', Path(__file__).parents[1] / 'notarize-macos.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
IDENTITY = 'Developer ID Application: Test Fixture (TESTTEAM)'


class NotarizationGate(unittest.TestCase):
    def exercise(self, status, identity_available=True, sign_only=False):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            app = root / 'dist/Game.app'
            (app / 'Contents').mkdir(parents=True)
            (app / 'Contents/Info.plist').write_bytes(b'fixture')
            calls = []

            def fake_run(*args, **kwargs):
                calls.append(tuple(map(str, args)))
                output = ''
                if args[0] == 'security':
                    output = f'1) TEST "{IDENTITY}"' if identity_available else '0 valid identities found'
                if args[:3] == ('xcrun', 'notarytool', 'submit'):
                    output = json.dumps({'id': 'fixture-id', 'status': status})
                return subprocess.CompletedProcess(args, 0, stdout=output, stderr='')

            argv = ['notarize-macos.py', str(app), '--identity', IDENTITY, '--keychain-profile', 'fixture']
            if sign_only:
                argv.append('--sign-only')
            with patch.object(module, 'run', side_effect=fake_run), patch.object(sys, 'argv', argv):
                if not identity_available:
                    with self.assertRaises(SystemExit):
                        module.main()
                elif status != 'Accepted':
                    with self.assertRaises(RuntimeError):
                        module.main()
                else:
                    module.main()
            receipt = root / 'Game-notarization.json'
            return calls, json.loads(receipt.read_text()) if receipt.exists() else None, list((root / 'dist').iterdir()) == [app]

    def test_missing_identity_never_changes_or_submits_app(self):
        calls, receipt, clean = self.exercise('Accepted', identity_available=False)
        self.assertEqual(len(calls), 1)
        self.assertIsNone(receipt)
        self.assertTrue(clean)

    def test_rejected_submission_never_reports_ready_or_staples(self):
        calls, receipt, clean = self.exercise('Invalid')
        self.assertEqual(receipt['status'], 'failed')
        self.assertFalse(any(c[:2] == ('xcrun', 'stapler') or c[0] == 'spctl' for c in calls))
        self.assertTrue(clean)

    def test_accepted_submission_requires_gatekeeper_check(self):
        calls, receipt, clean = self.exercise('Accepted')
        self.assertEqual(receipt['status'], 'accepted-and-gatekeeper-verified')
        self.assertEqual(calls[-1][0], 'spctl')
        self.assertTrue(clean)

    def test_sign_only_does_not_claim_notarization(self):
        calls, receipt, clean = self.exercise('Accepted', sign_only=True)
        self.assertEqual(receipt['status'], 'signed-not-yet-notarized')
        self.assertFalse(any(c[:2] == ('xcrun', 'notarytool') for c in calls))
        self.assertTrue(clean)


if __name__ == '__main__':
    unittest.main()
