# Starter scaffold for "Perception-Aware Lossless Simplification of 3D Meshes".
#
# To run:
#   pypy3 iter2-budget.py < mesh.in > mesh.out
#
# iter2-budget: iter1-runtime's endpoint-only QEM edge-collapse solver plus an
# internal, self-calibrating wall-clock budget. The grader allows ~1.1M vertices
# in 21 s / 2 GB; the pure edge-collapse loop is roughly linear but with a large
# constant, so on the biggest meshes CPython/pypy cannot drain the heap in time
# and the grader times out (= no valid output = 0 score for that mesh). The
# budget fixes exactly that: collapsing stops once a deadline is reached and the
# solver emits the best valid closed-manifold mesh produced so far. Because
# collapses are applied cheapest-QEM-error first, an interrupted run is still a
# good simplification, and because every applied collapse preserves the closed
# 2-manifold invariant, the partial mesh is always valid.
#
# The deadline is self-calibrating: setup (load + build adjacency) and the
# compact+save tail both run outside the collapse loop, so the loop stops early
# enough that setup + collapse + tail fit inside the budget. Setup time is
# measured at run time and the tail is reserved proportionally, which makes the
# total wall-clock safe on the grader's pypy3 (much faster than the local
# CPython) without hard-coding interpreter-specific constants.
#
# Memory: the input vertex/face lists are released right after the working
# copies are built, and the redundant "original coordinates" copy is dropped
# (endpoint-only collapses never move a surviving vertex, so the live
# coordinates already are the originals). On small meshes the heap drains well
# before the deadline and the output is identical to iter1-runtime / baseline.

import os
import sys
import time

# Wall-clock deadline for the whole program (load + simplify + save), measured
# from process start so it is interpreter-agnostic. The grader budget is 21 s;
# the default leaves a little headroom for interpreter start-up and imports
# (which happen before _T0) and is split at run time between collapsing and the
# compact+save tail (see the self-calibrating deadline in simplify). Tunable via
# the SIMPLIFY_BUDGET_SEC environment variable -- calibrate it against the real
# grader (pypy3), which is markedly faster than CPython.
_T0 = time.perf_counter()
try:
    _BUDGET_SEC = float(os.environ.get("SIMPLIFY_BUDGET_SEC", "18.0"))
except ValueError:
    _BUDGET_SEC = 18.0

# Mesh representation.
#   V : list of (x, y, z) vertex coordinates.
#   F : list of (a, b, c) faces, 0-indexed (input is 1-indexed; load_obj
#       subtracts 1, save_obj adds it back).
V = []
F = []
# Raw input bytes, retained so we can emit the (valid) input mesh unchanged as a
# graceful fallback when the interpreter is too slow to simplify within budget.
RAW = b""
_EMITTED = False


# --- fast input -------------------------------------------------------------

def load_obj():
    global V, F, RAW
    RAW = sys.stdin.buffer.read()
    tok = RAW.split()
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

def _passthrough():
    """Emit the original input mesh unchanged (valid, 0 % compression).

    Used when there is no time to build the QEM structures / collapse within the
    budget. Writing the raw bytes back is near-instant and always produces a
    valid closed-manifold submission (it is exactly the grader's input).
    """
    global _EMITTED
    sys.stdout.buffer.write(RAW)
    if not RAW.endswith(b"\n"):
        sys.stdout.buffer.write(b"\n")
    _EMITTED = True


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
#
# Runtime notes (vs. solutions/baseline): the arithmetic and collapse order are
# identical, but the hot paths avoid per-call list allocations (quadrics are
# combined/added with unrolled expressions), the link-condition check counts
# shared neighbours/faces with early exit instead of materializing intersection
# sets, neighbour sets are rebuilt with a single set.update per face, and the
# frequently used containers are bound to fast local names.

