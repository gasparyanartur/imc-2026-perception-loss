#!/usr/bin/env python3
"""Evaluate a mesh-simplification solver across a whole dataset of meshes.

The single-pair evaluator (``evaluate.py``) scores one ``(original,
simplified)`` mesh pair. That is not enough to tell whether a solver
generalizes: a solution can pass on a trivial cube yet fail on most real
meshes. This harness runs the solver on **every** mesh in a representative
dataset (see ``datasets/prepare_ppsurf.py`` and ``data/ppsurf``), scores each
one with the ``evaluate.py`` pipeline, and reports how many scenarios passed
plus the aggregate compression rate.

For each input mesh it:

  1. runs ``SOLVER < input > simplified`` (a subprocess, like the real grader);
  2. scores the pair with :func:`evaluate.evaluate`;
  3. records the per-scenario verdict, compression, Hausdorff and SSIM.

It then prints a per-scenario table and a machine-readable ``KEY=VALUE``
summary. The overall ``RESULT`` is ``VALID`` only when **every** scenario is
valid (``SCENARIOS_PASSED == SCENARIOS_TOTAL``); the aggregate
``COMPRESSION_RATE`` is the mean over all scenarios. The process exits ``0``
only if every scenario passed.

Usage::

    python3 evaluate_dataset.py [--solver solution.py] [--dataset data/ppsurf]
        [--resolution N] [--summary] [--quiet] [--timeout SECONDS]

Only NumPy is required (SciPy speeds up the Hausdorff step when available).
"""

from __future__ import annotations

import argparse
import glob
import os
import subprocess
import sys
import tempfile

import evaluate

# Default render resolution for the multi-mesh harness. The native grader uses
# 1024 (see docs/report.md 2.2); a smaller default keeps iteration over many
# meshes fast. Override with --resolution 1024 for real-grader-like scores.
DEFAULT_RESOLUTION = 256
DEFAULT_TIMEOUT = 300.0


def find_inputs(dataset):
    """Return the sorted list of input mesh files for a dataset path."""
    if os.path.isfile(dataset):
        return [dataset]
    if os.path.isdir(dataset):
        return sorted(glob.glob(os.path.join(dataset, "*.txt")))
    return []


def run_solver(python_bin, solver, input_path, output_path, timeout):
    """Run ``solver`` on ``input_path`` -> ``output_path``.

    Returns ``(ok, message)``; ``ok`` is False if the solver failed or timed
    out, with ``message`` describing the cause.
    """
    with open(input_path, "rb") as fin, open(output_path, "wb") as fout:
        try:
            proc = subprocess.run(
                [python_bin, solver],
                stdin=fin,
                stdout=fout,
                stderr=subprocess.PIPE,
                timeout=timeout,
                check=False,
            )
        except subprocess.TimeoutExpired:
            return False, "solver timed out after %.0fs" % timeout
    if proc.returncode != 0:
        err = proc.stderr.decode("utf-8", "replace").strip().splitlines()
        tail = err[-1] if err else "non-zero exit"
        return False, "solver error: %s" % tail
    return True, "ok"


class Scenario:
    """Per-mesh evaluation outcome."""

    def __init__(self, name):
        self.name = name
        self.valid = False
        self.compression = 0.0
        self.hausdorff = float("nan")
        self.final_ssim = float("nan")
        self.note = ""


def evaluate_dataset(solver, inputs, resolution, python_bin, timeout, tmp_dir):
    """Run and score the solver over every input mesh; return the scenarios."""
    scenarios = []
    for input_path in inputs:
        name = os.path.splitext(os.path.basename(input_path))[0]
        scenario = Scenario(name)
        output_path = os.path.join(tmp_dir, name + ".out.txt")

        ok, message = run_solver(python_bin, solver, input_path, output_path,
                                 timeout)
        if not ok:
            scenario.note = message
            scenarios.append(scenario)
            continue

        try:
            original = evaluate.load_mesh(input_path)
            simplified = evaluate.load_mesh(output_path)
            result = evaluate.evaluate(original, simplified,
                                       resolution=resolution)
        except Exception as exc:  # noqa: BLE001 - a bad mesh fails the scenario
            scenario.note = "evaluator error: %s" % exc
            scenarios.append(scenario)
            continue

        scenario.valid = result.valid
        scenario.compression = result.compression_rate
        scenario.hausdorff = result.hausdorff
        scenario.final_ssim = result.final_ssim
        if not result.valid:
            reasons = list(result.validity.reasons)
            if result.final_ssim < evaluate.SSIM_THRESHOLD:
                reasons.append("FinalSSIM %.4f < %.2f"
                               % (result.final_ssim, evaluate.SSIM_THRESHOLD))
            if result.hausdorff > result.hausdorff_bound:
                reasons.append("Hausdorff %.4f > %.4f"
                               % (result.hausdorff, result.hausdorff_bound))
            scenario.note = "; ".join(reasons) if reasons else "invalid"
        scenarios.append(scenario)
    return scenarios


