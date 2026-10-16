#!/usr/bin/env python3
"""Reject private assets and unapproved paths throughout public Git history."""
import hashlib
import subprocess
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
# The previous project filename remains allowed only to audit existing Git history.
ROOT_FILES = {'fdm.uproject', 'RespawnAndRage.uproject', 'README.md', 'AGENTS.md', '.gitignore', '.gitattributes',
              '.gitmodules', 'assets.json', 'ASSETS_REQUIRED.md', 'CODE_EXPORT.json', 'Branding/README.md'}
EXTENSIONS = {'Source': {'.cpp', '.h', '.cs', '.mm'}, 'Config': {'.ini'},
              'scripts': {'.py', '.command', '.sh'}, 'docs': {'.md', '.txt', '.json', '.csv'}}
SUBMODULES = {'Assets/characters', 'Assets/weapons', 'Assets/maps'}
# User-supplied game branding and its prepared derivatives, 16 October 2026.
# Pin both path and bytes: this is not permission for arbitrary binary assets.
PUBLIC_BRANDING = {
    'Branding/Originals/icon.png': '02af44bd47fd2183c5a48135377c0fb912d8ccdc012b5dfcc8b5bd1aca5736d5',
    'Branding/Originals/logo.png': 'c59c323dd69b356a575331386155e3e597e8174d2a3b630246cb49047012ebdf',
    'Branding/Originals/splash.png': '205a1e94c9be875585c24494a968e2bafb5753d419f487392a45bc8f9dca6828',
    'Branding/background.jpg': 'b08ac4f5e52748f2a22f53cf28dfcdeee63fb1efef3704ce9cb6f4c192d047b3',
    'Branding/icon.png': '10320d04e40b2685e42e012586f23189a729a9dc34c425fd590492d58c2126d5',
    'Branding/logo.png': '1abb569b4498bad262d43523c638d1d2355e8e847b2af1fea15ed577088446a2',
    'Build/Mac/Application.icns': '2b24c5fc260ecb241ec2c4a67c3ac0779f77110f3db6ca7aa2573853bdf079c7',
    'Build/Windows/Application.ico': '2629e2aa33d8408b7bcdbe154a6dd76bab2bfd2b97b8899fd52456590eb6e3a3',
    'Content/Splash/EdSplash.png': '2ee026ee51b2a4fd771e31f9440c4051c5240521fab56a316553a7d5733d1d06',
    'Content/Splash/Icon.png': '1a6386df237dfebda6c7ca6daf0a9cf7b4d925e2f4e7a8e3231d7f10eea618a8',
    'Content/Splash/Splash.png': '2ee026ee51b2a4fd771e31f9440c4051c5240521fab56a316553a7d5733d1d06',
}


def verify(root):
    revisions = subprocess.check_output(['git', 'rev-list', '--all'], cwd=root, text=True).splitlines()
    failures = []
    count = 0
    for revision in revisions:
        tree = subprocess.check_output(['git', 'ls-tree', '-rz', revision], cwd=root).split(b'\0')
        for item in tree:
            if not item:
                continue
            info, raw_path = item.split(b'\t', 1)
            mode, kind, oid = info.decode().split()
            name = raw_path.decode()
            path = Path(name)
            if mode == '160000' and name in SUBMODULES:
                continue
            if name in PUBLIC_BRANDING and kind == 'blob' and mode == '100644':
                blob = subprocess.check_output(['git', 'cat-file', 'blob', oid], cwd=root)
                if hashlib.sha256(blob).hexdigest() != PUBLIC_BRANDING[name]:
                    failures.append(revision[:8] + ': unapproved branding content ' + name)
                count += 1
                continue
            allowed = name in ROOT_FILES or (path.parts[0] in EXTENSIONS and path.suffix in EXTENSIONS[path.parts[0]])
            if kind != 'blob' or mode not in ('100644', '100755') or not allowed:
                failures.append(revision[:8] + ': forbidden path/mode ' + name)
                continue
            blob = subprocess.check_output(['git', 'cat-file', 'blob', oid], cwd=root)
            if b'\0' in blob or blob.startswith(b'version https://git-lfs.github.com/spec/v1'):
                failures.append(revision[:8] + ': binary/LFS blob ' + name)
            count += 1
    if not revisions:
        failures.append('No committed history to verify')
    if failures:
        raise RuntimeError('\n'.join(failures))
    return len(revisions), count


if __name__ == '__main__':
    try:
        commits, blobs = verify(ROOT)
        print(f'PUBLIC_TREE_PASS: {commits} commits, {blobs} approved entries; private assets remain gitlinks only')
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
