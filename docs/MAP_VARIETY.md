# Map variety and structural resistance — 26 October 2026

Rooms expands to16–24m chambers with winding lanes, turning chambers and an
upper transverse corridor above the lower base-to-base spine. Gallery portals
connect through4m switchbacks, with separate supported routes at both levels.
Boards occur only on long corridors, at5m intervals; rooms have at most two.

Central Building's OpenYard family includes centred cores and lateral cores
atX=+/-800cm. For lateral variants the spawn centroids shareX=0, so the core
is equally distant from both bases. Ground doors are centred on the core.
Main pieces carry a bounded X-only `SymmetryCenter`; paired core construction,
ladders and stairs follow it. Other structures keep their existing half-turn.
Multi-storey OpenYard cores step back250cm per side and100cm front/rear on each
storey. Ground stairs reach the first terrace; two ladders serve each next
terrace, with open side doors into the rooms. Fenced/wall variants retain their existing central geometry.

Warehouse is merged into Industrial instead of remaining a third uniform
palette. Both production themes (Industrial and Desert) use distinct shell
sheets, warm oxidized structural steel, cool galvanized ladders and painted
cargo. These existing textures separate the roles in distant views. Industrial
combines these steel roles with its existing concrete, brick and wood. Original
assets are unchanged; shared theme tests compare actual rendered role finishes.

All material cell HP is65% of its previous value:

| Material | Previous HP | Current HP |
|---|---:|---:|
| Drywall |20|13|
| Brick |100|65|
| Concrete |600|390|
| Wood |40|26|
| Stone |1000|650|
| Metal |100|65|
| Glass |8|5.2|

This reduces resistance to both shots and blasts. Material multipliers,
penetration costs, blast transmission, cell thickness and reinforcement factors
retain their meaning. A36-point structural hit removes unreinforced brick in
3 hits, wood in1 and sheet metal in4. Real firearm damage depends on the weapon,
energy, exposure, thickness and reinforcement. These are gameplay values.

`setup_map_variety.py` saves only DA_Rooms, DA_CentralBuilding, DA_MapThemes and
DA_ER_BlockMaterials through Unreal. It backs up those packages, validates516
seeds for both edited recipes, protects all saved maps/other production recipes,
and writes a receipt. No new models, textures or material assets are imported.
`setup_block_damage_preset.py` uses preset marker2 to preserve the new defaults.

Executed checks and limitations are recorded in `TEST_RESULTS.md`.
