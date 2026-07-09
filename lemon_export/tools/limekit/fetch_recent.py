#!/usr/bin/env python3
"""Fetch recent submissions for the current user from Kattis API."""
import argparse
import json
import subprocess
from collections import defaultdict

LIST_URL = "https://imc2-cvmaxxing.arturspace.dev/submissions"
USERNAME = "gasparyanartur"


def fetch_all(limit=200):
    """Fetch all submissions, paginating if needed."""
    items = []
    offset = 0
    while True:
        url = f"{LIST_URL}?limit={limit}&offset={offset}"
        r = subprocess.run(["curl", "-m", "10", "-s", url],
                          capture_output=True, text=True, timeout=15)
        data = json.loads(r.stdout)
        items.extend(data.get("items", []))
        if not data.get("has_more", False):
            break
        offset += limit
    return items


def filter_user(items, username):
    """Filter to one user."""
    return [s for s in items if s["username"] == username]


def filter_family(items, family):
    """Filter to one family."""
    return [s for s in items if s.get("family") == family]


def summarize(items):
    """Print summary stats."""
    by_family = defaultdict(list)
    for s in items:
        by_family[s.get("family", "none")].append(s)

    print(f"\n=== Summary ({len(items)} submissions) ===")
    for fam, subs in sorted(by_family.items(), key=lambda x: -len(x[1])):
        scores = [s["score"] for s in subs if s["score"] is not None]
        cases = [s["cases"] for s in subs if s["cases"]]
        pass_count = sum(1 for c in cases if c == "PPPPPPP")
        avg = sum(scores) / len(scores) if scores else 0
        max_score = max(scores) if scores else 0
        print(f"  {fam:<20} {len(subs):<5} | avg={avg:.3f} max={max_score:.3f} | {pass_count}/{len(subs)} pass")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--family", help="filter to one family")
    p.add_argument("--limit", type=int, default=200)
    p.add_argument("--json", help="save to JSON file")
    p.add_argument("--all", action="store_true", help="don't filter by user")
    args = p.parse_args()

    items = fetch_all(args.limit)
    if not args.all:
        items = filter_user(items, USERNAME)
    if args.family:
        items = filter_family(items, args.family)

    if args.json:
        with open(args.json, "w") as f:
            json.dump(items, f, indent=2)
        print(f"Saved {len(items)} items to {args.json}")

    summarize(items)
    print(f"\nLatest 10:")
    for s in items[:10]:
        print(f"  {s['id'][:8]} | {s.get('family','-'):<10} | {s['filename']:<25} | {s['status']:<10} | score={s['score']}")


if __name__ == "__main__":
    main()
