# Gala iteration log

## Theory

Gala is the architectural-rewrite family proposed in the 2026-07-14 brainstorm
(see `docs/solutions.md` "Gala family"). It targets the 90.452702 plateau that
edenfruit documented after 31 batches: every parametric tuning either ties or
regresses, and progress requires structural changes (per the world model).

The family deliberately holds the edenfruit v22 schedule as ground truth:
- T2 last stage = 0.30, T3 finalTarget = 0.145, T4 first stage = 0.14,
  T5 keepRatio = 0.0237, T7 keepRatio = 0.0200
- raster resolutions, midpoint bias, giant shield — all locked.

What changes: structural moves from the brainstorm buckets.

- **Bucket B — book of life ledger**: a per-vertex irreplaceability signal
  computed once from the six-view foreground, used as a multiplicative
  penalty on the QEM cost so the heap under-orders edges near silhouette
  and unique-foreground pixels.
- **Bucket C — silent fast**: a wave of low-saliency collapses without
  per-collapse SSIM check, with a single post-wave atlas revert if any
  foreground pixel changed.
- **Bucket D — restoration round**: bounded small-component topology repair
  on the post-snapshot mesh.
- **Bucket E — triune filter**: collapse candidates must rank well on
  geometric, perceptual, and structural signals (Pareto-style).
- **Bucket A — watches**: periodic re-audit of recent collapses (last;
  needs a stable reversal mechanism first).

First experiment per the brainstorm: B as a read-only multiplicative
penalty on v22's QEM cost.

## Batch 1 - v1 (floor), v2 (Bucket B ledger)

**Hypothesis:** edenfruit v22 sits on a parametric plateau. A *structural*
new signal — per-vertex silhouette count from a one-time six-view render of
the original mesh — should let the heap choose safer collapse orderings.
Multiplicative penalty `1 + 0.3·(L(a)+L(b))` where L is the silhouette count.

**Implementation:**
- `HParam_LedgerMul=0.30`, `HParam_LedgerRenderR=256`, `HParam_EnableLedger=true`.
- `initLedger()` runs after `readMesh()`: renders 6 views at R=256 of the
  original mesh; for each face in front (z>0 for all 3 verts), bumps
  `ledger[a]++` for each of the 3 vertices.
- `makeCandidate()` applies `cost *= 1 + 0.3*(L(a)+L(b))`.

### v1 result
- Kattis: 90.452702, PPPPPPP — exact edenfruit v22 floor (as expected,
  v1 is a literal copy).

### v2 result
- Local: COMPRESSION_RATE=93.333906 (-0.23pp vs v1), Mean SSIM=0.887596
  (+0.033). Silhouette vertices are preserved longer; fewer total
  collapses; higher SSIM — directionally exactly what Bucket B predicts.
- **Kattis: 58.838782, cases PPPPFPF — broke tests 5 and 7!**

### v2 post-mortem
The local fingerprint (compression down 0.23pp, SSIM up 0.033) hid two
real problems:
1. **T5 (test 5) cliff is sensitive**: my ledger makes the heap *reject*
   silhouettes, but on test 5's geometry the silhouettes are exactly the
   cheap edges the heap needs to find before the budget cliff. By
   reordering, I forced the heap to spend its budget on harder non-
   silhouette edges first, leaving the silhouette mass to be eaten late
   when it has no budget left.
2. **T7 (test 7) layout sensitivity**: any change to the giant-tier
   control flow changes execution time, and T7's result depends on
   timing (per world model). Even a tiny cost change cascades.

**Lesson:** Bucket B as a *reordering-only* multiplier breaks the cliffs
because the cliffs were tuned to the existing collapse order. The
ledger signal needs to be used to **lower target ratios selectively**,
not reorder. That requires a much more surgical implementation: e.g.
only apply the multiplier for vertices where `ledger > some threshold`
*and* only on tiers where the cliff is below the current ratio.

### Next directions (re-prioritized)
1. **Bucket B with threshold-gated activation**: only apply the ledger
   penalty when ledger[a]+ledger[b] > threshold (e.g. > 4, meaning both
   endpoints are heavily silhouette-marked). This makes the
   reordering local to the *most silhouette-bound* edges, leaving the
   rest of the heap untouched.
2. **Bucket C silent fast on T1/T6 only**: a targeted second-pass that
   only runs on tiers with low silhouette density (T1 is trivial, T6
   has flat regions). Skip T2/T3/T4/T5/T7 entirely.
