# Tranberry iteration log

## Theory

Tranberry starts from the official-valid Nebula v14 solver (90.187632,
PPPPPPP). Its tier targets are treated as tuned. The family instead changes
the primary collapse ordering to approximate the actual flat-normal and depth
render loss under the six axial cameras.

## Batch 1 — v001-v005

**Hypothesis:** QEM plane distance misranks perceptually costly collapses.
Patch-level normal rotation and axial screen-motion surrogates can preserve
more SSIM at the same target, creating room for later structural compression.
All five profiles affect every tier and retain Nebula's target schedule.

| Version | Structural rank | Local compression | Local observation |
|---|---|---:|---|
| v001 | area-weighted normal rotation | 91.322467% | highest fidelity on several proxies; expensive |
| v002 | six-view projected motion | 91.691530% | higher compression, broad SSIM regression |
| v003 | normal + motion + area Pareto sum | 91.483043% | behavior-distinct compromise |
| v004 | max(normal, motion) risk | 91.449979% | improves a few difficult shapes |
| v005 | combined worst-view minimax | 91.752947% | best batch compression, mixed fidelity |

All candidates were topology-valid on 10/10 default scenarios. The surrogate
is active, but recomputing full patch loss for queue refreshes consumes enough
time to prevent some meshes from reaching their tuned target. Kattis is needed
to determine whether official mid-tier SSIM gains outweigh this throughput
loss.

## Batch 2 — v006-v007

**Hypothesis:** Batch 1's full union-ring render surrogate is directionally
useful but too expensive on every stale queue refresh. Sparse all-tier patch
architectures should retain the perceptual signal while restoring throughput.
Per the updated workflow, this and future batches contain exactly two files.

| Version | Structural rank architecture | Local compression | Local observation |
|---|---|---:|---|
| v006 | deterministic 12-face union-ring cap | 91.025186% | still expensive; generally better SSIM but misses more targets |
| v007 | absorbed-endpoint one-ring only | 91.437292% | order-of-magnitude faster on several proxies; SSIM 0.9994/0.9971 on the two complex tier-2 meshes |

Both candidates are topology-valid on 10/10 default scenarios. v007 is the
stronger architecture: the directional absorbed-ring loss distinguishes which
endpoint should disappear and avoids duplicate kept-ring work. The files are
locally complete but will remain unsubmitted until batch 1 is terminal.

## Local infrastructure interval — v011-v018

Kattis batch 1 became abnormally slow; v001 eventually returned `PFFFFFF`
while v002-v005 were canceled without results. During the requested local-only
interval, the evaluator was changed to numbers-only: no local validity field,
PASS/FAIL, strict thresholds, scenario pass count, or RESULT verdict. It now
prints compression, SSIM, Hausdorff usage, IoU, topology counts, resolution,
runtime, and fingerprints.

| Version | Structural experiment | Mean compression | Mean SSIM | Mean Haus% | Finding |
|---|---|---:|---:|---:|---|
| v011 | Nebula edge order + perceptual placement only | 91.121086% | — | — | very safe on complex proxies, lower compression |
| v012 | bounded absorbed-ring queue perturbation | 91.549140% | — | — | better throughput/compression compromise |
| v013 | unified perceptual-first postpasses on every tier | 92.191970% | 0.853595 | 41.1425 | first clear local compression gain |
| v014 | unified topology-first postpasses | 92.191970% | 0.853595 | 41.1425 | byte-identical to v013; ordering inert |
| v015 | guarded over-target absorbed-ring QEM | 92.132212% | 0.863940 | 37.0016 | strong Pareto point |
| v016 | worst-view minimax gate | 35.933751% | 0.927320 | 20.5070 | rejected as drastically over-conservative |
| v017 | guarded over-target QEM + unified postpasses | 92.326736% | 0.862538 | 37.7760 | current local leader |
| v018 | fixed-point topology/perception postpasses | 92.303081% | 0.852390 | 41.1425 | more compression than v013 but weaker SSIM |

All reported topology counts for v011-v018 were zero. v017 improves
`abc_00011084` to 74.9473% compression at 0.9824 SSIM and is the leading
candidate for a future exact-two Kattis batch after the current job terminates.
