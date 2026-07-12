# Agent Working Notes

This repo is being worked as a competitive solver lab. The current approach is
to preserve strong baselines, create short named variants, and test one idea at
a time whenever possible.

## Ground Rules

- Do not overwrite `simplifygeometry.cpp` unless explicitly asked.
- New solver variants should use short contextual filenames, for example
  `v182_t3mild.cpp` or `v185_starflip.cpp`; keep all new filenames under 64
  characters.
- Do not commit unless explicitly asked.
- Keep `updates.md` current with commands, observed results, and why a variant
  exists.
- Treat local generated tests as signal, not truth. The official-style simulator
  results provided by the user are the deciding signal when they conflict.
- Use smart submissions, not volume spam: compile locally, do at least a targeted
  smoke when the change is algorithmic or risky, then submit only candidates with
  a clear hypothesis.
- The user has approved autopilot operation for this repo. Do not pause for
  permission on normal compile, local validation, variant creation, or promising
  official submissions.

## Current Baselines

- Immutable research parent for all new variants: `v46_isolation_control.cpp`,
  SHA-256 `9a5246f196a0f9e2ed7a9578e28fd623513059fd47164e15ebb05f980f1a1d09`.
- Current unsubmitted candidates from that exact parent:
  - `v63_splitpay_v46safe.cpp`: safer certified T3/T5 redistribution;
  - `v62_splitpay_v46.cpp`: stronger certified T3/T5 redistribution.
- Submission file under active review:
  `imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp`.
- Current official best: `imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp`,
  user-reported score `90.27`.
- Previous strongest numerically logged line: `v826_v3tail.cpp`, submitted as
  `v826.cpp`, score `89.435327`, cases `PPPPPPP`.
- Previous strong parents:
  - `v815_weldcap.cpp`: `89.434855`, cases `PPPPPPP`;
  - `v811_nlw_t2v.cpp`: `89.432187`, cases `PPPPPPP`;
  - `v808_nlateweld.cpp`: `89.431469`, cases `PPPPPPP`;
  - `v798_nudge4.cpp`: `89.429802`, cases `PPPPPPP`;
  - `v761_pair4s.cpp`: `89.429468`, cases `PPPPPPP`;
  - `v754_pdw6.cpp`: `89.428801`, cases `PPPPPPP`;
  - `prashant-kumar-singh-default-cinderella_V2-001.cpp`: about `89.416`,
    cases presumed `PPPPPPP`;
  - `v755_pdw4.cpp`: `89.424133`, cases `PPPPPPP`;
  - `v748_weld6.cpp`: `89.356323`, cases `PPPPPPP`;
  - `v753_occ6zero.cpp`: `89.356323`, cases `PPPPPPP`;
  - `v682_t3m156953_k31.cpp`: `89.342664`, cases `PPPPPPP`;
  - `v732_weld4.cpp`: `89.351989`, cases `PPPPPPP`;
  - `v713_xe24h437.cpp`: `89.343320`, cases `PPPPPPP`;
  - `v680_t3m156992_k31.cpp`: `89.342192`, cases `PPPPPPP`;
  - `v679_t3m156992_h039422.cpp`: `89.342060`, cases `PPPPPPP`;
  - `v674_t3m157031_k32.cpp`: `89.341720`, cases `PPPPPPP`;
  - `v672_t3c_h039414_k32.cpp`: `89.338414`, cases `PPPPPPP`;
  - `v663_t3b_h039408.cpp`: `89.333791`, cases `PPPPPPP`;
  - `v659_h039405_k32.cpp`: `89.323451`, cases `PPPPPPP`;
  - `v656_h039406_k32.cpp`: `89.323435`, cases `PPPPPPP`;
  - `v653_h039407_k35.cpp`: `89.323418`, cases `PPPPPPP`;
  - `v648_h039408_k35.cpp`: `89.323402`, cases `PPPPPPP`;
  - `v645_h039410_k40.cpp`: `89.323369`, cases `PPPPPPP`;
  - `v644_h039414_k35.cpp`: `89.323302`, cases `PPPPPPP`;
  - `v640_h039422_k40.cpp`: `89.323170`, cases `PPPPPPP`;
  - `v636_h039437_k30.cpp`: `89.322906`, cases `PPPPPPP`;
  - `v633_h039469_k18.cpp`: `89.322394`, cases `PPPPPPP`;
  - `v632_h0395_k18.cpp`: `89.321866`, cases `PPPPPPP`;
  - `v630_h039531_k14.cpp`: `89.321354`, cases `PPPPPPP`;
  - `v629_h039562_k14.cpp`: `89.320825`, cases `PPPPPPP`;
  - `v627_h039594_k10.cpp`: `89.320313`, cases `PPPPPPP`;
  - `v623_vieww_h039656.cpp`: `89.319273`, cases `PPPPPPP`;
  - `v606_h0396875.cpp`: `89.318744`, cases `PPPPPPP`;
  - `v603_t3r2_h03975.cpp`: `89.317703`, cases `PPPPPPP`;
  - `v601_t3k158125_h03975.cpp`: `89.317231`, cases `PPPPPPP`;
  - `v600_t3k158125.cpp`: `89.313069`, cases `PPPPPPP`;
  - `v599_t3low_h03975.cpp`: `89.306842`, cases `PPPPPPP`;
  - `v598_t3k15875_t2safe.cpp`: `89.302679`, cases `PPPPPPP`;
  - `v593_t23_h039625.cpp`: `89.293811`, cases `PPPPPPP`;
  - `v548_t4k0975.cpp`: `89.276626`, cases `PPPPPPP`;
  - `v506.cpp`: `89.237282`, cases `PPPPPPP`.
