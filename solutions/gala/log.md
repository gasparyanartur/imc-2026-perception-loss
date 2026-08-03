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

## Batch 14 - v26 (shift T5 passes later) + v27 (larger T2 counsel budget)

**Hypothesis v26:** Moving v15s T5 pass sequence later without changing its
contents tests whether unused wall-clock ordering limits the extra collapses.

**Hypothesis v27:** Raising `HParam_ExactWindowBudgetTier2` from 0.00024 to
0.00028 gives the original-reference counsel more cumulative perceptual budget
without changing the locked tier targets or giant path.

### Official results

- v26: **90.452834, PPPPPPP — TIED with v15**. Pass timing was not binding.
- v27: **90.452834, PPPPPPP — TIED with v15**. The larger tier-2 counsel
  budget was count-inert.

### Batch 14 post-mortem

Neither wall-clock placement nor tier-2 counsel budget explains the plateau.
The official collapse set remains pinned by the existing target and validation
cliffs. Further Gala work should stop extending the saturated pass schedule and
move to a bounded graph-basin change or rollback mechanism while preserving
v15s complete giant-tier layout.

## Batch 15 - v30 (late broad amortized tail) + v31 (early amortized tail)

**Hypothesis:** The dormant neighbor-disjoint tail batch is a bounded form of
Gala Bucket C: it scans a broad edge pool, selects a conflict-free wave, and
applies only collapses passing the local Vega guard. Activating it across every
nontrivial tier may escape greedy endgame basins without changing the locked
target schedule or replacement-position portfolio.

- v30 activates the tail on all tiers at 11.8 seconds and doubles the scan from
  131072 to 262144 edges.
- v31 activates the tail on all tiers at 11.0 seconds with the standard 131072
  edge scan, testing whether an earlier independent wave leaves more time for
  validated follow-up work.

Both candidates are built from the v15 champion and will receive exactly one
sequential local diagnostic before the complete immutable batch is submitted.

### Local diagnostics

- v30: `COMPRESSION_RATE=93.838936`, mean SSIM 0.854743, mean Hausdorff
  usage 40.348000%, and zero nonmanifold, boundary, or degenerate counts on
  all 10 canonical scenarios. Output: `outputs/native-20260714-092403289571900-metrics-compr-93.838936.txt`.
- v31: `COMPRESSION_RATE=94.079749`, mean SSIM 0.854589, mean Hausdorff
  usage 40.378720%, and zero nonmanifold, boundary, or degenerate counts on
  all 10 canonical scenarios. Output: `outputs/native-20260714-092536993384375-metrics-compr-94.079749.txt`.

The earlier v31 wave removes more vertices locally; both change the two
long-running tier-2 proxy fingerprints and preserve topology. Local SSIM is
diagnostic only, so both immutable candidates proceed to Kattis.

## Batch 16 preparation - v32 (strict tail geometry) + v33 (strict 80% envelope tail)

**Hypothesis:** Batch 15s amortized tail bypasses the strict continuation face
geometry audit. If official failures arise from that omission rather than from
the conflict-free schedule itself, adding the common strict face-orientation,
area, and duplicate-face gate should preserve the extra collapses more safely.
A second profile additionally limits tail cluster radius to 80% of the allowed
Hausdorff envelope, reserving geometric margin for later collapses. Both changes
apply to every tier through the common tail path.

- v32: v31 plus `strictCollapseGeometrySafe(best)` before every tail commit.
- v33: v32 plus `best.mergedRadius <= 0.80*hausd` in the tail only.

These candidates are prepared and evaluated while Batch 15 is pending, but are
held from upload until Batch 15 is terminal and its results are written here.

### Held local diagnostics

- v32: `COMPRESSION_RATE=93.804690`, mean SSIM 0.854754, mean Hausdorff
  usage 40.348000%, topology counts all zero. Output:
  `outputs/native-20260714-093706634752257-metrics-compr-93.804690.txt`.
- v33: `COMPRESSION_RATE=93.838936`, mean SSIM 0.854743, mean Hausdorff
  usage 40.348000%, topology counts all zero. Output:
  `outputs/native-20260714-093848441099422-metrics-compr-93.838936.txt`.

v32s strict face gate rejects some locally accepted tail collapses. v33 matches
v30s local fingerprints, so the 80% envelope cap is non-binding on the default
fixtures. Official Batch 15 evidence will determine whether this held batch is
still the right next hypothesis.

### Batch 15 official results and post-mortem

- v30: **74.108159, PPPPPPF** — failed test 7.
- v31: **74.108159, PPPPPPF** — failed test 7.

The amortized tail activation preserves tests 1-6 but is incompatible with the
giant-tier path. Doubling the scan breadth and moving the start from 11.8 to
11.0 seconds are officially identical, so either the tail is dormant on tests
1-6 and changes only T7, or both schedules converge to the same passing-case
counts. The common structural fact is decisive: making the dormant tail branch
reachable changes T7s output or timing enough to cross its SSIM cliff.

The held v32/v33 pair remains a meaningful diagnostic because it isolates a
missing safety gate in the tail commits. If either restores T7, the failure was
collapse-set geometry; if both repeat `PPPPPPF`, the reachable branch/layout
itself is the culprit and the all-tier tail direction is retired.

