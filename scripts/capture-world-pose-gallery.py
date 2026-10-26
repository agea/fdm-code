#!/usr/bin/env python3
"""Capture all 20 playable characters in the neutral rifle pose from four views."""
from pathlib import Path
import os
import subprocess
import time
from PIL import Image,ImageDraw,ImageFont

project=Path(__file__).resolve().parents[1]
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
editor=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
testing=project/'Saved/Testing';individual=testing/'WorldPoses';gallery=testing/'WorldPoseAll'
individual.mkdir(parents=True,exist_ok=True);gallery.mkdir(parents=True,exist_ok=True)
log=testing/'world-pose-all-gallery.log';started=time.time()
command=[str(editor),str(project/'RespawnAndRage.uproject'),
    '/Game/EclipseRecoil/Maps/L_Depot?ERFullArsenalTest?ERAlphaBots=1?EROmegaBots=0?ERBotMode=Target?Name=PoseGallery',
    '-game','-nosound','-unattended','-NoSplash','-Multiprocess','-stdout','-FullStdOutLogOutput',
    '-windowed','-ResX=1000','-ResY=1000','-ExecCmds=t.MaxFPS 60,r.DynamicRes.OperationMode 0,r.ScreenPercentage 100',
    '-ERWorldPoseProbe','-ERWorldPoseExternal','-ERWorldPoseGallery','-ERWorldPoseAllGallery']
with log.open('w') as output:
    result=subprocess.run(command,stdout=output,stderr=subprocess.STDOUT,timeout=240)
text=log.read_text(errors='replace')
if result.returncode or 'ER_WORLD_POSE_COMPLETE result=PASS samples=20' not in text or 'ER_WORLD_POSE_FAIL' in text:
    raise SystemExit('World pose gallery failed; see '+str(log))
files={(skin,view):individual/f'Skin{skin:02d}-Case03-View{view}.png' for skin in range(20) for view in range(4)}
missing=[str(path) for path in files.values() if not path.exists() or path.stat().st_mtime<started]
if missing:raise SystemExit('Missing fresh captures:\n'+'\n'.join(missing))

try:font=ImageFont.truetype('/System/Library/Fonts/Helvetica.ttc',22)
except OSError:font=ImageFont.load_default()
names=[f'Alpha {i} ({"Twinblast" if i<5 else "Belica"})' for i in range(10)]
names += [f'Omega {i} ({"Drongo" if i<5 else "Phase"})' for i in range(10)]
view_names=['tre-quarti frontale','frontale','tre-quarti posteriore','dall’alto']
tile_w,tile_h,label_h=300,390,36
for view,title in enumerate(view_names):
    sheet=Image.new('RGB',(tile_w*5,(tile_h+label_h)*4),(24,27,31));draw=ImageDraw.Draw(sheet)
    for skin in range(20):
        source=Image.open(files[skin,view]).convert('RGB');w,h=source.size
        crop=source.crop((int(w*.22),int(h*.04),int(w*.78),int(h*.98)))
        crop.thumbnail((tile_w,tile_h),Image.Resampling.LANCZOS)
        x=(skin%5)*tile_w+(tile_w-crop.width)//2;y=(skin//5)*(tile_h+label_h)+(tile_h-crop.height)//2
        sheet.paste(crop,(x,y));label=names[skin];box=draw.textbbox((0,0),label,font=font)
        draw.text(((skin%5)*tile_w+(tile_w-(box[2]-box[0]))//2,(skin//5)*(tile_h+label_h)+tile_h+5),label,fill=(235,238,242),font=font)
    path=gallery/f'Tutti-personaggi-{view}-{title}.png';sheet.save(path,optimize=True)
    print(path)
print('ER_WORLD_POSE_ALL_GALLERY PASS captures=80 sheets=4')
