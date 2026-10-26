# Fire Training

26 October2026: the shared Warehouse palette is merged into Industrial. The
production choices are now Industrial and Desert. Both use the existing rusty
steel for columns/arches, light galvanized ladder/parapet steel, original painted
cargo and separate masonry/timber/corrugated walls. This provides visible
contrast at distance and supersedes the older Warehouse-only palette below.
Global structural HP is reduced35%; see `MAP_VARIETY.md`.

Updated25 October 2026. Select **Fire Training · Warehouse** in the match menu.
Template `Art/MapTemplates/DA_FireTraining`, level `Maps/L_FireTraining`,
archetype `FireTraining`. It joins Central Building and Middle Eastern Alleys
in the production rotation; Depot remains a reference.

The owner's specification `RR_Warehouse_Killhouse_OpenSpace_Istruzioni.md` and
the supplied `warehouse-fbx-model-free.zip` are kept privately in
`Assets/maps/SourceAssets/FireTraining`. The references (Call of Duty 4
Killhouse, Steam Workshop 526448928) guide pace and composition only: this is
an original seeded arena built from the supplied warehouse. No game assets,
outlines or spawn arrangements are copied.

## One open volume

The warehouse is always a single hall. Length, width and height come from
repeated kit modules; nothing is stretched and no storey is generated:

| Parameter | Values |
| --- | --- |
| Width | 1–4 naves of 16, 20 or 24 m (complete kits `W1600`, `W2000`, `W2400`): 16–96 m |
| Length | `BayCount` 14–36 × 4.2 m (about 59–151 m), floor 1300–7000 m², length ≤ 7 × width |
| Height | `HeightBandCount` 2–4 wall bands of 3 m (recipe accepts up to 5); ridge = eave + 2 m |

Revision 3 (owner request: wider, taller and longer halls) repeats the arched
nave side by side. Between naves there is no wall: an open column line (the
kit post on every rib boundary and band) under a steel gutter at the eave, with
hidden column volumes so routes, cargo and cover keep clear of the posts. Ends
repeat per nave. With an even nave count the start rows sit mid-nave instead
of on the central column line, and the front screen stays within ±6 m of them.
Larger halls reserve up to three empty plazas per half (10–18 m, one per
2000 m² of floor) before cargo and cover are placed; cargo grows with the floor
area and cover is spread across the whole central band. The bridge variant is
limited to single-nave halls. Wide halls are therefore flatter: the elevated
share drops to a few percent in the largest ones. `setup_fire_training_scale.py`
is the revision 3 migration (backup in `Saved/FireTrainingBackup/revision2`).

Bands only raise walls, posts and end walls (`EndBand`); the roof sits on the
last band. The 25 October route update adds two concrete-walled base courts
(12 × 11.1 m, 2.8 m walls) with two offset 1.8 m gates. Each contains an
enterable one-storey timber barracks, a 3 m steel roof and two exterior ladders.
All ten starts per team are inside the barracks. Roof parapets have openings
only at the ladder landings; the court has clear dogleg routes to both gates.

Two 2.2 m steel walkways run along both sides of the hall, starting 15 m
from either end. Their walking plane is now as high as possible below the
lighting: `WarehouseCatwalkHeight` leaves 2 m for a standing player below the
lowest lamp (and roof valleys in multi-nave halls), rounded down to 5 cm.
For 6/9/12 m eaves, the decks sit at about 4/7/10 m. Flanks start 1.2 m from
the outer wall, beyond the original 85 cm knee braces. Base roofs stay at 3 m. A central cross-map bridge joins the high walkways.
Multi-nave halls with
an even number of bays use two crossbars at ±2.1 m, between column rows;
other layouts use one crossbar at the middle. Every route is mandatory,
including the deterministic fallback. Four ground ladders reach the walkway
network through notched end rails.
These ladders retain their full-height handles but begin the existing horizontal
exit at deck+5 cm, avoiding the ordinary 1.1 m climb above a parapet and keeping
the player's head below the lighting. Other ladder exits retain their behavior.
The existing original industrial steel barrier supplies the fixed
1.27 m parapets, without a concrete plinth on the decks. Openings occur only
at ladders and walkway junctions. This supersedes the earlier local optional
catwalk, stair access, timber upper screens and deck-room/lookout variants.
Ground timber rooms/corridors, steel containers/stacks with ladders, low cover
and empty plazas remain. Each half is sampled once and rotated 180°.

