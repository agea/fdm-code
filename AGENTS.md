# Respawn & Rage Unreal development

World weapon poses (26 October, latest): third-person firearms use geometry-
derived right/left palm contacts and absolute per-weapon scale, independent of
the fitted FPS rig. Neutral rifles sit at right-elbow height, with the stock
passing below the upper arm outside the chest; scope raises the weapon to the
eye through a smooth blend. Pistols use
a dedicated two-hand cup contact. Fit both arms with bounded two-bone IK on every
one of the 20 selectable skins. For long guns, turn the chest 30 degrees in
neutral / 15 degrees in ADS plus clavicles 25 / 30 degrees (55 / 45 total).
Counterturn the neck so the face keeps the aim direction. Keep the neutral
stock line in the arm/torso opening at half the original shoulder span minus 1 cm
(4 cm inward from the earlier outer-arm line);
it must pass between the body and right upper arm. Pistols retain their prior
clavicle-only pose. The stock shoulder stays behind the support shoulder;
the right wrist-to-knuckles axis points forward with a slight downward rake,
and the left palm supports the underside with fingers across the handguard.
Its measured contact is inset toward the body by up to 1.5 world cm, reduced
for narrow guards and rejected if it leaves the guard's actual underside.
Revision 6 advances this support station 4 world cm toward the muzzle from
revision 5, remeasuring the underside and retaining the lateral inset.
Revision 7 advances it by another 2 cm (6 total from revision 5) and seats the
palm socket another 2 cm toward character-left / weapon -Y to align the thumb.
The measured guard contact is the reference; this final palm seating offset
is intentional and need not itself lie on the guard centreline.
Revision 8 rotates the wrist-to-knuckles axis 30 degrees from weapon +Y toward
+X (weapon yaw 60 degrees), counterclockwise in the supplied overhead view.
Keep palm normal +Z and the revision-7 seating position. Weapon -Y folds this
hand backwards. Validate a positive
dot product between left forearm direction and the palm's longitudinal axis.
Phase-only refinement: its five appearances have WorldSupportPalmOffsetCm
=(0,2,0) in weapon axes / world cm, bringing the left hand back under the guard.
All other skins keep zero. Apply through ERWorldWeaponPose::SupportContact only
to long guns; do not edit shared weapon profiles or change the other grips.
setup_phase_world_support.py preserves appearance fields and weapon hashes.
Do not turn the whole right hand vertically downward. Selectable skins must
not add field-pack/backpack attachments. Preserve the underlying locomotion,
legs, head and finger animation. The world gun follows replicated base aim,
including +/-35-degree pitch. Do not reuse these world transforms for FPS, reload
clips, damage, muzzle data, spread or scope mechanics. `setup_world_weapon_pose.py`
may update only the seven firearm presentation profiles, creates verified package
backups and rejects changes to protected gameplay/FPS fields. Validate13 cases per
skin locally and on a real client (260+260) with `test-world-poses.py`, then retain
reference-weapons and scope regressions. See docs/WORLD_WEAPON_POSES.md.

Map variety (26 October, latest): supersedes the dense Rooms screen counts,
three shared themes, origin-only central building and previous material HP.
Rooms use 22/20/18m nominal rooms (16–24m with variation), real doglegs and turning
chambers, a lower spine under a 7.2m transverse corridor, and 4m galleries with
upper portals and switchback access. Long corridors alone get boards at 5m
intervals; rooms have at most two separated boards, rejected on access routes.
Keep paired geometry and real standing/floor clearance on every generated path.
Central Building OpenYard can move its core to +/-8m on X, equidistant from the
two base centres. Main pieces mirror about their own SymmetryCenter; other
pieces retain the world half-turn. OpenYard upper floors shrink 5m/2m per storey
and use paired terrace ladders, with ground stairs retained. Shared themes are
Industrial (merged Warehouse roles) and Desert. Both keep shell sheets, wall
materials, warm rusty steel columns, light galvanized ladders and preserved
cargo paint distinct. Reuse existing
art. All seven native/saved material HP values become 65% of the previous
preset; preserve penetration, multipliers, thickness and actor damage tuning.
setup_map_variety.py backs up the four owned assets and saves recipe/library
revisions plus damage preset2, protecting maps and other recipes. Validate full
destruction tests, real upper portal traversal, Metal and initial/late LAN peers.

Rooms timber screens (25 October, latest): replace crates with destructible wood
panels, 240cm high and one calibrated 2cm wood depth. Corridors use three
alternating baffles (four in the self-symmetric cross-link), blocking standing
sightlines while keeping a standing zigzag passage. Ordinary rooms have four
L pockets; bases/gallery rooms have two, preserving spawns and ramps. Preserve
half-turn geometry, existing wood materials/damage and shared themes. Validate
screen clearance on actual detour routes, corridor sightlines, live pawn travel
and LAN damage/regeneration. setup_rooms_screens.py saves DA_Rooms revision3
with a revision2 backup and protected-asset hashes.

Warehouse steel variety (25 October, latest): the shared Warehouse theme must
separate shell sheets, plated court/room walls, dark structural frames, light
galvanized ladders/parapets, dark riveted decks and the roof. Reuse the installed
warehouse, PixelatoR steel, Iron Plates2K and Factory beam textures. Owned
WarehouseTheme parents/instances adapt broad tint and roughness for the game's
low-reflection profile; keep original materials/textures unchanged. Optional
ShellSteel/LadderSteel/DeckSteel theme fields enable these roles only for
Warehouse; Physical EERBlockMaterial, HP, support and collision stay unchanged.
Use authored ladder frames for their panel finish, preserve reinforced cargo's
original paint, and apply the detail finish to actual fence render components.
setup_warehouse_steel.py backs up DA_MapThemes and updates only Warehouse/library
revision2 after candidate checks; preserve all four recipe packages and the
other theme palettes. Validate SharedMapThemes/physical regressions, real Metal
near/far shots and Warehouse LAN seeds7919/1001.

Fire Training high walkways (25 October, follow-up): put the connected steel
routes as high as standing movement permits, immediately below the lighting.
WarehouseCatwalkHeight uses the real roof/lamp bounds, 200cm clearance and5cm
rounding (about4/7/10m for2/3/4 bands). Multi-nave crossings also respect the
low roof valleys. Flanks start120cm from the outer walls, past85cm knee braces.
Keep base roofs at3m. Four end ladders step through notched cap rails at deck+5cm
instead of the ordinary110cm parapet overshoot, retaining real upper handles
and the shared movement implementation. Native tests check full ascent/top
transition; the Metal probe climbs a real ladder with ordinary movement before
capturing the gallery. Route migration marker2 backs up and advances only the
current Fire Training recipe; retain the themes, kit, base/cover rules and
all earlier layout budgets. The old250cm headroom rule below remains for local
base/cargo decks; high walkways use200cm below the limiting roof/lamp surface.

Fire Training bases/routes (25 October, latest): supersedes optional local
catwalks, stair access and upper timber deck rooms below. Always generate two
concrete-walled courts with offset gates and an enterable one-storey barracks,
all ten starts inside, a walkable3m roof and two exterior ladders per base.
Two2.2m steel flank walkways run most of the hall length, joined by mandatory
cross-map bridges. Multi-nave even-bay halls put paired crossbars at+/-210cm,
avoiding column rows; otherwise cross at0. Four ground ladders reach the network.
Use FireTraining.ParapetModule (original industrial barrier) at127cm with no
plinth, physical pawn collision and original triangle shot apertures. Notch only
ladder mouths and junctions. Preserve half-turn pairing, size/theme tuning,
open-volume budgets, ground timber rooms/corridors, cargo/cover and shared combat.
setup_fire_training_routes.py uses a feature marker, backs up the current recipe
and advances its revision after516-seed validation (shared themes already raised
it to4). Validate native geometry/physics, Metal gallery and --network initial,
late and regenerated geometry through test-fire-training.py.

Fire Training (23 October, latest map request): new production template
DA_FireTraining/L_FireTraining, archetype FireTraining, from the owner's
warehouse ZIP and RR_Warehouse_Killhouse_OpenSpace_Istruzioni.md (both private in
maps SourceAssets/FireTraining; licence unknown, keep the warning). One open
hall only: 1–4 side-by-side naves of 16/20/24m complete kits (open column
lines + gutter between naves, never a wall), 14–36 bays of 4.2m, 1300–7000m²,
length <=7x width, 2–4 bands (revision3; migrations setup_fire_training_length.py
then setup_fire_training_scale.py); larger halls reserve empty plazas, 2–3 wall bands of 3m for height; never stretch modules or add an
upper floor (ContinuousUpperFloors is rejected). Interior structures stay local:
catwalk/bridge, timber corridor and rooms, deck room/lookout, containers and
stacks with ladders, low cover; half-turn symmetric. Budgets on the union
raster: elevated <=30%, overhead <=40%, open-to-roof region >=45% (narrow
walkways do not split it). Two accesses per elevated area; 2.5m headroom
under WarehouseRoofHeight. Bounded deterministic retries end in the verified
reference layout (attempt in the digest). Kit modules keep unit scale, sheets
use two-sided complex collision plus hidden backstops; roof groups are hidden
from the minimap capture and recon flies under the eave (Frame.Indoor). Use
bake_fire_training_kit.py then setup_fire_training.py (preserves completed
assets, refuses a changed kit). Validate with test-fire-training.py and
--render. See docs/FIRE_TRAINING.md.

Claymore lethality (latest): enemies eligible to activate the mine must die even
with full health/armor/helmet at the 2m/60-degree sector edge. Apply lethal damage
through the normal armor/death/kill-credit pipeline, not direct health mutation.
Retain forward/LOS restrictions, rear zero damage, and ordinary falloff outside
the activation sector. Preserve protected-player immunity and owner attribution.

Claymore placement gesture (latest): fire runs a 2.2s crouch/deposit/rise action,
not an instant camera ray placement. Deposit at1.3s, begin standing at1.6s. Place
65cm ahead of the feet independent of pitch; retain safety/cover/spawn checks.
Server consumes exactly once on valid deposit. Lock walking/jump/selection during
the action; releasing fire does not cancel. Replicate posture/action; interruption
before deposit consumes nothing. Keep the existing mine arm/lifetime separate.

Claymore direction (23 October, latest): activation is exactly a 200cm ground-plane
sector with 60° total opening, ±30° from the authored +X front. Two emissive,
collision-free team-colored rays delimit this sector and stop at static cover.
Use ERRecon team colors (Alpha blue/Omega orange). Replicate trigger range/angle.
Keep blast direction independent: forward damage only, zero rear damage; standing
capsule height must not create a blind spot near the mine. Preserve lifetime,
ownership, no mine cap, and imported M18 geometry. setup_claymore_lasers.py owns
only M_ClaymoreLaser and is called by setup_loadouts.py.

Closed loadouts (23 October, latest): supersede the normal full-arsenal, free
supplies and independent-skin selection below. Five stable preset IDs each have
Gunner/Offense/Defense; server resolution, replicated catalog and next-spawn
confirmation share ERLoadouts. See docs/LOADOUTS.md. Claymores survive death and
respawn, have independent60s armed lifetimes, and never replace or cap existing
mines. Use zhezhela's CC BY4 M18 model from the private weapons mount. Preserve
serialized payload values, approved skins and explicit ERLegacyArsenal fixtures.
Shotgun8x22/.35s overrides apply after immutable reference data in every generator.

Industrial low barriers (23 October, latest request): supersede tall stacked
steel barriers with a fixed100cm concrete plinth plus exactly one127cm original-
height steel course (227cm total above the supporting floor). Apply on both
perimeter axes, the ground separator and each central-building floor separator.
Keep opaque steel, original triangle apertures, concrete palette and half-turn
pairing. Collision ends at the visible top. The user explicitly limited the
invalicability requirement to approach from the ground; roof jumps may cross.
Validate full base collision, jump-apex/150cm mantle rejection and no invisible
upper wall, deterministic seeds, Metal view and initial/late client regeneration.
No global movement/destruction calibration or imported source art changes.


Bullet decals (22 October, latest attachment): use the original owner-supplied
FourInkImpacts.png in private weapons SourceAssets/BulletImpacts. Four existing
material instances select its quadrants without modifying pixels/alpha.
setup_bullet_decals.py imports/reimports only the owned decal group; the full
hitscan setup delegates to it. Firearm cell hits use weapon multicast variants
instead of the previous material mark, including penetrated surfaces/pellets.
Preserve grenade contact/blast rules, pawn exclusion, 128 marks/20s and damage.
The earlier FourBulletImpacts.png remains a historical source.

Player wound decals (26 October, latest attachment): successful firearm point
damage to health or armor records one of the four owner-supplied wound1..4 PNGs at the skeletal
impact bone. The server chooses the variant and replicates bounded bone-local
state (32 marks) so animation and late clients agree. Protected/rejected damage,
melee and explosions add no wound mark. Copy the marks to the replicated corpse
when the player dies. Preserve every source PNG and its alpha; the last displayed
attachment is wound2.png and receives a fixed 180-degree decal rotation, while
wound1, wound3 and wound4 retain their authored orientation. setup_wound_decals.py
owns only Effects/Wounds. Render wound width/height at 40% of the initial size,
preserving aspect ratio and projection depth. Verify the orientation automation plus real host/client
damage, protected hits, corpse transfer and Metal material loading.
Armor-only damage uses one of four fabric tears from the original owner
ArmorImpacts.png atlas (setup_armor_decals.py, four quadrant instances). Any
positive health loss, including mixed health/armor damage, uses the four original
wounds. Replicate the selected eight-way variant and copy it to the corpse.

