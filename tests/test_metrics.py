"""Tests for rendering, SSIM, and Hausdorff helpers."""

import numpy as np
import pytest

import evaluate


# --- Rendering --------------------------------------------------------------

def test_render_produces_foreground_and_background(cube_mesh):
    eye = evaluate.EYES[0]
    # The focal length is calibrated for the native 1024 resolution; at lower
    # resolutions the projected object overflows the (cropped) frame, so use the
    # native size here to guarantee visible background.
    res = evaluate.RESOLUTION
    normal_map, depth_map, coverage = evaluate.render_view(cube_mesh, eye, res)

    assert normal_map.shape == (res, res, 3)
    assert depth_map.shape == (res, res)
    assert coverage.shape == (res, res)

    # The cube must cover some pixels and leave some background.
    assert coverage.any()
    assert not coverage.all()

    # Background pixels keep their default values.
    bg = ~coverage
    assert np.allclose(depth_map[bg], evaluate.BACKGROUND_DEPTH)
    assert np.allclose(normal_map[bg], evaluate.BACKGROUND_NORMAL)

    # Foreground depth is finite and in front of the camera.
    fg = coverage
    assert np.all(depth_map[fg] < evaluate.BACKGROUND_DEPTH)
    assert np.all(depth_map[fg] > 0.0)


def test_render_front_face_depth_matches_geometry(cube_mesh):
    # Camera at +x looking at the origin sees the x=0.5 face at depth ~2.0.
    eye = np.array([evaluate.CAMERA_DISTANCE, 0.0, 0.0])
    _, depth_map, coverage = evaluate.render_view(cube_mesh, eye, 256)
    front_depth = depth_map[coverage].min()
    assert front_depth == pytest.approx(evaluate.CAMERA_DISTANCE - 0.5, abs=1e-3)


# --- SSIM -------------------------------------------------------------------

def test_ssim_identical_images_is_one():
    rng = np.random.default_rng(0)
    img = rng.uniform(0, 255, size=(64, 64))
    foreground = np.ones((64, 64), dtype=bool)
    score = evaluate.foreground_ssim(img, img.copy(), foreground)
    assert score == pytest.approx(1.0, abs=1e-9)


def test_ssim_identical_rgb_is_one():
    rng = np.random.default_rng(1)
    img = rng.uniform(0, 255, size=(48, 48, 3))
    foreground = np.ones((48, 48), dtype=bool)
    score = evaluate.foreground_ssim(img, img.copy(), foreground)
    assert score == pytest.approx(1.0, abs=1e-9)


def test_ssim_different_images_below_one():
    rng = np.random.default_rng(2)
    a = rng.uniform(0, 255, size=(64, 64))
    b = rng.uniform(0, 255, size=(64, 64))
    foreground = np.ones((64, 64), dtype=bool)
    score = evaluate.foreground_ssim(a, b, foreground)
    assert score < 0.5


def test_ssim_no_foreground_returns_one():
    img_a = np.zeros((32, 32))
    img_b = np.ones((32, 32)) * 200
    foreground = np.zeros((32, 32), dtype=bool)
    # With no foreground windows, SSIM is vacuously perfect.
    assert evaluate.foreground_ssim(img_a, img_b, foreground) == 1.0


def test_box_sum_matches_bruteforce():
    rng = np.random.default_rng(3)
    img = rng.uniform(0, 1, size=(10, 12))
    win = 4
    result = evaluate._box_sum(img, win)
    # Brute-force reference.
    h, w = img.shape
    expected = np.empty((h - win + 1, w - win + 1))
    for i in range(h - win + 1):
        for j in range(w - win + 1):
            expected[i, j] = img[i:i + win, j:j + win].sum()
    np.testing.assert_allclose(result, expected, atol=1e-9)


# --- Hausdorff --------------------------------------------------------------

def test_hausdorff_identical_is_zero(cube_mesh):
    d = evaluate.symmetric_hausdorff(cube_mesh.vertices, cube_mesh.vertices)
    assert d == pytest.approx(0.0, abs=1e-12)


def test_hausdorff_known_translation(cube_mesh):
    shifted = cube_mesh.vertices + np.array([0.1, 0.0, 0.0])
    d = evaluate.symmetric_hausdorff(cube_mesh.vertices, shifted)
    assert d == pytest.approx(0.1, abs=1e-9)


def test_hausdorff_symmetry(cube_mesh):
    other = cube_mesh.vertices + np.array([0.05, -0.02, 0.03])
    d_ab = evaluate.symmetric_hausdorff(cube_mesh.vertices, other)
    d_ba = evaluate.symmetric_hausdorff(other, cube_mesh.vertices)
    assert d_ab == pytest.approx(d_ba, abs=1e-12)


def test_nearest_distances_fallback_matches_kdtree(cube_mesh):
    query = cube_mesh.vertices
    reference = cube_mesh.vertices + 0.01
    # Exercise the brute-force fallback directly.
    out = np.empty(len(query))
    chunk = len(query)
    block = query[:chunk]
    dmat = np.linalg.norm(block[:, None, :] - reference[None, :, :], axis=2)
    out[:chunk] = dmat.min(axis=1)
    via_helper = evaluate._nearest_distances(query, reference)
    np.testing.assert_allclose(np.sort(via_helper), np.sort(out), atol=1e-9)


def test_aabb_diagonal_unit_cube(cube_mesh):
    diag = evaluate.aabb_diagonal(cube_mesh.vertices)
    assert diag == pytest.approx(np.sqrt(3.0), abs=1e-9)
