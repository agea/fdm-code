# Minimap and reconnaissance drones

The circular minimap sits at the top right, below the FPS readout. It follows
the local player's heading: forward is always up, for either team. Terrain,
fog, contacts, heading ticks and the north reference rotate together every
rendered frame using the controller's yaw. The owner's direction tick stays
up. The view follows the owner's current position and shows a fixed 40m radius
(80m diameter), clipping larger maps instead of shrinking them. Small maps can
fit within that range. Terrain UVs and the shared fog mask retain their original
world coordinates while the view moves and rotates. This works for generated and
authored maps, without waiting for a server sight update when turning.
The circular frame uses 128 segments with feathered alpha edges, a soft radial
shadow and a team-colored antialiased rim.
A trilinear-filtered, mipmapped 1024px orthographic bird view is captured once after map generation/loading,
with characters, corpses, weapons, projectiles and drones excluded. It remains
static until a new generation/round. A 64x64 shared team fog mask reveals only
explored cells; the unexplored area stays opaque. Geometry and fog snapshots
carry epochs, and a client waits for matching procedural collision before capture.

The server samples sight at 4Hz, including living human players and bots.
Players have a 120-degree sight cone covering the map extent; the drone has
360-degree sight within a42m horizontal footprint. Drone altitude does not
consume that footprint. Player eyes match the authoritative combat camera. Visibility queries respect actual walls/roofs and current
breaches. Two head/torso samples can spot a partly exposed character. A seen
position appears in that character's Alpha blue or Omega orange; the owner has
a white direction tick, and the team's drone uses a small cross. Enemy contacts
use larger team-colored diamonds with a white/dark outline; allies use circles. Living allies
share their own positions and observations. Dead bodies supply no observations
or contacts. The terrain remains discovered after losing sight, while enemy
positions disappear at the next visibility update. No last-known-enemy markers.
The HUD discards stale contacts after one second without a fresh sample.

Each human controller receives an owner-only atomic snapshot with that team's
fog, currently visible contacts, stock and replacement deadline. The HUD does
not enumerate replicated characters to derive enemy locations. Late joiners
receive the existing team history and current stock, without resetting either.
Team changes discard the previous team's snapshot in the HUD. Death/respawn
preserves team exploration. Round reset and map regeneration clear it and
restore both teams' five drones. The static image intentionally does not track
later destruction. The single overhead image combines storeys and shows roofs;
it is not a selectable floor plan.

Smoke blocks all teams' player, teammate and drone reconnaissance, including
new terrain discovery. Already explored cells remain known. The server clips
each ray to the current cloud volume and integrates48 density samples through
its occupied containment cells, with growth, fade, replicated contour phase and
shot/blast cuts. Optical depth1 (about63% opacity) hides a sample; several thin
clouds accumulate. Head/torso samples must both be hidden to remove a player.
The visual texture noise is represented by its mean, so this is a deterministic
visibility model rather than a pixel-exact readback of the rendered material.
Bullet cuts are recorded on dedicated servers before skipping cosmetic shot
feedback. Smoke changes no bullets, damage, movement or bot AI target selection.

Each team has one fixed, closed, indestructible Pelican case on clear outdoor
ground near a spawn. Placement also reserves a120cm corridor around authored
bot patrol segments (with vertical overlap checked), keeping the cases out of
the Depot walking loop. The first drone takes off automatically, climbs vertically
for four seconds, joins its circular orbit over the roofs during the next four,
and completes a lap in about 39 seconds. The server and clients evaluate the
same replicated flight parameters against server time; damage remains server-only.
An ordinary authorized bullet point hit destroys the drone immediately. Generic,
melee and radial damage do not trigger it. The team's vision loses that source
immediately, but already explored terrain remains. A new drone launches after
30 real server seconds if stock remains. Five means five total, including the
initial drone; after the fifth destruction reconnaissance stays offline. The HUD
shows remaining stock including the active unit, live status, and launch countdown,
using the same Teko SemiBold typography and black text shadow as the rest of the HUD.
A hit also multicasts one cosmetic breakup through the surviving recon director.
Each rendered peer spawns five actual source fragments (hull plus four arms/
rotors), with gravity/spin, physics-only WorldStatic contacts and no player,
bullet, visibility or navigation collision. They settle after4s, expire after6s,
are capped at24 local fragments, and clear on reset, generation and level exit.
Dedicated/headless peers skip debris rendering and physics.

## Assets and rebuild

Original ZIPs, original models/textures/credits, static FBX derivatives and the
import receipt live privately in `Assets/maps/SourceAssets/ReconDrones`.
Runtime meshes/materials/Blueprints live under `/Game/EclipseRecoil/Art/Recon`.
The GameMode's ReconClass references BP_ReconDirector; it references the base
and drone Blueprints. No Downloads path is used at runtime. Original attribution:

- [S9 drone](https://skfb.ly/6SSyD), blockv_lb.
- [Pelican Case Game Ready](https://skfb.ly/6xwPJ), polbrainstorm.
- Both supplied credits identify [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).

Changes: static evaluated mesh conversion, centering/scaling (drone 85cm,
case 70cm), case lid/lock aligned closed, original diffuse colors recreated for
the drone and original case PBR textures retained. Five centered source-geometry
fragments provide the drone breakup. Credits are also retained in
asset metadata, CREDITS.txt and the game's settings menu/tooltip. This records
the supplied attribution; it is not a separate online provenance verification.

```sh
/Applications/Blender.app/Contents/MacOS/Blender --background --factory-startup --gpu-backend opengl --python scripts/bake_recon_models.py -- Assets/maps/SourceAssets/ReconDrones
python3 scripts/setup-assets.py
# Run scripts/setup_recon.py through Unreal Python in the full Editor to import/save defaults.
python3 scripts/test-recon.py --automation
python3 scripts/test-recon.py
python3 scripts/test-recon.py --render
```

The network probe runs an actual listen host, opposing owner, teammate and late
joiner. It includes sightings beyond the old30m limit, altitude-independent
drone spotting, actual HUD drawing, remote contact assertions, smoke hiding/
blast clearing/removal and falling source fragments. It checks physical cover, shared sight, current-only contacts, fog
persistence, drone 360 sight, an actual client rifle RPC and hit, a real 30-second
wait, five-loss exhaustion and reset. The last four losses use server point
hits and advance only the probe's replacement deadline to bound test duration.
`-ERReconProbe -ERReconGallery` is an explicit standalone art fixture with
frozen drone pose and camera; it is never enabled in ordinary play. Test logs,
screenshots and results are stored in Saved/Testing and recorded in TEST_RESULTS.md.