Inflicted damage notices (26 October): the server sends actual health/armor lost
to the attacking player only. Draw red HP and green AP text matching the circular
vitals under the reticle, newest first, maximum five rows. Hold full opacity for
one second then fade text and shadow over 0.2 seconds. Every new notice pushes
older rows down. Reject protected/zero-loss hits and self damage notifications.
Wound visibility refinement: disable UDecalComponent screen-size fading for
retained player/corpse marks; the default .01 hides the 40%-sized marks at combat
distances. Validate living-target Metal captures with ERTeamGallery/ERWoundGallery
at 2m and 6m. Damage notices use text scale .5 and 17-unit row spacing.
Subsequent wound projection correction: physics hit bodies do not match rendered
skin/clothing. Project along the bullet trace rather than the hull normal, with
40cm half-depth centred at the hit (measured gaps up to30cm; supersedes the shallow 3cm projector). Keep
visible width/height at40%. Resolve missing hit bones to the nearest body bone;
rebind retained marks after skin application. All held weapons, sights, muzzle
meshes, thrown weapon visuals and Claymore/placement meshes reject decals at
runtime, including serialized Blueprint components. Geometry audit in the wound
gallery compares physics contacts with skinned render triangles on25 chest rays.

Industrial wire removal (22 October, subsequent request): the user explicitly
authorized completely removing the obsolete industrial wire assets, superseding
the earlier reference/vendor retention rules for this retired fence family only.
Use cleanup_industrial_wire.py through Unreal after outside-reference and verified
rollback checks. Clear the historical FenceMesh slot, remove the old creator and
prune the Factory migration selection so reruns cannot restore the wire. Preserve
the aluminium FenceModule, shared steel/platform materials, Depot and Alleys.

Industrial barriers (22 October, latest request): replace Central Building's
runtime chain-link geometry with the single original straight module from
Nullified's Aluminium Fences 57 Modular Parts (CC BY4.0). Treat bars as opaque
steel with the existing SteelSurface, without chain-link textures or masks.
FenceModule instances fit the existing spans on both axes; preserve movement
barriers, routes, symmetry and fixed status. Use original triangles for bullet,
visibility and blast queries, keeping apertures open. Preserve source ZIP,
metadata, credits and derived FBX privately in SourceAssets/AluminiumFences;
setup_industrial_fence.py backs up the recipe and imports through Unreal.
Validate actual module collision, Metal close-up and initial/late LAN clients.
This supersedes the historical production diagonal-wire requirement below.


Alley architecture revision6 (22 October, explicit follow-up): make the twelve
Middle Eastern house shells, arches, internal masonry, low walls and added roof
decks destructible. Preserve original LOD0 corners/UVs/normals, doors, windows
and walkable interiors through baked UERDestructibleMesh assets and clipped
runtime skins. Original art is immutable. Use the existing server-only5cm HP,
blast, support graph and alive-mask snapshots. Cell protocol9 adds bounded
zlib framing to large shell masks; never raise network bunch limits. Imported shells keep hollow
triangle collision; authored masonry boxes keep solid aggregate collision.
Foundations at0/+180cm anchor bottom rows; upper parts need actual cell support.
Outer perimeter, terrain, tunnel retaining structure and solid stair foundations
remain fixed. setup_alley_destruction.py backs up revision5, bakes13 owned
structure assets and migrates only the own recipe. Validate native facade/roof
cuts and support loss, real firearm/HE and pawn traversal/fall, initial/late
peers, regeneration and Metal before/after captures. Do not claim a particular
number of grenades opens an entire doorway: HP/calibration are unchanged.


Team graffiti (22 October): supplied Alpha/Omega transparent PNGs belong to the
private maps assets. Sparse wall decals fade toward the central30% exclusion;
cosmetic RNG must not change geometry or collision. Rebuild/clear per map epoch
after local geometry is ready. Recon cases carry their team emblem on the lid
and play original FBX angular keys from replicated launch time, before ascent.
This supersedes the historical closed-static-case requirement. Keep originals,
CC BY attribution and stock/replacement authority. See setup_team_graffiti.py
and bake_recon_case_animation.py; verify build, recon LAN and Metal gallery.


Alley passages revision5 (22 October, subsequent request): reduce empty space
with symmetric low cover in the plaza and both districts, preserving circulation.
Add a real lower passage at -3.6m, seeded base-to-base or west-to-east with bends,
paired open ramps and two brick paving patches that can be breached. Terrain
must contain exact holes behind these cell panels, so damage opens a real fall
route. Limit negative construction bounds to this owned recipe. Preserve shared
combat calibration, source art, other maps, house/roof/tower access and half-turn
parity. setup_alley_passages.py backs up revision4 and saves only the own recipe.
Validate both lower routes/ramps, real pawn traversal/fall, grenade/road collision,
initial and late clients before/after damage/regeneration and Metal captures.

Alley core revision4 (22 October, subsequent request): vary central city walls
independently between zero, one and two. With no wall, 2–4 main approaches
join freely in an open center; do not claim an exact choke count there. Twin
walls enclose a wide open core, with houses/towers outside it and 2–4 actual
arches per wall. Preserve half-turn parity, bridges, roofs, enterable shells
and terrain. setup_alley_core.py backs up revision3 and saves only the own
recipe after seed validation. Verify each real wall and the open center with
capsule sweeps, complete routes, clients and four new aerial captures.

Alley district revision3 (22 October, subsequent request): supersedes the fixed
Dust-style street graph below for Middle Eastern Alleys. Generate 2–4 physical
inter-base passages, varying plazas, street winding and climbable sandstone
towers. Pair gameplay spaces/terrain/spawns/roof stairs/towers by a half-turn;
keep the original enterable house shells, sand palette and raised over/under
crossing. Count actual capsule passages and validate full base-to-base routes,
tower ladders/decks, deterministic reconstruction and initial/late clients.
Only the own recipe is migrated through setup_alley_district.py with rollback;
preserve house art, Depot, Central Building and the shared destruction catalog.
Capture four aerial seeds for comparison. Geometric symmetry is not a claim
of competitive balance or subjective playtesting.

Alley traversal revision2 (22 October, subsequent request): houses must admit
players wherever their actual openings and interior clearance permit. Supersedes
sealed fitted-box collision below. Use the twelve original mesh shells with
double-sided complex-as-simple collision and visible interior faces; never fill
their openings with box collision. Add reserved exterior switchback stairs to
several roofs and real graded sand terrain (north +1.2m, south +1.8m, core zero).
Preserve the bridge underpass and shared gameplay. Use the explicit backed-up
setup_alley_traversal.py migration through Unreal. Validate doors, roof stairs,
terrain support, real pawn entry/exit and up/down walking, plus initial/late
client reconstruction. Inspect Interior/Rooftop/Ramp Metal gallery captures.

Middle Eastern Alleys (22 October, latest map request): add the new production
template/map DA_MiddleEasternAlleys/L_MiddleEasternAlleys beside Central Building.
This supersedes the industrial-only map scope below. Use all three requested
RR sand sources and twelve Desert Mud Concrete House variants, with private
originals/metadata/credits in SourceAssets/MiddleEasternAlleys. Preserve Depot,
Central Building, the shared combat catalog and retired Arabian vendor content.
Reuse the owned town street graph with a B-to-Mid link, covered B tunnels and
3.2m Short A over a 2.9m underpass. Courts A/B remain deathmatch landmarks;
no new bomb/capture mode was requested. Building meshes use fitted fixed box
collision; steps/decks use matching solids. Visual RNG stays independent of
geometry. See docs/MIDDLE_EASTERN_ALLEYS.md. Run native layout/physics checks,
test-dust-template.py --alleys --render --network and destruction regressions;
distinguish geometric validation from competitive balance/playtesting.

Account-free Internet multiplayer (22 October, latest request): supersedes the
EAS/account-login requirement in the historical EOS section below. Online play
uses Connect Device ID, with bUseEAS=false, no browser/launcher login and the
existing untrusted client configuration. Never delete/reset a Device ID for a
test. scripts/prepare-eos-guest.py generates an ignored project override of the
locally licensed UE5.8.3 OnlineSubsystemEOS plugin, with version/source-hash
guards; only scripts/eos-guest project extension snippets belong in public Git.
Keep the installed engine untouched and never commit the generated plugin.
Build/open and isolated desktop/archive helpers prepare it automatically.
Supply the in-game nickname/UserLoginInfo and null Device ID token, create or
reuse the device credential, and refresh through Device ID instead of NULL's
external token path. Keep the standard OSS user mappings for Sessions/P2P.
Run test-online-service.py: two real launches, no Epic Account ID, same hashed
Product User ID, guest refresh, public session search/nickname, lobby/travel,
cancel/rehost and clean exit0. Its development-only refresh hook invokes the
actual expiration handler, without claiming a naturally elapsed token lifetime.
The user's current validation scope remains this Mac; distinct-device remote
JoinSession and two-network NAT/relay are still unverified. Preserve LAN/offline.

HUD visual refinement (21 October, subsequent request): shrink minimap radius
by15% to97.92 reference units (13.6% height at16:9), raise its top inset from90
to64 reference units (8.889% height), retaining its percentage right anchor.
Use darker shared sRGB vitals/icon colors: health154/84/84, armor86/132/100,
stamina82/130/154, with darker empty tracks. Keep all proportional scaling.

HUD viewport proportions (21 October, latest request): supersedes the fixed-pixel
minimap cap and clamped HUD scales below. Use ERHUDLayout for all gameplay HUD
and FPS dimensions: reference1280x720 coordinates are fractions of current
viewport width/height; horizontal and vertical edge offsets scale independently.
Use min(width/1280,height/720) without min/max pixel caps for isotropic text,
circles, bars, markers, strokes and shadows. Measure text before corner anchoring;
recompute from the live viewport on resize/fullscreen changes, independent of
3D dynamic resolution and the1080p scene cap. Preserve the real FOV/spread
projection and40m recon world radius/look-ahead. Run scripts/test-hud.py and
--render; inspect the seven live resized HUD/scoreboard capture pairs.

Vitals/minimap revision (21 October, latest request): supersedes the earlier
HP/AP text and labeled stamina layout. Three equal proportional Canvas bars,
top to bottom health red, armor green, stamina light blue, with left-hand
medical cross/shield/running-person silhouettes and no vitals text or numbers.
Use darker pastel red/green/light blue for both bars and icons; the runner
supersedes the initial lightning icon.
Stamina stays blue while sprinting. Keep the shared viewport-scaled40 inset
and posture prompt above the group; use the existing replicated values.
Code-drawn icons require no texture assets. The heading-up minimap retains its
40m radius, but centers26m ahead along local yaw: owner at65% radius below
disc center with35% lower margin. Translate terrain/fog/contacts together;
preserve server visibility, world fog, capture and clipping behavior.

Bullet aim punch and damage HUD (21 October, latest request): supersedes ALL
older cosmetic-only damage-flinch rules below and in DEVELOPMENT.md. Positive,
nonlethal server-accepted Bullet point damage adds pitch/yaw to the pawn's
GetEffectiveAimRotation/GetViewRotation, shared by camera/FPS rig and authoritative
weapon fire before unchanged spread. ControlRotation/capsule/hitboxes stay intact.
Keep worksheet firing recoil independent; never add cosmetic bullet pitch again.
FERAimPunchTuning owns all defaults (.4 + .012/HP, head1.5, mitigated armor.6,
yaw.35, caps2.5/1.5deg, half-life.2s). Use actual HP lost/bone/armor consumption.
Pellets share the base term per accepted weapon shot; their actual HP terms add.
AuthorityPunch is gameplay; reliable owner-only ClientDamageFeedback carries
absolute post-hit snapshots, sequence and server time. OwnerPunch ages them on
a dedicated latest-sample server clock plus measured half-RTT and monotonic
recovery; leave existing averaged match/recoil clocks unchanged. Reject duplicate or
older events; never accept client offsets.
Reset on death/possession; fresh pawns and match restarts start empty. Bots use
the same final firing accessor. Damage HUD draws transient red directional arcs
(.65s) and feathered edge flashes (.45s), owner view only, without texture assets.
See docs/AIM_PUNCH.md; run test-aim-punch.py --automation, default, --lag 100
and --render, plus network/accuracy/recoil/arsenal regressions.
Record the unavoidable pre-notification latency interval; do not claim zero-latency
camera/trace agreement, physical two-PC LAN or subjective playtest without evidence.

Audio pass (21 October, latest request): see docs/AUDIO_PASS.md. Six current
Bullet weapon data assets reference separate real-recording palettes (4/6/4/6/4/4)
under the private weapons Audio mount. Launcher retains one existing Epic take;
never claim extra recordings. Preserve server-only firing and the existing shot
multicast, whose accepted weapon-data snapshot avoids property-replication races.
No variant indices or client-selected audio/gameplay authority. Shuffle bags use
an independent cosmetic RNG and exclude immediate repeats across bag boundaries.
SoundCue graph edits/saves go through Unreal; keep authoring references intact.
UERFootsteps emits authority-distance-based grounded transient events for humans
and bots. Clients play once, owner 2D/others 3D; keep camera bob independent.
Surface/fallback mappings and levels belong in DA_ER_Footsteps, including cell
material IDs; never change HP/collision for audio. Preserve current crouch/prone,
sprint, menus, death and traversal gates. No looping walk track or landing double.
Smoke ReleaseVolume starts at the prior .65 * .31622777 (~.205548), local only;
preserve later Blueprint tuning on all regeneration. Keep HE, original/runtime
Sonniss WAVs, 6s audio/12s cloud, offset and cleanup unchanged. Originals/licenses/
hashes stay in private SourceAssets/AudioPass; retain swuing/Eelke/congusbongus
CC BY3.0 credits. Tabasco's conflicting license is not cleared or imported.
Run test-audio.py, --network --render, network --shot-feedback, movement and
grenades --render --audio. Headless tests never establish subjective listening.

