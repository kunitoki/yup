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

    /** A triangle at (@p x, 0, 0), 1 wide and 1 high, with normals, so addPrimitive() keeps its three vertices as given. */
    static std::vector<Mesh::Vertex> makeTriangle (float x)
    {
        std::vector<Mesh::Vertex> vertices { vertexAt (x, 0.0f, 0.0f), vertexAt (x + 1.0f, 0.0f, 0.0f), vertexAt (x, 1.0f, 0.0f) };
        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        return vertices;
    }
};

TEST_F (MeshTests, VertexIsTightlyPacked)
{
    EXPECT_EQ (sizeof (Mesh::Vertex), 64u);
    EXPECT_EQ (offsetof (Mesh::Vertex, tangent), 48u);

    Mesh::Vertex vertex;
    EXPECT_EQ (vertex.tangent, (std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f }));
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

TEST_F (MeshTests, GeneratedFlatNormalsDropTangents)
{
    std::vector<Mesh::Vertex> vertices { vertexAt (0.0f, 0.0f, 0.0f), vertexAt (1.0f, 0.0f, 0.0f), vertexAt (0.0f, 1.0f, 0.0f) };
    for (auto& vertex : vertices)
        vertex.tangent = { 1.0f, 0.0f, 0.0f, 1.0f };

    Mesh mesh;
    mesh.addPrimitive (vertices, {});

    // No normals were given: the generated flat frame cannot trust tangents authored for another one
    for (const auto& vertex : mesh.getPrimitive (0).vertices)
        EXPECT_EQ (vertex.tangent, (std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f }));
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

TEST_F (MeshTests, SetVerticesReplacesTheVerticesAndKeepsIndicesAndMaterial)
{
    Mesh mesh;
    auto material = Material::Ptr (new Material());
    mesh.addPrimitive (makeTriangle (0.0f), { 2, 0, 1 }, material);

    // Taken as they are: a normal left at zero stays zero, and no vertex is added or moved
    auto moved = makeTriangle (5.0f);
    moved[1].normal = {};
    moved[2].color = { 0.25f, 0.5f, 0.75f, 1.0f };
    mesh.setVertices (0, moved);

    const auto& primitive = mesh.getPrimitive (0);
    ASSERT_EQ (primitive.vertices.size(), 3u);
    expectNear (primitive.vertices[0].position, { 5.0f, 0.0f, 0.0f });
    expectNear (primitive.vertices[1].position, { 6.0f, 0.0f, 0.0f });
    expectNear (primitive.vertices[2].position, { 5.0f, 1.0f, 0.0f });
    expectNear (primitive.vertices[0].normal, { 0.0f, 0.0f, 1.0f });
    expectNear (primitive.vertices[1].normal, { 0.0f, 0.0f, 0.0f });
    EXPECT_EQ (primitive.vertices[2].color, (std::array<float, 4> { 0.25f, 0.5f, 0.75f, 1.0f }));
    EXPECT_EQ (primitive.indices, (std::vector<uint32> { 2, 0, 1 }));
    EXPECT_EQ (primitive.material, material);
}

TEST_F (MeshTests, SetVerticesMovesTheBoundsOfThePrimitiveAndTheMesh)
{
    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), {});
    mesh.addPrimitive (makeTriangle (10.0f), {});
    expectNear (mesh.getBounds().getMax(), { 11.0f, 1.0f, 0.0f });

    // The second primitive moves next to the first, so the mesh shrinks with it
    mesh.setVertices (1, makeTriangle (2.0f));

    expectNear (mesh.getPrimitive (1).bounds.getMin(), { 2.0f, 0.0f, 0.0f });
    expectNear (mesh.getPrimitive (1).bounds.getMax(), { 3.0f, 1.0f, 0.0f });
    expectNear (mesh.getPrimitive (0).bounds.getMin(), { 0.0f, 0.0f, 0.0f });
    expectNear (mesh.getPrimitive (0).bounds.getMax(), { 1.0f, 1.0f, 0.0f });
    expectNear (mesh.getBounds().getMin(), { 0.0f, 0.0f, 0.0f });
    expectNear (mesh.getBounds().getMax(), { 3.0f, 1.0f, 0.0f });
}

