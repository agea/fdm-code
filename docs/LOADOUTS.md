# Closed loadouts

Five stable preset IDs (`breacher`, `rifleman`, `marksman`, `sniper`, `grenadier`)
and three variants (`gunner`, `offense`, `defense`) resolve through ERLoadouts.
Preset skins (same for all three variants; index in parentheses):

| Preset | Alpha | Omega |
|---|---|---|
| breacher | Wraith · Assault (1) | Drongo · Jackal (1) |
| rifleman | Belica · Arctic Patrol (6) | Drongo · Raider (0) |
| marksman | Wraith · Patrol (2) | Gadget · Mechanic (6) |
| sniper | Wraith · Officer (0) | Gadget · Patrol (7) |
| grenadier | Belica · Armored (8) | Gadget · Field Engineer (8) |

The remaining appearances stay selectable in the catalog.
Rifleman/Offense is the safe default. Config/DefaultGame.ini exposes initial
primary/secondary base ammunition and stable team skin names. Quantities are rounds.
The resolver doubles total carried ammunition for every preset/variant: firearm
loaded counts stay unchanged and the extra rounds go into reserve. For example,
rifleman 30+90 becomes 30+210, pistol 7+14 becomes 7+35, and SMG 30+60 becomes
30+150. Grenadier carries 6+6 (gunner) or 4+4 (offense/defense).
Utility stacks double directly: offense gets 4 HE + 2 smoke; defense gets
4 smoke + 2 claymores. Gunner still has no utility and the knife is unchanged.
Both menu/server resolution and spawned inventory use these same totals.
Existing config values are base quantities, not pre-multiplied supplies.
These defaults and Claymore tuning are initial implementation choices, not playtest findings.

The server resolves inventory and skin together on spawn. GameState replicates
the resolved catalog, so the menu uses server quantities even if a client changes
its local configuration. Asset names identify weapons independently of catalog order. The menu maintains a
local draft; Apply confirms the next spawn, leaving the live pawn untouched.
Preferences are versioned in the existing EclipseRecoil.Loadout section; old
starting-weapon indices3..7 migrate to the five roles with Offense. Free supply
and skin RPCs only work in the explicit ERLegacyArsenal fixture.

Input preserves weapon identity1..8 and F/G for HE/Smoke, skipping absent or empty
utility. H selects the carried Claymore. Wheel selection uses owned inventory.
No normal kit contains Stun. ERFullArsenalTest is an explicit ten-slot development
fixture for historical firearm probes, distinct from ordinary play.

Claymore fire starts a server-authoritative 2.2s placement action: smooth crouch,
deposit at 1.3s, rise from 1.6s, completion at 2.2s. The device is placed 65cm
in front of the feet, following view yaw independently of pitch. The existing
horizontal reach, valid ground, cover and protected-spawn checks still apply.
Movement, jumping and weapon selection are blocked during placement. Releasing
fire does not cancel; repeated presses cannot start another action. Consumption
occurs once on successful deposit, never at action start. Death/round cleanup
cancels the unfinished action. Failed validation explains the reason in a HUD
toast. The server revalidates the surface on deposit. Posture and action state
replicate, using the existing smooth crouch and camera movement.

Claymores are server actors owned by session PlayerState, independent of the
pawn and inventory cleanup. They arm after two seconds and expire silently60
seconds after arming. No active count limit, FIFO or pool replacement exists.
Activation uses a 200cm ground-plane radius and a 60° full sector (±30°),
with a 200cm height limit and intact-world line of sight. Capsule height does not
produce a close-range blind spot. Damage uses its independent 110° forward sector,
800cm falloff and line of sight; rear damage is zero. Enemy targets eligible to
trigger the mine receive lethal damage even at the sector edge with full health,
armor and helmet. This uses normal damage/death/kill attribution; targets outside
the activation sector keep the existing falloff. Two collision-free emissive
rays delimit the activation sector, stop at solid cover, and use Alpha blue/Omega
orange. Rays appear when armed on each rendering client; the server replicates
the detection range/angle so client configuration cannot move their boundaries.
PlayerState replicates the active count even when individual mines are irrelevant.
Logout, team change, round end and map regeneration clear affected mines.
Surface loss disarms them. Fire/explosions neutralize without a chain detonation.
Damage/cone/radii/structural settings are dedicated configurable initial values;
HE's140 peak damage and1.6 armor ratio are the initial player-damage baseline.

