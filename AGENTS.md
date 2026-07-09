# AGENTS.md

Guidance for coding agents working in this repository. The goal is to produce a
mesh-simplification solver that maximizes vertex reduction while remaining a
valid submission for the IMC 2026 challenge.

## Workflow

* **Overview**: Read [`docs/problem-specification.md`](docs/problem-specification.md) to understand the challenge. When brainstorming solution ideas, add them to [`docs/solutions.md`](docs/solutions.md) with a brief description and initial status. Use [`docs/math-formalism.md`](docs/math-formalism.md) for mathematical reference. When you need a full description of the problem, see [`docs/problem-formulation-full.md`](docs/problem-formulation-full.md).

* **Iterative Solutions:** You are iterating on a solution in a current solution family. Each solution family is a set of related ideas and code changes, e.g. solutions/lemon/v1.cpp. When iterating a solution, use the skill [`skills/iterate.md`](skills/iterate.md) to guide your work.

* **Maintain docs/solutions.md**. Update the solution ideas and their status in `docs/solutions.md` as you iterate. This will help track which approaches have been tried, what the results were, and what remains to be explored. If you think of a new idea while working, add a new bucket with a brief description and initial status.

* **Maintain docs/world-model.md**. You are a rational belief-driven agent. Maintain a set of beliefs about the true Kattis environment (the meshes). Make hypotheses and test them using your iterations. Update `docs/world-model.md` with any new insights you have gained about the meshes, such as shape, topology, or properties to exploit.

* **Brainstorming**: Use [`skills/brainstorm.md`](skills/brainstorm.md) to brainstorm new solution ideas. Each idea should have a brief description and an initial status (e.g. "not started", "in progress", "completed"). Score and add notes to existing entries in `docs/solutions.md`.

* **Canonical evaluation**: Use [`skills/evaluate.md`](skills/evaluate.md) and
  `scripts/evaluate.sh`. The native C++ evaluator is the sole local acceptance
  path. Use both `data/ppsurf/` and `data/synth_bench/` when the experiment
  concerns generalizationdo brainstorming in iterate phase. or rendering.

* **Baseline**: `solutions/lemon/v115.cpp` is the canonical C++ baseline.
  Build arbitrary C++ candidates with `scripts/build.sh`; do not assume a
  source-specific executable name.

* **Online submissions**: Follow [`skills/submit.md`](skills/submit.md).
  Submissions require explicit approval, keep source files immutable during
  upload, and use `scripts/submit.py` for one file or
  `scripts/submit_batch.py` for batches.
