#include <bits/stdc++.h>
using namespace std;

/*
IMC2026 Problem B — simplifygeometry_v38_edge_flip.cpp

V25 tail batching plus conservative huge-tail edge-flip unlock pass.

This iteration intentionally removes the previous star-removal/render-gating code path.
The core algorithm is classical progressive edge collapse:

  area-weighted QEM candidate cost
  + free-position optimal vertex placement
  + explicit target vertex ratios
  + scalar cluster-radius guard as a cheap Hausdorff-envelope proxy
  + closed-manifold link condition

strawberry-v3 keeps the small QEM core and adds three algorithmic changes:
  - symmetric collapse direction selection: either endpoint may be absorbed
  - multi-position candidate selection: QEM optimum, midpoint, endpoint A, endpoint B
  - priority-queue versioning: stale candidates are recomputed instead of accepted


Aggressive replay constants from user sweep:
  5k=0.00, 25k=0.35, 45k=0.19, 50k=0.11, 400k=0.045, huge=0.00.
This file intentionally preserves the exact strawberry-v3 algorithmic core; only the
keep-ratio ladder is changed.

This version deliberately excludes non-binding ranking modifiers:
  - vertex curvature computation
  - short-edge cost multiplier
  - unused evaluator constants
  - per-size cost caps

Theoretical basis:
  - Garland-Heckbert QEM: minimize squared distance to incident face planes.
  - Iterative edge collapse: choose low-cost edge, validate locally, collapse, update local costs.
  - Simplification-envelope intuition: accumulated cluster radius upper-bounds how far absorbed
    original vertices have moved from their representative.

Design constraints:
  - CParam_* are fixed problem/numerical constants.
  - HParam_* are tunable hyperparameters.
  - No fallback to original mesh.
  - No star-removal, patch remeshing, renderer proxy, or endpoint-only restriction.
*/

// =============================================================================
// Constants fixed by problem/evaluator or numerical representation
// =============================================================================

static constexpr double CParam_HausdorffDiagFraction = 0.05;
static constexpr double CParam_MinNormalNorm = 1e-12;
static constexpr double CParam_QemSolveDeterminantEps = 1e-12;
static constexpr double CParam_Inf = 1e100;

// =============================================================================
// Hyperparameters: tune this block first
// =============================================================================

static constexpr double HParam_TimeBudgetSeconds = 18.2;
static constexpr int    HParam_OutputPrecisionSignificantDigits = 10;

// Keep-ratio ladder.  These are intentionally explicit; score is vertex-count based
// once hard constraints and SSIM pass.
static constexpr double HParam_KeepRatio_UpTo5k    = 0.00;
static constexpr double HParam_KeepRatio_UpTo25k   = 0.35;
static constexpr double HParam_KeepRatio_UpTo45k   = 0.19;
static constexpr double HParam_KeepRatio_UpTo50k   = 0.11;
static constexpr double HParam_KeepRatio_UpTo400k  = 0.045;
static constexpr double HParam_KeepRatio_Huge      = 0.00;

// Pure-QEM cost cap. Actual cap is coefficient * AABBDiagonal^2.
// Sweep result: this is not currently the main active lever; it remains only as a safety brake.
static constexpr double HParam_QemCostCapCoeff = 0.014;

// Neighbor-distance guard switch.
// v7 passes at 68.89, but this guard is not an official constraint and may be
// blocking valid large-triangle collapses.  v8 disables it as an isolated
// algorithmic probe; the cluster-radius Hausdorff-envelope proxy remains active.
static constexpr bool   HParam_EnableNeighborDistanceGuard = false;
static constexpr double HParam_NeighborDistanceSafetyFrac = CParam_HausdorffDiagFraction;
static constexpr int    HParam_TailOriginalVertexThreshold = 400000;
static constexpr double HParam_TailBatchElapsedStart = 11.0;
static constexpr double HParam_TailBatchStopElapsed = 18.2;
static constexpr int    HParam_TailBatchScanEdges = 65536;
static constexpr int    HParam_TailBatchTargetAccepts = 2048;
static constexpr int    HParam_TailFlipScanEdges = 8192;
static constexpr int    HParam_TailFlipMaxAccepted = 96;

// =============================================================================
// Fast input
// =============================================================================

struct FastScanner {
    vector<char> buffer;
    char* ptr = nullptr;

    FastScanner() {
        buffer.reserve(1 << 27);
        char chunk[1 << 16];
        size_t n = 0;
        while ((n = fread(chunk, 1, sizeof(chunk), stdin)) > 0) {
            buffer.insert(buffer.end(), chunk, chunk + n);
        }
        buffer.push_back('\0');
        ptr = buffer.data();
    }

    inline void skipWhitespace() {
        while (*ptr && *ptr <= ' ') ++ptr;
    }

    int readInt() {
        skipWhitespace();
        int sign = 1;
        if (*ptr == '-') { sign = -1; ++ptr; }
        int value = 0;
        while (*ptr >= '0' && *ptr <= '9') {
            value = value * 10 + (*ptr - '0');
            ++ptr;
        }
        return sign * value;
    }