- Best retained research baseline from the late sweep: `v286_orion_area_nudge.cpp`.
- Other retained comparators:
  - `dragonfruit_v10.cpp`;
  - `v265_lens_mixed_fix_stepdown_trimmed.cpp`.
- Old generated variants have been removed from the root to keep the workspace
  readable.

## Variant Discipline

- Prefer isolated probes:
  - one tier changed;
  - one profile changed;
  - one algorithmic feature changed.
- After an isolated probe works, make a combined candidate.
- Keep a fallback version whenever a large tier or official simulator result is
  noisy.
- For boundary shaving, keep a pass/fail bracket in `updates.md` and move by
  small midpoints. Current fragile brackets:
  - T2: `0.3196875` passes, `0.319375` fails;
  - T3: `0.158125` passes on safe huge `0.040`; lower values need bracketing;
  - T4: `0.0975` passes, `0.0970` fails;
  - T3/huge combined: the pure parent is `v682`, with T3 keep `0.156953125`,
    huge keep `0.0394140625`, and huge k31/cap18.6. T3 `0.156875` failed hidden
    T3, while `0.156953125` passed with k31 and failed largest with k32. The
    best v713-family line leaves T3 untouched and adds only T2/T4 post-target
    xedge compression. The current prashant/cinderella best is a separate
    stronger parent and should be inspected before applying these brackets.
  - huge-only: current official bracket is `v659` pass at `0.0394052734375` with
    k32/cap19.2 versus `0.039404296875` failures for k31/k32/k33. Treat
    huge-only one-vertex shaving as near-exhausted unless a new quality model is
    added.
- Name future variants with both a number and a compact hint, for example
  `v190_replay.cpp`. Avoid both opaque names like `v190.cpp` and very long
  descriptive filenames.

## Test Discipline

- Compile check:
  - `g++ -O2 -std=c++17 -I /usr/include/eigen3 <file>.cpp -o /tmp/<name>`
- Local large smoke:
  - `python3 tests/solver_validity_smoke.py <file>.cpp --cxxflags '-I /usr/include/eigen3' --large --score --timeout 180`
- Local extreme smoke:
  - `python3 tests/solver_validity_smoke.py <file>.cpp --cxxflags '-I /usr/include/eigen3' --extreme --timeout 240`

## Current Algorithm Lines

- `simplifygeometry.cpp` is a compact QEM edge-collapse solver with a batched
  tail mode and an attempted camera-aware/invisible-edge post-pass.
- `v46_isolation_control.cpp` is the required immutable parent for current
  research. Official feedback: v48/v50 tied it, v49 improved only about
  `0.0001`, and the v51-v55 cohort family failed T4 without improving T3/T5.
- `v63_splitpay_v46safe.cpp` and `v62_splitpay_v46.cpp` preserve v46
  byte-for-byte on T1/T2/T4/T6/T7. On T3/T5 they oversimplify a separate
  candidate, spend part of the removed-vertex budget splitting high-residual
  faces projected onto the original surface, and require a grid-certified
  bidirectional `0.049 * diagonal` envelope plus native 1024 rollback.
- `imc_sol_handoff/imc_mesh_repro_package/solvers/current/nebula_atomic_region_v33_t7.cpp`
  is the current best official line, with user-reported score `90.27`. It uses
  screen-aware medium-tier paths, transactional continuation, atomic region
  replacement, and a dedicated million-vertex tail.
- `v826_v3tail.cpp` is the previous strongest numerically logged line: it starts from `v815`,
  adds the third guarded T2 Vega pass, and uses the free huge tail stop fuse
  from `v825`.
- `v815_weldcap.cpp` is the previous best T4 line: it starts from `v811`, keeps
  the safe T2 Vega push and T4 root-nudge/late-cleanup structure, and widens the
  T4 late weld budget/scan/cap. The useful signal is that T4 can still accept
  more late weld surgery when the root move itself is not made more aggressive.
- `v825_tail19.cpp` ties `v815` exactly while reducing the huge tail stop fuse
  from `23.0` to `19.4`. Use this tighter fuse for risky medium-tier variants;
  unlike a huge keep-ratio buffer, it did not cost official score on the parent.
- `v811_nlw_t2v.cpp` is the previous best combo line: it starts from `v808`,
  keeps guarded T4-only root nudging plus late T4 valence-weld/pair cleanup, and
  adds the safer T2 Vega-star cleanup push from `v802`.
- `v808_nlateweld.cpp` is the previous best late-cleanup line: it starts from
  `v798`, keeps guarded T4-only root nudging after star/disk deletion, then
  reruns T4 valence-weld and pair-disk cleanup late.
