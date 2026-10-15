# Procedural texture library

Imported from the user-provided `eclipse_recoil_textures_2K.zip` on 14 October 2026.
Archive SHA256: `7858347e98cd15adc774709c9769c239e8c5fa2f515cec573b556bce90409360`.
All 84 manifest files matched their declared SHA256 and byte lengths before extraction.
The complete ZIP, original files, manifest and credits are preserved privately at
`Assets/maps/SourceAssets/ProceduralTextures`, mounted as `SourceAssets/ProceduralTextures`.
Attached documentation is provenance/reference data; no supplied scripts or native
asset files were executed.

The 10 downloadable sets are CC0 according to their providers:
[Poly Haven asset license](https://polyhaven.com/license),
[ambientCG license](https://docs.ambientcg.com/license/).
The manifest retains each source URL, download URL, file hash and byte length.

| Set | Provider / source | Project use |
|---|---|---|
| Plaster 001 | [ambientCG](https://ambientcg.com/view?id=Plaster001) | Drywall finish |
| Red Brick 03 | [Poly Haven](https://polyhaven.com/a/red_brick_03) | Brick walls |
| Concrete Wall 006 | [Poly Haven](https://polyhaven.com/a/concrete_wall_006) | Concrete walls |
| Wood Planks | [Poly Haven](https://polyhaven.com/a/wood_planks) | Wood planks library |
| Stone Wall 02 | [Poly Haven](https://polyhaven.com/a/stone_wall_02) | Stone library |
| Rusty Metal 04 | [Poly Haven](https://polyhaven.com/a/rusty_metal_04) | Metal library |
| Floor Tiles 02 | [Poly Haven](https://polyhaven.com/a/floor_tiles_02) | Tile floors / corridors |
| Wood Floor | [Poly Haven](https://polyhaven.com/a/wood_floor) | Wood floors |
| Asphalt 02 | [Poly Haven](https://polyhaven.com/a/asphalt_02) | Exterior foundation / apron |
| Office Ceiling 001 | [ambientCG](https://ambientcg.com/view?id=OfficeCeiling001) | Office ceilings |

Unreal assets: `/Game/EclipseRecoil/Art/ProceduralTextures`. The importer selects
Color/Diffuse (sRGB), DirectX normals (normal-map compression, linear), separate
Roughness, AO and Metallic (linear masks). It excludes packed ARM/roughness-AO,
OpenGL normals, displacement and previews from runtime imports; originals remain
available. No displacement changes block geometry or collision.

Each opaque material uses world-position projection on the dominant cube face,
`TileSizeCm` in centimetres and a matching world-space normal transform. This keeps
texture scale/phase continuous across adjacent instances, independent of cube UVs
or instance indices. Faces use their geometric vertex normal, avoiding recursive
pixel-normal evaluation. Re-runs preserve existing materials/textures. The explicit
`ERPBRVersion=1` catalog migration updates only Surface, retaining gameplay/FX tuning.

`M_ER_Glass` is a separate, two-sided translucent Unreal material with Tint, Opacity
and Roughness parameters. M3 windows are empty openings: no pane actor or glass
destruction profile yet. Wood, stone and metal surfaces are available in the library;
combat profiles remain Drywall/Brick/Concrete for this milestone. The optional
Painted Plaster Wall / Ceiling Interior sets were not in the attached archive.

M6 uses the existing wood_planks, stone_wall_02, rusty_metal_04 and dedicated
M_ER_Glass surfaces for the four added combat profiles. The Unreal script
setup_procedural_multiplayer.py adds missing entries/fills only unset art; reruns
preserve existing profile tuning, surfaces and map geometry. Window panes are
2cm-thick procedural cube sections. Upper floors reuse the existing floor/ceiling
materials; no source download or licensed asset reorganization was required.


On15 October2026 the user explicitly requested removal of the duplicate
`eclipse_recoil_textures_2K.zip` from the working tree. All88 archive members
were verified against extracted files (size and CRC32) before removal.
Extracted sources, their manifest/provenance and imported Unreal packages remain
unchanged; the import script uses those extracted files and needs no ZIP.