    double readDouble() {
        skipWhitespace();
        char* endPtr = nullptr;
        double value = strtod(ptr, &endPtr);
        ptr = endPtr;
        return value;
    }

    char readTokenChar() {
        skipWhitespace();
        char c = *ptr;
        if (*ptr) ++ptr;
        return c;
    }
};

// =============================================================================
// Geometry primitives
// =============================================================================

struct Vec3 {
    double x = 0.0, y = 0.0, z = 0.0;

    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(double s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(double s) const { return Vec3(x / s, y / s, z / s); }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
};

static inline double dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    );
}

static inline double norm2(const Vec3& v) { return dot(v, v); }
static inline double norm(const Vec3& v) { return sqrt(norm2(v)); }

static inline Vec3 normalizedOrZero(const Vec3& v) {
    double n = norm(v);
    if (n < CParam_MinNormalNorm) return Vec3();
    return v / n;
}

static inline bool finiteVec(const Vec3& v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

struct Face {
    int v[3] = {0, 0, 0};
};

// Symmetric homogeneous 4x4 quadric stored as:
// [ a b c d ]
// [ b e f g ]
// [ c f h i ]
// [ d g i j ]
struct Quadric {
    double a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;

    Quadric& operator+=(const Quadric& o) {
        a+=o.a; b+=o.b; c+=o.c; d+=o.d; e+=o.e;
        f+=o.f; g+=o.g; h+=o.h; i+=o.i; j+=o.j;
        return *this;
    }

    friend Quadric operator+(Quadric lhs, const Quadric& rhs) {
        lhs += rhs;
        return lhs;
    }

    void scale(double s) {
        a*=s; b*=s; c*=s; d*=s; e*=s; f*=s; g*=s; h*=s; i*=s; j*=s;
    }

    static Quadric fromTriangle(const Vec3& p0, const Vec3& p1, const Vec3& p2) {
        Vec3 n = cross(p1 - p0, p2 - p0);
        double twiceArea = norm(n);
        if (twiceArea < CParam_MinNormalNorm) return Quadric();
        n = n / twiceArea;
        double planeD = -dot(n, p0);

        Quadric q;
        q.a = n.x*n.x;       q.b = n.x*n.y;       q.c = n.x*n.z;       q.d = n.x*planeD;
        q.e = n.y*n.y;       q.f = n.y*n.z;       q.g = n.y*planeD;
        q.h = n.z*n.z;       q.i = n.z*planeD;    q.j = planeD*planeD;
        q.scale(0.5 * twiceArea); // area-weighted plane quadric
        return q;
    }

    double evaluate(const Vec3& p) const {
        const double x=p.x, y=p.y, z=p.z;
        return a*x*x + 2*b*x*y + 2*c*x*z + 2*d*x
             + e*y*y + 2*f*y*z + 2*g*y
             + h*z*z + 2*i*z + j;
    }
};

// =============================================================================
// Small linear algebra
// =============================================================================

static double det3(
    double a00,double a01,double a02,
    double a10,double a11,double a12,
    double a20,double a21,double a22
) {
    return a00*(a11*a22 - a12*a21)
         - a01*(a10*a22 - a12*a20)
         + a02*(a10*a21 - a11*a20);
}

static bool solveQem3x3(const Quadric& q, Vec3& out) {
    // Solve H p = -c where H is the upper-left 3x3 and c=(d,g,i).
    const double A00=q.a, A01=q.b, A02=q.c;
    const double A10=q.b, A11=q.e, A12=q.f;
    const double A20=q.c, A21=q.f, A22=q.h;
    const double B0=-q.d, B1=-q.g, B2=-q.i;

    double D = det3(A00,A01,A02, A10,A11,A12, A20,A21,A22);
    if (fabs(D) < CParam_QemSolveDeterminantEps) return false;

    double Dx = det3(B0,A01,A02, B1,A11,A12, B2,A21,A22);
    double Dy = det3(A00,B0,A02, A10,B1,A12, A20,B2,A22);
    double Dz = det3(A00,A01,B0, A10,A11,B1, A20,A21,B2);
    out = Vec3(Dx/D, Dy/D, Dz/D);
    return finiteVec(out);
}

// =============================================================================
// Priority queue candidate
// =============================================================================

struct CollapseCandidate {
    int absorbed = -1;
    int kept = -1;
    int versionAbsorbed = -1;
    int versionKept = -1;
    double cost = CParam_Inf;
    double mergedRadius = 0.0;
    Vec3 position;

    bool operator<(const CollapseCandidate& o) const {
        return cost > o.cost; // min-heap via priority_queue
    }

    bool valid() const {
        return absorbed >= 0 && kept >= 0 && cost < CParam_Inf;
    }
};

// =============================================================================
// Simplifier
// =============================================================================

class QemSimplifier {
public:
    void run() {
        readMesh();
        if (vertices.size() <= 4) {
            writeMesh();
            return;
        }

        startTime = chrono::steady_clock::now();
        initializeScaleAndTargets();
        buildInitialConnectivityAndQuadrics();
        initializeQueue();
        collapseLoop();
        compactMesh();
        writeMesh();
    }

private:
    vector<Vec3> vertices;
    vector<Face> faces;

    vector<char> vertexDead;
    vector<char> faceDead;
    vector<int> vertexVersion;
    vector<Quadric> vertexQuadrics;
    vector<double> clusterRadius;

    vector<vector<int>> vertexFaces;
    vector<unordered_set<int>> vertexNeighbors;
    vector<int> tailLocks;

    priority_queue<CollapseCandidate> queue;

    int targetVertexCount = 0;
    int collapseLimit = 0;
    int acceptedCollapses = 0;
    int tailScanCursor = 0;
    int tailFlipCursor = 0;
    int tailLockStamp = 1;
    int lastFailedTailBatchAccepted = -1;
    int lastTailFlipAttemptAccepted = -1;

    double diagonal = 0.0;
    double hausdorffLimit = 0.0;
    double neighborDistanceLimit = 0.0;
    double costCap = CParam_Inf;

    chrono::steady_clock::time_point startTime;

private:
    // -------------------------------------------------------------------------
    // Input / output
    // -------------------------------------------------------------------------

    void readMesh() {
        FastScanner fs;
        int nV = fs.readInt();
        int nF = fs.readInt();
        vertices.resize(nV);
        faces.resize(nF);

        for (int i = 0; i < nV; ++i) {
            (void)fs.readTokenChar(); // 'v'
            vertices[i].x = fs.readDouble();
            vertices[i].y = fs.readDouble();
            vertices[i].z = fs.readDouble();
        }
        for (int i = 0; i < nF; ++i) {
            (void)fs.readTokenChar(); // 'f'
            faces[i].v[0] = fs.readInt() - 1;
            faces[i].v[1] = fs.readInt() - 1;
            faces[i].v[2] = fs.readInt() - 1;
        }
    }

    void writeMesh() const {
        string out;
        out.reserve(vertices.size() * 42 + faces.size() * 26 + 64);
        char line[128];
        snprintf(line, sizeof(line), "%d %d\n", (int)vertices.size(), (int)faces.size());
        out += line;

        const char* vertexFormat = nullptr;
        // Kattis output accepts standard decimal/scientific forms.  %.10g is compact and
        // matches the known high-scoring simple implementation profile.
        (void)vertexFormat;
        for (const Vec3& p : vertices) {
            snprintf(line, sizeof(line), "v %.*g %.*g %.*g\n",
                     HParam_OutputPrecisionSignificantDigits, p.x,
                     HParam_OutputPrecisionSignificantDigits, p.y,
                     HParam_OutputPrecisionSignificantDigits, p.z);
            out += line;
        }
        for (const Face& f : faces) {
            snprintf(line, sizeof(line), "f %d %d %d\n", f.v[0]+1, f.v[1]+1, f.v[2]+1);
            out += line;
        }
        fwrite(out.data(), 1, out.size(), stdout);
    }

    // -------------------------------------------------------------------------
    // Parameter schedule
    // -------------------------------------------------------------------------

    static double keepRatioForVertexCount(int nV) {
        if (nV <= 5000)   return HParam_KeepRatio_UpTo5k;
        if (nV <= 25000)  return HParam_KeepRatio_UpTo25k;
        if (nV <= 45000)  return HParam_KeepRatio_UpTo45k;
        if (nV <= 50000)  return HParam_KeepRatio_UpTo50k;
        if (nV <= 400000) return HParam_KeepRatio_UpTo400k;
        return HParam_KeepRatio_Huge;
    }


    void initializeScaleAndTargets() {
        Vec3 mn = vertices[0], mx = vertices[0];
        for (const Vec3& p : vertices) {
            mn.x = min(mn.x, p.x); mn.y = min(mn.y, p.y); mn.z = min(mn.z, p.z);
            mx.x = max(mx.x, p.x); mx.y = max(mx.y, p.y); mx.z = max(mx.z, p.z);
        }
        diagonal = norm(mx - mn);
        hausdorffLimit = CParam_HausdorffDiagFraction * diagonal;
        neighborDistanceLimit = HParam_NeighborDistanceSafetyFrac * diagonal;
        costCap = HParam_QemCostCapCoeff * diagonal * diagonal;

        const double keepRatio = keepRatioForVertexCount((int)vertices.size());
        targetVertexCount = max(10, (int)floor((double)vertices.size() * keepRatio));
        targetVertexCount = min(targetVertexCount, (int)vertices.size() - 1);
        collapseLimit = (int)vertices.size() - targetVertexCount;
    }

    // -------------------------------------------------------------------------
    // Connectivity / QEM initialization
    // -------------------------------------------------------------------------

    void buildInitialConnectivityAndQuadrics() {
        const int nV = (int)vertices.size();
        const int nF = (int)faces.size();

        vertexDead.assign(nV, 0);
        faceDead.assign(nF, 0);
        vertexVersion.assign(nV, 0);
        vertexQuadrics.assign(nV, Quadric());
        clusterRadius.assign(nV, 0.0);
        vertexFaces.assign(nV, {});
        vertexNeighbors.assign(nV, {});
        tailLocks.assign(nV, 0);

        for (int fi = 0; fi < nF; ++fi) {
            const Face& f = faces[fi];
            const Vec3& p0 = vertices[f.v[0]];
            const Vec3& p1 = vertices[f.v[1]];
            const Vec3& p2 = vertices[f.v[2]];
            Quadric q = Quadric::fromTriangle(p0, p1, p2);

            for (int k = 0; k < 3; ++k) {
                int v = f.v[k];
                vertexFaces[v].push_back(fi);
                vertexQuadrics[v] += q;
            }
            for (int k = 0; k < 3; ++k) {
                int a = f.v[k];
                int b = f.v[(k + 1) % 3];
                if (a == b) continue;
                vertexNeighbors[a].insert(b);
                vertexNeighbors[b].insert(a);
            }
        }
    }


    // -------------------------------------------------------------------------
    // Candidate computation
    // -------------------------------------------------------------------------

    vector<Vec3> candidatePositionsForEdge(int a, int b, const Quadric& q) const {
        vector<Vec3> positions;
        positions.reserve(4);

        Vec3 qemPoint;
        if (solveQem3x3(q, qemPoint)) positions.push_back(qemPoint);

        positions.push_back((vertices[a] + vertices[b]) * 0.5);
        positions.push_back(vertices[a]);
        positions.push_back(vertices[b]);

        // Remove exact duplicates to avoid redundant guard checks in flat/singular regions.
        vector<Vec3> uniquePositions;
        uniquePositions.reserve(positions.size());
        for (const Vec3& p : positions) {
            if (!finiteVec(p)) continue;
            bool duplicate = false;
            for (const Vec3& qpos : uniquePositions) {
                if (norm2(p - qpos) < 1e-30) { duplicate = true; break; }
            }
            if (!duplicate) uniquePositions.push_back(p);
        }
        return uniquePositions;
    }

    CollapseCandidate makeCandidate(int absorbed, int kept, const Vec3& p, const Quadric& q) const {
        CollapseCandidate c;
        c.absorbed = absorbed;
        c.kept = kept;
        c.versionAbsorbed = vertexVersion[absorbed];
        c.versionKept = vertexVersion[kept];
        c.position = p;
        c.cost = q.evaluate(p);
        c.mergedRadius = 0.0;
        return c;
    }

    CollapseCandidate computeQueueCandidate(int a, int b) const {
        // Cheap ranking candidate: choose the lowest-QEM candidate over both collapse
        // directions and the deterministic candidate-position set.  Validity guards are
        // re-run on pop, when the candidate is current and actually considered for collapse.
        Quadric q = vertexQuadrics[a] + vertexQuadrics[b];
        vector<Vec3> positions = candidatePositionsForEdge(a, b, q);

        CollapseCandidate best;
        for (const Vec3& p : positions) {
            CollapseCandidate c1 = makeCandidate(a, b, p, q);
            if (c1.cost < best.cost) best = c1;
            CollapseCandidate c2 = makeCandidate(b, a, p, q);
            if (c2.cost < best.cost) best = c2;
        }
        return best;
    }

    CollapseCandidate computeBestValidCandidate(int a, int b) const {
        Quadric q = vertexQuadrics[a] + vertexQuadrics[b];
        vector<Vec3> positions = candidatePositionsForEdge(a, b, q);

        CollapseCandidate best;
        for (const Vec3& p : positions) {
            for (int dir = 0; dir < 2; ++dir) {
                int absorbed = (dir == 0 ? a : b);
                int kept = (dir == 0 ? b : a);

                double mergedRadius = 0.0;
                if (!passesEnvelopeGuard(absorbed, kept, p, mergedRadius)) continue;
                if (!passesNeighborDistanceGuard(absorbed, kept, p)) continue;

                CollapseCandidate c = makeCandidate(absorbed, kept, p, q);
                c.mergedRadius = mergedRadius;
                if (c.cost < best.cost) best = c;
            }
        }
        return best;
    }

    void initializeQueue() {
        for (int a = 0; a < (int)vertices.size(); ++a) {
            for (int b : vertexNeighbors[a]) {
                if (a < b) {
                    CollapseCandidate c = computeQueueCandidate(a, b);
                    if (c.valid()) queue.push(c);
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Local validity checks
    // -------------------------------------------------------------------------

    bool timeExceeded(int tick) const {
        if ((tick & 8191) != 0) return false;
        double elapsed = chrono::duration<double>(chrono::steady_clock::now() - startTime).count();
        return elapsed > HParam_TimeBudgetSeconds;
    }

    double elapsedSeconds() const {
        return chrono::duration<double>(chrono::steady_clock::now() - startTime).count();
    }

    double currentCostCap() const {
        return costCap;
    }

    bool tailBatchMode() const {
        if ((int)vertices.size() <= HParam_TailOriginalVertexThreshold) return false;
        double elapsed = elapsedSeconds();
        return elapsed > HParam_TailBatchElapsedStart && elapsed < HParam_TailBatchStopElapsed;
    }

    bool edgeStillExists(int a, int b) const {
        if (a < 0 || b < 0) return false;
        if (a >= (int)vertices.size() || b >= (int)vertices.size()) return false;
        if (vertexDead[a] || vertexDead[b]) return false;
        return vertexNeighbors[a].find(b) != vertexNeighbors[a].end();
    }

    vector<int> commonFaces(int a, int b) const {
        vector<int> result;
        const vector<int>& fa = vertexFaces[a];
        const vector<int>& fb = vertexFaces[b];

        if (fa.size() < fb.size()) {
            unordered_set<int> setA;
            setA.reserve(fa.size() * 2 + 1);
            for (int f : fa) if (!faceDead[f]) setA.insert(f);
            for (int f : fb) if (!faceDead[f] && setA.find(f) != setA.end()) result.push_back(f);
        } else {
            unordered_set<int> setB;
            setB.reserve(fb.size() * 2 + 1);
            for (int f : fb) if (!faceDead[f]) setB.insert(f);
            for (int f : fa) if (!faceDead[f] && setB.find(f) != setB.end()) result.push_back(f);
        }
        return result;
    }

    int commonNeighborCount(int a, int b) const {
        int count = 0;
        const auto& na = vertexNeighbors[a];
        const auto& nb = vertexNeighbors[b];
        if (na.size() < nb.size()) {
            for (int x : na) {
                if (x == a || x == b || vertexDead[x]) continue;
                if (nb.find(x) != nb.end()) ++count;
            }
        } else {
            for (int x : nb) {
                if (x == a || x == b || vertexDead[x]) continue;
                if (na.find(x) != na.end()) ++count;
            }
        }
        return count;
    }

    bool passesEnvelopeGuard(int a, int b, const Vec3& p, double& mergedRadius) const {
        mergedRadius = max(clusterRadius[a] + norm(vertices[a] - p),
                           clusterRadius[b] + norm(vertices[b] - p));
        return mergedRadius <= hausdorffLimit;
    }

    bool passesNeighborDistanceGuard(int absorbedVertex, int keptVertex, const Vec3& p) const {
        if (!HParam_EnableNeighborDistanceGuard) return true;
        for (int nb : vertexNeighbors[absorbedVertex]) {
            if (nb == keptVertex || vertexDead[nb]) continue;
            if (norm(vertices[nb] - p) > neighborDistanceLimit) return false;
        }
        return true;
    }

    static void eraseValue(vector<int>& xs, int value) {
        auto it = find(xs.begin(), xs.end(), value);
        if (it != xs.end()) xs.erase(it);
    }

    double triangleQuality(const Vec3& a, const Vec3& b, const Vec3& c) const {
        double area2 = norm(cross(b - a, c - a));
        double l0 = norm2(b - a);
        double l1 = norm2(c - b);
        double l2 = norm2(a - c);
        double denom = l0 + l1 + l2;
        if (denom <= 1e-30) return 0.0;
        return area2 / denom;
    }

    int oppositeVertexInFace(int fi, int a, int b) const {
        int result = -1;
        bool hasA = false, hasB = false;
        for (int k = 0; k < 3; ++k) {
            int v = faces[fi].v[k];
            if (v == a) hasA = true;
            else if (v == b) hasB = true;
            else result = v;
        }
        return (hasA && hasB) ? result : -1;
    }

    Face orientedFace(int a, int b, int c, const Vec3& refNormal) const {
        Face f;
        Vec3 n = cross(vertices[b] - vertices[a], vertices[c] - vertices[a]);
        if (dot(n, refNormal) >= 0.0) {
            f.v[0] = a; f.v[1] = b; f.v[2] = c;
        } else {
            f.v[0] = a; f.v[1] = c; f.v[2] = b;
        }
        return f;
    }

    void addFaceRef(int v, int fi) {
        if (v >= 0 && v < (int)vertexFaces.size()) vertexFaces[v].push_back(fi);
    }

    void pushLocalEdgesForVertex(int v) {
        if (v < 0 || v >= (int)vertices.size() || vertexDead[v]) return;
        for (int nb : vertexNeighbors[v]) {
            if (nb == v || vertexDead[nb]) continue;
            CollapseCandidate c = computeQueueCandidate(v, nb);
            if (c.valid()) queue.push(c);
        }
    }

    bool tryFlipEdge(int a, int b) {
        if (!edgeStillExists(a, b)) return false;
        vector<int> cf = commonFaces(a, b);
        if (cf.size() != 2) return false;

        int f0 = cf[0], f1 = cf[1];
        int c = oppositeVertexInFace(f0, a, b);
        int d = oppositeVertexInFace(f1, a, b);
        if (c < 0 || d < 0 || c == d || c == a || c == b || d == a || d == b) return false;
        if (vertexDead[c] || vertexDead[d]) return false;
        if (vertexNeighbors[c].find(d) != vertexNeighbors[c].end()) return false;

        Vec3 oldN0 = cross(vertices[faces[f0].v[1]] - vertices[faces[f0].v[0]],
                           vertices[faces[f0].v[2]] - vertices[faces[f0].v[0]]);
        Vec3 oldN1 = cross(vertices[faces[f1].v[1]] - vertices[faces[f1].v[0]],
                           vertices[faces[f1].v[2]] - vertices[faces[f1].v[0]]);
        if (norm2(oldN0) < 1e-24 || norm2(oldN1) < 1e-24) return false;

        double oldQuality = triangleQuality(vertices[a], vertices[b], vertices[c])
                          + triangleQuality(vertices[a], vertices[b], vertices[d]);
        double newQuality = triangleQuality(vertices[c], vertices[d], vertices[b])
                          + triangleQuality(vertices[d], vertices[c], vertices[a]);
        if (!(newQuality > oldQuality * 1.03)) return false;

        CollapseCandidate oldCandidate = computeQueueCandidate(a, b);
        CollapseCandidate newCandidate = computeQueueCandidate(c, d);
        if (!newCandidate.valid()) return false;
        if (oldCandidate.valid() && newCandidate.cost > oldCandidate.cost * 1.05) return false;

        int old0[3] = {faces[f0].v[0], faces[f0].v[1], faces[f0].v[2]};
        int old1[3] = {faces[f1].v[0], faces[f1].v[1], faces[f1].v[2]};
        for (int k = 0; k < 3; ++k) {
            eraseValue(vertexFaces[old0[k]], f0);
            eraseValue(vertexFaces[old1[k]], f1);
        }

        faces[f0] = orientedFace(c, d, b, oldN0);
        faces[f1] = orientedFace(d, c, a, oldN1);

        for (int k = 0; k < 3; ++k) {
            addFaceRef(faces[f0].v[k], f0);
            addFaceRef(faces[f1].v[k], f1);
        }

        vertexNeighbors[a].erase(b);
        vertexNeighbors[b].erase(a);
        vertexNeighbors[c].insert(d);
        vertexNeighbors[d].insert(c);

        ++vertexVersion[a];
        ++vertexVersion[b];
        ++vertexVersion[c];
        ++vertexVersion[d];

        pushLocalEdgesForVertex(a);
        pushLocalEdgesForVertex(b);
        pushLocalEdgesForVertex(c);
        pushLocalEdgesForVertex(d);
        return true;
    }

    int runTailFlipPass() {
        if (!tailBatchMode()) return 0;
        const int nV = (int)vertices.size();
        int scannedEdges = 0;
        int visitedVertices = 0;
        int flipped = 0;
        for (; visitedVertices < nV && scannedEdges < HParam_TailFlipScanEdges; ++visitedVertices) {
            int a = (tailFlipCursor + visitedVertices) % nV;
            if (vertexDead[a]) continue;
            vector<int> neighbors(vertexNeighbors[a].begin(), vertexNeighbors[a].end());
            for (int b : neighbors) {
                if (scannedEdges >= HParam_TailFlipScanEdges) break;
                if (b <= a || vertexDead[b]) continue;
                ++scannedEdges;
                if (tryFlipEdge(a, b)) {
                    ++flipped;
                    if (flipped >= HParam_TailFlipMaxAccepted) break;
                }
            }
            if (flipped >= HParam_TailFlipMaxAccepted) break;
        }
        tailFlipCursor = (tailFlipCursor + max(1, visitedVertices)) % nV;
        return flipped;
    }

    struct EdgeBatchCandidate {
        int a = -1;
        int b = -1;
        double cost = CParam_Inf;
        bool operator<(const EdgeBatchCandidate& o) const {
            return cost < o.cost;
        }
    };

    bool batchNeighborhoodFree(int a, int b) const {
        if (tailLocks[a] == tailLockStamp || tailLocks[b] == tailLockStamp) return false;
        const auto& na = vertexNeighbors[a];
        const auto& nb = vertexNeighbors[b];
        if (na.size() < nb.size()) {
            for (int x : na) {
                if (x == a || x == b || vertexDead[x]) continue;
                if (nb.find(x) != nb.end() && tailLocks[x] == tailLockStamp) return false;
            }
        } else {
            for (int x : nb) {
                if (x == a || x == b || vertexDead[x]) continue;
                if (na.find(x) != na.end() && tailLocks[x] == tailLockStamp) return false;
            }
        }
        return true;
    }

    void lockBatchNeighborhood(int a, int b) {
        tailLocks[a] = tailLockStamp;
        tailLocks[b] = tailLockStamp;
        const auto& na = vertexNeighbors[a];
        const auto& nb = vertexNeighbors[b];
        if (na.size() < nb.size()) {
            for (int x : na) {
                if (x == a || x == b || vertexDead[x]) continue;
                if (nb.find(x) != nb.end()) tailLocks[x] = tailLockStamp;
            }
        } else {
            for (int x : nb) {
                if (x == a || x == b || vertexDead[x]) continue;
                if (na.find(x) != na.end()) tailLocks[x] = tailLockStamp;
            }
        }
    }

    int runTailBatch() {
        if (!tailBatchMode()) return 0;
        if (++tailLockStamp == INT_MAX) {
            fill(tailLocks.begin(), tailLocks.end(), 0);
            tailLockStamp = 1;
        }

        vector<EdgeBatchCandidate> candidates;
        candidates.reserve(HParam_TailBatchScanEdges / 4);

        const int nV = (int)vertices.size();
        int scannedEdges = 0;
        int visitedVertices = 0;
        const double activeCap = currentCostCap();

        for (; visitedVertices < nV && scannedEdges < HParam_TailBatchScanEdges; ++visitedVertices) {
            int a = (tailScanCursor + visitedVertices) % nV;
            if (vertexDead[a]) continue;
            for (int b : vertexNeighbors[a]) {
                if (scannedEdges >= HParam_TailBatchScanEdges) break;
                if (b <= a || vertexDead[b]) continue;
                ++scannedEdges;
                CollapseCandidate c = computeQueueCandidate(a, b);
                if (!c.valid() || c.cost > activeCap) continue;
                candidates.push_back({a, b, c.cost});
            }
        }
        tailScanCursor = (tailScanCursor + max(1, visitedVertices)) % nV;

        if (candidates.empty()) return 0;
        sort(candidates.begin(), candidates.end());

        vector<EdgeBatchCandidate> selected;
        selected.reserve(HParam_TailBatchTargetAccepts);
        for (const EdgeBatchCandidate& e : candidates) {
            if ((int)selected.size() >= HParam_TailBatchTargetAccepts) break;
            if (!edgeStillExists(e.a, e.b)) continue;
            if (!batchNeighborhoodFree(e.a, e.b)) continue;
            lockBatchNeighborhood(e.a, e.b);
            selected.push_back(e);
        }

        int acceptedInBatch = 0;
        for (const EdgeBatchCandidate& e : selected) {
            if (acceptedCollapses >= collapseLimit) break;
            if (elapsedSeconds() > HParam_TailBatchStopElapsed) break;
            if (!edgeStillExists(e.a, e.b)) continue;

            vector<int> cf = commonFaces(e.a, e.b);
            if (cf.size() != 2) continue;
            if (commonNeighborCount(e.a, e.b) != 2) continue;

            CollapseCandidate best = computeBestValidCandidate(e.a, e.b);
            if (!best.valid()) continue;
            if (best.cost > currentCostCap()) continue;

            applyCollapse(best.absorbed, best.kept, best.position, best.mergedRadius);
            ++acceptedCollapses;
            ++acceptedInBatch;
        }
        return acceptedInBatch;
    }

    // -------------------------------------------------------------------------
    // Collapse update
    // -------------------------------------------------------------------------

    void collapseLoop() {
        int tick = 0;
        while (acceptedCollapses < collapseLimit && (!queue.empty() || tailBatchMode())) {
            if (timeExceeded(++tick)) break;

            if (tailBatchMode() && lastTailFlipAttemptAccepted != acceptedCollapses) {
                int flipped = runTailFlipPass();
                lastTailFlipAttemptAccepted = acceptedCollapses;
                if (flipped > 0) continue;
            }

            if (tailBatchMode() && lastFailedTailBatchAccepted != acceptedCollapses) {
                int batchAccepted = runTailBatch();
                if (batchAccepted > 0) continue;
                lastFailedTailBatchAccepted = acceptedCollapses;
            }

            if (queue.empty()) break;

            CollapseCandidate c = queue.top();
            queue.pop();

            int a = c.absorbed;
            int b = c.kept;
            if (!edgeStillExists(a, b)) continue;

            // Lazy priority queue with explicit endpoint versions.  Old queue entries are
            // not trusted after either endpoint has moved or absorbed another cluster.
            if (c.versionAbsorbed != vertexVersion[a] || c.versionKept != vertexVersion[b]) {
                CollapseCandidate fresh = computeQueueCandidate(a, b);
                if (fresh.valid()) queue.push(fresh);
                continue;
            }

            double activeCap = currentCostCap();
            if (c.cost > activeCap) break;

            vector<int> cf = commonFaces(a, b);
            if (cf.size() != 2) continue;
            if (commonNeighborCount(a, b) != 2) continue;

            CollapseCandidate best = computeBestValidCandidate(a, b);
            if (!best.valid()) continue;
            if (best.cost > activeCap) continue;

            applyCollapse(best.absorbed, best.kept, best.position, best.mergedRadius);
            ++acceptedCollapses;
        }
    }

    void applyCollapse(int absorbed, int kept, const Vec3& newPosition, double newClusterRadius) {
        // Move kept vertex to free QEM optimum.
        vertices[kept] = newPosition;
        clusterRadius[kept] = newClusterRadius;
        clusterRadius[absorbed] = 0.0;
        vertexDead[absorbed] = 1;
        ++vertexVersion[absorbed];
        ++vertexVersion[kept];

        // Replace absorbed by kept in all incident faces.  Degenerate faces are killed.
        vector<int> absorbedFaces = vertexFaces[absorbed];
        vector<int> deadFacesThisCollapse;
        deadFacesThisCollapse.reserve(absorbedFaces.size());

        for (int fi : absorbedFaces) {
            if (faceDead[fi]) continue;
            bool touched = false;
            for (int k = 0; k < 3; ++k) {
                if (faces[fi].v[k] == absorbed) {
                    faces[fi].v[k] = kept;
                    touched = true;
                }
            }
            if (!touched) continue;

            int x = faces[fi].v[0], y = faces[fi].v[1], z = faces[fi].v[2];
            if (x == y || y == z || x == z) {
                faceDead[fi] = 1;
                deadFacesThisCollapse.push_back(fi);
            } else {
                vertexFaces[kept].push_back(fi);
            }
        }

        // Remove killed faces from all currently referenced vertices.
        for (int fi : deadFacesThisCollapse) {
            for (int k = 0; k < 3; ++k) {
                int v = faces[fi].v[k];
                if (v >= 0 && v < (int)vertexFaces.size()) eraseValue(vertexFaces[v], fi);
            }
        }
        vertexFaces[absorbed].clear();

        // Merge quadrics.
        vertexQuadrics[kept] += vertexQuadrics[absorbed];

        // Update adjacency sets.
        unordered_set<int> mergedNeighbors = vertexNeighbors[kept];
        mergedNeighbors.erase(absorbed);
        mergedNeighbors.erase(kept);

        for (int nb : vertexNeighbors[absorbed]) {
            if (nb == kept || vertexDead[nb]) continue;
            vertexNeighbors[nb].erase(absorbed);
            vertexNeighbors[nb].insert(kept);
            mergedNeighbors.insert(nb);
        }

        vertexNeighbors[absorbed].clear();
        vertexNeighbors[kept] = std::move(mergedNeighbors);
        vertexNeighbors[kept].erase(absorbed);
        vertexNeighbors[kept].erase(kept);

        // Push updated local edge costs.  Lazy invalidation handles old queue entries.
        for (int nb : vertexNeighbors[kept]) {
            if (nb == kept || vertexDead[nb]) continue;
            CollapseCandidate c = computeQueueCandidate(kept, nb);
            if (c.valid()) queue.push(c);
        }
    }

    // -------------------------------------------------------------------------
    // Final compaction
    // -------------------------------------------------------------------------

    static array<int,3> canonicalFaceKey(int a, int b, int c) {
        array<int,3> key = {a,b,c};
        sort(key.begin(), key.end());
        return key;
    }

    void compactMesh() {
        const int nV = (int)vertices.size();
        vector<int> oldToNew(nV, -1);
        vector<Vec3> newVertices;
        newVertices.reserve(nV - acceptedCollapses);

        for (int i = 0; i < nV; ++i) {
            if (!vertexDead[i]) {
                oldToNew[i] = (int)newVertices.size();
                newVertices.push_back(vertices[i]);
            }
        }

        struct FaceWithKey {
            array<int,3> key;
            Face face;
            bool operator<(const FaceWithKey& o) const { return key < o.key; }
        };

        vector<FaceWithKey> faceCandidates;
        faceCandidates.reserve(faces.size());

        for (int fi = 0; fi < (int)faces.size(); ++fi) {
            if (faceDead[fi]) continue;
            int a = faces[fi].v[0];
            int b = faces[fi].v[1];
            int c = faces[fi].v[2];
            if (a < 0 || b < 0 || c < 0 || a >= nV || b >= nV || c >= nV) continue;
            if (vertexDead[a] || vertexDead[b] || vertexDead[c]) continue;
            if (a == b || b == c || a == c) continue;

            int na = oldToNew[a];
            int nb = oldToNew[b];
            int nc = oldToNew[c];
            if (na < 0 || nb < 0 || nc < 0) continue;
            if (na == nb || nb == nc || na == nc) continue;

            Face nf;
            nf.v[0] = na; nf.v[1] = nb; nf.v[2] = nc;
            faceCandidates.push_back({canonicalFaceKey(na, nb, nc), nf});
        }

        sort(faceCandidates.begin(), faceCandidates.end());
        vector<Face> newFaces;
        newFaces.reserve(faceCandidates.size());
        array<int,3> previousKey = {-1,-1,-1};
        bool hasPrevious = false;
        for (const FaceWithKey& item : faceCandidates) {
            if (hasPrevious && item.key == previousKey) continue;
            previousKey = item.key;
            hasPrevious = true;
            newFaces.push_back(item.face);
        }

        vertices.swap(newVertices);
        faces.swap(newFaces);
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    QemSimplifier simplifier;
    simplifier.run();
    return 0;
}