The local menu preview uses only collision-free mesh components and a frozen
presentation pose. The camera fits the actual weapon/display bounds and preserves user rotation
across variant changes. Capture happens on selection/rotation; closing destroys the
scene. The same saved weapon mesh and GripTransform drive the primary. Remaining
items occupy ordered display positions beside the character. Rig-specific grip
quality must be assessed visually in Unreal, separately from compilation.

The imported Claymore is [Claymore m18 by zhezhela](https://sketchfab.com/3d-models/claymore-m18-0e44b2adeb974830804c3ec68c0920f0),
CC BY4.0. Original ZIP/DAE/textures, metadata and derived FBX live in the private
weapons SourceAssets/ClaymoreM18 tree. bake_claymore.py uses Blender3.6;
import_claymore.py and setup_loadouts.py save only owned assets through Unreal.
The latter makes a timestamped rollback ZIP before changing existing packages.
No character regeneration is involved. Source ballistics JSON remains immutable;
weapon_gameplay_overrides.py reapplies Shotgun8x22/0.35s after reference imports.

Validation entry points: scripts/test-loadouts.py, --network, --render and
--network --mines (real staggered60s deadlines and a posthumous kill).
See TEST_RESULTS.md for executed outcomes and remaining visual/network checks.

The selection menu uses the supplied HUD weapon icons for role choices and a
single equipment list. Variant buttons contain names only: obsolete hardcoded
recipes and the duplicate Loaded/Reserve summary have been removed. Every item
and quantity is read from ResolveMenuLoadout (the server catalog when online).
Firearms use the same magazine / total convention as the HUD, utility shows
carried quantities and melee has no ammunition count. Selection highlights,
confirmed next-spawn selection and the apply action are presented separately;
a different equipped kit is shown only while waiting for the next spawn.
The rotatable character preview remains available in a more compact panel.

### Drongo Scavenger on the active Breacher preset (30 October)

Omega1 keeps the stable label Drongo · Jackal but now shows Scavenger's original
round goggles, yellow skull bandana and shoulder pad. Material slot0 is named
M00_Teddybear although it contains those costume parts; hiding it removed them.
The original mask material/texture packages were recovered from characters commit
0f177de using Git LFS and SHA256 verification. An owned matte material preserves
the original skull colours. The additional GDC gas-mask attachment is removed
from this skin only. Weapon5 and teddy prop6 stay hidden. Raider/Rifleman remains
unchanged, as do all body/FPS meshes, rigs, physics, catalog indices and role IDs.

setup_drongo_scavenger_head.py applies the change to player/target/corpse catalogs;
setup_drongo_masks.py respects the ERScavengerOriginalHead marker. Receipts and
311 protected original package hashes: Saved/Testing/DrongoScavenger.

The mask uses the original atlas stencil (`T_Drongo_Scavenger_Upperbody_Pattern.R`)
explicitly as white BaseColor paint, with dark lenses/shells and zero emissive.
Its owned rubber function repairs an empty imported texture default; original
material/function packages are unchanged. The menu preview mesh now forces its
own texture mips while active (`bForceMipStreaming`), preserving scene-capture
detail without disabling streaming globally. The ordinary preview was inspected
through frame35 after the texture requests completed.

### Gadget Marksman / Sniper separation (30 October)

> Superseded by the later user selection below: Sniper now uses gallery model8, Gadget · Patrol (index7).

Marksman keeps Omega6 Gadget · Mechanic (Guerilla, visible face/exposed waist).
Sniper keeps the stable Omega9 Gadget · Veteran identity but now wears closed
Shellshock armor/helmet, with a gray upper body and orange lower plates. Source
Engineer8, all other choices and role IDs remain unchanged. Independent body,
FPS, physics and material copies preserve the approved Shellshock bindings.
See setup_gadget_sniper_shellshock.py and Saved/Testing/GadgetMarksmanSniper.

### Sniper uses gallery model8 (30 October, latest correction)

Omega Sniper now selects the existing **Gadget · Patrol, index7 / gallery8**, with
its approved light camouflage. Both DefaultGame.ini and ERLoadouts' native
name/index fallbacks use Patrol for all three Sniper variants. Marksman remains
Mechanic6; Grenadier remains Field Engineer8. No character asset, palette, rig,
physics or catalog entry changed. The prior Veteran9 Shellshock study remains
an optional catalog appearance and is no longer assigned to Sniper.
