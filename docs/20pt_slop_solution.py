import sys
from array import array
import heapq

V_flat = array('d')          # x0,y0,z0, x1,y1,z1, ...
F_flat = array('I')          # a0,b0,c0, a1,b1,c1, ...

def load_obj():
    global V_flat, F_flat
    data = sys.stdin.buffer.read().split()
    nv = int(data[0])
    nf = int(data[1])
    p = 2
    V_flat = array('d')
    for _ in range(nv):
        V_flat.append(float(data[p+1]))
        V_flat.append(float(data[p+2]))
        V_flat.append(float(data[p+3]))
        p += 4
    F_flat = array('I')
    for _ in range(nf):
        F_flat.append(int(data[p+1]) - 1)
        F_flat.append(int(data[p+2]) - 1)
        F_flat.append(int(data[p+3]) - 1)
        p += 4

def save_obj():
    out = [f"{len(V_flat)//3} {len(F_flat)//3}"]
    out += [f"v {V_flat[i]:.10g} {V_flat[i+1]:.10g} {V_flat[i+2]:.10g}"
            for i in range(0, len(V_flat), 3)]
    out += [f"f {F_flat[i]+1} {F_flat[i+1]+1} {F_flat[i+2]+1}"
            for i in range(0, len(F_flat), 3)]
    sys.stdout.write("\n".join(out) + "\n")

