"""Tests for mesh I/O and the validity gate."""

import numpy as np
import pytest

import evaluate


def test_load_sample_input(sample_input_path):
    mesh = evaluate.load_mesh(sample_input_path)
    assert mesh.vertices.shape == (9, 3)
    assert mesh.faces.shape == (14, 3)
    # Faces are stored zero-indexed after loading.
    assert mesh.faces.min() == 0
    assert mesh.faces.max() == 8
    # First vertex round-trips.
    np.testing.assert_allclose(mesh.vertices[0], [0.5, 0.5, 0.5])


def test_load_sample_output(sample_output_path):
    mesh = evaluate.load_mesh(sample_output_path)
    assert mesh.vertices.shape == (8, 3)
    assert mesh.faces.shape == (12, 3)


def test_sample_output_is_valid(sample_input_path, sample_output_path):
    original = evaluate.load_mesh(sample_input_path)
    simplified = evaluate.load_mesh(sample_output_path)
    validity = evaluate.check_validity(simplified, original)
    assert validity.ok, validity.reasons


def test_cube_fixture_is_valid(cube_mesh):
    validity = evaluate.check_validity(cube_mesh, cube_mesh)
    assert validity.ok, validity.reasons


def test_sample_input_against_itself_is_valid(sample_input_path):
    mesh = evaluate.load_mesh(sample_input_path)
    validity = evaluate.check_validity(mesh, mesh)
    assert validity.ok, validity.reasons


def test_validity_rejects_too_many_vertices(cube_mesh):
    # Simplified mesh may not have more vertices than the original.
    smaller_original = evaluate.Mesh(cube_mesh.vertices[:4], cube_mesh.faces)
    validity = evaluate.check_validity(cube_mesh, smaller_original)
    assert not validity.ok
    assert any("out of range" in r for r in validity.reasons)


def test_validity_rejects_open_mesh(cube_mesh):
    # Drop one face to open a hole -> some edges shared by only one face.
    open_mesh = evaluate.Mesh(cube_mesh.vertices, cube_mesh.faces[:-1])
    validity = evaluate.check_validity(open_mesh, open_mesh)
    assert not validity.ok
    assert any("2-manifold" in r for r in validity.reasons)


def test_validity_rejects_out_of_range_index(cube_mesh):
    bad_faces = cube_mesh.faces.copy()
    bad_faces[0, 0] = 999
    bad = evaluate.Mesh(cube_mesh.vertices, bad_faces)
    validity = evaluate.check_validity(bad, bad)
    assert not validity.ok
    assert any("out-of-range" in r for r in validity.reasons)


def test_validity_rejects_repeated_index_face(cube_mesh):
    bad_faces = cube_mesh.faces.copy()
    bad_faces[0] = [0, 0, 1]
    bad = evaluate.Mesh(cube_mesh.vertices, bad_faces)
    validity = evaluate.check_validity(bad, bad)
    assert not validity.ok
    assert any("repeated vertex" in r for r in validity.reasons)


def test_validity_rejects_empty_faces(cube_mesh):
    empty = evaluate.Mesh(cube_mesh.vertices, np.empty((0, 3), dtype=np.int64))
    validity = evaluate.check_validity(empty, cube_mesh)
    assert not validity.ok
    assert any("no faces" in r for r in validity.reasons)


def test_validity_rejects_degenerate_zero_area(cube_mesh):
    # Collapse two vertices so a face has zero area while keeping edge counts
    # balanced is hard; instead place a duplicate vertex and a sliver face.
    verts = np.vstack([cube_mesh.vertices, cube_mesh.vertices[0]])
    faces = np.vstack([cube_mesh.faces, [[0, 8, 0]]])  # zero-area + repeated
    bad = evaluate.Mesh(verts, faces)
    validity = evaluate.check_validity(bad, evaluate.Mesh(verts, faces))
    assert not validity.ok
