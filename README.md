# Respawn & Rage

**Serious guns. Questionable players**

A tactical multiplayer FPS for quick deathmatch sessions with friends, compact
maps and server-authoritative gunplay. Built with Unreal Engine **5.8.3**;
currently playable locally and over LAN through the macOS arm64 Editor runtime.
Packaging and distribution are deferred.

Code is public in **respawn-and-rage/core**; licensed content is held in three private
submodules for characters, weapons and maps. A public code clone needs asset
access before it can run. See [asset setup and editing](docs/ASSET_SUBMODULES.md).

```sh
git clone https://github.com/respawn-and-rage/core.git respawn-and-rage/core
cd respawn-and-rage/core
git lfs install
python3 scripts/setup-assets.py --init
python3 scripts/setup-assets.py --check
```

Open `RespawnAndRage.uproject` with Unreal 5.8.3, or run `scripts/open-editor.command`.
Run `scripts/play.command` to play in a separate window using the Editor runtime.
The first window, including the main menu, uses 70% of the main screen's width
and height, accounting for Retina scaling (1280x720 fallback without a graphical
session). Choose modes and window size from the game menu.
The menu separates **Partita**, **Personaggio e armi**, and **Impostazioni**.
Choose **Locale**, **Crea server e gioca**, or **Connettiti come client**.
Local/server sessions select a map, 0–10 bots for each team, and stationary,
movement-only or combat bots. Select Alpha or Omega before starting/joining.
Clients enter the host IP and port; the host controls the map and bots.
The launcher binds servers to **0.0.0.0:7777**. Matches begin with one human,
including listen servers. Human LAN admission remains capped at four; explicitly
configured bots have a separate bounded budget (20 total).
All laboratories replenish reload reserve on authority while retaining magazine
capacity and reload duration; ordinary depot ammo remains finite.

Controls: WASD, mouse aim / left-click fire, Space jump / climb / respawn, Shift walk, double-tap W sprint (5s, then 10s recovery), hold Ctrl or C crouch,
double-tap Ctrl or C prone (press again to rise),
R reload, 1–8 / F / G or mouse wheel select weapon, right-click zoom, Tab scoreboard,
Esc menu. After dying, wait three seconds and press
fire or jump again. The respawn press is consumed; release before firing.

The requested Sketchfab arsenal is the default: 1 knife, 2 pistol, 3 SMG,
4 shotgun, 5 assault rifle, 6 carbine, 7 sniper, 8 Milkor grenade launcher,
F HE and G smoke. All ten slots keep independent ammunition and cooldowns.
The assault/carbine use the requested AK-47/SCAR-20 statistics. Server-owned
armor starts at 100 AP and bullets lose damage over distance using the supplied
worksheet. Source archives and credits are private in
`SourceAssets/SketchfabWeapons`; runtime assets live in
`Content/EclipseRecoil/Weapons/Sketchfab`. The original list, its English
translation, source mappings and rebuild details are in
[the weapon import notes](docs/WEAPON_IMPORT.md).
The previous Fab arsenal remains available with `?ERLegacyArsenal=1` for its
baseline regression fixture. Smoke does not affect bot vision.

Each server-authorized gunshot now plays a short first-person firing animation,
gentle cosmetic camera recoil, a muzzle flash and the template firing sound.
Other players hear spatialized shots and see the world rifle's muzzle flash.
The firing pose preserves both hand grips; camera recoil does not change aim.
The rifle leaves a small chipped crater on struck walls, with a dark center
and short cracks. The server replicates the hit to clients; marks fade after
20 seconds.

The crosshair has a ring of small borderless dots, about 3 pixels across at
720p: one white dot at 70% opacity per remaining round, with empty slots
black at 10% opacity. Soft Canvas edges smooth each dot; TSR and Mac high-DPI
rendering are enabled for the scene and HUD. The ring
widens with real rifle spread: crouching improves accuracy, movement and
jumping reduce it, and sustained fire builds spread that recovers after release.
Its radius includes a fixed readability margin; it indicates accuracy rather
than marking the exact screen boundary of the hitscan cone.

