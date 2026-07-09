#!/usr/bin/env python3
"""Upload one C++ submission to the IMC submission service."""
from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.error
import urllib.request
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_URL = "https://imc2-cvmaxxing.arturspace.dev/submit"
DEFAULT_TEAM_SECRET = "cvmaxxing-95"
DEFAULT_PROBLEM = "simplifygeometry"
DEFAULT_USERNAME = "gasparynaartur"


class SubmissionError(RuntimeError):
    """Raised when the submission service rejects or cannot receive a file."""


def _multipart(fields: dict[str, str], file_field: str, filename: str,
               content: bytes) -> tuple[bytes, str]:
    boundary = f"----imc-submit-{uuid.uuid4().hex}"
    chunks: list[bytes] = []
    for name, value in fields.items():
        chunks.extend([
            f"--{boundary}\r\n".encode(),
            f'Content-Disposition: form-data; name="{name}"\r\n\r\n'.encode(),
            value.encode(),
            b"\r\n",
        ])
    chunks.extend([
        f"--{boundary}\r\n".encode(),
        (f'Content-Disposition: form-data; name="{file_field}"; '
         f'filename="{filename}"\r\n').encode(),
        b"Content-Type: text/plain\r\n\r\n",
        content,
        b"\r\n",
        f"--{boundary}--\r\n".encode(),
    ])
    return b"".join(chunks), f"multipart/form-data; boundary={boundary}"


def submit_file(path: Path, *, teamsecret: str, problem: str, family: str,
                username: str, url: str = DEFAULT_URL) -> dict:
    """Upload ``path`` and return the decoded service response."""
    path = path.expanduser().resolve()
    if not path.is_file():
        raise SubmissionError(f"file does not exist: {path}")
    if path.suffix.lower() != ".cpp":
        raise SubmissionError(f"only C++ source files (*.cpp) are supported: {path}")
    if not teamsecret:
        raise SubmissionError("teamsecret must not be empty")
    if not family:
        raise SubmissionError("family must not be empty")

    body, content_type = _multipart(
        {
            "username": username,
            "problem_id": problem,
            "filename": path.name,
            "family": family,
        },
        "code",
        path.name,
        path.read_bytes(),
    )
    request = urllib.request.Request(
        url,
        data=body,
        method="POST",
        headers={
            "Content-Type": content_type,
            "X-Team-Secret": teamsecret,
            "Accept": "application/json",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            raw = response.read()
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", "replace")[:500]
        raise SubmissionError(f"HTTP {exc.code}: {detail}") from exc
    except (urllib.error.URLError, TimeoutError) as exc:
        raise SubmissionError(f"request failed: {exc}") from exc

    try:
        result = json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise SubmissionError(f"service returned invalid JSON: {raw[:300]!r}") from exc
    if not isinstance(result, dict):
        raise SubmissionError(f"service returned unexpected JSON: {result!r}")
    if not result.get("id"):
        raise SubmissionError(f"service response has no submission id: {result!r}")
    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path, help="C++ source file to upload")
    parser.add_argument("--teamsecret", "--team-secret",
                        default=os.environ.get("SUBMIT_TEAM_SECRET",
                                               DEFAULT_TEAM_SECRET))
    parser.add_argument("--problem", default=os.environ.get(
        "KATTIS_PROBLEM", DEFAULT_PROBLEM))
    parser.add_argument("--family", required=True)
    parser.add_argument("--username", default=os.environ.get(
        "KATTIS_USERNAME", DEFAULT_USERNAME))
    parser.add_argument("--url", default=os.environ.get("SUBMIT_URL", DEFAULT_URL),
                        help=argparse.SUPPRESS)
    args = parser.parse_args(argv)

    try:
        result = submit_file(args.file, teamsecret=args.teamsecret,
                             problem=args.problem, family=args.family,
                             username=args.username, url=args.url)
    except SubmissionError as exc:
        print(f"submit: ERROR: {exc}", file=sys.stderr)
        return 1

    submission_id = result["id"]
    status = result.get("status", "unknown")
    if result.get("duplicate"):
        print(f"DUPLICATE: {args.file.name} -> {submission_id} ({status})")
    else:
        print(f"SUBMITTED: {args.file.name} -> {submission_id} ({status})")
    print(f"SUBMISSION_ID={submission_id}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
