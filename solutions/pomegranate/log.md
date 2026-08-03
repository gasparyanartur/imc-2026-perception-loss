# Pomegranate iteration log

## Theory

Pomegranate starts from Tranberry v150, the final all-pass QEM-only structural
control. The family changes the connectivity basin before simplification with
bounded, manifold-safe edge flips. It does not globally reorder QEM collapses.
The intended mechanism is to reduce valence defects and skinny local fans so
that later QEM retains more legal, low-error collapses at the same perceptual
damage. Tier-scaled work caps apply the mechanism to every input tier while
keeping the giant-tier schedule bounded.

## Batch 1 — v001-v002 (local diagnostics complete; Kattis not launched)

**Hypothesis:** A small conflict-free round of curvature-safe diagonal flips
will unlock better collapse sequences without changing vertex count or moving
vertices. A conservative valence-first profile should establish safety; a
broader diagonal-shortening/quality profile tests whether more graph diversity
improves the fixed-count render frontier.

| Version | Structural change | Mean compression (24) | Mean SSIM | Mean Hausdorff use | Topology |
|---|---|---:|---:|---:|---|
| v001 | Nearly coplanar flips; require valence-defect gain | 90.115179% | 0.852669 | 22.772229% | 0 defects on all scenarios |
| v002 | Broader curvature-safe flips; allow strong quality + shorter diagonal | 90.117263% | 0.853144 | 22.672712% | 0 defects on all scenarios |

Both candidates were evaluated exactly once with default, synthetic, and stress
fixtures. v001 measured 92.0% compression and 0.6999 SSIM on the 48k boundary
fixture. v002 produced the same boundary fingerprint and metrics, so the broader
profile did not survive into a distinct end-state there. It did improve several
small proxy outcomes and mean metrics slightly. Both preserved the exact same
1.1M stress fingerprint (97.8001% compression, 0.9951 SSIM), which is evidence
that the bounded prefix scan does not disturb the proven giant-tier output on
the available stress mesh.

**Post-mortem / next hypothesis:** The first batch establishes local topology
safety but does not yet demonstrate the desired lower-count continuation.
Pre-QEM flips are often erased by subsequent greedy collapses, especially on the
48k frontier. If official results are score-neutral, move flips into the strict
endgame after the safe 8% snapshot, then evaluate flip-collapse pairs by
original-reference render damage and future legal-collapse count. That is a
more direct test of the topology-basin theory than broadening preprocessing.

Kattis status: Batch 1 terminal; v001/v002 both `PPFPPPP`.


### Batch 1 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v001 | `PPFPPPP` | 78.732726 |
| v002 | `PPFPPPP` | 78.732637 |

Both profiles fail only test 3. Tests 4-7, including the timing-sensitive giant
case, pass. The nearly identical scores show that the broad versus conservative
flip distinction is not the official lever; the active issue is that even a
small pre-QEM connectivity change in the <=25k regime crosses its SSIM margin.
Next batch sharply isolates that tier with only ultra-coplanar, tiny-budget
flips while retaining bounded structural changes in every other tier.


## Batch 2 — v003-v004

**Hypothesis:** Test 3 fails because ordinary pre-QEM flips cross a very narrow
SSIM boundary. Ultra-coplanar flips with one-to-four accepted operations can
retain a meaningful topology-basin perturbation without disturbing that case;
every other tier simultaneously receives a distinct retuned scan, acceptance,
normal-alignment, and valence budget.

| Version | Tier-2 profile | Mean compression (24) | Mean SSIM | Mean Hausdorff use | Topology |
|---|---|---:|---:|---:|---|
| v003 | <=4 flips, dot >=0.9999, valence gain >=6 | 90.116794% | 0.852164 | 22.712308% | 0 defects |
| v004 | <=1 flip, dot >=0.99999, valence gain >=10 | 90.115895% | 0.852394 | 22.687542% | 0 defects |

