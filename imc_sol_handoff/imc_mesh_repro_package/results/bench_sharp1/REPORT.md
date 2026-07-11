# IMC multi-shape benchmark

Render proxy resolution: 128. Sampled Hausdorff is approximate.

| Shape | Solver | V in | V out | Compression | Final | Min normal | Min depth | H/sample diag | Manifold | Time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| cube_planar_sharp | v23 | 6146 | 1813 | 70.501% | 0.87768 | 0.75949 | 0.94179 | 0.0000 | False | 6.87s |
| cube_planar_sharp | v33 | 6146 | 1813 | 70.501% | 0.87554 | 0.76621 | 0.94286 | 0.0000 | False | 7.10s |
| thin_box_sharp | v23 | 6146 | 1843 | 70.013% | 0.80547 | 0.45087 | 0.95509 | 0.0000 | False | 6.30s |
| thin_box_sharp | v33 | 6146 | 1641 | 73.300% | 0.80354 | 0.49339 | 0.94922 | 0.0000 | True | 3.34s |
