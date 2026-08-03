# IMC2026 Evaluation Protocol

## 1. Evaluation layers

### Layer A — source/build

Record:

```text
source bytes
warning-build result
static optimized-build result
compiler warnings
```

Commands:

```bash
g++ -std=c++17 -O1 -Wall -Wextra -pipe solution.cpp -o solution_warn
g++ -std=c++17 -O2 -pipe -static -s solution.cpp -o solution
wc -c solution.cpp
```

Submission source must remain below 100,000 bytes.

### Layer B — structural validity

For every output, check:

```text
1 <= output vertices <= input vertices
valid 1-based face indices
three distinct vertices per face
positive world-space triangle area
no duplicate faces
every undirected edge has exactly two incident faces
opposite directed orientation across the two incident faces
no unused output vertices
Euler characteristic as a diagnostic
```

Use `evaluation/mesh_validate.py`.

### Layer C — deterministic synthetic suite

Use meshes near the official medium tiers. The included generator/runner creates:

```text
smooth/spiky torus knot
corrugated torus
bumpy sphere
rounded cube
pinched sphere
small tetrahedron and octahedron sanity meshes
```

Synthetic results diagnose mechanisms. They do not establish official correctness.

### Layer D — internal renderer diagnostics

When a solution supports diagnostic output, record:

```text
mean SSIM
minimum view SSIM
minimum normal SSIM
minimum depth SSIM
resolution used
reference mesh used
```

A 256/512 diagnostic is a ranking signal, not the official 1024 result.

### Layer E — branch funnel instrumentation

For every experimental phase, report:

```text
eligible
entered
seed faces tested
smooth probes passed
regions emitted
region face/interior/boundary distributions
parameterizations passed
fits passed
replacement attempts
valid replacements
batch patches
predicted vertices removed
full audit started
full audit score
batch committed
actual vertices removed
later QEM vertices removed
final output differs from control
```

Without these counters, an unchanged score is ambiguous.

### Layer F — official submission

Record the full official result and compare it against the predicted signal.

## 2. Required controls

Every experimental version should be compared against:

1. The reliable official incumbent.
2. A same-source feature-disabled control when possible.
3. The previous version.

The same-flow control is essential. If spline v6 produces 3,638 vertices and the same v6 flow without spline produces 3,719, spline contributed 81 vertices even if Mango v20 still produces 3,589.

## 3. Timing accounting

Report phase timing separately:

```text
input and setup
reference render
QEM bootstrap
region discovery
parameterization
spline fitting
sampling/Lloyd
triangulation/edge optimization
low-resolution ranking
full audit
final QEM
output
```

A method that removes vertices but prevents later QEM from running is not automatically beneficial.

## 4. Rate-distortion accounting

For every accepted operation or batch:

```text
V_before
V_after
removed
score_before
score_after
SSIM_loss
seconds
removed_per_second
removed_per_SSIM_loss
```

When SSIM loss is numerically zero at the diagnostic resolution, record it as below-resolution rather than infinite quality.

## 5. Official score interpretation

The competition score is driven by output vertex ratios on passing cases. An unchanged aggregate can mean:

- no output changed;
- changed outputs rounded below displayed precision;
- gains on one tier were offset by losses elsewhere;
- an experimental branch committed but later logic neutralized the gain;
- a branch was never entered;
- a candidate failed SSIM or validity and rolled back.

Use case patterns, expected ratio changes, and score fingerprints to distinguish these.

## 6. Experiment ledger fields

Use `evaluation/experiment_log.csv` with:

```text
date
family
version
source_sha256
pseudocode_version
hypothesis
algorithmic_change
parameters_changed
expected_affected_tiers
expected_signal
local_suite_summary
source_bytes
runtime_summary
submission_id
case_pattern
official_score
interpretation
next_action
```

## 7. No-claim policy

Do not state that a version is better unless one of these is true:

- official all-pass score is higher;
- on a controlled local fixture it produces fewer valid vertices at comparable or better internal quality;
- a mechanism-specific diagnostic improved exactly as predicted.

Compilation, activation, or accepted SSIM alone is not an improvement claim.
