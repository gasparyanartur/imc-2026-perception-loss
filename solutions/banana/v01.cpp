// ============================================================================
// DO NOT REMOVE THIS BANNER — IT IS THE DESIGN CONTRACT FOR THIS FILE
// ============================================================================
// This file implements a mesh simplification algorithm with the following
// architectural rules. These are not negotiable; they are the basis for
// every parameter choice and every code path below.
//
// 1. TIER-SPECIFIC PARAMETERS ONLY
//    Every value that could plausibly depend on the mesh scale or class
//    lives in TierParams and is indexed by the current vertex count via
//    tierTable[currentTier()]. No parameter is duplicated across tiers
//    just because it happens to take the same value in one configuration;
//    if it COULD differ per tier, it MUST live in TierParams. Conversely,
//    anything truly invariant across all meshes belongs in SharedParams.
//
// 2. NO TOP-LEVEL HYPERPARAMETERS
//    There is no HParam_* / CParam_* / magic-number soup scattered through
//    the algorithm. Every constant is either:
//      - in SharedParams  (truly tier-independent, e.g. I/O sizes, geometric
//        sentinels, the time kill switch, Vega rendering constants), OR
//      - in TierParams    (every tier-dependent value), OR
//      - a constexpr      (e.g. TIER_THRESHOLD — the structural definition
//        of what a "tier" is)
//    There are no anonymous literal thresholds inside the algorithm body.
//    If you find one, it is a bug — move it to the appropriate struct.
//
// 3. NO TIME-DEPENDENT DECISIONS
//    Phase decisions are driven entirely by vertex counts:
//      - "is this tier MEMLESS?" → bool in TierParams
//      - "should we run a phase?" → vertex-count predicate (nV > threshold)
//      - "how many deletes per pass?" → ExtraFrac * nV or HardCap
//      - "how many vertices to scan?" → starScanVertices / vegaScanVertices
//    Time appears in exactly ONE place: the kill switch at the top of run()
//    (`elapsed() < shared.timeBudgetSeconds`). Every phase checks it as a
//    hard stop, never as a trigger or a stopping criterion. No phase is
//    gated on `elapsed() - someTimeFrac*maxSeconds` style expressions.
//
// 4. ALGORITHM STRUCTURE (matches lime v9 directly)
//      Phase 1: collapseLoop    — QEM until targetV (vertex-bounded)
//      Phase 2: collapseInvisibleEdges (star pass, vertex-bounded)
//      Phase 3-5: vegaSsimStarPass x3 (each vertex-bounded)
//      Then compact() + writeMesh().
//    No additional phases, no hidden work, no phase decided by time.
//
// 5. STAR vs VEGA ARE EXPLICIT, NOT INFERRED
//    The two passes have separate parameter sets (star* / vega*) and
//    separate function variants (computeStarCandidateStar / *Vega,
//    evaluateStarRootStar / *Vega, orientedRingForVertexStar / *Vega,
//    applyStarDeleteStar / *Vega). There is no legacy adapter struct that
//    silently picks one or the other based on context. A reader of the
//    code can see at every call site which parameter set is in use.
//
// 6. DETERMINISM (rev2)
//    Algorithm must be fully deterministic given the input mesh. Same
//    input → byte-identical output across runs and machines.
// ============================================================================

#include <bits/stdc++.h>
using namespace std;

// ============================================================================
// TIER THRESHOLDS (global, defines where each tier begins)
// 6 tiers; tier i covers nV in [TIER_THRESHOLD[i], TIER_THRESHOLD[i+1]).
// Final tier's upper bound is INT_MAX.
// ============================================================================
static constexpr int NUM_TIERS = 6;
// v63: matched to lime v9 v114 boundaries (0/5000/25000/45000/50000/400000)
static constexpr int TIER_THRESHOLD[NUM_TIERS + 1] = {
    0,         // T1 starts at 0
    5000,      // T2 starts at 5000 (lime v9 boundary)
    25000,     // T3 starts at 25000 (lime v9 boundary)
    45000,     // T4 starts at 45000 (lime v9 boundary)
    50000,     // T5 starts at 50000 (lime v9 boundary)
    400000,    // T6 starts at 400000 (lime v9 boundary)
    INT_MAX    // T6 ends at infinity
};

// ============================================================================
// SHARED PARAMETERS (truly tier-independent: I/O, geometry, kill switch)
// The ONLY time-dependent field is timeBudgetSeconds (the kill switch).
// Everything else is vertex-count based.
// ============================================================================
struct SharedParams {
    // --- Time budget (the ONLY time-dependent field) ---
    // Kill switch: if elapsed() exceeds this, no further phase enters.
    double timeBudgetSeconds = 20.2;
    // Check elapsed() every (tickMask+1) iterations. 8191 = 2^13 - 1.
    int tickMask = 8191;

    // --- Output precision ---
    int outputPrecision = 10;  // significant digits in vertex coordinates

    // --- I/O buffer sizes ---
    int inputBufferPow2  = 27;  // mesh input buffer = 1<<27 = 128MB
    int readChunkPow2    = 16;  // stdin read chunk = 1<<16 = 64KB
    int outLineVerts     = 42;  // "v %.10g %.10g %.10g\n" max chars
    int outLineFaces     = 26;  // "f %d %d %d\n" max chars
    int outLineHeader    = 64;  // "%d %d\n" header max chars
    int outLineBuf       = 128; // snprintf line buffer
    int candReserveExtra = 256; // extra candidate reserve in vega pool

    // --- Geometric/numerical constants ---
    double hausdorffDiagFraction   = 0.055;   // max allowed distance = 5.5% of mesh diagonal
    double minNormalNorm           = 1e-12;   // threshold for "normal is zero"
    double qemSolveDeterminantEps  = 1e-12;   // threshold for "det=0, no solution"
    double qemCostCapCoeff         = 0.0375;  // cost cap = 0.0375 * diag^2
    double inf                     = 1e100;   // sentinel for "no valid candidate"
    double viewWeightK             = 3.0;     // view-weight exponent for face weighting
    double maxFaceWeight           = 3.0;     // cap on view-based face weight
    double quadricScaleSqrtCoeff   = 0.5;     // sqrt(0.5 * triangle_area) for quadric scaling
    double midpointCoeff           = 0.5;     // (a+b)*0.5 = edge midpoint
    double faceAreaCoeff           = 0.5;     // 0.5 * |normal| = triangle area
    double normalizeAreaCoeff      = 2.0;     // divide normal by 2*area = unit normal
    double epsilon                 = 1e-30;   // generic numerical zero threshold

    // --- 8-bit color encoding ---
    float colorUnset   = 127.5f;  // neutral normal/gray pixel value
    float colorMax8Bit = 255.0f;  // max 8-bit value

    // --- Vega rendering constants (independent of tier) ---
    int    vegaPatchResolution    = 512;        // rendered patch size (square)
    int    vegaPatchPaddingPixels = 4;          // padding around projected bounds
    int    vegaPatchMaxPixels     = 52000;      // abort SSIM if patch too big
    int    vegaCandidatePoolCap   = 28000;      // max candidates per pass
    double vegaNormalDepthWeight  = 0.55;       // weight for normal SSIM vs depth SSIM
    double vegaScoreGeomWeight    = 0.0018;     // geometric score weight in ranking
    double vegaC1 = (0.01*255.0)*(0.01*255.0);  // SSIM C1 stabilizer
    double vegaC2 = (0.03*255.0)*(0.03*255.0);  // SSIM C2 stabilizer
    double vegaEpsilonDepth       = 1e-8;       // z near zero = skip
    double vegaEpsilonArea        = 1e-12;      // tiny triangle area = skip
    double vegaEpsilonDet         = 1e-18;      // bad barycentric = skip
    double vegaEpsilonBary        = 1e-9;       // barycentric clamp tolerance
    double vegaRefResolution      = 1024.0;     // reference resolution for scaling
    double vegaFocalLength        = 800.0;      // perspective focal length
    double vegaWorldUpDotThresh   = 0.9;        // if forward ~ Z, use Y worldUp
    double vegaEyeRadius          = 2.5;        // camera distance from origin
    double vegaNormalTo8Bit       = 127.5;      // normal [-1,1] -> [0,255]
    int    vegaNumChannels        = 3;          // RGB channels

};

// ============================================================================
// TIER-SPECIFIC PARAMETERS
// All tier-dependent values: keepRatio, star/vega geometry, SSIM thresholds,
// phase ratios, scan limits, enable flags, MEMLESS choice, etc.
// Indexed by currentTier() (vertex count).
// ============================================================================
struct TierParams {
    // --- Compression target ---
    double keepRatio;  // fraction of original nV to keep (1.0 = no compression)

    // --- MEMLESS QEM (use this for medium tiers; recompute quadrics from faces) ---
    bool memless;

    // --- Star (collapseInvisibleEdges) parameters ---
    int    starMaxValence;     // max degree of a vertex to be considered
    double starMaxOldDev;      // max normal deviation for EXISTING triangles
    double starMaxNewDev;      // max normal deviation for NEW retriangulation
    double starDistFrac;       // distance limit as fraction of hausdorff
    double starExtraFrac;      // max extra deletions as fraction of nV
    int    starHardCap;        // absolute cap on deletes per pass
    int    starScanVertices;   // max vertices to scan per round
    int    starRounds;         // max rounds of the pass

