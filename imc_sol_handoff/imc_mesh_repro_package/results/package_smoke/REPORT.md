# IMC multi-shape benchmark

Render proxy resolution: 64. Sampled Hausdorff is approximate.

| Shape | Solver | V in | V out | Compression | Final | Min normal | Min depth | H/sample diag | Manifold | Time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| sphere_smooth | v23 | 10242 | 2918 | 71.509% | 0.99636 | 0.99267 | 1.00000 | 0.0000 | True | 5.47s |
| sphere_smooth | current | 10242 | 2900 | 71.685% | 0.99635 | 0.99264 | 1.00000 | 0.0000 | True | 7.89s |
