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


## Batches 5–20 — Pine composition and structural frontier

- v023/v024 introduced transactional rendering and passed tests 1–6, but exposed the test-7 timing hazard.
- v045 reproduced Pineapple v072 exactly and established the all-pass `90.254291` control. v046 established a deterministic 4% giant-tier exit at `90.054298`.
- v047/v048 proved that the deterministic giant exit eliminates test-7 failures; both failed only test 4 (`PPPFPPP`).
- v051/v052 blended future collapsibility into the global heap key. This reordered too much perceptual work and fell to `PFPFFPP`; global rank blending is rejected.
- v053/v054 used future collapsibility only for edge-local endpoint/placement selection. Both remained all-pass at `90.054298`, proving the mechanism safe but target-limited.
- v055/v056 composed edge-local selection with the 2.8% deterministic giant target. Both passed all seven at `90.254291`; test 7 is no longer the active failure source.
- v057/v058 replaced greedy scheduling with neighbor-disjoint versus endpoint-disjoint collapse rounds. Official results were `PPFFPPP`/64.368120 and `PFFFFPP`/32.458893. Neighbor-disjointness is essential, but round scheduling still loses too much SSIM on tests 3–4.
- v059/v060 persisted perceptual quadrics through memoryless rebuilds. They failed tests 3–4; generic history also failed test 5. Persistent QEM energy is not a substitute for direct image loss.
- v061/v062 restored the transactional renderer with conservative tier-3 handling. Both passed all seven at `90.083522`.
- v063/v064 composed that all-pass renderer architecture with the proven deterministic 2.8% giant target. Both scored **90.283515 (`PPPPPPP`)**, the current Tranberry champion. v064's deeper no-SSIM-drop local search was officially inert.
- v065/v066 extend transactional rendering to tier 3, with v066 comparing directly to the original mesh. Batch 20 is in flight.

Current belief: improvements require direct original-image-aware search that changes accepted official output. Durian transfer established v072 at 90.433026 PPPPPPP; preserve its full code layout because exact v083 replay failed test 7 while v097 replay passed./ Global QEM reordering, conflict rounds, and persistent quadrics are rejected. Test 7 is protected by a deterministic early exit and must remain unchanged.


## Durian transfer — v071-v072

Durian's v083/v097 work was replayed as Tranberry v071/v072. Both measured 95.365420 locally with zero topology defects and 98% compression on the 1.1M stress mesh. Officially, however, exact v083 replay v071 failed test 7 (`PPPPPPF`, 74.099686), while v097 replay v072 passed all seven at **90.433026**. v072 is the current Tranberry champion.

This is strong timing/layout evidence: medium-tier-only original-reference code can change giant-tier behavior even when its pass is never invoked there. Therefore future Tranberry work must retain v072's complete translation-unit layout and deterministic giant path, not merely copy its numeric keep ratios. Durian v099/v100 confirm threshold/cap expansion is score-inert; v101's large-tier view-weight architecture remains pending.

## Structural batches 24–29 — weighting, transactions, samples, planar deletion

- v073/v074 made raster importance persistent or adaptive in the main QEM and regressed broadly (`PPPFPFP` / `PPPFPFF`). Persistent view energy strands collapsibility and can disturb test 7.
- v075/v076 used visibility-aware feasibility and priority; neither protected test 6, and the broader variant also failed test 7.
- v079/v080 wrapped the full tier-5 continuation against an absolute original-image floor; both failed test 6. v081/v082 changed this to relative/minimax damage guards and restored `PPPPPPP`, but scored 90.421048/90.420959, slightly below v072.
- v083/v084 carried original surface sample moments through collapses. Despite plausible local metrics, both failed tests 3–7 (`PPFFFFF`); sample-centroid preservation is badly misaligned with the judge render.
- v085/v086 added exact coplanar star deletion with sequential and independent scheduling. Both were locally fingerprint-identical to v072 and officially `90.433026 PPPPPPP`; strict exact-planar deletion is safe but inert on hidden meshes.

Current champion remains v072 at 90.433026. The next experiments replace geometry-only placement with signed-volume and perspective screen-space quadrics while retaining the v072 large-tier runtime path.

## Batch 30 and local v089-v098 — volume rejected, screen Jacobians promising

- v087 weak medium-only signed-volume placement was officially score-neutral/all-pass at 90.433026. v088 extended the stronger constraint across screen tiers and failed test 4 (`PPPFPPP`, 76.182865). Volume preservation is rejected for sensitive medium geometry.
- v089/v090 replace isotropic silhouette anchors with perspective view-ray tangent quadrics while leaving the large/T7 runtime branch untouched. v089 retains 95.365420 local compression with changed medium fingerprints and tiny SSIM/Hausdorff gains; Batch 31 is in flight.
- v091/v092 use the exact perspective projection Jacobian rather than an angular tangent approximation. Local compression remains 95.365420; Hausdorff use improves and giant stress output stays fingerprint-identical.
- v095/v096 carry the camera Jacobian through collapse clusters and memoryless rebuilds. v095 raises the hardest proxy from about 0.9920 to 0.9922 SSIM at identical compression.
- v097/v098 combine persistent screen memory with deeper isolated medium continuations. v097 reaches about 0.9923 on the hard proxy at the same local vertex count; hidden tier-3 evidence remains decisive.

## Local v099-v110 — render-channel decomposition and cost separation

- v099/v100 add analytic unit-normal Jacobians. Both underperform pixel-only screen Jacobians locally, indicating face-plane QEM already captures most normal constraints; this branch is rejected.
- v101 adds a camera-depth derivative to the pixel Jacobian. It raises the hard proxy to 0.9927 SSIM but stops earlier (95.315860 mean compression) because perceptual error feeds the collapse cap. v102 persistence restores compression but loses most fidelity gain.
- v103/v104 separate perceptual placement from geometry scheduling and improve local mean SSIM/compression, but accidentally apply the new geometry scheduler to T5/T6/T7; the T7 fingerprint changes and runtime approaches 18s, so these forms are explicitly not submission candidates.
- v105/v106 correct the scope to screen tiers 2–3. v105 is the best local Pareto point: 95.365420 compression, 0.890177 mean SSIM, hard proxy 0.9924, and exact v072 T7 fingerprint/runtime path. Persistence is slightly worse.
- v107 deeper tier-2 staging enters a safer but less collapsible basin (hard proxy 0.9932 but only 79.8786% reduction there). v108 isolates the hidden tier-3 schedule.
- v109 render-ranked queue selection reproduces the high-fidelity/lower-compression behavior of v101. Perceptual placement with geometry ordering (v105) is better than perceptual ordering.