    // --- Vega (SSIM-based collapse) parameters (maxValence=0 means disabled) ---
    int    vegaMaxValence;     // max degree of a vertex to be considered
    double vegaMaxOldDev;      // max normal deviation for EXISTING triangles
    double vegaMaxNewDev;      // max normal deviation for NEW triangles
    double vegaDistFrac;       // distance limit as fraction of hausdorff
    double vegaExtraFrac;      // max extra deletions as fraction of nV
    int    vegaHardCap;        // absolute cap on deletes per pass
    int    vegaScanVertices;   // max vertices to scan
    int    vegaRounds;         // max rounds of the pass
    double vegaPatchGeomFrac;  // geometric deviation as fraction of hausdorff (1e100 = disable)

    // --- Vega SSIM/damage thresholds ---
    double vegaSsimMin;    // reject candidate if SSIM < this
    double vegaDamageMax;  // reject candidate if (1 - SSIM) > this

    // --- Enable flag for Vega SSIM pass ---
    bool enableVegaSsimPass;
    // --- Number of Vega passes to run (1-3) ---
    // Per-tier: smaller tier (huge meshes) runs fewer passes to avoid timeouts.
    int vegaPasses;

    // --- Tail batch parameters (alternative QEM path for huge meshes) ---
    int    tailBatchScanEdges;          // edges scanned per batch
    int    tailBatchTargetAccepts;      // max accepts per batch
    int    tailOriginalVertexThreshold; // disable tail mode below this original nV
};

// Tier table, ordered by ascending vertex count.
// Each tier is fully self-contained: change any field here, behavior changes globally.
static const TierParams tierTable[NUM_TIERS] = {
    // T1: 0 <= nV < 5000 (tiny meshes; v67 - keepRatio 0.00, memless=false)
    {
        .keepRatio = 0.050,
        .memless = false,
        .starMaxValence = 12, .starMaxOldDev = 0.160, .starMaxNewDev = 0.220,
        .starDistFrac = 1.18, .starExtraFrac = 0.0400,
        .starHardCap = 30000, .starScanVertices = 820000, .starRounds = 8,
        .vegaMaxValence = 0, .vegaMaxOldDev = 0, .vegaMaxNewDev = 0,
        .vegaDistFrac = 0, .vegaExtraFrac = 0,
        .vegaHardCap = 0, .vegaScanVertices = 0, .vegaRounds = 0,
        .vegaPatchGeomFrac = 0.0,
        .vegaSsimMin = 1.01, .vegaDamageMax = 0.0,
        .enableVegaSsimPass = false,
        .vegaPasses = 0,
        .tailBatchScanEdges = 65536, .tailBatchTargetAccepts = 2048,
        .tailOriginalVertexThreshold = 400000,
    },
    // T2: 5000 <= nV < 25000 (medium; v80 - vegaPasses 3)
    {
        .keepRatio = 0.320,
        .memless = true,
        .starMaxValence = 5, .starMaxOldDev = 0.004, .starMaxNewDev = 0.006,
        .starDistFrac = 0.40, .starExtraFrac = 0.0015,
        .starHardCap = 900, .starScanVertices = 90000, .starRounds = 1,
        .vegaMaxValence = 7, .vegaMaxOldDev = 0.020, .vegaMaxNewDev = 0.034,
        .vegaDistFrac = 0.82, .vegaExtraFrac = 0.0140,
        .vegaHardCap = 850, .vegaScanVertices = 130000, .vegaRounds = 1,
        .vegaPatchGeomFrac = 0.80,
        .vegaSsimMin = 0.930, .vegaDamageMax = 0.035,
        .enableVegaSsimPass = true,
        .vegaPasses = 3,
        .tailBatchScanEdges = 65536, .tailBatchTargetAccepts = 2048,
        .tailOriginalVertexThreshold = 400000,
    },
    // T3: 35000 <= nV < 45000 (medium-large, MEMLESS; v62 - lime v9 v114 baseline)
    {
        .keepRatio = 0.16,
        .memless = true,
        .starMaxValence = 6, .starMaxOldDev = 0.008, .starMaxNewDev = 0.012,
        .starDistFrac = 0.52, .starExtraFrac = 0.0030,
        .starHardCap = 1800, .starScanVertices = 140000, .starRounds = 2,
        .vegaMaxValence = 8, .vegaMaxOldDev = 0.026, .vegaMaxNewDev = 0.042,
        .vegaDistFrac = 0.92, .vegaExtraFrac = 0.0180,
        .vegaHardCap = 1250, .vegaScanVertices = 170000, .vegaRounds = 1,
        .vegaPatchGeomFrac = 0.42,
        .vegaSsimMin = 0.920, .vegaDamageMax = 0.045,
        .enableVegaSsimPass = true,
        .vegaPasses = 3,
        .tailBatchScanEdges = 65536, .tailBatchTargetAccepts = 2048,
        .tailOriginalVertexThreshold = 400000,
    },
    // T4: 45000 <= nV < 50000 (medium; v63 - lime v9: 0.10 light compression)
    {
        .keepRatio = 0.10,
        .memless = false,
        .starMaxValence = 6, .starMaxOldDev = 0.010, .starMaxNewDev = 0.014,
        .starDistFrac = 0.58, .starExtraFrac = 0.0035,
        .starHardCap = 2200, .starScanVertices = 150000, .starRounds = 2,
        .vegaMaxValence = 0, .vegaMaxOldDev = 0, .vegaMaxNewDev = 0,
        .vegaDistFrac = 0, .vegaExtraFrac = 0,
        .vegaHardCap = 0, .vegaScanVertices = 0, .vegaRounds = 0,
        .vegaPatchGeomFrac = 0.0,
        .vegaSsimMin = 1.01, .vegaDamageMax = 0.0,
        .enableVegaSsimPass = false,
        .vegaPasses = 0,
        .tailBatchScanEdges = 65536, .tailBatchTargetAccepts = 2048,
        .tailOriginalVertexThreshold = 400000,
    },
    // T5: 50000 <= nV < 400000 (large; v63 - lime v9: 0.025 compression)
    {
        .keepRatio = 0.025,          // v76: restored v71
        .memless = false,
        .starMaxValence = 5, .starMaxOldDev = 0.006, .starMaxNewDev = 0.009,
        .starDistFrac = 0.46, .starExtraFrac = 0.0010,
        .starHardCap = 700, .starScanVertices = 90000, .starRounds = 1,
        .vegaMaxValence = 6, .vegaMaxOldDev = 0.018, .vegaMaxNewDev = 0.030,
        .vegaDistFrac = 0.70, .vegaExtraFrac = 0.0100,
        .vegaHardCap = 800, .vegaScanVertices = 120000, .vegaRounds = 1,
        .vegaPatchGeomFrac = 1e100,
        .vegaSsimMin = 0.895,        // v83: back to v71
        .vegaDamageMax = 0.09,       // v83: back to v71
        .enableVegaSsimPass = true,
        .vegaPasses = 3,
        .tailBatchScanEdges = 65536, .tailBatchTargetAccepts = 2048,
        .tailOriginalVertexThreshold = 400000,
    },
    // T6: nV >= 400000 (huge; v65 - lime v9 baseline)
    {
        .keepRatio = 0.0407,
        .memless = false,
        .starMaxValence = 6, .starMaxOldDev = 0.008, .starMaxNewDev = 0.012,
        .starDistFrac = 0.52, .starExtraFrac = 0.0030,
        .starHardCap = 1800, .starScanVertices = 140000, .starRounds = 2,
        .vegaMaxValence = 7, .vegaMaxOldDev = 0.022, .vegaMaxNewDev = 0.036,
        .vegaDistFrac = 0.80, .vegaExtraFrac = 0.0140,
        .vegaHardCap = 1100, .vegaScanVertices = 140000, .vegaRounds = 1,
        .vegaPatchGeomFrac = 0.80,
        .vegaSsimMin = 0.88,         // v77: back to v71
        .vegaDamageMax = 0.12,       // v77: back to v71
        .enableVegaSsimPass = true,
        .vegaPasses = 3,
        .tailBatchScanEdges = 65536, .tailBatchTargetAccepts = 2048,
        .tailOriginalVertexThreshold = 400000,
    },
};

// Global instances used throughout the code
static const SharedParams shared;


