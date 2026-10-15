# Eclipse Recoil — procedural destruction laboratory

Eclipse Recoil is a tactical multiplayer FPS. This procedural laboratory adds
a logical block world with penetration and radial destruction to the existing UE 5.8.3 project and FPS controls.
The imported design is [ECLIPSE_RECOIL_PROCEDURAL_DESTRUCTION.md](ECLIPSE_RECOIL_PROCEDURAL_DESTRUCTION.md).
It is a design reference; current scope is Milestones 1–6. Seeded multi-room/multi-floor layouts reuse
logical storage/combat, controlled collapse and authoritative replication.

## Launch

Current session menu (15 October): run `scripts/play.command`, choose local or
listen server, then **Mappa / laboratorio** → **Laboratorio distruzione** or
**Piano procedurale**. Set team and bots before starting. Clients select
**Connettiti come client** and enter the host address. Listen sessions start
with one human; all laboratories allow repeated timed reloads without exhausting
reserve. Earlier milestone restrictions below describe their historical scope.

Compile the existing Editor module using the root README command. Generate the
new owned assets once, using Unreal (the command below is for the verified Mac
installation; `UE_ROOT` can point to another installed UE 5.8.3):

```sh
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/fdm.uproject" -run=pythonscript -script="$PWD/scripts/setup_destruction_lab.py" \
  -nullrhi -nosound -unattended -NoSplash
scripts/play.command
```

Alternatively choose **LABORATORIO DISTRUZIONE** from the main menu, or open
`/Game/EclipseRecoil/Maps/L_DestructionLab` in the Editor and use standalone
Play with one player. Do not use a listen/dedicated server for this milestone.
The normal two-team match condition is bypassed in the two standalone destruction labs,
so the existing rifle fires without requiring an opponent or bot.

WASD/mouse, crouch, jump and reload keep their existing bindings. Shoot a wall
with the ordinary weapon. Open the Unreal console (its configured Console key)
and run:

- `ERBlockDamage 25`: damage the block under the aim point by 25 HP.
- `ERBlockDestroy`: destroy that block immediately, independently of material HP.
- `ERBlockShot 100 10`: shoot a low-damage test ray with 100 gameplay energy
  units. It can hit the target behind drywall while the wall remains solid.
- `ERBlockExplosion 250 180`: block-only debug explosion 10 cm before the aimed
  surface, radius 250 cm and damage 180. It opens a breach without damaging the pawn.
- `ERBlockRegenerate`: return the pawn to the room centre, reset target HP and regenerate intact
  walls, resetting mappings/health/counters without duplicating components.

The HUD/log shows the last affected stable ID and HP, block/instance counts,
chunk/render-group counts, generation and last update times. The original fixed-room lab uses no randomness; the procedural lab below uses
the seed to determine room dimensions, wall materials and window positions. Collision updates are included in update
elapsed time, not separately profiled. No FPS or collision-time guarantee is made.

## Geometry and state

The interior is exactly 500×500×300 cm. Blocks are 50 cm along the wall, 20 cm
thick and 25 cm high. Two walls have 120 blocks each. The perpendicular returns
cover the exterior corners over 540 cm with clipped 40 cm end caps, giving 132
blocks each: **504 total**. Corner volumes touch without overlapping. The floor
is a separate static mesh and extends outside the room for breach traversal.
It accepts no block damage and never enters the logical block database.

`UERBlockWorld` owns records keyed by `(Wall, Column, Row)` independently of the
rendering. `AERBlockRoom` projects them into one ISM per spatial chunk/material,
with both directions of the ID/instance mapping. Damage is event-driven.
Destroyed records retain their IDs; only their rendering/collision disappears.

After destruction, each dirty group removes only destroyed instances through
Unreal's persistent `FPrimitiveInstanceId` API, then rebuilds its logical/index
mapping from `GetInstanceIndexForId`. It never assumes RemoveAt/RemoveAtSwap
behaviour, ignores an entire group for tracing, or recreates surviving bodies.
The handle is an internal rendering/collision identity for that generated
component's lifetime; gameplay still uses wall/column/course IDs.

