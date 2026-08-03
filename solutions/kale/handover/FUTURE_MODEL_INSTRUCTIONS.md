# Instructions for the next model

## Mission

Continue beyond the current `90.546038` champion without losing the proven all-pass control. The current canonical solver is `v010a_drop_aggressive_final`; treat it as the reference implementation and keep its non-target tiers byte-stable unless the experiment explicitly targets them.

## First actions in a new session

1. Read `README.md`, then `CANONICAL_PSEUDOCODE.md`, then `RECENT_INSIGHTS.md`.
2. Open the exact control source and pseudocode from `../v010a_drop_aggressive_final/`.
3. Before changing code, rewrite the algorithm in pseudocode space under the new candidate directory.
4. Keep the package layout `solutions/kale/<name>/pseudocode.cpp` and `solutions/kale/<name>/code.cpp`.
5. Diff every proposed variant against the exact control before compiling.

## Working rules

- Pseudocode first, code second. Do not tune only hyperparameters in code-space.
- Submit only immutable batches of exactly 2 candidates.
- Evaluate each candidate locally once, sequentially, before any upload.
- Keep one Kattis batch in flight per family.
- Update `solutions/kale/log.md` and `solutions/kale/jobs.md` after each batch.
- Use `scripts/evaluate.sh` as the local diagnostic harness and `cppcheck --enable=all --std=c++14 --max-ctu-depth=8` on each candidate.
- Use `solutions/kale/minify_cpp.py` if the upload artifact needs to be shrunk under the byte budget.

## What the evidence says

- The medium cliff is real. Official tests 3 and 4 are the ones that have moved most often.
- Source layout matters even when a branch is unreachable at runtime. A dead branch can still perturb medium hidden behavior.
- `v013a_qem_relocated_sixth` and `v013b_render_relocated_sixth` both failed official test 4.
- The T2 branch is the newest live direction, but it is still experimental.
- The all-pass control `v010a` is still the safest base for any future batch.

## Submission discipline

- Batch size is exactly 2.
- Do not upload a second batch until the first has reached terminal status.
- Record exact immutable filenames and returned IDs in `solutions/kale/jobs.md`.
- Keep the code under the service size limit before upload.

## Reporting format

For each batch, record:

- candidate name
- exact control
- single changed lever
- local diagnostic result
- official score and case string
- which test changed
- interpretation
- next experiment