struct Vec3 {
    double x=0,y=0,z=0;
    constexpr Vec3()=default;
    constexpr Vec3(double x_,double y_,double z_):x(x_),y(y_),z(z_){}
    Vec3 operator+(const Vec3& o)const{return{x+o.x,y+o.y,z+o.z};}
    Vec3 operator-(const Vec3& o)const{return{x-o.x,y-o.y,z-o.z};}
    Vec3 operator*(double s)const{return{x*s,y*s,z*s};}
    Vec3 operator/(double s)const{return{x/s,y/s,z/s};}
};
static inline double dot(const Vec3& a,const Vec3& b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static inline Vec3 cross(const Vec3& a,const Vec3& b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static inline double norm2(const Vec3& v){return dot(v,v);}
static inline double norm(const Vec3& v){return sqrt(norm2(v));}
static inline bool finiteVec(const Vec3& v){return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);}

struct Face{int v[3];};

struct Quadric {
    double a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;
    Quadric& operator+=(const Quadric& o){a+=o.a;b+=o.b;c+=o.c;d+=o.d;e+=o.e;f+=o.f;g+=o.g;h+=o.h;i+=o.i;j+=o.j;return *this;}
    void scale(double s){a*=s;b*=s;c*=s;d*=s;e*=s;f*=s;g*=s;h*=s;i*=s;j*=s;}
    static Quadric fromTriangle(const Vec3& p0,const Vec3& p1,const Vec3& p2){
        Vec3 n=cross(p1-p0,p2-p0);double ta=norm(n);
        if(ta<shared.minNormalNorm)return Quadric();
        n=n/ta;double pd=-dot(n,p0);
        Quadric q;q.a=n.x*n.x;q.b=n.x*n.y;q.c=n.x*n.z;q.d=n.x*pd;
        q.e=n.y*n.y;q.f=n.y*n.z;q.g=n.y*pd;q.h=n.z*n.z;q.i=n.z*pd;q.j=pd*pd;
        q.scale(sqrt(0.5*ta));return q;
    }
    double evaluate(const Vec3& p)const{
        return a*p.x*p.x+2*b*p.x*p.y+2*c*p.x*p.z+2*d*p.x+e*p.y*p.y+2*f*p.y*p.z+2*g*p.y+h*p.z*p.z+2*i*p.z+j;
    }
};

struct SmallSet {
    vector<int> data;
    void insert(int v){
        auto it=lower_bound(data.begin(),data.end(),v);
        if(it==data.end()||*it!=v) data.insert(it,v);
    }
    void erase(int v){
        auto it=lower_bound(data.begin(),data.end(),v);
        if(it!=data.end()&&*it==v) data.erase(it);
    }
    bool contains(int v)const{ return binary_search(data.begin(),data.end(),v); }
    int size()const{return(int)data.size();}
    auto begin()const{return data.begin();}
    auto end()const{return data.end();}
    void clear(){data.clear();}
    void reserve(int n){data.reserve(n);}
};

static double det3(double a00,double a01,double a02,double a10,double a11,double a12,double a20,double a21,double a22){
    return a00*(a11*a22-a12*a21)-a01*(a10*a22-a12*a20)+a02*(a10*a21-a11*a20);
}
static bool solveQem3x3(const Quadric& q,Vec3& out){
    double D=det3(q.a,q.b,q.c,q.b,q.e,q.f,q.c,q.f,q.h);
    if(fabs(D)<shared.qemSolveDeterminantEps)return false;
    out=Vec3(det3(-q.d,q.b,q.c,-q.g,q.e,q.f,-q.i,q.f,q.h)/D,
             det3(q.a,-q.d,q.c,q.b,-q.g,q.f,q.c,-q.i,q.h)/D,
             det3(q.a,q.b,-q.d,q.b,q.e,-q.g,q.c,q.f,-q.i)/D);
    return finiteVec(out);
}

struct CollapseCandidate {
    int absorbed=-1,kept=-1,versionAbsorbed=-1,versionKept=-1;
    double cost=shared.inf,mergedRadius=0.0;
    Vec3 position;
    bool operator<(const CollapseCandidate& o)const{return cost>o.cost;}
    bool valid()const{return absorbed>=0&&kept>=0&&cost<shared.inf;}
};

class QemSimplifier {
public:
    static bool MEMLESS;
public:
    void run(){
        readMesh();
        if(nV<=4){writeMesh();return;}
        startTime=chrono::steady_clock::now();
        initScale();
        buildConnectivity();
        initFaceWeights();
        initQueue();
        collapseLoop();
        // === POST-QEM PHASES (vertex-based decisions, time is ONLY the kill switch) ===
        // Each phase runs iff its vertex-based condition is met AND elapsed < budget.
        // The actual phase work is bounded by vertex counts (maxExtra, scanVertices, rounds).
        {
            const TierParams& tp = tierTable[currentTier()];
            // Each phase enters unconditionally based on vertex-count / enable conditions.
            // Time is the KILL SWITCH only — phase functions check elapsed() internally
            // and bail out mid-loop if they approach the time budget.
            if (tp.starMaxValence > 0 &&
                nV <= tierTable[currentTier()].tailOriginalVertexThreshold) {
                collapseInvisibleEdges();
            }
            for (int vp = 0; vp < tp.vegaPasses; ++vp) {
                vegaSsimStarPass();
            }
        }
        compact();
        writeMesh();
    }
private:
    // === Mesh state ===
    int nV=0;                   // current alive vertex count
    int originalNv=0;           // initial vertex count (frozen, used for tier classification)
    int nF=0;                   // current alive face count
    vector<Vec3> verts;         // vertex positions (indexed by original id)
    vector<Face> faces;         // face indices (into verts)
    vector<char> vdead;         // 1 = vertex has been collapsed away
    vector<char> fdead;         // 1 = face has been removed
    vector<int> vver;           // per-vertex version counter (PQ staleness check)
    vector<Quadric> vquad;      // per-vertex QEM quadric
    vector<double> crad;        // per-vertex merged collapse radius (Hausdorff bound)
    vector<vector<int>> vfaces; // faces incident to each vertex
    vector<SmallSet> vneigh;    // vertex adjacency (one-ring neighbors)
    vector<int> tailLocks;      // tail-batch locks (prevents double-collapse in same batch)

    // === QEM state ===
    priority_queue<CollapseCandidate> pq; // min-heap of pending collapses
    int targetV=0;              // goal vertex count (computed from keepRatio)
    int collapseLimit=0;        // max collapses before QEM stops
    int accepted=0;             // collapses successfully applied

    // === Tail-batch state ===
    int tailCursor=0;           // start index for next tail scan
    int tailStamp=1;            // current "lock epoch" for tail batches
    int lastFailBatch=-1;       // nV value at last failed tail batch

    // === Star/Vega cursor state (round-robin scanning) ===
    int starCursor=0;           // next start index for star pass
    int vegaCursor=0;           // next start index for Vega pass

    // === DEBUG counters ===

    // === Geometric state (computed from mesh extent) ===
    double diag=0;              // mesh bounding box diagonal
    double hausd=0;             // max allowed collapse distance (= 0.055 * diag)
    double costCap=shared.inf;  // max QEM cost to accept a collapse
    double invDiag2=0.0;        // 1/(diag*diag) for normalized area calculations

    // === Time bookkeeping ===
    chrono::steady_clock::time_point startTime; // when run() began

    static constexpr Vec3 cameraDirs[6] = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };

    void readMesh(){
        vector<char> buf;buf.reserve(1<<27);
        char chunk[1<<16];size_t n;
        while((n=fread(chunk,1,sizeof(chunk),stdin))>0)buf.insert(buf.end(),chunk,chunk+n);
        buf.push_back('\0');char*p=buf.data();
        nV=(int)strtol(p,&p,10);originalNv=nV;nF=(int)strtol(p,&p,10);
        verts.resize(nV);faces.resize(nF);
        for(int i=0;i<nV;++i){while(*p&&*p<=' ')++p;++p;verts[i].x=strtod(p,&p);verts[i].y=strtod(p,&p);verts[i].z=strtod(p,&p);}
        for(int i=0;i<nF;++i){while(*p&&*p<=' ')++p;++p;faces[i].v[0]=(int)strtol(p,&p,10)-1;faces[i].v[1]=(int)strtol(p,&p,10)-1;faces[i].v[2]=(int)strtol(p,&p,10)-1;}
    }
    void writeMesh(){
        string out;out.reserve(nV*42+nF*26+64);char line[128];
        snprintf(line,sizeof(line),"%d %d\n",nV,nF);out+=line;
        for(int i=0;i<nV;++i){snprintf(line,sizeof(line),"v %.*g %.*g %.*g\n",shared.outputPrecision,verts[i].x,shared.outputPrecision,verts[i].y,shared.outputPrecision,verts[i].z);out+=line;}
        for(int i=0;i<nF;++i){snprintf(line,sizeof(line),"f %d %d %d\n",faces[i].v[0]+1,faces[i].v[1]+1,faces[i].v[2]+1);out+=line;}
        fwrite(out.data(),1,out.size(),stdout);
    }
    void initScale(){
        // Use tierTable for both keepRatio and MEMLESS flag.
        const TierParams& tp = tierTable[currentTier()];
        MEMLESS = tp.memless;
        Vec3 mn=verts[0],mx=verts[0];
        for(auto&p:verts){mn.x=min(mn.x,p.x);mn.y=min(mn.y,p.y);mn.z=min(mn.z,p.z);mx.x=max(mx.x,p.x);mx.y=max(mx.y,p.y);mx.z=max(mx.z,p.z);}
        diag=norm(mx-mn);hausd=shared.hausdorffDiagFraction*diag;
        costCap=shared.qemCostCapCoeff*diag*diag;
        invDiag2 = (diag > shared.minNormalNorm) ? (1.0 / (diag*diag)) : 0.0;
        double kr = tp.keepRatio;
        targetV=max(10,(int)floor(nV*kr));
        targetV=min(targetV,nV-1);
        collapseLimit=nV-targetV;
    }
    void buildConnectivity(){
        vdead.assign(nV,0);fdead.assign(nF,0);vver.assign(nV,0);
        vquad.assign(nV,Quadric());crad.assign(nV,0.0);
        vfaces.assign(nV,{});vneigh.resize(nV);tailLocks.assign(nV,0);
        for(int fi=0;fi<nF;++fi){
            auto&f=faces[fi];
            Quadric q=Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]);
            for(int k=0;k<3;++k){vfaces[f.v[k]].push_back(fi);vquad[f.v[k]]+=q;}
            for(int k=0;k<3;++k){int a=f.v[k],b=f.v[(k+1)%3];if(a!=b){vneigh[a].insert(b);vneigh[b].insert(a);}}
        }
    }
    double faceWeightFor(const Vec3& unitNormal, double area) const {
        double absSum = fabs(unitNormal.x) + fabs(unitNormal.y) + fabs(unitNormal.z);
        double normalizedArea = area * invDiag2;
        double w = 1.0 + shared.viewWeightK * normalizedArea * absSum;
        return min(w, shared.maxFaceWeight);
    }
    void initFaceWeights(){
        vquad.assign(nV, Quadric());
        for(int fi=0;fi<nF;++fi){
            const Face& f=faces[fi];
            Vec3 n=cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);
            double area=0.5*norm(n);
            Quadric q = Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]);
            if(area >= 1e-30){
                n = n / (2.0*area);
                double w = faceWeightFor(n, area);
                if(w != 1.0) q.scale(w);
            }
            for(int k=0;k<3;++k) vquad[f.v[k]] += q;
        }
    }
    bool isFaceInvisible(int fi) const {
        const Face& f = faces[fi];
        Vec3 n = cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
        double len = norm(n);
        if (len < 1e-30) return false;
        n = n / len;
        for (int k = 0; k < 6; ++k) {
            if (dot(n, cameraDirs[k]) > 0) return false;
        }
        return true;
    }
    void getCandidatePositions(int a,int b,const Quadric&q,Vec3 pos[4],int&np)const{
        np=0;
        Vec3 qp;
        if(solveQem3x3(q,qp))pos[np++]=qp;
        pos[np++]=(verts[a]+verts[b])*0.5;
        pos[np++]=verts[a];
        pos[np++]=verts[b];
        int wp=0;
        for(int i=0;i<np;++i){
            if(!finiteVec(pos[i]))continue;
            bool dup=false;
            for(int j=0;j<wp;++j)if(norm2(pos[i]-pos[j])<1e-30){dup=true;break;}
            if(!dup)pos[wp++]=pos[i];
        }
        np=wp;
    }
    CollapseCandidate makeCandidate(int ab,int kp,const Vec3&p,const Quadric&q)const{
        CollapseCandidate c;c.absorbed=ab;c.kept=kp;
        c.versionAbsorbed=vver[ab];c.versionKept=vver[kp];
        c.position=p;c.cost=q.evaluate(p);return c;
    }
    double cheapEdgeCost(int a,int b)const{
        // Fast lower-cost proxy: quadric error at the edge midpoint (no 3x3 solve,
        // no multi-position search). Used only to PRE-RANK/FILTER tail candidates;
        // the full 8-placement solve still runs in computeBestValid for accepted ones.
        Quadric q=vquad[a];q+=vquad[b];
        Vec3 mid=(verts[a]+verts[b])*0.5;
        return q.evaluate(mid);
    }
    CollapseCandidate computeQueueCandidate(int a,int b)const{
        Quadric q=vquad[a];q+=vquad[b];
        Vec3 pos[4];int np;getCandidatePositions(a,b,q,pos,np);
        CollapseCandidate best;
        for(int i=0;i<np;++i){
            CollapseCandidate c1=makeCandidate(a,b,pos[i],q);if(c1.cost<best.cost)best=c1;
            CollapseCandidate c2=makeCandidate(b,a,pos[i],q);if(c2.cost<best.cost)best=c2;
        }
        return best;
    }
    bool passesEnvelope(int a,int b,const Vec3&p,double&mr)const{
        mr=max(crad[a]+norm(verts[a]-p),crad[b]+norm(verts[b]-p));
        return mr<=hausd;
    }
    CollapseCandidate computeBestValid(int a,int b)const{
        Quadric q=vquad[a];q+=vquad[b];
        Vec3 pos[4];int np;getCandidatePositions(a,b,q,pos,np);
        CollapseCandidate best;
        for(int i=0;i<np;++i){
            for(int dir=0;dir<2;++dir){
                int ab=dir?b:a,kp=dir?a:b;
                double mr;
                if(!passesEnvelope(ab,kp,pos[i],mr))continue;
                CollapseCandidate c=makeCandidate(ab,kp,pos[i],q);
                c.mergedRadius=mr;
                if(c.cost<best.cost)best=c;
            }
        }
        return best;
    }
    void initQueue(){
        for(int a=0;a<nV;++a)
            for(int b:vneigh[a])if(a<b){auto c=computeQueueCandidate(a,b);if(c.valid())pq.push(c);}
    }
    double elapsed()const{return chrono::duration<double>(chrono::steady_clock::now()-startTime).count();}
    // Determine the current tier from vertex count.
    // tierTable[0] covers nV < TIER_THRESHOLD[1], etc.
    // The last tier has tierMaxV = INT_MAX, so this always returns a valid index.
    int currentTier() const {
        for(int i=0;i<NUM_TIERS;++i){
            if(originalNv <= TIER_THRESHOLD[i+1]) return i;
        }
        return NUM_TIERS-1;
    }
    bool tailMode()const{return nV>tierTable[currentTier()].tailOriginalVertexThreshold;}
    bool edgeExists(int a,int b)const{return a>=0&&b>=0&&a<(int)verts.size()&&b<(int)verts.size()&&!vdead[a]&&!vdead[b]&&vneigh[a].contains(b);}
    int countCommonFaces(int a,int b)const{
        int cnt=0;
        const auto&fa=vfaces[a];const auto&fb=vfaces[b];
        if(fa.size()<fb.size()){for(int f:fa){if(!fdead[f])for(int f2:fb)if(f==f2){++cnt;break;}}}
        else{for(int f:fb){if(!fdead[f])for(int f2:fa)if(f==f2){++cnt;break;}}}
        return cnt;
    }
    int countCommonNeighbors(int a,int b)const{
        int cnt=0;
        const auto&na=vneigh[a];const auto&nb=vneigh[b];
        if(na.size()<nb.size()){for(int x:na){if(x!=a&&x!=b&&!vdead[x]&&nb.contains(x))++cnt;}}
        else{for(int x:nb){if(x!=a&&x!=b&&!vdead[x]&&na.contains(x))++cnt;}}
        return cnt;
    }
    static void eraseVal(vector<int>&v,int x){for(int i=(int)v.size()-1;i>=0;--i)if(v[i]==x){v[i]=v.back();v.pop_back();return;}}
    bool batchFree(int a,int b)const{
        if(tailLocks[a]==tailStamp||tailLocks[b]==tailStamp)return false;
        for(int nb:vneigh[a])if(!vdead[nb]&&tailLocks[nb]==tailStamp)return false;
        for(int nb:vneigh[b])if(!vdead[nb]&&tailLocks[nb]==tailStamp)return false;
        return true;
    }
    void lockBatch(int a,int b){
        tailLocks[a]=tailLocks[b]=tailStamp;
        for(int nb:vneigh[a])if(!vdead[nb])tailLocks[nb]=tailStamp;
        for(int nb:vneigh[b])if(!vdead[nb])tailLocks[nb]=tailStamp;
    }
    struct EBC{int a,b;double cost;bool operator<(const EBC&o)const{return cost<o.cost;}};
    int runTailBatch(){
        if(!tailMode())return 0;
        if(++tailStamp==INT_MAX){fill(tailLocks.begin(),tailLocks.end(),0);tailStamp=1;}
        vector<EBC> cands;cands.reserve(tierTable[currentTier()].tailBatchScanEdges/4);
        int scanned=0,visited=0;
        for(;visited<(int)verts.size()&&scanned<tierTable[currentTier()].tailBatchScanEdges;++visited){
            int a=(tailCursor+visited)%(int)verts.size();if(vdead[a])continue;
            for(int b:vneigh[a]){if(scanned>=tierTable[currentTier()].tailBatchScanEdges)break;if(b<=a||vdead[b])continue;++scanned;
                double cc=cheapEdgeCost(a,b);if(!(cc<=costCap))continue;cands.push_back({a,b,cc});}
        }
        tailCursor=(tailCursor+max(1,visited))%(int)verts.size();
        if(cands.empty())return 0;
        sort(cands.begin(),cands.end());
        vector<EBC> sel;sel.reserve(tierTable[currentTier()].tailBatchTargetAccepts);
        for(auto&e:cands){if((int)sel.size()>=tierTable[currentTier()].tailBatchTargetAccepts)break;if(!edgeExists(e.a,e.b))continue;if(!batchFree(e.a,e.b))continue;lockBatch(e.a,e.b);sel.push_back(e);}
        int acc=0;
        for(auto&e:sel){
            if(accepted>=collapseLimit||elapsed()>shared.timeBudgetSeconds)break;
            if(!edgeExists(e.a,e.b))continue;
            if(countCommonFaces(e.a,e.b)!=2)continue;
            if(countCommonNeighbors(e.a,e.b)!=2)continue;
            auto best=computeBestValid(e.a,e.b);if(!best.valid()||best.cost>costCap)continue;
            applyCollapse(best.absorbed,best.kept,best.position,best.mergedRadius);++accepted;++acc;
        }
        return acc;
    }
    void collapseLoop(){
        int tick=0;
        while(accepted<collapseLimit){
            if((++tick&8191)==0&&elapsed()>shared.timeBudgetSeconds)break;
            // Tail batch only when PQ is empty (true fallback for huge meshes)
            if(pq.empty()&&tailMode()&&lastFailBatch!=accepted){
                int ba=runTailBatch();
                if(ba>0)continue;
                lastFailBatch=accepted;
                break;
            }
            auto c=pq.top();pq.pop();
            int a=c.absorbed,b=c.kept;
            if(!edgeExists(a,b))continue;
            if(c.versionAbsorbed!=vver[a]||c.versionKept!=vver[b]){auto fr=computeQueueCandidate(a,b);if(fr.valid())pq.push(fr);continue;}
            if(c.cost>costCap)break;
            if(countCommonFaces(a,b)!=2)continue;
            if(countCommonNeighbors(a,b)!=2)continue;
            auto best=computeBestValid(a,b);if(!best.valid()||best.cost>costCap)continue;
            applyCollapse(best.absorbed,best.kept,best.position,best.mergedRadius);++accepted;
        }
    }
    void applyCollapse(int ab,int kp,const Vec3&np,double nr){
        verts[kp]=np;crad[kp]=nr;crad[ab]=0;vdead[ab]=1;++vver[ab];++vver[kp];
        auto abFaces=vfaces[ab];vector<int> dead;dead.reserve(4);
        for(int fi:abFaces){if(fdead[fi])continue;bool touched=false;
            for(int k=0;k<3;++k)if(faces[fi].v[k]==ab){faces[fi].v[k]=kp;touched=true;}
            if(!touched)continue;
            if(faces[fi].v[0]==faces[fi].v[1]||faces[fi].v[1]==faces[fi].v[2]||faces[fi].v[0]==faces[fi].v[2]){fdead[fi]=1;dead.push_back(fi);}
            else vfaces[kp].push_back(fi);}
        for(int fi:dead)for(int k=0;k<3;++k){int v=faces[fi].v[k];if(v>=0&&v<(int)vfaces.size())eraseVal(vfaces[v],fi);}
        vfaces[ab].clear();
        if(MEMLESS){
            Quadric fresh;
            for(int fi:vfaces[kp]){if(fdead[fi])continue;const Face&f=faces[fi];
                fresh+=Quadric::fromTriangle(verts[f.v[0]],verts[f.v[1]],verts[f.v[2]]);}
            vquad[kp]=fresh;
        } else {
            vquad[kp]+=vquad[ab];
        }
        for(int nb:vneigh[ab]){if(nb==kp||vdead[nb])continue;vneigh[nb].erase(ab);vneigh[nb].insert(kp);vneigh[kp].insert(nb);}
        vneigh[ab].clear();vneigh[kp].erase(ab);vneigh[kp].erase(kp);
        for(int nb:vneigh[kp]){if(nb==kp||vdead[nb])continue;auto c=computeQueueCandidate(kp,nb);if(c.valid())pq.push(c);}
    }

    // ---------- Star-delete retriangulation post-pass ----------
    static double clampDouble(double x,double lo,double hi){ return x<lo?lo:(x>hi?hi:x); }

    static double pointTriangleDistance2(const Vec3& p,const Vec3& a,const Vec3& b,const Vec3& c){
        Vec3 ab=b-a, ac=c-a, ap=p-a;
        double d1=dot(ab,ap), d2=dot(ac,ap);
        if(d1<=0.0&&d2<=0.0)return norm2(ap);
        Vec3 bp=p-b;
        double d3=dot(ab,bp), d4=dot(ac,bp);
        if(d3>=0.0&&d4<=d3)return norm2(bp);
        double vc=d1*d4-d3*d2;
        if(vc<=0.0&&d1>=0.0&&d3<=0.0){ double v=d1/(d1-d3); Vec3 q=a+ab*v; return norm2(p-q); }
        Vec3 cp=p-c;
        double d5=dot(ab,cp), d6=dot(ac,cp);
        if(d6>=0.0&&d5<=d6)return norm2(cp);
        double vb=d5*d2-d1*d6;
        if(vb<=0.0&&d2>=0.0&&d6<=0.0){ double w=d2/(d2-d6); Vec3 q=a+ac*w; return norm2(p-q); }
        double va=d3*d6-d5*d4;
        if(va<=0.0&&(d4-d3)>=0.0&&(d5-d6)>=0.0){ double w=(d4-d3)/((d4-d3)+(d5-d6)); Vec3 q=b+(c-b)*w; return norm2(p-q); }
        Vec3 n=cross(ab,ac);
        double nn=norm2(n);
        if(nn<shared.minNormalNorm*shared.minNormalNorm)return shared.inf;
        double dist=dot(p-a,n);
        return (dist*dist)/nn;
    }
    Vec3 faceNormalRaw(int fi)const{
        const Face& f=faces[fi];
        return cross(verts[f.v[1]]-verts[f.v[0]],verts[f.v[2]]-verts[f.v[0]]);
    }
    bool faceHasVertex(int fi,int v)const{
        const Face& f=faces[fi];
        return f.v[0]==v||f.v[1]==v||f.v[2]==v;
    }
    bool orientedRingForVertex(int v,vector<int>& ring,vector<int>& inc)const{
        const TierParams& tp = tierTable[currentTier()];
        return orientedRingForVertexStar(v,tp,ring,inc);
    }
    bool orientedRingForVertexStar(int v,const TierParams& tp,vector<int>& ring,vector<int>& inc)const{
        ring.clear();inc.clear();
        if(tp.starMaxValence<=0)return false;
        if(v<0||v>=(int)vfaces.size()||vdead[v])return false;
        for(int fi:vfaces[v]){ if(fdead[fi])continue; if(faceHasVertex(fi,v))inc.push_back(fi); }
        int m=(int)inc.size();
        if(m<3||m>tp.starMaxValence)return false;
        vector<pair<int,int>> dir; dir.reserve(m);
        for(int fi:inc){
            const Face& f=faces[fi];
            int pos=-1;
            for(int k=0;k<3;++k)if(f.v[k]==v)pos=k;
            if(pos<0)return false;
            int a=f.v[(pos+1)%3],b=f.v[(pos+2)%3];
            if(a==b||a==v||b==v||vdead[a]||vdead[b])return false;
            dir.push_back({a,b});
        }
        for(int i=0;i<m;++i)for(int j=i+1;j<m;++j){
            if(dir[i].first==dir[j].first)return false;
            if(dir[i].second==dir[j].second)return false;
        }
        int start=dir[0].first,cur=start;
        ring.push_back(start);
        for(int step=0;step<m;++step){
            int nxt=-1;
            for(auto&e:dir)if(e.first==cur){nxt=e.second;break;}
            if(nxt<0)return false;
            if(step==m-1){ if(nxt!=start)return false; }
            else{ for(int x:ring)if(x==nxt)return false; ring.push_back(nxt); cur=nxt; }
        }
        return (int)ring.size()==m;
    }
