"""Replace the drone beacon with the original supplied team decal painted onto the underside."""
from pathlib import Path
from datetime import datetime
import zipfile
import unreal as ue
assets=ue.EditorAssetLibrary
edit=ue.MaterialEditingLibrary
folder='/Game/EclipseRecoil/Art/Recon'
project=Path(ue.Paths.project_dir())
backup=project/'Saved/AssetBackups'/('DroneSymbols-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as archive:
    for path in assets.list_assets(folder,recursive=True):
        filename=project/'Content'/(path.removeprefix('/Game/').split('.')[0]+'.uasset')
        if filename.exists():archive.write(filename,str(filename.relative_to(project)))
def save(asset):
    assert assets.save_loaded_asset(asset,False),asset.get_path_name()
materials=[]
for team in ['Alpha','Omega']:
    texture=ue.load_asset('/Game/EclipseRecoil/Art/TeamGraffiti/T_Graffiti'+team)
    assert texture,team
    name='M_DroneUnderside'+team
    material=ue.load_asset(folder+'/'+name) if assets.does_asset_exist(folder+'/'+name) else ue.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,ue.Material,ue.MaterialFactoryNew())
    edit.delete_all_material_expressions(material)
    material.set_editor_property('material_domain',ue.MaterialDomain.MD_DEFERRED_DECAL)
    material.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT)
    sample=edit.create_material_expression(material,ue.MaterialExpressionTextureSample)
    sample.set_editor_property('texture',texture)
    assert edit.connect_material_property(sample,'RGB',ue.MaterialProperty.MP_BASE_COLOR)
    assert edit.connect_material_property(sample,'A',ue.MaterialProperty.MP_OPACITY)
    # A restrained colour floor keeps printed art legible on the unlit underside.
    # Values stay below .2: no HDR bloom or illumination of neighbouring objects.
    floor=edit.create_material_expression(material,ue.MaterialExpressionMultiply)
    floor.set_editor_property('const_b',.2)
    assert edit.connect_material_expressions(sample,'RGB',floor,'A')
    assert edit.connect_material_property(floor,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
    rough=edit.create_material_expression(material,ue.MaterialExpressionConstant)
    rough.set_editor_property('r',.85)
    assert edit.connect_material_property(rough,'',ue.MaterialProperty.MP_ROUGHNESS)
    edit.recompile_material(material);save(material);materials.append(material)
bp=ue.load_asset(folder+'/BP_ReconDrone');cdo=ue.get_default_object(bp.generated_class())
assert cdo.get_editor_property('underside_symbol'), 'Missing native underside decal'
cdo.set_editor_property('alpha_symbol_material',materials[0])
cdo.set_editor_property('omega_symbol_material',materials[1])
cdo.get_editor_property('body').set_editor_property('receives_decals',True)
ue.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
for name in ['M_ReconBeaconAlpha','M_ReconBeaconOmega','M_DroneSymbolAlpha','M_DroneSymbolOmega']:
    path=folder+'/'+name
    if assets.does_asset_exist(path):
        assert not assets.find_package_referencers_for_asset(path,True),path
        assert assets.delete_asset(path),path
ue.log('ER_DRONE_SYMBOLS_COMPLETE backup='+str(backup))
