# Holyfruit iteration log

## Theory

Holyfruit adds exact official-style 11x11 foreground-window SSIM contribution
ledgers to the established screen-space simplifier. Candidate collapses are
rendered against the original mesh, selected under a cumulative per-view image
budget, applied conflict-free, and accepted only after a full-state audit.
Kattis remains the acceptance and ranking oracle.

## Baseline - v1

The repository contained v1 without a family log, job ledger, or local output.
The user reports that its official result is slightly better than Tangerine's
best. A ppsurf plus synth_bench diagnostic measured 88.075950% mean
compression, 0.826775 mean SSIM, and zero nonmanifold edges, boundaries, or
degenerate faces across all 20 scenarios.

## Batch 1 - v2, v3

**Hypothesis:** v1 is limited by stale candidate contributions and narrow
proposal breadth. Additional original-reference-safe collapses can be obtained
by either spending a slightly larger audited image budget or rerendering and
reseeding after the first conflict-free round.

| Version | Meaningful all-tier / exact-window change | Local diagnostic | Kattis |
|---|---|---|---|
| v2 | Global QEM cap 0.0375 to 0.0400; wider exact pool, acceptance caps, time, and 1.5x visual budget | 88.076560% compression, 0.826784 SSIM, zero topology defects | pending |
| v3 | Global QEM cap 0.0375 to 0.0350; broader pool and two audited counsel rounds under the original visual budget | 88.107399% compression, 0.826715 SSIM, zero topology defects | pending |

**Pre-submission interpretation:** v2's near-neutral result says increasing a
single stale round's budget is weak. v3's 0.031449-point gain indicates that
rerender/reseed rounds unlock a materially different set of locally safe
collapses. Both candidates are immutable after their single diagnostic.

### Official result and post-mortem

- v2: 74.103464, PPPPPPF. The looser global QEM cap changed the giant path and failed test 7.
- v3: 90.441526, PPPPPPP. New champion over 90.436296.

Repeated exact-window rerender/reseed transfers officially. Preserve v3 as the
new base. A wider single-round visual budget is not worth global giant-tier
risk; next test repeated audited rounds with tighter all-tier QEM caps.

## Batch 2 - v4, v5

**Hypothesis:** repeated rerender/reseed rounds expose new safe collapses; finer
per-round budgets may preserve the gain with more safety.

- v4: global QEM cap 0.0340 and three original-budget audited rounds; local 88.146525% compression, 0.826642 SSIM, zero topology defects.
- v5: global QEM cap 0.0330 and four half-budget audited rounds; local 88.131757% compression, 0.826688 SSIM, zero topology defects.

v4 removes more vertices; v5 spends less image damage but approaches the time
limit on one proxy. Both are immutable and will be submitted together.

### Batch 2 official result and post-mortem

- v4: 59.849526, PPPFPPF; hidden test 4 and giant test 7 failed.
- v5: 76.182865, PPPFPPP; hidden test 4 failed, giant passed.

The hidden 25-45k tier has no demonstrated slack beyond v3 two rounds. More
rounds fail even when per-round image budgets are halved. Revert that tier to
v3 and extend exact-window counsel to the uncovered 45-50k tier instead.

## Batch 3 - v6, v7

**Hypothesis:** exact-window counsel can improve the previously uncovered
45-50k tier while conservative independent large-tier targets preserve validity.

- v6: one test-5 exact round, T5 0.0239, T6 0.0202, cap 0.0345; local 88.106890%, 0.826725 SSIM, zero topology defects.
- v7: two test-5 exact rounds, T5 0.0238, T6 0.0201, cap 0.0335; local 88.107816%, 0.826717 SSIM, zero topology defects.

Both changed the 48k boundary fingerprint without reducing its count. v7 gains
more on the local large proxy; official test 5 determines whether its audited
redistribution creates hidden SSIM slack.

### Batch 3 official result and post-mortem

- v6: 90.439825, PPPPPPP.
- v7: 90.443129, PPPPPPP; new champion.

Exact counsel is valid on the 45-50k tier, but the score change is dominated by
the large and giant keep-ratio deltas. Candidate evaluation still loops over
every face for every patch, limiting the exact search to tens of edges. Next:
spatially index scene faces by projected tile to scale the audited search.

## Batch 4 - v8, v9

**Hypothesis:** projected-tile scene indexing preserves exact audits while
allowing a materially broader candidate search inside the same deadline.

