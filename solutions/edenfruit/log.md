# Edenfruit iteration log

## Theory

Edenfruit combines the proven Holyfruit v37 baseline (90.451111) with new
directions inspired by Tranberry and the world model. The champion solves the
metric per collapse, not by parameter tuning. The goal of edenfruit is to break
the ~90.4 plateau and reach 95 by exploring genuinely different structural
moves that the two families have not converged on.

The starting belief is that both families have saturated the safe QEM- and
SSIM-counsel ordering; remaining headroom must come from either:

1. Accepting more collapses per pass under stricter per-collapse quality.
2. Exploiting geometric regularities that current solvers never touch.
3. Combining structural passes with a deterministic, full-state validation
   audit that the current code does not run.

Each candidate must be an end-to-end meaningful change that affects every tier
and is verified by Kattis.

## Batch 1 - v01, v02

**Hypothesis:** both families have plateaued because the proven safe tuning
sits inside a narrow basin. A small combined push on every tier (lower keep
ratios and a slightly higher QEM cost cap) should preserve the per-collapse
quality floor while freeing more collapses.

- v01: direct copy of Holyfruit v37 (90.451111) as the local floor.
- v02: identical to v01 except `HParam_KeepRatio_UpTo400k` 0.0237 -> 0.0227,
  `HParam_KeepRatio_Huge` 0.0200 -> 0.0190, and `HParam_QemCostCapCoeff`
  0.0330 -> 0.0340. All other parameters, all renderer guards, the exact-window
  counsel breadth, the envelope-balanced giant proposal, and the translation-
  unit layout are unchanged.

### Batch 1 official result and post-mortem

- v01: service deduplicated to the holyfruit v37 submission; same 90.451111
  PPPPPPP. Confirms the canonical baseline lives at this score.
- v02: 57.833380, PPPPPFF; failed hidden test 5 and giant test 7.

Lowering the keep ratios across the whole 50k-400k band and the giant tier
loses both cliffs simultaneously. Local evaluator reported the same
93.563877 mean compression as v37, so it cannot distinguish a 90.45 from a
57.8 collapse set. The keep ratios sit exactly on the SSIM threshold, and
pushing them below 0.023 / 0.020 collapses too much of the silhouette.

Conclusion: do not move the keep ratios for the next batches; vary collapse
ordering, position portfolio, or pass timing instead.

## Batch 2 - v03, v04

**Hypothesis:** changing the QEM cost cap and the exact-window counsel budget
should expose a different collapse ordering. The Holyfruit family has already
nudged these in v02-v13, but a tighter combined setting may find a new
basin.

- v03: `HParam_QemCostCapCoeff` 0.0330 -> 0.0300, `HParam_TailBatchElapsedStart`
  11.8 -> 10.8, `HParam_TailBatchStopElapsed` 19.8 -> 19.6. Everything else
  identical to v01.
- v04: `HParam_ExactWindowBudgetTier2` 0.00024 -> 0.00030, `Tier3` 0.00018 ->
  0.00022, `MaxAcceptTier2` 48 -> 60, `MaxAcceptTier3` 72 -> 90. Counsel
  breadth is wider and accepts more per tier.

### Batch 2 official result and post-mortem

- v03: 90.451111 PPPPPPP (identical to v37). The earlier tail start and tighter
  cost cap preserve the same safe trajectory.
- v04: 90.451111 PPPPPPP. The wider counsel budget is still count-inert on the
  default mesh set.

Both batches 1 and 2 confirm what the Holyfruit log already says: parameter
nudges inside the existing basin saturate at the v37 score. Even a smaller
QEM cap, a wider counsel budget, and an earlier tail batch produce no
movement. The basin is robust because the keep ratios, the giant T7 source
layout, and the medium-tier endgame already define the safe collapse set.

**Next direction:** stop tuning and change the algorithm itself. Candidates
must add or remove an actual structural pass, not adjust constants.

## Batch 3 - v05, v06

**Hypothesis:** every parameter nudge inside the v37 basin saturates at
90.451111, so we need a real structural change. Two candidates:

