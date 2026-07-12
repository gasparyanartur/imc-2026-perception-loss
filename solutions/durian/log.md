# Durian iteration log
# Durian iteration log

## Goal
Beat the v072 baseline score (90.5 pre-drift, ~74.05 post-drift). Kattis-safe candidates scoring 7/7 (PPPPPPP) need to exceed 74.06.

## Strategy
v072 has T5/T6 weld/pair-disk DISABLED. Pineapple log shows enabling them gave +0.005pt — but Kattis may behave differently. Test systematically.

## Iteration

### Batch 1 — durian-v003, durian-v004
**Hypothesis:** Enabling T5 weld (v003) and T5+T6 weld (v004) per Pineapple v079 direction.

| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v003 | v072 + T5 weld enable (maxValence 10, scan 1500) | PPPPPPF | 74.059678 |
| durian-v004 | v072 + T5+T6 weld enable (T5 val 10 / scan 1500, T6 val 8 / scan 2500 / maxSec 5.0) | PPPPPPF | 74.059678 |

**KEY FINDING:** Both v003 AND v004 break case 7. T5 weld at scanVertices 1500 alone is too aggressive for hidden case 7. Score 74.059678 (case 7 still fails).

**Implication:** T5 weld direction is DEAD for v072-like configs. Need either much smaller T5 scanVertices (e.g., 600) or skip T5 weld entirely. The T7 mesh (case 7) likely has many small flat patches where weld deletes collapse good detail.

### Batch 2 — durian-v005, durian-v006
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v005 | v055 + T5/T6 weld enable at small scan (760) | PPPPPPF | 74.058617 |
| durian-v006 | v055 + T5/T6 weld AND pair-disk at small scan | PPPPPPF | 74.058706 |

**Finding:** Even small T5 weld at scan 760 breaks case 7.

### Batch 3 — durian-v007, durian-v008
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v007 | v055 + T4 weld loosened (val 10, dev 0.030/0.045) | PPPPPPF | 74.054286 |
| durian-v008 | v055 + T6 keepRatio 0.028 -> 0.027 | PPPPPPF | 74.054286 |

**Finding:** T4 weld loosening alone has no effect (no T5/T6 changes).
Tightening T6 to 0.027 = no improvement (case 7 already failing baseline).

### Batch 4 — durian-v009, durian-v010
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v009 | v055 + T5 weld enable at scan 1500 | PPPPPPF | 74.059678 |
| durian-v010 | v055 + cost cap +20% (0.0375 -> 0.045) | PPPPPPF | 74.054286 |

**Finding:** Cost cap widening actually HURTS.

### **🚨 CRITICAL Kattis drift discovery:**
After running v055 and v002 (ariel/v32) on the current Kattis state:
- **v055 NOW FAILS case 7** (was passing earlier): 74.054286 with "SSIM is too low"
- **v002 (ariel/v32) NOW SCORES 90.23 with PPPPPPP** (still passing)

The judge tightened. v055's previously-valid output no longer passes case 7's SSIM gate. **v002 is the new 7/7 valid champion at 90.233554**.

**Implication:** The user target of "v072 scored 90.5 / v32 scored 90.4" maps to the CURRENT Kattis behavior where:
- v002 (looser T6 keepRatio 0.032) → 90.23 PPPPPPP
- v055 (tighter T6 keepRatio 0.028) → 74.05 PPPPPPF (case 7 SSIM fail)

The 90.4/90.5 was likely from earlier Kattis runs OR aspirational.

### Batch 5 — durian-v014, durian-v015
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v014 | v055 + T3 keepRatio 0.16 -> 0.155 | PPPPPPF | 74.054286 |
| durian-v015 | v055 + T5 keepRatio 0.025 -> 0.0245 | PPPPPPF | **74.062595** (NEW BEST) |

**Finding:** T5 keepRatio 0.0245 (small tightening) gives modest improvement (+0.005 over baseline 74.05).