Generation (or recovery from invalid native handles) clears and bulk-inserts a
group. These runtime-generated ISMs must not be replaced by HISM or manipulated
through Editor APIs that invalidate native IDs. Component-to-group hit lookup
is direct; grouping preserves first-seen deterministic storage order. A blast
aggregates requests per logical ID and updates each dirty group once. Empty
groups remain available until regeneration; their collision bodies are released.
The default render mesh remains the Engine cube with centred 100 cm dimensions
and simple box collision.

`DA_ER_BlockMaterials` configures the three surfaces and maximum health:
Drywall 30 (walls 0/3), Brick 140 (wall 1), Concrete 300 (wall 2). The native catalog
provides the same fallback values. Generation rejects missing/duplicate material
profiles, invalid dimensions and allocations above 100,000 logical blocks.
Runtime edits to the catalog take effect on regeneration.

## Assets and scope

The setup script creates three owned materials, one material catalog and the
lab map through Unreal; reruns preserve existing geometry/materials and apply
the managed fill-light defaults. The map reuses
`BP_ERGameMode`, its character/controller and existing input. No Fab assets or
commercial destruction plugins were added. Native source/docs/scripts live in
fdm-code; binaries are generated under the existing private maps asset mount.
No asset binary belongs in the public code repository. Existing cache/build
ignores and private-asset LFS rules already cover this milestone.

Milestones 1–5 required authority and NM_Standalone to avoid mismatched network
collision. M6 replaces that restriction with server authority and validated
client descriptor/snapshot application, described below.
The existing server-authorized point-damage pipeline is reused only for ordinary
shots in the standalone lab. No client-supplied damage/BlockID RPC was added.

The ordinary server-authorized weapon pipeline continues rays after an
authoritative block hit; ordinary map surfaces and obstructed muzzles retain their
first-impact behaviour. Physical HE/rocket detonations now also damage blocks
on authority. Smoke/stun do not. Player cover is evaluated before wall removal.
At Milestone 2, the room used collision-free baked explosion fragments.
Milestone 5 adds the bounded dynamic structural debris described below. M6 adds block networking and late-join snapshots below.

## Repeatable checks

```sh
python3 scripts/test-destruction.py
python3 scripts/test-destruction.py --game
python3 scripts/test-destruction.py --render
python3 scripts/test-destruction.py --combat
python3 scripts/test-destruction.py --combat --render
python3 scripts/test-network.py --shot-feedback
```

The first command runs Unreal Automation tests for storage/geometry and real
ISM/Chaos collision. It removes all 504 blocks in a seeded random order, checks
surviving IDs/transforms and traces after each removal, sweeps a full-size capsule
through the cleared wall, checks floor collision and regenerates five times.
Return corner caps are checked by mapping/transform rather than perpendicular
traces, which could meet the touching wall.

`--game` uses an opt-in probe in the saved standalone map: actual character
movement and wall blocking, console damage/removal, an authorized ammo-consuming
shot, walking through a 150×200 cm breach and safe regeneration. `--render` uses
Metal and additionally captures `Saved/Testing/Destruction-{Blocks,Breach}.png`.
Automated motion does not replace human keyboard/mouse feel testing. The network
suite is a regression check of existing gameplay, not block replication coverage.
Logs and automation reports stay under ignored `Saved/Testing/`; actual outcomes
are recorded in [TEST_RESULTS.md](TEST_RESULTS.md).

Manual acceptance: remove a block, then an adjacent block; remove a complete
column; verify the hole with a trace and movement after making it wide/tall enough;
regenerate and repeat. A single 50×25 cm missing block is smaller than the player.

Subsequent Milestones 4–5 add profiling and controlled structural collapse below.
M6 below supplies coherent replicated state/collision; earlier standalone
restrictions remain the historical Milestones 1–5 acceptance boundary.

API references checked against the installed 5.8.3 headers and Epic's
[ISM overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/instanced-static-mesh-component-in-unreal-engine)
and [ISM API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UInstancedStaticMeshComponent).


## Milestone 2: energy, exposure and targets

