#!/usr/bin/env python3
"""Saved template: real game movement/shots, optional Metal and LAN late join."""
import argparse,json,os,re,subprocess,time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--render',action='store_true');p.add_argument('--network',action='store_true');p.add_argument('--gallery',action='store_true');args=p.parse_args()
if args.gallery:args.render=True
if args.gallery and args.network:p.error('--gallery is standalone; use --render --network for LAN')
root=Path(__file__).resolve().parents[1];out=root/'Saved/Testing';out.mkdir(parents=True,exist_ok=True)
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
exe=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
label='template'+('-network' if args.network else '-game')+('-metal' if args.render else '')
children=[];handles=[];paths={}
def launch(role,url,render=False,extra=()):
 path=out/(label+'-'+role+'.log');paths[role]=path;f=path.open('w');handles.append(f)
 cmd=[str(exe),str(root/'fdm.uproject'),url,'-game','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-windowed','-ResX=1280','-ResY=720','-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1']+list(extra)
 if not render:cmd.append('-nullrhi')
 process=subprocess.Popen(cmd,cwd=root,stdout=f,stderr=subprocess.STDOUT);children.append(process);return process
def read(role):return paths[role].read_text(errors='replace')
def wait_for(marker,timeout=70):
 end=time.monotonic()+timeout
 while time.monotonic()<end:
  text=read('host')
  if marker in text:return
  if children[0].poll() is not None:raise RuntimeError('Host stopped before '+marker)
  time.sleep(.25)
 raise RuntimeError('Timed out: '+marker)
try:
 url='/Game/EclipseRecoil/Maps/L_Killhouse?Name=Host?Primary=0'
 if args.network:url+='?listen'
 host=launch('host',url,args.render,tuple(['-ERMapTemplateProbe','-port=17789']+(['-ERTemplateGallery'] if args.gallery else [])))
 if args.network:
  wait_for('ER_TEMPLATE_STATE phase=1 ')
  launch('initial','127.0.0.1:17789?Name=Initial?Primary=0')
  wait_for('ER_TEMPLATE_STATE phase=3 ')
  launch('late','127.0.0.1:17789?Name=Late?Primary=0')
 host.wait(timeout=100)
 text=read('host');passed=host.returncode==0 and 'ER_TEMPLATE_PROBE_COMPLETE result=PASS' in text and 'ER_TEMPLATE_PROBE_FAIL' not in text
 states={int(a):int(b) for a,b in re.findall(r'ER_TEMPLATE_STATE phase=(\d+) digest=(\d+)',text)}
 if args.network:
  for role,phases in [('initial',[1,3,4]),('late',[3,4])]:
   client=read(role)
   for phase in phases:
    matched=f'ER_TEMPLATE_CLIENT_PASS phase={phase} digest={states.get(phase)} ' in client
    print(role,'phase',phase,'PASS' if matched else 'FAIL');passed&=matched
   passed&='ER_TEMPLATE_CLIENT_FAIL' not in client and 'ER_BLOCK_NET_INCOMPATIBLE' not in client
 for line in text.splitlines():
  if 'ER_TEMPLATE_' in line:print(line)
 receipt=dict(result='PASS' if passed else 'FAIL',render=args.render,network=args.network,states=states,logs={k:str(v) for k,v in paths.items()})
 (out/(label+'-report.json')).write_text(json.dumps(receipt,indent=2)+'\n')
 raise SystemExit(0 if passed else 1)
except (RuntimeError,subprocess.TimeoutExpired) as e:
 print('FAIL',e)
 print('\n'.join(read('host').splitlines()[-35:]));raise SystemExit(1)
finally:
 for process in children:
  if process.poll() is None:process.terminate()
 for process in children:
  try:process.wait(timeout=10)
  except subprocess.TimeoutExpired:process.kill();process.wait()
 for f in handles:f.close()
