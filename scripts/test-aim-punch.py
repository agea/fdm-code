#!/usr/bin/env python3
"""Native math or two actual Unreal peers: authoritative bullet aim and red HUD."""
from pathlib import Path
import argparse, json, math, os, re, subprocess, time
from client_shutdown import settle_clients
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--automation',action='store_true')
p.add_argument('--render',action='store_true')
p.add_argument('--lag',type=int,default=0,help='Outgoing packet lag in ms on each peer')
a=p.parse_args();root=Path(__file__).resolve().parents[1]
editor=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=root/'Saved/Testing';logs.mkdir(parents=True,exist_ok=True)
common=[str(editor),str(root/'RespawnAndRage.uproject'),'-unattended','-nosound','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput']
if a.automation:
    log=logs/'aim-punch-automation.log'
    with log.open('w') as f:
        run=subprocess.run(common+['/Engine/Maps/Entry','-nullrhi','-ExecCmds=Automation RunTests EclipseRecoil.Weapons.AimPunch','-TestExit=Automation Test Queue Empty'],stdout=f,stderr=subprocess.STDOUT,timeout=150)
    text=log.read_text(errors='replace')
    print('\n'.join(x for x in text.splitlines() if 'Test Completed.' in x or 'LogAutomationController: Error:' in x))
    raise SystemExit(0 if run.returncode==0 and 'Result={Success} Name={AimPunch}' in text else 1)
