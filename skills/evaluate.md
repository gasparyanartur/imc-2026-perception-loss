# Skill: evaluate

Use this skill for one local diagnostic run per candidate. Kattis evaluation is
the ground truth for official score and acceptance; local results guide
debugging and local-evaluator parity work.

## When to use

After any change to a solver in a solution family, every time you want to
know whether the current mesh-simplification logic produces a valid submission
and whether it beats the previous best model.

## How to run

Build the native evaluator binaries and call the canonical harness from the
repository root:

```sh
scripts/build-evaluators.sh
scripts/evaluate.sh --candidate solutions/lemon/v115.cpp
```

For broader shape and renderer diagnostics:

```sh
scripts/evaluate.sh --candidate solutions/lemon/v115.cpp \
  --include-synthetic --json outputs/latest.json
```

For fast routine iteration, retain the essential topology, Hausdorff, and
normal/depth signals while using the existing 256-pixel renderer on a
tier-matched diagnostic core. Generate the 45k-50k fixture once with
`python3 scripts/generate_tangerine_core.py`, then select fixtures matching
every tier changed by the candidate:

```sh
scripts/evaluate.sh --candidate solutions/tangerine/v001.cpp \
  --diagnostic-resolution 256 --surface-samples 64 \
  --metrics-max-vertices 50000 \
  --dataset data/ppsurf/abc_00010098.txt \
  --dataset data/synth_bench/tier2_bumpy.obj \
  --dataset data/synth_bench/tier3_bumpy_hard.obj \
  --dataset data/diagnostic_core/tier4_bumpy_torus.obj \
  --dataset data/synth_bench/tier4_bumpy.obj \
  --dataset data/stress/stress_600k.txt
```

The core covers the T1 retention cliff and one topology/rendering fixture for
each larger solver tier. Use this same fast proxy for every routine candidate
so comparisons are consistent. The JSON output records an output SHA-256 for
every scenario. Before submission, verify that each candidate has a distinct
output fingerprint on every tier it claims to change; behavior-identical
variants are not meaningful iterations. Run the
full default 1024-pixel suite only when the experiment specifically concerns
local-evaluator parity; Kattis provides the batch-wide ground truth. Use both
ppsurf and synthetic data only when the hypothesis specifically concerns
generalization or rendering behavior.

The candidate is compiled with `scripts/build.sh` and run on every mesh in the
dataset. Only `.cpp` source candidates are supported. The native evaluator
runs at the real-grader 1024-pixel resolution by default.
`--diagnostic-resolution 256` selects the low-resolution diagnostic for
routine iteration; it is deliberately non-authoritative and should be
interpreted as a stable proxy alongside Kattis results.
`--metrics-max-vertices 50000` keeps high-tier routine probes cheap by
checking solver time, topology, compression, and output fingerprints while
skipping their expensive renderer and brute-force sampled Hausdorff passes.

The harness will:

1. compile the C++ candidate and run it on every mesh in the selected dataset;
2. aggregate the per-scenario verdicts — the submission is **VALID** only when
   all scenarios pass (`SCENARIOS_PASSED == SCENARIOS_TOTAL`); the reported
   `CompressionRate` is the mean over all scenarios;
3. log the output to `outputs/` through `scripts/evaluate.sh`;
4. provide native validity, Hausdorff, SSIM, per-view, compression, and runtime
   diagnostics for comparison with the previous best.

## Interpreting the result

- **Exit code `0`** — every scenario in the selected dataset passes the local
  diagnostic gates. Compare the detailed metrics with prior runs, but do not
  treat this result as proof of official acceptance or as a prerequisite for
  submitting the candidate.
- **Exit code `1`** — one of:
  - at least one scenario is invalid (failed the manifold, Hausdorff, SSIM, or
    time-budget gate);
  - the solver or native evaluator errored;
  - the native evaluator binaries are unavailable.

In every `1` case, read the logged output under `outputs/`, identify the failing
scenario and gate, and iterate on the current C++ solution family.

## The improvement rule

The challenge ranks officially valid submissions purely by
`CompressionRate` (higher is better). A candidate is an improvement only when
Kattis reports a strictly better valid score. Local pass/fail, mean
compression, SSIM, and Hausdorff are diagnostic evidence: they explain,
predict, and help design the next candidate, but they must not block an
otherwise planned batch submission or select the official winner.

Keep every local result under `outputs/`, including locally invalid runs.
Bound a specific hypothesis to at most **5 local attempts** before updating or
dropping it, but submit the completed immutable batch so Kattis can test local
evaluator parity.

## Scope

This skill targets C++ solutions, scored across the representative dataset by
the native evaluator in `evaluators/` and orchestrated by
`scripts/evaluate_candidate.py`.

## Note:

- The evaluator is a diagnostic tool, not the ground truth. The official score and
  acceptance are determined by Kattis evaluation. Use the local evaluator to
  debug and improve parity with Kattis.
- The evaluator needs to be continuously improved to extract more information about the meshes and score changes. If tests are uninformative or take too long, investigate how to improve the local evaluator and remove the bottlenecks.
- If test-cases are not informative or too easy, consider adding more challenging scenarios to better evaluate the candidate solutions.
- A local threshold failure is a diagnostic observation, not a submission
  veto. Kattis is the sole acceptance and ranking oracle.
