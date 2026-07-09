# Skill: evaluate

Use this skill to score a C++ candidate and confirm it is an improvement before
accepting it.

## When to use

After any change to a solver in a solution family — every time you want to
know whether the current mesh-simplification logic produces a valid submission
and whether it beats the previous best model.

The harness scores the C++ solver on a representative dataset of meshes
(`data/ppsurf/`, derived from the ppsurf dataset; see
[`docs/evaluation.md`](../docs/evaluation.md)), not a single mesh. A solver that
passes a trivial cube but fails real meshes is caught here: the submission is
only valid when every scenario passes.

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

The candidate is compiled with `scripts/build.sh` and run on every mesh in the
dataset. Only `.cpp` source candidates are supported. The native evaluator
runs at the real-grader 1024-pixel resolution by default. The low-resolution
diagnostic is for quick investigation only and must not be used to accept a
change.

The harness will:

1. compile the C++ candidate and run it on every mesh in the selected dataset;
2. aggregate the per-scenario verdicts — the submission is **VALID** only when
   all scenarios pass (`SCENARIOS_PASSED == SCENARIOS_TOTAL`); the reported
   `CompressionRate` is the mean over all scenarios;
3. log the output to `outputs/` through `scripts/evaluate.sh`;
4. provide native validity, Hausdorff, SSIM, per-view, compression, and runtime
   diagnostics for comparison with the previous best.

## Interpreting the result

- **Exit code `0`** — every scenario in the selected dataset is valid. Compare
  the mean compression with the previous best before accepting the change.
- **Exit code `1`** — one of:
  - at least one scenario is invalid (failed the manifold, Hausdorff, SSIM, or
    time-budget gate);
  - the solver or native evaluator errored;
  - the native evaluator binaries are unavailable.

In every `1` case, read the logged output under `outputs/`, identify the failing
scenario and gate, and iterate on the current C++ solution family.

## The improvement rule

The challenge ranks valid submissions purely by `CompressionRate` (higher is
better). A candidate is an improvement only if it is valid on all required
dataset scenarios and its mean `CompressionRate` is strictly greater than the
best previously logged valid run. The `outputs/` history is what makes this
comparison possible, so do not delete it between iterations.

Bound the search: make at most **5 attempts** to beat the previous best. If none
of the 5 attempts improves the score, stop iterating and report a short
post-mortem to the user — the hypotheses for why the score did not improve and
suggested next directions — rather than looping indefinitely.

## Scope

This skill targets C++ solutions, scored across the representative dataset by
the native evaluator in `evaluators/` and orchestrated by
`scripts/evaluate_candidate.py`.
