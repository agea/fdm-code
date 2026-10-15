# Eclipse Recoil Unreal development

Use `docs/ECLIPSE_RECOIL_UNREAL_BASE.md` as the gameplay specification and
`docs/DEVELOPMENT.md` for the user's current scope and implementation status.
The user's current request takes precedence over the attached specification:
work on a locally playable Unreal alternative; defer packaging, distribution,
CI, external publication and standalone platform builds.

14 October publication scope: the user explicitly authorized public agea/fdm-code
and three private asset submodules (characters, weapons, maps), preserving the
original private agea/fdm. See docs/ASSET_SUBMODULES.md. This supersedes the
earlier publication deferral for these four repositories only. Keep licensed
binary assets out of every public commit; do not choose a code license implicitly.
Content/SourceAssets are generated mounts into Assets/*; edit/save via Unreal
at unchanged /Game paths, commit private assets first and then their gitlinks.
Do not run old cleanup/migration commands against linked vendor folders without
reviewing their new repository ownership.

- Keep this existing UE 5.8.3 project (`fdm.uproject`, native module `fdm`).
- Do not modify the original game under `/Users/agea/git/csgopen/eclipse-recoil`.
- Combat, ammo, teams, scoring, protection and respawn are server authoritative.
- Point damage supplies server-authorized shot direction to owner hit feedback.
  Keep camera flinch cosmetic; never move the capsule or change ControlRotation.
- Death spawns a separate replicated cosmetic team-character corpse. Ragdoll contacts
  only WorldStatic, never players/projectiles; retain it through respawn, settle
  after five seconds, cap twelve/90 seconds and clear on round reset.
- Rifle spread is real and server sampled; the ammo reticle reads that same
  cone plus replicated owner burst state against the synchronized server clock.
  Keep posture, speed, airborne and burst penalties shared with the HUD.
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
  runs at 18km/h for at most five seconds, followed by ten seconds of recovery
  including early interruption. Require a fresh gesture after recovery. Hold
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
Local LFS storage is /Users/agea/git/fdm/_cache/lfs; no objects pruned.
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
