# Asset licensing and public repository review

Review date: 14 October 2026. This is a provenance/distribution assessment,
not confirmation of the terms accepted in the owner's Fab account. Public
listing text returns an empty license field for these packs. Retain the
actual acquisition license/receipt with the private asset archive.

| Content | Publisher and source | Current conclusion for raw public distribution |
| --- | --- | --- |
| Mercenary | Waveforce Studio, [AAA Modular Mercenary System](https://www.fab.com/listings/57082fbf-3f85-407e-98ef-d858c2eefba7) | Vendor geometry/materials now unused. Preserved small fitted rig/pose derivatives keep their provenance and remain private pending acquisition terms. |
| Twinblast | Epic Games, [Paragon: Twinblast](https://www.fab.com/listings/9fa88852-5711-42e1-94fa-2491498a64da) | Keep source models, textures and derivatives private pending acquisition terms. |
| Drongo | Epic Games, [Paragon: Drongo](https://www.fab.com/listings/40e3af76-9b2d-4680-81b6-606f6d9269ef) | Same treatment as Twinblast. |
| Low-poly arsenal | Simon Mercuzot, [Low Poly Weapons VOL.1](https://www.fab.com/listings/a1dde692-c321-4c0e-9835-06cf335e2e57) | Acquisition license not recorded; keep FBX, palette and imported meshes private. |
| Interactive smoke | Theos, [Bullet Interactive Smoke](https://www.fab.com/listings/34604c10-bd96-44be-a2be-8b8233774eac) | Acquisition license not recorded; keep original/derived Niagara, materials and archive private. |
| Explosion/debris | Paul Yang, [Bomb Crack_Ground_00 (Free)](https://www.fab.com/listings/ad281506-98a2-40b7-a5e8-f3fde43870b1) | Potential exception: the [creator's Sketchfab listing](https://sketchfab.com/3d-models/bomb-crack-ground-00-free-c5cb1a953d1747b0af697c004d74735d) reports CC Attribution. Confirm the exact version and that it covers the same files before public redistribution. |
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
