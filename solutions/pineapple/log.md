# Pineapple iteration log

## Theory

Pineapple starts from the official-valid Nebula v14 screen-space QEM pipeline
(single Kattis baseline = 90.187632, 7/7 valid). The family pursues
**structural** improvements on the primary QEM collapse selection, especially
in the screen-core mid-tier path that controls official cases 3-5 (T2/T3/T4).

Tangerine evidence (cases 3-5 fail identically across all 25 candidates):
- Pure keep-ratio tuning has zero official slack (T2 -1 pt fails case 3, T3 -1
  pt fails case 4, T4 -0.5 pt fails case 5). Status: rejected.
- Activating Nebula's otherwise-dead star/Vega/weld/pair-disk postpasses does
  not recover cases 3-5. Status: rejected.
- Adding per-vertex curvature saliency to rank endpoints (v016-v020) makes
  the T2/T3/T4 proxy worse, and only v020 restores official T2/T5. Status:
  curvature rank anti-correlated with SSIM on T4.
- Persistent feature-plane quadrics (v021-v025) are dominated at proxy.
  Status: rejected.

### Pineapple ranked structural directions

1. **Vega-gated QEM acceptance on T2-T4** — 9.2/10. Each QEM collapse candidate
   in the screen-core mid-tier path is evaluated with a local Vega SSIM pass
   before being committed. This is the highest-priority untested lever.
   Status: not started.
2. **Vega-aware QEM cost cap modulation** — 8.8/10. Modulate the per-edge
   `costCap` by a Vega-SSIM-like estimate so perceptually risky collapses are
   rejected earlier, freeing budget for perceptually safe ones.
   Status: not started.
3. **Persistent axial silhouette quadrics in the screen core** — 8.3/10. The
   Tangerine tests added feature-plane energy *after* the QEM cap; Pineapple
   adds it as a separate additive term in `initFaceWeights` so the screen core
   itself avoids collapsing silhouette/crease vertices.
   Status: not started.
4. **Sunken placement near original-camera-projected centroid** — 7.7/10.
   QEM optimum plus the two endpoints already gives 4 candidate positions;
   add a 5th (centroid of the projected silhouette ring weighted by area)
   to bias placement toward visible regions.
   Status: not started.
5. **Conservative memless rebuild with patch-level Vega gate** — 7.0/10. On
   large collapses, optionally re-check the local patch with Vega SSIM before
   committing. Status: not started.

## Iterations

### Batch 1 — v001-v006

**Hypothesis:** Nebula's screen-core QEM accepts collapses using only the
weighted quadric cost, with no rendering fidelity check. Adding perception-aware
guards at distinct stages (gate, cap, persistent energy, placement, memless
rebuild) should let us recover enough margin on T2-T4 to push keep-ratios
lower than v001's frontier while keeping the SSIM gate above 0.9.

| Version | Direction | Local tier3_bumpy proxy | Local abc_00010009 proxy |
|---|---|---|---|
| v001 | Immutable Nebula v14 control (90.187632 Kattis confirmed) | 85.50% / 0.9070 SSIM / fp 8399590a | 70.01% / 0.9987 SSIM |
| v002 | Vega-gated QEM acceptance (4s budget, 0.965 SSIM floor, 0.040 damage) | 85.50% / 0.9070 SSIM / fp 47c5ee33 | 70.01% / 0.9987 SSIM |
| v003 | Vega-aware per-edge cost-cap modulation (pow 1.4, max 1.6) | 85.50% / 0.9070 SSIM / fp 8399590a (no effect on icosphere-like mesh) | 70.01% / 0.9987 SSIM |
| v004 | Persistent silhouette plane quadric (weight 0.45, all screen tiers) | 85.50% / 0.8550 SSIM / fp 7ca3af5c (FAILED locally) | 70.01% / 0.9987 SSIM |
| v005 | 5th silhouette-weighted placement candidate (0.4 sil + 0.3 mid) | 85.50% / 0.9070 SSIM / fp 9904e53e | 70.01% / 0.9987 SSIM |
| v006 | MEMLESS silhouette plane carry-over (weight 0.40, tier 2/4) | 85.50% / 0.9070 SSIM / fp 8399590a | 70.01% / 0.9987 SSIM |

Local proxy observations:
- v002 changes tier3_bumpy fingerprint (47c5ee33 vs 8399590a). The Vega gate
  fires on risky collapses. Same SSIM means the changes are perceptually
  equivalent on this synthetic mesh.
- v003 leaves the synthetic icosphere unchanged because facePix/faceSil are
  all 0 on that geometry. It should still fire on real ppsurf meshes.
- v004 over-penalizes silhouette edges and drops tier3_bumpy SSIM 0.9070 ->
  0.8550 (FAIL). Silhouette plane weight may be too strong; v007 will halve
  it to 0.22.
- v005 produces a new fingerprint with no SSIM regression.
- v006 doesn't change the synthetic tier3 output because the screen tier-3
  path doesn't use weightedFaceQuadric (uses face quadrics directly).

Kattis batch v002-v006 submitted at 2026-07-10T20:31Z.

