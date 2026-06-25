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

    python3 evaluate_dataset.py [--solver solutions/baseline/baseline.py] [--dataset data/ppsurf]
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
import time

import evaluate

# Default render resolution for the multi-mesh harness. The native grader uses
# 1024 (see docs/report.md 2.2), and the camera focal length is calibrated for
# that resolution: lower values do not just lower fidelity, they narrow the
# field of view and crop the object, which *changes* the SSIM and can flip the
# validity verdict. To make the reported score reflect real (final) grading
# performance, the harness defaults to the native resolution. Pass a smaller
# --resolution only for quick, non-representative previews.
DEFAULT_RESOLUTION = evaluate.RESOLUTION  # 1024 (native grader resolution)
DEFAULT_TIMEOUT = 300.0

# Grader-matching wall-clock budget per mesh (docs/report.md 2.8: 21 s). The
# harness measures each solver's wall-clock time and, when a budget is enforced,
# treats a mesh that exceeds it like the real grader does: the submission for
# that mesh is rejected (invalid, 0 compression). This is what catches a solver
# that is correct but too slow at scale -- the failure mode that a small dataset
# alone hides. Set the budget to 0 to disable enforcement (measure-and-report
# only).
#
# Caveat: the real grader runs the solver under pypy3, which is typically several
# times faster than the CPython used here, so a CPython time slightly over budget
# may still pass on the grader. The representative `data/ppsurf` meshes all solve
# in well under a second, so the default budget never bites there; it is meant to
# flag the large `data/stress` meshes, where the gap to the limit is large.
DEFAULT_TIME_BUDGET = 21.0


def find_inputs(dataset):
    """Return the sorted list of input mesh files for a dataset path."""
    if os.path.isfile(dataset):
        return [dataset]
    if os.path.isdir(dataset):
        return sorted(glob.glob(os.path.join(dataset, "*.txt")))
    return []


def run_solver(python_bin, solver, input_path, output_path, timeout):
    """Run ``solver`` on ``input_path`` -> ``output_path``.

    Returns ``(ok, message, seconds)``; ``ok`` is False if the solver failed or
    timed out, with ``message`` describing the cause. ``seconds`` is the measured
    wall-clock time of the solver subprocess (``timeout`` on a hard timeout).
    """
    start = time.perf_counter()
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
            return False, "solver timed out after %.0fs" % timeout, float(timeout)
    seconds = time.perf_counter() - start
    if proc.returncode != 0:
        err = proc.stderr.decode("utf-8", "replace").strip().splitlines()
        tail = err[-1] if err else "non-zero exit"
        return False, "solver error: %s" % tail, seconds
    return True, "ok", seconds


def _score_full(scenario, original, simplified, resolution):
    """Full grader-equivalent scoring (manifold + Hausdorff + rasterized SSIM)."""
    result = evaluate.evaluate(original, simplified, resolution=resolution)
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


def _score_no_ssim(scenario, original, simplified):
    """Runtime/geometry scoring: manifold gate + Hausdorff bound, no SSIM render.

    Intended for grader-scale stress meshes that the pure-Python rasterizer
    cannot render in reasonable time. The SSIM perceptual gate is skipped, so a
    "valid" verdict here means "valid manifold within the Hausdorff bound" only.
    """
    validity = evaluate.check_validity(simplified, original)
    hausdorff = evaluate.symmetric_hausdorff(original.vertices,
                                             simplified.vertices)
    bound = evaluate.HAUSDORFF_FRACTION * evaluate.aabb_diagonal(
        original.vertices)
    scenario.hausdorff = hausdorff
    scenario.final_ssim = float("nan")
    scenario.compression = (1.0 - len(simplified.vertices)
                            / len(original.vertices))
    scenario.valid = bool(validity.ok and hausdorff <= bound)
    if not scenario.valid:
        reasons = list(validity.reasons)
        if hausdorff > bound:
            reasons.append("Hausdorff %.4f > %.4f" % (hausdorff, bound))
        scenario.note = "; ".join(reasons) if reasons else "invalid"
    else:
        scenario.note = "ssim skipped (--no-ssim)"


