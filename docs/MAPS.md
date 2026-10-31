# Maps

`ERMapCatalog` is the single menu, travel and rotation catalog. All maps cook through
`MapsToCook` in `Config/DefaultGame.ini`.

| ID | Level | Recipe | Rotation |
| --- | --- | --- | --- |
| `central_building` | `L_CentralBuilding` | `DA_CentralBuilding` | yes |
| `middle_eastern_alleys` | `L_MiddleEasternAlleys` | `DA_MiddleEasternAlleys` | yes |
| `fire_training` | `L_FireTraining` | `DA_FireTraining` | yes |
| `rooms` | `L_Rooms` | `DA_Rooms` | yes |
| `village` | `L_Village` | `DA_Village` | yes |
| `depot` | `L_Depot` | fixed reference | offline only |

Recipes live in `Art/MapTemplates`. Reproduce a layout with
`/Game/EclipseRecoil/Maps/<Level>?MapSeed=42`, or `ERMapGenerate <seed>` on authority.

**Code:** `ERMapCatalog`, `ERMapTemplate.h`, `ERProceduralConfig.h`, `ERProceduralMap`, `ERMapSizing.cpp`, `ERMapTheme.h`.

## Shared rules

- `ERProcedural::Build` is deterministic from the replicated seed and validates before
  committing; invalid layouts are rejected atomically. Each peer repeats the same bounded
  attempts (16, or 256 for alleys), and the attempt index enters the digest.
- **Half-turn pairing:** sample one half, rotate geometry, spawns and fixed collision 180°.
  Teams never roll independently. Symmetry is not a claim of balance.
- Walkable decks, roofs and treads use solid surfaces with matching visible and collision
  geometry. Detailed art never provides a walkable surface.
- All peers rebuild the same layout behind the loading gate. See [DESTRUCTION.md](DESTRUCTION.md).
- Each template has ten supported starts per team, plus server-side overflow near the base.
- Rounds advance the seed before respawn. Menu sessions use fresh seeds.
- Cosmetic rolls use a separate stream and never consume geometry RNG.

## Team sizing

`Map.ExpectedTeamSize` (1..16) budgets **500m² per expected player**:
`target area = team size × 1000m²`, **±10% of area**, not per axis. Upper floors, roofs and
lower passages do not count; Rooms counts its enclosing rectangle. No saturation at 8.

`ERMapSizing.cpp` configures transient recipe copies: structural counts, spacing and bounds.
It never scales characters, portals, stairs, ladders, kit meshes, material thickness or cells.
Zero keeps authored studies. The size travels in the protocol descriptor.
Admission, capacity and bot counts are independent of it.

```sh
python3 scripts/preview-map-sizing.py --team-size 2 --seeds 42 73 7919 --verify-seeds 512
```

Plans land in `Saved/Testing/MapPlans/<N>v<N>/`, with metres, actual area, target and
deviation. They are technical plans, not renders.

## Themes

`DA_MapThemes` (`ThemeLibrary`) offers **Industrial, Desert, Jungle** to all five templates,
chosen from a cosmetic stream of the map seed: the same seed gives the same theme everywhere.
Themes change finishes and ground only, never geometry, physics, calibration or damage. They
separate roles (shell sheets, structural steel, galvanized ladders, decks, painted cargo) so
structures read at distance. Theme identity enters the digest. Alley house walls differ from the
Stone perimeter, and slab finishes differ from both.

## Add a map

1. Recipe `DA_<Name>` and level `L_<Name>` through a `setup_<name>.py` that validates 516 seeds.
2. Register in `ERMapCatalog.cpp`, and add it to `MapsToCook` in `Config/DefaultGame.ini`.
3. Native builder `ER<Name>.cpp`, reached from `ERProcedural::Build`; team sizing in `ERMapSizing.cpp`.
4. Bind `ThemeLibrary`, and add the map to the digest-affecting fields if it has new geometry data.
5. Bump `AERBlockRoom::Protocol` for any change in generated geometry.
6. Layout and physics automation, a `test-<name>.py`, and a page in `docs/maps/`.

## Depot

Fixed reference map with finite ammunition, used by offline play and baseline regressions.
Excluded from lobby buttons, votes and rotation through `InRotation`.

## Textures

Production palettes: ten Sketchfab sources / twelve materials from the owner's RR collection
(original ZIPs and credits in private `SourceAssets/SketchfabTextures`), seven CC0 sets in
`SourceAssets/ProceduralTextures` from Poly Haven and ambientCG, plus Factory paint for
containers. Use DX normals, linear masks, sRGB base colour. Check packed ARM channels and
preserve 16-bit grayscale. Never execute supplied scripts. World-position tiling stays
continuous; no runtime displacement. Removed sets must not be reacquired by pipeline reruns.

## Rebuilding

Each template has one setup script (`setup_central_building.py`, `setup_middle_eastern_alleys.py`,
`setup_fire_training.py`, `setup_village.py`, ...) plus versioned migrations (`setup_alley_*`,
`setup_village_*`, `setup_map_variety.py`, `setup_map_themes.py`). They validate 516 seeds, keep
a verified rollback and save only owned packages. Completed markers stop older
scripts from downgrading. Do not recreate retired levels (Killhouse, Dust, laboratories).

## Verify

`preview-map-sizing.py --verify-seeds 512`, `test-lobby.py --industrial --map-sizing`. Native:
`EclipseRecoil.Maps.*`, `EclipseRecoil.Destruction.SharedMapThemes`. Every template meets the same
bar: 516 seeds natively, several physical seeds with real capsules, a real pawn on stairs and doors,
damage, regeneration, and initial plus late client snapshots.
