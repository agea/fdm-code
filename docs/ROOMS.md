# Rooms — 26 October 2026

`DA_Rooms` / `L_Rooms` generates enclosed 3v3/4v4 maps with 6, 8 or 10
rooms, including two bases. Nominal sides are now 22, 20 and 18m, with independent
proportion variation of up to 2m. More rooms still means smaller individual
rooms. Shapes, floor elevations and lane offsets vary deterministically by seed.

The two lanes wind through real right-angle turning chambers. A direct lower
spine crosses below an elevated transverse corridor at 7.2m. The upper route
opens onto 4m galleries in selected rooms; two smooth 2m switchback rises and
supported perimeter landings connect those portals to the lower floor. Separate
stacked openings retain the wall between levels. There is no stair at the
crossing itself: level changes use the connected room galleries.

Corridors retain their 3.6m width, 3.4m headroom and four 55cm clipped corners.
Closed 20cm shell prisms provide synchronous triangle collision. There is no
arena-wide floor outside the rooms and links. Every room and junction has at
least two connections. Half-turn pairing preserves elevations and shape;
each base has eight clear supported starts. Balance still needs playtesting.

Timber cover is sparse. Runs shorter than 10m have no boards. Longer runs get
alternating 2.4m boards at 5m longitudinal intervals, with clear side passages.
Room panels are independent boards rather than L pockets, at most two per room
and at least 5m apart. Any room board obstructing a stair or portal route is
omitted with its paired copy. Long corridors with at least three screens block
both direct and diagonal standing sightlines; bends also interrupt visibility.
All boards use the existing calibrated 2cm wood cells, floor anchoring, damage
and replicated snapshots. The global 26 October HP reduction applies.

Production samples the merged **Industrial** or **Desert** shared theme,
independently of geometry. The recipe-local three-theme array is a legacy
fallback. Roof groups are hidden from the minimap; lighting remains local to
rooms and links. Recon uses lower playable surfaces and real sight restrictions.

Build with `scripts/build-editor.command`. Apply `scripts/setup_map_variety.py`
through Unreal Python for the current migration (Rooms revision4). Its verified
backup and receipt live under `Saved/MapVarietyBackup/2026-10-26`; provenance
is copied to private `SourceAssets/MapTemplates/map-variety.json`.

```sh
python3 scripts/test-rooms.py
python3 scripts/test-rooms.py --render --offscreen --network
```

Native tests cover 516 layouts and six physical seeds: connectivity, sizing,
height separation, half-turn pairing, 5m spacing, deterministic art, all route
capsule sweeps and continuous floor support. Physical sight tests cover 25 rays
per sufficiently furnished corridor. The real pawn probe walks a sloping
corridor, a switchback and an upper portal approach, then damages wood and
regenerates. LAN mode compares initial and late clients. Metal produces 18 views
across seeds42/73/13. Logs and reports live under `Saved/Testing/rooms-*`.

Direct launch: `/Game/EclipseRecoil/Maps/L_Rooms?MapSeed=42`.