Both retained the 48k boundary and 1.1M stress fingerprints. Local default
fingerprints confirm that the tiny-tier outputs still change, so these are not
byte-identical controls. Kattis test 3 is the decisive signal.


### Batch 2 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v003 | `PPPPPPP` | 90.399652 |
| v004 | `PPPPPPP` | 90.387586 |

Ultra-small tier-2 budgets restore validity. v003 is effectively count-neutral
relative to Tranberry v150; v004 loses a small amount, likely through timing or
different small-tier collapse basins. Pre-QEM flips are rejected as the primary
Pomegranate mechanism. The next batch moves graph repair to the 45k-50k safe
snapshot and tests whether it supports a 6% or 4% continuation.


## Batch 3 — v005-v006

**Hypothesis:** Connectivity repair must occur after the safe 8% snapshot to
survive QEM. Conservative endgame flips may support a 6% continuation; broader
flips test the already-known 4% lower-count basin.

| Version | Boundary continuation | Boundary local result | Mean compression (24) | Topology |
|---|---|---|---:|---|
| v005 | conservative flips then 6% | 94.0%, SSIM 0.6784 | 90.200128% | 0 defects |
| v006 | broad flips then 4% | 96.0%, SSIM 0.6539 | 90.282562% | 0 defects |

The mechanism is active and reaches both intended counts. Fidelity declines
monotonically on the synthetic boundary proxy, but local SSIM is not the
official oracle; both immutable candidates proceed to Kattis.


### Batch 3 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v005 | `PPPPFPP` | 75.049328 |
| v006 | `PPPPFPP` | 75.037261 |

Both fail only test 5. Six percent is already beyond the hidden SSIM boundary,
so the missing ingredient is survivor distribution rather than additional graph
unlocking. Pure edge-flip continuation is stopped. Next: diffuse curvature over
patch neighborhoods and couple regional density to rendered silhouette support.


## Batch 4 — v007-v008

**Hypothesis:** Patch-diffused curvature plus rendered silhouette support should
allocate the 6% survivor budget better than raw one-ring curvature.

| Version | Density field | Boundary result | Mean compression | Topology |
|---|---|---|---:|---|
| v007 | one diffusion round, 10% render support | 94.0%, SSIM 0.6830, Haus 7.2% | 90.200128% | 0 defects |
| v008 | two rounds, 25% render support | 94.0%, SSIM 0.6815, Haus 34.9% | 90.200128% | 0 defects |

v007 improves boundary SSIM over the 0.6784 result of v005 at the same count
and keeps Hausdorff much lower. v008 over-diffuses importance and is less
promising, but both are submitted as the immutable planned batch.


### Batch 4 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v007 | `PPPPFPF` | 58.749318 |
| v008 | `PPPPFPP` | 75.049328 |

Neither density closes test 5 at 6%. v007 also changes T7 behavior, whereas
the v008 translation-unit/profile remains T7-safe. Preserve v008 exactly as the
next base and calibrate 7.0%/7.5% boundary continuations before revising density.


## Batch 5 — v009-v010

**Hypothesis:** Preserve the T7-safe v008 layout and map whether its patch
density moves the hidden test-5 boundary between the known 6% failure and 8%
safe snapshot.

| Version | Boundary keep ratio | Boundary local result | Mean compression | Topology |
|---|---:|---|---:|---|
| v009 | 7.0% | 93.0%, SSIM 0.6940 | 90.158461% | 0 defects |
| v010 | 7.5% | 92.5%, SSIM 0.7000 | 90.137628% | 0 defects |

Both are behavior-distinct only in the active boundary continuation while
retaining all-tier bounded graph repair and the exact known T7-safe density
layout.


### Batch 5 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v009 | `PPPPFPF` | 58.749318 |
| v010 | `PPPPFPP` | 75.049328 |

The v008 density does not move the hidden boundary: 7.5% still fails test 5.
v009 also loses T7 while the v010 layout remains T7-safe. Stop ratio
interpolation. Next profiles make regional allocation render-dominant rather
than mildly mixing silhouette support into curvature.


## Batch 6 — v011-v012

