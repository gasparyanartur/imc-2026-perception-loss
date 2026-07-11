# Skill: evaluate

Use this skill for one local diagnostic run per candidate. Kattis evaluation is
the ground truth for official score and acceptance; local results guide
debugging and local-evaluator parity work.

## When to use

After any change to a solver in a solution family, every time you want to
measure how the current mesh-simplification logic changes compression,
topology, geometry, SSIM, Hausdorff, IoU, and runtime.

## How to run

Build the unified native evaluator and call the canonical harness from the
repository root:

```sh
scripts/build-evaluators.sh
scripts/evaluate.sh --candidate solutions/lemon/v115.cpp
```

For broader shape and renderer diagnostics:

```sh
scripts/evaluate.sh --candidate solutions/lemon/v115.cpp \
  --include-synthetic --include-stress --json outputs/latest.json
```

The native evaluator is a single binary (`build/evaluators/evaluator`) that
reads both meshes once and reports every diagnostic as `KEY=VALUE` lines. It
reports numbers only: there is no local PASS/FAIL, validity verdict, strict
mode, or acceptance threshold. Interpret the metrics diagnostically; Kattis is
the only acceptance oracle.

The candidate is compiled with `scripts/build.sh` and run on every mesh in the
dataset. Only `.cpp` source candidates are supported.

Render resolution auto-adapts to the largest vertex count in the pair:

- `<100k` — 1024 px, SS×4 (Kattis parity for tier 1-3)
- `100k-300k` — 384 px, SS×2
- `300k-1M` — 256 px, SS×2
- `≥1M` — 192 px, SS×1

Override with `--render-res=N --ss=N` when you need a fixed resolution.

The harness will:

1. compile the C++ candidate and run it on every mesh in the selected dataset;
2. call the unified evaluator once per scenario and collect its full metric
   block (topology, geometry, Hausdorff, six-view SSIM, per-view IoU, profile
   timing);
3. aggregate numeric means, including `CompressionRate`, SSIM, and Hausdorff
   usage, without producing any acceptance verdict;
4. log the output to `outputs/` through `scripts/evaluate.sh`.

## Available metrics per scenario

The orchestrator surfaces (and the JSON record contains) the following fields:

- `tier` — `tier1`...`tier6` classified from the original vertex count
- `compression` — vertex reduction percent
- `final_ssim`, `normal_ssim`, `depth_ssim`
- `hausdorff_sym`, `hausdorff_usage_pct`, `hausdorff_limit`
- `nonmanifold_edges`, `degenerate_faces`, `repeated_faces`,
  `orientation_errors`, `boundary_edges`, `euler_chi`, `genus`,
  `sharp_edges_60`
- `min_tri_area`, `max_edge_len`
- `render_res`, `render_ss`, `render_threads`
- `normal_iou_avg`, `depth_iou_avg`, `fg_pixels_orig`, `fg_pixels_simp`
- `hausdorff_samples`, `hausdorff_grid_cells`
- per-view: `normal_ssim`, `depth_ssim`, `combined_ssim`, `normal_iou`,
  `depth_iou`, `fg_orig`, `fg_simp`
- raw `metrics` dict with every KEY=VALUE the C++ binary emits

## Interpreting the result

- **Exit code `0`** means measurement completed. It says nothing about
  acceptance.
- A nonzero exit means the solver, build, orchestration, or native evaluator
  could not complete. Read the recorded error and logs; do not convert metric
  values into an exit-code verdict.

## The improvement rule

The challenge ranks officially valid submissions purely by
`CompressionRate` (higher is better). A candidate is an improvement only when
Kattis reports a strictly better valid score. Local compression, SSIM,
Hausdorff, topology counts, and runtime are diagnostic evidence: they explain,
predict, and help design the next candidate, but they must not block an
otherwise planned batch submission or select the official winner.

Keep every local numeric result under `outputs/`, including extreme or
obviously risky metric profiles.
Bound a specific hypothesis to at most **5 local attempts** before updating or
dropping it, but submit the completed immutable batch so Kattis can test local
evaluator parity.

## Scope

This skill targets C++ solutions, scored across the representative dataset by
the unified native evaluator in `evaluators/evaluator.cpp` and orchestrated by
`scripts/evaluate_candidate.py`.

## Note:

- The evaluator is a diagnostic tool, not the ground truth. The official score and
  acceptance are determined by Kattis evaluation. Use the local evaluator to
  debug and improve parity with Kattis.
- The evaluator needs to be continuously improved to extract more information about the meshes and score changes. If tests are uninformative or take too long, investigate how to improve the local evaluator and remove the bottlenecks.
- If test-cases are not informative or too easy, consider adding more challenging scenarios to better evaluate the candidate solutions.
- Do not add local PASS/FAIL or threshold logic. Kattis is the sole acceptance
  and ranking oracle.