def aggregate(scenarios):
    """Compute aggregate statistics over the scenarios."""
    total = len(scenarios)
    passed = sum(1 for s in scenarios if s.valid)
    if total:
        mean_compression = sum(s.compression for s in scenarios) / total
        min_compression = min(s.compression for s in scenarios)
    else:
        mean_compression = 0.0
        min_compression = 0.0
    return total, passed, mean_compression, min_compression


def print_report(scenarios, total, passed, mean_c, min_c, resolution):
    print("=" * 72)
    print("IMC 2026 dataset evaluation (%d scenarios, resolution %d)"
          % (total, resolution))
    print("=" * 72)
    print("%-22s %-7s %10s %10s %10s   %s"
          % ("scenario", "result", "compr%", "hausdorff", "ssim", "note"))
    print("-" * 72)
    for s in scenarios:
        print("%-22s %-7s %10.4f %10.4f %10.4f   %s"
              % (s.name[:22], "VALID" if s.valid else "INVALID",
                 s.compression, s.hausdorff, s.final_ssim, s.note))
    print("-" * 72)
    print("Scenarios passed : %d / %d" % (passed, total))
    print("Mean compression : %.4f %%" % mean_c)
    print("Min  compression : %.4f %%" % min_c)
    print("Overall result   : %s"
          % ("VALID" if total and passed == total else "INVALID"))
    print("=" * 72)


def print_summary(scenarios, total, passed, mean_c, min_c):
    """Machine-readable block consumed by ``evaluate.sh`` (keep keys stable)."""
    overall_valid = total > 0 and passed == total
    print("RESULT=%s" % ("VALID" if overall_valid else "INVALID"))
    print("SCENARIOS_TOTAL=%d" % total)
    print("SCENARIOS_PASSED=%d" % passed)
    print("MEAN_COMPRESSION_RATE=%.6f" % mean_c)
    print("MIN_COMPRESSION_RATE=%.6f" % min_c)
    # Alias kept so existing tooling that greps COMPRESSION_RATE keeps working.
    print("COMPRESSION_RATE=%.6f" % mean_c)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Evaluate a solver across a whole mesh dataset."
    )
    parser.add_argument("--solver", default="solution.py",
                        help="solver script to run (default: solution.py)")
    parser.add_argument("--dataset", default="data/ppsurf",
                        help="dataset directory or single mesh file "
                             "(default: data/ppsurf)")
    parser.add_argument("--resolution", type=int, default=DEFAULT_RESOLUTION,
                        help="render resolution (default: %d)"
                             % DEFAULT_RESOLUTION)
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT,
                        help="per-scenario solver timeout in seconds "
                             "(default: %.0f)" % DEFAULT_TIMEOUT)
    parser.add_argument("--python", default=sys.executable,
                        help="python interpreter used to run the solver")
    parser.add_argument("--quiet", action="store_true",
                        help="suppress the per-scenario report")
    parser.add_argument("--summary", action="store_true",
                        help="print a machine-readable KEY=VALUE summary block")
    args = parser.parse_args(argv)

    inputs = find_inputs(args.dataset)
    if not inputs:
        print("error: no input meshes found at %s" % args.dataset,
              file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="evaldataset-") as tmp_dir:
        scenarios = evaluate_dataset(args.solver, inputs, args.resolution,
                                     args.python, args.timeout, tmp_dir)

    total, passed, mean_c, min_c = aggregate(scenarios)

    if args.summary:
        if not args.quiet:
            print_report(scenarios, total, passed, mean_c, min_c,
                         args.resolution)
        print_summary(scenarios, total, passed, mean_c, min_c)
    elif args.quiet:
        print("VALID" if total and passed == total else "INVALID")
    else:
        print_report(scenarios, total, passed, mean_c, min_c, args.resolution)

    return 0 if (total and passed == total) else 1


if __name__ == "__main__":
    sys.exit(main())
