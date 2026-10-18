"""Package an isolated project copy on macOS using the existing UE Installed Build.
Default: universal Intel + Apple Silicon, Development. No publication/signing identity.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import plistlib
import shutil
import signal
import subprocess
import time

PROJECT_ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--project', type=Path, default=PROJECT_ROOT)
p.add_argument('--engine', type=Path, default=Path('/Users/Shared/Epic Games/UE_5.8'))
p.add_argument('--workspace', type=Path, default=PROJECT_ROOT.parent / 'builds' / 'macos')
p.add_argument('--architecture', choices=['universal', 'arm64', 'x64'], default='universal')
p.add_argument('--configuration', choices=['Development', 'Shipping'], default='Development')
p.add_argument('--resume', type=Path, help='Reuse an existing isolated build directory')
p.add_argument('--xcode-stdin-workaround', action='store_true', help='Finalize outside UBA for the UE5.8.3/Xcode27 closed-stdin bug')
p.add_argument('--check', action='store_true', help='Environment/space check with a temporary APFS clone probe')
a = p.parse_args()
project, engine, workspace = a.project.resolve(), a.engine.resolve(), a.workspace.resolve()
if platform.system() != 'Darwin':
    raise RuntimeError('Run on macOS with Xcode and UE 5.8.3 installed')
if workspace.is_relative_to(project) or project.is_relative_to(workspace):
    raise ValueError('Use a workspace outside the original project')
if not (project / 'RespawnAndRage.uproject').is_file():
    raise ValueError('RespawnAndRage.uproject missing')
v = json.loads((engine / 'Engine/Build/Build.version').read_text())
if tuple(v[k] for k in ('MajorVersion', 'MinorVersion', 'PatchVersion')) != (5, 8, 3):
    raise ValueError('UE 5.8.3 required')
xcode_version = subprocess.check_output(['xcodebuild', '-version'], text=True)
print(xcode_version, end='')
if platform.mac_ver()[0].split('.')[0] == '27' and xcode_version.startswith('Xcode 27.'):
    a.xcode_stdin_workaround = True
subprocess.run(['xcrun', '--find', 'clang'], check=True)
nearest = workspace
while not nearest.exists():
    nearest = nearest.parent
free = shutil.disk_usage(nearest).free
from apfs_snapshot import clone_supported, copy_file
use_clones = clone_supported(project / 'RespawnAndRage.uproject', nearest)
# The earlier 40 GiB included a complete second content tree. APFS clones
# preserve isolation without immediately allocating another copy of every asset.
minimum_gib = (25 if a.architecture == 'universal' else 20) if use_clones else 40
if a.resume:
    minimum_gib = 8  # Existing snapshot and compiled objects are reused.
minimum = minimum_gib * 1024**3
print(json.dumps({'engine': str(engine), 'workspace': str(workspace),
                  'architecture': a.architecture, 'xcode_stdin_workaround': a.xcode_stdin_workaround, 'free_gib': round(free / 1024**3, 1),
                  'minimum_free_gib': minimum_gib, 'apfs_clones': use_clones, 'space_ok': free >= minimum}), flush=True)
if a.check:
    raise SystemExit(0 if free >= minimum else 2)
if free < minimum:
    raise RuntimeError(f'Need at least {minimum_gib} GiB free before starting; actual cook may need more')
if a.resume:
    run_dir = a.resume.resolve()
    if not run_dir.is_relative_to(workspace) or not (run_dir / 'receipt.json').is_file():
        raise ValueError('Resume only a previous build inside this workspace')
    snapshot = run_dir / 'project'
    previous = json.loads((run_dir / 'receipt.json').read_text())
    if previous.get('scratch_removed'):
        raise ValueError('This successful build was cleaned to recover disk space; start a fresh build instead of resuming')
    expected_arches = 'arm64+x64' if a.architecture == 'universal' else a.architecture
    if previous['architecture'] != expected_arches or not (snapshot / 'RespawnAndRage.uproject').is_file():
        raise ValueError('Resume architecture or project mismatch')
else:
    run_dir = workspace / time.strftime('%Y%m%d-%H%M%S')
    snapshot = run_dir / 'project'
    snapshot.mkdir(parents=True)
    # Follow the private asset mounts while excluding unrelated source archives,
    # repository data, previous binaries and caches. Never build in the live checkout.
    for name in ('RespawnAndRage.uproject', 'Config', 'Content', 'Source', 'Build', 'Plugins'):
        src, dst = project / name, snapshot / name
        if src.is_dir():
            shutil.copytree(src, dst, symlinks=False, copy_function=copy_file if use_clones else shutil.copy2, ignore=shutil.ignore_patterns('.git', '__pycache__', '.DS_Store'))
        elif src.is_file():
            (copy_file if use_clones else shutil.copy2)(src, dst)
    branding = snapshot / 'Branding'
    branding.mkdir(exist_ok=True)
    for name in ('background.jpg', 'icon.png'):
        (copy_file if use_clones else shutil.copy2)(project / 'Branding' / name, branding / name)
uproject = snapshot / 'RespawnAndRage.uproject'
archive = run_dir / 'dist'
logs = run_dir / 'build.log'
arches = 'arm64+x64' if a.architecture == 'universal' else a.architecture
host = 'arm64' if platform.machine() == 'arm64' else 'x64'
commands = [
    [str(engine / 'Engine/Build/BatchFiles/Mac/Build.sh'), 'RespawnAndRageEditor', 'Mac',
     'Development', f'-Project={uproject}', f'-architecture={host}', '-NoHotReloadFromIDE'],
    [str(engine / 'Engine/Build/BatchFiles/RunUAT.sh'), 'BuildCookRun', f'-project={uproject}',
     '-target=RespawnAndRage', '-platform=Mac', f'-clientconfig={a.configuration}',
     f'-clientarchitecture={arches}', '-nop4', '-unattended', '-utf8output', '-installed',
     '-allmaps', '-build', '-cook', '-stage', '-package', '-pak', '-iostore',
     '-skipbuildeditor', '-archive', f'-archivedirectory={archive}'],
]
if a.resume:
    commands = commands[1:]  # The Editor module already compiled in this snapshot.
if a.xcode_stdin_workaround:
    # Build under Xcode's documented environment (defers finalization), then run
    # the real finalization outside UBA with a valid stdin. No engine patch.
    game_build = ['env', 'UE_BUILD_FROM_XCODE=1',
                  str(engine / 'Engine/Build/BatchFiles/Mac/Build.sh'),
                  'RespawnAndRage', 'Mac', a.configuration, f'-Project={uproject}',
                  f'-architecture={arches}', '-skipdeploy']
    sync_input = snapshot / f'Intermediate/Build/Mac/{arches}/RespawnAndRage/{a.configuration}/PostBuildSync_RespawnAndRage.json'
    finalize = [str(engine / 'Engine/Build/BatchFiles/RunUBT.sh'), '-Mode=ApplePostBuildSync',
                f'-Input={sync_input}', f'-XmlConfigCache={snapshot / "Intermediate/Build/XmlConfigCache.bin"}',
                f'-remoteini={snapshot}']
    commands[-1] = ['-skipbuild' if x == '-build' else x for x in commands[-1]]
    commands[-1:-1] = [game_build, finalize]
receipt = {'project_head': subprocess.check_output(['git','-C',str(project),'rev-parse','HEAD'],text=True).strip(),
           'engine_version': '5.8.3', 'architecture': arches,
           'xcode_stdin_workaround': a.xcode_stdin_workaround, 'apfs_clones': use_clones, 'project_snapshot': str(snapshot), 'archive': str(archive), 'commands': commands,
           'status': 'running', 'distribution_signing': 'Not configured'}
receipt_path = run_dir / 'receipt.json'
receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')
try:
    with logs.open('a' if a.resume else 'w') as log:
        for command in commands:
            process = subprocess.Popen(command, cwd=snapshot, stdout=log,
                                       stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, start_new_session=True)
            try:
                while process.poll() is None:
                    if shutil.disk_usage(run_dir).free < 5 * 1024**3:
                        raise RuntimeError('Build stopped to preserve 5 GiB free disk space')
                    time.sleep(5)
                if process.returncode:
                    raise subprocess.CalledProcessError(process.returncode, command)
            finally:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGTERM)
                    try:
                        process.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()
    apps = [app for app in archive.rglob('*.app') if app.name.startswith('RespawnAndRage')]
    if not apps:
        raise RuntimeError('Packaged RespawnAndRage.app missing')
    for app in apps:
        with (app / 'Contents/Info.plist').open('rb') as info:
            bundle_info = plistlib.load(info)
            executable = bundle_info['CFBundleExecutable']
        if not bundle_info.get('NSHighResolutionCapable'):
            raise RuntimeError('Packaged app does not enable Retina; check project Info.Template.plist')
        if not (app / 'Contents/Resources/AppIcon.icns').is_file():
            raise RuntimeError('Packaged app icon is missing')
        binary = app / 'Contents/MacOS' / executable
        actual = subprocess.check_output(['lipo', '-archs', str(binary)], text=True).strip()
        for required in arches.split('+'):
            required = 'x86_64' if required == 'x64' else required
            if required not in actual.split():
                raise RuntimeError(f'Packaged binary missing {required}: {actual}')
        receipt['verified_binary_architectures'] = actual
    receipt['status'] = 'packaged'
except BaseException:
    receipt['status'] = 'failed'
    raise
finally:
    receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')
    print('Build receipt: ' + str(receipt_path), flush=True)