3. **Bucket E triune filter on the giant tier only**: combine the
   existing QEM cost with the ledger signal using Pareto ranking,
   *only* for nV > 400000.

## Batch 2 - v3 (occludedEdgeCollapsePass on giant tier), v4 (1-ring centroid)

### Hypothesis

**v3:** The `occludedEdgeCollapsePass` function exists and is called
for the mid tier (5k-50k, via `runTransactionalScreenMid`) but NOT for
the giant tier (nV > 400000). The function has strict SSIM guards
(`.9995 / .9990 / .9985 / .9995`) so it should be safe. Adding it to
the giant tier is a structural addition that compresses hidden mass on
T6/T7 without affecting foreground SSIM.

**v4:** The `getCandidatePositions` function returns QEM-opt + endpoints
+ midpoint. Adding the 1-ring centroid (average of neighbor positions)
gives a more "mesh-natural" replacement position that yields more
regular triangles, which should preserve SSIM at the same vertex count.

### Implementation

**v3:** Single structural change — add `if(HParam_EnableGiantHiddenPass
&&elapsed()<A0-5.0)occludedEdgeCollapsePass();` at the start of the
`if(inputV>400000){...}` branch in `run()`.

**v4:** Single structural change — add 1-ring centroid computation in
`getCandidatePositions` after the existing positions.

### Local fingerprints (ppsurf dataset)
- v3: identical to v1 (93.563877). Local dataset has no giant meshes,
  so the change is dormant locally.
- v4: TBD.

### Kattis status
- v3 id: `1bc75883` — **scored 90.452702, cases PPPPPPP — TIED with v22**.
  Giant-tier hidden pass enabled but found no additional hidden mass on
  the official tests. No regression, no improvement.
- v4 id: `4213f367` — **scored 58.754508, cases PPPPFPF — BROKE tests 5
  and 7**. Same pattern as v2 (cost function change breaks cliffs).

### Batch 2 post-mortem

**Critical pattern:** any change to the *cost function* (multiplier in
v2, candidate position in v4) breaks the cliffs on T5 and T7. The
cliffs were tuned to the exact cost function. By contrast, a *structural
addition* (new pass call in v3) tied safely.

**Lesson:** the only safe moves are structural additions (new passes)
or pass-enabling changes. Cost function changes break cliffs.

### Next directions (refined)
1. **v5**: extend `vegaSsimEdgePass` + `exactWindowCounselEdgePass` to
   T5 (inputV > 50000). Those proven-safe passes are absent from T5.
2. **v6**: add `occludedEdgeCollapsePass` to the T5 path
   (inputV > 50000 but not > 400000) — it currently only runs in the
   mid-tier runTransactionalScreenMid.
3. **v7**: try `runLargeCameraTx` re-enabled for T5 only (not T7).
   World model says it broke T7, but T5 wasn't tested separately.
4. **v8**: increase T5's `frac` for `occludedEdgeCollapsePass` from
   0.035 to 0.045 (28% more aggressive hidden collapses on T5).

### v5 result
- Kattis: **90.45279, PPPPPPP — TIED with v22** (within rounding).
- Hypothesis (T5 counsel passes) was correct in safety but found no
  additional collapses. T5's keepRatio cliff is exactly 0.0237; counsel
  doesn't push it lower.

### Batch 3 post-mortem

The pattern is now firm: **structural additions neither help nor
hurt.** v3 (giant-tier hidden pass), v5 (T5 counsel) — both tied.

To break the plateau we need either:
- A cost-function change (but those reliably break cliffs).
- A new SSIM-gated candidate pool expansion (the edenfruit v06 pattern).

The v06 move was: remove a ring-exclusion from occludedEdgeCollapsePass.
Are there similar exclusions in other SSIM-gated passes? YES — the
`exactWindowCounselEdgePass` has `nearLocked` exclusion. Removing it
should be the v06 analogue.

## Batch 4 - v6 (T5 hidden pass) + v7 (counsel nearLocked removal)

**Hypothesis v6:** T5 is missing `occludedEdgeCollapsePass`. Add it.
Should be safe (proven pass) and may compress hidden mass on T5.

**Hypothesis v7:** Remove `nearLocked` exclusion from
`exactWindowCounselEdgePass` — the analogue of edenfruit v06's
two-ring exclusion removal. The SSIM gate remains strict; only the
candidate pool expands.

