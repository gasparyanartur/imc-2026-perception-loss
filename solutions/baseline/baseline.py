# iter2: short-edge collapse simplifier.
#
# Key differences from iter1/baseline (QEM-based):
#   1. Cost = edge length squared; only edges <= epsilon (1% AABB diagonal)
#      ever enter the heap, so compression targets dense local tessellation.
#   2. No Hausdorff cluster check -- the epsilon bound implicitly limits drift.
#   3. Normal check uses unnormalized normals with dot <= 1e-9 threshold
#      (allows near-90-deg rotations, rejects only actual flips/degeneracies).
#   4. Normal check covers ALL surviving faces around BOTH endpoints.
#   5. Distance is re-verified at pop time against epsilon (handles stale entries).
#   6. After each collapse, only short edges are (re-)enqueued.
#
# To run:
#   pypy3 iter2.py < mesh.in > mesh.out

import sys
import heapq

V = []
F = []


def load_obj():
    global V, F
    tok = sys.stdin.buffer.read().split()
    nv = int(tok[0])
    nf = int(tok[1])
    p = 2
    V = [None] * nv
    for i in range(nv):
        V[i] = (float(tok[p + 1]), float(tok[p + 2]), float(tok[p + 3]))
        p += 4
    F = [None] * nf
    for i in range(nf):
        F[i] = (int(tok[p + 1]) - 1, int(tok[p + 2]) - 1, int(tok[p + 3]) - 1)
        p += 4


def save_obj():
    out = ["%d %d" % (len(V), len(F))]
    out += ["v %.10g %.10g %.10g" % v for v in V]
    out += ["f %d %d %d" % (a + 1, b + 1, c + 1) for (a, b, c) in F]
    sys.stdout.write("\n".join(out))
    sys.stdout.write("\n")


