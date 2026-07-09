# AGENTS.md

Guidance for coding agents working in this repository. The goal is to produce a
mesh-simplification solver that maximizes vertex reduction while remaining a
valid submission for the IMC 2026 challenge.

## Workflow

* **Overview**: Read [`docs/problem-specification.md`](docs/problem-specification.md) to understand the challenge. When brainstorming solution ideas, add them to [`docs/solutions.md`](docs/solutions.md) with a brief description and initial status. Use [`docs/math-formalism.md`](docs/math-formalism.md) for mathematical reference. When you need a full description of the problem, see [`docs/problem_formulation-full.md`](docs/problem_formulation-full.md). 

* **Iterative Solutions:** You are iterating on a solution in a current solution family. Each solution family is a set of related ideas and code changes, e.g. solutions/lemon/v1.cpp. Each solution should also maintain a solution/(solution-family)/log.md file that records the changes made, the results, and a post-mortem for each iteration. The goal is to produce a valid submission that improves on the previous best `CompressionRate` while remaining a valid closed 2-manifold within the Hausdorff and SSIM constraints. When iterating a solution, use the skill [`skills/iterate.md`](skills/iterate.md) to guide your work.


* **Test every iteration.** After each change, evaluate the solution using the
   [`skills/evaluate.md`](skills/evaluate.md) skill (which runs `./evaluate.sh`).
   Only accept a change if the skill reports a **valid** submission that
   **improves** on the previous best `CompressionRate`. If it is invalid, errors,
   or regresses, read the logged result in `outputs/`, diagnose, and iterate.

* **Bound the iteration.** Do not loop indefinitely. Make at most **50 attempts**
   to produce a valid improvement over the previous best. If none of the 50
   attempts improves the best `CompressionRate`, stop and write a short
   **post-mortem** for the user: your hypotheses for why the score did not
   improve (e.g. which validity gate blocks further reduction, where the
   simplification plateaus) and suggested next directions.

* **Maintain a log of iterations.** Keep a record of the changes made, the results, and the
   post-mortem in `log.md`. This will help you and others understand what was tried, what worked, and what did not.

* **Maintain docs/solutions.md**. Update the solution ideas and their status in `docs/solutions.md` as you iterate. This will help track which approaches have been tried, what the results were, and what remains to be explored. If you think of a new idea while working, add a new bucket with a brief description and initial status.

* **Mathematical Formalism**: When brainstorming solution ideas, add them to [`docs/solutions.md`](docs/solutions.md) with a brief description and initial status. Use [`docs/math-formalism.md`](docs/math-formalism.md) for mathematical reference.


## Solution iteration



## Scope

For now, focus on the **Python** solution (`solutions/baseline/baseline.py`), scored across the
representative ppsurf dataset (`data/ppsurf/`) by `evaluate_dataset.py` (which
wraps `evaluate.py`) via `evaluate.sh`. A submission is valid only when **every**
dataset scenario passes. See [`docs/evaluation.md`](docs/evaluation.md) for full
details of the evaluator, the dataset, and the harness.

## Conventions

- Keep the mesh I/O format intact: `V F` header, `v x y z`, `f i j k`
  (1-indexed on disk).
- The output mesh must always be a closed watertight triangular 2-manifold with
  positive-area faces and `1 ≤ |V'| ≤ |V|`.
- Do not delete the `outputs/` score history between iterations; the
  improvement check depends on it.
