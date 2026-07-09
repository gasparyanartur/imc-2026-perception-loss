#!/usr/bin/env python3
"""Submit C++ code to Kattis with proper logging."""
import argparse
import csv
import json
import subprocess
import sys
from datetime import datetime
from pathlib import Path

CSV_PATH = Path("/workspace/data/submissions.csv")
SUBMIT_URL = "https://imc2-cvmaxxing.arturspace.dev/submit"
TEAM_SECRET = "cvmaxxing-95"

def log_submission(filename, family, hypothesis, predicted_range, predicted_cases="PPPPPPP"):
    """Append a row to submissions.csv."""
    new_file = not CSV_PATH.exists()
    with CSV_PATH.open("a", newline="") as f:
        w = csv.writer(f)
        if new_file:
            w.writerow(["timestamp", "family", "filename", "hypothesis", "predicted", "predicted_cases", "score", "cases", "kattis_id", "notes"])
        w.writerow([datetime.utcnow().isoformat(), family, filename, hypothesis, predicted_range, predicted_cases, "", "", "", ""])

def update_submission(filename, score, cases, kattis_id, notes=""):
    """Update the last row for filename with the result."""
    if not CSV_PATH.exists():
        return
    rows = list(csv.reader(CSV_PATH.open()))
    if len(rows) < 2:
        return
    for i in range(len(rows) - 1, 0, -1):
        if rows[i][2] == filename and not rows[i][6]:
            rows[i][6] = score
            rows[i][7] = cases
            rows[i][8] = kattis_id
            rows[i][9] = notes
            break
    with CSV_PATH.open("w", newline="") as f:
        csv.writer(f).writerows(rows)

def submit(code_path, filename, family, hypothesis, predicted_range, predicted_cases="PPPPPPP",
           username="gasparyanartur", problem_id="simplifygeometry", priority="normal"):
    """Submit code to Kattis. Logs to submissions.csv."""
    code_path = Path(code_path).absolute()
    if not code_path.exists():
        print(f"ERROR: {code_path} does not exist", file=sys.stderr)
        return None

    log_submission(filename, family, hypothesis, predicted_range, predicted_cases)

    cmd = [
        "curl", "-s", "-m", "30", "-X", "POST", SUBMIT_URL,
        "-H", f"X-Team-Secret: {TEAM_SECRET}",
        "-F", f"username={username}",
        "-F", f"problem_id={problem_id}",
        "-F", f"filename={filename}",
        "-F", f"family={family}",
        "-F", f"priority={priority}",
        "-F", f"code=<{code_path}",
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    if result.returncode != 0:
        print(f"ERROR submitting: {result.stderr}", file=sys.stderr)
        return None
    try:
        resp = json.loads(result.stdout)
        submission_id = resp.get("id")
        if resp.get("duplicate"):
            print(f"DUPLICATE: {filename} already submitted as {submission_id}")
        else:
            print(f"Submitted: {filename} → {submission_id} ({resp.get('status')})")
        return submission_id
    except json.JSONDecodeError:
        print(f"Bad response: {result.stdout[:200]}", file=sys.stderr)
        return None

def main():
    p = argparse.ArgumentParser()
    p.add_argument("code", help="path to .cpp file")
    p.add_argument("filename", help="filename for Kattis")
    p.add_argument("family", help="family name")
    p.add_argument("--hypothesis", required=True, help="one-sentence hypothesis")
    p.add_argument("--predicted", required=True, help="predicted score range, e.g. '89.30-89.35'")
    p.add_argument("--cases", default="PPPPPPP", help="predicted case pattern")
    p.add_argument("--username", default="gasparyanartur")
    args = p.parse_args()
    submit(args.code, args.filename, args.family,
           args.hypothesis, args.predicted, args.cases, args.username)

if __name__ == "__main__":
    main()
