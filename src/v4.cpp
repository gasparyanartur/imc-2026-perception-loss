// lynx v4 82pts no param tuning

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

static constexpr double hparam_HausdorffDiagFraction = 0.05;

// ---- Numerical guards ----
static constexpr double CParam_MinNormalNorm = 1e-8;
static constexpr double CParam_QemSolveDeterminantEps = 1e-8;
static constexpr double CParam_Inf = 1e100;

// ---- Overall time budget (seconds) ----
static constexpr double hparam_TotalBudgetSeconds = 20.5;

// ---- Phase 0 (coplanar) time budget ----
static constexpr double hparam_CoplanarBudgetSeconds = 0.5;

// ---- Phase 1 (QEM) time budget (seconds) ----
static constexpr double hparam_FirstQemBudgetSeconds = 10.0;

// ---- Phase 2 (general star-delete) time budget (seconds) ----
static constexpr double hparam_GeneralStarPostBudgetSeconds = 17.0;

// ---- Output precision ----
static constexpr int    hparam_OutputPrecisionSignificantDigits = 10;

// ---- QEM target keep ratios (by input vertex count) ----
static constexpr double hparam_KeepRatio_UpTo5k    = 0.00;
static constexpr double hparam_KeepRatio_UpTo25k   = 0.36;
static constexpr double hparam_KeepRatio_UpTo45k   = 0.25;
static constexpr double hparam_KeepRatio_UpTo50k   = 0.15;
static constexpr double hparam_KeepRatio_UpTo400k  = 0.027;
static constexpr double hparam_KeepRatio_Huge      = 0.0325;

// ---- QEM cost cap ----
static constexpr double hparam_QemCostCapCoeff = 0.0375;

// ---- Coplanar edge collapse thresholds ----
static constexpr double COPLANAR_EPS_NORMAL = 1e-5;
static constexpr double COPLANAR_EPS_OFFSET = 1e-5;

// ---- Camera-aware face weight ----
static constexpr double hparam_ViewWeightK = 3.0;
static constexpr double hparam_MaxFaceWeight = 3.0;

// ---- General star-delete parameters (tier dependent) ----
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
    {5,0.004,0.006,0.40,0.0015,900,105000,1,0.25,0.90},
    {6,0.008,0.012,0.52,0.0030,1800,160000,2,0.36,1.40},
    {5,0.0088,0.0130,0.56,0.0036,1950,160000,1,0.38,1.55},
    {5,0.006,0.009,0.46,0.0022,1300,130000,1,0.31,1.10},
    {7,0.012,0.018,0.64,0.0050,3200,240000,3,0.48,2.05}
};

// ---- SSIM-based star-delete parameters (tier dependent) ----
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

// ---- Relaxed thresholds for the global‑safety‑net extra pass ----
static constexpr double hparam_SsimAcceptMinRelaxed = 0.88;
static constexpr double hparam_SsimMaxDamageRelaxed = 0.015;
static constexpr double hparam_SsimGlobalSafeThreshold = 0.92;
static constexpr double hparam_SsimGlobalFinalThreshold = 0.90;

// ---- Star-delete params for SSIM pass (original) ----
static constexpr StarDeleteParams hparam_SsimStarParamsByTier[7] = {
    {0,0,0,0,0,0,0,0,0,0},
    {14,0.260,0.340,1.00,0.0120,6000,280000,1,0.60,1.10},
    {7, 0.018,0.026,0.82,0.0012,500, 90000, 1,0.45,0.55},
    {8, 0.026,0.038,0.86,0.0016,700, 110000,1,0.45,0.65},
    {8, 0.028,0.042,0.88,0.0018,850, 120000,1,0.45,0.75},
    {7, 0.018,0.028,0.80,0.0008,350, 80000, 1,0.38,0.45},
    {9, 0.040,0.060,0.92,0.0020,1600,180000,1,0.48,0.95}
};

// ---- Lens‑original cache parameters ----
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

// ---- Global safety‑net timing ----
static constexpr double hparam_GlobalSafetyNetTimeFrac = 0.8;
static constexpr double hparam_GlobalSafetyNetMaxSeconds = 2.5;

// ============================================================================
// Scaffold I/O (using Eigen matrices V, F)
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
        ++p; // 'v'
        V(i, 0) = strtod(p, &p);
        V(i, 1) = strtod(p, &p);
        V(i, 2) = strtod(p, &p);
    }
    for (long i = 0; i < nf; ++i) {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p;
        ++p; // 'f'
        F(i, 0) = (int)strtol(p, &p, 10) - 1;
        F(i, 1) = (int)strtol(p, &p, 10) - 1;
        F(i, 2) = (int)strtol(p, &p, 10) - 1;
    }
}

static void save_obj() {
    string out;
    out.reserve((size_t)V.rows() * 40 + (size_t)F.rows() * 24 + 32);
    char line[96];

    out.append(line, snprintf(line, sizeof line, "%ld %ld\n",
                              (long)V.rows(), (long)F.rows()));
    for (Eigen::Index i = 0; i < V.rows(); ++i)
        out.append(line, snprintf(line, sizeof line, "v %.10g %.10g %.10g\n",
                                  V(i, 0), V(i, 1), V(i, 2)));
    for (Eigen::Index i = 0; i < F.rows(); ++i)
        out.append(line, snprintf(line, sizeof line, "f %d %d %d\n",
                                  F(i, 0) + 1, F(i, 1) + 1, F(i, 2) + 1));

    fwrite(out.data(), 1, out.size(), stdout);
}

// ============================================================================
// Geometry primitives
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
// Main simplification function
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
        for (int k = 0; k < 3; ++k)
            vfaces[f.v[k]].push_back(fi);
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

    // ===== Phase 0: original coplanar pass (safe, short budget) =====
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
        {
            for (int i = 0; i < nV; ++i) vquad[i] += localQuad[i];
        }
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

    // ---- Smarter vertex targeting (conservative) ----
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

    if (avgWeight > 2.5) kr *= 1.15;
    if (kr > 0.95) kr = 0.95;

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

    // ---- Final compact and output ----
    vector<int> o2n(nV, -1);
    vector<Vec3> nv; nv.reserve(nV-accepted);
    for (int i=0; i<nV; ++i) if (!vdead[i]) { o2n[i] = (int)nv.size(); nv.push_back(verts[i]); }

    struct FK { array<int,3> key; Face face; bool operator<(const FK& o) const { return key < o.key; } };
    vector<FK> fc; fc.reserve(faces.size());
    for (int fi=0; fi<nF; ++fi) {
        if (fdead[fi]) continue;
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