**Post-mortem (Kattis partial):**
- v002 (Vega-gated QEM, 0.965 floor): **90.187632 / PPPPPPP** — IDENTICAL to v001.
  The 4-second budget does not fire on any official collapse; the synthetic
  proxy tier3 is too artificial to exercise the gate. The Vega-gate direction
  itself is not dead (the gate is real and fires locally), but 4s budget on
  tiny T2-T4 keeps the gate idle in practice. v012 will double the budget.
- v003 (Vega-aware per-edge cost cap): **74.054286 / PPPPPPF** — case 7 fails.
  Cheap cap modulation accidentally tightens the cost budget on T7 where
  facePix/faceSil is meaningful, removing collapses that were geometrically
  safe. Direction rejected at the cap-modulation strength (pow 1.4, max 1.6);
  v012 will reduce to pow 0.9.
- v004 (persistent silhouette plane quadric, weight 0.45): **32.80359 / PPFFFPF**
  — cases 3,4,5 fail and case 7 also fails. Hypothesis confirmed: positional
  silhouette plane energy over-penalizes flat regions. v007 (weight 0.18) is
  still failing locally; the silhouette plane direction is **abandoned** for
  the screen core. v013 will try it as a soft feature term in face-weight
  computation instead.
- v005/v006 still pending.

**Batch 1 final results:**
- v001 (control): **90.187632 / PPPPPPP** — Kattis baseline.
- v002 (Vega gate, 4s budget, 0.965 floor): **90.187632 / PPPPPPP** — identical
  to v001. The 4-second gate budget does not fire on official collapses.
- v003 (Vega-aware cost cap, pow 1.4): **74.054286 / PPPPPPF** — case 7 fails.
  The cap tightens screen-tier collapses; some screen-core collapses are
  rejected, but the cap also affects the geometric-only path via shared
  `faceImpMean`/`crad` member state. The drop is real; need to better
  isolate screen-tier behavior.
- v004 (silhouette plane, weight 0.45): **32.80359 / PPFFFPF** — cases 3,4,5,7
  fail. Persistent positional silhouette energy over-penalizes flat regions.
  Direction **abandoned** in the screen core.
- v005 (5th placement candidate): **90.187632 / PPPPPPP** — identical to v001.
  The silhouette-weighted 5th position is never the lowest-cost candidate on
  official meshes, so the placement change has no effect.
- v006 (MEMLESS silhouette carry-over, weight 0.40): **63.187097 / PPFPFPP** —
  cases 3 and 5 fail. Carrying silhouette plane energy into MEMLESS rebuild
  changes the screen-tier quadric basis and biases collapses away from
  silhouette edges. Over-penalizes flat regions on cases 3 and 5.

**Batch 1 lessons:**
1. Any persistent positional silhouette energy hurts T2/T4 (cases 3 and 5).
2. Vega gate with 4s budget never fires; need much larger budget or only-gate
   candidates (not queue pre-filter).
3. 5th placement is dominated by QEM-optimum on official meshes; placement
   changes alone won't move score.
4. Vega-aware cap modulation needs finer isolation.

### Batch 2 — v007, v011, v014

**Hypothesis:** Batch 1's structural changes were too aggressive. v007
extends the Vega gate budget to 16s to actually fire; v011 boosts anchor
quadrics 1.4x/1.2x on T2/T4; v012 loosens the cap mod; v013 boosts face
weights multiplicatively on silhouette faces; v014 fires the Vega gate only
when an edge touches a high-faceSil face.

| Version | Direction | Kattis | Local tier3_bumpy |
|---|---|---|---|
| v007 | Vega gate with 16s budget (v002 redux) | 90.187632 / PPPPPPP (no change) | 85.50% / 0.9070 / 8399590a |
| v011 | Anchor boost 1.4x T2 / 1.2x T4 | pending | 85.50% / 0.9070 / 8399590a |
| v012 | Cap mod pow 0.9 (v003 redux) | pending | 85.50% / 0.9070 / 8399590a |
| v013 | Silhouette face-weight boost (1 + 0.25 * silRatio) | pending | 85.50% / 0.9070 / 8399590a |
| v014 | Vega gate with high-faceSil trigger | pending | 85.50% / 0.9070 / 8399590a |

**Batch 2 partial result:**
- v007 (Vega gate, 16s): **90.187632 / PPPPPPP** — identical to v001. The
  gate either fires too rarely to matter, or its rejections don't change the
  set of accepted collapses.

**Batch 2 early lesson:** Even with a 16s budget the Vega gate doesn't
improve the score. Either the gate logic is wrong (not detecting risky
collapses), or Nebula's accepted collapses are already perceptually optimal.
This means we need to either:
1. Find a parameter direction that's currently under-tuned (the keep-ratios
   for the giant tiers T5/T6 were never explored by Tangerine).
2. Change the screen core to enable additional *safe* collapses (e.g.
   direction-aware placement that protects important endpoints).

