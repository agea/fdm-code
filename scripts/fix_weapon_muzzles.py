"""Save actual barrel-tip centers in owned weapon data, preserving source meshes.
The supplied gun geometry points +X; use the center of its front 4mm instead
of imported bounding-box X with Y/Z zero. Gameplay hitscan is independent.
"""
import unreal as ue
for key in ['AK74','M4_8','Uzi','SG_M4','M249','M107','M1911']:
    root='/Game/EclipseRecoil/Weapons/LowPoly/'
    data=ue.load_asset(root+'DA_'+key)
    mesh=ue.load_asset(root+'SK_'+key)
    if not data or not mesh:
        raise RuntimeError('Missing weapon data/mesh '+key)
    muzzle=ue.ERCharacter.get_weapon_muzzle_from_geometry(mesh)
    if muzzle.x <= 0:
        raise RuntimeError('Cannot resolve actual muzzle geometry '+key)
    previous=str(data.get_editor_property('muzzle_offset'))
    data.set_editor_property('muzzle_offset',muzzle)
    if not ue.EditorAssetLibrary.save_loaded_asset(data,only_if_is_dirty=False):
        raise RuntimeError('Cannot save muzzle '+key)
    ue.log('ER_MUZZLE_FIXED '+key+' previous='+previous+' tip='+str(muzzle))
ue.log('ER_MUZZLE_GEOMETRY_PASS: seven bullet weapons, meshes unchanged')
