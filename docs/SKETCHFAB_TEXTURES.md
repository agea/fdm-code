# RR Sketchfab texture palette

Requested20 October2026 from the owner's [RR collection](https://sketchfab.com/agea1/collections/rr-792d73a736504af1b1bd0e05599fc256).
Ten original downloadable packages are retained in private
`Assets/maps/SourceAssets/SketchfabTextures`. They provide twelve texture sets;
DJMaesen's brick package contains three distinct finishes. No supplied model,
Blender file, native plugin or script is executed. Only texture images are imported.

## Active industrial recipe

`DA_CentralBuilding` revision7 owns these visual references:

| Element | Selected materials | Reason |
| --- | --- | --- |
| Concrete walls and screens | Seamless PBR Texture Concrete01 by -Arthur-; Concrete2K by Matthew Hartman | Quieter finishes than the previous dark panel joints; two coherent concrete treatments. |
| Brick walls and screens | Brick: Tiled material by Cat O.; three brick textures by DJMaesen | Weathered masonry in red, ochre and gray, plus the scanned brick variant. |
| Timber, crates, bridges and towers | wood05 by Publicdomaintextures | Readable board grain and weathering. |
| Structural steel, ladders and wire | Steel texture with irregularities by PixelatoR | Gray steel without forcing heavy rust on every metal element. |
| Fixed metal decks | Iron Plates2K by Matthew Hartman | Riveted sheet pattern suited to industrial platforms. |
| Exterior ground | Asphalt11, Grass02 or Snow01 by Publicdomaintextures | One continuous exterior finish per seed. No grass geometry or new snow/friction mechanics. |
| Containers | Existing four Factory paint instances | Retain readable blue/red/green/ochre identity. |

On the subsequent20 October cleanup request, four unused library choices were
removed through Unreal: Concrete material by roko, Corroded Metal2K by Matthew
Hartman, Floor material by DJMaesen and Seamless floor Tiled TextureII by -Arthur-.
Their original/prepared inputs, manifest entries and menu credits were also
removed. The two inspected but unimported Shrub tiles/Floor Patreon listings
were pruned from the active source inventory. Only the current ten sources are
included in acquisition/staging/preparation/import scripts.

The previous CC0 library retains its seven referenced sets and authored Glass;
the unused wood-floor, tile-floor and office-ceiling sets were removed. The shared destruction catalog, Factory
original materials and Depot are unchanged; their protected hashes are in the
private import receipt.

All37 removed packages (seven materials/thirty texture maps) had no outside
referencers. Retained asset/source hashes were verified unchanged. A verified
private rollback archive is outside the project in the sibling `_backups`
directory; historical attribution for removed sources stays with that archive.
See `Saved/AssetAudit/unused-map-textures-cleanup.json` and
[TEXTURE_CLEANUP_RESULTS.json](TEXTURE_CLEANUP_RESULTS.json).

## Determinism and physical behavior

`ConcreteSurfaces`, `BrickSurfaces`, `WoodSurfaces` and `GroundSurfaces` are optional
recipe palettes, bounded to eight non-null entries. A separate seeded hash selects
one coherent finish per construction type and round. These choices never consume
the geometry random stream. Existing independent container-color rolls remain.
An empty palette retains the previous saved floor/catalog fallback.

Panel finish paths already enter the cell identity. The selected ground path now
also enters the template digest, so host, initial clients and late clients must
agree on the appearance. Surface changes do not change cell calibration, material
types, HP, penetration, support, friction, collision or geometric symmetry.
The PCG authoring graph now reads `DA_CentralBuilding_42_SketchfabTextures`.

## Texture preparation

Original ZIPs and extracted source images stay untouched. Runtime copies are PNGs
up to2048px. Sixteen-bit grayscale maps are divided by257 before conversion to
eight bits; direct Pillow conversion would clip their values to white. This is
particularly important for Concrete2K albedo and the supplied AO/roughness maps.

Color is sRGB. Normals and masks are linear, with appropriate Unreal compression.
The strong height/normal gradient relationship identifies the supplied Grass02
and Snow01 normals as OpenGL; their green channel is converted for Unreal.
Other generic normal filenames retain their original convention rather than
claiming a convention which was not established. Height maps remain source data;
no displacement changes gameplay geometry.

Materials project from world position on the dominant geometric face, keeping V
aligned with world Z on vertical faces. Every texture's `UVs` connection and every
material output connection is checked. `NormalStrength` defaults to0.65.
Tiles are200cm for concrete/brick/wood/grass/decks,400cm for asphalt,300cm for snow,
and150cm for structural steel. The original legacy importer's unchecked
`Coordinates` connections are not reused.

Ground materials blend a second, rotated/offset sample with a continuous
world-position weight to reduce obvious rectangular repetition. Both normal
bases are aligned before blending. Grass uses a subdued material tint after
the first Metal review showed overly bright yellow-green ground. Original
texture pixels remain unchanged; this adds a second sample per ground map.

The numeric filenames in Cat O.'s package were reviewed visually: `4096.jpg` is
the brick normal map and `ao_4096.jpg` its AO. The package's mud textures and radial
mask belong to the demonstration model and are not mixed into the brick material.
DJMaesen's Color/Normal/Roughness/AO B/C suffixes stay in matching independent sets.
Iron Plates' unrelated `internal_ground_ao_texture.jpeg` is not used as tile AO.

## Provenance and credits

Each retained listing records Creative Commons Attribution4.0 in the preserved
Sketchfab metadata. Publicdomaintextures is credited under that actual listing
license; its name does not imply CC0. Per-model `metadata.json`, `CREDITS.txt`,
`provenance.json`, source URL, author URL, license URL, archive SHA256 and extracted
file hashes are retained. Imported textures/materials also have source/author/
license metadata. `Branding/Credits.txt`, displayed by the existing Credits menu,
names every imported source and describes the adaptations.

## Rebuild

1. `scripts/acquire_sketchfab_texture_metadata.py` preserves public metadata and credits.
2. Obtain original ZIPs through the normal authenticated Sketchfab download UI.
3. `scripts/stage_sketchfab_textures.py` preserves ZIPs, safely extracts images and hashes sources.
4. `scripts/prepare_sketchfab_textures.py` prepares the matched runtime maps.
5. Run `scripts/import_sketchfab_textures.py` through the project's compiled Unreal
   Editor with `-run=pythonscript -script=<absolute script path>` and the absolute
   `RespawnAndRage.uproject` path. The verified executable is
   `/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor`.
6. Run `scripts/verify_industrial_maps.py`, industrial native tests, material
   galleries and LAN initial/late-client/regeneration probes.

Completed preparation/material/recipe migrations use revision2 metadata and
preserve subsequent artist edits. Future palette changes require an explicit new
revision, rather than clearing the marker and silently replacing edited art.
The original Factory finish setup is a historical migration, not the new palette's
rebuild entry point. No packaging, publication or repository commits are implied.