TEST_F (MeshTests, ChangeCountCountsEverySetVertices)
{
    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), {});
    mesh.addPrimitive (makeTriangle (10.0f), {});

    // Adding primitives does not change the vertices of one
    EXPECT_EQ (mesh.getChangeCount(), 0u);

    mesh.setVertices (0, makeTriangle (1.0f));
    mesh.setVertices (1, makeTriangle (11.0f));

    // The same vertices again are a change too: the count is of the calls
    mesh.setVertices (1, makeTriangle (11.0f));
    EXPECT_EQ (mesh.getChangeCount(), 3u);
}

TEST_F (MeshTests, SetVerticesRejectsAMissingPrimitiveAndAnotherNumberOfVertices)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "A primitive that does not exist and another number of vertices trigger an assertion when assertions are enabled or logged";
#endif

    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), {});

    mesh.setVertices (1, makeTriangle (5.0f));
    mesh.setVertices (-1, makeTriangle (5.0f));

    auto oneMore = makeTriangle (5.0f);
    oneMore.push_back (vertexAt (9.0f, 9.0f, 9.0f));
    mesh.setVertices (0, oneMore);
    mesh.setVertices (0, std::vector<Mesh::Vertex>());

    const auto& primitive = mesh.getPrimitive (0);
    ASSERT_EQ (primitive.vertices.size(), 3u);
    expectNear (primitive.vertices[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (primitive.bounds.getMax(), { 1.0f, 1.0f, 0.0f });
    expectNear (mesh.getBounds().getMax(), { 1.0f, 1.0f, 0.0f });
    EXPECT_EQ (mesh.getChangeCount(), 0u);
}

TEST_F (MeshTests, EdgeIndicesListSharedEdgesOnceInOrder)
{
    const std::vector<uint32> triangles { 0, 1, 2, 0, 2, 3 };

    const auto edges = Mesh::computeEdgeIndices (triangles);

    EXPECT_EQ (edges, (std::vector<uint32> { 0, 1, 1, 2, 2, 0, 2, 3, 3, 0 }));
}

TEST_F (MeshTests, EdgeIndicesSkipDegenerateEdgesAndIncompleteTriangles)
{
    const std::vector<uint32> triangles { 0, 0, 1, 2, 1 };

    const auto edges = Mesh::computeEdgeIndices (triangles);

    EXPECT_EQ (edges, (std::vector<uint32> { 0, 1 }));
}

TEST_F (MeshTests, EdgeIndicesOfNoTrianglesAreEmpty)
{
    EXPECT_TRUE (Mesh::computeEdgeIndices (std::vector<uint32>()).empty());
}

TEST_F (MeshTests, PrimitivesAreNeitherSkinnedNorMorphedByDefault)
{
    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), {});

    EXPECT_TRUE (mesh.getPrimitive (0).influences.empty());
    EXPECT_TRUE (mesh.getPrimitive (0).morphTargets.empty());
    EXPECT_EQ (mesh.getNumJoints(), 0);
    EXPECT_EQ (mesh.getNumMorphTargets(), 0);
    EXPECT_TRUE (mesh.getMorphWeights().empty());
}

