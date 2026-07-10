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
