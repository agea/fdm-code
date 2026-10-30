"""Legacy entry point: preserve the latest hairless Phase Sniper request.

The previous long-hair materials remain available for rollback, but this helper
must no longer restore their visibility or regenerate those material assets.
"""
from datetime import datetime
from pathlib import Path
import zipfile
import unreal as ue

assets=ue.EditorAssetLibrary
project=Path(ue.Paths.project_dir()).resolve()
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter',
       '/Game/EclipseRecoil/Core/BP_ERTrainingTarget',
       '/Game/EclipseRecoil/Core/BP_ERCorpse']
backup=project/'Saved/AssetBackups'/('PhaseSniperHairless-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as archive:
    for path in paths:
        filename=project/'Content'/(path.removeprefix('/Game/')+'.uasset')
        archive.write(filename,str(filename.relative_to(project)))
with zipfile.ZipFile(backup) as archive:assert archive.testzip() is None
hidden_material=assets.load_asset('/Game/EclipseRecoil/Characters/Teams/Gear/M_ER_HiddenSection')
assert hidden_material
for path in paths:
    bp=assets.load_asset(path);defaults=ue.get_default_object(bp.generated_class())
    choices=list(defaults.omega_skins);appearance=choices[5]
    assert appearance.display_name=='Phase · Recon'
    hidden=set(appearance.hidden_material_slots);materials=list(appearance.materials)
    for slot,section in enumerate(appearance.mesh.materials):
        if any(part in str(section.material_slot_name).lower() for part in ('hair_long','hair_short')):
            hidden.add(slot);materials[slot]=hidden_material
    appearance.set_editor_property('hidden_material_slots',sorted(hidden))
    appearance.set_editor_property('materials',materials);choices[5]=appearance
    defaults.set_editor_property('omega_skins',choices)
    assets.set_metadata_tag(bp,'ERPhaseReconLongHairVersion','removed')
    assets.set_metadata_tag(bp,'ERPhaseSniperHairVersion','hairless-1')
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assert assets.save_loaded_asset(bp,False)
ue.log('ER_PHASE_SNIPER_HAIRLESS_COMPLETE catalogs=3 backup='+str(backup))
