"""Use the approved Grenadier skirt surface on every Phase appearance.

Keep cloth3, all mesh/LOD geometry, body materials, FPS, head choices and gameplay.
Only the three appearance catalogs may be saved. Run through Unreal Python.
"""
from datetime import datetime
from pathlib import Path
import hashlib,json,re,zipfile
import unreal as ue

assets=ue.EditorAssetLibrary
root=Path(ue.Paths.project_dir()).resolve()
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
marker='ERPhaseGrenadierSkirtVersion'
def file(path):return root/'Content'/(path.removeprefix('/Game/')+'.uasset')
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def slots(appearance):return [i for i,s in enumerate(appearance.mesh.materials) if 'leather_skirt' in str(s.material_slot_name).lower()]
def state(appearance):
    data={k:re.sub(r' \(0x[0-9a-fA-F]+\)','',str(appearance.get_editor_property(k))) for k in ['display_name','mesh','first_person_mesh','animation_class','corpse_pose','hidden_bones','hidden_material_slots','attachments','world_support_palm_offset_cm','copy_physics_pose_for_rendering','scale','feet_offset']}
    excluded=slots(appearance) if appearance.mesh and 'SK_ER_Phase_' in appearance.mesh.get_name() else []
    data['materials']=[(i,m.get_path_name() if m else None) for i,m in enumerate(appearance.materials) if i not in excluded]
    return data
protected={p:digest(p) for p in (root/'Assets/characters/Content').rglob('*.uasset')}
backup=root/'Saved/AssetBackups'/('PhaseGrenadierSkirt-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
    for path in paths:z.write(file(path),str(file(path).relative_to(root)))
with zipfile.ZipFile(backup) as z:assert z.testzip() is None
catalog=ue.get_default_object(assets.load_blueprint_class(paths[0]));source=catalog.omega_skins[8]
assert source.display_name=='Phase · Engineer' and len(slots(source))==1
material=source.materials[slots(source)[0]]
assert '/PhaseEngineer/' in material.get_path_name()
changes=[]
for path in paths:
    bp=assets.load_asset(path);c=ue.get_default_object(bp.generated_class())
    before={k:[state(a) for a in getattr(c,k)] for k in ['alpha_skins','omega_skins']}
    fallbacks={k:state(getattr(c,k)) for k in ['alpha_appearance','omega_appearance']}
    choices=list(c.omega_skins)
    for index in range(5,10):
        a=choices[index];assert len(slots(a))==1
        assert assets.get_metadata_tag(a.mesh,'ERFieldClothingVersion')=='3'
        materials=list(a.materials);slot=slots(a)[0]
        if materials[slot]!=material:
            changes.append(dict(catalog=path,skin=index,slot=slot,previous=materials[slot].get_path_name()))
            materials[slot]=material;a.set_editor_property('materials',materials);choices[index]=a
    c.set_editor_property('omega_skins',choices)
    assert before=={k:[state(a) for a in getattr(c,k)] for k in before}
    assert fallbacks=={k:state(getattr(c,k)) for k in fallbacks}
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assets.set_metadata_tag(bp,marker,'1');assert assets.save_loaded_asset(bp,False)
assert all(digest(p)==old for p,old in protected.items()),'Protected character asset changed'
(root/'Saved/Testing/phase-grenadier-skirt-setup.json').write_text(json.dumps(dict(material=material.get_path_name(),changes=changes,backup=str(backup),protected=len(protected)),indent=2))
ue.log('ER_PHASE_GRENADIER_SKIRT_SETUP_COMPLETE catalogs=3 changed=%d protected=%d'%(len(changes),len(protected)))