TEST_F (MeshTests, AddPrimitiveKeepsInfluencesAndMorphTargets)
{
    std::vector<Mesh::Influences> influences (3);
    influences[0] = { { 0, 0, 0, 0 }, { 1.0f, 0.0f, 0.0f, 0.0f } };
    influences[1] = { { 2, 1, 0, 0 }, { 0.75f, 0.25f, 0.0f, 0.0f } };
    influences[2] = { { 1, 9, 0, 0 }, { 1.0f, 0.0f, 0.0f, 0.0f } };

    std::vector<Mesh::MorphTarget> targets (2);
    targets[0].positions = { { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 2.0f }, { 0.0f, 0.0f, 3.0f } };
    targets[0].normals = { { 0.0f, 1.0f, 0.0f }, { 0.0f, 2.0f, 0.0f }, { 0.0f, 3.0f, 0.0f } };
    targets[1].tangents = { { 1.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.0f }, { 3.0f, 0.0f, 0.0f } };

    // With normals, the vertices are kept as they are given, and so is what comes with them
    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), { 2, 0, 1 }, nullptr, influences, targets);

    const auto& primitive = mesh.getPrimitive (0);
    ASSERT_EQ (primitive.vertices.size(), 3u);
    ASSERT_EQ (primitive.influences.size(), 3u);
    EXPECT_EQ (primitive.influences[1].joints, (std::array<uint16, 4> { 2, 1, 0, 0 }));
    EXPECT_EQ (primitive.influences[1].weights, (std::array<float, 4> { 0.75f, 0.25f, 0.0f, 0.0f }));
    EXPECT_EQ (primitive.influences[2].joints, (std::array<uint16, 4> { 1, 9, 0, 0 }));

    ASSERT_EQ (primitive.morphTargets.size(), 2u);
    ASSERT_EQ (primitive.morphTargets[0].positions.size(), 3u);
    ASSERT_EQ (primitive.morphTargets[0].normals.size(), 3u);
    ASSERT_EQ (primitive.morphTargets[1].tangents.size(), 3u);
    expectNear (primitive.morphTargets[0].positions[2], { 0.0f, 0.0f, 3.0f });
    expectNear (primitive.morphTargets[0].normals[1], { 0.0f, 2.0f, 0.0f });
    expectNear (primitive.morphTargets[1].tangents[0], { 1.0f, 0.0f, 0.0f });
    EXPECT_TRUE (primitive.morphTargets[0].tangents.empty());
    EXPECT_TRUE (primitive.morphTargets[1].positions.empty());

    // Joint 2 is the highest one a vertex follows: joint 9 has no weight, so no vertex does
    EXPECT_EQ (mesh.getNumJoints(), 3);
    EXPECT_EQ (mesh.getNumMorphTargets(), 2);
}

TEST_F (MeshTests, MeshCountsTheJointsAndMorphTargetsOfEveryPrimitive)
{
    std::vector<Mesh::Influences> influences (3);
    influences[0] = { { 5, 0, 0, 0 }, { 1.0f, 0.0f, 0.0f, 0.0f } };

    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), {}, nullptr, influences, std::vector<Mesh::MorphTarget> (3));
    mesh.addPrimitive (makeTriangle (2.0f), {}, nullptr, {}, std::vector<Mesh::MorphTarget> (1));
    mesh.addPrimitive (makeTriangle (4.0f), {});

    EXPECT_EQ (mesh.getNumJoints(), 6);
    EXPECT_EQ (mesh.getNumMorphTargets(), 3);
    EXPECT_TRUE (mesh.getPrimitive (2).influences.empty());
    EXPECT_EQ (mesh.getPrimitive (1).morphTargets.size(), 1u);
}

TEST_F (MeshTests, GeneratedFlatNormalsTakeInfluencesAndMorphDeltasWithTheirVertices)
{
    // The folded triangles of MissingNormalsAreGeneratedFlat. Each vertex follows the joint of its own
    // index, and moves along x by its index
    const std::vector<Mesh::Vertex> vertices { vertexAt (0.0f, 0.0f, 0.0f), vertexAt (1.0f, 0.0f, 0.0f), vertexAt (0.0f, 1.0f, 0.0f), vertexAt (0.0f, 0.0f, 1.0f) };
    std::vector<Mesh::Influences> influences (4);
    std::vector<Mesh::MorphTarget> targets (1);

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        influences[i] = { { static_cast<uint16> (i), 0, 0, 0 }, { 1.0f, 0.0f, 0.0f, 0.0f } };
        targets[0].positions.push_back ({ static_cast<float> (i), 0.0f, 0.0f });
    }

    Mesh mesh;
    mesh.addPrimitive (vertices, { 0, 1, 2, 0, 3, 1 }, nullptr, influences, targets);

    // Every triangle got its own three vertices, in the order of the indices: so did what comes with them
    const auto& primitive = mesh.getPrimitive (0);
    const std::array<uint16, 6> source { 0, 1, 2, 0, 3, 1 };
    ASSERT_EQ (primitive.vertices.size(), source.size());
    ASSERT_EQ (primitive.influences.size(), source.size());
    ASSERT_EQ (primitive.morphTargets[0].positions.size(), source.size());

    for (size_t i = 0; i < source.size(); ++i)
    {
        EXPECT_EQ (primitive.influences[i].joints[0], source[i]) << i;
        EXPECT_FLOAT_EQ (primitive.influences[i].weights[0], 1.0f) << i;
        EXPECT_FLOAT_EQ (primitive.morphTargets[0].positions[i].getX(), static_cast<float> (source[i])) << i;
    }

    EXPECT_EQ (mesh.getNumJoints(), 4);
}

