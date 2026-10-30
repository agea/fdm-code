"""Undo only the Assault head trial; no backup folder or source mesh edits required."""
import unreal as u
A=u.EditorAssetLibrary
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']:
 bp=A.load_asset(path);c=u.get_default_object(bp.generated_class());skins=list(c.alpha_skins);a=skins[1]
 assert a.display_name=='Wraith · Assault'
 slot=next(i for i,s in enumerate(a.mesh.materials) if str(s.material_slot_name)=='M02_backpackhead')
 materials=list(a.materials);materials[slot]=a.mesh.materials[slot].material_interface
 hidden=set(a.hidden_material_slots);hidden.discard(slot)
 a.set_editor_property('materials',materials);a.set_editor_property('hidden_material_slots',sorted(hidden))
 a.set_editor_property('attachments',[part for part in a.attachments if part.mesh.get_name()!='SM_ER_WraithAssault_TwinblastHead'])
 skins[1]=a;c.set_editor_property('alpha_skins',skins)
 u.BlueprintEditorLibrary.compile_blueprint(bp);A.set_metadata_tag(bp,'ERWraithTwinblastHead','');assert A.save_loaded_asset(bp,False)
u.log('ER_WRAITH_ASSAULT_MASK_RESTORED catalogs=3')
