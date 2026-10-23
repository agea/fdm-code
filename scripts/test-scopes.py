#!/usr/bin/env python3
"""Exercise scope cycles, settled/hip accuracy and shot behavior with two UE peers."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import time

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--render',action='store_true',help='Capture rifle hip/iron/optical states with Metal')
args=parser.parse_args()
project=Path(__file__).resolve().parents[1]
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
editor=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=project/'Saved/Testing';logs.mkdir(parents=True,exist_ok=True)
common=[str(editor),str(project/'RespawnAndRage.uproject'),'-game','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60','-ERScopeTest']
if not args.render:common.append('-nullrhi')
else:common+=['-windowed','-ResX=1280','-ResY=720','-ERReferenceGallery']
if args.render:common[common.index('-ExecCmds=t.MaxFPS 60')]='-ExecCmds=t.MaxFPS 60,r.DynamicRes.OperationMode 0,r.ScreenPercentage 100'
tag='scopes'+('-render' if args.render else '')
processes=[];handles=[]
try:
    capture_started=time.time()
    hostlog=logs/(tag+'-host.log');clientlog=logs/(tag+'-client.log')
    handles.append(hostlog.open('w'))
    host=subprocess.Popen(common+['/Game/EclipseRecoil/Maps/L_Depot?listen?ERFullArsenalTest?Name=WeaponHost','-port=17796','-ERReferenceWeaponProbe'],stdout=handles[-1],stderr=subprocess.STDOUT)
    processes.append(host);deadline=time.monotonic()+60
    while 'listening on port 17796' not in hostlog.read_text(errors='replace'):
        if host.poll() is not None or time.monotonic()>deadline:raise RuntimeError('Scope host did not listen')
        time.sleep(.2)
    handles.append(clientlog.open('w'))
    client=subprocess.Popen(common+['127.0.0.1:17796?Name=WeaponClient'],stdout=handles[-1],stderr=subprocess.STDOUT)
    processes.append(client)
    host.wait(timeout=240 if args.render else 120)
    server=hostlog.read_text(errors='replace');remote=clientlog.read_text(errors='replace')
    print('\n'.join(line for line in server.splitlines() if 'ER_REFERENCE_' in line))
    expected=[0,1,0,0,1,2,0,0,1,2,0,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0]
    observations={int(p):(int(level),float(fov)) for p,level,fov in
                  re.findall(r'ER_SCOPE_OWNER phase=(\d+) level=(\d+) fov=([\d.]+)',remote)}
    slots=all(observations.get(100+i)==(level,75. if i in (1,20,21) else 40. if level==1 else 10. if level==2 else 90.)
              for i,level in enumerate(expected))
    print('ER_SCOPE_REMOTE_ALL_STATES',slots)
    success=host.returncode==0 and 'ER_REFERENCE_COMPLETE result=PASS' in server and 'ER_REFERENCE_FAIL' not in server and slots
    if args.render:
        captures=all((logs/'Scopes'/f'Scope_{i}.png').exists() and
                     (logs/'Scopes'/f'Scope_{i}.png').stat().st_mtime>=capture_started for i in range(100,126))
        print('ER_SCOPE_METAL_CAPTURES',captures);success &= captures
        centred='ER_SCOPE_IRON_ALIGN result=PASS' in remote and 'ER_SCOPE_IRON_ALIGN result=FAIL' not in remote
        print('ER_SCOPE_IRON_ALIGN',centred);success &= centred
        transitions=all('ER_SCOPE_TRANSITION phase='+str(i)+' ' in remote and
                        (logs/'Scopes'/f'Transition_{i}.png').exists() and
                        (logs/'Scopes'/f'Transition_{i}.png').stat().st_mtime>=capture_started for i in (101,102,120,122))
        print('ER_SCOPE_CONTINUOUS_TRANSITIONS',transitions);success &= transitions
    raise SystemExit(0 if success else 1)
finally:
    for process in processes:
        if process.poll() is None:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
    for handle in handles:handle.close()
