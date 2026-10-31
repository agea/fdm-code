"""Warm the shade of the outdoor Alleys and Central Building levels.

Run through UnrealEditor -run=pythonscript. A warm sun, a less blue sky and a
lower hemisphere in the ground's colour keep stone and brick recognizable in
shadow, also on presets without global illumination. Only the two levels change;
rerunning reapplies the values below.
"""
from pathlib import Path
import hashlib,json,zipfile
import unreal as ue
root=Path(ue.Paths.project_dir()).resolve();private=root/'Assets/maps'
maps='/Game/EclipseRecoil/Maps/'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
levels=dict(
    L_MiddleEasternAlleys=dict(sun=(255,246,232),sky=(255,244,234),sky_intensity=1.7,lower=(.24,.21,.17)),
    L_CentralBuilding=dict(sun=(255,244,228),sky=(255,238,222),sky_intensity=1.8,lower=(.20,.19,.17)),
)
files=[private/('Content/EclipseRecoil/Maps/'+n+'.umap') for n in levels]
backup=root/'Saved/MapDaylightBackup/before-map-daylight.zip';backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists():
    with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
        for p in files:z.write(p,p.name)
        z.writestr('hashes.json',json.dumps({p.name:sha(p) for p in files}))
with zipfile.ZipFile(backup) as z:
    for name,digest in json.loads(z.read('hashes.json')).items():
        if hashlib.sha256(z.read(name)).hexdigest()!=digest:raise RuntimeError('Invalid daylight rollback')
def color(rgb):return ue.Color(r=rgb[0],g=rgb[1],b=rgb[2],a=255)
for name,spec in levels.items():
    if not ue.EditorLoadingAndSavingUtils.load_map(maps+name):raise RuntimeError('Cannot load '+name)
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    suns=list(ue.GameplayStatics.get_all_actors_of_class(world,ue.DirectionalLight))
    skies=list(ue.GameplayStatics.get_all_actors_of_class(world,ue.SkyLight))
    if len(suns)!=1 or len(skies)!=1:raise RuntimeError('Unexpected lights in '+name)
    suns[0].light_component.set_editor_property('light_color',color(spec['sun']))
    sky=skies[0].light_component
    sky.set_editor_property('light_color',color(spec['sky']));sky.set_editor_property('intensity',spec['sky_intensity'])
    sky.set_editor_property('lower_hemisphere_is_black',True)
    sky.set_editor_property('lower_hemisphere_color',ue.LinearColor(*spec['lower'],1))
    if not ue.EditorLoadingAndSavingUtils.save_current_level():raise RuntimeError('Cannot save '+name)
print('ER_MAP_DAYLIGHT_SETUP_COMPLETE levels='+str(len(levels)))
