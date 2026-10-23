#!/usr/bin/env python3
"""Fire Training warehouse: native layout/physics tests or real Metal captures."""
import argparse,json,os,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--render',action='store_true',help='Launch L_FireTraining and capture plan/spawn/overview/catwalk views')
p.add_argument('--seeds',default='42,73,7919',help='Gallery seeds for --render')
p.add_argument('--offscreen',action='store_true',help='Use real rendering without showing a game window')
args=p.parse_args();args.render|=args.offscreen
root=Path(__file__).resolve().parents[1];out=root/'Saved/Testing';out.mkdir(parents=True,exist_ok=True)
exe=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.8'))/'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
common=[str(exe),str(root/'RespawnAndRage.uproject'),'-nosound','-unattended','-NoSplash','-stdout','-FullStdOutLogOutput']
if not args.render:
    path=out/'fire-training-automation.log'
    with path.open('w') as f:
        r=subprocess.run(common+['-nullrhi','-ExecCmds=Automation RunTests EclipseRecoil.Destruction.FireTraining','-TestExit=Automation Test Queue Empty'],stdout=f,stderr=subprocess.STDOUT,timeout=900)
    text=path.read_text(errors='replace')
    passed=r.returncode==0 and all('Result={Success} Name={'+s+'}' in text for s in ['FireTrainingLayout','FireTrainingPhysics']) and 'Result={Fail}' not in text
    print('\n'.join(x for x in text.splitlines() if 'ER_FIRE_TRAINING_' in x or 'Result={' in x));print('Log:',path)
    raise SystemExit(0 if passed else 1)
path=out/'fire-training-metal.log'
command=common+['/Game/EclipseRecoil/Maps/L_FireTraining?Name=Host?MapSeed=42','-game','-windowed','-ResX=1600','-ResY=900','-ERFireTrainingProbe','-ERFireTrainingSeeds='+args.seeds,
                '-ExecCmds=t.MaxFPS 60,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1']+(['-RenderOffscreen'] if args.offscreen else [])
with path.open('w') as f:r=subprocess.run(command,stdout=f,stderr=subprocess.STDOUT,timeout=600)
text=path.read_text(errors='replace')
passed=r.returncode==0 and 'ER_FIRE_PROBE_COMPLETE result=PASS' in text
for line in text.splitlines():
    if 'ER_FIRE_' in line or 'ER_RECON_RESET' in line or 'ER_RECON_BASE' in line:print(line)
shots=sorted(str(x) for x in out.glob('FireTraining-*.png'))
(out/'fire-training-report.json').write_text(json.dumps(dict(result='PASS' if passed else 'FAIL',captures=shots,
    galleries=re.findall(r'ER_FIRE_GALLERY [^\n\[]*',text),log=str(path)),indent=2)+'\n')
print('Captures:',*shots,sep='\n  ')
raise SystemExit(0 if passed else 1)
