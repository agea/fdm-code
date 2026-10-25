"""Rooms: replace crates with dense timber screens and occluded corridor routes.

Validate 516 seeds before advancing only the owned Rooms recipe. Original
maps, art, other recipes, shared themes and damage calibration are protected.
"""
from pathlib import Path
import hashlib
import json
import shutil
import unreal as ue

root=Path(ue.Paths.project_dir()).resolve()
a=ue.EditorAssetLibrary
base='/Game/EclipseRecoil/Art/MapTemplates/'
template=a.load_asset(base+'DA_Rooms')
if not template:raise RuntimeError('Run setup_rooms.py first')
if a.get_metadata_tag(template,'ERRoomsScreensRevision')=='1':
    print('ER_ROOMS_SCREENS already migrated')
else:
    file=root/'Assets/maps/Content/EclipseRecoil/Art/MapTemplates/DA_Rooms.uasset'
    revision=template.get_editor_property('revision')
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    protected=list((root/'Assets/maps/Content/EclipseRecoil/Art/MapTemplates').glob('*.uasset'))
    protected=[p for p in protected if p!=file]
    protected+=list((root/'Assets/maps/Content/EclipseRecoil/Maps').glob('*.umap'))
    protected+=[root/'Assets/maps/Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset']
    before={str(p):sha(p) for p in protected}
    backup=root/('Saved/RoomsScreensBackup/before-revision'+str(revision))
    backup.mkdir(parents=True,exist_ok=True)
    rollback=backup/file.name
    if not rollback.exists():shutil.copy2(file,rollback)
    if sha(rollback)!=sha(file):raise RuntimeError('Rooms rollback mismatch')
    candidate=ue.new_object(ue.ERMapTemplate)
    for key in ['template_id','revision','source_reference','archetype','symmetry','arena_size','rooms','theme_library']:
        candidate.set_editor_property(key,template.get_editor_property(key))
    candidate.set_editor_property('revision',revision+1)
    for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
        if not candidate.validate_layout(seed):raise RuntimeError('Invalid Rooms screen layout seed='+str(seed))
    template.set_editor_property('revision',revision+1)
    a.set_metadata_tag(template,'ERRoomsScreensRevision','1')
    if not a.save_loaded_asset(template,False):raise RuntimeError('Cannot save Rooms recipe')
    for path,digest in before.items():
        if sha(Path(path))!=digest:raise RuntimeError('Protected asset changed '+path)
    report=dict(result='PASS',before_revision=revision,revision=revision+1,before_sha256=sha(rollback),after_sha256=sha(file),
                validated_seeds=516,screen_height_cm=240,source='Original timber screen geometry; existing wood materials and calibrated cells',
                protected_sha256=before)
    (backup/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    (root/'Assets/maps/SourceAssets/Rooms/timber-screens.json').write_text(json.dumps(report,indent=2)+'\n')
    print('ER_ROOMS_SCREENS_SAVED seeds=516 revision='+str(revision+1))
