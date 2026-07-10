# Banana iteration log

## Baseline

- Source: `solutions/banana/v1.cpp`, cloned from `solutions/lemon/v115.cpp`.
- The smoke run is intentionally local; official scores remain authoritative.

## Baseline clone

- 2026-07-10: Lemon baseline evaluated as INVALID on 4/10 ppsurf scenarios,
  with mean compression 91.499795%. Failures were all native SSIM below 0.90.
  The failing meshes are mostly below 5,000 vertices, so the T1
  `keepRatio = 0.00` is the leading hypothesis.

## Iterations

Results are recorded after each candidate evaluation. A change is retained only
when it is valid and improves the preceding valid result.

| Iterations | T1 keep ratio | T2 keep ratio | Local result | Local compression |
|---:|---:|---:|---|---:|
| 1 | 0.05 | 0.32 | INVALID (9/10; SSIM) | 90.000303% |
| 2 | 0.10 | 0.32 | VALID (10/10) | 86.422571% |
| 3–10 | 0.15–0.50 | 0.32 | VALID (10/10) | 82.420910% → 54.407160% |
| 11–16 | 0.55–0.80 | 0.22–0.32 | VALID (10/10) | 52.437504% → 30.447274% |

The best locally valid candidate was iteration 2. The sweep confirms that
increasing T1 retention monotonically reduces local compression while adding
SSIM margin. These are local diagnostics only; no Kattis score was obtained
because the submission service hostname could not be resolved.

## Post-mortem

- The local lemon baseline failed 6/10 scenarios because T1 retained too few
  vertices, not because of Hausdorff or topology failures.
- `v02.cpp` is the local smoke candidate to submit first: it is valid on all
  local ppsurf scenarios and has the highest locally valid compression.
- The planned Kattis comparison could not start: `submit_batch.py` failed with
  DNS error `No address associated with hostname`. Repeat the submission when
  the service is reachable; do not treat the local percentage as an official
  score.
- The next iteration should use Kattis feedback to tune the local evaluator
  and solver together, rather than treating local evaluation as the ranking
  oracle.

## Workflow smoke run (2026-07-10)

- Preflight checks passed: shell linting, Python syntax/imports, native
  evaluator builds, baseline compilation, and Markdown parsing.
- `v17.cpp` was evaluated sequentially on `data/ppsurf/`: **VALID (10/10)**,
  **26.061328%** mean compression. This is worse than the previous local
  `v16.cpp` result (30.447274%), which confirms that the higher T1/T2 keep
  ratios trade compression for already ample perceptual margin.
- A four-candidate local run (`v17`–`v20`) with the synthetic suite launched
  the native renderer concurrently. All four timed out at the fixed 120-second
  diagnostic timeout on `tier1_sharp_crystal`, so it produced no usable
  candidate comparison. Local candidates must be evaluated sequentially; the
  3–5 batch rule applies to immutable online submissions, not concurrent
  renderer-heavy local evaluations.
- `submit.py --help` and `submit_batch.py --help` both succeeded. No online
  submission was sent because no specific artifact was approved. The prior
  `banana-smoke.json` records that the service DNS lookup failed, so the next
  approved 3–5-candidate batch should first verify service reachability and
  retain its IDs-file for recovery.

### Workflow feedback

- The evaluator should handle a per-mesh diagnostic timeout as a structured
  invalid record and continue the suite, rather than terminating with a
  traceback and losing the JSON report.
- Add a batch-local evaluation script or skill that runs candidates
  sequentially, collects their JSON reports, ranks valid candidates, and
  selects an immutable 3–5-candidate online batch.
- Add a submission preflight skill that checks endpoint DNS/connectivity,
  validates all sources are immutable `.cpp` files, and creates the IDs-file
  before uploading. This would turn the earlier DNS failure into an immediate,
  actionable preflight result.
