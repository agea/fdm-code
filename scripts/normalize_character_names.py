"""Normalize only display labels in all three saved skin catalogs."""
import runpy
from pathlib import Path
import unreal as ue

names = runpy.run_path(str(Path(__file__).with_name('character_catalog.py')))['NAMES']
changed = 0
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter',
             '/Game/EclipseRecoil/Core/BP_ERTrainingTarget',
             '/Game/EclipseRecoil/Core/BP_ERCorpse']:
    bp = ue.load_asset(path)
    defaults = ue.get_default_object(bp.generated_class())
    dirty = False
    for team, labels in names.items():
        choices = list(defaults.get_editor_property(team.lower()+'_skins'))
        assert len(choices) == len(labels) == 10
        for index, label in enumerate(labels):
            if choices[index].display_name != label:
                choices[index].set_editor_property('display_name', label)
                dirty = True
        defaults.set_editor_property(team.lower()+'_skins', choices)
    if dirty:
        ue.BlueprintEditorLibrary.compile_blueprint(bp)
        assert ue.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
        changed += 1
ue.log('ER_CHARACTER_NAMES_COMPLETE skins=20 changed_catalogs=%d' % changed)
