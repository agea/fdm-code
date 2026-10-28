# Village · temi condivisi

Village (`DA_Village`, `L_Village`, catalog ID `village`) follows the owner's
Shoots references, including the revised top-down plan and game view in private
`SourceAssets/Village/references`. The source Shoots scene is a private visual
reference; its geometry is not imported into the game.

Village now shares Industrial, Desert and Jungle with Central Building, Alleys,
Fire Training and Rooms. Every template uses the same cosmetic seed stream,
independent of geometry. Seed0 selects Jungle, seed1 Desert and seed3 Industrial.
Changing the palette affects finishes and ground. Jungle retains Village's
original imported UV skins and gray cladding; the other palettes recolor those
surfaces too. Trees and the panorama retain their authored slots. Exterior and
playable ground use the same sampled terrain finish. The legacy `Village.Theme`
field remains serialized for compatibility and no longer controls selection.

## Topology

Recipe8 samples one to five central blocks. Even counts use half-turn pairs;
odd counts add one block at the origin, internally symmetric under a180-degree
turn. Paired blocks may be staggered by up to3m horizontally. Width varies
20–30m, depth6–8m, independently between pairs. Opposing pairs always share
size, height, access and cover. Arena width/length are sampled separately and
follow the occupied footprint, preserving room for bases, stairs and flanks.
Configured limits are34–56 ×44–132m; ordinary small variants use the lower
length range instead of surrounding one block with a long empty arena.

Each block has one to three platform floors, starting at2.4m and separated by
3m. Every level has two enterable huts, gray timber cover and a low store.
Intermediate levels support the next deck; only the top level has gable roofs.
Upper floors have two timber ladders with real openings in the deck, beside
the huts, clear of their doors. Successive flights alternate front/back so
the next ladder and its supports cannot obstruct an arriving climber. A maximum of ten central platform floors across
the whole arena bounds construction growth. This is a generation limit, not
a measured GPU/FPS guarantee.

Each team receives the same base variant: open firing gallery, a gallery with
a metal canopy, or an enclosed wooden barracks with a central through-door.
Gallery width/depth and elevation vary, along with lookout height and side
shack dimensions. Every base retains two climbable spawn structures (rear
gallery and corner lookout), a side shack with roof access, and ten supported
starts. Exterior decorative houses remain removed. Bridge-pack module20 forms
the lookout decks; editable modular timber forms the central gameplay blocks.

Cross lanes are reserved between blocks. With an odd central block the cross
lane moves away fromY=0; the flanks still connect both bases. Native tests use
the generated frame/dimensions and do not assume a square arena or two blocks.
Competitive balance still requires human playtesting.

## Construction and boundary

Each central ground block contains three enclosed timber bays around two
angled passages. The original dogleg proportions scale with the block width
and depth; openings remain wide enough for the standing capsule at the20m
minimum. The odd origin block reverses its second dogleg to preserve its own
half-turn symmetry.12cm timber walls reach the deck underside, with massive
corner posts and doorway frames. Upper hut doors stay open. Ordinary damage
can subsequently breach the closed bays. Ground-only outlines/routes export
as `Saved/Testing/Village-GroundPlan.svg`; brown is closed, hatching is a corridor.

Lateral stops are now2.4m tall (previously1.25m), built from24cm massive
wood with2mm metal faces and a top cap. One-block variants have four7m-long
walls; two-block variants have four5.8m walls; denser maps retain two2.6m walls.
Every wall has a standing route around its inner end. Sparse approaches reserve
another3m per team so the wider barriers and turns stay clear of stair entries,
lookouts and side shacks. Half-turn symmetry and normal destruction apply.
The native manifest includes their actual bounds, and the ground plans show
them in gray. Standing eye-height and higher220cm traces test concealment.

Walls are 8cm timber, columns 20cm, decks 6cm planks and metal sheets 2mm.
Massive timber contains multiple calibrated 2cm wood depth cells, preserving
shared HP, support and penetration. The gray covers have massive 24cm timber
cores, gray metal faces/caps and subtle corrugation/seams. Their color does not
change their underlying physical material. Upper timber/metal uses cell
support and destruction; matching stair treads remain fixed.

The old continuous fence is replaced by original tapered logs, 3.8–4.4m high,
with varied heights and tangential lean up to 2 degrees. Pole diameters 24–28cm
on at most 32cm pitch keep visible gaps below the standing/prone capsule width,
even at their tops. A hidden continuation to at least11m (raised for tall variants) at the same rectangle blocks
locomotion and dynamic bodies only. It does not block visibility/shot traces;
the actual logs provide cover, and cracks remain real apertures. The barrier
never projects into playable space and persists after destruction.

## Background and art

64 exterior instances use Ricardo Sanchez's supplied 850-triangle tree, with
original bark and masked leaf textures. They have no collision/shadows, cull
at 70–120m and sit completely outside the arena.

