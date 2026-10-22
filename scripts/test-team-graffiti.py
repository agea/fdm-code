#!/usr/bin/env python3
"""Real Metal wall/case gallery on each active map; preserves named captures."""
from pathlib import Path
import argparse,subprocess,os,re,shutil,json
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--map',choices=['Depot','CentralBuilding','MiddleEasternAlleys'],default='CentralBuilding');args=p.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'Saved/Testing';out.mkdir(exist_ok=True,parents=True)
exe=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
log=out/('graffiti-'+args.map+'.log')
with log.open('w') as f:
    result=subprocess.run([str(exe),str(root/'RespawnAndRage.uproject'),'/Game/EclipseRecoil/Maps/L_'+args.map,'-game','-ERGraffitiGallery','-unattended','-nosound','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput','-windowed','-ResX=1280','-ResY=720','-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1'],stdout=f,stderr=subprocess.STDOUT,timeout=150)
text=log.read_text(errors='replace');passed=result.returncode==0 and 'ER_GRAFFITI_GALLERY_PASS' in text
counts=re.findall(r'ER_BASE_GRAFFITI team=(\d) count=(\d+)',text)
passed &= len(counts)==2 and all(0<int(n)<=36 for t,n in counts)
poses=re.findall(r'ER_GRAFFITI_CASE_CAPTURE step=(\d) roll=([\d.-]+)',text)
passed &= len(poses)==4 and abs(float(poses[0][1]))<1 and abs(float(poses[1][1]))>45 and abs(float(poses[3][1]))<1
captures=[]
for name in ['GraffitiCase0','GraffitiCase1','GraffitiCase2','GraffitiCase3','GraffitiWall']:
    src=out/(name+'.png');dest=out/('graffiti-'+args.map+'-'+name+'.png')
    if src.exists():shutil.copy2(src,dest);captures.append(str(dest))
    else:passed=False
report={'result':'PASS' if passed else 'FAIL','map':args.map,'counts':counts,'poses':poses,'captures':captures,'log':str(log)}
(out/('graffiti-'+args.map+'-report.json')).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
raise SystemExit(0 if passed else 1)