For solo testing, use **Locale**, set the Alpha/Omega bot counts and choose
**Fermi**, **Solo movimento**, or **Movimento e sparo**. Zero bots is supported.
Bots share server-authoritative health, weapons, scoring and respawn rules.
The same setup works when creating a server; clients cannot change its bots.
Players, patrol bots and stationary targets use Twinblast ActionHero for Alpha
and Drongo for Omega, with cold Alpha / warm Omega clothing and hidden Paragon weapons
and selected decorative attachments. The local first-person view uses the
selected skin's forearms and hands, with the left hand under the rifle and upper arms
excluded. Mannequin FPS assets remain available for reference.
Mouse sensitivity and acceleration are adjustable in the main and Esc menus.
Acceleration defaults to 50%: slow corrections use lower sensitivity, fast
turns use higher sensitivity. Set acceleration to 0% for uniform response.
Accepted hits play a directional impact sound and a small cosmetic camera
flinch. Dead characters leave a matching team-character ragdoll through respawn, for up to
90 seconds (maximum twelve bodies); bodies do not block players or bullets.

Select **Laboratorio movimento** under **Mappa / laboratorio** to
try twenty 2x2m blocks from 10cm to 200cm. Default run is 10km/h; hold either
Shift to walk at 5km/h. The standing capsule is 185cm, with eye height 170cm.
Steps through 30cm are automatic. Forward input mounts 40–80cm obstacles with
weapon held; hold Space+forward for higher ledges, including while jumping.
Mouse sensitivity uses an IA_Look scalar of 0.18. The first-person camera has
a subtle speed-dependent step bob that fades at rest and while airborne or
traversing; high climbing hides the left arm with the rifle.
Shoulder reach is 150cm above the current feet. The jump rises about 60cm.
This first traversal pass uses swept capsule motion; hand/climb animations
remain to be added. `scripts/setup_movement_lab.py` updates these assets through
Unreal and creates the lab only if absent. `python3 scripts/test-movement.py`
checks the actual map with host/client game processes.

Local execution requires the installed Unreal Editor and a compiled Editor module.
It is not a packaged standalone application.

The local launcher caps rendering at 90 FPS and selects medium antialiasing and
low global illumination, reflection and shadow settings. Dynamic resolution targets
about 72 FPS; the 3D scene output is capped at 1920×1080 before scaling to the
window, while the HUD keeps the window resolution. These are performance targets,
not measured guarantees. The menu offers 50–90% window sizes and fullscreen; the
FPS overlay also shows the approximate internal 3D resolution.

The local workspace lives in `/Users/agea/git/respawn-and-rage/core`; its sibling
`_cache` holds the private asset repositories’ Git LFS caches. Repository URLs
and cloning instructions use the `respawn-and-rage` GitHub organization.

To compile the Editor module after C++ changes, run `scripts/build-editor.command`
or use the equivalent command:

```sh
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" \
  RespawnAndRageEditor Mac Development -Project="$PWD/RespawnAndRage.uproject" \
  -architecture=arm64 -NoHotReloadFromIDE
```

The native `fdm` module, `/Script/fdm` classes, `/Game/EclipseRecoil` packages
and saved settings keys remain internal compatibility identifiers. They preserve
existing Blueprint references, edited maps and player preferences; the project,
build targets and player-facing title use Respawn & Rage.

`scripts/setup_editor.py` creates initial Blueprint/input/data/map assets inside
Unreal. It preserves existing maps. Do not rerun it to overwrite manual art work.
`scripts/fix_viewmodel.py` applies the saved rifle/hand correction to an existing
prototype through Unreal Python. It generates a separate static hip-fire pose
without changing the template animation.
`scripts/fix_controls.py` adds the C crouch alternative to an existing input
mapping without replacing its other bindings. Add `-ERInputAudit` to a local
launcher to log actual movement, stance, camera and ammo during graphical
testing. The audit is off by default and unavailable in Shipping.

See `docs/DEVELOPMENT.md`, `docs/TEST_RESULTS.md`, `docs/ASSETS.md` and the imported
Italian development specification. Only recorded tests are considered verified.

Choose one of ten Alpha and ten Omega characters in the main or Escape menu.
Alpha uses cold blue/navy clothing and Omega warm orange/rust, with neutral
equipment and natural skin. Variants combine different existing hero clothing
with owned field helmets and packs. Both preferences are saved locally; team
assignment remains automatic. In-match changes apply at the next spawn, and
existing corpses retain the appearance worn at death. First-person rendering
uses the chosen skin's extracted forearms/hands on the fitted independent FPS rig,
preserving existing weapon poses and grips.
Labels follow `character · role` consistently for Twinblast, Drongo, Belica and
Phase; `scripts/character_catalog.py` defines the twenty stable menu names.
Approved skins are saved and ready to play. Historical generation scripts
(`setup_team_characters.py`, `setup_team_skins.py`, `setup_phase_belica.py`)
require restoring archived source inputs before a new import/rebuild; they are
not launch steps. Use `normalize_character_names.py` only to refresh labels.
Verify saved references with `scripts/verify_team_skins.py`,
render the twenty choices and all corpse variants with
`python3 scripts/test-team-skins.py`, and test replicated selection with
`python3 scripts/test-network.py --skins --shot-feedback`. Check the new
characters with `scripts/verify_phase_belica.py` inside Unreal and
`python3 scripts/test-network.py --phase-belica`; the target-skin test checks
all twenty variants on a listen host and remote peer.

