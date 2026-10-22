#!/usr/bin/env python3
"""Actual listen/owner/teammate/late-join recon and real 30s replacement probe."""
from pathlib import Path
import argparse,os,subprocess,time,json
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--render',action='store_true');p.add_argument('--automation',action='store_true');args=p.parse_args()
root=Path(__file__).resolve().parents[1];logs=root/'Saved/Testing';logs.mkdir(parents=True,exist_ok=True)
exe=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
common=[str(exe),str(root/'RespawnAndRage.uproject'),'-unattended','-nosound','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-ERShotAudit','-ERReconAudit']
if args.automation:
    log=logs/'recon-automation.log'
    with log.open('w') as f:r=subprocess.run(common+['-nullrhi','-ExecCmds=Automation RunTests EclipseRecoil.Recon','-TestExit=Automation Test Queue Empty'],stdout=f,stderr=subprocess.STDOUT,timeout=150)
    text=log.read_text(errors='replace');print('\n'.join(l for l in text.splitlines() if 'Result={' in l or 'Error:' in l))
    raise SystemExit(0 if r.returncode==0 and all('Result={Success} Name={'+n+'}' in text for n in ['FogAndSight','Flight','SmokeOcclusion']) and 'Result={Fail}' not in text else 1)
children=[];handles=[];paths={}
def launch(role,url,render=False,extra=()):
    path=logs/('recon-'+('metal-' if args.render else '')+role+'.log');paths[role]=path;f=path.open('w');handles.append(f)
    pr=subprocess.Popen(common+[url,'-game','-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1']+(['-windowed','-ResX=1280','-ResY=720'] if render else ['-nullrhi'])+list(extra),stdout=f,stderr=subprocess.STDOUT);children.append(pr);return pr
def read(role):return paths[role].read_text(errors='replace')
def wait(marker,seconds=75):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        if marker in read('host'):return
        if children[0].poll() is not None:raise RuntimeError('Host exited before '+marker)
        time.sleep(.2)
    raise RuntimeError('Timeout '+marker)
try:
    host=launch('host','/Game/EclipseRecoil/Maps/L_Depot?listen?Name=ReconHost?Team=0',args.render,['-ERReconProbe','-port=17796'])
    wait('listening on port 17796')
    launch('enemy','127.0.0.1:17796?Name=Enemy?Team=1',False,['-PktLag=40'])
    launch('ally','127.0.0.1:17796?Name=Ally?Team=0',False,['-PktLag=40'])
    wait('ER_RECON_LATE_JOIN_READY')
    launch('late','127.0.0.1:17796?Name=Late?Team=0',False,['-PktLag=40','-PktIncomingLoss=2'])
    host.wait(timeout=100)
    text=read('host');passed=host.returncode==0 and 'ER_RECON_PROBE_COMPLETE result=PASS' in text and 'ER_RECON_PROBE_FAIL' not in text
    for role in ['enemy','ally','late']:
        txt=read(role);good='ER_RECON_CLIENT_PASS' in txt and not any(marker in txt for marker in ['ER_RECON_CLIENT_FAIL','Critical error','Fatal error'])
        print(role,'PASS' if good else 'FAIL');passed &= good
        for phase in [14,15]:
            good='ER_RECON_CLIENT_PASS case_animation phase='+str(phase) in txt
            print(role,'case animation',phase,'PASS' if good else 'FAIL');passed &= good
        if role!='late':
            for phase in [2,4,5,6,7,8,9]:
                good='ER_RECON_CLIENT_PASS sightings phase='+str(phase)+' ' in txt
                print(role,'sightings',phase,'PASS' if good else 'FAIL');passed &= good
    print('\n'.join(l for l in text.splitlines() if 'ER_RECON_' in l))
    report={'result':'PASS' if passed else 'FAIL','render':args.render,'logs':{k:str(v) for k,v in paths.items()}}
    (logs/('recon-'+('metal-' if args.render else '')+'report.json')).write_text(json.dumps(report,indent=2)+'\n')
    raise SystemExit(0 if passed else 1)
finally:
    for pr in children:
        if pr.poll() is None:pr.terminate()
    for pr in children:
        try:pr.wait(timeout=10)
        except subprocess.TimeoutExpired:pr.kill();pr.wait()
    for f in handles:f.close()
