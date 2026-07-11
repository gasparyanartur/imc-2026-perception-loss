// Pineapple v156: v153 + T5 weld maxValence 10 → 12. Mild T5 bump.
#include <bits/stdc++.h>

static constexpr double HParam_Pineapple_KeepRatio_UpTo400k = 0.025;
static constexpr double HParam_Pineapple_KeepRatio_Huge = 0.028;

using namespace std;

static constexpr double CParam_HausdorffDiagFraction = 0.055;

static constexpr double CParam_MinNormalNorm = 1e-12;

static constexpr double CParam_QemSolveDeterminantEps = 1e-12;

static constexpr double CParam_Inf = 1e100;

static constexpr double HParam_TimeBudgetSeconds = 20.2;

static constexpr int HParam_OutputPrecisionSignificantDigits = 10;

static constexpr double HParam_KeepRatio_UpTo5k = 0.00;

static constexpr double HParam_KeepRatio_UpTo25k = 0.32;

static constexpr double HParam_KeepRatio_UpTo45k = 0.16;

static constexpr double HParam_KeepRatio_UpTo50k = 0.1;

static constexpr double HParam_KeepRatio_UpTo400k = HParam_Pineapple_KeepRatio_UpTo400k;

static constexpr double HParam_KeepRatio_Huge = HParam_Pineapple_KeepRatio_Huge;

static constexpr double HParam_QemCostCapCoeff = 0.0375;

static constexpr int HParam_TailOriginalVertexThreshold = 1000000;

static constexpr double HParam_TailBatchElapsedStart = 11.8;

static constexpr double HParam_TailBatchStopElapsed = 19.4;

static constexpr int HParam_TailBatchScanEdges = 65536;

static constexpr int HParam_TailBatchTargetAccepts = 2048;

static constexpr bool HParam_EnableRootNudge = true;

static constexpr int HParam_RootNudgeProfile = 1;

static constexpr double CParam_ViewWeightK = 3.0;

static constexpr double CParam_MaxFaceWeight = 3.0;

static constexpr bool HParam_EnableVegaSsimPass = true;

static constexpr int HParam_VegaPatchResolution = 512;

static constexpr int HParam_VegaPatchPaddingPixels = 4;

static constexpr int HParam_VegaPatchMaxPixels = 52000;

static constexpr int HParam_VegaCandidatePoolCap = 28000;

static constexpr double HParam_VegaNormalDepthWeight = 0.55;

static constexpr double HParam_VegaScoreGeomWeight = 0.0018;

static constexpr double HParam_VegaC1 = (0.01 * 255.0) * (0.01 * 255.0);

static constexpr double HParam_VegaC2 = (0.03 * 255.0) * (0.03 * 255.0);

struct Vec3
{

    double x = 0, y = 0, z = 0;

    constexpr Vec3() = default;

    constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }

    Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }

    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }

    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
};

static inline double dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

static inline Vec3 cross(const Vec3 &a, const Vec3 &b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

static inline double norm2(const Vec3 &v) { return dot(v, v); }

static inline double norm(const Vec3 &v) { return sqrt(norm2(v)); }

static inline bool finiteVec(const Vec3 &v) { return isfinite(v.x) && isfinite(v.y) && isfinite(v.z); }

struct Face
{
    int v[3];
};

struct Quadric
{

    double a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0, j = 0;

    Quadric &operator+=(const Quadric &o)
    {
        a += o.a;
        b += o.b;
        c += o.c;
        d += o.d;
        e += o.e;
        f += o.f;
        g += o.g;
        h += o.h;
        i += o.i;
        j += o.j;
        return *this;
    }

    void scale(double s)
    {
        a *= s;
        b *= s;
        c *= s;
        d *= s;
        e *= s;
        f *= s;
        g *= s;
        h *= s;
        i *= s;
        j *= s;
    }

    static Quadric fromTriangle(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2)
    {

        Vec3 n = cross(p1 - p0, p2 - p0);
        double ta = norm(n);

        if (ta < CParam_MinNormalNorm)
            return Quadric();

        n = n / ta;
        double pd = -dot(n, p0);

        Quadric q;
        q.a = n.x * n.x;
        q.b = n.x * n.y;
        q.c = n.x * n.z;
        q.d = n.x * pd;

        q.e = n.y * n.y;
        q.f = n.y * n.z;
        q.g = n.y * pd;
        q.h = n.z * n.z;
        q.i = n.z * pd;
        q.j = pd * pd;

        q.scale(sqrt(0.5 * ta));
        return q;
    }

    double evaluate(const Vec3 &p) const
    {

        return a * p.x * p.x + 2 * b * p.x * p.y + 2 * c * p.x * p.z + 2 * d * p.x + e * p.y * p.y + 2 * f * p.y * p.z + 2 * g * p.y + h * p.z * p.z + 2 * i * p.z + j;
    }
};

struct SmallSet
{

    vector<int> data;

    void insert(int v)
    {

        auto it = lower_bound(data.begin(), data.end(), v);

        if (it == data.end() || *it != v)
            data.insert(it, v);
    }

    void erase(int v)
    {

        auto it = lower_bound(data.begin(), data.end(), v);

        if (it != data.end() && *it == v)
            data.erase(it);
    }

    bool contains(int v) const { return binary_search(data.begin(), data.end(), v); }

    int size() const { return (int)data.size(); }

    auto begin() const { return data.begin(); }

    auto end() const { return data.end(); }

    void clear() { data.clear(); }

    void reserve(int n) { data.reserve(n); }
};

static double det3(double a00, double a01, double a02, double a10, double a11, double a12, double a20, double a21, double a22)
{

    return a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);
}

static bool solveQem3x3(const Quadric &q, Vec3 &out)
{

    double D = det3(q.a, q.b, q.c, q.b, q.e, q.f, q.c, q.f, q.h);

    if (fabs(D) < CParam_QemSolveDeterminantEps)
        return false;

    out = Vec3(det3(-q.d, q.b, q.c, -q.g, q.e, q.f, -q.i, q.f, q.h) / D,

               det3(q.a, -q.d, q.c, q.b, -q.g, q.f, q.c, -q.i, q.h) / D,

               det3(q.a, q.b, -q.d, q.b, q.e, -q.g, q.c, q.f, -q.i) / D);

    return finiteVec(out);
}

struct CollapseCandidate
{

    int absorbed = -1, kept = -1, versionAbsorbed = -1, versionKept = -1;

    double cost = CParam_Inf, mergedRadius = 0.0;

    Vec3 position;

    bool operator<(const CollapseCandidate &o) const { return cost > o.cost; }

    bool valid() const { return absorbed >= 0 && kept >= 0 && cost < CParam_Inf; }
};

class QemSimplifier
{

public:
    static bool MEMLESS;

public:
    void run()
    {

        readMesh();

        if (nV <= 10)
        {
            writeMesh();
            return;
        }

        startTime = chrono::steady_clock::now();

        if (nV > 5000 && nV <= 50000)
        {
            runScreenCoreMid();
            compact();
            writeMesh();
            return;
        }

        initScale();

        buildConnectivity();

        initFaceWeights();

        initQueue();

        collapseLoop();

        if (elapsed() < HParam_TimeBudgetSeconds - 0.95)
        {

            valenceWeldPass();
        }

        if (elapsed() < HParam_TimeBudgetSeconds - 1.10)
        {

            pairDiskPass();
        }

        // Star-delete is disabled on the huge tier (T7): on very large, irregular

        // meshes the time-gated retriangulation is nondeterministic and can cross a

        // validity/Hausdorff edge on some judge runs. T7 gains little from it anyway

        // (it is Hausdorff-capped, not flat-vertex-capped). Keep it for T2-T6.

        if (nV <= HParam_TailOriginalVertexThreshold &&

            elapsed() < HParam_TimeBudgetSeconds - 1.0)
        {

            collapseInvisibleEdges();
        }

        if (elapsed() < HParam_TimeBudgetSeconds - 0.88)
        {

            valenceWeldPass();
        }

        if (elapsed() < HParam_TimeBudgetSeconds - 0.78)
        {

            pairDiskPass();
        }

        if (HParam_EnableVegaSsimPass &&

            elapsed() < HParam_TimeBudgetSeconds - 0.65)
        {

            vegaSsimStarPass();
        }

        // Sharon v13: 2nd Vega pass (Cherry v24 style)

        if (HParam_EnableVegaSsimPass &&

            elapsed() < HParam_TimeBudgetSeconds - 0.30)
        {

            vegaSsimStarPass();
        }

        if (HParam_EnableVegaSsimPass &&

            elapsed() < HParam_TimeBudgetSeconds - 0.12)
        {

            vegaSsimStarPass();
        }

        compact();

        writeMesh();
    }

private:
    int nV = 0, nF = 0;

    vector<Vec3> verts;

    vector<Face> faces;

    vector<char> vdead, fdead;

    vector<int> vver;

    vector<Quadric> vquad, vmoment;

    vector<double> crad;

    vector<vector<int>> vfaces;

    vector<SmallSet> vneigh;

    vector<int> tailLocks;

    vector<int> facePix, faceSil, faceWin;
    vector<double> vertAnchor;

    int screenTier = 0;
    double faceImpMean = 0.0;

    priority_queue<CollapseCandidate> pq;

    int targetV = 0, collapseLimit = 0, accepted = 0;

    int tailCursor = 0, tailStamp = 1, lastFailBatch = -1, starCursor = 0, vegaCursor = 0;

    double diag = 0, hausd = 0, costCap = CParam_Inf;

    double invDiag2 = 0.0;

    chrono::steady_clock::time_point startTime;

