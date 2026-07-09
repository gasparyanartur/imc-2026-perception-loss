#!/usr/bin/env bash
# Compatibility wrapper. The canonical pipeline lives in scripts/evaluate.sh.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_root"

candidate="${CANDIDATE:-solutions/lemon/v115.cpp}"
args=(--candidate "$candidate")
if [[ -n "${DATASET_DIR:-}" ]]; then
    args+=(--dataset "$DATASET_DIR")
fi
if [[ "${INCLUDE_SYNTHETIC:-0}" == "1" ]]; then
    args+=(--include-synthetic)
fi
if [[ -n "${TIME_BUDGET:-}" ]]; then
    args+=(--time-budget "$TIME_BUDGET")
fi
exec scripts/evaluate.sh "${args[@]}"
