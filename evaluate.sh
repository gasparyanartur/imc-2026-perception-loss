#!/usr/bin/env bash
#
# evaluate.sh -- run the Python solution, score it with the offline evaluator,
# log the result, and check that the new score beats previous runs.
#
# Pipeline (Python only, for now):
#   1. Run the solver (solution.py) on the input mesh to produce a simplified mesh.
#   2. Score it with evaluate.py against the original mesh.
#   3. Append a timestamped log to outputs/<date>-<result>.txt, where <result>
#      is the metric (compression rate) on success or "error"/"invalid" on failure.
#   4. Compare the new compression rate against the best previous VALID run and
#      report whether the new model improved.
#
# The ranking objective is CompressionRate (higher is better), counted only for
# submissions that pass every validity gate (see docs/evaluation.md).
#
# Configuration (environment variables):
#   SCRIPT_FILE  solver script to run            (default: solution.py)
#   INPUT_PATH   original mesh fed to the solver (default: data/sample-input.txt)
#   OUTPUTS_DIR  directory for logs              (default: outputs)
#   EVAL_SCRIPT  evaluator script                (default: evaluate.py)
#   RESOLUTION   render resolution               (default: evaluate.py default)
#   PYTHON       python interpreter              (default: python3)
#
# Exit codes:
#   0  valid submission AND not worse than the previous best
#   1  invalid submission, evaluator/solver error, or a regression vs. best
set -u

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_root" || exit 1

script_file=${SCRIPT_FILE:-"solution.py"}
input_path=${INPUT_PATH:-"data/sample-input.txt"}
outputs_dir=${OUTPUTS_DIR:-"outputs"}
eval_script=${EVAL_SCRIPT:-"evaluate.py"}
python_bin=${PYTHON:-"python3"}

mkdir -p "$outputs_dir"

# Returns success only if the argument is a plain decimal number. Used to guard
# values that are interpolated into awk expressions (defense against malformed
# or tampered log files).
is_number() {
    case "$1" in
        ''|*[!0-9.+-]*) return 1 ;;
        *) [ "$(printf '%s' "$1" | tr -cd '.' | wc -c)" -le 1 ] ;;
    esac
}

timestamp="$(date +%Y%m%d-%H%M%S)"
simplified_path="$outputs_dir/py-$timestamp.mesh.txt"

# --- helper: compute best previous VALID compression rate -------------------
# Scans existing logs (before this run) for VALID results and prints the max
# COMPRESSION_RATE, or nothing if there is no prior valid run.
best_previous_rate() {
    local best="" rate result f
    for f in "$outputs_dir"/*.txt; do
        [ -e "$f" ] || continue
        grep -q '^RESULT=VALID$' "$f" || continue
        rate="$(grep -m1 '^COMPRESSION_RATE=' "$f" | cut -d= -f2)"
        is_number "$rate" || continue
        if [ -z "$best" ] || awk "BEGIN{exit !($rate > $best)}"; then
            best="$rate"
        fi
    done
    printf '%s' "$best"
}

# Capture the best previous rate BEFORE writing this run's log.
prev_best="$(best_previous_rate)"

# --- helper: finalize a log file under the result-tagged name ---------------
write_log() {
    # $1 = result label used in the filename; remaining args ignored.
    local label="$1"
    local log_path="$outputs_dir/$timestamp-$label.txt"
    mv "$tmp_log" "$log_path"
    echo "Logged result to $log_path"
}

tmp_log="$(mktemp)"
trap 'rm -f "$tmp_log"' EXIT

{
    echo "# evaluate.sh run $timestamp"
    echo "SOLVER=$script_file"
    echo "INPUT=$input_path"
    echo "EVALUATOR=$eval_script"
    echo "RESOLUTION=${RESOLUTION:-default}"
    echo
} >> "$tmp_log"

# --- step 1: run the solver -------------------------------------------------
solver_err="$(mktemp)"
if ! "$python_bin" "$script_file" < "$input_path" > "$simplified_path" 2> "$solver_err"; then
    {
        echo "RESULT=ERROR"
        echo "STAGE=solver"
        echo "--- solver stderr ---"
        cat "$solver_err"
    } >> "$tmp_log"
    rm -f "$solver_err"
    cat "$tmp_log"
    write_log "error"
    echo "FAILED: solver ($script_file) returned a non-zero exit code." >&2
    exit 1
fi
rm -f "$solver_err"

# --- step 2: score the simplified mesh --------------------------------------
eval_args=("$eval_script" "$input_path" "$simplified_path" "--summary")
if [ -n "${RESOLUTION:-}" ]; then
    eval_args+=("--resolution" "$RESOLUTION")
fi

# A single --summary run prints the full human report followed by a
# machine-readable KEY=VALUE block.
everr="$(mktemp)"
output="$("$python_bin" "${eval_args[@]}" 2> "$everr")"
summary="$(printf '%s\n' "$output" | grep -E '^[A-Z_]+=')"

if [ -z "$summary" ]; then
    {
        echo "RESULT=ERROR"
        echo "STAGE=evaluator"
        echo "--- evaluator stderr ---"
        cat "$everr"
    } >> "$tmp_log"
    rm -f "$everr"
    cat "$tmp_log"
    write_log "error"
    echo "FAILED: evaluator ($eval_script) did not produce a summary." >&2
    exit 1
fi
rm -f "$everr"

# Persist the full report (which already contains the summary block).
{
    echo "$output"
} >> "$tmp_log"

# --- step 3: parse the result -----------------------------------------------
result="$(printf '%s\n' "$summary" | grep -m1 '^RESULT=' | cut -d= -f2)"
new_rate="$(printf '%s\n' "$summary" | grep -m1 '^COMPRESSION_RATE=' | cut -d= -f2)"

cat "$tmp_log"

if [ "$result" != "VALID" ]; then
    write_log "invalid"
    echo "RESULT: INVALID submission -- not eligible for ranking." >&2
    exit 1
fi

# A VALID result must carry a numeric compression rate; guard before using it
# in awk expressions and filenames.
if ! is_number "$new_rate"; then
    write_log "error"
    echo "FAILED: evaluator reported VALID but no numeric COMPRESSION_RATE." >&2
    exit 1
fi

# Valid submission: tag the log filename with the compression metric.
rate_label="$(awk "BEGIN{printf \"compr-%.4f\", $new_rate}")"
write_log "$rate_label"

# --- step 4: compare against the best previous valid run --------------------
if [ -z "$prev_best" ] || ! is_number "$prev_best"; then
    echo "RESULT: VALID. CompressionRate=$new_rate% (no previous valid run to compare)."
    exit 0
fi

echo "RESULT: VALID. CompressionRate=$new_rate% vs previous best=$prev_best%"
if awk "BEGIN{exit !($new_rate > $prev_best)}"; then
    echo "IMPROVED: new model beats the previous best by $(awk "BEGIN{printf \"%.4f\", $new_rate - $prev_best}") points."
    exit 0
else
    echo "NOT IMPROVED: new model does not beat the previous best ($new_rate% <= $prev_best%)." >&2
    exit 1
fi
