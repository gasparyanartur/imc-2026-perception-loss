# How to Proceed Beyond QEM Parameter Tuning

## Executive decision

The next serious solver should not be another keep-ratio, cost-cap, scan-budget,
or SSIM-floor sweep. The authoritative current best is
`imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp`,
SHA-256 `fbd49803b080033deb77aae8fd2df8b13daa8fb4b6e36c8a84dae7153a7b4333`,
with the user-reported official score `90.27`. Reaching `92.1` means reducing
the average keep rate from about `9.73%` to `7.9%`: roughly **18.8% fewer
vertices than the vertices that remain now**. Operators that save a few dozen
vertices, or a slightly different ordering of the same greedy collapse prefix,
cannot plausibly produce that jump.

The recommended architecture is:

1. build an original-referenced, aggregate, incrementally updated version of the
   stated six-view evaluator;
2. simplify deliberately past the current safe endpoint while recording a
   reversible progressive-mesh history;
3. selectively reinsert only the high-perceptual-value collapses until the real
   aggregate score is safely above the threshold;
4. improve that result with count-neutral edge flips and constrained vertex
   relocation;
5. spend the recovered score margin on generalized multi-vertex patch
   replacements and split-many/remove-more vertex-budget transactions;
6. replace the path-dependent collapse-radius guard with the actual checker
   constraint, once its vertex-set-versus-surface interpretation is resolved.

This is a rate-distortion optimizer over mesh operations, not a more elaborate
QEM weight. It searches final meshes that are not prefixes of one greedy
collapse sequence and can cross the local minima that have limited the current
solver family.

The handoff package independently reached a related conclusion in its
[vertex-redistribution design](../imc_sol_handoff/imc_mesh_repro_package/design/V34_VERTEX_REDISTRIBUTION_DESIGN.md),
but that design was never implemented. This roadmap uses
[`nebula_atomic_region_v33_t7.cpp`](../imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp)
as the frozen fallback. `v826_v3tail.cpp` remains a useful older, numerically
logged comparator, not the parent for the next solver.

## 1. What the evidence says has saturated

The repository now contains enough official experiments to distinguish a weak
implementation from an exhausted search space.

| Attempted family | Observed result | What it tells us |
|---|---|---|
| Lower keep ratios and narrow cost/ratio brackets | Small gains until a sharp hidden-test cliff | The current trajectory is close to its safe prefix boundary. |
| Extra placements and render-risk/normal-aware heap ordering | Broad hidden T2-T4 failures, sometimes also largest-tier failure | Reordering the same collapse family changes the trajectory but not the reachable family of final meshes. |
| Extra post-target edge collapses | T2/T4 produced a small passing gain; T3 consistently failed | A few safe residual collapses exist, but not enough for the required jump. |
| Edge flips used only to expose more ordinary collapses | About two local vertices on T2 and no meaningful T4 movement | A flip must be optimized as a perceptual margin builder, not only as a QEM-unlock heuristic. |
| One-ring, pair-disk, weld, and connected multi-disk deletion | Useful small T4 gains; aggressive multi-disk failed; strict multi-disk tied | Topology-changing deletion is real, but current geometry/local-render guards do not identify a large safe set. |
| Local Vega patch SSIM guards | Some small official gains, but aggressive view-weld and windowed-edge branches failed | The local guard is not the official objective and cannot safely price large transactions. |
| Whole-mesh six-view remeshing | Reported smooth-shape compression, no official improvement | Replacing all connectivity changes flat face-normal maps too much. Local, boundary-preserving remeshing is safer. |
| Handoff atomic-region replacement | Tiny smooth-shape gains and normal regression; claimed huge gains of only 5-6 vertices | Atomic replacement is a useful primitive, not a complete strategy. |
| Profile selection and micro-batch rollback | Safe variants fell back or tied; aggressive variants failed | Candidate selection cannot rescue a generator that stays in the same basin, especially with a checkpoint-relative proxy. |

The detailed official ledger is in [`updates.md`](../updates.md). The handoff's
chronology and failure analysis are in its
[`REPORT.md`](../imc_sol_handoff/imc_mesh_repro_package/REPORT.md), especially
sections 7, 13, and 15.

Two cautions matter when using the handoff data:

- Several handoff claims are qualitative and have no preserved official score
  or raw judge log.
- The prose says every multi-shape row is manifold, but the raw
  [`imc_multishape_results.csv`](../imc_sol_handoff/imc_mesh_repro_package/results/imc_multishape_results.csv)
  marks several sharp-shape outputs as `False,area`. Those rows are useful
  stress signals, not clean rate-distortion evidence.

## 2. The problem should be optimized as written

Let the original mesh be $O$ and a candidate be $M$. The real optimization
problem is

\[
\begin{aligned}
\min_M\quad & |V(M)| \\
\text{subject to}\quad
& S(O,M) \ge 0.9,\\
& d_H(O,M) \le H=0.05D_{\mathrm{AABB}},\\
& M \in \mathcal M,
\end{aligned}
\]

where \(\mathcal M\) is the checker-accepted class of closed triangular
manifolds. Quality above the threshold has no ranking value. A final mesh at
`0.905` is preferable to one at `0.99` if it uses fewer vertices.

### 2.1 The aggregate score contains much more usable slack than current guards

For view $k$, let $N_k$ be the mean of the three normal-channel SSIM values
and $D_k$ the depth SSIM. Then

\[
S(O,M)=\frac1{12}\sum_{k=1}^{6}(N_k+D_k).
\]

Equivalently, the allowed aggregate deficit is

\[
\sum_{k=1}^{6}\left[(1-N_k)+(1-D_k)\right] \le 1.2.
\]

There is no stated minimum per view, per normal channel, or per depth map. If
eleven of the twelve $N_k,D_k$ terms were perfect, the twelfth could be zero
and the final score would still be $11/12\approx0.9167$. If five complete view
scores were one, the sixth view score would only need to be $0.4$.

This does not mean deliberately destroying a view is always optimal. It means
that guards such as `minDepth >= 0.999`, `minView >= 0.995`, or comparison to a
safe simplified checkpoint solve a much stricter and different problem. The
handoff explicitly identifies this unused aggregate margin, while its own
transactions still use approximately `0.99-0.999` component minima.

### 2.2 Interior depth is cheap; silhouettes and flat normals are expensive