    static constexpr Vec3 cameraDirs[6] = {

        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}

    };

    void readMesh()
    {

        vector<char> buf;
        buf.reserve(1 << 27);

        char chunk[1 << 16];
        size_t n;

        while ((n = fread(chunk, 1, sizeof(chunk), stdin)) > 0)
            buf.insert(buf.end(), chunk, chunk + n);

        buf.push_back('\0');
        char *p = buf.data();

        nV = (int)strtol(p, &p, 10);
        nF = (int)strtol(p, &p, 10);

        verts.resize(nV);
        faces.resize(nF);

        for (int i = 0; i < nV; ++i)
        {
            while (*p && *p <= ' ')
                ++p;
            ++p;
            verts[i].x = strtod(p, &p);
            verts[i].y = strtod(p, &p);
            verts[i].z = strtod(p, &p);
        }

        for (int i = 0; i < nF; ++i)
        {
            while (*p && *p <= ' ')
                ++p;
            ++p;
            faces[i].v[0] = (int)strtol(p, &p, 10) - 1;
            faces[i].v[1] = (int)strtol(p, &p, 10) - 1;
            faces[i].v[2] = (int)strtol(p, &p, 10) - 1;
        }
    }

    void writeMesh()
    {

        string out;
        out.reserve(nV * 42 + nF * 26 + 64);
        char line[128];

        snprintf(line, sizeof(line), "%d %d\n", nV, nF);
        out += line;

        for (int i = 0; i < nV; ++i)
        {
            snprintf(line, sizeof(line), "v %.*g %.*g %.*g\n", HParam_OutputPrecisionSignificantDigits, verts[i].x, HParam_OutputPrecisionSignificantDigits, verts[i].y, HParam_OutputPrecisionSignificantDigits, verts[i].z);
            out += line;
        }

        for (int i = 0; i < nF; ++i)
        {
            snprintf(line, sizeof(line), "f %d %d %d\n", faces[i].v[0] + 1, faces[i].v[1] + 1, faces[i].v[2] + 1);
            out += line;
        }

        fwrite(out.data(), 1, out.size(), stdout);
    }

    void cameraBasisImp(int view, Vec3 &eye, Vec3 &right, Vec3 &up, Vec3 &fwd) const
    {
        Vec3 dirs[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        eye = dirs[view] * 2.5;
        fwd = eye * (-1.0);
        double fl = norm(fwd);
        fwd = fwd / fl;
        Vec3 wu = (fabs(dot(fwd, Vec3(0, 0, 1))) > 0.9) ? Vec3(0, 1, 0) : Vec3(0, 0, 1);
        right = cross(wu, fwd);
        double rl = norm(right);
        right = rl > 1e-15 ? right / rl : Vec3(1, 0, 0);
        up = cross(fwd, right);
        double ul = norm(up);
        up = ul > 1e-15 ? up / ul : Vec3(0, 1, 0);
    }
    struct ImpProj
    {
        double u = 0, v = 0, z = 0;
        bool ok = false;
    };
    ImpProj projectImp(const Vec3 &p, int view, int R) const
    {
        Vec3 e, r, u, f;
        cameraBasisImp(view, e, r, u, f);
        Vec3 q = p - e;
        double x = dot(q, r), y = dot(q, u), z = dot(q, f);
        if (z <= 1e-8)
            return {};
        double sc = double(R) / 1024.0;
        return {800.0 * sc * x / z + 0.5 * R, 800.0 * sc * y / z + 0.5 * R, z, true};
    }
    void buildRasterImportance(int R)
    {
        facePix.assign(nF, 0);
        faceSil.assign(nF, 0);
        faceWin.assign(nF, 0);
        vector<Vec3> fn(nF);
        for (int fi = 0; fi < nF; ++fi)
        {
            if (fi < (int)fdead.size() && fdead[fi])
                continue;
            auto &f = faces[fi];
            Vec3 n = cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
            double l = norm(n);
            if (l > 1e-15)
                fn[fi] = n / l;
        }
        int rad = max(1, (int)ceil(5.0 * R / 1024.0));
        for (int view = 0; view < 6; ++view)
        {
            vector<float> z((size_t)R * R, 1e30f);
            vector<int> id((size_t)R * R, -1);
            for (int fi = 0; fi < nF; ++fi)
            {
                if (fi < (int)fdead.size() && fdead[fi])
                    continue;
                auto &f = faces[fi];
                auto a = projectImp(verts[f.v[0]], view, R), b = projectImp(verts[f.v[1]], view, R), c = projectImp(verts[f.v[2]], view, R);
                if (!a.ok || !b.ok || !c.ok)
                    continue;
                double den = (b.v - c.v) * (a.u - c.u) + (c.u - b.u) * (a.v - c.v);
                if (fabs(den) < 1e-18)
                    continue;
                int x0 = max(0, (int)floor(min({a.u, b.u, c.u}))), x1 = min(R - 1, (int)ceil(max({a.u, b.u, c.u}))), y0 = max(0, (int)floor(min({a.v, b.v, c.v}))), y1 = min(R - 1, (int)ceil(max({a.v, b.v, c.v})));
                for (int y = y0; y <= y1; ++y)
                    for (int x = x0; x <= x1; ++x)
                    {
                        double X = x + .5, Y = y + .5, w0 = ((b.v - c.v) * (X - c.u) + (c.u - b.u) * (Y - c.v)) / den, w1 = ((c.v - a.v) * (X - c.u) + (a.u - c.u) * (Y - c.v)) / den, w2 = 1 - w0 - w1;
                        if (w0 < -1e-9 || w1 < -1e-9 || w2 < -1e-9)
                            continue;
                        double iz = w0 / a.z + w1 / b.z + w2 / c.z;
                        if (iz <= 0)
                            continue;
                        float zz = 1.0 / iz;
                        int q = y * R + x;
                        if (zz < z[q])
                        {
                            z[q] = zz;
                            id[q] = fi;
                        }
                    }
            }
            for (int q = 0; q < R * R; ++q)
                if (id[q] >= 0)
                    ++facePix[id[q]];
            vector<unsigned short> d((size_t)R * R, 60000);
            for (int y = 0; y < R; ++y)
                for (int x = 0; x < R; ++x)
                {
                    int q = y * R + x, f = id[q];
                    if (f < 0)
                        continue;
                    bool ft = false;
                    for (int dy = -1; dy <= 1 && !ft; ++dy)
                        for (int dx = -1; dx <= 1; ++dx)
                        {
                            if (!dx && !dy)
                                continue;
                            int xx = x + dx, yy = y + dy;
                            if (xx < 0 || yy < 0 || xx >= R || yy >= R)
                            {
                                ft = true;
                                break;
                            }
                            int g = id[yy * R + xx];
                            if (g < 0 || (g != f && (fabs(z[q] - z[yy * R + xx]) > 0.0035 || dot(fn[f], fn[g]) < 0.992)))
                            {
                                ft = true;
                                break;
                            }
                        }
                    if (ft)
                    {
                        d[q] = 0;
                        ++faceSil[f];
                    }
                }
            for (int y = 0; y < R; ++y)
                for (int x = 0; x < R; ++x)
                {
                    int q = y * R + x, v = d[q];
                    if (x)
                        v = min<int>(v, d[q - 1] + 1);
                    if (y)
                        v = min<int>(v, d[q - R] + 1);
                    if (x && y)
                        v = min<int>(v, d[q - R - 1] + 1);
                    if (x + 1 < R && y)
                        v = min<int>(v, d[q - R + 1] + 1);
                    d[q] = v;
                }
            for (int y = R - 1; y >= 0; --y)
                for (int x = R - 1; x >= 0; --x)
                {
                    int q = y * R + x, v = d[q];
                    if (x + 1 < R)
                        v = min<int>(v, d[q + 1] + 1);
                    if (y + 1 < R)
                        v = min<int>(v, d[q + R] + 1);
                    if (x + 1 < R && y + 1 < R)
                        v = min<int>(v, d[q + R + 1] + 1);
                    if (x && y + 1 < R)
                        v = min<int>(v, d[q + R - 1] + 1);
                    d[q] = v;
                    if (v <= rad && id[q] >= 0)
                        ++faceWin[id[q]];
                }
        }
    }
    void buildRasterImportanceTier3(int R)
    {
        facePix.assign(nF, 0);
        faceSil.assign(nF, 0);
        faceWin.assign(nF, 0);
        vector<Vec3> fn(nF);
        for (int fi = 0; fi < nF; ++fi)
        {
            if (fi < (int)fdead.size() && fdead[fi])
                continue;
            auto &f = faces[fi];
            Vec3 n = cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
            double l = norm(n);
            if (l > 1e-15)
                fn[fi] = n / l;
        }
        for (int view = 0; view < 6; ++view)
        {
            vector<float> z((size_t)R * R, 1e30f);
            vector<int> id((size_t)R * R, -1);
            for (int fi = 0; fi < nF; ++fi)
            {
                if (fi < (int)fdead.size() && fdead[fi])
                    continue;
                auto &f = faces[fi];
                auto a = projectImp(verts[f.v[0]], view, R), b = projectImp(verts[f.v[1]], view, R), c = projectImp(verts[f.v[2]], view, R);
                if (!a.ok || !b.ok || !c.ok)
                    continue;
                double den = (b.v - c.v) * (a.u - c.u) + (c.u - b.u) * (a.v - c.v);
                if (fabs(den) < 1e-18)
                    continue;
                int x0 = max(0, (int)floor(min({a.u, b.u, c.u}))), x1 = min(R - 1, (int)ceil(max({a.u, b.u, c.u}))), y0 = max(0, (int)floor(min({a.v, b.v, c.v}))), y1 = min(R - 1, (int)ceil(max({a.v, b.v, c.v})));
                for (int y = y0; y <= y1; ++y)
                    for (int x = x0; x <= x1; ++x)
                    {
                        double X = x + .5, Y = y + .5, w0 = ((b.v - c.v) * (X - c.u) + (c.u - b.u) * (Y - c.v)) / den, w1 = ((c.v - a.v) * (X - c.u) + (a.u - c.u) * (Y - c.v)) / den, w2 = 1 - w0 - w1;
                        if (w0 < -1e-9 || w1 < -1e-9 || w2 < -1e-9)
                            continue;
                        double iz = w0 / a.z + w1 / b.z + w2 / c.z;
                        if (iz <= 0)
                            continue;
                        float zz = 1.0 / iz;
                        int q = y * R + x;
                        if (zz < z[q])
                        {
                            z[q] = zz;
                            id[q] = fi;
                        }
                    }
            }
            for (int q = 0; q < R * R; ++q)
                if (id[q] >= 0)
                    ++facePix[id[q]];
            for (int y = 1; y < R - 1; ++y)
                for (int x = 1; x < R - 1; ++x)
                {
                    int q = y * R + x, f = id[q];
                    if (f < 0)
                        continue;
                    bool sil = false;
                    for (int d : {-1, 1, -R, R})
                    {
                        int g = id[q + d];
                        if (g < 0 || (g != f && (fabs(z[q] - z[q + d]) > 0.006 || dot(fn[f], fn[g]) < 0.985)))
                        {
                            sil = true;
                            break;
                        }
                    }
                    if (sil)
                        ++faceSil[f];
                }
        }
    }
    void runScreenCoreMid()
    {
        screenTier = nV <= 25000 ? 2 : (nV <= 45000 ? 3 : 4);
        initScale();
        buildConnectivity();
        vmoment.assign(nV, Quadric());
        if (screenTier == 4)
            for (int i = 0; i < nV; ++i)
                vmoment[i] = anchorPointQuadric(verts[i], 1e-6);
        if (screenTier == 3)
        {
            int finalTarget = (int)floor(nV * 0.145), safeTarget = (int)floor(nV * 0.16);
            buildRasterImportanceTier3(384);
            initFaceWeights();
            targetV = safeTarget;
            collapseLimit = nV - targetV;
            initQueue();
            collapseLoop();
            if (accepted < nV - finalTarget && elapsed() < HParam_TimeBudgetSeconds - 0.4)
            {
                buildRasterImportanceTier3(384);
                initFaceWeights();
                priority_queue<CollapseCandidate> e;
                pq.swap(e);
                targetV = finalTarget;
                collapseLimit = nV - targetV;
                initQueue();
                collapseLoop();
            }
            return;
        }
        vector<double> st = screenTier == 2 ? vector<double>{0.36, 0.33, 0.30} : vector<double>{0.14, 0.10, 0.08};
        for (double kr : st)
        {
            if (elapsed() > HParam_TimeBudgetSeconds - 0.5)
                break;
            buildRasterImportance(screenTier == 2 ? 1024 : 768);
            initFaceWeights();
            priority_queue<CollapseCandidate> e;
            pq.swap(e);
            targetV = (int)floor(nV * kr);
            collapseLimit = nV - targetV;
            initQueue();
            collapseLoop();
        }
    }

    void initScale()
    {

        // Per-tier algorithm dispatch: memoryless QEM ONLY for medium tiers.

        // T7 (and T6, T2) get MEMLESS=false → EXACTLY the 88.83 champion path.

        MEMLESS = (nV > 5000); // advisor T4: also enable memoryless on T7

        Vec3 mn = verts[0], mx = verts[0];

        for (auto &p : verts)
        {
            mn.x = min(mn.x, p.x);
            mn.y = min(mn.y, p.y);
            mn.z = min(mn.z, p.z);
            mx.x = max(mx.x, p.x);
            mx.y = max(mx.y, p.y);
            mx.z = max(mx.z, p.z);
        }

        diag = norm(mx - mn);
        hausd = CParam_HausdorffDiagFraction * diag;

        costCap = HParam_QemCostCapCoeff * diag * diag;

        invDiag2 = (diag > CParam_MinNormalNorm) ? (1.0 / (diag * diag)) : 0.0;

        double kr;

        if (nV <= 5000)
            kr = HParam_KeepRatio_UpTo5k;

        else if (nV <= 25000)
            kr = HParam_KeepRatio_UpTo25k;

        else if (nV <= 45000)
            kr = HParam_KeepRatio_UpTo45k;

        else if (nV <= 50000)
            kr = HParam_KeepRatio_UpTo50k;

        else if (nV <= 400000)
            kr = HParam_KeepRatio_UpTo400k;

        else
            kr = HParam_KeepRatio_Huge;

        targetV = max(10, (int)floor(nV * kr));

        targetV = min(targetV, nV - 1);

        collapseLimit = nV - targetV;
    }

    void buildConnectivity()
    {

        vdead.assign(nV, 0);
        fdead.assign(nF, 0);
        vver.assign(nV, 0);

        vquad.assign(nV, Quadric());
        crad.assign(nV, 0.0);

        vfaces.assign(nV, {});
        vneigh.resize(nV);
        tailLocks.assign(nV, 0);

        for (int fi = 0; fi < nF; ++fi)
        {

            auto &f = faces[fi];

            Quadric q = Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]);

            for (int k = 0; k < 3; ++k)
            {
                vfaces[f.v[k]].push_back(fi);
                vquad[f.v[k]] += q;
            }

            for (int k = 0; k < 3; ++k)
            {
                int a = f.v[k], b = f.v[(k + 1) % 3];
                if (a != b)
                {
                    vneigh[a].insert(b);
                    vneigh[b].insert(a);
                }
            }
        }
    }

    double faceWeightFor(const Vec3 &unitNormal, double area) const
    {

        double absSum = fabs(unitNormal.x) + fabs(unitNormal.y) + fabs(unitNormal.z);

        double normalizedArea = area * invDiag2;

        double w = 1.0 + CParam_ViewWeightK * normalizedArea * absSum;

        return min(w, CParam_MaxFaceWeight);
    }

    Quadric anchorPointQuadric(const Vec3 &p, double w) const
    {
        Quadric q;
        q.a = q.e = q.h = w;
        q.d = -w * p.x;
        q.g = -w * p.y;
        q.i = -w * p.z;
        q.j = w * norm2(p);
        return q;
    }

    void initFaceWeights()
    {

        vquad.assign(nV, Quadric());
        faceImpMean = 0.0;
        int activeF = 0;
        if (screenTier)
            for (int fi = 0; fi < nF; ++fi)
                if (!(fi < (int)fdead.size() && fdead[fi]))
                {
                    faceImpMean += screenTier == 3 ? (facePix[fi] + 10.0 * faceSil[fi]) : (facePix[fi] + 12.0 * faceSil[fi] + 2.0 * faceWin[fi]);
                    ++activeF;
                }
        if (screenTier)
            faceImpMean /= max(1, activeF);
        vertAnchor.assign(nV, 0.0);
        if (screenTier == 2 || screenTier == 4)
            for (int fi = 0; fi < nF; ++fi)
                if (!(fi < (int)fdead.size() && fdead[fi]))
                    for (int k = 0; k < 3; ++k)
                        vertAnchor[faces[fi].v[k]] += faceSil[fi] + 0.05 * facePix[fi] + 0.10 * faceWin[fi];

        for (int fi = 0; fi < nF; ++fi)
        {
            if (fi < (int)fdead.size() && fdead[fi])
                continue;
            const Face &f = faces[fi];
            Vec3 n = cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
            double area = 0.5 * norm(n);
            Quadric q = Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]);
            if (screenTier)
            {
                double imp = screenTier == 3 ? (facePix[fi] + 10.0 * faceSil[fi]) : (facePix[fi] + 12.0 * faceSil[fi] + 2.0 * faceWin[fi]);
                double r = screenTier == 3 ? (imp + 0.20 * faceImpMean) / (faceImpMean + 1e-12) : (imp + 0.12 * faceImpMean) / (faceImpMean + 1e-12);
                double w;
                if (screenTier == 2)
                    w = min(18.0, max(0.06, 0.08 + 0.92 * pow(max(0.0, r), 0.90)));
                else if (screenTier == 3)
                    w = min(7.0, max(0.12, 0.15 + 0.85 * sqrt(max(0.0, r))));
                else
                    w = min(16.0, max(0.06, 0.08 + 0.92 * pow(max(0.0, r), 0.72)));
                if (w != 1.0)
                    q.scale(w);
            }
            else if (area >= 1e-30)
            {
                n = n / (2.0 * area);
                double w = faceWeightFor(n, area);
                if (w != 1.0)
                    q.scale(w);
            }
            for (int k = 0; k < 3; ++k)
                vquad[f.v[k]] += q;
        }
        if (screenTier == 2 || screenTier == 4)
        {
            double am = 0;
            int ac = 0;
            for (int i = 0; i < nV; ++i)
                if (!vdead[i])
                {
                    am += vertAnchor[i];
                    ++ac;
                }
            am /= max(1, ac);
            double bw = (screenTier == 2 ? 0.0000035 : 0.0000050) * diag * diag / (am + 1.0);
            for (int i = 0; i < nV; ++i)
                if (!vdead[i] && vertAnchor[i] > 0)
                    vquad[i] += anchorPointQuadric(verts[i], bw * min(18.0, vertAnchor[i] / (am + 1e-12)));
            if (screenTier == 4)
                for (int i = 0; i < nV; ++i)
                    if (!vdead[i])
                        vquad[i] += vmoment[i];
        }
    }

    Quadric weightedFaceQuadric(int fi) const
    {
        const Face &f = faces[fi];
        Vec3 n = cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
        double area = 0.5 * norm(n);
        Quadric q = Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]);
        if (screenTier && fi < (int)facePix.size())
        {
            double imp = facePix[fi] + 12.0 * faceSil[fi] + 2.0 * faceWin[fi];
            double r = (imp + 0.12 * faceImpMean) / (faceImpMean + 1e-12);
            double w = screenTier == 2 ? min(18.0, max(0.06, 0.08 + 0.92 * pow(max(0.0, r), 0.90))) : min(16.0, max(0.06, 0.08 + 0.92 * pow(max(0.0, r), 0.72)));
            if (w != 1.0)
                q.scale(w);
        }
        else if (area >= 1e-30)
        {
            n = n / (2.0 * area);
            double w = faceWeightFor(n, area);
            if (w != 1.0)
                q.scale(w);
        }
        return q;
    }

    bool isFaceInvisible(int fi) const
    {

        const Face &f = faces[fi];

        Vec3 n = cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);

        double len = norm(n);

        if (len < 1e-30)
            return false;

        n = n / len;

        for (int k = 0; k < 6; ++k)
        {

            if (dot(n, cameraDirs[k]) > 0)
                return false;
        }

        return true;
    }

    void getCandidatePositions(int a, int b, const Quadric &q, Vec3 pos[4], int &np) const
    {

        np = 0;

        Vec3 qp;

        if (solveQem3x3(q, qp))
            pos[np++] = qp;

        pos[np++] = (verts[a] + verts[b]) * 0.5;

        pos[np++] = verts[a];

        pos[np++] = verts[b];

        int wp = 0;

        for (int i = 0; i < np; ++i)
        {

            if (!finiteVec(pos[i]))
                continue;

            bool dup = false;

            for (int j = 0; j < wp; ++j)
                if (norm2(pos[i] - pos[j]) < 1e-30)
                {
                    dup = true;
                    break;
                }

            if (!dup)
                pos[wp++] = pos[i];
        }

        np = wp;
    }

    CollapseCandidate makeCandidate(int ab, int kp, const Vec3 &p, const Quadric &q) const
    {

        CollapseCandidate c;
        c.absorbed = ab;
        c.kept = kp;

        c.versionAbsorbed = vver[ab];
        c.versionKept = vver[kp];

        c.position = p;
        c.cost = q.evaluate(p);
        return c;
    }

    double cheapEdgeCost(int a, int b) const
    {

        // Fast lower-cost proxy: quadric error at the edge midpoint (no 3x3 solve,

        // no multi-position search). Used only to PRE-RANK/FILTER tail candidates;

        // the full 8-placement solve still runs in computeBestValid for accepted ones.

        Quadric q = vquad[a];
        q += vquad[b];

        Vec3 mid = (verts[a] + verts[b]) * 0.5;

        return q.evaluate(mid);
    }

    CollapseCandidate computeQueueCandidate(int a, int b) const
    {

        Quadric q = vquad[a];
        q += vquad[b];

        Vec3 pos[4];
        int np;
        getCandidatePositions(a, b, q, pos, np);

        CollapseCandidate best;

        for (int i = 0; i < np; ++i)
        {

            CollapseCandidate c1 = makeCandidate(a, b, pos[i], q);
            if (c1.cost < best.cost)
                best = c1;

            CollapseCandidate c2 = makeCandidate(b, a, pos[i], q);
            if (c2.cost < best.cost)
                best = c2;
        }

        return best;
    }

    bool passesEnvelope(int a, int b, const Vec3 &p, double &mr) const
    {

        mr = max(crad[a] + norm(verts[a] - p), crad[b] + norm(verts[b] - p));

        return mr <= hausd;
    }

    CollapseCandidate computeBestValid(int a, int b) const
    {

        Quadric q = vquad[a];
        q += vquad[b];

        Vec3 pos[4];
        int np;
        getCandidatePositions(a, b, q, pos, np);

        CollapseCandidate best;

        for (int i = 0; i < np; ++i)
        {

            for (int dir = 0; dir < 2; ++dir)
            {

                int ab = dir ? b : a, kp = dir ? a : b;

                double mr;

                if (!passesEnvelope(ab, kp, pos[i], mr))
                    continue;

                CollapseCandidate c = makeCandidate(ab, kp, pos[i], q);

                c.mergedRadius = mr;

                if (c.cost < best.cost)
                    best = c;
            }
        }

        return best;
    }

    void initQueue()
    {

        for (int a = 0; a < nV; ++a)

            for (int b : vneigh[a])
                if (a < b)
                {
                    auto c = computeQueueCandidate(a, b);
                    if (c.valid())
                        pq.push(c);
                }
    }

    double elapsed() const { return chrono::duration<double>(chrono::steady_clock::now() - startTime).count(); }

    bool tailMode() const
    {
        if (nV <= HParam_TailOriginalVertexThreshold)
            return false;
        double e = elapsed();
        return e > HParam_TailBatchElapsedStart && e < HParam_TailBatchStopElapsed;
    }

    bool edgeExists(int a, int b) const { return a >= 0 && b >= 0 && a < (int)verts.size() && b < (int)verts.size() && !vdead[a] && !vdead[b] && vneigh[a].contains(b); }

    int countCommonFaces(int a, int b) const
    {

        int cnt = 0;

        const auto &fa = vfaces[a];
        const auto &fb = vfaces[b];

        if (fa.size() < fb.size())
        {
            for (int f : fa)
            {
                if (!fdead[f])
                    for (int f2 : fb)
                        if (f == f2)
                        {
                            ++cnt;
                            break;
                        }
            }
        }

        else
        {
            for (int f : fb)
            {
                if (!fdead[f])
                    for (int f2 : fa)
                        if (f == f2)
                        {
                            ++cnt;
                            break;
                        }
            }
        }

        return cnt;
    }

    int countCommonNeighbors(int a, int b) const
    {

        int cnt = 0;

        const auto &na = vneigh[a];
        const auto &nb = vneigh[b];

        if (na.size() < nb.size())
        {
            for (int x : na)
            {
                if (x != a && x != b && !vdead[x] && nb.contains(x))
                    ++cnt;
            }
        }

        else
        {
            for (int x : nb)
            {
                if (x != a && x != b && !vdead[x] && na.contains(x))
                    ++cnt;
            }
        }

        return cnt;
    }

    static void eraseVal(vector<int> &v, int x)
    {
        for (int i = (int)v.size() - 1; i >= 0; --i)
            if (v[i] == x)
            {
                v[i] = v.back();
                v.pop_back();
                return;
            }
    }

    bool batchFree(int a, int b) const
    {

        if (tailLocks[a] == tailStamp || tailLocks[b] == tailStamp)
            return false;

        for (int nb : vneigh[a])
            if (!vdead[nb] && tailLocks[nb] == tailStamp)
                return false;

        for (int nb : vneigh[b])
            if (!vdead[nb] && tailLocks[nb] == tailStamp)
                return false;

        return true;
    }

    void lockBatch(int a, int b)
    {

        tailLocks[a] = tailLocks[b] = tailStamp;

        for (int nb : vneigh[a])
            if (!vdead[nb])
                tailLocks[nb] = tailStamp;

        for (int nb : vneigh[b])
            if (!vdead[nb])
                tailLocks[nb] = tailStamp;
    }

    struct EBC
    {
        int a, b;
        double cost;
        bool operator<(const EBC &o) const { return cost < o.cost; }
    };

    int runTailBatch()
    {

        if (!tailMode())
            return 0;

        if (++tailStamp == INT_MAX)
        {
            fill(tailLocks.begin(), tailLocks.end(), 0);
            tailStamp = 1;
        }

        vector<EBC> cands;
        cands.reserve(HParam_TailBatchScanEdges / 4);

        int scanned = 0, visited = 0;

        for (; visited < (int)verts.size() && scanned < HParam_TailBatchScanEdges; ++visited)
        {

            int a = (tailCursor + visited) % (int)verts.size();
            if (vdead[a])
                continue;

            for (int b : vneigh[a])
            {
                if (scanned >= HParam_TailBatchScanEdges)
                    break;
                if (b <= a || vdead[b])
                    continue;
                ++scanned;

                double cc = cheapEdgeCost(a, b);
                if (!(cc <= costCap))
                    continue;
                cands.push_back({a, b, cc});
            }
        }

        tailCursor = (tailCursor + max(1, visited)) % (int)verts.size();

        if (cands.empty())
            return 0;

        sort(cands.begin(), cands.end());

        vector<EBC> sel;
        sel.reserve(HParam_TailBatchTargetAccepts);

        for (auto &e : cands)
        {
            if ((int)sel.size() >= HParam_TailBatchTargetAccepts)
                break;
            if (!edgeExists(e.a, e.b))
                continue;
            if (!batchFree(e.a, e.b))
                continue;
            lockBatch(e.a, e.b);
            sel.push_back(e);
        }

        int acc = 0;

        for (auto &e : sel)
        {

            if (accepted >= collapseLimit || elapsed() > HParam_TailBatchStopElapsed)
                break;

            if (!edgeExists(e.a, e.b))
                continue;

            if (countCommonFaces(e.a, e.b) != 2)
                continue;

            if (countCommonNeighbors(e.a, e.b) != 2)
                continue;

            auto best = computeBestValid(e.a, e.b);
            if (!best.valid() || best.cost > costCap)
                continue;

            applyCollapse(best.absorbed, best.kept, best.position, best.mergedRadius);
            ++accepted;
            ++acc;
        }

        return acc;
    }

    void collapseLoop()
    {

        int tick = 0;

        while (accepted < collapseLimit && (!pq.empty() || tailMode()))
        {

            if ((++tick & 8191) == 0 && elapsed() > HParam_TimeBudgetSeconds)
                break;

            if (tailMode() && lastFailBatch != accepted)
            {
                int ba = runTailBatch();
                if (ba > 0)
                    continue;
                lastFailBatch = accepted;
            }

            if (pq.empty())
                break;

            auto c = pq.top();
            pq.pop();

            int a = c.absorbed, b = c.kept;

            if (!edgeExists(a, b))
                continue;

            if (c.versionAbsorbed != vver[a] || c.versionKept != vver[b])
            {
                auto fr = computeQueueCandidate(a, b);
                if (fr.valid())
                    pq.push(fr);
                continue;
            }

            if (c.cost > costCap)
                break;

            if (countCommonFaces(a, b) != 2)
                continue;

            if (countCommonNeighbors(a, b) != 2)
                continue;

            auto best = computeBestValid(a, b);
            if (!best.valid() || best.cost > costCap)
                continue;

            applyCollapse(best.absorbed, best.kept, best.position, best.mergedRadius);
            ++accepted;
        }
    }

    void applyCollapse(int ab, int kp, const Vec3 &np, double nr)
    {

        verts[kp] = np;
        crad[kp] = nr;
        crad[ab] = 0;
        vdead[ab] = 1;
        ++vver[ab];
        ++vver[kp];

        auto abFaces = vfaces[ab];
        vector<int> dead;
        dead.reserve(4);

        for (int fi : abFaces)
        {
            if (fdead[fi])
                continue;
            bool touched = false;

            for (int k = 0; k < 3; ++k)
                if (faces[fi].v[k] == ab)
                {
                    faces[fi].v[k] = kp;
                    touched = true;
                }

            if (!touched)
                continue;

            if (faces[fi].v[0] == faces[fi].v[1] || faces[fi].v[1] == faces[fi].v[2] || faces[fi].v[0] == faces[fi].v[2])
            {
                fdead[fi] = 1;
                dead.push_back(fi);
            }

            else
                vfaces[kp].push_back(fi);
        }

        for (int fi : dead)
            for (int k = 0; k < 3; ++k)
            {
                int v = faces[fi].v[k];
                if (v >= 0 && v < (int)vfaces.size())
                    eraseVal(vfaces[v], fi);
            }

        vfaces[ab].clear();

        if (kp < (int)vmoment.size() && ab < (int)vmoment.size())
            vmoment[kp] += vmoment[ab];

        if (MEMLESS)
        {

            Quadric fresh;

            for (int fi : vfaces[kp])
            {
                if (fdead[fi])
                    continue;
                const Face &f = faces[fi];

                fresh += ((screenTier == 2 || screenTier == 4) ? weightedFaceQuadric(fi) : Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]));
            }

            if (screenTier == 4 && kp < (int)vmoment.size())
                fresh += vmoment[kp];
            vquad[kp] = fresh;
        }
        else
        {

            vquad[kp] += vquad[ab];
        }

        for (int nb : vneigh[ab])
        {
            if (nb == kp || vdead[nb])
                continue;
            vneigh[nb].erase(ab);
            vneigh[nb].insert(kp);
            vneigh[kp].insert(nb);
        }

        vneigh[ab].clear();
        vneigh[kp].erase(ab);
        vneigh[kp].erase(kp);

        for (int nb : vneigh[kp])
        {
            if (nb == kp || vdead[nb])
                continue;
            auto c = computeQueueCandidate(kp, nb);
            if (c.valid())
                pq.push(c);
        }
    }

    // ---------- Star-delete retriangulation post-pass ----------

    static double clampDouble(double x, double lo, double hi) { return x < lo ? lo : (x > hi ? hi : x); }

    static double pointTriangleDistance2(const Vec3 &p, const Vec3 &a, const Vec3 &b, const Vec3 &c)
    {

        Vec3 ab = b - a, ac = c - a, ap = p - a;

        double d1 = dot(ab, ap), d2 = dot(ac, ap);

        if (d1 <= 0.0 && d2 <= 0.0)
            return norm2(ap);

        Vec3 bp = p - b;

        double d3 = dot(ab, bp), d4 = dot(ac, bp);

        if (d3 >= 0.0 && d4 <= d3)
            return norm2(bp);

        double vc = d1 * d4 - d3 * d2;

        if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
        {
            double v = d1 / (d1 - d3);
            Vec3 q = a + ab * v;
            return norm2(p - q);
        }

        Vec3 cp = p - c;

        double d5 = dot(ab, cp), d6 = dot(ac, cp);

        if (d6 >= 0.0 && d5 <= d6)
            return norm2(cp);

        double vb = d5 * d2 - d1 * d6;

        if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
        {
            double w = d2 / (d2 - d6);
            Vec3 q = a + ac * w;
            return norm2(p - q);
        }

        double va = d3 * d6 - d5 * d4;

        if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0)
        {
            double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            Vec3 q = b + (c - b) * w;
            return norm2(p - q);
        }

        Vec3 n = cross(ab, ac);

        double nn = norm2(n);

        if (nn < CParam_MinNormalNorm * CParam_MinNormalNorm)
            return CParam_Inf;

        double dist = dot(p - a, n);

        return (dist * dist) / nn;
    }

    Vec3 faceNormalRaw(int fi) const
    {

        const Face &f = faces[fi];

        return cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
    }

    bool faceHasVertex(int fi, int v) const
    {

        const Face &f = faces[fi];

        return f.v[0] == v || f.v[1] == v || f.v[2] == v;
    }

    struct StarParams
    {

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

    int originalTier() const
    {

        if (nV >= 1000000)
            return 6;

        if (nV >= 350000)
            return 5;

        if (nV >= 45000)
            return 4;

        if (nV >= 35000)
            return 3;

        if (nV >= 20000)
            return 2;

        return 1;
    }

    StarParams starParams() const
    {

        StarParams p1 = {12, 0.160, 0.220, 1.18, 0.0400, 30000, 820000, 8, 0.90, 6.20};

        StarParams p2 = {5, 0.004, 0.006, 0.40, 0.0015, 900, 90000, 1, 0.22, 0.75};

        StarParams p3 = {6, 0.008, 0.012, 0.52, 0.0030, 1800, 140000, 2, 0.32, 1.20};

        StarParams p4 = {6, 0.010, 0.014, 0.58, 0.0035, 2200, 150000, 2, 0.35, 1.35};

        StarParams p5 = {5, 0.006, 0.009, 0.46, 0.0022, 1300, 115000, 1, 0.27, 0.95};

        StarParams p6 = {6, 0.008, 0.012, 0.52, 0.0030, 1800, 140000, 2, 0.32, 1.20};

        int tier = originalTier();

        if (tier == 1)
            return p1;

        if (tier == 2)
            return p2;

        if (tier == 3)
            return p3;

        if (tier == 4)
            return p4;

        if (tier == 5)
            return p5;

        return p6;
    }

    StarParams vegaSsimParams() const
    {

        StarParams p0 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        StarParams p2 = {8, 0.024, 0.040, 0.90, 0.0200, 1250, 175000, 1, 0.44, 1.95};

        StarParams p3 = {8, 0.026, 0.042, 0.92, 0.0180, 1250, 170000, 1, 0.45, 1.95};

        StarParams p4 = {8, 0.030, 0.048, 1.00, 0.0220, 1600, 190000, 1, 0.50, 2.20};

        int tier = originalTier();

        if (tier == 2)
            return p2;

        if (tier == 3)
            return p3;

        if (tier == 4)
            return p0;

        return p0;
    }

    double vegaSsimMin() const
    {

        int tier = originalTier();

        if (tier == 2)
            return 0.95; // Sharon v3: Goji v98 value

        if (tier == 3)
            return 0.94; // Sharon v3: Goji v98 value

        if (tier == 4)
            return 1.01;

        return 1.01;
    }

    double vegaMaxDamage() const
    {

        int tier = originalTier();

        if (tier == 2)
            return 0.052; // Sharon v3

        if (tier == 3)
            return 0.057; // Sharon v3

        if (tier == 4)
            return 0.0;

        return 0.0;
    }

    bool orientedRingForVertex(int v, vector<int> &ring, vector<int> &inc) const
    {

        StarParams sp = starParams();

        return orientedRingForVertexWithParams(v, sp, ring, inc);
    }

    bool orientedRingForVertexWithParams(int v, const StarParams &sp, vector<int> &ring, vector<int> &inc) const
    {

        ring.clear();
        inc.clear();

        if (sp.maxValence <= 0)
            return false;

        if (v < 0 || v >= (int)vfaces.size() || vdead[v])
            return false;

        for (int fi : vfaces[v])
        {
            if (fdead[fi])
                continue;
            if (faceHasVertex(fi, v))
                inc.push_back(fi);
        }

        int m = (int)inc.size();

        if (m < 3 || m > sp.maxValence)
            return false;

        vector<pair<int, int>> dir;
        dir.reserve(m);

        for (int fi : inc)
        {

            const Face &f = faces[fi];

            int pos = -1;

            for (int k = 0; k < 3; ++k)
                if (f.v[k] == v)
                    pos = k;

            if (pos < 0)
                return false;

            int a = f.v[(pos + 1) % 3], b = f.v[(pos + 2) % 3];

            if (a == b || a == v || b == v || vdead[a] || vdead[b])
                return false;

            dir.push_back({a, b});
        }

        for (int i = 0; i < m; ++i)
            for (int j = i + 1; j < m; ++j)
            {

                if (dir[i].first == dir[j].first)
                    return false;

                if (dir[i].second == dir[j].second)
                    return false;
            }

        int start = dir[0].first, cur = start;

        ring.push_back(start);

        for (int step = 0; step < m; ++step)
        {

            int nxt = -1;

            for (auto &e : dir)
                if (e.first == cur)
                {
                    nxt = e.second;
                    break;
                }

            if (nxt < 0)
                return false;

            if (step == m - 1)
            {
                if (nxt != start)
                    return false;
            }

            else
            {
                for (int x : ring)
                    if (x == nxt)
                        return false;
                ring.push_back(nxt);
                cur = nxt;
            }
        }

        return (int)ring.size() == m;
    }

    bool activeFaceWithSameKey(int a, int b, int c, const vector<int> &skip) const
    {

        array<int, 3> key = {a, b, c};
        sort(key.begin(), key.end());

        for (int fi : vfaces[a])
        {

            if (fdead[fi])
                continue;

            bool skipFace = false;

            for (int s : skip)
                if (s == fi)
                {
                    skipFace = true;
                    break;
                }

            if (skipFace)
                continue;

            const Face &f = faces[fi];

            array<int, 3> k2 = {f.v[0], f.v[1], f.v[2]};
            sort(k2.begin(), k2.end());

            if (k2 == key)
                return true;
        }

        return false;
    }

    struct StarCandidate
    {

        int v = -1, root = 0;
        double score = CParam_Inf;

        bool valid() const { return v >= 0 && score < CParam_Inf; }

        bool operator<(const StarCandidate &o) const { return score < o.score; }
    };

    bool evaluateStarRoot(int v, const vector<int> &ring, const vector<int> &inc, int root, double oldDev, const Vec3 &avgN, StarCandidate &out) const
    {

        StarParams sp = starParams();

        return evaluateStarRootWithParams(v, ring, inc, root, oldDev, avgN, sp, out);
    }

    bool evaluateStarRootWithParams(int v, const vector<int> &ring, const vector<int> &inc, int root, double oldDev, const Vec3 &avgN, const StarParams &sp, StarCandidate &out) const
    {

        int m = (int)ring.size();

        vector<int> rr;
        rr.reserve(m);

        for (int i = 0; i < m; ++i)
            rr.push_back(ring[(root + i) % m]);

        int r0 = rr[0];

        for (int i = 2; i <= m - 2; ++i)
        {
            if (vneigh[r0].contains(rr[i]))
                return false;
        }

        double maxNewDev = 0.0;
        double minDist2 = CParam_Inf;

        for (int i = 1; i < m - 1; ++i)
        {

            int a = rr[0], b = rr[i], c = rr[i + 1];

            if (a == b || b == c || a == c)
                return false;

            if (activeFaceWithSameKey(a, b, c, inc))
                return false;

            Vec3 n = cross(verts[b] - verts[a], verts[c] - verts[a]);

            double nl = norm(n);

            if (nl < CParam_MinNormalNorm)
                return false;

            Vec3 un = n / nl;

            double d = clampDouble(dot(un, avgN), -1.0, 1.0);

            if (d <= 0.0)
                return false;

            maxNewDev = max(maxNewDev, 1.0 - d);

            if (maxNewDev > sp.maxNewDev)
                return false;

            minDist2 = min(minDist2, pointTriangleDistance2(verts[v], verts[a], verts[b], verts[c]));
        }

        double dist = sqrt(max(0.0, minDist2));

        if (crad[v] + dist > hausd * sp.distFrac)
            return false;

        out.v = v;
        out.root = root;

        out.score = (crad[v] + dist) / (hausd + CParam_MinNormalNorm) + 0.35 * oldDev + 0.25 * maxNewDev + 1e-4 * m;

        return true;
    }

    StarCandidate computeStarCandidate(int v) const
    {

        StarParams sp = starParams();

        return computeStarCandidateWithParams(v, sp);
    }

    StarCandidate computeStarCandidateWithParams(int v, const StarParams &sp) const
    {

        StarCandidate best;

        vector<int> ring, inc;

        if (!orientedRingForVertexWithParams(v, sp, ring, inc))
            return best;

        Vec3 avg;
        double areaSum = 0.0, oldDev = 0.0;

        for (int fi : inc)
        {

            Vec3 n = faceNormalRaw(fi);

            double nl = norm(n);

            if (nl < CParam_MinNormalNorm)
                return best;

            avg = avg + n;
            areaSum += 0.5 * nl;
        }

        double al = norm(avg);

        if (al < CParam_MinNormalNorm || areaSum <= 0.0)
            return best;

        avg = avg / al;

        for (int fi : inc)
        {

            Vec3 n = faceNormalRaw(fi);
            double nl = norm(n);
            Vec3 un = n / nl;

            double d = clampDouble(dot(un, avg), -1.0, 1.0);

            if (d <= 0.0)
                return best;

            oldDev = max(oldDev, 1.0 - d);
        }

        if (oldDev > sp.maxOldDev)
            return best;

        for (int root = 0; root < (int)ring.size(); ++root)
        {

            StarCandidate c;

            if (evaluateStarRootWithParams(v, ring, inc, root, oldDev, avg, sp, c) && c.score < best.score)
                best = c;
        }

        return best;
    }

    struct RootNudgeParams
    {

        double frac = 0.0, maxMoveDiag = 0.0, radiusFrac = 0.0, minDot = 1.01;

        int maxIncident = 0;
    };

    RootNudgeParams rootNudgeParams() const
    {

        RootNudgeParams z;

        if (!HParam_EnableRootNudge)
            return z;

        int tier = originalTier();

        if (HParam_RootNudgeProfile == 1)
        {

            if (tier == 4)
                return {0.055, 0.0012, 0.96, 0.992, 34};
        }
        else if (HParam_RootNudgeProfile == 2)
        {

            if (tier == 2)
                return {0.035, 0.0008, 0.92, 0.995, 28};

            if (tier == 4)
                return {0.050, 0.0011, 0.94, 0.993, 34};
        }
        else if (HParam_RootNudgeProfile == 3)
        {

            if (tier == 2)
                return {0.045, 0.0010, 0.94, 0.993, 30};

            if (tier == 3)
                return {0.030, 0.0007, 0.90, 0.996, 30};

            if (tier == 4)
                return {0.065, 0.0015, 0.98, 0.990, 36};
        }

        return z;
    }

    bool tryRootNudgeToward(int root, const Vec3 &target, const RootNudgeParams &rp)
    {

        if (rp.frac <= 0.0 || root < 0 || root >= (int)vdead.size() || vdead[root])
            return false;

        if ((int)vfaces[root].size() > rp.maxIncident)
            return false;

        Vec3 cur = verts[root];

        Vec3 delta = (target - cur) * rp.frac;

        double dl = norm(delta);

        if (dl < CParam_MinNormalNorm)
            return false;

        double cap = diag * rp.maxMoveDiag;

        if (cap <= 0.0)
            return false;

        if (dl > cap)
        {
            delta = delta * (cap / dl);
            dl = cap;
        }

        if (crad[root] + dl > hausd * rp.radiusFrac)
            return false;

        Vec3 np = cur + delta;

        for (int fi : vfaces[root])
        {

            if (fdead[fi])
                continue;

            const Face &f = faces[fi];

            Vec3 oldN = cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);

            double oldLen = norm(oldN);

            if (oldLen < CParam_MinNormalNorm)
                return false;

            Vec3 p[3] = {verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]};

            for (int k = 0; k < 3; ++k)
                if (f.v[k] == root)
                    p[k] = np;

            Vec3 newN = cross(p[1] - p[0], p[2] - p[0]);

            double newLen = norm(newN);

            if (newLen < CParam_MinNormalNorm)
                return false;

            double d = clampDouble(dot(oldN / oldLen, newN / newLen), -1.0, 1.0);

            if (d < rp.minDot)
                return false;
        }

        verts[root] = np;

        crad[root] += dl;

        ++vver[root];

        return true;
    }

    bool applyStarDelete(int v, int root)
    {

        StarCandidate best = computeStarCandidate(v);

        if (!best.valid())
            return false;

        return applyStarDeleteWithParams(v, best.root, starParams());
    }

    bool applyStarDeleteWithParams(int v, int root, const StarParams &sp)
    {

        StarCandidate best = computeStarCandidateWithParams(v, sp);

        if (!best.valid())
            return false;

        root = best.root;

        vector<int> ring, inc;

        if (!orientedRingForVertexWithParams(v, sp, ring, inc))
            return false;

        int m = (int)ring.size();

        vector<int> rr;
        rr.reserve(m);

        for (int i = 0; i < m; ++i)
            rr.push_back(ring[(root + i) % m]);

        Vec3 removedPos = verts[v];

        for (int fi : inc)
        {

            if (fdead[fi])
                continue;

            Face old = faces[fi];
            fdead[fi] = 1;

            for (int k = 0; k < 3; ++k)
            {
                int u = old.v[k];
                if (u >= 0 && u < (int)vfaces.size())
                    eraseVal(vfaces[u], fi);
            }
        }

        for (int nb : ring)
            if (nb >= 0 && nb < (int)vneigh.size())
                vneigh[nb].erase(v);

        vneigh[v].clear();
        vfaces[v].clear();
        vdead[v] = 1;
        crad[v] = 0.0;
        ++vver[v];

        for (int i = 1; i < m - 1; ++i)
        {

            Face nf;
            nf.v[0] = rr[0];
            nf.v[1] = rr[i];
            nf.v[2] = rr[i + 1];

            int fi = (int)faces.size();

            faces.push_back(nf);
            fdead.push_back(0);

            for (int k = 0; k < 3; ++k)
                vfaces[nf.v[k]].push_back(fi);

            for (int k = 0; k < 3; ++k)
            {
                int a = nf.v[k], b = nf.v[(k + 1) % 3];
                if (a != b)
                {
                    vneigh[a].insert(b);
                    vneigh[b].insert(a);
                }
            }
        }

        RootNudgeParams rp = rootNudgeParams();

        if (rp.frac > 0.0 && !rr.empty())
            tryRootNudgeToward(rr[0], removedPos, rp);

        return true;
    }

    bool weldTierEnabled() const
    {

        int t = originalTier();
        return t == 4 || t == 5 || t == 6;
    }

    StarParams weldParams() const
    {

        int t = originalTier();
        if (t == 4)
            return {10, 0.015, 0.024, 0.66, 0.0110, 1500, 280000, 1, 0.52, 2.30};
        if (t == 5)
            return {12, 0.020, 0.032, 0.80, 0.0150, 1500, 280000, 1, 0.60, 2.80};
        if (t == 6)
            return {8, 0.024, 0.038, 0.95, 0.0180, 2500, 280000, 1, 0.68, 5.00};

        return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }

    bool valenceWeldRingForParams(int v, const StarParams &sp) const
    {

        vector<int> ring, inc;

        return orientedRingForVertexWithParams(v, sp, ring, inc) && (int)ring.size() >= 4 && (int)ring.size() <= sp.maxValence;
    }

    void valenceWeldPass()
    {

        if (!weldTierEnabled())
            return;

        StarParams sp = weldParams();

        if (sp.maxValence <= 0 || sp.hardCap <= 0 || sp.scanVertices <= 0)
            return;

        double timeLeft = HParam_TimeBudgetSeconds - elapsed();

        if (timeLeft < 0.45)
            return;

        double stopTime = elapsed() + min(sp.maxSeconds, timeLeft * sp.timeFrac);

        int maxExtra = min(sp.hardCap, max(0, (int)floor(nV * sp.extraFrac)));

        if (maxExtra <= 0)
            return;

        vector<StarCandidate> cands;

        cands.reserve(min(sp.scanVertices, maxExtra * 12 + 512));

        int scanned = 0;

        for (int v = 0; v < (int)verts.size() && scanned < sp.scanVertices && elapsed() < stopTime; ++v)
        {

            if (vdead[v])
                continue;

            ++scanned;

            if (!valenceWeldRingForParams(v, sp))
                continue;

            StarCandidate c = computeStarCandidateWithParams(v, sp);

            if (c.valid())
                cands.push_back(c);
        }

        if (cands.empty())
            return;

        sort(cands.begin(), cands.end());

        int extra = 0;

        for (const StarCandidate &c : cands)
        {

            if (extra >= maxExtra || elapsed() >= stopTime)
                break;

            if (c.v < 0 || c.v >= (int)vdead.size() || vdead[c.v])
                continue;

            if (!valenceWeldRingForParams(c.v, sp))
                continue;

            if (applyStarDeleteWithParams(c.v, c.root, sp))
            {

                ++accepted;

                ++extra;
            }
        }
    }

    struct PairDiskCandidate
    {

        int a = -1, b = -1, root = 0;

        double score = CParam_Inf;

        vector<int> boundary;

        vector<int> patchFaces;

        bool valid() const { return a >= 0 && b >= 0 && score < CParam_Inf && !boundary.empty() && !patchFaces.empty(); }

        bool operator<(const PairDiskCandidate &o) const { return score < o.score; }
    };

    StarParams pairDiskParams() const
    {

        int t = originalTier();
        if (t == 4)
            return {8, 0.018, 0.029, 0.66, 0.0025, 90, 120000, 1, 0.28, 0.85};
        if (t == 5)
            return {8, 0.024, 0.038, 0.78, 0.0035, 90, 120000, 1, 0.30, 0.95};
        if (t == 6)
            return {8, 0.030, 0.045, 0.95, 0.0045, 90, 120000, 1, 0.34, 1.05};

        return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }

    static long long edgeKeyFor(int a, int b)
    {

        if (a > b)
            swap(a, b);

        return ((long long)a << 32) ^ (unsigned int)b;
    }

    bool orderBoundaryCycle(const vector<pair<int, int>> &edges, vector<int> &ring) const
    {

        ring.clear();

        if (edges.size() < 4 || edges.size() > 16)
            return false;

        vector<int> vertsLocal;

        vertsLocal.reserve(edges.size() * 2);

        for (auto &e : edges)
        {
            vertsLocal.push_back(e.first);
            vertsLocal.push_back(e.second);
        }

        sort(vertsLocal.begin(), vertsLocal.end());

        vertsLocal.erase(unique(vertsLocal.begin(), vertsLocal.end()), vertsLocal.end());

        if (vertsLocal.size() != edges.size())
            return false;

        vector<vector<int>> adj(vertsLocal.size());

        for (auto &e : edges)
        {

            int ia = (int)(lower_bound(vertsLocal.begin(), vertsLocal.end(), e.first) - vertsLocal.begin());

            int ib = (int)(lower_bound(vertsLocal.begin(), vertsLocal.end(), e.second) - vertsLocal.begin());

            if (ia < 0 || ib < 0 || ia == ib || ia >= (int)vertsLocal.size() || ib >= (int)vertsLocal.size())
                return false;

            adj[ia].push_back(ib);

            adj[ib].push_back(ia);
        }

        for (auto &v : adj)
            if (v.size() != 2)
                return false;

        int start = 0, prev = -1, cur = start;

        for (int step = 0; step < (int)vertsLocal.size(); ++step)
        {

            ring.push_back(vertsLocal[cur]);

            int n0 = adj[cur][0], n1 = adj[cur][1];

            int nxt = (n0 == prev) ? n1 : n0;

            prev = cur;

            cur = nxt;

            if (cur == start)
                return step + 1 == (int)vertsLocal.size();
        }

        return false;
    }

    double triSetDistanceSamples(const vector<array<Vec3, 3>> &src, const vector<array<Vec3, 3>> &dst) const
    {

        if (src.empty() || dst.empty())
            return CParam_Inf;

        double worst = 0.0;

        for (const auto &t : src)
        {

            Vec3 samples[7] = {

                t[0], t[1], t[2],

                (t[0] + t[1]) * 0.5,

                (t[1] + t[2]) * 0.5,

                (t[2] + t[0]) * 0.5,

                (t[0] + t[1] + t[2]) / 3.0

            };

            for (const Vec3 &p : samples)
            {

                double best = CParam_Inf;

                for (const auto &q : dst)
                    best = min(best, pointTriangleDistance2(p, q[0], q[1], q[2]));

                worst = max(worst, best);
            }
        }

        return worst;
    }

    bool buildPairDiskPatch(int a, int b, const StarParams &sp, PairDiskCandidate &out) const
    {

        out = PairDiskCandidate();

        if (!edgeExists(a, b) || sp.maxValence <= 0)
            return false;

        vector<int> patch;

        patch.reserve(vfaces[a].size() + vfaces[b].size());

        for (int fi : vfaces[a])
            if (!fdead[fi])
                patch.push_back(fi);

        for (int fi : vfaces[b])
            if (!fdead[fi])
                patch.push_back(fi);

        sort(patch.begin(), patch.end());

        patch.erase(unique(patch.begin(), patch.end()), patch.end());

        if (patch.size() < 4 || patch.size() > 24)
            return false;

        map<long long, pair<int, int>> edgeEnds;

        map<long long, int> edgeCount;

        for (int fi : patch)
        {

            const Face &f = faces[fi];

            bool touches = false;

            for (int k = 0; k < 3; ++k)
                if (f.v[k] == a || f.v[k] == b)
                    touches = true;

            if (!touches)
                return false;

            for (int k = 0; k < 3; ++k)
            {

                int x = f.v[k], y = f.v[(k + 1) % 3];

                long long key = edgeKeyFor(x, y);

                edgeEnds[key] = {min(x, y), max(x, y)};

                ++edgeCount[key];
            }
        }

        vector<pair<int, int>> boundaryEdges;

        for (auto &kv : edgeCount)
        {

            if (kv.second != 1)
                continue;

            int x = edgeEnds[kv.first].first, y = edgeEnds[kv.first].second;

            if (x == a || x == b || y == a || y == b)
                return false;

            if (vdead[x] || vdead[y])
                return false;

            boundaryEdges.push_back({x, y});
        }

        vector<int> ring;

        if (!orderBoundaryCycle(boundaryEdges, ring))
            return false;

        int m = (int)ring.size();

        if (m < 4 || m > sp.maxValence)
            return false;

        Vec3 avg;

        double oldDev = 0.0;

        for (int fi : patch)
        {

            Vec3 n = faceNormalRaw(fi);

            double nl = norm(n);

            if (nl < CParam_MinNormalNorm)
                return false;

            avg = avg + n;
        }

        double al = norm(avg);

        if (al < CParam_MinNormalNorm)
            return false;

        avg = avg / al;

        for (int fi : patch)
        {

            Vec3 n = faceNormalRaw(fi);

            double nl = norm(n);

            Vec3 un = n / nl;

            double d = clampDouble(dot(un, avg), -1.0, 1.0);

            if (d <= 0.0)
                return false;

            oldDev = max(oldDev, 1.0 - d);
        }

        if (oldDev > sp.maxOldDev)
            return false;

        vector<array<Vec3, 3>> oldTris;

        oldTris.reserve(patch.size());

        for (int fi : patch)
        {

            const Face &f = faces[fi];

            oldTris.push_back({verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]});
        }

        PairDiskCandidate best;

        for (int root = 0; root < m; ++root)
        {

            vector<int> rr;

            rr.reserve(m);

            for (int i = 0; i < m; ++i)
                rr.push_back(ring[(root + i) % m]);

            int r0 = rr[0];

            bool ok = true;

            for (int i = 2; i <= m - 2 && ok; ++i)
                if (vneigh[r0].contains(rr[i]))
                    ok = false;

            if (!ok)
                continue;

            double maxNewDev = 0.0;

            vector<array<Vec3, 3>> newTris;

            newTris.reserve(max(0, m - 2));

            for (int i = 1; i < m - 1 && ok; ++i)
            {

                int x = rr[0], y = rr[i], z = rr[i + 1];

                if (x == y || y == z || x == z)
                {
                    ok = false;
                    break;
                }

                if (activeFaceWithSameKey(x, y, z, patch))
                {
                    ok = false;
                    break;
                }

                Vec3 n = cross(verts[y] - verts[x], verts[z] - verts[x]);

                double nl = norm(n);

                if (nl < CParam_MinNormalNorm)
                {
                    ok = false;
                    break;
                }

                Vec3 un = n / nl;

                double d = clampDouble(dot(un, avg), -1.0, 1.0);

                if (d <= 0.0)
                {
                    ok = false;
                    break;
                }

                maxNewDev = max(maxNewDev, 1.0 - d);

                if (maxNewDev > sp.maxNewDev)
                {
                    ok = false;
                    break;
                }

                newTris.push_back({verts[x], verts[y], verts[z]});
            }

            if (!ok || newTris.empty())
                continue;

            double worst2 = max(triSetDistanceSamples(oldTris, newTris), triSetDistanceSamples(newTris, oldTris));

            if (!isfinite(worst2))
                continue;

            double worst = sqrt(max(0.0, worst2));

            double removedCover = max(crad[a], crad[b]) + worst;

            if (removedCover > hausd * sp.distFrac)
                continue;

            PairDiskCandidate cand;

            cand.a = a;
            cand.b = b;
            cand.root = root;
            cand.boundary = ring;
            cand.patchFaces = patch;

            cand.score = removedCover / (hausd + CParam_MinNormalNorm) + 0.35 * oldDev + 0.25 * maxNewDev + 1e-4 * m;

            if (cand.score < best.score)
                best = cand;
        }

        if (!best.valid())
            return false;

        out = best;

        return true;
    }

    bool applyPairDiskDelete(const PairDiskCandidate &c, const StarParams &sp)
    {

        if (c.a < 0 || c.b < 0 || vdead[c.a] || vdead[c.b] || !edgeExists(c.a, c.b))
            return false;

        PairDiskCandidate fresh;

        if (!buildPairDiskPatch(c.a, c.b, sp, fresh))
            return false;

        if (!fresh.valid())
            return false;

        int m = (int)fresh.boundary.size();

        vector<int> rr;

        rr.reserve(m);

        for (int i = 0; i < m; ++i)
            rr.push_back(fresh.boundary[(fresh.root + i) % m]);

        for (int fi : fresh.patchFaces)
        {

            if (fdead[fi])
                continue;

            Face old = faces[fi];

            fdead[fi] = 1;

            for (int k = 0; k < 3; ++k)
            {

                int u = old.v[k];

                if (u >= 0 && u < (int)vfaces.size())
                    eraseVal(vfaces[u], fi);
            }
        }

        vector<int> na = vneigh[fresh.a].data, nb = vneigh[fresh.b].data;

        for (int x : na)
            if (x >= 0 && x < (int)vneigh.size())
                vneigh[x].erase(fresh.a);

        for (int x : nb)
            if (x >= 0 && x < (int)vneigh.size())
                vneigh[x].erase(fresh.b);

        vneigh[fresh.a].clear();
        vneigh[fresh.b].clear();

        vfaces[fresh.a].clear();
        vfaces[fresh.b].clear();

        vdead[fresh.a] = 1;
        vdead[fresh.b] = 1;

        crad[fresh.a] = 0.0;
        crad[fresh.b] = 0.0;

        ++vver[fresh.a];
        ++vver[fresh.b];

        for (int i = 1; i < m - 1; ++i)
        {

            Face nf;
            nf.v[0] = rr[0];
            nf.v[1] = rr[i];
            nf.v[2] = rr[i + 1];

            int fi = (int)faces.size();

            faces.push_back(nf);
            fdead.push_back(0);

            for (int k = 0; k < 3; ++k)
                vfaces[nf.v[k]].push_back(fi);

            for (int k = 0; k < 3; ++k)
            {

                int x = nf.v[k], y = nf.v[(k + 1) % 3];

                if (x != y)
                {
                    vneigh[x].insert(y);
                    vneigh[y].insert(x);
                }
            }
        }

        return true;
    }

    void pairDiskPass()
    {

        StarParams sp = pairDiskParams();

        if (sp.maxValence <= 0 || sp.hardCap <= 0 || sp.scanVertices <= 0)
            return;

        double timeLeft = HParam_TimeBudgetSeconds - elapsed();

        if (timeLeft < 0.55)
            return;

        double stopTime = elapsed() + min(sp.maxSeconds, timeLeft * sp.timeFrac);

        int maxExtra = min(sp.hardCap, max(0, (int)floor(nV * sp.extraFrac)));

        if (maxExtra <= 0)
            return;

        vector<PairDiskCandidate> cands;

        cands.reserve(min(sp.scanVertices, maxExtra * 8 + 256));

        int scanned = 0;

        for (int a = 0; a < (int)verts.size() && scanned < sp.scanVertices && elapsed() < stopTime; ++a)
        {

            if (vdead[a])
                continue;

            for (int b : vneigh[a])
            {

                if (scanned >= sp.scanVertices || elapsed() >= stopTime)
                    break;

                if (b <= a || vdead[b])
                    continue;

                ++scanned;

                PairDiskCandidate c;

                if (buildPairDiskPatch(a, b, sp, c))
                    cands.push_back(c);
            }
        }

        if (cands.empty())
            return;

        sort(cands.begin(), cands.end());

        int extra = 0;

        for (const PairDiskCandidate &c : cands)
        {

            if (extra >= maxExtra || elapsed() >= stopTime)
                break;

            if (c.a < 0 || c.b < 0 || c.a >= (int)vdead.size() || c.b >= (int)vdead.size())
                continue;

            if (vdead[c.a] || vdead[c.b])
                continue;

            if (applyPairDiskDelete(c, sp))
            {

                accepted += 2;

                extra += 2;
            }
        }
    }

    struct VegaProj
    {
        double u = 0.0, v = 0.0, z = 0.0;
        bool ok = false;
    };

    struct VegaTri
    {
        Vec3 p[3];
    };

    struct VegaPixel
    {
        float n[3];
        float d;
        unsigned char fg;
    };

    struct VegaCandidate
    {

        int v = -1, root = 0;
        double ssim = 1.0, score = CParam_Inf;

        bool operator<(const VegaCandidate &o) const { return score < o.score; }
    };

    static VegaPixel vegaBackgroundPixel()
    {

        VegaPixel p;

        p.n[0] = 127.5f;
        p.n[1] = 127.5f;
        p.n[2] = 127.5f;

        p.d = 255.0f;
        p.fg = 0;

        return p;
    }

    void vegaCameraBasis(int view, Vec3 &eye, Vec3 &right, Vec3 &up, Vec3 &fwd) const
    {

        switch (view)
        {

        case 0:
            eye = Vec3(2.5, 0, 0);
            break;

        case 1:
            eye = Vec3(-2.5, 0, 0);
            break;

        case 2:
            eye = Vec3(0, 2.5, 0);
            break;

        case 3:
            eye = Vec3(0, -2.5, 0);
            break;

        case 4:
            eye = Vec3(0, 0, 2.5);
            break;

        default:
            eye = Vec3(0, 0, -2.5);
            break;
        }

        fwd = eye * (-1.0);

        double fl = norm(fwd);

        if (fl < CParam_MinNormalNorm)
            fwd = Vec3(0, 0, -1);
        else
            fwd = fwd / fl;

        Vec3 worldUp = (fabs(dot(fwd, Vec3(0, 0, 1))) > 0.9) ? Vec3(0, 1, 0) : Vec3(0, 0, 1);

        right = cross(worldUp, fwd);

        double rl = norm(right);

        if (rl < CParam_MinNormalNorm)
            right = Vec3(1, 0, 0);
        else
            right = right / rl;

        up = cross(fwd, right);

        double ul = norm(up);

        if (ul < CParam_MinNormalNorm)
            up = Vec3(0, 1, 0);
        else
            up = up / ul;
    }

    VegaProj vegaProjectPoint(const Vec3 &p, int view, int R) const
    {

        Vec3 eye, right, up, fwd;

        vegaCameraBasis(view, eye, right, up, fwd);

        Vec3 rel = p - eye;

        double x = dot(rel, right), y = dot(rel, up), z = dot(rel, fwd);

        if (z <= 1e-8)
            return {};

        double scale = double(R) / 1024.0;

        double f = 800.0 * scale;

        double c = 0.5 * double(R);

        return {f * x / z + c, f * y / z + c, z, true};
    }

    bool buildVegaStarPatchTris(int v, int root, const StarParams &sp, vector<VegaTri> &oldTris, vector<VegaTri> &newTris) const
    {

        oldTris.clear();
        newTris.clear();

        vector<int> ring, inc;

        if (!orientedRingForVertexWithParams(v, sp, ring, inc))
            return false;

        StarCandidate check = computeStarCandidateWithParams(v, sp);

        if (!check.valid())
            return false;

        root = check.root;

        for (int fi : inc)
        {

            const Face &f = faces[fi];

            VegaTri t;

            t.p[0] = verts[f.v[0]];
            t.p[1] = verts[f.v[1]];
            t.p[2] = verts[f.v[2]];

            oldTris.push_back(t);
        }

        int m = (int)ring.size();

        vector<int> rr;
        rr.reserve(m);

        for (int i = 0; i < m; ++i)
            rr.push_back(ring[(root + i) % m]);

        for (int i = 1; i < m - 1; ++i)
        {

            VegaTri t;

            t.p[0] = verts[rr[0]];
            t.p[1] = verts[rr[i]];
            t.p[2] = verts[rr[i + 1]];

            newTris.push_back(t);
        }

        return !oldTris.empty() && !newTris.empty();
    }

    double vegaPatchGeomFrac() const
    {

        int tier = originalTier();

        if (tier == 2)
            return 0.80;

        if (tier == 3)
            return 0.42;

        if (tier == 4)
            return 0.0;

        return 1e100;
    }

    double vegaSampleToTrisMaxDistance2(const vector<VegaTri> &src, const vector<VegaTri> &dst) const
    {

        if (src.empty() || dst.empty())
            return CParam_Inf;

        double worst = 0.0;

        for (const VegaTri &t : src)
        {

            Vec3 samples[7] = {

                t.p[0], t.p[1], t.p[2],

                (t.p[0] + t.p[1]) * 0.5,

                (t.p[1] + t.p[2]) * 0.5,

                (t.p[2] + t.p[0]) * 0.5,

                (t.p[0] + t.p[1] + t.p[2]) / 3.0

            };

            for (const Vec3 &p : samples)
            {

                double best = CParam_Inf;

                for (const VegaTri &q : dst)
                    best = min(best, pointTriangleDistance2(p, q.p[0], q.p[1], q.p[2]));

                worst = max(worst, best);
            }
        }

        return worst;
    }

    double vegaPatchDeviation(const vector<VegaTri> &oldTris, const vector<VegaTri> &newTris) const
    {

        double d2 = max(vegaSampleToTrisMaxDistance2(oldTris, newTris),

                        vegaSampleToTrisMaxDistance2(newTris, oldTris));

        if (!isfinite(d2))
            return CParam_Inf;

        return sqrt(max(0.0, d2));
    }

    static double vegaPixelChannel(const VegaPixel &p, int ch)
    {

        if (ch < 3)
            return p.n[ch];

        return p.d;
    }

    double vegaScalarSsim(const vector<VegaPixel> &a, const vector<VegaPixel> &b, int ch) const
    {

        double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
        int cnt = 0;

        int n = (int)a.size();

        for (int i = 0; i < n; ++i)
        {

            if (!a[i].fg && !b[i].fg)
                continue;

            double x = vegaPixelChannel(a[i], ch), y = vegaPixelChannel(b[i], ch);

            sx += x;
            sy += y;
            sxx += x * x;
            syy += y * y;
            sxy += x * y;
            ++cnt;
        }

        if (cnt < 4)
            return 1.0;

        double inv = 1.0 / double(cnt);

        double mx = sx * inv, my = sy * inv;

        double vx = max(0.0, sxx * inv - mx * mx), vy = max(0.0, syy * inv - my * my);

        double cov = sxy * inv - mx * my;

        double num = (2.0 * mx * my + HParam_VegaC1) * (2.0 * cov + HParam_VegaC2);

        double den = (mx * mx + my * my + HParam_VegaC1) * (vx + vy + HParam_VegaC2);

        if (den <= 0.0)
            return 1.0;

        return clampDouble(num / den, -1.0, 1.0);
    }

    double vegaBuffersSsim(const vector<VegaPixel> &a, const vector<VegaPixel> &b) const
    {

        int cnt = 0;

        for (int i = 0; i < (int)a.size(); ++i)
            if (a[i].fg || b[i].fg)
                ++cnt;

        if (cnt == 0)
            return 1.0;

        double sn = (vegaScalarSsim(a, b, 0) + vegaScalarSsim(a, b, 1) + vegaScalarSsim(a, b, 2)) / 3.0;

        double sd = vegaScalarSsim(a, b, 3);

        return HParam_VegaNormalDepthWeight * sn + (1.0 - HParam_VegaNormalDepthWeight) * sd;
    }

    bool renderVegaPatch(const vector<VegaTri> &tris, int view, int x0, int y0, int w, int h, vector<VegaPixel> &buf) const
    {

        VegaPixel bg = vegaBackgroundPixel();

        buf.assign(w * h, bg);

        vector<float> zbuf(w * h, numeric_limits<float>::infinity());

        const int R = HParam_VegaPatchResolution;

        for (const VegaTri &tri : tris)
        {

            VegaProj p0 = vegaProjectPoint(tri.p[0], view, R);

            VegaProj p1 = vegaProjectPoint(tri.p[1], view, R);

            VegaProj p2 = vegaProjectPoint(tri.p[2], view, R);

            if (!p0.ok || !p1.ok || !p2.ok)
                continue;

            double area2 = (p1.u - p0.u) * (p2.v - p0.v) - (p1.v - p0.v) * (p2.u - p0.u);

            if (fabs(area2) < 1e-12)
                continue;

            Vec3 nr = cross(tri.p[1] - tri.p[0], tri.p[2] - tri.p[0]);

            double nl = norm(nr);

            if (nl < CParam_MinNormalNorm)
                continue;

            nr = nr / nl;

            int bx0 = max(x0, (int)floor(min({p0.u, p1.u, p2.u})));

            int bx1 = min(x0 + w - 1, (int)ceil(max({p0.u, p1.u, p2.u})));

            int by0 = max(y0, (int)floor(min({p0.v, p1.v, p2.v})));

            int by1 = min(y0 + h - 1, (int)ceil(max({p0.v, p1.v, p2.v})));

            if (bx0 > bx1 || by0 > by1)
                continue;

            double den = (p1.v - p2.v) * (p0.u - p2.u) + (p2.u - p1.u) * (p0.v - p2.v);

            if (fabs(den) < 1e-18)
                continue;

            for (int py = by0; py <= by1; ++py)
            {

                for (int px = bx0; px <= bx1; ++px)
                {

                    double sx = px + 0.5, sy = py + 0.5;

                    double w0 = ((p1.v - p2.v) * (sx - p2.u) + (p2.u - p1.u) * (sy - p2.v)) / den;

                    double w1 = ((p2.v - p0.v) * (sx - p2.u) + (p0.u - p2.u) * (sy - p2.v)) / den;

                    double w2 = 1.0 - w0 - w1;

                    if (w0 < -1e-9 || w1 < -1e-9 || w2 < -1e-9)
                        continue;

                    double iz = w0 / p0.z + w1 / p1.z + w2 / p2.z;

                    if (iz <= 0.0)
                        continue;

                    double z = 1.0 / iz;

                    int idx = (py - y0) * w + (px - x0);

                    if (z < zbuf[idx])
                    {

                        zbuf[idx] = (float)z;

                        buf[idx].n[0] = (float)((nr.x + 1.0) * 127.5);

                        buf[idx].n[1] = (float)((nr.y + 1.0) * 127.5);

                        buf[idx].n[2] = (float)((nr.z + 1.0) * 127.5);

                        buf[idx].d = (float)z;

                        buf[idx].fg = 1;
                    }
                }
            }
        }

        return true;
    }

    double localVegaSsimForStarCandidate(int v, int root, const StarParams &sp) const
    {

        vector<VegaTri> oldTris, newTris;

        if (!buildVegaStarPatchTris(v, root, sp, oldTris, newTris))
            return -1.0;

        double dev = vegaPatchDeviation(oldTris, newTris);

        if (!(dev <= hausd * vegaPatchGeomFrac()))
            return -1.0;

        const int R = HParam_VegaPatchResolution;

        double total = 0.0;
        int usedViews = 0;

        vector<VegaPixel> a, b;

        for (int view = 0; view < 6; ++view)
        {

            double mnU = 1e100, mnV = 1e100, mxU = -1e100, mxV = -1e100;

            bool any = false;

            auto includeTri = [&](const VegaTri &t)
            {
                for (int k = 0; k < 3; ++k)
                {

                    VegaProj p = vegaProjectPoint(t.p[k], view, R);

                    if (!p.ok)
                        continue;

                    any = true;

                    mnU = min(mnU, p.u);
                    mxU = max(mxU, p.u);

                    mnV = min(mnV, p.v);
                    mxV = max(mxV, p.v);
                }
            };

            for (const VegaTri &t : oldTris)
                includeTri(t);

            for (const VegaTri &t : newTris)
                includeTri(t);

            if (!any)
                continue;

            int pad = HParam_VegaPatchPaddingPixels;

            int x0 = max(0, (int)floor(mnU) - pad);

            int y0 = max(0, (int)floor(mnV) - pad);

            int x1 = min(R - 1, (int)ceil(mxU) + pad);

            int y1 = min(R - 1, (int)ceil(mxV) + pad);

            if (x0 > x1 || y0 > y1)
                continue;

            int w = x1 - x0 + 1, h = y1 - y0 + 1;

            if (w * h > HParam_VegaPatchMaxPixels)
                return -1.0;

            renderVegaPatch(oldTris, view, x0, y0, w, h, a);

            renderVegaPatch(newTris, view, x0, y0, w, h, b);

            total += vegaBuffersSsim(a, b);

            ++usedViews;
        }

        if (usedViews == 0)
            return 1.0;

        return total / double(usedViews);
    }

    void vegaSsimStarPass()
    {

        StarParams sp = vegaSsimParams();

        if (sp.maxValence <= 0)
            return;

        double timeLeft = HParam_TimeBudgetSeconds - elapsed();

        if (timeLeft < 0.45)
            return;

        double stopTime = elapsed() + min(sp.maxSeconds, timeLeft * sp.timeFrac);

        int maxExtra = min(sp.hardCap, max(0, (int)floor(nV * sp.extraFrac)));

        if (maxExtra <= 0)
            return;

        double minS = vegaSsimMin();

        double maxDamage = vegaMaxDamage();

        vector<VegaCandidate> cands;

        cands.reserve(min(HParam_VegaCandidatePoolCap, maxExtra * 10 + 256));

        int scanned = 0, visited = 0, total = (int)verts.size();

        int start = (total > 0) ? (vegaCursor % total) : 0;

        for (; visited < total && scanned < sp.scanVertices && elapsed() < stopTime; ++visited)
        {

            int v = (start + visited) % total;

            if (vdead[v])
                continue;

            ++scanned;

            StarCandidate geom = computeStarCandidateWithParams(v, sp);

            if (!geom.valid())
                continue;

            double s = localVegaSsimForStarCandidate(geom.v, geom.root, sp);

            if (s < 0.0)
                continue;

            double damage = 1.0 - s;

            if (s < minS || damage > maxDamage)
                continue;

            VegaCandidate vc;

            vc.v = geom.v;
            vc.root = geom.root;
            vc.ssim = s;

            vc.score = damage + HParam_VegaScoreGeomWeight * geom.score;

            cands.push_back(vc);

            if ((int)cands.size() >= HParam_VegaCandidatePoolCap)
                break;
        }

        if (total > 0)
            vegaCursor = (start + max(1, visited)) % total;

        if (cands.empty())
            return;

        sort(cands.begin(), cands.end());

        int extra = 0;

        for (const VegaCandidate &c : cands)
        {

            if (extra >= maxExtra || elapsed() >= stopTime)
                break;

            if (c.v < 0 || c.v >= (int)vdead.size() || vdead[c.v])
                continue;

            double s = localVegaSsimForStarCandidate(c.v, c.root, sp);

            if (s < minS || 1.0 - s > maxDamage)
                continue;

            if (applyStarDeleteWithParams(c.v, c.root, sp))
            {

                ++accepted;

                ++extra;
            }
        }
    }

    void collapseInvisibleEdges()
    {

        StarParams sp = starParams();

        double timeLeft = HParam_TimeBudgetSeconds - elapsed();

        if (timeLeft < 0.35)
            return;

        double stopTime = elapsed() + min(sp.maxSeconds, timeLeft * sp.timeFrac);

        int maxExtra = min(sp.hardCap, max(0, (int)floor(nV * sp.extraFrac)));

        if (maxExtra <= 0)
            return;

        int extra = 0;

        for (int round = 0; round < sp.rounds && extra < maxExtra && elapsed() < stopTime; ++round)
        {

            vector<StarCandidate> cands;
            cands.reserve(4096);

            int scanned = 0, visited = 0, total = (int)verts.size();

            int start = (total > 0) ? (starCursor % total) : 0;

            for (; visited < total && scanned < sp.scanVertices && elapsed() < stopTime; ++visited)
            {

                int v = (start + visited) % total;

                if (vdead[v])
                    continue;

                ++scanned;

                StarCandidate c = computeStarCandidate(v);

                if (c.valid())
                    cands.push_back(c);
            }

            if (total > 0)
                starCursor = (start + max(1, visited)) % total;

            if (cands.empty())
                break;

            sort(cands.begin(), cands.end());

            bool progress = false;

            for (const StarCandidate &c : cands)
            {

                if (extra >= maxExtra || elapsed() >= stopTime)
                    break;

                if (vdead[c.v])
                    continue;

                if (applyStarDelete(c.v, c.root))
                {
                    ++accepted;
                    ++extra;
                    progress = true;
                }
            }

            if (!progress)
                break;
        }
    }

    void compact()
    {

        vector<int> o2n(verts.size(), -1);

        vector<Vec3> nv;
        nv.reserve(verts.size() - accepted);

        for (int i = 0; i < (int)verts.size(); ++i)
            if (!vdead[i])
            {
                o2n[i] = (int)nv.size();
                nv.push_back(verts[i]);
            }

        struct FK
        {
            array<int, 3> key;
            Face face;
            bool operator<(const FK &o) const { return key < o.key; }
        };

        vector<FK> fc;
        fc.reserve(faces.size());

        for (int fi = 0; fi < (int)faces.size(); ++fi)
        {

            if (fdead[fi])
                continue;
            int a = faces[fi].v[0], b = faces[fi].v[1], c = faces[fi].v[2];

            if (a < 0 || b < 0 || c < 0 || a >= (int)verts.size() || b >= (int)verts.size() || c >= (int)verts.size())
                continue;

            if (vdead[a] || vdead[b] || vdead[c] || a == b || b == c || a == c)
                continue;

            int na = o2n[a], nb = o2n[b], nc = o2n[c];

            if (na < 0 || nb < 0 || nc < 0 || na == nb || nb == nc || na == nc)
                continue;

            Face nf;
            nf.v[0] = na;
            nf.v[1] = nb;
            nf.v[2] = nc;

            array<int, 3> key = {na, nb, nc};
            sort(key.begin(), key.end());

            fc.push_back({key, nf});
        }

        sort(fc.begin(), fc.end());

        vector<Face> nf;
        nf.reserve(fc.size());

        array<int, 3> prev = {-1, -1, -1};

        for (auto &item : fc)
        {
            if (item.key == prev)
                continue;
            prev = item.key;
            nf.push_back(item.face);
        }

        verts.swap(nv);
        faces.swap(nf);
        nV = (int)verts.size();
        nF = (int)faces.size();
    }
};

bool QemSimplifier::MEMLESS = false;

int main()
{
    QemSimplifier s;
    s.run();
    return 0;
}