Repeat local checks with `python3 scripts/test-network.py` and
`python3 scripts/test-admission.py`. The network probe is opt-in and is never
spawned during normal play. Graphical input and multi-window tests remain listed
separately from headless test results.
Run `python3 scripts/test-bots.py` for bot gameplay and remote replication,
and `python3 scripts/test-admission.py --bots` for human priority over bot slots.
Run `python3 scripts/test-network.py --shot-feedback` to also check authorized
shot counts, empty-clip silence, unchanged server aim, and host/client assets.
`scripts/setup_shot_feedback.py` regenerates only firing presentation assets
through Unreal Python. `-ERShotAudit` enables per-shot development diagnostics.
Run `python3 scripts/test-network.py --accuracy` for real hitscan cone,
posture/movement, burst recovery and remote owner replication checks.
`-ERAccuracyAudit` logs graphical reticle ammunition, spread and radius.
Bullet weapons use immediate server Line Traces on the Projectile channel,
with cosmetic Niagara tracers from the visible muzzle (one in three shots by
default; configurable through `TracerEveryNShots`). Rockets and grenades retain
physical movement. Impact/muzzle glints are deliberately faint; tracers have clear gold emission.
Every second bullet shot ejects a small local dark casing with gravity and floor
contact (three-second lifetime, maximum 32; ignores players and shot traces).
`scripts/setup_hitscan_effects.py` builds owned Niagara effects
and four transparent decal variants from the private BulletImpacts source atlas.
Run `python3 scripts/test-reference-weapons.py` for the current ten-slot catalog,
actual client key bindings, reload, armor and physical Milkor projectile;
add `--render` for Metal viewmodel captures. `python3 scripts/test-arsenal.py`
retains the original catalog's shotgun/utility regression fixture.
`-ERHitscanGallery` with `?ERBots=2?ERBotMode=Target` captures the four decal
variants and a tracer sample in `Saved/Testing/Hitscan-Effects.png`; the gallery
holds the sample for 350 ms for screenshot readback (ordinary tracers last about
20 ms, with one visible frame guaranteed). The tracer origin follows the current
rendered barrel tip, 0.75 cm forward, after camera/animation updates.
Add `-ERTracerMotion -ERTracerAudit` to the gallery to verify a moving burst.
`scripts/fix_weapon_muzzles.py` updates owned muzzle data from actual gun geometry.


## Repository maintenance

See [cleanup and asset separation](docs/REPOSITORY_CLEANUP.md),
[license/provenance review](docs/ASSET_LICENSES.md) and
[measured asset summary](docs/ASSET_AUDIT_SUMMARY.json).
`python3 scripts/clean-project.py` previews log cleanup; `--apply` executes
only after Unreal/build processes are closed. Profiles `screenshots`, `cache`
and `build` are explicit. Settings, autosaves, Content, SourceAssets and Git/LFS
are protected. `scripts/audit-assets.py` reads Unreal package dependencies
without modifying assets. `python3 scripts/export-code.py --apply` creates
an ignored code-only review tree in BuildArtifacts/code-only, excluding assets
and Git history. These tools never upload files or change repository visibility.


The Mercenary vendor tree and old Characters/Mercenary tree have been removed. Fitted FPS rig/poses live in Characters/FirstPerson, while
forearm geometry comes from the chosen team skin. Verify removal readiness
with Unreal scripts/remove_legacy_mercenary.py (preview by default) and run
scripts/verify_no_mercenary.py. A game fixture with both trees absent passed
network and graphical checks. See docs/REPOSITORY_CLEANUP.md for safe removal
and backup details.


Character cleanup keeps the approved twenty skins and their required dependency
closure, with unused original/demo/retarget assets removed through Unreal.
The private rollback archive is outside the project in the sibling `_backups`
directory. Current evidence, exact counts and recovery instructions are in
`docs/CHARACTER_CLEANUP_RESULT.json` and `docs/REPOSITORY_CLEANUP.md`.
Earlier cleanup sizes/receipts describe historical checkpoints. No Git/LFS
history is pruned by asset cleanup.

