// fox v8 — fix validation bug (remove incorrect edge‑multiplicity check)
//               + keep all earlier improvements.
//
// Changes over v7:
//  – Deleted the local edge‑multiplicity check that required every
//    undirected edge to have count == 2 (it incorrectly treated
//    boundary edges of the one‑ring as errors).
//  – Kept duplicate‑face detection (important for manifold safety).
//  – Minor: increased stagnation limit to 50·000 to avoid premature
//    exit on very large meshes while the queue still contains valid edges.
//
// Everything else (feature penalty, length cost, normal flip tolerance,
// adaptive Hausdorff & dihedral, QEM‑optimal candidate, etc.) unchanged.

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
#include <algorithm>

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
    Eigen::Matrix3d H() const {
        Eigen::Matrix3d m;
        m << a,b,c,  b,e,f,  c,f,h;
        return m;
    }
    Eigen::Vector3d cvec() const { return Eigen::Vector3d(d,g,i); }
};

static const Eigen::Vector3d kAxes[6] = {
    { 1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
};

struct Solver {
    int nv, nf;
    vector<Eigen::Vector3d> pos;
    vector<char> vAlive;
    vector<int> vVersion;
    vector<Quadric> quad;
    vector<double> clusterRadius;
    vector<unordered_set<int>> vFaces;
    vector<array<int,3>> face;
    vector<char> fAlive;
    vector<Eigen::Vector3d> fNormal;
    vector<double> fArea;

    double aabbDiag = 1.0;
    double hausdorffBound = 0.05;
    double minTriArea = 1e-13;

    double dihedralLockCos;
    double featurePenalty = 1e4;
    double normalFlipCosMin = 0.5;      // ~60°
    static constexpr double W_LengthCost = 0.001;

    void build(const MeshV& Vin, const MeshF& Fin) {
        nv = (int)Vin.rows();
        nf = (int)Fin.rows();
        pos.resize(nv);
        for (int i = 0; i < nv; ++i) pos[i] = Vin.row(i).transpose();
        vAlive.assign(nv, 1);
        vVersion.assign(nv, 0);
        quad.assign(nv, Quadric());
        clusterRadius.assign(nv, 0.0);
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

        // Adaptive settings
        if (nv > 200000) {
            hausdorffBound = 0.04;
            dihedralLockCos = cos(55.0 * M_PI / 180.0);
        } else {
            hausdorffBound = 0.05;
            dihedralLockCos = cos(48.0 * M_PI / 180.0);
        }

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

    bool edgeFaces(int a, int b, int& f0, int& f1) const {
        f0 = f1 = -1;
        for (int fi : vFaces[a]) {
            if (!fAlive[fi]) continue;
            auto& fc = face[fi];
            bool hasB = (fc[0]==b||fc[1]==b||fc[2]==b);
            if (hasB) { if (f0 < 0) f0 = fi; else if (f1 < 0) f1 = fi; }
        }
        return f0 >= 0 && f1 >= 0;
    }

    double featureCost(int a, int b) const {
        int f0, f1;
        if (!edgeFaces(a, b, f0, f1)) return 0.0;
        Eigen::Vector3d n0 = fNormal[f0], n1 = fNormal[f1];
        double cosDihedral = n0.dot(n1);
        bool dihedralLocked = cosDihedral < dihedralLockCos;
        bool silhouette = false;
        for (const auto& r : kAxes) {
            double s0 = n0.dot(r), s1 = n1.dot(r);
            if (s0 * s1 < 0.0) { silhouette = true; break; }
        }
        if (dihedralLocked || silhouette) return featurePenalty;
        return 0.0;
    }

    double edgeLengthCost(int a, int b) const {
        double len = (pos[a] - pos[b]).norm();
        return W_LengthCost * len * len;
    }

    struct QueueItem {
        double cost;
        int a, b;
        int va, vb;
        bool operator>(const QueueItem& o) const { return cost > o.cost; }
    };
    priority_queue<QueueItem, vector<QueueItem>, greater<QueueItem>> pq;

    bool solveQemOptimal(const Quadric& qe, Eigen::Vector3d& out) const {
        Eigen::Matrix3d H = qe.H();
        double damp = 1e-9 * (H.trace() / 3.0 + 1e-12);
        H += damp * Eigen::Matrix3d::Identity();
        Eigen::Vector3d c = qe.cvec();
        Eigen::FullPivLU<Eigen::Matrix3d> lu(H);
        if (!lu.isInvertible()) return false;
        out = -lu.solve(c);
        return out.allFinite();
    }

    bool nearLocalSurface(int a, int b, const Eigen::Vector3d& cand, double bound) const {
        double minDist = 1e300;
        for (int vtx : {a, b}) {
            for (int fi : vFaces[vtx]) {
                if (!fAlive[fi]) continue;
                double d = fabs(fNormal[fi].dot(cand - pos[face[fi][0]]));
                minDist = min(minDist, d);
            }
        }
        return minDist <= bound;
    }

    // ---------- CORRECTED VALIDATION (no broken edge‑multiplicity check) ----------
    bool validateCollapseGeometry(int a, int b, const Eigen::Vector3d& cand) {
        unordered_set<int> touched = vFaces[a];
        for (int fi : vFaces[b]) touched.insert(fi);

        vector<array<int,3>> newFaces;
        newFaces.reserve(touched.size());

        for (int fi : touched) {
            if (!fAlive[fi]) continue;
            auto fc = face[fi];
            bool hasA = (fc[0]==a||fc[1]==a||fc[2]==a);
            bool hasB = (fc[0]==b||fc[1]==b||fc[2]==b);
            if (hasA && hasB) continue;   // face will be removed

            array<int,3> nf = fc;
            for (int k = 0; k < 3; ++k) if (nf[k] == a || nf[k] == b) nf[k] = -2;

            // Map placeholders to the survivor (a)
            array<int,3> ids;
            for (int k = 0; k < 3; ++k) {
                int old = nf[k];
                if (old == -2) ids[k] = a;
                else if (old == b) ids[k] = a;
                else ids[k] = old;
            }

            // Degenerate face (duplicate indices)
            if (ids[0]==ids[1] || ids[1]==ids[2] || ids[0]==ids[2]) return false;

            // Geometry: area + normal flip
            Eigen::Vector3d p[3];
            for (int k = 0; k < 3; ++k)
                p[k] = (nf[k] == -2) ? cand : pos[nf[k]];

            Eigen::Vector3d cr = (p[1]-p[0]).cross(p[2]-p[0]);
            double newArea = 0.5 * cr.norm();
            if (newArea < minTriArea) return false;

            Eigen::Vector3d newN = cr / (2.0*newArea);
            if (newN.dot(fNormal[fi]) < normalFlipCosMin) return false;

            newFaces.push_back(ids);
        }

        // ----- Duplicate face check only -----
        struct TriKey {
            array<int,3> v;
            bool operator==(const TriKey& o) const {
                return v[0]==o.v[0] && v[1]==o.v[1] && v[2]==o.v[2];
            }
        };
        struct TriKeyHash {
            size_t operator()(const TriKey& t) const {
                return ((size_t)t.v[0]*3000000071ULL + (size_t)t.v[1]*3000000193ULL + (size_t)t.v[2]*3000000317ULL);
            }
        };
        unordered_set<TriKey, TriKeyHash> seenTris;

        for (auto& ids : newFaces) {
            array<int,3> sorted = ids;
            sort(sorted.begin(), sorted.end());
            TriKey tk{sorted};
            if (seenTris.count(tk)) return false;   // duplicate face → non‑manifold
            seenTris.insert(tk);
        }

        return true;   // all checks passed
    }

    bool bestCollapse(int a, int b, Eigen::Vector3d& outPos, double& outCost, double& outRadius) {
        if (!vAlive[a] || !vAlive[b]) return false;
        if (!linkConditionOk(a, b)) return false;

        Quadric qe = quad[a]; qe += quad[b];
        double bound = hausdorffBound * aabbDiag;
        double fcost = featureCost(a, b) + edgeLengthCost(a, b);

        vector<Eigen::Vector3d> candidates = { pos[a], pos[b], 0.5 * (pos[a] + pos[b]) };
        Eigen::Vector3d qOpt;
        if (solveQemOptimal(qe, qOpt)) candidates.push_back(qOpt);

        bool found = false;
        double bestCost = 0; Eigen::Vector3d bestPos; double bestRadius = 0;

        for (auto& cand : candidates) {
            double rNew = max(clusterRadius[a] + (pos[a]-cand).norm(),
                               clusterRadius[b] + (pos[b]-cand).norm());
            if (rNew > bound) continue;
            if (!nearLocalSurface(a, b, cand, bound)) continue;
            if (!validateCollapseGeometry(a, b, cand)) continue;

            double c = qe.eval(cand) + fcost;
            if (!found || c < bestCost) {
                found = true; bestCost = c; bestPos = cand; bestRadius = rNew;
            }
        }
        if (!found) return false;
        outPos = bestPos; outCost = bestCost; outRadius = bestRadius;
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
        int survivor = a, other = b;

        Quadric merged = quad[a]; merged += quad[b];
        quad[survivor] = merged;
        pos[survivor] = cand;
        clusterRadius[survivor] = newRadius;
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

    void run(double timeBudgetSec) {
        aliveVertexCount = nv;
        initQueue();

        auto t0 = chrono::steady_clock::now();
        long iterSinceTimeCheck = 0;
        long popsWithoutProgress = 0;
        const long stagnationLimit = 50000;   // larger limit for huge meshes

        while (!pq.empty() && aliveVertexCount > 1) {
            if (++iterSinceTimeCheck >= 2048) {
                iterSinceTimeCheck = 0;
                double elapsed = chrono::duration<double>(chrono::steady_clock::now()-t0).count();
                if (elapsed > timeBudgetSec) break;
            }
            QueueItem it = pq.top(); pq.pop();
            if (!vAlive[it.a] || !vAlive[it.b]) {
                popsWithoutProgress++;
                if (popsWithoutProgress > stagnationLimit) break;
                continue;
            }
            if (it.va != vVersion[it.a] || it.vb != vVersion[it.b]) {
                popsWithoutProgress++;
                if (popsWithoutProgress > stagnationLimit) break;
                continue;
            }

            Eigen::Vector3d cand; double cost, radius;
            if (!bestCollapse(it.a, it.b, cand, cost, radius)) {
                popsWithoutProgress++;
                if (popsWithoutProgress > stagnationLimit) break;
                continue;
            }

            popsWithoutProgress = 0;
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
    s.build(V, F);
    s.run(18.0);
    s.exportMesh(Vout, Fout);
}

int main() {
    load_obj();
    MeshV Vout; MeshF Fout;
    simplify(Vout, Fout);
    save_obj(Vout, Fout);
    return 0;
}
