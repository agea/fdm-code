# Industrial map archetypes

22 October2026: **Middle Eastern Alleys · Desert** joins Central Building in
the production rotation. The industrial-only scope below is historical; see
[the new alley template](MIDDLE_EASTERN_ALLEYS.md). Depot remains a reference.

20 October2026 art update: revision7 adds the owner's
[Sketchfab texture palette](SKETCHFAB_TEXTURES.md). Concrete, masonry and timber
finishes vary coherently by seed; exteriors choose asphalt, grass or snow. New
gray steel and riveted deck finishes replace the corresponding Factory surface
overrides; Factory prop meshes and four container paint colors remain. This
supersedes the revision5 steel/deck art choice below while preserving physical
profiles and all geometry. The active PCG snapshot is now
`DA_CentralBuilding_42_SketchfabTextures`.

Current scope, 19 October 2026: production maps use industrial art only. The
playable recipe is **Central Building**. **Depot** is a selectable reference
environment, excluded from the production rotation. Movement, destruction and
procedural laboratories, Killhouse and Dust are retired levels.

## References and adaptation

The user's [Steam guide](https://steamcommunity.com/sharedfiles/filedetails/?id=2186934885)
contains the three reference plans, inspected directly during this change:

| Reference | Plan | Rule in this game |
| --- | --- | --- |
| [Lake](https://steamcommunity.com/sharedfiles/filedetails/?id=125440557) | [Overview](https://images.steamusercontent.com/ugc/1488956685136672729/54D5A1629A4DE61EF1DAB21744BC5F9885EA56B0/) | Outdoor circulation around the main building; no midfield separator. |
| [Safehouse](https://steamcommunity.com/sharedfiles/filedetails/?id=125440714) | [Overview](https://images.steamusercontent.com/ugc/2015961096469913053/D9F8E2CC4B1DF0D44250D7631EBD2B5D3798034D/) | Separate courtyards, joined through the building or over a 3m wall using paired stairs and bridges. |
| [Bank](https://steamcommunity.com/sharedfiles/filedetails/?id=125440342) | [Overview](https://images.steamusercontent.com/ugc/1488956685136626658/426308A8E59C24583AFEAB4930B7272B01710286/) | Fixed fencing closes the ground midfield; cross the main roof or either side building. |

CT maps to **Alpha**, T to **Omega**. These are original, balanced industrial
adaptations of the circulation patterns requested by the user. Original map
outlines, spawn arrangements, residential themes and bomb objectives are not
replicated. No Valve models, textures or map binaries enter the project.

## Recipe, seed and collision

`UERMapTemplate.Archetype` distinguishes a recipe from a legacy authored assembly.
`FERCentralBuildingRecipe` owns building/arena ranges, floor counts, crossing
rules and industrial art references. `ERMaps` is the single menu/travel/rotation
catalog. A production round advances its seed before respawning players.
The crossing preset is always sampled from that seed; the menu offers no preset
selection and the recipe has no fixed crossing property.

Supported variation: arenas 64–80 × 84–100m; central building 18–24 × 22–30m;
one to three 3.2m storeys; two stairs per main storey; paired secondary buildings
or open timber towers with one to three floors and working stairs; alternating
brick/concrete secondary walls; variable rows of steel containers, timber crates,
stacked crates, brick/concrete barriers, tanks and barrels. Rectangular cover
uses yaw angles in 15-degree increments up to +/-45 degrees. Secondary structures
can rotate +/-20 degrees, except the bank-style buildings whose doors must meet
the midfield separator. Wall-style structures occupy reserved courtyard space.
Each half is sampled once, then rotated 180 degrees, including partial cell
edges, all fixed collision, doors, stairs and ten starts per team. The central
building is also symmetric under that rotation. Teams never roll independently.

Stairs have 20cm risers, 30cm treads, open floor wells and solid landings. Roof
parapets leave the stair mouths and roof crossing open. Reserved routes receive
full standing-capsule segment validation before the new layout is committed.
Revision6 furnishes both approaches with several bands of destructible cover:
standing-height L/U pockets, brick/concrete screens, timber crates and reinforced
steel containers. Low screens and crates protect crouched players. OpenYard adds
paired midfield flank screens to interrupt long outdoor sightlines. Each pocket
reserves an entrance and capsule-sized hiding space; furnishing also reserves
spawn areas, patrol/crossing routes, building footprints and ladder approaches.
Every group keeps its half-turn gameplay counterpart. Destroying a screen can
expose its hiding place. Cover rows fit the available courtyard depth.
Recipes reject invalid ranges and geometry
atomically. A 0.01cm numerical boundary tolerance accommodates divided fence tiles.
The wall variant uses a destructible 300cm brick separator, with two mirrored
320cm-high timber bridges and stairs on both courtyard sides. Their treads,
posts and landings are all cells. Removing supports can collapse an access;
breaching the brick creates a new ground route. No upper deck is permanently
anchored to the world. Ground-level vaulting cannot clear an intact wall.

Destructible concrete/brick/timber uses the existing 5cm cell damage, support,
collapse and protocol-8 replication. Main building slabs/stairs, tanks, barrels
and wire are fixed. Secondary floors/stairs and access structures are destructible.
Containers are hollow steel cell shells, with reinforcement Strength=8. This
multiplies their cell HP, penetration costs and material shielding, leaving the
shared damage profiles and calibrated 5cm/2mm sheet cells unchanged. A rifle
damages the steel but cannot penetrate the reinforced intact shell. Cell geometry
supplies the visible surface and collision; no intact vendor mesh hides a breach.
All oriented constructions retain orthonormal local cell grids through damage,
support and replication. Wire mesh blocks
pawns and dynamic objects across the whole panel. A synchronous native triangle
mesh supplies both the visible metal and shot/visibility/blast collision: 4cm
frames and 0.4cm diagonal wires on a 16cm pitch. Shots pass through actual diamond
apertures and stop on the metal; there is no random hit probability. Both sides
use the same collision, and the wire geometry version enters the layout digest.
Metal is cooked synchronously in bounded parts below Unreal's triangle limit;
the full movement volume is excluded from WorldStatic blast-cover queries.
Hidden authored boxes approximate solid vendor prop bounds. Detailed art never
provides the walkable surface of a stair or floor. Geometry digests include
barrier behavior as well as meshes/transforms and the template identity.

The server replicates seed and crossing override in the existing descriptor;
clients reconstruct the same validated layout behind the existing loading gate.
Protocol 8 adds a count and digest for each shard's complete mask payload. A
header arriving before its fast array cannot open collision on a late join.
Old protocol-7 executables fail closed; all peers use the rebuilt module.
Menu matches use fresh seeds. For reproduction:

```text
/Game/EclipseRecoil/Maps/L_CentralBuilding?MapSeed=42
```

`MapCrossing` is ignored during normal play. Only a non-shipping process launched
with `-ERIndustrialMapProbe` accepts it for test coverage: 0 seeded, 1 open yard,
2 tall wall with stair access, 3 fenced crossings. The replicated test override
is hidden from Blueprint/editor settings. Normal finite ammunition rules apply.

## PCGEx and art

PCG and installed **PCGExtendedToolkit 0.79.2** are enabled. The generator uses
PCGEx's native path representation to analyze crossing paths. `CreatePCGPreview`
exports validated spawn, crossing, cover-pocket, fixed-volume and destructible-cell-volume collections, and
`setup_industrial_pcg_preview.py` saves a PCGEx **Path : Properties** graph for
route inspection. This authoring snapshot is for its named seed; changing it
does not silently replace authoritative collision. Runtime generation and
destruction remain synchronous and share one layout/identity pipeline. The current
snapshot is `DA_CentralBuilding_42_FactoryFinishes`; the earlier seed42 assets are retained
as an inactive authoring snapshot.

Private industrial assets come from the user-specified
`/Users/agea/Documents/Unreal Projects/FactoryEnvironmentCollect` through Unreal
migration, under `/Game/EclipseRecoil/Art/FactoryEnvironment`. Added wire-mesh
materials/textures preserve vendor provenance. The source project is unchanged.
`Art/Industrial/SM_ER_Chainlink` remains an authoring reference with an owned,
two-sided masked material using the supplied wire mask's red channel. Runtime
fences use explicit metal geometry so visible apertures agree with shot queries;
their surface comes from the existing industrial metal catalog. The migrated
prop material parents enable instancing via `setup_industrial_material_usage.py`;
their appearance parameters and the original Factory project remain unchanged.
The shared destructible catalog and edited Depot are preserved byte-for-byte.
A truck is not part of the prop palette. Towers and compact secondary buildings
prefer ladders as their primary access to every floor, through alternating
160cm floor hatches placed beside the door corridor. Each storey has its own
ladder and supported landing. Non-tower buildings at least800cm wide and1150cm
deep retain full stairwells. The central building retains both main stairwells
and also has paired exterior ladders. Secondary exterior ladders remain an
alternative roof access. All rails/rungs use the existing destructible2mm steel
calibration. Roof parapets have real openings, and handles project110cm above
the landing. Placement avoids the bank divider and preserves half-turn symmetry.
Traversal automatically stows heavy weapons, retains pistols/knives/hand
grenades in hand, allows inventory changes and checks actual surviving
rungs/handles. Factory's `SM_Ladder01/02` remain source references; runtime ladder
geometry uses the shared destructible materials.

## Maintenance

Create/save through `scripts/setup_central_building.py`; completed data assets
and maps are preserved on reruns. Compile with `scripts/build-editor.command`.
Run `scripts/test-industrial-map.py` for native recipe/collision tests, `--game`
for actual stair walking and rifle damage, `--render --gallery` for Metal captures,
and `--network` for initial/late clients, destruction and a replacement seed epoch.
Executed outcomes belong in `TEST_RESULTS.md`.

`retire_legacy_maps.py` checks outside package references and verifies a SHA-256
archive before using Unreal to remove the explicit retired set. Recovery lives
in `Saved/MapRefactorBackup/2026-10-19`; it is private and excluded from public
Git. Historical template notes remain in `MAP_TEMPLATES.md` as a record.

Recipe revision4 and `setup_industrial_ladders.py` apply this access policy.
The PCG graph loads `DA_CentralBuilding_42_FactoryFinishes`; the earlier Evolution snapshot
remains an inactive authoring reference.


Revision5 adds a recipe-owned Factory steel/paint palette. Ladders, metal decks,
structures and wire use gray, lightly worn steel; containers vary across blue,
red, green and ochre. Cosmetic rolls are independent across the symmetric copies
and deterministic on all peers. Factory atlas regions are adapted by dedicated
private materials; vendor originals and the shared destruction catalog stay
unchanged. Cell surfaces affect appearance/layout identity, not physical profiles.
Run setup_factory_finishes.py to save this palette and its PCG snapshot.
