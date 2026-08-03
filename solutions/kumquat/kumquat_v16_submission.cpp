// Kumquat v16 unified medium pipeline
// T3/T4: audited absolute-target QEM -> audited Kumquat post-pass
// profile: growth=20deg, turn=15deg, one-pass remove-until-stuck, greedy selection
#include <algorithm>
#include <bits/stdc++.h>
#include <numeric>
using namespace std;
static constexpr double cp_haus = 0.055;
static constexpr double cp_eps_n = 1e-12;
static constexpr double cp_ssim_c1 = (0.01 * 255.0) * (0.01 * 255.0);
static constexpr double cp_ssim_c2 = (0.03 * 255.0) * (0.03 * 255.0);
static constexpr double INF = 1e100;
static constexpr int cp_views = 6;
static constexpr int cp_R = 1024;
static constexpr double cp_cam_d = 2.5;
static constexpr double cp_focal = 800.0;
static constexpr double cp_bg_n = 127.5;
static constexpr double cp_bg_d = 255.0;
static constexpr int cp_ssim_win = 11;
static constexpr int cp_ssim_rad = 5;
static constexpr int cp_ssim_area = cp_ssim_win * cp_ssim_win;
struct ExpCfg {
double depth, ndot;
bool win;
};
static constexpr ExpCfg exp_strict{0.0035, 0.992, true};
static constexpr ExpCfg exp_loose{0.0060, 0.985, false};
struct QCfg {
double pix, sil, win, bias, wMin, wBase, wSpan, wMax, wPow, anchorW,
  anchorCap;
bool useSqrt, anchor;
};
static constexpr QCfg qc[5] = {
 {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, false, false},
 {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, false, false},
 {1.0, 12.0, 2.0, 0.12, 0.06, 0.08, 0.92, 18.0, 0.90, 0.0000035, 18.0, false,
 true},
 {1.0, 10.0, 0.0, 0.20, 0.12, 0.15, 0.85, 7.0, 0.50, 0.0, 0.0, true, false},
 {1.0, 12.0, 2.0, 0.12, 0.06, 0.08, 0.92, 16.0, 0.72, 0.0000050, 18.0, false,
 true},
};
struct Gate {
double mean, view, norm, depth;
};
static constexpr Gate large_abs_t5{0.9945, 0.9925, 0.9880, 0.9970};
static constexpr double boot_t3_final = 0.145;
static constexpr double boot_t3_pre = 0.160;
static constexpr double boot_retry_margin = 0.40;
static constexpr double boot_t2_ratio[] = {0.36, 0.33, 0.30};
static constexpr double boot_t4_ratio[] = {0.14, 0.10, 0.08};
static constexpr int hp_tick_mask_normal = 4095;
static constexpr int hp_tick_mask_huge = 8191;
static constexpr double hp_guard_area_eps_scale = 1e-11;
static constexpr double hp_guard_min_normal_dot = 0.10;
static constexpr double hp_guard_min_area_ratio = 0.015;
static constexpr double hp_guard_max_area_ratio = 45.0;
struct TierCfg {
int maxV;
double keep;
};
static constexpr TierCfg tc[7] = {
 {0, 0.0},        {5000, 0.0000},   {25000, 0.0000},   {45000, 0.0000},
 {50000, 0.0000}, {400000, 0.0237}, {1100000, 0.0200},
};
static int TIER = 0;
static inline int tier_of(int n) {
if (n <= tc[1].maxV)
 return 1;
if (n <= tc[2].maxV)
 return 2;
if (n <= tc[3].maxV)
 return 3;
if (n <= tc[4].maxV)
 return 4;
if (n <= tc[5].maxV)
 return 5;
return 6;
}
static constexpr double hp_time = 20.2;
static constexpr int hp_prec = 10;
static constexpr double hp_qem_det = 1e-12;
static constexpr double hp_qem_cap = 0.0330;
static constexpr double hp_area_k = 3.0;
static constexpr double hp_area_max = 3.0;
static constexpr double hp_boot_t3_reserve = 4.20;
static constexpr double hp_base_t6_reserve = 0.80;
static constexpr double hp_base_t5_reserve = 1.90;
static constexpr double hp_large_tx_margin = 0.62;
struct FinalTxCfg {
int qTier, expR, R, iters;
double entryMargin, refMargin, iterMargin, qemStopMargin, loRatio;
bool looseExposure;
};
static constexpr double hp_medium_ssim_accept = 0.9200;
static constexpr FinalTxCfg ftx_t2{2, 1024, 1024, 5, 6.0, 4.2, 1.25, 0.72, 0.16, false};
static constexpr FinalTxCfg ftx_t3{3, 1024, 1024, 3, 6.0, 4.2, 1.25, 0.72, 0.08, true};
static constexpr FinalTxCfg ftx_t4{4, 768, 1024, 3, 6.0, 4.2, 1.25, 0.72, 0.055, false};
struct PatchCfg {
bool t3, t4;
int probeDepth, minF, maxF, minInterior, maxSeedTests, maxRegions;
double angleDeg, minUvArea, minWorldArea, maxTurnDeg, keepRatio;
};
static constexpr PatchCfg pc{
 true, true,
 2, 512, 8192, 20, 256, 16,
 20, 1e-12, 1e-12, 15, 0
};
struct V7Cfg {
double postReserve, auditSafety, outputReserve;
int maxBatch;
};
static constexpr V7Cfg v7{9.0, 1.20, 0.45, 8};
struct LTxCfg {
int R;
double entryMargin, qemMargin, qemStopMargin;
double minRatio, cut, midCutK, keepK;
};
static constexpr LTxCfg ltx_t5{256, 0.48, 0.43, 0.25, 0.0155, 0.18, 0.55, 0.97};
static constexpr int cp_min_work_vertices = 10;
static constexpr int cp_min_tx_vertices = 20;
static constexpr int cp_ascii_base = 10;
static constexpr int cp_io_read_reserve_shift = 27;
static constexpr int cp_io_chunk_shift = 16;
static constexpr int cp_out_vertex_chars = 42;
static constexpr int cp_out_face_chars = 26;
static constexpr int cp_out_header_slack = 64;
static constexpr int cp_out_line_chars = 128;
static constexpr int cp_face_vertices = 3;
static constexpr double cp_pixel_center = 0.5;
static constexpr double cp_projection_min_z = 1e-8;
static constexpr double cp_frame_eps = 1e-15;
static constexpr double cp_projected_area_eps = 1e-18;
static constexpr double cp_bary_eps = 1e-9;
static constexpr float cp_depth_inf = 1e30f;
static constexpr unsigned short cp_dist_inf = 60000;
static constexpr double cp_camera_up_parallel_dot = 0.90;
static constexpr double cp_weight_den_eps = 1e-12;
static constexpr double cp_tiny_area = 1e-30;
static constexpr double hp_exp_pix_w = 0.05;
static constexpr double hp_exp_win_w = 0.10;
static constexpr double hp_ltx_ratio_eps = 1e-5;
struct BootCfg {
int t3ExpR, t2ExpR, t4ExpR;
double q4AnchorW, retryReserve;
};
static constexpr BootCfg boot_cfg{384, 1024, 768, 1e-6, 0.50};
struct Vec2 {
double x = 0, y = 0;
Vec2() = default;
Vec2(double x_, double y_) : x(x_), y(y_) {}
Vec2 operator+(const Vec2 &o) const { return {x + o.x, y + o.y}; }
Vec2 operator-(const Vec2 &o) const { return {x - o.x, y - o.y}; }
Vec2 operator*(double s) const { return {x * s, y * s}; }
Vec2 operator/(double s) const { return {x / s, y / s}; }
};
static inline double cross2(const Vec2 &a, const Vec2 &b) {
return a.x * b.y - a.y * b.x;
}
static inline double norm2(const Vec2 &v) { return v.x * v.x + v.y * v.y; }
struct Vec3 {
double x = 0, y = 0, z = 0;
constexpr Vec3() = default;
constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
};
static inline double dot(const Vec3 &a, const Vec3 &b) {
return a.x * b.x + a.y * b.y + a.z * b.z;
}
static inline Vec3 cross(const Vec3 &a, const Vec3 &b) {
return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static inline double norm2(const Vec3 &v) { return dot(v, v); }
static inline double norm(const Vec3 &v) { return sqrt(norm2(v)); }
static inline bool isFiniteVec(const Vec3 &v) {
return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}
struct Face {
int v[3];
};
struct Pixel {
float n[3];
float d;
unsigned char fg;
};
using RenderSet = vector<vector<Pixel>>;
struct Quadric {
double a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0, j = 0;
Quadric &operator+=(const Quadric &o) {
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
void scale(double s) {
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
static Quadric fromTriangle(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2) {
 Vec3 n = cross(p1 - p0, p2 - p0);
 double ta = norm(n);
 if (ta < cp_eps_n)
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
double eval(const Vec3 &p) const {
 return a * p.x * p.x + 2 * b * p.x * p.y + 2 * c * p.x * p.z + 2 * d * p.x +
    e * p.y * p.y + 2 * f * p.y * p.z + 2 * g * p.y + h * p.z * p.z +
    2 * i * p.z + j;
}
};
struct SmallSet {
vector<int> data;
void insert(int v) {
 auto it = lower_bound(data.begin(), data.end(), v);
 if (it == data.end() || *it != v)
  data.insert(it, v);
}
void erase(int v) {
 auto it = lower_bound(data.begin(), data.end(), v);
 if (it != data.end() && *it == v)
  data.erase(it);
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
static double det3(double a00, double a01, double a02, double a10, double a11,
        double a12, double a20, double a21, double a22) {
return a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) +
   a02 * (a10 * a21 - a11 * a20);
}
static bool solveQuadricPosition(const Quadric &q, Vec3 &out) {
double D = det3(q.a, q.b, q.c, q.b, q.e, q.f, q.c, q.f, q.h);
if (fabs(D) < hp_qem_det)
 return false;
out = Vec3(det3(-q.d, q.b, q.c, -q.g, q.e, q.f, -q.i, q.f, q.h) / D,
     det3(q.a, -q.d, q.c, q.b, -q.g, q.f, q.c, -q.i, q.h) / D,
     det3(q.a, q.b, -q.d, q.b, q.e, -q.g, q.c, q.f, -q.i) / D);
return isFiniteVec(out);
}
struct Cand {
int removed = -1, kept = -1, removedVersion = -1, keptVersion = -1;
double cost = INF, radius = 0.0;
Vec3 position;
bool operator<(const Cand &o) const { return cost > o.cost; }
bool valid() const { return removed >= 0 && kept >= 0 && cost < INF; }
};
struct PatchStats {
 int regions = 0, seedCur = 0, seedN = 0, seedOwned = 0;
 int probePass = 0, probeAng = 0, probeTopo = 0;
 int growQ = 0, growOwned = 0, growNoEdge = 0;
 int growAng = 0, growTopo = 0, growTO = 0, qMax = 0;
 int growBendCap = 0, growRoughCap = 0;
 int insertArea = 0, insertBend = 0, insertRough = 0;
 int noSplittable = 0, targetFail = 0, sourceAssignFail = 0;
 int collarPass = 0, collarEmpty = 0, collarTopo = 0, collarLowI = 0;
 long long collarPreservedFaces = 0, collarCoreFaces = 0;
 int regSmall = 0, regLowI = 0, regFinal = 0, regMaxF = 0;
 int paramPass = 0, paramFail = 0, mlsBuilds = 0;
 int projN = 0, projOk = 0, failLocate = 0, failSupport = 0;
 int failRadius = 0, failWeight = 0, failCov = 0, failEigen = 0;
 int replN = 0, valid = 0;
 int dataFail = 0, sampleFail = 0, genFail = 0, triFail = 0;
 int chordFail = 0, structuralFail = 0, patchNoReduce = 0;
 int selected = 0, predRm = 0, actualRm = 0;
 int batchConflict = 0, noSelected = 0, batchApply = 0, batchNoReduce = 0;
 int commit = 0, preMlsV = 0, postMlsV = 0, postBootV = 0, finalV = 0;
 double qualityRmsSum = 0.0, qualityMax = 0.0, qualityNormalSum = 0.0;
 double dispSum = 0.0, dispMax = 0.0;
 double tRegions = 0.0, tParam = 0.0, tMls = 0.0, tSample = 0.0;
 double tOptimize = 0.0, tProject = 0.0, tTri = 0.0, tNormal = 0.0;
 double tValidate = 0.0;
};class Solver {
public:
static bool useLiteConn;
public:
void run() {
 readMesh();
 if (nV <= cp_min_work_vertices) {
  writeMesh();
  return;
 }
 startTime = chrono::steady_clock::now();
 switch (TIER) {
 case 1:
  run_base_qem();
  compact();
  break;
 case 2:
  boot_qem();
  compact();
  tx_medium(false);
  break;
 case 3:
 case 4:
  initScaleTargets();
  tx_medium(true);
  break;
 case 5:
  run_base_qem();
  if (timeBefore(hp_large_tx_margin))
   tx_t5_fullrender(inputV);
  else
   compact();
  break;
 default:
  run_base_qem();
  compact();
  break;
 }
 writeMesh();
}
private:
void run_base_qem() {
 initScaleTargets();
 buildConnectivity();
 rebuildQuadrics();
 if (TIER == 6)
  setStopReserve(hp_base_t6_reserve);
 else if (TIER == 5)
  setStopReserve(hp_base_t5_reserve);
 else
  stopT = hp_time;
 seedPriorityQueue();
 collapseLoop();
}
int nV = 0, nF = 0;
int inputV = 0;
vector<Vec3> verts;
vector<Face> faces;
vector<Vec3> origV;
vector<Face> origF;
vector<double> outRad;
bool screenGuard = false;
vector<char> vdead, fdead;
vector<int> vver;
vector<Quadric> vquad, vmoment;
vector<double> crad;
vector<vector<int>> vfaces;
vector<SmallSet> vneigh;
vector<int> fPix, fSil, fWin;
vector<double> vExp;
int qTier = 0;
double meanExp = 0.0;
double stopT = hp_time;
double wr = v7.outputReserve;
mutable PatchStats ps;
priority_queue<Cand> pq;
int targetV = 0, budget = 0, acc = 0;
double diag = 0, hausd = 0, costCap = INF;
double invDiagSquared = 0.0;
chrono::steady_clock::time_point startTime;
static constexpr Vec3 viewDirs[cp_views] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
double deadline(double reserve) const { return hp_time - reserve; }
bool timeBefore(double reserve) const {
 return elapsed() < deadline(reserve);
}
bool timeAfter(double reserve) const { return elapsed() > deadline(reserve); }
bool completeAuditFits(double auditCost) const {
 return hp_time - elapsed() > auditCost * v7.auditSafety + v7.outputReserve;
}
void setStopReserve(double reserve) { stopT = deadline(reserve); }
void readMesh() {
 vector<char> buf;
 buf.reserve(1 << cp_io_read_reserve_shift);
 char chunk[1 << cp_io_chunk_shift];
 size_t n;
 while ((n = fread(chunk, 1, sizeof(chunk), stdin)) > 0)
  buf.insert(buf.end(), chunk, chunk + n);
 buf.push_back('\0');
 char *p = buf.data();
 nV = (int)strtol(p, &p, cp_ascii_base);
 nF = (int)strtol(p, &p, cp_ascii_base);
 inputV = nV;
 TIER = tier_of(inputV);
 verts.resize(nV);
 faces.resize(nF);
 for (int i = 0; i < nV; ++i) {
  while (*p && *p <= ' ')
   ++p;
  ++p;
  verts[i].x = strtod(p, &p);
  verts[i].y = strtod(p, &p);
  verts[i].z = strtod(p, &p);
 }
 for (int i = 0; i < nF; ++i) {
  while (*p && *p <= ' ')
   ++p;
  ++p;
  faces[i].v[0] = (int)strtol(p, &p, cp_ascii_base) - 1;
  faces[i].v[1] = (int)strtol(p, &p, cp_ascii_base) - 1;
  faces[i].v[2] = (int)strtol(p, &p, cp_ascii_base) - 1;
 }
 if (TIER >= 2 && TIER <= 4) {
  origV = verts;
  origF = faces;
 }
}
void writeMesh() {
 string out;
 out.reserve(nV * cp_out_vertex_chars + nF * cp_out_face_chars +
       cp_out_header_slack);
 char line[cp_out_line_chars];
 snprintf(line, sizeof(line), "%d %d\n", nV, nF);
 out += line;
 for (int i = 0; i < nV; ++i) {
  snprintf(line, sizeof(line), "v %.*g %.*g %.*g\n", hp_prec, verts[i].x,
      hp_prec, verts[i].y, hp_prec, verts[i].z);
  out += line;
 }
 for (int i = 0; i < nF; ++i) {
  snprintf(line, sizeof(line), "f %d %d %d\n", faces[i].v[0] + 1,
      faces[i].v[1] + 1, faces[i].v[2] + 1);
  out += line;
 }
 fwrite(out.data(), 1, out.size(), stdout);
}
void computeViewFrame(int view, Vec3 &eye, Vec3 &right, Vec3 &up,
           Vec3 &fwd) const {
 eye = viewDirs[view] * cp_cam_d;
 fwd = eye * (-1.0);
 double fl = norm(fwd);
 fwd = fwd / fl;
 Vec3 wu = (fabs(dot(fwd, Vec3(0, 0, 1))) > cp_camera_up_parallel_dot)
        ? Vec3(0, 1, 0)
        : Vec3(0, 0, 1);
 right = cross(wu, fwd);
 double rl = norm(right);
 right = rl > cp_frame_eps ? right / rl : Vec3(1, 0, 0);
 up = cross(fwd, right);
 double ul = norm(up);
 up = ul > cp_frame_eps ? up / ul : Vec3(0, 1, 0);
}
struct ImpProj {
 double u = 0, v = 0, z = 0;
 bool ok = false;
};
ImpProj projectToView(const Vec3 &p, int view, int R) const {
 Vec3 e, r, u, f;
 computeViewFrame(view, e, r, u, f);
 Vec3 q = p - e;
 double x = dot(q, r), y = dot(q, u), z = dot(q, f);
 if (z <= cp_projection_min_z)
  return {};
 double sc = double(R) / double(cp_R);
 return {cp_focal * sc * x / z + cp_pixel_center * R,
     cp_focal * sc * y / z + cp_pixel_center * R, z, true};
}
void buildFaceExposure(int R, const ExpCfg &ec = exp_strict) {
 fPix.assign(nF, 0);
 fSil.assign(nF, 0);
 fWin.assign(nF, 0);
 vector<Vec3> fn(nF);
 for (int fi = 0; fi < nF; ++fi) {
  if (fi < (int)fdead.size() && fdead[fi])
   continue;
  const Face &f = faces[fi];
  Vec3 n =
    cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
  double l = norm(n);
  if (l > cp_frame_eps)
   fn[fi] = n / l;
 }
 const int winRad =
   max(1, (int)ceil(double(cp_ssim_rad) * R / double(cp_R)));
 for (int view = 0; view < cp_views; ++view) {
  vector<float> z((size_t)R * R, cp_depth_inf);
  vector<int> id((size_t)R * R, -1);
  for (int fi = 0; fi < nF; ++fi) {
   if (fi < (int)fdead.size() && fdead[fi])
    continue;
   const Face &f = faces[fi];
   auto a = projectToView(verts[f.v[0]], view, R),
     b = projectToView(verts[f.v[1]], view, R),
     c = projectToView(verts[f.v[2]], view, R);
   if (!a.ok || !b.ok || !c.ok)
    continue;
   double den = (b.v - c.v) * (a.u - c.u) + (c.u - b.u) * (a.v - c.v);
   if (fabs(den) < cp_projected_area_eps)
    continue;
   int x0 = max(0, (int)floor(min({a.u, b.u, c.u})));
   int x1 = min(R - 1, (int)ceil(max({a.u, b.u, c.u})));
   int y0 = max(0, (int)floor(min({a.v, b.v, c.v})));
   int y1 = min(R - 1, (int)ceil(max({a.v, b.v, c.v})));
   for (int y = y0; y <= y1; ++y)
    for (int x = x0; x <= x1; ++x) {
     double X = x + cp_pixel_center, Y = y + cp_pixel_center;
     double w0 =
       ((b.v - c.v) * (X - c.u) + (c.u - b.u) * (Y - c.v)) / den;
     double w1 =
       ((c.v - a.v) * (X - c.u) + (a.u - c.u) * (Y - c.v)) / den;
     double w2 = 1.0 - w0 - w1;
     if (w0 < -cp_bary_eps || w1 < -cp_bary_eps || w2 < -cp_bary_eps)
      continue;
     double iz = w0 / a.z + w1 / b.z + w2 / c.z;
     if (iz <= 0.0)
      continue;
     float zz = float(1.0 / iz);
     int q = y * R + x;
     if (zz < z[q]) {
      z[q] = zz;
      id[q] = fi;
     }
    }
  }
  for (int q = 0; q < R * R; ++q)
   if (id[q] >= 0)
    ++fPix[id[q]];
  vector<unsigned short> dist;
  if (ec.win) {
   dist.assign((size_t)R * R, cp_dist_inf);
   for (int y = 0; y < R; ++y)
    for (int x = 0; x < R; ++x) {
     int q = y * R + x, f = id[q];
     if (f < 0)
      continue;
     bool sil = false;
     for (int dy = -1; dy <= 1 && !sil; ++dy)
      for (int dx = -1; dx <= 1; ++dx) {
       if (!dx && !dy)
        continue;
       int xx = x + dx, yy = y + dy;
       if (xx < 0 || yy < 0 || xx >= R || yy >= R) {
        sil = true;
        break;
       }
       int qq = yy * R + xx, g = id[qq];
       if (g < 0 || (g != f && (fabs(z[q] - z[qq]) > ec.depth ||
                   dot(fn[f], fn[g]) < ec.ndot))) {
        sil = true;
        break;
       }
      }
     if (sil) {
      dist[q] = 0;
      ++fSil[f];
     }
    }
  } else {
   for (int y = 1; y < R - 1; ++y)
    for (int x = 1; x < R - 1; ++x) {
     int q = y * R + x, f = id[q];
     if (f < 0)
      continue;
     bool sil = false;
     for (int d : {-1, 1, -R, R}) {
      int g = id[q + d];
      if (g < 0 || (g != f && (fabs(z[q] - z[q + d]) > ec.depth ||
                  dot(fn[f], fn[g]) < ec.ndot))) {
       sil = true;
       break;
      }
     }
     if (sil)
      ++fSil[f];
    }
  }
  if (!ec.win)
   continue;
  for (int y = 0; y < R; ++y)
   for (int x = 0; x < R; ++x) {
    int q = y * R + x, v = dist[q];
    if (x)
     v = min<int>(v, dist[q - 1] + 1);
    if (y)
     v = min<int>(v, dist[q - R] + 1);
    if (x && y)
     v = min<int>(v, dist[q - R - 1] + 1);
    if (x + 1 < R && y)
     v = min<int>(v, dist[q - R + 1] + 1);
    dist[q] = v;
   }
  for (int y = R - 1; y >= 0; --y)
   for (int x = R - 1; x >= 0; --x) {
    int q = y * R + x, v = dist[q];
    if (x + 1 < R)
     v = min<int>(v, dist[q + 1] + 1);
    if (y + 1 < R)
     v = min<int>(v, dist[q + R] + 1);
    if (x + 1 < R && y + 1 < R)
     v = min<int>(v, dist[q + R + 1] + 1);
    if (x && y + 1 < R)
     v = min<int>(v, dist[q + R - 1] + 1);
    dist[q] = v;
    if (v <= winRad && id[q] >= 0)
     ++fWin[id[q]];
   }
 }
}
void boot_qem() {
 qTier = TIER;
 initScaleTargets();
 buildConnectivity();
 vmoment.assign(nV, Quadric());
 if (qTier == 4)
  for (int i = 0; i < nV; ++i)
   vmoment[i] = pointAnchorQuadric(verts[i], boot_cfg.q4AnchorW);
 if (qTier == 3) {
  int targetFinal = (int)floor(nV * boot_t3_final),
    targetPre = (int)floor(nV * boot_t3_pre);
  buildFaceExposure(boot_cfg.t3ExpR, exp_loose);
  rebuildQuadrics();
  targetV = targetPre;
  budget = nV - targetV;
  seedPriorityQueue();
  collapseLoop();
  if (acc < nV - targetFinal && timeBefore(boot_retry_margin)) {
   buildFaceExposure(boot_cfg.t3ExpR, exp_loose);
   rebuildQuadrics();
   priority_queue<Cand> e;
   pq.swap(e);
   targetV = targetFinal;
   budget = nV - targetV;
   seedPriorityQueue();
   collapseLoop();
  }
  return;
 }
 const double *ratios = qTier == 2 ? boot_t2_ratio : boot_t4_ratio;
 for (int ri = 0; ri < 3; ++ri) {
  double kr = ratios[ri];
  if (timeAfter(boot_cfg.retryReserve) || elapsed() > stopT)
   break;
  buildFaceExposure(qTier == 2 ? boot_cfg.t2ExpR : boot_cfg.t4ExpR);
  rebuildQuadrics();
  priority_queue<Cand> e;
  pq.swap(e);
  targetV = (int)floor(nV * kr);
  budget = nV - targetV;
  seedPriorityQueue();
  collapseLoop();
 }
}
static double pixelChannel(const Pixel &p, int ch) {
 if (ch < 3)
  return p.n[ch];
 return p.d;
}
double ssimChannelWindowed(const vector<Pixel> &a, const vector<Pixel> &b,
             int ch, int R) const {
 int S = R + 1;
 size_t N = (size_t)S * S;
 vector<double> ix(N), iy(N), ix2(N), iy2(N), ixy(N);
 for (int y = 0; y < R; ++y) {
  double sx = 0, sy = 0, sx2 = 0, sy2 = 0, sxy = 0;
  size_t pr = (size_t)y * S, ro = (size_t)(y + 1) * S;
  for (int x = 0; x < R; ++x) {
   int idx = y * R + x;
   double X = pixelChannel(a[idx], ch), Y = pixelChannel(b[idx], ch);
   sx += X;
   sy += Y;
   sx2 += X * X;
   sy2 += Y * Y;
   sxy += X * Y;
   size_t q = ro + x + 1, u = pr + x + 1;
   ix[q] = ix[u] + sx;
   iy[q] = iy[u] + sy;
   ix2[q] = ix2[u] + sx2;
   iy2[q] = iy2[u] + sy2;
   ixy[q] = ixy[u] + sxy;
  }
 }
 auto rect = [&](const vector<double> &I, int x0, int y0, int x1, int y1) {
  size_t A = (size_t)y0 * S + x0, B = (size_t)y0 * S + x1,
     C = (size_t)y1 * S + x0, D = (size_t)y1 * S + x1;
  return I[D] - I[B] - I[C] + I[A];
 };
 double total = 0.0;
 long long count = 0;
 for (int y = cp_ssim_rad; y < R - cp_ssim_rad; ++y)
  for (int x = cp_ssim_rad; x < R - cp_ssim_rad; ++x) {
   int idx = y * R + x;
   if (!a[idx].fg && !b[idx].fg)
    continue;
   int x0 = x - cp_ssim_rad, y0 = y - cp_ssim_rad;
   int x1 = x + cp_ssim_rad + 1, y1 = y + cp_ssim_rad + 1;
   double sx = rect(ix, x0, y0, x1, y1);
   double sy = rect(iy, x0, y0, x1, y1);
   double sxx = rect(ix2, x0, y0, x1, y1);
   double syy = rect(iy2, x0, y0, x1, y1);
   double sxy = rect(ixy, x0, y0, x1, y1);
   total += ssimFromWindowSums(sx, sy, sxx, syy, sxy);
   ++count;
  }
 return count ? total / double(count) : 1.0;
}
struct Ssim {
 double mean = 0, minView = 1, minNormal = 1, minDepth = 1;
};
RenderSet renderSsimReference(const vector<Vec3> &vv, const vector<Face> &ff,
               int R) const {
 RenderSet ref(cp_views);
 for (int view = 0; view < cp_views; ++view)
  renderFullSsimBuffer(vv, ff, view, R, ref[view]);
 return ref;
}
Ssim scoreAgainstReference(const RenderSet &ref, const vector<Vec3> &vv,
             const vector<Face> &ff, int R) const {
 RenderSet cur(cp_views);
 for (int view = 0; view < cp_views; ++view)
  renderFullSsimBuffer(vv, ff, view, R, cur[view]);
 Ssim out;
 for (int view = 0; view < cp_views; ++view) {
  double sn = (ssimChannelWindowed(ref[view], cur[view], 0, R) +
        ssimChannelWindowed(ref[view], cur[view], 1, R) +
        ssimChannelWindowed(ref[view], cur[view], 2, R)) /
        double(cp_face_vertices);
  double sd = ssimChannelWindowed(ref[view], cur[view], 3, R);
  double sv = cp_pixel_center * (sn + sd);
  out.mean += sv;
  out.minView = min(out.minView, sv);
  out.minNormal = min(out.minNormal, sn);
  out.minDepth = min(out.minDepth, sd);
 }
 out.mean /= cp_views;
 return out;
}
void exportLiveMesh(vector<Vec3> &vv, vector<Face> &ff) const {
 vector<int> m(verts.size(), -1);
 vv.clear();
 ff.clear();
 vv.reserve(verts.size());
 for (int i = 0; i < (int)verts.size(); ++i)
  if (i >= (int)vdead.size() || !vdead[i]) {
   m[i] = (int)vv.size();
   vv.push_back(verts[i]);
  }
 for (int fi = 0; fi < (int)faces.size(); ++fi) {
  if (fi < (int)fdead.size() && fdead[fi])
   continue;
  const Face &f = faces[fi];
  if (f.v[0] < 0 || f.v[1] < 0 || f.v[2] < 0 || f.v[0] >= (int)m.size() ||
    f.v[1] >= (int)m.size() || f.v[2] >= (int)m.size())
   continue;
  int a = m[f.v[0]], b = m[f.v[1]], c = m[f.v[2]];
  if (a < 0 || b < 0 || c < 0 || a == b || b == c || a == c)
   continue;
  Face q;
  q.v[0] = a;
  q.v[1] = b;
  q.v[2] = c;
  ff.push_back(q);
 }
}
struct Snap {
 vector<Vec3> verts;
 vector<Face> faces;
 vector<char> vdead, fdead;
 vector<int> vver;
 vector<Quadric> vquad, vmoment;
 vector<double> crad;
 vector<vector<int>> vfaces;
 vector<SmallSet> vneigh;
 vector<int> fPix, fSil, fWin;
 vector<double> vExp;
 int targetV, budget, acc, qTier;
 double meanExp;
};
Snap snapshot() const {
 return {verts,   faces,  vdead,  fdead,  vver,  vquad,  vmoment,
     crad,    vfaces, vneigh, fPix,   fSil,  fWin,   vExp,
     targetV, budget, acc,    qTier, meanExp};
}
void restoreSnapshot(const Snap &s) {
 verts = s.verts;
 faces = s.faces;
 vdead = s.vdead;
 fdead = s.fdead;
 vver = s.vver;
 vquad = s.vquad;
 vmoment = s.vmoment;
 crad = s.crad;
 vfaces = s.vfaces;
 vneigh = s.vneigh;
 fPix = s.fPix;
 fSil = s.fSil;
 fWin = s.fWin;
 vExp = s.vExp;
 targetV = s.targetV;
 budget = s.budget;
 acc = s.acc;
 qTier = s.qTier;
 meanExp = s.meanExp;
 priority_queue<Cand> e;
 pq.swap(e);
}
static bool passGate(const Ssim &s, const Gate &g) {
 return s.mean >= g.mean && s.minView >= g.view && s.minNormal >= g.norm &&
    s.minDepth >= g.depth;
}
void rebuildLiveMeshConnectivity() {
 vector<int> mapv(verts.size(), -1);
 vector<Vec3> nv;
 vector<double> nr;
 nv.reserve(verts.size());
 nr.reserve(verts.size());
 for (int i = 0; i < (int)verts.size(); ++i)
  if (i >= (int)vdead.size() || !vdead[i]) {
   mapv[i] = (int)nv.size();
   nv.push_back(verts[i]);
   nr.push_back(i < (int)crad.size() ? crad[i] : 0.0);
  }
 vector<Face> nf;
 nf.reserve(faces.size());
 for (int fi = 0; fi < (int)faces.size(); ++fi) {
  if (fi < (int)fdead.size() && fdead[fi])
   continue;
  const Face &f = faces[fi];
  if (f.v[0] < 0 || f.v[1] < 0 || f.v[2] < 0 ||
    f.v[0] >= (int)mapv.size() || f.v[1] >= (int)mapv.size() ||
    f.v[2] >= (int)mapv.size())
   continue;
  int a = mapv[f.v[0]], b = mapv[f.v[1]], c = mapv[f.v[2]];
  if (a < 0 || b < 0 || c < 0 || a == b || b == c || a == c)
   continue;
  Face q;
  q.v[0] = a;
  q.v[1] = b;
  q.v[2] = c;
  nf.push_back(q);
 }
 verts.swap(nv);
 faces.swap(nf);
 crad.swap(nr);
 nV = (int)verts.size();
 nF = (int)faces.size();
 vdead.assign(nV, 0);
 fdead.assign(nF, 0);
 vver.assign(nV, 0);
 vquad.assign(nV, Quadric());
 vmoment.assign(nV, Quadric());
 vfaces.assign(nV, {});
 vneigh.assign(nV, SmallSet());
 fPix.clear();
 fSil.clear();
 fWin.clear();
 vExp.clear();
 qTier = 0;
 meanExp = 0;
 for (int fi = 0; fi < nF; ++fi) {
  const Face &f = faces[fi];
  Quadric q =
    Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]);
  for (int k = 0; k < 3; ++k) {
   vfaces[f.v[k]].push_back(fi);
   vquad[f.v[k]] += q;
  }
  for (int k = 0; k < 3; ++k) {
   int a = f.v[k], b = f.v[(k + 1) % 3];
   if (a != b) {
    vneigh[a].insert(b);
    vneigh[b].insert(a);
   }
  }
 }
 priority_queue<Cand> empty;
 pq.swap(empty);
 acc = 0;
 targetV = nV;
 budget = 0;
}
void tx_t5_fullrender(int baseV) {
 rebuildLiveMeshConnectivity();
 if (nV < cp_min_tx_vertices || timeAfter(ltx_t5.entryMargin)) {
  compact();
  return;
 }
 vector<Vec3> safeV;
 vector<Face> safeF;
 exportLiveMesh(safeV, safeF);
 const int R = ltx_t5.R;
 RenderSet ref = renderSsimReference(safeV, safeF, R);
 Snap safe = snapshot();
 int live = (int)count_if(vdead.begin(), vdead.end(), [](char dead) { return !dead; });
 double liveRatio = double(live) / double(baseV);
 vector<double> tries = {
   max(ltx_t5.minRatio, liveRatio * (1.0 - ltx_t5.cut)),
   max(ltx_t5.minRatio, liveRatio * (1.0 - ltx_t5.midCutK * ltx_t5.cut)),
   max(ltx_t5.minRatio, liveRatio * ltx_t5.keepK)};
 sort(tries.begin(), tries.end());
 tries.erase(unique(tries.begin(), tries.end(),
          [](double a, double b) { return fabs(a - b) < hp_ltx_ratio_eps; }),
       tries.end());
 bool kept = false;
 vector<Vec3> candV;
 vector<Face> candF;
 for (double ratio : tries) {
  if (timeAfter(ltx_t5.qemMargin))
   break;
  restoreSnapshot(safe);
  setStopReserve(ltx_t5.qemStopMargin);
  rebuildQuadrics();
  priority_queue<Cand> empty;
  pq.swap(empty);
  acc = 0;
  targetV = max(cp_min_work_vertices, (int)floor(baseV * ratio));
  targetV = min(targetV, nV);
  budget = max(0, nV - targetV);
  seedPriorityQueue();
  collapseLoop();
  exportLiveMesh(candV, candF);
  Ssim sc = scoreAgainstReference(ref, candV, candF, R);
  if (passGate(sc, large_abs_t5)) {
   kept = true;
   break;
  }
 }
 if (!kept)
  restoreSnapshot(safe);
 compact();
}
void initScaleTargets() {
 useLiteConn = (nV > tc[1].maxV);
 Vec3 mn = verts[0], mx = verts[0];
 for (const auto &p : verts) {
  mn.x = min(mn.x, p.x);
  mn.y = min(mn.y, p.y);
  mn.z = min(mn.z, p.z);
  mx.x = max(mx.x, p.x);
  mx.y = max(mx.y, p.y);
  mx.z = max(mx.z, p.z);
 }
 diag = norm(mx - mn);
 hausd = cp_haus * diag;
 costCap = hp_qem_cap * diag * diag;
 invDiagSquared = (diag > cp_eps_n) ? (1.0 / (diag * diag)) : 0.0;
 const double kr = tc[tier_of(nV)].keep;
 targetV = max(cp_min_work_vertices, (int)floor(nV * kr));
 targetV = min(targetV, nV - 1);
 budget = nV - targetV;
}
void buildConnectivity() {
 vdead.assign(nV, 0);
 fdead.assign(nF, 0);
 vver.assign(nV, 0);
 vquad.assign(nV, Quadric());
 vmoment.assign(nV, Quadric());
 crad.assign(nV, 0.0);
 vfaces.assign(nV, {});
 vneigh.resize(nV);
 for (int fi = 0; fi < nF; ++fi) {
  const auto &f = faces[fi];
  Quadric q =
    Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]);
  for (int k = 0; k < 3; ++k) {
   vfaces[f.v[k]].push_back(fi);
   vquad[f.v[k]] += q;
  }
  for (int k = 0; k < 3; ++k) {
   int a = f.v[k], b = f.v[(k + 1) % 3];
   if (a != b) {
    vneigh[a].insert(b);
    vneigh[b].insert(a);
   }
  }
 }
}
double areaNormalQuadricWeight(const Vec3 &normalVec, double area) const {
 double absSum = fabs(normalVec.x) + fabs(normalVec.y) + fabs(normalVec.z);
 double areaDiagNorm = area * invDiagSquared;
 double w = 1.0 + hp_area_k * areaDiagNorm * absSum;
 return min(w, hp_area_max);
}
Quadric pointAnchorQuadric(const Vec3 &p, double w) const {
 Quadric q;
 q.a = q.e = q.h = w;
 q.d = -w * p.x;
 q.g = -w * p.y;
 q.i = -w * p.z;
 q.j = w * norm2(p);
 return q;
}
double faceImp(int fi) const {
 const QCfg &c = qc[qTier];
 return c.pix * fPix[fi] + c.sil * fSil[fi] + c.win * fWin[fi];
}
double expQuadricWeight(double imp) const {
 const QCfg &c = qc[qTier];
 double r = (imp + c.bias * meanExp) / (meanExp + cp_weight_den_eps);
 double shaped = c.useSqrt ? sqrt(max(0.0, r)) : pow(max(0.0, r), c.wPow);
 return min(c.wMax, max(c.wMin, c.wBase + c.wSpan * shaped));
}
void rebuildQuadrics() {
 vquad.assign(nV, Quadric());
 meanExp = 0.0;
 int activeF = 0;
 if (qTier)
  for (int fi = 0; fi < nF; ++fi)
   if (!(fi < (int)fdead.size() && fdead[fi])) {
    meanExp += faceImp(fi);
    ++activeF;
   }
 if (qTier)
  meanExp /= max(1, activeF);
 vExp.assign(nV, 0.0);
 if (qTier && qc[qTier].anchor)
  for (int fi = 0; fi < nF; ++fi)
   if (!(fi < (int)fdead.size() && fdead[fi]))
    for (int k = 0; k < 3; ++k)
     vExp[faces[fi].v[k]] +=
       fSil[fi] + hp_exp_pix_w * fPix[fi] + hp_exp_win_w * fWin[fi];
 for (int fi = 0; fi < nF; ++fi) {
  if (fi < (int)fdead.size() && fdead[fi])
   continue;
  const Face &f = faces[fi];
  Vec3 n =
    cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
  double area = 0.5 * norm(n);
  Quadric q =
    Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]);
  if (qTier) {
   double w = expQuadricWeight(faceImp(fi));
   if (w != 1.0)
    q.scale(w);
  } else if (area >= cp_tiny_area) {
   n = n / (2.0 * area);
   double w = areaNormalQuadricWeight(n, area);
   if (w != 1.0)
    q.scale(w);
  }
  for (int k = 0; k < 3; ++k)
   vquad[f.v[k]] += q;
 }
 if (qTier && qc[qTier].anchor) {
  double am = 0.0;
  int ac = 0;
  for (int i = 0; i < nV; ++i)
   if (!vdead[i]) {
    am += vExp[i];
    ++ac;
   }
  am /= max(1, ac);
  double bw = qc[qTier].anchorW * diag * diag / (am + 1.0);
  for (int i = 0; i < nV; ++i)
   if (!vdead[i] && vExp[i] > 0.0)
    vquad[i] += pointAnchorQuadric(
      verts[i], bw * min(qc[qTier].anchorCap,
               vExp[i] / (am + cp_weight_den_eps)));
  if (qTier == 4)
   for (int i = 0; i < nV; ++i)
    if (!vdead[i])
     vquad[i] += vmoment[i];
 }
}
Quadric faceQuadric(int fi) const {
 const Face &f = faces[fi];
 Vec3 n =
   cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
 double area = 0.5 * norm(n);
 Quadric q =
   Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]], verts[f.v[2]]);
 if (qTier && fi < (int)fPix.size()) {
  double w = expQuadricWeight(faceImp(fi));
  if (w != 1.0)
   q.scale(w);
 } else if (area >= cp_tiny_area) {
  n = n / (2.0 * area);
  double w = areaNormalQuadricWeight(n, area);
  if (w != 1.0)
   q.scale(w);
 }
 return q;
}
void collectCollapsePositions(int a, int b, const Quadric &q, Vec3 pos[5],
               int &np) const {
 np = 0;
 Vec3 qp;
 if (solveQuadricPosition(q, qp))
  pos[np++] = qp;
 if (TIER == 6) {
  Vec3 ed = verts[b] - verts[a];
  double el = norm(ed);
  if (el > cp_tiny_area) {
   double t = max(0.0, min(1.0, 0.5 * (1.0 + (crad[b] - crad[a]) / el)));
   pos[np++] = verts[a] + ed * t;
  }
 }
 pos[np++] = (verts[a] + verts[b]) * 0.5;
 pos[np++] = verts[a];
 pos[np++] = verts[b];
 int wp = 0;
 for (int i = 0; i < np; ++i) {
  if (!isFiniteVec(pos[i]))
   continue;
  bool dup = false;
  for (int j = 0; j < wp; ++j)
   if (norm2(pos[i] - pos[j]) < cp_tiny_area) {
    dup = true;
    break;
   }
  if (!dup)
   pos[wp++] = pos[i];
 }
 np = wp;
}
Cand makeCand(int ab, int kp, const Vec3 &p, const Quadric &q) const {
 Cand c;
 c.removed = ab;
 c.kept = kp;
 c.removedVersion = vver[ab];
 c.keptVersion = vver[kp];
 c.position = p;
 c.cost = q.eval(p);
 return c;
}
double tieExposure(int kept) const {
 return (qTier && kept >= 0 && kept < (int)vExp.size()) ? vExp[kept] : 0.0;
}
bool betterCollapseCandidate(const Cand &cand, const Cand &best) const {
 if (cand.cost != best.cost)
  return cand.cost < best.cost;
 if (TIER == 6)
  return false;
 double candExposure = tieExposure(cand.kept);
 double bestExposure = tieExposure(best.kept);
 if (candExposure != bestExposure)
  return candExposure > bestExposure;
 if (cand.kept < 0 || best.kept < 0 || cand.kept >= (int)vneigh.size() ||
   best.kept >= (int)vneigh.size())
  return false;
 return vneigh[cand.kept].size() > vneigh[best.kept].size();
}
Cand bestCollapseNoRadius(int a, int b) const {
 Quadric q = vquad[a];
 q += vquad[b];
 Vec3 pos[5];
 int np;
 collectCollapsePositions(a, b, q, pos, np);
 Cand best;
 for (int i = 0; i < np; ++i) {
  Cand c1 = makeCand(a, b, pos[i], q);
  if (betterCollapseCandidate(c1, best))
   best = c1;
  Cand c2 = makeCand(b, a, pos[i], q);
  if (betterCollapseCandidate(c2, best))
   best = c2;
 }
 return best;
}
bool passesRadiusGuard(int a, int b, const Vec3 &p, double &mr) const {
 mr = max(crad[a] + norm(verts[a] - p), crad[b] + norm(verts[b] - p));
 return mr <= hausd;
}
Cand bestCollapse(int a, int b) const {
 Quadric q = vquad[a];
 q += vquad[b];
 Vec3 pos[5];
 int np;
 collectCollapsePositions(a, b, q, pos, np);
 Cand best;
 for (int i = 0; i < np; ++i) {
  for (int dir = 0; dir < 2; ++dir) {
   int ab = dir ? b : a, kp = dir ? a : b;
   double mr;
   if (!passesRadiusGuard(ab, kp, pos[i], mr))
    continue;
   Cand c = makeCand(ab, kp, pos[i], q);
   c.radius = mr;
   if (betterCollapseCandidate(c, best))
    best = c;
  }
 }
 return best;
}
void seedPriorityQueue() {
 for (int a = 0; a < nV; ++a)
  for (int b : vneigh[a])
   if (a < b) {
    auto c = bestCollapseNoRadius(a, b);
    if (c.valid())
     pq.push(c);
   }
}
double elapsed() const {
 return chrono::duration<double>(chrono::steady_clock::now() - startTime)
   .count();
}
bool isLiveEdge(int a, int b) const {
 return a >= 0 && b >= 0 && a < (int)verts.size() && b < (int)verts.size() &&
    !vdead[a] && !vdead[b] && vneigh[a].contains(b);
}
int commonFaceCount(int a, int b) const {
 int cnt = 0;
 const auto &fa = vfaces[a];
 const auto &fb = vfaces[b];
 if (fa.size() < fb.size()) {
  cnt = std::count_if(fa.begin(), fa.end(), [&](int f) {
   return !fdead[f] && std::any_of(fb.begin(), fb.end(),
                   [&](int f2) { return f == f2; });
  });
 } else {
  cnt = std::count_if(fb.begin(), fb.end(), [&](int f) {
   return !fdead[f] && std::any_of(fa.begin(), fa.end(),
                   [&](int f2) { return f == f2; });
  });
 }
 return cnt;
}
int commonNeighborCount(int a, int b) const {
 int cnt = 0;
 const auto &na = vneigh[a];
 const auto &nb = vneigh[b];
 if (na.size() < nb.size()) {
  cnt = std::count_if(na.begin(), na.end(), [&](int x) {
   return x != a && x != b && !vdead[x] && nb.contains(x);
  });
 } else {
  cnt = std::count_if(nb.begin(), nb.end(), [&](int x) {
   return x != a && x != b && !vdead[x] && na.contains(x);
  });
 }
 return cnt;
}
static void eraseValueUnordered(vector<int> &v, int x) {
 for (int i = (int)v.size() - 1; i >= 0; --i)
  if (v[i] == x) {
   v[i] = v.back();
   v.pop_back();
   return;
  }
}
bool passesScreenGuard(const Cand &c) const {
 int a = c.removed, b = c.kept;
 vector<int> patch;
 std::copy_if(vfaces[a].begin(), vfaces[a].end(), std::back_inserter(patch),
       [&](int fi) { return !fdead[fi]; });
 std::copy_if(vfaces[b].begin(), vfaces[b].end(), std::back_inserter(patch),
       [&](int fi) { return !fdead[fi]; });
 sort(patch.begin(), patch.end());
 patch.erase(unique(patch.begin(), patch.end()), patch.end());
 vector<array<int, 3>> newKeys;
 newKeys.reserve(patch.size());
 for (int fi : patch) {
  const Face &f = faces[fi];
  int id[3] = {f.v[0], f.v[1], f.v[2]};
  const Vec3 op[3] = {verts[id[0]], verts[id[1]], verts[id[2]]};
  for (int k = 0; k < 3; ++k)
   if (id[k] == a)
    id[k] = b;
  if (id[0] == id[1] || id[1] == id[2] || id[0] == id[2])
   continue;
  const Vec3 np[3] = {id[0] == b ? c.position : verts[id[0]],
            id[1] == b ? c.position : verts[id[1]],
            id[2] == b ? c.position : verts[id[2]]};
  Vec3 on = cross(op[1] - op[0], op[2] - op[0]),
    nn = cross(np[1] - np[0], np[2] - np[0]);
  double ol = norm(on), nl = norm(nn);
  if (ol < cp_eps_n ||
    nl < max(cp_eps_n, hp_guard_area_eps_scale * diag * diag))
   return false;
  double d = dot(on, nn) / (ol * nl);
  if (d < hp_guard_min_normal_dot)
   return false;
  double ar = nl / ol;
  if (ar < hp_guard_min_area_ratio || ar > hp_guard_max_area_ratio)
   return false;
  array<int, 3> key = {id[0], id[1], id[2]};
  sort(key.begin(), key.end());
  newKeys.push_back(key);
 }
 sort(newKeys.begin(), newKeys.end());
 for (int i = 1; i < (int)newKeys.size(); ++i)
  if (newKeys[i] == newKeys[i - 1])
   return false;
 return true;
}
void collapseLoop() {
 int tick = 0;
 while (acc < budget && !pq.empty()) {
  ++tick;
  int tickMask = (TIER == 6 ? hp_tick_mask_huge : hp_tick_mask_normal);
  if ((tick & tickMask) == 0 && elapsed() > stopT)
   break;
  auto c = pq.top();
  pq.pop();
  int a = c.removed, b = c.kept;
  if (!isLiveEdge(a, b))
   continue;
  if (c.removedVersion != vver[a] || c.keptVersion != vver[b]) {
   auto fr = bestCollapseNoRadius(a, b);
   if (fr.valid())
    pq.push(fr);
   continue;
  }
  if (c.cost > costCap)
   break;
  if (commonFaceCount(a, b) != 2)
   continue;
  if (commonNeighborCount(a, b) != 2)
   continue;
  auto best = bestCollapse(a, b);
  if (!best.valid() || best.cost > costCap)
   continue;
  if (screenGuard && !passesScreenGuard(best))
   continue;
  applyCollapse(best.removed, best.kept, best.position, best.radius);
  ++acc;
 }
}
void applyCollapse(int ab, int kp, const Vec3 &np, double nr) {
 verts[kp] = np;
 crad[kp] = nr;
 crad[ab] = 0;
 vdead[ab] = 1;
 ++vver[ab];
 ++vver[kp];
 auto abFaces = vfaces[ab];
 vector<int> dead;
 dead.reserve(4);
 for (int fi : abFaces) {
  if (fdead[fi])
   continue;
  bool touched = false;
  for (int k = 0; k < 3; ++k)
   if (faces[fi].v[k] == ab) {
    faces[fi].v[k] = kp;
    touched = true;
   }
  if (!touched)
   continue;
  if (faces[fi].v[0] == faces[fi].v[1] ||
    faces[fi].v[1] == faces[fi].v[2] ||
    faces[fi].v[0] == faces[fi].v[2]) {
   fdead[fi] = 1;
   dead.push_back(fi);
  } else
   vfaces[kp].push_back(fi);
 }
 for (int fi : dead)
  for (int k = 0; k < 3; ++k) {
   int v = faces[fi].v[k];
   if (v >= 0 && v < (int)vfaces.size())
    eraseValueUnordered(vfaces[v], fi);
  }
 vfaces[ab].clear();
 if (kp < (int)vmoment.size() && ab < (int)vmoment.size())
  vmoment[kp] += vmoment[ab];
 if (useLiteConn) {
  Quadric fresh;
  for (int fi : vfaces[kp]) {
   if (fdead[fi])
    continue;
   const Face &f = faces[fi];
   fresh += ((qTier == 2 || qTier == 4)
          ? faceQuadric(fi)
          : Quadric::fromTriangle(verts[f.v[0]], verts[f.v[1]],
                      verts[f.v[2]]));
  }
  if (qTier == 4 && kp < (int)vmoment.size())
   fresh += vmoment[kp];
  vquad[kp] = fresh;
 } else {
  vquad[kp] += vquad[ab];
 }
 for (int nb : vneigh[ab]) {
  if (nb == kp || vdead[nb])
   continue;
  vneigh[nb].erase(ab);
  vneigh[nb].insert(kp);
  vneigh[kp].insert(nb);
 }
 vneigh[ab].clear();
 vneigh[kp].erase(ab);
 vneigh[kp].erase(kp);
 for (int nb : vneigh[kp]) {
  if (nb == kp || vdead[nb])
   continue;
  auto c = bestCollapseNoRadius(kp, nb);
  if (c.valid())
   pq.push(c);
 }
}
struct Region {
 vector<int> faceIds;
 vector<int> boundary;
 vector<int> interior;
 double area = 0.0;
 double bendRaw = 0.0;
 double roughRaw = 0.0;
};
struct PatchComplexityEdge {
 int a = -1, b = -1, f0 = -1, f1 = -1;
 double bend = 0.0, rough = 0.0;
};
struct PatchComplexityData {
 vector<Vec3> normals, centroids;
 vector<double> faceArea;
 vector<array<int, 3>> adjacency, edgeId;
 vector<PatchComplexityEdge> edges;
 double totalBend = 0.0, totalRough = 0.0;
};
struct LocalPatch {
 vector<int> localVerts;
 vector<array<int, 3>> localFaces;
};
struct Replacement {
 vector<int> oldFaces;
 vector<int> conflictFaces;
 vector<int> boundary;
 vector<Vec3> newInterior;
 vector<double> newRadius;
 vector<array<int, 3>> newFaces;
 int removed = 0;
};
static uint64_t patchEdgeKey(int a, int b) {
 if (a > b)
  swap(a, b);
 return (uint64_t)(uint32_t)a << 32 | (uint32_t)b;
}
static double clampUnit(double x) { return max(-1.0, min(1.0, x)); }
static Vec3 normalizedOrZero(const Vec3 &v) {
 double l = norm(v);
 return l > cp_eps_n ? v / l : Vec3();
}
bool patchEnabled() const {
 return (TIER == 3 && pc.t3) || (TIER == 4 && pc.t4);
}
vector<Vec3> faceNormals(const vector<Vec3> &vv,
                   const vector<Face> &ff) const {
 vector<Vec3> out(ff.size());
 for (int fi = 0; fi < (int)ff.size(); ++fi) {
  const Face &f = ff[fi];
  out[fi] = normalizedOrZero(
    cross(vv[f.v[1]] - vv[f.v[0]], vv[f.v[2]] - vv[f.v[0]]));
 }
 return out;
}
vector<array<int, 3>> faceAdjacency(const vector<Face> &ff) const {
 vector<array<int, 3>> adj(ff.size(), array<int, 3>{-1, -1, -1});
 unordered_map<uint64_t, pair<int, int>> owner;
 owner.reserve(ff.size() * 2);
 for (int fi = 0; fi < (int)ff.size(); ++fi) {
  for (int e = 0; e < 3; ++e) {
   int a = ff[fi].v[e], b = ff[fi].v[(e + 1) % 3];
   uint64_t key = patchEdgeKey(a, b);
   auto it = owner.find(key);
   if (it == owner.end()) {
    owner.emplace(key, make_pair(fi, e));
   } else {
    auto o = it->second;
    adj[fi][e] = o.first;
    adj[o.first][o.second] = fi;
   }
  }
 }
 return adj;
}
PatchComplexityData buildPatchComplexity(const vector<Vec3> &vv,
                              const vector<Face> &ff) const {
 PatchComplexityData out;
 const int F = (int)ff.size();
 out.normals = faceNormals(vv, ff);
 out.centroids.resize(F);
 out.faceArea.resize(F, 0.0);
 out.adjacency.assign(F, array<int, 3>{-1, -1, -1});
 out.edgeId.assign(F, array<int, 3>{-1, -1, -1});
 unordered_map<uint64_t, pair<int, int>> owner;
 owner.reserve(ff.size() * 2);
 for (int fi = 0; fi < F; ++fi) {
  const Face &f = ff[fi];
  Vec3 a = vv[f.v[0]], b = vv[f.v[1]], c = vv[f.v[2]];
  out.centroids[fi] = (a + b + c) / 3.0;
  out.faceArea[fi] = 0.5 * norm(cross(b - a, c - a));
  for (int k = 0; k < 3; ++k) {
   int x = f.v[k], y = f.v[(k + 1) % 3];
   uint64_t key = patchEdgeKey(x, y);
   auto it = owner.find(key);
   if (it == owner.end()) {
    int id = (int)out.edges.size();
    PatchComplexityEdge e; e.a = min(x, y); e.b = max(x, y); e.f0 = fi;
    out.edges.push_back(e);
    out.edgeId[fi][k] = id;
    owner.emplace(key, make_pair(fi, k));
   } else {
    int of = it->second.first, ok = it->second.second;
    int id = out.edgeId[of][ok];
    out.edgeId[fi][k] = id;
    out.adjacency[fi][k] = of;
    out.adjacency[of][ok] = fi;
    out.edges[id].f1 = fi;
   }
  }
 }
 for (PatchComplexityEdge &e : out.edges) if (e.f1 >= 0) {
  double jump2 = max(0.0, 2.0 - 2.0 * clampUnit(dot(out.normals[e.f0], out.normals[e.f1])));
  double len = norm(vv[e.a] - vv[e.b]);
  double d = norm(out.centroids[e.f0] - out.centroids[e.f1]);
  e.bend = len * sqrt(jump2);
  e.rough = (len / max(d, 1e-15 * max(1.0, diag))) * jump2;
  out.totalBend += e.bend;
  out.totalRough += e.rough;
 }
 return out;
}
bool finalizeRegion(const vector<Face> &ff,
            const vector<int> &regionF,
            Region &out, bool enforceMinInterior = true) const {
 unordered_map<uint64_t, int> edgeCount;
 edgeCount.reserve(regionF.size() * 2);
 unordered_set<int> vertexSet;
 vertexSet.reserve(regionF.size());
 for (int fi : regionF) {
  const Face &f = ff[fi];
  for (int k = 0; k < 3; ++k) {
   vertexSet.insert(f.v[k]);
   ++edgeCount[patchEdgeKey(f.v[k], f.v[(k + 1) % 3])];
  }
 }
 vector<pair<int, int>> boundaryEdges;
 boundaryEdges.reserve(regionF.size());
 for (int fi : regionF) {
  const Face &f = ff[fi];
  for (int k = 0; k < 3; ++k) {
   int a = f.v[k], b = f.v[(k + 1) % 3];
   if (edgeCount[patchEdgeKey(a, b)] == 1)
    boundaryEdges.emplace_back(a, b);
  }
 }
 if (boundaryEdges.size() < 3) {
  return false;
 }
 unordered_map<int, vector<int>> badj;
 badj.reserve(boundaryEdges.size());
 for (auto [a, b] : boundaryEdges) {
  badj[a].push_back(b);
  badj[b].push_back(a);
 }
 for (const auto &kv : badj)
  if (kv.second.size() != 2) {
   return false;
  }
 vector<int> boundary;
 boundary.reserve(boundaryEdges.size());
 int start = boundaryEdges[0].first, prev = -1, cur = start;
 do {
  boundary.push_back(cur);
  const auto &nb = badj[cur];
  int nxt = nb[0] == prev ? nb[1] : nb[0];
  prev = cur;
  cur = nxt;
  if ((int)boundary.size() > (int)boundaryEdges.size()) {
   return false;
  }
 } while (cur != start);
 if (boundary.size() != boundaryEdges.size()) {
  return false;
 }
 const int V = (int)vertexSet.size();
 const int E = (int)edgeCount.size();
 const int F = (int)regionF.size();
 if (V - E + F != 1) {
  return false;
 }
 unordered_set<int> boundarySet(boundary.begin(), boundary.end());
 vector<int> interior;
 interior.reserve(vertexSet.size());
 for (int v : vertexSet)
  if (!boundarySet.count(v))
   interior.push_back(v);
 if (enforceMinInterior &&
   (int)interior.size() < pc.minInterior) {
  return false;
 }
 out.faceIds = regionF;
 out.boundary = boundary;
 out.interior = interior;
 return true;
}
// Preserve a one-vertex-wide collar at the original region boundary.
// Every original region face incident to an outer-boundary vertex remains unchanged.
// Only the strictly interior core is eligible for replacement.
bool buildBoundaryProtectedCore(const vector<Face> &ff,
                                const Region &grown,
                                const PatchComplexityData &cx,
                                Region &core,
                                int &preservedFaces) const {
 unordered_set<int> outerBoundary(grown.boundary.begin(), grown.boundary.end());
 vector<char> eligible(ff.size(), 0), seen(ff.size(), 0);
 int eligibleCount = 0;
 preservedFaces = 0;
 for (int fi : grown.faceIds) {
  const Face &f = ff[fi];
  bool touchesOuterBoundary = outerBoundary.count(f.v[0]) ||
                              outerBoundary.count(f.v[1]) ||
                              outerBoundary.count(f.v[2]);
  if (touchesOuterBoundary) ++preservedFaces;
  else { eligible[fi] = 1; ++eligibleCount; }
 }
 if (eligibleCount == 0) return false;
 Region best;

 for (int seed : grown.faceIds) if (eligible[seed] && !seen[seed]) {
  vector<int> component;
  queue<int> q; q.push(seed); seen[seed] = 1;
  while (!q.empty()) {
   int f = q.front(); q.pop(); component.push_back(f);
   for (int g : cx.adjacency[f])
    if (g >= 0 && eligible[g] && !seen[g]) { seen[g] = 1; q.push(g); }
  }
  Region candidate;
  if (!finalizeRegion(ff, component, candidate, false)) {
   continue;
  }
  if (candidate.interior.size() > best.interior.size()) best = move(candidate);
 }

 if ((int)best.interior.size() < pc.minInterior) return false;
 vector<char> inCore(ff.size(), 0);
 for (int fi : best.faceIds) inCore[fi] = 1;
 best.area = 0.0;
 best.bendRaw = 0.0;
 best.roughRaw = 0.0;
 for (int fi : best.faceIds) {
  best.area += cx.faceArea[fi];
  for (int k = 0; k < 3; ++k) {
   int g = cx.adjacency[fi][k];
   if (g >= 0 && fi < g && inCore[g]) {
    const PatchComplexityEdge &e = cx.edges[cx.edgeId[fi][k]];
    best.bendRaw += e.bend;
    best.roughRaw += e.rough;
   }
  }
 }
 core = move(best);
 return true;
}