class Scenario:
    """Per-mesh evaluation outcome."""

    def __init__(self, name):
        self.name = name
        self.valid = False
        self.compression = 0.0
        self.hausdorff = float("nan")
        self.final_ssim = float("nan")
        self.seconds = float("nan")
        self.over_budget = False
        self.note = ""


def evaluate_dataset(solver, inputs, resolution, python_bin, timeout, tmp_dir,
                     time_budget=0.0, no_ssim=False):
    """Run and score the solver over every input mesh; return the scenarios.

    When ``time_budget`` is > 0, a mesh whose solver wall-clock time exceeds it
    is treated like a grader rejection: the scenario is marked invalid with zero
    compression, mirroring a real timeout (no valid output produced in time).

    When ``no_ssim`` is True, the (expensive, pure-Python) rasterized SSIM step
    is skipped and validity is judged on the manifold gate plus the vertex-based
    Hausdorff bound only. This is what makes grader-scale stress meshes (hundreds
    of thousands of faces, which the rasterizer cannot render quickly) runnable
    end-to-end for runtime/geometry checks. The SSIM gate is NOT enforced in this
    mode, so use it for stress/runtime testing, not for final scoring.
    """
    scenarios = []
    for input_path in inputs:
        name = os.path.splitext(os.path.basename(input_path))[0]
        scenario = Scenario(name)
        output_path = os.path.join(tmp_dir, name + ".out.txt")

        ok, message, seconds = run_solver(python_bin, solver, input_path,
                                          output_path, timeout)
        scenario.seconds = seconds
        if not ok:
            scenario.note = message
            scenarios.append(scenario)
            continue

        try:
            original = evaluate.load_mesh(input_path)
            simplified = evaluate.load_mesh(output_path)
            if no_ssim:
                _score_no_ssim(scenario, original, simplified)
            else:
                _score_full(scenario, original, simplified, resolution)
        except Exception as exc:  # noqa: BLE001 - a bad mesh fails the scenario
            scenario.note = "evaluator error: %s" % exc
            scenarios.append(scenario)
            continue

        # Grader-style time budget: a mesh that is correct but too slow would be
        # rejected by the real grader, so fold that into the verdict here.
        if time_budget > 0.0 and seconds > time_budget:
            scenario.over_budget = True
            scenario.valid = False
            scenario.compression = 0.0
            budget_note = ("over time budget: %.2fs > %.2fs"
                           % (seconds, time_budget))
            scenario.note = (budget_note if not scenario.note
                             else budget_note + "; " + scenario.note)

        scenarios.append(scenario)
    return scenarios


def aggregate(scenarios):
    """Compute aggregate statistics over the scenarios."""
    total = len(scenarios)
    passed = sum(1 for s in scenarios if s.valid)
    over_budget = sum(1 for s in scenarios if s.over_budget)
    if total:
        mean_compression = sum(s.compression for s in scenarios) / total
        min_compression = min(s.compression for s in scenarios)
        solve_times = [s.seconds for s in scenarios
                       if s.seconds == s.seconds]  # drop NaN
        slowest = max(solve_times) if solve_times else float("nan")
    else:
        mean_compression = 0.0
        min_compression = 0.0
        slowest = float("nan")
    return total, passed, mean_compression, min_compression, slowest, over_budget


def print_report(scenarios, total, passed, mean_c, min_c, slowest, over_budget,
                 resolution, time_budget):
    print("=" * 84)
    print("IMC 2026 dataset evaluation (%d scenarios, resolution %d)"
          % (total, resolution))
    print("=" * 84)
    print("%-22s %-7s %9s %9s %8s %8s   %s"
          % ("scenario", "result", "compr%", "hausdorff", "ssim", "secs",
             "note"))
    print("-" * 84)
    for s in scenarios:
        print("%-22s %-7s %9.4f %9.4f %8.4f %8.2f   %s"
              % (s.name[:22], "VALID" if s.valid else "INVALID",
                 s.compression, s.hausdorff, s.final_ssim, s.seconds, s.note))
    print("-" * 84)
    print("Scenarios passed : %d / %d" % (passed, total))
    print("Mean compression : %.4f %%" % mean_c)
    print("Min  compression : %.4f %%" % min_c)
    print("Slowest solve    : %.2f s" % slowest)
    if time_budget > 0.0:
        print("Time budget      : %.2f s/mesh (%d over budget)"
              % (time_budget, over_budget))
    print("Overall result   : %s"
          % ("VALID" if total and passed == total else "INVALID"))
    print("=" * 84)