- v8 indexing control plus giant 0.0200: local 88.108314%, 0.826709 SSIM, zero topology defects. Exact-tier fingerprints match v7; runtimes fall about 15-45%.
- v9 widened indexed pool/caps, large 0.0237, giant 0.0200, cap 0.0330: local 88.116903%, 0.826718 SSIM, zero topology defects.

The index is behavior-preserving at control breadth. v9 converts the speedup
into 0.008589 local mean compression with no mean-SSIM loss; both are immutable.

### Batch 4 official result and post-mortem

- v8: 78.777915, PPFPPPP.
- v9: 78.780923, PPFPPPP.

Both fail only hidden test 3 while tests 4-7 pass. The projected index or its
runtime acceleration perturbs the <=25k transactional trajectory. Retain the
validated index only for tiers 3-4 and restore full-face rendering for tier 2.

## Batch 5 - v10, v11

**Hypothesis:** disabling spatial indexing only on tier 2 restores hidden test 3
while retaining validated indexed behavior on tiers 3-4.

- v10 gated indexing control: local 88.108314%, 0.826709 SSIM, zero topology defects; control fingerprints restored.
- v11 gated widened search: local 88.116903%, 0.826718 SSIM, zero topology defects; tier-2 pool remains broader by design.

v10 isolates the index gate. v11 reveals whether Batch 4 test-3 failure came
from indexing or from broader counseling; both are immutable.

### Batch 5 official result and post-mortem

- v10: 90.444797, PPPPPPP; new champion at completion time.
- v11: 90.447805, PPPPPPP; new champion.

The tier gate fully restores test 3. Broader counseling is valid when tier 2
uses full-face rendering. Scale candidate breadth only on indexed tiers 3-4
and keep v11 tier-2 settings fixed.

## Manual tuning handoff

Automated tuning stopped after v11 at the user's request. `tunable.cpp` is a
behavior-preserving readability refactor of the all-pass v11 champion. It
centralizes and names the tier keep schedules, QEM envelope, large/giant
targets, tail timing, star-search controls, and exact-window SSIM budgets. Its
compiled `.text` section is byte-identical to v11 before any values are edited.

Prepared v12/v13 are unsubmitted exploratory files and are not the handoff
baseline. No Batch 6 was launched.

## Batch 6 - Manual tuning by user
I'm copying from from tunable.cpp
I'll start with the T2 and T3 acceptance limit, since that seems to be relevant in previous iterations.

v14: 48 72 -> PPPPPPF 74.114466 (last one randomly fails sometimes, no worries)
v15: 64 128 -> PPPPPPP 90.447805 (exacly same as v11, no difference in the output but larger acceptance limit, not sure if helpful)

Seems like the acceptance limit does not directly impact score much, next I tune the hp_t2KR3, hp_t3KRCore, hp_t4KR3
Start values: 0.30, 0.145, 0.08

v16 0.25, 0.12, 0.06 PPFFFPP 49.170521 (way too aggressive)
v17 0.15, 0.06, 0.03 PPFFFPP 49.170521

v18 0.28, 0.135, 0.07 PPFFFPP 49.170521 still too agressive

v19 0.29, 0.143, 0.075 PPFFFPP 49.170521 suspicious
v20 0.30, 0.145, 0.08  PPPPPPP 90.447805 (back to v11, seems like the previous values were too aggressive)

Leave those, try moving the left-most parameters instead
hp_t2KR1, hp_t3KRSafe, hp_t4KR1
(start) 0.36, 0.16, 0.14

v21 0.35, 0.15, 0.13 -> PPPFFPP 60.837403 Interesting, very sensitive
v22 0.34, 0.14, 0.12 -> PPPFFPP 60.837403 

Seems like these parameters are tuned, let's try the trans parameters instead

hp_t3KR1Trans, hp_t3KR2Trans, hp_t3KR3Trans, hp_t3KR4Trans, hp_t3KR5Trans, hp_t3KR6Trans
(start) 0.125, 0.13, 0.135, 0.1375, 0.14, 0.1425
v23 0.12, 0.12, 0.12, 0.12, 0.12, 0.12 -> PPPFFPP 60.837403

Not sure what these parameters are actually doing. Enough hand-tuning, doesn't seem to be a winning strategy. I'll hand the wheel back to the LLM.