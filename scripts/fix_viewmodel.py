"""Apply the existing prototype's rifle/hand correction through Unreal Editor."""
from pathlib import Path
import runpy
import unreal as ue

runpy.run_path(str(Path(__file__).with_name("create_rifle_pose.py")))

asset = ue.EditorAssetLibrary.load_asset("/Game/EclipseRecoil/Core/BP_ERCharacter")
if not asset:
    raise RuntimeError("Missing BP_ERCharacter")
defaults = ue.get_default_object(asset.generated_class())
body = defaults.get_editor_property("first_person_body")
body.set_editor_property("relative_location", ue.Vector(-10, 0, -152))
body.set_editor_property("relative_rotation", ue.Rotator(pitch=0, yaw=-90, roll=0))
anim = body.get_editor_property("animation_data")
anim.set_editor_property("anim_to_play", ue.load_asset("/Game/EclipseRecoil/Characters/A_ER_Rifle_HipPose"))
anim.set_editor_property("saved_looping", True)
body.set_editor_property("animation_data", anim)
for name in ["first_person_rifle", "world_rifle"]:
    component = defaults.get_editor_property(name)
    component.set_editor_property("relative_location", ue.Vector(0, 0, 0))
    component.set_editor_property("relative_rotation", ue.Rotator(pitch=0, yaw=0, roll=0))
if not ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError("Cannot save BP_ERCharacter")
runpy.run_path(str(Path(__file__).with_name("fix_spawn_rotations.py")))
ue.log("ER_VIEWMODEL_FIXED: rifle aligned to HandGrip_R; explicit rotation axes saved.")