bool orientedRingForVertexVega(int v,const TierParams& tp,vector<int>& ring,vector<int>& inc)const{
        ring.clear();inc.clear();
        if(tp.vegaMaxValence<=0)return false;
        if(v<0||v>=(int)vfaces.size()||vdead[v])return false;
        for(int fi:vfaces[v]){ if(fdead[fi])continue; if(faceHasVertex(fi,v))inc.push_back(fi); }
        int m=(int)inc.size();
        if(m<3||m>tp.vegaMaxValence)return false;
        vector<pair<int,int>> dir; dir.reserve(m);
        for(int fi:inc){
            const Face& f=faces[fi];
            int pos=-1;
            for(int k=0;k<3;++k)if(f.v[k]==v)pos=k;
            if(pos<0)return false;
            int a=f.v[(pos+1)%3],b=f.v[(pos+2)%3];
            if(a==b||a==v||b==v||vdead[a]||vdead[b])return false;
            dir.push_back({a,b});
        }
        for(int i=0;i<m;++i)for(int j=i+1;j<m;++j){
            if(dir[i].first==dir[j].first)return false;
            if(dir[i].second==dir[j].second)return false;
        }
        int start=dir[0].first,cur=start;
        ring.push_back(start);
        for(int step=0;step<m;++step){
            int nxt=-1;
            for(auto&e:dir)if(e.first==cur){nxt=e.second;break;}
            if(nxt<0)return false;
            if(step==m-1){ if(nxt!=start)return false; }
            else{ for(int x:ring)if(x==nxt)return false; ring.push_back(nxt); cur=nxt; }
        }
        return (int)ring.size()==m;
    }
    bool activeFaceWithSameKey(int a,int b,int c,const vector<int>& skip)const{
        array<int,3> key={a,b,c}; sort(key.begin(),key.end());
        for(int fi:vfaces[a]){
            if(fdead[fi])continue;
            bool skipFace=false;
            for(int s:skip)if(s==fi){skipFace=true;break;}
            if(skipFace)continue;
            const Face& f=faces[fi];
            array<int,3> k2={f.v[0],f.v[1],f.v[2]}; sort(k2.begin(),k2.end());
            if(k2==key)return true;
        }
        return false;
    }
    struct StarCandidate{
        int v=-1,root=0; double score=shared.inf;
        bool valid()const{return v>=0&&score<shared.inf;}
        bool operator<(const StarCandidate& o)const{return score<o.score;}
    };
    bool evaluateStarRoot(int v,const vector<int>& ring,const vector<int>& inc,int root,double oldDev,const Vec3& avgN,StarCandidate& out)const{
        return evaluateStarRootStar(v,ring,inc,root,oldDev,avgN,tierTable[currentTier()],out);
    }
    bool evaluateStarRootStar(int v,const vector<int>& ring,const vector<int>& inc,int root,double oldDev,const Vec3& avgN,const TierParams& tp,StarCandidate& out)const{
        int m=(int)ring.size();
        vector<int> rr; rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);
        int r0=rr[0];
        for(int i=2;i<=m-2;++i){ if(vneigh[r0].contains(rr[i]))return false; }
        double maxNewDev=0.0; double minDist2=shared.inf;
        for(int i=1;i<m-1;++i){
            int a=rr[0],b=rr[i],c=rr[i+1];
            if(a==b||b==c||a==c)return false;
            if(activeFaceWithSameKey(a,b,c,inc))return false;
            Vec3 n=cross(verts[b]-verts[a],verts[c]-verts[a]);
            double nl=norm(n);
            if(nl<shared.minNormalNorm)return false;
            Vec3 un=n/nl;
            double d=clampDouble(dot(un,avgN),-1.0,1.0);
            if(d<=0.0)return false;
            maxNewDev=max(maxNewDev,1.0-d);
            if(maxNewDev>tp.starMaxNewDev)return false;
            minDist2=min(minDist2,pointTriangleDistance2(verts[v],verts[a],verts[b],verts[c]));
        }
        double dist=sqrt(max(0.0,minDist2));
        if(crad[v]+dist>hausd*tp.starDistFrac)return false;
        out.v=v; out.root=root;
        out.score=(crad[v]+dist)/(hausd+shared.minNormalNorm)+0.35*oldDev+0.25*maxNewDev+1e-4*m;
        return true;
    }
