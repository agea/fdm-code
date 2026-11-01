#!/usr/bin/env python3
"""Real local/listen/client tests for session setup and bot behavior."""
from pathlib import Path
import argparse, os, subprocess, time
from client_shutdown import settle_clients
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--map", help="Run only scenarios for this map")
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
exe=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=root/'Saved/Testing'; logs.mkdir(parents=True,exist_ok=True)
common=[str(exe),str(root/'RespawnAndRage.uproject'),'-game','-nullrhi','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60','-MULTIHOME=0.0.0.0']
for index,(map_name,behavior,server) in enumerate([('L_Depot','Target',False),('L_Depot','Movement',True)]):
    if args.map and map_name!=args.map: continue
    port=17930+index
    label=f'session-{map_name}-{behavior}'
    path=logs/(label+'.log'); handles=[]; processes=[]
    try:
        handle=path.open('w'); handles.append(handle)
        url=f'/Game/EclipseRecoil/Maps/{map_name}?Name=Host?Team=1?ERAlphaBots=2?EROmegaBots=1?ERBotMode={behavior}'+('?listen' if server else '')
        host=subprocess.Popen(common+[url,f'-port={port}','-ERSessionProbe'],stdout=handle,stderr=subprocess.STDOUT); processes.append(host)
        deadline=time.monotonic()+80
        while 'ER_SESSION_PASS solo_match_started' not in path.read_text(errors='replace'):
            if host.poll() is not None or time.monotonic()>deadline: raise RuntimeError('Solo start failed: '+str(path))
            time.sleep(.2)
        if server:
            sockets=subprocess.check_output(['/usr/sbin/lsof','-nP','-a','-p',str(host.pid),'-iUDP'],text=True)
            assert f'*:{port}' in sockets,sockets
            print(label+' bind 0.0.0.0 verified',flush=True)
            handle=(logs/(label+'-client.log')).open('w'); handles.append(handle)
            client=subprocess.Popen(common+[f'127.0.0.1:{port}?Name=Client?Team=0'],stdout=handle,stderr=subprocess.STDOUT);processes.append(client)
        host.wait(timeout=90)
        output=path.read_text(errors='replace')
        print('\n'.join(line for line in output.splitlines() if 'ER_SESSION_' in line),flush=True)
        assert host.returncode==0 and 'ER_SESSION_COMPLETE result=PASS' in output and 'ER_SESSION_FAIL' not in output,str(path)
    finally:
        settle_clients(processes,handles)
        for process in processes:
            if process.poll() is None: process.terminate()
        for process in processes:
            try: process.wait(timeout=10)
            except subprocess.TimeoutExpired: process.kill(); process.wait()
        for handle in handles: handle.close()
