# Skill: evaluate

Use this skill to score a candidate Python solution and confirm it is an
improvement before accepting it.

## When to use

After any change to the solver (`solution.py`) — every time you want to know
whether the current mesh-simplification logic produces a **valid** submission
and whether it **beats the previous best** model.

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

1. run the solver (`solution.py`) on the input mesh to produce a simplified mesh;
2. score it with `evaluate.py` (see [`docs/evaluation.md`](../docs/evaluation.md));
3. **log the output** to `outputs/<date>-<result>.txt`, where `<result>` is the
   compression metric on success (e.g. `compr-11.1111`) or `error` / `invalid`
   on failure — the log records the metric on success and the error otherwise;
4. compare the new compression rate against the **best previous valid run** in
   `outputs/` and print `IMPROVED` or `NOT IMPROVED`.

## Interpreting the result

- **Exit code `0`** — the submission is valid **and** strictly beats the
  previous best (or there is no previous valid run). Keep the change.
- **Exit code `1`** — one of:
  - the submission is **invalid** (failed the manifold / Hausdorff / SSIM gate),
  - the solver or evaluator **errored**, or
  - the submission is valid but did **not improve** (`NOT IMPROVED`) on the
    best previous valid run.

  In every `1` case, do **not** accept the change: read the logged
  `outputs/<date>-<result>.txt` file, diagnose the cause, and iterate on
  `solution.py`.

## The improvement rule

The challenge ranks valid submissions purely by `CompressionRate` (higher is
better). A new model is only an improvement if it is **valid** and its
`CompressionRate` is **strictly greater** than the best previously logged valid
run. The `outputs/` log history is what makes this comparison possible, so do
not delete it between iterations.

Bound the search: make at most **5 attempts** to beat the previous best. If none
of the 5 attempts improves the score, stop iterating and report a short
**post-mortem** to the user — your hypotheses for why the score did not improve
and suggested next directions — rather than looping indefinitely.

## Scope

For now this skill targets the **Python** solution only (`solution.py` +
`evaluate.py`).
