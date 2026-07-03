// ============================================================================
// Improved Solution for IMC 2026
// Coplanar pre‑pass + camera‑aware QEM + geometric star‑delete
// Compile:   g++ -O2 -I /path/to/Eigen solution.cpp -o solution
// Run:       ./solution < input.obj > output.obj
// ============================================================================

#include "Eigen/Dense"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <cmath>
#include <algorithm>
#include <limits>
#include <chrono>
#include <array>

using namespace std;

// ---------------------------------------------------------------------------
// Hyperparameters (tune offline)
// ---------------------------------------------------------------------------
constexpr double kHausdorffDiagFraction = 0.055;
constexpr double kQemCostCapCoeff        = 0.0375;
constexpr double kCoplanarNormalEps      = 1e-5;
constexpr double kCoplanarOffsetEps      = 1e-5;
constexpr double kMaxCollapseLengthFrac  = 0.018;
constexpr double kMinTriangleArea        = 1e-14;
constexpr double kViewWeightK            = 3.0;
constexpr double kMaxFaceWeight          = 3.0;

constexpr double kKeepRatio_UpTo5k    = 0.1;
constexpr double kKeepRatio_UpTo25k   = 0.4;
constexpr double kKeepRatio_UpTo45k   = 0.3;
constexpr double kKeepRatio_UpTo50k   = 0.2;
constexpr double kKeepRatio_UpTo400k  = 0.1;
constexpr double kKeepRatio_Huge      = 0.1;

struct StarDeleteParams {
    int maxValence;
    double maxOldDev, maxNewDev, distFrac, extraFrac;
    int hardCap, scanVertices, rounds;
    double timeFrac, maxSeconds;
};
constexpr StarDeleteParams kStarParams[7] = {
    {0,0,0,0,0,0,0,0,0,0},
    {12,0.170,0.235,1.22,0.0450,33000,950000,8,0.94,6.90},
    {5,0.004,0.006,0.40,0.0015,900,105000,1,0.25,0.90},
    {6,0.008,0.012,0.52,0.0030,1800,160000,2,0.36,1.40},
    {5,0.0088,0.0130,0.56,0.0036,1950,160000,1,0.38,1.55},
    {5,0.006,0.009,0.46,0.0022,1300,130000,1,0.31,1.10},
    {7,0.012,0.018,0.64,0.0050,3200,240000,3,0.48,2.05}
};

// ============================================================================
// Geometry helpers
// ============================================================================
struct Vec3 {
    double x=0, y=0, z=0;
    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(double s) const { return {x*s, y*s, z*s}; }
    Vec3 operator/(double s) const { return {x/s, y/s, z/s}; }
};
static double dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
static double norm2(const Vec3& v) { return dot(v, v); }
static double norm(const Vec3& v) { return sqrt(norm2(v)); }
static bool finiteVec(const Vec3& v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

struct Quadric {
    double a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;
    Quadric& operator+=(const Quadric& o) {
        a+=o.a; b+=o.b; c+=o.c; d+=o.d; e+=o.e; f+=o.f; g+=o.g; h+=o.h; i+=o.i; j+=o.j;
        return *this;
    }
    void scale(double s) { a*=s; b*=s; c*=s; d*=s; e*=s; f*=s; g*=s; h*=s; i*=s; j*=s; }
    static Quadric fromTriangle(const Vec3& p0, const Vec3& p1, const Vec3& p2) {
        Vec3 n = cross(p1-p0, p2-p0);
        double ta = norm(n);
        if (ta < 1e-30) return Quadric();
        n = n / ta;
        double pd = -dot(n, p0);
        Quadric q;
        q.a=n.x*n.x; q.b=n.x*n.y; q.c=n.x*n.z; q.d=n.x*pd;
        q.e=n.y*n.y; q.f=n.y*n.z; q.g=n.y*pd;
        q.h=n.z*n.z; q.i=n.z*pd; q.j=pd*pd;
        q.scale(sqrt(0.5*ta));
        return q;
    }
    double evaluate(const Vec3& p) const {
        return a*p.x*p.x + 2*b*p.x*p.y + 2*c*p.x*p.z + 2*d*p.x
             + e*p.y*p.y + 2*f*p.y*p.z + 2*g*p.y
             + h*p.z*p.z + 2*i*p.z + j;
    }
};

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
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    void clear() { data.clear(); }
    void reserve(int n) { data.reserve(n); }
};

// ============================================================================
// Scaffold I/O (unchanged)
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
    long nv = strtol(p, &p, 10); long nf = strtol(p, &p, 10);
    V.resize(nv, 3); F.resize(nf, 3);
    for (long i = 0; i < nv; ++i) {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p; ++p;
        V(i,0) = strtod(p, &p); V(i,1) = strtod(p, &p); V(i,2) = strtod(p, &p);
    }
    for (long i = 0; i < nf; ++i) {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p; ++p;
        F(i,0) = (int)strtol(p, &p, 10) - 1;
        F(i,1) = (int)strtol(p, &p, 10) - 1;
        F(i,2) = (int)strtol(p, &p, 10) - 1;
    }
}
static void save_obj() {
    string out;
    out.reserve((size_t)V.rows() * 40 + (size_t)F.rows() * 24 + 32);
    char line[96];
    out.append(line, snprintf(line, sizeof line, "%ld %ld\n", (long)V.rows(), (long)F.rows()));
    for (Eigen::Index i = 0; i < V.rows(); ++i)
        out.append(line, snprintf(line, sizeof line, "v %.10g %.10g %.10g\n", V(i,0), V(i,1), V(i,2)));
    for (Eigen::Index i = 0; i < F.rows(); ++i)
        out.append(line, snprintf(line, sizeof line, "f %d %d %d\n", F(i,0)+1, F(i,1)+1, F(i,2)+1));
    fwrite(out.data(), 1, out.size(), stdout);
}