tag='aim-punch'+('-render' if a.render else '')+(f'-lag{a.lag}' if a.lag else '')
common+=['-game','-ERAimPunchAudit','-ExecCmds=t.MaxFPS 60,DisableAllScreenMessages',f'-PktLag={a.lag}']
common+=['-windowed','-ResX=1280','-ResY=720','-ERAimPunchGallery'] if a.render else ['-nullrhi']
processes=[];handles=[];started=time.time()
try:
    hostlog=logs/(tag+'-host.log');clientlog=logs/(tag+'-client.log')
    handles.append(hostlog.open('w'))
    host=subprocess.Popen(common+['/Game/EclipseRecoil/Maps/L_Depot?listen?Name=AimHost?ERFullArsenalTest?ERAlphaBots=1?EROmegaBots=0?ERBotMode=Target','-port=17831','-ERAimPunchProbe'],stdout=handles[-1],stderr=subprocess.STDOUT);processes.append(host)
    end=time.monotonic()+70
    while 'listening on port 17831' not in hostlog.read_text(errors='replace'):
        if host.poll() is not None or time.monotonic()>end:raise RuntimeError('Host failed to listen')
        time.sleep(.2)
    handles.append(clientlog.open('w'))
    client=subprocess.Popen(common+['127.0.0.1:17831?Name=AimClient'],stdout=handles[-1],stderr=subprocess.STDOUT);processes.append(client)
    host.wait(timeout=150);time.sleep(.5)
    ht=hostlog.read_text(errors='replace');ct=clientlog.read_text(errors='replace')
    print('\n'.join(x for x in (ht+'\n'+ct).splitlines() if 'ER_AIMPUNCH_' in x))
    checks={'authority':host.returncode==0 and 'ER_AIMPUNCH_COMPLETE result=PASS' in ht and 'ER_AIMPUNCH_FAIL' not in ht,
            'owner':len(re.findall('ER_AIMPUNCH_CLIENT_PASS owner_camera_once',ct))>=2 and 'ER_AIMPUNCH_CLIENT_FAIL' not in ct,
            'owner_recovers':'ER_AIMPUNCH_CLIENT_PASS owner_recovers' in ct,
            'scoped_camera':'ER_AIMPUNCH_CLIENT_PASS scoped_camera_and_hud' in ct}
    samples=[float(x) for x in re.findall(r'age_ms=([\d.-]+)',ct)]
    if a.render:
        gallery=logs/'AimPunch'
        checks['hit_captures']=any(x.stat().st_mtime>=started for x in gallery.glob('owner-hit-*.png'))
        checks['recovered_capture']=(gallery/'owner-recovered.png').exists() and (gallery/'owner-recovered.png').stat().st_mtime>=started
    # Replicated actor object names can differ between the two processes.
    remote_name=re.search(r'ER_AIMPUNCH_TARGET pawn=(\S+)',ht)
    owner_name=re.search(r'ER_AIMPUNCH_RECEIVED pawn=(\S+)',ct)
    remote_text=''
    if remote_name and owner_name:
        remote_text='\n'.join(line.replace('pawn='+remote_name[1]+' ','pawn='+owner_name[1]+' ')
                              for line in ht.splitlines() if 'pawn='+remote_name[1]+' ' in line)
    states={(pawn,int(seq)):(float(real),float(pitch),float(half)) for pawn,seq,real,pitch,half in
            re.findall(r'ER_AIMPUNCH_STATE pawn=(\S+) seq=(\d+) time=[\d.]+ real=([\d.]+) pitch=([\d.-]+) half_life=([\d.]+)',remote_text)}
    received={(pawn,int(seq)):(float(real),float(pitch)) for pawn,seq,real,pitch in
              re.findall(r'ER_AIMPUNCH_RECEIVED pawn=(\S+) seq=(\d+) real=([\d.]+) pitch=([\d.-]+)',ct)}
    transit=[]
    for key,(when,pitch) in received.items():
        if key in states:
            sent,angle,half=states[key];elapsed=max(0.,when-sent)
            expected=angle*math.pow(.5,elapsed/half)
            transit.append({'sequence':key[1],'real_transit_ms':elapsed*1000,'snapshot_pitch_at_receipt_deg':expected,'owner_pitch_deg':pitch,'difference_deg':pitch-expected})
    cameras=[(pawn,float(real),float(pitch),float(yaw),float(punch)) for pawn,real,pitch,yaw,punch in
             re.findall(r'ER_AIMPUNCH_CAMERA pawn=(\S+) real=([\d.]+) pitch=([\d.-]+) yaw=([\d.-]+) punch=([\d.-]+)',ct)]
    early_shots=[];comparisons=[]
    for pawn,seq,real,punch,pitch,yaw in re.findall(r'ER_AIMPUNCH_SHOT pawn=(\S+) seq=(\d+) time=[\d.]+ real=([\d.]+) punch_pitch=([\d.-]+) punch_yaw=[\d.-]+ aim_pitch=([\d.-]+) aim_yaw=([\d.-]+)',remote_text):
        when=float(real);key=(pawn,int(seq));receipt=received.get(key)
        if receipt is not None and when<receipt[0] and float(punch)>.01:
            early_shots.append({'sequence':int(seq),'ms_before_owner_receipt':(receipt[0]-when)*1000,'server_punch_pitch_deg':float(punch)})
        near=[c for c in cameras if c[0]==pawn and abs(c[1]-when)<.05]
        if near:
            c=min(near,key=lambda c:abs(c[1]-when))
            comparisons.append({'sequence':int(seq),'before_owner_notification':receipt is not None and when<receipt[0],
                                'sample_separation_ms':abs(c[1]-when)*1000,'pitch_difference_deg':float(pitch)-c[2],
                                'yaw_difference_deg':(float(yaw)-c[3]+180)%360-180,'punch_difference_deg':float(punch)-c[4]})
    checks['measured_camera_and_transit']=bool(transit) and bool(comparisons)
    receipt={'checks':checks,'owner_event_age_ms':samples,'lag_per_peer_ms':a.lag,'hit_receipt_measurements':transit,
             'shots_before_owner_notification':early_shots,'server_aim_vs_nearest_owner_camera':comparisons}
    (logs/(tag+'-results.json')).write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))
    raise SystemExit(0 if all(checks.values()) else 1)
finally:
    settle_clients(processes,handles)
    for proc in processes:
        if proc.poll() is None:
            proc.terminate()
            try:proc.wait(timeout=8)
            except subprocess.TimeoutExpired:proc.kill();proc.wait()
    for f in handles:f.close()
