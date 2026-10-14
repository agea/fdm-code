#!/usr/bin/env python3
"""Two actual local Unreal peers: weapon selection, ammo, respawn and payloads."""
from pathlib import Path
import os
import subprocess
import time

project=Path(__file__).resolve().parents[1]
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
editor=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=project/'Saved/Testing'; logs.mkdir(parents=True,exist_ok=True)
common=[str(editor),str(project/'fdm.uproject'),'-game','-nullrhi','-nosound','-unattended','-NoSplash','-Multiprocess',
        '-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']
processes=[];handles=[]
try:
    hostlog=logs/'arsenal-host.log'; clientlog=logs/'arsenal-client.log'
    handles.append(hostlog.open('w'))
    host=subprocess.Popen(common+['/Game/EclipseRecoil/Maps/L_Depot?listen?Primary=0','-port=17784','-ERArsenalProbe'],stdout=handles[-1],stderr=subprocess.STDOUT)
    processes.append(host)
    deadline=time.monotonic()+45
    while 'listening on port 17784' not in hostlog.read_text(errors='replace'):
        if host.poll() is not None or time.monotonic()>deadline: raise RuntimeError('Host failed to listen')
        time.sleep(.2)
    handles.append(clientlog.open('w'))
    client=subprocess.Popen(common+['127.0.0.1:17784?Name=ArsenalClient?Primary=0'],stdout=handles[-1],stderr=subprocess.STDOUT)
    processes.append(client)
    host.wait(timeout=70)
    time.sleep(.3)
    server=hostlog.read_text(errors='replace'); remote=clientlog.read_text(errors='replace')
    print('\n'.join(line for line in server.splitlines() if 'ER_ARSENAL_' in line or 'ER_PAYLOAD_DETONATE' in line))
    success='ER_ARSENAL_COMPLETE result=PASS' in server and 'ER_ARSENAL_FAIL' not in server
    conditions={
        'remote_new_loadout_mesh': 'name=M249 mesh=SK_M249 ammo=100' in remote,
        'remote_directional_stun_feedback':'ER_ARSENAL_REMOTE_FLASH' in remote,
        'he_smoke_stun_fuses': all('ER_PAYLOAD_DETONATE kind='+str(kind) in server for kind in [2,3,4]),
        'he_burst_on_host_and_remote': all('ER_GRENADE_BURST mesh=SK_ER_BombDebris animation=A_ER_BombDebris' in peer for peer in [server,remote]),
        'smoke_cloud_on_host_and_remote': all('ER_SMOKE_CLOUD system=NS_ER_InteractiveSmoke' in peer for peer in [server,remote]),
        'remote_shot_carving':'ER_SMOKE_REMOTE_CUT events=' in remote,
    }
    for label,passed in conditions.items():
        print('ER_ARSENAL_'+('PASS ' if passed else 'FAIL ')+label)
        success &= passed
    raise SystemExit(0 if success else 1)
finally:
    for p in processes:
        if p.poll() is None:
            p.terminate()
            try:p.wait(timeout=5)
            except subprocess.TimeoutExpired:p.kill();p.wait()
    for f in handles:f.close()