## Batch 17 preparation - v34/v35 (bounded all-tier edge-flip preconditioning)

**Hypothesis:** Gala v15s collapse schedule may be trapped in a connectivity
basin. Pomegranates v003/v004 proved that ultra-coplanar, tiny-budget flips can
remain all-pass. Transplanting those flip rounds into the exact v15 champion
keeps Galas targets, counsel, double-Vega T5 path, and giant shield while
changing connectivity before QEM on every tier.

- v34 uses Pomegranate profile 3: at most four ultra-coplanar flips on T2 and
  conservative tier-scaled valence-gain flips elsewhere.
- v35 uses profile 4: one near-exact planar T2 flip and broader
  quality/diagonal-shortening flips on other tiers.

The files are local-only while Batch 16 is pending and will each receive one
sequential diagnostic.

### Held local diagnostics

- v34: `COMPRESSION_RATE=93.549558`, mean SSIM 0.854870, mean Hausdorff
  usage 40.421800%, topology counts all zero. Output:
  `outputs/native-20260714-095352304151987-metrics-compr-93.549558.txt`.
- v35: `COMPRESSION_RATE=93.547399`, mean SSIM 0.855426, mean Hausdorff
  usage 40.362360%, topology counts all zero. Output:
  `outputs/native-20260714-095523236994881-metrics-compr-93.547399.txt`.

Both profiles change almost every default fingerprint while preserving the
locked Gala schedule and topology. v35 trades roughly 0.0022 points of local
compression for +0.00056 mean SSIM versus v34, consistent with a safer but
different connectivity basin. The immutable pair remains held behind Batch 16.

## Batch 18 preparation - v36/v37 (post-snapshot restoration flips)

**Hypothesis:** Pre-QEM flips may be erased by greedy collapse. Moving the same
bounded, manifold-safe profiles to the safe post-QEM snapshot changes the
connectivity basin immediately before Galas existing Vega, counsel, star, and
transactional endgames. This is Gala Bucket D: graph restoration without moving
vertices or lowering any locked target.

- v36 moves profile 3 from preprocessing to the post-QEM snapshot on every tier.
- v37 moves profile 4 in the same way, retaining its one-flip ultra-planar T2
  envelope and broader quality/diagonal repair elsewhere.

The pair is local-only behind the currently pending Kattis batch.

### Local diagnostics and implementation post-mortem

- v36: `COMPRESSION_RATE=45.056776`, mean SSIM 0.858424, topology counts
  zero. Several scenarios emitted 0% compression. Output:
  `outputs/native-20260714-095852958534965-metrics-compr-45.056776.txt`.
- v37: `COMPRESSION_RATE=54.618681`, mean SSIM 0.854833, topology counts
  zero, with the same zero-compression failure mode. Output:
  `outputs/native-20260714-100028985051851-metrics-compr-54.618681.txt`.

Root cause: `preconditionEdgeFlips()` calls `buildConnectivity()` after a
successful flip, and `buildConnectivity()` resets `vdead` over the un-compacted
vertex array. When invoked after QEM, this resurrects removed vertex slots. The
planned files remain immutable evidence. New versions must compact raw QEM state
first (or reset already-compacted transactional state), then rebuild connectivity
and run restoration flips.

### Batch 18 official results and post-mortem

- v46: **90.4395, PPPPPPP** — all-pass but 0.013334 below v15.
- v47: **74.131433, PPPPPPF** — failed test 7.

The standard profile-4 transplant is structurally valid but loses score, while
reducing its scan and acceptance budgets destabilizes the giant case even though
that version was slightly more compressive locally. Flip work and translation-
unit layout remain entangled with T7s timing cliff. Preserve v46s standard
budgets as the only safe profile-4 anchor; v15 remains champion. Batch 19 tests
whether changing only the ranking within that standard safe envelope recovers
score, with v49 retained as a deliberate parity probe of the unsafe reduced
envelope.

## Batch 19 preparation - v38/v39 (corrected compacted restoration flips)

**Hypothesis:** Post-snapshot graph restoration must operate on a compacted
mesh. v38/v39 preserve the profile-3/profile-4 distinction but normalize each
solver path first: raw QEM states are compacted, already-transactional states
have liveness arrays reset, connectivity is rebuilt, then bounded flips run.
This prevents dead-slot resurrection while retaining Bucket Ds intended
post-snapshot basin change on every tier.

### Local diagnostics and second state post-mortem

- v38: `COMPRESSION_RATE=15.666753`; only two scenarios produced metrics,
  eight exited after roughly 1.1 seconds. Output:
  `outputs/native-20260714-100305644296460-metrics-compr-15.666753.txt`.
- v39: identical aggregate and failure pattern. Output:
  `outputs/native-20260714-100419941857984-metrics-compr-15.666753.txt`.

Root cause: `buildConnectivity()` uses `vneigh.resize(nV)`, which preserves
stale `SmallSet` contents when rebuilding a compacted mesh. Old neighbor IDs
then survive beside the new adjacency and cause invalid access. The next new
versions replace resize with a clearing assign before the corrected restoration
path. v38/v39 remain immutable.

