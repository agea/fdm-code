# Sketchfab weapon import — 15 October 2026

The requested source list is preserved verbatim in
[references/weapons.it.md](references/weapons.it.md), with an English translation
in [WEAPONS_REFERENCE_EN.md](WEAPONS_REFERENCE_EN.md). The subsequent user choice
is AK-47 statistics for the assault rifle and SCAR statistics for the carbine.
The sheet identifies the latter as `scar20`. AWP is the retained sniper profile
because the supplied list does not specify a sniper statistics row.

## Catalog and source statistics

The requested keys select ten independently replicated inventory weapons. All
ten are available each life; the assault rifle is initially equipped. Mouse
wheel selection keeps its existing 250ms quiet-period rule. Switching weapons
cancels reload/fire without refilling ammunition. Respawn restores inventory
and 100 armor points. The previous selectable-primary arsenal remains available
with the server URL option `?ERLegacyArsenal=1` for its existing regression fixture.

| Key | Supplied model | Statistics row | Damage | Cycle (s) | Armor ratio | Range modifier |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| 1 | DJMaesen knife | Project melee profile | 40 | 0.50 | 2.000 | 1.00 |
| 2 | DJMaesen Beretta | `deagle` | 53 | 0.23 | 1.864 | 0.85 |
| 3 | Cransh / eNse7en SST30 | `mp9` | 26 | 0.07 | 1.200 | 0.87 |
| 4 | DJMaesen shotgun | `xm1014` | 20/pellet, six pellets | 0.35 | 1.600 | 0.70 |
| 5 | wburton AR15 | `ak47` | 36 | 0.10 | 1.550 | 0.98 |
| 6 | DJMaesen carbine | `scar20` | 80 | 0.25 | 1.650 | 0.98 |
| 7 | DJMaesen sniper | `awp` | 115 | 1.46 | 1.950 | 0.99 |
| 8 | user77 / JaumWF / DJMaesen Milkor M32 | Project HE launcher | 140 peak | 0.50 | 1.000 | n/a |
| F | Riskyerick RGD-5 | Project HE | 140 peak | 0.80 | 1.000 | n/a |
| G | Riskyerick M18 | Smoke | 0 | 0.80 | n/a | n/a |

