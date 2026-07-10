#!/usr/bin/env python3
"""Upload any number of C++ files, then wait for every submission to finish."""
from __future__ import annotations

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

# The scripts directory is on sys.path when this file is run directly.
from submit import (  # type: ignore[import-not-found]
    DEFAULT_PROBLEM,
    DEFAULT_TEAM_SECRET,
    DEFAULT_URL,
    DEFAULT_USERNAME,
    SubmissionError,
    submit_file,
)

ROOT = Path(__file__).resolve().parents[1]
TERMINAL_STATUSES = {
    "scored", "failed", "canceled", "cancelled", "error", "completed",
    "finished",
}


def _status_url(submit_url: str, submission_id: str) -> str:
    base = submit_url.rstrip("/")
    if base.endswith("/submit"):
        base = base[:-len("/submit")]
    return f"{base}/submission/{submission_id}"


def _get_json(url: str, teamsecret: str) -> dict:
    request = urllib.request.Request(
        url,
        headers={"Accept": "application/json", "X-Team-Secret": teamsecret},
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            result = json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", "replace")[:500]
        raise SubmissionError(f"HTTP {exc.code}: {detail}") from exc
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError,
            UnicodeDecodeError) as exc:
        raise SubmissionError(f"status request failed: {exc}") from exc
    if not isinstance(result, dict):
        raise SubmissionError(f"status response is not an object: {result!r}")
    if "detail" in result:
        raise SubmissionError(str(result["detail"]))
    return result


def _retry_seconds(result: dict) -> float | None:
    """Read the service's pending retry interval from known response shapes."""
    candidates = [
        result.get("retry_in"),
        result.get("retry_in_seconds"),
        result.get("retry-in"),
        result.get("retry-in-seconds"),
    ]
    queue = result.get("queue_info")
    if isinstance(queue, dict):
        candidates.extend([
            queue.get("retry_in"),
            queue.get("retry_in_seconds"),
        ])
        scheduler = queue.get("scheduler_status")
        if isinstance(scheduler, dict):
            candidates.extend([
                scheduler.get("retry_in"),
                scheduler.get("retry_in_seconds"),
            ])
    for value in candidates:
        try:
            if value is not None:
                return max(0.0, float(value))
        except (TypeError, ValueError):
            continue
    return None


def _write_batch(path: Path, batch: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(batch, indent=2, sort_keys=True) + "\n")


def _poll_batch(batch: dict, *, ids_file: Path, teamsecret: str,
                status_base_url: str, max_wait: float) -> bool:
    started = time.monotonic()
    pending = {
        item["id"]: item for item in batch["submissions"] if item.get("id")
    }
    while pending:
        if max_wait > 0 and time.monotonic() - started >= max_wait:
            print("batch: maximum wait reached", file=sys.stderr)
            return False

        next_wait: float | None = None
        for submission_id, item in list(pending.items()):
            try:
                result = _get_json(
                    _status_url(status_base_url, submission_id), teamsecret)
            except SubmissionError as exc:
                print(f"[{submission_id[:8]}] status error: {exc}",
                      file=sys.stderr)
                delay = 30.0
                next_wait = delay if next_wait is None else min(next_wait, delay)
                continue

            item["status_response"] = result
            status = str(result.get("status", "unknown")).lower()
            item["status"] = status
            if status in TERMINAL_STATUSES:
                item["finished_at"] = datetime.now(timezone.utc).isoformat()
                pending.pop(submission_id)
                print(f"[{submission_id[:8]}] {status}: score={result.get('score')} "
                      f"cases={result.get('cases')}")
                continue

            retry = _retry_seconds(result)
            # The service tells us how long until a pending submission can be
            # considered again. Add the requested 30-second safety margin.
            delay = (retry + 30.0) if retry is not None else 30.0
            next_wait = delay if next_wait is None else min(next_wait, delay)
            print(f"[{submission_id[:8]}] {status}; retrying in "
                  f"{delay:.0f}s", flush=True)

        _write_batch(ids_file, batch)
        if pending:
            delay = max(0.0, next_wait if next_wait is not None else 30.0)
            if max_wait > 0:
                remaining = max_wait - (time.monotonic() - started)
                delay = min(delay, max(0.0, remaining))
            time.sleep(delay)
    _write_batch(ids_file, batch)
    return True


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", type=Path,
                        help="4–6 C++ source files")
    parser.add_argument("--family",
                        help="optional family label, recommended for filtering")
    parser.add_argument("--teamsecret", "--team-secret",
                        default=os.environ.get("SUBMIT_TEAM_SECRET",
                                               DEFAULT_TEAM_SECRET))
    parser.add_argument("--problem", default=os.environ.get(
        "KATTIS_PROBLEM", DEFAULT_PROBLEM))
    parser.add_argument("--username", default=os.environ.get(
        "KATTIS_USERNAME", DEFAULT_USERNAME))
    parser.add_argument("--url", default=os.environ.get("SUBMIT_URL", DEFAULT_URL),
                        help=argparse.SUPPRESS)
    parser.add_argument("--ids-file", type=Path,
                        help="where to store IDs and final results")
    parser.add_argument("--max-wait", type=float, default=0,
                        help="optional timeout in seconds; default: wait forever")
    args = parser.parse_args(argv)
    if not 4 <= len(args.files) <= 6:
        parser.error("a batch must contain 4–6 source files")

    ids_file = args.ids_file or (
        ROOT / "data/submission-batches" /
        f"batch-{datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S')}.json"
    )
    batch = {
        "created_at": datetime.now(timezone.utc).isoformat(),
        "family": args.family,
        "problem": args.problem,
        "username": args.username,
        "submissions": [],
    }
    _write_batch(ids_file, batch)
    failed = False
    for path in args.files:
        item = {"file": str(path), "filename": path.name}
        try:
            response = submit_file(
                path,
                teamsecret=args.teamsecret,
                problem=args.problem,
                family=args.family,
                username=args.username,
                url=args.url,
            )
            item["id"] = response["id"]
            item["status"] = response.get("status", "submitted")
            print(f"SUBMITTED: {path} -> {item['id']}")
        except SubmissionError as exc:
            item["status"] = "upload_failed"
            item["error"] = str(exc)
            failed = True
            print(f"batch: {path}: {exc}", file=sys.stderr)
        batch["submissions"].append(item)
        _write_batch(ids_file, batch)

    ids = [item.get("id") for item in batch["submissions"] if item.get("id")]
    print(f"Stored {len(ids)} submission ID(s) in {ids_file}")
    if ids and not _poll_batch(batch, ids_file=ids_file,
                               teamsecret=args.teamsecret,
                               status_base_url=args.url,
                               max_wait=args.max_wait):
        failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
