"""Sign a packaged Mac app, notarize it, staple the ticket and verify Gatekeeper.

Requires a Developer ID Application identity and notarytool Keychain profile.
Credentials stay in Keychain. Does not upload anything to itch.io.
"""
import argparse
import json
from pathlib import Path
import plistlib
import re
import subprocess
import tempfile


MACHO = {bytes.fromhex(x) for x in (
    'feedface', 'cefaedfe', 'feedfacf', 'cffaedfe',
    'cafebabe', 'bebafeca', 'cafebabf', 'bfbafeca')}


def run(*args, **kwargs):
    kwargs.setdefault('check', True)
    return subprocess.run(list(map(str, args)), **kwargs)


def is_macho(path):
    if path.is_symlink() or not path.is_file():
        return False
    with path.open('rb') as stream:
        return stream.read(4) in MACHO


def check_credentials(identity, keychain_profile, sign_only=False):
    identities = run('security', 'find-identity', '-v', '-p', 'codesigning', capture_output=True, text=True).stdout
    available = re.findall(r'"(Developer ID Application:[^"\n]+)"', identities)
    if identity is None and len(set(available)) == 1:
        identity = available[0]
    if not identity or identity not in available:
        raise RuntimeError('Install a valid Developer ID Application certificate with its private key in Keychain; '
                           'if several are installed, choose one with --identity "Developer ID Application: Name (TEAMID)"')
    if not sign_only:
        try:
            run('xcrun', 'notarytool', 'history', '--keychain-profile', keychain_profile,
                '--output-format', 'json', stdout=subprocess.DEVNULL)
        except subprocess.CalledProcessError as error:
            raise RuntimeError('Notarytool credentials unavailable or authentication failed; configure/check '
                               'the Keychain profile with: xcrun notarytool store-credentials ' +
                               str(keychain_profile)) from error
    return identity


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('app', type=Path)
    parser.add_argument('--identity', required=True, help='Full Developer ID Application identity name')
    parser.add_argument('--keychain-profile')
    parser.add_argument('--sign-only', action='store_true', help='Sign and verify locally; notarization will be done through Xcode Organizer')
    parser.add_argument('--check', action='store_true', help='Check prerequisites without changing the app or submitting it')
    parser.add_argument('--scratch-directory', type=Path, help='Existing directory for the temporary submission ZIP (e.g. the build SSD)')
    args = parser.parse_args()
    if not args.sign_only and not args.keychain_profile:
        parser.error('--keychain-profile is required unless --sign-only is selected')
    app = args.app.resolve()
    if app.suffix != '.app' or not (app / 'Contents/Info.plist').is_file():
        parser.error('Pass the final packaged .app')
    if args.scratch_directory and not args.scratch_directory.is_dir():
        parser.error('--scratch-directory must exist')
    try:
        check_credentials(args.identity, args.keychain_profile, args.sign_only)
    except RuntimeError as error:
        parser.error(str(error))
    if args.check:
        print('Signing identity ready' if args.sign_only else 'Signing identity and notarization credentials ready')
        return
    # Keep dist clean: the itch uploader accepts only the packaged app there.
    receipt_path = app.parent.parent / (app.stem + '-notarization.json')
    receipt = {'app': str(app), 'identity': args.identity, 'status': 'signing'}
    try:
        with tempfile.TemporaryDirectory(prefix='rr-notarize-', dir=args.scratch_directory) as scratch:
            scratch = Path(scratch)
            entitlements = scratch / 'entitlements.plist'
            # No debugger/get-task-allow entitlement in a distributed app.
            entitlements.write_bytes(plistlib.dumps({}))
            code = [p for p in app.rglob('*') if is_macho(p)]
            bundles = [p for p in app.rglob('*') if p.is_dir() and not p.is_symlink()
                       and p.suffix in ('.app', '.framework', '.xpc', '.appex', '.bundle')
                       and ((p / 'Contents/Info.plist').is_file() or p.suffix == '.framework')]
            # Sign nested code before its enclosing bundle. Do not use --deep
            # for signing: every embedded Mach-O gets an explicit signature.
            for path in sorted(set(code + bundles), key=lambda p: len(p.parts), reverse=True) + [app]:
                run('codesign', '--force', '--sign', args.identity, '--timestamp',
                    '--options', 'runtime', '--entitlements', entitlements, path)
            run('codesign', '--verify', '--deep', '--strict', '--verbose=2', app)
            if args.sign_only:
                receipt['status'] = 'signed-not-yet-notarized'
                return
            archive = scratch / 'submission.zip'
            run('ditto', '-c', '-k', '--keepParent', app, archive)
            receipt['status'] = 'submitting'
            submission = run('xcrun', 'notarytool', 'submit', archive,
                '--keychain-profile', args.keychain_profile, '--wait',
                '--output-format', 'json', capture_output=True, text=True, check=False)
            receipt['submission'] = json.loads(submission.stdout)
            if submission.returncode or receipt['submission'].get('status') != 'Accepted':
                submission_id = receipt['submission'].get('id')
                if submission_id:
                    run('xcrun', 'notarytool', 'log', submission_id,
                        '--keychain-profile', args.keychain_profile,
                        receipt_path.with_name(app.stem + '-notary-log.json'))
                raise RuntimeError('Apple did not accept the submission; inspect notarization receipt/log')
            run('xcrun', 'stapler', 'staple', app)
            run('xcrun', 'stapler', 'validate', app)
            run('codesign', '--verify', '--deep', '--strict', '--verbose=2', app)
            run('spctl', '--assess', '--type', 'execute', '--verbose=2', app)
            receipt['status'] = 'accepted-and-gatekeeper-verified'
    except BaseException:
        receipt['failed_at'] = receipt['status']
        receipt['status'] = 'failed'
        raise
    finally:
        receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')
        print('Notarization receipt:', receipt_path)


if __name__ == '__main__':
    main()