### Batch 19 official results and post-mortem

- v48: **74.10594, PPPPPPF** — failed test 7.
- v49: **74.166247, PPPPPFP** — failed test 6.

Future-collapsibility ranking is not safe inside either flip breadth. Under the
standard budget it destabilizes T7; under the reduced budget it restores T7 but
loses T6. The exact flip ordering, not merely flip count, controls different
hidden-tier cliffs. Retire this ranking. Batch 20 now partitions the established
v46 quality ranking by T5 versus all other tiers.

## Batch 20 preparation - v40/v41 (fully reset compacted restoration state)

**Hypothesis:** Clearing the adjacency container during connectivity rebuild
completes the state normalization needed by post-snapshot flips. v40/v41 keep
the corrected compact/reset/rebuild sequence and profile-3/profile-4 graph
repair, but replace stale-preserving `vneigh.resize` with a clearing assign.
This should preserve compression and topology while finally measuring Bucket D
rather than state corruption.

### Local diagnostics and retirement

- v40: `COMPRESSION_RATE=15.666753`; eight scenarios still exited before
  metrics. Output:
  `outputs/native-20260714-100623323988725-metrics-compr-15.666753.txt`.
- v41: identical aggregate and failure class. Output:
  `outputs/native-20260714-100744817268985-metrics-compr-15.666753.txt`.

Clearing adjacency does not restore the compacted-state invariants. The Gala
solver couples liveness, raster importance, source snapshots, radii, versions,
and transactional state too tightly for this direct post-snapshot transplant.
Retire further repair attempts in this code path; future Bucket D work should
start from an architecture with a supported compact-and-rebuild boundary.

### Batch 16 official results and post-mortem

- v32: **74.108159, PPPPPPF** — failed test 7.
- v33: **74.108159, PPPPPPF** — failed test 7.

Both exactly repeat v30/v31 despite strict face geometry and an additional 80%
Hausdorff-envelope reserve. Therefore unsafe individual tail collapses are not
the discriminating cause. Making the all-tier tail branch reachable changes the
giant translation-unit/runtime path enough to fail T7. Gala Bucket C via this
existing tail mechanism is retired. The next official batch switches to the
independently all-pass bounded edge-flip basin experiment.

### Batch 17 upload failure

The service rejected both files with HTTP 422 because the literal source exceeds
its 100 KB code limit. No Kattis IDs were created. The algorithmic candidates
remain useful, but the next versions must mechanically collapse whitespace, be
evaluated as the exact uploadable source, and then submitted.

### Batch 20 official results and post-mortem

- v50: **90.439972, PPPPPPP** — T5-only flips cost 0.012862 vs v15.
- v51: **90.452362, PPPPPPP** — non-T5 flips cost 0.000472 vs v15.

The effects are additive to six decimal places: `0.012862 + 0.000472 =
0.013334`, exactly v46s regression. T5 accounts for 96.5% of the profile-4
loss; all remaining tiers jointly account for 3.5%. Profile-4 repair does not
contain a hidden aggregate gain, but Batch 21 tests whether reduced breadth or
future ranking can repair the dominant T5 loss while T7 is fully protected.

## Batch 21 preparation - v42/v43 (uploadable bounded pre-QEM flips)

**Hypothesis:** v42/v43 are the exact v34/v35 algorithms with mechanically
collapsed whitespace so they fit the service limit. v42 retains profile 3; v43
retains profile 4. Because whole-source layout can affect T7, these exact files
receive fresh diagnostics before upload.

### v42/v43 pre-evaluation abandonment

Mechanical whitespace collapse left the files at 104956 and 105078 bytes,
still above the service limit. They were never evaluated or uploaded and are
retired as non-submittable source compositions. The next files use the compact
97 KB Pomegranate implementation as their base.

## Batch 17 retry preparation - v44/v45 (uploadable flip basin + Gala envelope)

**Hypothesis:** The all-pass Pomegranate v003/v004 flip basins may support
Galas more aggressive large and giant schedule. v44/v45 retain their complete
known-safe source architectures and all-tier profile-3/profile-4 flips, while
changing only the large keep ratio 0.024 to 0.0237, giant 0.022 to 0.0200, and
QEM cap 0.0375 to Galas 0.0330. This is a cross-family structural composition
that fits the upload limit.

### Local diagnostics

- v44: `COMPRESSION_RATE=94.615102`, mean SSIM 0.853908, mean Hausdorff
  usage 40.580120%, topology counts all zero. Output:
  `outputs/native-20260714-101214528638490-metrics-compr-94.615102.txt`.
- v45: `COMPRESSION_RATE=94.612943`, mean SSIM 0.854461, mean Hausdorff
  usage 40.520680%, topology counts all zero. Output:
  `outputs/native-20260714-101319450107646-metrics-compr-94.612943.txt`.

Both are behavior-distinct and materially more compressive locally than the
Gala v15 proxy result. v45 again buys about +0.00055 mean SSIM for negligible
compression loss. Both immutable candidates proceed to Kattis.

### Batch 17 retry official results and post-mortem

- v44: **74.092703, PPPPPPF** — profile 3 fails T7.
- v45: **90.425954, PPPPPPP** — profile 4 is all-pass.

