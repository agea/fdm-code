# Rooms — 25 October 2026

`DA_Rooms` / `L_Rooms` is an original enclosed template for 3v3 and 4v4
deathmatch. It joins the production catalog as **Rooms · Indoor**. Team capacity
and the shared movement, weapon and damage tuning are unchanged.

Each seed creates two bases, two connected side lanes and a transverse link
between the lanes. Larger graphs can add another symmetric pair of transverse
links. There are 6, 8 or 10 rooms, including the bases; the nominal room side
decreases from 16m to 13m to 11m as their number increases. Room proportions,
rectangular or clipped-corner plans and floor elevations vary by seed. Junctions
in the middle of corridors are tracked separately from the room count.

All gameplay geometry is paired by a 180-degree rotation, preserving height.
The graph has no room with a single exit. Eight starts in each base support
the intended teams and the existing safe overflow spawn rules. Geometric
symmetry does not establish competitive balance; that still needs playtesting.

Corridors have a 3.6m-wide, 3.4m-high rectangular section with four 55cm corner
cuts. These are actual octagonal collision surfaces, including the lower cuts.
Their floors and ceilings slope continuously between elevations 0, 1.6m and
3.2m; they contain no steps. The visible shell is a set of closed 20cm prisms
with synchronous triangle collision. Door openings are omitted from the room
walls and corridor ends stay open, so there is no second wall behind a door.
There is no arena-wide floor or ceiling outside these enclosed volumes.

One paired set of rooms has a 3.2m gallery reached by two smooth switchback
ramps and a landing. The gallery leaves the ground crossing open. The fixed
envelope and ramps keep the network stable. Destructible timber panels replace
crates: each is 2.4m high and one calibrated 2cm wood depth. Ordinary rooms have
four open L pockets (eight boards); bases and gallery rooms have two pockets,
leaving spawns and ramps clear. Corridors contain three alternating baffles,
or four in the self-symmetric cross-link. Overlapping projections block direct
standing sightlines, with side gaps of 1.5–1.9m and a traversable zigzag route.
Layouts contain 60–110 boards, paired by half-turn. They use the existing cell
damage, floor anchoring and snapshot system; material HP calibration is unchanged.

One complete material theme is sampled per seed from the existing **Industrial**,
**Desert** and **Warehouse** palettes. This cosmetic stream is independent of
geometry. `setup_rooms.py` copies material references into the recipe, creates
only absent assets through Unreal and checks hashes of existing recipes, maps
and the shared destruction catalog. It imports no models or textures. The
private `SourceAssets/Rooms/recipe.json` records provenance and protected hashes.

Lighting is local to the rooms and corridors. Roof groups are excluded from
the minimap capture. Fog exploration samples the lower playable floor rather
than the ceiling. Each recon drone stays within its team's enclosed base;
player and drone contacts retain real line-of-sight restrictions.

Build with `scripts/build-editor.command`, then run Unreal Python:

```sh
"${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/RespawnAndRage.uproject" -run=pythonscript -script="$PWD/scripts/setup_rooms.py" \
  -nullrhi -nosound -unattended
# Existing recipes: migrate timber screens with a revision2 backup.
"${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/RespawnAndRage.uproject" -run=pythonscript -script="$PWD/scripts/setup_rooms_screens.py" \
  -nullrhi -nosound -unattended
python3 scripts/test-rooms.py
python3 scripts/test-rooms.py --game --network
python3 scripts/test-rooms.py --render --offscreen
```

Native tests cover 516 layout seeds and six physical seeds, graph connectivity,
inverse room sizing, half-turn symmetry, deterministic art, standing capsule
clearance, continuous floor support and closed corridor walls/roofs/corner cuts.
Physical tests also check 25 eye-height rays per corridor and sweep the actual
zigzag routes. The game probe walks a real pawn along an inclined corridor and the upper
switchback, damages wooden cover and regenerates the map. LAN mode compares
initial and late-client collision/state digests before and after these changes.
The Metal gallery captures 18 plan, base, corridor, upper-floor, room-panel and
slalom views for seeds
42, 73 and 13: all three room counts and all three themes. Logs and receipts
live in `Saved/Testing/rooms-*`.

Direct launch: `/Game/EclipseRecoil/Maps/L_Rooms?MapSeed=42`.

Validation on this Mac (25 October2026): Editor build passed; all516 layout
seeds and six physical seeds passed, covering6/8/10 rooms. The20 destruction
regression tests, three menu tests and three cook-coverage unit tests passed.
Metal captures cover all three themes and counts. LAN validation uses separate
host, initial-client and late-client processes on this Mac; it is not evidence
of a distinct-device test or a competitive3v3/4v4 playtest.

Shared theme follow-up: all four production templates now reference
`DA_MapThemes`, installed by `setup_map_themes.py`. Its final cosmetic pass
supersedes the recipe-local finishes while preserving the Rooms geometry and
legacy recipe fallback. See `MAP_ARCHETYPES.md` for selection and validation.

Timber screen follow-up: `setup_rooms_screens.py` saves recipe revision3 and
records protected hashes in `SourceAssets/Rooms/timber-screens.json`. The prior
recipe is backed up under `Saved/RoomsScreensBackup/before-revision2`.
The updated Editor build, 516 layout seeds, six physical seeds and shared-theme
regression passed. The combined Metal/LAN probe passed actual zigzag and ramp
walking, wood damage, regeneration and initial/late-client agreement.
