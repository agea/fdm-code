#!/usr/bin/env python3
"""Render ten silhouettes per team and verify every selectable corpse during and after physics."""
from pathlib import Path
import os
import subprocess
import shutil
project=Path(__file__).resolve().parents[1]
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))
editor=engine/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
logs=project/'Saved/Testing'
common=[str(editor),str(project/'RespawnAndRage.uproject'),'/Game/EclipseRecoil/Maps/L_Depot?listen?Name=SkinReview',
    '-game','-windowed','-ResX=1800','-ResY=1000','-NoSplash','-nosound','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60,sg.ShadowQuality 1']
def run(label,flags,complete,passes):
    path=logs/(label+'.log')
    with path.open('w') as handle:
        child=subprocess.Popen(common+flags,stdout=handle,stderr=subprocess.STDOUT)
        try:child.wait(timeout=150)
        finally:
            if child.poll() is None:
                child.terminate()
                try:child.wait(timeout=10)
                except subprocess.TimeoutExpired:child.kill();child.wait()
    output=path.read_text(errors='replace')
    assert child.returncode==0 and complete in output and '_FAIL' not in output,(label,'failure',path)
    assert output.count(passes)==(10 if label.startswith('skins-gallery-final') else 6),(label,'pass count')
    assert 'Failed to compile Material' not in output,(label,'material compilation')
    print('ER_SKINS_RENDER_PASS '+label,flush=True)
for page in range(2):
    run('skins-gallery-final-'+str(page),['-ERSkinGallery','-ERSkinPage='+str(page)],'ER_SKIN_GALLERY_COMPLETE','ER_SKIN_GALLERY_PASS')
for pair in range(10):
    run('skins-corpses-'+str(pair),['-ERCorpseGallery','-ERSkinPair='+str(pair)],'ER_CORPSE_GALLERY_COMPLETE','ER_CORPSE_GALLERY_PASS')
    for sample in range(1,4):
        shutil.copyfile(logs/('Team-Corpses-'+str(sample)+'.png'),logs/('Skin-Corpses-'+str(pair)+'-'+str(sample)+'.png'))
print('ER_SKINS_RENDER_COMPLETE',flush=True)
