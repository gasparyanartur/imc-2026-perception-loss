// ITERATION 2 — Fixes the Hausdorff bug found in iteration 1.
//
// What iteration 1 got wrong (found empirically, not just theoretically):
//   endpoint-only QEM collapse used a *per-collapse* displacement guard
//   (`maxDisp * aabbDiag`) as a proxy for the Hausdorff constraint. That
//   guard bounds a single step, but says nothing about the accumulated
//   drift after many sequential collapses land near the same region.
//   On cube20.in this produced a real, measured violation:
//     approx one-way d(M,M') = 0.200000  (10.23% of diagonal, limit 5%) -> FAIL
//
// Fix (theory doc, Iteration 3 "Hausdorff proxy is provably an
// over-approximation in one direction and silently unchecked in the
// other"): track, for every surviving vertex, the radius of the cluster of
// original vertices it represents:
//     r_c = max(r_a + |p_a - p_c|, r_b + |p_b - p_c|)
// and reject any collapse whose resulting r_c would exceed the true bound
// 0.05 * Diagonal. This is a provable bound on d(M, M') (every original
// vertex a survivor represents stays within r_c of the survivor), which is
// exactly the one-way term of the symmetric Hausdorff constraint that a
// purely-local per-step guard cannot capture.
//
// Also widens the placement candidate set to {a, b, midpoint}, each checked
// against the same cluster-radius bound, and picks whichever is QEM-cheapest
// among the *valid* candidates (theory doc Solution 3 / Rank 1 candidate set,
// minus the QEM-optimal point for now -- that arrives in iteration 4 since it
// needs its own tighter guard, see theory doc Iteration 3's plane-distance
// refinement).

#include "Eigen/Dense"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <unordered_set>
#include <queue>
#include <cmath>
#include <chrono>

using namespace std;

using MeshV = Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor>;
using MeshF = Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor>;

static MeshV V;
static MeshF F;

static vector<char> slurp_stdin() {
    vector<char> buf;
    buf.reserve(1 << 27);
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
    V.resize(nv, 3);
    F.resize(nf, 3);
    for (long i = 0; i < nv; ++i) {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p;
        ++p;
        V(i, 0) = strtod(p, &p);
        V(i, 1) = strtod(p, &p);
        V(i, 2) = strtod(p, &p);
    }
    for (long i = 0; i < nf; ++i) {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p;
        ++p;
        F(i, 0) = (int)strtol(p, &p, 10) - 1;
        F(i, 1) = (int)strtol(p, &p, 10) - 1;
        F(i, 2) = (int)strtol(p, &p, 10) - 1;
    }
}

static void save_obj(const MeshV& Vo, const MeshF& Fo) {
    string out;
    out.reserve((size_t)Vo.rows() * 40 + (size_t)Fo.rows() * 24 + 32);
    char line[96];
    out.append(line, snprintf(line, sizeof line, "%ld %ld\n",
                              (long)Vo.rows(), (long)Fo.rows()));
    for (Eigen::Index i = 0; i < Vo.rows(); ++i)
        out.append(line, snprintf(line, sizeof line, "v %.10g %.10g %.10g\n",
                                  Vo(i, 0), Vo(i, 1), Vo(i, 2)));
    for (Eigen::Index i = 0; i < Fo.rows(); ++i)
        out.append(line, snprintf(line, sizeof line, "f %d %d %d\n",
                                  Fo(i, 0) + 1, Fo(i, 1) + 1, Fo(i, 2) + 1));
    fwrite(out.data(), 1, out.size(), stdout);
}

struct Quadric {
    double a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;
    Quadric& operator+=(const Quadric& o) {
        a+=o.a;b+=o.b;c+=o.c;d+=o.d;e+=o.e;f+=o.f;g+=o.g;h+=o.h;i+=o.i;j+=o.j;
        return *this;
    }
    double eval(const Eigen::Vector3d& v) const {
        double x=v.x(), y=v.y(), z=v.z();
        return a*x*x + 2*b*x*y + 2*c*x*z + 2*d*x
             +          e*y*y + 2*f*y*z + 2*g*y
             +                   h*z*z + 2*i*z
             +                            j;
    }
    static Quadric fromPlane(const Eigen::Vector3d& n, double dist) {
        Quadric q;
        q.a = n.x()*n.x(); q.b = n.x()*n.y(); q.c = n.x()*n.z(); q.d = n.x()*dist;
        q.e = n.y()*n.y(); q.f = n.y()*n.z(); q.g = n.y()*dist;
        q.h = n.z()*n.z(); q.i = n.z()*dist;
        q.j = dist*dist;
        return q;
    }
};