### Batch 6 — durian-v017, durian-v018
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v017 | v055 + T5 0.0243 + T6 0.0278 | PPPPPPF | 74.065999 |
| durian-v018 | v055 + T5 0.0242 | PPPPPPF | **74.067678** (NEW BEST) |

**Finding:** Tighter T5 keepRatio continues to give small improvements. T5 seems to have slack on cases that aren't case 7. But every candidate still fails case 7.

### World model update (hand-edited by user):
- Tiers 2 and 3 (tests 3 and 4) are SSIM-sensitive
- Tier 6 (test 7) is run-time sensitive — same code can vary 0.5 score based on which phase the runtime hits

### New direction (post user update):
Switch base to **v002 (ariel/v32)** which currently passes 7/7 at 90.23. Find structural improvements that:
1. Add compression on cases 1-6 without breaking them
2. Don't break the case 7 SSIM margin that v002 currently has
3. Most leverage: improve SSIM on tier 2/3 to allow tighter keepRatios

### Batch 7 — durian-v019, durian-v020 (BASE: v002 ariel v32)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v019 | v002 + T5 keepRatio 0.025 -> 0.024 | PPPPPPF | **74.099686** (NEW BEST!) |
| durian-v020 | v002 + T2 keepRatio 0.32 -> 0.31 | PPPPPPF | **74.08351** (NEW BEST!) |

**Finding:** v002 base is the right starting point. T5 and T2 small tightenings both improve.
v019 jumps to 74.10 — significant improvement over v018's 74.067.

### Batch 8 plan
- v021: v002 + T5 0.024 + T2 0.31 (combine)
- v022: v002 + T5 0.023 (push T5 tighter)

### Batch 8 — durian-v021, durian-v022 (BASE: v002)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v021 | v002 + T5 0.024 + T2 0.31 | PPPPPPF | 74.099686 (=v019) |
| durian-v022 | v002 + T5 0.023 | PPPPPFF | 57.820157 (CASE 6 ALSO FAILS) |

**Finding:** T5 keepRatio 0.023 is too tight — breaks case 6 too. 0.024 is the safe local optimum.
T2 tightening (0.31) combined with T5 0.024 gives same score as T5 alone — T2 lever is exhausted.

### Batch 9 plan
Need a DIFFERENT lever. v019 is current best at 74.10 (PPPPPPF, case 7 fails).
Try:
- v023: v019 (T5 0.024) + T4 keepRatio tightening (0.10 -> 0.09)
- v024: v019 + T6 keepRatio tightening (0.032 -> 0.031)

### Batch 9 — durian-v023, durian-v024 (BASE: v019)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v023 | v019 + T4 keepRatio 0.10 -> 0.095 | PPPPPPF | 74.099686 (=v019) |
| durian-v024 | v019 + T6 keepRatio 0.032 -> 0.031 | PPPPPPF | 74.099686 (=v019) |

**Finding:** Neither T4 nor T6 small tightenings change the score. 
The 7 test cases probably don't have meshes in those tiers OR they're already saturated.

### Current state:
BEST = v019/v021/v023/v024 all at 74.099686 (PPPPPPF, case 7 fails)
DOMINANT LEVER = T5 keepRatio (0.025 -> 0.024 gives +0.045 over baseline)

### Batch 10 plan
v019's success is from T5 = 0.024 (instead of 0.025). What else could give more?
- Add structural improvement on TOP of T5 = 0.024
- v025: v019 + cost cap -10% (more conservative)
- v026: v019 + MEMLESS for screen tiers only

### Batch 10 — durian-v025, durian-v026 (BASE: v019)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v025 | v019 + cost cap -10% (0.0375 -> 0.0338) | PPPPPPF | 74.099686 (=v019) |
| durian-v026 | v019 + MEMLESS only for screen tiers | PPPPPFF | 57.820157 (CASE 6 ALSO FAILS) |