The Pomegranate profile distinction survives composition with Galas envelope.
Profile 4s one near-exact planar T2 flip and broader quality-based budgets are
compatible with all seven official cases; profile 3s four-flip T2/conservative
other-tier envelope still destabilizes T7. v45 improves substantially over the
original Pomegranate v004 score but trails Gala v15 by 0.026880. Preserve v15
as champion and use profile 4 only as evidence that bounded connectivity
changes can be safe.

The prepared v36-v41 transplant files also exceed the service source limit and
have local state-corruption failures; they are non-submittable and retired
without upload attempts.

## Batch 18 - v46/v47 (profile-4 flips replacing dormant code in v15)

**Hypothesis:** The safe Pomegranate profile-4 topology basin can be composed
with the exact Gala v15 champion if its code replaces dormant tail machinery
rather than being appended past the service limit. Both candidates delete the
unreachable all-tier tail implementation and duplicate dead giant block, then
run manifold-safe pre-QEM flips on every tier without changing v15 targets,
counsel, Vega passes, or replacement positions.

- v46 uses the established profile-4 scan and acceptance budgets.
- v47 halves or more than halves scan/accept work on every tier, retaining one
  ultra-planar T2 flip but reducing source-layout and basin disturbance.

### Local diagnostics

- v46: `COMPRESSION_RATE=93.547399`, mean SSIM 0.855426, mean Hausdorff
  usage 40.362360%, topology counts all zero. Output:
  `outputs/native-20260714-102449516292404-metrics-compr-93.547399.txt`.
- v47: `COMPRESSION_RATE=93.555642`, mean SSIM 0.854833, mean Hausdorff
  usage 40.335650%, topology counts all zero. Output:
  `outputs/native-20260714-102623190265932-metrics-compr-93.555642.txt`.

The reduced flip budget recovers 0.00824 local compression points but gives up
0.00059 mean SSIM. Both preserve topology and are under the service limit.


## Batch 19 preparation - v48/v49 (future-collapsibility flip ranking)

**Hypothesis:** Profile-4 flips should prefer a new diagonal whose merged
quadric has low midpoint error, because that edge is more likely to become a
cheap later QEM collapse. v48 and v49 are exact descendants of v46 and v47,
respectively, changing only the flip-candidate ranking from valence/triangle
quality to `gain - cheapEdgeCost(c,d) * invDiag2`.

### Local diagnostics

- v48: `COMPRESSION_RATE=93.547399`, mean SSIM 0.855415, mean Hausdorff
  usage 40.362180%, topology counts all zero. Output:
  `outputs/native-20260714-103231840411587-metrics-compr-93.547399.txt`.
- v49: `COMPRESSION_RATE=93.555642`, mean SSIM 0.854830, mean Hausdorff
  usage 40.335650%, topology counts all zero. Output:
  `outputs/native-20260714-103352687334070-metrics-compr-93.555642.txt`.

The new ranking changes output fingerprints and slightly changes image metrics,
but local compression is exactly unchanged from each parent. This is therefore
a clean official-parity probe of whether future-collapsibility ranking changes
the hidden mesh basin; it does not displace the parent pair locally.


## Batch 20 preparation - v50/v51 (profile-4 tier partition)

**Hypothesis:** The standard profile-4 transplant is all-pass but loses
0.013334 official points. Gala historically improved only on T5, so isolate
that tier instead of perturbing every official mesh. v50 runs the exact v46
flip profile only for `50000 < inputV <= 400000`; v51 runs it on every other
tier and skips T5. Their union reconstructs v46s tier coverage.

### Local diagnostics

- v50: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-104443493603904-metrics-compr-93.563877.txt`.
- v51: `COMPRESSION_RATE=93.547399`, mean SSIM 0.855426, mean Hausdorff
  usage 40.362360%, topology counts all zero. Output:
  `outputs/native-20260714-104607953671401-metrics-compr-93.547399.txt`.

The canonical local fixtures are all below the T5 threshold, so v50 correctly
reproduces the no-flip v15 fingerprint while v51 reproduces v46. The official
pair is required to attribute the regression to T5 versus the remaining tiers;
both immutable files remain held while Batch 19 is pending.


## Batch 21 preparation - v52/v53 (T5-only flip ordering and breadth)

**Hypothesis:** If Batch 20 attributes the profile-4 effect to T5, its hidden
mesh may respond to either future-collapsibility ordering or a smaller repair
breadth. Both candidates protect every other tier, including T7. v52 applies
the v48 midpoint-QEM ranking only on T5; v53 applies v47s reduced scan/accept
budget only on T5 with the established profile-4 quality ranking.

### Local diagnostics

- v52: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-105002667502747-metrics-compr-93.563877.txt`.
- v53: identical canonical aggregate and fingerprints. Output:
  `outputs/native-20260714-105128633594923-metrics-compr-93.563877.txt`.

All canonical fixtures are below T5, so both correctly reproduce v15 locally.
They remain immutable and held behind Batch 19 and the already prepared tier-
partition Batch 20.


### Batch 21 official results and post-mortem

