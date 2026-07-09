#!/usr/bin/env bash
# Build the native diagnostic evaluators used by scripts/evaluate.sh.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

out_dir="${EVALUATOR_BUILD_DIR:-$repo_root/build/evaluators}"
mkdir -p "$out_dir"

STATIC="${STATIC:-1}" scripts/build.sh evaluators/diagnostic_v3.cpp "$out_dir/diagnostic_v3"
STATIC="${STATIC:-1}" scripts/build.sh evaluators/diag_small.cpp "$out_dir/diag_small"
STATIC="${STATIC:-1}" scripts/build.sh evaluators/hausdorff_validator.cpp "$out_dir/hausdorff_validator"
STATIC="${STATIC:-1}" scripts/build.sh evaluators/mesh_validity.cpp "$out_dir/mesh_validity"