The linked `sky dome demo` describes its source as Poly Haven's
[J&E Gray Park](https://polyhaven.com/a/je_gray_park), by Greg Zaal, CC0. The
original 8192× 4096 tonemapped panorama is preserved with URL/hash and reduced
to 4096×2048 for the game. It is mapped onto an original 960-triangle sky sphere
with an unlit sky material, no collision, shadows or distance culling. A
query-free flat ground extends to the distant backdrop so aerial views do not
reveal the edge of the small arena ground. The sphere, trees and exterior
ground are omitted on dedicated servers. The Sketchfab dome mesh itself is
not imported. This bounds asset work; FPS/GPU costs remain unmeasured.

The five owner-supplied ZIPs remain byte-identical under private model-id
folders, with extracted originals, public metadata, licences, source manifests
and SHA256 receipts. Runtime content is under `Art/Village/Imported` and
`Art/Village/Topology`; there is no runtime dependency on Downloads.

| Source | Author | Current use |
| --- | --- | --- |
| Jungle House / Rusty Roof Wooden House | iGauravRajput | Optimized imported derivatives preserved; no exterior instances. |
| Old Building | YadroGames | Paired 399-triangle hollow side shacks; UV-preserving through-door cuts and cleared passage, replaced roof. |
| Wooden Bridge Pack | padmadev_designs | Module 20, 396 triangles, adapted as paired lookout platforms. |
| Tree 4cd243… | Ricardo Sanchez | 850 triangles, 64 exterior instances. |

Those five listings report CC BY 4.0. Author links, licences and changes are in
`Branding/Credits.txt` and private `CREDITS.txt`. Jungle's shared wood, rusty
metal and grass reuse installed sources with their existing credits.

## Authoring and validation

`setup_village.py` creates absent original assets. `bake_village_assets.py`
and `setup_village_assets.py` prepare the supplied models. The current
`bake_village_backdrop.py` and `setup_village_topology.py` add logs/panorama,
clear exterior house references and migrate the owned recipe to revision 5. `setup_village_passages.py` adds the
subsequent ground-corridor revision6 with its own verified backup.
`setup_village_variety.py` advances only the Village recipe to revision7, keeping
the revision6 package in a separate verified rollback ZIP.
`setup_village_side_walls.py` then advances only DA_Village to revision8 with
an independent rollback of revision7 and protected other-package hashes.
Run Blender bakes with its background runner and Unreal imports using
`-ExecutePythonScript` in the full editor. All package saves use Unreal APIs.
The scripts retain verified rollback ZIPs and protect other recipes/maps,
the shared themes and damage catalog. Import markers preserve later edits.

`test-village.py` checks 516 layouts and fifteen physical seeds spanning all counts, base styles and240/260/280cm deck heights. `--offscreen`
adds real pawn ascent of central/gallery/lookout stairs, a ground corridor and
a side-wall bypass and an upper-storey ladder, cover and imported
skin damage, settled support/regeneration and six Metal captures (Plan,
Aerial, Deck, Underpass, Spawn, SideCover). `--network` compares initial/late local LAN
clients after settled damage and regeneration. Native checks include crouched
cover access/LOS, raised spawn support, doors, ladder rungs/landings, flanks,
perimeter height and query-free upper boundary. Executed results are recorded
in `TEST_RESULTS.md` and private `validation.json`.

The `Village` struct on `DA_Village` exposes arena limits, minimum/maximum
block count, width/depth ranges, minimum/maximum storeys and total storey
budget. Set equal minimum/maximum counts to pin a particular family. Invalid
or spatially impossible recipes fail atomically. `test-village.py --offscreen
--seed 42 --next-seed 73` checks a compact three-storey centre and regenerates
a longer three-block plan. `Saved/Testing/village-variety.json` contains actual
layout dimensions, block frames and floor counts for the512 ordinary seeds.

The network descriptor accepts the existing96M skinned-architecture limit for
Village recipes with imported structures; ordinary rooms retain8M. Regeneration
still validates the cell world's storage/occupied-cell and chunk budgets before
opening collision. This allows tall Village variants to synchronize without
relaxing the general descriptor limit.

`render_village_variety.py` turns the native manifest into
`Saved/Testing/Village-Variety.svg`, comparing five real generated footprints.
Spawn bands are schematic and each card uses its own scale. Floor counts and
block dimensions/offsets come directly from the native layout output.

`setup_shared_jungle.py` completes Jungle's shell/ladder/deck roles with existing
Village gray corrugated sheets, galvanized access steel and dark deck plates.
Cargo paint stays distinct. The migration saves only `DA_MapThemes` (library
revision6), with a verified rollback and hashes for50 protected packages;
production recipes, maps and original materials do not need resaving.
