#!/usr/bin/env python3
"""Prepare real, isolated CC0 firearm takes and attributed surface footsteps.

Requires numpy, macOS afconvert and sndfile-convert (for the original OGGs).
Original archives, creator-page snapshots, licenses and source hashes stay private.
Does not modify the Sonniss WAVs or import/save Unreal packages.
"""
from pathlib import Path
import hashlib,json,subprocess,tempfile,wave
import numpy as np

root=Path(__file__).resolve().parents[1]/'Assets/weapons/SourceAssets/AudioPass'
fire=root/'FreeFirearm/original/Prepared SFX Library'
out=root/'derived';out.mkdir(exist_ok=True)

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):
    with wave.open(str(p)) as w:
        sr=w.getframerate();ch=w.getnchannels();width=w.getsampwidth();raw=w.readframes(w.getnframes())
    if width==3:
        b=np.frombuffer(raw,np.uint8).reshape(-1,3).astype(np.int32)
        v=b[:,0]|(b[:,1]<<8)|(b[:,2]<<16);v=(v^0x800000)-0x800000
        x=v.astype(np.float32)/8388608
    elif width==2:x=np.frombuffer(raw,'<i2').astype(np.float32)/32768
    else:raise ValueError('Unsupported source width')
    return x.reshape(-1,ch).mean(axis=1),sr

