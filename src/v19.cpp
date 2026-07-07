#include "Eigen/Dense"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <limits>
#include <chrono>
#include <omp.h>

using namespace std;

// ============================================================================
// Basic geometry
// ============================================================================
struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
};
static double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static double norm2(const Vec3& v) { return dot(v, v); }
static double norm(const Vec3& v) { return sqrt(norm2(v)); }
static bool finiteVec(const Vec3& v) { return isfinite(v.x) && isfinite(v.y) && isfinite(v.z); }

struct Face { int v[3]; };

// ============================================================================
// Quadric error metric
// ============================================================================
struct Quadric {
    double a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0, j = 0;
    Quadric& operator+=(const Quadric& o) {
        a += o.a; b += o.b; c += o.c; d += o.d;
        e += o.e; f += o.f; g += o.g; h += o.h; i += o.i; j += o.j;
        return *this;
    }
    void scale(double s) {
        a *= s; b *= s; c *= s; d *= s; e *= s;
        f *= s; g *= s; h *= s; i *= s; j *= s;
    }
    static Quadric fromTriangle(const Vec3& p0, const Vec3& p1, const Vec3& p2, double w = 1.0) {
        Vec3 n = cross(p1 - p0, p2 - p0);
        double area = norm(n);
        if (area < 1e-12) return Quadric();
        n = n / area;           // area‑weighted normal
        double pd = -dot(n, p0);
        Quadric q;
        q.a = n.x * n.x; q.b = n.x * n.y; q.c = n.x * n.z; q.d = n.x * pd;
        q.e = n.y * n.y; q.f = n.y * n.z; q.g = n.y * pd;
        q.h = n.z * n.z; q.i = n.z * pd; q.j = pd * pd;
        q.scale(w * area * 0.5);
        return q;
    }
    double evaluate(const Vec3& p) const {
        return a * p.x * p.x + 2 * b * p.x * p.y + 2 * c * p.x * p.z + 2 * d * p.x
             + e * p.y * p.y + 2 * f * p.y * p.z + 2 * g * p.y
             + h * p.z * p.z + 2 * i * p.z + j;
    }
};

// ============================================================================
// Small ordered set for adjacency
// ============================================================================
struct SmallSet {
    vector<int> data;
    void insert(int v) {
        auto it = lower_bound(data.begin(), data.end(), v);
        if (it == data.end() || *it != v) data.insert(it, v);
    }
    void erase(int v) {
        auto it = lower_bound(data.begin(), data.end(), v);
        if (it != data.end() && *it == v) data.erase(it);
    }
    bool contains(int v) const { return binary_search(data.begin(), data.end(), v); }
    int size() const { return (int)data.size(); }
    const vector<int>& get() const { return data; }
    void clear() { data.clear(); }
};

// ============================================================================
// Parameters – moderate compression with normal guard
// ============================================================================
static constexpr double HausdorffFrac = 0.05;
static constexpr double CostCapCoeff  = 0.0375;
static constexpr double ViewWeightK   = 3.0;
static constexpr double MaxFaceWeight = 3.0;
static constexpr double CoplanarTime  = 0.5;
static constexpr double TotalQEMTime  = 20.0;
static constexpr double CoplanarNormalEps = 1e-5;
static constexpr double CoplanarOffsetEps = 1e-5;

// Normal deviation threshold (degrees) – reject collapse if any incident face
// would change its normal by more than this angle.
static constexpr double MaxNormalChangeDeg = 20.0;

// Keep ratios – aggressive but safe thanks to normal guard
static double targetKeepRatio(int nV) {
    if (nV <= 5000)   return 0.05;
    if (nV <= 25000)  return 0.45;
    if (nV <= 45000)  return 0.35;
    if (nV <= 50000)  return 0.25;
    if (nV <= 400000) return 0.1;
    return 0.05;
}

// ============================================================================
// I/O (unchanged)
// ============================================================================
using MeshV = Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor>;
using MeshF = Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor>;
static MeshV V;
static MeshF F;