# Numerical tolerances.
_EPS_AREA = 1e-12          # reject faces whose area drops to ~0
# Reject a collapse if it turns any incident face normal by more than ~66 deg
# (cos < 0.4). The flat-shaded normal maps dominate the FinalSSIM score, so
# allowing near-90-deg flips (cos 0.0) lets the surface deviate enough to drop
# SSIM below the 0.9 gate on detailed meshes. 0.4 keeps every dataset scenario
# valid with a safe SSIM margin while preserving aggressive compression.
_NORMAL_FLIP_COS = 0.4
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
    # Unrolled (vs. a range(10) loop): identical result, fewer interpreter ops.
    into[0] += other[0]
    into[1] += other[1]
    into[2] += other[2]
    into[3] += other[3]
    into[4] += other[4]
    into[5] += other[5]
    into[6] += other[6]
    into[7] += other[7]
    into[8] += other[8]
    into[9] += other[9]


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

    perf = time.perf_counter
    budget = _BUDGET_SEC
    # Graceful-degradation gate 1: if loading alone already consumed this much of
    # the budget, the full pipeline (adjacency + quadrics + heap + collapse +
    # write) cannot finish in time or in 2 GB on this interpreter -- emit the
    # valid input unchanged before building anything else, keeping both wall-clock
    # and peak memory low. pypy3 (the grader) loads several times faster than
    # CPython, so on the grader this only trips on a genuinely oversized mesh.
    if perf() - _T0 > 0.15 * budget:
        _passthrough()
        return

    # Vertex coordinates. Endpoint-only collapses never move a surviving
    # vertex, so a live vertex always sits at its original position -- these are
    # therefore also the "original" coordinates used by the Hausdorff
    # cluster-radius check, and no separate `orig` copy is needed.
    coords = [list(v) for v in V]
    del V[:]  # free the input copy; save_obj rebuilds V from the result.

    # AABB diagonal of the original mesh -> Hausdorff bound.
    xs = [p[0] for p in coords]
    ys = [p[1] for p in coords]
    zs = [p[2] for p in coords]
    diag = (
        (max(xs) - min(xs)) ** 2
        + (max(ys) - min(ys)) ** 2
        + (max(zs) - min(zs)) ** 2
    ) ** 0.5
    hbound = _HAUSDORFF_FRAC * diag
    hbound_sq = hbound * hbound

    # Connectivity. faces[f] is a 3-tuple or None once removed.
    faces = [tuple(f) for f in F]
    del F[:]  # free the input copy; save_obj rebuilds F from the result.
    vfaces = [set() for _ in range(nv)]   # vertex -> incident face ids
    nbrs = [set() for _ in range(nv)]     # vertex -> adjacent vertices
    for fid, (a, b, c) in enumerate(faces):
        vfaces[a].add(fid)
        vfaces[b].add(fid)
        vfaces[c].add(fid)
        nbrs[a].update((b, c))
        nbrs[b].update((a, c))
        nbrs[c].update((a, b))

    # Graceful-degradation gate 2: adjacency is the first O(faces) build pass; if
    # we are already this deep into the budget there is no time left to also
    # build quadrics, seed the heap, collapse, and write the result -- so emit
    # the valid input unchanged instead of overrunning the limit.
    if perf() - _T0 > 0.38 * budget:
        _passthrough()
        return

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

    # Bind hot globals/builtins to locals (faster name lookups in the loops).
    fplane = _face_plane
    heappush = heapq.heappush
    heappop = heapq.heappop
    eps_area = _EPS_AREA
    flip_cos = _NORMAL_FLIP_COS
    ncollapse = 0

    heap = []

    def _best_target(a, b):
        """Cheapest endpoint placement for edge (a, b): (cost, keep, drop)."""
        qa, qb = quad[a], quad[b]
        # Combine the two quadrics (unrolled; same floats as a range(10) loop).
        c0 = qa[0] + qb[0]; c1 = qa[1] + qb[1]; c2 = qa[2] + qb[2]
        c3 = qa[3] + qb[3]; c4 = qa[4] + qb[4]; c5 = qa[5] + qb[5]
        c6 = qa[6] + qb[6]; c7 = qa[7] + qb[7]; c8 = qa[8] + qb[8]
        c9 = qa[9] + qb[9]
        # Inline v^T Q v at both endpoints (avoids a list alloc + 2 calls).
        x, y, z = coords[a]
        ca = (c0 * x * x + 2.0 * c1 * x * y + 2.0 * c2 * x * z + 2.0 * c3 * x
              + c4 * y * y + 2.0 * c5 * y * z + 2.0 * c6 * y
              + c7 * z * z + 2.0 * c8 * z + c9)
        x, y, z = coords[b]
        cb = (c0 * x * x + 2.0 * c1 * x * y + 2.0 * c2 * x * z + 2.0 * c3 * x
              + c4 * y * y + 2.0 * c5 * y * z + 2.0 * c6 * y
              + c7 * z * z + 2.0 * c8 * z + c9)
        if ca <= cb:
            return ca, a, b
        return cb, b, a

    def _push_edge(a, b):
        # Only the cost is needed here (keep/drop are re-derived on pop), so
        # inline _best_target's math and skip the keep/drop branch. This is the
        # hottest path -- one push per incident edge after every collapse.
        qa = quad[a]; qb = quad[b]
        c0 = qa[0] + qb[0]; c1 = qa[1] + qb[1]; c2 = qa[2] + qb[2]
        c3 = qa[3] + qb[3]; c4 = qa[4] + qb[4]; c5 = qa[5] + qb[5]
        c6 = qa[6] + qb[6]; c7 = qa[7] + qb[7]; c8 = qa[8] + qb[8]
        c9 = qa[9] + qb[9]
        x, y, z = coords[a]
        ca = (c0 * x * x + 2.0 * c1 * x * y + 2.0 * c2 * x * z + 2.0 * c3 * x
              + c4 * y * y + 2.0 * c5 * y * z + 2.0 * c6 * y
              + c7 * z * z + 2.0 * c8 * z + c9)
        x, y, z = coords[b]
        cb = (c0 * x * x + 2.0 * c1 * x * y + 2.0 * c2 * x * z + 2.0 * c3 * x
              + c4 * y * y + 2.0 * c5 * y * z + 2.0 * c6 * y
              + c7 * z * z + 2.0 * c8 * z + c9)
        cost = ca if ca <= cb else cb
        # Store endpoint versions so stale entries can be skipped on pop.
        heappush(heap, (cost, a, b, version[a], version[b]))

    for a in range(nv):
        na = nbrs[a]
        for b in na:
            if a < b:
                _push_edge(a, b)

    def _collapse_ok(keep, drop):
        """Validate collapsing `drop` into `keep` (keep's position is fixed)."""
        # Closed-manifold link condition: exactly two shared neighbours, and
        # exactly two faces shared by the edge endpoints. Set intersection (&)
        # runs in C and is faster than a Python counting loop on CPython.
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

        kx, ky, kz = coords[keep]
        # Hausdorff cluster-radius proxy: every original vertex that would be
        # represented by `keep` must stay within the bound of keep's position.
        for oi in cluster[drop]:
            op = coords[oi]
            dx = op[0] - kx
            dy = op[1] - ky
            dz = op[2] - kz
            if dx * dx + dy * dy + dz * dz > hbound_sq:
                return None

        # Degeneracy / normal-flip check on every face that survives the
        # collapse and currently touches `drop`.
        kp = coords[keep]
        for fid in vfaces[drop]:
            if fid in edge_faces:
                continue
            a, b, c = faces[fid]
            oa, ob, oc = coords[a], coords[b], coords[c]
            o_na, o_nb, o_nc, _, o_area = fplane(oa, ob, oc)
            if o_area <= 0.0:
                return None
            na = kp if a == drop else oa
            nb = kp if b == drop else ob
            nc = kp if c == drop else oc
            n_na, n_nb, n_nc, _, n_area = fplane(na, nb, nc)
            if n_area <= eps_area:
                return None
            if o_na * n_na + o_nb * n_nb + o_nc * n_nc <= flip_cos:
                return None
        return edge_faces

    def _do_collapse(keep, drop, edge_faces):
        # The two vertices opposite the collapsed edge (its shared neighbours).
        # Their incident-face set shrinks (an edge face is removed), so they
        # need a full adjacency rebuild; capture them before mutating faces.
        opp = set()
        for fid in edge_faces:
            for v in faces[fid]:
                if v != keep and v != drop:
                    opp.add(v)

        # Snapshot the neighbourhoods before mutation so we can update them
        # incrementally afterwards.
        drop_nbrs = nbrs[drop]
        affected = (nbrs[keep] | drop_nbrs) - {keep, drop}

        # Remove the two faces incident to the collapsed edge.
        for fid in edge_faces:
            a, b, c = faces[fid]
            vfaces[a].discard(fid)
            vfaces[b].discard(fid)
            vfaces[c].discard(fid)
            faces[fid] = None

        # Rewire the remaining faces of `drop` onto `keep`.
        vfk = vfaces[keep]
        for fid in list(vfaces[drop]):
            a, b, c = faces[fid]
            a = keep if a == drop else a
            b = keep if b == drop else b
            c = keep if c == drop else c
            faces[fid] = (a, b, c)
            vfk.add(fid)

        # Merge quadrics and clusters; retire `drop`.
        _quadric_add(quad[keep], quad[drop])
        cluster[keep].extend(cluster[drop])
        alive[drop] = False
        vfaces[drop] = set()
        nbrs[drop] = set()

        # Update adjacency only where it actually changes (same result as a
        # full rebuild of every touched vertex, but far cheaper):
        #   * ordinary neighbours of `drop` (not `keep`, not opposite) simply
        #     trade `drop` for `keep` -- two set ops instead of a rebuild;
        #   * `keep` and the two opposite vertices lost/gained faces, so they
        #     are rebuilt from their incident faces;
        #   * `keep`'s exclusive neighbours are unchanged and keep their set.
        # `version` is still bumped for every affected vertex (and `keep`),
        # exactly as a full rebuild would, so heap staleness is identical.
        for w in drop_nbrs:
            if w == keep or w in opp:
                continue
            nw = nbrs[w]
            nw.discard(drop)
            nw.add(keep)
        for v in opp:
            nb = set()
            upd = nb.update
            for fid in vfaces[v]:
                upd(faces[fid])
            nb.discard(v)
            nbrs[v] = nb
        nb = set()
        upd = nb.update
        for fid in vfk:
            upd(faces[fid])
        nb.discard(keep)
        nbrs[keep] = nb

        for v in affected:
            version[v] += 1
        version[keep] += 1

        # Re-rank edges incident to the kept vertex.
        for v in nbrs[keep]:
            _push_edge(keep, v)

    # Self-calibrating deadline. Total wall = setup (load + build structures +
    # seed the heap) + collapse loop + tail (compact + write). The tail is one
    # O(V'+F') compaction pass plus one formatting/write pass over the *output*,
    # so it is a modest fraction of the setup cost; we measure setup directly and
    # reserve a proportional slice for the tail, stopping the collapse loop that
    # much before the hard budget. This keeps the *whole program* within the
    # budget on any interpreter: pypy3 (the grader) runs setup/tail far faster
    # than CPython and so spends correspondingly more of the budget collapsing.
    t_setup = perf() - _T0
    reserve = 0.35 * t_setup
    if reserve < 0.5:
        reserve = 0.5
    elif reserve > 0.5 * _BUDGET_SEC:
        reserve = 0.5 * _BUDGET_SEC
    deadline = _T0 + _BUDGET_SEC - reserve
    if perf() >= deadline:
        heap = []  # no time left to collapse; go straight to compact + save.

    while heap:
        cost, a, b, va, vb = heappop(heap)
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

        # Wall-clock budget: stop collapsing once the deadline is reached so the
        # remaining time can compact and write a valid mesh. Checked on a stride
        # (every 4096 applied collapses) to keep perf_counter() off the hot path.
        ncollapse += 1
        if (ncollapse & 4095) == 0 and perf() >= deadline:
            break

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
if not _EMITTED:
    save_obj()
