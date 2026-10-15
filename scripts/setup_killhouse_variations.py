"""Explicit revision3 migration: solid walk surfaces and seeded compositions.

Run through Unreal after compilation. Validates a detached candidate before
saving; completed revision metadata preserves subsequent artist edits.
"""
import hashlib,json
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir());private=root/'Assets/maps';assets=ue.EditorAssetLibrary
path='/Game/EclipseRecoil/Art/MapTemplates/DA_Killhouse';template=assets.load_asset(path)
if not template:raise RuntimeError('Create the Killhouse template first')
source=private/'SourceAssets/MapTemplates/killhouse_variations.json'
recipe=dict(revision=3,arena_min=[3600,7000],arena_max=[4800,9000],base_width=[1000,1400],base_depth=[850,1050],min_rows=2,max_rows=3,
            modules=['ROOM','L_COVER','U_COVER','STAGGERED_WALLS','CARGO','TANKS','CRATES'],
            palette=dict(cargo_mesh='SM_Container01_01',tank_mesh='SM_WaterContainer01',barrel_mesh='SM_Barrel02',column_mesh='SM_PlatformPillar_01',beam_mesh='SM_MetalBeam15',guard_mesh='SM_MetalWallPart_01'),
            invariants=['HalfTurn symmetry','rear ground entry','two front firing windows','internal stairs rise <=20cm','solid roof and treads','protected roof and stairwell sides','two through doors per room','reserved outer circulation'])
if not source.exists():source.write_text(json.dumps(recipe,indent=2)+'\n')
recipe=json.loads(source.read_text());recipe_hash=hashlib.sha256(source.read_bytes()).hexdigest()
if recipe['revision']!=3:raise RuntimeError('Unsupported variation recipe revision')
marker=assets.get_metadata_tag(template,'ERTemplateVariationRevision')
previous=assets.get_metadata_tag(template,'ERVariationManifestSHA256')
if marker and (marker!='3' or previous!=recipe_hash):raise RuntimeError('Completed variation recipe changed: use an explicit new revision')
catalog=private/'Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset'
map_file=private/'Content/EclipseRecoil/Maps/L_Killhouse.umap'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before=sha(catalog);map_before=sha(map_file)
if marker=='3':
    ue.log('ER_KILLHOUSE_VARIATION_PRESERVED revision3')
else:
    candidate=ue.new_object(ue.ERMapTemplate)
    fields=['template_id','source_reference','revision','symmetry','shell_wall_mesh','roof_mesh','arena_size','pieces','alpha_spawns','omega_spawns','patrol_route','variation']
    for field in fields:candidate.set_editor_property(field,template.get_editor_property(field))
    candidate.set_editor_property('revision',3)
    candidate.set_editor_property('symmetry',ue.ERTemplateSymmetry.HALF_TURN)
    # The reference assembly remains available when disabling variation. Fix its
    # old detailed platform/tread meshes too; other geometry/art stays intact.
    parts=candidate.get_editor_property('pieces')
    for part in parts:
        mesh=part.get_editor_property('fixed_mesh')
        if mesh and mesh.get_name() in ['SM_IndustrialPlatform02','SM_PlatformGrill_01'] and not part.get_editor_property('destructible'):
            part.set_editor_property('fixed_mesh',None);part.set_editor_property('walkable_surface',True)
    candidate.set_editor_property('pieces',parts)
    variation=ue.ERMapTemplateVariation();variation.set_editor_property('enabled',True)
    for name in ['arena_min','arena_max','base_width','base_depth']:variation.set_editor_property(name,ue.Vector2D(*recipe[name]))
    for name in ['min_rows','max_rows']:variation.set_editor_property(name,recipe[name])
    variation.set_editor_property('modules',[getattr(ue.ERTemplateModule,k) for k in recipe['modules']])
    for name,asset in recipe['palette'].items():
        mesh=assets.load_asset('/Game/EclipseRecoil/Art/FactoryEnvironment/Meshes/'+asset)
        if not mesh:raise RuntimeError('Missing Factory mesh '+asset)
        variation.set_editor_property(name,mesh)
    candidate.set_editor_property('variation',variation)
    seeds=sorted(set(list(range(512))+[7919,-17,2147483647,-2147483648]))
    for seed in seeds:
        if not candidate.validate_layout(seed):raise RuntimeError('Invalid variant seed='+str(seed))
    for field in fields:template.set_editor_property(field,candidate.get_editor_property(field))
    assets.set_metadata_tag(template,'ERTemplateVariationRevision','3');assets.set_metadata_tag(template,'ERVariationManifestSHA256',recipe_hash)
    if not assets.save_loaded_asset(template):raise RuntimeError('Cannot save variation template')
    ue.log('ER_KILLHOUSE_VARIATION_UPGRADED seeds='+str(len(seeds)))
if sha(catalog)!=before or sha(map_file)!=map_before:raise RuntimeError('Unrelated material catalog or map changed')
receipt=dict(revision=3,recipe_sha256=recipe_hash,destruction_catalog_sha256=before,map_sha256=map_before,destruction_catalog_unchanged=True,map_unchanged=True,validated_seeds=516)
(private/'SourceAssets/FactoryEnvironment/variation_upgrade.json').write_text(json.dumps(receipt,indent=2)+'\n')
ue.log('ER_KILLHOUSE_VARIATION_COMPLETE')
