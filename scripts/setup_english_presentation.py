"""Save English labels only; preserve asset paths, loadout order and gameplay data."""
from pathlib import Path
import json
import runpy
import unreal as ue
script_dir=Path(__file__).parent
labels=runpy.run_path(str(script_dir/'presentation_labels.py'))
skins=runpy.run_path(str(script_dir/'character_catalog.py'))['NAMES']
changes=[]
def save(obj):
    assert ue.EditorAssetLibrary.save_loaded_asset(obj,only_if_is_dirty=False), obj.get_path_name()
for key,label in labels['WEAPON_NAMES'].items():
    path='/Game/EclipseRecoil/Weapons/Sketchfab/%s/DA_%s'%(key,key)
    data=ue.load_asset(path)
    assert data,path
    data.set_editor_property('display_name',label);save(data)
    changes.append(dict(asset=path,label=label))
for key,label in dict(labels['LEGACY_NAMES'],Rifle_A='Assault Rifle').items():
    path='/Game/EclipseRecoil/Weapons/'+('DA_Rifle_A' if key=='Rifle_A' else 'LowPoly/DA_'+key)
    data=ue.load_asset(path)
    assert data,path
    data.set_editor_property('display_name',label);save(data)
    changes.append(dict(asset=path,label=label))
for name in ['BP_ERCharacter','BP_ERTrainingTarget','BP_ERCorpse']:
    bp=ue.load_asset('/Game/EclipseRecoil/Core/'+name)
    assert bp,name
    defaults=ue.get_default_object(bp.generated_class())
    for team in ['Alpha','Omega']:
        key=team.lower()+'_skins'
        appearances=list(defaults.get_editor_property(key))
        assert len(appearances)==len(skins[team]), (name,key,len(appearances))
        for index,appearance in enumerate(appearances):
            appearance.set_editor_property('display_name',skins[team][index])
        defaults.set_editor_property(key,appearances)
        appearance=defaults.get_editor_property(team.lower()+'_appearance')
        appearance.set_editor_property('display_name',skins[team][0])
        defaults.set_editor_property(team.lower()+'_appearance',appearance)
    ue.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
# Saved-default verification, including exact slot labels, without regenerating content.
character=ue.get_default_object(ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter').generated_class())
weapons=list(character.get_editor_property('reference_arsenal'))
expected=list(labels['WEAPON_NAMES'].values())
assert [w.get_editor_property('display_name') for w in weapons]==expected
project=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir()))
(project/'Saved/Testing/english-presentation-assets.json').write_text(json.dumps(changes,indent=2)+'\n')
ue.log('ER_ENGLISH_PRESENTATION_READY weapons=%d skins=20 blueprints=3 slots=%s'%(len(changes),expected))
