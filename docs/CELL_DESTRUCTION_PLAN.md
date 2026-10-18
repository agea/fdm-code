# Respawn & Rage — plan: from 50x20x25 blocks to 5cm cells

Status: **5cm cells everywhere** (18 October, §9): lab, procedural/multi-floor, Killhouse and Dust.
The 50x20x25 block path is removed and collapse is ported to cells. §8 is the 17 October lab prototype.
Source: the user's `RR_DANNI_MATERIALI_BLOCCHI_GRANATE.md`, sections 2 and 7, plus the
[damage preset](PROCEDURAL_DESTRUCTION.md#damage-preset-17-october) that is already applied
provisionally per block. Numbers marked *measured* come from runs on this machine (M2 Pro,
UE 5.8.3, `nullrhi`). Numbers marked *model* come from `scripts/model-cell-destruction.py`
(pure Python, spec formulas, no Unreal). Numbers marked *estimate* are engineering guesses
that the prototype must confirm.

## 1. What the current solution really costs

### 1.1 Today's maps (measured, `test-destruction-stress.py --samples 8 --label preset17oct`)

10 rooms, seed 42, chunk 400, 9,907 blocks, binary with the 17 October preset. The run
passed (2,544 CSV rows). It matches the 14 October receipt within noise.

| Operation | Median | p95 | Notes |
|---|---:|---:|---|
| Generation | 48.6 ms | 50.9 ms | ISM add 37.9 ms ≈ 3.8 µs per instance including Chaos |
| Single removal | 0.016 ms | 0.034 ms | one ISM group, about 108 instances |
| Batch, 256 unique IDs | 1.77 ms | 1.83 ms | 77 groups |
| Linear explosion r250 | 0.43 ms | 0.46 ms | 132 candidates, 117 removed |
| Clear all walls | 14.1 ms | 14.8 ms | 9,518 removals |

Tracked memory 4.06 MiB, about **410 B per block**: logical records 1.79 MB (181 B), mappings
0.66 MB (67 B), ISM resources 1.47 MB (148 B), support graph 0.32 MB (33 B). 9,949 Chaos
bodies, 92 components. Network: one absolute page per 128 records (4 B HP + 1 B state), so
78 pages / ~50 KB for this map and 137 pages / ~88 KB for the 3-floor 17,427-block map,
whatever the damage. Logged map sizes: Killhouse 3,236–5,220 blocks, procedural 5.4k–11.5k
(1–2 floors), 17,427 (3 floors), Dust Mirror 168–288 (mostly fixed art).

### 1.2 One ISM instance and one Chaos body per cell (measured, `scripts/measure-ism-cell-scaling.py`)

The lab room was regenerated in a real `-game` world with finer blocks (thickness stays 20 cm,
one instance per block, real Chaos bodies, three repetitions):

| Block size (cm) | Instances | Regenerate | RSS delta | Single removal (median) |
|---|---:|---:|---:|---:|
| 50x20x25 | 504 | 4.5 ms | ~0 | 0.013 ms |
| 10x20x10 | 6,240 | 33–39 ms | 1.2–1.3 MiB | 0.13 ms |
| 5x20x10 | 12,480 | 71–74 ms | 6–9 MiB | 0.26 ms |
| 5x20x5 | 24,960 | 143–156 ms | 11.6–12.6 MiB | 0.56 ms |
| 5x20x5, chunk 100 | 24,960 (69 components) | 156–166 ms | 11–13 MiB | 0.06 ms |

So generation costs **about 6 µs per instance**, Chaos and ISM add **about 0.5 KB RSS per
instance** on top of the tracked arrays, and a removal costs **about 0.15 µs per surviving
instance in its group** (the native-ID index remapping is linear). An editor-world run
creates no bodies, so it under-reports by about 6x. Use only the game-world figures.

### 1.3 Cell counts

| Map | Today | 5cm cells, all layers solid | With constructions (§3.2) |
|---|---:|---:|---:|
| Lab room | 504 blocks | 100,800 | ~75,600 (drywall walls = 2 boards) |
| Procedural 10 rooms, 1 floor | 9.9k–10.1k | ~2.0M | ~1.7M (a third of the walls are drywall) |
| Procedural 3 floors | 17,427 | ~3.5M | ~2.3M |
| Killhouse base (mirrored) | 3,366 | 1.17M (wood crates solid at 5x5x2) | 0.70M (crates as 2cm shells) |

All 87 Killhouse pieces have sizes that are multiples of 5 cm. Its wood crates are 100–120 cm
solid volumes. As solid 5x5x2 cells they would be 532k cells. The spec says wood means
2 cm boards ("pannelli/assi"), so they must become hollow 2 cm shells (57k cells).

## 2. Options

### Geometry and collision

| | A. ISM per cell (current path scaled) | B. Fracture on hit | **C. Chunk voxels: greedy face mesh + box collision per chunk** | D. Greedy boxes as ISM instances |
|---|---|---|---|---|
| Idea | Today's pipeline, 5cm blocks | Intact block = 1 instance, first hit splits it into 200–500 cell instances | Panel grids, 32x32-cell chunks, one mesh + one body (aggregate boxes) per chunk, rebuild only dirty chunks | Merge alive cells into maximal boxes and render/collide each box as an ISM instance |
| Internal faces | all cell faces | all faces of fractured cells | **none** | only between adjacent boxes |
| Generation, 3.5M cells | ~21 s *measured rate* | ~today | 65–130 ms *estimate* | ~10 ms (1–2k instances) |
| Memory, 3.5M cells | ~3.2 GB (410 B tracked + ~0.5 KB Chaos per cell) | +~4 MB and ~8k bodies per grenade | ~5–8 MB total *estimate* | ~2 MB + boxes |
| Chaos bodies | 3.5M | 17k + 8k per grenade | 1.1–1.6k (1 box each while intact) | 1.1–1.6k + 40–230 per grenade |
| Removal cost | 3–6 ms per dirty 400cm group (20–40k instances) | 5 ms per grenade (instance add) | 0.2–0.4 ms per dirty chunk *estimate* | ~0.05 ms per dirty chunk |
| Replication | 27,300 pages (cap 391), 17.5 MB snapshot | per-cell pages | sparse chunk masks (below) | sparse chunk masks |
| Verdict | **rejected** | rejected: cost grows with every grenade, still no aggregation | **recommended** | fallback if C's mesh rebuild or draw cost is too high |

C uses `UDynamicMeshComponent` (GeometryFramework, a runtime module).
`SetSimpleCollisionShapes(FKAggregateGeom)` with `FKBoxElem` gives Chaos boxes with no
triangle-mesh cooking. `CollisionType=CTF_UseSimpleAsComplex` makes queries hit exactly the
alive cells. The existing PBR materials use world-position mapping, so the meshes need no
authored UVs. Hit resolution no longer depends on `Hit.Item`: the cell is
`floor(local(ImpactPoint - Normal*eps) / CellSize)`, which is deterministic and immune to
instance reordering.

### Replication

| | N1. Absolute pages per cell | **N2. Sparse chunk masks** | N3. Replicate the event, re-simulate on clients |
|---|---|---|---|
| Payload | 5 B per cell: 17.5 MB | only modified chunks: alive bitmask (≤512 B raw, RLE 20–150 B), revision | origin + profile + sequence |
| Late join | everything | modified chunks only; untouched chunks follow from the layout fingerprint | replay all events |
| Risk | impossible | low: authority-only HP, clients need alive bits only | float divergence across Mac/Windows → needs digests and resync |
| Verdict | rejected | **recommended** | later optimisation at most |

N2 keeps the M6 contract: a descriptor (protocol 7, epoch, fingerprint, sequence, manifest of
shard versions) plus N tickless, always-relevant shard actors. Each shard holds an
`FFastArraySerializer` of at most 128 chunk items, which stays under `net.MaxRepArraySize`
2048 and keeps bunches below 64 KB. Clients still apply only complete manifests and fail
closed on mismatch. **HP stays on the server**: clients do not compute damage. A HUD readout
of the hit cell's HP travels on the existing unreliable impact multicast. Stable IDs are
`FERCellId{Panel, Index}`, where `Panel` derives from today's wall/piece ID plus the
construction layer and `Index = i + nx*(k + nz*j)`. They never use instance indices.

## 3. Recommended design (C + N2)

### 3.1 Storage

- **Panel**: an oriented box (today's wall/piece transform, any yaw) holding one construction
  layer of one material. It is a grid `nx x nz x ny` with cell size `5 x 5 x depth`: 5 for
  massive materials, 2 for wood, the board or sheet thickness for drywall and metal.
- **Chunk**: 32x32 cells on the face x all layers of the panel (≤4096 cells). It holds a dense
  alive bitset (128–512 B), a sorted sparse `(uint16 index, float HP)` array for damaged
  cells (8 B each, full float: fractions are kept as the spec requires) and a revision.
- Glass stays a **pane record**: one entity, HP 8, pane break. It has no cells.
- *Estimate* for 2.3–3.5M cells: bitsets 0.3–0.45 MB, 1.1–1.6k chunk headers 0.2 MB,
  sparse HP ≤0.11 MB per grenade. This replaces 181 B + 67 B per block today.

### 3.2 Constructions (section 2 of the spec)

A construction is a list of layers in the wall slot. Thicknesses are **provisional
configuration, not agreed physical values**:

| Construction | Layers | Cells per 5x5 face |
|---|---|---:|
| Drywall partition (20cm slot) | 1.25 board, 17.5 cavity, 1.25 board | 2 |
| Brick 20 / 10 | 4 / 2 layers of 5 cm | 4 / 2 |
| Concrete 20, Stone 20 | 4 layers of 5 cm | 4 |
| Wood wall | 2 cm board (optionally 2 boards + cavity) | 1 |
| Container sheet | 0.2 cm sheet (collision box at least 1 cm) | 1 |
| Glass | pane, 2 cm as today | pane |

Templates must assign constructions. Solid wood crates become 2 cm shells.

### 3.3 Render and collision

Each dirty chunk rebuilds its greedy faces (visible faces between alive cells and empty
space, including across chunk and panel boundaries) and its greedy boxes, once per batch.
Intact chunks are about 6 quads and 1 box.
*Model* per grenade (4x3 m wall, grenade 10 cm from the face at 110 cm): 3–6 dirty chunks,
160–630 quads, 40–190 boxes. *Estimate*: 0.2–0.4 ms per dirty chunk. Collision is
rebuilt in the same frame because movement is server-authoritative; render can be deferred
by one frame if needed. A 5 cm hole is a 5 cm hole in the collision boxes too, so the
capsule passes only when the breach really is larger than the capsule.

### 3.4 Combat

- **Bullets**: voxel DDA through the panel grids replaces one physics trace per block.
  `EntryEnergyCost` is charged once per entry from air into a material run, and
  `EnergyCostPerCm` per cm of path. For a 20 cm wall this is identical to today's per-block
  cost; charging the entry per cell would make walls 4x harder to penetrate. Each crossed
  cell takes `Structural x residual/initial x Bullet x`, as the spec requires.
- **Grenades**: candidates come from chunk AABBs within 200 cm. Distance is measured to the
  cell centre. Exposure is a DDA from the origin to the cell's exposed face across all
  panels; intact cells attenuate by **path length** (`T_cm ^ cm`). Static, non-destructible
  occlusion uses up to 9 physics traces per candidate chunk, not one per cell. Everything is
  evaluated on the pre-event snapshot and applied as one batch, as today.
  *Model*: 16.5k candidates for a 20 cm 4x3 m wall. *Estimate*: 3–10 ms of exposure plus
  1–2.5 ms of rebuild per grenade (today 0.4 ms for 117 blocks). This is the main CPU risk;
  the prototype must measure it on a multi-layer wall (spec §9.8).

### 3.5 Support and collapse

The spec keeps collapse out of this update, and the existing M5 collapse must not break.
- **Prototype**: collapse is off for cell worlds. This is logged as today's
  `ER_BLOCK_SUPPORT_UNAVAILABLE` fallback; block worlds keep `FERBlockSupportGraph` unchanged.
- **Phase 2**: the graph nodes become *connected components of alive cells inside a chunk*
  (flood fill of ≤4096 cells, about 20 µs, only for dirty chunks), and the edges become chunk
  faces sharing alive cells. That is about 1–3k nodes, so the existing budgeted `Step`
  (4096 units, 1.5 ms) and `Detached` replication apply unchanged after generalising the
  graph from `FERBlockRecord` to nodes.

### 3.6 Map templates

ERMapTemplate and ERDustTown pieces are already axis-aligned boxes on a 50 cm grid with
optional yaw, so each piece becomes one panel per construction layer. Fixed art,
`ERTemplateVisual` and walkable slabs are unchanged. The fingerprint adds panel grids, cell
sizes, constructions and the calibration stamp (§4). Migrating the procedural map and the
templates is **phase 2**. Removing the per-block ISM path is **phase 3**: one system remains,
not two in parallel (spec §9).

## 4. HP and parameters at the new resolution

- **Cell HP stay the spec values** (20/100/600/40/1000/100, glass 8 per pane). The spec
  calibrates them for exactly this discretisation; what ends is their provisional use on
  50x20x25 blocks. Bullet multipliers, explosion multipliers and the 400/0/25/200/^2 profile
  are unchanged.
- **Calibration stamp**: the catalog records the cell size (5, wood 5x5x2) and construction
  thicknesses the HP were tuned for. Any mismatch logs `ER_BLOCK_CALIBRATION_MISMATCH` and
  generation refuses it until HP are explicitly re-stamped. HP are never silently reused.
- **Transmission becomes per cm of intact material**. Today's rule, multiplied per crossed
  cell, breaks at 5 cm: oblique rays cross several cells of the same layer. In the model
  a 20 cm brick wall then gets only 0.01–0.06 m² of through-hole after three grenades.
  Equivalent values (`T20^(1/20)`): Drywall 0.966, Brick 0.933, Concrete 0.861, Wood 0.966,
  Stone 0.822, Metal 0.966, Glass 0.995. `MaxShieldingLayers 4` becomes `MaxShieldingCm 40`.

*Model*: grenades on the same spot of a 4x3 m wall. "Chest" means 10 cm from the face at
110 cm height; "foot" means on the floor, 5 cm from the face. Capsule: standing 68x185 cm,
crouched 68x125 cm, step-up ≤45 cm.

| Construction | Cells removed (1 grenade, chest) | Through-hole (1 grenade, chest) | Grenades for a standing breach | At the foot |
|---|---:|---:|---|---|
| Drywall board 2.5 | 3,258 | 8.1 m² | 1 | crouch after 1 (hole up to 175 cm) |
| Wood board 2 | 2,734 | 6.8 m² | 1 | crouch after 1 (160 cm) |
| Container sheet 0.2 | 1,568 | 3.9 m² | 1 | no passage (115 cm) |
| Brick 10, T 0.933/cm | 2,426 | 1.1 m² | 2 | crouch after 2 |
| Brick 20, T 0.933/cm | 2,766 | 0.3 m² | 3 (crouch after 2) | none after 3 |
| Brick 20, T 0.96/cm | 3,558 | 0.8 m² | 2 | none after 3 |
| Concrete 20 | 0 | 0 | never (48–60 surface cells after 3) | — |
| Stone 20 | 0 | 0 | never | — |

Bullets at 36 structural points keep the spec table per cell: drywall 1 hit, brick 4, wood 2,
sheet 6, concrete 3.6 points and stone 1.8 points per hit.

## 5. Network estimate (N2)

| | Today | Cells (N2) |
|---|---:|---:|
| Late-join payload | 50–88 KB, constant | modified chunks only: ~15–40 KB after 50 grenades *estimate*; worst case everything modified 1.6k x 512 B = 0.8 MB, sharded |
| Grenade delta | ~2 KB (3 pages x 640 B) | 0.14–0.75 KB *model* |
| Cell removed by a bullet | 640 B page | 20–140 B chunk item |
| HP on the wire | yes | no (server only) |

## 6. Prototype on L_DestructionLab (approved and implemented, results in §8)

1. Cell storage, IDs, constructions and calibration stamp, plus automation tests for spec
   §9 cases 1–6 at cell resolution (preset, local damage, 36-point table, grenade at ≤25 and
   ≥200 cm, 4-layer vs 1-layer shielding, order independence).
2. Chunk meshes and box collision, dirty rebuild, point-based hit resolution, and a real
   capsule sweep through a grenade breach (§9.7).
3. DDA ballistics, voxel exposure plus chunk static occlusion, glass pane record (§9.8).
4. Protocol 7 descriptor plus chunk shards, initial and late join, state digest equality in
   the LAN probe (§9.7).
5. A stress probe extension with the same CSV columns plus grenade-on-multilayer-wall, chunk
   rebuild ms, bytes per event and late-join bytes, recorded as a new benchmark receipt.

Lab walls: 0 and 3 drywall partitions, 1 brick 20, 2 concrete 20, plus one wood wall and one
sheet sample. The procedural map and templates are untouched until phase 2.

## 7. Risks to measure first

`NotifyMeshUpdated` rebuilds all buffers of a component, so chunks stay small. Body
recreation can cause hitches. Draw calls rise to 1.1–1.6k components; render regions can be
merged later if needed. Thin sheets need a minimum collision thickness. Sparse HP could
grow under sustained fire. Decals must be placed on cell faces. If C's mesh rebuild or render
cost is too high, D (greedy-box ISM) reuses the proven native-ID pipeline.

## 8. Prototype results (17 October, measured)

Implemented as a mode of `AERBlockRoom` (`bCellMode`), not a parallel system. Start
L_DestructionLab with `-ERCellLab` or `?Cells=1`, or type `ERCellMode 1` in the console;
`ERBlockGrenade` fires the HE structural profile at the aimed wall. Code:
`ERCellWorld` (storage, constructions, DDA, RLE), `ERCellMesher` (greedy faces/boxes),
`ERBlockRoomCells.cpp` (components, shots, blast, replication), `ERCellNetShard`.
The lab has 8 panels (two drywall partitions of 2x1.25 cm boards, brick 20, concrete 20,
a 2 cm wood board and a 0.2 cm sheet sample), 77,580 cells and 52 chunks.

| Measurement (M2 Pro, nullrhi, real game world) | Result |
|---|---|
| Generation, 77,580 cells / 52 chunks | 5.5 ms (mesh 3.7, collision 0.75); intact = 188 quads, 52 boxes, 52 bodies |
| Logical memory | 25 KB (0.33 B per cell) + 3 KB mappings |
| CPU mesh copies (`FDynamicMesh3`) | 1.97 MB = **~38 KB per chunk**, mostly fixed container overhead (see risks) |
| Grenade, drywall partition | 11,174 candidates, 7,937 removed, 14 chunks, **4.3 ms** total (exposure 2.0, mesh 0.83, collision 0.27) |
| Grenade, 20 cm brick (1st / 2nd / 3rd) | 2,570 / 3,138 / 2,290 removed, 6 chunks, 5.6 / 7.3 / 6.3 ms |
| Grenade, 20 cm concrete | 17,402 candidates, 0 removed, 4.5 ms |
| Grenade, wood board / sheet | 8,590 / 4,262 removed, 4.9 / 6.9 ms |
| Generation scaling (larger lab rooms) | 149,580 / 367,500 / 607,500 cells in 13 / 33 / 56 ms: ~0.14 ms per chunk, so ~155-230 ms for 1.1-1.6k chunks |
| Rebuild per dirty chunk | 0.06-0.1 ms mesh + 0.02-0.04 ms collision (estimate was 0.2-0.4 ms) |
| Exposure per candidate | ~0.2 µs (as estimated) |
| Network delta, drywall grenade | 13 chunks, **606 bytes** for 7,937 removed cells (model 0.14-0.75 KB) |
| Rifle cartridge into drywall | one cell per board removed, residual energy 88.7, rear target 500 → 468.1 |
| 36-point bullets into 20 cm brick | layers at 73 / 85.15 / ... HP, nothing removed |

Breaches with the equivalent transmission (brick 0.933/cm): drywall, wood and sheet open
a standing passage with one grenade, and the real character walks through drywall.
Twenty-centimetre brick holds after 1 grenade. A crouched capsule passes after 2
(sweep) and the real crouched character after 3. A standing capsule passes after 4.
Three grenades leave a 2.77 m² hole from 20 to 200 cm high (the model predicted
2.83 m²), 5 cm short of the 185 cm standing capsule. 5 cm brick opens with one grenade
(4 layers ≠ 1). Concrete is damaged but loses no cells. A single 5 cm hole lets a ray
through but not the capsule. Order independence, the calibration rejection, glass
panes, real HE fuse locality (nothing changes from 200 cm) and the protocol-7
transaction gating (out-of-order shards, future versions, invalid masks, new epoch,
protocol mismatch) are automated.

Rendering was checked on Metal (`--render`: `Saved/Testing/Cells-{Overview,DrywallBreach,BrickBreach,WoodEast,WoodWest}.png`
against `Blocks-Overview.png`). Lighting matches block mode, and the 5cm stepped edges,
both drywall boards with their cavity, and the four brick layers are visible. The first
captures exposed two mesh bugs that nullrhi tests could not see. Triangles were wound the
right-handed way, but Unreal front faces wind the other way (`GetTriNormal` is edge2 x edge1).
One normal element was also shared by the four corners of a quad; overlay elements belong to
one vertex, so the triangles were left without normals. Both are fixed, and `MeshAndCollision`
now checks winding and normals on every triangle. A faint line remains on the wood board at a
chunk border.

Remaining work is in §7 plus: lighter per-chunk meshes or merged render regions, because
the ~38 KB fixed `FDynamicMesh3` cost per chunk would be 40-60 MB at 1.1-1.6k chunks;
the 1-2.5 ms `TMap` damage aggregation; a real LAN run in cell mode (the protocol is
only tested in-process); support/collapse over chunk components; and phase 2
(procedural maps and templates with constructions, crates as 2 cm shells).

## 9. Cells everywhere (18 October, measured)

The user asked to remove the old blocks everywhere and chose to port collapse now. Phase 2
and 3 of this plan are done:
- **Procedural maps:** walls become constructions with door and window openings as absent
  cells, plus one glass pane per window.
- **Template pieces:** keep their exact volume; each axis is divided into
  round(L / 5 cm) equal cells (`FERCellWorld::FitCell`), so for example a 297.18 cm
  length gets 59 cells of 5.04 cm. Massive materials become layered slabs, thin
  materials become boards, and volumetric wood crates become 2 cm shells. The first
  port truncated pieces to whole 5 cm cells; the Metal captures showed 2.5 cm
  see-through slits between stacked pieces, and pieces thinner than 5 cm vanished.
  The MapTemplate test now rejects any slit under 5 cm between facing pieces.
- **Removed:** ISM block groups, block records/IDs, protocol 6 pages, the block support
  graph and the block lab probe/tests. Seven block tests were replaced by cell versions.

| Map | Panels | Chunks | Cells |
|---|---:|---:|---:|
| Lab | 8 | 52 | 77,580 |
| Procedural 8/10 rooms, 1 floor | 70 / 82 | 620 / 704 | 1.29M / 1.71M |
| Procedural 6 rooms, 2 / 3 floors (7 materials, glass) | 104 / 156 | ~780 / ~1,170 | 1.62–1.65M / 2.42–2.45M |
| Killhouse, 512 seeds | — | — | 0.46M–1.37M |

Stress benchmark on the 10-room map (1.71M cells), 3 samples × 2 seeds; the block
values are from the 17 October run on the same map (10k blocks):

| Operation | 80cm chunks | **160cm chunks** | 320cm chunks | Blocks (chunk 400) |
|---|---:|---:|---:|---:|
| Generation | 373 ms | **151 ms** | 116 ms | 49 ms |
| Single removal, median / p95 | 0.14 / 0.35 ms | **0.19 / 0.33 ms** | 0.56 / 0.70 ms | 0.016 / 0.034 ms |
| 256 scattered cells (216 chunks at 160cm) | 30 ms | **34 ms** | 54 ms | 1.8 ms |
| Support check after that batch | 4.0 ms | **9.9 ms** | 17.4 ms | — |
| HE grenade, room wall | 6.2 ms | **5.8 ms** | 5.9 ms | 0.43 ms (linear 250) |
| Chaos bodies / components | 2,696 | **704** | 194 | 9,949 / 92 |
| Tracked memory | 107 MiB | **33.5 MiB** | 15 MiB | 4.1 MiB |

Most of the tracked memory is the CPU copy of each chunk's `FDynamicMesh3` (25.7 MB at
160cm; about 36 KB per chunk regardless of content). Logical cell storage is 0.33 MB, and
the support graph with its contacts takes 7.4 MB. Removing many isolated cells is slower
than with blocks because every touched chunk rebuilds its mesh and body (about 0.15 ms each).
Grenades touch few chunks, so they stay at about 6 ms.

Collapse, measured by the `--collapse` probe:
- **Lab demo** (ring in the drywall partition): two islands of 551 cells each, graph build
  1.4 ms, evaluation 0.3–0.8 ms, one Chaos fragment per component that really falls.
- **Lattice cut:** 53,750 unsupported cells in 1,110 components, evaluation 7 ms.
  Debris caps hold: 8 per step and 64 global, also across two rooms.
- **Whole 10-room shell without footings** (1.69M cells): contact build 40 ms, one
  evaluation 106 ms, detachment over 52 steps (32,768 cells per step, p95 13–14 ms, peak
  116–121 ms), completed in about 2.8 s.

Network with real processes: `test-cell-network.py` (lab, including collapse replicated to
clients), `test-destruction-network.py` (procedural building: 3 epochs, 2.45M cells, 1,162
chunks, 10 shards, initial, late and dense joins consistent), and Killhouse and Dust with
`--network` all pass, and both LAN tests pass with `--loss` too.

Breach rules under the preset are unchanged: one grenade opens drywall, wood and sheet.
Twenty-centimetre brick needs several, and even the old linear 2400 blast opens only a
1.2 m hole in it. Concrete and stone resist. Probes that must walk through a breach now
pick thin constructions.

## Reproduce

```sh
python3 scripts/test-destruction-stress.py --samples 8 --label preset17oct
python3 -I scripts/model-cell-destruction.py Saved/Testing/cell-destruction-model.json
python3 scripts/test-destruction.py
python3 scripts/test-destruction.py --game
python3 scripts/test-destruction.py --collapse
python3 scripts/test-destruction.py --procedural
python3 scripts/test-destruction-stress.py --samples 3 --label cells18oct
python3 scripts/test-cell-network.py
python3 scripts/test-destruction-network.py
```

`scripts/measure-ism-cell-scaling.py` and `scripts/measure-cell-scaling.py` run inside the
game (commands in their headers).
