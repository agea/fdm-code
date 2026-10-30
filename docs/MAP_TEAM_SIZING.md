# Expected team size and map area

The host sets `Map.ExpectedTeamSize` when creating the server (default2,
supported1..16). It is visible read-only in the lobby and cannot be changed by
an RPC. There is no sizing-related READY reset. The saved draft may be changed
for the next server; the current session snapshot remains fixed through lobby,
travel and subsequent rounds. Offline/direct launches use the same area policy.
Human capacity, actual attendance, bot counts and team admission are independent.

Every production template uses **500m² per expected player, including both
teams**:

`target plan area = expected team size ×2 ×500m² = team size ×1000m²`.

The allowed deviation is **±10% of area**, not±10% on each axis. Upper floors,
rooftops and lower passages are not added to the horizontal arena area. For
Rooms, area is the enclosing arena rectangle, including gaps between its sealed
volumes. Width×length and actual area are reported in all generated plans.

| Teams | Target area | Accepted plan area |
| --- | ---: | ---: |
|1vs1 |1000m² |900..1100m² |
|2vs2 |2000m² |1800..2200m² |
|3vs3 |3000m² |2700..3300m² |
|4vs4 |4000m² |3600..4400m² |
|8vs8 |8000m² |7200..8800m² |
|16vs16 |16000m² |14400..17600m² |

There is no saturation at8/team. Changing a template's aspect ratio never
bypasses its area budget. Both the native builder and tests reject out-of-budget
layouts atomically rather than silently producing an oversized fallback.

## Native generation

`FERProceduralConfig.ExpectedTeamSize` is part of protocol18's replicated cell
descriptor. Zero retains authored/reference studies; production hosting copies
the positive creation-time server rule before spawn preparation. Geometry digest
and reconstruction include the expected size. Clients use this descriptor and
the server snapshot rather than their local host preferences.

`ERMapSizing.cpp` configures transient recipe copies. It sizes structural counts,
room spacing and valid arena bounds; it never uniformly scales characters,
portals, stairs, ladders, acquired kit meshes, material thickness or5cm cells.
Original saved recipes and art remain untouched. All five native builders keep
half-turn gameplay pairing and existing overlap/spawn/complete-route checks.
  Larger alley districts cap dense construction at20paired houses/6m height,
  leaving other parcels as open yards to retain cell-memory budgets. Street
  terrain is checked across the standing capsule footprint before acceptance.
Composition gets at most16 deterministic attempts (256for the tightly constrained
alley street/terrain graph), separate from the unchanged
requested seed's cosmetic stream; every peer chooses the same accepted layout.

- **Central Building:** choose a near-square arena around the target area. Compact plans
  omit secondary buildings/core displacement to preserve flanks. Very small
  plans omit cargo where a real approach cannot fit. The main stairs and roof
  retain physical dimensions; larger budgets admit more floors and side buildings.
- **Alleys:** choose paired streets and modular houses inside the area rectangle.
  Compact markets/courts shrink before the street/parcel graph is partitioned.
  The flat spawn court ends inside the upper terrace. Compact2vs2 omits towers
  but retains house/market stairs and the lower passage.1vs1 omits the market
  bridge/lower passage and optional towers. Ramp turns/breach openings follow
  their actual mouths; loose-cover quotas prioritize unobstructed routes.
- **Fire Training:** enumerate complete unscaled width kits, nave counts and
  4.2m bays. Only options whose outside perimeter (including walls) satisfies
  the area budget are admitted, including the bounded fallback. Through3vs3 use exactly one nave and grow lengthwise (aspect≤10);
  from4vs4 allow multiple naves with aspect≤3. Every outside perimeter,
  including the reference fallback, has length at least1.2×width. Existing elevated/open-area ratios
  and local catwalks remain enforced.