vector<Region> findPatchRegions(const vector<Vec3> &vv,
                      const vector<Face> &ff,
                      const PatchComplexityData &cx) const {
 vector<Region> regions;
 if (ff.empty()) return regions;
 const vector<Vec3> &fn = cx.normals;
 const vector<array<int, 3>> &adj = cx.adjacency;
 const double angleCos = cos(pc.angleDeg * acos(-1.0) / 180.0);
 const int F = (int)ff.size(), N = (int)vv.size();
 vector<int> owner(F, -1), order(F), inside(F, 0), cstamp(F, 0), depth(F);
 vector<unsigned char> cstate(F, 0), vuse(N, 0), bdeg(N, 0);
 vector<array<int, 2>> bn(N, array<int, 2>{-1, -1});
 iota(order.begin(), order.end(), 0);
 uint32_t rng = 2166136261u ^ (uint32_t)F;
 for (int i = F - 1; i > 0; --i) {
  rng = 1664525u * rng + 1013904223u;
  swap(order[i], order[(int)(rng % (uint32_t)(i + 1))]);
 }
 int stamp = 0, cursor = 0, tests = 0;
 vector<int> touched;
 while (cursor < F && tests < pc.maxSeedTests &&
      (int)regions.size() < pc.maxRegions && !timeAfter(wr)) {
  int seed = order[cursor++];
  ps.seedCur = cursor;
  if (owner[seed] >= 0) { ++ps.seedOwned; continue; }
  ++tests; ps.seedN = tests; ++stamp;
  vector<int> regionF, frontier;
  queue<int> probe;
  inside[seed] = stamp; depth[seed] = 0; probe.push(seed);
  while (!probe.empty()) {
   int f = probe.front(); probe.pop(); regionF.push_back(f);
   if (depth[f] == pc.probeDepth) continue;
   for (int g : adj[f]) if (g >= 0 && owner[g] < 0 && inside[g] != stamp) {
    inside[g] = stamp; depth[g] = depth[f] + 1; probe.push(g);
   }
  }
  bool smooth = true;
  for (int f : regionF) for (int g : adj[f])
   if (g >= 0 && f < g && inside[g] == stamp && dot(fn[f], fn[g]) < angleCos)
    smooth = false;
  if (!smooth) { ++ps.probeAng; continue; }
  Region probeRegion;
  if (!finalizeRegion(ff, regionF, probeRegion, false)) {
   ++ps.probeTopo; continue;
  }
  ++ps.probePass;
  double patchArea = 0.0, patchBend = 0.0, patchRough = 0.0;
  for (int f : regionF) {
   patchArea += cx.faceArea[f];
   for (int k = 0; k < 3; ++k) {
    int g = adj[f][k];
    if (g >= 0 && f < g && inside[g] == stamp) {
     const PatchComplexityEdge &e = cx.edges[cx.edgeId[f][k]];
     patchBend += e.bend;
     patchRough += e.rough;
    }
   }
  }
  for (int v : touched) { vuse[v] = bdeg[v] = 0; bn[v] = {-1, -1}; }
  touched.clear();
  unordered_map<uint64_t, unsigned char> ec;
  ec.reserve((size_t)pc.maxF * 2);
  int V = 0, E = 0, RF = (int)regionF.size(), BE = 0;
  auto touch = [&](int v) { if (!vuse[v]) { touched.push_back(v); ++V; } ++vuse[v]; };
  for (int f : regionF) {
   const Face &x = ff[f];
   for (int k = 0; k < 3; ++k) {
    touch(x.v[k]); uint64_t e = patchEdgeKey(x.v[k], x.v[(k + 1) % 3]);
    auto it = ec.find(e); if (it == ec.end()) { ec[e] = 1; ++E; } else ++it->second;
   }
  }
  auto addB = [&](int a, int b) {
   if (bdeg[a] >= 2 || bdeg[b] >= 2) return false;
   bn[a][bdeg[a]++] = b; bn[b][bdeg[b]++] = a; ++BE; return true;
  };
  auto remB = [&](int a, int b) {
   auto rem = [&](int x, int y) {
    int q = bn[x][0] == y ? 0 : (bn[x][1] == y ? 1 : -1);
    if (q < 0) return false;
    bn[x][q] = bn[x][bdeg[x] - 1]; bn[x][bdeg[x] - 1] = -1; --bdeg[x]; return true;
   };
   if (!rem(a, b) || !rem(b, a)) return false;
   --BE; return true;
  };
  bool initOk = true;
  for (const auto &kv : ec) if (kv.second == 1) {
   int a = (int)(kv.first >> 32), b = (int)(uint32_t)kv.first;
   if (!addB(a, b)) { initOk = false; break; }
  }
  if (!initOk) { ++ps.probeTopo; continue; }
  auto oneCycle = [&]() {
   if (BE < 3) return false;
   int start = -1;
   for (int v : touched) if (bdeg[v]) { start = v; break; }
   if (start < 0) return false;
   int prev = -1, cur = start, steps = 0;
   do {
    if (bdeg[cur] != 2) return false;
    int nxt = bn[cur][0] == prev ? bn[cur][1] : bn[cur][0];
    if (nxt < 0 || ++steps > BE) return false;
    prev = cur; cur = nxt;
   } while (cur != start);
   return steps == BE;
  };
  ++stamp;
  const int stateStamp = stamp;
  deque<int> q;
  auto enqueue = [&](int f) {
   if (f < 0 || inside[f] == stateStamp || owner[f] >= 0 ||
     (cstamp[f] == stateStamp && cstate[f] != 0)) return;
   cstamp[f] = stateStamp; cstate[f] = 1; q.push_back(f);
   ++ps.growQ; ps.qMax = max(ps.qMax, (int)q.size());
  };
  for (int f : regionF) inside[f] = stateStamp;
  for (int f : regionF) for (int g : adj[f]) enqueue(g);
  while (!q.empty() && (int)regionF.size() < pc.maxF) {
   if (timeAfter(wr)) { ++ps.growTO; break; }
   int f = q.front(); q.pop_front(); cstate[f] = 2;
   if (owner[f] >= 0) { ++ps.growOwned; continue; }
   vector<int> shared, sharedEdgeIds;
   for (int k = 0; k < 3; ++k) {
    int g = adj[f][k];
    if (g >= 0 && inside[g] == stateStamp) {
     shared.push_back(g);
     sharedEdgeIds.push_back(cx.edgeId[f][k]);
    }
   }
   if (shared.empty()) { ++ps.growNoEdge; continue; }
   bool angleOk = true;
   for (int g : shared) if (dot(fn[f], fn[g]) < angleCos) { angleOk = false; break; }
   if (!angleOk) { ++ps.growAng; continue; }
   double trialBend = patchBend, trialRough = patchRough;
   for (int eid : sharedEdgeIds) {
    trialBend += cx.edges[eid].bend;
    trialRough += cx.edges[eid].rough;
   }
   const Face &x = ff[f];
   int ovV = V, ovE = E, ovF = RF, ovBE = BE;
   array<unsigned char, 3> ovUse, ovDeg;
   array<array<int, 2>, 3> ovBn;
   array<unsigned char, 3> oldEc;
   bool topoOk = true;
   for (int k = 0; k < 3; ++k) { int v = x.v[k]; ovUse[k] = vuse[v]; ovDeg[k] = bdeg[v]; ovBn[k] = bn[v]; }
   for (int k = 0; k < 3; ++k) {
    int a = x.v[k], b = x.v[(k + 1) % 3]; uint64_t e = patchEdgeKey(a, b);
    auto it = ec.find(e); oldEc[k] = it == ec.end() ? 0 : it->second;
    if (oldEc[k] >= 2) { topoOk = false; break; }
   }
   if (topoOk) {
    for (int k = 0; k < 3; ++k) { int v = x.v[k]; if (!vuse[v]) { touched.push_back(v); ++V; } ++vuse[v]; }
    for (int k = 0; k < 3; ++k) if (oldEc[k] == 1) {
     int a = x.v[k], b = x.v[(k + 1) % 3]; ec[patchEdgeKey(a, b)] = 2;
     if (!remB(a, b)) { topoOk = false; break; }
    }
    for (int k = 0; topoOk && k < 3; ++k) if (oldEc[k] == 0) {
     int a = x.v[k], b = x.v[(k + 1) % 3]; ec[patchEdgeKey(a, b)] = 1; ++E;
     if (!addB(a, b)) topoOk = false;
    }
    ++RF;
    if (topoOk && (V - E + RF != 1 || !oneCycle())) topoOk = false;
   }
   if (!topoOk) {
    V = ovV; E = ovE; RF = ovF; BE = ovBE;
    for (int k = 0; k < 3; ++k) { int v = x.v[k]; vuse[v] = ovUse[k]; bdeg[v] = ovDeg[k]; bn[v] = ovBn[k]; }
    for (int k = 0; k < 3; ++k) {
     uint64_t e = patchEdgeKey(x.v[k], x.v[(k + 1) % 3]);
     if (oldEc[k] == 0) ec.erase(e); else ec[e] = oldEc[k];
    }
    ++ps.growTopo; continue;
   }
   inside[f] = stateStamp; regionF.push_back(f);
   patchArea += cx.faceArea[f];
   patchBend = trialBend;
   patchRough = trialRough;
   for (int g : adj[f]) enqueue(g);
  }
  Region region;
  ps.regMaxF = max(ps.regMaxF, (int)regionF.size());
  if ((int)regionF.size() < pc.minF) { ++ps.regSmall; continue; }
  if (!finalizeRegion(ff, regionF, region, false)) { ++ps.regFinal; continue; }
  if ((int)region.interior.size() < pc.minInterior) { ++ps.regLowI; continue; }
  region.area = patchArea;
  region.bendRaw = patchBend;
  region.roughRaw = patchRough;
  int id = (int)regions.size();
  for (int f : region.faceIds) owner[f] = id;

  regions.push_back(region); ++ps.regions;
 }
 return regions;
}
bool buildLocalPatch(const vector<Vec3> &vv, const vector<Face> &ff,
             const Region &region, LocalPatch &out) const {
 out.localVerts = region.boundary;
 out.localVerts.insert(out.localVerts.end(), region.interior.begin(), region.interior.end());
 vector<int> g2l(vv.size(), -1);
 for (int i = 0; i < (int)out.localVerts.size(); ++i) g2l[out.localVerts[i]] = i;
 out.localFaces.clear(); out.localFaces.reserve(region.faceIds.size());
 for (int fi : region.faceIds) {
  const Face &f = ff[fi];
  array<int,3> t{g2l[f.v[0]], g2l[f.v[1]], g2l[f.v[2]]};
  if (t[0] < 0 || t[1] < 0 || t[2] < 0) return false;
  out.localFaces.push_back(t);
 }
 return true;
}
static bool pointInTri2(const Vec2 &p, const Vec2 &a, const Vec2 &b,
             const Vec2 &c, double eps) {
 return cross2(b-a,p-a) >= -eps && cross2(c-b,p-b) >= -eps &&
     cross2(a-c,p-c) >= -eps;
}
bool triangulateRing(const vector<Vec3> &vv, const LocalPatch &p, int center,
           const vector<int> &ring, const Vec3 &refNormal,
           vector<array<int,3>> &out) const {
 const int n = (int)ring.size();
 if (n < 3) return false;
 const Vec3 c0 = vv[p.localVerts[center]];
 Vec3 u;
 for (int x : ring) {
  Vec3 d = vv[p.localVerts[x]] - c0;
  d = d - refNormal * dot(d, refNormal);
  if (norm2(d) > cp_eps_n * cp_eps_n) { u = normalizedOrZero(d); break; }
 }
 if (norm2(u) <= cp_eps_n) return false;
 Vec3 vaxis = normalizedOrZero(cross(refNormal, u));
 if (norm2(vaxis) <= cp_eps_n) return false;
 vector<Vec2> q(n);
 for (int i = 0; i < n; ++i) {
  Vec3 d = vv[p.localVerts[ring[i]]] - c0;
  q[i] = Vec2(dot(d,u), dot(d,vaxis));
 }
 double polyArea = 0.0;
 for (int i = 0; i < n; ++i) polyArea += cross2(q[i],q[(i+1)%n]);
 if (polyArea <= pc.minUvArea * max(1.0, diag * diag)) return false;
 vector<int> poly(n); iota(poly.begin(),poly.end(),0);
 out.clear(); out.reserve(n-2);
 const double minArea = pc.minWorldArea * diag * diag;
 const double minDot = cos(pc.maxTurnDeg * acos(-1.0) / 180.0);
 while (poly.size() > 3) {
  int best = -1; double bestDot = -2.0, bestDiag = INF;
  for (int j = 0; j < (int)poly.size(); ++j) {
   int ia=poly[(j+(int)poly.size()-1)%poly.size()], ib=poly[j], ic=poly[(j+1)%poly.size()];
   if (cross2(q[ib]-q[ia],q[ic]-q[ia]) <= pc.minUvArea) continue;
   bool contains = false;
   for (int k : poly) if (k!=ia && k!=ib && k!=ic &&
       pointInTri2(q[k],q[ia],q[ib],q[ic],pc.minUvArea)) { contains=true; break; }
   if (contains) continue;
   Vec3 a=vv[p.localVerts[ring[ia]]], b=vv[p.localVerts[ring[ib]]],
      c=vv[p.localVerts[ring[ic]]];
   Vec3 cr=cross(b-a,c-a); double ar=norm(cr);
   if (ar <= minArea) continue;
   double nd=dot(cr/ar,refNormal);
   if (nd < minDot) continue;
   double dl=norm2(c-a);
   if (nd > bestDot + 1e-12 || (fabs(nd-bestDot)<=1e-12 &&
       (dl < bestDiag - 1e-18 || (fabs(dl-bestDiag)<=1e-18 && ib < (best<0?INT_MAX:poly[best]))))) {
    best=j; bestDot=nd; bestDiag=dl;
   }
  }
  if (best < 0) return false;
  int ia=poly[(best+(int)poly.size()-1)%poly.size()], ib=poly[best], ic=poly[(best+1)%poly.size()];
  out.push_back({ring[ia],ring[ib],ring[ic]});
  poly.erase(poly.begin()+best);
 }
 Vec3 a=vv[p.localVerts[ring[poly[0]]]], b=vv[p.localVerts[ring[poly[1]]]],
    c=vv[p.localVerts[ring[poly[2]]]];
 Vec3 cr=cross(b-a,c-a); double ar=norm(cr);
 if (ar <= minArea || dot(cr/ar,refNormal) < minDot) return false;
 out.push_back({ring[poly[0]],ring[poly[1]],ring[poly[2]]});
 return true;
}
void buildReplacements(const vector<Vec3> &vv, const vector<Face> &ff,
            const Region &region, const PatchComplexityData &cx,
            vector<Replacement> &out) const {
 Region core; int preservedFaces = 0;
 if (!buildBoundaryProtectedCore(ff, region, cx, core, preservedFaces)) return;
 ++ps.collarPass; ps.collarPreservedFaces += preservedFaces;
 ps.collarCoreFaces += core.faceIds.size();
 LocalPatch p;
 if (!buildLocalPatch(vv,ff,core,p)) { ++ps.structuralFail; return; }
 const int nb=(int)core.boundary.size(), oldInterior=(int)core.interior.size();
 if (nb<3 || oldInterior<=1) { ++ps.patchNoReduce; return; }
 const int wantedRemoved=oldInterior-(int)ceil(pc.keepRatio*oldInterior);
 if (wantedRemoved<=0) { ++ps.patchNoReduce; return; }
 const int nv=(int)p.localVerts.size();
 struct LocalFace { array<int,3> v; bool alive=true; };
 vector<LocalFace> lf; lf.reserve(p.localFaces.size()+2*wantedRemoved+16);
 vector<vector<int>> vertexFaces(nv);
 unordered_map<uint64_t,int> edgeCount; edgeCount.reserve(p.localFaces.size()*3);
 unordered_set<uint64_t> faceKeys; faceKeys.reserve(p.localFaces.size()*2);
 auto faceKey=[](int a,int b,int c) {
  array<uint32_t,3> q{(uint32_t)a,(uint32_t)b,(uint32_t)c}; sort(q.begin(),q.end());
  return (uint64_t)q[0]|((uint64_t)q[1]<<21)|((uint64_t)q[2]<<42);
 };
 auto addFace=[&](const array<int,3> &t) {
  int id=(int)lf.size(); lf.push_back({t,true});
  for (int k=0;k<3;++k) { vertexFaces[t[k]].push_back(id); ++edgeCount[patchEdgeKey(t[k],t[(k+1)%3])]; }
  faceKeys.insert(faceKey(t[0],t[1],t[2]));
 };
 for (const auto &t:p.localFaces) addFace(t);
 vector<char> vertexAlive(nv,1);
 vector<double> vBend(nv),vRough(nv); vector<int> g2l(vv.size(),-1);
 for (int i=0;i<nv;++i) g2l[p.localVerts[i]]=i;
 vector<char> inCore(ff.size(),0); for (int fi:core.faceIds) inCore[fi]=1;
 for (int fi:core.faceIds) for (int k=0;k<3;++k) {
  int g=cx.adjacency[fi][k]; if (g<0 || !inCore[g] || fi>g) continue;
  const auto &e=cx.edges[cx.edgeId[fi][k]]; int x=g2l[e.a],y=g2l[e.b];
  if (x>=0) { vBend[x]+=.5*e.bend; vRough[x]+=.5*e.rough; }
  if (y>=0) { vBend[y]+=.5*e.bend; vRough[y]+=.5*e.rough; }
 }
 double meanB=0,meanR=0;
 for (int i=nb;i<nv;++i) { meanB+=vBend[i]/max(diag,cp_eps_n); meanR+=vRough[i]; }
 meanB/=oldInterior; meanR/=oldInterior;
 struct Order { double score,rough,bend; int v; }; vector<Order> order; order.reserve(oldInterior);
 for (int i=nb;i<nv;++i) {
  double b=(vBend[i]/max(diag,cp_eps_n))/max(meanB,1e-30), r=vRough[i]/max(meanR,1e-30);
  order.push_back({max(b,r),r,b,i});
 }
 sort(order.begin(),order.end(),[](const Order&a,const Order&b){
  if (a.score!=b.score) return a.score<b.score;
  if (a.rough!=b.rough) return a.rough<b.rough;
  if (a.bend!=b.bend) return a.bend<b.bend;
  return a.v<b.v;
 });
 int removed=0;
 auto tryRemove=[&](int x) {
  if (x<nb || x>=nv || !vertexAlive[x]) return false;
  vector<int> incident;
  for (int fi:vertexFaces[x]) if (fi>=0 && fi<(int)lf.size() && lf[fi].alive) incident.push_back(fi);
  if (incident.size()<3) return false;
  unordered_map<int,int> succ,indeg; succ.reserve(incident.size()*2); indeg.reserve(incident.size()*2);
  Vec3 ref;
  for (int fi:incident) {
   const auto &t=lf[fi].v; int k=t[0]==x?0:(t[1]==x?1:(t[2]==x?2:-1));
   if (k<0) return false;
   int a=t[(k+1)%3],b=t[(k+2)%3];
   if (a==b || !vertexAlive[a] || !vertexAlive[b] || succ.count(a)) return false;
   succ[a]=b; ++indeg[b];
   Vec3 p0=vv[p.localVerts[t[0]]],p1=vv[p.localVerts[t[1]]],p2=vv[p.localVerts[t[2]]];
   Vec3 cr=cross(p1-p0,p2-p0); if (norm(cr)<=pc.minWorldArea*diag*diag) return false; ref=ref+cr;
  }
  if (succ.size()!=incident.size()) return false;
  for (const auto &kv:succ) if (indeg[kv.first]!=1) return false;
  ref=normalizedOrZero(ref); if (norm2(ref)<=cp_eps_n) return false;
  vector<int> ring; ring.reserve(succ.size()); int start=succ.begin()->first,cur=start;
  do { ring.push_back(cur); auto it=succ.find(cur); if (it==succ.end()) return false;
   cur=it->second; if (ring.size()>succ.size()) return false; } while(cur!=start);
  if (ring.size()!=succ.size()) return false;
  vector<array<int,3>> hole;
  if (!triangulateRing(vv,p,x,ring,ref,hole)) return false;
  unordered_map<uint64_t,int> delta; delta.reserve((incident.size()+hole.size())*3);
  unordered_set<uint64_t> removedKeys,newKeys; removedKeys.reserve(incident.size()*2); newKeys.reserve(hole.size()*2);
  for (int fi:incident) { const auto&t=lf[fi].v; removedKeys.insert(faceKey(t[0],t[1],t[2]));
   for (int k=0;k<3;++k) --delta[patchEdgeKey(t[k],t[(k+1)%3])]; }
  for (const auto&t:hole) {
   uint64_t fk=faceKey(t[0],t[1],t[2]);
   if (!newKeys.insert(fk).second || (faceKeys.count(fk)&&!removedKeys.count(fk))) return false;
   for (int k=0;k<3;++k) ++delta[patchEdgeKey(t[k],t[(k+1)%3])];
  }
  for (const auto&kv:delta) { int old=0; auto it=edgeCount.find(kv.first); if (it!=edgeCount.end()) old=it->second;
   int now=old+kv.second; if (now<0 || now>2) return false; }
  for (int fi:incident) { const auto&t=lf[fi].v; lf[fi].alive=false; faceKeys.erase(faceKey(t[0],t[1],t[2]));
   for (int k=0;k<3;++k) { uint64_t e=patchEdgeKey(t[k],t[(k+1)%3]); auto it=edgeCount.find(e);
    if (it!=edgeCount.end() && --it->second==0) edgeCount.erase(it); } }
  vertexAlive[x]=0; for (const auto&t:hole) addFace(t); return true;
 };
 for (const Order &o:order) {
  if (removed>=wantedRemoved || timeAfter(wr)) break;
  if (tryRemove(o.v)) ++removed;
 }
 if (removed<=0) { ++ps.patchNoReduce; return; }
 vector<int> remap(nv,-1); vector<Vec3> world; vector<double> radius;
 world.reserve(nv-removed); radius.reserve(nv-removed-nb);
 for (int i=0;i<nb;++i) { if (!vertexAlive[i]) return; remap[i]=(int)world.size(); world.push_back(vv[p.localVerts[i]]); }
 for (int i=nb;i<nv;++i) if (vertexAlive[i]) { remap[i]=(int)world.size(); world.push_back(vv[p.localVerts[i]]);
  int g=p.localVerts[i]; radius.push_back(g<(int)outRad.size()?outRad[g]:0.0); }
 vector<array<int,3>> newFaces; newFaces.reserve(p.localFaces.size()-2*removed);
 for (const LocalFace &f:lf) if (f.alive) {
  array<int,3> t{remap[f.v[0]],remap[f.v[1]],remap[f.v[2]]};
  if (t[0]<0 || t[1]<0 || t[2]<0 || t[0]==t[1] || t[1]==t[2] || t[0]==t[2]) return;
  Vec3 a=world[t[0]],b=world[t[1]],c=world[t[2]];
  if (norm(cross(b-a,c-a))<=pc.minWorldArea*diag*diag) return;
  newFaces.push_back(t);
 }
 Replacement r; r.oldFaces=core.faceIds; r.conflictFaces=region.faceIds; r.boundary=core.boundary;
 r.newInterior.assign(world.begin()+nb,world.end()); r.newRadius=move(radius); r.newFaces=move(newFaces);
 r.removed=removed; out.push_back(move(r)); ++ps.valid;
}
static void compactMesh(vector<Vec3> &vv, vector<Face> &ff,
               vector<double> *rr = nullptr) {
 vector<char> used(vv.size(), 0);
 for (const Face &f : ff)
  for (int k = 0; k < 3; ++k)
   if (f.v[k] >= 0 && f.v[k] < (int)vv.size())
    used[f.v[k]] = 1;
 vector<int> map(vv.size(), -1);
 vector<Vec3> nv;
 vector<double> nr;
 nv.reserve(vv.size());
 if (rr) nr.reserve(vv.size());
 for (int i = 0; i < (int)vv.size(); ++i)
  if (used[i]) {
   map[i] = (int)nv.size();
   nv.push_back(vv[i]);
   if (rr) nr.push_back(i < (int)rr->size() ? (*rr)[i] : 0.0);
  }
 for (Face &f : ff)
  for (int k = 0; k < 3; ++k)
   f.v[k] = map[f.v[k]];
 vv.swap(nv);
 if (rr) rr->swap(nr);
}
bool applyPatchPrefix(const vector<Vec3> &baseV, const vector<Face> &baseF,
           const vector<double> &baseR,
           const vector<Replacement> &selected, int count,
           vector<Vec3> &candV, vector<Face> &candF,
           vector<double> &candR) const {
 vector<char> removeFace(baseF.size(), 0);
 for (int i = 0; i < count; ++i)
  for (int fi : selected[i].oldFaces) {
   if (fi < 0 || fi >= (int)baseF.size() || removeFace[fi])
    return false;
   removeFace[fi] = 1;
  }
 candV = baseV;
 candR = baseR;
 if (candR.size() != candV.size()) candR.assign(candV.size(), 0.0);
 candF.clear();
 candF.reserve(baseF.size());
 for (int fi = 0; fi < (int)baseF.size(); ++fi)
  if (!removeFace[fi])
   candF.push_back(baseF[fi]);
 for (int i = 0; i < count; ++i) {
  const Replacement &r = selected[i];
  int base = (int)candV.size();
  candV.insert(candV.end(), r.newInterior.begin(), r.newInterior.end());
  candR.insert(candR.end(), r.newRadius.begin(),
        r.newRadius.end());
  int nb = (int)r.boundary.size();
  for (const auto &lt : r.newFaces) {
   Face f;
   for (int k = 0; k < 3; ++k) {
    int x = lt[k];
    f.v[k] = x < nb ? r.boundary[x] : base + (x - nb);
   }
   if (f.v[0] == f.v[1] || f.v[1] == f.v[2] || f.v[0] == f.v[2])
    return false;
   candF.push_back(f);
  }
 }
 compactMesh(candV, candF, &candR);
 return true;
}
bool structurallyValidMesh(const vector<Vec3> &vv, const vector<Face> &ff) const {
 unordered_map<uint64_t, pair<int, int>> edges;
 edges.reserve(ff.size() * 2);
 vector<array<int, 3>> keys; keys.reserve(ff.size());
 for (const Face &f : ff) {
  int a = f.v[0], b = f.v[1], c = f.v[2];
  if (a < 0 || b < 0 || c < 0 || a >= (int)vv.size() || b >= (int)vv.size() ||
    c >= (int)vv.size() || a == b || b == c || a == c ||
    !isFiniteVec(vv[a]) || !isFiniteVec(vv[b]) || !isFiniteVec(vv[c]) ||
    norm(cross(vv[b] - vv[a], vv[c] - vv[a])) <= pc.minWorldArea * diag * diag)
   return false;
  array<int, 3> key{a,b,c}; sort(key.begin(), key.end()); keys.push_back(key);
  int x[3] = {a,b,c};
  for (int k = 0; k < 3; ++k) {
   int u = x[k], v = x[(k + 1) % 3]; auto &e = edges[patchEdgeKey(u,v)];
   ++e.first; e.second += u < v ? 1 : -1;
  }
 }
 sort(keys.begin(), keys.end());
 for (int i = 1; i < (int)keys.size(); ++i) if (keys[i] == keys[i-1]) return false;
 for (const auto &kv : edges) if (kv.second.first != 2 || kv.second.second != 0) return false;
 return true;
}
void runPatchFirst() {
 if (!patchEnabled() || origV.empty() || origF.empty()) return;
 wr = v7.postReserve;
 const vector<Vec3> baseV = verts;
 const vector<Face> baseF = faces;
 vector<double> baseR(baseV.size(), 0.0);
 double phaseStart = elapsed();
 PatchComplexityData complexity = buildPatchComplexity(baseV, baseF);
 vector<Region> regions = findPatchRegions(baseV, baseF, complexity);
 ps.tRegions += elapsed() - phaseStart;

 vector<Replacement> replacements;
 for (const Region &region : regions) {
  if (timeAfter(wr)) break;
  buildReplacements(baseV, baseF, region, complexity, replacements);
 }
 sort(replacements.begin(), replacements.end(),
    [](const Replacement &a, const Replacement &b) { return a.removed > b.removed; });
 vector<Replacement> selected;
 unordered_set<int> used;
 int totalRemoved = 0;
 for (const Replacement &r : replacements) {
  bool conflict = false;
  vector<int> touched;
  const vector<int> &conflictFaces = r.conflictFaces.empty() ? r.oldFaces : r.conflictFaces;
  for (int fi : conflictFaces) {
   if (fi < 0 || fi >= (int)baseF.size()) { conflict = true; break; }
   for (int k = 0; k < 3; ++k) {
    int v = baseF[fi].v[k];
    if (used.count(v)) { conflict = true; break; }
    touched.push_back(v);
   }
   if (conflict) break;
  }
  if (conflict) { ++ps.batchConflict; continue; }
  sort(touched.begin(), touched.end());
  touched.erase(unique(touched.begin(), touched.end()), touched.end());
  for (int v : touched) used.insert(v);
  selected.push_back(r); totalRemoved += r.removed;
  if ((int)selected.size() >= v7.maxBatch) break;
 }
 ps.selected = (int)selected.size(); ps.predRm = totalRemoved;
 if (selected.empty()) { ++ps.noSelected; return; }
 vector<Vec3> candV; vector<Face> candF; vector<double> candR;
 if (!applyPatchPrefix(baseV, baseF, baseR, selected, (int)selected.size(),
             candV, candF, candR) || !structurallyValidMesh(candV, candF)) {
  ++ps.batchApply; return;
 }
 for (double r : candR) if (!isfinite(r)) { ++ps.batchApply; return; }
 if (candV.size() >= baseV.size()) { ++ps.batchNoReduce; return; }
 verts.swap(candV); faces.swap(candF); outRad.swap(candR);
 nV = (int)verts.size(); nF = (int)faces.size();
 ps.commit = 1; ps.actualRm = (int)baseV.size() - nV;
}
static double clampDouble(double x, double lo, double hi) {
 return x < lo ? lo : (x > hi ? hi : x);
}
static Pixel backgroundPixel() {
 Pixel p;
 p.n[0] = (float)cp_bg_n;
 p.n[1] = (float)cp_bg_n;
 p.n[2] = (float)cp_bg_n;
 p.d = (float)cp_bg_d;
 p.fg = 0;
 return p;
}
template <class PutPixel>
void rasterizeOfficialTri(const Vec3 tri[3], int view, int R, int clipX0,
             int clipY0, int clipX1, int clipY1,
             double minProjectedArea2, double minNormalNorm,
             PutPixel putPixel) const {
 ImpProj p0 = projectToView(tri[0], view, R);
 ImpProj p1 = projectToView(tri[1], view, R);
 ImpProj p2 = projectToView(tri[2], view, R);
 if (!p0.ok || !p1.ok || !p2.ok)
  return;
 double area2 =
   (p1.u - p0.u) * (p2.v - p0.v) - (p1.v - p0.v) * (p2.u - p0.u);
 if (minProjectedArea2 > 0.0 && fabs(area2) < minProjectedArea2)
  return;
 Vec3 nr = cross(tri[1] - tri[0], tri[2] - tri[0]);
 double nl = norm(nr);
 if (nl < minNormalNorm)
  return;
 nr = nr / nl;
 int bx0 = max(clipX0, (int)floor(min({p0.u, p1.u, p2.u})));
 int bx1 = min(clipX1, (int)ceil(max({p0.u, p1.u, p2.u})));
 int by0 = max(clipY0, (int)floor(min({p0.v, p1.v, p2.v})));
 int by1 = min(clipY1, (int)ceil(max({p0.v, p1.v, p2.v})));
 if (bx0 > bx1 || by0 > by1)
  return;
 double den = (p1.v - p2.v) * (p0.u - p2.u) + (p2.u - p1.u) * (p0.v - p2.v);
 if (fabs(den) < cp_projected_area_eps)
  return;
 for (int py = by0; py <= by1; ++py)
  for (int px = bx0; px <= bx1; ++px) {
   double sx = px + cp_pixel_center, sy = py + cp_pixel_center;
   double w0 =
     ((p1.v - p2.v) * (sx - p2.u) + (p2.u - p1.u) * (sy - p2.v)) / den;
   double w1 =
     ((p2.v - p0.v) * (sx - p2.u) + (p0.u - p2.u) * (sy - p2.v)) / den;
   double w2 = 1.0 - w0 - w1;
   if (w0 < -cp_bary_eps || w1 < -cp_bary_eps || w2 < -cp_bary_eps)
    continue;
   double iz = w0 / p0.z + w1 / p1.z + w2 / p2.z;
   if (iz <= 0.0)
    continue;
   putPixel(px, py, (float)(1.0 / iz), nr);
  }
}
void renderFullSsimBuffer(const vector<Vec3> &vv, const vector<Face> &ff,
             int view, int R, vector<Pixel> &buf) const {
 Pixel bg = backgroundPixel();
 buf.assign(R * R, bg);
 vector<float> zbuf(R * R, numeric_limits<float>::infinity());
 for (const Face &f : ff) {
  if (f.v[0] < 0 || f.v[1] < 0 || f.v[2] < 0 || f.v[0] >= (int)vv.size() ||
    f.v[1] >= (int)vv.size() || f.v[2] >= (int)vv.size())
   continue;
  Vec3 tri[3] = {vv[f.v[0]], vv[f.v[1]], vv[f.v[2]]};
  rasterizeOfficialTri(tri, view, R, 0, 0, R - 1, R - 1, cp_projected_area_eps,
            cp_frame_eps,
            [&](int px, int py, float z, const Vec3 &nr) {
             int idx = py * R + px;
             if (z >= zbuf[idx])
              return;
             zbuf[idx] = z;
             buf[idx].n[0] = (float)((nr.x + 1.0) * cp_bg_n);
             buf[idx].n[1] = (float)((nr.y + 1.0) * cp_bg_n);
             buf[idx].n[2] = (float)((nr.z + 1.0) * cp_bg_n);
             buf[idx].d = z;
             buf[idx].fg = 1;
            });
 }
}
static double ssimFromWindowSums(double sx, double sy, double sxx, double syy,
                double sxy) {
 constexpr double inv = 1.0 / double(cp_ssim_area);
 double mx = sx * inv, my = sy * inv;
 double vx = max(0.0, sxx * inv - mx * mx),
    vy = max(0.0, syy * inv - my * my);
 double cov = sxy * inv - mx * my;
 double num = (2.0 * mx * my + cp_ssim_c1) * (2.0 * cov + cp_ssim_c2);
 double den = (mx * mx + my * my + cp_ssim_c1) * (vx + vy + cp_ssim_c2);
 if (den <= 0.0)
  return 1.0;
 return clampDouble(num / den, -1.0, 1.0);
}
const FinalTxCfg &mediumFinalCfg() const {
 if (TIER == 2)
  return ftx_t2;
 if (TIER == 3)
  return ftx_t3;
 return ftx_t4;
}
bool mediumSsimPasses(const Ssim &s) const {
 return s.mean >= hp_medium_ssim_accept;
}
void resetToTxBase(const vector<Vec3> &safeV, const vector<Face> &safeF,
         const vector<double> &safeR, double oldDiag,
         double oldHausdorff) {
 verts = safeV;
 faces = safeF;
 nV = (int)verts.size();
 nF = (int)faces.size();
 vneigh.assign(nV, SmallSet());
 buildConnectivity();
 if (safeR.size() == verts.size())
  crad = safeR;
 else
  crad.assign(nV, 0.0);
 diag = oldDiag;
 hausd = oldHausdorff;
 invDiagSquared = diag > cp_eps_n ? 1.0 / (diag * diag) : 0.0;
 costCap = hp_qem_cap * diag * diag;
 priority_queue<Cand> empty;
 pq.swap(empty);
 acc = 0;
}
void runSimpleRatioQem(double ratio, const FinalTxCfg &cfg,
            double reserve) {
 qTier = cfg.qTier;
 if (cfg.looseExposure)
  buildFaceExposure(cfg.expR, exp_loose);
 else
  buildFaceExposure(cfg.expR);
 rebuildQuadrics();
 targetV = max(cp_min_work_vertices, (int)floor(inputV * ratio));
 targetV = min(targetV, nV);
 budget = max(0, nV - targetV);
 acc = 0;
 priority_queue<Cand> empty;
 pq.swap(empty);
 setStopReserve(max(cfg.qemStopMargin, reserve));
 if (qTier == 4) {
  vmoment.assign(nV, Quadric());
  for (int i = 0; i < nV; ++i)
   vmoment[i] = pointAnchorQuadric(verts[i], boot_cfg.q4AnchorW);
 }
 screenGuard = true;
 seedPriorityQueue();
 collapseLoop();
 screenGuard = false;
 compact();
}
void tx_medium(bool patchAfter) {
 if (TIER < 2 || TIER > 4 || origV.empty() || origF.empty())
  return;
 const FinalTxCfg &cfg = mediumFinalCfg();
 const double oldDiag = diag, oldHausdorff = hausd;
 vector<Vec3> acceptedV = verts;
 vector<Face> acceptedF = faces;
 vector<double> acceptedR = outRad;
 if (acceptedR.size() != acceptedV.size())
  acceptedR.assign(acceptedV.size(), 0.0);
 auto restoreAccepted = [&]() {
  verts = acceptedV; faces = acceptedF; outRad = acceptedR;
  nV = (int)verts.size(); nF = (int)faces.size();
 };
 auto saveAccepted = [&]() {
  acceptedV = verts; acceptedF = faces; acceptedR = outRad;
  if (acceptedR.size() != acceptedV.size())
   acceptedR.assign(acceptedV.size(), 0.0);
 };
 if (TIER == 2 && timeAfter(cfg.entryMargin)) {
  origV.clear(); origF.clear(); return;
 }
 RenderSet ref = renderSsimReference(origV, origF, cfg.R);
 if (TIER == 2 && timeAfter(cfg.refMargin)) {
  origV.clear(); origF.clear(); return;
 }
 double scoreStart = elapsed();
 Ssim acceptedScore = scoreAgainstReference(ref, acceptedV, acceptedF, cfg.R);
 double auditCost = max(hp_ltx_ratio_eps, elapsed() - scoreStart);
 wr = auditCost * v7.auditSafety + v7.outputReserve;
#ifdef LOCAL_DIAGNOSTICS
 fprintf(stderr, "K16 BASE source=%s vertices=%zu mean=%.9f view=%.9f normal=%.9f depth=%.9f accepted=%d\n",
     acceptedV.size() < origV.size() ? "LEGACY_QEM" : "ORIGINAL", acceptedV.size(),
     acceptedScore.mean, acceptedScore.minView, acceptedScore.minNormal, acceptedScore.minDepth,
     mediumSsimPasses(acceptedScore) ? 1 : 0);
#endif
 if (!mediumSsimPasses(acceptedScore)) {
  acceptedV = origV; acceptedF = origF; acceptedR.assign(origV.size(), 0.0);
  acceptedScore = Ssim{1.0,1.0,1.0,1.0}; restoreAccepted();
 }
 auto tryQem = [&](double ratio) {
  double current = double(acceptedV.size()) / double(inputV);
  if (ratio + hp_ltx_ratio_eps >= current || !completeAuditFits(auditCost))
   return false;
  wr = auditCost * v7.auditSafety + v7.outputReserve;
  resetToTxBase(acceptedV, acceptedF, acceptedR, oldDiag, oldHausdorff);
  runSimpleRatioQem(ratio, cfg, wr);
  if (verts.size() >= acceptedV.size() || !structurallyValidMesh(verts, faces)) {
#ifdef LOCAL_DIAGNOSTICS
   fprintf(stderr, "K16 QEM ratio=%.8f vertices=%zu structural=0 accepted=0\n", ratio, verts.size());
#endif
   restoreAccepted();
   return false;
  }
  scoreStart = elapsed();
  Ssim score = scoreAgainstReference(ref, verts, faces, cfg.R);
  auditCost = max(auditCost, elapsed() - scoreStart);
  wr = auditCost * v7.auditSafety + v7.outputReserve;
  if (!mediumSsimPasses(score)) {
#ifdef LOCAL_DIAGNOSTICS
   fprintf(stderr, "K16 QEM ratio=%.8f vertices=%zu mean=%.9f view=%.9f normal=%.9f depth=%.9f accepted=0\n",
       ratio, verts.size(), score.mean, score.minView, score.minNormal, score.minDepth);
#endif
   restoreAccepted();
   return false;
  }
#ifdef LOCAL_DIAGNOSTICS
  fprintf(stderr, "K16 QEM ratio=%.8f vertices=%zu mean=%.9f view=%.9f normal=%.9f depth=%.9f accepted=1\n",
      ratio, verts.size(), score.mean, score.minView, score.minNormal, score.minDepth);
#endif
  acceptedScore = score;
  saveAccepted();
  return true;
 };
 double hi = double(acceptedV.size()) / double(inputV);
 double lo = min(cfg.loRatio, hi);
 for (int it = 0; it < cfg.iters && lo + hp_ltx_ratio_eps < hi &&
           completeAuditFits(auditCost); ++it) {
  double ratio = 0.5 * (lo + hi);
  if (tryQem(ratio))
   hi = double(acceptedV.size()) / double(inputV);
  else
   lo = ratio;
 }
 restoreAccepted();
 bool patchCommitted = false;
 if (patchAfter && patchEnabled() && timeBefore(v7.postReserve) && completeAuditFits(auditCost)) {
  size_t beforePatch = acceptedV.size();
  runPatchFirst();
  if (verts.size() < beforePatch && structurallyValidMesh(verts, faces) && completeAuditFits(auditCost)) {
   scoreStart = elapsed();
   Ssim patchScore = scoreAgainstReference(ref, verts, faces, cfg.R);
   auditCost = max(auditCost, elapsed() - scoreStart);
   wr = auditCost * v7.auditSafety + v7.outputReserve;
   if (mediumSsimPasses(patchScore)) {
    acceptedScore = patchScore;
    saveAccepted();
    patchCommitted = true;
#ifdef LOCAL_DIAGNOSTICS
    fprintf(stderr, "K16 PATCH vertices=%zu removed=%zu mean=%.9f view=%.9f normal=%.9f depth=%.9f accepted=1\n",
        verts.size(), beforePatch - verts.size(), patchScore.mean, patchScore.minView,
        patchScore.minNormal, patchScore.minDepth);
#endif
   } else {
#ifdef LOCAL_DIAGNOSTICS
    fprintf(stderr, "K16 PATCH vertices=%zu removed=%zu mean=%.9f view=%.9f normal=%.9f depth=%.9f accepted=0\n",
        verts.size(), beforePatch - verts.size(), patchScore.mean, patchScore.minView,
        patchScore.minNormal, patchScore.minDepth);
#endif
    restoreAccepted();
   }
  } else {
#ifdef LOCAL_DIAGNOSTICS
   fprintf(stderr, "K16 PATCH generated=%d vertices=%zu base=%zu auditable=0 accepted=0\n",
       ps.commit, verts.size(), beforePatch);
#endif
   restoreAccepted();
  }
 }
 restoreAccepted();
#ifndef LOCAL_DIAGNOSTICS
 (void)patchCommitted;
#endif
#ifdef LOCAL_DIAGNOSTICS
 fprintf(stderr, "K16 FINAL state=%s vertices=%zu mean=%.9f\n",
     patchCommitted ? "PATCH_AFTER_QEM" : (acceptedV.size() < origV.size() ? "QEM" : "ORIGINAL"),
     acceptedV.size(), acceptedScore.mean);
#endif
 origV.clear(); origF.clear();
}
void compact() {
 vector<int> o2n(verts.size(), -1);
 vector<Vec3> nv;
 nv.reserve(verts.size() - acc);
 vector<double> nr;
 nr.reserve(verts.size() - acc);
 for (int i = 0; i < (int)verts.size(); ++i)
  if (!vdead[i]) {
   o2n[i] = (int)nv.size();
   nv.push_back(verts[i]);
   nr.push_back(i < (int)crad.size() ? crad[i] : 0.0);
  }
 struct FK {
  array<int, 3> key;
  Face face;
  bool operator<(const FK &o) const { return key < o.key; }
 };
 vector<FK> fc;
 fc.reserve(faces.size());
 for (int fi = 0; fi < (int)faces.size(); ++fi) {
  if (fdead[fi])
   continue;
  int a = faces[fi].v[0], b = faces[fi].v[1], c = faces[fi].v[2];
  if (a < 0 || b < 0 || c < 0 || a >= (int)verts.size() ||
    b >= (int)verts.size() || c >= (int)verts.size())
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
 for (auto &item : fc) {
  if (item.key == prev)
   continue;
  prev = item.key;
  nf.push_back(item.face);
 }
 verts.swap(nv);
 faces.swap(nf);
 outRad.swap(nr);
 nV = (int)verts.size();
 nF = (int)faces.size();
}
};
bool Solver::useLiteConn = false;
int main() {
Solver s;
s.run();
return 0;
}
