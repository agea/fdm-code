"""Add M6 material profiles; preserve existing HP, art and map geometry."""
import unreal as ue
assets=ue.EditorAssetLibrary
catalog=assets.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials')
if not catalog: raise RuntimeError('Missing destruction catalog')
rows=list(catalog.get_editor_property('materials'))
default=ue.get_default_object(ue.ERBlockMaterialCatalog)
added=0;filled=0
mapping={ue.ERBlockMaterial.WOOD:'M_ER_PBR_wood_planks',ue.ERBlockMaterial.STONE:'M_ER_PBR_stone_wall_02',ue.ERBlockMaterial.METAL:'M_ER_PBR_rusty_metal_04',ue.ERBlockMaterial.GLASS:'M_ER_Glass'}
for row in default.get_editor_property('materials'):
    kind=row.get_editor_property('type')
    if kind not in mapping: continue
    existing=next((r for r in rows if r.get_editor_property('type')==kind),None)
    if existing and existing.get_editor_property('surface'): continue
    surface=assets.load_asset('/Game/EclipseRecoil/Art/ProceduralTextures/'+mapping[kind])
    if not surface: raise RuntimeError('Missing PBR surface '+mapping[kind])
    if existing:
        existing.set_editor_property('surface',surface);filled+=1
    else:
        row.set_editor_property('surface',surface);rows.append(row);added+=1
if added or filled:
    catalog.set_editor_property('materials',rows)
    assets.set_metadata_tag(catalog,'ERExtendedMaterialsVersion','6')
    if not assets.save_loaded_asset(catalog,only_if_is_dirty=False):raise RuntimeError('Cannot save additive catalog migration')
ue.log('ER_M6_MATERIALS_SAVED added='+str(added)+' filled='+str(filled)+' total='+str(len(rows)))