- v05: copy of v01 but adds a second `pairDiskPass` and a `vegaSsimStarPass`
  to the T5-T6 path (inputV>50000). The T5-T6 path currently has only one
  pairDiskPass, one valenceWeldPass, one collapseInvisibleEdges, and the
  large-camera endgame. Small-tier paths have two pairDiskPass and three
  vegaSsimStarPass calls; bringing T5-T6 closer to that shape may find more
  collapses.
- v06: copy of v01 but removes the two-ring visibility exclusion from
  `occludedEdgeCollapsePass`. The original code skips any candidate edge
  whose 1-ring contains a visible vertex; the change makes the per-pixel
  behind-depth proof do the gating instead. Matches the holyfruit v45
  hypothesis: substantial vertex mass is hidden behind the axial depth
  layers, and the two-ring exclusion discards too much of it.

### Batch 3 official result and post-mortem

- v05: 90.451111 PPPPPPP. Adding a star pass before the existing
  large-camera endgame does not move the official count on any tier. The
  endgame already does star work at tier 5/6.
- v06: **90.452702** PPPPPPP. **NEW CHAMPION.** +0.001591 over v37.

Removing the two-ring visibility exclusion unlocked additional hidden
collapses that the per-pixel depth proof then certified. The two-ring
guard was overly conservative. The proof still gates the collapse so
the mesh remains valid.

This is the first edenfruit candidate that breaks the 90.451111 plateau,
and it comes from a structural change rather than parameter tuning.
The v37 keep ratios, giant T7 layout, and counsel breadth are still in
place; only the visibility exclusion was relaxed.

**Next direction:** build on v06. The two-ring exclusion removal proves
hidden mass was being discarded. Try widening the per-pixel proof
budget, lowering the QEM cap on hidden edges, or extending v06 to
the small-tier path. v06 becomes the new baseline for further
structural exploration.

## Batch 4 - v07, v08

**Hypothesis:** v06's gain came from relaxing the two-ring visibility
exclusion. v07 re-adds the looser one-ring form, and v08 tightens the
tier-3 safe continuation target.

- v07: identical to v06 except `near` is set in a one-ring loop around
  each endpoint, recovering the conservative exclusion.
- v08: same as v01, but `finalTarget = nV*0.135` (was 0.145) in
  `runScreenCoreMid`'s screen-tier 3 path.

### Batch 4 official result and post-mortem

- v07: 90.451111 PPPPPPP. Regression of 0.001591 vs v06. Restoring even
  a one-ring visible-vertex exclusion in `occludedEdgeCollapsePass`
  discards the hidden vertices the per-pixel depth proof was already
  certifying. The conservative loop is strictly worse.
- v08: 76.187727 PPPFPPP. Test 4 failed. Lowering the safe continuation
  target on the screen-tier 3 band (25k–45k) crosses the SSIM cliff for
  the 45k input. The current 0.145 floor is exactly on the boundary.

**Conclusion:** keep the v06 inclusion-open policy; the v06 champion is
the only safe direction in the hidden-edge neighborhood.

## Batch 5 - v09, v10

**Hypothesis:** widening the per-tier caps and relaxing the transactional
screen SSIM guard could free more collapses without changing ranking
order.

- v09: `occludedEdgeCollapsePass` per-tier caps widened
  (T5 3500→5000, T6 4500→6000; per-active-frac 0.035→0.05 on T5 and
  0.018→0.025 on T6).
- v10: `runTransactionalScreenMid` SSIM guard relaxed
  from `0.9995/0.9990/0.9985/0.9995` to `0.9985/0.9980/0.9975/0.9985`.

### Batch 5 official result and post-mortem

- v09: 90.452702 PPPPPPP. Tied with v06. The additional occluded-edge
  candidates collapse the same vertices v06 already unlocked, so the
  count is unchanged.
- v10: 90.452702 PPPPPPP. Tied. Loosening the per-pass guard does not
  free any extra collapse because the per-collapse validation gates
  remain intact.

**Conclusion:** cap widening and guard relaxation alone are no-ops on
top of v06. The validator is the binding filter, not the per-pass
guard.

## Batch 6 - v11, v12

**Hypothesis:** the `h.B4` membership check inside the candidate loop
of `occludedEdgeCollapsePass` is still a soft barrier. Removing it
entirely and adding a uniform 0.8 weight to `cheapEdgeCost` should
shift the ranking.