Three standalone cube targets behind the walls show remaining HP. Aim south
for drywall, east for brick or north for concrete. The HUD/log reports crossings,
remaining gameplay energy and terminal target damage; an intact wall can hide a
damaged rear target. Normal weapon shots use their existing damage and spread.
`UERWeaponData.BlockShot` exposes energy (default 100), maximum crossings
(default 8, hard cap 16) and minimum impact cosine (default 0.15).

| Material | HP | Bullet damage scale | Entry energy | Energy/cm | Explosion scale |
| --- | ---: | ---: | ---: | ---: | ---: |
| Drywall | 30 | 0.5 | 5 | 0.5 | 2 |
| Brick | 140 | 1 | 20 | 5 | 0.8 |
| Concrete | 300 | 1 | 40 | 15 | 0.25 |

These are configurable gameplay values, not real ballistic measurements.
Block damage is weapon damage × residual/initial energy × material damage scale.
Crossing cost is entry energy + actual path length in cm × energy/cm.
Keep room actor scale uniform; use configuration dimensions for axis-specific
sizing, since ISM physics composes instance/component FTransforms. The
rotated/scaled logical cube supplies its exit via slab intersection; the next
trace starts 0.1 cm past that exit, even if the block survives. A body beyond
receives weapon damage × residual/initial energy. No room/ISM component is
ignored, so another block in the same group can stop the projectile. Range,
crossing count, repeated IDs and grazing angle bound the continuation.

Explosions query logical chunk buckets with conservative volume padding and
then compute distance to each oriented block box. The initial collision
snapshot supplies one ray to an exposed face per candidate; neighbouring blocks
on that face do not shield each other. Static intervening geometry blocks damage.
All room exposures are evaluated before any removals: a concrete wall broken by
this blast still protects a wall behind it until the next event. This is a
simple line-of-sight model, without multi-sample exposure, blast-wave wrapping
or material transmission. Debug radii above 5000 cm are rejected.

Duplicate damage events aggregate per ID; each dirty chunk/material updates
once after the whole batch. HUD/log reports candidate, covered, damaged, removed
and rebuilt-group counts. The floor remains outside this damage database.

Material profiles select a small owned circular impact decal and the existing
faint Niagara glint. Per room: at most 24 decals and 24 emitters by default (hard cap 64 each),
8-second decals and a 15 m camera-distance budget. Regeneration clears them.
Debug explosions reuse BP_ERGrenadeExplosion with an eight-burst global budget;
physical grenades retain their existing effects and player-damage rules.
No new material-specific sounds are provided.

Rerun setup_destruction_lab.py after the C++ build to migrate the new combat
fields once (catalog metadata ERBlockCombatVersion=2), add owned impact materials
and three targets, and assign the existing explosion effect. It preserves HP,
surface materials and edited geometry. Later artist changes to migrated tuning
are preserved. New native fallback catalogs use the same gameplay defaults.

The BallisticsAndExplosion Automation test adds real target damage behind a
surviving block, default brick/concrete stops, higher-energy brick penetration,
normal/oblique/scaled paths, starting-inside/range/grazing/crossing limits,
spatial face queries, duplicate aggregation, blast capsule clearance and cover
across two room actors. The --combat probe exercises these controls in the saved
map, a real ammo-consuming shot, movement through the explosion breach and a
physical HE fuse. Metal captures Destruction-{Penetration,Explosion}.png.


## Source changes

- ERBlockCombat: bounded hitscan continuation and oriented-box exit calculation.
- ERBlockWorld: configurable combat profiles and logical spatial buckets.
- ERBlockRoom: batch mutation, snapshot exposure, effect budgets and diagnostics.
- ERBlockTarget: standalone rear target and HP label.
- ERWeapon: standalone block continuation and HE/rocket block damage.
- ERPlayerController: test-shot/explosion commands, target reset and HUD.
- ERBlockCombatTests / ERBlockLabProbe: physics and real-game acceptance.
- ERNetworkProbe / ERArsenalProbe: observed-event waits in test-only audits;
  production firing, smoke and networking behaviour unchanged.
- setup_destruction_lab.py / test-destruction.py: owned assets and repeatable
  combat runs. Real validation is in TEST_RESULTS.md.


## Milestone 3: connected single-floor layout

