"""Shared pytest fixtures and path setup for the evaluator tests."""

import os
import sys

import numpy as np
import pytest

# Make the repository root importable so ``import evaluate`` works regardless of
# the directory pytest is invoked from.
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if REPO_ROOT not in sys.path:
    sys.path.insert(0, REPO_ROOT)

DATA_DIR = os.path.join(REPO_ROOT, "data")


@pytest.fixture(scope="session")
def data_dir():
    return DATA_DIR


@pytest.fixture(scope="session")
def sample_input_path(data_dir):
    return os.path.join(data_dir, "sample-input.txt")


@pytest.fixture(scope="session")
def sample_output_path(data_dir):
    return os.path.join(data_dir, "sample-output.txt")


# --- Synthetic mesh helpers -------------------------------------------------

def unit_cube():
    """Return a Mesh for an axis-aligned cube of side 1.

    This matches the geometry of ``data/sample-output.txt`` (8 vertices,
    12 triangular faces), a closed watertight 2-manifold.
    """
    import evaluate

    verts = np.array(
        [
            [0.5, 0.5, 0.5],
            [0.5, 0.5, -0.5],
            [0.5, -0.5, 0.5],
            [0.5, -0.5, -0.5],
            [-0.5, 0.5, 0.5],
            [-0.5, 0.5, -0.5],
            [-0.5, -0.5, 0.5],
            [-0.5, -0.5, -0.5],
        ],
        dtype=np.float64,
    )
    faces = np.array(
        [
            [0, 2, 3], [0, 3, 1],
            [4, 5, 7], [4, 7, 6],
            [0, 1, 5], [0, 5, 4],
            [2, 6, 7], [2, 7, 3],
            [0, 4, 6], [0, 6, 2],
            [1, 3, 7], [1, 7, 5],
        ],
        dtype=np.int64,
    )
    return evaluate.Mesh(verts, faces)


@pytest.fixture
def cube_mesh():
    return unit_cube()
