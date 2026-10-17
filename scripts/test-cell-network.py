#!/usr/bin/env python3
"""Real listen server + initial client, then a late client, on the 5cm cell lab (protocol 7).

L_DestructionLab?Cells=1 with -ERCellNetworkProbe on the host. Checks state digests after
the drywall and brick grenades, collision gating until the first snapshot, real breach
crossings by the client's own character, late join with modified chunks only, and new
epochs from ERCellMode/ERBlockRegenerate. No packaging or publication.
"""
from pathlib import Path
import argparse,json,os,re,subprocess,time
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--render',action='store_true',help='Render host/initial client with Metal; the late client stays headless')
parser.add_argument('--loss',action='store_true',help='Apply 3%% incoming packet loss and 40 ms lag on remote clients')
args=parser.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'Saved/Testing';out.mkdir(parents=True,exist_ok=True)
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
exe=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
label='cell-network'+('-metal' if args.render else '-loss' if args.loss else '')
common=[str(exe),str(root/'RespawnAndRage.uproject'),'-game','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-windowed','-ResX=1280','-ResY=720']
port='17787';children=[];handles=[];paths={}
def launch(role,url,render=False,extra=None):
 path=out/(label+'-'+role+'.log');paths[role]=path;handle=path.open('w');handles.append(handle)
 cmd=common+[url,'-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1,p.NetShowCorrections 1'] # corrections stay visible in the logs
 if not render:cmd.append('-nullrhi')
 if extra:cmd.extend(extra)
 p=subprocess.Popen(cmd,cwd=root,stdout=handle,stderr=subprocess.STDOUT);children.append(p);return p
def read(role):return paths[role].read_text(errors='replace')
def wait_marker(marker,timeout):
 # Only a dead host aborts early: a failed check still lets the late client run.
 deadline=time.monotonic()+timeout
 while time.monotonic()<deadline:
  if marker in read('host'):return
  if children[0].poll() is not None:raise RuntimeError('Host exited before '+marker)
  time.sleep(.2)
 raise RuntimeError('Timeout before '+marker)
def client(role):
 extra=['-ERCellNetRole='+role]
 if args.loss:extra.extend(['-PktIncomingLoss=3','-PktIncomingLagMin=40','-PktIncomingLagMax=40'])
 return launch(role,'127.0.0.1:'+port+'?Name='+role.capitalize()+'?Primary=0',args.render and role=='initial',extra)
def fields(line):return dict(re.findall(r'(\w+)=(\S+)',line))
try:
 host=launch('host','/Game/EclipseRecoil/Maps/L_DestructionLab?listen?Name=Host?Primary=0?Cells=1',args.render,['-port='+port,'-ERCellNetworkProbe'])
 wait_marker('listening on port '+port,60);client('initial')
 wait_marker('ER_CELL_NET_READY_FOR_LATE_JOIN',180);client('late')
 host.wait(timeout=240)
 logs={r:read(r) for r in paths}
 server=logs['host'];passed=host.returncode==0 and 'ER_CELL_NET_PROBE_COMPLETE result=PASS' in server and 'ER_CELL_NET_PROBE_FAIL' not in server
 # Every state a client applied must be a state the host published: no torn manifests.
 published={}
 for line in server.splitlines():
  if 'ER_CELL_NET_PUBLISHED ' in line:f=fields(line);published[(f['epoch'],f['sequence'])]=f['digest']
 applied={}
 for role,stages in [('initial',range(1,14)),('late',range(8,14))]:
  text=logs[role]
  for stage in stages:
   condition=any('ER_CELL_NET_CLIENT_PASS phase='+str(stage)+' ' in line and 'role='+role in line for line in text.splitlines())
   print('ER_CELL_NET_RUN_'+('PASS' if condition else 'FAIL'),role,stage);passed&=condition
  states=[fields(line) for line in text.splitlines() if 'ER_CELL_NET_APPLIED ' in line]
  torn=[s for s in states if published.get((s['epoch'],s['sequence']))!=s['digest']]
  applied[role]=len(states)
  print('ER_CELL_NET_RUN_'+('PASS' if states and not torn else 'FAIL'),role,'applied_states=%d torn=%d'%(len(states),len(torn)))
  passed&=bool(states) and not torn
  # Unresolvable movement bases (runtime chunk components) make clients drop every server correction.
  for bad in ('ER_CELL_NET_CLIENT_FAIL','ER_BLOCK_NET_INCOMPATIBLE','ER_CELL_NET_GATE_VIOLATION','could not resolve the new relative movement base'):
   if bad in text:print('ER_CELL_NET_RUN_FAIL',role,bad);passed=False
 for role in ('host','initial','late'):
  for line in logs[role].splitlines():
   if any(k in line for k in ('ER_CELL_NET_PROBE_','ER_CELL_NET_READY_','ER_CELL_NET_REPORT ','ER_CELL_NET_GRENADE ','ER_CELL_NET_BYTES ','ER_CELL_NET_PHASE ')):print(line[line.find('ER_CELL_NET_'):])
 joins={f['role']:f for f in (fields(l) for l in (logs['initial']+logs['late']).splitlines() if 'ER_CELL_NET_BYTES initial_join' in l or 'ER_CELL_NET_BYTES late_join' in l)}
 delta=None
 if 'initial' in joins and 'late' in joins:
  delta=int(joins['late']['first_ready_in_bytes'])-int(joins['initial']['first_ready_in_bytes'])
  print('ER_CELL_NET_LATE_JOIN_BYTES initial_first_ready=%s late_first_ready=%s delta=%d late_mask_bytes=%s full_bitmask_bytes=%s shard_chunks=%s'%(
   joins['initial']['first_ready_in_bytes'],joins['late']['first_ready_in_bytes'],delta,joins['late']['mask_bytes'],joins['late']['full_bitmask_bytes'],joins['late']['shard_chunks']))
 report={'result':'PASS' if passed else 'FAIL','render':args.render,'packet_loss':args.loss,'participants':3,'published_states':len(published),'applied_states':applied,
  'late_join':joins.get('late'),'initial_join':joins.get('initial'),'first_ready_bytes_delta':delta,'logs':{r:p.name for r,p in paths.items()},
  'method':'Listen host on L_DestructionLab?Cells=1, initial remote, late remote after drywall and brick grenades and real breach crossings; '
   'clients verify full state digest, per-column collision parity against alive bits, collision gating before the first snapshot, '
   'shard contents equal to the modified chunks, and new epochs after ERCellMode 0/1 and ERBlockRegenerate.'}
 (out/(label+'-report.json')).write_text(json.dumps(report,indent=2)+'\n')
 raise SystemExit(0 if passed else 1)
except (RuntimeError,subprocess.TimeoutExpired) as exc:
 print('FAIL:',exc)
 for role in paths:
  print(role,'\n'+'\n'.join(line for line in read(role).splitlines() if 'ER_CELL_NET_' in line or 'ER_BLOCK_NET_' in line)[-8000:])
 raise SystemExit(1)
finally:
 for p in children:
  if p.poll() is None:p.terminate()
 for p in children:
  try:p.wait(timeout=10)
  except subprocess.TimeoutExpired:p.kill();p.wait()
 for h in handles:h.close()
