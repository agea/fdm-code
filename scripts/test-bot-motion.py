#!/usr/bin/env python3
"""Observe actual movement-only bot jumps/traversal/postures from a LAN peer."""
from pathlib import Path
import json,os,subprocess,time
root=Path(__file__).resolve().parents[1];out=root/'Saved/Testing';out.mkdir(parents=True,exist_ok=True)
exe=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
common=[str(exe),str(root/'RespawnAndRage.uproject'),'-game','-nullrhi','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']
children=[];handles=[]
def read(role):return (out/('bot-motion-'+role+'.log')).read_text(errors='replace')
def launch(role,url,extra):
 f=(out/('bot-motion-'+role+'.log')).open('w');handles.append(f)
 c=subprocess.Popen(common+[url]+extra,cwd=root,stdout=f,stderr=subprocess.STDOUT);children.append(c);return c
try:
 host=launch('host','/Game/EclipseRecoil/Maps/L_MovementLab?listen?Name=Host?ERAlphaBots=1?EROmegaBots=1?ERBotMode=Movement',['-port=17793','-ERBotMotionProbe'])
 end=time.monotonic()+65
 while 'listening on port 17793' not in read('host'):
  if host.poll() is not None or time.monotonic()>end:raise RuntimeError('Host did not listen')
  time.sleep(.2)
 launch('client','127.0.0.1:17793?Name=Observer',[])
 host.wait(timeout=100)
 h,c=read('host'),read('client');passed=host.returncode==0 and 'ER_BOT_MOTION_COMPLETE result=PASS' in h and 'ER_BOT_MOTION_CLIENT_PASS' in c and 'ER_BOT_MOTION_CLIENT_FAIL' not in c
 for t in [h,c]:
  for line in t.splitlines():
   if 'ER_BOT_MOTION_' in line:print(line)
 (out/'bot-motion-report.json').write_text(json.dumps(dict(result='PASS' if passed else 'FAIL',bots=2,evidence_mask=127,checks=['jump','traversal','crouch','prone_world_mesh','stand_after_prone','crawl','mounted80_140cm','no_firing','team_appearance']),indent=2)+'\n')
 raise SystemExit(0 if passed else 1)
except (RuntimeError,subprocess.TimeoutExpired) as e:
 print('FAIL',e);print('\n'.join(read('host').splitlines()[-20:]));raise SystemExit(1)
finally:
 for c in children:
  if c.poll() is None:c.terminate()
 for c in children:
  try:c.wait(timeout=10)
  except subprocess.TimeoutExpired:c.kill();c.wait()
 for f in handles:f.close()