def write(p,x,sr):
    with wave.open(str(p),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(sr)
        w.writeframes((np.clip(x,-1,.999969)*32768).astype('<i2').tobytes())

records=[]
def render(p,name,start=None,duration=1.1,step=False):
    x,sr=read(p)
    begin=0 if start is None else max(0,int((start-.006)*sr))
    end=min(len(x),begin+int(duration*sr));x=x[begin:end].copy()
    clipped=float(np.mean(np.abs(x)>.999))
    # Align to the attack, preserving a 6ms lead; no attack fade or synthetic copies.
    if step:
        active=np.flatnonzero(np.abs(x)>max(.002,float(abs(x).max())*.025))
        if len(active):
            offset=max(0,int(active[0])-int(.006*sr));stop=min(len(x),int(active[-1])+int(.05*sr))
            x=x[offset:stop];end=begin+stop;begin+=offset
    x-=float(np.mean(x))
    rms=float(np.sqrt(np.mean(x[:int(sr*.1)]**2)))
    target=10**((-22 if step else -15)/20)
    gain=min(target/max(rms,1e-6),10**((-6 if step else -1)/20)/max(float(abs(x).max()),1e-6))
    x*=gain
    fade=min(len(x),int(sr*(.025 if step else .055)));x[-fade:]*=np.linspace(1,0,fade)
    dest=out/(name+'.wav')
    with tempfile.TemporaryDirectory() as tmp:
        intermediate=Path(tmp)/'clip.wav';write(intermediate,x,sr)
        subprocess.run(['/usr/bin/afconvert','-f','WAVE','-d','LEI16@48000',str(intermediate),str(dest)],check=True)
    cooked,rate=read(dest)
    record=dict(name=name,source=str(p.relative_to(root)) if p.is_relative_to(root) else p.name,source_sha256=sha(p),source_start_frame=begin,source_end_frame=end,
                source_rate=sr,source_clipped_fraction=clipped,gain=gain,file=str(dest.relative_to(root)),sha256=sha(dest),
                duration=len(cooked)/rate,rate=rate,channels=1,peak_dbfs=20*np.log10(max(float(abs(cooked).max()),1e-9)),
                attack_rms_dbfs=20*np.log10(max(float(np.sqrt(np.mean(cooked[:4800]**2))),1e-9)))
    records.append(record);return name

# Each timestamp is a separate semi-auto transient in a real recording. Burst
# recordings are deliberately excluded: no chopped rapid fire tails masquerading
# as independent takes. Carbine/Sniper are stable serialized keys, not UI labels.
shots={
 'Pistol':[('1911/A_34P.wav',[1.542,6.654]),('1911/A_42P.wav',[.940,5.000])],
 'SMG':[('Carl Gustav M45/G_20P.wav',[.346,2.258,5.266]),('Carl Gustav M45/G_31P.wav',[.310,3.500,6.726])],
 'Shotgun':[('Nova/O_17P.wav',[.692,3.708]),('Nova/O_21P.wav',[.430,3.464])],
 'Assault':[('AK-47/C_28P.wav',[.610,3.256,6.020,9.156]),('AK-47/C_31P.wav',[.356,4.418])],
 'Carbine':[('Tikka/W_24P.wav',[.754,5.376]),('Tikka/W_29P.wav',[.576,5.664])],
 'Sniper':[('SKS/U_14P.wav',[3.576,6.754,10.210,13.290])],
}
palettes={}
for key,takes in shots.items():
    names=[]
    for relative,onsets in takes:
        for onset in onsets:names.append(render(fire/relative,f'S_ER_{key}_{len(names)+1:02}',onset,duration=.75 if key=='SMG' else 1.1))
    palettes[key]=names
steps={}
pack=root/'SurfaceSteps/original/footsteps'
# These are shoe/boot recordings: the robot-foot Thimras pack remains source-only.
for surface,folder in [('Hard','boots'),('Wood','wood'),('Metal','metal'),('Grass','grass'),('Gravel','gravel')]:
    names=[]
    for p in sorted((pack/folder).glob('*.ogg'))[:8]:
        with tempfile.TemporaryDirectory() as tmp:
            converted=Path(tmp)/p.with_suffix('.wav').name
            subprocess.run(['/opt/homebrew/bin/sndfile-convert','-pcm16',str(p),str(converted)],check=True,stdout=subprocess.DEVNULL)
            name=render(converted,f'S_ER_Step_{surface}_{len(names)+1:02}',duration=1,step=True)
            records[-1].update(source=str(p.relative_to(root)),source_sha256=sha(p),source_license=str((pack/folder/'license.txt').relative_to(root)))
            names.append(name)
    steps[surface]=names
# One small, non-squeaking stone recording for glass/drywall; documented single
# sample fallback rather than fabricating a new collection via pitch shifts.
p=root/'TinyWorlds/original/stone01.ogg'
with tempfile.TemporaryDirectory() as tmp:
    converted=Path(tmp)/'stone.wav'
    subprocess.run(['/opt/homebrew/bin/sndfile-convert','-pcm16',str(p),str(converted)],check=True,stdout=subprocess.DEVNULL)
    name=render(converted,'S_ER_Step_LightHard_01',duration=1,step=True)
    records[-1].update(source=str(p.relative_to(root)),source_sha256=sha(p))
steps['LightHard']=[name]
# Match attack loudness within each identity after resampling. Quiet, high-crest
# originals constrain the target: attenuate louder takes instead of hard limiting
# or clipping the sharp transient to boost a distant recording.
by_name={r['name']:r for r in records}
for names in list(palettes.values())+list(steps.values()):
    target=min(by_name[n]['attack_rms_dbfs'] for n in names)+.75
    for name in names:
        r=by_name[name];dest=root/r['file'];x,sr=read(dest)
        scale=min(1.,10**((target-r['attack_rms_dbfs'])/20),10**(-1/20)/max(float(abs(x).max()),1e-9))
        if scale<1:
            x*=scale;write(dest,x,sr);x,sr=read(dest)
            r.update(gain=r['gain']*scale,sha256=sha(dest),peak_dbfs=20*np.log10(max(float(abs(x).max()),1e-9)),attack_rms_dbfs=20*np.log10(max(float(np.sqrt(np.mean(x[:4800]**2))),1e-9)))
manifest=dict(revision=1,date='2026-10-21',firearms=palettes,footsteps=steps,records=records,limits=[
 'Milkor: no grenade launcher recording in the approved packs; one existing Epic launcher sample retained with subtle modulation.',
 'LightHard: one TinyWorlds stone sample; Dirt uses Grass, Snow uses Gravel as explicit provisional fallbacks, without claiming dedicated recordings.',
 'Thimras robot/metal-foot-on-concrete pack not used for human boots.',
 'Tabasco excluded: page says CC0 but included creativecommons.txt says copyright Vincent Sevedge, CC BY 3.0.',
 'Source waveforms/transients/levels inspected numerically; subjective headphone A/B remains an explicit listening check.'
])
(root/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
# All originals, not just imported ones, retain an integrity inventory.
files=[]
for p in sorted(root.rglob('*')):
    if p.is_file() and 'derived' not in p.parts and p.name!='source-hashes.json':files.append(dict(file=str(p.relative_to(root)),sha256=sha(p),bytes=p.stat().st_size))
(root/'source-hashes.json').write_text(json.dumps(files,indent=2)+'\n')
print('ER_AUDIO_PREPARED', {k:len(v) for k,v in palettes.items()}, {k:len(v) for k,v in steps.items()})