def print_summary(scenarios, total, passed, mean_c, min_c, slowest,
                  over_budget, time_budget):
    """Machine-readable block consumed by ``evaluate.sh`` (keep keys stable)."""
    overall_valid = total > 0 and passed == total
    print("RESULT=%s" % ("VALID" if overall_valid else "INVALID"))
    print("SCENARIOS_TOTAL=%d" % total)
    print("SCENARIOS_PASSED=%d" % passed)
    print("MEAN_COMPRESSION_RATE=%.6f" % mean_c)
    print("MIN_COMPRESSION_RATE=%.6f" % min_c)
    print("SLOWEST_SOLVE_SECONDS=%.4f" % slowest)
    print("TIME_BUDGET=%.4f" % time_budget)
    print("SCENARIOS_OVER_BUDGET=%d" % over_budget)
    # Alias kept so existing tooling that greps COMPRESSION_RATE keeps working.
    print("COMPRESSION_RATE=%.6f" % mean_c)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Evaluate a solver across a whole mesh dataset."
    )
    parser.add_argument("--solver", default="solutions/baseline/baseline.py",
                        help="solver script to run (default: solutions/baseline/baseline.py)")
    parser.add_argument("--dataset", default="data/ppsurf",
                        help="dataset directory or single mesh file "
                             "(default: data/ppsurf)")
    parser.add_argument("--resolution", type=int, default=DEFAULT_RESOLUTION,
                        help="render resolution (default: %d)"
                             % DEFAULT_RESOLUTION)
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT,
                        help="per-scenario solver hard timeout in seconds "
                             "(default: %.0f)" % DEFAULT_TIMEOUT)
    parser.add_argument("--time-budget", type=float, default=DEFAULT_TIME_BUDGET,
                        help="grader-style wall-clock budget per mesh in seconds;"
                             " meshes that exceed it are rejected (invalid, 0%%"
                             " compression). 0 disables enforcement "
                             "(default: %.0f)" % DEFAULT_TIME_BUDGET)
    parser.add_argument("--python", default=sys.executable,
                        help="python interpreter used to run the solver")
    parser.add_argument("--quiet", action="store_true",
                        help="suppress the per-scenario report")
    parser.add_argument("--summary", action="store_true",
                        help="print a machine-readable KEY=VALUE summary block")
    parser.add_argument("--no-ssim", action="store_true",
                        help="skip the rasterized SSIM gate; judge validity on "
                             "the manifold check + Hausdorff bound only. Use for "
                             "grader-scale stress meshes the rasterizer cannot "
                             "render quickly (runtime/geometry checks only).")
    args = parser.parse_args(argv)

    inputs = find_inputs(args.dataset)
    if not inputs:
        print("error: no input meshes found at %s" % args.dataset,
              file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="evaldataset-") as tmp_dir:
        scenarios = evaluate_dataset(args.solver, inputs, args.resolution,
                                     args.python, args.timeout, tmp_dir,
                                     time_budget=args.time_budget,
                                     no_ssim=args.no_ssim)

    total, passed, mean_c, min_c, slowest, over_budget = aggregate(scenarios)

    if args.summary:
        if not args.quiet:
            print_report(scenarios, total, passed, mean_c, min_c, slowest,
                         over_budget, args.resolution, args.time_budget)
        print_summary(scenarios, total, passed, mean_c, min_c, slowest,
                      over_budget, args.time_budget)
    elif args.quiet:
        print("VALID" if total and passed == total else "INVALID")
    else:
        print_report(scenarios, total, passed, mean_c, min_c, slowest,
                     over_budget, args.resolution, args.time_budget)

    return 0 if (total and passed == total) else 1


if __name__ == "__main__":
    sys.exit(main())