- v52: **74.106412, PPPPPPF** — failed test 7 despite T5-only execution.
- v53: **90.440238, PPPPPPP** — all-pass but 0.012596 below v15.

The future-ranking expression is T7-unsafe even when the function returns before
executing on T7, proving dormant source/layout effects. Reducing T5 flip breadth
recovers only 0.000266 versus v50; the regressive basin is insensitive to breadth.
Retire T5 pre-QEM flips and future ranking. The next batch changes mechanism to
an original-render-audited T2 snapshot portfolio.

## Batch 22 preparation - v54/v55 (two-order endgame snapshot portfolio)

**Hypothesis:** T2s five audited `absoluteQemEndgame` trials can explore two
collapse orderings without perturbing the main QEM path. Alternate trials use
a tiny valence-dependent cost factor only after restoration of the safe
snapshot; the existing original-render score remains the acceptance oracle.
v54 penalizes retaining high-valence roots, while v55 rewards them. T3 has one
portfolio iteration and therefore remains in mode zero.

### Local diagnostics

- v54: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-105611614915421-metrics-compr-93.563877.txt`.
- v55: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-105735116426139-metrics-compr-93.563877.txt`.

Neither alternate ordering beats the retained safe state on canonical fixtures,
so both emit exact v15 outputs locally. They remain meaningful hidden-mesh beam
probes and are held behind the earlier immutable batches.


### Batch 22 official results and post-mortem

- v54: **90.452834, PPPPPPP** — exactly ties v15.
- v55: **74.119495, PPPPPPF** — failed test 7.

The audited low-valence alternate is safely rejected or count-equivalent; the
high-valence expression crosses T7 through dormant layout despite operating only
in odd T2 trials. Snapshot portfolios can be made score-neutral, but this weak
perturbation adds no vertices. Batch 23 tests the completed stronger pair; after
that the locally bounded valence-beam hypothesis is retired.

## Batch 23 preparation - v56/v57 (stronger snapshot diversity)

**Hypothesis:** The 0.01%-per-valence perturbation in v54/v55 may be too weak
to alter an accepted audited snapshot. v56/v57 raise the alternate-trial factor
to 1% per valence in the low- and high-valence directions, respectively. The
perturbation remains confined to odd T2 portfolio trials and cannot affect the
main or giant QEM paths.

### Local diagnostics

- v56: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-110019887286164-metrics-compr-93.563877.txt`.
- v57: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-110148442304067-metrics-compr-93.563877.txt`.

Even strong alternate orderings do not beat the retained canonical state. This
completes four local attempts for the compact valence-beam hypothesis; do not
expand it further without official evidence. The pair remains a hidden-mesh
probe held behind earlier batches.


## Batch 24 preparation - v58/v59 (supported T5 post-QEM restoration)

**Hypothesis:** The failed v36-v41 restorations rebuilt liveness and adjacency
outside Galas supported state boundary. `compactRebuildPreserve()` already
compacts live vertices, preserves cluster radii, clears stale connectivity, and
resets transactional cursors. v58/v59 remove all pre-QEM flips, invoke standard
versus reduced profile-4 repair immediately after this T5 boundary inside
`runLargeCameraTx`, and use the same preserving rebuild after successful flips.
No other tier invokes the repair.

### Local diagnostics

- v58: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-110627945095085-metrics-compr-93.563877.txt`.
- v59: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-110800322828398-metrics-compr-93.563877.txt`.

Canonical fixtures do not enter T5, so both reproduce v15 locally, but they
compile and validate the corrected state transition. This is the first Bucket D
pair to use the architecture-supported compact-and-rebuild boundary; it remains
held behind the earlier batches.


### Batch 24 official results and post-mortem

- v58: **90.45279, PPPPPPP** — all-pass, only 0.000044 below v15.
- v59: **74.119451, PPPPPPF** — failed test 7.

Using `compactRebuildPreserve()` solves the post-QEM state-corruption failure:
standard-budget T5 restoration recovers 0.012818 of the 0.012862 loss seen when
flips ran pre-QEM. The remaining delta is negligible but not an improvement.
Reduced restoration breadth is again T7-unsafe through workload/layout. Preserve
v58 as the supported restoration anchor; v15 remains champion.

## Batch 25 preparation - v60/v61 (non-T5 tier partition)

**Hypothesis:** v51s non-T5 profile can be separated into screen meshes and
large/giant meshes to locate any safe positive tier island. v60 runs standard
profile-4 flips only for `inputV <= 50000`; v61 runs them only for
`inputV > 400000`. Neither touches the regressive T5 band.

### Local diagnostics

- v60: `COMPRESSION_RATE=93.547399`, mean SSIM 0.855426, mean Hausdorff
  usage 40.362360%, topology counts all zero. Output:
  `outputs/native-20260714-112310152748300-metrics-compr-93.547399.txt`.
