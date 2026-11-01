#!/usr/bin/env python3
"""Rooms: native layout/physics, real pawn ramps, Metal gallery and LAN snapshots."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time
from client_shutdown import settle_clients

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--game', action='store_true', help='Walk both ramp types, damage cover and regenerate in game')
p.add_argument('--render', action='store_true', help='Real Metal game and 18 captures across three seeds')
p.add_argument('--network', action='store_true', help='Initial and late LAN clients across damage and regeneration')
p.add_argument('--offscreen', action='store_true', help='Render Metal without showing a game window')
p.add_argument('--seed', type=int, help='Map seed; the default suits the chosen plan')
p.add_argument('--macro', action='store_true', help='Use the baked PCGEx macro graph: authority flag, clients follow the descriptor')
p.add_argument('--selection', action='store_true', help='Authority picks a map candidate, clients rebuild it from the descriptor')
p.add_argument('--gallery-only', action='store_true', help='Skip gameplay phases for lighting and camera iterations; requires rendering')
args = p.parse_args()
args.render |= args.offscreen
if args.gallery_only and (not args.render or args.network):
    p.error('--gallery-only requires --render or --offscreen and cannot be combined with --network')
root = Path(__file__).resolve().parents[1]
out = root/'Saved/Testing'
out.mkdir(parents=True, exist_ok=True)
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
common = [str(exe), str(root/'RespawnAndRage.uproject'), '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput']
if not (args.game or args.render or args.network):
    log = out/'rooms-automation.log'
    with log.open('w') as f:
        result = subprocess.run(common+['/Engine/Maps/Entry', '-nullrhi', '-ExecCmds=Automation RunTests EclipseRecoil.Destruction.Rooms', '-TestExit=Automation Test Queue Empty'], stdout=f, stderr=subprocess.STDOUT, timeout=600)
    text = log.read_text(errors='replace')
    passed = result.returncode == 0 and all('Result={Success} Name={'+name+'}' in text for name in ['RoomsLayout', 'RoomsPhysics']) and 'Result={Fail}' not in text
    print('\n'.join(line for line in text.splitlines() if 'ER_ROOMS_' in line or 'Result={' in line or 'Error:' in line))
    print('Log:', log)
    raise SystemExit(0 if passed else 1)

label = 'rooms'+('-macro' if args.macro else '')+('-selection' if args.selection else '')+('-network' if args.network else '-game')+('-metal' if args.render else '')
children, handles, paths = [], [], {}
def launch(role, url, render=False, extra=()):
    paths[role] = out/(label+'-'+role+'.log')
    f = paths[role].open('w')
    handles.append(f)
    command = common+[url, '-game', '-windowed', '-ResX=1600', '-ResY=900', '-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1']+list(extra)
    if not render:
        command.append('-nullrhi')
    elif args.offscreen:
        command.append('-RenderOffscreen')
    proc = subprocess.Popen(command, cwd=root, stdout=f, stderr=subprocess.STDOUT)
    children.append(proc)
    return proc
def read(role):
    return paths[role].read_text(errors='replace')
def wait(marker):
    deadline = time.monotonic()+120
    while marker not in read('host'):
        if children[0].poll() is not None or time.monotonic() > deadline:
            raise RuntimeError('Host stopped or timed out before '+marker)
        time.sleep(.2)

try:
    started_at = time.time()
    url = '/Game/EclipseRecoil/Maps/L_Rooms?Name=Host?MapSeed='+str(args.seed if args.seed is not None else 3 if args.macro else 42)+('?RoomsMacroGraph=1' if args.macro else '')+('?MapSelection=1' if args.selection else '')+'?ERAlphaBots=0?EROmegaBots=0'+('?listen' if args.network else '')
    host = launch('host', url, args.render, ['-ERRoomsProbe', '-port=17959']+(['-ERRoomsGalleryOnly'] if args.gallery_only else []))
    if args.network:
        wait('ER_ROOMS_STATE phase=1 ')
        launch('initial', '127.0.0.1:17959?Name=Initial')
        wait('ER_ROOMS_STATE phase=2 ')
        launch('late', '127.0.0.1:17959?Name=Late')
    host.wait(timeout=240)
    text = read('host')
    passed = host.returncode == 0 and 'ER_ROOMS_COMPLETE result=PASS' in text and '=FAIL' not in text
    states = {int(a): int(b) for a, b in re.findall(r'ER_ROOMS_STATE phase=(\d+) digest=(\d+)', text)}
    if args.network:
        for role, phases in [('initial', [1, 2, 3]), ('late', [2, 3])]:
            client = read(role)
            for phase in phases:
                good = f'ER_ROOMS_CLIENT_PASS phase={phase} digest={states.get(phase)}' in client
                print(role, phase, 'PASS' if good else 'FAIL')
                passed &= good
            passed &= 'ER_ROOMS_CLIENT_FAIL' not in client and 'ER_BLOCK_NET_INCOMPATIBLE' not in client
            if args.selection:
                chosen = re.search(r'ER_MAP_SELECTION seed=\S+ template=\S+ chosen=(\d+)', text)
                good = chosen is not None and f'candidate={chosen.group(1)}' in client
                print(role, 'map candidate', chosen.group(1) if chosen else 'missing', 'PASS' if good else 'FAIL')
                passed &= good
            if args.macro:
                good = 'macro=-1' not in client and 'macro=' in client and 'macro=-1' not in text
                print(role, 'macro plan', 'PASS' if good else 'FAIL')
                passed &= good
    if args.macro and args.seed in (None, 3):
        for walk in ('corridor_ramp', 'upper_switchback', 'upper_corridor_portal'):
            good = f'pawn_walks_{walk}=PASS' in text
            print('walk', walk, 'PASS' if good else 'FAIL')
            passed &= good
    for line in text.splitlines():
        if 'ER_ROOMS_' in line:
            print(line)
    captures = sorted(str(x) for x in out.glob('Rooms-*.png') if x.stat().st_mtime >= started_at-1) if args.render else []
    if args.render:
        passed &= len(captures) == 18
    (out/(label+'-report.json')).write_text(json.dumps(dict(result='PASS' if passed else 'FAIL', gallery_only=args.gallery_only, states=states, captures=captures, logs={k: str(v) for k, v in paths.items()}), indent=2)+'\n')
    print('Logs:', *(str(v) for v in paths.values()))
    raise SystemExit(0 if passed else 1)
finally:
    settle_clients(children, handles)
    for process in children:
        if process.poll() is None:
            process.terminate()
    for process in children:
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    for f in handles:
        f.close()