// ============================================================================
// Main simplification class
// ============================================================================
class Simplifier {
public:
    Simplifier(int nv, int nf, const Vec3* v, const array<int,3>* f)
        : nV(nv), nF(nf)
    {
        verts.assign(v, v+nv);
        faces.assign(f, f+nf);
    }

    void run() {
        if (nV <= 4) return;
        startTime = chrono::steady_clock::now();
        initScale();
        buildConnectivity();
        initFaceWeights();
        coplanarPass();
        initQueue();
        collapseLoop();
        if (nV >= 100 && elapsed() < 17.5) {
            generalStarDeletePostPass();
        }
        compact();
    }

    void getResult(vector<Vec3>& outVerts, vector<array<int,3>>& outFaces) const {
        outVerts = verts;
        outFaces = faces;
    }

private:
    int nV, nF;
    vector<Vec3> verts;
    vector<array<int,3>> faces;
    vector<char> vAlive, fAlive;
    vector<int> vVersion;
    vector<Quadric> vQuadric;
    vector<double> vRadius;
    vector<vector<int>> vFaces;
    vector<SmallSet> vNeigh;

    struct CollapseCandidate {
        int absorbed=-1, kept=-1, verAbsorbed=-1, verKept=-1;
        double cost=1e100, mergedRadius=0;
        Vec3 pos;
        bool valid() const { return absorbed>=0 && kept>=0 && cost<1e99; }
        bool operator<(const CollapseCandidate& o) const { return cost > o.cost; }
    };
    struct StarCandidate { int v=-1, root=0; double score=1e100; bool valid() const { return v>=0 && score<1e99; } bool operator<(const StarCandidate& o) const { return score<o.score; } };
    struct DoubleStarCandidate { int a=-1, b=-1, root=0; double score=1e100; vector<int> boundary, patchFaces; bool valid() const { return a>=0 && b>=0 && score<1e99; } bool operator<(const DoubleStarCandidate& o) const { return score<o.score; } };

    priority_queue<CollapseCandidate> pq;
    int targetV=0, collapseLimit=0, accepted=0;
    double diag=0, hausd=0, costCap=0, invDiag2=0;
    chrono::steady_clock::time_point startTime;
    double stageTimeBudget = 24.0;

    double elapsed() const { return chrono::duration<double>(chrono::steady_clock::now()-startTime).count(); }

    int tier() const {
        if (nV <= 6000) return 1;
        if (nV <= 30000) return 2;
        if (nV <= 45000) return 3;
        if (nV <= 60000) return 4;
        if (nV <= 450000) return 5;
        return 6;
    }
    StarDeleteParams starParams() const { int t=tier(); if(t<1)t=1; if(t>6)t=6; return kStarParams[t]; }

    double faceWeightFor(const Vec3& n, double area) const {
        double w = 1.0 + kViewWeightK * area * invDiag2 * (fabs(n.x)+fabs(n.y)+fabs(n.z));
        return min(w, kMaxFaceWeight);
    }

    Vec3 faceNormal(int fi) const {
        const auto& f = faces[fi];
        return cross(verts[f[1]]-verts[f[0]], verts[f[2]]-verts[f[0]]);
    }