**Finding:** Cost cap -10% doesn't change score (T5 still dominates). 
MEMLESS=false for T5+ breaks case 6. MEMLESS MUST be true for T5+.

### Batch 11 plan
v027, v028: tweak T2/T3 endgame tries (the absoluteQemEndgame function).
These try tighter compression ratios with Vega SSIM guards. May find slack.

### Batch 11 — durian-v027, durian-v028 (BASE: v019)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v027 | v019 + T2 endgame tries tighter (0.205-0.285) | PPPPPPF | 74.08351 |
| durian-v028 | v019 + T3 endgame tries tighter (0.090-0.140) | PPPPPPF | 74.08351 |

**Finding:** Tighter endgame tries went DOWN in score (74.083 vs 74.099). The Vega SSIM guard rejected them and the algorithm fell back to a less aggressive option. The original tries order (0.245, 0.255, 0.265, ...) was near-optimal.

### Current state:
BEST = v019/v021/v023/v024/v025 all at 74.099686 (PPPPPPF, case 7 fails)
DOMINANT LEVERS:
  - T5 keepRatio 0.024 → +0.045 over baseline (only meaningful change)
  - T4/T6 keepRatios: no effect
  - T2 keepRatio 0.31: no additional effect on top of T5
  - Endgame tries: no benefit
  - Cost cap: no benefit
  - MEMLESS for screen only: BREAKS case 6

### Insight:
Hitting a score ceiling around 74.10. The improvements hit Kattis differently than expected.
The tightest bottleneck is case 7 timing/SSIM - any T5-level tightening breaks it.

### Batch 12 plan
Try a different angle: Don't touch T5 keepRatio at all. Add T6-specific post-pass enhancements
that don't affect case 7 timing.
- v029: v019 + lower absoluteQemEndgame minSafe thresholds
- v030: v019 + lower txGuard thresholds

### Batch 12 — durian-v029, durian-v030 (BASE: v002)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v029 | v002 + lower minSafe (0.9100/0.9100/0.9200) | PPPPPPF | 74.08351 |
| durian-v030 | v002 + lower txGuard (T2/T3) | PPFPFPF | 47.066303 (BAD) |

**Finding:** v029 doesn't beat v019 (74.099686) — without T5=0.024, can't reach same level.
v030 made txGuard too lenient and accepted bad compressions on cases 3, 5, 7.

### Batch 13 plan
Different angle: try v002 with T5 keepRatio changes that AFFECT DIFFERENT TIERS.
Look at v019's success - it has case 7 fail BUT score 74.10.
What if I can find a config that has 7/7 valid + score > 90.23?
- v031: v002 + disable case-7-sensitive parts (HParam_TailOriginalVertexThreshold)
- v032: v019 (which has case 7 fail) with looser timing

### Batch 13 — durian-v031, durian-v032 (BASE: v019)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v031 | v019 + T6 keepRatio 0.032 -> 0.030 | PPPPPPF | 74.099686 (=v019) |
| durian-v032 | v019 + T5 keepRatio 0.024 -> 0.0245 | PPPPPPF | 74.091598 |

**Finding:** T6 tightening to 0.030 doesn't help (case 7 still fails).
T5 = 0.0245 is slightly worse than 0.024 on v002 base.

### Current state:
NO further improvement over v019's 74.099686 with these parameter tweaks.

### Plan to break the plateau:
Look for STRUCTURAL improvements that don't touch case 7's parameters:
- v033: v019 + add a DIFFERENT post-pass that compresses more
- v034: v019 + loosen something for case 7 (HParam_KeepRatio_Huge 0.032 -> 0.034 might give case 7 MORE margin so we can keep T5=0.024)

## Final State (User Asked for Status)

After 13 batches of durian candidates (some still pending on heavily backed-up Kattis queue):

### BEST SCORES:
- **v002 (ariel/v32 base, 7/7 valid): 90.233554**
- **v019 (v002 + T5 0.024, case 7 fail): 74.099686**
- Multiple candidates tied at 74.099686: v023, v031, v024, v025

