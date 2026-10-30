# Murdock and Gadget field variants

> **Superseded (29 October 2026):** Phase and Murdock were removed. Gadget now
> occupies Omega indices 5..9 (Recon, Mechanic, Patrol, Field Engineer, Veteran)
> and each team has ten skins. See `scripts/replace_phase_with_gadget.py`.

Latest 30 October correction: **Omega Sniper selects gallery model8**, the
existing Gadget · Patrol at index7, keeping its light camouflage. Marksman6 and
Grenadier8 are unchanged. Veteran9 Shellshock remains optional and is no longer
assigned to Sniper. This supersedes the Sniper9 assignment described below.

30 October: **Marksman (Omega6, Mechanic) and Sniper (Omega9, Veteran)** now
use different body families. Marksman retains Guerilla, visible face and exposed
waist. Sniper uses the closed Shellshock armor/helmet from independent copies
of Engineer8's approved body, FPS forearms and ragdoll. Upper armor is steel gray;
lower armor/gear has muted Omega orange. Engineer8 itself and all other skins
are unchanged. Stable names, indices and loadout assignments are preserved.

`setup_gadget_sniper_shellshock.py` publishes only Omega9 in the three catalogs.
It clones the approved material graphs before tinting, retains source sockets,
world-support offset, scale, animation and skeleton bindings, and keeps the
source machine/backpack/robot hiding policy. `verify_gadget_sniper_shellshock.py`
checks1526 previous package hashes, all other appearance fields, five body LODs,
material/pose bindings and exact cloned grip/palm sockets. Receipts and previews
are in `Saved/Testing/GadgetMarksmanSniper`. This supersedes Veteran's exposed
waist and Guerilla source in the historical description below.

Imported29 October2026 from the user's Fab downloads in
`../_imports/ParagonStaging`. Originals stay there. The private characters
submodule contains535 source packages from the skeletal-mesh dependency closure,
not the complete demos, particle effects and original animation libraries.
The original `/Game/ParagonMurdock` and `/Game/ParagonGadget` namespaces are
mounted by `assets.json`; owned derivatives live in `Characters/Teams`.

The original ten choices per team retain their indices, meshes, materials, rigs,
cloth and FPS packages. New choices append at indices10..14 (gallery11..15):

| Index | Alpha: Murdock | Omega: Gadget |
|---|---|---|
| 10 | Skull | Mechanic |
| 11 | Infantry | Patrol |
| 12 | Recon | Recon |
| 13 | Patrol | Field Engineer |
| 14 | Veteran | Veteran |

The revised Murdock set uses four source bodies: Merc (Skull/Veteran), base
(Infantry), Corrupt (Recon) and Shogun (Patrol), with blue/gray field finishes.
Skull keeps the Merc skull artwork; Veteran omits the separate Merc mask.
Recon replaces the Corrupt head with the complete standard helmeted Murdock
head, extracted in head-bone space and attached to that bone. Its exposed arm
uses a neutral skin finish instead of the original fiery surface. Patrol hides
the independent Shogun hip ornaments and packs. These are adaptations of the
supplied geometry; sculpted armor remains part of the bodies.

Gadget uses Guerilla, Winter, base and Shellshock clothing, with muted orange,
brown and neutral finishes. The authored alpha skin masks preserve face, arms
and the exposed abdomen independently of clothing recoloring. Both upperbody
and lowerbody atlases contain skin: preserving only the former had incorrectly
darkened the waist. Mechanic, Patrol, Recon and Veteran show the original exposed
waist; Field Engineer retains the closed suit.

Original weapon sections, Murdock's extra weapon mechanism, Gadget's machine
base/arm and robot props are hidden. Hidden material slots bind the existing
texture-free material rather than carrying weapon material dependencies.
Back-mounted accessories are suppressed where they have independent bones;
ordinary clothing/armor and field pouches remain part of the supplied bodies.
All new visible surfaces disable hero emissive effects and vertex offsets and
use matte finishes. The Recon head attachment has no collision and follows the
same bone during animation and ragdoll.

Both heroes have separate Manny rifle retargets, fitted anatomical ragdolls,
existing weapon grip sockets and extracted first-person forearms on the shared
FPS rig. Original vendor packages are not edited. Catalogs for player, targets
and corpses receive the same30 appearances. Existing role/loadout assignments
remain as authored; the new appearances are available in the extended catalog.
The full stationary-target gallery now preserves each explicit skin through
spawn/respawn instead of replacing it with a random role's appearance.

Generation order (Unreal Python commandlets):

1. `setup_murdock_gadget.py`: create derivatives and append catalog entries.
2. `refine_murdock_gadget.py`: reviewed palettes and mesh-specific hidden bones.
3. `finalize_murdock_gadget.py`: clean animation preview references and unused
   vendor physics-helper tracks.
4. `setup_murdock_gadget_variety.py`: replace Recon/Patrol with distinct owned
   bodies and FPS forearms, unmask Veteran, preserve Gadget's lowerbody skin
   mask and measure palm frames via `setup_murdock_gadget_palms.py`.
5. `setup_murdock_recon_human_head.py` with `-EnablePlugins=GeometryScripting`:
   replace the Corrupt head with the complete standard helmeted head and remove
   the fiery exposed-arm color. Run this after the variety migration.
6. `trim_murdock_details.py` with `-EnablePlugins=GeometryScripting`: Skull,
   Infantry and Veteran use Patrol's dark trouser material (shared Legs UVs; the
   beige Merc/base trouser textures ignore `ER_GroundedTint`); right-forearm hoses,
   wrist cover and gauntlet mechanism (`arm_hose_top/bott`, `wrist_cover`,
   `bot_loarm`) are hidden on all five, with FPS forearms rebuilt and palm
   sockets preserved; the Merc left pauldron (shoulder half of the left-arm armor
   island) is deleted from the Skull/Veteran mesh copies. `bot_hand` stays: it
   carries the mechanical right palm. Backup: `../_backups/murdock-trim-20261029`.
7. `verify_murdock_gadget.py`: fresh catalog/FPS/material/head checks and protected
   old-art SHA256 comparison.

Setup/finish metadata protects later edits. Restore the staging inputs before
rebuilding if these are archived in future. Backup of the three previous
catalogs and the protected art hashes:
`../_backups/murdock-gadget-20261029`.
The first five-variant revision is separately recoverable from
`../_backups/murdock-gadget-variety-20261029`.

Validation entry points: `test-team-skins.py --new-paragon` (one gallery page,
five corpse pairs); `test-target-skins.py` (30 targets and a real LAN client).
The target probe measures the authored world-palm contact for world weapon poses;
the separate FPS contact arrays are used only for legacy presentation. Its1cm
contact tolerance and both-side identity/visibility checks remain enabled.

The initial import passed full and incremental Mac cooks; its logs remain in
Saved/Testing/MurdockGadget/. The later body/abdomen revision has separate fresh
catalog/FPS checks, Metal gallery/corpse tests and LAN validation recorded in
TEST_RESULTS.md and Saved/Testing/MurdockGadgetVariety/. It has not received a
new packaged-build run.