    bool edgeExists(int a, int b) const {
        return a>=0 && b>=0 && a<nV && b<nV && vAlive[a] && vAlive[b] && vNeigh[a].contains(b);
    }
    int countCommonFaces(int a, int b) const {
        int cnt=0;
        for (int fi : vFaces[a]) if (fAlive[fi]) {
            const auto& f = faces[fi];
            if (f[0]==b || f[1]==b || f[2]==b) ++cnt;
        }
        return cnt;
    }
    int countCommonNeighbors(int a, int b) const {
        int cnt=0;
        const auto& na = vNeigh[a];
        const auto& nb = vNeigh[b];
        for (int x : na) if (x!=a && x!=b && vAlive[x] && nb.contains(x)) ++cnt;
        return cnt;
    }
    bool faceHasVertex(int fi, int v) const {
        const auto& f = faces[fi];
        return f[0]==v || f[1]==v || f[2]==v;
    }
    bool activeFaceWithSameKey(int a, int b, int c, const vector<int>& skip) const {
        array<int,3> key = {a,b,c}; sort(key.begin(), key.end());
        for (int fi : vFaces[a]) {
            if (!fAlive[fi]) continue;
            if (find(skip.begin(), skip.end(), fi) != skip.end()) continue;
            const auto& f = faces[fi];
            array<int,3> k = {f[0],f[1],f[2]}; sort(k.begin(), k.end());
            if (k == key) return true;
        }
        return false;
    }
    double pointTriangleDistance2(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) const {
        Vec3 ab=b-a, ac=c-a, ap=p-a;
        double d1=dot(ab,ap), d2=dot(ac,ap);
        if (d1<=0 && d2<=0) return norm2(ap);
        Vec3 bp=p-b; double d3=dot(ab,bp), d4=dot(ac,bp);
        if (d3>=0 && d4<=d3) return norm2(bp);
        double vc=d1*d4-d3*d2;
        if (vc<=0 && d1>=0 && d3<=0) { double v=d1/(d1-d3); return norm2(p-(a+ab*v)); }
        Vec3 cp=p-c; double d5=dot(ab,cp), d6=dot(ac,cp);
        if (d6>=0 && d5<=d6) return norm2(cp);
        double vb=d5*d2-d1*d6;
        if (vb<=0 && d2>=0 && d6<=0) { double w=d2/(d2-d6); return norm2(p-(a+ac*w)); }
        double va=d3*d6-d5*d4;
        if (va<=0 && (d4-d3)>=0 && (d5-d6)>=0) { double w=(d4-d3)/((d4-d3)+(d5-d6)); return norm2(p-(b+(c-b)*w)); }
        Vec3 n=cross(ab,ac); double nn=norm2(n);
        if (nn<1e-24) return 1e100;
        double dist=dot(p-a,n); return (dist*dist)/nn;
    }

    bool orientedRing(int v, const StarDeleteParams& sp, vector<int>& ring, vector<int>& inc) const {
        ring.clear(); inc.clear();
        if (v<0 || v>=nV || !vAlive[v]) return false;
        for (int fi : vFaces[v]) if (fAlive[fi] && faceHasVertex(fi,v)) inc.push_back(fi);
        int m = (int)inc.size();
        if (m<3 || m>sp.maxValence) return false;
        vector<pair<int,int>> dir; dir.reserve(m);
        for (int fi : inc) {
            const auto& f = faces[fi];
            int pos = -1;
            for (int k=0;k<3;++k) if (f[k]==v) pos=k;
            if (pos<0) return false;
            int a=f[(pos+1)%3], b=f[(pos+2)%3];
            if (a==b || a==v || b==v || !vAlive[a] || !vAlive[b]) return false;
            dir.push_back({a,b});
        }
        for (int i=0;i<m;++i) for (int j=i+1;j<m;++j) {
            if (dir[i].first==dir[j].first) return false;
            if (dir[i].second==dir[j].second) return false;
        }
        int start=dir[0].first, cur=start;
        ring.push_back(start);
        for (int step=0;step<m;++step) {
            int nxt=-1;
            for (auto& e:dir) if (e.first==cur) { nxt=e.second; break; }
            if (nxt<0) return false;
            if (step==m-1) { if (nxt!=start) return false; }
            else { for (int x:ring) if (x==nxt) return false; ring.push_back(nxt); cur=nxt; }
        }
        return (int)ring.size()==m;
    }

    bool evaluateStarRoot(int v, const vector<int>& ring, const vector<int>& inc, int root, double oldDev, const Vec3& avgN, const StarDeleteParams& sp, StarCandidate& out) const {
        int m=(int)ring.size();
        vector<int> rr; rr.reserve(m);
        for (int i=0;i<m;++i) rr.push_back(ring[(root+i)%m]);
        int r0=rr[0];
        for (int i=2;i<=m-2;++i) if (vNeigh[r0].contains(rr[i])) return false;
        double maxNewDev=0, minDist2=1e100;
        for (int i=1;i<m-1;++i) {
            int a=rr[0], b=rr[i], c=rr[i+1];
            if (a==b || b==c || a==c) return false;
            if (activeFaceWithSameKey(a,b,c,inc)) return false;
            Vec3 n=cross(verts[b]-verts[a], verts[c]-verts[a]);
            double nl=norm(n); if (nl<1e-12) return false;
            Vec3 un=n/nl;
            double d=clampDouble(dot(un,avgN),-1,1);
            if (d<=0) return false;
            maxNewDev=max(maxNewDev, 1-d);
            if (maxNewDev>sp.maxNewDev) return false;
            minDist2=min(minDist2, pointTriangleDistance2(verts[v], verts[a], verts[b], verts[c]));
        }
        double dist=sqrt(max(0.0,minDist2));
        if (vRadius[v]+dist > hausd*sp.distFrac) return false;
        out.v=v; out.root=root;
        out.score = (vRadius[v]+dist)/(hausd+1e-12) + 0.35*oldDev + 0.25*maxNewDev + 1e-4*m;
        return true;
    }

