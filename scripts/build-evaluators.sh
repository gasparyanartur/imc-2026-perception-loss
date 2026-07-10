#!/usr/bin/env bash
# Build the unified native evaluator used by scripts/evaluate.sh.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

out_dir="${EVALUATOR_BUILD_DIR:-$repo_root/build/evaluators}"
mkdir -p "$out_dir"

STATIC="${STATIC:-1}" scripts/build.sh evaluators/evaluator.cpp "$out_dir/evaluator"