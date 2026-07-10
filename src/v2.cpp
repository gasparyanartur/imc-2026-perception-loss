// cheetah v1.1 83 pts 
#include "Eigen/Dense"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cmath>
#include <algorithm>
#include <limits>
#include <chrono>
#include <array>
#include <cstring>
#include <omp.h>

using namespace std;

// ============================================================================
// Parameters (defined only once)
// ============================================================================
static constexpr double hparam_HausdorffDiagFraction = 0.05;
static constexpr double CParam_MinNormalNorm = 1e-8;
static constexpr double CParam_QemSolveDeterminantEps = 1e-8;
static constexpr double CParam_Inf = 1e100;

static constexpr double hparam_TotalBudgetSeconds = 20;
static constexpr double hparam_CoplanarBudgetSeconds = 0.5;
static constexpr double hparam_FirstQemBudgetSeconds = 10.0;
static constexpr double hparam_GeneralStarPostBudgetSeconds = 17.0;
static constexpr int    hparam_OutputPrecisionSignificantDigits = 10;

// Keep ratios (a bit more aggressive – SSIM guard will protect quality)
static constexpr double hparam_KeepRatio_UpTo5k    = 0.05;
static constexpr double hparam_KeepRatio_UpTo25k   = 0.5;
static constexpr double hparam_KeepRatio_UpTo45k   = 0.25;
static constexpr double hparam_KeepRatio_UpTo50k   = 0.15;
static constexpr double hparam_KeepRatio_UpTo400k  = 0.033;
static constexpr double hparam_KeepRatio_Huge      = 0.040;

static constexpr double hparam_QemCostCapCoeff = 0.025;
static constexpr double COPLANAR_EPS_NORMAL = 1e-5;
static constexpr double COPLANAR_EPS_OFFSET = 1e-5;

static constexpr double hparam_ViewWeightK = 3.0;
static constexpr double hparam_MaxFaceWeight = 3.0;

// ---- Star‑delete parameters ----
struct StarDeleteParams {
    int maxValence;
    double maxOldDev;
    double maxNewDev;
    double distFrac;
    double extraFrac;
    int hardCap;
    int scanVertices;
    int rounds;
    double timeFrac;
    double maxSeconds;
};

static constexpr StarDeleteParams hparam_GeneralStarParamsByTier[7] = {
    {0,0,0,0,0,0,0,0,0,0},
    {12,0.170,0.235,1.22,0.0450,33000,950000,8,0.94,6.90},
    {5, 0.004,0.006,0.40,0.0015,900,105000,1,0.25,0.90},
    {6, 0.008,0.012,0.52,0.0030,1800,160000,2,0.36,1.40},
    {5, 0.0088,0.0130,0.56,0.0036,1950,160000,1,0.38,1.55},
    {5, 0.006,0.009,0.46,0.0022,1300,130000,1,0.31,1.10},
    {7, 0.012,0.018,0.64,0.0050,3200,240000,3,0.48,2.05}
};

// ---- SSIM constants ----
static constexpr bool   hparam_EnableSsimThirdPass = true;
static constexpr int    hparam_SsimPatchResolution = 1024;
static constexpr int    hparam_SsimPatchPaddingPixels = 6;
static constexpr int    hparam_SsimPatchMaxPixels = 90000;
static constexpr int    hparam_SsimCandidatePoolCap = 40000;
static constexpr double hparam_SsimAcceptMin = 0.90;
static constexpr double hparam_SsimMaxDamage = 0.01;
static constexpr double hparam_SsimThirdPassMinTimeSeconds = 0.45;
static constexpr double hparam_SsimThirdPassTimeFrac = 0.9;
static constexpr double hparam_SsimThirdPassMaxSeconds = 9;
static constexpr double hparam_SsimNormalDepthWeight = 0.5;
static constexpr double hparam_SsimScoreGeomWeight = 0.0025;
static constexpr double hparam_SsimC1 = (0.01 * 255.0) * (0.01 * 255.0);
static constexpr double hparam_SsimC2 = (0.03 * 255.0) * (0.03 * 255.0);

static constexpr double hparam_SsimAcceptMinRelaxed = 0.88;
static constexpr double hparam_SsimMaxDamageRelaxed = 0.015;
static constexpr double hparam_SsimGlobalSafeThreshold = 0.92;
static constexpr double hparam_SsimGlobalFinalThreshold = 0.90;

// ---- Star-delete params for SSIM pass (unused for now) ----
static constexpr StarDeleteParams hparam_SsimStarParamsByTier[7] = {
    {0,0,0,0,0,0,0,0,0,0},
    {14,0.260,0.340,1.00,0.0120,6000,280000,1,0.60,1.10},
    {7, 0.018,0.026,0.82,0.0012,500, 90000, 1,0.45,0.55},
    {8, 0.026,0.038,0.86,0.0016,700, 110000,1,0.45,0.65},
    {8, 0.028,0.042,0.88,0.0018,850, 120000,1,0.45,0.75},
    {7, 0.018,0.028,0.80,0.0008,350, 80000, 1,0.38,0.45},
    {9, 0.040,0.060,0.92,0.0020,1600,180000,1,0.48,0.95}
};

