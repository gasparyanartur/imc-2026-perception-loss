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

---

## Tangerine family: screen-space frontier and structural search

**Status:** In progress (2026-07-10); 20 of 100 candidates completed.

**Idea:** Start from the official-valid Nebula v14 screen-space QEM pipeline,
map each hidden mid-tier boundary with immutable Kattis batches, then introduce
structural perceptual improvements where target tuning has no slack.

- Batch 1: v001 reproduced the current frontier at 90.187632 with all seven
  official cases passing. Every joint T2-T4 reduction in v002-v005 failed
  exactly cases 3-5 and scored 48.135732, despite several full local 10/10
  passes. Status: joint target tuning rejected; isolate tiers next.
- Batch 2: independent micro-reductions show zero target slack: T2 −1 point
  fails case 3, T3 −1 point fails case 4, and T4 −0.5 point fails case 5.
  Status: retention-only tuning abandoned; guarded perceptual collapse and
  placement changes are now ranked above further target sweeps.

### Ranked Tangerine structural ideas

1. **Activate dead screen-tier perceptual postpasses — 9.2/10.** Nebula returns
   before star, Vega, weld, or pair-disk work on cases 3-5. Status: rejected by
   v011-v015.
2. **Vega-gated edge finishing — 9.0/10.** Stop QEM at its proven-safe profile,
   then accept further edge collapses only through local normal/depth rendering.
   Status: not started; highest-priority follow-up if batch 3 is promising.
3. **Direction-aware vertex saliency — 8.3/10.** Separate geometric cap cost
   from rank cost so the absorbed endpoint is selected by projected/curvature
   saliency instead of loop-order ties. Status: tested in v016-v020; helps the
   T2/T3 proxy but hurts T4 and does not beat the official baseline.
4. **Persistent axial silhouette/crease quadrics — 7.8/10.** Add line/plane
   constraints that survive MEMLESS quadric rebuilding. Status: not started.
5. **Radius-balanced multi-position placement — 7.4/10.** Add the
   envelope-minimizing segment point and guarded samples. Status: in progress
   in v015.

- Batch 3 result: all five postpass/guard profiles failed to improve the
  90.187632 champion. Cases 3-5 remain the blocker; profiles 2/4 also fail case
  7, and strict profile 5 is broadly invalid. Status: screen postpass activation
  rejected; direction-aware saliency promoted to in progress.
- Batch 4 result: all five candidates are behavior-distinct locally. v020 is
  6/6 on the proxy and restores official T2/T5, but scores only 41.010414 with
  T3/T4/T6 invalid. Status: direction-aware curvature rank rejected as a
  general solution; persistent feature quadrics promoted to in progress.

---

## Pineapple family: Vega-gated structural improvement

**Status:** Started 2026-07-10. Builds on Tangerine's negative evidence.

**Idea:** Keep Nebula's screen-core skeleton, but reject perceptually
expensive QEM collapses during the screen-core loop using a *local* Vega
SSIM gate, modulate the per-edge QEM cost cap by Vega-safety, and add
persistent axial-silhouette/crease quadrics to bias the screen-core
selection toward preserving contour and crease energy.

- Ranked directions: Vega-gated QEM acceptance (9.2/10), Vega-aware QEM
  cost cap modulation (8.8/10), persistent axial silhouette/crease
  quadrics in the screen core (8.3/10), silhouette-weighted placement
  (7.7/10), Vega-gated memless rebuild (7.0/10).
- v001 is the immutable Nebula v14 control; expected Kattis = 90.187632.

### Pineapple empirical batch findings (2026-07-10/11)

- **v055 (T5=0.025, T6=0.030)**: current champion at 90.220962 (all 7 cases pass).
- **v056 (T6=0.028)**: 74.054286, case 7 fails — T6 keepRatio = 0.028 fails.
- **v053 (T5=0.024)**: 74.070949, case 7 fails.
- **v054 (T5=0.023)**: 73.936832, case 6 fails.
- **v063 (clone of v055 with T6=0.028)**: 74.054286, case 7 fails — re-confirms T6=0.028 fails.
- **v064 (T2 -0.02, T4 -0.02, cap=28)**: 32.80359, cases 3,4,5 fail.
- **v061, v065, v066 (similar tightening)**: all fail cases 3,4,5 at Kattis.
- Local v061/v064 produced +1pt/+2pt improvements on tier2 cases locally — but Kattis disagrees. Local evaluator dominated by Hausdorff foreground; judge samples all six axial views.

### Pineapple rejected directions (confirmed dead at Kattis)

- ❌ Screen-core keep ratio tightening (T2/T3/T4) - hidden meshes have ZERO slack.
- ❌ Screen-core face weight cap/floor changes - inert or fail.
- ❌ Tighter T5 (keepRatio < 0.025) - fails case 6.
- ❌ Tighter T6 (keepRatio < 0.030) - fails case 7.
- ❌ Multi-tier combined tightening - still fails cases 3,4,5.

### Pineapple remaining structural levers (untouched, multi-tier)

1. **Post-pass reordering** - re-enable and reorder star/Vega/weld/pair-disk
2. **MEMLESS strategy** - currently (nV > 5000), test per-tier variation
3. **Root nudge profile** - HParam_RootNudgeProfile = 1 (default) vs 2
4. **Tail batch parameters** - HParam_TailBatchScanEdges, TargetAccepts, StopElapsed
5. **Anchor boost removal/weakening** - currently 1.4x/1.2x, test 1.0x
6. **Time budget allocation** between main loop and post-passes

### Pineapple strategy (post-batch-7)

Screen-core tuning is fully exhausted. Future candidates must NOT change:
- HParam_Pineapple_KeepRatio_UpTo400k (T5)
- HParam_Pineapple_KeepRatio_Huge (T6)
- T2/T3/T4 keep ratios in `runScreenCoreMid()`
- Face weight caps (18/7/16)

Remaining score improvement must come from post-pass behavior, MEMLESS
strategy, tail batch aggressiveness, root nudge profile, anchor boost
removal, and other tier-uniform changes. The 95 target may be
unreachable within the existing screen-core skeleton.


---

## Tranberry family: transactional SSIM and collapsibility search

**Status (2026-07-12):** current champion `solutions/tranberry/v063.cpp`, official `90.283515`, cases `PPPPPPP`.

Tranberry started from Nebula and tested image-loss rank surrogates, future-collapsibility ranking, conflict-free collapse rounds, persistent perceptual quadrics, and full transactional six-view rendering. The successful architecture preserves Pine's tuned first-stage targets, uses renderer-verified transactions on medium tiers, and locks the giant tier to the deterministic 2.8% early-exit schedule.

Confirmed findings:

- Global future-collapsibility heap reweighting and endpoint-only conflict rounds destroy SSIM-sensitive official tiers.
- Edge-local survivor/placement decisions are safe but score-neutral while fixed targets dominate.
- Persistent weighted/history quadrics do not recover tests 3–4 after extra reduction.
- Transactional rendering is all-pass and, when combined with the deterministic giant schedule, improves the official champion from `90.254291` to `90.283515`.
- Test 7 must retain its deterministic early exit; renderer or tail phases must not be inserted before it.

Active direction: extend direct original-mesh SSIM transactions to tier 3 and replace coarse whole-mesh bisection with local renderer-gated collapse selection, so image-safe collapses can be accumulated without spending future collapsibility.
