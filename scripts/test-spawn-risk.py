#!/usr/bin/env python3
"""Spawn scoring: native scenarios, then bot matches read through the -ERSpawnAudit log."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--seconds', type=int, default=120, help='Length of each bot match')
p.add_argument('--maps', default='L_CentralBuilding,L_Rooms,L_Village', help='Levels, comma separated')
p.add_argument('--seed', type=int, default=3)
args = p.parse_args()
root = Path(__file__).resolve().parents[1]
out = root/'Saved/Testing'
out.mkdir(parents=True, exist_ok=True)
exe = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
common = [str(exe), str(root/'RespawnAndRage.uproject'), '-nullrhi', '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput']
log = out/'spawn-risk-native.log'
with log.open('w') as f:
    result = subprocess.run(common+['/Engine/Maps/Entry', '-ExecCmds=Automation RunTests EclipseRecoil.Maps.SpawnRisk', '-TestExit=Automation Test Queue Empty'], stdout=f, stderr=subprocess.STDOUT, cwd=root)
text = log.read_text(errors='replace')
good = result.returncode == 0 and 'Result={Success} Name={Choice}' in text and 'Result={Success} Name={Physics}' in text and 'Result={Fail}' not in text
print('native scenarios:', 'PASS' if good else 'FAIL', log)
choice = re.compile(r'ER_SPAWN_CHOICE team=(\d) candidates=(\d+) exposed=(\d+) enemies=(\d+) traces=(\d+) chosen_seen=(\d+) risk=(\d+)')
for level in args.maps.split(','):
    log = out/f'spawn-risk-{level}.log'
    with log.open('w') as f:
        match = subprocess.Popen(common+[f'/Game/EclipseRecoil/Maps/{level}?listen?Name=Host?MapSeed={args.seed}?ERBots=3', '-game', '-ERSpawnAudit', '-port=17961'], stdout=f, stderr=subprocess.STDOUT, cwd=root)
        time.sleep(args.seconds)
        match.terminate()
        match.wait(timeout=30)
    lines = log.read_text(errors='replace')
    rows = [tuple(map(int, row)) for row in choice.findall(lines) ]
    if not rows:
        print(level, 'no scored respawn')
        good = False
        continue
    risky = [r for r in rows if r[2] > 0]
    random_rate = sum(r[2]/r[1] for r in rows)/len(rows)
    chosen_rate = sum(r[5] > 0 for r in rows)/len(rows)
    print(f'{level}: {len(rows)} respawns, {len(risky)} with an exposed start; exposed choice random {random_rate:.1%}, scored {chosen_rate:.1%}; worst {max(r[4] for r in rows)} traces')
    good &= max(r[4] for r in rows) <= 96 and 'Fatal' not in lines
raise SystemExit(0 if good else 1)
