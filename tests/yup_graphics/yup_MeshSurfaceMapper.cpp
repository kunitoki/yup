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

#include <yup_graphics/yup_graphics.h>

#include <vector>

using namespace yup;

class MeshSurfaceMapperTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Unit quad in the z = 0 plane, counter-clockwise from +z, v pointing down like a texture
        quadPositions = { { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f } };
        quadUVs = { { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f } };
        quadIndices = { 0, 1, 2, 0, 2, 3 };
    }

    Matrix4 makeModelViewProjection (const Matrix4& model) const
    {
        return model.followedBy (Matrix4::lookAt ({ 0.0f, 0.0f, 5.0f }, {}, { 0.0f, 1.0f, 0.0f }))
            .followedBy (Matrix4::perspective (0.9f, viewport.getWidth() / viewport.getHeight(), 0.1f, 100.0f));
    }

    MeshSurfaceMapper makeQuadMapper (const Matrix4& model) const
    {
        MeshSurfaceMapper mapper;
        mapper.setMesh (quadPositions, quadUVs, quadIndices);
        mapper.setModelViewProjection (makeModelViewProjection (model), viewport);
        return mapper;
    }

    Point<float> project (const Matrix4& modelViewProjection, const Vector3<float>& point) const
    {
        const auto ndc = modelViewProjection.transformPoint (point);
        return { viewport.getX() + (ndc.getX() + 1.0f) * 0.5f * viewport.getWidth(),
                 viewport.getY() + (1.0f - ndc.getY()) * 0.5f * viewport.getHeight() };
    }

    static void expectNear (Point<float> actual, Point<float> expected, float tolerance)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
    }

    const Rectangle<float> viewport { 20.0f, 10.0f, 400.0f, 300.0f };
    std::vector<Vector3<float>> quadPositions;
    std::vector<Point<float>> quadUVs;
    std::vector<uint32> quadIndices;
};

TEST_F (MeshSurfaceMapperTests, ObliqueQuadCornersMapExactly)
{
    const auto model = Matrix4::rotationY (0.6f).followedBy (Matrix4::rotationX (-0.3f));
    const auto mapper = makeQuadMapper (model);
    const auto modelViewProjection = makeModelViewProjection (model);

    for (std::size_t i = 0; i < quadPositions.size(); ++i)
    {
        const auto viewportPoint = mapper.uvToViewport (quadUVs[i]);
        ASSERT_TRUE (viewportPoint.has_value());
        expectNear (*viewportPoint, project (modelViewProjection, quadPositions[i]), 1.0e-3f);
    }
}

TEST_F (MeshSurfaceMapperTests, UVToViewportRoundTripsThroughViewportToUV)
{
    const auto mapper = makeQuadMapper (Matrix4::rotationY (0.6f));

    for (const auto uv : { Point<float> (0.1f, 0.1f), Point<float> (0.9f, 0.2f), Point<float> (0.5f, 0.5f), Point<float> (0.3f, 0.8f) })
    {
        const auto viewportPoint = mapper.uvToViewport (uv);
        ASSERT_TRUE (viewportPoint.has_value());

        const auto mapped = mapper.viewportToUV (*viewportPoint);
        ASSERT_TRUE (mapped.has_value());
        expectNear (*mapped, uv, 1.0e-3f);

        const auto hit = mapper.hitTest (*viewportPoint);
        ASSERT_TRUE (hit.has_value());
        expectNear (hit->uv, uv, 1.0e-3f);
    }
}

TEST_F (MeshSurfaceMapperTests, CylindricalStripHasMonotonicUVAcrossTheCurve)
{
    constexpr int segments = 16;
    constexpr float radius = 2.0f;
    constexpr float halfArc = 1.0f;

    std::vector<Vector3<float>> positions;
    std::vector<Point<float>> uvs;
    std::vector<uint32> indices;

    for (int i = 0; i <= segments; ++i)
    {
        const auto u = static_cast<float> (i) / segments;
        const auto angle = -halfArc + u * 2.0f * halfArc;
        const auto x = radius * std::sin (angle);
        const auto z = radius * std::cos (angle) - radius;

        positions.push_back ({ x, -1.0f, z });
        uvs.push_back ({ u, 1.0f });
        positions.push_back ({ x, 1.0f, z });
        uvs.push_back ({ u, 0.0f });
    }

    for (uint32 i = 0; i < segments; ++i)
    {
        const auto bottom = i * 2;
        const auto top = bottom + 1;
        const auto nextBottom = bottom + 2;
        const auto nextTop = bottom + 3;

        indices.insert (indices.end(), { bottom, nextBottom, nextTop, bottom, nextTop, top });
    }

    MeshSurfaceMapper mapper;
    mapper.setMesh (positions, uvs, indices);
    mapper.setModelViewProjection (makeModelViewProjection ({}), viewport);

    const auto left = mapper.uvToViewport ({ 0.02f, 0.5f });
    const auto right = mapper.uvToViewport ({ 0.98f, 0.5f });
    ASSERT_TRUE (left.has_value() && right.has_value());

    float previousU = -1.0f;
    for (int step = 0; step <= 50; ++step)
    {
        const auto point = *left + (*right - *left) * (static_cast<float> (step) / 50.0f);

        const auto hit = mapper.hitTest (point);
        ASSERT_TRUE (hit.has_value());
        EXPECT_GT (hit->uv.getX(), previousU);
        EXPECT_NEAR (hit->uv.getY(), 0.5f, 0.05f);

        previousU = hit->uv.getX();
    }
}