- v61: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-112444080315062-metrics-compr-93.563877.txt`.

The local fixtures confirm the gates: v60 reproduces the screen-tier flip
fingerprint and v61 reproduces v15 because no canonical mesh exceeds 400k.
Both are held behind the earlier immutable batches.


### Batch 25 official results and post-mortem

- v60: **90.452362, PPPPPPP** — screen-only flips lose 0.000472.
- v61: **90.452834, PPPPPPP** — large/giant-only flips tie v15.

This exactly attributes v51s non-T5 regression to inputs at or below 50k.
Profile-4 flips above 400k are aggregate count-neutral and safe; they do not
improve the champion. Batch 26 splits the remaining screen loss between the
small/T2 and T3/T4 bands.

## Batch 26 preparation - v62/v63 (screen-tier subpartition)

**Hypothesis:** Screen-tier profile-4 effects differ across the known sharp
cliffs. v62 runs flips only for `inputV <= 25000`, covering the small/T2
basin; v63 runs them only for `25000 < inputV <= 50000`, covering T3/T4.
Neither touches T5 or any giant path.

### Local diagnostics

- v62: `COMPRESSION_RATE=93.547399`, mean SSIM 0.855426, mean Hausdorff
  usage 40.362360%, topology counts all zero. Output:
  `outputs/native-20260714-112712280331272-metrics-compr-93.547399.txt`.
- v63: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-112839144065854-metrics-compr-93.563877.txt`.

The canonical set is entirely at or below 25k, so the fingerprints verify both
gates. The exact pair remains held behind earlier batches.


### Batch 26 official results and post-mortem

- v62: **90.452834, PPPPPPP** — small/T2-only flips tie v15.
- v63: **90.452362, PPPPPPP** — T3/T4-only flips lose 0.000472.

This fully localizes the non-T5 loss to the 25k-50k band. Small/T2 and
large/giant profile-4 flips are count-neutral; T3/T4 and especially T5 are
regressive. Batch 27 completes the prepared T6 versus T7 attribution.

## Batch 27 preparation - v64/v65 (T6/T7 partition)

**Hypothesis:** The large/giant profile effect can be attributed without
touching smaller meshes. v64 runs standard flips only for
`400000 < inputV <= 1000000` (T6), while v65 runs them only above 1M (T7).
This tests whether either giant sub-band is a safe positive island.

### Local diagnostics

