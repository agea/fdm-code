#!/usr/bin/env python3
"""Actual Dust door, underpass, stair and bridge traversal; optional LAN/Metal."""
import argparse, json, os, re, subprocess, time
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--render', action='store_true')
p.add_argument('--network', action='store_true')
args = p.parse_args()
root = Path(__file__).resolve().parents[1]
out = root / 'Saved/Testing'
out.mkdir(parents=True, exist_ok=True)
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
label = 'dust' + ('-network' if args.network else '-local') + ('-metal' if args.render else '')
children, handles, paths = [], [], {}

def launch(role, url, render=False, extra=()):
    paths[role] = out / (label + '-' + role + '.log')
    handle = paths[role].open('w'); handles.append(handle)
    command = [str(exe), str(root / 'RespawnAndRage.uproject'), url, '-game', '-ERDustTemplateProbe',
               '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout',
               '-FullStdOutLogOutput', '-windowed', '-ResX=1280', '-ResY=720',
               '-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1']
    if not render: command.append('-nullrhi')
    child = subprocess.Popen(command + list(extra), cwd=root, stdout=handle, stderr=subprocess.STDOUT)
    children.append(child)
    return child

def read(role): return paths[role].read_text(errors='replace')

def wait(marker):
    end = time.monotonic() + 180
    while time.monotonic() < end:
        if marker in read('host'): return
        if children[0].poll() is not None: raise RuntimeError('Host stopped before ' + marker)
        time.sleep(.3)
    raise RuntimeError('Timed out before ' + marker)

try:
    url = '/Game/EclipseRecoil/Maps/L_DustMirror?Name=Host?Primary=0?MapSeed=42'
    if args.network: url += '?listen'
    host = launch('host', url, args.render, ['-port=17793', '-MULTIHOME=127.0.0.1'])
    if args.network:
        wait('ER_DUST_STATE phase=1 ')
        launch('initial', '127.0.0.1:17793?Name=Initial?Primary=0')
        wait('ER_DUST_STATE phase=6 ')
        launch('late', '127.0.0.1:17793?Name=Late?Primary=0')
    host.wait(timeout=220)
    text = read('host')
    good = host.returncode == 0 and 'ER_DUST_COMPLETE result=PASS' in text and 'ER_DUST_FAIL' not in text
    states = {int(a): int(b) for a, b in re.findall(r'ER_DUST_STATE phase=(\d+) digest=(\d+)', text)}
    if args.network:
        for role, phases in [('initial', [1, 6, 7]), ('late', [6, 7])]:
            client = read(role)
            for phase in phases:
                matched = f'ER_DUST_CLIENT_PASS phase={phase} digest={states.get(phase)} ' in client
                print(role, phase, 'PASS' if matched else 'FAIL'); good &= matched
            good &= 'ER_DUST_CLIENT_FAIL' not in client and 'ER_BLOCK_NET_INCOMPATIBLE' not in client
    for line in text.splitlines():
        if 'ER_DUST_' in line: print(line)
    report = dict(result='PASS' if good else 'FAIL', network=args.network, render=args.render,
                  states=states, logs={role: str(path) for role, path in paths.items()})
    (out / (label + '-report.json')).write_text(json.dumps(report, indent=2) + '\n')
    raise SystemExit(0 if good else 1)
except (RuntimeError, subprocess.TimeoutExpired) as error:
    print('FAIL', error)
    print('\n'.join(read('host').splitlines()[-20:])); raise SystemExit(1)
finally:
    for child in children:
        if child.poll() is None: child.terminate()
    for child in children:
        try: child.wait(timeout=10)
        except subprocess.TimeoutExpired: child.kill(); child.wait()
    for handle in handles: handle.close()
