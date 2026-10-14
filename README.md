# Eclipse Recoil — Unreal local prototype

Unreal Engine **5.8.3**, macOS arm64. Existing First Person project with a native
server-authoritative Team Deathmatch module. Packaging and distribution are deferred.

Code is public in **agea/fdm-code**; licensed content is held in three private
submodules for characters, weapons and maps. A public code clone needs asset
access before it can run. See [asset setup and editing](docs/ASSET_SUBMODULES.md).

```sh
git clone https://github.com/agea/fdm-code.git
cd fdm-code
git lfs install
python3 scripts/setup-assets.py --init
python3 scripts/setup-assets.py --check
```

Open `fdm.uproject` with Unreal 5.8.3, or run `scripts/open-editor.command`.
Run `scripts/play-local.command` to play in a separate window using the Editor runtime.
Use **Host** in the menu, then run the same launcher again and **Join**
`127.0.0.1:7777`. A match begins once both teams have a player.

Controls: WASD, mouse aim / left-click fire, Space jump / climb / respawn, Shift walk, double-tap W sprint (5s, then 10s recovery), hold Ctrl or C crouch,
double-tap Ctrl or C prone (press again to rise),
R reload, 1–5 or mouse wheel select weapon, right-click zoom, Tab scoreboard,
Esc menu. After dying, wait three seconds and press
fire or jump again. The respawn press is consumed; release before firing.

The Fab low-poly arsenal is imported into the project: AK74, M4, Uzi, Shotgun
M4, M249, M107 and RPG7 as selectable primaries, with an M1911 sidearm and
HE/smoke/stun slots. Choose the primary in the menu; changes apply on the next
spawn. Ammo stays with each weapon. Source ZIP/FBX/TGA files are retained in
`SourceAssets/LowPolyWeapons`; runtime assets are in `Content/EclipseRecoil/Weapons/LowPoly`.
Smoke and explosion effects are provisional; smoke does not affect bot vision.

Each server-authorized shot now plays a short first-person firing animation,
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
than marking the exact screen boundary of the projectile cone.

For solo testing, choose **Allenamento contro bot** for one enemy bot, or
**Bersagli fermi · 5 Alpha / 5 Omega** for ten stationary characters, five per
team (blue/navy Alpha, orange/rust Omega). Targets do not move or shoot. Normal
**Host LAN** starts with zero bots.
You can also run `scripts/play-bots.command`, or
`scripts/play-bots.command target` for the ten stationary targets. Bots use the
same health, weapon, score and three-second respawn rules. Patrol bots share the four-player cap; human joins replace them when needed.
The ten exhibition targets do not consume human LAN slots.
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

Choose **Laboratorio movimento**, or run `scripts/play-movement.command`, to
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

To compile the Editor module after C++ changes:

```sh
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" \
  fdmEditor Mac Development -Project="$PWD/fdm.uproject" \
  -architecture=arm64 -NoHotReloadFromIDE
```

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

Choose one of five Alpha and five Omega characters in the main or Escape menu.
Alpha uses cold blue/navy clothing and Omega warm orange/rust, with neutral
equipment and natural skin. Variants combine different existing hero clothing
with owned field helmets and packs. Both preferences are saved locally; team
assignment remains automatic. In-match changes apply at the next spawn, and
existing corpses retain the appearance worn at death. First-person rendering
uses the chosen skin's extracted forearms/hands on the fitted independent FPS rig,
preserving existing weapon poses and grips.
Generate owned assets with Unreal Python `scripts/setup_team_skins.py` after
the base team rigs. Verify saved references with `scripts/verify_team_skins.py`,
render the ten choices and all corpse variants with
`python3 scripts/test-team-skins.py`, and test replicated selection with
`python3 scripts/test-network.py --skins --shot-feedback`.

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
Run `python3 scripts/test-network.py --accuracy` for real projectile cone,
posture/movement, burst recovery and remote owner replication checks.
`-ERAccuracyAudit` logs graphical reticle ammunition, spread and radius.


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


Cleanup has been executed: obsolete Mercenary and unreachable Paragon
packages are removed. The working project is about 4.1 GiB, Content 2.7 GiB.
Private backups are in /Users/agea/git/fdm/_backups/2026-10-14-cleanup;
local LFS cache is /Users/agea/git/fdm/_cache/lfs via local lfs.storage.
Both remain on this machine. See docs/CLEANUP_RESULT.json and
docs/REPOSITORY_CLEANUP.md for evidence, recovery and cache portability.

Sprint and posture: double-tap W within 300ms and hold the second press to sprint
at 18km/h. Releasing W, walking, crouching, leaving the ground or climbing ends
sprint and starts 10s of recovery. A fresh double-tap is required after recovery.
Crouching/rising takes 0.4s; going prone/rising takes about 1.1s. Movement slows
during transitions; prone crawling is 55cm/s. Standing waits for ceiling clearance.
The prone world pose is procedural, layered over the existing team skeletons.
`-ERPostureGallery` captures FPS, Alpha and Omega posture images in Saved/Testing.
