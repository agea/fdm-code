#!/usr/bin/env python3
"""Numeric model (pure Python, no Unreal) of HE grenades against a 5cm-cell wall, using the spec formula.

Wall-local axes: X along the wall (i), Y depth into the wall (j, layer 0 faces
the grenade), Z up (k). Exposure: ray from the origin to the exposed face of the
target cell, each intact cell crossed first multiplies exposure by the per-layer
transmission (pre-event snapshot, order independent). No physics, no art.
Outputs breach size, capsule fit, dirty chunks, greedy quads/boxes and delta bytes.
"""
import math, json, sys

def falloff(d, inner=25., outer=200., exp=2.):
    if d >= outer: return 0.
    if d <= inner: return 1.
    return ((outer - d) / (outer - inner)) ** exp

def cells_on_segment(a, b, sx, sy, sz, nx, ny, nz):
    """Amanatides-Woo DDA: grid cells crossed from a to b (exclusive of b's cell)."""
    out = []
    pos = [a[0] / sx, a[1] / sy, a[2] / sz]
    end = [b[0] / sx, b[1] / sy, b[2] / sz]
    cell = [math.floor(p) for p in pos]
    last = [math.floor(p) for p in end]
    d = [end[i] - pos[i] for i in range(3)]
    step = [1 if v > 0 else -1 for v in d]
    tmax, tdelta = [], []
    for i in range(3):
        if abs(d[i]) < 1e-12: tmax.append(math.inf); tdelta.append(math.inf)
        else:
            nb = cell[i] + (1 if d[i] > 0 else 0)
            tmax.append((nb - pos[i]) / d[i]); tdelta.append(abs(1 / d[i]))
    total = math.dist(a, b); t0 = 0.
    for _ in range(4096):
        if cell == last: break
        a_ = min(range(3), key=lambda i: tmax[i])
        t1 = min(tmax[a_], 1.)
        if 0 <= cell[0] < nx and 0 <= cell[1] < ny and 0 <= cell[2] < nz: out.append((tuple(cell), (t1 - t0) * total))
        if tmax[a_] > 1: break
        t0 = t1; cell[a_] += step[a_]; tmax[a_] += tdelta[a_]
    return out

def simulate(length, height, thickness, cell_depth, hp, mult, trans, origin, max_layers=None, cell=5., alive=None, health=None, mode='cell'):
    """mode 'cell': exposure *= trans per intact cell crossed (today's rule).
    mode 'path': exposure *= trans ** (cm crossed inside intact cells), trans per cm."""
    nx, nz = int(length / cell), int(height / cell)
    ny = max(1, round(thickness / cell_depth))
    sy = thickness / ny
    if alive is None: alive = [[[True] * nz for _ in range(ny)] for _ in range(nx)]
    if health is None: health = {}
    damage = {}
    candidates = 0
    ox, oy, oz = origin
    for i in range(nx):
        cx = (i + .5) * cell
        if abs(cx - ox) > 200: continue
        for k in range(nz):
            cz = (k + .5) * cell
            if abs(cz - oz) > 200: continue
            for j in range(ny):
                cy = (j + .5) * sy
                dist = math.dist((cx, cy, cz), origin)
                if dist >= 200: continue
                candidates += 1
                # Exposed-face sample: clamp origin into the cell, put it on the
                # face toward the origin (as ApplyWorldExplosion does today).
                lo = (i * cell, j * sy, k * cell); hi = ((i + 1) * cell, (j + 1) * sy, (k + 1) * cell)
                inset = [.25, 1e-4, .25]
                s = [min(max(origin[a], lo[a] + inset[a]), hi[a] - inset[a]) for a in range(3)]
                if not alive[i][j][k]: continue
                crossed = [(c, L) for c, L in cells_on_segment(origin, s, cell, sy, cell, nx, ny, nz) if c != (i, j, k)]
                e = 1.
                layers = 0
                for c, L in crossed:
                    if alive[c[0]][c[1]][c[2]]:
                        layers += 1
                        if max_layers is not None and layers > max_layers: e = 0; break
                        e *= trans if mode == 'cell' else trans ** L
                dmg = 400 * falloff(dist) * mult * e
                if dmg > 0: damage[(i, j, k)] = dmg
    removed = set()
    for c, v in damage.items():
        health[c] = health.get(c, hp) - v
        if health[c] <= 0: removed.add(c)
    for (i, j, k) in removed: alive[i][j][k] = False
    return dict(health=health, nx=nx, ny=ny, nz=nz, alive=alive, damaged=len(damage), removed=len(removed), candidates=candidates, sy=sy)