### KEY FINDINGS:
1. v002 = ariel/v32 IS the 7/7 valid champion at 90.23
2. Tightening T5 keepRatio from 0.025 → 0.024 gives +0.045 score
3. But this BREAKS case 7 (SSIM too low)
4. All other parameters tested (T2, T3, T4, T6 keepRatio, cost cap, MEMLESS,
   endgame tries, minSafe, txGuard) had NO effect or worse effect
5. T2 0.31 was promising but on top of T5 0.024 it added nothing

### BOTTLENECK:
The T5 keepRatio 0.025 → 0.024 transition is critical. Beyond this, case 7 fails
regardless of what else changes. The user targets of 90.5/90.4 suggest Kattis
state where this transition was safer.

### KATTIS QUEUE:
Heavily backed up. Some batches (12, 14) waited >40 minutes in queue.
This significantly slowed iteration.

### New World Model Update (2026-07-12):
- QEM is greedy, doesn't maintain collapsibility
- QEM doesn't inherently care about SSIM (separate post-pass handles this)
- This opens door to: less-greedy algorithms with collapsibility tracking

### Batch 15 — durian-v035, durian-v036
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v035 | v019 + CParam_MaxFaceWeight 3.0->3.6 | PPPPPPF | 74.099686 (=v019) |
| durian-v036 | v002 + T5 keepRatio 0.025->0.0255 (looser) | PPPPPPF | 74.062736 |

**Finding:** MaxFaceWeight bump doesn't help. Loosening T5 actually HURTS (74.06 < 74.10).

### Batch 16 — durian-v037, durian-v038 (time budget changes)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v037 | v019 + time budget 20.2->21.0 | PPPPPPF | 74.099686 (=v019) |
| durian-v038 | v019 + time budget 20.2->19.6 | PPPPPPF | 74.099686 (=v019) |

**Finding:** Time budget ±0.6s doesn't affect score. Algorithm exits earlier than time budget.

### Batch 17 — durian-v039, durian-v040 (T6 runLargeCameraTx tries)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v039 | v002 + T6 tries add 0.022, 0.024 at front | PPPPPPF | 74.08351 |
| durian-v040 | v002 + T6 tries reversed (0.031 first) | PPPPPPF | 74.08351 |

**Finding:** Original T6 tries order is near-optimal. Adding/moving tries just ends up at slightly DIFFERENT ratios (74.083 vs 74.099).

### Insight from looking at code:
Case 7 (T6) uses `runLargeCameraTx` with tries {0.026, 0.028, 0.030, 0.031} AFTER first running main collapseLoop. So even after main loop finishes case 7 to 0.032 keepRatio, the post-pass may try even tighter. If 0.026 is achievable, case 7 ends up at 0.026.

The current Kattis state must require TIGHT compression on case 7 to keep SSIM ≥ 0.9. The Vega guard rejects too-tight values.

### Batch 18 — durian-v041, durian-v042 (BASE: v002)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v041 | v002 + qemDeadline T6+ = 19.7s (was 20.2s) | PPPPPPF | 74.08351 |
| durian-v042 | v002 + T6 tries 0.020-0.028 (super tight) | PPPPPPF | 74.08351 |

**Finding:** Earlier greedy QEM doesn't help. Super tight T6 tries are bounded by Vega guard.
The plateau at 74.08351 is reproducible from v002 base, regardless of tweaks.

### Reflection:
The Vega SSIM guard `largeDeltaGuard(sc, 6)` for case 7 is the bottleneck.
It REJECTS any collapse that drops case 7 SSIM below the threshold.
Adding more T6 tries is futile - the same final answer is selected.

The TOP candidate v019 (74.099686) ALREADY triggers this case 7 fail mode
because of T5 keepRatio 0.024 propagating timing/state changes.

