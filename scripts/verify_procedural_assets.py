"""Read-only audit of imported Unreal texture settings and saved lab references."""
import unreal as ue
assets=ue.EditorAssetLibrary
edit=ue.MaterialEditingLibrary
root='/Game/EclipseRecoil/Art/ProceduralTextures'
textures=[]; materials=[]
for path in assets.list_assets(root, recursive=True, include_folder=False):
    obj=assets.load_asset(path)
    if isinstance(obj,ue.Texture2D):
        textures.append(obj)
        role=obj.get_name().rsplit('_',1)[-1]
        if obj.get_editor_property('srgb') != (role=='Color'): raise RuntimeError('Wrong sRGB '+path)
        expected=ue.TextureCompressionSettings.TC_NORMALMAP if role=='Normal' else ue.TextureCompressionSettings.TC_DEFAULT if role=='Color' else ue.TextureCompressionSettings.TC_MASKS
        if obj.get_editor_property('compression_settings') != expected: raise RuntimeError('Wrong compression '+path)
        if obj.blueprint_get_size_x()!=2048 or obj.blueprint_get_size_y()!=2048: raise RuntimeError('Unexpected resolution '+path)
        if not assets.get_metadata_tag(obj,'ERSourceSHA256'): raise RuntimeError('Missing provenance '+path)
    elif isinstance(obj,ue.Material):
        materials.append(obj)
        if obj.get_name().startswith('M_ER_PBR_'):
            if obj.get_editor_property('tangent_space_normal'): raise RuntimeError('World normal requires world-space material '+path)
            samples=[n for n in edit.get_material_expressions(obj) if isinstance(n,ue.MaterialExpressionTextureSample)]
            if len(samples)<3 or any(not n.get_editor_property('texture') for n in samples): raise RuntimeError('Missing PBR graph textures '+path)
            for prop in [ue.MaterialProperty.MP_BASE_COLOR,ue.MaterialProperty.MP_NORMAL,ue.MaterialProperty.MP_ROUGHNESS]:
                if not edit.get_material_property_input_node(obj,prop): raise RuntimeError('Disconnected PBR output '+path)
        elif obj.get_name()=='M_ER_Glass' and obj.get_editor_property('blend_mode')!=ue.BlendMode.BLEND_TRANSLUCENT: raise RuntimeError('Glass opacity mode')
if len(textures)!=41 or len(materials)!=11: raise RuntimeError('Unexpected library size')
catalog=assets.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials')
for row in catalog.get_editor_property('materials'):
    if not row.get_editor_property('surface').get_path_name().startswith(root): raise RuntimeError('Catalog surface not migrated')
if not assets.does_asset_exist('/Game/EclipseRecoil/Maps/L_ProceduralLab'): raise RuntimeError('Map not saved')
ue.log('ER_PROCEDURAL_ASSETS_PASS: 41 textures 2048x2048; sRGB/compression/provenance; 10 PBR graphs; glass; catalog; map')
