#!/usr/bin/env python3
"""Author a desert reference arena with invariant grade crossing and door gaps.

Geometry is a balanced procedural adaptation of Workshop 3117367319, not a
lossless CS2 conversion. All construction data stays in the private maps mount.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Assets/maps/SourceAssets/MapTemplates'
ART = '/Game/EclipseRecoil/Art/ArabianEnvironment/Meshes/ModularAssets/'
pieces = []


def box(center, size, role, material='Stone', fixed=True, mesh=None,
        group=0, jitter=(0, 0), mirrored=True, walkable=False, yaw=0):
    pieces.append(dict(center=center, size=size, role=role, material=material,
                       destructible=not fixed, fixed_mesh=ART + mesh if mesh else None,
                       group=group, jitter=jitter, mirrored=mirrored,
                       walkable_surface=walkable, mesh_rotation=[0, yaw, 0]))


# Ground route runs north/south beneath a 3.2m east/west catwalk. Its 2.9m
# under-clearance and 4m width are independent of seed and all random cover.
box((0, 0, 305), (3800, 400, 30), 'crossing_deck', mirrored=False, walkable=True)
# Leave each stair mouth genuinely open; half-turn pairs the opposite gap.
box((125, -210, 370), (3550, 20, 100), 'crossing_parapet')
for x in [-650, 650]:
    box((x, -100, 145), (100, 200, 290), 'crossing_pier')
# Two opposing stair assemblies, <=20cm rise, real 40cm treads. The last
# tread meets the deck; neither stair occupies the lower central passage.
for i in range(16):
    box((-1775, -840 + (i + .5) * 40, (i + 1) * 10),
        (250, 40, (i + 1) * 20), 'crossing_stair', walkable=True)
box((-1935, -520, 210), (20, 640, 420), 'stair_sidewall')

# Each gateway offers a wide masonry portal but only a 110cm opening between
# its timber leaves. The leaves/jambs are fixed: the choke survives damage.
for gate, y in enumerate([-1350, -2450]):
    for lo, hi in [(-1200, -170), (170, 1200)]:
        # Tile the masonry, retaining native texel scale rather than stretching
        # one 2.5m wall across an entire facade.
        count = 5
        for i in range(count):
            a = lo + (hi - lo) * i / count
            b = lo + (hi - lo) * (i + 1) / count
            box(((a + b) / 2, y, 150), (b - a, 30, 300),
                'gateway_wall', mesh='SM_VillageKit_2x2')
    box((0, y, 290), (340, 30, 20), 'gateway_lintel')
    for sign in [-1, 1]:
        box((sign * 112.5, y, 135), (115, 30, 270), 'narrow_door_leaf',
            material='Wood', mesh='SM_VillageKit_Door_01a')

# Long side lanes and traversable courts distinguish this from a single shed.
# Solid urban facade panels remain outside the reserved outer circulation.
for y in [-3200, -2950, -2700, -2250, -2000, -1750, -1500, -1250, -1000]:
    box((-2220, y, 250), (30, 250, 500), 'village_facade',
        mesh='SM_VillageKit_2x2', yaw=90)
for y in [-3125, -1875]:
    box((1700, y, 150), (700, 30, 300), 'courtyard_wall',
        mesh='SM_VillageKit_2x2')

# Paired, bounded cover offsets. No random volume may enter doors, either
# crossing axis, stairs, spawn bands or the two long circulation lanes.
for group, (x, y) in enumerate([(-700, -1900), (650, -3050),
                               (1700, -2650), (-1700, -2950), (1050, -650)], 1):
    box((x, y, 62.5), (200, 140, 125), 'wood_crates', material='Wood',
        fixed=False, group=group, jitter=(100, 100))
    box((x + 25, y, 150), (100, 100, 50), 'crate_stack', material='Wood',
        fixed=False, group=group, jitter=(100, 100))
for group, (x, y) in enumerate([(-700, -3050), (700, -1900), (1650, -1100)], 20):
    box((x, y, 75), (350, 30, 150), 'destructible_low_wall',
        material='Concrete', fixed=False, group=group, jitter=(100, 100))

alpha = [dict(position=[(i % 5 - 2) * 200, -3650 + (i // 5) * 200, 100], yaw=90)
         for i in range(10)]
omega = [dict(position=[-s['position'][0], -s['position'][1], 100], yaw=270)
         for s in alpha]
data = dict(schema=1, id='dust_mirror_desert', revision=2,
            source='Workshop 3117367319 / Dust 2 (Mirror) / mirror_de_dust2.vpk',
            symmetry='HalfTurn', arena_size=[6000, 8000, 1200],
            spawns=dict(alpha=alpha, omega=omega),
            patrol_route=[[-2700, -3450, 100], [-2700, 0, 100],
                          [-2700, 3450, 100], [2700, 3450, 100],
                          [2700, 0, 100], [2700, -3450, 100]],
            shell_wall_mesh=ART + 'SM_VillageKit_2x2', roof_mesh=None,
            floor_surface='/Game/EclipseRecoil/Art/ArabianEnvironment/Materials/SubstanceTilingMaterials/M_StoneFloor_SBSAR/M_StoneFloor',
            invariants={'crossing_top_cm': 320, 'crossing_clearance_cm': 290,
                        'crossing_width_cm': 400, 'door_gap_cm': 110,
                        'door_height_cm': 270, 'stair_rise_cm': 20,
                        'variation': 'Paired cover assembly offsets on a 50cm grid; topology and dimensions fixed.'},
            pieces=pieces)

volumes = []
for i, p in enumerate(pieces):
    for sign in ([1, -1] if p['mirrored'] else [1]):
        c = [p['center'][0] * sign, p['center'][1] * sign, p['center'][2]]
        for j, q, size in volumes:
            overlap = [min(c[k] + p['size'][k] / 2, q[k] + size[k] / 2)
                       - max(c[k] - p['size'][k] / 2, q[k] - size[k] / 2) for k in range(3)]
            if min(overlap) > .01: raise ValueError(('Overlapping pieces', i, j, p['role']))
        volumes.append((i, c, p['size']))
OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'dust_mirror_template.json').write_text(json.dumps(data, indent=2) + '\n')
print('DUST_MIRROR_MANIFEST', len(pieces), 'authored pieces;', len(volumes), 'paired volumes')
