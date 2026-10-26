#!/usr/bin/env python3
"""Validate all world weapon contacts, scales, aim states and skins on two UE peers."""
from pathlib import Path
import os
import subprocess
import time

project=Path(__file__).resolve().parents[1]
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
editor=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=project/'Saved/Testing';logs.mkdir(parents=True,exist_ok=True)
common=[str(editor),str(project/'RespawnAndRage.uproject'),'-game','-nullrhi','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']
hostlog=logs/'world-poses-network-host.log';clientlog=logs/'world-poses-network-client.log'
processes=[];handles=[]
try:
    handles.append(hostlog.open('w'))
    host=subprocess.Popen(common+['/Game/EclipseRecoil/Maps/L_Depot?listen?ERFullArsenalTest?ERAlphaBots=1?EROmegaBots=0?ERBotMode=Target?Name=PoseHost','-port=17967','-ERWorldPoseProbe','-ERWorldPoseExternal','-ERWorldPoseNetwork'],stdout=handles[-1],stderr=subprocess.STDOUT)
    processes.append(host);deadline=time.monotonic()+60
    while 'listening on port 17967' not in hostlog.read_text(errors='replace'):
        if host.poll() is not None or time.monotonic()>deadline:raise RuntimeError('World-pose host did not listen')
        time.sleep(.2)
    handles.append(clientlog.open('w'))
    client=subprocess.Popen(common+['127.0.0.1:17967?Name=PoseClient'],stdout=handles[-1],stderr=subprocess.STDOUT)
    processes.append(client)
    host.wait(timeout=300)
    server=hostlog.read_text(errors='replace');remote=clientlog.read_text(errors='replace')
    server_count=server.count('ER_WORLD_POSE_PASS sample=')
    client_count=sum('ER_WORLD_POSE_PASS sample=' in line and 'client=1' in line for line in remote.splitlines())
    complete='ER_WORLD_POSE_COMPLETE result=PASS samples=260' in server
    clean='ER_WORLD_POSE_FAIL' not in server and 'ER_WORLD_POSE_FAIL' not in remote
    print(f'ER_WORLD_POSES_NETWORK server={server_count}/260 client={client_count}/260 complete={int(complete)} clean={int(clean)}')
    raise SystemExit(0 if server_count==260 and client_count==260 and complete and clean else 1)
finally:
    for process in processes:
        if process.poll() is None:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
    for handle in handles:handle.close()