Worksheet recoil (21 October, subsequent request): six Bullet firearms now use
bUseWorksheetRecoil, HipRecoil/ScopedRecoil, exact magnitude and both variances
from the same XLSX. See docs/WEAPON_RECOIL.md for the explicit project response
and pattern adaptation; the sheet supplies neither spray seeds nor decay curves.
This supersedes the earlier identical cosmetic .35/.65 kick and .2 crouched
scope recoil bonus for these six guns. Keep recoil server-authoritative: shared
character state, owner-only snapshots, synchronized clock, GetViewRotation for
both camera/FPS rig and authoritative eyes. Never change ControlRotation or
apply the kick twice. Each authorized shell adds one impulse after its own ray
is sampled, using the scoped mode before AWP unscopes. Holstering retains the
shared residual; rejected shots add nothing. Bullet flinch now follows the
aim-punch section above; other damage retains its previous cosmetic response.
Migrate only six data assets through Unreal with setup_weapon_recoil.py.

Worksheet accuracy (21 October, latest request): all six current Bullet guns use
bUseWorksheetAccuracy and HipAccuracy/ScopedAccuracy from the preserved CS2 XLSX.
See docs/WEAPON_ACCURACY.md and the numeric extraction. This supersedes the old
custom rosette, zero settled scope cone, .65 prone bonus, flat sprint +1.5deg,
linear/capped bloom and 4x crouch recovery for these six guns. Raw offsets are
scaled tangent units: half-angle = degrees(atan(sum/1000)); the sheet's Analysis
angles are FULL angles. Keep spread and inaccuracy as independent sampled offsets.
Use source Fire impulses, 90%-recovery times, AK initial/final interpolation,
Alt scoped fields and source cycle times. Actual cm/s converts to Source units
at2.54cm/unit; stance interpolation follows the physical capsule. Prone reuses
crouch because no prone row exists. Landing is authority measured, owner state
replicated, and recovery stays continuous when posture changes. Legacy/utility
without worksheet rows keeps its previous behavior. Migrate only six data assets
with setup_weapon_accuracy.py; never rerun the art import for numeric tuning.

Weapon ballistics (21 October, latest request): all six current Bullet firearms
use skeletal hit zones and the owner-supplied CS2 workbook dated March18,2026.
Full original: weapons SourceAssets/References; exact values/cells/hash:
docs/references/cs2-weapon-ballistics-2026-03-18.json. Head: deagle3.9, others4;
stomach1.25, chest/arms1, legs.75. Keep RangeModifier^(distance_cm/1270), armor
and helmet/leg rules. This supersedes the earlier shotgun-only opt-in. Single
bullets keep their sampled direction without a shotgun rosette. Knife/blasts
have no worksheet entries and retain their existing paths. Cover now subtracts
each crossed cell's CURRENT HP from a shared bullet damage budget (100-60=40),
stopping at zero or surviving/indestructible cover. This supersedes entry/cm
costs and structural bullet multipliers. Preserve HP, reinforcement and blasts.
Residual damage receives distance, hit zone and armor once. Save only owned
firearm data through Unreal with setup_weapon_hit_zones.py.

Unused map textures (20 October, subsequent request): remove the four inactive
Sketchfab library finishes and three unreferenced legacy floor/ceiling sets,
including their active sources and menu credits. This supersedes earlier source
retention for those seven sets only. Preserve every referenced material and all
seeded alternatives. Use cleanup_unused_map_textures.py through Unreal after an
outside-reference check and verified private rollback archive. Current selection:
ten Sketchfab sources / twelve materials, seven CC0 sets plus authored Glass.
Do not reacquire/reimport the discarded sets on routine pipeline reruns.

Sketchfab textures (20 October): the user authorized the RR collection's texture
downloads/replacements and seeded asphalt, grass or snow exteriors. See
docs/SKETCHFAB_TEXTURES.md. Keep original ZIPs, metadata, source/license/author
links and hashes privately in maps SourceAssets/SketchfabTextures; retain credits
in material metadata and Branding/Credits.txt. Use the new texture pipeline and
recipe7 palettes; preserve shared combat profiles, Factory originals and Depot.
Palette choices must not consume geometry RNG and must agree on all LAN peers.
Check UV connections and preserve 16-bit grayscale intensities during conversion.
Do not rerun historical Factory finishes over the completed Sketchfab palette.

Distribution (19 October): the user now authorizes Mac/Linux builds and public
distribution on the existing itch.io page. This supersedes the earlier packaging
deferral for these platforms. Linux builds run on the existing Fargate builder
and publish via its configured itch channel; Mac universal builds use the local
isolated workspace. Mac distribution must use Developer ID signing, Apple
notarization, ticket stapling and Gatekeeper verification before upload. Never
disable Gatekeeper or remove quarantine as the distribution solution. See
docs/BUILD.md and scripts/notarize-macos.py. Keep credentials in Keychain/SSO.

Menu music (19 October): use the original owner-supplied WAV in private maps
SourceAssets/MenuMusic and setup_menu_music.py. Bind the looping Bink quality80
SoundWave through BP_ERPlayerController defaults. Options expose a saved 0..100%
music slider (zero disables playback), applied live. Play locally while a menu
is open, at half gain in the in-game Esc menu; stop/destroy on resume or travel.

Brand: **Respawn & Rage**. Tagline: **Serious guns. Questionable players**.
The 15 October rebrand supersedes earlier project naming. Workspace:
`/Users/agea/git/respawn-and-rage/core`; GitHub: `respawn-and-rage/core`
and private `assets-characters`, `assets-weapons`, `assets-maps` repositories.
The user authorized moving the active project and updating these repositories.


Use `docs/GAMEPLAY_BASE.md` as the gameplay specification and
`docs/DEVELOPMENT.md` for the user's current scope and implementation status.
The user's current request takes precedence over the attached specification:
work on a locally playable Unreal alternative; defer packaging, distribution,
CI, external publication and standalone platform builds.