- v64: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-113109423602463-metrics-compr-93.563877.txt`.
- v65: identical canonical aggregate and fingerprints. Output:
  `outputs/native-20260714-113242417971664-metrics-compr-93.563877.txt`.

No canonical fixture enters these bands, so both correctly reproduce v15
locally. They remain held behind earlier batches.


### Batch 23 official results and post-mortem

- v56: **74.119495, PPPPPPF** — failed test 7.
- v57: **74.119495, PPPPPPF** — failed test 7.

Raising the dormant alternate-trial valence factor from 0.0001 to 0.01 makes
both directions identically T7-unsafe. The original-render audit cannot protect
a tier whose failure is caused by translation-unit layout before the alternate
T2 trial executes. The four-attempt valence-beam hypothesis is retired.


### Batch 27 official results and post-mortem

- v64: **90.452834, PPPPPPP** — T6-only flips tie v15.
- v65: **90.452834, PPPPPPP** — T7-only flips tie v15.

Both large sub-bands are independently count-neutral; v61s tie was not a
cancellation. The complete profile map is now exact: small/T2, T6, and T7 tie;
T3/T4 lose 0.000472; T5 loses 0.012862 pre-QEM but only 0.000044 when moved
to the supported post-QEM boundary.

## Batch 28 preparation - v66/v67 (same-width post-QEM restoration ranking)

**Hypothesis:** v58 validates standard-workload post-QEM T5 restoration but
trails v15 by 0.000044. Preserve exact source width, scan count, accept count,
and state boundary while changing which flips win: v66 increases triangle-
quality gain weight from 5.0 to 8.0; v67 reduces diagonal-shortening weight from
0.40 to 0.20. Same-width constants minimize giant binary-layout disturbance.

### Local diagnostics

- v66: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-132642813675459-metrics-compr-93.563877.txt`.
- v67: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-132810325721087-metrics-compr-93.563877.txt`.

Canonical fixtures do not enter the post-QEM T5 branch, so both correctly
reproduce v15 locally. They are immutable and held behind Batch 27.


### Batch 28 official results and post-mortem

- v66: **90.45279, PPPPPPP** — ties v58, 0.000044 below v15.
- v67: **74.119451, PPPPPPF** — failed test 7.

Increasing quality weight is count-inert and safe. Lowering diagonal weight is
T7-unsafe despite identical source length and loop workload, proving same-width
constants still change generated-code layout or timing. Batch 29 completes the
opposite directions before this four-attempt ranking sweep is retired.

## Batch 29 preparation - v68/v69 (opposite same-width restoration ranking)

**Hypothesis:** Complete the bounded same-width sweep around v58. v68 lowers
triangle-quality gain weight from 5.0 to 3.0; v69 increases diagonal-shortening
weight from 0.40 to 0.80. Both retain standard workload, supported state
boundary, exact source length, and T5-only execution.

### Local diagnostics

- v68: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-134052932953185-metrics-compr-93.563877.txt`.
- v69: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-134228759369969-metrics-compr-93.563877.txt`.

Canonical fixtures do not enter T5. These complete four local attempts for the
same-width restoration-ranking hypothesis and remain held behind Batch 28.


### Batch 29 official results and post-mortem

- v68: **90.45279, PPPPPPP** — lower quality weight ties v58.
- v69: **90.45279, PPPPPPP** — higher diagonal weight ties v58.

The four-attempt same-width ranking sweep is complete. Quality weight 3.0/8.0
and diagonal weight 0.80 are count-inert; diagonal weight 0.20 alone is T7-
unsafe. No ranking variant improves v58, so retire coefficient tuning and vary
the T5 restoration acceptance envelope next.


## Batch 30 preparation - v70/v71 (post-QEM restoration acceptance envelope)

**Hypothesis:** v58s ranking is count-inert, so vary which legal flips enter
the repair set while preserving its standard workload and source width. v70
relaxes T5 triangle-quality floor from 0.87 to 0.85; v71 tightens coplanarity
from 0.992 to 0.995. Both retain the supported post-QEM boundary.

### Local diagnostics

- v70: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-135146830059094-metrics-compr-93.563877.txt`.
- v71: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-135325632676711-metrics-compr-93.563877.txt`.

Canonical fixtures do not enter T5. Both exact evaluated files are ready for
the next immutable batch.


### Batch 30 official results and post-mortem

- v70: **90.452746, PPPPPPP** — broader quality floor loses 0.000088.
- v71: **90.452834, PPPPPPP** — tighter coplanarity ties v15.

Relaxing triangle quality admits a slightly worse basin. Tightening coplanarity
to 0.995 recovers v58s residual 0.000044 loss and makes supported post-QEM
restoration score-neutral. v71 is a structural co-champion and the new
restoration anchor, though it does not exceed v15. Batch 31 completes the
opposite envelope directions.

## Batch 31 preparation - v72/v73 (opposite restoration acceptance envelope)

**Hypothesis:** Complete the four-attempt acceptance sweep. v72 tightens T5
triangle-quality floor from 0.87 to 0.90; v73 relaxes coplanarity from 0.992 to
0.990. Both retain v58s supported boundary, standard workload, and exact source
length.

### Local diagnostics

- v72: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-135647414795769-metrics-compr-93.563877.txt`.
- v73: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-135822135673505-metrics-compr-93.563877.txt`.

Canonical fixtures do not enter T5. These complete four local attempts for the
restoration-envelope hypothesis and remain held while Batch 30 is pending.


### Batch 31 official results and post-mortem

- v72: **74.119495, PPPPPPF** — tighter quality floor fails test 7.
- v73: **90.452834, PPPPPPP** — broader coplanarity ties v15.

The four-attempt acceptance sweep is complete. Quality-floor changes are
regressive or T7-unsafe. Coplanarity is non-monotonic: 0.990 and 0.995 tie the
champion while 0.992 trails by 0.000044. Retire quality-floor tuning; Batch 32
continues the successful coplanarity axis above 0.995.

## Batch 32 preparation - v74/v75 (stricter flat-patch restoration)

**Hypothesis:** v71s coplanarity 0.995 is a structural co-champion. Tightening
the same-width threshold to 0.997 (v74) and 0.999 (v75) tests whether removing
progressively less-flat T5 repairs turns score-neutral restoration into a gain.
Standard workload and supported post-QEM state handling remain unchanged.

### Local diagnostics

- v74: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-140318017985663-metrics-compr-93.563877.txt`.
- v75: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-140439117072127-metrics-compr-93.563877.txt`.

Canonical fixtures do not enter T5. Both candidates are immutable and held
while Batch 31 is pending.


### Batch 32 official results and post-mortem

- v74: **90.452702, PPPPPPP** — coplanarity 0.997 loses 0.000132 vs v15.
- v75: **90.452878, PPPPPPP** — **NEW CHAMPION**, +0.000044 vs v15.

The strict-flat response is non-monotonic and has a positive basin at 0.999.
v75 proves supported post-QEM restoration can improve the champion, not merely
tie it. Promote v75 as canonical Gala champion. Batch 33 completes the already
evaluated 0.996/0.998 interpolation before a tighter refinement around 0.999.

## Batch 33 preparation - v76/v77 (interpolated strict coplanarity)

**Hypothesis:** Complete the four-point strict-flat refinement around co-champion
v71. v76 uses coplanarity 0.996 and v77 uses 0.998, interpolating the prepared
0.997/0.999 pair. All preserve exact source length, standard workload, and the
supported T5 post-QEM boundary.

### Local diagnostics

- v76: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-141446058704157-metrics-compr-93.563877.txt`.
- v77: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-141620774369701-metrics-compr-93.563877.txt`.

Canonical fixtures do not enter T5. These complete four local attempts for the
strict-coplanarity refinement and remain held behind Batch 32.


### Batch 33 official results and post-mortem

- v76: **90.452746, PPPPPPP** — 0.996 loses 0.000132 vs v75.
- v77: **90.452702, PPPPPPP** — 0.998 loses 0.000176 vs v75.

The four-attempt strict refinement is complete: 0.996, 0.997, and 0.998 all
trail; 0.999 alone improves. The response is a sharp discrete survivor-set
jump rather than a smooth flatness trend. Batch 34 brackets 0.999 at half-step
resolution.

## Batch 34 preparation - v78/v79 (half-step refinement around v75)

**Hypothesis:** v75s 0.999 coplanarity is the first restoration improvement.
Bracket it with same-length half-step literals: v78 uses `.9985`, v79 `.9995`.
Both preserve exact source size, standard workload, and supported post-QEM T5
state handling.

### Local diagnostics

- v78: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-143050131483305-metrics-compr-93.563877.txt`.
- v79: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-143211285959522-metrics-compr-93.563877.txt`.

Canonical fixtures do not enter T5. The pair is immutable and held behind
Batch 33.


### Batch 34 official results and post-mortem

- v78: **90.45279, PPPPPPP** — `.9985` loses 0.000088 vs v75.
- v79: **90.452834, PPPPPPP** — `.9995` loses 0.000044 vs v75.

v75s 0.999 threshold is now bounded on both sides by worse all-pass scores. The
positive survivor-set change is narrow and discrete. Batch 35 completes the
quarter-step samples `.9988` and `.9992`; after that this threshold refinement
is locally bounded.

## Batch 35 preparation - v80/v81 (quarter-step refinement around v75)

**Hypothesis:** Complete the four-attempt local bracket around v75. v80 uses
`.9988` and v81 `.9992`, quarter-step values surrounding 0.999 while retaining
the exact five-character literal width, source size, workload, and supported
post-QEM boundary.

### Local diagnostics

- v80: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-145043796226661-metrics-compr-93.563877.txt`.
- v81: identical aggregate and fingerprints. Output:
  `outputs/native-20260714-145227334520376-metrics-compr-93.563877.txt`.