- `v798_nudge4.cpp` is the previous best official line: it starts from `v761`
  and adds guarded T4-only root nudging after star/disk deletion. The nudge is
  tiny, radius-charged, and rejects incident face flips.
- `v761_pair4s.cpp` is the previous best line: it starts from `v754` and adds a
  strict T4-only pair-disk surgery pass that removes two adjacent vertices at
  once through a validated boundary retriangulation.
- `v754_pdw6.cpp` is the previous best stable parent: it starts from
  `prashant-kumar-singh-default-cinderella_V2-001.cpp` and adds the missing
  T4-only valence 4-6 disk/weld pass. It keeps the prashant/cinderella medium
  ratios `0.32/0.16/0.10` and huge keep `0.032`.
- `prashant-kumar-singh-default-cinderella_V2-001.cpp` is the previous best
  parent: compact QEM with area/view-weighted quadrics, star-delete, two T2/T3
  Vega star passes, and aggressive huge keep `0.032`.
- `v748_weld6.cpp` is the strongest prior in-house topology line: it starts
  from `v713`, keeps T2/T3 untouched beyond the existing xedge behavior, adds a
  T4-only valence 4-6 disk/weld pass, uses huge keep `0.0394375`, and passes
  all official cases.
- `v732_weld4.cpp` is the previous best weld line: it adds only a T4 valence-4
  weld pass and passes all official cases.
- `v713_xe24h437.cpp` is the previous best official line: it starts from the v682
  core, adds a post-target extra edge-collapse pass only on T2/T4, skips T3, uses
  huge keep `0.0394375`, and passes all official cases.
- `v682_t3m156953_k31.cpp` is the strongest pure-QEM/vega parent: it keeps the
  v606 tier tuning and T3 second Vega round, lowers T3 keep to `0.156953125`,
  sets huge keep to `0.0394140625`, and uses huge-only view/area quadric
  weighting (`k=31`, cap `18.6`).
- `v286_orion_area_nudge.cpp` is the richer late-stage line: QEM collapse,
  tiered star deletion, local SSIM/lens scoring, area-weighted lens impact, and
  root nudging.
- Use a seven-tier naming convention everywhere: `T1` is the sample, followed
  by caps `T2 <= 5k`, `T3 <= 25k`, `T4 <= 40k`, `T5 <= 50k`,
  `T6 <= 400k`, and `T7 <= 1.1M`. The solver's internal `screenTier` values
  `2/3/4` therefore correspond to official `T3/T4/T5`.
- Current official signal: the v51-v55 cohort family failed `T4` and did not
  improve `T3` or `T5`; do not revive it. The new split/pay branch must leave
  T4 byte-identical to `v46_isolation_control.cpp` and target only T3/T5.

## Submission Workflow

- Default submit values:
  - username: `fredrik-nguyen1`;
  - problem_id: `simplifygeometry`;
  - priority: `normal`;
  - family: use a compact space-themed family name such as `cosmic-boundary`.
- Submit with the internal dashboard endpoint from the current prompt/context,
  using the provided `X-Team-Secret`; avoid committing secrets into source files.
- Poll every 10-30 seconds until `scored`, `failed`, or `canceled`.
- Record every official submission id, score, cases string, and conclusion in
  `updates.md`.

## Next Research Direction

- Strategic target: `92.1`. From the user-reported current score `90.27`, this
  requires about `18.8%` fewer vertices than currently remain on average. Follow
  `docs/algorithmic_roadmap.md` for the new algorithmic program.
- The primary track is now algorithmic: original-referenced aggregate scoring,
  reversible oversimplification/selective repair, fixed-count vertex-budget
  redistribution, and generalized region replacement. Historical constant
  brackets remain useful controls, but they are not a credible path to `92.1`.
- Promising algorithmic directions:
  - conservative rollback/reinsertion after tail collapses;
  - vertex motion or local smoothing that preserves silhouette and normals;
  - tier-specific guardrails for medium tiers 2-4;
  - cautious Vega SSIM rescan rounds, now proven safe for T3 in `v603`;
  - post-output face cleanup that removes degenerate or near-invisible detail
    only when it does not disturb validator-sensitive geometry.
- Recently discarded despite good local signal:
  - T3 accumulated quadrics / MEMLESS-off (`v616`) failed hidden tier 3
    (`PPPFPPP`), so do not revive that line without a stronger guard.
  - Raw T5/400k lowering (`v618`, keep `0.024`) failed hidden tiers 5 and 6
    (`PPPPPFF`) despite clean local extreme smoke. Keep T5 at `0.025`.
  - Relaxed T3 Vega SSIM floor (`v621`, `0.970 -> 0.965`) failed largest tier
    (`PPPPPPF`) despite local 1M target unchanged. Treat T3-only code changes as
    potentially capable of perturbing high-tier timing near the boundary.
  - T3 Vega round 3 (`v607`) and vertex-count-triggered wide huge tail (`v624`)
    both failed largest tier (`PPPPPPF`); do not combine these with the current
    huge boundary unless a stronger quality guard is added.
