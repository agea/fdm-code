#!/usr/bin/env python3
"""Exercise requested weapon keys, assets and combat with two actual UE peers."""
from pathlib import Path
import argparse
import os
import subprocess
import time
from client_shutdown import settle_clients

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--render',action='store_true',help='Capture all ten viewmodels with Metal')
parser.add_argument('--knife',action='store_true',help='Check only the knife; combine with --render for ready/contact captures')
args=parser.parse_args()
project=Path(__file__).resolve().parents[1]
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
editor=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=project/'Saved/Testing';logs.mkdir(parents=True,exist_ok=True)
common=[str(editor),str(project/'RespawnAndRage.uproject'),'-game','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']
if not args.render:common.append('-nullrhi')
else:common+=['-windowed','-ResX=1280','-ResY=720','-ERReferenceGallery']
if args.render:common[common.index('-ExecCmds=t.MaxFPS 60')]='-ExecCmds=t.MaxFPS 60,r.DynamicRes.OperationMode 0,r.ScreenPercentage 100'
if args.knife:common.append('-ERKnifeGallery')
tag='reference-weapons'+('-knife' if args.knife else '')+('-render' if args.render else '')
processes=[];handles=[]
try:
    capture_started=time.time()
    hostlog=logs/(tag+'-host.log');clientlog=logs/(tag+'-client.log')
    handles.append(hostlog.open('w'))
    host=subprocess.Popen(common+['/Game/EclipseRecoil/Maps/L_Depot?listen?ERFullArsenalTest?Name=WeaponHost','-port=17795','-ERReferenceWeaponProbe'],stdout=handles[-1],stderr=subprocess.STDOUT)
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
    slots=all('ER_REFERENCE_REMOTE slot='+str(i)+' ' in remote for i in range(1 if args.knife else 10))
    print('ER_REFERENCE_REMOTE_KNIFE' if args.knife else 'ER_REFERENCE_REMOTE_ALL_SLOTS',slots)
    marker='ER_REFERENCE_KNIFE_COMPLETE result=PASS' if args.knife else 'ER_REFERENCE_COMPLETE result=PASS'
    success=host.returncode==0 and marker in server and 'ER_REFERENCE_FAIL' not in server and slots
    if args.render:
        shader_errors=[line for text in [server,remote] for line in text.splitlines()
                       if 'Weapons/Sketchfab/' in line and 'Failed to compile Material' in line]
        print('ER_REFERENCE_METAL_WEAPON_SHADERS',not shader_errors);success &= not shader_errors
        captures=all((logs/'ReferenceWeapons'/('Slot_%02d.png'%i)).exists() and
                     (logs/'ReferenceWeapons'/('Slot_%02d.png'%i)).stat().st_mtime>=capture_started for i in range(10))
        if args.knife:
            captures=all((logs/'ReferenceWeapons'/name).exists() and (logs/'ReferenceWeapons'/name).stat().st_mtime>=capture_started for name in ('Slot_00.png','Knife_Contact.png'))
        print('ER_REFERENCE_METAL_CAPTURES',captures);success &= captures
        if not args.knife:
            reloads=all((logs/'ReferenceWeapons'/('Reload_%02d.png'%i)).exists() and
                        (logs/'ReferenceWeapons'/('Reload_%02d.png'%i)).stat().st_mtime>=capture_started for i in range(1,8))
            print('ER_REFERENCE_RELOAD_CAPTURES',reloads);success &= reloads
            alpha_pistol=logs/'ReferenceWeapons/Pistol_Alpha.png'
            alpha_capture=alpha_pistol.exists() and alpha_pistol.stat().st_mtime>=capture_started
            print('ER_REFERENCE_ALPHA_PISTOL_CAPTURE',alpha_capture);success &= alpha_capture
    raise SystemExit(0 if success else 1)
finally:
    settle_clients(processes,handles)
    for process in processes:
        if process.poll() is None:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
    for handle in handles:handle.close()