Sprint and posture: double-tap W within 300ms and hold the second press to sprint
at 18km/h. Releasing W, walking, crouching, leaving the ground or climbing ends
sprint and starts 10s of recovery. A fresh double-tap is required after recovery.
Crouching/rising takes 0.4s; going prone/rising takes about 1.1s. Movement slows
during transitions; prone crawling is 55cm/s. Standing waits for ceiling clearance.
The prone world pose is procedural, layered over the existing team skeletons.
`-ERPostureGallery` captures FPS, Alpha and Omega posture images in Saved/Testing.


## Procedural destruction laboratory

The block-destruction prototype reuses this UE 5.8.3 project in a separate
standalone 5×5×3 m room. Run `scripts/play.command` and choose
**LABORATORIO DISTRUZIONE** in the main menu after generating the owned lab assets.
See [setup, controls and implementation limits](docs/PROCEDURAL_DESTRUCTION.md).
The imported [design reference](docs/PROCEDURAL_DESTRUCTION_SPEC.md)
includes later milestones. Penetration and occluded batch explosions are now
implemented in standalone. Choose **PIANO PROCEDURALE** in the menu
for 6–10 connected rooms generated from a seed.
`ERMapGenerate 7919 10` changes the seed and room count. The imported CC0 2K
library supplies continuous PBR wall/floor/ceiling textures. Multiplayer
destruction remains future work. See the same guide for test shot/explosion commands.


## Destruction stability and benchmarks (Milestone 4)

Runtime ISMs now use persistent Unreal instance handles. A batch removes only
destroyed instances and derives every surviving collision index from the engine,
without recreating their bodies. Logical wall/column/course IDs remain unchanged.
Generation uses bulk insertion and hash lookup; layout metadata no longer stores
a second copy of the live blocks. `ERBlockStats` prints phase timings, tracked
allocations, ISM collision-body counts and current effect counts.

Measured locally on UE 5.8.3 CL58210709, Mac arm64 M2 Pro / 32 GB / macOS 27,
with ten rooms, seeds 42 and 7919: one excluded warmup and eight measured cycles
per seed/chunk. The table pools both seeds (16 generation/batch samples and
256 single-removal samples per chunk). Timing includes synchronous ISM/Chaos
work on the game thread; validation, GC and memory snapshots are outside timers.

| Chunk | Before: single p95 / batch256 median | After: single p95 / batch256 median | After generation median | Tracked memory before → after | Groups after |
|---|---|---|---|---|---|
| 200 cm | 0.321 / 25.867 ms | 0.020 / 1.365 ms | 50.289 ms | 5.99 → 3.83 MiB | 420 |
| 400 cm | 1.211 / 43.901 ms | 0.032 / 1.821 ms | 43.470 ms | 5.68 → 3.78 MiB | 93 |
| 800 cm | 3.084 / 47.768 ms | 0.081 / 1.865 ms | 42.862 ms | 5.50 → 3.75 MiB | 42 |

Default chunk size stays **400 cm**: 200 cm uses about 420 groups versus 93,
with slower generation; 800 cm increases the cost of remapping a single group.
Block dimensions stay 50×20×25 cm. Spatial-query p95 at 400 cm is 0.010 ms,
checked against a live brute-force reference. The radius250 explosion median
(including query, cover and mutation) is 0.516 ms versus 1.243 ms before.

A separate Metal run passed the same cases and real weapon/effect stress. At
400 cm it measured single-removal p95 0.038 ms and batch256 median 1.858 ms.
This is a CPU call benchmark, **not an FPS/GPU benchmark**: deferred physics,
render-thread and GPU work are excluded, and dynamic resolution remains enabled.
Tracked MiB describe owned arrays/maps and UE exclusive ISM resource reports,
including collision resources; shared meshes/textures, total engine heaps and
GPU allocations are excluded. Process RSS is recorded separately and includes
engine caches. Warm local measurements do not establish platform-wide guarantees.

Stress validation covers exhaustive collision/identity checks after removing
90% of a ten-room plane, complete-map destruction and GC, repeated regeneration,
a real 30-round automatic burst, and Metal effect caps/cleanup: 24 default
decals/emitters per map actor (hard cap 64), eight global explosion bursts.
Regeneration and EndPlay clear owned block effects. State changes remain
standalone; controlled collapse is implemented in Milestone 5 below, and block
replication is implemented in Milestone 6 below.