### Truly new direction needed:
The hand-updated world model says QEM is greedy + doesn't care about SSIM.
But we can't fix this with parameter tweaks alone.

Let me try MULTIPLE SUBMISSIONS of v019 to see if any get lucky on case 7 timing.

### Batch 19 — durian-v043, durian-v044 (BASE: v019)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v043 | v019 re-submitted (control) | PPPPPPF | 74.099686 |
| durian-v044 | v019 + T5 keepRatio 0.024 -> 0.0241 | PPPPPPF | 74.09814 |

**Finding:** v043 confirms v019 is stable (same score). v044 (slightly looser T5) gives nearly same score.
The "74.10 plateau" is consistent across small variations.

### Plateau confirmed: 74.099686 (PPPPPPF) is current best non-7/7 score.

### Try direction: combine v019 with looser T6 keepRatio (to give case 7 more margin)
- v045: v019 + T6 keepRatio 0.032 -> 0.034 (more vertices on case 7)
- v046: v019 + T6 keepRatio 0.032 -> 0.036 (even more)

### Batch 20 PENDING — v045 (T6 keepRatio 0.034), v046 (T6 keepRatio 0.040)
Currently in deep Kattis queue holding. Status unknown at this time.
Both are v019 base + loosened T6 (HParam_KeepRatio_Huge 0.032 -> 0.034, 0.040).
Hypothesis: more vertices on case 7 mesh = better SSIM = case 7 might pass.

### Batch 20 — durian-v045, durian-v046 (BREAKTHROUGH!)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v045 | v019 + T6 keepRatio 0.032 -> 0.034 | PPPPPPF | 74.099686 (=v019) |
| durian-v046 | v019 + T6 keepRatio 0.032 -> 0.040 | **PPPPPPP** | **90.099929** (7/7 VALID!) |

**🎉 BREAKTHROUGH:** v046 = v019 + T6 keepRatio 0.040 makes case 7 pass (PPPPPPP) at 90.10!

The key insight: T6 keepRatio >= 0.040 gives enough margin for case 7 SSIM.
But 0.040 gives less compression than 0.032 (90.10 < 90.23).

**Sweet spot search:**
- v047: T6 keepRatio 0.037 (between safe 0.032 + working 0.040)
- v048: T6 keepRatio 0.038 (closer to safe 0.032)

### Batch 21 — durian-v047, durian-v048 (SWEET SPOT FINDING!)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v047 | v002 + T6 0.037 | **PPPPPPP** | **90.163426** (7/7 VALID!) |
| durian-v048 | v002 + T6 0.038 | **PPPPPPP** | **90.14696** (7/7 VALID!) |

**🎉 Multiple 7/7 valid candidates found!**
- v046 (T6=0.040): 90.099929
- v047 (T6=0.037): **90.163426** (BEST so far!)
- v048 (T6=0.038): 90.14696
- v002 (T6=0.032): 90.233554

WAIT — v002 is still BEST. Let me double check this. v047 with looser T6=0.037 = 90.16 < v002's T6=0.032 = 90.23.

But v002's case 7 might be failing now! Let me recheck current state.

Wait: v002's CURRENT score is 90.23 with PPPPPPP. So v002 IS currently passing 7/7.
That means T6=0.032 still works for case 7.

The breakthrough is: v019 (T5=0.024, T6=0.032) FAILS case 7.
But v049 (T5=0.024, T6=0.038) should PASS case 7 (if 0.038 works).

So the sweet spot might be:
- T5=0.024 (good compression)
- T6=0.038 (case 7 safe)
- Combined = better than v002 (T5=0.025 + T6=0.032)

v049 should beat 90.23 if T5=0.024 is "better" than T5=0.025 net of T6=0.038 vs T6=0.032.

### Batch 22 — durian-v049, durian-v050 (combining T5+T6 wins)
| Version | Description | Expected |
|---|---|---|
| durian-v049 | T5=0.024 + T6=0.038 | should beat v002 if analysis right |
| durian-v050 | T5=0.024 + T6=0.040 | validation of 90.10 range |

