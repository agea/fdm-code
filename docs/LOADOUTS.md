# Closed loadouts

Five stable preset IDs (`breacher`, `rifleman`, `marksman`, `sniper`, `grenadier`)
and three variants (`gunner`, `offense`, `defense`) resolve through ERLoadouts.
Rifleman/Offense is the safe default. Config/DefaultGame.ini exposes initial
primary/secondary ammunition and stable team skin names. Quantities are rounds.
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