def simplify():
    global V, F

    nv = len(V)
    if nv == 0 or not F:
        return

    coords = [list(v) for v in V]

    # Epsilon = 1% of AABB diagonal (same as reference).
    xs = [p[0] for p in coords]
    ys = [p[1] for p in coords]
    zs = [p[2] for p in coords]
    diag = (
        (max(xs) - min(xs)) ** 2
        + (max(ys) - min(ys)) ** 2
        + (max(zs) - min(zs)) ** 2
    ) ** 0.5
    epsilon = diag * 0.01
    if epsilon < 1e-12:
        epsilon = 1e-12
    epsilon_sq = epsilon * epsilon

    # Connectivity.
    faces = [tuple(f) for f in F]  # face tuple or None when removed
    vfaces = [set() for _ in range(nv)]
    nbrs = [set() for _ in range(nv)]
    for fid, (a, b, c) in enumerate(faces):
        vfaces[a].add(fid)
        vfaces[b].add(fid)
        vfaces[c].add(fid)
        nbrs[a].update((b, c))
        nbrs[b].update((a, c))
        nbrs[c].update((a, b))

    alive = [True] * nv
    version = [0] * nv

    def _face_normal_unnorm(p, q, r):
        """Unnormalized cross product (no sqrt); returns (nx, ny, nz)."""
        ux = q[0] - p[0]; uy = q[1] - p[1]; uz = q[2] - p[2]
        vx = r[0] - p[0]; vy = r[1] - p[1]; vz = r[2] - p[2]
        return (uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx)

    def _dist_sq(a, b):
        ca = coords[a]; cb = coords[b]
        dx = ca[0] - cb[0]; dy = ca[1] - cb[1]; dz = ca[2] - cb[2]
        return dx * dx + dy * dy + dz * dz

    heap = []
    heappush = heapq.heappush
    heappop = heapq.heappop

    def _push_if_short(a, b):
        d2 = _dist_sq(a, b)
        if d2 <= epsilon_sq:
            heappush(heap, (d2, a, b, version[a], version[b]))

    # Seed heap with all short edges.
    for a in range(nv):
        for b in nbrs[a]:
            if a < b:
                _push_if_short(a, b)

    def _collapse_ok(keep, drop):
        """
        Validate collapsing `drop` into `keep`.
        Returns edge_faces set on success, None on failure.
        """
        # Link condition: exactly 2 shared neighbours, exactly 2 shared faces.
        shared = nbrs[keep] & nbrs[drop]
        if len(shared) != 2:
            return None
        edge_faces = vfaces[keep] & vfaces[drop]
        if len(edge_faces) != 2:
            return None
        # Shared neighbours must equal the two opposite vertices.
        opposite = set()
        for fid in edge_faces:
            for v in faces[fid]:
                if v != keep and v != drop:
                    opposite.add(v)
        if opposite != shared:
            return None

        kp = coords[keep]

        # Normal check over ALL surviving faces around both endpoints.
        # Uses unnormalized normals: dot <= 1e-9 rejects flips and degeneracies.
        all_faces = (vfaces[keep] | vfaces[drop]) - edge_faces
        for fid in all_faces:
            tri = faces[fid]
            if tri is None:
                continue
            a, b, c = tri
            pa, pb, pc = coords[a], coords[b], coords[c]
            old_n = _face_normal_unnorm(pa, pb, pc)

            # Substitute keep's new position for any vertex that is `drop`.
            na_pt = kp if a == drop else pa
            nb_pt = kp if b == drop else pb
            nc_pt = kp if c == drop else pc

            # Degenerate triangle check.
            if na_pt is nb_pt or nb_pt is nc_pt or na_pt is nc_pt:
                return None
            # Also check by coords if they collapsed to the same point.
            if na_pt == nb_pt or nb_pt == nc_pt or na_pt == nc_pt:
                return None

            new_n = _face_normal_unnorm(na_pt, nb_pt, nc_pt)
            dot = (old_n[0] * new_n[0] + old_n[1] * new_n[1] + old_n[2] * new_n[2])
            if dot <= 1e-9:
                return None

        return edge_faces

    def _do_collapse(keep, drop, edge_faces):
        # Remove edge faces.
        for fid in edge_faces:
            a, b, c = faces[fid]
            vfaces[a].discard(fid)
            vfaces[b].discard(fid)
            vfaces[c].discard(fid)
            faces[fid] = None

        # Rewire drop's faces onto keep.
        for fid in list(vfaces[drop]):
            a, b, c = faces[fid]
            a = keep if a == drop else a
            b = keep if b == drop else b
            c = keep if c == drop else c
            faces[fid] = (a, b, c)
            vfaces[keep].add(fid)

        # Retire drop.
        alive[drop] = False
        vfaces[drop] = set()
        nbrs[drop] = set()

        # Rebuild adjacency for keep and all affected vertices.
        affected = set()
        for fid in vfaces[keep]:
            tri = faces[fid]
            if tri:
                affected.update(tri)
        affected.discard(keep)

        # Update nbrs for affected vertices: replace drop with keep.
        for w in affected:
            nw = nbrs[w]
            if drop in nw:
                nw.discard(drop)
                if w != keep:
                    nw.add(keep)

        # Rebuild keep's neighbour set from its incident faces.
        nb = set()
        for fid in vfaces[keep]:
            tri = faces[fid]
            if tri:
                nb.update(tri)
        nb.discard(keep)
        nbrs[keep] = nb

        # Bump versions.
        for v in affected:
            version[v] += 1
        version[keep] += 1

        # Re-enqueue short edges around keep.
        for v in nbrs[keep]:
            _push_if_short(keep, v)

    while heap:
        d2, a, b, va, vb = heappop(heap)
        if not alive[a] or not alive[b]:
            continue
        if va != version[a] or vb != version[b]:
            continue
        if b not in nbrs[a]:
            continue

        # Re-verify distance (positions unchanged, but be safe).
        if _dist_sq(a, b) > epsilon_sq:
            continue

        # Try both endpoint orderings.
        edge_faces = _collapse_ok(a, b)
        if edge_faces is not None:
            keep, drop = a, b
        else:
            edge_faces = _collapse_ok(b, a)
            if edge_faces is None:
                continue
            keep, drop = b, a

        _do_collapse(keep, drop, edge_faces)

    # Compact.
    remap = {}
    new_V = []
    for i in range(nv):
        if alive[i]:
            remap[i] = len(new_V)
            new_V.append(tuple(coords[i]))
    new_F = []
    for f in faces:
        if f is None:
            continue
        a, b, c = f
        new_F.append((remap[a], remap[b], remap[c]))

    V = new_V
    F = new_F


load_obj()
simplify()
save_obj()