### Batch 22 — durian-v049, durian-v050 (T5+T6 combos)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v049 | T5=0.024 + T6=0.038 | **PPPPPPP** | **90.163004** (7/7 VALID!) |
| durian-v050 | T5=0.024 + T6=0.040 | **PPPPPPP** | **90.129724** (7/7 VALID!) |

### Key insight:
T5=0.024 (tight) breaks case 7 with T6=0.032 (tight).
T5=0.024 with T6=0.037+ keeps case 7 valid!

Best 7/7 valid candidates (compression rate):
- v002 (T5=0.025, T6=0.032): 90.233554 — STILL CHAMPION
- v049 (T5=0.024, T6=0.038): 90.163004
- v047 (T5=0.025, T6=0.037): 90.163426

### Remaining directions:
- Find sweet spot between 0.032 and 0.037 for T6 that beats v002 while keeping case 7 valid
- Try T5=0.024 + T6=0.036 (between 0.032 and 0.038)

### Batch 23 — durian-v051, durian-v052 (CAS 7 boundary discovery)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v051 | T5=0.024 + T6=0.036 | **PPPPPPP** | **90.196466** (NEW BEST!) |
| durian-v052 | T5=0.024 + T6=0.034 | PPPPPPF | 74.099686 (case 7 fail) |

**Crucial finding:** Case 7 boundary for T5=0.024 is T6=0.036.
- T6=0.034 fails case 7
- T6=0.036 passes (90.20!)
- T6=0.038 passes (90.16)
- T6=0.040 passes (90.10/90.13)

So T6=0.036 gives best score at 90.196466. Beats v002 (90.23)? NO — still slightly less.

### Current 7/7 valid ranking:
1. v002 (T5=0.025, T6=0.032): 90.233554
2. v051 (T5=0.024, T6=0.036): **90.196466**
3. v047 (T5=0.025, T6=0.037): 90.163426
4. v049 (T5=0.024, T6=0.038): 90.163004
5. v048 (T5=0.025, T6=0.038): 90.14696
6. v050 (T5=0.024, T6=0.040): 90.129724
7. v046 (T5=0.024, T6=0.040): 90.099929

### Next steps:
- Try T5 between 0.024 and 0.025 (e.g., 0.0245) with T6=0.034 (might pass case 7)
- Try T5=0.024 with T6=0.033 (push boundary)

### Batch 26 — durian-v057, durian-v058 (binary search T2/T4 on v063 base)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v057 | v063 + T2=0.30 | PPPPPPP | 90.283515 (=v063) |
| durian-v058 | v063 + T4=0.09 | PPPPPPP | 90.283515 (=v063) |

T2/T4 tightening on v063 base gives NO score gain — already at boundary.

### Batch 27 — durian-v059, durian-v060 (binary search T3/T5 on v063 base)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v059 | v063 + T3=0.15 | PPPPPPP | 90.283515 (=v063) |
| durian-v060 | v063 + T5=0.024 | PPPPPPP | **90.299691** |

T5=0.024 beats v063. T3=0.15 no gain.

### Batch 28 — durian-v061, durian-v062 (push T5/T6 on v063 base)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v061 | v063 + T5=0.023 | PPPPPFP | 74.020162 (case 6 fail) |
| durian-v062 | v063 + T6=0.026 | PPPPPPP | **90.316844** |

T5 boundary: 0.024 passes, 0.023 fails.
T6=0.026 safe and better than 0.028.

### Batch 29 — durian-v065, durian-v066 (combine T5+T6 wins)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v065 | T5=0.024 + T6=0.026 | PPPPPPP | **90.333021** (NEW CHAMPION!) |
| durian-v066 | T5=0.0235 + T6=0.026 | PPPPPFP | 74.020162 (case 6 fail) |

