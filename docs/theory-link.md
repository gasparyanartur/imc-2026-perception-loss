# Theory Link: Perception-Aware Mesh Simplification for IMC2026

## Source basis

This document links the competition formulation to the methods in **Kulkarni & Narayanan (2025), _A Comprehensive Guide to Mesh Simplification using Edge Collapse_**, and converts that theory into a practical implementation plan.

Primary sources:

- `problem_formulation.md`
- `problem-formulation-dense.md`
- `formulation.md`
- `Kulkarni and Narayanan - 2025 - A Comprehensive Guide to Mesh Simplification using Edge Collapse.pdf`

---

## 1. Core interpretation of the problem

The challenge is not generic mesh simplification. It is constrained vertex minimization under a very specific judge:

\[
\min_{\widehat M} |\widehat V|
\]

subject to:

\[
1 \le |\widehat V| \le |V_0|
\]

\[
\widehat M \text{ is a closed triangular 2-manifold}
\]

\[
\forall \widehat\tau\in\widehat T,\quad A(\widehat\tau)>0
\]

\[
d_H(M_0,\widehat M) \le 0.05D_{\mathrm{AABB}}(M_0)
\]

\[
\mathrm{FinalSSIM}(M_0,\widehat M) \ge 0.9
\]

The judge renders six axial views:

\[
E_k\in\{(\pm 2.5,0,0),(0,\pm 2.5,0),(0,0,\pm 2.5)\}
\]

For each view it compares:

1. **Flat face-normal maps**.
2. **Perspective-correct depth maps**.

There are no textures, colors, materials, or vertex normals. Face count is not directly scored. Vertex count is the ranking objective once the hard constraints and SSIM threshold pass.

### Consequence

The practical target is:

> Remove as many vertices as possible while preserving closed-manifold topology, approximate Hausdorff safety, six-view silhouettes, six-view depth, and flat face normals.

This makes a standard QEM collapse useful but insufficient. We need **perception-aware, feature-preserving QEM**.

---

## 2. Theoretical anchor: edge collapse

