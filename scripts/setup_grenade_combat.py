"""Migrate HE/Milkor gameplay defaults through Unreal, preserving all art."""
import json
from pathlib import Path
import unreal as ue
root='/Game/EclipseRecoil/Weapons'
updated=[]
fuse_updated=[]
for path in ue.EditorAssetLibrary.list_assets(root,recursive=True):
    # Load only weapon data assets; meshes/textures/animations are untouched.
    if '/DA_' not in path:continue
    data=ue.load_asset(path)
    if not isinstance(data,ue.ERWeaponData):continue
    payload=data.get_editor_property('payload')
    if payload not in (ue.ERPayload.HE,ue.ERPayload.LAUNCHED_HE,ue.ERPayload.SMOKE,ue.ERPayload.STUN):continue
    data.set_editor_property('fuse_duration',3.5)
    data.set_editor_property('projectile_speed',1750 if payload==ue.ERPayload.LAUNCHED_HE else 880)
    data.set_editor_property('launch_range_multiplier',1. if payload==ue.ERPayload.LAUNCHED_HE else 1.2)
    if payload!=ue.ERPayload.LAUNCHED_HE:
        data.set_editor_property('magazine_size',10)
        data.set_editor_property('initial_reserve',0)
    if payload in (ue.ERPayload.HE,ue.ERPayload.LAUNCHED_HE):
        for key,value in dict(damage=140,explosion_inner_radius=200,explosion_radius=600,armor_ratio=1.6).items():data.set_editor_property(key,value)
        updated.append(path)
    if not ue.EditorAssetLibrary.save_loaded_asset(data,False):raise RuntimeError('Cannot save '+path)
    fuse_updated.append(path)
if len(updated)<3:raise RuntimeError('Expected owned HE, Milkor and legacy HE profiles: '+str(updated))
ue.log('ER_GRENADE_COMBAT_READY '+str(updated))

report={'profiles':updated,'fuse_profiles':fuse_updated,'hand_grenade_test_stock':10,'hand_speed_cm_s':880,'milkor_speed_cm_s':1750,'peak_damage':140,'inner_radius_cm':200,'outer_radius_cm':600,'falloff':'quadratic','maximum_armor_reduction':.2,'cover_samples':['head','torso','lower body'],'structural_radius_cm':350,'structural_damage_multiplier':3,'hand_horizontal_range_multiplier':1.2,'scoped_launcher_horizontal_range_multiplier':1.5,'cook':'press arms, release throws same projectile','fuse_seconds':3.5,'origin_offset_cm':[-14,24,-4],'arm_hidden_on_press':True,'death_drops_live_grenade':True}
folder=Path(ue.Paths.project_dir())/'Assets/weapons/SourceAssets/EpicContentExamples'
folder.mkdir(parents=True,exist_ok=True)
(folder/'combat.json').write_text(json.dumps(report,indent=2)+'\n')
