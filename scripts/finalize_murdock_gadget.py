"""Detach new animation previews and discard unused vendor physics-helper tracks."""
import unreal as u
A=u.EditorAssetLibrary
root='/Game/EclipseRecoil/Characters/Teams/'
c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
for team,hero in [('Alpha','Murdock'),('Omega','Gadget')]:
 first=c.get_editor_property(team.lower()+'_skins')[10].mesh
 source=u.load_asset(root+team+'/'+hero+'/SK_ER_'+hero+'_Retarget')
 probe=u.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(first)
 for path in A.list_assets(root+team+'/'+hero+'/Animations',recursive=True):
  value=u.load_asset(path);changed=False
  if source:
   assert u.ERCharacter.replace_owned_asset_references(value,[source],[first]);changed=True
  if isinstance(value,u.AnimSequence):
   ctrl=value.get_editor_property('controller');model=ctrl.get_model_interface()
   remove=[str(b) for b in model.get_bone_track_names() if 'RigidBodyShape' in str(b) and probe.get_bone_index(b)<0]
   if remove:
    ctrl.open_bracket('Remove unused vendor physics helper tracks',False)
    try:
     for bone in remove:assert ctrl.remove_bone_track(bone,False)
    finally:ctrl.close_bracket(False)
    changed=True
  if isinstance(value,u.AnimBlueprint):u.BlueprintEditorLibrary.compile_blueprint(value)
  if changed:assert A.save_loaded_asset(value,False)
print('ER_NEW_ANIMATION_CLEANUP_COMPLETE')
