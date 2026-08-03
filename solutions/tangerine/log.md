# Tangerine iteration log

## Theory

Tangerine starts from the current champion, `solutions/nebula/v14.cpp`.
Nebula's multi-resolution screen-space importance pipeline is retained while
the family searches for valid reductions of its staged T2-T4 targets. Kattis
is the ground-truth oracle; local native evaluation is diagnostic.

## Batch 1 — v001-v005

**Hypothesis:** Nebula's raster-weighted QEM retains enough perceptual margin
to lower its conservative mid-tier endpoints before a new structural mechanism
is required.

| Version | Meaningful change | Local result | Kattis result |
|---|---|---|---|
| v001 | Immutable Nebula v14 control | INVALID 4/10, 91.898326% | 90.187632, PPPPPPP |
| v002 | Moderate T2-T4 target reduction | VALID 10/10, 90.979112% | 48.135732, PPFFFPP |
| v003 | Strong T2-T4 target reduction | VALID 10/10, 92.179863% | 48.135732, PPFFFPP |
| v004 | Near-95 target profile | 3-mesh topology/Hausdorff only, 91.885255% | 48.135732, PPFFFPP |
| v005 | Above-95 nominal target profile | VALID 10/10, 93.779145% | 48.135732, PPFFFPP |

**Post-mortem:** v001 establishes the official-valid 90.187632 baseline.
All four joint T2-T4 reductions fail exactly official cases 3-5, even though
v002, v003, and v005 pass the full local suite. The identical partial score
shows that the parameter changes do not affect official cases 1, 2, 6, or 7.
The local acceptance threshold is therefore non-blocking and substantially
miscalibrated for these hidden mid-tier meshes.

## Batch 2 — v006-v010

**Hypothesis:** isolate each mid-tier frontier with sub-two-point reductions to
identify which official cases have any parameter slack before changing the
screen-space algorithm.

| Version | Meaningful change | Fast local result | Kattis result |
|---|---|---|---|
| v006 | T2 endpoint 0.30 to 0.28 only | 2/3, 88.327917% | 78.520750, PPFPPPP |
| v007 | T2 endpoint 0.30 to 0.29 only | 2/3, 87.995829% | 78.520750, PPFPPPP |
| v008 | T3 endpoint 0.145 to 0.135 only | 2/3, 87.663741% | 75.937472, PPPFPPP |
| v009 | T4 endpoint 0.080 to 0.075 only | 2/3, 87.663741% | 74.853979, PPPPFPP |
| v010 | Half-step reduction in T2, T3, and T4 | 2/3, 87.829785% | 48.936936, PPFFFPP |

**Post-mortem:** retention-only tuning has no measurable official slack.
A one-point T2 reduction, one-point T3 reduction, and half-point T4 reduction
each independently invalidate its corresponding case. v006 and v007 also
produce the same partial score, so further micro-steps are not useful
iterations. Future candidates must change collapse quality, must tune every
tier simultaneously, and must be fingerprint-distinct on tier-matched proxies.

## Batch 3 — v011-v015

**Hypothesis:** Nebula's screen-space core returns before its perceptual
postpasses, leaving safe star/Vega/pair-disk reductions unused on exactly
official cases 3-5. Activating distinct guarded finishers, while tuning all six
tiers in every source, can cross the current target cliffs.

| Version | All-tier structural direction | Fast local result | Kattis result |
|---|---|---|---|
| v011 | Screen-tier star finisher; six-tier target/budget profile 1 | 3/6, 90.935392% | 48.970264, PPFFFPP |
| v012 | Screen-tier Vega finisher; six-tier profile 2 | 3/6, 90.425950% | 32.828562, PPFFFPF |
| v013 | Combined star/Vega and conservative tier-4 Vega; profile 3 | 3/6, 91.208686% | 48.976959, PPFFFPP |
| v014 | All finishers plus local normal-cone guard; profile 4 | 3/6, 91.536082% | 32.836739, PPFFFPF |
| v015 | Strict guard plus radius-balanced placements; profile 5 | compile-only after sandbox failure | no score, PFFFFFF |

