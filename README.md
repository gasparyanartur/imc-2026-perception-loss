# IMC 2026 perception-loss mesh simplification

This repository develops C++ mesh simplifiers for the IMC 2026 challenge. The
local acceptance pipeline is native C++: it compiles a `.cpp` candidate, runs
it on the mesh suite, validates the output topology and geometry, and measures
six-view perceptual similarity.

The official grader is authoritative for final results. Local reports are for
repeatable development and regression testing.

## Quick start

Build the native evaluator binaries:

```sh
scripts/build-evaluators.sh
```

Evaluate the canonical C++ baseline:

```sh
scripts/evaluate.sh --candidate solutions/lemon/v115.cpp
```

Add the synthetic suite and save a JSON report:

```sh
scripts/evaluate.sh --candidate solutions/lemon/v115.cpp \
    --include-synthetic --json outputs/latest.json
```

Every candidate must be a C++ source file with a `.cpp` extension. Build an
arbitrary candidate with:

```sh
scripts/build.sh path/to/solution.cpp
```

See [docs/evaluation.md](docs/evaluation.md) for mesh format, native metrics,
gates, datasets, and all evaluator options.

## Repository layout

- `solutions/` — C++ solution families and iteration records;
- `evaluators/` — native C++ validity and perceptual diagnostics;
- `scripts/` — build, evaluate, submit, and polling entry points;
- `data/ppsurf/` — representative evaluation meshes;
- `data/synth_bench/` — targeted synthetic shape suite;
- `docs/` — challenge, evaluation, mathematical, and solution documentation;
- `tests/` — repository checks for scripts, data, and documented invariants.

## Submissions

Submit one or more C++ sources with:

```sh
python3 scripts/submit.py --family lemon solutions/lemon/v115.cpp
```

Upload any number of C++ sources, store their IDs, and wait for all results with:

```sh
python3 scripts/submit_batch.py --family lemon \
    solutions/lemon/v115.cpp solutions/lemon/v116.cpp
```

Both commands use the service defaults for the team secret, problem, and
username. Use `--teamsecret`, `--problem`, or `--username` to override them.
The batch command writes its submission IDs and final status responses to
`data/submission-batches/`. Read [skills/submit.md](skills/submit.md) before
sending an official request.

## Development guidance

Read [AGENTS.md](AGENTS.md) before making changes. Use the native evaluator as
the source of truth for local decisions, record new solution ideas in
[docs/solutions.md](docs/solutions.md), and update
[docs/world-model.md](docs/world-model.md) when experiments change beliefs
about the challenge meshes.
