# Fire Training

23 October 2026. Select **Fire Training · Warehouse** in the match menu.
Template `Art/MapTemplates/DA_FireTraining`, level `Maps/L_FireTraining`,
archetype `FireTraining`. It joins Central Building and Middle Eastern Alleys
in the production rotation; Depot remains a reference.

The owner's specification `RR_Warehouse_Killhouse_OpenSpace_Istruzioni.md` and
the supplied `warehouse-fbx-model-free.zip` are kept privately in
`Assets/maps/SourceAssets/FireTraining`. The references (Call of Duty 4
Killhouse, Steam Workshop 526448928) guide pace and composition only: this is
an original seeded arena built from the supplied warehouse. No game assets,
outlines or spawn arrangements are copied.

## One open volume

The warehouse is always a single hall. Length, width and height come from
repeated kit modules; nothing is stretched and no storey is generated:

| Parameter | Values |
| --- | --- |
| Width | 1–4 naves of 16, 20 or 24 m (complete kits `W1600`, `W2000`, `W2400`): 16–96 m |
| Length | `BayCount` 14–36 × 4.2 m (about 59–151 m), floor 1300–7000 m², length ≤ 7 × width |
| Height | `HeightBandCount` 2–4 wall bands of 3 m (recipe accepts up to 5); ridge = eave + 2 m |

Revision 3 (owner request: wider, taller and longer halls) repeats the arched
nave side by side. Between naves there is no wall: an open column line (the
kit post on every rib boundary and band) under a steel gutter at the eave, with
hidden column volumes so routes, cargo and cover keep clear of the posts. Ends
repeat per nave. With an even nave count the start rows sit mid-nave instead
of on the central column line, and the front screen stays within ±6 m of them.
Larger halls reserve up to three empty plazas per half (10–18 m, one per
2000 m² of floor) before cargo and cover are placed; cargo grows with the floor
area and cover is spread across the whole central band. The bridge variant is
limited to single-nave halls. Wide halls are therefore flatter: the elevated
share drops to a few percent in the largest ones. `setup_fire_training_scale.py`
is the revision 3 migration (backup in `Saved/FireTrainingBackup/revision2`).

Bands only raise walls, posts and end walls (`EndBand`); the roof sits on the
last band. Interior structures are local: a perimeter catwalk at 3 m (open
steel rail or timber screen with slots), an optional 2.2 m bridge across the
middle linking both teams' catwalks, a closed timber corridor under the
catwalk, killhouse timber rooms (one or two, doors and windows), an optional
3 m deck room on top with a 1.6 m walkway, a 6 m lookout above it when the
hall has three bands, steel containers in the middle, stacked two high under a
9 m eave, with ladders, and low cover (crates, crate stacks, concrete
barriers, timber screens). Each half is sampled once and rotated 180°.

Budgets, measured on a 50 cm raster of the union of projections (overlaps
count once): walkable surfaces above the ground ≤ 30% (target 20%), every
horizontal closure ≤ 40%, and the largest region open to the roof ≥ 45%.
Narrow walkways (≤ 3 m wide) do not split that region because the volume above
them stays joined; wide decks do. `ContinuousUpperFloors` exists only to be
rejected. Every elevated area has two independent accesses (stair + ladder,
or two ladders); decks keep 2.5 m below the arched roof
(`ERProcedural::WarehouseRoofHeight`). Most direct start-to-start lines must
meet a solid ≥ 1.5 m (front timber screens, containers, rooms).

Revision 2 (same day, owner request: the hall was too small) doubled the
length range at the same widths. Interior features scale with it: the catwalk
covers up to 60% of its half (bridge variant 17–30 m), up to three room
blocks repeat along the east wall with 6–11 m passages, and halls of 70 m or
more get one or two extra containers per half. `setup_fire_training_length.py`
is the explicit migration: it backs up revision 1 to
`Saved/FireTrainingBackup/revision1`, validates 516 seeds and changes only the
recipe size fields and the revision.

Generation is server-side and deterministic. `ERProcedural::Build` retries up
to seven seeded attempts and then uses a verified reference layout (16 m,
22 bays, two bands, catwalk + corridor, deck room, one container); every peer
repeats the same attempts from the replicated seed. The attempt index is part
of the layout digest.