Canonical fixtures do not enter T5. These complete four local attempts for the
champion-threshold refinement and remain held behind Batch 34.


### Batch 35 official results and post-mortem

- v80: **90.452878, PPPPPPP** — `.9988` ties champion v75.
- v81: **74.166719, PPPPPFP** — `.9992` fails test 6.

The four-attempt champion bracket is complete. The safe winning interval spans
at least `.9988` through `.9990`; `.9985` is lower-scoring and `.9992` crosses
T6s cliff. Retire further scalar threshold refinement without a new structural
change. v75 remains canonical champion; v80 is an equivalent control.

## Batch 36 preparation - v82/v83 (co-champion tier composition)

**Hypothesis:** Compose v75s winning T5 post-QEM restoration with profile-4
preconditioning on tiers that were independently count-neutral. v82 adds a
small/T2-only pre-QEM call (`inputV <= 25000`); v83 adds a large/giant-only
call (`inputV > 400000`). T3/T4 and pre-QEM T5 remain excluded.

### Local diagnostics

- v82: `COMPRESSION_RATE=96.312371`, mean SSIM 0.849066, mean Hausdorff
  usage 41.825860%, topology counts all zero. Two long-running T2 fixtures move
  from 83.6654/73.0021% compression to 92.1440/92.0084%. Output:
  `outputs/native-20260714-150110750476849-metrics-compr-96.312371.txt`.
- v83: `COMPRESSION_RATE=93.563877`, mean SSIM 0.854867, mean Hausdorff
  usage 40.335620%, topology counts all zero. Output:
  `outputs/native-20260714-150233652112478-metrics-compr-93.563877.txt`.

v82 is the first co-champion descendant with a large local compression movement
and remains topology-clean, though its lower SSIM makes it a high-risk official
probe. v83 is locally dormant. Both exact candidates are immutable and held
behind Batch 35.


### Batch 36 official results and post-mortem

- v82: **74.119539, PPPPPPF** — failed T7 despite 96.312371 local
  compression and clean topology.
- v83: **74.119539, PPPPPPP** — all-pass but catastrophic official
  compression regression despite locally matching v75.

Tier compositions are not additive. Adding either pre-QEM call site to v75
changes global generated-code/timing behavior; v82s dramatic local T2 endgame
is not official-safe, while v83 shows severe local/Kattis compression divergence
without a validity failure. Retire co-champion tier composition. Batch 37 remains
a completed diagnostic of whether v82s scan breadth changes the failure.

## Batch 37 preparation - v84/v85 (small/T2 one-flip scan breadth)

**Hypothesis:** v82s large local movement may depend on which single ultra-
planar repair is found. v84 halves the small/T2 scan from 8000 to 4000 edges;
v85 expands it to 12000. Both retain the one-flip cap, v75 T5 restoration, and
all tier exclusions.

### Local diagnostics

- v84: `COMPRESSION_RATE=96.312371`, mean SSIM 0.849068, mean Hausdorff
  usage 41.825860%, topology counts all zero. The first long T2 fingerprint
  changes relative to v82. Output:
  `outputs/native-20260714-150933770736259-metrics-compr-96.312371.txt`.
- v85: `COMPRESSION_RATE=96.312371`, mean SSIM 0.849066, mean Hausdorff
  usage 41.825860%, topology counts all zero; canonical fingerprints match v82.
  Output: `outputs/native-20260714-151049866558746-metrics-compr-96.312371.txt`.

Scan breadth does not change local counts, but v84 selects a distinct repair.
Both immutable high-movement probes remain held behind Batch 36.