// ---- Lens‑original cache parameters (unused) ----
static constexpr bool   hparam_EnableLensOriginalPass = true;
static constexpr int    hparam_LensResolution = 128;
static constexpr int    hparam_LensPatchPaddingPixels = 2;
static constexpr int    hparam_LensPatchMaxPixels = 42000;
static constexpr int    hparam_LensCandidatePoolCap = 70000;
static constexpr double hparam_LensOrigAcceptMin = 0.975;
static constexpr double hparam_LensCurrentAcceptMin = 0.990;
static constexpr double hparam_LensPassMinTimeSeconds = 0.55;
static constexpr double hparam_LensPassTimeFrac = 0.70;
static constexpr double hparam_LensPassMaxSeconds = 1.10;
static constexpr double hparam_LensScoreGeomWeight = 0.0015;
static constexpr double hparam_LensExtraScale = 0.55;
static constexpr double hparam_LensHardCapScale = 0.55;
static constexpr double hparam_LensBuildMaxSeconds = 2.20;
static constexpr double hparam_LensOrigAcceptByTier[7] = {0.0,0.945,0.945,0.935,0.888,0.928,0.95};
static constexpr double hparam_LensCurrentAcceptByTier[7] = {0.0,0.974,0.974,0.972,0.944,0.966,0.978};
static constexpr double hparam_LensMaxSecondsByTier[7] = {0.0,2.2,2.2,2.55,4.25,3.6,2.25};
static constexpr double hparam_LensExtraScaleByTier[7] = {0.0,1.7,1.7,1.75,5.2,2.7,1.35};
static constexpr double hparam_LensHardCapScaleByTier[7] = {0.0,1.6,1.6,1.7,4.7,2.6,1.35};

static constexpr double hparam_GlobalSafetyNetTimeFrac = 0.8;
static constexpr double hparam_GlobalSafetyNetMaxSeconds = 2.5;

