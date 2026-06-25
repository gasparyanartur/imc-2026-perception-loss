# Skill: evaluate

Use this skill to score a candidate Python solution and confirm it is an
improvement before accepting it.

## When to use

After any change to the solver (`solution.py`) — every time you want to know
whether the current mesh-simplification logic produces a **valid** submission
and whether it **beats the previous best** model.

The harness scores the solver on a **representative dataset** of meshes
(`data/ppsurf/`, derived from the ppsurf dataset; see
[`docs/evaluation.md`](../docs/evaluation.md)), **not** a single mesh. A solver
that passes a trivial cube but fails real meshes is caught here: the submission
is only valid when **every** scenario passes.

## How to run

Call the evaluation harness from the repository root:

```sh
./evaluate.sh
```

To score at native (real-grader-like) resolution:

```sh
RESOLUTION=1024 ./evaluate.sh
```

`evaluate.sh` will:

1. run the solver (`solution.py`) on **every** mesh in the dataset directory
   (default `data/ppsurf/`) and score each one with `evaluate.py` (via
   `evaluate_dataset.py`);
2. aggregate the per-scenario verdicts — the submission is **VALID** only when
   all scenarios pass (`SCENARIOS_PASSED == SCENARIOS_TOTAL`); the reported
   `CompressionRate` is the **mean** over all scenarios;
3. **log the output** to `outputs/<date>-<result>.txt`, where `<result>` is the
   mean compression metric on success (e.g. `compr-78.2938`) or `error` /
   `invalid` on failure;
4. compare the new mean compression rate against the **best previous valid run**
   in `outputs/` and print `IMPROVED` or `NOT IMPROVED`.

The dataset is regenerated with
[`datasets/prepare_ppsurf.py`](../datasets/prepare_ppsurf.py); see
[`docs/evaluation.md`](../docs/evaluation.md) for how to grow it.

## Interpreting the result

- **Exit code `0`** — **every** scenario in the dataset is valid **and** the
  mean compression strictly beats the previous best (or there is no previous
  valid run). Keep the change.
- **Exit code `1`** — one of:
  - **at least one scenario is invalid** (failed the manifold / Hausdorff /
    SSIM gate) — the report shows `passed N/M scenarios` and lists the failing
    scenario's reason,
  - the solver or evaluator **errored**, or
  - all scenarios are valid but the mean compression did **not improve**
    (`NOT IMPROVED`) on the best previous valid run.

  In every `1` case, do **not** accept the change: read the logged
  `outputs/<date>-<result>.txt` file (it contains the per-scenario table),
  diagnose which mesh(es) failed and why, and iterate on `solution.py`.

## The improvement rule

The challenge ranks valid submissions purely by `CompressionRate` (higher is
better). Here a model is only an improvement if it is valid on **all** dataset
scenarios and its **mean** `CompressionRate` is **strictly greater** than the
best previously logged valid run. The `outputs/` log history is what makes this
comparison possible, so do not delete it between iterations.

Bound the search: make at most **5 attempts** to beat the previous best. If none
of the 5 attempts improves the score, stop iterating and report a short
**post-mortem** to the user — your hypotheses for why the score did not improve
and suggested next directions — rather than looping indefinitely.

## Scope

For now this skill targets the **Python** solution only (`solution.py`), scored
across the representative dataset by `evaluate_dataset.py` (which wraps
`evaluate.py`).
