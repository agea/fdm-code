#!/usr/bin/env python3
"""Capture three skins of each hero, each with a random gun and three views."""
from pathlib import Path
import os
import subprocess

project = Path(__file__).resolve().parents[1]
editor = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.8')) / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
output = project / 'Saved/Screenshots/CharacterSet'
output.mkdir(parents=True, exist_ok=True)
with (output / 'capture.log').open('w') as log:
    process = subprocess.Popen([str(editor), str(project / 'RespawnAndRage.uproject'),
        '/Game/EclipseRecoil/Maps/L_Depot?listen?Name=CharacterReview', '-game',
        '-ERTeamGallery', '-ERCharacterShots', '-windowed', '-ResX=1600', '-ResY=1200',
        '-NoSplash', '-nosound', '-unattended', '-stdout', '-FullStdOutLogOutput',
        '-ExecCmds=t.MaxFPS 30,sg.ShadowQuality 1'], stdout=log, stderr=subprocess.STDOUT)
    try:
        process.wait(timeout=360)
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=15)
text = (output / 'capture.log').read_text(errors='replace')
assert process.returncode == 0 and 'ER_CHARACTER_SHOTS_COMPLETE count=36' in text
files = list(output.glob('*/*.png'))
assert len(files) == 36, len(files)
(output / 'manifest.txt').write_text('\n'.join(line.split('ER_CHARACTER_SHOT ', 1)[1] for line in text.splitlines() if 'ER_CHARACTER_SHOT model=' in line) + '\n')
print(f'Captured {len(files)} screenshots: {output}')