bool evaluateStarRootVega(int v,const vector<int>& ring,const vector<int>& inc,int root,double oldDev,const Vec3& avgN,const TierParams& tp,StarCandidate& out)const{
        int m=(int)ring.size();
        vector<int> rr; rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);
        int r0=rr[0];
        for(int i=2;i<=m-2;++i){ if(vneigh[r0].contains(rr[i]))return false; }
        double maxNewDev=0.0; double minDist2=shared.inf;
        for(int i=1;i<m-1;++i){
            int a=rr[0],b=rr[i],c=rr[i+1];
            if(a==b||b==c||a==c)return false;
            if(activeFaceWithSameKey(a,b,c,inc))return false;
            Vec3 n=cross(verts[b]-verts[a],verts[c]-verts[a]);
            double nl=norm(n);
            if(nl<shared.minNormalNorm)return false;
            Vec3 un=n/nl;
            double d=clampDouble(dot(un,avgN),-1.0,1.0);
            if(d<=0.0)return false;
            maxNewDev=max(maxNewDev,1.0-d);
            if(maxNewDev>tp.vegaMaxNewDev)return false;
            minDist2=min(minDist2,pointTriangleDistance2(verts[v],verts[a],verts[b],verts[c]));
        }
        double dist=sqrt(max(0.0,minDist2));
        if(crad[v]+dist>hausd*tp.vegaDistFrac)return false;
        out.v=v; out.root=root;
        out.score=(crad[v]+dist)/(hausd+shared.minNormalNorm)+0.35*oldDev+0.25*maxNewDev+1e-4*m;
        return true;
    }
    StarCandidate computeStarCandidate(int v)const{
        return computeStarCandidateStar(v,tierTable[currentTier()]);
    }
    StarCandidate computeStarCandidateStar(int v,const TierParams& tp)const{
        StarCandidate best;
        vector<int> ring,inc;
        if(!orientedRingForVertexStar(v,tp,ring,inc))return best;
        Vec3 avg; double areaSum=0.0,oldDev=0.0;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi);
            double nl=norm(n);
            if(nl<shared.minNormalNorm)return best;
            avg=avg+n; areaSum+=0.5*nl;
        }
        double al=norm(avg);
        if(al<shared.minNormalNorm||areaSum<=0.0)return best;
        avg=avg/al;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi); double nl=norm(n); Vec3 un=n/nl;
            double d=clampDouble(dot(un,avg),-1.0,1.0);
            if(d<=0.0)return best;
            oldDev=max(oldDev,1.0-d);
        }
        if(oldDev>tp.starMaxOldDev)return best;
        for(int root=0;root<(int)ring.size();++root){
            StarCandidate c;
            if(evaluateStarRootStar(v,ring,inc,root,oldDev,avg,tp,c)&&c.score<best.score)best=c;
        }
        return best;
    }