    StarCandidate computeStarCandidate(int v, const StarDeleteParams& sp) const {
        StarCandidate best;
        vector<int> ring, inc;
        if (!orientedRing(v, sp, ring, inc)) return best;
        Vec3 avg; double areaSum=0, oldDev=0;
        for (int fi : inc) {
            Vec3 n=faceNormal(fi); double nl=norm(n); if (nl<1e-12) return best;
            avg=avg+n; areaSum+=0.5*nl;
        }
        double al=norm(avg); if (al<1e-12 || areaSum<=0) return best;
        avg=avg/al;
        for (int fi : inc) {
            Vec3 n=faceNormal(fi); Vec3 un=n/norm(n);
            double d=clampDouble(dot(un,avg),-1,1); if (d<=0) return best;
            oldDev=max(oldDev, 1-d);
        }
        if (oldDev>sp.maxOldDev) return best;
        for (int root=0; root<(int)ring.size(); ++root) {
            StarCandidate c;
            if (evaluateStarRoot(v, ring, inc, root, oldDev, avg, sp, c) && c.score<best.score) best=c;
        }
        return best;
    }

    bool applyStarDelete(int v, int root, const StarDeleteParams& sp) {
        StarCandidate best = computeStarCandidate(v, sp);
        if (!best.valid()) return false;
        root = best.root;
        vector<int> ring, inc;
        if (!orientedRing(v, sp, ring, inc)) return false;
        int m=(int)ring.size();
        vector<int> rr; rr.reserve(m);
        for (int i=0;i<m;++i) rr.push_back(ring[(root+i)%m]);
        // remove old faces
        for (int fi : inc) {
            if (!fAlive[fi]) continue;
            auto& f = faces[fi];
            fAlive[fi]=0;
            for (int k=0;k<3;++k) {
                int u=f[k];
                if (u>=0 && u<nV && vAlive[u]) {
                    auto& vec = vFaces[u];
                    vec.erase(remove(vec.begin(), vec.end(), fi), vec.end());
                }
            }
        }
        // remove vertex v from neighbours
        for (int nb : ring) if (nb>=0 && nb<nV && vAlive[nb]) vNeigh[nb].erase(v);
        vNeigh[v].clear(); vFaces[v].clear(); vAlive[v]=0; vRadius[v]=0; ++vVersion[v];
        // add new faces
        for (int i=1; i<m-1; ++i) {
            int a=rr[0], b=rr[i], c=rr[i+1];
            array<int,3> nf = {a,b,c};
            int fi = (int)faces.size();
            faces.push_back(nf);
            fAlive.push_back(1);
            for (int k=0;k<3;++k) vFaces[nf[k]].push_back(fi);
            for (int k=0;k<3;++k) {
                int x=nf[k], y=nf[(k+1)%3];
                if (x!=y) { vNeigh[x].insert(y); vNeigh[y].insert(x); }
            }
        }
        return true;
    }

    // Double star‑delete (fully implemented)
    bool combinedPatchForEdge(int a, int b, vector<int>& boundary, vector<int>& patchFaces) const {
        boundary.clear(); patchFaces.clear();
        if (!edgeExists(a,b)) return false;
        auto add = [&](int x) { for (int v:patchFaces) if(v==x) return false; patchFaces.push_back(x); return true; };
        for (int fi:vFaces[a]) if (fAlive[fi] && (faceHasVertex(fi,a)||faceHasVertex(fi,b))) add(fi);
        for (int fi:vFaces[b]) if (fAlive[fi] && (faceHasVertex(fi,a)||faceHasVertex(fi,b))) add(fi);
        if (patchFaces.size()<4 || patchFaces.size()>14) return false;
        vector<pair<int,int>> edges;
        for (int fi : patchFaces) {
            const auto& f = faces[fi];
            edges.push_back({f[0], f[1]});
            edges.push_back({f[1], f[2]});
            edges.push_back({f[2], f[0]});
        }
        vector<pair<int,int>> bedges;
        for (auto& e : edges) {
            bool internal = false;
            for (auto& r : edges) if (e.first==r.second && e.second==r.first) { internal=true; break; }
            if (!internal) bedges.push_back(e);
        }
        int m = (int)bedges.size();
        if (m<4 || m>10) return false;
        for (auto& e : bedges) if (e.first==a || e.first==b || e.second==a || e.second==b) return false;
        int start = bedges[0].first, cur = start;
        boundary.push_back(cur);
        for (int step=0; step<m; ++step) {
            int nxt = -1;
            for (auto& e : bedges) if (e.first==cur) { nxt = e.second; break; }
            if (nxt < 0) return false;
            if (step == m-1) { if (nxt != start) return false; }
            else { for (int x : boundary) if (x == nxt) return false; boundary.push_back(nxt); cur = nxt; }
        }
        return (int)boundary.size() == m;
    }

