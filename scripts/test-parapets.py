#!/usr/bin/env python3
"""Real Space+forward climbs on all four parapets of both Killhouse bases.

The host and autonomous remote each run eight cases. Roof fixture placement is
server-only; climbing uses ordinary input, collision and saved movement.
"""
import argparse,json,os,subprocess,time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--render',action='store_true');p.add_argument('--seed',type=int,default=42);args=p.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'Saved/Testing';out.mkdir(parents=True,exist_ok=True)
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'));exe=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
children=[];handles=[]
common=[str(exe),str(root/'RespawnAndRage.uproject'),'-game','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-ERParapetProbe','-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1']
def launch(role,url,extra):
 f=(out/('parapet-'+role+'.log')).open('w');handles.append(f)
 c=subprocess.Popen(common+[url]+extra,cwd=root,stdout=f,stderr=subprocess.STDOUT);children.append(c);return c
def read(role):return (out/('parapet-'+role+'.log')).read_text(errors='replace')
try:
 host=launch('host',f'/Game/EclipseRecoil/Maps/L_Killhouse?listen?Name=Host?MapSeed={args.seed}',['-ERMapTemplateProbe','-port=17791','-windowed','-ResX=1280','-ResY=720']+([] if args.render else ['-nullrhi']))
 end=time.monotonic()+65
 while 'listening on port 17791' not in read('host'):
  if host.poll() is not None or time.monotonic()>end:raise RuntimeError('Host did not listen')
  time.sleep(.2)
 launch('client','127.0.0.1:17791?Name=Remote',['-nullrhi'])
 host.wait(timeout=100)
 h,c=read('host'),read('client')
 passed=host.returncode==0 and 'ER_PARAPET_COMPLETE result=PASS' in h and 'ER_PARAPET_SERVER_FAIL' not in h and 'ER_PARAPET_CLIENT_FAIL' not in c
 for case in range(1,17):passed&=f'ER_PARAPET_SERVER_PASS case={case} ' in h
 for case in range(9,17):passed&=f'ER_PARAPET_CLIENT_PASS case={case} ' in c
 for text in [h,c]:
  for line in text.splitlines():
   if 'ER_PARAPET_' in line:print(line)
 (out/'parapet-report.json').write_text(json.dumps(dict(result='PASS' if passed else 'FAIL',seed=args.seed,server_cases=16,autonomous_client_cases=8,render=args.render),indent=2)+'\n')
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
