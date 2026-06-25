# Starter scaffold for "Perception-Aware Lossless Simplification of 3D Meshes".
#
# It reads the mesh, does not optimize it and then outputs it.
# Implement your simplification inside simplify()
#
# To run:
#   pypy3 baseline.py < mesh.in > mesh.out

import sys

# Mesh representation.
#   V : list of (x, y, z) vertex coordinates.
#   F : list of (a, b, c) faces, 0-indexed (input is 1-indexed; load_obj
#       subtracts 1, save_obj adds it back).
V = []
F = []


# --- fast input -------------------------------------------------------------

def load_obj():
    global V, F
    tok = sys.stdin.buffer.read().split()
    nv = int(tok[0])
    nf = int(tok[1])

    # tokens are: nv nf, then "v x y z" per vertex, then "f a b c" per face.
    p = 2
    V = [None] * nv
    for i in range(nv):
        V[i] = (float(tok[p + 1]), float(tok[p + 2]), float(tok[p + 3]))
        p += 4
    F = [None] * nf
    for i in range(nf):
        F[i] = (int(tok[p + 1]) - 1, int(tok[p + 2]) - 1, int(tok[p + 3]) - 1)
        p += 4


# --- fast output ------------------------------------------------------------

# Print the mesh. Print 10 significant digits using %.10g for performance.
def save_obj():
    out = ["%d %d" % (len(V), len(F))]
    out += ["v %.10g %.10g %.10g" % v for v in V]
    out += ["f %d %d %d" % (a + 1, b + 1, c + 1) for (a, b, c) in F]
    sys.stdout.write("\n".join(out))
    sys.stdout.write("\n")


# --- your implementation ----------------------------------------------------

import heapq

# Baseline simplifier: endpoint-only QEM edge collapse (report Solution 2).
#
# Each undirected mesh edge is a collapse candidate. We rank candidates by the
# Quadric Error Metric (QEM) but restrict the replacement vertex to one of the
# two endpoints (endpoint-only placement), which keeps every output vertex on
# the original surface and makes the Hausdorff bound easy to track. A collapse
# is only applied when it preserves a closed 2-manifold and respects the
# evaluator's gates:
#   * link condition (exactly two shared neighbours for a closed manifold);
#   * no degenerate (zero-area) face and no face-normal flip;
#   * cluster-radius proxy for the symmetric Hausdorff bound.

# Numerical tolerances.
_EPS_AREA = 1e-12          # reject faces whose area drops to ~0
_NORMAL_FLIP_COS = 0.0     # reject collapses that turn a face normal by > 90 deg
_HAUSDORFF_FRAC = 0.05     # bound = 0.05 * AABB diagonal (matches evaluator)


def _face_plane(p, q, r):
    """Unit normal (a, b, c) and offset d of the plane through p, q, r.

    Returns (a, b, c, d, area) with a*x + b*y + c*z + d = 0 on the plane.
    area is the triangle area; a degenerate triangle yields area 0.
    """
    ux, uy, uz = q[0] - p[0], q[1] - p[1], q[2] - p[2]
    vx, vy, vz = r[0] - p[0], r[1] - p[1], r[2] - p[2]
    nx = uy * vz - uz * vy
    ny = uz * vx - ux * vz
    nz = ux * vy - uy * vx
    length = (nx * nx + ny * ny + nz * nz) ** 0.5
    area = 0.5 * length
    if length == 0.0:
        return 0.0, 0.0, 0.0, 0.0, 0.0
    nx, ny, nz = nx / length, ny / length, nz / length
    d = -(nx * p[0] + ny * p[1] + nz * p[2])
    return nx, ny, nz, d, area


def _plane_quadric(a, b, c, d, w):
    """Area-weighted quadric (10 unique entries) of a single plane."""
    return [
        w * a * a, w * a * b, w * a * c, w * a * d,
        w * b * b, w * b * c, w * b * d,
        w * c * c, w * c * d,
        w * d * d,
    ]


def _quadric_add(into, other):
    for i in range(10):
        into[i] += other[i]


def _quadric_error(q, v):
    """Evaluate v^T Q v for the 10-entry symmetric quadric q at point v."""
    x, y, z = v
    return (
        q[0] * x * x + 2.0 * q[1] * x * y + 2.0 * q[2] * x * z + 2.0 * q[3] * x
        + q[4] * y * y + 2.0 * q[5] * y * z + 2.0 * q[6] * y
        + q[7] * z * z + 2.0 * q[8] * z
        + q[9]
    )