- v11: removes the `!h.B4[a] && !h.B4[b]` filter at the top of the
  candidate-generation loop (any endpoint can come from outside the
  hidden atlas).
- v12: multiplies `cheapEdgeCost` by 0.8 inside the same loop.

### Batch 6 official result and post-mortem

- v11: 90.452702 PPPPPPP. Tied. Atlas-stripping does not change ranking
  versus v06; the per-pixel proof is the binding filter.
- v12: 90.452702 PPPPPPP. Tied. A uniform 0.8 weight does not change
  ranking, so this is a no-op.

**Conclusion:** the ranking inside `occludedEdgeCollapsePass` is fully
determined by the per-pixel proof, not by the atlas or weight. Pushing
the function any further needs to attack the *proof* itself (its margin
or its per-pixel coverage), or find a completely different structural
pass that the family has not tried.

## Batch 7 - v23, v24

**Hypothesis:** the only structural lever left is the per-pixel
*proof* itself, not the candidate set. v22 had a uniform 0.7 weight on
`cheapEdgeCost` in `occludedEdgeCollapsePass`, so the costCap was the
binding filter on a wider pool. v23 loosens the per-pixel depth margin;
v24 lowers the QEM cost cap coefficient uniformly.

- v23: copy of v22 with `hiddenPatchBehind`'s margin loosened from
  `max(2e-5, 0.0012*diag)` to `max(3e-5, 0.0010*diag)`. This relaxes
  the proof so slightly tighter squeezes behind existing geometry are
  admitted on every tier that uses `occludedEdgeCollapsePass`.
- v24: copy of v22 with `HParam_QemCostCapCoeff` lowered from 0.0330
  to 0.0320. The `costCap` is recomputed at every QEM restart (2 places),
  so the cap is uniformly softer across the screen-mid, large-tier, and
  giant paths.

Both candidates compile. Local evaluator on the ppsurf dataset shows
identical fingerprints to v22 — the proxy does not exercise the SSIM
cliff or the medium tier, so we must rely on Kattis.

### Batch 7 official result and post-mortem

- v23: 90.452702 PPPPPPP. Tied with v22. Loosening the per-pixel proof
  margin does not enable any extra collapse; the local proxy gives
  bit-identical output.
- v24: 90.452702 PPPPPPP. Tied. Lowering the QEM cap coefficient by
  ~3% is count-inert on the v22 lineage.

**Conclusion:** the binding filter is not the proof margin nor the QEM
cap. It must be the per-collapse `txGuard` validation sequence, or a
more sophisticated ranking change. Next try: enabling a previously
disabled `tryRootNudgeToward` profile on additional tiers.

## Batch 8 - v25, v26

**Hypothesis:** the binding constraint is not the per-pixel proof or
the QEM cap. Try a different ordering lever: enable `tryRootNudgeToward`
on additional tiers via `HParam_RootNudgeProfile`. Profile 1 only nudges
on tier 4; profile 2 adds tier 2; profile 3 adds tier 2 + tier 3.

- v25: profile 2 — nudge on tier 2 (`{0.035, 0.0008, 0.92, 0.995, 28}`)
  + tier 4.
- v26: profile 3 — nudge on tier 2 + tier 3 + tier 4 with respective
  relaxed params.

Both compile. Local evaluator reproduces the v22 fingerprint exactly.

### Batch 8 official result and post-mortem

- v25: 90.452702 PPPPPPP. Tied. Enabling tier-2 root nudge does not
  alter any of the official collapse counts.
- v26: 90.452702 PPPPPPP. Tied. Profile 3 is also count-inert.

**Cross-batch insight:** v23, v24, v25, v26 have all scored exactly
90.452702 — the same as v22. Each of the four candidates modified a
different knob (per-pixel margin, QEM cost cap, root-nudge profile 2,
root-nudge profile 3), and none of them changed official output. The
binding cliff is more abstract than these knobs.

## Batch 9 - v27, v28

**Hypothesis:** every parameter nudge is inert on the official set,
which means the structural pipeline needs a different lever. Two
structural changes:

