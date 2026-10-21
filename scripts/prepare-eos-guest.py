#!/usr/bin/env python3
"""Prepare a private project override of UE 5.8.3's EOS plugin for Device ID.

The installed engine is read-only. Epic's source is copied locally, ignored by
Git; only our extension snippets and this reproducible preparation are tracked.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path


def prepare(project, engine):
    project, engine = Path(project).resolve(), Path(engine).resolve()
    version = json.loads((engine / 'Engine/Build/Build.version').read_text())
    if tuple(version[k] for k in ('MajorVersion', 'MinorVersion', 'PatchVersion')) != (5, 8, 3):
        raise RuntimeError('EOS guest extension requires UE 5.8.3; review against new engine sources before upgrading.')
    source = engine / 'Engine/Plugins/Online/OnlineSubsystemEOS'
    dest = project / 'Plugins/OnlineSubsystemEOS'
    snippets = Path(__file__).with_name('eos-guest')
    module = Path('Source/OnlineSubsystemEOS')
    expected = {
        'UserManagerEOS.cpp': '44e165f403aade610979d855feb6f6acab86777c783d3ebfb858e2dd51bdcf37',
        'UserManagerEOS.h': '6449236d3564759d5dc76caa8738d0ed6d138ed9365dbdfb591dc7b716237c71',
    }
    # Generate/validate everything before writing; never silently patch new code.
    files = {p.relative_to(source): p.read_bytes() for p in (source / 'Source').rglob('*') if p.is_file()}
    files[Path('OnlineSubsystemEOS.uplugin')] = (source / 'OnlineSubsystemEOS.uplugin').read_bytes()
    for name, digest in expected.items():
        path = module / 'Private' / name
        raw = files[path].decode('utf-8-sig').replace('\r\n', '\n').encode()
        if hashlib.sha256(raw).hexdigest() != digest:
            raise RuntimeError('Unreviewed EOS source: ' + name)
        files[path] = raw
    cpp_path, header_path = module / 'Private/UserManagerEOS.cpp', module / 'Private/UserManagerEOS.h'
    cpp, header = files[cpp_path].decode(), files[header_path].decode()

    def once(text, needle, replacement):
        if text.count(needle) != 1:
            raise RuntimeError('EOS source anchor changed: ' + needle[:70])
        return text.replace(needle, replacement, 1)

    header = once(header, 'struct FLocalUserEOS\n{', 'struct FLocalUserEOS\n{\n\tFString RRDeviceIdDisplayName;')
    cpp = once(cpp, 'if(!Settings.bUseEAS && Settings.bUseEOSConnect)\n\t{',
               'if(!Settings.bUseEAS && Settings.bUseEOSConnect)\n\t{\n#include "RRDeviceIdLogin.inl"')
    cpp = once(cpp, '\n\tFConnectLoginCallback* CallbackObj = new FConnectLoginCallback(AsWeak());\n\tCallbackObj->CallbackLambda = [this, LocalUserNum](const EOS_Connect_LoginCallbackInfo* Data)',
               '\n#include "RRDeviceIdUserInfo.inl"\n\tFConnectLoginCallback* CallbackObj = new FConnectLoginCallback(AsWeak());\n\tCallbackObj->CallbackLambda = [this, LocalUserNum](const EOS_Connect_LoginCallbackInfo* Data)')
    cpp = once(cpp, '\tif (IsLocalUserValid(LocalUserNum))\n\t{\n\t\tconst FEOSSettings Settings = UEOSSettings::GetSettings();',
               '\tif (IsLocalUserValid(LocalUserNum))\n\t{\n#include "RRDeviceIdRefresh.inl"\n\t\tconst FEOSSettings Settings = UEOSSettings::GetSettings();')
    end = cpp.rfind('#endif')
    cpp = cpp[:end] + '#include "RRGuestProbe.inl"\n\n' + cpp[end:]
    files[cpp_path], files[header_path] = cpp.encode(), header.encode()
    rules = module / 'OnlineSubsystemEOS.Build.cs'
    files[rules] = once(files[rules].decode(), 'PrivateDefinitions.Add("ONLINESUBSYSTEMEOS_PACKAGE=1");',
                       'PrivateDefinitions.Add("ONLINESUBSYSTEMEOS_PACKAGE=1");\n\t\tPublicDefinitions.Add("RR_EOS_DEVICE_ID=1");').encode()
    for snippet in snippets.iterdir():
        files[module / ('Public' if snippet.suffix == '.h' else 'Private') / snippet.name] = snippet.read_bytes()
    receipt = {str(path): hashlib.sha256(data).hexdigest() for path, data in files.items()}
    marker = dest / '.rr-guest-generated.json'
    if dest.exists() and not marker.exists():
        raise RuntimeError('Refusing to overwrite an unmanaged project EOS plugin: ' + str(dest))
    changed = 0
    for path, data in files.items():
        target = dest / path
        if not target.is_file() or target.read_bytes() != data:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            changed += 1
    # Remove only obsolete generated sources, preserving compiled outputs.
    previous = json.loads(marker.read_text()) if marker.exists() else {}
    for name in previous.keys() - receipt.keys():
        path = dest / name
        if not path.resolve().is_relative_to(dest):
            raise RuntimeError('Invalid generation receipt')
        path.unlink(missing_ok=True)
    marker.write_text(json.dumps(receipt, indent=2) + '\n')
    return changed


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--engine', type=Path, default=Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')))
    args = parser.parse_args()
    print('EOS Device ID plugin prepared; files updated:', prepare(args.project, args.engine))
