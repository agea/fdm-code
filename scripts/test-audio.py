#!/usr/bin/env python3
"""Saved audio assets/native cadence, or actual host/client firearm playback.

--render enables Metal and the host's real audio device. Client audio is enabled
as well; this checks playback/variant events, not a claim of subjective listening.
"""
from pathlib import Path
import argparse,os,subprocess,time,re,json
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--network',action='store_true')
p.add_argument('--render',action='store_true')
a=p.parse_args();root=Path(__file__).resolve().parents[1]
editor=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=root/'Saved/Testing';logs.mkdir(parents=True,exist_ok=True)
common=[str(editor),str(root/'RespawnAndRage.uproject'),'-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput']
if not (a.network or a.render):
    log=logs/'audio-automation.log'
    with log.open('w') as out:
        run=subprocess.run(common+['/Engine/Maps/Entry','-nullrhi','-nosound','-ExecCmds=Automation RunTests EclipseRecoil.Audio','-TestExit=Automation Test Queue Empty'],stdout=out,stderr=subprocess.STDOUT,timeout=180)
    text=log.read_text(errors='replace')
    for line in text.splitlines():
        if 'Test Completed.' in line or 'LogAutomationController: Error:' in line:print(line)
    raise SystemExit(0 if run.returncode==0 and all('Result={Success} Name={'+name+'}' in text for name in ['NoRepeat','Cadence','SavedAssets','SurfaceQueries','NoDeviceCleanup']) else 1)
processes=[];handles=[];tag='audio-network'+('-render' if a.render else '')
try:
    flags=['-game','-ERAudioAudit','-ExecCmds=t.MaxFPS 60']
    if a.render:flags+=['-ini:Engine:Audio:UnfocusedVolumeMultiplier=1.0']
    flags+=['-windowed','-ResX=1280','-ResY=720'] if a.render else ['-nullrhi','-nosound']
    hostlog=logs/(tag+'-host.log');clientlog=logs/(tag+'-client.log')
    handles.append(hostlog.open('w'))
    host=subprocess.Popen(common+flags+['/Game/EclipseRecoil/Maps/L_Depot?listen?Name=AudioHost?ERAlphaBots=1?EROmegaBots=1?ERBotMode=Movement?ERFullArsenalTest=1','-port=17829','-ERAudioProbe'],stdout=handles[-1],stderr=subprocess.STDOUT);processes.append(host)
    end=time.monotonic()+75
    while 'listening on port 17829' not in hostlog.read_text(errors='replace'):
        if host.poll() is not None or time.monotonic()>end:raise RuntimeError('Host failed to listen')
        time.sleep(.2)
    handles.append(clientlog.open('w'))
    client=subprocess.Popen(common+flags+['127.0.0.1:17829?Name=AudioClient'],stdout=handles[-1],stderr=subprocess.STDOUT);processes.append(client)
    host.wait(timeout=360);time.sleep(1)
    ht=hostlog.read_text(errors='replace');ct=clientlog.read_text(errors='replace')
    checks={'probe':host.returncode==0 and 'ER_AUDIO_COMPLETE result=PASS' in ht and 'ER_AUDIO_FAIL' not in ht}
    for peer,txt in [('host',ht),('client',ct)]:
        last={};counts={};distinct={};valid=True
        for pawn,local,palette,variant,event,pellets in re.findall(r'ER_SHOT_AUDIO pawn=(\S+) local=(\d) palette=(\S+) variant=(-?\d+) event=(\d+) pellets=(\d+)',txt):
            index=int(variant);key=(pawn,palette);last_index=last.get(key)
            valid &= index>=0 and (palette in ('DA_ER_Shot_Milkor','DA_ER_Shot_Sniper','DA_ER_Shot_Carbine','DA_ER_Shot_Shotgun') or last_index!=index)
            last[key]=index;counts[(palette,local)]=counts.get((palette,local),0)+1;distinct.setdefault(palette,set()).add(index)
        checks[peer+'_no_adjacent_repeat']=valid and bool(counts)
        checks[peer+'_empty_trigger_owner_feedback']=txt.count('ER_EMPTY_TRIGGER_AUDIO weapon=')>=7
        checks[peer+'_spatial_steps']='ER_STEP_AUDIO' in txt and 'local=0 surface=' in txt
        if peer=='host':checks['owner_steps_2d']=bool(re.search(r'ER_STEP_AUDIO .*local=1 surface=',txt))
        for gun in ['Pistol','SMG','Shotgun','Assault','Carbine','Sniper','Milkor']:
            palette='DA_ER_Shot_'+gun
            for perspective in ['0','1']:checks[f'{peer}_{gun}_perspective_{perspective}']=counts.get((palette,perspective),0)>=20
            checks[f'{peer}_{gun}_variety']=len(distinct.get(palette,set()))>=3 if gun not in ('Milkor','Sniper','Carbine','Shotgun') else len(distinct.get(palette,set()))>=1
        if a.render:
            voices=re.findall(r'ER_AUDIO_VOICE palette=(DA_ER_Shot_\S+) sample=\S+ local=\d started=(\d)',txt)
            events=re.findall(r'ER_SHOT_AUDIO pawn=',txt)
            autostarts=re.findall(r'ER_AUDIO_VOICE palette=DA_ER_Shot_[^\n]*autostart=(\d)',txt)
            checks[peer+'_no_registration_autoplay']=len(autostarts)==len(events) and all(v=='0' for v in autostarts)
            checks[peer+'_one_playing_voice_per_shot_event']=len(voices)==len(events) and bool(voices) and all(started=='1' for _,started in voices)
        if a.render:checks[peer+'_real_audio_device']='LogAudioMixer: Display: Starting AudioMixerPlatformInterface::RunInternal()' in txt and 'started=1' in txt and 'ER_AUDIO_VOICE palette=DA_ER_Shot_' in txt
    for key,value in checks.items():print('ER_AUDIO_CHECK',key,value)
    (logs/(tag+'-results.json')).write_text(json.dumps(checks,indent=2)+'\n')
    print('Logs:',hostlog,clientlog)
    raise SystemExit(0 if all(checks.values()) else 1)
finally:
    for process in processes:
        if process.poll() is None:
            process.terminate()
            try:process.wait(timeout=8)
            except subprocess.TimeoutExpired:process.kill();process.wait()
    for h in handles:h.close()
