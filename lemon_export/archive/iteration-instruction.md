# Iteration Instructions for Mesh Simplification (Lemon Family)

## Core principle: TUNE ALL 6 TIERS IN ONE RUN

The 6 tiers cover different vertex-count ranges. Each Kattis case maps to exactly one tier at init (based on INPUT vertex count), and then **post-QEM** (after vertex reduction), cases often end up in DIFFERENT tiers. So each tier's settings affect MULTIPLE cases. **Do NOT tune one tier at a time** — tune all 6 in a single version.

Since the 6 tiers are independent (their parameters are isolated in `TierParams`), you can change all 6 in one submission and learn 6 things per Kattis run. **That makes tuning 6x faster.**

## Tier boundaries and parameters

- Per-tier `vegaPasses` (0-3) controls how many vega SSIM passes run for that tier
- **DO NOT add time-based thresholds.** Time only as kill switch in `run()`

## Red flags (STOP iterating)

- **Same score on 3+ consecutive submissions** — your changes aren't being applied or aren't reaching the algorithm. Check if your tierTable entry was actually modified.
- **Only one case changes between submissions** — tier boundaries are wrong or your change was too localized.
- **All cases fail in the same pattern** — likely a structural bug (e.g., `vegaPatchGeomFrac` set wrong for the tier).

## Process (per submission)

1. **Pick the tier that needs the most attention** (usually the one whose case(s) are failing).
2. **Change ALL 6 tiers in this submission.** For tiers that don't need change, leave their settings alone. For the target tier, change the relevant threshold(s).
3. Wait for `scored`. If same score as previous → revert the change, you're not reaching the algorithm. If different → analyze.


## Submission cadence

- Max 4 submissions per batch, but **wait for all to score before next batch**.
- If same score 3+ times → stop iterating this family, switch to a structural change.
- Always copy the file to `/workspace/lemon_vN.cpp` BEFORE submitting (the submission script reads from there).


## Build flags (Kattis)

`g++-14 -g -O2 -std=gnu++23 -static -lrt -Wl,--whole-archive -lpthread -Wl,--no-whole-archive`

