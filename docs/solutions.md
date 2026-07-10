# Solution Bucket

This is a living list of solution ideas for the IMC 2026 mesh-simplification challenge. Each entry is a bucket that agents should update as the project progresses: add notes, observations, results, and decisions. Do not treat the order or initial status as final.

## How to update this file

When you try an idea, add a dated note under the relevant bucket. Include:
- What changed.
- What the result was (compression rate, validity, SSIM, Hausdorff).
- Whether the idea is promising, abandoned, or needs more work.
- Open questions or next steps.

For each bucket, form hypotheses about what will work and what will not. When you try an idea, record the result and update the hypothesis (as a note, dated). If you find something that already exists in the code, note that too. If you find a new idea, add a new bucket with a brief description and initial status.

---

## Bucket 1: Perception-aware QEM edge collapse

**Idea:** Iteratively collapse edges into optimal replacement vertices using a cost function that blends quadric error, normal preservation, silhouette preservation, and shape regularity.

**Status:** Primary candidate.

**Notes:**
- 2026-07-09: Identified as the most promising starting point. Standard QEM is well-understood and can be adapted to the judge's flat-normal and depth maps.- TODO: Implement basic QEM collapse in `solutions/baseline/baseline.py`.
- TODO: Add validity guards (link condition, positive area, no duplicate faces, normal-flip check).
- TODO: Add perception terms (normal penalty, silhouette penalty, dihedral penalty).

**Open questions:**
- What hyperparameters give the best compression/validity trade-off?
- Is the SSIM threshold the binding constraint, or is Hausdorff?
- Should candidate positions include endpoint A, endpoint B, midpoint, and QEM optimum, or fewer?

---

## Bucket 2: Plane-patch merging + retriangulation

**Idea:** Detect nearly coplanar regions, merge them into larger patches, and retriangulate with fewer vertices.

**Status:** Not started.

**Notes:**
- 2026-07-09: Strong conceptual fit for flat regions but risky for manifold validity and silhouette preservation.
- TODO: Evaluate whether patch merging helps after a QEM baseline is working.

**Open questions:**
- Can patch merging be done without creating non-manifold edges or duplicate faces?
- Does it improve compression enough to justify the complexity?

---

## Bucket 3: Feature-preserving decimation with silhouette locking

**Idea:** A QEM variant that aggressively protects silhouette edges and high-dihedral feature edges across the six axial views.

**Status:** Not started.

**Notes:**
- 2026-07-09: Likely useful if SSIM is the binding constraint.
- TODO: Add silhouette-saliency computation and feature-edge locking/penalty to the QEM cost.

**Open questions:**
- How much does over-locking silhouettes hurt compression?
- Is view-dependent cost computation too slow at 1.1M vertices?

---

## Bucket 4: Vertex clustering / quantization

**Idea:** Group nearby vertices into clusters and replace each cluster with a single representative.

**Status:** Not started.

**Notes:**
- 2026-07-09: Fast and scalable but unlikely to produce valid, high-scoring output on its own.
- TODO: Try as a baseline to understand the lower bound on compression.

**Open questions:**
- Can clustering be repaired into a manifold afterward?
- Does it give useful insight into achievable compression?

---

## Bucket 5: Voxelization + marching cubes

**Idea:** Convert the mesh to a signed distance field and extract an isosurface at coarser resolution.

**Status:** Not started.

**Notes:**
- 2026-07-09: Naturally watertight but tends to lose sharp features and fail SSIM.
- TODO: Probably skip unless other approaches fail.

**Open questions:**
- Is there a resolution that satisfies both Hausdorff and SSIM?

---

## Bucket 6: Neural / learned simplification

**Idea:** Train a model to predict simplified vertex positions and connectivity.

**Status:** Not started.

**Notes:**
- 2026-07-09: High engineering risk; no guarantee of manifold output; contest output must be a plain mesh.
- TODO: Not recommended as primary path.

**Open questions:**
- Is there a lightweight, deterministic neural approach that preserves topology?

---

## Bucket 7: Global energy optimization / remeshing

**Idea:** Formulate simplification as a global optimization over vertex positions and connectivity.

**Status:** Not started.

**Notes:**
- 2026-07-09: Too slow for contest input sizes; borrow local energy terms only.
- TODO: Not recommended as primary path.

**Open questions:**
- Are any global terms worth adding to the local QEM cost?

---

## Bucket 8: Post-process vertex position optimization

**Idea:** After simplification, optimize vertex positions to reduce Hausdorff distance or improve SSIM without changing connectivity.

**Status:** Not started.

**Notes:**
- 2026-07-09: Could recover margin from a collapse-heavy mesh.
- TODO: Try after a working simplification pipeline exists.

**Open questions:**
- Does small vertex perturbation improve SSIM enough to allow more aggressive collapse?

---

## Bucket 9: Output-size-aware simplification

**Idea:** Track output byte budget (100 MiB) and stop or adjust when approaching the limit.

**Status:** Not started.

**Notes:**
- 2026-07-09: Relevant for very large meshes if many vertices remain.
- TODO: Ensure output formatting uses compact decimal representation.

**Open questions:**
- At what compression level does the byte limit become active?

---

## Bucket 10: Hybrid pipeline

**Idea:** Combine multiple buckets, e.g., QEM collapse followed by patch merging or position optimization.

**Status:** Not started.

**Notes:**
- 2026-07-09: Most realistic path to a top score.
- TODO: Build a strong QEM baseline first, then layer improvements.

**Open questions:**
- Which combination of buckets gives the best valid compression rate?

---

## Starting hyperparameters

When implementing Bucket 1, these values are a reasonable initial guess. Update this section as tuning produces better values.

