# Sketchfab weapon import — 15 October 2026

The requested source list is preserved verbatim in
[references/weapons.it.md](references/weapons.it.md), with an English translation
in [WEAPONS_REFERENCE_EN.md](WEAPONS_REFERENCE_EN.md). The subsequent user choice
is AK-47 statistics for the assault rifle, SCAR statistics for the carbine and
AWP statistics for the sniper. The sheet identifies the carbine row as `scar20`.
The later explicit AWP request confirms the previously retained sniper profile.

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
| 8 | user77 / JaumWF / DJMaesen Milkor M32 | Project HE launcher | 140 peak | 0.50 | 1.600 | n/a |
| F | Riskyerick F1 | Project HE | 140 peak | 0.80 | 1.600 | n/a |
| G | Riskyerick M18 | Smoke | 0 | 0.80 | n/a | n/a |

The original downloaded worksheet snapshot is
[references/weapon-stats-2069175273.csv](references/weapon-stats-2069175273.csv).
Its source is the [user-supplied Google sheet](https://docs.google.com/spreadsheets/d/11tDzUNBq9zIX6_9Rel__fdAUezAQzSnh5AVYzCP060c/edit?gid=2069175273#gid=2069175273).
It is superseded for ballistics by the owner-supplied **CS2 Weapon Spreadsheet
(Last Weapon Update: March 18, 2026)**, received on 21 October. The complete XLSX
is preserved privately in `Assets/weapons/SourceAssets/References/`; its SHA-256,
six exact `Raw Values` ranges and 24 cached `Damage @Range` checks are recorded in
`references/cs2-weapon-ballistics-2026-03-18.json`. The new damage, armor and range
values match the earlier snapshot. HeadshotMultiplier is 3.9 for deagle and 4
for mp9/xm1014/ak47/scar20/awp. Analysis formulas BB/BC explicitly specify stomach
1.25 and legs .75; chest/arms retain base damage. Fractional runtime damage is
preserved (Analysis rounds only its displayed comparison values).
Magazine,
reserve, reload duration, zoom, melee and explosive values are project
choices, explicitly defined in `scripts/import_sketchfab_weapons.py`.
The subsequent accuracy request also imports the workbook's spread, posture,
motion, recovery, firing and optical Alt values for all six firearms, including
unrounded source cycle times. See [WEAPON_ACCURACY.md](WEAPON_ACCURACY.md).
Model names and statistical profiles are intentionally shown together: applying
Desert Eagle numbers does not turn the supplied Beretta geometry into a Deagle.

## Combat

Bullets retain server-sampled spread, first-hit muzzle obstruction, per-pellet
hitscan, block penetration, tracers and impact feedback. Range attenuation is
`damage * RangeModifier^(distance_cm / 1270)`: 500 Source units are 12.7m.
The receiving character applies that attenuation to each actual bullet hit,
including residual damage after a penetrated destructible cell. Cover now
subtracts its current HP directly, in traversal order, from one shared bullet
damage budget: 100 against 60HP leaves 40; against an already damaged 15HP cell
it leaves 85. A cell that survives stops the ray. Exact exhaustion destroys the
cell but deals nothing behind it. Multiple cells/panels/rooms share the budget;
empty cells cost nothing and indestructible geometry blocks. This replaces the
old entry/per-cm energy and structural bullet multiplier model. Blast damage,
HP presets and cover exposure are unchanged. Residual bullet damage then receives
the worksheet range factor, anatomical multiplier and target armor once.

Armor is server owned and replicated. The protected health fraction is
`clamp(ArmorRatio / 2, 0, 1)`; armor costs half the prevented health damage.
Insufficient armor prevents only the amount paid for, then reaches zero.
Spawn protection does not consume armor. All six Bullet weapons now query the
existing skeletal physics bodies for head/neck, stomach, chest/arms and legs;
movement capsules do not determine bullet hit zones. Legs bypass armor; the head
is protected only with a helmet. Single bullets keep their sampled direction;
only multiple pellets receive the shotgun rosette. Knife and explosive payloads
have no corresponding worksheet ballistic entries and keep their existing rules.
The HUD displays HP and AP. Direct environmental damage retains its existing
behavior. Tests for the original 36-damage fixture use the legacy catalog.

Knife attacks use a 160cm server trace, respect the first obstruction, have a
cooldown and consume no ammunition. The 27 October presentation uses the source
ready/contact poses around the supplied 3.57–3.61s viewer references
(frames 107–110, approximately 3.53–3.63s on the source 30Hz grid): a 67ms
forward stroke and 200ms eased return after a 33ms contact hold. The fitted
assembly sits 12cm farther forward. `setup_knife_presentation.py` preserves the
base idle, original files and unused support arm, verifies compressed poses and
backs up the knife packages before saving owned ready/stab derivatives. The
160cm range, 40 damage and 0.5s cooldown remain unchanged by explicit request.
The reference Metal gallery captures both ready and contact poses.
The subsequent grip correction rotates the fist 90 degrees around the weapon
frame, seats the handle between palm and curled fingers, and swings the forearm
around the seated wrist to leave through the bottom centre of the viewport.
`fit_knife_grip.py` preserves the approved blade transform at every sample and
both arm segment lengths. The compensating attachment uses the existing FPS
`ER_Palm_R` socket; shared skin sockets and the world weapon attachment stay intact.
The subsequent revision3 uses half that forearm rotation and a higher seat nearer
the guard. It aligns the hand with the original source grip, then fits all five
finger chains to the handle triangles using measured fingertip geometry and
unchanged phalanx lengths. `extract_knife_grip.py` reads the original FBX through
Blender into a private derived JSON; `fit_knife_fingers.py` generates the closed
grip once and retains it across the ready/stab keys. This supersedes the fixed
quarter-turn and bottom-centre endpoint of revision2.
Revision4 reduces the FPS knife to65% of its former dimensions, tilts the hand
8 degrees upward and opens the thumb with0.5cm clearance. Finger targets account
for the scaled handle while retaining measured hand/finger lengths. The existing
half forearm pivot, attack timing, attachment-anchor trajectory, world attachment
and160cm gameplay range remain unchanged.
The user's later manual wrist correction takes precedence over this generator.
`apply_manual_knife_grip.py` records hand_r local rotation
(Roll -46.2194, Pitch -10.9159, Yaw -8.93675 degrees), recovered from the numeric
Bone panel because ordinary Save had left it in preview only. It binds the real
knife data asset to ManualReady/ManualStab, preserving every other local ready
bone and compensating the FPS attachment to retain the knife trajectory. A saved
`ERKnifeManualGripApproved` flag makes subsequent imports/setups reuse this
manual correction rather than running the automatic grip fitter again.
The Milkor launches a replicated physical
`LaunchedHE`, using the supplied `GL_slug1` geometry. Its 1750cm/s launch speed
exceeds the 880cm/s thrown HE/smoke speed; gravity, bouncing/tumbling and the
3.5s fuse remain authoritative. Impacts never detonate this payload. HE damage still checks static cover and
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
finger poses (with the later fitted pistol thumb), sockets and component framing. The fitted hands use a 30Hz sampling
grid compatible with their existing Unreal compression target. Wrist-to-palm
offsets rotate with the wrist. The latest pistol retarget explicitly uses its
actual absolute weapon scale1 and the new canted motion frame; other weapon
adaptations retain their existing fitted trajectories. Out-of-reach poses translate the hidden FPS shoulder
instead of separating the palm from its target; arm segment lengths are retained.
This is an adaptation rather than a lossless full-body retarget.

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

The 15 October presentation revision freezes every mechanical idle bone at its
ready pose. The fitted skin forearms use transverse scale .82 for one-handed
weapons and .95 for rifles (formerly .5/.6); existing hand proportions and skin
geometry remain. Each weapon has an owned fitted idle. World support arms use
two-bone IK after locomotion/prone. The later anatomical correction measures
each skin's actual palm surface from hand-weighted vertices and finger joints.
`setup_palm_contacts.py` adds mesh-specific `ER_Palm_L/R` sockets to the twenty
world and twenty FPS meshes; existing physical `HandGrip` sockets and geometry
remain intact. Five rifles sample their idle-deformed underside triangles,
choosing a reachable measured contact and orienting the palm against that surface.
Their right arm retracts 12 cm to seat the stock. The Milkor retains its earlier
support segment. Equipping invalidates a stationary mesh's cached pose.
The pistol retains the relaxed arm pose after the user rejected the steep
centered arm. The latest supplied viewer image supersedes the earlier assault-tip
framing: the grip has no sideways roll and the bore converges on the central
camera aim ray at150cm, reproducing the visible barrel perspective. This is
cosmetic; server aim/spread remain unchanged. The muzzle is46cm from the
camera (assault about74.8cm) and projects near(58.2%,64.25%) of a16:9 viewport,
placing the front sight near60% height. This raises and slightly compacts the
previous44cm pose while showing little forearm. The palm sits5cm higher on the
backstrap than the earlier fit, with3.5mm handle embed. `fit_pistol_presentation.py` bends the three
right-thumb joints around the backstrap toward the opposite grip surface,
preserving phalanx lengths. Contact placement compensates for the half-scale
hand socket and the rifle component's absolute scale1. These fitted thumb keys carry into fire/reload;
other fingers retain their existing poses. Private `pistol-presentation.json`
records framing and thumb geometry targets. The user's viewer screenshot is
preserved without edits privately in
`SourceAssets/SketchfabWeapons/References/pistol-viewer-2026-10-15.png`.
The FPS instance retains fitted animation translations, including hidden-shoulder
reload motion. World pistol
attachment and authoritative aim are unchanged.
The AR15 grip origin is calibrated to its pistol grip, and its handless source
reload uses the actual animated magazine bone for the support palm trajectory.
The Milkor derivative omits GL_scope and consolidates the populated source UV
layer per material into UV0, including the drum and vertical grip.
The repaired smoothness/glossiness graphs explicitly connect the OneMinus input;
the rendered regression rejects weapon material compilation failures on Metal.

Stationary exhibition bots draw all ten slots without replacement within each
batch of ten, keeping the chosen slot on their controller through respawn.
Patrol bots and the explicitly selected legacy fixture retain their loadouts.

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
   reimports generated meshes; ordinary reruns preserve them.
   `-ERRepairReferenceMaterials` explicitly rebuilds the owned material graphs.
   Interchange retains
   existing animation packages/reference poses during mesh reimport: changed clip
   or grip normalization requires a new derivative namespace. The calibrated
   gun/knife generation uses `V3`, with `V4` for AR15/SMG resampled from 24/60 Hz
   onto the same 30 Hz grid as their hand clips. Earlier generations remain private.
   Palm calibration runs before import and writes its private geometry receipt
   to `SourceAssets/SketchfabWeapons/palm-contact-report.json`.
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


### Grenade combat revision, 16 October

Both HE and LaunchedHE use 140 peak damage inside 200cm and
`140 * (1 - (distance_cm - 200) / 400)^2` between 200 and 600cm, then zero.
Distances are 3D distances to the character's capsule centre. Three server
WorldStatic traces sample head, torso and lower body before any block damage;
visible samples weight exposure. Full cover prevents injury and partial cover
reduces it. This is a gameplay approximation rather than individual fragments.
Blast ArmorRatio is clamped to 1.6..2, so full armor reduces exposed damage by
at most 20%, consuming armor through the existing accounting. At default 1.6,
close damage is 112HP even with 100AP. Bullet armor/range rules stay unchanged.
Structural block damage keeps its own 3.5m envelope; 6m is the fragmentation
injury envelope, not a building-demolition radius.

Hand HE/smoke/stun use a 3.5s fuse from press, arm and consume ammunition on fire press. A real replicated
projectile owns the server fuse while held, follows a swept shoulder position and stays hidden as a separate projectile.
The third-person weapon mesh remains visible in the raised, bent-arm cook pose. Release launches that same actor
with the remaining fuse, from 14cm behind/24cm right/4cm below the eye when
clear, adding upward throw bias and player momentum. A sphere sweep clips the
origin against walls/ceilings; no client-selected launch transform or cook time.
Holding through the deadline detonates in hand; death/interrupted traversal
drops the live grenade without refund/reset. Switching/reloading is blocked
while cooking. The holding FPS arm and weapon hide immediately on local press,
with replicated held/recovery state for peers; a spare becomes visible again
after recovery. Milkor retains normal press-to-fire behavior.

Run scripts/setup_grenade_combat.py through Unreal once to migrate existing
profiles, then test-grenades.py --combat (also --render).

### Worksheet recoil

The six worksheet firearms also import exact hip/Alt recoil magnitude, magnitude
variance and angle variance plus the Simple Random/Set Pattern label. Use
`setup_weapon_recoil.py` through Unreal for an existing install, saving only six
data assets. See [WEAPON_RECOIL.md](WEAPON_RECOIL.md) for source cells, authoritative
view/shot integration and the declared project patterns/return response. The
workbook supplies no original spray path, seed or recoil decay settings.