Visible depth is roughly $1.5$ to $3.5$, while the SSIM constants use
$L=255$, $C_1=6.5025$, and $C_2=58.5225$, and background depth is $255$.
For a constant foreground window whose depth changes from $x$ to
$x+\delta$, the luminance factor is

\[
\frac{2x(x+\delta)+C_1}{x^2+(x+\delta)^2+C_1}
\approx
1-\frac{\delta^2}{2x^2+C_1}.
\]

At $x=2.5$ and $\delta=0.05$, the loss is only about
\(1.3\times10^{-4}\). By contrast, moving a silhouette replaces values near
`2.5` with `255` and also changes the normal map between a face normal and
neutral gray. Therefore the correct priority is:

1. foreground/silhouette and occlusion ownership;
2. flat face normals on visible pixels;
3. interior depth, except near discontinuities.

QEM plane distance and smooth geometric beauty are only indirect proxies for
this ordering.

### 2.3 SSIM rewards spatially coherent error

A changed candidate pixel can affect every 11x11 window whose center is within
five pixels. If $P$ is the set of changed pixels, only centers in

\[
A=P\oplus[-5,5]^2
\]

can change score. Widely separated damaged pixels may contaminate nearly
$121|P|$ centers. A compact damaged region pays roughly the area of its
five-pixel dilation, which is much smaller because the affected windows overlap.

The optimizer should therefore compare coherent region transactions, not assume
that perceptual damage is additive over isolated operations. Concentrating the
allowed error in a small, low-ownership region can be better than distributing
tiny errors over the entire object.

### 2.4 Exact planar retriangulation is a render invariant

