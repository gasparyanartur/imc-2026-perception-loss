#!/usr/bin/env python3
"""Analyze submission history, find champion, compute pass rates."""
import csv
import argparse
import re
from collections import defaultdict
from pathlib import Path

CSV_PATH = Path("/workspace/data/submissions.csv")

def load_data():
    """Load all submissions as a list of dicts."""
    if not CSV_PATH.exists():
        return []
    rows = []
    with CSV_PATH.open() as f:
        for row in csv.DictReader(f):
            try:
                row["score_f"] = float(row.get("score", "").strip() or "nan")
            except ValueError:
                row["score_f"] = None
            rows.append(row)
    return rows

def normalize_version(v):
    """Normalize version: 'v9-rerun1' -> 'v9', 'lime-v9' -> 'v9'."""
    # Strip suffix like -rerun, -control, -r1, -r2
    m = re.match(r'^(.+?)(?:-\w+\d*)?$', v)
    if m:
        # Strip prefix family name like 'lime-' if it matches the canonical pattern
        s = m.group(1)
        # Try to extract just the version number
        n = re.search(r'(v\d+)', s)
        if n:
            return n.group(1)
    return v

def find_champion(min_attempts=1):
    """Find the version with best (avg score) AND high pass rate."""
    rows = load_data()

    by_version = defaultdict(list)
    for r in rows:
        if r.get("score_f") is not None and r.get("score_f") == r.get("score_f"):
            base = normalize_version(r["version"])
            by_version[base].append(r)

    print(f"{'Version':<10} {'Avg':<8} {'StdDev':<8} {'PassRate':<10} {'N':<5} {'Cases':<10} {'Notes'}")
    print("-" * 85)
    candidates = []
    for ver, attempts in sorted(by_version.items()):
        scores = [a["score_f"] for a in attempts]
        cases = [a.get("cases", "") for a in attempts]
        pass_count = sum(1 for c in cases if c == "PPPPPPP")
        pass_rate = pass_count / len(attempts) if attempts else 0
        avg = sum(scores) / len(scores)
        stddev = (sum((s - avg)**2 for s in scores) / len(scores))**0.5 if len(scores) > 1 else 0
        lesson = attempts[-1].get("lesson", "")[:30]
        candidates.append((ver, avg, stddev, pass_rate, len(attempts), cases[0] if cases else "", lesson))
        if len(attempts) >= 1:
            print(f"{ver:<10} {avg:<8.3f} {stddev:<8.3f} {pass_rate*100:<10.0f} {len(attempts):<5} {cases[0]:<10} {lesson}")
    print("-" * 85)

    candidates.sort(key=lambda x: -x[1])
    if candidates:
        print(f"\nBest by avg score: {candidates[0][0]} = {candidates[0][1]:.3f} (std {candidates[0][2]:.3f}, {candidates[0][4]} runs, {candidates[0][3]*100:.0f}% pass)")

    reliable = [c for c in candidates if c[3] >= 0.5 and c[4] >= 2]
    if reliable:
        reliable.sort(key=lambda x: -x[1])
        print(f"Most reliable (≥50% pass rate, ≥2 runs): {reliable[0][0]} = {reliable[0][1]:.3f} ({reliable[0][4]} runs, {reliable[0][3]*100:.0f}% pass)")

    noisy = [c for c in candidates if c[4] >= 3]
    if noisy:
        print(f"\nNoise analysis (≥3 runs):")
        for c in noisy:
            print(f"  {c[0]}: stddev={c[2]:.3f} (lower = more deterministic)")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--min-attempts", type=int, default=1)
    args = p.parse_args()
    find_champion(args.min_attempts)

if __name__ == "__main__":
    main()