**Hypothesis:** Render-dominant regional density can preserve coverage at the
7.5% count even if mild curvature mixing cannot.

| Version | Render share | Boundary result | Mean compression | Topology |
|---|---:|---|---:|---|
| v011 | 55% | 92.5%, SSIM 0.6993, Haus 9.4% | 90.137628% | 0 defects |
| v012 | 75% | 92.5%, SSIM 0.6969, Haus 10.1% | 90.137628% | 0 defects |

The 55% profile sharply improves geometric coverage versus v010 but leaves
SSIM nearly unchanged; 75% over-allocates silhouette support. Official evidence
will determine whether the hidden mesh benefits from the coverage repair.


### Batch 6 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v011 | `PPPPFPP` | 75.049328 |
| v012 | `PPPPFPF` | 58.749318 |

Coverage repair alone does not recover test 5, and excessive render weighting
again destabilizes T7. Static density weighting is exhausted. Crucial code
audit: vertexArea and vertexCurv are never accumulated in applyCollapse, so the
existing regional quota is actually a static endpoint weight. Next batch turns
it into a true carried cluster budget.


## Batch 7 — v013-v014

**Hypothesis:** True carried cluster load should prevent survivor deserts that
static endpoint weights cannot detect.

| Version | Carried budget | Boundary result | Mean compression | Topology |
|---|---|---|---:|---|
| v013 | moderate saturation | 92.5%, SSIM 0.6990, Haus 10.3% | 90.137628% | 0 defects |
| v014 | strong feature saturation | 92.5%, SSIM 0.6939, Haus 7.1% | 90.137628% | 0 defects |

Carried load materially changes the output and strong saturation improves
Hausdorff, but neither improves local SSIM over v010. Both proceed to official
evaluation because the hidden test-5 surface distribution is unknown.


### Batch 7 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v013 | `PPPPFPF` | 58.749318 |
| v014 | `PPPPFPP` | 75.049328 |

Carried scalar budgets do not recover test 5; v014 remains T7-safe. The next
attempt changes the moment itself: persistent perspective screen-Jacobian
quadrics penalize motion perpendicular to camera rays while allowing motion
along them, targeting projected normal/depth consistency more directly.


## Batch 8 — v015-v016

**Hypothesis:** Persistent perspective screen-Jacobian moments can preserve
projected position without isotropically freezing depth.

| Version | Jacobian strength | Boundary result | Mean compression | Topology |
|---|---:|---|---:|---|
| v015 | 2.0 | 92.5%, SSIM 0.6992, Haus 29.1% | 90.137628% | 0 defects |
| v016 | 5.0 | 92.5%, SSIM 0.6990, Haus 31.2% | 90.137628% | 0 defects |

The directional moments are active but do not improve the local boundary
render. Official results will determine whether hidden axial projection responds
differently.


### Batch 8 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v015 | `PPPPFPP` | 75.049328 |
| v016 | `PPPPFPP` | 75.049328 |

Perspective position moments preserve T7 but do not recover test 5. Projected
vertex position is not sufficient. Next batch directly constrains flat
face-normal rotation and triangle-area distortion during the strict boundary
continuation.


## Batch 9 — v017-v018

**Hypothesis:** Tight local face-normal and area-ratio guards can redirect the
strict continuation toward normal-map-safe collapses.

| Version | Normal floor | Boundary result | Mean compression | Topology |
|---|---:|---|---:|---|
| v017 | 0.985 | 92.5%, SSIM 0.6989, Haus 8.9% | 90.137628% | 0 defects |
| v018 | 0.995 | 92.5%, SSIM 0.6976, Haus 26.5% | 90.137628% | 0 defects |

Both still reach the count, but stronger per-face constraints reduce local
SSIM. This suggests compensating patch errors matter more than independent
per-face limits.


### Batch 9 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v017 | `PPPPFPF` | 58.749318 |
| v018 | `PPPPFPP` | 75.049328 |