## Kit and materials

`scripts/bake_fire_training_kit.py` (Blender, source SHA-256 checked) splits
the 439 source parts by measured signature from the bay between ribs 20.9 m
and 25.1 m. The 4.2 m pitch is rib-centre to rib-centre; the irregular first
source interval is not used. Modules (metres, origin on the outer wall face,
bay centre, wall-band base or eave):

| Module | Content |
| --- | --- |
| `SideWallBay` | Corrugated sheet + girts, 3.8 × 3 m, repeated per bay and band |
| `Post` | One 20 cm post; three per inner rib boundary, two at the ends |
| `SideWallLamps` | Window light strips, top band only, visual |
| `RoofBay_W*` | Two arched skins + purlins between ribs |
| `RoofArch_W*` | Rib + knee brackets, once per boundary (N+1) |
| `RoofLamps_W*` | Hanging lamps, alternate bays, visual |
| `EndGround_W*` / `EndBand_W*` / `EndCrown_W*` | End wall: ground band, plain band, gable |
| `EndPortal` | Closed portal leaves and frame, visual |

Wider kits split the cross-section at the apex (roof, rib, gable) or beside
the portal (end bands) and bridge the cut with a straight insert; UVs are
extrapolated across the insert. The west side wall is turned 180° for the east
side. End sheets gained 1 cm of depth so their bounds are not zero; portal and
gable bases were raised 2.4 cm onto the floor.
`scripts/setup_fire_training.py` imports at unit scale (checking the measured
bounds), makes sheets, posts, ribs and roof collide exactly as drawn
(two-sided complex collision) and adds hidden backstops behind the side and
end walls; the hidden ceiling closes the top. Roof groups are tagged
`ERTemplateRoof`: the minimap capture hides them, recon flies 50 cm above the
eave inside the arch, and the cases search for ground under the eave.

Materials: `M_ER_WH_Metal` uses the source sheet normal map with a lighter
grey and partial metalness for the game's no-reflection profile;
`M_ER_WH_WetConcrete` projects the wet concrete colour/normal/roughness in
4 m world tiles; `M_ER_WH_LampEmissive` for lamp art. Timber, steel, deck and
container finishes reuse the Central Building palette. Unshadowed point lights
are added under each roof-lamp bay on clients only.

Destruction: timber walls, screens, crates, containers (Strength 8) and
ladders use the shared 5 cm cells, damage profiles and replication. The
envelope, steel decks, columns, stair blocks and rails are fixed.

## Provenance and licence

The archive carries no author or licence. Provenance, hashes and credits are
in `SourceAssets/FireTraining/{provenance.json,CREDITS.txt}` and the menu
credits. Confirm the licence before any public distribution.

## Regeneration and validation

```sh
python3 scripts/stage_fire_training_assets.py
/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python scripts/bake_fire_training_kit.py
scripts/build-editor.command
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/RespawnAndRage.uproject" -run=pythonscript -script="$PWD/scripts/setup_fire_training.py" \
  -nullrhi -nosound -unattended -NoSplash
python3 scripts/test-fire-training.py           # native layout + physics
python3 scripts/test-fire-training.py --render  # Metal captures in Saved/Testing
```

The setup validates 516 seeds before saving the template and refuses a
changed kit on an existing template (write a versioned migration instead).
Native `FireTrainingLayout` builds 516 greybox seeds (budgets, symmetry,
accesses, headroom, determinism, fallback, rejection of mixed kits and
continuous floors). `FireTrainingPhysics` generates eight seeds from the real
asset: unit-scale kit instances, closed envelope for pawns and bullets, roof
collision, raster check for raised coverage, ladders (rungs, climbing capsule,
landing floor), spawns, routes, patrol loop and destructible timber.

Reproducible travel URL:

```text
/Game/EclipseRecoil/Maps/L_FireTraining?MapSeed=42
```

Not yet verified: LAN initial/late clients on this map, bot behaviour inside
the rooms, packaged/cooked builds and performance measurements. Geometric
symmetry is not a claim of competitive balance; it needs playtesting.
