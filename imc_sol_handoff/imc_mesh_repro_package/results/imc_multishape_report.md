# IMC multi-shape evaluation: v23 vs v33

**Purpose:** prevent future algorithm choices from being based mainly on smooth spheres or toruses.

- T2 proxy renders use 128×128 for rapid relative comparisons.
- The synthetic T7 diagnostic uses 64×64 because of its size.
- These proxy values are for relative diagnosis, not substitutes for the official judge.
- Every listed candidate passed the local closed, consistently oriented edge-manifold check.

## Results

| Form | V in | v23 V | v33 V | ΔV | v23 final | v33 final | Δ final | v23 min normal | v33 min normal |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| sphere_smooth | 10242 | 2918 | 2900 | -18 | 0.988054 | 0.987996 | -0.000058 | 0.978278 | 0.978000 |
| ellipsoid | 10242 | 2918 | 2897 | -21 | 0.995797 | 0.995656 | -0.000142 | 0.991787 | 0.991540 |
| peanut_concave | 10242 | 2970 | 2962 | -8 | 0.992099 | 0.992056 | -0.000042 | 0.979690 | 0.979638 |
| dimple_concave | 10242 | 2970 | 2952 | -18 | 0.989857 | 0.989810 | -0.000047 | 0.979446 | 0.979421 |
| torus | 8192 | 2457 | 2457 | +0 | 0.986915 | 0.986915 | -0.000000 | 0.970981 | 0.970570 |
| rounded_cube | 10242 | 2918 | 2900 | -18 | 0.996988 | 0.996973 | -0.000015 | 0.993911 | 0.993921 |
| wavy_highfreq | 10242 | 3072 | 3072 | +0 | 0.953639 | 0.953491 | -0.000148 | 0.911364 | 0.911460 |
| cube_planar_sharp | 6146 | 1813 | 1813 | +0 | 0.877678 | 0.875538 | -0.002140 | 0.759490 | 0.766215 |
| thin_box_sharp | 6146 | 1843 | 1641 | -202 | 0.805472 | 0.803544 | -0.001927 | 0.450870 | 0.493386 |
| cylinder_sharp | 24578 | 7373 | 7250 | -123 | 0.870161 | 0.868924 | -0.001236 | 0.654169 | 0.616166 |
| cone_sharp | 12290 | 3502 | 3502 | +0 | 0.855326 | 0.858825 | +0.003499 | 0.647294 | 0.677399 |
| capsule | 8066 | 2379 | 2373 | -6 | 0.996682 | 0.996675 | -0.000007 | 0.989754 | 0.989732 |
| t7_symmetric_sphere_diagnostic | 408578 | 10623 | 10623 | +0 | 0.999249 | 0.999249 | -0.000000 | 0.998421 | 0.998420 |

## Conclusions

1. **v33 is rejected.** Its extra reduction is small and generally buys that reduction by lowering the normal-map score.
2. **Smooth-form gains do not generalize.** Sphere and ellipsoid improve by only 18–21 vertices, with a measurable proxy regression.
3. **Sharp and thin forms are the real stress test.** The thin box loses 202 additional vertices but also loses perceptual quality; this is exactly the type of misleading “compression gain” that a sphere-only test misses.
4. **The T7 failure is not evidence that the region operator ran on T7.** It did not. On a 408,578-vertex symmetric sphere, v23 and v33 produced identical counts but different vertex trajectories and slightly different normal scores. Extra code perturbed equal-cost/timing-sensitive behavior.
5. **Future candidate ordering must be deterministic**, and experimental code must be structurally isolated from T5–T7.

## Required benchmark categories for every future algorithm

| Category | Included forms | Failure mode exposed |
|---|---|---|
| Smooth convex | sphere, ellipsoid, capsule | baseline curvature approximation |
| Concave genus 0 | peanut, dimple | occlusion and newly exposed surfaces |
| Genus 1 | torus, thin torus | holes and self-occlusion |
| Sharp mechanical | cube, cylinder, cone | feature-edge and flat-normal preservation |
| Thin structures | thin box | opposite surfaces and Hausdorff risk |
| Frequency stress | bumpy/wavy surfaces | normal-map sensitivity |
| Near-axis-aligned rounded | rounded cube | six-view evaluator bias |
| Large-tier determinism | 400k+ symmetric mesh | priority-queue tie and time-gate sensitivity |

## Next algorithmic direction: budget-neutral split–collapse redistribution

All v28–v33 methods only moved or deleted vertices. They could not reallocate geometric detail after the greedy QEM trajectory placed vertices badly. The next method should deliberately **add** a small number of vertices in high-render-error regions while deleting more vertices in low-impact regions, so total count still decreases.

Proposed transaction:

1. Start from the trusted v23 candidate.
2. Render it against the original and accumulate normal/depth residual per visible triangle.
3. Select high-residual triangles for edge or centroid splits, projecting new points onto the original surface.
4. Select low-impact collapses elsewhere with enough total gain to pay for every split plus a net reduction.
5. Apply splits and collapses as one atomic transaction.
6. Tangentially relax only the newly affected one-rings.
7. Validate topology, sampled original-surface distance, and the six-view score against the original.
8. Commit only when the transaction has fewer vertices than v23 and improves or preserves the perceptual margin.

This is a materially different search space: it can move the vertex budget from hidden/flat regions to silhouettes and high-normal-error regions instead of merely trying another deletion order.

## Promotion rule

A future branch is not judge-ready until it:

- beats v23 in aggregate vertex count across at least four distinct form categories;
- does not regress minimum normal score materially;
- is byte-identical to v23 on all non-target tiers or uses deterministic tie-breaking verified on the large-tier diagnostic;
- passes the official sample and local manifold validator;
- has a documented fallback path.