The downloaded worksheet snapshot is
[references/weapon-stats-2069175273.csv](references/weapon-stats-2069175273.csv).
Its source is the [user-supplied Google sheet](https://docs.google.com/spreadsheets/d/11tDzUNBq9zIX6_9Rel__fdAUezAQzSnh5AVYzCP060c/edit?gid=2069175273#gid=2069175273).
Only the five numeric fields above are supplied by that worksheet. Magazine,
reserve, reload duration, spread, zoom, melee and explosive values are project
choices, explicitly defined in `scripts/import_sketchfab_weapons.py`.
Model names and statistical profiles are intentionally shown together: applying
Desert Eagle numbers does not turn the supplied Beretta geometry into a Deagle.

## Combat

Bullets retain server-sampled spread, first-hit muzzle obstruction, per-pellet
hitscan, block penetration, tracers and impact feedback. Range attenuation is
`damage * RangeModifier^(distance_cm / 1270)`: 500 Source units are 12.7m.
The receiving character applies that attenuation to each actual bullet hit,
including residual damage after a penetrated destructible block.

Armor is server owned and replicated. The protected health fraction is
`clamp(ArmorRatio / 2, 0, 1)`; armor costs half the prevented health damage.
Insufficient armor prevents only the amount paid for, then reaches zero.
Spawn protection does not consume armor. Armor covers the existing uniform
character hit volume; separate helmet/head/leg hit zones are not introduced.
The HUD displays HP and AP. Direct environmental damage retains its existing
behavior. Tests for the original 36-damage fixture use the legacy catalog.

Knife attacks use a 160cm server trace, respect the first obstruction, have a
cooldown and consume no ammunition. The Milkor launches a replicated physical
`LaunchedHE`, using the supplied `GL_slug1` geometry. Its 3600cm/s launch speed
exceeds the 1100cm/s thrown HE/smoke speed; gravity, impact detonation and a
2.5s backup fuse remain authoritative. HE damage still checks static cover and
the shared block explosion path. Smoke uses the existing replicated smoke cloud.

## Assets and animation

All nine original archives, author/license API snapshots, SHA-256 receipts and
extracted authoring inputs are private in
`Assets/weapons/SourceAssets/SketchfabWeapons`. Imported meshes, materials,
textures, sequences, retargeted hands and data assets live under
`/Game/EclipseRecoil/Weapons/Sketchfab`. The existing weapons mount owns those
packages. Character and training-target defaults in the private maps mount
reference the complete catalog. Runtime never reads Downloads or the worksheet.

`bake_sketchfab_weapons.py` converts object animation to a skeletal derivative,
preserving vertices, UVs, mechanical motion and original archives. It separates
gun-root motion from animated parts, preserves animated visibility scale and
extracts the launcher's actual projectile mesh. Original source arms are not
added to the player. `retarget_weapon_hands.py` transfers wrist/root trajectories
onto the fitted FPS rig with bounded two-bone IK, retaining current skin meshes,
finger poses, sockets and component framing. The fitted hands use a 30Hz sampling
grid compatible with their existing Unreal compression target. Some source
reaches are clamped to the fitted arm lengths; this is an adaptation rather than
a lossless full-body retarget.

Pistol, SMG, shotgun, assault, carbine, sniper and Milkor have weapon and hand
reload sequences; fire motion is also imported. Reload playback follows the
replicated weapon state and is scaled to authoritative reload duration. Switching
or canceling returns to idle. The static grenade set contains no authored throw
or reload animation; its original unanimated meshes are used with existing poses.
Weapon audio remains the existing template feedback; the optional external sound
downloads mentioned by some model authors have not been imported.

Color textures are sRGB; masks are linear. Supplied smoothness/glossiness is
inverted for roughness. OpenGL normal maps flip green; explicitly named DirectX
normals retain green. The Milkor keeps separate material regions and its source
textures; imported texture output is capped to 2048 pixels per axis.

## Rebuild and verification

1. Download the original-format archives from the nine supplied listings using
   an authorized Sketchfab session; public metadata is stored beside each model.
2. Run `python3 scripts/stage_sketchfab_weapons.py` to preserve originals,
   validate member paths, extract inputs and write hash/provenance receipts.
3. Run Blender with `--background --python scripts/bake_sketchfab_weapons.py --
   Assets/weapons/SourceAssets/SketchfabWeapons`.
4. Compile the Editor module, then run `import_sketchfab_weapons.py` through
   Unreal's PythonScript commandlet. `-ERStageReferenceWeapons` saves prepared
   assets without activating the catalog. `-ERReimportReferenceWeapons` explicitly
   reimports generated meshes; ordinary reruns preserve them. Interchange retains
   existing animation packages/reference poses during mesh reimport: changed clip
   or grip normalization requires a new derivative namespace. The calibrated
   gun/knife generation uses `V2`; the first generation remains for comparison.
5. Run `test-reference-weapons.py`, then `--render` for actual Metal captures.
   Existing `test-network.py --shot-feedback`, `test-network.py --accuracy` and
   `test-arsenal.py` cover the baseline combat/effects regressions.

The native `EclipseRecoil.Weapons.ReferenceBallistics` test covers source-unit
conversion, compounded falloff, armor depletion and full penetration. Executed
results and graphical evidence are recorded in `TEST_RESULTS.md`.

Licenses remain per asset. The Milkor listing reports **CC BY-NC 4.0**, including
separate weapon and animation credits. The grenade listing reports Sketchfab's
free standard license; the other listings report CC BY 4.0. Exact snapshots and
underlying author references are preserved privately. This import does not
change distribution permissions or publish assets.