- **Rooms:** sample complete graphs with four, six, eight or ten rooms
  (including the two bases), rather than fixing the count at each team size.
  Four-room plans place all four rooms at the corners of their rectangle,
  with Alpha northwest and Omega southeast. Bases keep two exits; the
  intermediate halls have two outside connections and may also share a direct
  central passage. Both outside attack routes remain available.
  The two non-spawn rooms become large rectangular halls extending into the
  centre and toward the outer boundary. Seeded variants include large
  rectangles, smaller rectangles and convex outlines with asymmetric corner
  cuts. Actual flat-floor coverage is at least55% (60%for the large rectangular
  fallback), and each hall has at least twice a base's footprint. Keep2m between halls and6m of
  approach corridor at the adjacent base. Off-centre wall portals preserve
  straight, full-size corridors. Separate openings on the same wall are
  carved independently, with complete clipped corridor mouths. The optional
  hall-to-hall link is highlighted in plans, alongside useful-floor area and
  hall dimensions.
  There are no unused north/south stubs or extra turning chambers. No single
  corridor may disconnect the bases. All starts and paths to room centres
  are reserved before furnishing. Compact eight-room2vs2 plans also stay at ground level;
  other graphs retain galleries and the upper crossing. Room dimensions, lane
  spacing and offsets vary together within the same area budget. Upper rooms
  keep enough width for their full-size switchbacks; portals never shrink.
  Sized corridor links have slope at most1:2; larger even-row profiles reserve
  at least13m spacing around intermediate junctions before sampling geometry.
- **Village:** sample one or more central blocks, their widths/depths and
  storeys before fitting the enclosing rectangle to their actual footprint.
  At2vs2 this admits a short wide single block or a longer pair.3vs3 requires
  two or three blocks; the rejected single-block61×54m variant is excluded.
  Minimum block counts grow with the expected area (up to five). Paired4.8×2.4m
  L shelters,2.4m tall, also fill available approaches/flanks from2vs2 upward.
  Their quota grows with actual area, with round-robin depth bands to distribute
  placements. Existing construction, stairs, spawn starts, ground routes and
  shelter entrances are reserved before adding destructible timber/metal panels.
  Raising the storey count does not satisfy the ground-cover quota. Keep half-turn symmetry, two dogleg passages per
  block, spawn stairs, side cover and standing flank bypasses.1vs1 keeps its
  compact one-block profile without optional lookouts/shacks.

## Reproducible plans and verification

```sh
python3 scripts/preview-map-sizing.py --team-size 2 --seeds 42 73 7919 --verify-seeds 512
python3 scripts/preview-map-sizing.py --team-size 4 --seeds 42 --verify-seeds 32
python3 scripts/test-lobby.py --industrial --map-sizing
```

The compiled local Unreal Editor and Pillow are required; `UE_ROOT` can select
the engine. Native `TeamSizing` checks density, geometry, deterministic repeat
witnesses, paired starts and source-recipe isolation across requested seeds,
0..N-1 and signed edge cases. Sweeps of32or more seeds also require several
room/block counts and at least four different arena proportions at2vs2/3vs3. `TeamSizingPhysics` builds real calibrated cell
collision at42/73/7919, using the actual68×185cm standing capsule. The independent
`EclipseRecoil.Maps.DensityBudgets` checks all1..16 team sizes across11 seeds
for every production template.

`Saved/Testing/MapPlans/<N>v<N>/` contains dimensioned PNG/SVG plans, a common-scale
overview and an HTML gallery. Every card includes dimensions in metres, actual
area, target area and percentage deviation, plus a5m grid and10m ruler. The
source JSON additionally retains height. Failed native validation prevents stale
output from being delivered. `--reuse` requires a matching verified JSON hash
and only redraws the last successful export; `--output` changes the destination.

The drawings project vertical construction with roofs omitted: pale outlines
show upper construction; green routes show passages, purple dashes lower routes
and teal dashes roof routes. They are technical plans, not photographic renders
or navigation meshes. Small tread/ladder paths are omitted from the overview.

These checks establish geometry, density and collision behavior. Competitive
balance still requires subjective playtesting at the current movement speeds.
