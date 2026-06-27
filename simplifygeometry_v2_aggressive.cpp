// Aggressive tuned simplifier for "Perception-Aware Lossless Simplification of
// 3D Meshes".
//
// This is a new solver version; simplifygeometry.cpp is intentionally left
// untouched. It keeps the proven endpoint-only QEM edge-collapse core and
// pushes compression with a calibrated normal-change threshold. Endpoint-only
// placement is less glamorous than free QEM placement, but it tested better:
// every output vertex remains on the original surface, the cluster-radius
// Hausdorff proxy stays cheap and conservative, and the local evaluator keeps
// all representative meshes above the 0.9 SSIM gate.
//
// A collapse is applied only when it preserves a closed 2-manifold and respects
// the evaluator's gates:
//   * link condition (exactly two shared neighbours for a closed manifold);
//   * no degenerate (zero-area) face and no face-normal flip;
//   * cluster-radius proxy for the symmetric Hausdorff bound.
//
// To run:
//   g++ -O2 -std=c++17 simplifygeometry_v2_aggressive.cpp -o simplifygeometry_v2
//   ./simplifygeometry_v2 < mesh.in > mesh.out

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

// Mesh representation.
//   V : vertex coordinates (x, y, z).
//   F : faces (a, b, c), 0-indexed (input is 1-indexed; load_obj subtracts 1,
//       save_obj adds it back).
static vector<array<double, 3>> V;
static vector<array<int, 3>> F;

// --- fast input -------------------------------------------------------------

static vector<char> slurp_stdin() {
    vector<char> buf;
    buf.reserve(1 << 20);
    char chunk[1 << 16];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), stdin)) > 0)
        buf.insert(buf.end(), chunk, chunk + n);
    buf.push_back('\0');
    return buf;
}

static void load_obj() {
    vector<char> buf = slurp_stdin();
    char* p = buf.data();

    long nv = strtol(p, &p, 10);
    long nf = strtol(p, &p, 10);
    V.resize(nv);
    F.resize(nf);

    for (long i = 0; i < nv; ++i) {
        // skip the leading 'v'
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p;
        ++p;
        V[i][0] = strtod(p, &p);
        V[i][1] = strtod(p, &p);
        V[i][2] = strtod(p, &p);
    }
    for (long i = 0; i < nf; ++i) {
        // skip the leading 'f'
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p;
        ++p;
        F[i][0] = (int)strtol(p, &p, 10) - 1;
        F[i][1] = (int)strtol(p, &p, 10) - 1;
        F[i][2] = (int)strtol(p, &p, 10) - 1;
    }
}

// --- fast output ------------------------------------------------------------

// Print the mesh. Print 10 significant digits using %.10g for performance.
static void save_obj() {
    string out;
    out.reserve(V.size() * 40 + F.size() * 24 + 32);
    char line[96];

    out.append(line, snprintf(line, sizeof line, "%zu %zu\n",
                              V.size(), F.size()));
    for (const auto& v : V)
        out.append(line, snprintf(line, sizeof line, "v %.10g %.10g %.10g\n",
                                  v[0], v[1], v[2]));
    for (const auto& f : F)
        out.append(line, snprintf(line, sizeof line, "f %d %d %d\n",
                                  f[0] + 1, f[1] + 1, f[2] + 1));

    fwrite(out.data(), 1, out.size(), stdout);
}

// --- your implementation ----------------------------------------------------

// Numerical tolerances.
static const double EPS_AREA = 1e-12;     // reject faces whose area drops to ~0
// Tuned locally: 0.15 failed one representative mesh, while 0.18 passed all
// local 1024-resolution scenarios and recovered meaningful extra compression.
static const double NORMAL_FLIP_COS = 0.18;
static const double HAUSDORFF_FRAC = 0.05;  // bound = 0.05 * AABB diagonal

using Quadric = array<double, 10>;

// Unit normal (a, b, c) and offset d of the plane through p, q, r, plus the
// triangle area. a*x + b*y + c*z + d = 0 on the plane; a degenerate triangle
// yields area 0 and a zero normal.
static inline void face_plane(const array<double, 3>& p,
                              const array<double, 3>& q,
                              const array<double, 3>& r,
                              double& a, double& b, double& c, double& d,
                              double& area) {
    double ux = q[0] - p[0], uy = q[1] - p[1], uz = q[2] - p[2];
    double vx = r[0] - p[0], vy = r[1] - p[1], vz = r[2] - p[2];
    double nx = uy * vz - uz * vy;
    double ny = uz * vx - ux * vz;
    double nz = ux * vy - uy * vx;
    double length = sqrt(nx * nx + ny * ny + nz * nz);
    area = 0.5 * length;
    if (length == 0.0) {
        a = b = c = d = 0.0;
        return;
    }
    nx /= length;
    ny /= length;
    nz /= length;
    a = nx;
    b = ny;
    c = nz;
    d = -(nx * p[0] + ny * p[1] + nz * p[2]);
}

