from pathlib import Path
import shutil, subprocess,wave,array
p=Path(__file__).resolve().parents[1]/'Assets/weapons/SourceAssets/SonnissGDC2024';p.mkdir(parents=True,exist_ok=True)
import argparse
parser=argparse.ArgumentParser(description='Prepare mono PCM HE/smoke WAVs with a baked smoke fade (macOS afconvert).')
parser.add_argument('source_directory',type=Path)
s=parser.parse_args().source_directory
for name,out in [('grenade1.wav','ER_Grenade_Explosion.wav'),('smoke_1.wav','ER_Smoke_Release.wav')]:
 shutil.copy2(s/name,p/name)
 subprocess.run(['afconvert','-f','WAVE','-d','LEI16@44100','-c','1',str(p/name),'/tmp/rr-convert.wav'],check=True)
 with wave.open('/tmp/rr-convert.wav','rb') as w: samples=array.array('h',w.readframes(w.getnframes()))
 if name=='smoke_1.wav':
  samples=samples[:6*44100]
  for i in range(int(4.5*44100),len(samples)):samples[i]=round(samples[i]*max(0,(len(samples)-1-i)/(1.5*44100)))
 with wave.open(str(p/out),'wb') as w:w.setparams((1,2,44100,0,'NONE','not compressed'));w.writeframes(samples.tobytes())
p.joinpath('README.txt').write_text('User supplied Sonniss GDC 2024 bundle, 17 October 2026. Originals unchanged. Runtime mono PCM16 44100Hz. HE full 2.361s; smoke 6s, linear fade 4.5..6s. Converted with afconvert and Python wave.\n')