14 October publication scope: the user explicitly authorized public respawn-and-rage/core
and three private asset submodules (characters, weapons, maps), preserving the
original private archive. See docs/ASSET_SUBMODULES.md. This supersedes the
earlier publication deferral for these four repositories only. Keep licensed
binary assets out of every public commit; do not choose a code license implicitly.
Content/SourceAssets are generated mounts into Assets/*; edit/save via Unreal
at unchanged /Game paths, commit private assets first and then their gitlinks.
Do not run old cleanup/migration commands against linked vendor folders without
reviewing their new repository ownership.

- Keep UE 5.8.3. The project is `RespawnAndRage.uproject`; build targets are
  `RespawnAndRage` and `RespawnAndRageEditor`. Preserve the internal native
  module `fdm`, `/Script/fdm`, `/Game/EclipseRecoil` and saved settings keys for
  serialized asset/player-preference compatibility. Do not expose them as branding.
- Do not modify the original game under `/Users/agea/git/csgopen/eclipse-recoil`.
- Combat, ammo, teams, scoring, protection and respawn are server authoritative.
- Point damage supplies server-authorized shot direction to owner hit feedback.
  Bullet damage changes effective aim as specified above; never move the capsule
  or change ControlRotation for flinch.
- Death spawns a separate replicated cosmetic team-character corpse. Ragdoll contacts
  only WorldStatic, never players/projectiles; retain it through respawn, settle
  after five seconds, cap twelve/30 seconds and clear on round reset. Enforce
  the 30-second lifetime on the server in BeginPlay, including saved Blueprints.
- Rifle spread is real and server sampled; the ammo reticle reads that same
  cone plus replicated owner burst state against the synchronized server clock.
  Keep posture, speed, airborne and burst penalties shared with the HUD.
  Sprint adds up to 1.5 degrees beyond the ordinary cone cap, scaled by actual
  horizontal speed between 10 and 18km/h (19 October). Apply to all weapons,
  including optical/iron aiming; ordinary run/walk and low-posture tuning stay
  unchanged. Never flatten this penalty with the ordinary 10-degree clamp.
  No client-selected shot direction or cosmetic-only dispersion indicator.
- Enhanced Input lives on the controller so dead players can request respawn.
- Crouch accepts Ctrl and C. `scripts/fix_controls.py` adds C to an existing
  mapping without replacing the other bindings. `-ERInputAudit` is an opt-in
  graphical-test log; distinguish observed inputs from headless probe results.
- Opt-in local test bots were requested by the user. Keep normal LAN hosting
  free of bots; bots share player combat, team, scoring and respawn rules.
- Movement scope (13 October): 185cm standing capsule, 10km/h default run,
  Shift walks at 5km/h, acceleration/braking inertia and a lower, faster jump.
  Step automatically through 30cm; timed weapon-held vault through 80cm;
  Space+forward climbs through 150cm relative to current feet, including in air.
- Sprint/posture scope (14 October): double W within 300ms, held second press,
  runs at 18km/h for at most five seconds, followed by recovery lasting twice the actual sprint duration
  (up to ten seconds), including early interruption. Require a fresh gesture after recovery. Hold
  C/Ctrl crouches; double C/Ctrl latches prone, next press/release rises.
  Keep physical posture/eye transitions timed (0.4s crouch, about 1.1s prone),
  validate final standing clearance and keep the owner camera/server eye aligned.
  Prone world pose is procedural on team rigs; its extra query hit volume must
  stop accepting hits at death. Movement still uses the standard vertical capsule.
  Verify with test-movement.py (including InputKey binding tests) and accuracy;
  ERPostureGallery is opt-in art capture, never enabled in ordinary play.
- Presentation: hide the FPS left arm with the rifle during high climbing.
  Render FPS arms, rifle and muzzle flash as FirstPerson primitives, using
  camera depth scale 0.2 and 1cm near plane. Keep physical socket transforms,
  world meshes and authoritative shots unchanged; preserve viewmodel framing.
  Keep walking bob cosmetic in CalcCamera, faded when idle/airborne/traversing;
  preserve the capsule and server eye position. IA_Look scalar is 0.18.
- Use UERMovement and Unreal saved/compressed moves for traversal and walking
  intent. Do not implement movement through unrestricted client position RPCs.
- L_MovementLab contains twenty 200x200cm blocks, 10..200cm in 10cm increments.
  Keep this test map separate from the depot and preserve edited geometry.
- User-approved Fab low-poly arsenal: seven selectable primaries, a fixed
  M1911 sidearm and HE/smoke/stun utility slots. Server validates slot changes
  and loadout indices; primary changes apply at respawn, never refill live ammo.
  Keep ammo, cooldown and bloom per weapon; cancel firing/reload when holstering.
  Mouse-wheel bursts select one immediate slot and require 250ms without wheel
  events before another gesture; numeric slot keys remain immediate.
  No pickups, corpse loot, melee or matchmaking.
- Store Fab source archives/FBX/palette in SourceAssets/LowPolyWeapons and
  imported assets in Content/EclipseRecoil/Weapons/LowPoly. Never load Downloads
  at runtime. Preserve supplied meshes/skeletons; grip transforms and animations
  belong in project-owned weapon data/poses. Use actual model names.
- Grenade/rocket damage is server-only with WorldStatic cover checks. Smoke
  is a cosmetic volume; stun is owner feedback. Do not claim smoke affects bot
  vision, bespoke weapon audio or authored reload animations without implementing.
- HE detonations multicast a local BP_ERGrenadeExplosion with baked flying
  fragments from SourceAssets/BombCrackGround, excluding the source floor.
  Keep this cosmetic actor separate from projectile cleanup, collision-free,
  non-replicated, capped at eight bursts and removed after animation playback.
  Art defaults live in Effects/Grenade; preserve the supplied FBX/archive.
- Smoke grenades spawn a server-owned replicated ERSmokeCloud, with a local
  Niagara Fluids simulation per peer. Keep position/lifetime authoritative,
  collision disabled, a 40-cell maximum axis, four clouds/12 seconds and round
  cleanup. Each simulation owns its volume render target. Bullet carve positions
  derive only from authorized shot feedback; never affect damage or server aim.
  The low-shadow profile disables volumetric fog, so enable its rendering pass
  only while clouds exist and use transient fog support without editing maps.
  Imported source packages stay in InteractiveSmoke; owned defaults in
  Effects/Smoke. Smoke still does not interrupt bot sight checks.
- Team world characters (13 October): Alpha uses owned Twinblast ActionHero;
  Omega uses owned Drongo GDC. Keep meshes/materials/retargeted locomotion in
  Characters/Teams and art references in FERTeamAppearance Blueprint defaults.
  Apply the replicated PlayerState team to humans, bots and matching corpses.
  Hide Paragon weapons and Drongo mask hoses/bone decorations at every LOD.
  Use matte olive/brown clothing with no hero emissive effects; preserve skin.
  FPS renders only the chosen skin's extracted forearms/hands on the fitted
  fitted FPS bind rig, keeping its existing component frame, weapon poses and grips.
  Do not claim the original cybernetic geometry was replaced with human limbs.
  Use owned PA_ER_{Alpha,Omega}_Ragdoll (16 bodies/15 joints); preserve saved
  DefaultProfile locks and limits. Corpse meshes retain LOD0 and evaluate the
  frozen animation until settling. Collapse hidden bones after physics blending
  in ERCorpseMeshComponent::FinalizeBoneTransform, before publishing the pose.
- Edit or move `.uasset` and `.umap` through Unreal, never by binary patching.
- Selectable skins (14 October): five appearances per team in AlphaSkins and
  OmegaSkins BP arrays. Cold blue/navy Alpha and warm rust/orange Omega are
  dominant, with neutral trousers/straps/boots/gear and natural skin/hair.
  Use distinct source clothing plus owned helmet/pack geometry; preserve the
  Paragon geometry limitation. Selection persists locally and is server validated;
  ActiveSkin changes at respawn, and each corpse retains its initial skin index.
  Gear follows head/spine bones, has no collision and shares body visibility.
  Use setup_team_skins.py after the base rig setup and verify_team_skins.py;
  render all ten silhouettes and corpse poses before claiming visual validation.
- Keep art references in Blueprint defaults. Reuse template assets first.
- In Unreal Python, always construct rotations with named `pitch`, `yaw`, `roll`
  arguments. Positional `Rotator` arguments use the Blueprint factory order.
- The template rifle's root GripPoint attaches to Manny's `HandGrip_R` socket;
  do not independently rotate the FPS rifle under the camera.
- FPS uses the project-owned `A_ER_Rifle_HipPose`, generated by
  `scripts/create_rifle_pose.py`. Preserve template animations and keep the
  left grip aligned with the rifle foregrip when replacing this static pose.
- FPS keeps the fitted independent rig after the user supplied the mannequin reference.
  Match the weapon framing, keep the left hand below the foregrip, exclude
  upper-arm material sections and show slightly less forearm than the mannequin.
  `restore_mannequin_first_person.py` remains available for comparison.
- FPS uses `Characters/FirstPerson/SKEL_ER_FirstPerson`, two base rifle poses
  and twenty-two weapon poses in FirstPerson/Weapons. The one-time
  detach_mercenary.py migration preserves fitted bone data and poses, removes
  vendor compatibility/preview links, and replaces every body/material fallback.
  Skin FPS meshes in Teams/{Alpha,Omega}/FirstPerson keep only lowerarm/hand
  descendant geometry and preserve skin UVs/material slots. The Alpha Officer
  forearms are the geometry/reference provider; no mercenary mesh or texture is
  needed. Use setup_skin_first_person_forearms.py through
  Unreal; keep (-11.2263,1.4908,-159.9167), yaw -90 and component scale 1.
  FirstPersonMesh selects the skin geometry while all existing weapon idle/fire
  poses and socket grips remain unchanged. Do not use the world animation
  blueprint or move/scale the complete hero body to frame FPS arms. Verify both
  grips and capture all ten variants with -ERFPSForearmsGallery.
- Firing uses the project-owned `A_ER_Rifle_Fire` (100ms), generated from the
  hip pose with retargeting disabled during copying. Preserve both grips and
  exact idle endpoints. `MulticastShot` triggers cosmetic recoil, muzzle flash
  and template audio only for server-authorized shots. Camera recoil must not
  change ControlRotation or server aim. Art references stay in BP defaults.
- `scripts/setup_shot_feedback.py` rebuilds only its generated firing assets;
  verify material expression connections and real Metal compilation. The
  opt-in `-ERShotAudit` and `test-network.py --shot-feedback` audit local and
  remote shot events; nullrhi/nosound do not verify visual or audio quality.
- Bullet weapons resolve server-authorized hitscan traces on the Projectile
  collision channel (Visibility ignores standing capsules). Keep server spread,
  muzzle obstruction checks, per-pellet damage and cosmetic feedback separate.
  Rockets and thrown utility remain physical. Niagara tracers start at the
  rendered local muzzle or world muzzle and end at the actual server hit/miss;
  default cadence is one in three authorized shots, configurable per weapon.
  Use AERAttachedTracer after camera/mesh updates (TG_PostUpdateWork); Niagara
  runs solo after that update so parameters use the same frame. Start 0.75cm beyond
  the current rendered tip, preserve the server endpoint, expire after about
  20ms with at least one visible frame. Never freeze the origin while moving.
  Actual tip centers live in weapon data, derived from the front 4mm of owned
  +X gun geometry by fix_weapon_muzzles.py; preserve source meshes/grip poses.
  Skip these render-only actors on dedicated/headless processes.
  Tracer opacity fades across its width only; a length-relative fade hides
  the muzzle end on long traces and must not be reintroduced.
- Surface impacts multicast Niagara plus one of four alpha-atlas decal variants;
  never draw hit marks on players. `scripts/setup_hitscan_effects.py` owns Shots
  assets and uses the supplied PNG in private SourceAssets/BulletImpacts.
  Preserve its source/alpha; cap decals at 128 with a 20-second lifetime.
  `setup_bullet_impact.py` remains a legacy procedural fallback only.
  Impacts are now a faint 1.6cm glint for 35ms, without a directional spark fan.
  Keep muzzle flash small/soft (18ms) and tracers clearly visible in gold. The same setup script
  assigns soft muzzle material after setup_shot_feedback.py when both are run.
  Bullet shot feedback ejects a small matte black AERSpentCasing every second
  shot: local/non-replicated, WorldStatic contacts only, three-second lifetime,
  maximum 32 per peer. Never block players, visibility or authoritative traces.
  Art references stay in BP_ERCharacter / Effects/Shots/BP_ER_SpentCasing.
  Validate with network/accuracy/arsenal tests and -ERHitscanGallery on Metal;
  the gallery checks actual casing gravity, floor contact and cleanup.
- `scripts/setup_editor.py` creates assets only when absent; preserve edited maps.
- Record real compiler, Editor and network test outcomes in `docs/TEST_RESULTS.md`.
- Update development decisions and the asset inventory whenever scope changes.
- Do not claim tests, Fab compatibility or license verification without evidence.
- README and AGENTS files are English; development notes may be Italian.

Compile the Editor module as needed for local execution; do not add packaging or
release automation unless the user requests it. Run meaningful network tests for
gameplay changes. Never publish the repository or acquired assets implicitly.

Stationary Target training opens ten targets (five skins per team) from
the menu. These exhibition targets do not consume the four human LAN slots;
keep patrol bots limited by the combined four-slot cap. Preserve each
target's team, skin and assigned clear start across respawns. Verify all ten
replicated meshes with scripts/test-target-skins.py and bot regressions.

Repository maintenance: clean-project.py defaults to a dry run and protects
Content/SourceAssets, Saved settings/autosaves and Git/LFS. Never equate an
Asset Registry candidate with proof that it is disposable. Keep the original
mesh inputs required by skin-generation scripts and the independent FPS rig/poses.
Use Unreal for asset deletion/migration; preserve package paths. See
docs/REPOSITORY_CLEANUP.md and docs/ASSET_LICENSES.md before separating assets
or preparing a public repository. Do not commit/publish the current index
indiscriminately; Fab raw assets and derivatives remain outside code exports.

Mercenary removal (14 October): the vendor Mercenary_Male_WaveforceStudio tree
and old EclipseRecoil/Characters/Mercenary tree are obsolete. Keep fitted rig
and poses in FirstPerson; preserve their provenance rather than assuming that
removing a vendor dependency changes licensing. Historical mercenary setup
scripts live in scripts/legacy and are not rebuild roots. Verify with
verify_no_mercenary.py and scripts/test-without-mercenary.py; use only read-only
verifiers or game execution in its hardlinked fixture, never setup/import/save.

Cleanup executed (14 October): 272 obsolete Mercenary and 3399 unreachable
Paragon packages removed through Unreal after independent verified backups.
Preserve the 892-package current authoring/runtime closure and edited maps.
DeleteUnloadedParagonPackages is an Editor-only, namespace-scoped package
removal helper: require an archive receipt, no loaded packages and no outside
referencers. Never treat runtime reachability alone as deletion approval.
Local LFS storage is in the sibling `_cache/lfs-characters`, `_cache/lfs-weapons`
and `_cache/lfs-maps` directories; no objects pruned.
Backups/verification fixture moved to the sibling _backups directory.


Procedural destruction (14 October): use docs/PROCEDURAL_DESTRUCTION.md for the
first increment; the full imported specification is a later-milestone reference.
Keep L_DestructionLab separate from existing edited maps. Block state is logical,
keyed by wall/column/row, with ISM groups per chunk/material and bidirectional
instance mappings. Rebuild only the affected group on removal. Until coherent
block replication is implemented, require authority and NM_Standalone for
generation/mutations; do not allow LAN peers to acquire mismatched collision.
Run test-destruction.py and its --game probe plus existing network regressions.
Use --render for real visual evidence. Save lab binaries through Unreal in the
private maps mount, never in the public code tree.

Block combat Milestone 2: trace continuation uses the actual oriented-volume
exit and residual gameplay energy; never ignore an entire room/ISM to get past
one block. Keep range, grazing and crossing limits. Explosions snapshot exposure
across all rooms before any mutations, then aggregate damage/rebuild each dirty
group once. Use exposed-face sampling, not block centres inside coplanar walls.
Preserve ordinary-map/network firing and obstructed-muzzle behaviour.
Run test-destruction.py --combat and --combat --render for actual shots, rear
targets, breach traversal and a physical HE fuse, plus network/accuracy/arsenal
regressions. setup_destruction_lab.py migrates only new catalog fields once via
ERBlockCombatVersion=2; preserve later tuning/HP/art and existing map geometry.


## Procedural floor and texture library (Milestone 3)

- `AERProceduralMap` inherits `AERBlockRoom`; keep one identity/damage/ISM path.
  `ERProcedural::Build` is deterministic and validates dimensions before replacing
  layout. `UERBlockWorld::LoadLayout` atomically validates records before commit.
- Room returns own corners, branch sides stop at outer wall faces, apertures clip
  exact jambs/courses. Never create a second wall at corridor/room doors. Keep
  safe player starts based on the same seed/config before pawn spawning.
- Texture originals and ZIP live privately in maps SourceAssets/ProceduralTextures.
  Verify manifest hashes before import; do not execute attached source documents.
  Import via Unreal only. Use DX normal maps, linear masks and sRGB base color.
  Do not mistake packed roughness-AO/ARM for separate roughness or AO maps.
- World-position tiling must remain continuous across block instances. Materials
  use geometric vertex normal and world-space normal transformation. Runtime
  displacement is excluded so visual surfaces cannot change collision geometry.
- Preserve artist edits on reruns; the versioned PBR catalog migration touches
  only Surface. Keep wood/stone/metal/glass library separate from the current
  three combat profiles until their gameplay rules are explicitly implemented.
- Verify `test-destruction.py` (now eight automation tests, including M4 stress/M5 support), `--procedural`,
  `--procedural --render`, and the existing combat/LAN regression probes.


## Block profiling and stability (Milestone 4)

- Group-local mutations now remove only destroyed instances using persistent
  FPrimitiveInstanceId handles and recover current indices through Unreal. This
  supersedes full-group clear/reinsert on every removal. Never assume swap/order
  semantics or expose native handles as gameplay/network IDs. Use the ID API only
  on the generated runtime ISMs; Editor/HISM manipulations invalidate its contract.
- Generation/recovery bulk-inserts, preserves logical ordering and maintains the
  component/group and logical/index/handle mappings. No duplicate live records in
  procedural layout metadata. Full-map teardown releases every wall body.
- Keep standalone/authority guards, explosion cover snapshot and shot limits.
  CPU timers include synchronous ISM/physics; do not claim isolated Chaos, GPU,
  FPS or total memory results from those measurements. Explain exclusive ISM
  resource bytes, owned allocations and whole-process RSS separately.
- Block effects are bounded and owned; Regenerate and EndPlay clear decals,
  Niagara and debug bursts. Preserve other combat effects and global budgets.
- Validate eight automation tests plus test-destruction-stress.py, --render --trace,
  real combat/procedural probes and LAN/accuracy/arsenal. Benchmark receipt is
  docs/DESTRUCTION_BENCHMARK.json; README must contain methodology and limits.
  Defaults stay chunk400 and block50x20x25 until a broader workload justifies tuning.


## Controlled collapse (Milestone 5)

- Use IsAttached for every live block query/hit/mapping path. Detached records
  retain identity/HP and a separate counter; never count them as bullet damage.
- Contacts require one touching box axis and positive face overlap on two axes.
  The local ground plane, explicit anchors and indestructible blocks seed support;
  ordinary ceilings do not support top courses. Current support is connectivity,
  not load-bearing simulation. Graph generation rejects unsupported/non-axis
  geometry and bounds storage/candidate work; log the fallback explicitly.
- Damage schedules bounded timer work, never synchronous collapse inside the
  global explosion exposure snapshot. Finish/coalesce removal-only jobs before
  rechecking damage revisions. Never restart on every shot or assume chunk-local
  support without considering connected neighbours.
- Detach uses the same persistent native-ID removal path. Clear batch static
  collision before spawning bounded, temporary Chaos fragments. Default global
  debris32 / hard64, per callback4 / hard8. No player/projectile/query or debris
  collision; no cover/damage gameplay. Settle/lifetime and regeneration/EndPlay
  cleanup are mandatory. Keep authority/NM_Standalone guards until M6.
- Run eight automation tests and --collapse / --collapse --render, including
  actual gravity, global budgets across rooms and ten-room full-shell collapse,
  plus stress and combat/procedural/LAN regressions. Graph time budgets do not
  guarantee full callback latency; report observed CPU steps and measurement
  limits. Keep the M4 benchmark receipt as its historical baseline.


## Replicated buildings (Milestone 6, 15 October)

- M6 supersedes the earlier standalone-only guards for the block laboratories.
  CanMutateBlocks requires authority and rejects NM_Client; client geometry
  generation is allowed only during a validated descriptor receive. Keep normal
  owned-pawn server fire and existing cover/penetration logic. Never add client
  block ID/damage mutation RPCs or unrestricted client-position RPCs.
- One active AERBlockRoom/AERProceduralMap owns the entire lab layout. Protocol6
  descriptor/epoch/layout fingerprint + absolute HP/state pages reconstruct on
  peers. Pages are independent tickless actors for128 stable records, max391;
  version manifests must match before any client transaction changes collision.
  Future pages wait, stale epochs are ignored, invalid data fails closed. Preserve
  native ISM handles and validate mappings after snapshot/epoch application.
- Initial loading pauses owner movement and server movement until an owned
  controller epoch acknowledgment. Native character correction handles server
  teleports after generation. Ongoing deltas retain the previous coherent state;
  streamed/multiple independent world actors are outside this gate's scope.
- Only authority computes support; Detached replicates as logical state. Local
  fragments are not replicated or replayed for late join and never provide query
  collision/cover. Preserve hard64 global cap, cleanup and dedicated-server culling.
- Stacked floors1..3 reuse deterministic per-floor seeds/unique wall IDs. Shared
  static upper slabs replace lower ceiling duplicates; upper base courses anchor
  to those slabs. Stairs rise<=20cm with separate return lanes. Require branch250+
  for multi-floor layouts, cap complete layouts/storage50k, reject atomically.
- Extended catalog adds Wood/Stone/Metal/Glass. Preserve all original and later
  artist/gameplay edits; setup_procedural_multiplayer.py is additive/fills only
  unset art, reuses the imported PBR library, saves through Unreal privately.
  Bootstrap/texture scripts must tolerate seven rows and preserve extra profiles.
- Run eleven automation tests, test-destruction-network.py with actual remotes,
  --loss and --render, explosion snapshots, real stairs and dense late join, then
  existing combat/procedural/collapse/stress/LAN/accuracy/arsenal regressions.
  Network buffers are a separate tracked allocation subtotal; actor/channel,
  shared/GPU heaps and bandwidth are outside that number. M4/M5 receipts remain
  historical. Report actual measures/limits and inspect final graphical capture.


Session setup (15 October): the current user request supersedes the earlier
menu target preset and combined patrol-slot cap for explicitly configured
ERAlphaBots/EROmegaBots sessions. Allow 0..10 per team independently of the
four human slots, selected Alpha/Omega, Target/Movement/Patrol behavior and
local/listen/client setup. Keep legacy ERBots probe compatibility. Start matches
with one human, including listen servers. All labs replenish server-owned reload
reserve while preserving reload timing/capacity; depot ammo remains finite.
The single play.command binds listen servers to 0.0.0.0 via MULTIHOME.


## Reference map templates (15 October)

- `FERProceduralConfig.TemplateAsset` selects an authored `UERMapTemplate`;
  empty preserves the existing room/corridor generator. Keep templates as data,
  with measurements/provenance and source assembly JSON in private maps assets.
- `L_Killhouse` / `Art/MapTemplates/DA_Killhouse` adapts Workshop526448928,
  `de_killhouse.bsp`. Read source with scripts/analyze_source_template.py and the
  existing read-only csgopen Source converter; never modify the original game.
  No Valve texture/model binaries are imported. Source cm mapping is
  (X,-Y,Z)*2.54 with reflected yaw, Z100 capsule. The later balance request
  supersedes preserving all original spawns: keep Alpha and half-turn Omega.
- Template pieces are bounded axis-aligned assembly volumes with deterministic
  group offsets on a50cm grid. Only one arena floor, max256 authored pieces and
  max50k logical blocks. Reject invalid geometry/overlap/spawns atomically.
- Destructible pieces always use the project block mesh/material catalog and
  the same damage/support/network pipeline. FixedMesh is optional for static
  industrial replacements; validate fitted collision and keep its dependency
  closure private. Factory art is now imported into Art/FactoryEnvironment;
  keep its original materials off the destructible catalog. Fixed visual ISMs
  are query-free with hidden authored box collision; preserve this during
  snapshot gating. This is an approximation, not exact vendor triangle collision.
- Fingerprint template identity/revision, fixed geometry/mesh collision GUID,
  arena/ceiling and spawn transforms in addition to logical blocks. Preserve
  protocol6 snapshot/epoch gating and server-only mutations.
- setup_killhouse_template.py creates only absent data assets/maps through Unreal;
  never overwrite artist edits on rerun. Template JSON is initial construction
  input, not a runtime external-file dependency.
- Run twelve destruction automation tests including MapTemplate and
  test-map-template.py --render --network; inspect actual captures. The probe
  prepares a low-HP panel band for real spread, places blast origins on an
  exposed face and leaves enough time for initial/late client loading. Keep
  existing LAN/accuracy/arsenal regressions and record executed outcomes.


Balanced Killhouse revision2 (15 October): future playable reference templates
must be symmetric/antisymmetric between teams. Default HalfTurn emits paired
pieces and transforms their shared random offsets; rotate the split block cells
too, preserving partial edge cells. Omega spawns derive from Alpha. Unmirrored
central pieces require XY0/jitter0. Keep rear ground entry, front firing windows,
internal stairwell and accessible parapeted roof on both bases, plus denser
paired cover. Native tests verify geometry pairing, rear/window/stair/roof
clearance; actual pawn probes traverse both bases and central stairs. Keep
TemplateCollision hidden and never enable query collision on ERTemplateVisual.
Use the native ValidateLayout before saving an upgrade; preserve completed
migration markers and all material/combat edits. Details in MAP_TEMPLATES.md.


Killhouse revision3 (15 October): Variation is an opt-in data-asset recipe with
arena/base ranges, seven module kinds and a private Factory palette. Sample a
complete half, then rotate geometry/spawns; never sample the teams independently.
Keep disjoint module cells, outer circulation and two standing doors per room.
Validate full route segments and all volumes before commit. Runtime Layout owns
the sampled ArenaSize; use it for floor/shed, fingerprint and patrol relocation.
WalkableSurface roofs/decks/treads must use solid project boxes with matching
visible/collision geometry; never fit detailed/open Factory art into these slabs.
Base metadata/stair routes drive physical tests/probes instead of fixed XY values.
setup_killhouse_variations.py is the explicit revision3 migration, preserves map
and catalog hashes, validates516 seeds and protects later artist edits by marker.
Run512 composition seeds plus six physical seed cases in test-destruction.py,
actual --gallery captures and --render --network template probes. Menu Killhouse
sessions choose a new seed; explicit travel/command seeds remain reproducible.


Narrow ledges (15 October): FindLedge must sample inside the contacted face,
then follow a bounded continuous top interval. A fixed far-only probe misses
20cm parapets; place narrow landings inside their support strip, retain broad
landing distance and reject gaps/standing obstructions. Preserve30/80/150cm
thresholds, normal Space binding and saved-move/server validation. MapTemplate
covers48 roof-parapet queries across six seeds; test-parapets.py drives16 actual
host/autonomous-client Space+forward cases. Run movement and accuracy regressions.


Movement-only bot demonstrations (15 October): keep server-only AI decisions in
MovementDemo, using normal Jump/Crouch/UERMovement intents and real collision.
Stagger22-second posture cycles with explicit roster offsets assigned by the
server (bot PlayerIds are not unique).
Bound local obstacle searches to12 directions/7m every2s; require clear supported
approaches, exclude pawns and retain normal30/80/150cm limits. Do not interrupt
traversal with posture changes; after bounded rise waiting allow slow movement
out of obstructed headroom. Reset demo intents/timers on new life. Preserve
stationary/combat modes and no-firing movement mode. test-bot-motion.py verifies
each team's actual jump/traversal/crouch/prone/crawl/rise and model replication
from a remote observer; use test-bots.py and test-session.py regressions too.


Dust Mirror (15 October): use Source2 Workshop3117367319 as reference, not the
empty-room DustII placeholder125438255. L_DustMirror/DA_DustMirror preserves
an orthogonal320cm catwalk over a290cm-clear lower route, two20cm-rise stairs
with open parapet mouths, and four110cm timber door gaps. Paired cover offsets
vary on a50cm grid; topology stays fixed. Keep10 starts/team and HalfTurn.
Urban fixed art is private Art/ArabianEnvironment (27 migrated packages plus
four owned PBR parents); repair missing Substance parents with
setup_arabian_materials.py using supplied baked textures only in private copies.
Never change the source vendor project or shared destruction catalog. Walk
surfaces remain solid matching boxes. Setup revisions preserve later artist
edits. Run13 destruction tests (including DustTemplate512 seeds/six physical
cases), test-dust-template.py --render --network, inspect actual captures and
retain existing LAN/accuracy/arsenal regressions. See MAP_TEMPLATES.md.


Dust town revision3 (15 October) supersedes the earlier symmetric arena-only
Dust recipe at the user's explicit reference-plan request. Preserve the
asymmetric macro graph (south T court, north CT court, B west/A east, Mid,
B tunnels, Long and elevated Short A) while seeds warp shared plan axes, sample
street widths/parcel heights and reshape entire building footprints. Keep the
Short/CT grade crossing, two stair approaches and narrow inward-open timber
gates under arched heads. Killhouse retains HalfTurn; do not alter it.
Town metadata drives physics/probes instead of old fixed coordinates. Door
leaf collision rotates with the wood; planar SAT permits adjacent tilted
planks without accepting overlapping solids. Visual-only facade/arch art has
no query collision. All walk surfaces keep solid matching collision.
Use setup_dust_town.py as the explicit versioned migration; preserve completed
artist edits and the shared destruction catalog. See the updated town section
in MAP_TEMPLATES.md and executed checks in TEST_RESULTS.md.


Dust town revision4 (15 October): the user's latest request supersedes the
inward-open pairs. Reduce plan dimensions to66..75x73..84m (about13% per axis)
without shrinking player clearances or the320cm/290cm crossing. One gate leaf
opens inward and the other outward; parallel leaves overlap24cm in frontal
projection. Preserve110..130cm diagonal slots and six-point gate passages in
both street/patrol metadata. Use local rotated-volume route validation, never
world AABBs that falsely seal this slot. Gate patrol points need10cm acceptance
to avoid cutting through a leaf; other map patrol defaults stay90cm.
Apply setup_dust_town_compact.py once after candidate validation, preserving
town art, Killhouse and the shared catalog. Test frontal line-of-sight blocking
in both directions, physical dogleg passage,512 plans/six physical seeds and
actual pawn traversal with initial/late LAN snapshot agreement.


Storage audit15 October: the user explicitly reserves the entire
Content/ParagonLtBelica and Content/ParagonPhase trees and their original inputs
for future character imports. Exclude them from any unused-asset pruning even
if not runtime-reachable yet. The audit was followed by explicit user-approved
removal of completed Factory/Arabian scratch projects, Saved/Source2DustMirror
and the duplicate texture ZIP. Keep all88 extracted texture inputs and their
manifest. Intermediate/Binaries and active packages were preserved.
See REPOSITORY_CLEANUP.md and Saved/AssetAudit/storage-cleanup-2026-10-15.json.


Character finalization (15 October, latest user request): the user approved the
twenty current skins and explicitly authorized removing downloaded originals and
other character assets unnecessary for their current in-game use. This supersedes
the earlier complete Phase/Belica reservation and rebuild-source retention for
this cleanup. Display names use `character_catalog.py`: hero name + middle dot +
role, with stable indices and unchanged geometry/palette. Preserve the independent
FPS rig/poses, all live skin/corpse/weapon bindings and source textures/materials
still referenced by them. Vendor folder names alone do not imply unused assets.
The approved cleanup is scoped to the characters submodule; never prune maps,
weapons, Git/LFS or other downloads implicitly. A verified private rollback archive
and manifests are kept outside the project in the sibling `_backups` directory.
Historical rebuild scripts require restoring archived inputs before generation;
ordinary play and saved-asset verification require no originals restoration.

## Requested Sketchfab arsenal (15 October)

The latest weapon request supersedes the seven-primary/fixed-sidearm/no-melee
baseline. Use docs/WEAPON_IMPORT.md and docs/WEAPONS_REFERENCE_EN.md, preserving
the original attachment and worksheet snapshot in docs/references. Ten independent
server-owned inventory slots use1..8/F/G: knife, pistol, SMG, shotgun, assault,
carbine, sniper, Milkor, HE, smoke. Initial weapon is the assault rifle. The user
specified AK-47 and SCAR statistics; the latter maps to scar20 in the sheet.
Keep all nine archives, source metadata/provenance and derived FBX privately in
weapons SourceAssets/SketchfabWeapons, runtime assets in Weapons/Sketchfab, and
Blueprint references in the existing private maps mount. No Downloads runtime.

Preserve server aim, spread, cover, per-weapon ammo/cooldown and cancellation.
RangeModifier applies per1270cm; armor consumes half the prevented health damage
using ArmorRatio/2, with correct depletion and protection guards. Starting AP100
belongs to the new catalog; direct environmental damage and the legacy fixture
retain their behavior. Melee is a bounded160cm first-hit server query. Milkor
uses LaunchedHE with the source GL_slug1 geometry, gravity, impact detonation and
fuse, never hitscan. Preserve enum values for the older payloads.

The fitted FPS skin rig/poses remain independent. Bake source rigid mechanics
and source root/wrist motion separately; retarget hands with bounded IK rather
than replacing skin geometry. Keep original files, author credits and per-asset
licenses (Milkor BY-NC); use Unreal to save/reimport generated packages. Source
normal conventions and smoothness inversion matter. Run the native Weapons
automation test, test-reference-weapons.py and --render, inspect all captures,
verify saved asset bindings with verify_sketchfab_weapons.py, then retain
network/accuracy/legacy-arsenal regressions. ERLegacyArsenal is the explicit server
URL option for the older regression fixture, not the default gameplay catalog.


Spawn protection appearance (16 October, gray revision): body, skin gear and
FPS arms use the existing engine LevelColorationLitMaterial through one transient gray
MID per character during the replicated protection deadline. No new material,
texture, skin copies or network fields. Cache originals by component AND slot,
restore before skin changes and on expiry/attack/death. Weapons retain materials.
setup_spawn_protection.py now saves BP defaults and removes the retired
Characters/Protection variants through Unreal after checking external references.
Never regenerate the translucent copies or edit original skin materials.
Verify LAN owner/proxy/respawn/attack and Metal protection galleries.

Head markers (16 October): the user removed all overhead player/bot indicators.
Keep TeamMarker hidden at runtime, including existing Blueprint defaults and
respawns; retain the component only for serialized asset compatibility.


Grenade revision (16 October): supersedes animated floor fragments, live smoke
fluids and Milkor impact detonation above. HE/LaunchedHE use compact Epic
Content Examples subUV burst defaults in Effects/Grenade, no active BombCrack
animation. Keep imported Epic originals/receipts private and the downloaded
ContentExamples project unchanged. Generate owned defaults with
setup_grenade_effects.py; older explosion/smoke setup scripts are historical.
Smoke is a 6x6x2.8m hemisphere, 16x16x8 conservative connected free-cell grid
with neighbour sweeps and a 48-step local density material. Each cloud owns
its density atlas; block revision/epoch refresh is bounded to every750ms.
Preserve four-cloud/12s and eight-burst budgets, server lifetime, round cleanup,
authorized short shot cuts, camera-inside rendering and opaque-depth clipping.
Never change global fog settings. Narrower-than-cell apertures may stay closed;
do not claim exact fluid transport or bot vision occlusion. Utility and Milkor
projectiles bounce/tumble with server-sampled bounded contact variation; only
the fuse detonates them. Rockets retain their impact rule. Run test-grenades.py
and --render, inspect actual captures, plus arsenal/reference/network/accuracy
and physical block-combat regressions. Preserve all unrelated in-progress work.


Grenade damage/cook revision (16 October, latest request): HE and LaunchedHE
share 140 peak through 5m with a quadratic tail to zero at 15m. Three server
WorldStatic head/torso/legs exposure samples see intact cover before any
structural mutation. Keep the separate 3.5m building-damage envelope. Explosive
ArmorRatio clamps1.6..2 (at most 20% reduction); default full armor still takes
112HP inside 5m. Bullet armor remains unchanged. Update existing owned data
through setup_grenade_combat.py, preserve art and all normal input mappings.
Hand HE/smoke/stun consume ammo/arm on press, throw the same replicated actor
on release, never restart its server fuse. Keep held/recovery replication,
immediate local holding-arm/weapon hide, overhead origin swept from the
server eye, upward velocity and normal projectile collisions. Never trust
client fuse duration, elapsed cook time or launch transform. Hold-too-long
detonates in hand; death/interrupt drops live ordnance; no holster/reload
cancellation/refund. Milkor remains press-to-fire. Run native Weapons tests,
test-grenades.py --combat --render with actual host/client inputs and inspect
captures, then baseline grenade/arsenal/reference/network/accuracy and physical
block-combat regressions.


Latest grenade tuning (16 October): supersedes the 5/15m and 2.5s values
above. HE/LaunchedHE retain 140 peak and the same quadratic/exposure/armor
model, now full through 2m and zero at 6m. All hand grenades and Milkor use
3.5s fuses; cook still starts on press. Preserve the separate 3.5m structural
envelope, cover checks, art and input behavior. Migrate saved weapon data
through setup_grenade_combat.py; verify updated radii/fuses in the combat
probe and the native Weapons tests.

Smoke shape refinement (16 October): 3m maximum radius and 3m height within
6x6x3m bounds. Smooth asymmetric shader lobes and drifting noise vary per
server-sampled replicated ShapePhase. Keep the conservative connected-cell
containment and opaque clipping; never distort density beyond the 3m envelope.
Use setup_smoke_shape.py for smoke-only asset regeneration, preserving HE.

Explosion/throw refinement (16 October): Milkor launch speed1800cm/s (half
3600), hand HE/smoke/stun880cm/s (80% of1100). Preserve 3.5s cook and2/6m
damage. Disable colorful Fire emitters in the owned compact burst; use a65ms
neutral flash and three small short smoke puffs. Add3..6 local cosmetic floor
chips only with a walkable WorldStatic surface within60cm beneath the blast;
never spawn chips for an airburst. Match surface material, bounded ballistic
motion with static traces, shrink/cleanup with the burst; no gameplay collision.
Use setup_explosion_refinement.py and setup_grenade_combat.py for saved assets.

Smoke/test-stock refinement (16 October): supersedes the3m/1800cm/s values.
Milkor1530cm/s (another15% reduction), hand speed880 unchanged. Hand utility
profiles start with10 each for testing; keep Milkor's six-round drum/reserve.
Smoke bounds8x8x4m, maximum radius4m, growth2.2s and fade4s within the same12s
lifetime. Animate replicated-phase contour lobes using synchronized smoke age;
keep16x16x8 containment (50cm cells), walls/windows, four-cloud budget.
Authorized shots retain12cm/.3s cosmetic cuts; locally replicated moving
ordnance adds25cm/.6s wakes at most10Hz; detonation multicast adds150cm/1s
clearing. Keep four cuts/cloud and automatic closure; no aim/damage/drag changes,
no client-authorized gameplay or fluid claims. setup_smoke_shape.py and
setup_grenade_combat.py update owned material/Blueprint/data only. Verify
actual projectile wake/blast interaction, test stock and new radii/speed.

Shoulder/cook presentation (16 October): Milkor1750cm/s, near earlier1800.
Hand throw origin is yaw-relative(-14,24,-4)cm from eye, swept against cover,
not25cm above head. Preserve local FPS hand/model hide but show WorldRifle
while cooking, including the last grenade. UERPostureMesh applies a blended
right-arm two-bone ready pose toward the shoulder origin from replicated cook
state; keep grip socket, skin/locomotion, damage and fuse rules. Verify actual
remote-player pose/visible grenade with combat --render and capture
Cook03_RemoteReady.png. HE/Milkor blast clears smoke300cm radius for2.5s;
protect blast pockets from displacement by small wakes, four cuts/cloud max.

Team smoke (16 October): smoke grenades capture the thrower's server team
when armed, using HUD colors Alpha(.2,.65,1) and Omega(1,.25,.1). Preserve the
snapshot through death/respawn/team changes; neutral fallback without a team.
Assign SmokeColor before deferred cloud BeginPlay and replicate with material
RepNotify, so all peers see the same tint even on late property delivery.
Do not alter HE burst smoke or density/lifetime/containment/interactions.
Verify both teams and remote colors with the actual smoke-fuse baseline/render.

Darker/billowing smoke (16 October): team smoke now uses40% of the previous
linear RGB tint: Alpha(.08,.26,.4), Omega(.4,.1,.04). Keep replicated team
snapshot semantics. Material growth has smooth per-direction onset delays
up to22% of the2.2s growth, all catching up at the same final time; slightly
stronger moving lobes, always inside4m. Preserve density/opacity, lifetime,
containment and gameplay. Regenerate only smoke with setup_smoke_shape.py;
verify both team colors remotely, shader/render and existing interactions.

Dense/dispersed smoke and shootable grenades (16 October): optical density
coefficient .035 (previous .026); bullet holes close over2s with square-root
radius recovery, moving-grenade wakes1.5s. HE/Milkor dispersal radius400cm
lasts until cloud expiry and loses only5% radius; retain four cuts and protect
blast pockets from small-cut eviction. Preserve team tints/containment/timing.
Armed fuse projectiles have a separate query-only ShotTarget on Projectile
channel (6cm thrown,12cm held). Do not alter movement/pawn collision. Only
server positive bullet point damage detonates; melee/radial damage cannot
trigger it. In-hand cook clears through existing OnGrenadeDetonated, without
refund, second throw or fuse restart. Ignore the held projectile during safe
origin sweeps. Verify actual weapon traces with cover and thrown/held targets,
smoke recovery after elapsed time, shader/render, Weapons and regressions.

Grenade chain (16 October): supersedes the earlier no-radial-trigger rule via
an explicit server blast-to-grenade path, not generic TakeDamage. Positive-damage
HE/LaunchedHE/Rocket blasts snapshot WorldStatic exposure to live fuse projectiles
before character/death callbacks and structural mutation. Use each source's blast
radius (HE6m), enqueue each target once next tick, guard already-detonated and
pending targets. Preserve covers at every hop and clear in-hand cook through
the normal detonation path; smoke/stun release their own payload without becoming
explosive propagation sources. Verify multi-hop, held and protected targets.

Grenade audio (16 October): owned BP_ERGrenadeExplosion binds Freesound162363
public HQ preview, imported mono PCM44.1kHz,0.601s, FORCE_INLINE. Source and
credit in private Assets/weapons/SourceAssets/Freesound/162363; no original
FLAC or library imported. Local detonation effect plays once on each peer,
volume.65, spatial attenuation inner5m/falloff75m, concurrency8 stop farthest
then oldest. Dedicated servers skip cosmetics. Regenerate via
setup_grenade_audio.py and verify test-grenades.py --render --audio.

XM1014 detail (16 October): active Shotgun data keeps6 pellets x20, armor
ratio1.6, range modifier.70 per1270cm. bUseBodyHitZones opts only this weapon
into anatomical hits (existing physics bodies) and head4/stomach1.25/
chest-arms1/legs.75. Legs bypass armor; head uses replicated Health.bHelmet
(defaulttrue, separate from cosmetic helmets). Other weapon balance unchanged.
Server meshes are query-only/ignore scene channels and always refresh poses;
ERShotTrace queries world cover first and explicit anatomical bodies second,
including block continuation. Reuse physics assets; generate no new hitbox assets.
Contact capsule obstruction starts pellets at the eye for anatomical resolution.
Rosette6 slots,3.2deg cone with small seeded disk jitter, shared .25deg base aim
plus existing posture/motion/bloom. One shell and presentation event per shot,
full-auto preserved. setup_shotgun.py updates only active data. Verify Weapons
automation, reference weapon LAN actual pellet/zone/armor/cover checks and
network/accuracy/legacy arsenal/block combat regressions.

English presentation/generic weapon names (16 October): all owned player-facing
menu/HUD/help/errors/results/default names and20 skin labels use English. Set
current game language to en in GameInstance Init without persisting OS/editor
preferences. Generic active slots:1 Knife,2 Pistol,3 SMG,4 Shotgun,5 Assault Rifle,
6 Sniper Rifle,7 Marksman Rifle,8 Grenade Launcher,F HE Grenade,G Smoke Grenade.
This is nomenclature only: keep slot order, saved IDs, asset paths and ballistics;
internal Carbine key is slot6 Sniper Rifle and Sniper key is slot7 Marksman Rifle.
Use presentation_labels.py and setup_english_presentation.py for saved metadata;
character_catalog.py owns English skin labels. Preserve model names in source
provenance/credits. Verify saved catalogs, native Menu validation/admission and
actual Metal menu layout. Do not regenerate maps or imported models for labels.

Rifle scopes (16 October): only Assault/Carbine/Sniper active data opt in via
ScopeMode (iron/optical); visible labels remain Assault/Sniper/Marksman. Right
click toggles Assault; optical cycles40/10/off with FOV/90 sensitivity. Server
owns level/settle/cone, no client directions. Assault retains ordinary spread;
optical stationary recovered aim reaches zero after.16/.25s, hip cones5/8deg
(19 October accuracy revision, Sniper/Marksman respectively).
Sniper retains zoom after shots, Marksman exits. Hide the normal centre reticle
on optical rifles, including hipfire; keep the ammo/spread ring and count in all
aiming modes (19 October revision). Assault/launcher retain the central crosshair
in iron sights. Move complete FPS rig for iron sights, never independently detach
gun/hands. Scope tuning/setup must preserve
source art and other weapon balance. Run test-scopes.py --render and retain
reference/accuracy/network/legacy-arsenal regressions; inspect actual captures.

Grenade/launcher range revision (16 October): hand utility horizontal launch
component x1.2; launcher iron-style ADS (right-click toggle75FOV) x1.5 only
while scoped. Preserve vertical launch component, gravity,3.5s cook/fuse and
player blast140HP/2..6m. Weapon data owns iron anchor/eye distance; retain the
Assault centre. Fuse speed cap10000 avoids vertical-cap artifacts. Server bounce
.18..25/friction.4. HE/LaunchedHE structural damage x3, radius350cm, intact-cover
snapshot/material resistance preserved; rocket/player/chain damage unchanged.
Use setup_grenade_combat.py/setup_scopes.py and verify scopes --render, grenade
baseline/combat, actual baseline-vs-boosted block blast, and regressions.

Scope motion/posture refinement (16 October): Assault/launcher iron aim raises
and lowers over.2s with smoothstep and matched FOV/sensitivity; cap integration
at50ms per frame so a hitch cannot skip the whole animation. Launcher anchor
2.5cm below centre with3deg upward rig pitch; preserve complete-rig grips.
Optical standing movement keeps focus blurred; server motion timestamps delay
recovery until actual stopping. Grounded crouch focus25ms; the 19 October
accuracy revision gives both optical rifles real crouch/crawl movement spread
through ScopedLowMovingSpreadDegrees (.8/1.2 at 10km/h, crawl x.65), while
stationary low-posture scopes remain precise. No airborne exemption. Sniper crouch reduces
burst/cosmetic recoil x.2 and accelerates recovery x4; standing auto bloom now
.5deg/shot, delay.3s, recovery6deg/s. Blur reads focus plus real bloom.
These requested rules are not an assertion of CS parity. Verify intermediate
enter/exit captures, grounded/moving/crouch server cones and accuracy regressions.
Use setup_optical_accuracy.py for the narrow two-asset hip/low-motion migration;
scope_profiles.py also preserves these values during future scope/import setup.

Launcher/player contact refinement (16 October): launch LaunchedHE six degrees above
authorized aim (clamp pitch +/-89); scope still multiplies horizontal velocity x1.5.
Fuse grenades hitting pawns retain 25% tangent/5% normal incoming speed. Only
LaunchedHE deals 25% base damage on contact, once per target per projectile, through
normal armor/protection rules. Preserve 3.5s fuse and subsequent blast.

Scoped launcher origin fix (16 October): while iron blend is active, derive
launcher muzzle from raised FPS rig in camera-local coordinates, then transform
through authoritative eye/aim and matching FOV projection. Ignore first-person
depth scale for a full-sized projectile; blend from ordinary hip origin. Retain
eye-to-muzzle obstruction sweep/fallback and server-only launch direction.

Grenade audio (17 October): user-supplied Sonniss grenade1.wav replaces the
HE sample (full2.361s); smoke_1.wav binds BP_ERSmokeCloud ReleaseSound.
Private SourceAssets/SonnissGDC2024 retains originals and mono PCM44.1kHz
runtime copies. Smoke is trimmed6s with baked linear fade4.5..6s, no loop;
replicated cloud plays once per peer with late-arrival offset and stops on
cleanup. setup_grenade_audio.py imports both; verify grenades --render --audio.

HE art (17 October): user selected F1 instead of RGD-5 from the supplied
Grenades.fbx. Bake HE object F1 and bind owned HE/F1/SK_HE with M_HE_F1
textures. setup_f1_grenade.py updates only HE art and removes the separate
Flash sphere mesh from BP_ERGrenadeExplosion. Preserve PointLight flash,
smoke/chips, Sonniss audio, fitted hands, fuse/damage/throw settings.


## Block damage preset (17 October)

- The user's RR_DANNI_MATERIALI_BLOCCHI_GRANATE.md is the source; details in
  docs/PROCEDURAL_DESTRUCTION.md. HP/multipliers are a gameplay preset per
  cell, calibrated for 5cm cells. The user chose to keep the 50x20x25 blocks for
  now; 5cm cells are a separate voxel-chunk milestone. Do not silently reuse
  these HP after changing resolution.
- Keep character damage, structural damage and penetration energy separate.
  Bullets: BlockShot.StructuralDamage (defaults to weapon Damage) x residual
  energy x BulletDamageScale. No hit zones on blocks. HE: Data->BlockExplosion
  (400/0/25/200/^2, cell-centre distance) x ExplosionDamageScale x exposure.
  Rockets keep FERBlockExplosionProfile::Linear.
- Exposure is computed on the pre-blast snapshot. Intact shielding cells multiply
  it by their untuned ExplosionTransmission; other geometry occludes. Apply one
  aggregated batch, with no chained excess. Only Glass breaks as a connected pane.
- setup_block_damage_preset.py (ERBlockDamagePresetVersion=1) touches only
  HP/multipliers/transmission of DA_ER_BlockMaterials. Run test-destruction.py
  (DamagePreset/DamageLocalAndShielding/GlassPane), --combat, network and grenade
  probes. Launched from an agent shell, a recent project in ~/Downloads can block
  the editor home screen on a macOS privacy prompt until the user answers it.


## 5cm cells everywhere (18 October)

- The user asked to remove the old blocks everywhere and to port collapse to
  cells. Every AERBlockRoom/AERProceduralMap (lab, procedural/multi-floor,
  Killhouse, Dust) is now 5cm cells only. ISM block groups, FERBlockRecord/
  FERBlockId, AERBlockNetPage (protocol 6), FERBlockSupportGraph and the block
  lab probe/tests are gone. UERBlockWorld stays only as an empty legacy
  subobject so saved maps load. The descriptor is protocol 7 only.
- Layouts emit FERCellPanelDesc: procedural walls via FERCellWorld::AddConstruction
  with door/window openings as Holes (absent cells, never "destroyed"), glass panes
  per window resting on their frame; template pieces via ERProcedural::AddPieceCells
  (massive slabs, boards, 2cm shells for volumetric thin-material crates). Pieces
  keep their exact validated volume: each axis holds round(L/5cm) equal cells
  (FERCellWorld::FitCell). Never truncate to whole 5cm cells: Source-derived sizes
  then leave see-through slits between stacked pieces (MapTemplate asserts none).
  Wall slot thickness is Config.BlockSize.Y.
- Support (FERCellSupport): nodes are face-connected components inside a chunk,
  edges per chunk pair (grid neighbours and precomputed panel face contacts),
  refreshed only for chunks whose revision changed; union-find from anchors
  (bAnchorBottom/bAnchored/indestructible/ground rows). Evaluation runs on the
  50ms timer, never in a blast snapshot; detachment is bounded per step
  (RemovalsPerStep components, 32768 cells) with one cosmetic debris cube per
  component (multicast, cap 8/step, 64 global). Detached cells replicate as masks.
- Under the preset one grenade opens drywall/wood/sheet; 20cm brick needs
  several, concrete/stone resist by design. Probes pick breachable walls.
- Verify: test-destruction.py (11 automation tests), --game (cell lab), --collapse,
  --procedural, test-destruction-stress.py, test-cell-network.py,
  test-destruction-network.py, test-map-template.py --network,
  test-dust-template.py --network. --combat now runs the cell lab probe too.

## 5cm cell prototype (17 October)

- The user approved option C of docs/CELL_DESTRUCTION_PLAN.md: logical 5x5x5cm
  cells (wood 5x5x2, drywall boards/sheet/glass thickness from FERCellSettings),
  per-chunk alive bitsets plus sparse float HP, one UDynamicMeshComponent per
  32x32-cell chunk with visible faces only and aggregate-box collision. It is a
  mode of AERBlockRoom; superseded on 18 October by cells everywhere (above).
- Cell HP are the spec values; FERCellCalibration in the catalog stamps the
  resolution they were tuned for. A mismatch rejects generation with
  ER_BLOCK_CALIBRATION_MISMATCH: never reuse HP after changing cell size or
  construction thickness. Blast transmission is per TransmissionReferenceCm
  (20) of intact material, applied by path length; MaxShieldingCm 40.
- Bullets: voxel DDA through the hit panel, EntryEnergyCost once per material
  run, EnergyCostPerCm by path, each crossed cell damaged with residual energy
  at its entry. Grenades: pre-event snapshot, 9 static traces per candidate chunk
  (ignoring cell components), voxel exposure, one aggregated batch.
- Hits resolve cells from the impact point (never Hit.Item). Mesh and collision
  rebuild together for dirty chunks and their face neighbours; deferred
  automatic collision updates stay enabled on the components.
- FDynamicMesh3 rules verified on Metal: Unreal front faces satisfy
  GetTriNormal (edge2 x edge1) == normal; one overlay element per vertex (never
  shared across quad corners); right-handed UV frames (mirror U on negative
  faces). nullrhi tests cannot see these, so inspect --render captures.
- Network protocol 7: descriptor (cell settings, layout digest, shard manifest)
  plus AERCellNetShard fast arrays of RLE alive masks for modified chunks only.
  HP never leave the server. Clients apply complete manifests only, fail closed.
- Glass is a single pane cell. (Collapse was off in the 17 October prototype;
  ported to cells on 18 October, see above.)
- Verify with scripts/test-cell-destruction.py (four Cells automation tests) and
  --game (real rifle, grenades, crouch/stand breaches, HE fuse, mode switching),
  plus test-destruction.py regressions. Scaling: measure-cell-scaling.py.
# Map archetypes (19 October 2026, latest map scope)

The user's latest request supersedes the playable Killhouse/Dust/laboratory
requirements below. Use docs/MAP_ARCHETYPES.md. Only Central Building Industrial
belongs to production rotation; Depot remains a selectable reference. Retire the
five old levels and Arabian runtime art through Unreal after a verified local
archive and outside-reference check. Do not recreate those levels on routine setup.
Keep reference provenance/CT=Alpha/T=Omega, sample one half and rotate geometry,
starts and fixed collision. The central recipe varies dimensions, floors, side
buildings and cover, with open/climbable-wall/fenced-roof-and-side crossing rules.
Wire mesh is a fixed pawn/dynamic barrier; explicit visible metal frames/wires
block bullet/visibility/blast queries, while diamond apertures transmit them.
Production always samples the crossing seed, with no player or recipe preset.
MapCrossing is restricted to the non-shipping industrial probe. The wall is 3m
high and made of destructible brick, crossed by paired timber bridges/stairs
with destructible supports. Containers are hollow reinforced steel cells
(Strength=8), not fitted indestructible meshes. Reinforcement scales structural
HP/penetration/blast shielding while keeping the shared seven profiles and
their cell calibration unchanged. Preserve those defaults for all other maps.
Secondary buildings/towers vary 1–3 floors, materials and yaw; their timber
floors/stairs/supports use the same cell graph. Only ground/fixed main slabs seed
support: never permanently anchor these upper decks. Oriented rectangular
pieces retain their local cell grids, collision, support and half-turn pair.
Factory includes SM_Ladder01/02 as references. Revision3 adds paired destructible
steel ladders to the secondary buildings/towers. Revision4 prefers ladders:
towers and compact buildings use alternating interior floor hatches with a
ladder on every storey, keeping the ground door corridor clear. Only non-tower
secondary buildings at least800cm wide and1150cm deep retain internal stairs.
The central building keeps both stairwells and gains paired exterior ladders;
secondary exterior ladders remain alternatives. All ladders use the same
calibrated destructible steel, clear landings and half-turn pairing. Refresh
DA_CentralBuilding_42_PreferredLadders and recipe revision4 through Unreal.
Tanks/barrels and the wire perimeter remain fixed.
PCGEx 0.79.2 is installed/enabled. Its path analysis and exported PCG preview
share validated layout data; do not introduce asynchronous client collision.
LAN protocol8 supersedes protocol7 below: shard headers carry payload count/digest
and clients require their fast array to match before any transaction applies.
Keep the deterministic header-before-array late-join test in NetworkTransactions.
New automation replaces retired Killhouse/Dust asset-bound tests. The current
suite has 12 tests including CentralBuildingEvolution (intact support, steel
resistance, container/brick breaches and loss of timber access). Run
test-industrial-map.py, --game, --render --gallery and --network, inspect captures
and retain cell/destruction plus Depot LAN/accuracy/arsenal regressions.
Preserve the shared material catalog, Depot and all unrelated character/weapons.


Player mechanics (19 October): Landed applies authority-only fall damage through
normal health/death/protection, bypassing armor (ratio2); free-fall equivalent
height <=300cm is safe, >=1100cm lethal. Use pre-impact vertical velocity and
actual movement gravity, never client-reported height/damage. Ordinary jumps,
steps and mantle/ladder exits remain safe.
Ladders use custom movement2 with saved-move replay; custom1 is ordinary mantle
and custom3 is the roof transition from a ladder (also treated as mantle).
W/S climbs, Space detaches; weapon stow/restore is automatic, with no X binding.
Keep Weapon as the selected inventory pointer. IsWeaponStowed derives from
movement and selected data, so all peers hide incompatible weapons, including
the roof transition. Pistols (bOneHanded+Bullet), knives and hand grenades remain
held and usable at144cm/s; other weapons are stowed at180cm/s.
Moving pistols add LadderMovingSpreadDegrees (default0.6 degrees half-angle)
scaled by abs(vertical speed), deadzone5cm/s and full penalty at144cm/s.
Stationary rungs add no penalty; GetSpreadDegrees is shared by shots and HUD. Permit selection of any
slot on ladders, showing compatible items immediately and restoring the latest
selected heavy weapon on exit. Stowed weapons cannot fire/reload/scope; cancel
active actions on entry. Physical rungs/handles must survive; no invisible
climb route. The bottom exit needs neutral input before reattaching. Roof exit
uses the existing swept timed traversal. Steel rails use disjoint calibrated
2mm sheets (including sub-cm template pieces), not changed catalog thickness.
Run test-player-mechanics.py (optionally --render), the two EclipseRecoil.Player
native tests, destruction, movement and accuracy. test-movement.py now uses a
transient probe fixture in Depot; never recreate a saved laboratory map.
Rendering uses ERResolutionPolicy through IDynamicResolutionState and real RHI
GPU frame history. Target60, entry thresholds56/66 FPS, sustained3s, stable2s,
hold4s, startup4s. Changes are continuous only during adaptation. Preserve the
secondary1080p cap, viewport/UI, supported upscaler bounds and reset behavior.
-ERPerformanceAudit logs governor state; synthetic30/60/144Hz tests check dips,
59FPS tolerance, convergence, cooldown and delayed quality recovery.


Industrial finishes (19 October): user approved gray, lightly worn Factory steel
for ladders/metal structures and varied Factory paint for containers. Geometric
symmetry exists for gameplay balance; cosmetic container colors need not match
between half-turn copies. Use independent deterministic visual rolls, never
consume the geometry RNG. Keep server/client choices identical for a given seed.
Recipe revision5 owns SteelSurface and ContainerSurfaces; per-cell-panel Surface
is visual only and part of layout identity. Cosmetic debris resolves the source
panel finish from its original centre using the existing payload. Preserve Metal calibration, Strength8
containers, penetration, support and collision, shared catalog and Depot. Generated
materials live in private Art/Industrial/Materials and adapt cropped BC/N/ORM
regions of existing Factory atlases; do not edit original Factory materials.
Use setup_factory_finishes.py, refresh DA_CentralBuilding_42_FactoryFinishes,
then verify_industrial_maps.py and inspect real Metal captures and LAN palette checks.

Intermediate cover (19 October, recipe6): furnish approaches in multiple bands
with destructible L/U pockets, brick/concrete screens, timber crates and steel
containers. OpenYard additionally reserves paired midfield flank screens.
Preserve half-turn geometry and independent cosmetic rolls. Validate entrances
and hide positions with real standing/crouched capsules and eye-height rays;
reserve spawn space, patrol/crossing/stair routes, footprints and ladder access.
Later pieces must not obstruct an earlier pocket's entrance or test sightline.
Keep cover metadata transient; export CoverPockets in the PCG snapshot. Use
setup_intermediate_cover.py and DA_CentralBuilding_42_DenseCover, then native
destruction/516-seed checks and the OpenYard Metal LAN probe with cover breach,
late join and regeneration. Preserve shared damage profiles, Depot and Factory art.


## Minimap and team recon (19 October)

See docs/MINIMAP_RECON.md. Orient the HUD using the local controller's yaw each
rendered frame: player forward is always up for both teams. Keep the overview
centered on the map. Rotate image polygons, fog, contacts, headings and north
reference together; retain the common world-space fog grid and bird capture.
Owner-only UERReconComponent snapshots carry each team's explored64x64 mask and
current visible contacts; never derive enemy
markers by enumerating client pawns. Server sight4Hz uses living players/bots
(120deg/map extent) and the active team drone(360deg/42m horizontal footprint),
with real geometry cover. Drone altitude does not shrink this footprint.
Terrain persists within a round; contacts expire immediately on lost server
sight (HUD1s stale timeout). Late join/respawn preserve fog and stock; only a
round reset or generation epoch clears them. Gate the static512px bird capture
on matching local geometry epoch, and hide dynamic actors from that image.
Each team has one closed indestructible case and five total drones, including
the initial launch. Positive authorized bullet point hits destroy a drone;
replace after30 server seconds, stop after five losses. Flight evaluates
replicated parameters against server time. Art references remain in private
Blueprint defaults under Art/Recon. Original ZIPs/models/credits and derivatives
remain in maps SourceAssets/ReconDrones; preserve blockv_lb/polbrainstorm CC BY4.0
attribution in source, metadata and menu. Use bake_recon_models.py/setup_recon.py,
test-recon.py --automation / --render, and LAN/accuracy/industrial regressions.
The overhead image combines floors and remains static after destruction.
Smoke now blocks recon on the server: integrate occupied density with growth,
fade, shared contour phase and authorized shot/blast pockets (visual noise mean).
Apply to player/drone contacts and new fog exploration, preserving known terrain.
Record bullet cuts before the dedicated-server presentation guard. Preserve bot
AI and combat behavior. Enemy HUD markers are outlined team-colored diamonds.
Drone hits multicast five source-derived local fragments, capped24, WorldStatic
physics only, settle4s/expire6s and clear on epoch/reset/exit. Assets stay in
Art/Recon Blueprint defaults with original CC BY4.0 credits. Verify SmokeOcclusion
automation and the far-sight/smoke/real HUD/debris LAN cases in test-recon.py.

## Configurable server capacity (20 October)

The latest user request supersedes every earlier fixed four-player/four-slot
rule. Start Server owns a saved positive human capacity (default 16, listen host
included); there is no fixed upper player cap. GameSession, pending admission,
LAN availability, replicated lobby status and travel must use that same setting.
Explicit per-team test bots remain additional; ordinary patrol bot slot budgets
use the configured capacity. Preserve strict majority of humans present for READY.
Use all procedural team starts, with cached seed/epoch preparation, and safe
server-side overflow positions near each team's base for occupied starts.
Never spawn through walls, on unsupported floor or on top of another capsule;
wait/retry when space is unavailable. Verify test-capacity.py on Depot and
Central Building, native Menu tests, saved settings and combat/bot regressions.


## HUD readability and local minimap (20 October)

The latest user request supersedes the earlier full-map overview and reticle
number requirements. Share Teko SemiBold between menu titles, HUD and FPS text
through ERInterfaceStyle.h; retain larger viewport-scaled text and dark shadows.
Keep the numeric ammo summary in the bottom weapon panel only. Ammo/spread dots
retain the real server cone and count in every aiming mode, with dark feathered
shadows and state colors. The minimap follows the local pawn at a fixed40m radius,
rotates with local yaw and clips terrain/fog/contacts together. Preserve world
fog coordinates, server visibility and stale-contact rules. Static bird capture
is now mipmapped trilinear1024px; the circular shadow/rim use feathered alpha128-segment
geometry. Do not change the main-scene60FPS governor for minimap presentation.


HUD refinement (20 October): the performance overlay uses Teko11 (half the
previous22), showing FPS and effective scene dimensions only, without ms/3D/auto.
Top-center HUD has blue Alpha score, neutral50 target, orange Omega score and
a smaller timer, without a panel or team names. Remove the top-left team name
and bottom TAB/ESC hints. Bottom-left HP/AP is25% smaller; sprint/posture about19%
smaller with viewport-scaled left margin and clear space below. Preserve inputs.


HUD edge alignment (20 October): bottom rows use a shared40*HUDScale edge inset.
Measure the actual font text in Canvas logical units with its current DPI.
Anchor HP/AP bottom-left and stack sprint/posture above using measured heights.
Anchor ammo/weapon/reload bottom-right using measured width and height, never a
fixed left X chosen for one weapon name. Preserve the current font sizes.


Menu version footer (20 October): keep Teko16 and shadow, disable AutoWrapText
for the single-line version, and anchor it bottom-right inside a title-safe
SSafeZone with32 padding. The footer must remain hit-test invisible and inside
the viewport after fullscreen/resolution changes on every menu page.

Cooked runtime correction (21 October): the game's AA2/GI0/reflections0/shadows1
profile lives in GameInstance startup for local and packaged execution; do not
restore launcher-only quality overrides. Preserve the 60 FPS governor/1080p cap.
Never use FBoneContainer::SetDisableRetargeting in gameplay: UE 5.8.3 forces raw
curve sampling and crashes in cooked games. The owned FPS skeleton now uses
Animation translation on pelvis/upperarm_r. bake_cooked_first_person.py migrated
51 clips through Unreal, preserving other weapons' effective poses and the raw
pistol fit; its private verified ZIP precedes the migration. Preserve the saved
ERCompressedFirstPersonPoseVersion marker and normal compressed evaluation.
Development Mac packaging must boot the actual cooked Depot with pistol-equipped
bots via test-packaged-runtime.py; a main-menu-only smoke test misses this crash.


Stamina revision (21 October): supersedes the five-second sprint limit. Double W
still requests 18km/h, now with ten seconds of stamina and
twenty seconds to refill from empty whenever not sprinting (normal run included).
Fresh gestures can spend partially restored stamina; held W never auto-restarts.
Use the existing replicated locomotion deadlines/server clock and saved moves;
interruptions retain the whole deficit. HUD uses a proportional Canvas stamina
bar above HP/AP, with posture stacked above it. No saved art/asset changes.
Verify test-movement.py (partial restart, owner drain/refill, InputKey/posture),
--posture-only --render for the actual HUD, and network/accuracy regressions.


Sprint key alternative (21 October): holding E while W is down requests the
same sprint as double W. E may be held before forward movement. Keep the two
input sources independent, clear both on menus/new gesture pawn, preserve
saved-move intent and the 10s stamina/20s refill rules. Held E alone must not
auto-restart after exhaustion. Verify real remote InputKey E/W press/release,
E-before-W and overlapping double-W release in test-movement.py.


Internet multiplayer (21 October): preserve LAN/offline alongside the new EOS
path. See docs/EOS_ONLINE.md. Internet explicitly selects OSS EOS/EAS/Connect
and public Sessions, while the default NULL subsystem supports local execution.
UERNetDriverEOS must use IP passthrough for ordinary listen launches and P2P
only with EROnline=1, including lobby travel. Never restore obsolete
bIsUsingP2PSockets config. Keep the private Config/EOS.local.ini ignored and use
only an untrusted Peer2Peer client credential. Do not claim EOS authentication,
NAT traversal or Internet matches verified until exercised against the service
with distinct accounts/two networks. A previous automatic approval rejection
blocked saving the clipboard credential to an unspecified document destination.
The user subsequently explicitly authorized saving the existing secret at
/Users/agea/git/respawn-and-rage/core/Config/EOS.local.ini and EOS service testing;
do not request the same approval again. Keep mode0600 and never print the secret.
The user limited the current EOS test to this Mac. Real one-account EOS
login/public-session discovery/P2P listen/READY travel and destroy/cancel/rehost
are exercised by scripts/test-online-service.py. Require process exit0 as well
as the PASS marker; a real module-shutdown crash was found this way. EROnline
must release its owned world-specific OSS instance while EOSShared is still
loaded, after world net drivers have shut down. Exit Game waits briefly for
session destruction before exit and suppresses LAN discovery while quitting.
Keep cross-account JoinSession and two-network NAT/relay claims separate from
this single-Mac service test. See docs/TEST_RESULTS.md for executed outcomes.

Team readability (23 October): stronger blue Alpha/orange Omega upper-body cloth
supersedes the muted clothing palette above. Preserve skin/hair and neutral lower
gear; no emissive clothing. setup_team_readability.py updates only owned saved
materials with a ZIP rollback and an idempotent cloth brightness lift. Recon drone
underside lens flashes180ms every2.5s from replicated flight/server time, using
team colours and ordinary depth testing; no collision or shadows. References stay
in private BP_ReconDrone defaults. Preserve recon authority, flight and stock.

Drone colour revision (23 October, follow-up): remove the underside beacon and
its flashing completely. Keep the approved character palettes. Colour the upper
white/beige drone shell with matte Alpha blue/Omega orange; preserve the original
underside, rotors and mechanical materials. setup_drone_team_colors.py backs up
owned recon assets, configures team arrays on BP_ReconDrone and deletes the two
unreferenced beacon materials through Unreal. Upward-facing shell mask follows
the existing yaw-only flight. This supersedes the beacon instructions above.

Smoke radius/border (23 October): horizontal maximum radius4.4m (+10%), retaining
4m height,12s lifetime, growth/fade, team tints and cut/dispersal rules.16x16x8
containment now spans880x880x400cm (55cm horizontal cells). Add a small travelling
sway to the contour using shared server age/replicated phase; keep shader and
ReconDensity coefficients aligned. Regenerate smoke only with backed-up
setup_smoke_shape.py; preserve HE and approved character/drone appearance.

Dark drone beacon / server maps (23 October, latest): restore the original drone
shell and underside team beacon, superseding upper-shell paint and beacon removal.
Flash180ms every2.5s as before, with low-intensity saturated linear colours
Alpha(.008,.065,.32), Omega(.35,.035,.003), no HDR multiplier.16cm lens. Keep
art references in BP_ReconDrone; setup_drone_beacon.py backs up recon assets and
removes unreferenced paint materials. The old paint setup delegates to it.
Depot is excluded from server lobby buttons, accepted votes and random/tied
selection; use catalog InRotation on all those paths. Keep catalog IDs stable
and preserve Depot as an offline/reference map. Production pool: Central Building
and Middle Eastern Alleys. Test lobby travel and excluded/stale/no-vote cases.

Drone underside emblems (23 October, latest clarification): apply the supplied
Alpha/Omega graffiti as stickers/paint directly on the drone hull underside,
using an upward-facing bounded decal attached to Body. No floating plane,
beacon or flashing. Reuse original graffiti textures with owned underside decal material variants
(.2 colour floor for shadow readability, no HDR boost);
team follows replicated Flight.Team. setup_drone_symbols.py backs up recon assets
and retires unused beacon/plane materials after reference checks. Old beacon/
paint entry points delegate to it. Preserve all gameplay and map exclusions.

Alpha uniform refinement (23 October): tone the approved character blue down to
(.10,.30,.72) times each skin's preserved strength, replacing the vivid
(.055,.38,1) tint. Keep the cloth readability lift, original skin/neutral surfaces,
Omega uniforms and team artwork. setup_team_readability.py supports a TEAMS
selection for scoped migrations; the latest pass updates Alpha only.

Phase Engineer helmet (23 October, latest): remove the rejected faceted procedural
helmet from Omega skin9 and retain its supplied scalp/buzz-cut appearance. No suitable separate
helmet/mask replacement was found locally; the user explicitly prefers no helmet
in that case. Preserve other approved appearances. remove_phase_engineer_helmet.py
backs up and updates character/target/corpse catalogs; the historical helmet-fix
entry point delegates to removal so reruns cannot restore it.
