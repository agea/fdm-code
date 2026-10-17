#!/usr/bin/env python3
"""Real listen server + two clients, then a third dense-snapshot late join.

Tests seed/layout/state/collision parity, owned weapon RPCs, breach traversal,
server collapse, epoch resets and actual stairs. No packaging or publication.
"""
from pathlib import Path
import argparse,json,os,subprocess,time
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--render',action='store_true',help='Render host/initial client with Metal; late clients stay headless')
parser.add_argument('--loss',action='store_true',help='Apply 3%% incoming packet loss and 40 ms lag on remote clients')
args=parser.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'Saved/Testing';out.mkdir(parents=True,exist_ok=True)
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
exe=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
label='destruction-network'+('-metal' if args.render else '-loss' if args.loss else '')
common=[str(exe),str(root/'RespawnAndRage.uproject'),'-game','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-windowed','-ResX=1280','-ResY=720']
port='17786';children=[];handles=[];paths={}
def launch(role,url,render=False,extra=None):
 path=out/(label+'-'+role+'.log');paths[role]=path;handle=path.open('w');handles.append(handle)
 cmd=common+[url,'-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1,p.NetShowCorrections 1']
 if not render:cmd.append('-nullrhi')
 if extra:cmd.extend(extra)
 p=subprocess.Popen(cmd,cwd=root,stdout=handle,stderr=subprocess.STDOUT);children.append(p);return p
def read(role):return paths[role].read_text(errors='replace')
def wait_marker(marker,timeout):
 deadline=time.monotonic()+timeout
 while time.monotonic()<deadline:
  text=read('host')
  if marker in text:return
  if 'ER_BLOCK_NET_PROBE_FAIL' in text or children[0].poll() is not None:raise RuntimeError('Host failed before '+marker)
  time.sleep(.2)
 raise RuntimeError('Timeout before '+marker)
def client(role,name):
 extra=['-ERBlockNetRole='+role]
 if args.loss:extra.extend(['-PktIncomingLoss=3','-PktIncomingLagMin=40','-PktIncomingLagMax=40'])
 return launch(role,'127.0.0.1:'+port+'?Name='+name+'?Primary=0',args.render and role=='initial',extra)
try:
 host=launch('host','/Game/EclipseRecoil/Maps/L_ProceduralLab?listen?Name=Host?Primary=0?MapSeed=42?MapFloors=2?MapRooms=6?MapMaterials=1?MapGlass=1',args.render,['-port='+port,'-ERBlockNetworkProbe'])
 wait_marker('listening on port '+port,45);client('initial','Initial')
 wait_marker('ER_BLOCK_NET_READY_FOR_LATE_JOIN',90);client('late','Late')
 wait_marker('ER_BLOCK_NET_READY_FOR_DENSE_JOIN',120);client('dense','Dense')
 host.wait(timeout=90)
 logs={r:read(r) for r in paths}
 server=logs['host'];passed=host.returncode==0 and 'ER_BLOCK_NET_PROBE_COMPLETE result=PASS' in server and 'ER_BLOCK_NET_PROBE_FAIL' not in server
 for role,stages in [('initial',[1,2,3,5,6,7,8,9,10,12]),('late',[5,6,9,10,12]),('dense',[9,10,12])]:
  for stage in stages:
   condition=any('ER_BLOCK_NET_CLIENT_PASS phase='+str(stage)+' ' in line and 'role='+role in line for line in logs[role].splitlines())
   print('ER_BLOCK_NET_RUN_'+('PASS' if condition else 'FAIL'),role,stage);passed&=condition
  passed&='ER_BLOCK_NET_CLIENT_FAIL' not in logs[role] and 'ER_BLOCK_NET_INCOMPATIBLE' not in logs[role]
  # Corrections relative to a block group the client cannot resolve are silently dropped.
  dropped=logs[role].count('could not resolve the new relative movement base')
  print('ER_BLOCK_NET_RUN_'+('PASS' if not dropped else 'FAIL'),role,'unresolved_movement_base_corrections='+str(dropped));passed&=not dropped
 for line in server.splitlines():
  if 'ER_BLOCK_NET_PROBE_' in line or 'ER_BLOCK_NET_READY_FOR_' in line or 'ER_BLOCK_NET_REPORT ' in line:print(line)
 report={'result':'PASS' if passed else 'FAIL','render':args.render,'packet_loss':args.loss,'participants':4,'logs':{r:p.name for r,p in paths.items()},'method':'Listen host, initial remote, late remote after destruction/collapse, then dense late remote after complete wall removal; all remote clients verify full state digest and sampled real collision plus native mappings. Initial remote fires via owned pawn RPC and walks breach/stairs using real replicated character movement.'}
 (out/(label+'-report.json')).write_text(json.dumps(report,indent=2)+'\n')
 raise SystemExit(0 if passed else 1)
except (RuntimeError,subprocess.TimeoutExpired) as exc:
 print('FAIL:',exc)
 for role in paths:
  print(role,'\n'+'\n'.join(line for line in read(role).splitlines() if 'ER_BLOCK_NET_' in line)[-8000:])
 raise SystemExit(1)
finally:
 for p in children:
  if p.poll() is None:p.terminate()
 for p in children:
  try:p.wait(timeout=10)
  except subprocess.TimeoutExpired:p.kill();p.wait()
 for h in handles:h.close()