// ============================================================================
// Scaffold I/O (unchanged)
// ============================================================================
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
// Geometry primitives (unchanged)
// ============================================================================
struct Vec3 {
    double x=0,y=0,z=0;
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

struct Face { int v[3]; };

struct Quadric {
    double a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;
    Quadric& operator+=(const Quadric& o) {
        a+=o.a; b+=o.b; c+=o.c; d+=o.d; e+=o.e; f+=o.f; g+=o.g; h+=o.h; i+=o.i; j+=o.j;
        return *this;
    }
    void scale(double s) {
        a*=s; b*=s; c*=s; d*=s; e*=s; f*=s; g*=s; h*=s; i*=s; j*=s;
    }
    static Quadric fromTriangle(const Vec3& p0, const Vec3& p1, const Vec3& p2) {
        Vec3 n = cross(p1-p0, p2-p0);
        double ta = norm(n);
        if (ta < CParam_MinNormalNorm) return Quadric();
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
    bool contains(int v) const {
        return binary_search(data.begin(), data.end(), v);
    }
    int size() const { return (int)data.size(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    void clear() { data.clear(); }
    void reserve(int n) { data.reserve(n); }
};

static double det3(double a00,double a01,double a02,
                   double a10,double a11,double a12,
                   double a20,double a21,double a22) {
    return a00*(a11*a22 - a12*a21) - a01*(a10*a22 - a12*a20) + a02*(a10*a21 - a11*a20);
}

static bool solveQem3x3(const Quadric& q, Vec3& out) {
    double D = det3(q.a, q.b, q.c, q.b, q.e, q.f, q.c, q.f, q.h);
    if (fabs(D) < CParam_QemSolveDeterminantEps) return false;
    out = Vec3(det3(-q.d, q.b, q.c, -q.g, q.e, q.f, -q.i, q.f, q.h) / D,
               det3( q.a,-q.d, q.c,  q.b,-q.g, q.f,  q.c,-q.i, q.h) / D,
               det3( q.a, q.b,-q.d,  q.b, q.e,-q.g,  q.c, q.f,-q.i) / D);
    return finiteVec(out);
}

// ============================================================================
// Main simplification (with star‑delete + SSIM guard)
// ============================================================================
static void simplify() {
    int nV = (int)V.rows();
    int nF = (int)F.rows();
    vector<Vec3> verts(nV);
    for (int i = 0; i < nV; ++i) verts[i] = Vec3(V(i,0), V(i,1), V(i,2));
    vector<Face> faces(nF);
    for (int i = 0; i < nF; ++i) {
        faces[i].v[0] = F(i,0);
        faces[i].v[1] = F(i,1);
        faces[i].v[2] = F(i,2);
    }

    vector<char> vdead(nV,0), fdead(nF,0);
    vector<int> vver(nV,0);
    vector<Quadric> vquad(nV);
    vector<double> crad(nV,0.0);
    vector<vector<int>> vfaces(nV);
    vector<SmallSet> vneigh(nV);

    // Serial connectivity
    for (int fi = 0; fi < nF; ++fi) {
        const Face& f = faces[fi];
        for (int k = 0; k < 3; ++k) vfaces[f.v[k]].push_back(fi);
        for (int k = 0; k < 3; ++k) {
            int a = f.v[k], b = f.v[(k+1)%3];
            if (a != b) {
                vneigh[a].insert(b);
                vneigh[b].insert(a);
            }
        }
    }

    Vec3 mn = verts[0], mx = verts[0];
    for (auto& p : verts) {
        mn.x = min(mn.x, p.x); mx.x = max(mx.x, p.x);
        mn.y = min(mn.y, p.y); mx.y = max(mx.y, p.y);
        mn.z = min(mn.z, p.z); mx.z = max(mx.z, p.z);
    }
    double diag = norm(mx - mn);
    double hausd = hparam_HausdorffDiagFraction * diag;
    double costCap = hparam_QemCostCapCoeff * diag * diag;
    double invDiag2 = (diag > CParam_MinNormalNorm) ? 1.0 / (diag*diag) : 0.0;

    auto edgeExists = [&](int a, int b) {
        return a>=0 && b>=0 && a<nV && b<nV && !vdead[a] && !vdead[b] && vneigh[a].contains(b);
    };
    auto countCommonFaces = [&](int a, int b) {
        int cnt=0;
        const auto& fa=vfaces[a], &fb=vfaces[b];
        if(fa.size()<fb.size()) {
            for(int f:fa) if(!fdead[f])
                for(int f2:fb) if(f==f2) { ++cnt; break; }
        } else {
            for(int f:fb) if(!fdead[f])
                for(int f2:fa) if(f==f2) { ++cnt; break; }
        }
        return cnt;
    };
    auto countCommonNeighbors = [&](int a, int b) {
        int cnt=0;
        const auto& na=vneigh[a], &nb=vneigh[b];
        if(na.size()<nb.size()) {
            for(int x:na) if(x!=a && x!=b && !vdead[x] && nb.contains(x)) ++cnt;
        } else {
            for(int x:nb) if(x!=a && x!=b && !vdead[x] && na.contains(x)) ++cnt;
        }
        return cnt;
    };
    auto faceNormalRaw = [&](int fi) -> Vec3 {
        const Face& f = faces[fi];
        return cross(verts[f.v[1]]-verts[f.v[0]], verts[f.v[2]]-verts[f.v[0]]);
    };

    struct CollapseCandidate {
        int absorbed=-1, kept=-1, versionAbsorbed=-1, versionKept=-1;
        double cost=CParam_Inf, mergedRadius=0.0;
        Vec3 position;
        bool valid() const { return absorbed>=0 && kept>=0 && cost<CParam_Inf; }
        bool operator<(const CollapseCandidate& o) const { return cost>o.cost; }
    };
    auto passesEnvelope = [&](int a, int b, const Vec3& p, double& mr) {
        mr = max(crad[a]+norm(verts[a]-p), crad[b]+norm(verts[b]-p));
        return mr <= hausd;
    };
    auto getCandidatePositions = [&](int a, int b, const Quadric& q, Vec3 pos[4], int& np) {
        np=0;
        Vec3 qp;
        if(solveQem3x3(q,qp)) pos[np++]=qp;
        pos[np++]=(verts[a]+verts[b])*0.5;
        pos[np++]=verts[a];
        pos[np++]=verts[b];
        int wp=0;
        for(int i=0;i<np;++i){
            if(!finiteVec(pos[i])) continue;
            bool dup=false;
            for(int j=0;j<wp;++j) if(norm2(pos[i]-pos[j])<1e-30) {dup=true; break;}
            if(!dup) pos[wp++]=pos[i];
        }
        np=wp;
    };
    auto makeCandidate = [&](int ab, int kp, const Vec3& p, const Quadric& q) {
        CollapseCandidate c;
        c.absorbed=ab; c.kept=kp;
        c.versionAbsorbed=vver[ab]; c.versionKept=vver[kp];
        c.position=p; c.cost=q.evaluate(p);
        return c;
    };
    auto computeBestValid = [&](int a, int b) {
        Quadric q = vquad[a]; q += vquad[b];
        Vec3 pos[4]; int np;
        getCandidatePositions(a,b,q,pos,np);
        CollapseCandidate best;
        for(int i=0;i<np;++i) {
            for(int dir=0;dir<2;++dir) {
                int ab=dir?b:a, kp=dir?a:b;
                double mr;
                if(!passesEnvelope(ab,kp,pos[i],mr)) continue;
                CollapseCandidate c = makeCandidate(ab,kp,pos[i],q);
                c.mergedRadius=mr;
                if(c.cost<best.cost) best=c;
            }
        }
        return best;
    };

    int accepted = 0;
    auto eraseVal = [](vector<int>& v, int x) {
        for(int i=(int)v.size()-1; i>=0; --i)
            if(v[i]==x) { v[i]=v.back(); v.pop_back(); return; }
    };
    auto applyCollapse = [&](int ab, int kp, const Vec3& np, double nr) {
        verts[kp]=np; crad[kp]=nr; crad[ab]=0;
        vdead[ab]=1; ++vver[ab]; ++vver[kp];
        auto abFaces=vfaces[ab];
        vector<int> dead; dead.reserve(4);
        for(int fi:abFaces){
            if(fdead[fi]) continue;
            bool touched=false;
            for(int k=0;k<3;++k)
                if(faces[fi].v[k]==ab) { faces[fi].v[k]=kp; touched=true; }
            if(!touched) continue;
            if(faces[fi].v[0]==faces[fi].v[1] ||
               faces[fi].v[1]==faces[fi].v[2] ||
               faces[fi].v[0]==faces[fi].v[2]) {
                fdead[fi]=1;
                dead.push_back(fi);
            } else {
                vfaces[kp].push_back(fi);
            }
        }
        for(int fi:dead)
            for(int k=0;k<3;++k) {
                int v=faces[fi].v[k];
                if(v>=0 && v<(int)vfaces.size()) eraseVal(vfaces[v],fi);
            }
        vfaces[ab].clear();
        vquad[kp] += vquad[ab];
        for(int nb:vneigh[ab]){
            if(nb==kp || vdead[nb]) continue;
            vneigh[nb].erase(ab);
            vneigh[nb].insert(kp);
            vneigh[kp].insert(nb);
        }
        vneigh[ab].clear();
        vneigh[kp].erase(ab);
        vneigh[kp].erase(kp);
    };

    auto startTime = chrono::steady_clock::now();
    auto elapsed = [&]() {
        return chrono::duration<double>(chrono::steady_clock::now() - startTime).count();
    };

    // ===== Phase 0: Coplanar edge collapse =====
    {
        struct CoplanarItem { double cost; int a,b; int ver_a,ver_b; };
        auto cmp = [](const CoplanarItem& x, const CoplanarItem& y) { return x.cost > y.cost; };
        priority_queue<CoplanarItem, vector<CoplanarItem>, decltype(cmp)> cq(cmp);
        for (int a=0; a<nV; ++a) {
            if (vdead[a]) continue;
            for (int b : vneigh[a]) {
                if (b <= a) continue;
                int fcnt=0; Vec3 n[2]; double d[2];
                for (int fi : vfaces[a]) {
                    if (fdead[fi]) continue;
                    const Face& f = faces[fi];
                    if (f.v[0]==b || f.v[1]==b || f.v[2]==b) {
                        if (fcnt < 2) {
                            Vec3 nr = faceNormalRaw(fi);
                            double len = norm(nr);
                            if (len < CParam_MinNormalNorm) continue;
                            n[fcnt] = nr/len;
                            d[fcnt] = -dot(n[fcnt], verts[f.v[0]]);
                            ++fcnt;
                        }
                    }
                }
                if (fcnt != 2) continue;
                double nd = fabs(dot(n[0], n[1]));
                double dd = fabs(d[0]-d[1]);
                if (1.0-nd > COPLANAR_EPS_NORMAL || dd > COPLANAR_EPS_OFFSET) continue;
                double cost = norm(verts[a]-verts[b]);
                cq.push({cost,a,b,vver[a],vver[b]});
            }
        }
        double stopTime = elapsed() + hparam_CoplanarBudgetSeconds;
        while (!cq.empty() && elapsed() < stopTime) {
            auto item = cq.top(); cq.pop();
            int a = item.a, b = item.b;
            if (vdead[a] || vdead[b]) continue;
            if (item.ver_a != vver[a] || item.ver_b != vver[b]) continue;
            if (!edgeExists(a,b)) continue;
            auto best = computeBestValid(a,b);
            if (!best.valid() || best.cost > costCap) continue;
            if (countCommonFaces(best.absorbed,best.kept) != 2) continue;
            if (countCommonNeighbors(best.absorbed,best.kept) != 2) continue;
            applyCollapse(best.absorbed,best.kept,best.position,best.mergedRadius);
            ++accepted;
            for (int nb : vneigh[best.kept]) {
                if (nb == best.kept || vdead[nb]) continue;
                int fcnt=0; Vec3 n[2]; double d[2];
                for (int fi : vfaces[best.kept]) {
                    if (fdead[fi]) continue;
                    const Face& f = faces[fi];
                    if (f.v[0]==nb || f.v[1]==nb || f.v[2]==nb) {
                        if (fcnt < 2) {
                            Vec3 nr = faceNormalRaw(fi);
                            double len = norm(nr);
                            if (len < CParam_MinNormalNorm) continue;
                            n[fcnt] = nr/len;
                            d[fcnt] = -dot(n[fcnt], verts[f.v[0]]);
                            ++fcnt;
                        }
                    }
                }
                if (fcnt==2 && 1.0-fabs(dot(n[0],n[1]))<COPLANAR_EPS_NORMAL && fabs(d[0]-d[1])<COPLANAR_EPS_OFFSET) {
                    double cost = norm(verts[best.kept]-verts[nb]);
                    cq.push({cost,best.kept,nb,vver[best.kept],vver[nb]});
                }
            }
        }
    }

    // ---- Camera‑aware face weights (parallel) ----
    vector<double> faceWeights(nF, 1.0);
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int fi = 0; fi < nF; ++fi) {
        if (fdead[fi]) continue;
        const Face& f = faces[fi];
        Vec3 n = cross(verts[f.v[1]]-verts[f.v[0]], verts[f.v[2]]-verts[f.v[0]]);
        double area = 0.5 * norm(n);
        if (area >= 1e-30) {
            Vec3 un = n / (2.0 * area);
            double absSum = fabs(un.x) + fabs(un.y) + fabs(un.z);
            double normalizedArea = area * invDiag2;
            double w = 1.0 + hparam_ViewWeightK * normalizedArea * absSum;
            if (w > hparam_MaxFaceWeight) w = hparam_MaxFaceWeight;
            faceWeights[fi] = w;
        }
    }

    // ---- Quadric accumulation (parallel reduction) ----
    vquad.assign(nV, Quadric());
#ifdef _OPENMP
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        vector<Quadric> localQuad(nV);
        #pragma omp for schedule(static)
        for (int fi = 0; fi < nF; ++fi) {
            if (fdead[fi]) continue;
            const Face& f = faces[fi];
            Quadric q = Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]);
            if (faceWeights[fi] != 1.0) q.scale(faceWeights[fi]);
            for (int k = 0; k < 3; ++k) localQuad[f.v[k]] += q;
        }
        #pragma omp critical
        { for (int i = 0; i < nV; ++i) vquad[i] += localQuad[i]; }
    }
#else
    for (int fi = 0; fi < nF; ++fi) {
        if (fdead[fi]) continue;
        const Face& f = faces[fi];
        Quadric q = Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]);
        if (faceWeights[fi] != 1.0) q.scale(faceWeights[fi]);
        for (int k = 0; k < 3; ++k) vquad[f.v[k]] += q;
    }
