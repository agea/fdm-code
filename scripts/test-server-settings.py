#!/usr/bin/env python3
"""Validate server tuning, real hostile-client replication and the settings menu."""
from pathlib import Path
import argparse
import os
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--network', action='store_true')
parser.add_argument('--regressions', action='store_true')
parser.add_argument('--render', action='store_true')
parser.add_argument('--width', type=int, default=1440)
parser.add_argument('--height', type=int, default=1000)
args = parser.parse_args()
project = Path(__file__).resolve().parents[1]
editor = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs = project / 'Saved/Testing'
logs.mkdir(parents=True, exist_ok=True)
common = [str(editor), str(project / 'RespawnAndRage.uproject'), '-nosound', '-unattended', '-NoSplash', '-Multiprocess', '-stdout', '-FullStdOutLogOutput']
if not args.network and not args.render:
    log = logs / ('server-settings-regressions.log' if args.regressions else 'server-settings-automation.log')
    filters = '+'.join('EclipseRecoil.' + v for v in ['ServerSettings', 'Loadout', 'Weapons.ReferenceBallistics', 'Menu', 'Destruction.Cells', 'Recon', 'Player']) if args.regressions else 'EclipseRecoil.ServerSettings'
    with log.open('w') as out:
        run = subprocess.run(common + ['/Engine/Maps/Entry', '-nullrhi', '-ExecCmds=Automation RunTests ' + filters, '-TestExit=Automation Test Queue Empty'], stdout=out, stderr=subprocess.STDOUT, timeout=180)
    text = log.read_text(errors='replace')
    print('\n'.join(line for line in text.splitlines() if 'Test Completed.' in line or 'LogAutomationController: Error:' in line))
    names = ['ValidationAndPersistence', 'WeaponAndKitOverrides', 'MaterialAndPredictionIsolation']
    raise SystemExit(0 if run.returncode == 0 and 'Result={Fail}' not in text and all('Result={Success} Name={' + name + '}' in text for name in names) else 1)
if args.render:
    log = logs / 'server-settings-render.log'
    before = time.time()
    with log.open('w') as out:
        run = subprocess.run(common + ['/Game/EclipseRecoil/Maps/L_MainMenu', '-game', '-windowed', f'-ResX={args.width}', f'-ResY={args.height}', '-ERServerRulesGallery', '-ExecCmds=t.MaxFPS 60'], stdout=out, stderr=subprocess.STDOUT, timeout=120)
    capture = logs / 'ServerRules-Menu.png'
    ok = run.returncode == 0 and all((logs / f'ServerRules-{kind}.png').exists() and (logs / f'ServerRules-{kind}.png').stat().st_mtime >= before for kind in ['Menu', 'Claymore', 'Weapon'])
    print('ER_SERVER_RULES_MENU_CAPTURE', 'PASS' if ok else 'FAIL', capture)
    raise SystemExit(0 if ok else 1)

settings = {
    'Move.Run': 21, 'Move.SprintDuration': 7, 'Move.Recovery': 11,
    'Health.Max': 150, 'Health.Start': 150, 'Health.Armor': 100,
    'Weapon.Assault.Damage': 37,
    'Kit.rifleman.offense.PrimaryLoaded': 20,
    'Kit.rifleman.offense.PrimaryReserve': 66,
    'Kit.rifleman.offense.HE': 5,
    'Smoke.Duration': 18, 'Smoke.Radius': 6,
    'Claymore.DamageRadius': 10, 'Claymore.TriggerPercent': 30,
    'Claymore.DetectionAngle': 40, 'Claymore.DamageAngle': 100,
    'Claymore.Damage': 20, 'Claymore.StructuralDamage': 0,
    'Structure.Health': 1.5,
}
processes = []
files = []
try:
    hostlog = logs / 'server-settings-host.log'
    clientlog = logs / 'server-settings-client.log'
    flags = [f'-ini:Game:[RespawnAndRage.ServerRules]:{key}={value}' for key, value in settings.items()]
    files.append(hostlog.open('w'))
    host = subprocess.Popen(common + ['/Game/EclipseRecoil/Maps/L_Depot?listen?Name=RulesHost?Team=0?Preset=rifleman?Variant=offense', '-game', '-nullrhi', '-port=17826', '-ERServerRulesProbe', '-ExecCmds=t.MaxFPS 60'] + flags, stdout=files[-1], stderr=subprocess.STDOUT)
    processes.append(host)
    deadline = time.monotonic() + 60
    while 'listening on port 17826' not in hostlog.read_text(errors='replace'):
        if host.poll() is not None or time.monotonic() > deadline:
            raise RuntimeError('Host did not listen: ' + str(hostlog))
        time.sleep(.2)
    files.append(clientlog.open('w'))
    client = subprocess.Popen(common + ['127.0.0.1:17826?Name=RulesClient?Team=1?Preset=rifleman?Variant=offense', '-game', '-nullrhi', '-ini:Game:[RespawnAndRage.ServerRules]:Move.Run=2', '-ini:Game:[RespawnAndRage.ServerRules]:Weapon.Assault.Damage=999', '-ExecCmds=t.MaxFPS 60'], stdout=files[-1], stderr=subprocess.STDOUT)
    processes.append(client)
    host.wait(timeout=100)
    server = hostlog.read_text(errors='replace')
    remote = clientlog.read_text(errors='replace')
    print('\n'.join(line for line in (server + '\n' + remote).splitlines() if 'ER_SERVER_RULES_' in line))
    ok = host.returncode == 0 and 'ER_SERVER_RULES_COMPLETE result=PASS' in server and 'ER_SERVER_RULES_REMOTE phase=1' in remote and 'ER_SERVER_RULES_REMOTE phase=2' in remote
    raise SystemExit(0 if ok else 1)
finally:
    for proc in processes:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
    for file in files:
        file.close()
