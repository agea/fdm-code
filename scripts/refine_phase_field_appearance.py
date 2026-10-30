"""One-time Phase field finish: short hair, opaque fabric sleeves, fitted helmet.

Operate only on the new project-owned Phase appearances; preserve later edits.
"""
import unreal as ue
assets=ue.EditorAssetLibrary
edit=ue.MaterialEditingLibrary
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
marker='ERPhaseFieldFinish'
bp=ue.load_asset(paths[0])
if assets.get_metadata_tag(bp,marker)!='4':
    defaults=ue.get_default_object(bp.generated_class())
    choices=list(defaults.omega_skins)
    assert len(choices)>=10
    natural_face=next(material for slot,material in zip(choices[5].mesh.materials,choices[5].materials) if 'body_head' in str(slot.material_slot_name).lower())
    for index in range(5,10):
        appearance=choices[index]
        hidden=set(appearance.hidden_material_slots)
        materials=list(appearance.materials)
        for slot,(section,material) in enumerate(zip(appearance.mesh.materials,appearance.materials)):
            name=str(section.material_slot_name).lower()
            if 'hair_long' in name or 'hair_short' in name or name in ['m13_leather','m14_lace']:hidden.add(slot)
            # Kitty's head atlas includes a dark decorative eye mask. The base
            # Phase face uses the same UV layout and keeps natural skin.
            if index in [6,9] and 'body_head' in name: materials[slot]=natural_face
            # Hero vertex offsets must not counteract skeletal ragdolls.
            graph=material
            while isinstance(graph,ue.MaterialInstanceConstant):graph=graph.parent
            for prop in [ue.MaterialProperty.MP_WORLD_POSITION_OFFSET]:
                previous=edit.get_material_property_input_node(graph,prop)
                ue.log('ER_PHASE_VERTEX_INPUT material=%s prop=%s source=%s'%(graph.get_name(),str(prop),previous.get_class().get_name() if previous else 'none'))
                zero=next((x for x in edit.get_material_expressions(graph) if str(x.get_editor_property('desc'))=='ER_NoHeroVertexOffset'),None) or edit.create_material_expression(graph,ue.MaterialExpressionConstant)
                zero.set_editor_property('desc','ER_NoHeroVertexOffset');zero.set_editor_property('r',0)
                edit.connect_material_property(zero,'',prop)
            edit.recompile_material(graph)
            assert assets.save_loaded_asset(graph,only_if_is_dirty=False)
            if not any(part in name for part in ['hair','eye','tear','mouth']):

                while isinstance(material,ue.MaterialInstanceConstant):material=material.parent
                assert material.get_path_name().startswith('/Game/EclipseRecoil/Characters/Teams/Omega/Skins/Phase')
                material.set_editor_property('blend_mode',ue.BlendMode.BLEND_OPAQUE)
                solid=next((x for x in edit.get_material_expressions(material) if str(x.get_editor_property('desc'))=='ER_FieldFabricOpacity'),None) or edit.create_material_expression(material,ue.MaterialExpressionConstant)
                solid.set_editor_property('desc','ER_FieldFabricOpacity')
                solid.set_editor_property('r',1)
                edit.connect_material_property(solid,'',ue.MaterialProperty.MP_OPACITY)
                edit.connect_material_property(solid,'',ue.MaterialProperty.MP_OPACITY_MASK)
                edit.recompile_material(material)
                assert assets.save_loaded_asset(material,only_if_is_dirty=False)
        appearance.set_editor_property('hidden_material_slots',sorted(hidden))
        appearance.set_editor_property('materials',materials)
        if index==8 and assets.get_metadata_tag(bp,'ERPhaseEngineerHoodVersion')!='1':
            part=list(appearance.attachments)[0]
            appearance.set_editor_property('attachments',[ue.ERCorpse.make_field_attachment(part.mesh,part.material,appearance.mesh,'head',ue.Vector(0,0,8),ue.Vector(.9,.9,.8))])
        choices[index]=appearance
        ue.log('ER_PHASE_FIELD_FINISH skin=%d hidden=%s'%(index,sorted(hidden)))
    for path in paths:
        bp=ue.load_asset(path)
        ue.get_default_object(bp.generated_class()).set_editor_property('omega_skins',choices)
        ue.BlueprintEditorLibrary.compile_blueprint(bp)
        assets.set_metadata_tag(bp,marker,'4')
        assert assets.save_loaded_asset(bp,only_if_is_dirty=False)
    ue.log('ER_PHASE_FIELD_FINISH_COMPLETE')
else:
    ue.log('ER_PHASE_FIELD_FINISH_ALREADY_APPLIED')
