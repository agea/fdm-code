# Middle Eastern Alleys

Shared rules are in [MAPS.md](../MAPS.md).

**Code:** `ERAlleyDistrict.cpp`, `ERAlleyHouseModule.cpp`, `ERAlleyCover.cpp`, `ERAlleyUnderpass.cpp`, `ERAlleyDestruction`, `ERDustTown.cpp`.

Original seeded districts informed by a user minimap and *Making of Dust 2*. Generates 2–4 main
approaches, winding streets, plazas, and 0–2 central walls with 2–4 arches. Existing separators
have paired two-sided ladders and a walkable crown, with climb aprons reserved before lot
partitioning. Teams use sand terraces up to +1.8m.

- `Town.ProceduralHouses`: native masonry, alternating Stone and weaker pale Brick pairs, 160×240cm doors, flat 15cm slabs, windows, flat
  roofs flush or with 25cm-high, 40cm-wide rims and 180cm gaps. Exterior switchback stairs. One hollow
  skin per house; floor and roof skins are separate bounded panels, not merged into wall panels.
  Keep parent structure tags and support contacts.
- Lower passage at -3.6m with paired ramps and two breachable brick road sections, over real
  terrain holes, kept clear of the bridge stairs and piers (a plan that would cover them is
  rejected and retried). Low cover: masonry 65–90cm, timber crates 110cm.
- Watchtowers 700–900cm with ladders. Perimeter, terrain and stair foundations stay permanent.

## Verify

`test-dust-template.py --alleys` (`--architecture`, `--render`, `--network`, `--plans`, `--passage`, `--modules`). Native: `EclipseRecoil.Destruction.MiddleEastern*`.