- v27: copy of v22, but add a `largeStarPass(inputV, 6, 1.0)` call to
  the T7 path between `pairDiskPass` and `vegaSsimStarPass`. Tier 7
  currently has the most-aggressive pure-QEM-and-stars pipeline but no
  `largeStarPass`; adding one may free a few star-style collapses that
  the multi-pass vega path leaves behind.
- v28: copy of v22, but reduce T7's `qemDeadline` from `A0` to
  `A0-0.50`, freeing 0.5 s for the endgame passes. The collapseLoop
  may reach the same count earlier with a tighter deadline, leaving
  more time for refinement.

### Batch 9 official result and post-mortem

- v27: 90.452702 PPPPPPP. Tied. Inspection after submitting revealed
  that the new `largeStarPass` call sits inside `if(inputV>400000){...}`
  which is reached only AFTER the EARLIER `if(inputV>400000)
  {compact();writeMesh();return;}`. The first block returns before
  the second one runs, so the new code never executes. v27 was
  structurally inert because of dead-code, not algorithm inertness.
- v28: 90.452702 PPPPPPP. Tied. Same dead-code issue: shortening
  `qemDeadline` only matters if the refinement block runs, and it
  doesn't on T7 today.

**Insight:** T7 has a "dead refinement" landmine. The original
implementation has two `if(inputV>400000)` blocks back-to-back; the
first returns, the second is unreachable. Every candidate since v06
that modified T7's pipeline was building on this dead second block.

## Batch 10 - v29, v30

**Hypothesis:** a real lever is to make T7 actually run its refinement
code. v29 re-routes the control flow so the refinement block is no
longer dead; v30 also shrinks T7's `qemDeadline` to free time for the
gates to fire.

- v29: copy of v22, but move the refinement code block to BEFORE the
  early `compact();writeMesh();return;` statement. Now T7 follows
  `collapseLoop() → refinement → compact → writeMesh`. Refinement is
  still time-gated, so it only executes when gates pass.
- v30: same as v29, plus reduce T7's `qemDeadline` from `A0` to
  `A0-1.50`. With 1.5 s margin the gates at `A0-1.10`, `A0-0.95`,
  `A0-0.88`, `A0-0.78`, `A0-0.65` all pass, so `pairDiskPass`,
  `valenceWeldPass`, and `vegaSsimStarPass` should now run on T7.

Both compile. Local evaluator (ppsurf, no T7 input) is identical for
v29 and bit-slower for v30 — consistent with v30 doing real extra work
on the small meshes via the still-applicable inputV<=50000 fallback.
The first-ever real T7-side change since v06 is on the wire.

### Batch 10 official result and post-mortem

- v29: 90.452702 PPPPPPP. Tied. With `qemDeadline = A0`,
  `collapseLoop` consumes the full 20.2 s and every refinement gate
  fails. The control-flow re-route is mechanically inert.
- v30: 90.452702 PPPPPPP. Tied. With `qemDeadline = A0-1.50`, the
  refinement gates now pass and `pairDiskPass` / `valenceWeldPass` /
  `vegaSsimStarPass` actually run on T7 for the first time. Their
  per-pass SSIM guards reject any collapse that would change the
  final count, so the result is byte-identical to v22 on every
  official case.

**Insight — structural landmine was found but did not unlock new
score:** the dead-code refactor in `run()` is correct, but T7's
collapse set is already at the SSIM cliff. The bindings on every tier
that we have not yet crossed are SSIM cliffs, not control-flow or
budget cliffs.

**Next direction:** stop reaching for T7 — it is plateaued at the
binding SSIM gate. The most likely test to break the plateau is the
mid tier (T2 or T3), where the binding constraint is delicate (the
v08 reduction to 0.135 crossed the cliff). Try widening/refining the
T2 screen-stage loop and the T3 finalTarget **defensively** — small
shifts only, with each candidate's expected SSIM impact computable.

## Batch 11 - v33, v34

**Hypothesis:** small shifts toward contraction on T2 and T3 might
break the 90.452702 plateau. Both candidates are minimal moves
within the existing screen-stage / finalTarget framework.

- v33: add `0.28` as a fourth T2 stage (stages become
  `[0.36, 0.33, 0.30, 0.28]`); pad T4 with a fourth `0.08`.
- v34: tighten T3 `finalTarget` from `0.145` to `0.140`.