static vector<char> slurp_stdin() {
    vector<char> buf; buf.reserve(1 << 27);
    char chunk[1 << 16]; size_t n;
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
    V.resize(nv, 3); F.resize(nf, 3);
    for (long i = 0; i < nv; ++i) {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p;
        ++p; V(i, 0) = strtod(p, &p); V(i, 1) = strtod(p, &p); V(i, 2) = strtod(p, &p);
    }
    for (long i = 0; i < nf; ++i) {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p;
        ++p; F(i, 0) = (int)strtol(p, &p, 10) - 1;
        F(i, 1) = (int)strtol(p, &p, 10) - 1;
        F(i, 2) = (int)strtol(p, &p, 10) - 1;
    }
}

static void save_obj() {
    string out; out.reserve((size_t)V.rows() * 40 + (size_t)F.rows() * 24 + 32);
    char line[96];
    out.append(line, snprintf(line, sizeof line, "%ld %ld\n", (long)V.rows(), (long)F.rows()));
    for (Eigen::Index i = 0; i < V.rows(); ++i)
        out.append(line, snprintf(line, sizeof line, "v %.10g %.10g %.10g\n", V(i, 0), V(i, 1), V(i, 2)));
    for (Eigen::Index i = 0; i < F.rows(); ++i)
        out.append(line, snprintf(line, sizeof line, "f %d %d %d\n", F(i, 0) + 1, F(i, 1) + 1, F(i, 2) + 1));
    fwrite(out.data(), 1, out.size(), stdout);
}

