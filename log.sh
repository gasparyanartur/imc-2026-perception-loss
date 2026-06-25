#!/usr/bin/env bash
#
# log.sh -- append a timestamped engineering-log entry to log/<date>.txt.
#
# The log is the running record of *why* each change was made: the thoughts,
# hypotheses, plans, and measured results behind the current commit. One file
# per day (log/YYYY-MM-DD.txt); entries accumulate in chronological order and
# are tagged with the branch and short commit SHA so a note can always be tied
# back to the state of the tree it describes.
#
# Usage:
#   ./log.sh -t "Title" "body text ..."        # title + inline body
#   ./log.sh "body text ..."                    # body only
#   ./log.sh -t "Title" < notes.txt             # body from stdin / heredoc
#   some_command | ./log.sh -t "Eval result"    # pipe a command's output
#
# Configuration (environment variables):
#   LOG_DIR   directory for log files   (default: log)
set -u
export LC_ALL=C

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_root" || exit 1

log_dir=${LOG_DIR:-"log"}
mkdir -p "$log_dir"

# --- parse options ----------------------------------------------------------
title=""
while [ $# -gt 0 ]; do
    case "$1" in
        -t|--title) title="${2:-}"; shift 2 ;;
        --) shift; break ;;
        -*) echo "log.sh: unknown option '$1'" >&2; exit 2 ;;
        *) break ;;
    esac
done

# --- gather the body (remaining args, else stdin) ---------------------------
if [ $# -gt 0 ]; then
    body="$*"
elif [ ! -t 0 ]; then
    body="$(cat)"
else
    body=""
fi

if [ -z "$title" ] && [ -z "$body" ]; then
    echo "log.sh: nothing to log (provide a title and/or body)" >&2
    exit 2
fi

# --- git context so the entry is tied to the current commit -----------------
branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '?')"
sha="$(git rev-parse --short HEAD 2>/dev/null || echo '?')"
dirty=""
git diff --quiet 2>/dev/null || dirty=" +dirty"

date_file="$(date +%Y-%m-%d)"
stamp="$(date +%Y-%m-%dT%H:%M:%S%z)"
log_path="$log_dir/$date_file.txt"

{
    echo "## $stamp  [$branch @ $sha$dirty]"
    [ -n "$title" ] && echo "### $title"
    echo
    printf '%s\n' "$body"
    echo
} >> "$log_path"

echo "Appended entry to $log_path"