Choose **PIANO PROCEDURALE** from the main menu or run
`scripts/play.command` → **PIANO PROCEDURALE**. The map is
`/Game/EclipseRecoil/Maps/L_ProceduralLab`, default seed 42 and eight rooms.
Create assets in order after compiling: `setup_destruction_lab.py`,
`import_procedural_textures.py`, `setup_procedural_lab.py`, using the Unreal
Python commandlet shown above. The source bundle must be mounted first via
`scripts/setup-assets.py`; see [texture inventory](PROCEDURAL_TEXTURES.md).

- `ERMapGenerate 7919 10`: regenerate using seed 7919 and ten rooms.
- `ERMapGenerate 42 6`: same generator with six rooms.
- `ERBlockRegenerate`: repeat the current seed/config, resetting damage.
- `ERBlockShot`, `ERBlockExplosion`, ordinary weapons and HE/rocket explosions
  use the same penetration/occlusion/batch implementation as the original room.
- `-ERMapSeed=7919` or the travel option `?MapSeed=7919` sets the initial seed.

`ERProceduralMap.LayoutConfig` exposes seed, room count 6–10, minimum/maximum
room size 400–800 cm in 50 cm steps, hall width 150–400 cm, branch length
150–800 cm, door width 100–200 cm and door height 200 cm to wall height minus
one 25 cm course. Default doors are 150×225 cm; windows are 100×100 cm with
a 100 cm sill. Block dimensions are currently restricted to 50×20×25 cm;
wall height is 250–500 cm in 25 cm steps. Invalid configuration preserves the
existing live world, and the console restores its prior configuration.

The bounded MVP uses two rows of variable rectangular rooms around a common
hall. Each room has a dedicated branch, two aligned doors and an exterior
window. Every branch reaches the shared hall; different seeds change room
dimensions, materials and window locations. Slot spacing reserves room for
the largest supported room and its walls. Return walls own each corner;
branch ends stop at the existing room/hall wall faces. Doors clip columns
at their jambs (including 25 cm end pieces) instead of leaving partial
blocking volumes. Stable IDs are wall/column/course coordinates in the
deterministic layout, independent of render instance indices.

Floors and ceilings are batched indestructible ISMs, with separate tile/wood
floors and OfficeCeiling001 above. Floor rectangles touch at doors without
overlap. An asphalt foundation touches their underside at -20 cm, providing
a walkable outer apron after breaches without overlapping floor volumes. `bCeilings` can be disabled in the generator for layout inspection.
Window openings contain no panes yet. The initial player start and respawns
are resolved from the same seeded layout; regeneration moves the player to
the first room centre and stops movement. A single map actor inherits all
existing block identity, damage, spatial queries, group updates and FX limits.

Milestone 3 was standalone; M5 adds collapse and M6 adds replication, multiple
floors and the four additional combat profiles below. Arbitrary packing remains
outside this layout. Generated arrays keep intact geometry descriptions;
the live health/destruction state belongs exclusively to `UERBlockWorld`.

Validation commands:

```sh
python3 scripts/test-destruction.py
python3 scripts/test-destruction.py --procedural
python3 scripts/test-destruction.py --procedural --render
python3 scripts/test-destruction.py --combat
```

The automation suite includes seed equality/difference, duplicate ID/volume
checks, real Chaos capsule sweeps from every room through both doors to the
hall, every opening's trace, random removals, regeneration and safe spawns.
The saved-map probe additionally walks the real character, changes seed via
console, fires the existing weapon and walks through an explosion breach onto
the asphalt apron. Results and measured timings are recorded in TEST_RESULTS.md.


## Milestone 4: profiling and stability

`ERBlockStats` prints generation phases (logical storage, grouping, ISM clear/add
and mapping), last update phases/upload count, tracked MiB, process resident
MiB, live ISM collision bodies and effect counts. HUD chunk counts are cached;
memory/resource inspection runs only on explicit request or the benchmark.
Engine CPU trace scopes identify regeneration, group rebuild/removal, spatial
queries, batch damage, shot continuation and explosion work in Unreal Insights.
ISM clear/remove/add timers include synchronous physics calls. They cannot
isolate pure Chaos time or deferred work; the same limitation applies to CSV.