```sh
python3 scripts/test-destruction.py
python3 scripts/test-destruction-stress.py
python3 scripts/test-destruction-stress.py --render --trace
```

Raw CSV, summaries, logs and the optional Unreal Insights `.utrace` live in
`Saved/Testing/destruction-benchmark-*`. The public
[measurement receipt](docs/DESTRUCTION_BENCHMARK.json) records the pooled data
and phase/memory definitions. Full test outcomes are in
[TEST_RESULTS](docs/TEST_RESULTS.md).


## Controlled collapse (Milestone 5)

Destruction now schedules a bounded structural check. Blocks connected through
real face contacts to the local ground plane or explicit/indestructible anchors
stay in place; disconnected components leave the ISM collision and a selected
sample falls under Chaos. Floors and ceilings remain static.

Defaults: 4,096 graph work units, 1.5 ms graph budget and 128 removals per 50 ms
callback; four fragments per callback, 32 global live fragments, physics off
after two seconds and cleanup after eight. Hard limits are 16,384 work units,
256 removals, eight fragment spawns, 64 global fragments and 20 seconds lifetime.
Graph deadlines exclude subsequent synchronous ISM/debris operations. This is
a connection-based support model for the current single-floor layout.

In **LABORATORIO DISTRUZIONE**, `ERBlockCollapseDemo` isolates a nine-block patch;
`ERBlockRegenerate` restores the walls and clears fragments. The same collapse
path follows ordinary shots and explosions. Detailed rules, configuration and
validation commands are in [the implementation guide](docs/PROCEDURAL_DESTRUCTION.md).

```sh
python3 scripts/test-destruction.py --collapse
python3 scripts/test-destruction.py --collapse --render
```


Local validation on UE5.8.3 / M2 Pro removed the base of a seed7919 ten-room
map: 827 directly destroyed blocks plus 9,248 detached blocks, 88 callbacks.
A forced 5 FPS interval also verified that timers never catch up with multiple
callbacks in one frame. Actual gravity, bounded bodies across two actors,
settle/lifetime, regeneration and EndPlay passed in both RHIs.

| Large collapse | Graph build | Callback p95 / max | Completion |
|---|---:|---:|---:|
| NullRHI | 6.733 ms | 0.512 / 0.630 ms | 4.424 s |
| Metal | 6.763 ms | 0.680 / 0.851 ms | 4.430 s |

These are CPU observations within one scripted run per RHI; callback times
exclude deferred physics/render/GPU work. Completion is game-world time.
The graph/traversal owned allocations were 427,567 bytes after collapse.
Eight automation tests, saved combat/procedural probes, 54-cycle stress in
NullRHI and Metal, and LAN/accuracy/arsenal regressions passed. Final Metal
stress preserves the 54-cycle workload and validates all CSV memory subtotals;
chunk400 ordinary generation median is 54.741 ms, single-removal p95 0.039 ms,
batch256 median 1.942 ms and tracked memory 4.094 MiB including the support
graph. Those operations run before deferred collapse, so they describe ordinary
calls and graph-build overhead. The M4 receipt above remains historical.

Methodology, source/CSV hashes and outcomes are in the
[collapse measurement receipt](docs/COLLAPSE_BENCHMARK.json) and
[TEST_RESULTS](docs/TEST_RESULTS.md). Local screenshots are
`Saved/Testing/Collapse-Before.png` and `Collapse-Falling.png`.


## Replicated procedural buildings (Milestone 6)

Choose **OSPITA EDIFICIO PROCEDURALE LAN**, then connect other players using the
existing IP field. The building has two floors and six rooms per floor, stairs,
wood/stone/metal walls and destructible glass alongside the original profiles.
The ordinary Depot LAN action and single-floor laboratory defaults remain.
Authority console `ERMapGenerate 42 6 3` selects up to three floors; server URL
options are MapSeed/MapRooms/MapFloors/MapMaterials/MapGlass.

The server owns HP, shots, explosions and collapse. Clients reconstruct from
seed/configuration and apply bounded versioned state pages only when the whole
manifest matches. Late join receives current absolute HP/removal states; epoch
reset discards old deltas. Initial movement waits for complete collision, then
uses the existing authoritative character movement. Local fragments remain
cosmetic and bounded. No client block-damage RPC was introduced.

