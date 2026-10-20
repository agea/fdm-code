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
              'scripts': {'.py', '.command', '.sh', '.ps1'}, 'docs': {'.md', '.txt', '.json', '.csv'}}
SUBMODULES = {'Assets/characters', 'Assets/weapons', 'Assets/maps'}
# User-supplied game branding and its prepared derivatives, 16 October 2026.
# Pin both path and bytes: this is not permission for arbitrary binary assets.
PUBLIC_BRANDING = {
    # Derived from the existing user-provided icon; Retina packaging, 18 October.
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/Contents.json': '13b1c7461192863c9f3ab3ceab313acd6f6531898360983495bf13d48dfc967f',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_128x128.png': 'bbf54f2e642f8e09ed47c56094f774faf00fe21aec5c33c80b8c722852682563',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_128x128@2x.png': '0df29e9caa13b07a55787e18d8072476c14f30bb92459fa942dfc04000584a6a',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_16x16.png': '7834f7ca94e62b732a280b396f6154c7e588fa2c1a9451b732ed2802ca13a38d',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_16x16@2x.png': '43a582b51dafd11441c5f9f35e22c593bedd8c8d14b7be65190b23a9e7bd22aa',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_256x256.png': '0df29e9caa13b07a55787e18d8072476c14f30bb92459fa942dfc04000584a6a',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_256x256@2x.png': 'a13f7be156e1e0c17a892b50926a3c2a16061f239ef4d9dc45936941c0699924',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_32x32.png': '43a582b51dafd11441c5f9f35e22c593bedd8c8d14b7be65190b23a9e7bd22aa',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_32x32@2x.png': 'd109f87fc24b9cb08edd3412e31773e94f763f78b70fb118d2e721c373ea8831',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_512x512.png': 'a13f7be156e1e0c17a892b50926a3c2a16061f239ef4d9dc45936941c0699924',
    'Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/icon_512x512@2x.png': '10320d04e40b2685e42e012586f23189a729a9dc34c425fd590492d58c2126d5',
    'Build/Mac/Resources/Assets.xcassets/Contents.json': 'f5fb0e1f3a8142892ed092efb5955c1c2cc537003a89ad7bbdb9dade7fe8985f',
    'Build/Mac/Resources/Info.Template.plist': '62d6341c5d79a25e70b46413ecffeed4b4aa457869129adbaaeda01c43f4798a',
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
