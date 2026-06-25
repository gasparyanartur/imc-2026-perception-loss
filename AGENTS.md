# AGENTS.md

Guidance for coding agents working in this repository. The goal is to produce a
mesh-simplification solver that maximizes vertex reduction while remaining a
valid submission for the IMC 2026 challenge.

## Workflow

1. **Understand the problem.** Read [`docs/report.md`](docs/report.md) first. It
   defines the challenge, the evaluation pipeline (cameras, normal/depth maps,
   foreground SSIM, Hausdorff, manifold validity), and the ranking objective
   (`CompressionRate`). Refer back to it whenever you are unsure what the grader
   rewards or rejects.

2. **Iterate on the solution.** Implement and improve the mesh simplification
   logic in `solution.py` (the `simplify()` function). Make incremental changes
   that aim to remove more vertices while keeping the output a valid closed
   2-manifold within the Hausdorff and SSIM constraints.

3. **Test every iteration.** After each change, evaluate the solution using the
   [`skills/evaluate.md`](skills/evaluate.md) skill (which runs `./evaluate.sh`).
   Only accept a change if the skill reports a **valid** submission that
   **improves** on the previous best `CompressionRate`. If it is invalid, errors,
   or regresses, read the logged result in `outputs/`, diagnose, and iterate.

4. **Bound the iteration.** Do not loop indefinitely. Make at most **5 attempts**
   to produce a valid improvement over the previous best. If none of the 5
   attempts improves the best `CompressionRate`, stop and write a short
   **post-mortem** for the user: your hypotheses for why the score did not
   improve (e.g. which validity gate blocks further reduction, where the
   simplification plateaus) and suggested next directions.

## Scope

For now, focus on the **Python** solution (`solution.py`), scored by
`evaluate.py` via `evaluate.sh`. See [`docs/evaluation.md`](docs/evaluation.md)
for full details of the evaluator and harness.

## Conventions

- Keep the mesh I/O format intact: `V F` header, `v x y z`, `f i j k`
  (1-indexed on disk).
- The output mesh must always be a closed watertight triangular 2-manifold with
  positive-area faces and `1 ≤ |V'| ≤ |V|`.
- Do not delete the `outputs/` score history between iterations; the
  improvement check depends on it.