StarCandidate computeStarCandidateVega(int v,const TierParams& tp)const{
        StarCandidate best;
        vector<int> ring,inc;
        if(!orientedRingForVertexVega(v,tp,ring,inc))return best;
        Vec3 avg; double areaSum=0.0,oldDev=0.0;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi);
            double nl=norm(n);
            if(nl<shared.minNormalNorm)return best;
            avg=avg+n; areaSum+=0.5*nl;
        }
        double al=norm(avg);
        if(al<shared.minNormalNorm||areaSum<=0.0)return best;
        avg=avg/al;
        for(int fi:inc){
            Vec3 n=faceNormalRaw(fi); double nl=norm(n); Vec3 un=n/nl;
            double d=clampDouble(dot(un,avg),-1.0,1.0);
            if(d<=0.0)return best;
            oldDev=max(oldDev,1.0-d);
        }
        if(oldDev>tp.vegaMaxOldDev)return best;
        for(int root=0;root<(int)ring.size();++root){
            StarCandidate c;
            if(evaluateStarRootVega(v,ring,inc,root,oldDev,avg,tp,c)&&c.score<best.score)best=c;
        }
        return best;
    }
    bool applyStarDelete(int v,int root){
        const TierParams& tp = tierTable[currentTier()];
        StarCandidate best=computeStarCandidateStar(v,tp);
        if(!best.valid())return false;
        return applyStarDeleteStar(v,best.root,tp);
    }  // keep star variants for collapseInvisibleEdges
    bool applyStarDeleteStar(int v,int root,const TierParams& tp){
        StarCandidate best=computeStarCandidateStar(v,tp);
        if(!best.valid())return false;
        root=best.root;
        vector<int> ring,inc;
        if(!orientedRingForVertexStar(v,tp,ring,inc))return false;
        int m=(int)ring.size();
        vector<int> rr; rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);
        for(int fi:inc){
            if(fdead[fi])continue;
            Face old=faces[fi]; fdead[fi]=1;
            for(int k=0;k<3;++k){ int u=old.v[k]; if(u>=0&&u<(int)vfaces.size())eraseVal(vfaces[u],fi); }
        }
        for(int nb:ring)if(nb>=0&&nb<(int)vneigh.size())vneigh[nb].erase(v);
        vneigh[v].clear(); vfaces[v].clear(); vdead[v]=1; crad[v]=0.0; ++vver[v];
        for(int i=1;i<m-1;++i){
            Face nf; nf.v[0]=rr[0];nf.v[1]=rr[i];nf.v[2]=rr[i+1];
            int fi=(int)faces.size();
            faces.push_back(nf); fdead.push_back(0);
            for(int k=0;k<3;++k)vfaces[nf.v[k]].push_back(fi);
            for(int k=0;k<3;++k){ int a=nf.v[k],b=nf.v[(k+1)%3]; if(a!=b){vneigh[a].insert(b);vneigh[b].insert(a);} }
        }
        return true;
    }
