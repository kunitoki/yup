import yup

#==================================================================================================

def test_grid_has_a_texture_coordinate_per_vertex():
    mesh = yup.ImageMesh.createGrid(yup.Rectangle[float](0.0, 0.0, 10.0, 10.0), 1, 1)

    assert mesh.isValid()
    assert len(mesh.getVertices()) == 4
    assert len(mesh.getTextureCoordinates()) == 4

#==================================================================================================

def test_set_vertices_needs_one_position_per_vertex():
    mesh = yup.ImageMesh.createGrid(yup.Rectangle[float](0.0, 0.0, 10.0, 10.0), 1, 1)

    moved = [yup.Point[float](0.0, 0.0), yup.Point[float](20.0, 0.0),
             yup.Point[float](0.0, 20.0), yup.Point[float](20.0, 20.0)]
    assert mesh.setVertices(moved)
    assert mesh.getVertices()[3].getX() == 20.0

    assert not mesh.setVertices([yup.Point[float](0.0, 0.0)])
