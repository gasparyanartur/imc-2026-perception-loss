# Original-Referenced Cohort Simplification

> **Deprecated / official dead end.** All submitted cohort variants failed
> official T4, and neither official T3 nor T5 improved. The sources were removed
> from the active workspace. This document is retained only as negative evidence.

## Tier convention

This iteration uses seven names consistently:

- `T1`: sample, at most 10 vertices;
- `T2`: at most 5,000 vertices;
- `T3`: at most 25,000 vertices;
- `T4`: at most 40,000 vertices;
- `T5`: at most 50,000 vertices;
- `T6`: at most 400,000 vertices;
- `T7`: at most 1.1 million vertices.

The solver's internal `screenTier=2/3/4` paths correspond to official
`T3/T4/T5`. Earlier notes incorrectly shifted these labels down by one.

## Why the operation changes the solution family

The v46 endpoint is produced by one global QEM priority queue. A continuation
attempt can fail even when most of its collapses are harmless, because a small
number of visually expensive collapses consumes the entire SSIM margin.

The cohort method separates those decisions. Let the current mesh be (M),
the original mesh be (M_0), and let

\[
S(M_0,M)=\frac{1}{6}\sum_{v=1}^{6}
\left(\frac{1}{2}S_N^{(v)}+\frac{1}{2}S_D^{(v)}\right)
\]

be the actual aggregate normal/depth objective.

For every currently legal QEM edge (e=(a,b)), the solver computes the normal
QEM candidate but does not immediately put it into one global queue. It first
builds an independent set: selecting an edge blocks its two endpoints and both
endpoint one-rings. This makes every operation in the set non-overlapping.

The edge midpoint is assigned to a spatial cohort. The adaptive variants use
16 cells: four bins along X and two bins each along Y and Z. Applying a cohort
therefore changes a localized, conflict-free part of the mesh.

For cohort (g), let (M_g) be the mesh after its valid collapses and let
(n_g=|V(M)|-|V(M_g)|). Its measured damage per removed vertex is

\[
D_g=\frac{\max(0,S(M_0,M)-S(M_0,M_g))}{\max(1,n_g)}.
\]

The cohorts are sorted by (D_g), not by QEM cost alone. They are then applied
transactionally in that order. A cohort is retained only while a low-resolution
original-referenced check stays above

\[
S_{\text{round}}=\max(0.91,S_{\text{start}}-0.006).
\]

After the accepted cohorts are combined, the entire mesh is rendered against
the original at 1024 resolution. The round is rolled back unless

\[
S(M_0,M_{\text{round}})\ge 0.91.
\]

All collapses still pass v46's link, orientation, QEM-placement, cluster-radius,
and accumulated Hausdorff guards. The complete output is additionally checked
as a closed oriented manifold in local validation.

## Variants

- `v51_midcohort.cpp`: conservative three-round, eight-cohort implementation.
  It uses the original static safety floors and is the lowest-risk official
  probe.
- `v52_midcells16.cpp`: 16 spatial cohorts and a margin-relative round budget.
  This activates on moderately difficult forms where v51 falls back.
- `v53_midcells32.cpp`: 32-cell resolution probe. It matched v52 locally while
  taking longer, so it is retained only as negative evidence.
- `v54_midconverge.cpp`: repeats the 16-cell transaction until no gain or the
  reserved render deadline. The candidate population adapts to the current
  mesh size. This is the maximum-gain version and retains v46's T7 atomic tail.
- `v55_midconv_safe7.cpp`: same middle-tier output as v54, but skips the final
  six-vertex T7 atomic transaction. It sacrifices approximately `0.0001` total
  official-score scale to reduce the known million-tier layout/timing risk.

## Local results that did not transfer

Native proxy values below use 1024 resolution.

| Case | v46 vertices / SSIM | v52 vertices / SSIM | v54 vertices / SSIM |
|---|---:|---:|---:|
| T3-size sphere, 10,242 V | `2849 / 0.973881` | `2115 / 0.968903` | `2115 / 0.968903` |
| 40,002-V boundary diagnostic | `5800 / 0.985548` | `4528 / 0.983537` | `1349 / 0.963936` |
| 40,002-V wavy diagnostic | `5800 / 0.857612` | fallback | fallback |
| T5 peanut, 48,002 V | `3600 / 0.978971` | `2879 / 0.975995` | `995 / 0.957810` |
| T5 bumpy, 48,002 V | `3792 / 0.924280` | `3047 / 0.917684` | `2438 / 0.911071` |

The independent evaluator marked the three aggressive v54 middle outputs
valid, including its vertex-Hausdorff proxy:

- T3 ellipsoid: `1349` vertices, `0.078766 <= 0.128339`;
- T4 peanut: `995` vertices, `0.102331 <= 0.146540`;
- T4 bumpy: `2438` vertices, `0.122284 <= 0.181796`.

The T3 ellipsoid and T4 peanut results remove respectively 76.7% and 72.4% of
the vertices left by v46. Repeated v54 T3 runs were byte-identical, despite the
convergence deadline.

## Official outcome

- Every tested cohort variant failed official T4.
- Official T3 and T5 compression did not improve.
- Conclusion: the low-resolution spatial-cohort selector overfit local forms
  and/or spent geometric margin not represented by its proxy. Do not tune or
  resubmit this family.