def simplify():
    global V_flat, F_flat
    nv0 = len(V_flat) // 3
    nf0 = len(F_flat) // 3

    # ---------- tolerance based on bounding box ----------
    xs = V_flat[0::3]; ys = V_flat[1::3]; zs = V_flat[2::3]
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    minz, maxz = min(zs), max(zs)
    diagonal = ((maxx-minx)**2 + (maxy-miny)**2 + (maxz-minz)**2) ** 0.5
    epsilon = diagonal * 0.01          # 1% of bounding box diagonal
    if epsilon < 1e-12:
        epsilon = 1e-12
    epsilon_sq = epsilon * epsilon


    # ---------- adjacency (sorted arrays) ----------
    adj_faces = [array('I') for _ in range(nv0)]
    adj_verts = [array('I') for _ in range(nv0)]

    face_cnt = array('I', [0]) * nv0
    for i in range(0, 3*nf0):
        face_cnt[F_flat[i]] += 1
    for v in range(nv0):
        adj_faces[v] = array('I', [0]) * face_cnt[v]
    pos = [0] * nv0
    for fi in range(nf0):
        for k in range(3):
            v = F_flat[3*fi + k]
            adj_faces[v][pos[v]] = fi
            pos[v] += 1

    for u in range(nv0):
        nbrs = set()
        for fi in adj_faces[u]:
            for k in range(3):
                w = F_flat[3*fi + k]
                if w != u:
                    nbrs.add(w)
        adj_verts[u] = array('I', sorted(nbrs))

    vertex_alive = bytearray([1]) * nv0
    face_alive = bytearray([1]) * nf0

    # ---------- edge ID infrastructure ----------
    edge_u = array('I')
    edge_v = array('I')
    edge_in_heap = bytearray()
    edge_ids = [array('I') for _ in range(nv0)]   # for each vertex, parallel to adj_verts
    next_eid = 0

    def intersect_sorted(a, b):
        i = j = 0
        res = []
        while i < len(a) and j < len(b):
            if a[i] < b[j]: i += 1
            elif b[j] < a[i]: j += 1
            else:
                res.append(a[i]); i += 1; j += 1
        return res

    def merge_sorted(a, b):
        i = j = 0
        res = []
        while i < len(a) and j < len(b):
            if a[i] < b[j]:
                res.append(a[i]); i += 1
            elif b[j] < a[i]:
                res.append(b[j]); j += 1
            else:
                res.append(a[i]); i += 1; j += 1
        res.extend(a[i:])
        res.extend(b[j:])
        return res

    def face_normal(i0, i1, i2):
        p0 = V_flat[3*i0:3*i0+3]; p1 = V_flat[3*i1:3*i1+3]; p2 = V_flat[3*i2:3*i2+3]
        u = (p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2])
        v = (p2[0]-p0[0], p2[1]-p0[1], p2[2]-p0[2])
        nx = u[1]*v[2] - u[2]*v[1]
        ny = u[2]*v[0] - u[0]*v[2]
        nz = u[0]*v[1] - u[1]*v[0]
        return (nx, ny, nz)

    def dot(a,b): return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]

    def normal_ok(u, v, target, edge_faces):
        merged = merge_sorted(adj_faces[u], adj_faces[v])
        for fi in merged:
            if fi in edge_faces: continue
            if not face_alive[fi]: continue
            old_tri = (F_flat[3*fi], F_flat[3*fi+1], F_flat[3*fi+2])
            old_n = face_normal(*old_tri)
            new_tri = [target if (x == u or x == v) else x for x in old_tri]
            if len(set(new_tri)) < 3:
                return False
            new_n = face_normal(new_tri[0], new_tri[1], new_tri[2])
            if dot(old_n, new_n) <= 1e-9:
                return False
        return True

    def link_ok(u, v, edge_faces):
        opp = set()
        for fi in edge_faces:
            for k in range(3):
                w = F_flat[3*fi + k]
                if w != u and w != v:
                    opp.add(w)
        common = intersect_sorted(adj_verts[u], adj_verts[v])
        return set(common) == opp

    # ---------- initial edge registration (only short edges) ----------
    heap = []
    for u in range(nv0):
        eids = array('I', [0]) * len(adj_verts[u])
        for i, v in enumerate(adj_verts[u]):
            if u < v:
                eid = next_eid
                next_eid += 1
                edge_u.append(u)
                edge_v.append(v)
                # compute edge length squared
                pu = V_flat[3*u:3*u+3]; pv = V_flat[3*v:3*v+3]
                dx = pu[0]-pv[0]; dy = pu[1]-pv[1]; dz = pu[2]-pv[2]
                d2 = dx*dx + dy*dy + dz*dz
                if d2 <= epsilon_sq:
                    edge_in_heap.append(1)
                    heapq.heappush(heap, (d2, eid))
                else:
                    edge_in_heap.append(0)
                eids[i] = eid
            else:
                for j, w in enumerate(adj_verts[v]):
                    if w == u:
                        eids[i] = edge_ids[v][j]
                        break
        edge_ids[u] = eids
    edge_in_heap.extend([0] * (next_eid - len(edge_in_heap)))

    # ---------- collapse loop ----------
    collapse_cnt = 0

    while heap:
        d2, eid = heapq.heappop(heap)
        edge_in_heap[eid] = 0
        u, v = edge_u[eid], edge_v[eid]
        if not vertex_alive[u] or not vertex_alive[v]:
            continue

        faces = intersect_sorted(adj_faces[u], adj_faces[v])
        if len(faces) != 2: continue
        f1, f2 = faces[0], faces[1]
        if not face_alive[f1] or not face_alive[f2]:
            continue

        if not link_ok(u, v, (f1, f2)):
            continue

        # Always collapse v into u (u survives). Re-check distance (could be outdated)
        pu = V_flat[3*u:3*u+3]; pv = V_flat[3*v:3*v+3]
        dx = pu[0]-pv[0]; dy = pu[1]-pv[1]; dz = pu[2]-pv[2]
        if dx*dx + dy*dy + dz*dz > epsilon_sq:
            continue

        target = u
        if not normal_ok(u, v, target, {f1, f2}):
            # try collapsing u into v instead
            target = v
            if not normal_ok(u, v, target, {f1, f2}):
                continue

        # ---- perform collapse (v -> target) ----
        face_alive[f1] = face_alive[f2] = 0

        # Update faces: replace u,v with target
        affected_faces = (set(adj_faces[u]) | set(adj_faces[v])) - {f1, f2}
        for fi in affected_faces:
            for k in range(3):
                idx = 3*fi + k
                if F_flat[idx] == u or F_flat[idx] == v:
                    F_flat[idx] = target

        # Update adj_faces for vertices that lose f1,f2 (except u,v)
        vertices_losing = set(F_flat[3*f1:3*f1+3]) | set(F_flat[3*f2:3*f2+3])
        vertices_losing.discard(u)
        vertices_losing.discard(v)
        for w in vertices_losing:
            arr = adj_faces[w]
            new_arr = [x for x in arr if x != f1 and x != f2 and face_alive[x]]
            adj_faces[w] = array('I', new_arr)

        # target gains all faces from u and v (minus dead ones)
        merged = merge_sorted(adj_faces[u], adj_faces[v])
        merged_target = merge_sorted(merged, adj_faces[target])
        new_faces = [x for x in merged_target if x != f1 and x != f2 and face_alive[x]]
        adj_faces[target] = array('I', new_faces)

        # Update vertex neighbours
        neigh_u = set(adj_verts[u])
        neigh_v = set(adj_verts[v])
        all_neigh = neigh_u | neigh_v | set(adj_verts[target])
        all_neigh.discard(u)
        all_neigh.discard(v)
        new_neighbors = sorted(all_neigh)
        adj_verts[target] = array('I', new_neighbors)

        for w in all_neigh:
            old = adj_verts[w]
            lst = [x for x in old if x != u and x != v]
            if target != w and target not in lst:
                lst.append(target)
                lst.sort()
            i = 1
            while i < len(lst):
                if lst[i] == lst[i-1]:
                    del lst[i]
                else:
                    i += 1
            adj_verts[w] = array('I', lst)

        # Mark the moved vertex dead
        if target == u:
            vertex_alive[v] = 0
            adj_faces[v] = adj_faces[u] = array('I')  # u may still be alive as target
        else:
            vertex_alive[u] = 0
            adj_faces[u] = adj_faces[v] = array('I')
        # but target remains alive, so we only clear the non-target
        if target == u:
            adj_verts[v] = array('I')
            edge_ids[v] = array('I')
        else:
            adj_verts[u] = array('I')
            edge_ids[u] = array('I')

        # Register new edges around target (only those with length <= epsilon)
        new_eids = array('I', [0]) * len(adj_verts[target])
        for i, w in enumerate(adj_verts[target]):
            if target < w:
                eid_new = next_eid
                next_eid += 1
                edge_u.append(target)
                edge_v.append(w)
                p1 = V_flat[3*target:3*target+3]; p2 = V_flat[3*w:3*w+3]
                ddx = p1[0]-p2[0]; ddy = p1[1]-p2[1]; ddz = p1[2]-p2[2]
                nd2 = ddx*ddx + ddy*ddy + ddz*ddz
                if nd2 <= epsilon_sq:
                    edge_in_heap.append(1)
                    heapq.heappush(heap, (nd2, eid_new))
                else:
                    edge_in_heap.append(0)
                new_eids[i] = eid_new
            else:
                for j, x in enumerate(adj_verts[w]):
                    if x == target:
                        new_eids[i] = edge_ids[w][j]
                        break
        edge_ids[target] = new_eids

        collapse_cnt += 1

    # ---------- final compaction ----------
    alive_faces = [fi for fi in range(len(face_alive)) if face_alive[fi]]
    new_F = array('I')
    for fi in alive_faces:
        new_F.extend(F_flat[3*fi:3*fi+3])
    used = set()
    for i in range(0, len(new_F), 3):
        used.add(new_F[i])
        used.add(new_F[i+1])
        used.add(new_F[i+2])
    old2new = {}
    new_V = array('d')
    for idx in sorted(used):
        old2new[idx] = len(new_V)//3
        new_V.extend(V_flat[3*idx:3*idx+3])
    for i in range(0, len(new_F), 3):
        new_F[i] = old2new[new_F[i]]
        new_F[i+1] = old2new[new_F[i+1]]
        new_F[i+2] = old2new[new_F[i+2]]
    V_flat = new_V
    F_flat = new_F

load_obj()
simplify()
save_obj()