#endif

    // ---- Smarter vertex targeting ----
    double totalWeighted = 0.0, totalArea = 0.0;
    for (int fi = 0; fi < nF; ++fi) {
        if (fdead[fi]) continue;
        const Face& f = faces[fi];
        Vec3 n = cross(verts[f.v[1]]-verts[f.v[0]], verts[f.v[2]]-verts[f.v[0]]);
        double area = 0.5 * norm(n);
        totalArea += area;
        totalWeighted += area * faceWeights[fi];
    }
    double avgWeight = totalWeighted / totalArea;
    double kr;
    if (nV <= 6000) kr = hparam_KeepRatio_UpTo5k;
    else if (nV <= 30000) kr = hparam_KeepRatio_UpTo25k;
    else if (nV <= 45000) kr = hparam_KeepRatio_UpTo45k;
    else if (nV <= 60000) kr = hparam_KeepRatio_UpTo50k;
    else if (nV <= 450000) kr = hparam_KeepRatio_UpTo400k;
    else kr = hparam_KeepRatio_Huge;

    if (avgWeight < 1.2)      kr *= 0.80;
    else if (avgWeight < 1.5) kr *= 0.90;
    else if (avgWeight > 2.5) kr *= 1.15;
    if (kr < 0.005) kr = 0.005;
    if (kr > 0.95)  kr = 0.95;

    int targetV = max(10, (int)floor(nV * kr));
    targetV = min(targetV, nV - 1);
    int collapseLimit = nV - targetV;

    // ---- Phase 1: QEM collapse loop ----
    priority_queue<CollapseCandidate> pq;
    for (int a=0; a<nV; ++a) {
        if (vdead[a]) continue;
        for (int b : vneigh[a]) {
            if (b <= a) continue;
            auto c = computeBestValid(a,b);
            if (c.valid()) pq.push(c);
        }
    }
    double stageTimeBudget = hparam_FirstQemBudgetSeconds;
    int tick = 0;
    while (accepted < collapseLimit && !pq.empty()) {
        if ((++tick & 8191) == 0 && elapsed() > stageTimeBudget) break;
        auto c = pq.top(); pq.pop();
        int a = c.absorbed, b = c.kept;
        if (!edgeExists(a,b)) continue;
        if (c.versionAbsorbed != vver[a] || c.versionKept != vver[b]) {
            auto fr = computeBestValid(a,b);
            if (fr.valid()) pq.push(fr);
            continue;
        }
        if (c.cost > costCap) break;
        if (countCommonFaces(a,b) != 2) continue;
        if (countCommonNeighbors(a,b) != 2) continue;
        auto best = computeBestValid(a,b);
        if (!best.valid() || best.cost > costCap) continue;
        applyCollapse(best.absorbed, best.kept, best.position, best.mergedRadius);
        ++accepted;
        for (int nb : vneigh[best.kept]) {
            if (nb == best.kept || vdead[nb]) continue;
            auto nc = computeBestValid(best.kept, nb);
            if (nc.valid()) pq.push(nc);
        }
    }

    // =============== Phase 2: Star‑delete + SSIM guard ===============
    int tier = 1;
    if (nV > 6000) tier = 2;
    if (nV > 25000) tier = 3;
    if (nV > 40000) tier = 4;
    if (nV > 50000) tier = 5;
    if (nV > 400000) tier = 6;
    if (tier > 6) tier = 6;

    const StarDeleteParams& params = hparam_GeneralStarParamsByTier[tier];
    int maxValence = params.maxValence;
    double distFrac = params.distFrac;
    int hardCap = params.hardCap;
    int rounds = params.rounds;
    double maxSeconds = params.maxSeconds;
    double availTime = hparam_TotalBudgetSeconds - elapsed();
    if (availTime > 0) {
        double phaseTime = min(availTime * 0.9, maxSeconds);

        // ----- SSIM rasteriser (128×128) -----
        const int SSIM_RES = 128;
        const int SSIM_VIEWS = 6;
        const double D_CAM = 2.5;
        const double FOCAL = 100.0;   // 800 * 128/1024
        const double PRINC = 64.0;    // 512 * 128/1024
        const Vec3 camPos[6] = {{D_CAM,0,0},{-D_CAM,0,0},{0,D_CAM,0},{0,-D_CAM,0},{0,0,D_CAM},{0,0,-D_CAM}};
        const Vec3 camUp[6]  = {{0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}};

        auto project = [&](const Vec3& world, int view, double& u, double& v, double& depth) {
            Vec3 dir = Vec3(0,0,0) - camPos[view];
            dir = dir / norm(dir);
            Vec3 right = cross(dir, camUp[view]);
            right = right / norm(right);
            Vec3 up = cross(right, dir);
            Vec3 rel = world - camPos[view];
            double x = dot(rel, right);
            double y = dot(rel, up);
            double z = dot(rel, dir);
            depth = -z;
            if (depth < 1e-9) return;
            double invZ = 1.0 / depth;
            u = FOCAL * x * invZ + PRINC;
            v = FOCAL * y * invZ + PRINC;
        };

        auto rasteriseTriangle = [&](const Vec3& p0, const Vec3& p1, const Vec3& p2,
                                     const Vec3& fn, int view,
                                     vector<double>& nr, vector<double>& ng,
                                     vector<double>& nb, vector<double>& depthBuf) {
            double u0,v0,d0, u1,v1,d1, u2,v2,d2;
            project(p0, view, u0, v0, d0);
            project(p1, view, u1, v1, d1);
            project(p2, view, u2, v2, d2);
            if (d0 < 1e-9 || d1 < 1e-9 || d2 < 1e-9) return;
            int minU = max(0, (int)floor(min({u0,u1,u2})));
            int maxU = min(SSIM_RES-1, (int)ceil(max({u0,u1,u2})));
            int minV = max(0, (int)floor(min({v0,v1,v2})));
            int maxV = min(SSIM_RES-1, (int)ceil(max({v0,v1,v2})));
            auto edge = [](double ax, double ay, double bx, double by, double cx, double cy) {
                return (bx-ax)*(cy-ay) - (by-ay)*(cx-ax);
            };
            double area = edge(u0,v0, u1,v1, u2,v2);
            if (fabs(area) < 1e-12) return;
            double invArea = 1.0 / area;
            double invD0 = 1.0/d0, invD1 = 1.0/d1, invD2 = 1.0/d2;
            for (int py = minV; py <= maxV; ++py) {
                for (int px = minU; px <= maxU; ++px) {
                    double cx = px + 0.5, cy = py + 0.5;
                    double w0 = edge(u1,v1, u2,v2, cx,cy);
                    double w1 = edge(u2,v2, u0,v0, cx,cy);
                    double w2 = edge(u0,v0, u1,v1, cx,cy);
                    if (w0 < 0 || w1 < 0 || w2 < 0) continue;
                    w0 *= invArea; w1 *= invArea; w2 *= invArea;
                    double z = 1.0 / (w0*invD0 + w1*invD1 + w2*invD2);
                    int idx = py*SSIM_RES + px;
                    if (z > depthBuf[idx]) continue;
                    depthBuf[idx] = z;
                    nr[idx] = (fn.x + 1.0) * 127.5;
                    ng[idx] = (fn.y + 1.0) * 127.5;
                    nb[idx] = (fn.z + 1.0) * 127.5;
                }
            }
        };

        // Render mesh into flat vectors (normal 3 channels, depth)
        auto renderMesh = [&](const vector<Vec3>& verts, const vector<Face>& faces,
                              const vector<char>& fdead,
                              vector<double> (&outNorm)[6], vector<double> (&outDepth)[6]) {
            for (int v = 0; v < 6; ++v) {
                int npix = SSIM_RES * SSIM_RES;
                outNorm[v].assign(npix * 3, 127.5);
                outDepth[v].assign(npix, 255.0);
                vector<double> nr(npix, 127.5), ng(npix, 127.5), nb(npix, 127.5), depth(npix, 255.0);
                for (int fi = 0; fi < (int)faces.size(); ++fi) {
                    if (fi < nF && fdead[fi]) continue;
                    const Face& f = faces[fi];
                    Vec3 n = cross(verts[f.v[1]]-verts[f.v[0]], verts[f.v[2]]-verts[f.v[0]]);
                    double len = norm(n);
                    if (len < 1e-12) continue;
                    n = n / len;
                    rasteriseTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]], n, v,
                                      nr, ng, nb, depth);
                }
                for (int i = 0; i < npix; ++i) {
                    outNorm[v][i] = nr[i];
                    outNorm[v][i + npix] = ng[i];
                    outNorm[v][i + 2*npix] = nb[i];
                    outDepth[v][i] = depth[i];
                }
            }
        };

        // SSIM helpers
        auto ssimChannel = [&](const vector<double>& a, const vector<double>& b, int N) -> double {
            double muA=0, muB=0, varA=0, varB=0, cov=0;
            int n=0;
            for (int i=0; i<N; ++i) {
                if (a[i] > 254.99 && b[i] > 254.99) continue;
                muA += a[i]; muB += b[i];
                ++n;
            }
            if (n == 0) return 1.0;
            muA /= n; muB /= n;
            for (int i=0; i<N; ++i) {
                if (a[i] > 254.99 && b[i] > 254.99) continue;
                double da = a[i] - muA, db = b[i] - muB;
                varA += da*da; varB += db*db; cov += da*db;
            }
            varA /= (n-1); varB /= (n-1); cov /= (n-1);
            double c1 = hparam_SsimC1, c2 = hparam_SsimC2;
            return (2*muA*muB + c1)*(2*cov + c2) / ((muA*muA + muB*muB + c1)*(varA + varB + c2));
        };

        auto computePatchSSIM = [&](const vector<double>& origN, const vector<double>& newN,
                                    const vector<double>& origD, const vector<double>& newD,
                                    int patchW, int patchH) -> double {
            int npix = patchW * patchH;
            vector<double> oR(npix), oG(npix), oB(npix);
            vector<double> nR(npix), nG(npix), nB(npix);
            for (int i=0; i<npix; ++i) {
                oR[i] = origN[i];           nR[i] = newN[i];
                oG[i] = origN[i + npix];    nG[i] = newN[i + npix];
                oB[i] = origN[i + 2*npix];  nB[i] = newN[i + 2*npix];
            }
            double ssimR = ssimChannel(oR, nR, npix);
            double ssimG = ssimChannel(oG, nG, npix);
            double ssimB = ssimChannel(oB, nB, npix);
            double ssimN = (ssimR + ssimG + ssimB) / 3.0;
            double ssimD = ssimChannel(origD, newD, npix);
            return hparam_SsimNormalDepthWeight * ssimN + (1.0 - hparam_SsimNormalDepthWeight) * ssimD;
        };

        // Render original mesh once
        vector<double> origNormal[6], origDepth[6];
        renderMesh(verts, faces, fdead, origNormal, origDepth);

        // Buffers for current rendering
        vector<double> curNormal[6], curDepth[6];
        for (int v = 0; v < 6; ++v) {
            curNormal[v].assign(SSIM_RES * SSIM_RES * 3, 0.0);
            curDepth[v].assign(SSIM_RES * SSIM_RES, 0.0);
        }

        double ssimDamage = 0.0;
        int starDeleted = 0;

        for (int round = 0; round < rounds && elapsed() < phaseTime; ++round) {
            vector<int> candidates;
            for (int v = 0; v < nV; ++v) {
                if (vdead[v]) continue;
                if ((int)vneigh[v].size() < 4 || (int)vneigh[v].size() > maxValence) continue;
                Vec3 avg(0,0,0);
                for (int nb : vneigh[v]) avg = avg + verts[nb];
                avg = avg / (double)vneigh[v].size();
                Vec3 n;
                {
                    Vec3 sumN(0,0,0);
                    for (int fi : vfaces[v]) {
                        if (fdead[fi]) continue;
                        Vec3 fn = cross(verts[faces[fi].v[1]]-verts[faces[fi].v[0]],
                                        verts[faces[fi].v[2]]-verts[faces[fi].v[0]]);
                        double len = norm(fn);
                        if (len < 1e-12) continue;
                        sumN = sumN + fn/len;
                    }
                    double len = norm(sumN);
                    if (len < 1e-12) continue;
                    n = sumN / len;
                }
                double dist = fabs(dot(verts[v] - avg, n));
                if (dist > distFrac * hausd) continue;
                candidates.push_back(v);
            }
            sort(candidates.begin(), candidates.end(), [&](int a, int b) {
                Vec3 avgA(0,0,0), avgB(0,0,0);
                for (int nb : vneigh[a]) avgA = avgA + verts[nb];
                avgA = avgA / (double)vneigh[a].size();
                for (int nb : vneigh[b]) avgB = avgB + verts[nb];
                avgB = avgB / (double)vneigh[b].size();
                return norm(verts[a] - avgA) < norm(verts[b] - avgB);
            });

            for (int v : candidates) {
                if (vdead[v]) continue;
                if (starDeleted >= hardCap) break;
                if (elapsed() >= phaseTime) break;

                // Order 1‑ring neighbors.
                // The original traversal used a do/while (cur != start) without ever
                // consulting its `visited` set, and relied on a fragile back-pop for
                // undo. Both were buggy on topology-corrupted meshes. Here we walk
                // strictly up to the expected ring size, dedupe visited vertices, and
                // require a clean cycle as a precondition before fan-triangulating.
                vector<int> ring;
                {
                    int target = (int)vneigh[v].size();
                    int start  = vneigh[v].data[0];
                    int cur    = start;
                    int prev   = -1;
                    bool valid = true;
                    ring.push_back(cur);
                    while ((int)ring.size() < target) {
                        bool found = false;
                        for (int nb : vneigh[v]) {
                            if (nb == prev || nb == cur) continue;
                            for (int fi : vfaces[v]) {
                                if (fi < nF && fdead[fi]) continue;
                                const Face& f = faces[fi];
                                bool ok = false;
                                int xs[3] = {f.v[0], f.v[1], f.v[2]};
                                for (int s = 0; s < 3; ++s) {
                                    if (xs[s] != v) continue;
                                    int x = xs[(s+1)%3], y = xs[(s+2)%3];
                                    if ((x == cur && y == nb) || (x == nb && y == cur)) {
                                        ok = true; break;
                                    }
                                }
                                if (!ok) continue;
                                bool dup = false;
                                for (int t : ring) { if (t == nb) { dup = true; break; } }
                                if (dup) { valid = false; break; }
                                prev = cur;
                                cur  = nb;
                                ring.push_back(cur);
                                found = true;
                                break;
                            }
                            if (!valid || found) break;
                        }
                        if (!valid) break;
                        if (!found) break;       // cannot extend further -> reject
                    }
                    if (!valid || (int)ring.size() != target) continue;
                }                // Fan triangulation from ring[0]
                int center = ring[0];
                vector<Face> newTris;
                for (int i = 1; i < (int)ring.size()-1; ++i) {
                    Face f;
                    f.v[0] = center;
                    f.v[1] = ring[i];
                    f.v[2] = ring[i+1];
                    Vec3 n = cross(verts[f.v[1]]-verts[f.v[0]], verts[f.v[2]]-verts[f.v[0]]);
                    if (norm(n) < 1e-12) continue;
                    newTris.push_back(f);
                }
                if (newTris.empty()) continue;

                // Check normal orientation
                Vec3 holeNormal(0,0,0);
                for (int fi : vfaces[v]) {
                    if (fdead[fi]) continue;
                    Vec3 fn = cross(verts[faces[fi].v[1]]-verts[faces[fi].v[0]],
                                    verts[faces[fi].v[2]]-verts[faces[fi].v[0]]);
                    double len = norm(fn);
                    if (len < 1e-12) continue;
                    holeNormal = holeNormal + fn/len;
                }
                double lenHN = norm(holeNormal);
                if (lenHN < 1e-12) continue;
                holeNormal = holeNormal / lenHN;
                bool flipped = false;
                for (const Face& f : newTris) {
                    Vec3 n = cross(verts[f.v[1]]-verts[f.v[0]], verts[f.v[2]]-verts[f.v[0]]);
                    double len = norm(n);
                    if (len < 1e-12) { flipped = true; break; }
                    n = n / len;
                    if (dot(n, holeNormal) < 0.5) { flipped = true; break; }
                }
                if (flipped) continue;

                // Hausdorff check
                double maxDev = 0;
                for (const Face& f : newTris) {
                    Vec3 n = cross(verts[f.v[1]]-verts[f.v[0]], verts[f.v[2]]-verts[f.v[0]]);
                    double area = norm(n);
                    if (area < 1e-12) continue;
                    n = n / area;
                    double d = fabs(dot(verts[v] - verts[f.v[0]], n));
                    if (d > maxDev) maxDev = d;
                }
                if (maxDev > distFrac * hausd) continue;

                // ---- Transactional star-deletion ----
                // Apply the candidate, render, compute SSIM, then either keep or
                // roll back precisely. The previous implementation had two serious
                // bugs that we are fixing here:
                //   1. rollback was a partial back-pop on vfaces that almost never
                //      matched (left stale entries that corrupted subsequent rounds),
                //   2. the "permanent apply" path used resize() then push_back(),
                //      which inserts N empty Face{0,0,0} slots *before* the actual
                //      new triangles, polluting the face array with degenerate
                //      entries that leak into vfaces[] and the winding checks.
                vector<int> oldFaces;
                oldFaces.reserve(vfaces[v].size());
                for (int fi : vfaces[v]) {
                    if (fi < nF && fdead[fi]) continue;
                    oldFaces.push_back(fi);
                }
                if ((int)oldFaces.size() != (int)ring.size()) continue; // must close 1-ring

                // Mark old faces dead and the vertex dead; snapshot sizes.
                for (int fi : oldFaces) fdead[fi] = 1;
                vdead[v] = 1;
                int oldFaceCount  = (int)faces.size();
                int oldFdeadCount = (int)fdead.size();

                // Push new fan triangles and remember *exact* indices/edges added.
                vector<int> newFaceIdx;
                newFaceIdx.reserve(newTris.size());
                vector<pair<int,int>> newEdges;
                newEdges.reserve(newTris.size() * 3);
                for (const Face& f : newTris) {
                    faces.push_back(f);
                    fdead.push_back(0);
                    int idx = (int)faces.size() - 1;
                    newFaceIdx.push_back(idx);
                    for (int k = 0; k < 3; ++k) vfaces[f.v[k]].push_back(idx);
                    for (int k = 0; k < 3; ++k) {
                        int a = f.v[k], b = f.v[(k+1)%3];
                        if (a == b) continue;
                        bool wasNew = !vneigh[a].contains(b);
                        vneigh[a].insert(b);
                        vneigh[b].insert(a);
                        if (wasNew) newEdges.push_back({a, b});
                    }
                }

                // SSIM check
                renderMesh(verts, faces, fdead, curNormal, curDepth);
                double patchSSIM = 0.0;
                for (int vi = 0; vi < 6; ++vi) {
                    patchSSIM += computePatchSSIM(origNormal[vi], curNormal[vi],
                                                  origDepth[vi], curDepth[vi],
                                                  SSIM_RES, SSIM_RES);
                }
                patchSSIM /= 6.0;

                bool accept = (patchSSIM >= hparam_SsimAcceptMin) &&
                              (ssimDamage + (1.0 - patchSSIM) <= hparam_SsimMaxDamage);

                if (!accept) {
                    // Precise rollback: removes the *exact* face indices and the
                    // *exact* vneigh edges we added (no resize+push duplication,
                    // no leaked neighborhood edges).
                    for (int fi : oldFaces) fdead[fi] = 0;
                    vdead[v] = 0;
                    faces.resize(oldFaceCount);
                    fdead.resize(oldFdeadCount);
                    for (int idx : newFaceIdx) {
                        const Face& f = faces[idx];
                        for (int k = 0; k < 3; ++k) {
                            int a = f.v[k];
                            auto& vf = vfaces[a];
                            for (auto it = vf.begin(); it != vf.end(); ++it) {
                                if (*it == idx) { vf.erase(it); break; }
                            }
                        }
                    }
                    for (auto& e : newEdges) {
                        vneigh[e.first].erase(e.second);
                        vneigh[e.second].erase(e.first);
                    }
                } else {
                    ssimDamage += (1.0 - patchSSIM);
                    ++starDeleted;
                    ++accepted;
                }
            }
        }
    }
    // ---- Final compact and output ----
    vector<int> o2n(nV, -1);
    vector<Vec3> nv; nv.reserve(nV-accepted);
    for (int i=0; i<nV; ++i) if (!vdead[i]) { o2n[i] = (int)nv.size(); nv.push_back(verts[i]); }

    struct FK { array<int,3> key; Face face; bool operator<(const FK& o) const { return key < o.key; } };
    vector<FK> fc; fc.reserve(faces.size());
    for (int fi=0; fi<(int)faces.size(); ++fi) {
        if (fi < nF && fdead[fi]) continue;
        int a = faces[fi].v[0], b = faces[fi].v[1], c = faces[fi].v[2];
        if (vdead[a] || vdead[b] || vdead[c] || a==b || b==c || a==c) continue;
        int na = o2n[a], nb = o2n[b], nc = o2n[c];
        if (na<0 || nb<0 || nc<0 || na==nb || nb==nc || na==nc) continue;
        Face nf; nf.v[0]=na; nf.v[1]=nb; nf.v[2]=nc;
        array<int,3> key = {na,nb,nc}; sort(key.begin(), key.end());
        fc.push_back({key,nf});
    }
    sort(fc.begin(), fc.end());
    vector<Face> final_faces; final_faces.reserve(fc.size());
    array<int,3> prev = {-1,-1,-1};
    for (auto& item : fc) {
        if (item.key == prev) continue;
        prev = item.key;
        final_faces.push_back(item.face);
    }

    V.resize((int)nv.size(), 3);
    for (int i=0; i<(int)nv.size(); ++i) {
        V(i,0) = nv[i].x; V(i,1) = nv[i].y; V(i,2) = nv[i].z;
    }
    F.resize((int)final_faces.size(), 3);
    for (int i=0; i<(int)final_faces.size(); ++i) {
        F(i,0) = final_faces[i].v[0];
        F(i,1) = final_faces[i].v[1];
        F(i,2) = final_faces[i].v[2];
    }
}

int main() {
    load_obj();
    simplify();
    save_obj();
    return 0;
}