If a connected patch remains on the same plane, with the same boundary,
orientation, and visibility ordering, then every replacement triangle has the
same flat normal and the same perspective-correct plane depth. The normal and
depth maps are unchanged apart from raster tie precision. This is proven in
[`docs/report.md`](report.md#36-planar-patch-invariance-theorem).

This is the strongest safe special case in the problem. The previous exact-
coplanar pass removed too little because it only looked for a narrow one-ring
operation. A maximal planar-region polygonization can remove *all* interior
vertices of a large CAD patch in one transaction.

### 2.5 One vertex is worth more on small tests

Saving one vertex on case $i$ changes its compression score by
$100/|V_i|$. An absolute vertex saved on a 5k mesh is worth about 220 times an
absolute vertex saved on a 1.1M mesh. Large tiers still matter because they
contain many removable vertices, but small and medium cases should not be
ignored in favor of impressive million-vertex absolute counts.

## 3. Resolve checker semantics before designing the final geometric guard

The written statement, root evaluator, and handoff validator do not describe
exactly the same constraint. These are questions to settle with isolated,
minimal checker probes, not assumptions to bake into a large solver.

| Question | Evidence now | Why it changes the algorithm |
|---|---|---|
| Is Hausdorff vertex-set-to-vertex-set or continuous surface-to-surface? | The statement discusses original and simplified vertices, and root `evaluate.py` uses KD trees on vertices. The handoff assumes/samples surfaces. | Vertex-set coverage admits an exact dynamic coverage algorithm; surface Hausdorff needs triangle-to-surface certification. |
| Must output remain connected? | Input is guaranteed connected; the output rules only explicitly say manifold. Local validators report/check this differently. | If not required, disconnected reconstruction or component surgery becomes legal. |
| Must every output vertex be incident to a face? | The written validity list does not explicitly reject unused vertices, and the root preliminary validator does not check them. | Under vertex-set Hausdorff, non-rendered coverage anchors could decouple the geometric gate from the visible triangulation. |
| Must genus/topology match the input? | Not stated. Current link-condition collapses preserve topology by construction. | Small handles, tunnels, and cavities may be removable wholesale. |
| Are vertex links checked, or only two faces per edge? | The statement defines manifold via edge incidence; validators disagree on stronger link checks. | Edge-incidence alone can admit pinched vertices that a true 2-manifold check rejects. |
| Are self-intersections forbidden? | Not stated and not checked locally. | Boundary-preserving patch search can be less conservative if intersections are allowed, but render behavior still matters. |
| Is there an output face-count cap other than 100 MiB? | Vertex count is bounded and scored; output face count is not explicitly bounded by input count. | Higher-genus or face-rich constructions are only relevant if the checker permits them. |
| Are backfaces culled and how are equal-depth ties resolved? | The formulation describes nearest-triangle z-buffering but local implementations differ in tolerances and precision. | Raster-exact planar and subpixel moves require the same tie rules as the judge. |

The production solver should continue satisfying the strongest ordinary
interpretation until a probe proves otherwise. Checker probing belongs in a
separate branch and must never use crashes, malformed indices, NaNs, or undefined
behavior.

## 4. Why the current guards cannot support a large jump

The current
[`nebula_atomic_region_v33_t7.cpp`](../imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp)
is substantially richer than ordinary QEM: it has raster ownership and
silhouette weights for medium tiers, transactional continuation, an occluded-
edge pass, local/checkpoint render screening, atomic region replacement, and a
dedicated million-vertex tail. It is nevertheless still limited by surrogate
acceptance:

- Main collapses choose only QEM optimum, midpoint, and endpoints, then minimize
  quadric error. The hard geometry test is the accumulated radius recurrence

  \[
  r'=\max(r_a+\|p_a-p\|,\ r_b+\|p_b-p\|)\le H.
  \]

  This is path-dependent and can reject a final position that is actually
  covered by other output geometry.
- The base link test uses two common faces and two common neighbors, which is
  appropriate for a closed-manifold edge collapse, but the main placement path
  does not explicitly reject every new near-zero-area or reversed incident face
  before mutation.
- Medium-tier raster importance is a much better signal than static projected
  area, but it is recomputed only at stage boundaries and becomes a QEM weight;
  it is not the marginal change in the official score.
- `isFaceInvisible()` is unusable as written: for every nonzero normal, one of
  the paired directions `+X/-X`, `+Y/-Y`, or `+Z/-Z` has positive dot product.
  The newer hidden atlas does use z-buffer ownership, but at reduced resolution
  and with broad visible-one-ring protection; it still cannot replace an entire
  occluded pocket atomically.
- The retained Vega helper, also used throughout older solver branches, compares
  an old local patch with a proposed local patch, not the
  candidate with the original full rendering. Its scalar SSIM uses one set of
  means and variances over the entire cropped patch rather than the official
  sliding 11x11 foreground-center average. It also omits surrounding faces that
  can occlude or be revealed by the edit. It should not be reused as the new
  acceptance oracle.
- The newer transactional renderer does implement sliding-window SSIM, but its
  reference is normally the already simplified safe checkpoint. Its acceptance
  thresholds require per-view, per-normal, and per-depth similarity around
  `0.989-0.999`, which is not the official aggregate constraint.
- The huge atomic transaction evaluates only an extremely conservative
  post-compaction macro replacement. Preserved local results remove roughly
  five or six vertices, useful as proof of a primitive but immaterial to a
  1.83-point score gap.
- The macro geometry thresholds are nested far inside the written bound. The
  code sets `hausd = 0.055 * diagonal`, rejects a general macro above
  `0.055 * hausd` (about `0.0030 * diagonal`), and filters the huge candidate
  again at `0.024 * hausd` (about `0.0013 * diagonal`). These sampled local
  thresholds are tens of times smaller than the written `0.05 * diagonal`
  limit, while still not being a continuous two-sided certificate.
- Several branches are controlled by elapsed time, and heap ties compare only
  floating cost. The official history shows that even cold or inactive code can
  perturb the largest-tier trajectory through timing and layout.

The handoff branches have the same central problem. v29, v31, v33, and the
transactional continuation frequently compare a candidate with an already
simplified checkpoint under near-one per-channel thresholds. SSIM has no
triangle inequality: being nearly identical to a baseline that has already
spent its margin does not imply being safe relative to the original.

The solution is not a looser local threshold. It is a different acceptance
engine.

## 5. Foundation: an original-referenced incremental transaction oracle

This is the highest-leverage implementation task. It does not directly remove a
vertex, but every high-upside operator below depends on it.

### 5.1 Persistent render state

For each of the six views, store:

- immutable original normal/depth maps and foreground mask;
- current candidate normal/depth maps, z-buffer, and winning face ID;
- screen tiles, initially 16x16 or 32x32 pixels;
- for each tile, the active faces whose projected bounding boxes overlap it;
- the current sum of foreground-window SSIM values and active-window count for
  each normal channel and depth.

Do not try to delete a face from a z-buffer in place. For a transaction, collect
the union of the old and new projected face bounds, update the tile-face bins,
and rerasterize every dirty tile from all faces in that tile. This correctly
reveals a previously occluded second layer.

For medium tiers, two full six-view 1024 maps are feasible under the documented
2 GiB memory limit. Per-window SSIM values do not need to be stored: recompute
the old and new contributions only over affected centers.

### 5.2 Exact local SSIM delta

For a channel and an 11x11 window $W_p$, maintain or calculate

\[
A=\sum X,\quad B=\sum Y,\quad
A_2=\sum X^2,\quad B_2=\sum Y^2,\quad C=\sum XY.
\]

With $m=121$,

\[
\mu_X=A/m,\quad \sigma_X^2=A_2/m-\mu_X^2,
\]

and analogously for $Y$, while

\[
\sigma_{XY}=C/m-\mu_X\mu_Y.
\]

Substitution into the stated SSIM formula gives the exact value for that
window. If a transaction changes pixels $P$, recompute only centers in
$P\oplus[-5,5]^2$. Update both the SSIM sum and the denominator because the
foreground-center predicate can enter or leave when the silhouette changes.

The exact aggregate delta is

\[
\Delta S=\frac1{12}\sum_{k=1}^{6}
\left[\Delta D_k+\frac{\Delta N_{kx}+\Delta N_{ky}+\Delta N_{kz}}3\right].
\]

This quantity, not a minimum component score, is the perceptual price of a
transaction.

### 5.3 Residual attribution that respects SSIM windows

A raw per-pixel normal difference is not enough because SSIM is windowed and
nonlinear. Define window deficit $r_{kcp}=1-s_{kcp}$. Attribute it to a
candidate face $f$ using its ownership inside the window:

\[
R_f=\sum_{k,c,p} w_c\,r_{kcp}
\frac{|\{q\in W_p:\operatorname{owner}_k(q)=f\}|}{121},
\]

where $w_D=1/12$ and each normal channel has its corresponding aggregate
weight. Also maintain a boundary ownership term for windows that mix foreground
and background. High $R_f$ regions need vertices; faces with zero ownership or
low marginal window contribution are payment candidates.

### 5.4 A fidelity ladder, not a low-resolution fiction

Use three stages:

1. geometry/topology/coverage prefilters;
2. a scaled low-resolution shortlist renderer;
3. native 1024 original-referenced acceptance before commit.

At lower resolution, scale focal length by $R/1024$ to preserve field of view
and scale the window toward $11R/1024$, rounded to an odd size. The root Python
evaluator keeps focal length 800 at lower resolution and therefore crops; the
handoff C++ renderer scales focal length but keeps an 11-pixel window. Neither
low-resolution score is the native evaluator. The final check must be 1024.

### 5.5 Deterministic transactions

Every queue key needs stable tie breakers after a quantized primary cost:

1. operation type;
2. minimum touched vertex ID;
3. maximum touched vertex ID;
4. absorbed/kept IDs or patch seed;
5. placement index.

Use operation-count budgets for the experimental medium path. Keep wall time as
an emergency output fuse, not as the normal decision rule. Put experimental
functions on an early, tier-gated cold path and verify that non-target-tier
outputs are byte-identical under irrelevant code-padding builds.

## 6. Capture the actual Hausdorff constraint

The correct implementation depends on the checker probe in section 3.

### 6.1 If the checker uses vertex sets

Let $X$ be original vertices, $Y$ current output vertices, and $H=0.05D$.
The constraint is exactly

\[
\forall x\in X:\quad c_x=|\{y\in Y:\|x-y\|\le H\}|\ge1,
\]

and

\[
\forall y\in Y:\quad \min_{x\in X}\|y-x\|\le H.
\]

Maintain original vertices in an $H$-sized spatial grid or KD tree and an
integer coverage count $c_x$. Removing sites $u,v$ and adding $p$ only
changes original vertices in
$B_H(u)\cup B_H(v)\cup B_H(p)$. Apply those count deltas transactionally and
reject exactly when a count reaches zero; require the new site to have an
original neighbor within $H$.

This is strictly more faithful than collapse ancestry. An original point may be
covered by any surviving output vertex, not only by the survivor of its greedy
collapse cluster. A final set-cover thinning pass can remove any output site
whose covered originals all have count at least two.

### 6.2 If the checker uses continuous surfaces

Use the cluster radius only as a cheap prefilter, then certify each changed
patch with a BVH over original triangles and adaptive subdivision.

Distance to a closed set is 1-Lipschitz. If samples $S_T$ cover triangle cell
$T$ with covering radius $h_T$, then

\[
\sup_{x\in T}d(x,O)
\le \max_{s\in S_T}d(s,O)+h_T.
\]

Therefore:

1. query vertices, edge midpoints, and centroid of a changed triangle against
   the original-triangle BVH;
2. accept that cell if the upper bound is below $H-m_H$;
3. reject it if a lower bound is above $H$;
4. otherwise split the triangle into four children and recurse.

Run both new-patch-to-original and original-patch-to-new-patch directions. The
outside mesh is unchanged, so the second direction only needs original
triangles whose former nearest witness could have been in the removed patch;
find them with a BVH traversal against the old patch expanded by $H$.

This produces a conservative certificate rather than a fixed seven-point or
random sample that can miss a narrow maximum.

### 6.3 A cheap improvement even before checker resolution

The current recurrence anchors each cluster radius at its current survivor and
adds every historical motion. A tighter conservative summary is a movable
enclosing ball. Merge balls $B(c_1,r_1)$ and $B(c_2,r_2)$. If neither
contains the other and $d=\|c_2-c_1\|$, use

\[
r=\frac{d+r_1+r_2}{2},\qquad
c=c_1+\frac{r-r_1}{d}(c_2-c_1).
\]

A placement $p$ is conservatively covered when

\[
\|p-c\|+r\le H.
\]

This is still not the true global constraint, but it removes some avoidable
path-length accumulation and is safe as a prefilter.

## 7. Primary algorithm: oversimplify, then selectively repair

The current solver is monotone: once a region loses too many vertices, no later
operator can put detail back. Its final mesh is therefore one prefix of one
greedy history. A lower target fails as soon as the cumulative damage of that
prefix crosses the threshold, even if most late collapses were harmless and only
a small subset was destructive.

The first new solver should construct a *non-prefix* solution.

### 7.1 Reversible collapse history

Start from the exact current-best result $M_0$ on T2, retained as a fallback.
Rebuild connectivity on its compact mesh and run a deliberately aggressive
continuation while preserving the hard topology and preliminary geometry
checks. For every accepted collapse, record a standard progressive-mesh inverse
operation:

- absorbed vertex ID and position;
- kept vertex's position before and after collapse;
- the two faces deleted by the collapse;
- every incident face whose vertex ID changed;
- the two opposite/link vertices needed by the inverse vertex split;
- parent/child dependency IDs;
- touched face and vertex generations;
- the operation's projected tile/window support in all six views.

An edge collapse becomes a `vertex_split` that restores exactly the old local
state. Standard dependency rules define when a split is exposed and legal. If a
desired split has inactive ancestors, its cost is the number of records in the
smallest dependency closure that must also be restored.

For the first T2 prototype, retaining full local undo records is acceptable:
only the aggressive tail beyond $M_0$ needs to be recorded. A compact standard
progressive-mesh representation is necessary only after the mechanism works on
larger tiers.

### 7.2 Residual-guided selective refinement

Render the over-simplified state $M_c$ against the original. For each exposed
split record $r$, estimate the recovered score from the residual windows in
its support and define

\[
\rho(r)=\frac{\widehat{\Delta S}(r)}{|\operatorname{closure}(r)|}.
\]

Use the incremental oracle to measure the exact delta for the top candidates.
Apply nonconflicting splits with the largest exact recovered score per restored
vertex. Recompute visibility and priorities after each small batch because a
split can reveal or occlude other faces.

Stop at the first state satisfying

\[
S(O,M)\ge S_{\mathrm{safe}},
\]

where `S_safe` should initially be `0.91-0.92` until the local renderer is
calibrated against judge outcomes. Then run a reverse pruning pass: attempt to
re-collapse the least useful exposed leaves and keep every exact-safe removal.

This is a precedence-constrained rate-distortion problem. A useful search form
is

\[
\min_{A\subseteq R}|V(M_c)|+|\operatorname{closure}(A)|
\quad\text{s.t.}\quad
S\!\left(O,\operatorname{refine}(M_c,A)\right)\ge S_{\mathrm{safe}}.
\]

A beam of 4-8 refinement states is enough for the first implementation. The
branches should emphasize normal repair, silhouette repair, mixed repair, and
the best score-per-vertex repair.

### 7.3 Why this can produce a large improvement

Suppose an additional 2,000 collapses take T2 below the threshold, but only 300
of them account for most normal/silhouette loss. A monotone checkpoint returns
to the state before all 2,000. Selective refinement can restore the dependency
closures of the harmful 300 and keep most of the other 1,700 deletions. The
actual numbers will differ, but this is the scale of mechanism required by the
remaining `90.27 -> 92.1` gap.

This also explains why the previous guided micro-batch failure does not refute
the idea. That branch used a reduced-resolution, checkpoint-relative scorer,
complex partial state rollback, and time-sensitive execution. Here every
accepted final state is a legal progressive-mesh state, is scored directly
against the original, and has an exact undo path to $M_0$.

### 7.4 Lower-risk fallback: patchwise progressive allocation

If arbitrary progressive dependencies are too complex, partition the current
surface into connected patches with frozen boundaries. Build a short local
collapse/refinement curve for each patch:

\[
S_j(k)=\text{global score after patch }j\text{ uses }k\text{ interior vertices}.
\]

Allocate vertices across patches by repeatedly choosing the largest marginal
score gain per restored vertex. Fixed shared boundaries make patches independent
and preserve manifoldness. Boundary overhead makes this less powerful than the
global hierarchy, but it still reallocates the budget according to the actual
renderer instead of QEM cost.

## 8. Recover score at fixed vertex count before deleting more

The current mesh may not be the highest-scoring mesh representable with its
vertex count. Use zero-count operations to manufacture perceptual margin, then
convert that margin into compression.

### 8.1 Evaluator-aware 2-2 edge flips

For two triangles $(a,b,c)$ and $(b,a,d)$ sharing diagonal $ab$, consider
the replacement $(c,d,b)$, $(d,c,a)$. A flip is eligible only if:

- $c\ne d$, edge $cd$ does not already exist, and the union is an embedded
  topological disk under the production interpretation;
- both new faces have signed area above
  \(\epsilon_A D^2\) and orientation consistent with the old patch;
- the local Hausdorff transaction passes;
- its exact original-relative aggregate score is nondecreasing, or it enables a
  complete paired transaction with a better final score/count.

Do not score flips only by how many existing QEM candidates they unlock. Their
primary value is rotating flat facet normals and improving the current render.
After a profitable flip, refresh exact deltas for nearby collapse and patch
candidates.

### 8.2 Constrained vertex relocation

For a face with vertices $a,b,c$, define

\[
m=(b-a)\times(c-a),\qquad n=\frac{m}{\|m\|}.
\]

A small perturbation gives

\[
\delta n=\frac{I-nn^T}{\|m\|}\,\delta m,
\]

where

\[
\delta m=(\delta b-\delta a)\times(c-a)
 +(b-a)\times(\delta c-\delta a).
\]

For visible pixels owned by the incident faces, minimize a local surrogate

\[
E(x)=\sum_{k,p}w_{kp}
\left[
\alpha\|n_f(x)-n^*_{kp}\|^2+
\beta(z_f(x,p)-z^*_{kp})^2
\right]
+\gamma\|x-\Pi_O(x)\|^2,
\]

where \(\Pi_O\) is projection onto the original surface. Use one or two damped
Gauss-Newton steps, or an eight-direction tangent-plane coordinate search when
derivative plumbing is too costly. Impose:

- a trust region tied to local edge length and remaining Hausdorff margin;
- an orientation/area log barrier for every incident face;
- no new duplicate edge or face;
- exact tile-delta acceptance against the original.

The previous root nudge tested one fixed direction and small displacement. That
does not exhaust a residual-driven fixed-budget position optimizer.

### 8.3 Vertex teleport: the cleanest redistribution primitive

A particularly useful count-neutral transaction is:

1. delete one low-value donor vertex with the best boundary retriangulation;
2. split one high-residual receiver face or edge and insert an optimized vertex;
3. validate and render the two edits atomically.

The net vertex change is zero. Let $D_a$ be donor deletion and $S_b$ receiver
split. Choose

\[
(a,b)^*=\arg\max_{a,b}
\left[S(O,M\oplus D_a\oplus S_b)-S(O,M)\right].
\]

Shortlist the top 16-32 donors by low marginal damage and the top 8-16 receivers
by residual recovery, then test a few hundred pairs with cheap filters and a
small exact-render beam. Repeating profitable teleports raises score without
raising count. The recovered margin can then pay for net-negative transactions.

This is more controlled than blindly inserting a midpoint: the new point is
placed specifically to fit the original normal/depth samples, and the donor and
receiver are judged as one final mesh.

## 9. Spend score margin as a conflict-aware perceptual knapsack

Unify the existing operation types behind a transaction interface:

- edge collapse;
- one-ring/valence removal;
- pair-disk removal;
- generalized atomic patch replacement;
- hidden-pocket replacement;
- repair split, flip, and relocation.

For operation $i$, store vertex gain $g_i$, exact or predicted score damage
$d_i$, affected-window sets $W_{ik}$, hard-constraint status, and a conflict
set. If current slack is $B=S(O,M)-S_{\mathrm{safe}}$, the first approximation
is

\[
\begin{aligned}
\max_x\quad &\sum_i g_i x_i\\
\text{s.t.}\quad &\sum_i d_i x_i\le B,\\
&x_i+x_j\le1\quad\text{for conflicting }i,j.
\end{aligned}
\]

This is only a proposal generator because SSIM and visibility interactions are
not additive. Prefer operations whose affected-window dilations overlap, since
they create less new contaminated screen area. Apply the proposed subset to a
copy, rerender all dirty tiles, and accept only the exact complete transaction.

A practical greedy key is

\[
\operatorname{key}(i)=
\frac{g_i}
{\lambda_d\max(d_i,0)+
 \lambda_w|W_i\setminus W_{\mathrm{already\ affected}}|+\epsilon}.
\]

Keep a small beam so a positive-score flip/teleport can be paired with a
larger damaging deletion. The final comparison remains lexicographic:

1. all hard constraints pass;
2. aggregate score is at least the safety floor;
3. fewer vertices wins;
4. at equal count, larger margin wins.

## 10. Generalized atomic patch replacement

The current macro operator fits one center and fans every boundary edge to it.
The previous one-ring operators are also fan dominated. A substantially larger
search should replace a disk containing $k$ interior vertices by the best
state with $r=0,1,2,\ldots$ replacement interior vertices.

For a triangulated disk with $b$ boundary vertices and $r$ interior
vertices, Euler's formula and edge incidence give

\[
F=b+2r-2,\qquad E=2b+3r-3.
\]

Replacing a current disk with $k$ interior vertices by one with $r$ gives
vertex gain $k-r$.

### 10.1 Patch extraction and legality

Grow face-dual regions from low-ownership or high-redundancy seeds. A candidate
must have:

- one consistently oriented simple boundary cycle for the disk prototype;
- Euler characteristic $V-E+F=1$;
- no interior vertex incident to an outside face;
- boundary edges that each lose one old patch face and gain one oppositely
  oriented replacement face;
- every new internal edge incident to exactly two opposite-oriented faces;
- a single circular link at every touched vertex;
- no duplicate undirected face and no positive-area/orientation failure.

These conditions reuse the best parts of the current macro validator while
allowing a richer replacement family. The relevant reusable code is the current
[`macroClosedOriented()` and boundary-cycle machinery](../imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp#L568).

### 10.2 Boundary-only dynamic programming

For $r=0$, project a near-planar boundary into a stable local frame and find
k-best polygon triangulations. An additive shortlist recurrence is

\[
D[i,j]=\min_{i<t<j}
\left(D[i,t]+D[t,j]+C(i,t,j)\right),
\]

where $C$ includes:

- signed area and minimum angle barriers;
- deviation of the triangle normal from residual-weighted target normals;
- adaptive patch-Hausdorff upper bound;
- predicted affected-window deficit;
- duplicate-chord and visibility-order rejection.

SSIM is not additive, so dynamic programming only creates a shortlist. Exact
tile rendering chooses among the best complete triangulations.

### 10.3 Exact planar regions

Region-grow maximal connected faces with the same oriented plane, not merely a
single removable one-ring. Extract all boundary loops, identify vertices used
only inside the patch, and polygonize the entire patch. For a simple disk, a
boundary-only triangulation removes every interior vertex with render-exact
geometry. Patches with holes can be handled later with a constrained
triangulation or split into disks.

Always verify native-resolution equality because equal-depth edge tie rules can
create a one-pixel difference even when the continuous theorem applies.

### 10.4 Curved patches with a few Steiner vertices

For $r=1$ to 4:

1. initialize interior points from QEM, residual centroids, or a fitted local
   quadratic patch;
2. project them to the original surface;
3. enumerate a small number of boundary partitions/connectivities;
4. optimize positions with the normal/depth surrogate from section 8;
5. certify the complete old/new patch geometrically;
6. exact-render the top complete states.

The search may use temporary splits, collapses, flips, and relocations inside a
copy of the patch. Only the final atomic state must pass. This is precisely how
to cross a sequence of individually bad edge collapses without exposing invalid
intermediate output.

## 11. Atomic hidden-pocket and topology-changing feature removal

### 11.1 Hidden-pocket replacement

At native resolution, mark faces that win zero pixels in all six views and
record the minimum positive occlusion slack behind the winning depth. Grow a
connected hidden region and replace it as a whole if:

- its boundary is a valid disk cycle;
- every new projected sample remains behind the unchanged front surface by a
  conservative depth margin in all six views;
- the local Hausdorff certificate passes;
- the full exact score and manifold validator pass.

This can close a cavity or fold that cannot be removed by individually safe
collapses. It is much more powerful than protecting all visible vertices plus
their one-ring and then trying ordinary hidden edges.

### 11.2 Same topology is not a written constraint

If checker probes confirm that genus need not be preserved, add a separate
operator for geometrically small handles and tunnels. For a connected oriented
closed mesh,

\[
\chi=V-E+F=2-2g.
\]

Use a tree-cotree decomposition or short-cycle search to find small
nonseparating cycles. A narrow handle can be removed by cutting a short annular
bridge and capping the two resulting boundary cycles. A short protrusion can be
cut at its neck and capped once. Accept only if all removed original geometry is
within the actual Hausdorff bound and the six-view score remains safe.

This direction can make a discontinuous vertex-count gain that topology-
preserving link-condition collapse can never reach. It is shape dependent and
belongs after the original-referenced oracle, not before it.

## 12. A second solver line: normal-proxy clustering and anisotropic remeshing

This should be pursued only after the exact acceptance engine exists. It is a
genuinely different geometric model, not another scalar multiplier on QEM.

### 12.1 Visible normal-proxy clustering

Partition connected original/current faces into regions $R_j$ represented by
one or a few planar normal proxies. Use the energy

\[
E_N(R_j,n_j)=
\int_{x\in R_j} w_6(x)\|n(x)-n_j\|^2\,dA,
\]

where $w_6$ is measured six-view visible-window ownership, not merely surface
area. The best proxy normal is the normalized weighted mean normal. Add large
penalties at foreground boundaries, depth discontinuities, and feature curves.

Iterate:

1. assign each face to the adjacent proxy giving the smallest incremental
   visible normal/depth energy;
2. refit proxy planes and normals;
3. trace region boundaries on the original mesh;
4. polygonize and triangulate each region with as few vertices as the exact
   render permits;
5. optimize shared boundaries with the transaction oracle.

This aligns directly with the judge's flat normal map. It is safer than the
failed whole-mesh visual-hull remesher because it retains original surface
regions and boundaries and accepts every local reconstruction against the
original render.

### 12.2 Anisotropic metric inside curved regions

Estimate the shape operator $S$ in a vertex tangent frame from

\[
n_j-n_i\approx-S(u_j-u_i)
\]

by least squares over original neighbors. For a tangent edge $e$, first-order
normal variation is

\[
\|\delta n\|^2\approx e^T S^TS e.
\]

Define a visible residual metric

\[
G=\alpha I+\beta_N S^TS+\beta_D G_D+\beta_S G_{\mathrm{sil}},
\]

where $G_D$ captures visible depth gradients and $G_{\mathrm{sil}}$ heavily
contracts allowed edge length across a silhouette. Within selected patches, use
split, collapse, flip, and relocation operations to make triangles roughly
equilateral under $G$, while allowing long triangles along low-curvature
directions.

The metric only proposes topology and placement. Native incremental SSIM and
the Hausdorff oracle still decide acceptance.

## 13. Simulator-aware strategies and checker probes

### 13.1 Safe strategies implied directly by the statement

These do not depend on undocumented behavior:

- **Use the aggregate, not invented minima.** Allocate the 1.2 total
  normal/depth-view deficit where it buys the most vertex reduction.
- **Preserve coverage before interior depth.** Foreground/background mistakes
  are much more expensive than small depth shifts within the object.
- **Optimize the finite pixel samples.** Geometry between camera rays is judged
  only through Hausdorff; a move preserving winning face, normal, and reciprocal
  depth at every pixel center is render-exact.
- **Exploit six-view ownership.** There are no oblique cameras. Fully hidden
  axial-view pockets can be simplified under topology/Hausdorff alone if the
  replacement stays hidden.
- **Cluster damage.** Prefer edits in already affected windows and compact
  regions rather than creating new isolated residual islands.
- **Use all of the safe score margin.** Once calibrated, a candidate at
  `0.91-0.92` should not be rejected merely because one component is below
  `0.99`.

### 13.2 Probe-only possibilities

Run these as isolated transformations with the current-best fallback on every
other tier. Do not combine probes with a score candidate.

| Probe | Minimal question | Algorithm unlocked if accepted |
|---|---|---|
| Vertex vs surface Hausdorff | Does a controlled patch pass vertex coverage but fail continuous surface distance, or vice versa? | Exact dynamic set cover versus adaptive triangle certificate. |
| Connectedness | Does a valid union of closed oriented components pass? | Component-wise reconstruction and cavity separation. |
| Vertex-link manifoldness | Does edge incidence pass when a vertex link has two cycles? | Whether global patch validator must enforce links. Production should still enforce them by default. |
| Same topology/genus | Does a tiny, render-neutral handle removal pass? | Small-handle and tunnel surgery. |
| Self-intersection | Does a tiny interior intersection with unchanged render pass? | Less conservative patch connectivity, though render and Hausdorff still constrain it. |
| Backface/tie behavior | Reverse or retriangulate a controlled planar sample patch. | Raster-exact planar surgery and correct z-tie rules. |
| Face-count freedom | Output more faces than input while respecting size/manifold rules. | Face-rich constructions, if they have real visible benefit. |
| Unused vertices | Add a small number of original-position vertices referenced by no face. | If accepted with vertex-set Hausdorff, separate geometric coverage anchors from render vertices. |
| Reversed duplicate face | Test a tiny two-sided triangle component under the stated edge-incidence rule. | Determines whether the checker enforces a simplicial face set or only edge counts/orientation. |

For a connected, closed, oriented triangular manifold,

\[
3F=2E,\qquad V-E+F=2-2g,
\]

so

\[
F=2V-4+4g.
\]

At fixed genus, unpenalized face count is not independently free: reducing
vertices also reduces faces. Increasing genus can add faces at fixed vertex
count, but exploiting that would require confirmed topology freedom and a
useful, geometrically valid construction. It is a research probe, not the main
road to 92.1.

If both unused vertices and vertex-set Hausdorff are confirmed, formulate the
output as two coupled budgets:

\[
Y=Y_{\mathrm{render}}\cup Y_{\mathrm{anchor}}.
\]

$Y_{\mathrm{render}}$ forms the scored manifold, while
$Y_{\mathrm{anchor}}\subseteq V(O)$ is a greedy $H$-cover of original vertices
and does not alter the z-buffer. Minimize the total
$|Y_{\mathrm{render}}|+|Y_{\mathrm{anchor}}|$. This could let the visual mesh
move or retriangulate more freely than the current collapse ancestry permits.
It must remain probe-only until the official checker demonstrates that unused
vertices are valid; under continuous surface Hausdorff it provides no shortcut.

## 14. Concrete implementation program

### Phase 0: freeze truth and instrument the current best

1. Copy and hash
   [`nebula_atomic_region_v33_t7.cpp`](../imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp).
2. Record the user-reported official reference: score `90.27`, current cases
   string/submission ID if available, compiler flags, and source hash.
3. Treat `v826_v3tail.cpp` only as an older comparator.
4. Add no behavioral changes to the frozen source.
5. Regenerate valid sharp/thin benchmark meshes; do not use the invalid sharp
   rows in the preserved CSV as promotion evidence.

### Phase 1: standalone judge-model diagnostic

Refactor the current renderer into reusable structures before embedding it:

```text
JudgeReference
  original maps[6]
  original foreground[6]

CandidateRender
  normal/depth/z/face-id maps[6]
  tile -> faces bins
  per-component SSIM sums and active counts

RenderTxn
  changed faces/vertices
  old and new tile membership
  saved dirty pixels and score totals
  commit() / rollback()
```

Required tests:

- original versus itself is exactly or numerically indistinguishably 1;
- embedded full rerender equals the external implementation at 1024 on saved
  meshes;
- tile rerender after deleting a front face matches a clean full rerender,
  including the newly revealed face;
- incremental SSIM delta matches full recomputation for random legal flips,
  collapses, and vertex moves;
- planar retriangulation is render-identical except documented tie pixels;
- foreground denominator changes correctly when silhouette pixels enter/leave.

Do not proceed until maximum component disagreement is understood. A fast wrong
oracle will repeat the v29/v31 failure mode more efficiently.

### Phase 2: embed a T2-only absolute scorer without changing output

The [current dispatch](../imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp#L207)
enters [`runTransactionalScreenT2Atomic()`](../imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp#L492)
for inputs up to 25k. Before that path mutates the mesh:

1. retain the original vertex/face arrays or render their immutable reference
   maps;
2. run the current T2 solver unchanged;
3. measure its absolute original-relative 1024 score and component deficits;
4. output the exact current result.

This diagnostic reveals whether the current local clone believes there is large
unused aggregate margin, which component consumes it, and how much runtime the
native reference costs. It must not silently replace the current checkpoint
guard yet.

### Phase 3: progressive oversimplification and selective repair

1. Save the compact current T2 result as $M_0$.
2. Rebuild connectivity and generate an aggressive reversible continuation.
3. Stop after an operation-count budget or a target 15-25% below the current
   remaining count, not after an elapsed-time decision.
4. Score the coarse result against the original.
5. Apply exact-benefit vertex splits until the safety margin is recovered.
6. Re-collapse low-benefit exposed leaves.
7. Run the strongest topology and chosen Hausdorff validator.
8. Output the candidate only if it has fewer vertices than $M_0$; otherwise
   return $M_0$ byte-for-byte.

Proof-of-mechanism gate: at least 5% fewer *remaining output vertices* on four
different valid shape categories with no full-resolution score below the local
safety floor. Judge-promotion gate: aim for at least 10% relative remaining-
vertex reduction on the target tier. The final 92.1 objective requires about
18.8% average reduction in the vertices that currently remain, so sub-percent
activation is not enough.

### Phase 4: fixed-budget repair and margin spending

Add, in this order:

1. exact-positive edge flips;
2. constrained visible-residual vertex relocation;
3. donor-to-receiver vertex teleport;
4. conflict-aware selection of current collapse/star/pair/macro operations.

Measure both quantities separately:

- score margin manufactured at fixed vertex count;
- extra vertices removed after spending that margin.

A fixed-count optimizer that raises SSIM but unlocks no deletions is useful
infrastructure, not yet a scoring candidate.

### Phase 5: generalized patches and hidden pockets

1. Replace the one-center fan with $r=0,1,2$ patch states.
2. Implement maximal exact-planar disk polygonization.
3. Add adaptive local Hausdorff certification.
4. Add native-resolution hidden-pocket region replacement.
5. Combine receiver splits with remove-many macro payments in a beam.

Only after these work across diverse forms should the same mechanism be exposed
to another official tier.

### Phase 6: tier rollout

Recommended order:

1. T2, because exact 1024 evaluation is affordable and each saved vertex has
   high score leverage;
2. T4, where root motion and topology cleanup have already produced positive
   official signal;
3. T3, last among medium tiers because it has repeatedly shown a sharp hidden
   cliff;
4. T5-T7 only after the method is proven. The current T7 path is already the
   authoritative best and its atomic pass saves too few vertices to justify
   destabilizing it early.

Every non-target tier must be byte-compared with the frozen current source.
Compile the experimental code as cold/noinline where possible, use deterministic
keys, and test builds with irrelevant padding to expose residual timing/layout
sensitivity.

## 15. Experiment design and promotion criteria

### Required shape categories

Use valid meshes from all of these categories:

- smooth convex: sphere, ellipsoid, capsule;
- concave genus zero: peanut, dimple;
- genus one: torus and thin torus;
- sharp mechanical: cube, cylinder, cone;
- thin structures: thin box, fins, narrow walls;
- high frequency: bumpy/wavy surface;
- near-axis-aligned rounded forms;
- large symmetric determinism stress.

The proxy dataset is a filter, not truth. Official results override it as the
repository rules require.

### Metrics to log per candidate

- input/output vertices and relative reduction versus the current best output;
- six $N_k,D_k$ terms and final aggregate;
- total active foreground windows per component;
- score deficit attributed to silhouette, normal interior, depth interior, and
  occlusion changes;
- exact dirty tiles and affected-window count;
- topology rejection reason counts;
- Hausdorff prefilter, lower bound, upper bound, and witness location;
- candidates generated, exact-rendered, committed, and rolled back by operator;
- progressive splits restored and dependency-closure sizes;
- wall time, operation counts, memory high-water mark, and output size;
- source hash and whether every non-target output is byte-identical.

### Submission rule

Submit one isolated algorithmic hypothesis at a time. A candidate is worth an
official submission when it:

- returns the frozen current output on fallback;
- passes topology and the strongest available Hausdorff interpretation;
- survives native 1024 scoring on all valid local stress forms;
- reduces remaining vertices materially, not merely by a few dozen;
- leaves all non-target tier outputs identical or has an explicitly measured,
  justified difference;
- fits the 21-second and 2 GiB limits with final validation reserve.

## 16. Diagnostic decision tree

Use the following outcomes to choose the next mechanism rather than tuning
everything at once.

| Observation | Interpretation | Next action |
|---|---|---|
| Current result has large absolute aggregate margin, but extra operations are rejected geometrically | The collapse-radius/sampled patch model is the bottleneck. | Resolve checker semantics and deploy exact coverage/adaptive surface certification. |
| Aggressive coarse result removes many vertices and selective repair restores only a small fraction | Progressive allocation works. | Improve the split-benefit beam and extend to T4. |
| Selective repair restores most deleted vertices | The generator damages normals/silhouette broadly, not in a sparse set. | Run fixed-count teleport/relocation or normal-proxy remeshing before more deletion. |
| Flips/relocation raise score but do not unlock removals | Margin is real but candidate family is topology-limited. | Generalize atomic disks and hidden pockets. |
| Exact-planar regions activate on only one synthetic form | Planar polygonization is a safe special case, not the main line. | Keep it, but focus on curved $r>0$ patches. |
| Hidden regions are rare or too shallow to save many vertices | Occlusion surgery is shape-specific. | Do not turn it into a global complexity burden. |
| Local original-relative score predicts pass but the judge fails | Evaluator/checker semantics remain wrong. | Stop algorithm changes and isolate raster, foreground, Hausdorff, or topology probes. |
| Non-target huge output changes after an inactive medium branch | Time/order determinism is not solved. | Isolate code, add stable keys, and remove normal-path elapsed decisions before submitting. |

## 17. First candidate pseudocode

```text
read original O
classify original tier once

if tier is not the experimental T2 tier:
    run frozen nebula_atomic_region_v33_t7 path
    return

R = render_original_1024(O)
M0 = run_frozen_current_T2(O)
validate M0
bind incremental renderer to (R, M0)

H = build_reversible_aggressive_continuation(M0)
Mc = coarsest safe-topology state requested by the operation budget

while exact_score(O, Mc) < safety_floor:
    candidates = exposed_vertex_splits_with_dependency_closures(H, Mc)
    shortlist by residual recovery / closure vertex cost
    exact-evaluate top nonconflicting alternatives
    apply best refinement batch

prune low-benefit exposed leaves
M1 = fixed_count_flip_relocate_teleport_search(Mc)
M2 = exact_budgeted_delete_and_patch_beam(M1)

if vertices(M2) < vertices(M0)
   and native_score(O, M2) >= safety_floor
   and topology_ok(M2)
   and hausdorff_ok(O, M2):
       output M2
else:
       output M0 exactly
```

The first implementation may stop after `prune`; flips, teleports, and patch
beam search are later isolated variants. The fallback must remain available at
every milestone.

## 18. Primary technical references

These are supporting designs, not substitutes for judge evidence:

1. [Hoppe, *Progressive Meshes*, SIGGRAPH 1996](https://doi.org/10.1145/237170.237216)
   and [view-dependent refinement, 1997](https://doi.org/10.1145/258734.258843):
   reversible edge collapse/vertex split and selective refinement dependencies.
2. [Lindstrom and Turk, *Image-Driven Simplification*, ACM TOG 2000](https://doi.org/10.1145/353981.353995):
   rendered image differences as simplification cost and incremental affected-
   pixel evaluation.
3. [Lindstrom and Turk, *Image-Driven Mesh Optimization*](https://hdl.handle.net/1853/3426):
   fixed-count edge swaps, vertex relocation/teleportation, and escape from a
   greedy simplified mesh.
4. [Cohen-Steiner, Alliez, and Desbrun, *Variational Shape Approximation*](https://doi.org/10.1145/1015706.1015817):
   planar normal proxies and normal-field clustering.
5. [Hu et al., *Error-Bounded and Feature Preserving Surface Remeshing*](https://doi.org/10.1109/TVCG.2016.2632720):
   collapse/split/flip/relocate search with local two-sided error maintenance.
6. [Cohen et al., *Simplification Envelopes*](https://doi.org/10.1145/237170.237220):
   conservative geometric tolerance envelopes.
7. [Sacht and Jacobson, *Cascading Upper Bounds for Triangle Soup Pompeiu-Hausdorff Distance*](https://doi.org/10.1111/cgf.15129):
   branch-and-bound upper/lower Hausdorff certification suitable for beam
   finalists and offline validation.

## Final recommendation

The current best already combines QEM, raster importance, transactional
continuation, hidden-edge cleanup, local SSIM screening, and atomic deletion.
Adding another version of any one of those under the same checkpoint-relative
guards is unlikely to reach 92.1.

The next solver should instead optimize the final constraint directly and make
vertex allocation reversible. Build the original-referenced incremental oracle,
oversimplify, selectively restore only perceptually necessary dependencies,
then improve the fixed-count mesh and pay for repairs with generalized atomic
deletions. That architecture has a credible route to removing the required
18.8% of currently retained vertices while keeping a byte-exact safe fallback.