TEST_F (MeshTests, GeneratedFlatNormalsKeepMorphTargetsFlat)
{
    const std::vector<Vector3<float>> authored (3, Vector3<float> (9.0f, 9.0f, 9.0f));

    // The first target takes the last vertex from (0, 1, 0) to (0, 0, 1): the triangle, which faces +z,
    // then faces -y. The second one moves no position
    std::vector<Mesh::MorphTarget> targets (2);
    targets[0].positions = { {}, {}, { 0.0f, -1.0f, 1.0f } };
    targets[0].normals = authored;
    targets[0].tangents = authored;
    targets[1].normals = authored;

    Mesh mesh;
    mesh.addPrimitive ({ vertexAt (0.0f, 0.0f, 0.0f), vertexAt (1.0f, 0.0f, 0.0f), vertexAt (0.0f, 1.0f, 0.0f) }, {}, nullptr, {}, targets);

    // The normal of the triangle in the target, (0, -1, 0), less the one it has in the primitive, (0, 0, 1)
    const auto& primitive = mesh.getPrimitive (0);
    ASSERT_EQ (primitive.morphTargets[0].normals.size(), 3u);

    for (const auto& delta : primitive.morphTargets[0].normals)
        expectNear (delta, { 0.0f, -1.0f, -1.0f });

    // Deltas authored for normals and tangents the primitive doesn't have are dropped, as its tangents are
    EXPECT_TRUE (primitive.morphTargets[0].tangents.empty());
    EXPECT_TRUE (primitive.morphTargets[1].normals.empty());
    EXPECT_EQ (primitive.morphTargets[0].positions.size(), 3u);
}

TEST_F (MeshTests, MorphWeightsAreKeptAsTheyAreSet)
{
    Mesh mesh;
    mesh.setMorphWeights ({ 0.25f, 1.0f });

    const auto weights = mesh.getMorphWeights();
    ASSERT_EQ (weights.size(), 2u);
    EXPECT_FLOAT_EQ (weights[0], 0.25f);
    EXPECT_FLOAT_EQ (weights[1], 1.0f);
}

TEST_F (MeshTests, AddPrimitiveRejectsInfluencesAndMorphDeltasThatDoNotFitTheVertices)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "Influences and morph deltas that are not one per vertex trigger an assertion when assertions are enabled or logged";
#endif

    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), {}, nullptr, std::vector<Mesh::Influences> (2));

    std::vector<Mesh::MorphTarget> targets (1);
    targets[0].normals.resize (4);
    mesh.addPrimitive (makeTriangle (0.0f), {}, nullptr, {}, targets);

    EXPECT_EQ (mesh.getNumPrimitives(), 0);
    EXPECT_EQ (mesh.getNumMorphTargets(), 0);
    EXPECT_TRUE (mesh.getBounds().isEmpty());
}

class MeshGpuResourceTests : public MeshTests
{
protected:
    /** What the mocked ore context was asked about the buffers of one usage. */
    struct BufferLog
    {
        int made = 0;
        int updates = 0;
        std::vector<uint8> contents; // The bytes the last buffer made or updated was given

        /** True when the last buffer made or updated was given exactly @p vertices. */
        bool holds (const std::vector<Mesh::Vertex>& vertices) const
        {
            return contents.size() == vertices.size() * sizeof (Mesh::Vertex)
                && std::memcmp (contents.data(), vertices.data(), contents.size()) == 0;
        }
    };

