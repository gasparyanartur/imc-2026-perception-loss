# IMC multi-shape benchmark

Render proxy resolution: 128. Sampled Hausdorff is approximate.

| Shape | Solver | V in | V out | Compression | Final | Min normal | Min depth | H/sample diag | Manifold | Time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| cone_sharp | v23 | 12290 | 3502 | 71.505% | 0.85533 | 0.64729 | 0.84841 | 0.0000 | False | 6.36s |
| cone_sharp | v33 | 12290 | 3502 | 71.505% | 0.85883 | 0.67740 | 0.85175 | 0.0000 | True | 6.48s |
| capsule | v23 | 8066 | 2379 | 70.506% | 0.99668 | 0.98975 | 0.99234 | 0.0000 | True | 6.68s |
| capsule | v33 | 8066 | 2373 | 70.580% | 0.99668 | 0.98973 | 0.99234 | 0.0000 | True | 10.03s |