### Batch 11 official result and post-mortem

- v33: **78.78582 PPFPPPP**. Test 3 (T2 band) failed. The 4th T2 stage
  of `0.28` crossed the SSIM cliff on a T2-sized mesh. The T2
  binding on the 5k–25k band sits exactly at the v22 default of
  30 % retained (three stages of [0.36, 0.33, 0.30]). Adding 0.28
  pushes below the cliff.
- v34: pending.

**Insight — the T2 SSIM cliff is sharp:** the gap between
`0.30` (passes) and `0.28` (fails) is what bounds the entire T2
contribution. We need either (a) a per-collapse SSIM-aware change
that shifts the cliff itself, or (b) to migrate the win onto T3/T4
which have lower keep targets but sharper per-test cliffs of their
own. Both directions need careful budgeted scoring — T3 had `0.145`
as its verified safe target and v08 showed `0.135` fails.

### v34 official result (added after submission)

- v34: **76.189318 PPPFPPP**. Test 4 (T3 band) failed. Pushing
  `finalTarget` from `0.145` to `0.140` is just below the T3 SSIM
  cliff. **The T3 cliff is between `0.140` and `0.145`.**

**Insight — both mid tier SSIM cliffs are sharp.** Test 3 (T2) is
pinned at 30% retained (cliffs immediately below). Test 4 (T3) is
pinned at 14.5% retained. We cannot push either of these directly;
both are at the limit of what the SSIM validator will accept.

**Next direction — find a way to shift the SSIM cliff itself.**
A change that improves per-collapse SSIM efficiency (so the same
target reduction results in higher SSIM) could push past the cliff
without changing target numbers. One candidate: tighten the local
SSIM-proof gates carefully (per-tier `txGuard` floors) or improve
the position portfolio. Another: change the T1 / T5 / T6 / T7
contributions where we have more room.

## Batch 12 - v37, v38

**Hypothesis:** counsel on the mid tiers has a self-skip if
`left = A0 - elapsed()` is too small, so adding a third call is
free at worst — try it on every mid-tier site. Separately, fully
re-enable T7's `runLargeCameraTx` while reducing its `qemDeadline`.

- v37: copy of v22 with a third `exactWindowCounselEdgePass()` call
  added after each pair (so each T2 / T3 / T4 site now has three
  calls instead of two). Self-skipping handles tight time budgets.
- v38: copy of v22 with the T7 dead-code fix re-routed; T7
  `qemDeadline` lowered from `A0` to `A0-3.00`; T7 explicitly
  invokes `runLargeCameraTx(inputV)` for the first time.

### v37 official result

- v37: **90.449839 PPPPPPP**. Regression of -0.002863 vs v22. The
  third counsel call commits slightly different collapses that hurt
  SSIM enough to alter the official count. The existing two calls
  were at the binding count.

(Defer v38 analysis to next batch.)

### v38 official result (added)

- v38: **74.119363 PPPPPPF**. **Test 7 (T7) failed.** Re-enabling
  `runLargeCameraTx` on T7 broke test 7. Layout-sensitive per world
  model. We now have an *empirical* confirmation in the edenfruit
  lineage: the T7 layout is brittle. Test 7 keeps our known-good
  pipeline intact.

**Insight — T7 has two binding constraints: (1) source layout is
extremely sensitive (runLargeCameraTx crashes it), (2) SSIM gates
prevent measurable change (v30). Stop reaching for T7.**
## Final post-mortem (after 24 batches / ~70 candidates)

After v06's two-ring-exclusion removal (+0.001591), edenfruit has been
unable to find any further gain over 24 batches. Every remaining
structural or parameter change has either tied at 90.452702 or
broken one of the binding SSIM cliffs on tests 3/4/5/7.

The canonical edenfruit champion is **v22** at **90.452702** (PPPPPPP).
This is +0.001591 over the holyfruit v37 baseline (90.451111) and
+0.015186 over the holyfruit v34 baseline (90.437516).

The plateau is structural: every per-tier target sits at the binding
SSIM boundary, every QEM/counsel/SSIM validator has slack that doesn't
translate to score, and every "dead-code" structural pass that we
reactivated turned out to be active enough to cross cliffs.