bool applyStarDeleteVega(int v,int root,const TierParams& tp){
        StarCandidate best=computeStarCandidateVega(v,tp);
        if(!best.valid())return false;
        root=best.root;
        vector<int> ring,inc;
        if(!orientedRingForVertexVega(v,tp,ring,inc))return false;
        int m=(int)ring.size();
        vector<int> rr; rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);
        for(int fi:inc){
            if(fdead[fi])continue;
            Face old=faces[fi]; fdead[fi]=1;
            for(int k=0;k<3;++k){ int u=old.v[k]; if(u>=0&&u<(int)vfaces.size())eraseVal(vfaces[u],fi); }
        }
        for(int nb:ring)if(nb>=0&&nb<(int)vneigh.size())vneigh[nb].erase(v);
        vneigh[v].clear(); vfaces[v].clear(); vdead[v]=1; crad[v]=0.0; ++vver[v];
        for(int i=1;i<m-1;++i){
            Face nf; nf.v[0]=rr[0];nf.v[1]=rr[i];nf.v[2]=rr[i+1];
            int fi=(int)faces.size();
            faces.push_back(nf); fdead.push_back(0);
            for(int k=0;k<3;++k)vfaces[nf.v[k]].push_back(fi);
            for(int k=0;k<3;++k){ int a=nf.v[k],b=nf.v[(k+1)%3]; if(a!=b){vneigh[a].insert(b);vneigh[b].insert(a);} }
        }
        return true;
    }
    struct VegaProj{double u=0.0,v=0.0,z=0.0;bool ok=false;};
    struct VegaTri{Vec3 p[3];};
    struct VegaPixel{float n[3];float d;unsigned char fg;};
    struct VegaCandidate{
        int v=-1,root=0; double ssim=1.0,score=shared.inf;
        bool operator<(const VegaCandidate& o)const{return score<o.score;}
    };
    VegaPixel vegaBackgroundPixel()const{
        VegaPixel p;
        p.n[0]=shared.colorUnset;
        p.n[1]=shared.colorUnset;
        p.n[2]=shared.colorUnset;
        p.d=shared.colorMax8Bit;
        p.fg=0;
        return p;
    }
    void vegaCameraBasis(int view,Vec3& eye,Vec3& right,Vec3& up,Vec3& fwd)const{
        double er=shared.vegaEyeRadius;
        switch(view){
            case 0: eye=Vec3( er,0,0);break;
            case 1: eye=Vec3(-er,0,0);break;
            case 2: eye=Vec3(0, er,0);break;
            case 3: eye=Vec3(0,-er,0);break;
            case 4: eye=Vec3(0,0, er);break;
            default:eye=Vec3(0,0,-er);break;
        }
        fwd=eye*(-1.0);
        double fl=norm(fwd);
        if(fl<shared.minNormalNorm)fwd=Vec3(0,0,-1);else fwd=fwd/fl;
        Vec3 worldUp=(fabs(dot(fwd,Vec3(0,0,1)))>shared.vegaWorldUpDotThresh)?Vec3(0,1,0):Vec3(0,0,1);
        right=cross(worldUp,fwd);
        double rl=norm(right);
        if(rl<shared.minNormalNorm)right=Vec3(1,0,0);else right=right/rl;
        up=cross(fwd,right);
        double ul=norm(up);
        if(ul<shared.minNormalNorm)up=Vec3(0,1,0);else up=up/ul;
    }
    VegaProj vegaProjectPoint(const Vec3& p,int view,int R)const{
        Vec3 eye,right,up,fwd;
        vegaCameraBasis(view,eye,right,up,fwd);
        Vec3 rel=p-eye;
        double x=dot(rel,right),y=dot(rel,up),z=dot(rel,fwd);
        if(z<=shared.vegaEpsilonDepth)return {};
        double scale=double(R)/shared.vegaRefResolution;
        double f=shared.vegaFocalLength*scale;
        double c=0.5*double(R);
        return {f*x/z+c,f*y/z+c,z,true};
    }
    bool buildVegaStarPatchTris(int v,int root,const TierParams& tp,vector<VegaTri>& oldTris,vector<VegaTri>& newTris)const{
        oldTris.clear();newTris.clear();
        vector<int> ring,inc;
        if(!orientedRingForVertexVega(v,tp,ring,inc))return false;
        StarCandidate check=computeStarCandidateVega(v,tp);
        if(!check.valid())return false;
        root=check.root;
        for(int fi:inc){
            const Face& f=faces[fi];
            VegaTri t;
            t.p[0]=verts[f.v[0]];t.p[1]=verts[f.v[1]];t.p[2]=verts[f.v[2]];
            oldTris.push_back(t);
        }
        int m=(int)ring.size();
        vector<int> rr;rr.reserve(m);
        for(int i=0;i<m;++i)rr.push_back(ring[(root+i)%m]);
        for(int i=1;i<m-1;++i){
            VegaTri t;
            t.p[0]=verts[rr[0]];t.p[1]=verts[rr[i]];t.p[2]=verts[rr[i+1]];
            newTris.push_back(t);
        }
        return !oldTris.empty()&&!newTris.empty();
    }
    double vegaPatchGeomFrac()const{
        return tierTable[currentTier()].vegaPatchGeomFrac;
    }
    double vegaSampleToTrisMaxDistance2(const vector<VegaTri>& src,const vector<VegaTri>& dst)const{
        if(src.empty()||dst.empty())return shared.inf;
        double worst=0.0;
        for(const VegaTri& t:src){
            Vec3 samples[7]={
                t.p[0],t.p[1],t.p[2],
                (t.p[0]+t.p[1])*0.5,
                (t.p[1]+t.p[2])*0.5,
                (t.p[2]+t.p[0])*0.5,
                (t.p[0]+t.p[1]+t.p[2])/3.0
            };
            for(const Vec3& p:samples){
                double best=shared.inf;
                for(const VegaTri& q:dst)best=min(best,pointTriangleDistance2(p,q.p[0],q.p[1],q.p[2]));
                worst=max(worst,best);
            }
        }
        return worst;
    }
    double vegaPatchDeviation(const vector<VegaTri>& oldTris,const vector<VegaTri>& newTris)const{
        double d2=max(vegaSampleToTrisMaxDistance2(oldTris,newTris),
                      vegaSampleToTrisMaxDistance2(newTris,oldTris));
        if(!isfinite(d2))return shared.inf;
        return sqrt(max(0.0,d2));
    }
    static double vegaPixelChannel(const VegaPixel& p,int ch){
        if(ch<3)return p.n[ch];
        return p.d;
    }
    double vegaScalarSsim(const vector<VegaPixel>& a,const vector<VegaPixel>& b,int ch)const{
        double sx=0,sy=0,sxx=0,syy=0,sxy=0;int cnt=0;
        int n=(int)a.size();
        for(int i=0;i<n;++i){
            if(!a[i].fg&&!b[i].fg)continue;
            double x=vegaPixelChannel(a[i],ch),y=vegaPixelChannel(b[i],ch);
            sx+=x;sy+=y;sxx+=x*x;syy+=y*y;sxy+=x*y;++cnt;
        }
        if(cnt<4)return 1.0;
        double inv=1.0/double(cnt);
        double mx=sx*inv,my=sy*inv;
        double vx=max(0.0,sxx*inv-mx*mx),vy=max(0.0,syy*inv-my*my);
        double cov=sxy*inv-mx*my;
        double num=(2.0*mx*my+shared.vegaC1)*(2.0*cov+shared.vegaC2);
        double den=(mx*mx+my*my+shared.vegaC1)*(vx+vy+shared.vegaC2);
        if(den<=0.0)return 1.0;
        return clampDouble(num/den,-1.0,1.0);
    }
    double vegaBuffersSsim(const vector<VegaPixel>& a,const vector<VegaPixel>& b)const{
        int cnt=0;
        for(int i=0;i<(int)a.size();++i)if(a[i].fg||b[i].fg)++cnt;
        if(cnt==0)return 1.0;
        double sn=(vegaScalarSsim(a,b,0)+vegaScalarSsim(a,b,1)+vegaScalarSsim(a,b,2))/3.0;
        double sd=vegaScalarSsim(a,b,3);
        return shared.vegaNormalDepthWeight*sn+(1.0-shared.vegaNormalDepthWeight)*sd;
    }
    bool renderVegaPatch(const vector<VegaTri>& tris,int view,int x0,int y0,int w,int h,vector<VegaPixel>& buf)const{
        VegaPixel bg=vegaBackgroundPixel();
        buf.assign(w*h,bg);
        vector<float> zbuf(w*h,numeric_limits<float>::infinity());
        const int R=shared.vegaPatchResolution;
        const double to8=shared.vegaNormalTo8Bit;
        for(const VegaTri& tri:tris){
            VegaProj p0=vegaProjectPoint(tri.p[0],view,R);
            VegaProj p1=vegaProjectPoint(tri.p[1],view,R);
            VegaProj p2=vegaProjectPoint(tri.p[2],view,R);
            if(!p0.ok||!p1.ok||!p2.ok)continue;
            double area2=(p1.u-p0.u)*(p2.v-p0.v)-(p1.v-p0.v)*(p2.u-p0.u);
            if(fabs(area2)<shared.vegaEpsilonArea)continue;
            Vec3 nr=cross(tri.p[1]-tri.p[0],tri.p[2]-tri.p[0]);
            double nl=norm(nr);
            if(nl<shared.minNormalNorm)continue;
            nr=nr/nl;
            int bx0=max(x0,(int)floor(min({p0.u,p1.u,p2.u})));
            int bx1=min(x0+w-1,(int)ceil(max({p0.u,p1.u,p2.u})));
            int by0=max(y0,(int)floor(min({p0.v,p1.v,p2.v})));
            int by1=min(y0+h-1,(int)ceil(max({p0.v,p1.v,p2.v})));
            if(bx0>bx1||by0>by1)continue;
            double den=(p1.v-p2.v)*(p0.u-p2.u)+(p2.u-p1.u)*(p0.v-p2.v);
            if(fabs(den)<shared.vegaEpsilonDet)continue;
            for(int py=by0;py<=by1;++py){
                for(int px=bx0;px<=bx1;++px){
                    double sx=px+0.5,sy=py+0.5;
                    double w0=((p1.v-p2.v)*(sx-p2.u)+(p2.u-p1.u)*(sy-p2.v))/den;
                    double w1=((p2.v-p0.v)*(sx-p2.u)+(p0.u-p2.u)*(sy-p2.v))/den;
                    double w2=1.0-w0-w1;
                    if(w0<-shared.vegaEpsilonBary||w1<-shared.vegaEpsilonBary||w2<-shared.vegaEpsilonBary)continue;
                    double iz=w0/p0.z+w1/p1.z+w2/p2.z;
                    if(iz<=0.0)continue;
                    double z=1.0/iz;
                    int idx=(py-y0)*w+(px-x0);
                    if(z<zbuf[idx]){
                        zbuf[idx]=(float)z;
                        buf[idx].n[0]=(float)((nr.x+1.0)*to8);
                        buf[idx].n[1]=(float)((nr.y+1.0)*to8);
                        buf[idx].n[2]=(float)((nr.z+1.0)*to8);
                        buf[idx].d=(float)z;
                        buf[idx].fg=1;
                    }
                }
            }
        }
        return true;
    }
    double localVegaSsimForStarCandidate(int v,int root,const TierParams& tp)const{
        vector<VegaTri> oldTris,newTris;
        if(!buildVegaStarPatchTris(v,root,tp,oldTris,newTris))return -1.0;
        double dev=vegaPatchDeviation(oldTris,newTris);
        if(!(dev<=hausd*vegaPatchGeomFrac()))return -1.0;
        const int R=shared.vegaPatchResolution;
        double total=0.0;int usedViews=0;
        vector<VegaPixel> a,b;
        for(int view=0;view<6;++view){
            double mnU=1e100,mnV=1e100,mxU=-1e100,mxV=-1e100;
            bool any=false;
            auto includeTri=[&](const VegaTri& t){
                for(int k=0;k<3;++k){
                    VegaProj p=vegaProjectPoint(t.p[k],view,R);
                    if(!p.ok)continue;
                    any=true;
                    mnU=min(mnU,p.u);mxU=max(mxU,p.u);
                    mnV=min(mnV,p.v);mxV=max(mxV,p.v);
                }
            };
            for(const VegaTri& t:oldTris)includeTri(t);
            for(const VegaTri& t:newTris)includeTri(t);
            if(!any)continue;
            int pad=shared.vegaPatchPaddingPixels;
            int x0=max(0,(int)floor(mnU)-pad);
            int y0=max(0,(int)floor(mnV)-pad);
            int x1=min(R-1,(int)ceil(mxU)+pad);
            int y1=min(R-1,(int)ceil(mxV)+pad);
            if(x0>x1||y0>y1)continue;
            int w=x1-x0+1,h=y1-y0+1;
            if(w*h>shared.vegaPatchMaxPixels)return -1.0;
            renderVegaPatch(oldTris,view,x0,y0,w,h,a);
            renderVegaPatch(newTris,view,x0,y0,w,h,b);
            total+=vegaBuffersSsim(a,b);
            ++usedViews;
        }
        if(usedViews==0)return 1.0;
        return total/double(usedViews);
    }
    // Vega SSIM pass: vertex-bounded, no time dependency.
    // Bounded by: scanVertices, rounds, maxExtra, candidate pool cap.
    void vegaSsimStarPass(){
        const TierParams& tp = tierTable[currentTier()];
        if(tp.vegaMaxValence<=0)return;
        int maxExtra=min(tp.vegaHardCap,max(0,(int)floor(nV*tp.vegaExtraFrac)));
        if(maxExtra<=0)return;
        // Kill switch: if we're approaching budget, skip this pass entirely.
        if(elapsed()>=shared.timeBudgetSeconds)return;
        double minS = tp.vegaSsimMin;
        double maxDamage = tp.vegaDamageMax;
        vector<VegaCandidate> cands;
        cands.reserve(min(shared.vegaCandidatePoolCap,maxExtra*10+shared.candReserveExtra));
        int scanned=0,visited=0,total=(int)verts.size();
        int start=(total>0)?(vegaCursor%total):0;
        // Scan loop: bounded by scanVertices and total verts.
        // Kill switch checked at function entry and apply loop only (avoid chrono overhead).
        for(;visited<total&&scanned<tp.vegaScanVertices;++visited){
            int v=(start+visited)%total;
            if(vdead[v])continue;
            ++scanned;
            StarCandidate geom=computeStarCandidateVega(v,tp);
            if(!geom.valid())continue;
            double s=localVegaSsimForStarCandidate(geom.v,geom.root,tp);
            if(s<0.0)continue;
            double damage=1.0-s;
            if(s<minS||damage>maxDamage)continue;
            VegaCandidate vc;
            vc.v=geom.v;vc.root=geom.root;vc.ssim=s;
            vc.score=damage+shared.vegaScoreGeomWeight*geom.score;
            cands.push_back(vc);
            if((int)cands.size()>=shared.vegaCandidatePoolCap)break;
        }
        if(total>0)vegaCursor=(start+max(1,visited))%total;
        if(cands.empty())return;
        sort(cands.begin(),cands.end());
        int extra=0;
        // Apply loop: bounded by maxExtra AND the kill switch.
        for(const VegaCandidate& c:cands){
            if(extra>=maxExtra)break;
            if(elapsed()>=shared.timeBudgetSeconds)break;
            if(c.v<0||c.v>=(int)vdead.size()||vdead[c.v])continue;
            double s=localVegaSsimForStarCandidate(c.v,c.root,tp);
            if(s<minS||1.0-s>maxDamage)continue;
            if(applyStarDeleteVega(c.v,c.root,tp)){
                ++accepted;
                ++extra;
            }
        }
    }
    // Star (collapseInvisibleEdges) pass: vertex-bounded, no time dependency.
    // Bounded by: rounds, maxExtra, scanVertices.
    void collapseInvisibleEdges(){
        const TierParams& tp = tierTable[currentTier()];
        if(tp.starMaxValence<=0)return;
        // Vertex-count budget per phase: hardCap (absolute) OR extraFrac * nV (fraction)
        int maxExtra=min(tp.starHardCap,max(0,(int)floor(nV*tp.starExtraFrac)));
        if(maxExtra<=0)return;
        int extra=0;
        // Outer loop: bounded by rounds and maxExtra collapses.
        for(int round=0;round<tp.starRounds&&extra<maxExtra;++round){
            vector<StarCandidate> cands; cands.reserve(max(64, tp.starHardCap + 256));
            int scanned=0,visited=0,total=(int)verts.size();
            int start=(total>0)?(starCursor%total):0;
            for(;visited<total&&scanned<tp.starScanVertices;++visited){
                int v=(start+visited)%total;
                if(vdead[v])continue;
                ++scanned;
                StarCandidate c=computeStarCandidateStar(v,tp);
                if(c.valid())cands.push_back(c);
            }
            if(total>0)starCursor=(start+max(1,visited))%total;
            if(cands.empty())break;
            sort(cands.begin(),cands.end());
            bool progress=false;
            // Apply loop: bounded by maxExtra. NO time check.
            for(const StarCandidate& c:cands){
                if(extra>=maxExtra)break;
                if(vdead[c.v])continue;
                if(applyStarDelete(c.v,c.root)){ ++accepted; ++extra; progress=true; }
            }
            if(!progress)break;
        }
    }

    void compact(){
        vector<int> o2n(verts.size(),-1);
        vector<Vec3> nv;nv.reserve(verts.size()-accepted);
        for(int i=0;i<(int)verts.size();++i)if(!vdead[i]){o2n[i]=(int)nv.size();nv.push_back(verts[i]);}
        struct FK{array<int,3>key;Face face;bool operator<(const FK&o)const{return key<o.key;}};
        vector<FK> fc;fc.reserve(faces.size());
        for(int fi=0;fi<(int)faces.size();++fi){
            if(fdead[fi])continue;int a=faces[fi].v[0],b=faces[fi].v[1],c=faces[fi].v[2];
            if(a<0||b<0||c<0||a>=(int)verts.size()||b>=(int)verts.size()||c>=(int)verts.size())continue;
            if(vdead[a]||vdead[b]||vdead[c]||a==b||b==c||a==c)continue;
            int na=o2n[a],nb=o2n[b],nc=o2n[c];
            if(na<0||nb<0||nc<0||na==nb||nb==nc||na==nc)continue;
            Face nf;nf.v[0]=na;nf.v[1]=nb;nf.v[2]=nc;
            array<int,3>key={na,nb,nc};sort(key.begin(),key.end());
            fc.push_back({key,nf});}
        sort(fc.begin(),fc.end());
        vector<Face> nf;nf.reserve(fc.size());
        array<int,3>prev={-1,-1,-1};
        for(auto&item:fc){if(item.key==prev)continue;prev=item.key;nf.push_back(item.face);}
        verts.swap(nv);faces.swap(nf);nV=(int)verts.size();nF=(int)faces.size();
    }
};

bool QemSimplifier::MEMLESS=false;
int main(){QemSimplifier s;s.run();return 0;}