The spatial index reports visited nonempty buckets, examined records and
returned live candidates. Destroyed records retain identity; queries filter
them, and an entirely destroyed world returns immediately. Effects retain
default limits (24 impacts of each kind / hard 64, eight global bursts) and
distance culling. Invalid effect positions/normals are rejected. Regeneration
and EndPlay destroy all block-owned decals, Niagara components and debug bursts.
Other combat effects retain their existing global limits/lifetimes.

```sh
python3 scripts/test-destruction-stress.py --samples 8 --label optimized
python3 scripts/test-destruction-stress.py --render --trace --label optimized
```

Two seeds × three chunks × one warmup plus eight samples produce 54 cycles
per run; 2,544 measured CSV rows cover generation, sixteen singles, a duplicate
request batch for 256 unique blocks, 32 sphere queries, an explosion, complete
destruction and a post-GC memory snapshot. The runner checks every removed
volume for absent collision, all mappings/body counts and floor/capsule support.
The automation suite adds eighteen 90%-removal cycles with exhaustive survivor
traces and allocation/component equality after regeneration. Metal adds actual
30-round firing, impact/burst caps, distance culling, regeneration cleanup and
component/burst cleanup on actor destruction. No artificial FPS threshold or
process-RSS equality is used; engine caches and GPU work are outside that claim.

The pre-optimization baseline used full affected-group clear/reinsert per
survivor. Both algorithm versions were measured with the same instrumentation,
seeds and validation workload. The current `--benchmark-only` skips the final
weapon/FX phases for call-time comparisons; it does not restore the old algorithm.
Baseline CSV is preserved locally, with aggregate results/methodology in
[DESTRUCTION_BENCHMARK.json](DESTRUCTION_BENCHMARK.json) and the root README.
Keep chunk400/block50×20×25 defaults; smaller chunks create many more components,
while larger ones increase the CPU work of rebuilding current-index mappings.

## Milestone 5: controlled structural collapse

`FERBlockSupportGraph` stores deterministic face contacts in local space across
chunk/material boundaries. Exactly one touching box axis (tolerance 0.01 cm)
and positive overlap on both others is required; edges/corners do not carry
support. Geometry must be axis aligned locally (90-degree turns are accepted).
The generated clipped wall returns and openings use this geometry. Graph build
is bounded to 50,000 records / eight million candidate pairs; unsupported
custom geometry logs `ER_BLOCK_SUPPORT_UNAVAILABLE` and keeps ordinary damage.

The default ground reference is local Z=0. Blocks touching/below that plane,
explicit `bStructuralAnchor` records and indestructible records seed the flood.
This is a rigid footing rule for the single-floor layout, including wall bases
outside the room slabs, rather than a load/weight/bending simulation. Ordinary
floor/ceiling meshes remain indestructible; ceilings do not anchor top courses.
Explicit anchors belong to block records supplied to LoadLayout. A surviving
side connection supports a lintel/cantilever. Removing its last connection
marks the whole unsupported component for collapse.

The graph is built once on regeneration. GenerationProfile.LogicalMs includes
logical storage, collapse cleanup and graph build; total generation includes
the subsequent ISM/floor work. Actual block removals schedule a
50 ms repeating timer, at most once per rendered/game frame (no catch-up bursts
after a hitch); no actor tick or idle graph traversal. A global flood
across this map actor is deliberately used for cross-chunk correctness. Each
callback examines at most 4,096 units (records/edges/phase transitions; hard
16,384), with a 1.5 ms graph deadline (hard 4 ms, checked every 16 units).
Additional damage is coalesced: finish the current removal-only traversal,
then repeat if the damage revision changed. Intermediate support can be a
conservative overestimate; it cannot detach a still-connected anchored block.
No repeated restart starvation during automatic fire.

Commit removes up to 128 unsupported blocks per callback (hard 256), updating
each dirty ISM once with native instance IDs. `Detached` retains logical ID,
material, transform and HP separately from direct `Destroyed` damage. Neither
state participates in hit resolution, damage or spatial queries. The removal
and spawn counts are bounded; the graph deadline does not promise an overall
callback deadline because synchronous ISM/Chaos calls are included separately.