Per-face normal guards do not recover test 5; v018 remains T7-safe. The next
batch activates original-reference local Vega scoring only for the last 10% of
the aggressive continuation, testing patch-level compensating render errors
without making the whole QEM loop renderer-bound.


## Batch 10 — v019-v020

**Hypothesis:** Original-reference local Vega gates in the final 10% can reject
collapses whose compensating patch error is invisible to independent face
guards.

| Version | Local Vega threshold | Boundary result | Mean compression | Topology |
|---|---:|---|---:|---|
| v019 | 0.90 | 92.4604%, SSIM 0.6980, Haus 26.5% | 90.135978% | 0 defects |
| v020 | 0.94 | 92.4604%, SSIM 0.6980, Haus 26.5% | 90.135978% | 0 defects |

Both thresholds stop at the same count and produce the same fingerprint. The
reference gate is active enough to prevent the final few collapses but does not
identify a better trajectory. Both proceed to official evaluation; the next
experiment will compare complete alternative trajectories against the original
render rather than making independent greedy accept/reject decisions.


### Batch 10 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v019 | `PPPPFPP` | 75.049328 |
| v020 | `PPPPFPP` | 75.049328 |

The local reference gate does not recover test 5. Both variants preserve T7,
strengthening the conclusion that the active barrier is hidden render fidelity
at the 45–50k boundary rather than topology or a generic layout instability.


## Batch 11 — v021-v022

**Hypothesis:** Whole-trajectory render selection can distinguish unsafe strict
continuations, while deterministic QEM priority diversification may find a
different 7.5% trajectory that stays within the original-render budget.

| Version | Priority jitter | Boundary result | Mean compression | Topology |
|---|---:|---|---:|---|
| v021 | 1% | 92.1%, SSIM 0.7020, Haus 26.5% | 89.992414% | 0 defects |
| v022 | 4% | 92.1%, SSIM 0.7020, Haus 26.5% | 89.992414% | 0 defects |

Both diversified 7.5% trajectories fail the measured whole-state budget, so
the solver restores the identical 7.9% safe fingerprint. This validates the
selection mechanism but shows cost jitter alone does not produce an acceptable
continuation. Official evaluation tests whether the restored state clears the
hidden test-5 frontier and whether the reduced 512 reference resolution is safe.


### Batch 11 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v021 | `PPPPFPP` | 75.049328 |
| v022 | `PPPPFPF` | 58.749318 |

The 512-resolution whole-state check does not predict hidden test 5, and the
4% diversified trajectory reintroduces the familiar T7 instability. More
importantly, v018 was not a canonical safe scaffold: it carries earlier density
and guard changes. The next batch removes that confound by returning to the
all-pass v003 lineage and changing only native-resolution endgame selection.


## Batch 12 — v023-v024

**Hypothesis:** Native-1024 whole-trajectory validation on the canonical all-pass
v003 scaffold can safely distinguish an aggressive endgame from its exact 8%
fallback without the confounds introduced in v005-v022.

| Version | Aggressive target / budget | Boundary result | Mean compression | Topology |
|---|---|---|---:|---|
| v023 | 7.5%, standard budget | 92.0%, SSIM 0.6999, Haus 4.9% | 89.988248% | 0 defects |
| v024 | 7.0%, strict budget | 92.0%, SSIM 0.6999, Haus 4.9% | 89.988248% | 0 defects |

Both aggressive trajectories are rejected and restore the identical canonical
8% fingerprint. Unlike Batch 11, the fallback is the known all-pass lineage.
Official evaluation now directly tests whether native whole-state validation
preserves the hidden cases.


### Batch 12 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v023 | `PPPPPPP` | 90.399652 |
| v024 | `PPPPPPF` | 74.099642 |

The canonical 8% fallback is hidden-test safe, validating native whole-state
selection on the unmodified lineage. v024 still perturbs T7 through translation
unit/timing sensitivity despite restoring the same local boundary mesh. The
next experiment keeps v023 as its exact scaffold and replaces the unsafe edge-
collapse continuation with topologically different star retriangulation.


