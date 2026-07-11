#!/usr/bin/env bash
# Run the canonical local evaluation pipeline.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if [[ "${SKIP_BUILD_EVALUATORS:-0}" != "1" ]]; then
    scripts/build-evaluators.sh >/dev/null
fi

outputs_dir="${OUTPUTS_DIR:-outputs}"
mkdir -p "$outputs_dir"
timestamp="$(date +%Y%m%d-%H%M%S%N)"
tmp_log="$(mktemp)"
trap 'rm -f "$tmp_log"' EXIT

set +e
python3 scripts/evaluate_candidate.py "$@" | tee "$tmp_log"
status="${PIPESTATUS[0]}"
set -e

rate="$(grep -m1 '^COMPRESSION_RATE=' "$tmp_log" | cut -d= -f2 || true)"
label="metrics"
if [[ -n "$rate" ]]; then
    label="${label}-compr-${rate}"
fi
log_path="$outputs_dir/native-${timestamp}-${label}.txt"
cp "$tmp_log" "$log_path"
echo "Logged result to $log_path"
exit "$status"