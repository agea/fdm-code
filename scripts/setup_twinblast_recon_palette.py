"""Gallery variant4 / Alpha Sniper: neutral short jacket and blue trousers.

Run through UnrealEditor-Cmd -run=pythonscript. Preserve source graphs, mesh,
cloth, FPS geometry, all other appearances and gameplay. Own the two palette
parents independently so broad team-palette refreshes cannot recolor this skin.
"""
from datetime import datetime
from pathlib import Path
import hashlib
import json
import zipfile
import unreal as ue

assets = ue.EditorAssetLibrary
edit = ue.MaterialEditingLibrary
project = Path(ue.Paths.project_dir()).resolve()
catalogs = ['/Game/EclipseRecoil/Core/BP_ERCharacter',
            '/Game/EclipseRecoil/Core/BP_ERTrainingTarget',
            '/Game/EclipseRecoil/Core/BP_ERCorpse']
folder = '/Game/EclipseRecoil/Characters/Teams/Alpha/Skins/Scout'
palette = {
    'M01_cloth': ('M_ER_Recon_NeutralJacket', (.52, .50, .47)),
    'M04_LowerBody': ('M_ER_Recon_BlueTrousers', (.10, .30, .72)),
}
outputs = [folder+'/'+name for name, _ in palette.values()]

def file(path):
    return project/'Content'/(path.removeprefix('/Game/').split('.')[0]+'.uasset')

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def save(asset):
    assert assets.save_loaded_asset(asset, False), asset.get_path_name()

