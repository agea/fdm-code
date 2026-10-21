# Asset licensing and public repository review

Review date: 14 October 2026. This is a provenance/distribution assessment,
not confirmation of the terms accepted in the owner's Fab account. Public
listing text returns an empty license field for these packs. Retain the
actual acquisition license/receipt with the private asset archive.

| Content | Publisher and source | Current conclusion for raw public distribution |
| --- | --- | --- |
| Menu music | Project owner supplied `RR-Menu-Loop.wav`, 19 October 2026 | Original WAV and imported looping SoundWave remain in private maps; supplied for in-game menu use. No standalone redistribution license asserted. |
| Mercenary | Waveforce Studio, [AAA Modular Mercenary System](https://www.fab.com/listings/57082fbf-3f85-407e-98ef-d858c2eefba7) | Vendor geometry/materials now unused. Preserved small fitted rig/pose derivatives keep their provenance and remain private pending acquisition terms. |
| Twinblast | Epic Games, [Paragon: Twinblast](https://www.fab.com/listings/9fa88852-5711-42e1-94fa-2491498a64da) | Keep source models, textures and derivatives private pending acquisition terms. |
| Drongo | Epic Games, [Paragon: Drongo](https://www.fab.com/listings/40e3af76-9b2d-4680-81b6-606f6d9269ef) | Same treatment as Twinblast. |
| Low-poly arsenal | Simon Mercuzot, [Low Poly Weapons VOL.1](https://www.fab.com/listings/a1dde692-c321-4c0e-9835-06cf335e2e57) | Acquisition license not recorded; keep FBX, palette and imported meshes private. |
| Interactive smoke | Theos, [Bullet Interactive Smoke](https://www.fab.com/listings/34604c10-bd96-44be-a2be-8b8233774eac) | Acquisition license not recorded; keep original/derived Niagara, materials and archive private. |
| Explosion/debris | Paul Yang, [Bomb Crack_Ground_00 (Free)](https://www.fab.com/listings/ad281506-98a2-40b7-a5e8-f3fde43870b1) | Potential exception: the [creator's Sketchfab listing](https://sketchfab.com/3d-models/bomb-crack-ground-00-free-c5cb1a953d1747b0af697c004d74735d) reports CC Attribution. Confirm the exact version and that it covers the same files before public redistribution. |
| Content Examples effects | Epic Games, ContentExamples UE 5.8 downloaded by the user on 16 October 2026 | Twelve packages migrated through Unreal into the private weapons repository; original source project preserved. Provenance and integration receipts in Assets/weapons/SourceAssets/EpicContentExamples. No new public asset distribution. |
| Unreal template | Epic Games, installed UE 5.8.3 First Person/Shooter template | Examples have separate distribution permission; do not apply Fab restrictions indiscriminately to templates. |
| Project C++/Python/configuration | Project-owned work plus retained template source | Suitable for a separate public code tree after choosing the owner's code license and preserving template notices. This is not an engine-source redistribution. |

Fab Standard permits project use/modification and sharing with project
collaborators, including a private repository; its summary forbids standalone
asset redistribution. A public editable asset download is not equivalent to
a packaged game. A private Drive folder follows the same recipient restrictions
as a private repository. Storage location does not create distribution rights.
[Fab license summary](https://www.fab.com/eula).

Migrated products can still use the legacy UE Marketplace license. A current
listing does not establish what was accepted when a file was obtained.
[Fab license documentation](https://dev.epicgames.com/documentation/fab/licenses-and-pricing-in-fab).

Under the Epic Content License Agreement, project distribution and source
content sharing have distinct rules. Sections 3–4 limit editable/source sharing
to specified development relationships; section 5 also covers designated
UE-only content and incompatible copyleft licenses. Recoloring, trimming a mesh
or creating a derivative does not erase the underlying content rights.
[Epic Content License Agreement](https://www.unrealengine.com/eula/content).

The Unreal Engine EULA defines Examples as Samples/Templates content and
permits their distribution under section 5(b). Engine source, Starter Content
and separately acquired Fab content have different rules. Retain Epic notices;
do not place third-party content under the project's own license.
[Unreal Engine EULA, sections 1 and 5](https://www.unrealengine.com/eula/unreal).

CC-BY can permit redistribution and adaptation with attribution, a license
link and an indication of changes. For the explosion, document Paul Yang,
the source model, the actual license version, and our baked flying fragments
with the source floor excluded. The [CC-BY 4.0 summary](https://creativecommons.org/licenses/by/4.0/)
illustrates these requirements; version 4.0 has not been established for our
download by this review. No blanket public asset clearance has been inferred.

Recommended split: public code/config/scripts/docs, private complete Content
tree, private original download/rebuild archive. Original public-domain or
permissively licensed project artwork can be moved to the public tree later
after a per-file review. Maps and Blueprints may contain only owned logic and
references, but keeping the whole Content tree private initially avoids mixing
them with restricted derivatives.

No repository visibility, remote permissions or uploads were changed.


## CC0 procedural texture sources — 14 October 2026

The user-provided 2K ZIP adds ten CC0 downloadable texture sets from Poly Haven
and ambientCG. Source pages, full manifest hashes, private archive location and
import transformations are recorded in [the texture inventory](PROCEDURAL_TEXTURES.md).
Verified against [Poly Haven](https://polyhaven.com/license) and
[ambientCG](https://docs.ambientcg.com/license/). Original texture files and Unreal
packages remain in the private maps submodule under the existing asset split.

15 October M6: Wood/Stone/Metal combat surfaces reuse the previously documented
CC0 procedural texture library; Glass reuses the project-authored material.
Procedural stairs/slabs/panes reuse Unreal's normalized engine cube. No new
external art provenance or redistribution scope was introduced; modified
catalog binaries remain private under the existing maps asset ownership.


## Factory Environment Collection source — 15 October 2026

The user supplied the completed local FactoryEnvironmentCollect project and
explicitly authorized its industrial assets for fixed Killhouse geometry.
Its source project records UE5.6; the selected packages were migrated with the
installed UE5.8.3 Editor into the existing private maps submodule. Local source,
selected dependency closure and fitted runtime uses are documented in
SourceAssets/FactoryEnvironment and ASSETS.md. The acquisition receipt/license
variant was not inspected during this integration. All vendor originals and
derivatives remain private; no public asset publication or license clearance
is inferred. The project's CC0 procedural destruction materials are retained.


## MOUT_Arabian reference art — 15 October 2026

The user explicitly authorized using the urban/desert assets imported into
FactoryEnvironmentCollect for the Dust-inspired template. A27-package selected
closure remains in the existing private maps submodule under
Art/ArabianEnvironment, with provenance and SHA256 receipt in private
SourceAssets/ArabianEnvironment. Acquisition/license terms were not inspected
during integration; no publication or license clearance is inferred.
Workshop3117367319 Source2 resources are read-only reference data and local
ignored analysis exports; their meshes/textures are not imported into Unreal.
Existing project destruction materials and gameplay profiles remain unchanged.


Dust town revision3: the owned arch/curved door OBJ sources and five town
material graphs were authored for this project. Texture references reuse the
previously documented private MOUT_Arabian selection and CC0 procedural library.
The user's reference plan and doorway photograph are retained privately as
reference/provenance only. No Source2 model/texture or external media copy is
added to public code; no publication or new license review is claimed.


## Sketchfab arsenal acquired 15 October 2026

The nine model URLs in WEAPONS_REFERENCE_EN.md were downloaded in original
format through the owner's existing Sketchfab session for this local project.
Metadata and exact license objects are retained per model in the private weapons
SourceAssets/SketchfabWeapons tree, with archive hashes and authors' underlying
credit links. Seven listings report CC BY4.0; the Milkor composite reports
CC BY-NC4.0 (weapon JaumWF, arms/animation DJMaesen, uploader user77), and the
grenade set reports Sketchfab free standard terms. Keep each model's own terms;
this acquisition does not imply a blanket public or commercial clearance.
Derived skeletal FBX, weapon packages and fitted hand-animation adaptations
remain private with their source provenance. See WEAPON_IMPORT.md.

## Grenade explosion audio — 16 October 2026

[Distant Explosion of Propelled Grenade](https://freesound.org/people/qubodup/sounds/162363/)
by qubodup: the sound page lists CC0 1.0. Credit retained voluntarily.
Imported the public HQ MP3 preview, converted to mono44.1kHz PCM16 WAV
(0.601s) without gain changes; the login-only original FLAC was not acquired.
Downloaded preview, WAV, source-page snapshot, hashes and credit are retained
in private `Assets/weapons/SourceAssets/Freesound/162363`. Derived Unreal assets
are under `/Game/EclipseRecoil/Effects/Grenade/Audio`.

## Sonniss GDC 2024 audio supplied by user (17 October 2026)
grenade1.wav and smoke_1.wav supplied directly from the user’s Sonniss GDC 2024
bundle folder. Originals and derivatives remain in the private weapons mount,
SourceAssets/SonnissGDC2024. No separate license receipt supplied or verified
in this change. The previous Freesound source and credit remain historical.


Recon assets supplied19 October2026: [S9 drone](https://skfb.ly/6SSyD) by
blockv_lb and [Pelican Case Game Ready](https://skfb.ly/6xwPJ) by polbrainstorm.
Both original credits.txt files identify CC BY4.0. Originals, archives,
textures, credits and modified static derivatives remain in the private maps
ReconDrones source folder. Changes and attribution are recorded in
MINIMAP_RECON.md, private CREDITS.txt/import receipt, asset metadata and the game
menu. This preserves supplied provenance without claiming a separate online
license verification or performing publication.

## Audio pass — 21 October 2026

28 real single-shot firearm takes and 41 step samples are imported under
`/Game/EclipseRecoil/Audio/`, owned by the private weapons submodule. Originals,
creator-page captures, included license/attribution files, archive/source/derived
SHA-256s and exact preparation manifest are preserved in
`Assets/weapons/SourceAssets/AudioPass`. See [AUDIO_PASS.md](AUDIO_PASS.md).

The Free Firearm Sound Library (Ben Jaszczak, Brian Nelson, Kevin Heras, Matthew
Nanney) and TinyWorlds' stone sample are declared CC0. The imported concrete,
wood, grass and metal steps from congusbongus's pack retain swuing/Eelke CC BY3.0
attribution; its gravel recordings credit Ali_6868 under CC0. All required
author/source/license links and edits appear in the private CREDITS/license
files and public `Branding/Credits.txt` shown in-game. Derivatives are trimmed,
converted to mono PCM48kHz, level matched by attenuation, and end faded.

Thimras' CC0 metal-foot recordings remain source-only. Tabasco's ZIP is not
imported: the page's CC0 label conflicts with the included Copyright2009 Vincent
Sevedge / CC BY3.0 notice. Both records are preserved; no CC0 clearance is
asserted for that pack. The launcher retains the existing Epic template sample.
No new audio binaries belong in public core. Sonniss HE/smoke original/runtime
WAVs are unchanged; smoke reduction is a local playback gain only.