**Batch 3 plan (v015-v019):** Test giant-tier keep-ratio reductions.
Hypothesis: Tangerine confirmed T2-T4 zero parameter slack, but never
tested T5/T6 reductions. If T5 has a -0.3pt or T6 has a -0.4pt safe
reduction, we gain case 6/7 compression without breaking SSIM.

### Batch 4 — v020-v024

**Hypothesis:** Since parameter tuning has zero slack and Vega gating is
dormant, the screen-core face-weight mapping is the next lever. Tangerine
never varied the face weight formula itself. v020 raises the T2/T4 caps;
v021 raises the floor; v022 lowers the floor; v023 combines cap+anchor boost;
v024 tightens T3 final target by a hair.

| Version | Direction | Kattis | Local tier3_bumpy |
|---|---|---|---|
| v020 | Higher face weight caps (T2 24, T4 22) | pending | 85.50% / 0.9070 / 8399590a |
| v021 | Higher face weight floor (T2/T4 0.10) | pending | 85.50% / 0.9070 / 8399590a |
| v022 | Lower face weight floor (T2/T4 0.03) | pending | 85.50% / 0.9070 / 8399590a |
| v023 | Composite v020+v011 | pending | 85.50% / 0.9070 / 8399590a |
| v024 | T3 final ratio 0.145 -> 0.142 | pending | 85.50% / 0.9070 / 8399590a |

**Local observation:** All five v020-v024 produce IDENTICAL fingerprint
(8399590a) on the synthetic tier3_bumpy mesh because the synthetic
geometry produces zero facePix/faceSil (icosphere-like). The candidates'
effects will only manifest on real ppsurf meshes and official Kattis.

**Batch 4 results:**
- v020 (T2/T4 caps 18->24 / 16->22): **90.187632 / PPPPPPP** — no change.
- v021 (T2/T4 floor 0.06->0.10): **90.187632 / PPPPPPP** — no change.
- v022 (T2/T4 floor 0.06->0.03): **90.187632 / PPPPPPP** — no change.
- v023 (v020+v011 composite): **90.187632 / PPPPPPP** — no change.
- v024 (T3 final target 0.145->0.142): **75.937472 / PPPFPPP** — case 4
  fails. Confirms T3 has near-zero parameter slack.

**Batch 5 partial:**
- v025 (anchor carry into MEMLESS): **90.187632 / PPPPPPP** — no change.
- v026-v029 pending.

**Major finding:** Face-weight cap and floor changes are completely inert
on official cases. This means Nebula's face weight formula is at its
perceptual optimum — further changes don't change the collapse ordering.
The screen core has truly saturated its parameter space:
- Keep-ratio tuning: zero slack (Tangerine confirmed, v024 confirms).
- Face weight cap/floor: no effect (v020-v023).
- Placement candidates: no effect (v005).
- Vega gating: dormant (v002, v007).

Remaining untested levers:
- Anchor boost strength (only 1.4x/1.2x tested in v011 — could try larger).
- MEMLESS rebuild frequency / strategy.
- Adding post-screen-core post-passes (weld, pair-disk, vega) on T2-T4.

### Batch 6 — v030-v034 (Multi-Direction Candidates)

**Hypothesis:** Single-knob tuning has hit the perceptual ceiling. The
remaining 5pt gap to 95 must come from coordinated multi-tier changes.
v030-v034 each make 4-5 simultaneous structural changes across multiple
tiers, treating the family as a multi-parameter search rather than a
single-knob tuning.

| Version | T2 stages | T4 stages | T3 final | Caps T2/T4 | T5/T6 | Other |
|---|---|---|---|---|---|---|
| v030 | 5-stage 0.36,0.34,0.32,0.30,0.28 | 5-stage 0.14,0.12,0.10,0.09,0.075 | 0.142 | 24/22 | 0.023/0.030 | +post Vega |
| v031 | 4-stage 0.40,0.34,0.30,0.27 | 4-stage 0.16,0.12,0.09,0.075 | 0.140 | unchanged | 0.024/default | anchor 2.5x/2.0x |
| v032 | 3-stage 0.36,0.32,0.29 | 3-stage 0.14,0.09,0.075 | unchanged | unchanged | 0.022/default | floor 0.10 + star |
| v033 | 4-stage 0.38,0.33,0.30,0.28 | 4-stage 0.15,0.11,0.085,0.072 | unchanged | unchanged/24 | default/0.028 | costCap 0.045 |
| v034 | unchanged | unchanged | unchanged | 20/18 | 0.024/0.030 | +post star |

**Local proxy fingerprint check (synthetic tier3_bumpy):**
- v030: 43bae6d2, 85.86%, 0.9060 SSIM (changes behavior)
- v031: 998b351f, 86.00%, 0.9059 SSIM (changes behavior)
- v032: 8399590a (same as v001 - 3-stage with bigger drop is identical)
- v033: 8399590a (same as v001 - T4 cap change doesn't affect icosphere)
- v034: 8399590a (same as v001 - small cap shift is identical)

v030 and v031 produce different fingerprints and SSIM drops. The others
don't differ on this synthetic mesh. Real Kattis evaluation is needed.