`setup_fire_training_routes.py` backs up the current recipe, validates 516
candidate seeds and binds only the parapet reference plus a migration marker;
size tuning and the shared theme library remain intact. The current saved
recipe advanced from revision 4 (shared themes) to revision 5 for the bases;
route marker2 advances it to revision6 for the high walkways.

Budgets, measured on a 50 cm raster of the union of projections (overlaps
count once): walkable surfaces above the ground ≤ 30% (target 20%), every
horizontal closure ≤ 40%, and the largest region open to the roof ≥ 45%.
Narrow walkways (≤ 3 m wide) do not split that region because the volume above
them stays joined; wide decks do. `ContinuousUpperFloors` exists only to be
rejected. Every elevated area has at least two independent ladder accesses. Local
base/cargo decks keep 2.5 m below the arched roof; the high walkways use 2 m
below their limiting roof/lamp surface. Physical standing and ladder-exit
sweeps check the actual kit geometry. Most direct start-to-start lines must
meet a solid ≥ 1.5 m (front timber screens, containers, rooms).

Revision 2 (same day, owner request: the hall was too small) doubled the
length range at the same widths. Interior features scale with it: the catwalk
covers up to 60% of its half (bridge variant 17–30 m), up to three room
blocks repeat along the east wall with 6–11 m passages, and halls of 70 m or
more get one or two extra containers per half. `setup_fire_training_length.py`
is the explicit migration: it backs up revision 1 to
`Saved/FireTrainingBackup/revision1`, validates 516 seeds and changes only the
recipe size fields and the revision.

Generation is server-side and deterministic. `ERProcedural::Build` retries up
to seven seeded attempts and then uses a verified reference layout (16 m,
22 bays, two bands, fortified bases, connected catwalks + corridor, ground rooms and cargo); every peer
repeats the same attempts from the replicated seed. The attempt index is part
of the layout digest.

## Kit and materials

`scripts/bake_fire_training_kit.py` (Blender, source SHA-256 checked) splits
the 439 source parts by measured signature from the bay between ribs 20.9 m
and 25.1 m. The 4.2 m pitch is rib-centre to rib-centre; the irregular first
source interval is not used. Modules (metres, origin on the outer wall face,
bay centre, wall-band base or eave):

| Module | Content |
| --- | --- |
| `SideWallBay` | Corrugated sheet + girts, 3.8 × 3 m, repeated per bay and band |
| `Post` | One 20 cm post; three per inner rib boundary, two at the ends |
| `SideWallLamps` | Window light strips, top band only, visual |
| `RoofBay_W*` | Two arched skins + purlins between ribs |
| `RoofArch_W*` | Rib + knee brackets, once per boundary (N+1) |
| `RoofLamps_W*` | Hanging lamps, alternate bays, visual |
| `EndGround_W*` / `EndBand_W*` / `EndCrown_W*` | End wall: ground band, plain band, gable |
| `EndPortal` | Closed portal leaves and frame, visual |

Wider kits split the cross-section at the apex (roof, rib, gable) or beside
the portal (end bands) and bridge the cut with a straight insert; UVs are
extrapolated across the insert. The west side wall is turned 180° for the east
side. End sheets gained 1 cm of depth so their bounds are not zero; portal and
gable bases were raised 2.4 cm onto the floor.
`scripts/setup_fire_training.py` imports at unit scale (checking the measured
bounds), makes sheets, posts, ribs and roof collide exactly as drawn
(two-sided complex collision) and adds hidden backstops behind the side and
end walls; the hidden ceiling closes the top. Roof groups are tagged
`ERTemplateRoof`: the minimap capture hides them, recon flies 50 cm above the
eave inside the arch, and the cases search for ground under the eave.