**Post-mortem:** activating the dead postpasses changes every larger-tier
fingerprint but does not recover cases 3-5. Vega/strict profiles also break
case 7, and the radius-balanced strict profile is broadly invalid. The
star/Vega postpass hypothesis is rejected. Return to v001 and change directed
edge ranking while retaining the known-safe topology schedule.

## Batch 4 — v016-v020

**Hypothesis:** QEM assigns equal raw cost to both collapse directions, so loop
order arbitrarily chooses the absorbed endpoint. A separate curvature-saliency
rank cost can preserve visually important endpoints and make small reductions
safe. Every profile changes the tiny floor, T2-T4 stages, T5/T6 targets, and
tier-scaled saliency strength.

| Version | Quality profile | Fast six-tier proxy | Kattis result |
|---|---|---|---|
| v016 | Light saliency + sub-0.05 envelope + local geometry guard | 5/6, 84.632023%; T4 SSIM 0.8710 | 16.487718, PPFFFFF |
| v017 | 2x saliency and stricter guard | 5/6, 83.916601%; T4 SSIM 0.8677 | 16.499919, PPFFFFF |
| v018 | 4x saliency and stricter guard | 5/6, 83.144329%; T4 SSIM 0.8659 | 16.487718, PPFFFFF |
| v019 | 8x saliency except near-raw T4 rank | 5/6, 82.323481%; T4 SSIM 0.8852 | 16.451114, PPFFFFF |
| v020 | Strict T1-T3/T5-T6; raw-QEM T4 control | 6/6, 81.240115%; T4 SSIM 0.9180, Haus 0.0100 | 41.010414, PPPFFPF |

**Post-mortem:** local normal cones and curvature saliency genuinely improve
the proxy's first three quality margins, but the all-tier profiles overfit it.
Only v020 restores official T2 and T5 acceptance; T3/T4 remain SSIM failures
and its more aggressive T6 target loses case 7. Curvature ordering is actively
harmful on the T4 torus, where raw QEM raises proxy SSIM from 0.8951 to 0.9180.
Return T5/T6 to the v001-safe frontier and test persistent feature quadrics in
the primary screen core rather than accumulated per-collapse normal cones.

## Batch 5 — v021-v025

**Hypothesis:** Original feature-plane energy that survives MEMLESS rebuilds
can preserve flat normals, depth, creases, and axial silhouettes at the
zero-slack T2-T4 frontier. v021-v023 use persistent crease/silhouette line
planes; after the proxy showed a losing strength trend, v024-v025 use
persistent area/view-weighted original tangent-plane bundles. Every version
also independently changes its T1 floor/envelope/star budget, all T2-T4
stages, and conservative T5/T6 targets/envelopes.

| Version | Persistent screen energy | Fast six-tier proxy |
|---|---|---|
| v021 | Feature-line planes, light | 3/6, 89.715789%; T2/T3/T4 SSIM 0.8910/0.8806/0.9111 |
| v022 | Feature-line planes, 2x | 3/6, 89.731043%; 0.8892/0.8753/0.9069 |
| v023 | Feature-line planes, 4x | 3/6, 89.755533%; 0.8883/0.8742/0.9023 |
| v024 | Original tangent-plane bundle | 3/6, 89.808407%; 0.8911/0.8798/0.9153 |
| v025 | Original tangent-plane bundle, 2x | 3/6, 89.868990%; 0.8895/0.8787/0.9139 |

All thirty tier outputs are fingerprint-distinct. Locally, increasing either
feature energy is dominated; tangent-plane persistence is less harmful and
keeps T4 strong, but neither proxy direction crosses T2/T3 SSIM. Kattis is the
ranking oracle and the complete batch was submitted.
