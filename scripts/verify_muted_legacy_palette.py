"""Verify the scoped muted palette and protect all other character art."""
from pathlib import Path
import json,hashlib,runpy
import unreal as u
p=Path(u.Paths.project_dir()).resolve();out=p/'Saved/Testing/MutedLegacyPalette';A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary
rows=json.loads((out/'changed.json').read_text());protected=json.loads((out/'protected-hashes.json').read_text())
assert len(rows)==21
for row in rows:
 assert hashlib.sha256((p/row['file']).read_bytes()).hexdigest()==row['after_hash'],row['file']
 m=A.load_asset(row['asset']);g=m
 while isinstance(g,u.MaterialInstanceConstant):g=g.get_editor_property('parent')
 nodes={str(n.get_editor_property('desc')):n for n in E.get_material_expressions(g)}
 color=E.get_material_instance_vector_parameter_value(m,'ER_GroundedTint') if isinstance(m,u.MaterialInstanceConstant) else nodes['ER_GroundedTint'].get_editor_property('default_value')
 assert all(abs(a-b)<1e-5 for a,b in zip([color.r,color.g,color.b],row['tint']))
 assert A.get_metadata_tag(m,'ERMutedLegacyPalette')=='1'
 assert abs(nodes['ER_DesaturationAmount'].get_editor_property('r')-.85)<1e-5
 lift=nodes['ER_TeamReadabilityLift'];assert abs(lift.get_editor_property('const_alpha')-row['lift_alpha'])<1e-5 and abs(lift.get_editor_property('const_b')-.5)<1e-5
 assert E.get_material_property_input_node(g,u.MaterialProperty.MP_EMISSIVE_COLOR).get_editor_property('r')==0
for rel,h in protected.items():assert hashlib.sha256((p/rel).read_bytes()).hexdigest()==h,rel
for name in ['verify_team_skins.py','verify_team_characters.py','verify_drongo_scavenger_head.py','verify_gadget_sniper_shellshock.py']:runpy.run_path(str(p/'scripts'/name))
(out/'verified.json').write_text(json.dumps({'owned_materials':len(rows),'protected_packages':len(protected),'belica_blue':[.12,.28,.52],'drongo_ochre_reference':[.62,.29,.115],'wraith_gadget_and_other_original_art_unchanged':True,'natural_and_neutral_materials_unchanged':True,'emissive_zero':True},indent=2))
u.log('ER_MUTED_LEGACY_PALETTE_VERIFIED')
