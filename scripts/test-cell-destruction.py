#!/usr/bin/env python3
"""Run the 5cm cell prototype checks: automation tests and the real-game lab probe.

Every destruction map runs on 5cm cells; this script is kept as a focused entry point.
No packaging or publication; logs and captures stay in Saved/Testing.
"""
from pathlib import Path
import argparse
import os
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game', action='store_true', help='Run the real pawn/weapon/grenade probe in the saved lab instead of automation')
parser.add_argument('--render', action='store_true', help='Use Metal and capture breach screenshots (implies --game)')
args = parser.parse_args()
project = Path(__file__).resolve().parents[1]
engine = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8'))
editor = engine / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs = project / 'Saved/Testing'
logs.mkdir(parents=True, exist_ok=True)
flags = ['-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput']
if not args.render:
    flags.append('-nullrhi')
game = args.game or args.render
log = logs / ('cells-' + ('gallery' if args.render else 'game' if game else 'automation') + '.log')
if game:
    command = [str(editor), str(project / 'RespawnAndRage.uproject'), '/Game/EclipseRecoil/Maps/L_DestructionLab?Primary=0',
               '-game', '-ERCellLabProbe', '-windowed', '-ResX=1280', '-ResY=720',
               '-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1'] + flags
else:
    command = [str(editor), str(project / 'RespawnAndRage.uproject'), '/Engine/Maps/Entry',
               '-ExecCmds=Automation RunTests EclipseRecoil.Destruction.Cells', '-TestExit=Automation Test Queue Empty',
               '-ReportExportPath=' + str(logs / 'cells-report')] + flags
with log.open('w') as output:
    try:
        result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=300)
    except subprocess.TimeoutExpired:
        raise SystemExit('FAIL: Unreal timed out; inspect ' + str(log))
output = log.read_text(errors='replace')
for line in output.splitlines():
    if any(key in line for key in ('ER_CELL_', 'ER_BLOCK_GRENADE', 'Test Completed.', 'LogAutomationController: Error:')):
        print(line)
if game:
    passed = 'ER_CELL_PROBE_COMPLETE result=PASS' in output and 'ER_CELL_PROBE_FAIL' not in output
else:
    passed = all('Result={Success} Name={' + name + '}' in output for name in ['StorageAndCalibration', 'MeshAndCollision', 'BallisticsAndBlast', 'NetworkTransactions'])
    passed = passed and 'Result={Fail}' not in output
print('Log:', log)
raise SystemExit(0 if result.returncode == 0 and passed else 1)