Floors/stairs are static; upper base courses anchor to shared slabs. Layouts
are bounded to50,000 records, one active block-world actor and1–3 stacked floors;
multi-floor return lanes require BranchLength>=250cm. The support model remains
connectivity, and the system does not provide arbitrary packing/streaming or
load-bearing floor collapse. Compatibility fingerprints reject differing
protocol/layout/catalog/collision assets. Page actors add bounded channel
and CPU-memory overhead; no platform-wide bandwidth or FPS claim is made.

```sh
python3 scripts/test-destruction.py
python3 scripts/test-destruction-network.py --loss
python3 scripts/test-destruction-network.py --render
```

The network probe uses a listen host, two remotes including a late join, then a
third remote after complete wall removal. It checks coherent state/collision,
real client firing/breach traversal, three generation epochs, actual ascent
and descent across two storeys, and replicated explosion changes. Methodology
and actual outcomes are recorded in [TEST_RESULTS](docs/TEST_RESULTS.md).
Configuration, material values and replication details are in
[the implementation guide](docs/PROCEDURAL_DESTRUCTION.md).

Final local validation passed11 automation tests and both real network probes:
incoming loss3%/lag40ms on all remotes, and host/initial remote rendered with
Metal. The two-floor case had11,529 blocks/91 pages; the three-floor case had
17,427 blocks/137 pages. Late joins matched full HP/state, sampled UE collision
and all mappings, including complete wall removal and regeneration. Client
movement traversed the breach and climbed/descended two storeys; the final
explosion changed141 blocks and removed53 identically on every remote.
Existing combat, procedural/collapse,54-cycle stress, LAN/accuracy/arsenal
regressions also passed. The simultaneous local graphical capture showed12FPS
with dynamic resolution; this is no isolated-game performance guarantee.
Evidence/method/limits and hashes are in the
[network receipt](docs/DESTRUCTION_NETWORK_RESULTS.json). Licensed binaries
remain in the private maps mount; no publication/packaging was performed.


## Community-inspired procedural templates

Select **Killhouse procedurale** for an industrial arena inspired by the user's
CS:GO reference. Each menu session chooses a new seed: arena dimensions vary
36..48 x70..90m, with8..12 paired modules chosen from through rooms, L/U cover,
staggered walls, cargo/tank clusters and wooden stacks. Base positions and sizes
vary while retaining rear entry, front windows, internal stairs and protected
walkable roofs. Roofs/decks/treads use solid visible geometry matching collision;
Factory art remains on industrial props, guards, beams and the shed.

All seed variations preserve half-turn symmetry between Alpha and Omega,
including spawn positions/facing, collision, materials and block-cell seams.
Destructible surfaces keep the existing project Wood/Concrete catalog. Reserved
circulation and native overlap/access checks bound generation before commit.

`ERMapGenerate 7919 8 1` regenerates with a reproducible seed; the legacy room-count
argument is ignored by arena templates. `?MapSeed=7919` also works at level
travel. Add `-ERBlockDebug` to show block diagnostics. The older laboratories
remain available. `UERMapTemplate` assets allow further balanced templates.

See [MAP_TEMPLATES.md](docs/MAP_TEMPLATES.md) for geometry, asset provenance,
migration, symmetry rules and rebuilding. Run
`python3 scripts/test-map-template.py --render --network` for actual movement
through both bases, stairs, shots, damage, seed reset and initial/late remotes.


Movement-only bots (15 October): select **Solo movimento** in session setup.
They alternate ordinary jumps, crouch walking, a visible prone pause/crawl and
standing recovery on a staggered22-second cycle. They also try nearby reachable
obstacles using the player's normal vault/climb component. Local searches are
bounded and check clear approach/support; no baked NavMesh is required. The
existing Movement Laboratory makes different obstacle heights easy to inspect.
Bots remain server-driven and never fire in this mode; their movement/postures
are visible to other LAN players. Stationary and combat bot modes are unchanged.


Dust Mirror is available as a procedural desert town in the local/LAN menu.
Its reference-plan streets connect northern A/B courts, a southern spawn, Mid,
B tunnels and the long route. Seeds reshape buildings, alley widths and court
spacing while preserving the route graph and Short A crossing above CT.
The compact town is about13% smaller per axis. Arched timber gates swing in
opposite directions, hiding the frontal sightline while leaving a narrow
diagonal standing passage. Urban
art uses the private MOUT_Arabian palette and owned architecture; destructible
cover retains the project materials. See
[reference templates](docs/MAP_TEMPLATES.md) for source provenance and checks.