Materials: `M_ER_WH_Metal` uses the source sheet normal map with a lighter
grey and partial metalness for the game's no-reflection profile;
`M_ER_WH_WetConcrete` projects the wet concrete colour/normal/roughness in
4 m world tiles; `M_ER_WH_LampEmissive` for lamp art. Timber, steel, deck and
container finishes reuse the Central Building palette. Unshadowed point lights
are added under each roof-lamp bay on clients only.

The shared **Warehouse** theme now separates steel by use, so different
structures retain distinct tones when their fine texture detail fades at distance:

| Element | Installed finish reused |
| --- | --- |
| Warehouse wall sheets and end portals | Original corrugated `M_ER_WH_Metal`, light gray |
| Court walls and internal masonry | Iron Plates2K, medium blue-gray, 3 m tiles |
| Posts and arched frames | Existing dark Factory beam steel |
| Ladders and parapets | Steel texture with irregularities, light galvanized gray |
| Steel platforms and roof decks | Iron Plates2K, darker charcoal, 2 m tiles |
| Warehouse roof | Separate dark Factory steel instance |
| Reinforced cargo | Original Factory paint palette |

`setup_warehouse_steel.py` creates two owned parent derivatives and four material
instances in `Art/FireTraining/WarehouseTheme` and updates only Warehouse in
`DA_MapThemes` revision2. Its verified rollback is in `Saved/WarehouseSteelBackup`.
Original texture pixels, parent materials, all four template recipes and damage
catalog are preserved. Owned derived parents use partial metalness and a
roughness floor for the game's low-reflection lighting profile. Industrial and
Desert retain their existing assignments. Geometry, physical material IDs, HP,
penetration, support and collision are unchanged. The optional theme steel
roles also apply wherever Warehouse is selected on the other templates.

Destruction: timber walls, screens, crates, containers (Strength 8) and
ladders use the shared 5 cm cells, damage profiles and replication. The
envelope, steel decks, columns and parapets are fixed. Court walls use the
existing concrete cells; barracks walls use timber cells.

## Provenance and licence

The archive carries no author or licence. Provenance, hashes and credits are
in `SourceAssets/FireTraining/{provenance.json,CREDITS.txt}` and the menu
credits. Confirm the licence before any public distribution.

## Regeneration and validation

```sh
python3 scripts/stage_fire_training_assets.py
/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python scripts/bake_fire_training_kit.py
scripts/build-editor.command
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/RespawnAndRage.uproject" -run=pythonscript -script="$PWD/scripts/setup_fire_training.py" \
  -nullrhi -nosound -unattended -NoSplash
python3 scripts/test-fire-training.py           # native layout + physics
python3 scripts/test-fire-training.py --render  # Metal captures in Saved/Testing
python3 scripts/test-fire-training.py --network # initial/late clients and regeneration
```

The setup validates 516 seeds before saving the template and refuses a
changed kit on an existing template (write a versioned migration instead).
Native `FireTrainingLayout` builds 516 greybox seeds (mandatory bases/network, budgets, symmetry,
accesses, headroom, determinism, fallback, rejection of mixed kits and
continuous floors). `FireTrainingPhysics` generates eight seeds from the real
asset: unit-scale kit instances, closed envelope for pawns and bullets, roof
collision, raster check for raised coverage, ladders (rungs, climbing capsule,
landing floor), protected courts, continuous catwalk/roof floors, parapet
collision/height, spawns, routes, patrol loop and destructible timber.

Reproducible travel URL:

```text
/Game/EclipseRecoil/Maps/L_FireTraining?MapSeed=42
```

Bot behaviour inside the rooms, packaged/cooked builds and performance
measurements require separate validation. Executed local checks are recorded in
`TEST_RESULTS.md`. Geometric
symmetry is not a claim of competitive balance; it needs playtesting.
