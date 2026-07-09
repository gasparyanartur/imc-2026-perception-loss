#!/usr/bin/env python3
"""Poll Kattis submissions using proper queue_info timing."""
import argparse
import json
import subprocess
import sys
import time

LIST_URL = "https://imc2-cvmaxxing.arturspace.dev/submissions"
SINGLE_URL = "https://imc2-cvmaxxing.arturspace.dev/submission/{}"


def get_status(submission_id):
    """Get current status of a submission."""
    url = SINGLE_URL.format(submission_id)
    try:
        r = subprocess.run(["curl", "-m", "10", "-s", url],
                          capture_output=True, text=True, timeout=15)
        data = json.loads(r.stdout)
        if "detail" in data:
            return None
        return data
    except Exception as e:
        print(f"  Error: {e}", file=sys.stderr)
        return None


def poll_one(submission_id, max_wait=1800, label=None):
    """Poll a single submission until terminal. Returns final dict."""
    label = label or submission_id[:8]
    start = time.time()
    while time.time() - start < max_wait:
        d = get_status(submission_id)
        if d is None:
            time.sleep(5)
            continue
        status = d.get("status")
        if status in ("scored", "failed", "canceled"):
            print(f"  [{label}] {status}: score={d.get('score')} cases={d.get('cases')}")
            return d
        elif status == "evaluating":
            time.sleep(31)
        else:
            qi = d.get("queue_info", {})
            ss = qi.get("scheduler_status", {})
            retry = ss.get("retry_in_seconds", 0) or 0
            sleep_time = min(30, retry + 1)
            time.sleep(sleep_time)
    print(f"  [{label}] TIMEOUT after {max_wait}s", file=sys.stderr)
    return None


def poll_batch(submission_ids, max_wait=1800):
    """Poll a batch of submissions until all terminal."""
    results = [None] * len(submission_ids)
    pending = set(range(len(submission_ids)))
    start = time.time()
    while pending and time.time() - start < max_wait:
        for idx in list(pending):
            sid = submission_ids[idx]
            d = get_status(sid)
            if d is None:
                continue
            status = d.get("status")
            if status in ("scored", "failed", "canceled"):
                results[idx] = d
                pending.discard(idx)
                print(f"  [{sid[:8]}] {status}: score={d.get('score')} cases={d.get('cases')}")
        if not pending:
            break
        time.sleep(5)
    return results


def main():
    p = argparse.ArgumentParser()
    p.add_argument("submission_ids", nargs="+", help="One or more submission IDs to poll")
    p.add_argument("--max-wait", type=int, default=1800, help="Max seconds to wait (default 30 min)")
    p.add_argument("--batch", action="store_true", help="Poll all in parallel")
    args = p.parse_args()

    if args.batch and len(args.submission_ids) > 1:
        poll_batch(args.submission_ids, args.max_wait)
    else:
        for sid in args.submission_ids:
            poll_one(sid, args.max_wait)


if __name__ == "__main__":
    main()
