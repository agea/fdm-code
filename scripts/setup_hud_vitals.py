"""Import owner-supplied circular HUD art unchanged into the private maps mount."""
from pathlib import Path
import unreal as ue

root = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir()))
source = root / 'Assets/maps/SourceAssets/HUDVitals'
folder = '/Game/EclipseRecoil/Art/HUDVitals'
tools = ue.AssetToolsHelpers.get_asset_tools()
for kind in ('health', 'armor', 'stamina'):
    for suffix in ['full'] + [f'black_sector_{i:02d}' for i in range(16)]:
        name = f'{kind}_{suffix}'
        task = ue.AssetImportTask()
        for key, value in dict(filename=str(source / (name + '.png')), destination_path=folder,
                               destination_name='T_' + name, automated=True, replace_existing=True, save=False).items():
            task.set_editor_property(key, value)
        tools.import_asset_tasks([task])
        texture = ue.load_asset(folder + '/T_' + name)
        assert texture, name
        texture.set_editor_property('lod_group', ue.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property('mip_gen_settings', ue.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
        texture.set_editor_property('filter', ue.TextureFilter.TF_TRILINEAR)
        texture.set_editor_property('address_x', ue.TextureAddress.TA_CLAMP)
        texture.set_editor_property('address_y', ue.TextureAddress.TA_CLAMP)
        texture.set_editor_property('srgb', True)
        texture.set_editor_property('never_stream', True)
        assert ue.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
ue.log('ER_HUD_VITALS_IMPORT_PASS textures=51')