Kulkarni & Narayanan identify edge collapse as the dominant practical simplification primitive. An edge collapse replaces an edge \((v_i,v_j)\) by a single vertex \(v'\), removes the two adjacent triangles, and locally reconnects the one-ring.

The generic loop is:

1. Compute a collapse cost for each edge.
2. Store edges in a priority queue.
3. Pop the cheapest edge.
4. Check whether collapse preserves mesh validity.
5. Collapse if valid.
6. Update local connectivity and local costs.
7. Repeat until a target size or error limit is reached.

This matches our setting because:

- the input/output mesh is triangular,
- the output must remain a closed 2-manifold,
- local edge collapse can preserve topology when guarded correctly,
- the maximum mesh size requires near-linear or \(O(E\log E)\) behavior,
- global remeshing/retiling is too risky under manifold and SSIM constraints.

---

## 3. Relevancy table: methods, tricks, and fit to our judge

| Article method / trick | Description | Fit for this problem | Decision |
|---|---|---:|---|
| Coplanar patch merging | Detects near-coplanar regions, merges them, and retriangulates. | Strong conceptual fit because flat regions can be simplified aggressively with little normal/depth change. Direct retriangulation risks manifold failures. | Use the idea through edge collapses, not arbitrary patch remeshing. |
| Vertex clustering | Groups nearby vertices and replaces each cluster by one representative. | Fast but topology-unsafe and silhouette-unsafe. Can violate manifoldness and produce bad SSIM. | Reject for final solution. Maybe use only as a diagnostic baseline. |
| Iterative local decimation | Removes vertices/edges/faces through local cost functions. | Best match. Local, scalable, compatible with validity checks. | Primary framework. |
| Simplification envelopes | Keeps simplified mesh within bounded offset surfaces. | Strong match to Hausdorff constraint. Exact envelope construction is heavy. | Approximate with max collapse-length and local displacement guards. |
| Energy-based optimization | Uses global energy and operations like collapse/swap/split. | Quality can be high, but global optimization is too expensive at 1.1M vertices. | Borrow local energy terms only. |
| Retiling | Samples a reduced vertex set and reconstructs triangulation. | Topology and flat-normal preservation are risky. | Reject. |
| Voxelization + marching cubes | Converts mesh to volumetric field and extracts simplified surface. | Smooths sharp details, lacks explicit metric control, expensive. | Reject. |
| Neural mesh simplification | Learns vertex selection or remeshing. | Too much engineering; no controlled train distribution; topology risk. | Reject for current contest path. |
| Neural progressive meshes | QEM base mesh plus neural remeshing/splits. | Output must be a plain mesh; no decoder/latent channel. | Reject. |
| SDF / implicit LOD | Represents shape implicitly and extracts surfaces at different resolutions. | Extraction can damage flat normals, sharp features, and topology. | Reject. |
| Corner table | Compact triangle-corner connectivity. | Good for large meshes; memory-friendly. | Recommended conceptually. |
| Half-edge | Explicit local topology structure. | Convenient but memory-heavy at 2.1M faces. | Avoid full OO half-edge; use compact adjacency. |
| Priority queue over edge costs | Repeatedly collapses cheapest edge. | Standard and scalable. | Use. |
| Lazy priority queue invalidation | Push updated edge costs without decrease-key; discard stale entries on pop. | Reduces implementation complexity and keeps runtime acceptable. | Use. |
| Local cost recomputation | Recompute only edges near changed vertex. | Essential for speed. | Use. |
| Triangle flip check | Reject if local face orientation inverts. | Essential for manifold safety and normal-map preservation. | Use as hard validity check. |
| Two-neighbor / link condition | Endpoints of a collapsible closed-manifold edge should share exactly two common neighbors. | Directly relevant. Prevents non-manifold collapse. | Use as hard validity check. |
| Boundary merge check | Prevents merging holes/boundaries. | Input and output should be closed; no real boundary should appear. | Keep as sanity logic, but not central. |
| Plane QEM | Penalizes squared distance to incident face planes. | Strong geometric baseline; cheap and local. | Use as base cost. |
| Boundary QEM imaginary plane | Adds a perpendicular plane to keep boundary vertices on boundary. | No real boundary. But the idea is excellent for preserving feature/silhouette edges. | Repurpose as pseudo-boundary protection. |
| Treat attribute boundaries as boundaries | Preserves material/color discontinuities. | No attributes. But high-dihedral and silhouette edges are analogous discontinuities in the evaluator. | Repurpose for high-dihedral/silhouette edges. |
| Volume preservation | Preserves signed local volume but underdetermines vertex position. | Useful heuristic for depth/contour preservation. | Use only as soft inspiration. |
| Volume optimization | Minimizes squared unsigned local volume change. | Relevant to depth shrinkage and silhouette stability. | Use as secondary term if runtime permits. |
| Boundary preservation | Preserves boundary area. | No true boundary. | Ignore except for pseudo-feature curves. |
| Boundary optimization | Minimizes squared boundary-area change. | Same as above. | Usually ignore. |
| Triangle shape optimization | Reduces skinny triangles by minimizing squared lengths to neighbors. | Useful for numerical robustness and nondegenerate faces. Can over-smooth if overweighted. | Use small regularizer. |
| Midpoint fallback | Uses edge midpoint when QEM solve is singular. | Safe, but sometimes worse than choosing an endpoint. | Use multiple candidates: endpoint A, endpoint B, midpoint, QEM point. |
| Constraint selection / independence test | Selects stable independent equations. | Useful in theory, but more code than needed initially. | Replace with damped 3×3 solve + candidates. |
| Weighted constraint sum | Combines QEM, volume, boundary, shape, etc. | Very relevant because we need tunable contest heuristics. | Use. |
| Higher-dimensional QEM | Extends QEM to attributes. | No output attributes. | Repurpose conceptually: treat view-normal/depth sensitivity as pseudo-attributes in the cost. |
| Hoppe progressive-mesh energy | Distance + spring + scalar attribute + discontinuity penalty. | Too expensive literally, but terms map well to our problem. | Borrow local distance, spring, and discontinuity penalties. |
| Surface sampling | Uses points on original surface to estimate distance. | Useful for local Hausdorff safety. | Add in later version. |
| Sharp-edge sampling | Extra samples along discontinuities. | Highly relevant for high-dihedral and silhouette edges. | Add in later version. |
| Spring energy | Penalizes long local edges. | Helps avoid poor triangle quality. | Small weight only. |
| Scalar attribute energy | Preserves color/texture/etc. | No true attributes. | Repurpose as normal/depth saliency cost. |
| Discontinuity preservation offset | Penalizes or forbids collapse of sharp discontinuities. | Highly relevant: normal discontinuities and silhouettes dominate SSIM. | Use large penalty or lock. |
| Alternating projection optimization | Alternates projection and vertex-position solve. | Too slow online. | Avoid initially. |
| Spatial partitioning for projections | Accelerates closest-face queries. | Useful if adding Hausdorff/local evaluator. | Later improvement. |
| Locality assumption | Collapse only changes local neighborhood. | Essential for scalable implementation. | Use throughout. |

---

## 4. Recommended approach ranking

### Rank 1: Perception-aware QEM edge collapse

This is the best initial solution.

Use QEM as the geometric baseline:

\[
C_{\mathrm{QEM}}(v)=\sum_{(n,d)\in\mathcal P}(n^Tv+d)^2
\]

Expanded:

\[
C_{\mathrm{QEM}}(v)=v^THv+2c^Tv+k
\]

with optimum:

\[
v^*=-H^{-1}c
\]

Then modify the collapse cost:

\[
C(e,v)=
W_Q C_{\mathrm{QEM}}
+W_N C_{\Delta n}
+W_S C_{\mathrm{sil}}
+W_D C_{\mathrm{dihedral}}
+W_L C_{\mathrm{length}}
+W_T C_{\mathrm{shape}}
\]

This directly maps the theory to the judge:

- QEM controls geometric deviation.
- Normal-change cost protects flat normal maps.
- Silhouette cost protects foreground/background and depth contours.
- Dihedral cost protects sharp visual features.
- Length/shape cost avoids degenerate and skinny triangles.

### Rank 2: Conservative QEM + high-dihedral lock

Simpler, safer, less aggressive.

Lock or heavily penalize collapse of:

- high-dihedral edges,
- likely six-view silhouette edges,
- edges whose collapse flips local face normals,
- edges longer than a small fraction of AABB diagonal.

This is a strong baseline if the full perceptual cost becomes too slow.

### Rank 3: Planar-region post-pass

After the first simplification, detect near-planar regions and collapse more aggressively:

\[
1-n_i^Tn_j < \epsilon_n
\]

for neighboring face normals. This can increase compression while preserving flat normal maps.

Risk: if the region contributes to silhouettes, excessive collapse can still damage depth/foreground SSIM.

### Reject for now

- vertex clustering,
- voxelization,
- neural simplification,
- retiled global remeshing,
- SDF reconstruction.

These are too risky under the manifold, Hausdorff, and flat-normal/depth SSIM constraints.

---

## 5. Practical recommendations

### 5.1 Preserve the true judge features

The evaluator does not care about smooth vertex normals. It uses flat face normals. Therefore, a collapse is dangerous if it changes visible face normals, even when geometric distance remains small.

Approximate normal penalty:

\[
C_{\Delta n}(e,v')=
\sum_{f\in\mathcal N(e)}
A_f^{\mathrm{proj}}
\left(1-n_f^T\widehat n_f\right)
\]

where \(A_f^{\mathrm{proj}}\) is an approximate six-view projected-area weight.

### 5.2 Preserve silhouettes aggressively

A local edge is silhouette-like in view direction \(r\) if the adjacent face normals face opposite sides:

\[
(n_{f_1}\cdot r)(n_{f_2}\cdot r)<0
\]

Approximate silhouette weight:

\[
S(e)=\sum_{r\in\{\pm x,\pm y,\pm z\}}
\mathbf 1[(n_{f_1}\cdot r)(n_{f_2}\cdot r)<0]
\cdot \ell_{\mathrm{proj}}(e,r)
\]

Then either lock the edge or add:

\[
C_{\mathrm{sil}}(e,v')=S(e)\|v'-v_{\mathrm{safe}}\|^2
\]

where \(v_{\mathrm{safe}}\) can be the closer endpoint. Endpoint candidates often preserve silhouettes better than midpoint/QEM candidates.

### 5.3 Use pseudo-boundaries

The article's boundary-QEM trick adds an imaginary plane to stop boundary drift. Our meshes have no boundaries, but the same idea applies to **feature edges**:

- high dihedral,
- six-view silhouette,
- strong projected length,
- local normal discontinuity.

Practical approximation:

- do not literally add all imaginary planes initially;
- instead, multiply cost or forbid collapse for feature edges.

### 5.4 Candidate positions are safer than one optimal point

For edge \(e=(a,b)\), test:

\[
\mathcal C_e=\{p_a,p_b,(p_a+p_b)/2,p_{\mathrm{QEM}}\}
\]

Pick the valid candidate with lowest cost.

This handles:

- singular QEM systems,
- flat areas,
- bad QEM points outside the local feature region,
- cases where preserving one endpoint is visually better than interpolating.

### 5.5 Use hard topological guards

Before accepting a collapse:

\[
|N(a)\cap N(b)|=2
\]

for a closed triangular 2-manifold edge.

Reject if any resulting face has:

\[
A(f')\le\epsilon_A
\]

Reject if any local face normal changes too much:

\[
n_f^T\widehat n_f < \cos(\theta_{\max})
\]

Reject if the candidate displacement is too large relative to the AABB diagonal:

\[
\max(\|v'-a\|,\|v'-b\|)>\lambda D_{\mathrm{AABB}}
\]

### 5.6 Do not optimize face count directly

The score is based on vertex count. Face count matters only because:

- it affects output size,
- it affects rasterized appearance,
- it affects runtime,
- invalid faces/edges cause WA.

---

## 6. Proposed pseudo-algorithm

### 6.1 Top-level algorithm

```text
read mesh M0=(V,F)
compute AABB diagonal D
build compact connectivity
compute face normals and face areas
compute per-vertex QEM quadrics
compute per-edge dihedral and approximate six-view silhouette saliency
initialize priority queue with best valid collapse for every edge

while active_vertex_count > target_vertex_count and queue not empty:
    item = pop cheapest queue item

    if item is stale:
        continue

    recompute best collapse for this edge

    if collapse is invalid:
        continue

    if cost exceeds safety budget:
        break

    apply edge collapse
    update local connectivity
    update local faces, normals, quadrics, dihedral, and saliency
    push updated neighboring edge costs

compact vertices and faces
validate no degenerate faces
validate every undirected edge has incidence two
write mesh
```

### 6.2 Best-collapse routine

```text
best_collapse(edge e=(a,b)):
    if a or b inactive:
        return invalid

    if common_neighbors(a,b) != 2:
        return invalid

    if edge_length(a,b) > MaxCollapseLength:
        return invalid

    candidates = []
    candidates.add(position[a])
    candidates.add(position[b])
    candidates.add(midpoint(a,b))

    qem_point = solve_qem(Q[a]+Q[b])
    if qem_point exists:
        candidates.add(qem_point)

    best = invalid

    for v_new in candidates:
        if creates_repeated_indices(e,v_new):
            continue
        if creates_degenerate_faces(e,v_new):
            continue
        if creates_duplicate_faces(e,v_new):
            continue
        if flips_or_over-rotates_normals(e,v_new):
            continue
        if violates_local_displacement_guard(e,v_new):
            continue

        cost = collapse_cost(e,v_new)

        if best invalid or cost < best.cost:
            best = (e,v_new,cost)

    return best
```

### 6.3 Collapse cost

```text
collapse_cost(e=(a,b), v_new):
    qem_cost = eval_quadric(Q[a]+Q[b], v_new)

    normal_cost = 0
    shape_cost = 0
    length_cost = 0

    for each affected face f:
        old_n = normal[f]
        new_n = simulated_normal_after_replacing(a/b by v_new)
        normal_cost += projected_area_weight[f] * (1 - dot(old_n,new_n))

    for each affected edge g:
        length_cost += squared_new_edge_length(g)

    dihedral_cost = max(0, dihedral[e] - SoftDihedral)^2
    silhouette_cost = silhouette_saliency[e]

    if dihedral[e] > LockDihedral and silhouette_saliency[e] > threshold:
        silhouette_cost *= FeaturePenaltyMultiplier

    return
        W_QEM        * qem_cost       +
        W_Normal     * normal_cost    +
        W_Dihedral   * dihedral_cost  +
        W_Silhouette * silhouette_cost+
        W_Length     * length_cost    +
        W_Shape      * shape_cost
```

---

## 7. Initial implementation design

### 7.1 Parameter separation

Use explicit constants and hyperparameters.

Constants are fixed by the judge/problem:

```cpp
static constexpr double CParam_CameraDistance = 2.5;
static constexpr double CParam_FocalLength = 800.0;
static constexpr int    CParam_ImageResolution = 1024;
static constexpr double CParam_HausdorffFrac = 0.05;
static constexpr double CParam_SSIMThreshold = 0.90;
```

Hyperparameters are algorithmic and tunable:

```cpp
static constexpr double HParam_TargetVertexRatio_Small  = 0.70;
static constexpr double HParam_TargetVertexRatio_Medium = 0.78;
static constexpr double HParam_TargetVertexRatio_Large  = 0.88;

static constexpr double HParam_MaxCollapseLengthFrac = 0.018;
static constexpr double HParam_MinTriangleArea       = 1e-14;

static constexpr double HParam_W_QEM        = 1.0;
static constexpr double HParam_W_Normal     = 2.0;
static constexpr double HParam_W_Dihedral   = 4.0;
static constexpr double HParam_W_Silhouette = 8.0;
static constexpr double HParam_W_Length     = 0.01;
static constexpr double HParam_W_Shape      = 0.02;

static constexpr double HParam_LockFeatureDihedralDeg = 48.0;
static constexpr double HParam_SoftDihedralDeg        = 20.0;
static constexpr double HParam_MaxNormalChangeDeg     = 70.0;
```

### 7.2 Data structures

Avoid object-heavy half-edge structures for large meshes. Prefer compact arrays.

Suggested structures:

```cpp
struct Vec3 { double x, y, z; };
struct Face { int v[3]; bool alive; };
struct Edge { int a, b; int f0, f1; int version; bool alive; };
struct Quadric { double q[10]; };
```

Core arrays:

```cpp
vector<Vec3> vertices;
vector<Face> faces;
vector<char> vertex_alive;
vector<char> face_alive;
vector<Vec3> face_normals;
vector<double> face_areas;
vector<Quadric> vertex_quadrics;
vector<vector<int>> vertex_faces;
vector<vector<int>> vertex_neighbors;
vector<Edge> edges;
priority_queue<QueueItem> pq;
```

For an initial implementation, rebuilding connectivity after each accepted batch is simpler and safer than maintaining a fully dynamic half-edge. But repeated global rebuilds become expensive. A practical compromise:

1. Use local edge updates for small/medium meshes.
2. Use batched conservative collapses for large meshes.
3. Compact/rebuild after a controlled number of accepted collapses.

### 7.3 QEM implementation

For a face with unit normal \(n\) and plane offset:

\[
d=-n^Tp_0
\]

fundamental quadric:

\[
(n_xx+n_yy+n_zz+d)^2
\]

Store compressed symmetric 4×4 coefficients:

```cpp
q00 q01 q02 q03
    q11 q12 q13
        q22 q23
            q33
```

For a vertex, sum incident face quadrics. For an edge collapse candidate, use:

\[
Q_e=Q_a+Q_b
\]

Evaluate candidate \(v=(x,y,z,1)\):

\[
v^TQ_ev
\]

For the optimal QEM point, solve the upper-left 3×3 system:

\[
H p = -c
\]

If singular/unstable, rely on endpoint/midpoint candidates.

### 7.4 Validity checks

Minimum checks:

```text
common-neighbor count exactly 2
candidate does not create repeated face indices
candidate does not create near-zero area triangles
candidate does not create duplicate triangles in local one-ring
candidate does not flip local face normals
candidate does not move farther than MaxCollapseLength
```

The common-neighbor condition is the key manifold guard for closed triangular meshes.

### 7.5 Compaction

After simplification, remove inactive vertices/faces and remap indices.

```text
old vertex id -> new compact id
copy alive vertices
copy alive faces using remapped ids
skip dead/degenerate faces defensively
```

Then optionally verify:

```text
all indices valid
no repeated face indices
every triangle area > eps
every undirected edge incidence == 2
```

### 7.6 Output precision

Use compact decimal output. Excess decimals can hit the 100 MiB output limit. The baseline uses `%.10g`, which is a reasonable default.

---

## 8. Initial tuning plan

The first implementation should be conservative. Passing all cases matters more than maximum compression on the first run.

Suggested sweep dimensions:

```text
TargetVertexRatio_Small:  0.55, 0.65, 0.75
MaxCollapseLengthFrac:    0.012, 0.018, 0.025
LockFeatureDihedralDeg:   35, 48, 65
```

Interpretation:

- Lower target ratio means more aggressive compression.
- Higher collapse length allows more geometric movement, more risk to Hausdorff/depth.
- Higher lock dihedral allows simplifying sharper features, more risk to normal-map SSIM.

If failures are SSIM-related, increase:

```text
W_Normal
W_Silhouette
W_Dihedral
```

or lower:

```text
MaxCollapseLengthFrac
LockFeatureDihedralDeg
```

If failures are manifold/degenerate-related, tighten:

```text
MinTriangleArea
MaxNormalChangeDeg
```

and strengthen duplicate-face checks.

---

## 9. Expected weaknesses of the initial version

### 9.1 No exact local renderer

The actual judge uses 1024×1024 rasterization, z-buffering, flat normals, perspective-correct depth, and foreground-only SSIM. The initial implementation only approximates these effects. That is acceptable for a baseline but not enough for final performance.

Later improvement:

- implement a low-resolution six-view proxy renderer,
- evaluate local before/after raster patches,
- use actual normal/depth proxy SSIM or MSE-like penalties.

### 9.2 Hausdorff is only indirectly controlled

QEM and collapse-length guards reduce geometric drift but do not prove the official symmetric Hausdorff constraint.

Later improvement:

- sample original face centroids/edge midpoints,
- build spatial grid/BVH over current simplified triangles,
- approximate one-way distances periodically,
- reject collapses near local distance budget.

### 9.3 Feature locking may be too conservative

High dihedral and silhouette edges may be overprotected, reducing compression.

Later improvement:

- allow collapses along feature curves but not across them,
- use endpoint-preserving feature collapses,
- add pseudo-boundary QEM planes instead of hard locks.

### 9.4 No adaptive per-test-case strategy yet

The hidden cases have very different sizes. The best target ratio is likely size- and geometry-dependent.

Later improvement:

- classify mesh by vertex count, average edge length, dihedral distribution, area distribution, and projected silhouette density,
- select hyperparameters accordingly.

---

## 10. Final recommended baseline

The most defensible initial implementation is:

> **Conservative perception-aware QEM edge collapse with hard topology checks, endpoint/midpoint/QEM candidates, high-dihedral/silhouette protection, local normal-change rejection, and compact output.**

It has a clean theoretical lineage:

- QEM from Garland–Heckbert: geometric plane-distance error.
- Boundary QEM idea: repurposed as feature-edge / pseudo-boundary protection.
- Lindstrom–Turk terms: volume/shape as local distortion regularizers.
- Hoppe progressive-mesh energy: local distance/spring/discontinuity terms.
- Competition formulation: six-view flat-normal/depth metric, manifold validity, Hausdorff threshold, and vertex-count objective.

It is not the final maximal compression solution. It is the right stable base for iterative contest tuning.