# Optimize the mesh: replace V and F
def simplify():
    global V, F

    nv = len(V)
    if nv == 0 or not F:
        return

    # Mutable coordinates (lists) and original coordinates (for Hausdorff).
    coords = [list(v) for v in V]
    orig = [tuple(v) for v in V]

    # AABB diagonal of the original mesh -> Hausdorff bound.
    xs = [p[0] for p in orig]
    ys = [p[1] for p in orig]
    zs = [p[2] for p in orig]
    diag = (
        (max(xs) - min(xs)) ** 2
        + (max(ys) - min(ys)) ** 2
        + (max(zs) - min(zs)) ** 2
    ) ** 0.5
    hbound = _HAUSDORFF_FRAC * diag

    # Connectivity. faces[f] is a 3-tuple or None once removed.
    faces = [tuple(f) for f in F]
    vfaces = [set() for _ in range(nv)]   # vertex -> incident face ids
    nbrs = [set() for _ in range(nv)]     # vertex -> adjacent vertices
    for fid, (a, b, c) in enumerate(faces):
        vfaces[a].add(fid)
        vfaces[b].add(fid)
        vfaces[c].add(fid)
        nbrs[a].update((b, c))
        nbrs[b].update((a, c))
        nbrs[c].update((a, b))

    # Per-vertex quadric (area-weighted sum of incident face plane quadrics).
    quad = [[0.0] * 10 for _ in range(nv)]
    for a, b, c in faces:
        na, nb, nc, d, area = _face_plane(coords[a], coords[b], coords[c])
        if area <= 0.0:
            continue
        qe = _plane_quadric(na, nb, nc, d, area)
        _quadric_add(quad[a], qe)
        _quadric_add(quad[b], qe)
        _quadric_add(quad[c], qe)

    alive = [True] * nv
    # Original vertices currently represented by each live vertex (cluster).
    cluster = [[i] for i in range(nv)]
    version = [0] * nv  # bumped whenever a vertex's neighbourhood changes

    def _best_target(a, b):
        """Cheapest endpoint placement for edge (a, b): (cost, keep, drop)."""
        qa, qb = quad[a], quad[b]
        comb = [qa[i] + qb[i] for i in range(10)]
        ca = _quadric_error(comb, coords[a])
        cb = _quadric_error(comb, coords[b])
        if ca <= cb:
            return ca, a, b
        return cb, b, a

    heap = []

    def _push_edge(a, b):
        cost, keep, drop = _best_target(a, b)
        # Store endpoint versions so stale entries can be skipped on pop.
        heapq.heappush(heap, (cost, a, b, version[a], version[b]))

    for a in range(nv):
        for b in nbrs[a]:
            if a < b:
                _push_edge(a, b)

    def _collapse_ok(keep, drop):
        """Validate collapsing `drop` into `keep` (keep's position is fixed)."""
        # Closed-manifold link condition: exactly two shared neighbours, and
        # exactly two faces shared by the edge endpoints.
        shared = nbrs[keep] & nbrs[drop]
        if len(shared) != 2:
            return None
        edge_faces = vfaces[keep] & vfaces[drop]
        if len(edge_faces) != 2:
            return None
        # The shared neighbours must be exactly the two vertices opposite the
        # edge in its incident faces; otherwise the collapse folds the surface.
        opposite = set()
        for fid in edge_faces:
            for v in faces[fid]:
                if v != keep and v != drop:
                    opposite.add(v)
        if opposite != shared:
            return None

        kp = coords[keep]
        # Hausdorff cluster-radius proxy: every original vertex that would be
        # represented by `keep` must stay within the bound of keep's position.
        hbound_sq = hbound * hbound
        for oi in cluster[drop]:
            op = orig[oi]
            dx, dy, dz = op[0] - kp[0], op[1] - kp[1], op[2] - kp[2]
            if dx * dx + dy * dy + dz * dz > hbound_sq:
                return None

        # Degeneracy / normal-flip check on every face that survives the
        # collapse and currently touches `drop`.
        for fid in vfaces[drop]:
            if fid in edge_faces:
                continue
            a, b, c = faces[fid]
            oa, ob, oc = coords[a], coords[b], coords[c]
            o_na, o_nb, o_nc, _, o_area = _face_plane(oa, ob, oc)
            if o_area <= 0.0:
                return None
            na = kp if a == drop else oa
            nb = kp if b == drop else ob
            nc = kp if c == drop else oc
            n_na, n_nb, n_nc, _, n_area = _face_plane(na, nb, nc)
            if n_area <= _EPS_AREA:
                return None
            if o_na * n_na + o_nb * n_nb + o_nc * n_nc <= _NORMAL_FLIP_COS:
                return None
        return edge_faces

    def _do_collapse(keep, drop, edge_faces):
        # Remove the two faces incident to the collapsed edge.
        for fid in edge_faces:
            a, b, c = faces[fid]
            for v in (a, b, c):
                vfaces[v].discard(fid)
            faces[fid] = None

        # Rewire the remaining faces of `drop` onto `keep`.
        for fid in list(vfaces[drop]):
            a, b, c = faces[fid]
            a = keep if a == drop else a
            b = keep if b == drop else b
            c = keep if c == drop else c
            faces[fid] = (a, b, c)
            vfaces[keep].add(fid)

        # Merge quadrics and clusters; retire `drop`.
        _quadric_add(quad[keep], quad[drop])
        cluster[keep].extend(cluster[drop])
        affected = (nbrs[keep] | nbrs[drop]) - {keep, drop}
        alive[drop] = False
        vfaces[drop] = set()
        nbrs[drop] = set()

        # Rebuild adjacency for every vertex touched by the collapse.
        touched = affected | {keep}
        for v in touched:
            nb = set()
            for fid in vfaces[v]:
                a, b, c = faces[fid]
                if a != v:
                    nb.add(a)
                if b != v:
                    nb.add(b)
                if c != v:
                    nb.add(c)
            nbrs[v] = nb
            version[v] += 1

        # Re-rank edges incident to the kept vertex.
        for v in nbrs[keep]:
            _push_edge(keep, v)

    while heap:
        cost, a, b, va, vb = heapq.heappop(heap)
        if not alive[a] or not alive[b]:
            continue
        if va != version[a] or vb != version[b]:
            continue  # stale entry; a fresh one was pushed
        if b not in nbrs[a]:
            continue  # edge no longer exists

        _, keep, drop = _best_target(a, b)
        edge_faces = _collapse_ok(keep, drop)
        if edge_faces is None:
            # Endpoint placement that was cheapest is invalid; try the other.
            other_keep, other_drop = drop, keep
            edge_faces = _collapse_ok(other_keep, other_drop)
            if edge_faces is None:
                continue
            keep, drop = other_keep, other_drop
        _do_collapse(keep, drop, edge_faces)

    # Compact: drop retired vertices and reindex faces.
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