struct Solver {
    int nv, nf;
    vector<Eigen::Vector3d> pos;
    vector<char> vAlive;
    vector<int> vVersion;
    vector<Quadric> quad;
    vector<double> clusterRadius;              // NEW: iteration 2
    vector<unordered_set<int>> vFaces;
    vector<array<int,3>> face;
    vector<char> fAlive;
    vector<Eigen::Vector3d> fNormal;
    vector<double> fArea;

    double aabbDiag = 1.0;
    double hausdorffBound = 0.05;    // fraction of AABB diagonal -- the ACTUAL contest limit
    double minTriArea = 1e-13;
    double targetRatio = 0.5;

    void build(const MeshV& Vin, const MeshF& Fin) {
        nv = (int)Vin.rows();
        nf = (int)Fin.rows();
        pos.resize(nv);
        for (int i = 0; i < nv; ++i) pos[i] = Vin.row(i).transpose();
        vAlive.assign(nv, 1);
        vVersion.assign(nv, 0);
        quad.assign(nv, Quadric());
        clusterRadius.assign(nv, 0.0);         // every original vertex starts as its own exact cluster
        vFaces.assign(nv, {});
        face.resize(nf);
        fAlive.assign(nf, 1);
        fNormal.resize(nf);
        fArea.resize(nf);

        for (int i = 0; i < nf; ++i) {
            face[i] = {Fin(i,0), Fin(i,1), Fin(i,2)};
            for (int k = 0; k < 3; ++k) vFaces[face[i][k]].insert(i);
        }

        Eigen::Vector3d lo = pos[0], hi = pos[0];
        for (auto& p : pos) { lo = lo.cwiseMin(p); hi = hi.cwiseMax(p); }
        aabbDiag = (hi - lo).norm();
        if (aabbDiag < 1e-9) aabbDiag = 1.0;

        recomputeAllFaceGeom();
        recomputeAllQuadrics();
    }

    void recomputeFaceGeom(int fi) {
        if (!fAlive[fi]) return;
        auto& fc = face[fi];
        Eigen::Vector3d p0 = pos[fc[0]], p1 = pos[fc[1]], p2 = pos[fc[2]];
        Eigen::Vector3d cr = (p1 - p0).cross(p2 - p0);
        double norm = cr.norm();
        fArea[fi] = 0.5 * norm;
        fNormal[fi] = (norm > 1e-18) ? (cr / norm) : Eigen::Vector3d(0,0,0);
    }
    void recomputeAllFaceGeom() { for (int i = 0; i < nf; ++i) recomputeFaceGeom(i); }

    void recomputeAllQuadrics() {
        for (auto& q : quad) q = Quadric();
        for (int fi = 0; fi < nf; ++fi) {
            if (!fAlive[fi] || fArea[fi] <= 0) continue;
            Eigen::Vector3d n = fNormal[fi];
            double dist = -n.dot(pos[face[fi][0]]);
            Quadric q = Quadric::fromPlane(n, dist);
            for (int k = 0; k < 3; ++k) quad[face[fi][k]] += q;
        }
    }

    void neighbors(int v, unordered_set<int>& out) const {
        out.clear();
        for (int fi : vFaces[v]) {
            if (!fAlive[fi]) continue;
            for (int k = 0; k < 3; ++k) {
                int u = face[fi][k];
                if (u != v) out.insert(u);
            }
        }
    }

    bool linkConditionOk(int a, int b) const {
        unordered_set<int> na, nb;
        neighbors(a, na);
        neighbors(b, nb);
        int common = 0;
        for (int x : na) if (nb.count(x)) ++common;
        return common == 2;
    }

