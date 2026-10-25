# Circular vitals HUD

Owner-supplied ZIP and original PNGs/manifest are preserved in the private maps
repository at SourceAssets/HUDVitals. scripts/setup_hud_vitals.py imports three
full icons and 48 sector overlays into /Game/EclipseRecoil/Art/HUDVitals unchanged.
Use Unreal Editor's -ExecutePythonScript option to run the importer.

ERVitalsHUD uses hard references for cooking and GC retention. Health, armor and
stamina read existing gameplay values, left to right, at 66 reference units with
4-unit gaps (206 x 66 overall, matching the previous bars). Shared viewport scale,
40-unit corner inset and prone prompt above the group remain active.

Each full image is followed by sectors N..15 at the same coordinates and size,
where N = round(clamp(fraction, 0, 1) * 16). The center emblem stays visible at zero.
Display changes in authored 6.25% steps. Textures use straight alpha, sRGB,
uncompressed UI storage, SimpleAverage mipmaps, explicit trilinear filtering and
clamped UV edges. Original 648 x 648 PNGs and on-screen geometry stay unchanged.

Validate with scripts/build-editor.command and scripts/test-hud.py, including
--render for live resized captures.

Recon stock uses five supplied drone icons below the minimap, with 90% opacity
for the replicated remaining stock and 33% for destroyed drones. The original
PNG is retained in private maps SourceAssets/HUDRecon; setup_hud_recon.py imports
it with the same mip/filter settings as weapon icons. UV cropping preserves the
source and aspect ratio. Icons are 24 reference units high with 6-unit gaps.
The triangle uses the opposing team color and a fixed world bearing captured
from the server-authored pawn facing at respawn (initial-only replication). It
starts up at respawn and rotates with the heading-up map as the camera turns,
without retargeting based on player movement or enemy positions. Its swept extent
ends at radius + 23, while the icon row starts at radius + 34, leaving 11 scaled
reference units of clearance at every yaw. No recon authority or launch rules change.

Drone stock icons use the shared team tint (Alpha blue / Omega orange), preserving
90% / 33% opacity. The minimap uses the same 30 * uniform viewport scale inset
from both the top and right, including clearance for the orbiting bearing marker.
FPS/resolution uses Teko9
(previously 11), centered at the top with a 2-unit inset above the unchanged scores.

A second triangle mirrors the enemy bearing across the minimap center and uses
the player team color to mark the home spawn side. Both share size, opacity,
fixed spawn orientation and the existing clearance above the drone row.
