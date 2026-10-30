#!/usr/bin/env python3
"""Exercise real cloth particles through eight movement/pose cases per clothing skin."""
from pathlib import Path
import argparse,json,os,re,subprocess,time
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--skin',type=int,help='Global skin index: Alpha0..9, Omega10..19')
p.add_argument('--render',action='store_true',help='Capture each pose with the real renderer')
p.add_argument('--lods',action='store_true',help='Exercise fast walking at all five render LODs')
p.add_argument('--preview',action='store_true',help='Capture the actual menu preview (Alpha Sniper by default, or --skin) and require completed cloth on every capture')
p.add_argument('--preview-yaw',type=int,default=-25)
p.add_argument('--preview-lod',type=int,choices=range(5),default=0)
p.add_argument('--preview-head',action='store_true',help='Capture a detailed head/shoulders view of the selected skin')
p.add_argument('--tag',default='field-clothing')
a=p.parse_args();project=Path(__file__).resolve().parents[1]
if a.preview and not a.render:p.error('--preview requires --render')
editor=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
folder=project/'Saved/Testing';folder.mkdir(exist_ok=True);log=folder/(a.tag+'.log')
flags=['-windowed','-ResX=1200','-ResY=1000','-ERFieldClothingGallery'] if a.render else ['-nullrhi']
if a.skin is not None:flags.append('-ERFieldClothingSkin='+str(a.skin))
if a.lods:flags.append('-ERFieldClothingLODs')
if a.preview:flags+=['-ERFieldClothingPreview',f'-ERFieldClothingPreviewYaw={a.preview_yaw}',f'-ERFieldClothingPreviewLOD={a.preview_lod}']
if a.preview_head:flags.append('-ERFieldClothingPreviewHead')
started=time.time()
with log.open('w') as stream:
 result=subprocess.run([str(editor),str(project/'RespawnAndRage.uproject'),'/Game/EclipseRecoil/Maps/L_Depot?listen?Name=FieldClothingReview','-game','-NoSplash','-nosound','-unattended','-stdout','-FullStdOutLogOutput','-ERFieldClothingProbe','-ExecCmds=t.MaxFPS 60,r.DynamicRes.OperationMode 0,r.ScreenPercentage 100']+flags,stdout=stream,stderr=subprocess.STDOUT,timeout=300)
text=log.read_text(errors='replace')
if a.preview:
 frames=[dict(frame=int(f),wait=int(w),sim_assets=int(n)) for f,w,n in re.findall(r'ER_CLOTH_PREVIEW_FRAME frame=(\d+) wait=(\d+) sim_assets=(\d+)',text)]
 label=('head-' if a.preview_head else '')+(f'skin-{a.skin}-' if a.skin is not None else '')+'wait'
 captures=folder/'ClothingPreview'/f'{label}-yaw-{a.preview_yaw}-lod-{a.preview_lod}'
 success=result.returncode==0 and len(frames)==36 and all(f['wait']==1 and f['sim_assets']>0 for f in frames) and 'ER_CLOTH_PREVIEW_COMPLETE result=PASS' in text and 'Failed to compile Material' not in text
 success &= all((captures/f"frame-{f['frame']:02d}.png").is_file() and (captures/f"frame-{f['frame']:02d}.png").stat().st_mtime>=started for f in frames)
 (folder/(a.tag+'.json')).write_text(json.dumps(dict(success=success,exit_code=result.returncode,frames=frames,captures=str(captures),log=str(log)),indent=2))
 print('ER_FIELD_CLOTHING_PREVIEW_TEST_COMPLETE result='+('PASS' if success else 'FAIL')+' captures='+str(len(frames)))
 raise SystemExit(0 if success else 1)
rows=[]
for match in re.finditer(r'ER_FIELD_CLOTHING_SAMPLE_(PASS|FAIL) skin=(\d+) case=(\d+) frames=(\d+) free_points=(\d+) deep_points=(\d+) worst_cm=([\d.]+) missing=(\d+)(?: lod=(\d+))?',text):
 state,skin,case,frames,points,deep,worst,missing,lod=match.groups()
 rows.append(dict(state=state,skin=int(skin),case=int(case),frames=int(frames),points=int(points),deep_points=int(deep),worst_cm=float(worst),missing=int(missing),lod=int(lod or 0)))
expected=(1 if a.skin is not None else 9)*(5 if a.lods else 8)
success=result.returncode==0 and len(rows)==expected and all(r['state']=='PASS' and r['deep_points']==0 for r in rows) and 'ER_FIELD_CLOTHING_SIM_COMPLETE result=PASS' in text
if a.render:
 captures=[folder/'Clothing'/(f"skin-{r['skin']}-case-{r['case']}"+(f"-lod-{r['lod']}" if a.lods else '')+'.png') for r in rows]
 success &= all(f.is_file() and f.stat().st_mtime>=started for f in captures)
report=dict(success=success,exit_code=result.returncode,cases=rows,log=str(log))
(folder/(a.tag+'.json')).write_text(json.dumps(report,indent=2))
for r in rows:print('ER_CLOTHING_RESULT',json.dumps(r))
print('ER_FIELD_CLOTHING_TEST_COMPLETE result='+('PASS' if success else 'FAIL'))
raise SystemExit(0 if success else 1)