// ============================================================================
// Main simplification – QEM + normal change guard
// ============================================================================
static void simplify() {
    int nV = (int)V.rows(), nF = (int)F.rows();
    vector<Vec3> verts(nV);
    for (int i = 0; i < nV; ++i) verts[i] = Vec3(V(i, 0), V(i, 1), V(i, 2));
    vector<Face> faces(nF);
    for (int i = 0; i < nF; ++i) {
        faces[i].v[0] = F(i, 0); faces[i].v[1] = F(i, 1); faces[i].v[2] = F(i, 2);
    }

    vector<char> vdead(nV, 0), fdead(nF, 0);
    vector<int> vver(nV, 0);
    vector<Quadric> vquad(nV);
    vector<double> vradius(nV, 0.0);
    vector<vector<int>> vfaces(nV);
    vector<SmallSet> vneigh(nV);

    for (int fi = 0; fi < nF; ++fi) {
        const Face& f = faces[fi];
        for (int k = 0; k < 3; ++k) vfaces[f.v[k]].push_back(fi);
        for (int k = 0; k < 3; ++k) {
            int a = f.v[k], b = f.v[(k + 1) % 3];
            if (a != b) { vneigh[a].insert(b); vneigh[b].insert(a); }
        }
    }

    Vec3 mn = verts[0], mx = verts[0];
    for (auto& p : verts) {
        mn.x = min(mn.x, p.x); mx.x = max(mx.x, p.x);
        mn.y = min(mn.y, p.y); mx.y = max(mx.y, p.y);
        mn.z = min(mn.z, p.z); mx.z = max(mx.z, p.z);
    }
    double diag = norm(mx - mn);
    double hausd = HausdorffFrac * diag;
    double costCap = CostCapCoeff * diag * diag;
    double invDiag2 = 1.0 / (diag * diag);

    // Original face normals (never updated) and view‑dependent weights
    vector<Vec3> origN(nF);
    vector<double> origArea(nF);
    for (int fi = 0; fi < nF; ++fi) {
        Vec3 n = cross(verts[faces[fi].v[1]] - verts[faces[fi].v[0]],
                       verts[faces[fi].v[2]] - verts[faces[fi].v[0]]);
        double len = norm(n);
        origArea[fi] = 0.5 * len;
        origN[fi] = len > 1e-30 ? n / len : Vec3(0, 0, 0);
    }

    vector<double> faceWeight(nF, 1.0);
    #pragma omp parallel for schedule(static)
    for (int fi = 0; fi < nF; ++fi) {
        const Vec3& n = origN[fi];
        double area = origArea[fi];
        if (area < 1e-30) continue;
        double absSum = fabs(n.x) + fabs(n.y) + fabs(n.z);
        double w = 1.0 + ViewWeightK * area * invDiag2 * absSum;
        if (w > MaxFaceWeight) w = MaxFaceWeight;
        faceWeight[fi] = w;
    }

    // Quadric accumulation
    vquad.assign(nV, Quadric());
    #pragma omp parallel
    {
        vector<Quadric> localQ(nV);
        #pragma omp for schedule(static)
        for (int fi = 0; fi < nF; ++fi) {
            if (fdead[fi]) continue;
            Quadric q = Quadric::fromTriangle(verts[faces[fi].v[0]],
                                               verts[faces[fi].v[1]],
                                               verts[faces[fi].v[2]], faceWeight[fi]);
            for (int k = 0; k < 3; ++k) localQ[faces[fi].v[k]] += q;
        }
        #pragma omp critical
        { for (int i = 0; i < nV; ++i) vquad[i] += localQ[i]; }
    }

    // Helpers
    auto countCommonFaces = [&](int a, int b) {
        int cnt = 0;
        const auto& fa = vfaces[a], & fb = vfaces[b];
        for (int fi : fa) if (!fdead[fi])
            for (int fj : fb) if (fi == fj) { ++cnt; break; }
        return cnt;
    };
    auto countCommonNeighbors = [&](int a, int b) {
        int cnt = 0;
        const auto& na = vneigh[a], & nb = vneigh[b];
        for (int x : na.get()) if (x != a && x != b && !vdead[x] && nb.contains(x)) ++cnt;
        return cnt;
    };
    auto edgeExists = [&](int a, int b) {
        return a != b && !vdead[a] && !vdead[b] && vneigh[a].contains(b);
    };

    // QEM solve
    auto solveQem = [&](const Quadric& q, Vec3& out) -> bool {
        double D = q.a*(q.e*q.h - q.f*q.f) - q.b*(q.b*q.h - q.f*q.c) + q.c*(q.b*q.f - q.e*q.c);
        if (fabs(D) < 1e-8) return false;
        double detX = -q.d*(q.e*q.h - q.f*q.f) + q.b*(q.g*q.h - q.i*q.f) - q.c*(q.g*q.f - q.e*q.i);
        double detY =  q.a*(-q.g*q.h + q.i*q.f) + q.d*(q.b*q.h - q.f*q.c) - q.c*(q.b*q.i - q.g*q.c);
        double detZ =  q.a*(q.e*q.i - q.g*q.f) - q.b*(q.b*q.i - q.g*q.c) + q.d*(q.b*q.f - q.e*q.c);
        out.x = detX / D; out.y = detY / D; out.z = detZ / D;
        return finiteVec(out);
    };

    // Check normal deviation for all faces incident to the edge (excluding the two deleted)
    auto normalChangeOK = [&](int a, int b, const Vec3& p, int absorbed) -> bool {
        const double cosLimit = cos(MaxNormalChangeDeg * M_PI / 180.0);
        for (int vi : {a, b}) {
            for (int fi : vfaces[vi]) {
                if (fdead[fi]) continue;
                const Face& f = faces[fi];
                bool hasA = (f.v[0]==a || f.v[1]==a || f.v[2]==a);
                bool hasB = (f.v[0]==b || f.v[1]==b || f.v[2]==b);
                if (hasA && hasB) continue; // will be deleted
                // simulate new positions: absorbed vertex moves to p, the other remains
                Vec3 newV[3];
                for (int k = 0; k < 3; ++k) {
                    int idx = f.v[k];
                    if (idx == absorbed) newV[k] = p;
                    else if (idx == (a + b - absorbed)) newV[k] = p; // both merge to p
                    else newV[k] = verts[idx];
                }
                Vec3 nn = cross(newV[1] - newV[0], newV[2] - newV[0]);
                double len = norm(nn);
                if (len < 1e-12) return false; // degenerate
                nn = nn / len;
                if (dot(nn, origN[fi]) < cosLimit) return false;
            }
        }
        return true;
    };

    struct Collapse {
        int absorbed, kept;
        int verA, verB;
        double cost;
        Vec3 newPos;
    };

    auto computeBestValid = [&](int a, int b) -> Collapse {
        if (!edgeExists(a, b)) return {-1, -1, 0, 0, 1e100, {}};

        Quadric qab = vquad[a]; qab += vquad[b];
        vector<Vec3> cands = { verts[a], verts[b], (verts[a] + verts[b]) * 0.5 };
        Vec3 qp;
        if (solveQem(qab, qp)) cands.push_back(qp);

        Collapse best{-1, -1, 0, 0, 1e100, {}};
        for (const Vec3& p : cands) {
            if (!finiteVec(p)) continue;
            // Forward envelope
            double maxDist = max(vradius[a] + norm(verts[a] - p),
                                 vradius[b] + norm(verts[b] - p));
            if (maxDist > hausd) continue;

            double cost = qab.evaluate(p);
            for (int dir = 0; dir < 2; ++dir) {
                int absorbed = dir ? b : a;
                int kept     = dir ? a : b;
                if (countCommonFaces(absorbed, kept) != 2) continue;
                if (countCommonNeighbors(absorbed, kept) != 2) continue;
                if (!normalChangeOK(a, b, p, absorbed)) continue;  // perceptual guard
                if (cost < best.cost) {
                    best = {absorbed, kept, vver[absorbed], vver[kept], cost, p};
                }
            }
        }
        return best;
    };

    auto applyCollapse = [&](int absorbed, int kept, const Vec3& np) {
        verts[kept] = np;
        vradius[kept] = max(vradius[absorbed] + norm(verts[absorbed] - np), vradius[kept]);
        vdead[absorbed] = 1;
        ++vver[absorbed]; ++vver[kept];

        auto& af = vfaces[absorbed];
        vector<int> deadFaces;
        for (int fi : af) {
            if (fdead[fi]) continue;
            Face& f = faces[fi];
            bool touch = false;
            for (int k = 0; k < 3; ++k)
                if (f.v[k] == absorbed) { f.v[k] = kept; touch = true; }
            if (!touch) continue;
            if (f.v[0] == f.v[1] || f.v[1] == f.v[2] || f.v[0] == f.v[2]) {
                fdead[fi] = 1;
                deadFaces.push_back(fi);
            } else {
                vfaces[kept].push_back(fi);
            }
        }
        for (int fi : deadFaces) {
            for (int k = 0; k < 3; ++k) {
                int v = faces[fi].v[k];
                auto& vf = vfaces[v];
                vf.erase(remove(vf.begin(), vf.end(), fi), vf.end());
            }
        }
        vfaces[absorbed].clear();

        vquad[kept] += vquad[absorbed];

        for (int nb : vneigh[absorbed].get()) {
            if (nb == kept || vdead[nb]) continue;
            vneigh[nb].erase(absorbed);
            vneigh[nb].insert(kept);
            vneigh[kept].insert(nb);
        }
        vneigh[absorbed].clear();
        vneigh[kept].erase(absorbed);
        vneigh[kept].erase(kept);
    };

    // ========================================================================
    // Phase 0: coplanar edge collapse
    // ========================================================================
    {
        struct CopItem { double cost; int a, b; int verA, verB; };
        auto cmp = [](const CopItem& x, const CopItem& y) { return x.cost > y.cost; };
        priority_queue<CopItem, vector<CopItem>, decltype(cmp)> cq(cmp);

        for (int a = 0; a < nV; ++a) {
            if (vdead[a]) continue;
            for (int b : vneigh[a].get()) {
                if (b <= a) continue;
                int fc = 0;
                Vec3 norms[2]; double offsets[2];
                for (int fi : vfaces[a]) {
                    if (fdead[fi]) continue;
                    const Face& f = faces[fi];
                    if (f.v[0] != b && f.v[1] != b && f.v[2] != b) continue;
                    Vec3 n = origN[fi];
                    double d = -dot(n, verts[f.v[0]]);
                    if (fc < 2) { norms[fc] = n; offsets[fc] = d; ++fc; }
                }
                if (fc == 2 &&
                    1.0 - fabs(dot(norms[0], norms[1])) < CoplanarNormalEps &&
                    fabs(offsets[0] - offsets[1]) < CoplanarOffsetEps)
                {
                    cq.push({norm(verts[a] - verts[b]), a, b, vver[a], vver[b]});
                }
            }
        }

        double t0 = chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count();
        auto elapsed = [&]() {
            return chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count() - t0;
        };

        while (!cq.empty() && elapsed() < CoplanarTime) {
            auto item = cq.top(); cq.pop();
            int a = item.a, b = item.b;
            if (vdead[a] || vdead[b]) continue;
            if (item.verA != vver[a] || item.verB != vver[b]) continue;
            auto best = computeBestValid(a, b);
            if (best.absorbed < 0 || best.cost > costCap) continue;
            applyCollapse(best.absorbed, best.kept, best.newPos);
            for (int nb : vneigh[best.kept].get()) {
                if (vdead[nb] || nb == best.kept) continue;
                auto nc = computeBestValid(best.kept, nb);
                if (nc.absorbed >= 0)
                    cq.push({nc.cost, best.kept, nb, nc.verA, nc.verB});
            }
        }
    }

    // ========================================================================
    // Phase 1: QEM edge collapse with normal guard
    // ========================================================================
    using PQ = priority_queue<Collapse, vector<Collapse>,
          function<bool(const Collapse&, const Collapse&)>>;
    PQ pq([](const Collapse& x, const Collapse& y) { return x.cost > y.cost; });

    for (int a = 0; a < nV; ++a) {
        if (vdead[a]) continue;
        for (int b : vneigh[a].get()) {
            if (b <= a) continue;
            auto c = computeBestValid(a, b);
            if (c.absorbed >= 0) pq.push(c);
        }
    }

    double startTime = chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count();
    auto elapsedQEM = [&]() {
        return chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count() - startTime;
    };

    int targetVerts = max(10, (int)floor(nV * targetKeepRatio(nV)));
    int collapseLimit = nV - targetVerts;
    int accepted = 0;

    while (accepted < collapseLimit && !pq.empty() && elapsedQEM() < TotalQEMTime) {
        auto c = pq.top(); pq.pop();
        int a = c.absorbed, b = c.kept;
        if (!edgeExists(a, b)) continue;
        if (c.verA != vver[a] || c.verB != vver[b]) {
            auto nc = computeBestValid(a, b);
            if (nc.absorbed >= 0) pq.push(nc);
            continue;
        }
        if (c.cost > costCap) break;

        auto best = computeBestValid(a, b);
        if (best.absorbed < 0 || best.cost > costCap) continue;

        applyCollapse(best.absorbed, best.kept, best.newPos);
        ++accepted;

        for (int nb : vneigh[best.kept].get()) {
            if (vdead[nb] || nb == best.kept) continue;
            auto nc = computeBestValid(best.kept, nb);
            if (nc.absorbed >= 0) pq.push(nc);
        }
    }

    // ========================================================================
    // Final compaction and output
    // ========================================================================
    vector<int> old2new(nV, -1);
    vector<Vec3> newVerts;
    newVerts.reserve(nV);
    for (int i = 0; i < nV; ++i) {
        if (!vdead[i]) {
            old2new[i] = (int)newVerts.size();
            newVerts.push_back(verts[i]);
        }
    }

    vector<Face> newFaces;
    newFaces.reserve(faces.size());
    for (int fi = 0; fi < (int)faces.size(); ++fi) {
        if (fi < nF && fdead[fi]) continue;
        int a = faces[fi].v[0], b = faces[fi].v[1], c = faces[fi].v[2];
        if (vdead[a] || vdead[b] || vdead[c]) continue;
        int na = old2new[a], nb = old2new[b], nc = old2new[c];
        if (na < 0 || nb < 0 || nc < 0 || na == nb || nb == nc || na == nc) continue;
        newFaces.push_back({na, nb, nc});
    }

    sort(newFaces.begin(), newFaces.end(), [](const Face& x, const Face& y) {
        array<int,3> kx{x.v[0], x.v[1], x.v[2]}, ky{y.v[0], y.v[1], y.v[2]};
        sort(kx.begin(), kx.end()); sort(ky.begin(), ky.end());
        return kx < ky;
    });
    newFaces.erase(unique(newFaces.begin(), newFaces.end(), [](const Face& x, const Face& y) {
        array<int,3> kx{x.v[0], x.v[1], x.v[2]}, ky{y.v[0], y.v[1], y.v[2]};
        sort(kx.begin(), kx.end()); sort(ky.begin(), ky.end());
        return kx == ky;
    }), newFaces.end());

    V.resize((int)newVerts.size(), 3);
    for (int i = 0; i < (int)newVerts.size(); ++i) {
        V(i, 0) = newVerts[i].x; V(i, 1) = newVerts[i].y; V(i, 2) = newVerts[i].z;
    }
    F.resize((int)newFaces.size(), 3);
    for (int i = 0; i < (int)newFaces.size(); ++i) {
        F(i, 0) = newFaces[i].v[0]; F(i, 1) = newFaces[i].v[1]; F(i, 2) = newFaces[i].v[2];
    }
}

int main() {
    load_obj();
    simplify();
    save_obj();
    return 0;
}
