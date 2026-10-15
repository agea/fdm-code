#!/usr/bin/env python3
"""Run block storage/real Chaos collision tests and the optional standalone probe.

No packaging or publication. Engine writes normal local cache/log files.
"""
from pathlib import Path
import argparse
import os
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--procedural', action='store_true', help='Test seeded map generation and real pawn/combat (implies --game)')
parser.add_argument('--game', action='store_true', help='Test real pawn, console actions and authorized firing in the saved lab')
parser.add_argument('--render', action='store_true', help='Use Metal and capture lab screenshots (implies --game)')
parser.add_argument('--combat', action='store_true', help='Exercise penetration, block explosion and actual HE fuse in the saved lab (implies --game)')
parser.add_argument('--collapse', action='store_true', help='Test structural support and actual Chaos debris (implies --game)')
args = parser.parse_args()
if args.collapse and (args.combat or args.procedural):
    parser.error("--collapse uses its own destruction-lab probe; combine only with --render")
project = Path(__file__).resolve().parents[1]
engine = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8'))
editor = engine / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs = project / 'Saved/Testing'
logs.mkdir(parents=True, exist_ok=True)
flags = ['-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput']
if not args.render:
    flags.append('-nullrhi')
game = args.game or args.render or args.combat or args.procedural or args.collapse
label = 'destruction-gallery' if args.render else 'destruction-game' if game else 'destruction-automation'
if args.combat: label += '-combat'
if args.procedural: label += '-procedural'
if args.collapse: label += '-collapse'
log = logs / (label + '.log')
if game:
    command = [str(editor), str(project / 'fdm.uproject'), '/Game/EclipseRecoil/Maps/' + ('L_ProceduralLab' if args.procedural else 'L_DestructionLab') + '?Primary=0',
               '-game', '-ERBlockCollapseProbe' if args.collapse else '-ERProceduralProbe' if args.procedural else '-ERBlockLabProbe', '-windowed', '-ResX=1280', '-ResY=720', '-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1'] + flags
    if args.combat: command.append('-ERBlockCombatProbe')
else:
    command = [str(editor), str(project / 'fdm.uproject'), '/Engine/Maps/Entry',
               '-ExecCmds=Automation RunTests EclipseRecoil.Destruction', '-TestExit=Automation Test Queue Empty',
               '-ReportExportPath=' + str(logs / 'destruction-report')] + flags
with log.open('w') as output:
    try:
        result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=180)
    except subprocess.TimeoutExpired:
        raise SystemExit('FAIL: Unreal timed out; inspect ' + str(log))
output = log.read_text(errors='replace')
for line in output.splitlines():
    if 'ER_COLLAPSE_' in line or 'ER_BLOCK_SUPPORT' in line or 'ER_PROCEDURAL_PROBE_' in line or 'ER_BLOCK_PROBE_' in line or 'Test Completed.' in line or 'LogAutomationController: Error:' in line:
        print(line)
if game:
    prefix = 'ER_COLLAPSE_PROBE_' if args.collapse else 'ER_PROCEDURAL_PROBE_' if args.procedural else 'ER_BLOCK_PROBE_'
    passed = prefix + 'COMPLETE result=PASS' in output and prefix + 'FAIL' not in output
else:
    passed = all('Result={Success} Name={' + name + '}' in output for name in ['Storage', 'CollisionAndIdentity', 'BallisticsAndExplosion', 'ProceduralLayout', 'ProceduralCollision', 'StressAndBatchedIdentity', 'StructuralSupport', 'CollapseAndIdentity', 'MultiFloorLayout', 'MultiFloorCollision', 'NetworkTransactions', 'MapTemplate', 'DustTemplate'])
    passed = passed and 'Result={Fail}' not in output
print('Log:', log)
raise SystemExit(0 if result.returncode == 0 and passed else 1)
