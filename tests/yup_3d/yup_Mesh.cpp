/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <yup_3d/yup_3d.h>

using namespace yup;

class MeshTests : public ::testing::Test
{
protected:
    static Mesh::Vertex vertexAt (float x, float y, float z)
    {
        Mesh::Vertex vertex;
        vertex.position = { x, y, z };
        return vertex;
    }

    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }
};

TEST_F (MeshTests, VertexIsTightlyPacked)
{
    EXPECT_EQ (sizeof (Mesh::Vertex), 48u);
}

TEST_F (MeshTests, NewMeshIsEmpty)
{
    const Mesh mesh ("empty");

    EXPECT_EQ (mesh.getName(), "empty");
    EXPECT_EQ (mesh.getNumPrimitives(), 0);
    EXPECT_TRUE (mesh.getBounds().isEmpty());
}

TEST_F (MeshTests, AddPrimitiveKeepsIndexedData)
{
    Mesh mesh;
    auto material = Material::Ptr (new Material());

    std::vector<Mesh::Vertex> vertices { vertexAt (0.0f, 0.0f, 0.0f), vertexAt (1.0f, 0.0f, 0.0f), vertexAt (1.0f, 1.0f, 0.0f), vertexAt (0.0f, 1.0f, 0.0f) };
    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    mesh.addPrimitive (vertices, { 0, 1, 2, 0, 2, 3 }, material);

    ASSERT_EQ (mesh.getNumPrimitives(), 1);
    const auto& primitive = mesh.getPrimitive (0);
    EXPECT_EQ (primitive.vertices.size(), 4u);
    EXPECT_EQ (primitive.indices, (std::vector<uint32> { 0, 1, 2, 0, 2, 3 }));
    EXPECT_EQ (primitive.material, material);
    expectNear (primitive.vertices[2].normal, { 0.0f, 0.0f, 1.0f });
}

TEST_F (MeshTests, AddPrimitiveWithoutIndicesDrawsInOrder)
{
    Mesh mesh;
    std::vector<Mesh::Vertex> vertices { vertexAt (0.0f, 0.0f, 0.0f), vertexAt (1.0f, 0.0f, 0.0f), vertexAt (0.0f, 1.0f, 0.0f) };
    vertices[0].normal = vertices[1].normal = vertices[2].normal = { 0.0f, 0.0f, 1.0f };

    mesh.addPrimitive (vertices, {});

    EXPECT_EQ (mesh.getPrimitive (0).indices, (std::vector<uint32> { 0, 1, 2 }));
}

TEST_F (MeshTests, MissingNormalsAreGeneratedFlat)
{
    Mesh mesh;

    // Two triangles sharing an edge, folded at a right angle: shared vertices need two normals
    mesh.addPrimitive ({ vertexAt (0.0f, 0.0f, 0.0f), vertexAt (1.0f, 0.0f, 0.0f), vertexAt (0.0f, 1.0f, 0.0f), vertexAt (0.0f, 0.0f, 1.0f) },
                       { 0, 1, 2, 0, 3, 1 });

    const auto& primitive = mesh.getPrimitive (0);
    ASSERT_EQ (primitive.vertices.size(), 6u);
    ASSERT_EQ (primitive.indices.size(), 6u);

    for (int i = 0; i < 3; ++i)
        expectNear (primitive.vertices[primitive.indices[(size_t) i]].normal, { 0.0f, 0.0f, 1.0f });

    for (int i = 3; i < 6; ++i)
        expectNear (primitive.vertices[primitive.indices[(size_t) i]].normal, { 0.0f, 1.0f, 0.0f });

    expectNear (primitive.vertices[primitive.indices[4]].position, { 0.0f, 0.0f, 1.0f });
}

TEST_F (MeshTests, BoundsCoverEveryPrimitive)
{
    Mesh mesh;
    mesh.addPrimitive ({ vertexAt (0.0f, 0.0f, 0.0f), vertexAt (1.0f, 0.0f, 0.0f), vertexAt (0.0f, 1.0f, 0.0f) }, {});
    mesh.addPrimitive ({ vertexAt (-2.0f, 0.0f, 3.0f), vertexAt (-1.0f, 0.0f, 3.0f), vertexAt (-2.0f, 5.0f, 3.0f) }, {});

    expectNear (mesh.getPrimitive (0).bounds.getMin(), { 0.0f, 0.0f, 0.0f });
    expectNear (mesh.getPrimitive (0).bounds.getMax(), { 1.0f, 1.0f, 0.0f });
    expectNear (mesh.getBounds().getMin(), { -2.0f, 0.0f, 0.0f });
    expectNear (mesh.getBounds().getMax(), { 1.0f, 5.0f, 3.0f });
}