// Area-weighted quadric (10 unique entries) of a single plane.
static inline Quadric plane_quadric(double a, double b, double c, double d,
                                    double w) {
    return Quadric{w * a * a, w * a * b, w * a * c, w * a * d,
                   w * b * b, w * b * c, w * b * d,
                   w * c * c, w * c * d,
                   w * d * d};
}

static inline void quadric_add(Quadric& into, const Quadric& other) {
    for (int i = 0; i < 10; ++i) into[i] += other[i];
}

// Evaluate v^T Q v for the 10-entry symmetric quadric q at point v.
static inline double quadric_error(const Quadric& q,
                                   const array<double, 3>& v) {
    double x = v[0], y = v[1], z = v[2];
    return q[0] * x * x + 2.0 * q[1] * x * y + 2.0 * q[2] * x * z
           + 2.0 * q[3] * x
           + q[4] * y * y + 2.0 * q[5] * y * z + 2.0 * q[6] * y
           + q[7] * z * z + 2.0 * q[8] * z
           + q[9];
}

// Heap entry: a candidate edge (a, b) with its cost and endpoint versions.
struct Edge {
    double cost;
    int a, b;
    int va, vb;
};

// Min-heap by cost, then endpoints (mirrors Python's heapq tuple ordering).
struct EdgeGreater {
    bool operator()(const Edge& x, const Edge& y) const {
        if (x.cost != y.cost) return x.cost > y.cost;
        if (x.a != y.a) return x.a > y.a;
        return x.b > y.b;
    }
};