Only a deterministic sample becomes temporary `AERBlockDebris` actors: four
per callback (hard eight), at most 32 globally by default (hard 64 across rooms).
Overflow is removed too, so no unsupported static remnants float in place.
Each selected block reuses its cube/material at 92% size, then enables Chaos.
The existing world-position PBR mapping reprojects while a fragment moves; a
dedicated fragment-local shader is outside this increment.
Physics-only collision contacts WorldStatic, ignores players/projectiles and
other debris, and cannot provide ballistic/explosion cover. Physics/collision
are disabled after two seconds; the actor expires after eight seconds (hard
20). These are cosmetic fragments, not persistent structural or damage bodies.
Regenerate and EndPlay clear owned fragments and the support timer. The owned
support arrays are counted in `SupportBytes`; shared meshes/materials, temporary
graph-build buckets, debris/Chaos engine heap and GPU allocations are outside
tracked memory. Debris actor/body counts are reported separately.

Open **LABORATORIO DISTRUZIONE**, face its south wall, and run:

```text
ERBlockCollapseDemo
ERBlockStats
ERBlockRegenerate
```

The demo removes the 5×5 perimeter on wall0, columns3–7 / rows5–9, isolating
nine inner blocks. Default sampling shows four falling fragments. Ordinary
weapon/explosion destruction schedules exactly the same structural path.
`Collapse.Enabled` and the budgets are editable on the map actor; changing
geometry/anchor settings requires Regenerate. M6 extends this path to authoritative replicated worlds below.

```sh
python3 scripts/test-destruction.py
python3 scripts/test-destruction.py --collapse
python3 scripts/test-destruction.py --collapse --render
```

Eight automation cases include face/edge/corner support, explicit/indestructible
anchors, supported headers, clipped returns, in-flight damage, bounded work,
native mappings, absent detached collision and supported procedural openings.
The saved-game collapse probe tests actual gravity, settle/lifetime cleanup,
hard global/per-step budgets across two actors, a forced 5 FPS interval proving
no timer catch-up, overflow, regeneration/EndPlay,
and an entire ten-room shell. Metal additionally saves before/falling images.

## Milestone 6: replicated worlds and multiple floors

The new menu action **OSPITA EDIFICIO PROCEDURALE LAN** opens the existing
procedural laboratory as a listen server with two floors, six rooms per floor,
all six opaque wall profiles and glass window panes. Join with the existing
IP address field. The ordinary Depot host action is unchanged. Single-floor
standalone launches retain their previous seed/layout/material selection.

Server options: `MapSeed`, `MapRooms` (6–10 per floor), `MapFloors` (1–3),
`MapMaterials` (0/1), `MapGlass` (0/1). `ERMapGenerate 42 6 3` changes seed,
rooms per floor and floor count on authority; omitting the third argument
preserves current floors. Unsupported config/allocation requests preserve the
previous live layout. Generation/load are bounded to 50,000 records; multi-floor
layouts require BranchLength>=250cm for the return lane. These are bounded
stacked layouts, not arbitrary room packing, streaming or dynamic navigation.

Each floor reuses the seeded room/hall algorithm with a deterministic seed
offset and unique wall IDs. Storey spacing is wall height+20cm. Upper floor
slabs are shared lower ceilings; dedicated ceilings appear only above the top
floor. A western stair tower has 150cm-wide treads, rise<=20cm, 30cm run and a
static inclined support. Landings return through a separate north-hall doorway,
so the next flight cannot block the previous flight's exit. Floors, stairs and
supports remain indestructible. Upper base courses have explicit structural
anchors to their static slab; this retains the M5 connectivity model without
claiming load/weight analysis or collapse of structural floors.

Seven gameplay profiles now exist: Drywall, Brick, Concrete, Wood, Stone,
Metal and Glass. Additional opaque profiles distribute deterministically across
rooms; WindowGlass fills selected openings with 2cm-thick destructible panes.
Current default gameplay values (editable catalog data, not real material laws):

| Material | HP | Bullet damage multiplier | Entry energy | Energy/cm | Explosion multiplier |
|---|---:|---:|---:|---:|---:|
| Wood | 60 | 1 | 7 | 1.5 | 1.2 |
| Stone | 200 | 1 | 30 | 7 | 0.5 |
| Metal | 400 | 0.5 | 60 | 15 | 0.35 |
| Glass | 8 | 2 | 1 | 0.1 | 1.5 |

