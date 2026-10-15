#!/usr/bin/env python3
"""Exercise requested weapon keys, assets and combat with two actual UE peers."""
from pathlib import Path
import argparse
import os
import subprocess
import time

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--render',action='store_true',help='Capture all ten viewmodels with Metal')
args=parser.parse_args()
project=Path(__file__).resolve().parents[1]
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
editor=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=project/'Saved/Testing';logs.mkdir(parents=True,exist_ok=True)
common=[str(editor),str(project/'fdm.uproject'),'-game','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']
if not args.render:common.append('-nullrhi')
else:common+=['-windowed','-ResX=1280','-ResY=720','-ERReferenceGallery']
tag='reference-weapons'+('-render' if args.render else '')
processes=[];handles=[]
try:
    capture_started=time.time()
    hostlog=logs/(tag+'-host.log');clientlog=logs/(tag+'-client.log')
    handles.append(hostlog.open('w'))
    host=subprocess.Popen(common+['/Game/EclipseRecoil/Maps/L_Depot?listen?Name=WeaponHost','-port=17795','-ERReferenceWeaponProbe'],stdout=handles[-1],stderr=subprocess.STDOUT)
    processes.append(host);deadline=time.monotonic()+60
    while 'listening on port 17795' not in hostlog.read_text(errors='replace'):
        if host.poll() is not None or time.monotonic()>deadline:raise RuntimeError('Reference host did not listen')
        time.sleep(.2)
    handles.append(clientlog.open('w'))
    client=subprocess.Popen(common+['127.0.0.1:17795?Name=WeaponClient'],stdout=handles[-1],stderr=subprocess.STDOUT)
    processes.append(client)
    host.wait(timeout=240 if args.render else 120)
    server=hostlog.read_text(errors='replace');remote=clientlog.read_text(errors='replace')
    print('\n'.join(line for line in server.splitlines() if 'ER_REFERENCE_' in line))
    slots=all('ER_REFERENCE_REMOTE slot='+str(i)+' ' in remote for i in range(10))
    print('ER_REFERENCE_REMOTE_ALL_SLOTS',slots)
    success=host.returncode==0 and 'ER_REFERENCE_COMPLETE result=PASS' in server and 'ER_REFERENCE_FAIL' not in server and slots
    if args.render:
        captures=all((logs/'ReferenceWeapons'/('Slot_%02d.png'%i)).exists() and
                     (logs/'ReferenceWeapons'/('Slot_%02d.png'%i)).stat().st_mtime>=capture_started for i in range(10))
        print('ER_REFERENCE_METAL_CAPTURES',captures);success &= captures
        reloads=all((logs/'ReferenceWeapons'/('Reload_%02d.png'%i)).exists() and
                    (logs/'ReferenceWeapons'/('Reload_%02d.png'%i)).stat().st_mtime>=capture_started for i in range(1,8))
        print('ER_REFERENCE_RELOAD_CAPTURES',reloads);success &= reloads
    raise SystemExit(0 if success else 1)
finally:
    for process in processes:
        if process.poll() is None:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
    for handle in handles:handle.close()
