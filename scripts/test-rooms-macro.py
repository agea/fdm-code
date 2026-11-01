#!/usr/bin/env python3
"""Rooms macro graph: baked table against the real graph, then legacy against prototype."""
import argparse
import csv
import os
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--teams', default='2,4,8', help='Expected team sizes')
p.add_argument('--seeds', default=','.join(str(s) for s in range(20)), help='Physical comparison seeds')
args = p.parse_args()
root = Path(__file__).resolve().parents[1]
out = root/'Saved/Testing'
out.mkdir(parents=True, exist_ok=True)
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
common = [str(exe), str(root/'RespawnAndRage.uproject')]
flags = ['-nullrhi', '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput']


def run(label, *command):
    log = out/f'rooms-macro-{label}.log'
    with log.open('w') as f:
        result = subprocess.run([*common, *command, *flags], stdout=f, stderr=subprocess.STDOUT, cwd=root)
    return result.returncode, log.read_text(errors='replace'), log


code, text, log = run('bake', '-run=pythonscript', f'-script={root/"scripts/verify_rooms_macro_graph.py"}')
bake = code == 0 and 'ER_ROOMS_MACRO_VERIFIED' in text
print('bake reproduced by the real graph:', bake, log)
code, text, log = run('native', '/Engine/Maps/Entry', '-ExecCmds=Automation RunTests EclipseRecoil.Destruction.Rooms', '-TestExit=Automation Test Queue Empty',
    f'-ERPlayabilityTeams={args.teams}', f'-ERPlayabilitySeeds={args.seeds}')
print('\n'.join(line[line.index('Test Completed'):] for line in text.splitlines() if 'Test Completed' in line and 'LogInit' not in line))
print('\n'.join(line[line.index('ER_ROOMS_MACRO_LAYOUT'):] for line in text.splitlines() if 'ER_ROOMS_MACRO_LAYOUT' in line and 'LogInit' not in line and '[log]' not in line))
native = code == 0 and 'Result={Success} Name={RoomsMacroPhysics}' in text and 'Result={Success} Name={RoomsMacroLayout}' in text and 'Result={Fail}' not in text
rows = list(csv.DictReader((out/'MapPlayability/rooms-macro-comparison.csv').open()))
legacy = {(r['team'], r['seed']): r for r in rows if r['graph'] == 'legacy'}
macro = {(r['team'], r['seed']): r for r in rows if r['graph'] == 'macro'}
for team in sorted({k[0] for k in macro}, key=int):
    keys = [k for k in macro if k[0] == team]
    used = [k for k in keys if macro[k]['variant'] != '-1']
    exposed = lambda side, group: sum(float(side[k]['open_shot_pairs']) > 0 for k in group)
    mean = lambda side, group, field: sum(float(side[k][field]) for k in group)/max(1, len(group))
    print(f'{team}v{team}: macro plan on {len(used)}/{len(keys)} seeds; exposed starts legacy {exposed(legacy, keys)}/{len(keys)}, prototype {exposed(macro, keys)}/{len(keys)};'
          f' on those seeds walk {mean(legacy, used, "base_to_base_m"):.0f}m to {mean(macro, used, "base_to_base_m"):.0f}m, plans {len({macro[k]["variant"] for k in used})}')
print('Log:', log)
raise SystemExit(0 if bake and native else 1)
