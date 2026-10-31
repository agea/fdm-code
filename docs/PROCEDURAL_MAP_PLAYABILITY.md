# Procedural map playability

Reads a generated map as a tactical graph and says why it is weak. **Report-only:** no metric
rejects, retries or changes a layout yet, and the default mode is off.

**Code:** `ERMapPlayability.h/.cpp`, `ERMapPlayabilityTests.cpp`, `AERProceduralMap::GenerateLogicalWorld`
and `OnGenerationFinished`.

## Use

```sh
python3 scripts/test-map-playability.py
python3 scripts/test-map-playability.py --batch --reports
python3 scripts/preview-map-sizing.py --team-size 4 --seeds 42 --playability
```

In any non-Shipping run, `-ERMapPlayability=ReportOnly` analyses each map the authority generates.
Without that flag no analysis code runs, and in Shipping `ERPlayability::Mode()` is always `Off`.
Rollback is removing the flag. Output lands in `Saved/Testing/MapPlayability/`: one JSON per stage,
`<Archetype>-<N>v<N>-seed<Seed>-<stage>.json`, plus batch summaries `layout-summary.csv` and
`physical-summary.csv`. Each analysis logs one `ER_MAP_PLAYABILITY_LIVE` line.
[Example report](references/map-playability-report.example.json).

The overlay draws the graph in blue, chokes in magenta, spawn sight lines dashed in red.

## Two stages

| Stage | When | Works on | Decides |
| --- | --- | --- | --- |
| `layout` | After `ERProcedural::Build`, before the cell world commits | `FERProceduralLayout`, panels included | Topology, sight against known solids |
| `physical` | `OnGenerationFinished`, collision complete | The real world | Capsule sweep of every walking edge, real sight and shot traces |

The physical stage marks edges confirmed or blocked, then reevaluates the topology. Traces never run
at the layout stage: no geometry exists yet. Only the authority analyses, and clients are unaffected.

The layout stage knows boxes, cell panels with their holes, and Rooms shell faces. Hollow house
skins, mesh colliders and graded terrain are counted as `unknown_solids`. They can only hide more,
and known solids shrink by 1cm for sight, because rays that graze a cell edge can pass in real
collision. So a clear sight verdict stands, but a visible pair over unknown solids stays
`not_evaluated` until the physical stage.

## Graph

| Archetype | Coverage | Source | Exits |
| --- | --- | --- | --- |
| Rooms | `exact` | `RoomNodes`, `RoomLinks`: the sealed shell allows nothing else | Evaluated |
| Fire Training | `routes` | Patrol, access, crossing, ladders; base zone is the court | Evaluated |
| Alleys | `routes` | Patrol, district and underpass routes; base zone around the starts | Evaluated |
| Central Building | `routes` | Patrol, crossing, stairs | `not_evaluated`: open yard |
| Village | `routes` | Reserved exits, passages, bypasses, stairs | `not_evaluated`: open field |

Routes that cross are cut at the crossing. Loose ends and starts join nearby routes when a standing
pawn fits against known solids. Those joins are unconfirmed until swept, and a join may never cross
a bounded base zone: bases are left through validated routes only. Stairs and ladders are not swept:
each template's pawn probes prove them.

Missing metadata reads `not_evaluated` with a reason, never `pass`. An `exact` graph that loses a
zone fails; a `routes` graph only warns, because open ground may join what metadata cannot. Fire
Training's isolated nodes are ladders onto cargo tops, dead ends far from any recorded route.

## Metrics

| Code | Reads | State |
| --- | --- | --- |
| `CONNECTED_ZONES` | Components, isolated nodes, shortest base-to-base walk, edges unconfirmed or blocked | Active |
| `BASE_EXIT_PATHS` | Edges leaving the base zone; vertex-disjoint paths between bases, so routes that merge count once; chokes every path from the starts crosses | Active |
| `INITIAL_SPAWN_LOS` | Opposing start pairs with a line, eye to head, chest and legs, both ways | Active |
| `DOMINANT_VANTAGE`, `ROUTE_EXPOSURE`, `ROUTE_DIVERSITY`, `COVER_ACCESS` | | Planned, `not_evaluated` |

Sight and shots stay separate: `visible_pairs` traces `ECC_Visibility`, `open_shot_pairs` the
Projectile channel. A blocked shot can still penetrate, so neither proves ballistic cover. Pawn
barriers stop movement but neither sight nor shots. Visual-only foliage has no collision, so it
never blocks a line here, although players cannot see through it.

Neighbouring common nodes group into one choke: both sides of a door are one. `alpha_chokes` also
counts chokes inside the base, which is how a spawn room with one door surfaces behind two gates.

No threshold is calibrated. `FERPlayabilityPolicy` carries only measuring parameters and the
proposed minimum of two independent paths. A warning is a finding to read, not a rule.

## Units and determinism

Layout and analysis use centimetres; JSON uses metres (`_m`, `position_m`) and milliseconds.
Positions are map-local. Pawn-centre samples sit at 100cm above the floor, as builders record them.

Analysis takes a const layout and draws no random numbers, so it cannot move geometry. A seed and
policy give identical report bytes without `timings_ms`. Order follows metadata, never `TMap`
iteration. Node ids are stable only within one layout: compare `source` across code versions.

## Not covered

- Graph edges carry length and width, not time: gameplay speeds are not read yet.
- Routes through open ground can underestimate alternatives. Central Building and Village keep their
  counts in the report, without a verdict.
- The hook sits after candidate selection. Enforcing a metric needs the analysis inside the
  `ERProcedural::Build` attempt loop, and its mode in the replicated descriptor: every peer repeats
  those attempts, so a local flag would split the map. Stay report-only until then.
- PCGEx plays no part here.

## Verify

`test-map-playability.py` runs `EclipseRecoil.Maps.Playability.*` and proves report-only neutrality:
identical geometry digests with and without the flag, on the same seeds. `--batch` covers 20 seeds
per template and size.