The old three profiles retain their tuned data. `setup_procedural_multiplayer.py`
adds missing profiles/fills unset surfaces using the existing wood_planks,
stone_wall_02, rusty_metal_04 and dedicated glass materials. It preserves existing
HP/art edits and map geometry; save binaries through Unreal in the private maps
mount. Legacy three-row catalogs have native fallback gameplay defaults for the
four additions. The original texture archive/library is reused; no new download.

`AERBlockRoom` now replicates a generation descriptor: protocol6, epoch,
seed/dimensions/layout settings, block count, layout/catalog/collision-mesh
fingerprint, sequence and expected version for every state page. Clients build
geometry locally. Gameplay remains authority-only: ordinary owned-pawn fire,
ballistic continuation, point damage, explosions and structural checks run on
the server. There is no client RPC carrying desired block damage or block IDs.
Native debug mutation console commands execute only on authority.

`AERBlockNetPage` is one tickless, always-relevant channel per128 logical
records (at most391 for the global cap), with absolute HP/state and a version.
Only affected page properties/manifests are dirtied; geometry is never resent
on every shot/frame. Wire offsets address stable record order within an epoch,
validated by the layout fingerprint, rather than ISM indices. UE property delta
replication resends the latest state after loss. Independent bounded channels
avoid putting a fully removed world into one oversized initial snapshot bunch.
This trades some actor/channel overhead for bounded payloads; it is not one
actor per block and is not a bandwidth or FPS guarantee.

Before applying a transaction, a client matches **all** page versions against
the descriptor's manifest. Changed payloads are validated, then logical HP/state
and dirty ISMs commit together. Future pages wait for their manifest; skipped
intermediate transactions converge through absolute snapshots. Old epochs are
ignored. Protocol/layout/catalog/collision-mesh mismatches fail closed, leaving
collision gated. The fingerprint includes normalized geometry/IDs, gameplay
catalog values/surface paths, mesh path and serialized BodySetup GUID; it is a
consistency check, not cryptographic attestation.

Initial/late-join clients keep movement paused until the complete snapshot is
built/applied, then acknowledge that epoch through their own controller. Server
movement stays paused until acknowledgment; the server still validates physics
and shots. Regeneration destroys old page actors, starts a new epoch and moves
participants to safe team spawns. Native movement corrections synchronize those
server teleports. Normal ongoing deltas keep the previous committed collision
until the next coherent transaction arrives. A single block-world actor owns
this laboratory's layout; multiple independently streamed world actors are
outside this loading-gate workflow.

Structural decisions replicate as `Detached` states. Cosmetic debris remains
local, query-free and bounded; late join does not replay old fragments. Small
block impacts use epoch-tagged unreliable multicast with local distance/budget
culling. Server-only/dedicated sessions skip rendered cosmetics. `ERBlockStats`
reports epoch, sequence, page count/readiness and owned network buffers. Tracked
bytes now include page HP/state arrays and descriptor/index buffers, excluding
actor/channel engine heaps, wire bandwidth, shared assets and GPU memory.

```sh
python3 scripts/test-destruction.py
python3 scripts/test-destruction-network.py
python3 scripts/test-destruction-network.py --loss
python3 scripts/test-destruction-network.py --render
```

Eleven automation cases include multi-floor determinism/materials/IDs/slab
separation, real stair headroom/step-up clearance and upper anchors, plus staged,
reversed/future/invalid page arrivals and epoch reset. The LAN probe starts a
listen host plus an initial remote, a remote after destruction/collapse and an
additional dense-snapshot late remote (four human slots total). It compares full
state digests, sampled actual collision and all ISM mappings; the initial remote
fires owned weapon RPCs, walks a breach and ascends/descends two storeys with
real replicated character movement. It regenerates through three epochs and
checks an explosion batch on all remotes. `--loss` configures3% incoming loss
and40ms incoming lag on remote clients; `--render` uses Metal for host/initial
remote and captures the building view. Actual outcomes belong in TEST_RESULTS.md.
