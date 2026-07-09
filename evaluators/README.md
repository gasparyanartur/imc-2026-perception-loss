# Native C++ evaluator suite

The sources in this directory implement the complete local acceptance path.
They are compiled into `build/evaluators/`, which is ignored by Git.

- `diagnostic_v3.cpp`: native 1024-pixel normal/depth SSIM diagnostic;
- `diag_small.cpp`: 256-pixel diagnostic for fast experiments;
- `hausdorff_validator.cpp`: sampled bidirectional surface-distance check;
- `mesh_validity.cpp`: topology, indexing, degeneracy, and vertex-count gate.

Build the suite with:

```sh
scripts/build-evaluators.sh
```

The orchestration layer accepts only `.cpp` candidate sources and requires the
native validity, perceptual, and surface-distance components for acceptance.