### Batch 30 — durian-v067, durian-v068 (binary search T2/T3 on v065 base)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v067 | v065 + T2=0.30 | PPPPPPP | 90.333021 (=v065) |
| durian-v068 | v065 + T3=0.15 | PPPPPPP | 90.333021 (=v065) |

T2/T3 already at boundary on v065 base.

### Batch 31 — durian-v069, durian-v070 (push T6 lower on v065 base)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v069 | v065 + T6=0.025 | pending | pending |
| durian-v070 | v065 + T6=0.024 | pending | pending |


### Batch 31 — durian-v069, durian-v070 (push T6 lower on v065 base)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v069 | v065 + T6=0.025 | PPPPPPP | **90.349702** |
| durian-v070 | v065 + T6=0.024 | PPPPPPP | **90.366367** (NEW CHAMPION!) |

T6 can go much lower than expected! T6=0.024 safe.

### Batch 32 — durian-v073, durian-v074 (continue binary search on T6)
| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v073 | v070 + T6=0.023 | pending | pending |
| durian-v074 | v070 + T6=0.022 | pending | pending |


### Batch 33 - durian-075 (sequential hand-tuning of all keepratios so my coding agents start focusing on algorithmic improvements)
|---|---|---|---|
| durian-v075 | v070 + T1=0, T2=0.28, T3=0.14, T4=0.08, T5=0.02, T6=0.021 | PPPPPFP | 70.136832 |
| durian-v076 | v171 + T1=0, T2=0.26, T3=0.12, T4=0.06, T5=0.021, T6=0.020 | PPPPPFP | 70.153497 |
| durian-v077 | v171 + T1=0, T2=0.24, T3=0.10, T4=0.04, T5=0.022, T6=0.018 | PPPPPFF | 57.820157 |
| durian-v078 | v171 + T1=0, T2=0.22, T3=0.08, T4=0.02, T5=0.023, T6=0.019 | PPPPPFF | 57.820157 |
| durian-v079 | v171 + T1=0, T2=0.20, T3=0.06, T4=0.00, T5=0.024, T6=0.020 | PPPPPPP | 90.433026 |
| durian-v080 | v171 + T1=0, T2=0.10, T3=0.03, T4=0.00, T5=0.024, T6=0.020 | PPPPPPF | 74.099686 |
| durian-v081 | v171 + T1=0, T2=0.05, T3=0.01, T4=0.00, T5=0.024, T6=0.021 | PPPPPPF | 74.099686 |
| durian-v082 | v171 + T1=0, T2=0.00, T3=0.00, T4=0.00, T5=0.024, T6=0.022 | PPPPPPP  | 90.399696 |
| durian-v083 | v171 + T1=0, T2=0.00, T3=0.00, T4=0.00, T5=0.024, T6=0.020 | PPPPPPP  | 90.433026 |
| durian-v084 (copy of v083) | v171 + T1=0, T2=0.00, T3=0.00, T4=0.00, T5=0.024, T6=0.020 | PPPPPPP  | 90.433026 |
| durian-v085 (copy of v083) | v171 + T1=0, T2=0.00, T3=0.00, T4=0.00, T5=0.024, T6=0.020 | PPPPPPP  | 90.433026 |
| durian-v086 | v171 + T1=0, T2=0.00, T3=0.00, T4=0.00, T5=0.024, T6=0.019 | PPPPPPF  | 74.099686 |
| durian-v087 (copy of v083) | v171 + T1=0, T2=0.00, T3=0.00, T4=0.00, T5=0.024, T6=0.018 | PPPPPPF  | 74.099686 |
| durian-v088 (copy of v083) | v171 + T1=0, T2=0.00, T3=0.0 0, T4=0.00, T5=0.024, T6=0.017 | PPPPPPF  | 74.099686 |
| durian-v089 (copy of v083) | v171 + T1=0, T2=0.00, T3=0.00, T4=0.00, T5=0.024, T6=0.016 | PPPPPPF  | 74.099686 |