#!/usr/bin/env python3
"""Profile and stress real block ISM/Chaos operations in the saved procedural map.

Sequential deterministic cases: two seeds x three chunk sizes, one warmup then
measured samples. Raw CSV and summaries stay in Saved/Testing. No packaging.
"""
from pathlib import Path
import argparse
import csv
import json
import math
import os
import re
import statistics
import subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--render',action='store_true',help='Use Metal, exercise real FX and capture the stress screenshot')
parser.add_argument('--benchmark-only',action='store_true',help='Skip real weapon/FX phases; used for baseline comparisons')
parser.add_argument('--samples',type=int,default=8,help='Measured samples per seed/chunk, 1..30 (plus one warmup)')
parser.add_argument('--label',default='optimized',help='Output label: lowercase letters, digits, underscore or hyphen')
parser.add_argument('--trace',action='store_true',help='Capture CPU/frame/bookmark scopes for Unreal Insights')
args=parser.parse_args()
if not 1<=args.samples<=30 or not re.fullmatch('[a-z0-9_-]{1,40}',args.label): parser.error('Invalid samples/label')
project=Path(__file__).resolve().parents[1]
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
editor=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=project/'Saved/Testing';logs.mkdir(parents=True,exist_ok=True)
rhi='metal' if args.render else 'nullrhi'
label='destruction-benchmark-'+args.label+'-'+rhi
log=logs/(label+'.log'); raw=logs/(label+'.csv')
command=[str(editor),str(project/'fdm.uproject'),'/Game/EclipseRecoil/Maps/L_ProceduralLab?Primary=0',
 '-game','-ERBlockStressProbe','-ERBlockBenchmarkLabel='+args.label,'-ERBlockBenchmarkSamples='+str(args.samples),
 '-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-windowed','-ResX=1280','-ResY=720',
 '-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1']
if not args.render:command.append('-nullrhi')
if args.benchmark_only:command.append('-ERBlockBenchmarkOnly')
if args.trace:command.extend(['-trace=cpu,frame,bookmark','-tracefile='+str(logs/(label+'.utrace'))])
with log.open('w') as output:
 try: result=subprocess.run(command,stdout=output,stderr=subprocess.STDOUT,timeout=600)
 except subprocess.TimeoutExpired: raise SystemExit('FAIL: stress run timed out; inspect '+str(log))
output=log.read_text(errors='replace')
for line in output.splitlines():
 if 'ER_BLOCK_STRESS_' in line or 'ER_BLOCK_BENCHMARK_CASE' in line:print(line)
passed=result.returncode==0 and 'ER_BLOCK_STRESS_COMPLETE result=PASS' in output and 'ER_BLOCK_STRESS_FAIL' not in output
if not passed:raise SystemExit('FAIL: inspect '+str(log))
with raw.open() as stream: rows=list(csv.DictReader(stream))
for row in rows:
 if int(row['tracked_bytes'])!=sum(int(row[key]) for key in ['logical_bytes','layout_bytes','mapping_bytes','instance_bytes','support_bytes','network_bytes']):
  raise SystemExit('FAIL: tracked-memory columns do not add up')
expected=args.samples*6*53
if len(rows)!=expected:raise SystemExit(f'FAIL: CSV contains {len(rows)} rows, expected {expected}')
summary={'label':args.label,'rhi':rhi,'samples_per_case':args.samples,'warmup_per_case':1,'cases':[],
 'method':'Game-thread synchronous calls. ISM clear/add include synchronous Chaos. Excludes deferred physics/render/GPU and validation, GC, logging and memory snapshots. Tracked bytes are exclusive reported ISM resources plus owned arrays/maps, support graph and network snapshot buffers. ISM body counts exclude temporary debris, reported separately; debris engine/Chaos heaps, shared meshes/textures, total engine heaps and GPU allocations are excluded. Process resident bytes include the entire Unreal process.'}
for seed in [42,7919]:
 for chunk in [200,400,800]:
  case={'seed':seed,'rooms':10,'chunk_cm':chunk,'operations':{}}
  for operation in sorted({r['operation'] for r in rows}):
   selected=[r for r in rows if int(r['seed'])==seed and int(r['chunk_cm'])==chunk and r['operation']==operation]
   if not selected:continue
   values=sorted(float(r['total_ms']) for r in selected)
   profile={'n':len(values),'median_ms':round(statistics.median(values),6),'p95_ms':round(values[math.ceil(len(values)*.95)-1],6),'max_ms':round(max(values),6)}
   for key in ['logical_ms','clear_ms','add_ms','mapping_ms','groups','uploaded','removed','candidates','examined','tracked_bytes','logical_bytes','layout_bytes','mapping_bytes','instance_bytes','collision_bodies','components','resident_bytes','support_bytes','debris','simulating_debris','network_bytes']:
    profile['median_'+key]=statistics.median(float(r[key]) for r in selected)
   case['operations'][operation]=profile
  summary['cases'].append(case)
(logs/(label+'-summary.json')).write_text(json.dumps(summary,indent=2)+'\n')
for case in summary['cases']:
 op=case['operations'];print(f"seed={case['seed']} chunk={case['chunk_cm']} generation={op['generation']['median_ms']:.3f}ms single_p95={op['single']['p95_ms']:.3f}ms batch256={op['batch256']['median_ms']:.3f}ms tracked={op['generation']['median_tracked_bytes']/1048576:.2f}MiB")
print('CSV:',raw,'\nLog:',log)
