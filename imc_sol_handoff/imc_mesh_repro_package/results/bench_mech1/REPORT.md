# IMC multi-shape benchmark

Render proxy resolution: 128. Sampled Hausdorff is approximate.

| Shape | Solver | V in | V out | Compression | Final | Min normal | Min depth | H/sample diag | Manifold | Time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| cylinder_sharp | v23 | 24578 | 7373 | 70.002% | 0.87016 | 0.65417 | 0.92282 | 0.0000 | False | 9.12s |
| cylinder_sharp | v33 | 24578 | 7250 | 70.502% | 0.86892 | 0.61617 | 0.91708 | 0.0000 | False | 9.79s |
