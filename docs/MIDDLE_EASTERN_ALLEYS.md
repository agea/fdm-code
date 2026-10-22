# Middle Eastern Alleys

22 October 2026. Select **Middle Eastern Alleys · Desert** in the match menu.
The new production template is `Art/MapTemplates/DA_MiddleEasternAlleys`, with
level `Maps/L_MiddleEasternAlleys`. Central Building and Depot remain available.

The user's minimap and Dave Johnston's
[Making of Dust 2](https://www.johnsto.co.uk/design/making-dust2/) informed the
original town. Revision3 replaces its fixed lane graph with seeded districts:
2–4 main approaches, winding streets, paired market squares/courtyards and
climbable watchtowers. Each functional space has a counterpart rotated180°:
spawns, lots, slopes, plazas, towers and roof stairs. This provides geometric
parity; competitive timing/balance still needs playtesting. The existing team
deathmatch scoring applies, without added bomb or capture rules.

Revision5 adds low cover to the plaza and both districts: masonry barriers
65–90cm high and timber crates110cm high, placed in exact half-turn pairs.
The generator targets14 central and18 district obstacles, using curb-side
sites in narrow streets; it validates at least8 central and6 district pieces.
Main walking routes, roof stairs, ladders, house clearance and ramps remain
reserved. The plaza retains its broad circulation and open center.

A seeded S-shaped lower passage runs either between the team districts or
west-to-east. Its floor lies3.6m below the center, with approximately3.3m clear
headroom and paired open ramps. Base-to-base entrances have surface connectors
back to the two spawn courts. Exact rectangle unions keep the lower floor,
retaining walls and ceiling continuous around bends. The terrain mesh has real
openings for the ramps and two paired300cm brick paving sections. These
10cm horizontal cell panels use unchanged Brick HP, blast and bullet rules;
no permanent terrain or slab sits behind them. Destroying a section opens a
real fall into the lower passage. The rest of the ground remains permanent.
The2–4 access count now describes surface approaches/arches, in addition to
this lower connection. Negative construction bounds are limited to this
owned recipe, down to -4.2m; other templates retain their existing bounds.

Arena dimensions vary from80–94m by88–106m. The seed chooses2/3/4 accesses,
four winding settings, lane widths/offsets, one or two pairs of differently
sized plazas and two or four towers. Revision4 independently chooses zero,
one or two historic city walls. An unwalled center joins the main approaches
freely across a14–22m deep open band. One wall retains the original divider.
Two walls stand26–32m apart, leaving23.6–29.6m clear between their faces;
houses and towers stay outside this core. Each existing wall has the chosen
2–4 actual arch passages, with tall solid stretches preventing bypasses.
Paired bridges and small timber cover still provide local height and cover
inside the broad center. Side streets create loops within each district. Ten starts per team occupy opposite spawn courts.
Ordinary20cm steps climb to paired3.2m crossings with2.9m clear beneath.
Buildings and enclosing plaster walls close the perimeter. Revision2 uses the
original triangle shells for house collision: doors, windows, rooms, existing
stairs and roofs follow the visible models, with two-sided interior materials.
Players can enter whenever the actual opening and room fit their capsule.
Reserved exterior switchback stairs (risers at most20cm, 30cm treads) lead to
several roofs. Towers have700–900cm high observation decks, notched parapets
and real steel ladders using the existing climbing/destruction rules. The sand
ground is a synchronous collision mesh with a central low area and matching
terraces up to+1.8m at both bases, joined by ramps. House/stair foundations,
squares and tower aprons are levelled locally; bridge underpasses remain clear.
Walkable added decks and stairs use matching project box geometry. Timber
cover and tower ladders share the authoritative5cm-cell destruction system.

All three requested sand textures blend spatially on world coordinates. The
house package supplies twelve variants, six concrete and six mud, with its
original two atlases. Long parcels split into frontages up to12m instead of
stretching one house over an entire block. Paired mesh/orientation choices
use a separate seeded stream. The existing layout digest includes meshes,
surfaces, transforms and route metadata; the current network loading gate and
late-join snapshots apply. New rounds advance the seed before player respawn.

Source collection: [owner's RR assets](https://sketchfab.com/agea1/collections/rr-792d73a736504af1b1bd0e05599fc256).

| Source | Creator | Use |
| --- | --- | --- |
| [Tiled PBR material – Sand](https://sketchfab.com/3d-models/fdd6f27663c049638afbbe30f9e9c93d) | XY01 | Ground blend |
| [Rocky Sand](https://sketchfab.com/3d-models/cbc349f96250408b8f75fa1171d0cadb) | CATholic | Ground blend |
| [Loose Sand](https://sketchfab.com/3d-models/201fa0a7659c42cc865cf2ada9427bf6) | CATholic | Ground blend |
| [Desert Mud Concrete House Lowpoly](https://sketchfab.com/3d-models/bd4917fa72c84e41aa1e5299871f1b2e) | 00amza | Twelve building variants |

All four public model records specify CC BY 4.0. Original archives, public
metadata, per-file hashes, credits, prepared maps, normalized house FBXs and the
user minimap remain in private maps `SourceAssets/MiddleEasternAlleys`. Runtime
assets live under `Art/MiddleEasternAlleys`; attribution also enters menu credits.
Only the old project-owned street graph and three owned arch/door OBJ sources
are reused from the verified local retirement archive. Retired levels and the
Arabian vendor packages remain retired. No Valve binary assets are imported.

Preparation:

```sh
python3 scripts/stage_middle_eastern_assets.py
/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python scripts/bake_middle_eastern_houses.py
python3 scripts/build_middle_eastern_recipe.py
scripts/build-editor.command
```

Run `setup_middle_eastern_alleys.py` with Unreal's Python commandlet, using
absolute project/script paths. It creates absent art, the template and level,
validates 516 seeds, and preserves completed edits. It verifies unchanged
hashes for Depot, Central Building, its recipe and the shared material catalog.
Run `setup_alley_traversal.py` through the same Unreal Python commandlet for
revision2. It verifies a rollback ZIP, updates only the alley house collision,
two interior materials and the recipe, and validates 516 seeds. The existing
level and other maps/catalogs are preserved. Subsequent changes require an
explicit versioned migration.
Run `setup_alley_district.py` through Unreal for revision3: it backs up the
revision2 recipe, validates516 seeds, and changes only the own recipe. Runtime
house art, the level and shared assets remain unchanged. `MinAccesses` and
`MaxAccesses` on the town recipe bound the main approach count2–4. Run
`setup_alley_core.py` for revision4: verified revision3 rollback,516 candidate
seeds, own recipe only. `MinCentralWalls`/`MaxCentralWalls` bound the wall count
0–2. Earlier migrations preserve completed later revisions rather than downgrading
them. Run `setup_alley_passages.py` for revision5: verified revision4 rollback,
516 candidate seeds and only the own recipe saved. The new behavior is stored
in `Town.LowCoverAndUnderpass`; art and shared combat data remain unchanged.

Validation: native `EclipseRecoil.Destruction.MiddleEasternAlleys` tests sample
512 layouts and six actual physics worlds. Run `test-dust-template.py --alleys`
for real pawn gate/stair/bridge traversal, house entry/exit, roof access,
ramps in both directions and weapon damage; add `--render` for
Metal captures, `--seed 511` for an unwalled initial layout, or `--network` for initial/late clients and seed regeneration.
`--alleys --gallery` captures ten held cameras after asset compilation,
without the gameplay HUD. Its larger shadow page pool serves full-map art
captures only; normal play keeps the game's existing performance settings.
`--alleys --tower` captures the watchtower/ladder view alone (the gallery's
`Rooftop` image in revision3). Network probes wait for the late client's
confirmed damaged snapshot before regenerating; the receipt is a local,
opt-in test fixture, independent of production multiplayer behavior.
`--alleys --plans` generates seeds42/73/511/7919 and captures four square
1536×1536 orthographic overhead views without HUD, named
`Saved/Testing/MiddleEasternPassages-Aerial-<seed>.png` for revision5. Seed,
arena dimensions, layout digest, access count, winding setting, plaza count
tower count, wall count, open center depth, cover count and passage axis are recorded in the accompanying gallery-plans report.
`--alleys --passage` captures the ramp entrance, lower corridor and the road
before/after a structural blast. Real gameplay probes traverse the complete
lower route in both directions, breach one road section and fall onto the
lower floor. Client snapshots check the hole after damage and the restored
road after regeneration. Executed results and limits belong in `TEST_RESULTS.md`.

Reproducible travel URL:

```text
/Game/EclipseRecoil/Maps/L_MiddleEasternAlleys?MapSeed=42
```