TEST_F (MeshSurfaceMapperTests, MissIsExtrapolatedOutsideTheSurface)
{
    const auto mapper = makeQuadMapper (Matrix4::rotationY (0.3f));

    const auto rightEdge = mapper.uvToViewport ({ 1.0f, 0.5f });
    ASSERT_TRUE (rightEdge.has_value());

    const auto outside = *rightEdge + Point<float> (40.0f, 0.0f);
    EXPECT_FALSE (mapper.hitTest (outside).has_value());

    const auto uv = mapper.viewportToUV (outside);
    ASSERT_TRUE (uv.has_value());
    EXPECT_GT (uv->getX(), 1.0f);
    EXPECT_NEAR (uv->getY(), 0.5f, 0.05f);

    const auto above = mapper.viewportToUV (*mapper.uvToViewport ({ 0.5f, 0.0f }) - Point<float> (0.0f, 30.0f));
    ASSERT_TRUE (above.has_value());
    EXPECT_LT (above->getY(), 0.0f);
}

TEST_F (MeshSurfaceMapperTests, NearestOfTwoOverlappingTrianglesWins)
{
    const std::vector<Vector3<float>> positions {
        { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }
    };
    const std::vector<Point<float>> uvs {
        { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f },
        { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }
    };

    for (const auto& indices : { std::vector<uint32> { 0, 1, 2, 3, 4, 5 }, std::vector<uint32> { 3, 4, 5, 0, 1, 2 } })
    {
        MeshSurfaceMapper mapper;
        mapper.setMesh (positions, uvs, indices);
        mapper.setModelViewProjection (makeModelViewProjection ({}), viewport);

        const auto hit = mapper.hitTest (viewport.getCenter());
        ASSERT_TRUE (hit.has_value());
        expectNear (hit->uv, { 1.0f, 1.0f }, 1.0e-5f);
        EXPECT_EQ (hit->triangleIndex, indices[0] == 3 ? 0 : 1);
    }
}

TEST_F (MeshSurfaceMapperTests, CloserHitsHaveLowerDepth)
{
    const auto nearMapper = makeQuadMapper (Matrix4::translation ({ 0.0f, 0.0f, 1.0f }));
    const auto farMapper = makeQuadMapper ({});

    const auto nearHit = nearMapper.hitTest (viewport.getCenter());
    const auto farHit = farMapper.hitTest (viewport.getCenter());
    ASSERT_TRUE (nearHit.has_value() && farHit.has_value());
    EXPECT_LT (nearHit->depth, farHit->depth);
}

TEST_F (MeshSurfaceMapperTests, BackFaceIsRejectedWhenCulling)
{
    auto mapper = makeQuadMapper (Matrix4::rotationY (MathConstants<float>::pi));
    EXPECT_TRUE (mapper.isBackFaceCulling());

    EXPECT_FALSE (mapper.hitTest (viewport.getCenter()).has_value());
    EXPECT_FALSE (mapper.viewportToUV (viewport.getCenter()).has_value());

    mapper.setBackFaceCulling (false);
    EXPECT_FALSE (mapper.isBackFaceCulling());

    const auto hit = mapper.hitTest (viewport.getCenter());
    ASSERT_TRUE (hit.has_value());
    expectNear (hit->uv, { 0.5f, 0.5f }, 1.0e-3f);
}

TEST_F (MeshSurfaceMapperTests, UVOutsideTheMeshHasNoViewportPoint)
{
    const auto mapper = makeQuadMapper ({});
    EXPECT_FALSE (mapper.uvToViewport ({ 1.5f, 0.5f }).has_value());
}

TEST_F (MeshSurfaceMapperTests, EmptyMapperFindsNothing)
{
    MeshSurfaceMapper mapper;
    EXPECT_FALSE (mapper.hitTest ({ 10.0f, 10.0f }).has_value());
    EXPECT_FALSE (mapper.viewportToUV ({ 10.0f, 10.0f }).has_value());
    EXPECT_FALSE (mapper.uvToViewport ({ 0.5f, 0.5f }).has_value());

    mapper.setMesh (quadPositions, quadUVs, quadIndices);
    EXPECT_FALSE (mapper.hitTest ({ 10.0f, 10.0f }).has_value()); // No viewport yet
}