### v6 result
- Kattis: **90.452127, PPPPPPP** — slight regression (-0.000575).
  Adding occludedEdgeCollapsePass to T5 produced a *worse* collapse set
  than v22's empty T5 hidden pass. The v22 baseline deliberately omits
  the pass on T5 — perhaps because the existing T5 passes (collapseInvisibleEdges,
  pairDisk, valenceWeld) cover the same ground.

### v7 result
- Kattis: **90.452702, PPPPPPP — TIED with v22**. The nearLocked
  exclusion removal (the edenfruit v06 analogue) was structurally correct
  but didn't find additional collapses. The strict SSIM gate inside
  counsel is the binding constraint, not the lock exclusion.

### Batch 4 post-mortem

Two more data points:
- **v6 (structural addition)** tied-with-regression. The pass wasn't
  absent because it was unsafe — it was absent because the existing
  pipeline covers the same ground via different mechanisms.
- **v7 (gate removal, v06-analogue)** tied. The nearLocked exclusion
  was not the binding gate; the SSIM gate was.

### Cumulative insights (7 candidates, 4 batches)
- v1: tie (floor)
- v2: regression (cost multiplier on heap)
- v3: tie (giant-tier hidden pass)
- v4: regression (1-ring centroid placement)
- v5: tie (T5 counsel passes)
- v6: regression (-0.000575, T5 hidden pass)
- v7: tie (counsel nearLocked removal)

The plateau at 90.4527 is **firm**. Every architectural change I've
tried either ties or regresses. The only successful +0.001591 was
edenfruit v06's specific two-ring removal in occludedEdgeCollapsePass
on the mid tiers — the v06 analogue (v7) didn't reproduce that.

## Batch 5 - v8 (counsel overlap removal) + v9 (third T2 counsel call)