## Batch 13 — v025-v026

**Hypothesis:** Vertex-star deletion with ring retriangulation may preserve normal
fields better than additional edge collapses at the canonical 8% frontier.

| Version | Star passes | Boundary result | Mean compression | Topology |
|---|---:|---|---:|---|
| v025 | 2 | 92.0%, SSIM 0.6999, Haus 4.9% | 89.988248% | 0 defects |
| v026 | 4 | 92.0%, SSIM 0.6999, Haus 4.9% | 89.988248% | 0 defects |

Both retriangulated candidates exceed the native render-loss budget and restore
the exact v023 fingerprint. The four-pass attempt provides no compensating
recovery. They proceed to Kattis to check source-layout safety and the hidden
transaction decision.


### Batch 13 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v025 | `PPPPPPF` | 74.099642 |
| v026 | `PPPPPPF` | 74.099642 |

Both fail only T7 despite restoring the canonical local mesh. The added helper
changes compilation/timing enough to destabilize the huge case. Subsequent
structural experiments reserve explicit T7 survivor margin so this unrelated
fragility does not mask the active 45-50k hypothesis.


## Batch 14 — v027-v028

**Hypothesis:** If the prior star transactions were rejected only by the render
budget, forcing commit would reveal their true fidelity; T7 uses a robust 2.4%
margin to isolate the experiment.

| Version | Forced star passes | Boundary result | Topology |
|---|---:|---|---|
| v027 | 2 | 92.0%, SSIM 0.6999, Haus 4.9% | 0 defects |
| v028 | 4 | 92.0%, SSIM 0.6999, Haus 4.9% | 0 defects |

Both fingerprints equal v023 even with validation bypassed. The star pass finds
no valid deletions under its geometric constraints; the earlier interpretation
as budget rejection was wrong. This branch is count-inert and proceeds only to
verify the T7-margin control.


### Batch 14 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v027 | `PPPPPPF` | 74.102643 |
| v028 | `PPPPPPF` | 74.102643 |

A 2.4% huge-tier ratio is still insufficient to stabilize this source layout.
The next pair raises the independent T7 control to 4.0%, matching the robust
range observed in Durian, while widening the active star envelope.


## Batch 15 — v029-v030

**Hypothesis:** Widening star normal/deviation constraints will activate safe ring
retriangulations that the default envelope excludes; T7 is isolated at 4%.

| Version | Star envelope | Boundary result | Topology |
|---|---:|---|---|
| v029 | 2x | 92.0042%, SSIM 0.7000, Haus 5.1% | 0 defects |
| v030 | 4x | 92.0167%, SSIM 0.7001, Haus 5.8% | 0 defects |

The branch is finally active: 2 and 8 vertices are removed, respectively, and
SSIM improves slightly rather than declining. This supports star retriangulation
as a locally compensating operation, though the current yield is far too small.


### Batch 15 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v029 | `PPPPFPF` | 58.749318 |
| v030 | `PPPPFPF` | 58.749318 |

Even two extra local boundary removals cross hidden T5, while 4% does not
stabilize T7 for this translation-unit layout. Local SSIM improvement is not
directionally reliable at the hidden cliff. One final envelope batch maps the
operation before pivoting away from T5.


## Batch 16 — v031-v032

**Hypothesis:** Very wide star envelopes may create enough adjacent patch
retriangulation for errors to compensate globally.

| Version | Star envelope | Boundary result | Topology |
|---|---:|---|---|
| v031 | 8x | 92.1208%, SSIM 0.6985, Haus 6.5% | 0 defects |
| v032 | 16x | 92.4375%, SSIM 0.6896, Haus 7.0% | 0 defects |

Yield rises to 58 and 210 extra removals, but fidelity declines monotonically
after the narrow 4x point. Broad star retriangulation does not create global
error compensation. This closes the T5 star branch.


### Batch 16 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v031 | `PPPPFPF` | 58.749318 |
| v032 | `PPPPFPF` | 58.749318 |

