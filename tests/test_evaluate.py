"""End-to-end tests of the evaluate() pipeline and the CLI."""

import numpy as np
import pytest

import evaluate


def test_evaluate_sample_is_valid(sample_input_path, sample_output_path):
    original = evaluate.load_mesh(sample_input_path)
    simplified = evaluate.load_mesh(sample_output_path)
    # A small resolution keeps the test fast while still exercising rendering.
    result = evaluate.evaluate(original, simplified, resolution=128)

    assert result.valid
    assert result.validity.ok
    assert result.final_ssim >= evaluate.SSIM_THRESHOLD
    assert result.hausdorff <= result.hausdorff_bound
    # 9 -> 8 vertices is a 1/9 reduction.
    assert result.compression_rate == pytest.approx(100.0 / 9.0, abs=1e-6)
    assert len(result.per_view) == 6


def test_evaluate_identical_meshes_scores_perfect(sample_input_path):
    mesh = evaluate.load_mesh(sample_input_path)
    result = evaluate.evaluate(mesh, mesh, resolution=128)
    assert result.valid
    assert result.final_ssim == pytest.approx(1.0, abs=1e-9)
    assert result.hausdorff == pytest.approx(0.0, abs=1e-12)
    assert result.compression_rate == pytest.approx(0.0, abs=1e-9)


def test_evaluate_rejects_invalid_topology(sample_input_path, cube_mesh):
    original = evaluate.load_mesh(sample_input_path)
    # Open mesh: not a closed 2-manifold -> overall invalid even if SSIM is ok.
    broken = evaluate.Mesh(cube_mesh.vertices, cube_mesh.faces[:-1])
    result = evaluate.evaluate(original, broken, resolution=64)
    assert not result.valid
    assert not result.validity.ok


def test_evaluate_per_view_scores_bounded(sample_input_path, sample_output_path):
    original = evaluate.load_mesh(sample_input_path)
    simplified = evaluate.load_mesh(sample_output_path)
    result = evaluate.evaluate(original, simplified, resolution=128)
    for view in result.per_view:
        assert -1.0 <= view["ssim_normal"] <= 1.0 + 1e-9
        assert -1.0 <= view["ssim_depth"] <= 1.0 + 1e-9
        assert "eye" in view


# --- CLI --------------------------------------------------------------------

def test_cli_valid_returns_zero(sample_input_path, sample_output_path, capsys):
    code = evaluate.main(
        [sample_input_path, sample_output_path, "--resolution", "128"]
    )
    assert code == 0
    out = capsys.readouterr().out
    assert "VALID SUBMISSION : YES" in out


def test_cli_quiet_mode(sample_input_path, sample_output_path, capsys):
    code = evaluate.main(
        [sample_input_path, sample_output_path, "--resolution", "128", "--quiet"]
    )
    assert code == 0
    assert capsys.readouterr().out.strip() == "VALID"