    struct QueueItem {
        double cost;
        int a, b;
        int va, vb;
        bool operator>(const QueueItem& o) const { return cost > o.cost; }
    };
    priority_queue<QueueItem, vector<QueueItem>, greater<QueueItem>> pq;

    // NEW: cluster-radius-aware candidate evaluation.
    //   r_new(candidate) = max(r_a + |p_a-cand|, r_b + |p_b-cand|)
    // A candidate is only admissible if r_new <= hausdorffBound * aabbDiag.
    // Among admissible candidates {a, b, midpoint}, pick lowest QEM cost.
    bool bestCollapse(int a, int b, Eigen::Vector3d& outPos, double& outCost, double& outRadius) {
        if (!vAlive[a] || !vAlive[b]) return false;
        if (!linkConditionOk(a, b)) return false;

        Quadric qe = quad[a]; qe += quad[b];
        double bound = hausdorffBound * aabbDiag;

        Eigen::Vector3d candidates[3] = {
            pos[a], pos[b], 0.5 * (pos[a] + pos[b])
        };
        bool found = false;
        double bestCost = 0; Eigen::Vector3d bestPos; double bestRadius = 0;

        for (auto& cand : candidates) {
            double rNew = max(clusterRadius[a] + (pos[a]-cand).norm(),
                               clusterRadius[b] + (pos[b]-cand).norm());
            if (rNew > bound) continue; // provably-safe rejection (Hausdorff proxy)

            if (!validateCollapseGeometry(a, b,
                    (cand - pos[a]).squaredNorm() < 1e-20 ? a :
                    (cand - pos[b]).squaredNorm() < 1e-20 ? b : -1,
                    cand))
                continue;

            double c = qe.eval(cand);
            if (!found || c < bestCost) {
                found = true; bestCost = c; bestPos = cand; bestRadius = rNew;
            }
        }
        if (!found) return false;
        outPos = bestPos; outCost = bestCost; outRadius = bestRadius;
        return true;
    }

    // survivorHint: if candidate coincides with vertex a or b exactly, pass
    // that id so the degeneracy check can skip re-deriving it; -1 for the
    // (new, non-endpoint) midpoint case, which needs a slightly different
    // remap (both a and b are being replaced by a brand-new position).
    bool validateCollapseGeometry(int a, int b, int survivorHint, const Eigen::Vector3d& cand) {
        unordered_set<int> touched = vFaces[a];
        for (int fi : vFaces[b]) touched.insert(fi);

        for (int fi : touched) {
            if (!fAlive[fi]) continue;
            auto fc = face[fi];
            bool hasA = (fc[0]==a||fc[1]==a||fc[2]==a);
            bool hasB = (fc[0]==b||fc[1]==b||fc[2]==b);
            if (hasA && hasB) continue; // removed by the collapse itself

            array<int,3> nf = fc;
            for (int k = 0; k < 3; ++k) if (nf[k] == a || nf[k] == b) nf[k] = -2; // mark "moves to cand"

            Eigen::Vector3d p[3];
            for (int k = 0; k < 3; ++k)
                p[k] = (nf[k] == -2) ? cand : pos[nf[k]];

            Eigen::Vector3d cr = (p[1]-p[0]).cross(p[2]-p[0]);
            double newArea = 0.5 * cr.norm();
            if (newArea < minTriArea) return false;

            Eigen::Vector3d newN = cr / (2.0*newArea);
            if (newN.dot(fNormal[fi]) < 0.35) return false;
        }
        (void)survivorHint;
        return true;
    }

    void pushEdge(int a, int b) {
        Eigen::Vector3d cand; double cost, radius;
        if (!bestCollapse(a, b, cand, cost, radius)) return;
        pq.push({cost, a, b, vVersion[a], vVersion[b]});
    }

    void initQueue() {
        unordered_set<long long> seen;
        for (int fi = 0; fi < nf; ++fi) {
            auto& fc = face[fi];
            for (int k = 0; k < 3; ++k) {
                int u = fc[k], v = fc[(k+1)%3];
                int lo = min(u,v), hi = max(u,v);
                long long key = (long long)lo * 2000000000LL + hi;
                if (seen.insert(key).second) pushEdge(lo, hi);
            }
        }
    }

