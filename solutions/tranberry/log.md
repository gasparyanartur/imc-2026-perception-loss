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
