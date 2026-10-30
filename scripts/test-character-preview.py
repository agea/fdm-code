#!/usr/bin/env python3
"""Capture actual AERMenuPreview walks and camera metrics, including non-cloth skins."""
from pathlib import Path
import argparse,subprocess,re,json,os,shutil
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--skin',type=int,action='append',required=True);p.add_argument('--head',action='store_true',help='Capture head/shoulders using the real menu preview');p.add_argument('--no-texture-streaming',action='store_true',help='Diagnostic full-resolution texture capture');args=p.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'Saved/Testing/WraithReplacement/Preview';out.mkdir(parents=True,exist_ok=True)
editor=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
for skin in args.skin:
 label=('head-' if args.head else '')+f'skin-{skin}'
 log=out/f'{label}.log'
 with log.open('w') as stream:
  r=subprocess.run([str(editor),str(root/'RespawnAndRage.uproject'),'/Game/EclipseRecoil/Maps/L_Depot?listen?Name=PreviewCompare','-game','-windowed','-ResX=1000','-ResY=900','-nosound','-unattended','-stdout','-FullStdOutLogOutput','-ERFieldClothingProbe','-ERFieldClothingPreview','-ERCharacterPreview',f'-ERFieldClothingSkin={skin}','-ERFieldClothingPreviewYaw=-25','-ExecCmds=t.MaxFPS 60,r.DynamicRes.OperationMode 0,r.ScreenPercentage 100'+(',r.TextureStreaming 0' if args.no_texture_streaming else '')]+(['-ERFieldClothingPreviewHead'] if args.head else []),stdout=stream,stderr=subprocess.STDOUT,timeout=180)
 text=log.read_text(errors='replace')
 rows=[dict(skin=int(s),frame=int(f),scale=float(scale),camera_cm=float(camera),head_z=float(head),foot_z=float(foot)) for s,f,scale,camera,head,foot in re.findall(r'ER_CHARACTER_PREVIEW_MEASURE skin=(\d+) frame=(\d+) scale=([\d.]+) camera_cm=([\d.]+) head_z=([-\d.]+) foot_z=([-\d.]+)',text)]
 assert r.returncode==0 and len(rows)==36 and 'ER_CLOTH_PREVIEW_COMPLETE result=PASS' in text and 'Failed to compile Material' not in text,(skin,log)
 source=root/'Saved/Testing/ClothingPreview'/((('head-' if args.head else '')+f'character-skin-{skin}-wait-yaw--25-lod-0'))
 for frame in [0,12,24]:shutil.copyfile(source/f'frame-{frame:02d}.png',out/f'{label}-frame-{frame:02d}.png')
 (out/f'{label}.json').write_text(json.dumps(rows,indent=2));print('ER_CHARACTER_PREVIEW_PASS skin='+str(skin),flush=True)