    bool evaluateDoubleRoot(int a, int b, const vector<int>& boundary, const vector<int>& patchFaces, int root, const Vec3& avgN, double oldDev, DoubleStarCandidate& out) const {
        StarDeleteParams sp = starParams();
        int m = (int)boundary.size();
        vector<int> rr; rr.reserve(m);
        for (int i=0; i<m; ++i) rr.push_back(boundary[(root+i)%m]);
        int r0 = rr[0];
        for (int i=2; i<=m-2; ++i) if (vNeigh[r0].contains(rr[i])) return false;
        double maxNewDev = 0, minDa = 1e100, minDb = 1e100;
        for (int i=1; i<m-1; ++i) {
            int x = rr[0], y = rr[i], z = rr[i+1];
            if (x==y || y==z || x==z) return false;
            if (activeFaceWithSameKey(x,y,z,patchFaces)) return false;
            Vec3 n = cross(verts[y]-verts[x], verts[z]-verts[x]);
            double nl = norm(n); if (nl<1e-12) return false;
            Vec3 un = n/nl;
            double d = clampDouble(dot(un,avgN), -1, 1);
            if (d <= 0) return false;
            maxNewDev = max(maxNewDev, 1-d);
            if (maxNewDev > sp.maxNewDev*1.12) return false;
            minDa = min(minDa, pointTriangleDistance2(verts[a], verts[x], verts[y], verts[z]));
            minDb = min(minDb, pointTriangleDistance2(verts[b], verts[x], verts[y], verts[z]));
        }
        double da = sqrt(max(0.0, minDa)), db = sqrt(max(0.0, minDb));
        if (vRadius[a]+da > hausd*sp.distFrac) return false;
        if (vRadius[b]+db > hausd*sp.distFrac) return false;
        out.a = a; out.b = b; out.root = root; out.boundary = boundary; out.patchFaces = patchFaces;
        double nd = max(vRadius[a]+da, vRadius[b]+db) / (hausd+1e-12);
        out.score = nd + 0.35*oldDev + 0.25*maxNewDev + 2e-4*m;
        return true;
    }

    DoubleStarCandidate computeDoubleStarCandidate(int a, int b) const {
        DoubleStarCandidate best;
        vector<int> boundary, patchFaces;
        if (!combinedPatchForEdge(a,b,boundary,patchFaces)) return best;
        Vec3 avg; vector<Vec3> ns;
        for (int fi : patchFaces) {
            Vec3 n = faceNormal(fi);
            double nl = norm(n); if (nl < 1e-12) return best;
            ns.push_back(n/nl); avg = avg + ns.back();
        }
        double al = norm(avg); if (al < 1e-12) return best;
        avg = avg / al;
        double oldDev = 0;
        for (const Vec3& n : ns) {
            double d = clampDouble(dot(n,avg), -1,1);
            if (d <= 0) return best;
            oldDev = max(oldDev, 1-d);
        }
        StarDeleteParams sp = starParams();
        if (oldDev > sp.maxOldDev*1.35) return best;
        for (int root=0; root<(int)boundary.size(); ++root) {
            DoubleStarCandidate c;
            if (evaluateDoubleRoot(a,b,boundary,patchFaces,root,avg,oldDev,c) && c.score<best.score) best=c;
        }
        return best;
    }