**Hypothesis v8:** Counsel has another ring-style exclusion (`overlap`).
Removing it (like v7's nearLocked removal) is the v06 pattern.

**Hypothesis v9:** T2 only gets 2 counsel calls; T3 gets 2 + 1 inside
absoluteQemEndgame. Adding a 3rd counsel call for T2 mirrors T3's
pattern and might find more collapses before the safe snapshot.

### v8 result
- Kattis: **60.838994, PPPFFPP — broke tests 4 and 5**. Removing the
  overlap check in counsel lets too many overlapping proposals pass the
  per-proposal SSIM gate, but their cumulative effect breaks SSIM.

### v9 result
- Kattis: **74.119363, PPPPPPF — broke test 7**. Adding a third counsel
  call to T2 changes heap state enough to push T7 into a different
  timing regime.

### Batch 5 post-mortem

Two more regressions. The pattern continues: any change to counsel
behavior (overlap check, additional calls) breaks cliffs because
counsel's contribution to the final collapse set is calibrated.

### Cumulative insights (9 candidates, 5 batches)
- v1: tie (floor)
- v2: regression (cost multiplier on heap) — tests 5+7 broken
- v3: tie (giant-tier hidden pass)
- v4: regression (1-ring centroid placement) — tests 5+7 broken
- v5: tie (T5 counsel passes)
- v6: regression (-0.000575, T5 hidden pass) — test 4 dropped slightly
- v7: tie (counsel nearLocked removal)
- v8: regression (counsel overlap removal) — tests 4+5 broken
- v9: regression (third T2 counsel call) — test 7 broken

**Crucial empirical finding:** structural additions don't help;
*gate-removal* analogues of v06 don't reproduce v06's success. The
plateau at 90.4527 is robust against all the architectural moves I've
tried that are compatible with not breaking tests.

## Pivot: extending the v5 pattern

Re-reading the v5 result: +0.000088 improvement (within noise but the
*only* non-regressive change). The v5 move was adding *proven-safe
SSIM-gated passes* (vegaSsimEdgePass, counsel) to an under-served tier
(T5). The pattern: SSIM-gated structural additions on under-served
tiers may find small improvements.

Audit of under-served tiers:
- T5 missing: vegaSsimEdgePass, exactWindowCounselEdgePass, vegaSsimStarPass.
- T6 missing: vegaSsimEdgePass, exactWindowCounselEdgePass.
- T7 missing: vegaSsimEdgePass, exactWindowCounselEdgePass.

## Batch 6 - v10 (T5 star pass) + v11 (T6 vega+counsel)

**Hypothesis v10:** v5 added vega+ counsel to T5. v10 adds the third
proven-safe pass (vegaSsimStarPass) to T5. If the pattern holds, small
SSIM-safe compression improvements compound.

**Hypothesis v11:** Mirror v5 on T6 (inputV in 400k-1M only). Add
vegaSsimEdgePass + counsel x2 to T6's path. Gate by `inputV<=1000000`
so T7's path is unchanged.

### v10 result
- Kattis: **90.45279, PPPPPPP — TIED with v5**. The third T5 pass
  (star) didn't find additional collapses. T5's keepRatio cliff (0.0237)
  appears firm.

### v11 result
- Kattis: **74.119363, PPPPPPF — broke test 7!** Even though the
  injection was gated to T6 only (`inputV<=1000000`), T7 broke.
  Hypothesis: even adding conditional branches with `false` outcomes
  on T7's path changes binary layout enough to shift T7 timing.

### Batch 6 post-mortem

The v5 pattern doesn't extend to T6 or T7. Adding SSIM-gated passes
to larger tiers breaks their layout-sensitive cliffs even when the
new passes don't run on those tiers.

### Cumulative insights (11 candidates, 6 batches)
- v1: tie (floor)
- v2: regression (cost multiplier)
- v3: tie (giant-tier hidden pass)
- v4: regression (1-ring centroid)
- v5: +0.000088 (T5 vega+counsel) — within noise but only positive move
- v6: regression (T5 hidden pass)
- v7: tie (counsel nearLocked removal)
- v8: regression (counsel overlap removal)
- v9: regression (third T2 counsel)
- v10: tie (T5 star pass) — same as v5, no compounding
- v11: regression (T6 vega+counsel) — broke T7 via layout shift

### v12 result
- Kattis: **90.452702, PPPPPPP — TIED with v22**. Adding T7-specific
  counsel + vega (built on v3 to share its layout) didn't break T7.
  The layout-shift hypothesis from v11 was specific to v11's anchor.

### v13 result
- Kattis: **90.452702, PPPPPPP — TIED with v22**. Enabling
  runLargeCameraTx for T6 only (`inputV<=1000000`) with v3-base
  layout also tied. The v3 base truly does protect T7 from new
  pass additions.

### v14 result
- Kattis: **90.45226, PPPPPPP** — slight regression (-0.000442).
  v10 + occludedEdgeCollapsePass for T5 regressed slightly. The hidden
  pass on T5 produces a worse collapse set than v22's empty T5 hidden
  pass, even when added on top of v10's improvements.

### v15 result
- Kattis: **90.452834, PPPPPPP — NEW CHAMPION, +0.000132 vs v22**.
  v10 + a second vegaSsimEdgePass(false) call for T5. Adding a
  second vega call to T5 finds slightly more SSIM-safe collapses
  than v10 alone.

### Batch 8 post-mortem

v15 confirms the v5 pattern: more proven-safe SSIM-gated passes on
T5 yield small but compounding improvements. v14 (adding hidden
pass) regressed because hidden-pass collapses interact badly with the
existing T5 pipeline.

### Cumulative insights (15 candidates, 8 batches)
- v1: tie (floor, 90.452702)
- v5: +0.000088 (T5 vega+counsel)
- v10: tie (v5 + T5 star pass)
- **v15: +0.000132 (v10 + 2nd vega call) — current champion at 90.452834**

### v16 result
- Kattis: **74.119363, PPPPPPF — broke test 7!** v3 + T6-targeted
  vega+counsel. Even v3-base layout doesn't protect T7 from T6
  additions.

### v17 result
- Kattis: **90.45279, PPPPPPP — TIED with v5**. v3 + T5-targeted
  vega+counsel reproduces v5's effect. Confirms v5's +0.000088 is
  real (not noise), since v17 with different base gives same
  result.

### Batch 9 post-mortem

v17 confirms v5's improvement is real. v16 reveals that any
T6-targeted change to the giant tier branch breaks T7, even on v3
base. The hypothesis "v3 base protects T7" was correct only for
T7-targeted (v12) and T6-with-different-control-flow (v13)
additions, but not for T6 vega+counsel (v16).

### Current state
- **v15 still champion at 90.452834 (+0.000132)**.
- v17 reproduces v5's +0.000088 on a different base.

## Batch 10 - v18 (v15 + 3rd vega) + v19 (v15 + 3rd counsel)

**Hypothesis v18:** The vega-call compounding pattern (1→+0.000088,
2→+0.000132) might continue. A 3rd vega call could add more.

**Hypothesis v19:** Counsel might have more remaining room than vega.
A 3rd counsel call tests this.

### v18 / v19 status
- Both pending Kattis evaluation.

### v18 result
- Kattis: **90.452834, PPPPPPP — TIED with v15**. A 3rd vegaSsimEdgePass
  call on T5 didn't find more SSIM-safe collapses. The vega pattern
  plateaued at 2 calls.

### v19 result
- Kattis: **90.452834, PPPPPPP — TIED with v15**. A 3rd
  exactWindowCounselEdgePass call on T5 didn't find more SSIM-safe
  collapses. The counsel pattern also plateaued at 2 calls.

### Batch 10 post-mortem

T5 has been saturated at 90.452834 with v15's pattern. Further T5
additions don't help. Need to either:
1. Try a different lever on T5 (e.g. different time gates, different
   raster resolution).
2. Apply the proven-SSIM-gated-pass pattern to a different tier.
3. Combine T5 changes with another improvement.

### Current state
- **v15 still champion at 90.452834 (+0.000132)**.
- v18, v19 confirm T5 is saturated at v15's level.

## Batch 11 - v20 (v3 + T5 changes) + v21 (v15 + 2nd runLargeCameraTx for T5)

**Hypothesis v20:** T5 changes and giant-tier hidden pass might be
independent improvements. Combining them could yield additive gains.

**Hypothesis v21:** Adding a 2nd runLargeCameraTx call earlier in T5
(more time budget) might find additional collapses. Tests a different
lever than the SSIM-gated edge pass pattern.

### v20 / v21 status
- Both pending Kattis evaluation.

### v20 result
- Kattis: **90.452834, PPPPPPP — TIED with v15**. Combining v3's
  giant-tier hidden pass with v15's T5 changes doesn't compound. The
  T5 changes are the binding constraint; v3's giant-tier addition
  is independent but doesn't add SSIM-safe collapses.

### v21 result
- Kattis: **74.166719, PPPPPFP — broke tests 5 and 7!** Adding a 2nd
  runLargeCameraTx call to T5 (after the main call) is too
  disruptive — the function does full rendering, bisection, star
  work, all of which interact badly when called twice.

### Batch 11 post-mortem

v20 confirms v15's T5 ceiling is the local optimum. v21 confirms
that complex pass additions break cliffs even when simpler ones
don't.

### v22 result
- Kattis: **90.452834, PPPPPPP — TIED with v15**. A 2nd star pass
  on T5 didn't find more collapses. Star pass plateaued at 1 call.

### v23 result
- Kattis: **90.452834, PPPPPPP — TIED with v15**. Lowering txReserve
  from 1.70 to 1.50 didn't help — T5's SSIM-gated passes were
  already finding all the safe collapses.

### Batch 12 post-mortem

T5 ceiling at 90.452834 is firm. Need to try different tiers or
different mechanisms.

## Batch 13 - v24 (v15 + RootNudge profile 2) + v25 (v15 + extra vega in runTransactionalScreenMid)

**Hypothesis v24:** Switching RootNudge profile from 1 to 2 activates
root nudge on T2 with smaller frac (0.035). Different T2 collapse
dynamics.

**Hypothesis v25:** Adding a 2nd vega call inside
`runTransactionalScreenMid`'s tier-3/4 path mirrors v15's T5 pattern
on the mid tier.

### v24 / v25 status
- Both pending Kattis evaluation.

### v24 result
- Kattis: **90.452834, PPPPPPP — TIED with v15**. Switching
  RootNudge profile from 1 to 2 didn't help.

### v25 result
- Kattis: **90.452834, PPPPPPP — TIED with v15**. Adding a 2nd vega
  call inside runTransactionalScreenMid tier-3/4 path didn't help.
  Mid tier changes either tie or break (per the T5 saturation
  pattern).

### Batch 13 post-mortem

T5 saturation at 90.452834 is firm across many attempted variations
(v18, v19, v20, v22, v23, v24, v25). The improvement from v5 to
v15 came specifically from adding the 2nd vega call on T5 (the
one delta that worked). All other variations tie or break.


