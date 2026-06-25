"""Tests for the multi-sample dataset harness and dataset preparation helpers."""

import importlib.util
import os
import stat

import pytest

import evaluate_dataset

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _load_prepare_module():
    """Import datasets/prepare_ppsurf.py by path (it is not on sys.path)."""
    path = os.path.join(REPO_ROOT, "datasets", "prepare_ppsurf.py")
    spec = importlib.util.spec_from_file_location("prepare_ppsurf", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


prepare_ppsurf = _load_prepare_module()


# --- solver stubs -----------------------------------------------------------

def _write_solver(tmp_path, name, body):
    path = tmp_path / name
    path.write_text("import sys\n" + body)
    path.chmod(path.stat().st_mode | stat.S_IEXEC)
    return str(path)


IDENTITY_SOLVER = (
    "data = sys.stdin.buffer.read()\n"
    "sys.stdout.buffer.write(data)\n"
)

# Drops the last face -> opens the mesh -> invalid (not a closed 2-manifold).
BREAK_SOLVER = (
    "tok = sys.stdin.buffer.read().split()\n"
    "nv = int(tok[0]); nf = int(tok[1])\n"
    "p = 2\n"
    "verts = []\n"
    "for _ in range(nv):\n"
    "    verts.append((tok[p+1], tok[p+2], tok[p+3])); p += 4\n"
    "faces = []\n"
    "for _ in range(nf):\n"
    "    faces.append((tok[p+1], tok[p+2], tok[p+3])); p += 4\n"
    "faces = faces[:-1]\n"
    "out = ['%d %d' % (nv, len(faces))]\n"
    "out += ['v %s %s %s' % v for v in verts]\n"
    "out += ['f %s %s %s' % f for f in faces]\n"
    "sys.stdout.write('\\n'.join(out) + '\\n')\n"
)


def _dataset_with_sample(tmp_path, sample_input_path, copies=1):
    ddir = tmp_path / "ds"
    ddir.mkdir()
    text = open(sample_input_path).read()
    for i in range(copies):
        (ddir / ("mesh_%d.txt" % i)).write_text(text)
    return str(ddir)


# --- evaluate_dataset -------------------------------------------------------

def test_find_inputs_directory_and_file(tmp_path, sample_input_path):
    ddir = _dataset_with_sample(tmp_path, sample_input_path, copies=3)
    found = evaluate_dataset.find_inputs(ddir)
    assert len(found) == 3
    assert all(p.endswith(".txt") for p in found)
    # A single file path is also accepted.
    assert evaluate_dataset.find_inputs(sample_input_path) == [sample_input_path]


def test_dataset_all_valid_exits_zero(tmp_path, sample_input_path, capsys):
    solver = _write_solver(tmp_path, "id.py", IDENTITY_SOLVER)
    ddir = _dataset_with_sample(tmp_path, sample_input_path, copies=2)
    code = evaluate_dataset.main(
        ["--solver", solver, "--dataset", ddir, "--resolution", "64",
         "--summary"]
    )
    out = capsys.readouterr().out
    summary = _parse_summary(out)
    assert code == 0
    assert summary["RESULT"] == "VALID"
    assert summary["SCENARIOS_TOTAL"] == "2"
    assert summary["SCENARIOS_PASSED"] == "2"
    # Identity solver removes nothing -> 0% compression.
    assert float(summary["COMPRESSION_RATE"]) == pytest.approx(0.0, abs=1e-6)


def test_dataset_one_failure_makes_overall_invalid(
    tmp_path, sample_input_path, capsys
):
    solver = _write_solver(tmp_path, "break.py", BREAK_SOLVER)
    ddir = _dataset_with_sample(tmp_path, sample_input_path, copies=2)
    code = evaluate_dataset.main(
        ["--solver", solver, "--dataset", ddir, "--resolution", "64",
         "--summary"]
    )
    out = capsys.readouterr().out
    summary = _parse_summary(out)
    # Every scenario is broken -> 0/2 passed -> invalid, non-zero exit.
    assert code == 1
    assert summary["RESULT"] == "INVALID"
    assert summary["SCENARIOS_PASSED"] == "0"


def test_dataset_solver_error_is_reported(tmp_path, sample_input_path, capsys):
    solver = _write_solver(tmp_path, "boom.py",
                           "raise SystemExit('boom')\n")
    ddir = _dataset_with_sample(tmp_path, sample_input_path, copies=1)
    code = evaluate_dataset.main(
        ["--solver", solver, "--dataset", ddir, "--resolution", "64",
         "--summary"]
    )
    out = capsys.readouterr().out
    summary = _parse_summary(out)
    assert code == 1
    assert summary["RESULT"] == "INVALID"
    assert summary["SCENARIOS_PASSED"] == "0"


def test_dataset_missing_path_returns_error_code(tmp_path):
    code = evaluate_dataset.main(["--dataset", str(tmp_path / "nope")])
    assert code == 2


def _parse_summary(out):
    summary = {}
    for line in out.splitlines():
        if "=" in line and line.split("=", 1)[0].isupper():
            key, value = line.split("=", 1)
            summary[key] = value
    return summary


# --- prepare_ppsurf helpers -------------------------------------------------

def test_representative_indices_spans_range():
    sizes = [10, 50, 20, 80, 30, 60, 40, 70, 90, 100]
    idx = prepare_ppsurf.representative_indices(sizes, 4)
    chosen = sorted(sizes[i] for i in idx)
    assert len(idx) == 4
    assert chosen[0] == min(sizes)
    assert chosen[-1] == max(sizes)


def test_representative_indices_handles_small_pool():
    sizes = [5, 1, 3]
    idx = prepare_ppsurf.representative_indices(sizes, 10)
    assert sorted(idx) == [0, 1, 2]


def test_is_valid_manifold_accepts_cube(cube_mesh):
    ok, _ = prepare_ppsurf.is_valid_manifold(cube_mesh.vertices, cube_mesh.faces)
    assert ok


def test_is_valid_manifold_rejects_open_mesh(cube_mesh):
    ok, reason = prepare_ppsurf.is_valid_manifold(
        cube_mesh.vertices, cube_mesh.faces[:-1]
    )
    assert not ok
    assert "closed" in reason


def test_normalize_fits_unit_sphere(cube_mesh):
    import numpy as np

    scaled = cube_mesh.vertices * 7.0 + np.array([3.0, -2.0, 5.0])
    out = prepare_ppsurf.normalize(scaled)
    radii = np.linalg.norm(out, axis=1)
    assert radii.max() == pytest.approx(1.0, abs=1e-9)
    assert out.min() >= -1.0 - 1e-9 and out.max() <= 1.0 + 1e-9