Both fail T5 and T7. The envelope sweep is closed; no star continuation below
the canonical T5 state transfers to hidden validity.


## Batch 17 — v033-v034

**Hypothesis:** Transfer bounded star retriangulation to the 25-45k tier, where
the survivor frontier is less discontinuous; isolate T7 with a 10% control.

| Version | Focus fixture / star envelope | Result | Topology |
|---|---|---|---|
| v033 | bumpy-hard / 4x | 85.5012%, SSIM 0.8728, Haus 3.4% (safe fallback) | 0 defects |
| v034 | icosphere / 8x | 88.7213%, SSIM 0.9879, Haus 0.5% | 0 defects |

The difficult proxy rejects the absolute endgame before the star pass, while
the smooth proxy accepts 90 extra deletions with only a small SSIM change.
Official results will reveal which hidden medium case resembles each regime.


### Batch 17 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v033 | `PPPPPPF` | 74.099642 |
| v034 | `PPPPPPF` | 74.099642 |

Both medium variants pass tests 1-6, so 90 additional star deletions do not
cross the hidden medium thresholds. T7 still fails even with 10% survivors,
showing that this is a runtime/layout failure rather than insufficient fidelity
margin. Future candidates require an explicit giant-tier time shield.


## Batch 18 — v035-v036

**Hypothesis:** Rank the same 90 mid-tier star deletions by the post-retriangulation
valence-six defect to preserve future collapsibility.

| Version | Valence weight | Icosphere result | Topology |
|---|---:|---|---|
| v035 | 0.002 | 88.7213%, SSIM 0.987948, Haus 0.4646% | 0 defects |
| v036 | 0.010 | 88.7213%, SSIM 0.987927, Haus 0.4852% | 0 defects |

Weak valence ranking changes the fingerprint with a marginal SSIM gain over
pure geometry; strong regularization is worse. Kattis tests whether the root
choice changes hidden medium fidelity.


### Batch 18 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v035 | `PPPPPPP` | 89.099655 |
| v036 | `PPPPPPF` | 74.099642 |

Weak future-valence ranking yields a fully valid source layout with the 10% T7
control; strong ranking destabilizes T7. v035 becomes the structural scaffold.
Its score penalty is almost entirely the deliberately loose giant ratio, which
can now be recovered on the exact stable layout.


## Batch 19 — v037-v038

**Hypothesis:** Add minimum new-triangle quality to weak future-valence root
ranking, preserving the same deletion count while avoiding skinny fans.

| Version | Quality weight | Icosphere result | Topology |
|---|---:|---|---|
| v037 | 0.02 | 88.7213%, SSIM 0.987941, Haus 0.5866% | 0 defects |
| v038 | 0.10 | 88.7213%, SSIM 0.987916, Haus 0.4646% | 0 defects |

Both change the root choices but neither beats v035 locally; the weak quality
term has especially worse Hausdorff. Official results test hidden behavior and
source-layout stability.


### Batch 19 official result

| Version | Kattis cases | Kattis score |
|---|---|---:|
| v037 | `PPPPPPF` | 74.099642 |
| v038 | `PPPPPPF` | 74.099642 |

Triangle-quality root ranking does not affect medium validity and both layouts
lose T7. v035 remains the only all-pass structural scaffold in this branch.


## Batch 20 — v039-v040

**Hypothesis:** A post-QEM Vega-ranked star pool can provide material mid-tier
yield, while 14s/17s giant deadlines test whether T7 failures are timeout-driven.

| Version | Focus diagnostic | Result | Topology |
|---|---|---|---|
| v039 | icosphere | 88.5015%, SSIM 0.9881, Haus 0.3% (count-inert) | 0 defects |
| v040 | 501k giant | 90.0001%, SSIM 0.3982, Haus 0.5% | 0 defects |

The Vega pool finds no valid post-QEM medium deletions. v040 confirms the 10%
giant target completes quickly and cleanly locally; Kattis distinguishes timeout
from hidden fidelity/layout failure.