**Empirical SSIM cliffs (from 24 batches)**:
- T2 (test 3): 30 % retained last-stage. 0.28 or 0.29 → fails.
- T3 (test 4): 14.5 % retained finalTarget, 16 % safeTarget. 0.140 or 0.143 → fails.
- T4 (test 5): 14 % first stage. 0.13 → fails.
- T7 (test 7): source layout — `if(false)runLargeCameraTx` → fails.

**Per-tier slack (tied at 90.452702)**:
- T2 first-stage 0.36 → 0.34: tied.
- T3 absoluteQemBudget floors ±0.0002: tied.
- T3 absoluteQemEndgame B9(512) → B9(1024): tied.
- T5 txReserve 1.70 → 1.50: tied.
- T5 qemDeadline A0-1.90 → A0-2.50: tied.
- Counsel `HParam_ExactWindowBudgetTier3` 0.00018 → 0.00022: tied.
- Counsel `HParam_ExactWindowSeedCheap` and SeedVisual: tied (v04 baseline).
- QEM cost cap ±0.001: tied.
- Hidden-edge margin ±0.00005: tied.
- MEMLESS=true: tied.
- Hidden-edge atlas restriction removed: tied (v11 was already at the v22 baseline).

**Conclusion:** the v22 baseline is *essentially optimal* within the
edenfruit pipeline. To reach 95, we need an architectural change
beyond parametric adjustment. Candidates we did not pursue:
edge flips (rejected in tranberry v149), snapshot beam search
(Bucket 15), regional projected-volume budgets (Bucket 17), and
reversible progressive simplification (Bucket 18). Of these, only
Bucket 18 (regional rollback) is operationally feasible in the
remaining iteration budget.

## Final session post-mortem (after 31 batches / 82 candidates)

After resuming the edenfruit iteration through batches 25-31 (10
more batches, ~12 more candidates), no further progress was made.
The plateau at **90.452702** remains the canonical edenfruit
champion. The user explicitly asked to wrap up and write a final
post-mortem after batch 31.

**Final tallies (across 31 batches)**:
- v22 baseline (90.452702) held throughout.
- All post-baseline attempts either tied or broke one or more
  tests, with one exception: **v06 (two-ring visibility exclusion
  removal, +0.001591)** which became the canonical champion.

**Final batch (batch 31) breakdown**:
- v73 (VegaScoreGeomWeight halved): tied
- v74 (VegaNormalDepthWeight 0.55→0.50): tied
- v75 (VegaPatchMaxPixels 52000→36000): tied
- v76 (VegaPatchPaddingPixels 4→8): tied
- v77 (counsel MaxEdgeEvaluations 112→168): tied
- v78 (counsel MaxAcceptTier2/3 raised): tied
- v79 (T7 _Huge 0.0200→0.0205 looser): regressed to 90.444378
- v80 (T5 _UpTo400k 0.0237→0.0240 looser): regressed to 90.447928
- v81 (midpoint 0.4·a+0.6·b): 62.44, tests 3+7 failed
- v82 (midpoint 0.3·a+0.7·b): 63.42, tests 3+5+7 failed

**Where the plateau actually sits**: every tunable knob is at the
binding SSIM boundary on at least one tier:
- T2 last stage = 0.30 (test 3 cliff)
- T3 finalTarget = 0.145 / safeTarget = 0.16 (test 4 cliff)
- T4 first stage = 0.14 (test 5 cliff)
- T5 keepRatio = 0.0237 (test 6 binding from above)
- T7 keepRatio = 0.0200 (test 7 binding from above)
- Counsel / vega / raster knobs: all robust at the binding collapse set
- Unbiased midpoint `(verts[a]+verts[b])*0.5` is exactly calibrated

**Final edenfruit champion: v22 at 90.452702 (PPPPPPP).**

Edgenfruit delta vs prior champions:
- vs holyfruit v37 baseline (90.451111): +0.001591
- vs holyfruit v34 (90.437516): +0.015186
- vs original nebula v14 (90.187632): +0.265070

We did not reach the 95 target. The plateau appears to be a hard
property of the v37-style pipeline + the binding SSIM cliffs on
each official test. Further progress would require either an
architectural rewrite (edge flips, snapshot beam, regional rollback)
or a different problem formulation entirely.