    void SetUp() override
    {
        using ::testing::_;
        using ::testing::Invoke;

        ON_CALL (oreCtx, makeBuffer (_)).WillByDefault (Invoke ([this] (const rive::ore::BufferDesc& desc)
        {
            auto& log = desc.usage == rive::ore::BufferUsage::vertex ? vertexBuffers : indexBuffers;
            const auto keep = [&log] (const void* data, uint32_t size)
            {
                log.contents.assign (static_cast<const uint8*> (data), static_cast<const uint8*> (data) + size);
            };

            ++log.made;
            keep (desc.data, desc.size);

            auto buffer = rive::make_rcp<::testing::NiceMock<MockOreBuffer>> (desc.size, desc.usage);
            ON_CALL (*buffer, update (_, _, _)).WillByDefault (Invoke ([&log, keep] (const void* data, uint32_t size, uint32_t)
            {
                ++log.updates;
                keep (data, size);
            }));

            return rive::rcp<rive::ore::Buffer> (buffer);
        }));

        device = new OreInjectedGpuDevice (&oreCtx);
    }

    ::testing::NiceMock<MockOreContext> oreCtx;
    GpuDevice::Ptr device;
    BufferLog vertexBuffers;
    BufferLog indexBuffers;
};

TEST_F (MeshGpuResourceTests, SetVerticesRewritesTheVertexBufferWhenItIsNextAskedFor)
{
    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), {});
    mesh.addPrimitive (makeTriangle (10.0f), {});

    const auto vertices = mesh.getVertexBuffer (device, 0);
    const auto otherVertices = mesh.getVertexBuffer (device, 1);
    const auto indices = mesh.getIndexBuffer (device, 0);
    ASSERT_NE (vertices, nullptr);
    ASSERT_NE (otherVertices, nullptr);
    ASSERT_NE (indices, nullptr);
    EXPECT_EQ (vertexBuffers.made, 2);
    EXPECT_EQ (indexBuffers.made, 2);

    const auto moved = makeTriangle (5.0f);
    mesh.setVertices (0, moved);

    // Nothing is uploaded until the buffer is asked for, and the other buffers have nothing to take
    EXPECT_EQ (mesh.getVertexBuffer (device, 1), otherVertices);
    EXPECT_EQ (mesh.getIndexBuffer (device, 0), indices);
    EXPECT_EQ (vertexBuffers.updates, 0);

    // The buffer is kept and rewritten, once
    EXPECT_EQ (mesh.getVertexBuffer (device, 0), vertices);
    EXPECT_EQ (mesh.getVertexBuffer (device, 0), vertices);
    EXPECT_EQ (vertexBuffers.updates, 1);
    EXPECT_TRUE (vertexBuffers.holds (moved));

    EXPECT_EQ (vertexBuffers.made, 2);
    EXPECT_EQ (indexBuffers.made, 2);
    EXPECT_EQ (indexBuffers.updates, 0);
}

TEST_F (MeshGpuResourceTests, VerticesSetWithoutABufferAreUploadedWithIt)
{
    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), {});

    // Before the first upload there is no buffer to rewrite: the upload takes the new vertices
    const auto moved = makeTriangle (5.0f);
    mesh.setVertices (0, moved);

    ASSERT_NE (mesh.getVertexBuffer (device, 0), nullptr);
    EXPECT_EQ (vertexBuffers.made, 1);
    EXPECT_EQ (vertexBuffers.updates, 0);
    EXPECT_TRUE (vertexBuffers.holds (moved));

    // Another device makes its buffers from the vertices as they are now, with nothing left to rewrite
    const auto movedAgain = makeTriangle (7.0f);
    mesh.setVertices (0, movedAgain);

    GpuDevice::Ptr otherDevice = new OreInjectedGpuDevice (&oreCtx);
    ASSERT_NE (mesh.getVertexBuffer (otherDevice, 0), nullptr);
    EXPECT_EQ (vertexBuffers.made, 2);
    EXPECT_EQ (vertexBuffers.updates, 0);
    EXPECT_TRUE (vertexBuffers.holds (movedAgain));
}