    int aliveVertexCount = 0;

    void applyCollapse(int a, int b, const Eigen::Vector3d& cand, double newRadius) {
        // Survivor is whichever vertex id we keep; since candidate can now be
        // the midpoint (a brand new location), we always keep `a`'s id and
        // retire `b`, but write `cand` as a's new position. This keeps vertex
        // bookkeeping simple while still supporting non-endpoint placement.
        int survivor = a, other = b;

        Quadric merged = quad[a]; merged += quad[b];
        quad[survivor] = merged;
        pos[survivor] = cand;
        clusterRadius[survivor] = newRadius;   // NEW: propagate the proven bound
        vVersion[survivor]++;

        vector<int> toKill;
        unordered_set<int> touched = vFaces[a];
        for (int fi : vFaces[b]) touched.insert(fi);

        for (int fi : touched) {
            if (!fAlive[fi]) continue;
            auto& fc = face[fi];
            bool hasA = (fc[0]==a||fc[1]==a||fc[2]==a);
            bool hasB = (fc[0]==b||fc[1]==b||fc[2]==b);
            if (hasA && hasB) { toKill.push_back(fi); continue; }
            for (int k = 0; k < 3; ++k) if (fc[k] == other) fc[k] = survivor;
            vFaces[survivor].insert(fi);
            recomputeFaceGeom(fi);
        }
        for (int fi : toKill) {
            fAlive[fi] = 0;
            vFaces[a].erase(fi);
            vFaces[b].erase(fi);
        }
        vFaces[other].clear();
        vAlive[other] = 0;
        aliveVertexCount--;

        unordered_set<int> nbrs;
        neighbors(survivor, nbrs);
        for (int u : nbrs) pushEdge(min(survivor,u), max(survivor,u));
    }

    void run() {
        aliveVertexCount = nv;
        initQueue();
        int target = max(1, (int)llround(targetRatio * nv));

        auto t0 = chrono::steady_clock::now();
        const double timeBudgetSec = 17.0;

        while (!pq.empty() && aliveVertexCount > target) {
            if (pq.size() % 4096 == 0) {
                double elapsed = chrono::duration<double>(chrono::steady_clock::now()-t0).count();
                if (elapsed > timeBudgetSec) break;
            }
            QueueItem it = pq.top(); pq.pop();
            if (!vAlive[it.a] || !vAlive[it.b]) continue;
            if (it.va != vVersion[it.a] || it.vb != vVersion[it.b]) continue;

            Eigen::Vector3d cand; double cost, radius;
            if (!bestCollapse(it.a, it.b, cand, cost, radius)) continue;

            applyCollapse(it.a, it.b, cand, radius);
        }
    }

    void exportMesh(MeshV& Vo, MeshF& Fo) {
        vector<int> remap(nv, -1);
        int cnt = 0;
        for (int i = 0; i < nv; ++i) if (vAlive[i]) remap[i] = cnt++;
        Vo.resize(cnt, 3);
        for (int i = 0; i < nv; ++i)
            if (vAlive[i]) Vo.row(remap[i]) = pos[i].transpose();

        vector<array<int,3>> outFaces;
        outFaces.reserve(nf);
        for (int fi = 0; fi < nf; ++fi) {
            if (!fAlive[fi]) continue;
            auto& fc = face[fi];
            if (fc[0]==fc[1] || fc[1]==fc[2] || fc[0]==fc[2]) continue;
            if (fArea[fi] < minTriArea) continue;
            outFaces.push_back({remap[fc[0]], remap[fc[1]], remap[fc[2]]});
        }
        Fo.resize((int)outFaces.size(), 3);
        for (size_t i = 0; i < outFaces.size(); ++i)
            for (int k = 0; k < 3; ++k) Fo((int)i, k) = outFaces[i][k];
    }
};

static void simplify(MeshV& Vout, MeshF& Fout) {
    Solver s;
    s.targetRatio = 0.5;
    s.build(V, F);
    s.run();
    s.exportMesh(Vout, Fout);
}

int main() {
    load_obj();
    MeshV Vout; MeshF Fout;
    simplify(Vout, Fout);
    save_obj(Vout, Fout);
    return 0;
}