def through_mask(r):
    nx, ny, nz, alive = r['nx'], r['ny'], r['nz'], r['alive']
    return [[all(not alive[i][j][k] for j in range(ny)) for k in range(nz)] for i in range(nx)]

def stadium_fits(mask, radius, half_height, cell=5., floor_only=True, max_bottom=0.):
    """Does a vertical capsule silhouette fit in the through-hole?"""
    nx, nz = len(mask), len(mask[0])
    core = half_height - radius
    best = None
    for bottom_k in range(0, nz):
        bz = bottom_k * cell
        if floor_only and bz > max_bottom: break
        for ci in range(nx):
            cx = ci * cell
            ok = True
            for i in range(max(0, int((cx - radius) // cell) - 1), min(nx, int((cx + radius) // cell) + 2)):
                for k in range(bottom_k, min(nz, bottom_k + int(2 * half_height // cell) + 2)):
                    x0, x1, z0, z1 = i * cell, (i + 1) * cell, k * cell, (k + 1) * cell
                    # Distance from capsule core segment (x=cx, z in [bz+r, bz+2h-r]) to box.
                    za, zb = bz + radius, bz + radius + 2 * core
                    dx = max(x0 - cx, 0, cx - x1)
                    dz = max(z0 - zb, 0, za - z1)
                    if math.hypot(dx, dz) < radius - 1e-6 and not mask[i][k]: ok = False; break
                if not ok: break
            if ok and bz + 2 * half_height <= nz * cell: return bz
    return best

def greedy_rects(grid):
    """Greedy rectangle count over a 2D boolean grid."""
    w = len(grid); h = len(grid[0]) if w else 0
    used = [[False] * h for _ in range(w)]; n = 0
    for x in range(w):
        for y in range(h):
            if not grid[x][y] or used[x][y]: continue
            y1 = y
            while y1 + 1 < h and grid[x][y1 + 1] and not used[x][y1 + 1]: y1 += 1
            x1 = x
            while x1 + 1 < w and all(grid[x1 + 1][yy] and not used[x1 + 1][yy] for yy in range(y, y1 + 1)): x1 += 1
            for xx in range(x, x1 + 1):
                for yy in range(y, y1 + 1): used[xx][yy] = True
            n += 1
    return n

def chunk_stats(r, cx, cz):
    """Per dirty chunk (cx*cz cells on the face, full thickness): quads, boxes, delta bytes."""
    nx, ny, nz, alive = r['nx'], r['ny'], r['nz'], r['alive']
    out = []
    for bx in range(0, nx, cx):
        for bz in range(0, nz, cz):
            xs, zs = range(bx, min(nx, bx + cx)), range(bz, min(nz, bz + cz))
            removed = [(i, j, k) for i in xs for j in range(ny) for k in zs if not alive[i][j][k]]
            if not removed: continue
            A = lambda i, j, k: 0 <= i < nx and 0 <= j < ny and 0 <= k < nz and alive[i][j][k]
            quads = 0
            # +-Y faces per layer slice, +-X per column slice, +-Z per row slice.
            for j in range(ny):
                for dj in (-1, 1):
                    quads += greedy_rects([[A(i, j, k) and not A(i, j + dj, k) for k in zs] for i in xs])
            for i in xs:
                for di in (-1, 1):
                    quads += greedy_rects([[A(i, j, k) and not A(i + di, j, k) for k in zs] for j in range(ny)])
            for k in zs:
                for dk in (-1, 1):
                    quads += greedy_rects([[A(i, j, k) and not A(i, j, k + dk) for j in range(ny)] for i in xs])
            # Collision: greedy 2D rects per layer, then merge identical rects across layers.
            layer_rects = []
            for j in range(ny):
                g = [[A(i, j, k) for k in zs] for i in xs]
                layer_rects.append(greedy_rects(g))
            boxes = max(layer_rects) if layer_rects else 0  # upper bound when layers differ
            bits = len(xs) * len(zs) * ny
            seq = [alive[i][j][k] for j in range(ny) for k in zs for i in xs]
            runs = 1 + sum(1 for a, b in zip(seq, seq[1:]) if a != b)
            rle = 4 + runs * 1  # 4B chunk header + ~1B varint per run (runs < 128 cells)
            out.append(dict(removed=len(removed), quads=quads, boxes_upper=sum(layer_rects), boxes_merged=boxes,
                            mask_bytes=4 + bits // 8, rle_bytes=rle, list_bytes=4 + 2 * len(removed)))
    return out

MATERIALS = {
    # name: hp, explosion mult, transmission per 20cm block today
    'Drywall': (20, 2.0, .50), 'Brick': (100, 1.5, .25), 'Concrete': (600, .5, .05),
    'Wood': (40, 1.5, .50), 'Stone': (1000, .2, .02), 'Metal': (100, 1.0, .50),
}
CONSTRUCTIONS = [('Drywall', 2.5, 2.5), ('Wood', 2, 2), ('Metal', .2, .2), ('Brick', 10, 5), ('Brick', 20, 5),
                 ('Concrete', 20, 5), ('Stone', 20, 5)]
POSES = {'foot': (200, -5, 5), 'chest': (200, -10, 110), 'standoff50': (200, -50, 100)}

def run_case(mat, th, cd, mode, trans, pose, grenades):
    hp, mult, _ = MATERIALS[mat]
    alive = None; health = None; total_removed = 0; per = []
    for g in range(grenades):
        r = simulate(400, 300, th, cd, hp, mult, trans, POSES[pose], alive=alive, health=health, mode=mode)
        alive, health = r['alive'], r['health']; total_removed += r['removed']
        c32 = chunk_stats(r, 32, 32)
        per.append(dict(candidates=r['candidates'], damaged=r['damaged'], removed=r['removed'], dirty32=len(c32),
                        quads32=sum(c['quads'] for c in c32), boxes32=sum(c['boxes_upper'] for c in c32),
                        delta32=sum(min(c['rle_bytes'], c['mask_bytes'], c['list_bytes']) for c in c32)))
    mask = through_mask(r)
    return dict(material=mat, thickness=th, layers=r['ny'], mode=mode, t=round(trans, 4), pose=pose, grenades=grenades,
                cells=r['nx'] * r['ny'] * r['nz'], removed_total=total_removed, through_cm2=sum(map(sum, mask)) * 25,
                stand=stadium_fits(mask, 34, 92.5, max_bottom=45) is not None,
                crouch=stadium_fits(mask, 34, 62.5, max_bottom=45) is not None, events=per)

def main():
    rows = []
    jobs = []
    for mat, th, cd in CONSTRUCTIONS:
        t20 = MATERIALS[mat][2]
        modes = [('cell', t20)] if th == cd else [('cell', t20), ('path', t20 ** (1 / 20.))]
        for mode, trans in modes:
            for pose in POSES:
                jobs.append((mat, th, cd, mode, trans, pose, 3 if th > cd else 1))
    # Retune sweep for multilayer brick: per-cm transmission giving a breach.
    for t in (.94, .96, .97, .98):
        for th in (10, 20):
            for pose in ('chest', 'foot'):
                jobs.append(('Brick', th, 5, 'path', t, pose, 2))
    for job in jobs:
        row = run_case(*job); rows.append(row); print(json.dumps(row)); sys.stdout.flush()
    json.dump(rows, open(sys.argv[1] if len(sys.argv) > 1 else 'Saved/Testing/cell-destruction-model.json', 'w'), indent=1)

if __name__ == '__main__':
    main()