    bool applyDoubleStarDelete(int a, int b) {
        DoubleStarCandidate best = computeDoubleStarCandidate(a, b);
        if (!best.valid()) return false;
        const vector<int>& boundary = best.boundary;
        const vector<int>& patchFaces = best.patchFaces;
        int m = (int)boundary.size();
        vector<int> rr; rr.reserve(m);
        for (int i=0;i<m;++i) rr.push_back(boundary[(best.root+i)%m]);
        for (int fi : patchFaces) {
            if (!fAlive[fi]) continue;
            auto& f = faces[fi];
            fAlive[fi] = 0;
            for (int k=0;k<3;++k) {
                int u = f[k];
                if (u>=0 && u<nV && vAlive[u]) {
                    auto& vec = vFaces[u];
                    vec.erase(remove(vec.begin(), vec.end(), fi), vec.end());
                }
            }
        }
        for (int x : {a,b}) {
            for (int nb : vNeigh[x]) if (nb>=0 && nb<nV && vAlive[nb]) vNeigh[nb].erase(x);
            vNeigh[x].clear(); vFaces[x].clear(); vAlive[x]=0; vRadius[x]=0; ++vVersion[x];
        }
        for (int i=1; i<m-1; ++i) {
            int x = rr[0], y = rr[i], z = rr[i+1];
            array<int,3> nf = {x,y,z};
            int fi = (int)faces.size();
            faces.push_back(nf);
            fAlive.push_back(1);
            for (int k=0;k<3;++k) vFaces[nf[k]].push_back(fi);
            for (int k=0;k<3;++k) {
                int p = nf[k], q = nf[(k+1)%3];
                if (p != q) { vNeigh[p].insert(q); vNeigh[q].insert(p); }
            }
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // Pipeline passes
    // -----------------------------------------------------------------------
    void coplanarPass() {
        struct CoplanarItem { double cost; int a,b; int ver_a,ver_b; };
        auto cmp = [](const CoplanarItem& x, const CoplanarItem& y) { return x.cost > y.cost; };
        priority_queue<CoplanarItem, vector<CoplanarItem>, decltype(cmp)> cq(cmp);
        for (int a=0; a<nV; ++a) {
            if (!vAlive[a]) continue;
            for (int b : vNeigh[a]) {
                if (b <= a) continue;
                int fcnt = 0; Vec3 n[2]; double d[2];
                for (int fi : vFaces[a]) {
                    if (!fAlive[fi]) continue;
                    const auto& f = faces[fi];
                    if (f[0]==b || f[1]==b || f[2]==b) {
                        if (fcnt < 2) {
                            Vec3 nr = faceNormal(fi);
                            double len = norm(nr);
                            if (len < 1e-12) continue;
                            n[fcnt] = nr / len;
                            d[fcnt] = -dot(n[fcnt], verts[f[0]]);
                            ++fcnt;
                        }
                    }
                }
                if (fcnt != 2) continue;
                double nd = fabs(dot(n[0], n[1]));
                double dd = fabs(d[0] - d[1]);
                if (1.0 - nd > kCoplanarNormalEps || dd > kCoplanarOffsetEps) continue;
                double cost = norm(verts[a]-verts[b]);
                cq.push({cost, a, b, vVersion[a], vVersion[b]});
            }
        }
        double stopTime = elapsed() + 0.4;
        while (!cq.empty() && elapsed() < stopTime) {
            auto item = cq.top(); cq.pop();
            int a = item.a, b = item.b;
            if (!vAlive[a] || !vAlive[b]) continue;
            if (item.ver_a != vVersion[a] || item.ver_b != vVersion[b]) continue;
            if (!edgeExists(a,b)) continue;
            auto best = bestValidCandidate(a,b);
            if (!best.valid() || best.cost > costCap) continue;
            if (countCommonFaces(best.absorbed, best.kept) != 2) continue;
            if (countCommonNeighbors(best.absorbed, best.kept) != 2) continue;
            applyCollapse(best);
            ++accepted;
            for (int nb : vNeigh[best.kept]) {
                if (nb == best.kept || !vAlive[nb]) continue;
                int fcnt=0; Vec3 n[2]; double d[2];
                for (int fi : vFaces[best.kept]) {
                    if (!fAlive[fi]) continue;
                    const auto& f = faces[fi];
                    if (f[0]==nb || f[1]==nb || f[2]==nb) {
                        if (fcnt < 2) {
                            Vec3 nr = faceNormal(fi);
                            double len = norm(nr);
                            if (len < 1e-12) continue;
                            n[fcnt] = nr/len;
                            d[fcnt] = -dot(n[fcnt], verts[f[0]]);
                            ++fcnt;
                        }
                    }
                }
                if (fcnt==2 && 1.0-fabs(dot(n[0],n[1]))<kCoplanarNormalEps && fabs(d[0]-d[1])<kCoplanarOffsetEps) {
                    double cost = norm(verts[best.kept]-verts[nb]);
                    cq.push({cost, best.kept, nb, vVersion[best.kept], vVersion[nb]});
                }
            }
        }
    }

    void initQueue() {
        for (int a=0; a<nV; ++a) {
            if (!vAlive[a]) continue;
            for (int b : vNeigh[a]) {
                if (b <= a) continue;
                auto c = bestValidCandidate(a,b);
                if (c.valid()) pq.push(c);
            }
        }
    }

    void collapseLoop() {
        int tick = 0;
        while (accepted < collapseLimit && !pq.empty()) {
            if ((++tick & 8191) == 0 && elapsed() > 16.0) break;
            auto c = pq.top(); pq.pop();
            int a = c.absorbed, b = c.kept;
            if (!edgeExists(a,b)) continue;
            if (c.verAbsorbed != vVersion[a] || c.verKept != vVersion[b]) {
                auto fr = bestValidCandidate(a,b);
                if (fr.valid()) pq.push(fr);
                continue;
            }
            if (c.cost > costCap) break;
            if (countCommonFaces(a,b) != 2) continue;
            if (countCommonNeighbors(a,b) != 2) continue;
            auto best = bestValidCandidate(a,b);
            if (!best.valid() || best.cost > costCap) continue;
            applyCollapse(best);
            ++accepted;
            for (int nb : vNeigh[best.kept]) {
                if (nb == best.kept || !vAlive[nb]) continue;
                auto nc = bestValidCandidate(best.kept, nb);
                if (nc.valid()) pq.push(nc);
            }
        }
    }

    CollapseCandidate bestValidCandidate(int a, int b) const {
        Quadric Q = vQuadric[a]; Q += vQuadric[b];
        Vec3 cand[4]; int nc = 0;
        // QEM solve
        {
            Eigen::Matrix3d H;
            H << Q.a, Q.b, Q.c,
                 Q.b, Q.e, Q.f,
                 Q.c, Q.f, Q.h;
            Eigen::Vector3d c(-Q.d, -Q.g, -Q.i);
            Eigen::Vector3d opt = H.colPivHouseholderQr().solve(c);
            if ((H*opt - c).norm() < 1e-3) cand[nc++] = Vec3(opt(0),opt(1),opt(2));
        }
        cand[nc++] = (verts[a] + verts[b]) * 0.5;
        cand[nc++] = verts[a];
        cand[nc++] = verts[b];
        int wp = 0;
        for (int i=0; i<nc; ++i) {
            if (!finiteVec(cand[i])) continue;
            bool dup = false;
            for (int j=0; j<wp; ++j) if (norm2(cand[i]-cand[j]) < 1e-30) { dup = true; break; }
            if (!dup) cand[wp++] = cand[i];
        }
        nc = wp;
        CollapseCandidate best;
        for (int i=0; i<nc; ++i) {
            for (int dir = 0; dir < 2; ++dir) {
                int ab = dir ? b : a;
                int kp = dir ? a : b;
                double rad = max(vRadius[ab] + norm(verts[ab]-cand[i]),
                                 vRadius[kp] + norm(verts[kp]-cand[i]));
                if (rad > hausd) continue;
                if (norm(verts[ab]-cand[i]) > kMaxCollapseLengthFrac*diag ||
                    norm(verts[kp]-cand[i]) > kMaxCollapseLengthFrac*diag) continue;
                CollapseCandidate candC;
                candC.absorbed = ab; candC.kept = kp;
                candC.verAbsorbed = vVersion[ab]; candC.verKept = vVersion[kp];
                candC.pos = cand[i];
                candC.cost = Q.evaluate(cand[i]);
                candC.mergedRadius = rad;
                if (candC.cost < best.cost) best = candC;
            }
        }
        return best;
    }

    void applyCollapse(const CollapseCandidate& best) {
        int ab = best.absorbed, kp = best.kept;
        Vec3 np = best.pos;
        double nr = best.mergedRadius;
        verts[kp] = np; vRadius[kp] = nr; vRadius[ab] = 0;
        vAlive[ab] = 0; ++vVersion[ab]; ++vVersion[kp];
        vector<int> dead;
        for (int fi : vFaces[ab]) {
            if (!fAlive[fi]) continue;
            bool changed = false;
            for (int k=0;k<3;++k) if (faces[fi][k] == ab) { faces[fi][k] = kp; changed = true; }
            if (!changed) continue;
            if (faces[fi][0]==faces[fi][1] || faces[fi][1]==faces[fi][2] || faces[fi][0]==faces[fi][2]) {
                fAlive[fi] = 0; dead.push_back(fi);
            } else {
                vFaces[kp].push_back(fi);
            }
        }
        for (int fi : dead) {
            for (int k=0;k<3;++k) {
                int v = faces[fi][k];
                if (v>=0 && v<nV && vAlive[v]) {
                    auto& vec = vFaces[v];
                    vec.erase(remove(vec.begin(), vec.end(), fi), vec.end());
                }
            }
        }
        vFaces[ab].clear();
        vQuadric[kp] += vQuadric[ab];
        for (int nb : vNeigh[ab]) {
            if (nb == kp || !vAlive[nb]) continue;
            vNeigh[nb].erase(ab);
            vNeigh[nb].insert(kp);
            vNeigh[kp].insert(nb);
        }
        vNeigh[ab].clear();
        vNeigh[kp].erase(ab);
        vNeigh[kp].erase(kp);
    }

    void generalStarDeletePostPass() {
        StarDeleteParams sp = starParams();
        double timeLeft = stageTimeBudget - elapsed();
        if (timeLeft < 0.35) return;
        double stopTime = elapsed() + min(sp.maxSeconds, timeLeft*sp.timeFrac);
        int maxExtra = min(sp.hardCap, max(0, (int)(nV*sp.extraFrac)));
        if (maxExtra <= 0) return;
        int extra = 0;
        for (int round=0; round<sp.rounds && extra<maxExtra && elapsed()<stopTime; ++round) {
            vector<StarCandidate> cands; cands.reserve(4096);
            int scanned = 0;
            for (int v=0; v<nV && scanned<sp.scanVertices && elapsed()<stopTime; ++v) {
                if (!vAlive[v]) continue;
                ++scanned;
                StarCandidate c = computeStarCandidate(v, sp);
                if (c.valid()) cands.push_back(c);
            }
            if (cands.empty()) break;
            sort(cands.begin(), cands.end());
            int gained = 0;
            for (const StarCandidate& c : cands) {
                if (extra >= maxExtra || elapsed() >= stopTime) break;
                if (!vAlive[c.v]) continue;
                if (applyStarDelete(c.v, c.root, sp)) { ++accepted; ++extra; ++gained; }
            }
            if (extra < maxExtra && elapsed() < stopTime) {
                // double star‑delete
                int beforeDouble = extra;
                int maxDouble = max(2, min(maxExtra-extra, sp.hardCap/3));
                vector<DoubleStarCandidate> doubleCands;
                int scanEdges = max(1000, sp.scanVertices*2);
                for (int a=0; a<nV && (int)doubleCands.size()<maxDouble && elapsed()<stopTime; ++a) {
                    if (!vAlive[a]) continue;
                    for (int b : vNeigh[a]) {
                        if (b <= a || !vAlive[b]) continue;
                        if (scanned >= scanEdges) break;
                        ++scanned;
                        DoubleStarCandidate dc = computeDoubleStarCandidate(a,b);
                        if (dc.valid()) doubleCands.push_back(dc);
                    }
                }
                sort(doubleCands.begin(), doubleCands.end());
                for (const DoubleStarCandidate& dc : doubleCands) {
                    if (extra+2 > maxExtra || elapsed() >= stopTime) break;
                    if (!vAlive[dc.a] || !vAlive[dc.b] || !edgeExists(dc.a, dc.b)) continue;
                    if (applyDoubleStarDelete(dc.a, dc.b)) { accepted += 2; extra += 2; }
                }
                if (extra > beforeDouble) gained = 1;
            }
            if (gained == 0) break;
        }
    }

    void compact() {
        vector<int> old2new(nV, -1);
        vector<Vec3> newVerts;
        for (int i=0; i<nV; ++i) if (vAlive[i]) { old2new[i] = (int)newVerts.size(); newVerts.push_back(verts[i]); }
        vector<array<int,3>> newFaces;
        for (int fi=0; fi<(int)faces.size(); ++fi) {
            if (!fAlive[fi]) continue;
            int a = faces[fi][0], b = faces[fi][1], c = faces[fi][2];
            if (!vAlive[a] || !vAlive[b] || !vAlive[c]) continue;
            int na = old2new[a], nb = old2new[b], nc = old2new[c];
            if (na==nb || nb==nc || na==nc) continue;
            Vec3 n = cross(newVerts[nb]-newVerts[na], newVerts[nc]-newVerts[na]);
            if (norm(n) < 1e-14) continue;
            newFaces.push_back({na, nb, nc});
        }
        verts.swap(newVerts);
        faces.swap(newFaces);
        nV = (int)verts.size();
        nF = (int)faces.size();
    }

    static double clampDouble(double x, double lo, double hi) { return x<lo?lo:(x>hi?hi:x); }

    void initScale() {
        Vec3 mn=verts[0], mx=verts[0];
        for (auto& p : verts) {
            mn.x=min(mn.x,p.x); mx.x=max(mx.x,p.x);
            mn.y=min(mn.y,p.y); mx.y=max(mx.y,p.y);
            mn.z=min(mn.z,p.z); mx.z=max(mx.z,p.z);
        }
        diag = norm(mx-mn);
        hausd = kHausdorffDiagFraction * diag;
        costCap = kQemCostCapCoeff * diag * diag;
        invDiag2 = (diag>1e-12) ? 1.0/(diag*diag) : 0.0;
        double kr = kKeepRatio_UpTo400k;
        if (nV<=6000) kr=kKeepRatio_UpTo5k;
        else if (nV<=30000) kr=kKeepRatio_UpTo25k;
        else if (nV<=45000) kr=kKeepRatio_UpTo45k;
        else if (nV<=60000) kr=kKeepRatio_UpTo50k;
        else if (nV<=450000) kr=kKeepRatio_UpTo400k;
        else kr=kKeepRatio_Huge;
        targetV = max(10, (int)(nV*kr));
        targetV = min(targetV, nV-1);
        collapseLimit = nV - targetV;
    }

    void buildConnectivity() {
        vAlive.assign(nV,1); fAlive.assign(nF,1);
        vVersion.assign(nV,0);
        vQuadric.assign(nV, Quadric());
        vRadius.assign(nV,0);
        vFaces.assign(nV, {});
        vNeigh.resize(nV);
        for (int fi=0; fi<nF; ++fi) {
            const auto& f = faces[fi];
            Quadric q = Quadric::fromTriangle(verts[f[0]], verts[f[1]], verts[f[2]]);
            for (int k=0;k<3;++k) {
                vFaces[f[k]].push_back(fi);
                vQuadric[f[k]] += q;
            }
            for (int k=0;k<3;++k) {
                int a=f[k], b=f[(k+1)%3];
                if (a!=b) { vNeigh[a].insert(b); vNeigh[b].insert(a); }
            }
        }
    }

    void initFaceWeights() {
        vQuadric.assign(nV, Quadric());
        for (int fi=0; fi<nF; ++fi) {
            const auto& f = faces[fi];
            Vec3 n = faceNormal(fi);
            double area = 0.5*norm(n);
            Quadric q = Quadric::fromTriangle(verts[f[0]], verts[f[1]], verts[f[2]]);
            if (area >= 1e-30) {
                n = n/norm(n);
                double w = faceWeightFor(n, area);
                if (w != 1.0) q.scale(w);
            }
            for (int k=0;k<3;++k) vQuadric[f[k]] += q;
        }
    }
};

// ============================================================================
// simplify() – bridge between scaffold and solver
// ============================================================================
static void simplify() {
    int nV = (int)V.rows(), nF = (int)F.rows();
    vector<Vec3> verts(nV);
    vector<array<int,3>> faces(nF);
    for (int i=0;i<nV;++i) verts[i] = Vec3(V(i,0), V(i,1), V(i,2));
    for (int i=0;i<nF;++i) faces[i] = {F(i,0), F(i,1), F(i,2)};

    Simplifier s(nV, nF, verts.data(), faces.data());
    s.run();

    vector<Vec3> outVerts;
    vector<array<int,3>> outFaces;
    s.getResult(outVerts, outFaces);

    V.resize((int)outVerts.size(), 3);
    for (int i=0; i<(int)outVerts.size(); ++i) {
        V(i,0) = outVerts[i].x; V(i,1) = outVerts[i].y; V(i,2) = outVerts[i].z;
    }
    F.resize((int)outFaces.size(), 3);
    for (int i=0; i<(int)outFaces.size(); ++i) {
        F(i,0) = outFaces[i][0]; F(i,1) = outFaces[i][1]; F(i,2) = outFaces[i][2];
    }
}

int main() {
    load_obj();
    simplify();
    save_obj();
    return 0;
}
