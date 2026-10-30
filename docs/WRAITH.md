# Wraith replacement

30 October2026: Alpha indices0,1,2 retain their roles and become Wraith Officer,
Assault and Patrol. Both teams keep ten selectable appearances. Alpha3/4 remain
Twinblast Recon/Support, Belica5..9 and the complete Omega catalog are unchanged.
Original Twinblast art stays available as the head donor.

| Index | Role label | Source costume | Finish |
|---|---|---|---|
| 0 | Wraith · Officer | OD Green | Steel blue cloth, gray field armor |
| 1 | Wraith · Assault | Base | Navy cloth, dark neutral lower body |
| 2 | Wraith · Patrol | Lunar Ops | Lighter blue, contrasting plates and field pouches |

These use three authored material/geometry layouts, rather than three recolors
of one mesh. The initial replacement retained the masked heads; the subsequent
Assault head trial is described below. Owned materials preserve authored textures/normals and disable emissive
effects and hero vertex offsets. The original weapon sections use the existing
texture-free hidden material. Backpack/cannon/weapon bones are hidden without
hiding head or body bones.

Imported145 hard-reference source packages from ParagonStaging into the private
characters submodule at the original `/Game/ParagonWraith` namespace, mounted via
assets.json. No source package was edited. Owned skins, physics, FPS meshes and
Manny rifle retargets live under Characters/Teams. Anatomical ragdolls and measured
world/FPS palm frames use the existing helpers. FPS bind rig, weapon poses and
camera transforms remain unchanged.

setup_wraith_replacement.py publishes the three player/target/corpse catalogs
only after all owned meshes, retargets, physics and palms exist. Alpha fallback
and first-person defaults follow Officer. DefaultGame.ini and native loadout
names match; existing saved Twinblast Officer/Assault/Patrol IDs resolve to the
corresponding Wraith identity. Other role assignments retain their indices.

Only Murdock was removed from ParagonStaging:1899 unique asset/map packages,
after SHA256-verified backup and Unreal's outside-reference checks. Gadget and
Wraith staging packages retain their original hashes. Gadget's game assets were
also preserved. Backup: ../_backups/staging-murdock-20261030. prepare_wraith_staging.py
audits the source closure and guarded deletion; its first deletion ran in an
isolated project with the game's editor module and staging Content.

Catalog/config rollback and416 protected character package hashes:
../_backups/wraith-replacement-20261030. Inspection, import/setup receipts,
validation logs and previews: Saved/Testing/WraithReplacement/.

Validation entry points: verify_wraith_replacement.py, test-team-skins.py
--page0 --pair0 --pair1 --pair2, test-target-skins.py and test-loadouts.py.
inspect_wraith_height.py measures authored head/boot geometry in actual animation
poses. test-character-preview.py --skin0 --skin6 compares the real AERMenuPreview
with the same primary weapon and records camera distance and pose metrics.

The user's Player-menu height comparison was reproduced on the actual preview:
with the same sniper rifle, Wraith Officer fits at410.364cm camera distance and
Belica Arctic Patrol at388.862cm. Their appearance scales are1.04266 and1.03944;
the projected Wraith size is about94.8% of Belica's due to the weapon/pose framing.
The sampled head-bone ranges differ by only about2cm. Authored head/boot sampling
also finds similar overall body heights in the initial animated pose. No gameplay
scale or production camera change was made merely to compensate for that preview.
See Preview/ captures and preview-comparison.json in the validation folder.

## Twinblast head trial

Wraith Assault (Alpha1, gallery2) now uses the complete human head from Twinblast
Support, including its authored UVs, skin, eyes and hair, without sunglasses.
Patrol retains its original masked head. Officer now has the sunglasses head described below. The original Assault
head/backpack material section is hidden at every body LOD; its head bone remains
active. The new static head attaches to that bone and has four render LODs.
No gameplay collision is added. The expression stays neutral: no facial animation
rig, blinking or lip animation was transferred.

The extraction removes124 disconnected donor collar triangles and trims the long
donor neck/shirt to the upper neck. An owned material instance overrides only the donor
shader's ArmColor cloth pigment from red to neutral blue, preserving skin inputs.
This is a first visual trial; the collar transition
can still be refined after reviewing the head choice.

setup_wraith_twinblast_head.py generates only the owned head/material instance and updates
the three catalogs. Run it from the full editor with GeometryScripting enabled
when rebuilding: StaticMeshEditorSubsystem is unavailable in commandlets.
Body/FPS meshes, animations, original materials and source packages are untouched.
verify_wraith_twinblast_head.py checks all catalogs/FPS bindings, complete head,
stable body identities/scales and four LODs. Earlier validation verified1482
original character package hashes. The shared _backups directory was removed
externally during the trial, so that final hash comparison cannot be repeated;
the current receipt reports the baseline as unavailable rather than passing it.

To undo only this trial, run restore_wraith_assault_mask.py through Unreal.
It restores the original mask section/material from the retained body mesh and
removes only this head attachment; it needs no backup and preserves other skins.
Logs, geometry receipts and reviewed images: Saved/Testing/WraithHeadStudy/.

## Officer sunglasses head

Alpha0 uses the retained Twinblast Officer / Action Movie head with sunglasses,
attached in head-bone space with its original face, hair and material UVs.
The sunglasses share the donor upperbody material, so the extraction includes
that section inside the head crop as well as the face sections.

The OD Green costume also has helmet accessories in material0. An owned copy,
`WraithOfficer/HeadStudy/SK_ER_WraithOfficer_Unmasked`, removes6346 triangles in
the bounded head region of that section; torso and limb armor are retained.
All five body LODs are regenerated. Original backpack/head slot4 is hidden,
weapon slot5 stays hidden, and the head bone remains live. The static head has
four LODs. Physics, sockets, scale, FPS and animation bindings are retained.

`setup_wraith_officer_head.py` updates Alpha0 and the canonical Alpha fallback
in player/target/corpse catalogs. Assault's approved head and Patrol's mask stay
unchanged. `verify_wraith_officer_head.py` includes fresh catalog/FPS checks and
hashes of the19 retained Assault/Patrol/old-Officer packages. Task receipts live
in `Saved/Testing/WraithOfficerHead`.