backup = project/'Saved/AssetBackups'/('TwinblastReconPalette-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(backup, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in catalogs+outputs:
        filename = file(path)
        if filename.exists():
            archive.write(filename, str(filename.relative_to(project)))
with zipfile.ZipFile(backup) as archive:
    assert archive.testzip() is None

# Protect every other licensed character package, including the original palette,
# skin/FPS meshes, hair, hood, physics, cloth and animation assets.
allowed = {file(path).resolve() for path in outputs}
protected = {p: digest(p) for p in (project/'Assets/characters/Content').rglob('*.uasset')
             if p.resolve() not in allowed}
source = ue.get_default_object(assets.load_blueprint_class(catalogs[0])).alpha_skins[3]
assert source.display_name == 'Twinblast · Recon'
assert source.mesh.get_name() == 'SK_ER_Twinblast_Alpha_Scout'
materials = list(source.materials)
for slot, section in enumerate(source.mesh.materials):
    key = str(section.material_slot_name)
    if key not in palette:
        continue
    name, tint = palette[key]
    path = folder+'/'+name
    material = assets.load_asset(path) if assets.does_asset_exist(path) else assets.duplicate_asset(materials[slot].get_path_name(), path)
    assert isinstance(material, ue.Material), (key, material)
    nodes = {str(n.get_editor_property('desc')): n for n in edit.get_material_expressions(material)}
    nodes['ER_GroundedTint'].set_editor_property('default_value', ue.LinearColor(*tint, 1))
    nodes['ER_DesaturationAmount'].set_editor_property('r', 1.0)
    if key == 'M04_LowerBody':
        # The original trousers are almost black. Lift their textured albedo
        # slightly before dyeing, retaining seams, normals and AO.
        lift = nodes.get('ER_TeamReadabilityLift') or edit.create_material_expression(material, ue.MaterialExpressionLinearInterpolate)
        lift.set_editor_property('desc', 'ER_TeamReadabilityLift')
        lift.set_editor_property('const_b', .50)
        lift.set_editor_property('const_alpha', .22)
        assert edit.connect_material_expressions(nodes['ER_GroundedDesaturation'], '', lift, 'A')
        assert edit.connect_material_expressions(lift, '', nodes['ER_GroundedColor'], 'A')
        # Shoes share this material. Keep their original dark finish using bind-
        # pose height, which stays attached to the garment through every animation.
        def node(kind, desc, **values):
            value = nodes.get(desc) or edit.create_material_expression(material, kind)
            value.set_editor_property('desc', desc)
            for key, setting in values.items(): value.set_editor_property(key, setting)
            return value
        position = node(ue.MaterialExpressionPreSkinnedPosition, 'ER_ReconBindPosition')
        height = node(ue.MaterialExpressionComponentMask, 'ER_ReconBindHeight', r=False, g=False, b=True, a=False)
        # The user wants the whole shin blue too. Reserve only the actual shoe
        # below the ankle; the previous58cm cutoff left the calves dark.
        edge = node(ue.MaterialExpressionSubtract, 'ER_ReconTrouserStart', const_b=14.0)
        feather = node(ue.MaterialExpressionDivide, 'ER_ReconTrouserFeather', const_b=2.0)
        mask = node(ue.MaterialExpressionClamp, 'ER_ReconTrouserMask', min_default=0.0, max_default=1.0)
        interpolated = node(ue.MaterialExpressionVertexInterpolator, 'ER_ReconTrouserVertexMask')
        original = assets.load_asset(folder+'/M_ER_Alpha_M04_LowerBody_Grounded')
        original_tint = next(n.get_editor_property('default_value') for n in edit.get_material_expressions(original)
                             if str(n.get_editor_property('desc')) == 'ER_GroundedTint')
        gear_tint = node(ue.MaterialExpressionConstant3Vector, 'ER_ReconOriginalBootTint', constant=original_tint)
        gear = node(ue.MaterialExpressionMultiply, 'ER_ReconOriginalBootColor')
        final = node(ue.MaterialExpressionLinearInterpolate, 'ER_ReconTrouserAndBootColor')
        for src, output, dst, pin in [(position,'',height,''),(height,'',edge,'A'),
                                     (edge,'',feather,'A'),(feather,'',mask,''),(mask,'',interpolated,''),
                                     (nodes['ER_GroundedDesaturation'],'',gear,'A'),(gear_tint,'',gear,'B'),
                                     (gear,'',final,'A'),(nodes['ER_GroundedColor'],'',final,'B'),
                                     (interpolated,'',final,'Alpha')]:
            assert edit.connect_material_expressions(src, output, dst, pin), (src, dst, pin)
        assert edit.connect_material_property(final, '', ue.MaterialProperty.MP_BASE_COLOR)
    assets.set_metadata_tag(material, 'ERPalettePolicy', 'ReconNeutralJacketBlueTrousers')
    material.set_editor_property('used_with_skeletal_mesh', True)
    edit.recompile_material(material)
    save(material)
    materials[slot] = material
    ue.log('ER_RECON_PALETTE_MATERIAL slot=%s tint=%s' % (key, tint))

for path in catalogs:
    bp = assets.load_asset(path)
    defaults = ue.get_default_object(bp.generated_class())
    choices = list(defaults.alpha_skins)
    appearance = choices[3]
    assert appearance.mesh == source.mesh and appearance.display_name == source.display_name
    appearance.set_editor_property('materials', materials)
    choices[3] = appearance
    defaults.set_editor_property('alpha_skins', choices)
    assets.set_metadata_tag(bp, 'ERReconPaletteVersion', '2')
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    save(bp)
    ue.log('ER_RECON_PALETTE_CATALOG '+path)

assert all(digest(p) == before for p, before in protected.items()), 'Protected character package changed'
receipt = dict(backup=str(backup), protected_packages=len(protected),
               materials=outputs, catalogs=catalogs, skin=3, gallery_variant=4,
               trouser_start_cm=14.0, trouser_feather_cm=2.0)
(project/'Saved/Testing').mkdir(parents=True, exist_ok=True)
(project/'Saved/Testing/recon-palette-migration.json').write_text(json.dumps(receipt, indent=2))
ue.log('ER_RECON_PALETTE_COMPLETE protected=%d backup=%s' % (len(protected), backup))