| Parameter | Initial value | Notes |
|---|---:|---|
| Max collapse length fraction | $0.018 \, D_{\mathrm{AABB}}$ | Hausdorff safety guard. |
| Minimum triangle area | $10^{-14}$ | Degenerate-face guard. |
| Max normal change | $70°$ | Flat-shading flip guard. |
| Lock feature dihedral | $48°$ | Sharp-edge protection. |
| Soft dihedral | $20°$ | Moderate-crease penalty start. |
| $w_Q$ (QEM) | 1.0 | Base geometric cost. |
| $w_N$ (normal) | 2.0 | Flat-normal preservation. |
| $w_D$ (dihedral) | 4.0 | Crease preservation. |
| $w_S$ (silhouette) | 8.0 | Six-view contour preservation. |
| $w_L$ (length) | 0.01 | Mild edge-length regularizer. |
| $w_{\mathrm{shape}}$ | 0.02 | Mild shape-quality regularizer. |

---

## Honeydew family: ranked 2026-07-10 brainstorm

The Honeydew family starts from Lemon v115. The local baseline is invalid on
six of ten ppsurf meshes: its T1 zero-retention target over-simplifies the
small-mesh tier. The following independent ideas are ranked by expected
official-score impact, implementation risk, and fit with flat-normal SSIM.

1. **Tier-calibrated retention frontier** — Sweep T1/T2 keep ratios and post-pass
   budgets independently to find the highest valid compression margin. **Status:
   in progress; rank 1.**
2. **Occlusion-aware star patches** — Reject star retriangulations when nearby
   non-incident faces can occlude the altered patch in an axial view. **Status:
   not started; rank 2.**
3. **True local Gaussian SSIM** — Replace Vega's global patch statistic with the
   evaluator's foreground-masked 11×11 Gaussian SSIM. **Status: not started;
   rank 3.**
4. **Screen-space normal-area cost** — Weight each face quadric by projected
   area in all six views, not only world-space area and axial normal magnitude.
   **Status: not started; rank 4.**
5. **Silhouette-stratified collapse budget** — Classify vertex projections as
   silhouette/interior per axial view and reserve aggressive collapses for
   interior-only vertices. **Status: not started; rank 5.**
6. **Feature-line quadrics** — Add quadrics along high-dihedral edges to preserve
   crease location and flat-shaded normal discontinuities. **Status: not
   started; rank 6.**
7. **Adaptive per-component normal cones** — Use local normal-cone spread,
   rather than a fixed deviation threshold, to authorize smooth-region star
   deletes. **Status: not started; rank 7.**
8. **Bidirectional sampled envelope** — Track source samples assigned to each
   survivor, augmenting the existing collapse radius with directional
   point-to-patch bounds. **Status: not started; rank 8.**
9. **Multi-root polygon triangulation** — Evaluate all valid ears or a dynamic
   programming triangulation for star cavities instead of root fans. **Status:
   not started; rank 9.**
10. **Constrained vertex relaxation** — Relax surviving vertices on their
    original local tangent planes after collapse while preserving a geometric
    envelope. **Status: not started; rank 10.**
11. **View-balanced priority queues** — Penalize candidates concentrated in the
    currently most-damaged axial view so visual error is spread across cameras.
    **Status: not started; rank 11.**
12. **Curvature-density targets** — Allocate lower keep ratios to low-curvature
    regions and retain denser samples near curvature extrema. **Status: not
    started; rank 12.**
13. **Topology-safe valence regularization** — Include predicted post-collapse
    valence in the cost to avoid poorly shaped, visually noisy triangles.
    **Status: not started; rank 13.**
14. **Coarse-to-fine candidate rescoring** — Use a fast low-resolution axial
    renderer to pre-rank candidates, followed by exact local SSIM for finalists.
    **Status: not started; rank 14.**
15. **Patch coalescing with constrained retriangulation** — Merge nearly
    coplanar, non-silhouette patches after QEM. **Status: deferred; rank 15**
    because robust manifold and occlusion handling is high risk.

**Initial Honeydew hypothesis:** the immediate local blocker is T1 retention,
while screen-space normal preservation is the most likely path beyond the
retention frontier on the official large tiers. Start with ranked idea 1, then
use its diagnostic margin to decide whether ideas 2–6 justify their cost.

---

## Smoke-run ideas: banana family

**Status:** In progress (2026-07-10).

- **Adaptive small-mesh retention:** Raise the tiny-mesh keep ratio until all
  perceptual gates pass, then spend the remaining reduction budget on
  geometry-safe star collapses. Hypothesis: the lemon T1 target of zero
  vertices is the dominant cause of SSIM failures on the ppsurf smoke suite.
- **Tier-boundary calibration:** Tune T1 and T2 independently because the
  local ppsurf inputs straddle the 5,000-vertex boundary. Hypothesis: a single
  aggressive parameter set cannot preserve both sparse and dense meshes.
- **Perceptual safety margin:** Compare progressively stricter Vega SSIM and
  damage limits after establishing validity. Hypothesis: a small margin above
  the 0.90 native gate may permit more reliable compression than geometric
  guards alone.

These ideas are being tested in the `solutions/banana` smoke-run family.

- 2026-07-10 workflow smoke run: `v17.cpp` was locally valid on all 10 ppsurf
  meshes at 26.061328% compression, below `v16.cpp`'s 30.447274%. Further
  increases in T1/T2 retention are therefore not promising without official
  evidence that the local evaluator underestimates perceptual risk. A
  concurrent four-candidate synthetic evaluation instead exhausted the native
  diagnostic timeout, so local candidate sweeps must be sequential.