//==============================================================================

TEST_F (MeshTests, IntersectTrianglesReturnsTheNearestHit)
{
    Mesh mesh;

    const Ray ray ({ 0.25f, 0.25f, 5.0f }, { 0.0f, 0.0f, -1.0f });
    EXPECT_FALSE (mesh.intersectTriangles (ray).has_value());

    // Two triangles facing +z, the second one 2 behind the first
    auto behind = makeTriangle (0.0f);
    for (auto& vertex : behind)
        vertex.position = vertex.position + Vector3<float> (0.0f, 0.0f, -2.0f);

    mesh.addPrimitive (makeTriangle (0.0f), {});
    mesh.addPrimitive (behind, {});

    EXPECT_FLOAT_EQ (mesh.intersectTriangles (ray).value_or (-1.0f), 5.0f);

    // Between two distances
    EXPECT_FLOAT_EQ (mesh.intersectTriangles (ray, 5.5f).value_or (-1.0f), 7.0f);
    EXPECT_FALSE (mesh.intersectTriangles (ray, 5.5f, 6.5f).has_value());
    EXPECT_FALSE (mesh.intersectTriangles (ray, 0.0f, 4.5f).has_value());

    // Inside the bounds of the triangles, and beside them
    EXPECT_FALSE (mesh.intersectTriangles (Ray ({ 0.75f, 0.75f, 5.0f }, { 0.0f, 0.0f, -1.0f })).has_value());

    // A distance is in multiples of the length of the direction
    EXPECT_FLOAT_EQ (mesh.intersectTriangles (Ray ({ 0.25f, 0.25f, 5.0f }, { 0.0f, 0.0f, -2.0f })).value_or (-1.0f), 2.5f);
}

TEST_F (MeshTests, IntersectTrianglesHitsBackFacesOnlyWhenTheirMaterialIsDoubleSided)
{
    auto doubleSided = Material::Ptr (new Material());
    doubleSided->doubleSided = true;

    Mesh mesh;
    mesh.addPrimitive (makeTriangle (0.0f), {});
    mesh.addPrimitive (makeTriangle (2.0f), {}, doubleSided);

    // From behind: the triangles face +z
    const Ray overFirst ({ 0.25f, 0.25f, -5.0f }, { 0.0f, 0.0f, 1.0f });
    const Ray overSecond ({ 2.25f, 0.25f, -5.0f }, { 0.0f, 0.0f, 1.0f });

    EXPECT_FALSE (mesh.intersectTriangles (overFirst).has_value());
    EXPECT_FLOAT_EQ (mesh.intersectTriangles (overSecond).value_or (-1.0f), 5.0f);

    // A material drawing the whole mesh decides for every primitive
    const auto farthest = std::numeric_limits<float>::max();
    EXPECT_FLOAT_EQ (mesh.intersectTriangles (overFirst, 0.0f, farthest, doubleSided.get()).value_or (-1.0f), 5.0f);

    const Material singleSided {};
    EXPECT_FALSE (mesh.intersectTriangles (overSecond, 0.0f, farthest, &singleSided).has_value());
}

TEST_F (MeshTests, IntersectTrianglesHitsTrianglesOfAnySize)
{
    // A tenth of a millimeter wide for a mesh in meters, which is under what a ray takes as parallel to it, and a kilometer
    for (const auto size : { 1.0e-4f, 1.0e3f })
    {
        Mesh mesh;
        mesh.addPrimitive ({ vertexAt (0.0f, 0.0f, 0.0f), vertexAt (size, 0.0f, 0.0f), vertexAt (0.0f, size, 0.0f) }, {});

        const Ray inside ({ size * 0.25f, size * 0.25f, 1.0f }, { 0.0f, 0.0f, -1.0f });
        const Ray beside ({ size * 0.75f, size * 0.75f, 1.0f }, { 0.0f, 0.0f, -1.0f });

        EXPECT_NEAR (mesh.intersectTriangles (inside).value_or (-1.0f), 1.0f, 1.0e-5f) << size;
        EXPECT_FALSE (mesh.intersectTriangles (beside).has_value()) << size;
    }
}