// Optimize the mesh: replace V and F.
static void simplify() {
    int nv = (int)V.size();
    if (nv == 0 || F.empty()) return;

    // Mutable coordinates and original coordinates (for Hausdorff).
    vector<array<double, 3>> coords = V;
    const vector<array<double, 3>>& orig = V;

    // AABB diagonal of the original mesh -> Hausdorff bound.
    double minx = orig[0][0], maxx = orig[0][0];
    double miny = orig[0][1], maxy = orig[0][1];
    double minz = orig[0][2], maxz = orig[0][2];
    for (const auto& p : orig) {
        minx = min(minx, p[0]); maxx = max(maxx, p[0]);
        miny = min(miny, p[1]); maxy = max(maxy, p[1]);
        minz = min(minz, p[2]); maxz = max(maxz, p[2]);
    }
    double dx = maxx - minx, dy = maxy - miny, dz = maxz - minz;
    double diag = sqrt(dx * dx + dy * dy + dz * dz);
    double hbound = HAUSDORFF_FRAC * diag;
    double hbound_sq = hbound * hbound;

    // Connectivity. faces[f] uses {-1,-1,-1} once removed.
    int nf = (int)F.size();
    vector<array<int, 3>> faces = F;
    vector<unordered_set<int>> vfaces(nv);  // vertex -> incident face ids
    vector<unordered_set<int>> nbrs(nv);    // vertex -> adjacent vertices
    for (int fid = 0; fid < nf; ++fid) {
        int a = faces[fid][0], b = faces[fid][1], c = faces[fid][2];
        vfaces[a].insert(fid);
        vfaces[b].insert(fid);
        vfaces[c].insert(fid);
        nbrs[a].insert(b); nbrs[a].insert(c);
        nbrs[b].insert(a); nbrs[b].insert(c);
        nbrs[c].insert(a); nbrs[c].insert(b);
    }

    // Per-vertex quadric (area-weighted sum of incident face plane quadrics).
    vector<Quadric> quad(nv);
    for (auto& q : quad) q.fill(0.0);
    for (int fid = 0; fid < nf; ++fid) {
        int a = faces[fid][0], b = faces[fid][1], c = faces[fid][2];
        double na, nb, nc, d, area;
        face_plane(coords[a], coords[b], coords[c], na, nb, nc, d, area);
        if (area <= 0.0) continue;
        Quadric qe = plane_quadric(na, nb, nc, d, area);
        quadric_add(quad[a], qe);
        quadric_add(quad[b], qe);
        quadric_add(quad[c], qe);
    }

    vector<char> alive(nv, 1);
    // Original vertices currently represented by each live vertex (cluster).
    vector<vector<int>> cluster(nv);
    for (int i = 0; i < nv; ++i) cluster[i].push_back(i);
    vector<int> version(nv, 0);  // bumped when a vertex's neighbourhood changes

    priority_queue<Edge, vector<Edge>, EdgeGreater> heap;

    // Cheapest endpoint placement for edge (a, b): cost, keep, drop.
    auto best_target = [&](int a, int b, double& cost, int& keep, int& drop) {
        Quadric comb;
        for (int i = 0; i < 10; ++i) comb[i] = quad[a][i] + quad[b][i];
        double ca = quadric_error(comb, coords[a]);
        double cb = quadric_error(comb, coords[b]);
        if (ca <= cb) {
            cost = ca; keep = a; drop = b;
        } else {
            cost = cb; keep = b; drop = a;
        }
    };

    auto push_edge = [&](int a, int b) {
        double cost;
        int keep, drop;
        best_target(a, b, cost, keep, drop);
        // Store endpoint versions so stale entries can be skipped on pop.
        heap.push(Edge{cost, a, b, version[a], version[b]});
    };

    for (int a = 0; a < nv; ++a)
        for (int b : nbrs[a])
            if (a < b) push_edge(a, b);

    // Validate collapsing `drop` into `keep` (keep's position is fixed).
    // Returns true and fills edge_faces (size 2) on success.
    auto collapse_ok = [&](int keep, int drop,
                           array<int, 2>& edge_faces) -> bool {
        // Closed-manifold link condition: exactly two shared neighbours, and
        // exactly two faces shared by the edge endpoints.
        int shared_count = 0;
        // Collect shared neighbours.
        const unordered_set<int>& nk = nbrs[keep];
        const unordered_set<int>& nd = nbrs[drop];
        const unordered_set<int>* small = nk.size() <= nd.size() ? &nk : &nd;
        const unordered_set<int>* large = small == &nk ? &nd : &nk;
        int shared0 = -1, shared1 = -1;
        for (int v : *small) {
            if (large->count(v)) {
                if (shared_count == 0) shared0 = v;
                else if (shared_count == 1) shared1 = v;
                ++shared_count;
                if (shared_count > 2) return false;
            }
        }
        if (shared_count != 2) return false;

        // Edge faces = faces incident to both keep and drop.
        int ef_count = 0;
        const unordered_set<int>& fk = vfaces[keep];
        const unordered_set<int>& fd = vfaces[drop];
        const unordered_set<int>* sf = fk.size() <= fd.size() ? &fk : &fd;
        const unordered_set<int>* lf = sf == &fk ? &fd : &fk;
        for (int fid : *sf) {
            if (lf->count(fid)) {
                if (ef_count < 2) edge_faces[ef_count] = fid;
                ++ef_count;
                if (ef_count > 2) return false;
            }
        }
        if (ef_count != 2) return false;

        // The shared neighbours must be exactly the two vertices opposite the
        // edge in its incident faces; otherwise the collapse folds the surface.
        int opp0 = -1, opp1 = -1, opp_count = 0;
        for (int e = 0; e < 2; ++e) {
            int fid = edge_faces[e];
            for (int j = 0; j < 3; ++j) {
                int v = faces[fid][j];
                if (v != keep && v != drop) {
                    if (opp_count == 0) opp0 = v;
                    else opp1 = v;
                    ++opp_count;
                }
            }
        }
        // opposite set must equal shared set (both size 2).
        bool eq = (opp0 == shared0 && opp1 == shared1)
                  || (opp0 == shared1 && opp1 == shared0);
        if (!eq) return false;

        const array<double, 3>& kp = coords[keep];
        // Hausdorff cluster-radius proxy: every original vertex represented by
        // `keep` must stay within the bound of keep's position.
        for (int oi : cluster[drop]) {
            const array<double, 3>& op = orig[oi];
            double ex = op[0] - kp[0], ey = op[1] - kp[1], ez = op[2] - kp[2];
            if (ex * ex + ey * ey + ez * ez > hbound_sq) return false;
        }

        // Degeneracy / normal-flip check on every face that survives the
        // collapse and currently touches `drop`.
        for (int fid : vfaces[drop]) {
            if (fid == edge_faces[0] || fid == edge_faces[1]) continue;
            int a = faces[fid][0], b = faces[fid][1], c = faces[fid][2];
            const array<double, 3>& oa = coords[a];
            const array<double, 3>& ob = coords[b];
            const array<double, 3>& oc = coords[c];
            double o_na, o_nb, o_nc, o_d, o_area;
            face_plane(oa, ob, oc, o_na, o_nb, o_nc, o_d, o_area);
            if (o_area <= 0.0) return false;
            const array<double, 3>& na = (a == drop) ? kp : oa;
            const array<double, 3>& nb = (b == drop) ? kp : ob;
            const array<double, 3>& nc = (c == drop) ? kp : oc;
            double n_na, n_nb, n_nc, n_d, n_area;
            face_plane(na, nb, nc, n_na, n_nb, n_nc, n_d, n_area);
            if (n_area <= EPS_AREA) return false;
            if (o_na * n_na + o_nb * n_nb + o_nc * n_nc <= NORMAL_FLIP_COS)
                return false;
        }
        return true;
    };

    auto do_collapse = [&](int keep, int drop,
                           const array<int, 2>& edge_faces) {
        // Remove the two faces incident to the collapsed edge.
        for (int e = 0; e < 2; ++e) {
            int fid = edge_faces[e];
            for (int j = 0; j < 3; ++j) vfaces[faces[fid][j]].erase(fid);
            faces[fid] = {-1, -1, -1};
        }

        // Rewire the remaining faces of `drop` onto `keep`.
        vector<int> drop_faces(vfaces[drop].begin(), vfaces[drop].end());
        for (int fid : drop_faces) {
            for (int j = 0; j < 3; ++j)
                if (faces[fid][j] == drop) faces[fid][j] = keep;
            vfaces[keep].insert(fid);
        }

        // Merge quadrics and clusters; retire `drop`.
        quadric_add(quad[keep], quad[drop]);
        cluster[keep].insert(cluster[keep].end(),
                             cluster[drop].begin(), cluster[drop].end());

        // affected = (nbrs[keep] | nbrs[drop]) - {keep, drop}
        unordered_set<int> affected;
        for (int v : nbrs[keep])
            if (v != keep && v != drop) affected.insert(v);
        for (int v : nbrs[drop])
            if (v != keep && v != drop) affected.insert(v);

        alive[drop] = 0;
        vfaces[drop].clear();
        nbrs[drop].clear();
        cluster[drop].clear();

        // Rebuild adjacency for every vertex touched by the collapse.
        affected.insert(keep);
        for (int v : affected) {
            unordered_set<int> nb;
            for (int fid : vfaces[v]) {
                for (int j = 0; j < 3; ++j) {
                    int w = faces[fid][j];
                    if (w != v) nb.insert(w);
                }
            }
            nbrs[v] = move(nb);
            version[v] += 1;
        }

        // Re-rank edges incident to the kept vertex.
        for (int v : nbrs[keep]) push_edge(keep, v);
    };

    while (!heap.empty()) {
        Edge top = heap.top();
        heap.pop();
        int a = top.a, b = top.b;
        if (!alive[a] || !alive[b]) continue;
        if (top.va != version[a] || top.vb != version[b]) continue;  // stale
        if (!nbrs[a].count(b)) continue;  // edge no longer exists

        double cost;
        int keep, drop;
        best_target(a, b, cost, keep, drop);
        array<int, 2> edge_faces{-1, -1};
        if (!collapse_ok(keep, drop, edge_faces)) {
            // Cheapest endpoint placement is invalid; try the other.
            int other_keep = drop, other_drop = keep;
            if (!collapse_ok(other_keep, other_drop, edge_faces)) continue;
            keep = other_keep;
            drop = other_drop;
        }
        do_collapse(keep, drop, edge_faces);
    }

    // Compact: drop retired vertices and reindex faces.
    vector<int> remap(nv, -1);
    vector<array<double, 3>> new_V;
    for (int i = 0; i < nv; ++i) {
        if (alive[i]) {
            remap[i] = (int)new_V.size();
            new_V.push_back(coords[i]);
        }
    }
    vector<array<int, 3>> new_F;
    for (const auto& f : faces) {
        if (f[0] < 0) continue;
        new_F.push_back({remap[f[0]], remap[f[1]], remap[f[2]]});
    }

    V = move(new_V);
    F = move(new_F);
}

int main() {
    load_obj();
    simplify();
    save_obj();
    return 0;
}
