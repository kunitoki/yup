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

#include <yup_renderer/yup_renderer.h>

#include <gtest/gtest.h>

using namespace yup;

class LightClustersTests : public ::testing::Test
{
protected:
    static GpuLight pointAt (Vector3<float> position, float range)
    {
        GpuLight light {};
        light.positionType[0] = position.getX();
        light.positionType[1] = position.getY();
        light.positionType[2] = position.getZ();
        light.positionType[3] = 1.0f;
        light.directionRange[3] = range;
        light.colorIntensity[3] = 1.0f;
        light.spotRadiusShadow[3] = -1.0f;
        return light;
    }

    static GpuLight spotAt (Vector3<float> position, Vector3<float> direction, float range, float outerConeAngle)
    {
        auto light = pointAt (position, range);
        const auto unit = direction.normalized();
        const auto cosOuter = std::cos (outerConeAngle);
        const auto scale = 1.0f / jmax (0.001f, 1.0f - cosOuter); // inner cone angle 0
        light.positionType[3] = 2.0f;
        light.directionRange[0] = unit.getX();
        light.directionRange[1] = unit.getY();
        light.directionRange[2] = unit.getZ();
        light.spotRadiusShadow[0] = scale;
        light.spotRadiusShadow[1] = -cosOuter * scale;
        return light;
    }

    LightClusters::Grid grid;
    Matrix4 projection = Matrix4::perspective (0.8f, 16.0f / 9.0f, 0.1f, 1000.0f);
    Matrix4 inverseProjection = projection.inverted();
    Matrix4 view; // identity: camera at the origin looking down -Z
};

TEST_F (LightClustersTests, SlicesAreExponentialAndRoundTrip)
{
    EXPECT_EQ (LightClusters::getSliceForDepth (grid, 0.1f), 0);
    EXPECT_EQ (LightClusters::getSliceForDepth (grid, 999.0f), grid.slices - 1);
    EXPECT_EQ (LightClusters::getSliceForDepth (grid, 5000.0f), grid.slices - 1);
    EXPECT_EQ (LightClusters::getSliceForDepth (grid, 0.01f), 0);

    for (int k = 0; k < grid.slices; ++k)
    {
        const auto mid = std::sqrt (LightClusters::getDepthForSlice (grid, k) * LightClusters::getDepthForSlice (grid, k + 1));
        EXPECT_EQ (LightClusters::getSliceForDepth (grid, mid), k);
    }

    EXPECT_NEAR (LightClusters::getDepthForSlice (grid, grid.slices), grid.farPlane, 1.0e-2f);
}

TEST_F (LightClustersTests, DegenerateDepthsFallInTheEndSlices)
{
    EXPECT_EQ (LightClusters::getSliceForDepth (grid, 0.0f), 0);
    EXPECT_EQ (LightClusters::getSliceForDepth (grid, -3.0f), 0);
    EXPECT_EQ (LightClusters::getSliceForDepth (grid, std::numeric_limits<float>::quiet_NaN()), 0);
    EXPECT_EQ (LightClusters::getSliceForDepth (grid, std::numeric_limits<float>::infinity()), grid.slices - 1);
}

TEST_F (LightClustersTests, GridFollowsTheCameraPlanes)
{
    Scene scene;
    auto entity = EntityNode::Ptr (new EntityNode ("camera"));
    auto& camera = entity->attach<CameraNode>();
    scene.getRoot()->addChild (entity);
    scene.setActiveCamera (entity.get());

    const auto gridOf = [&] (float zNear, float zFar)
    {
        camera.zNear = zNear;
        camera.zFar = zFar;

        ViewData viewData;
        viewData.update (scene, SceneSettings::resolve (scene), 64, 64, false, 1, 0);
        return LightClusters::getGrid (viewData);
    };

    const auto finite = gridOf (0.5f, 200.0f);
    EXPECT_FLOAT_EQ (finite.nearPlane, 0.5f);
    EXPECT_FLOAT_EQ (finite.farPlane, 200.0f);
    EXPECT_FALSE (finite.infiniteFar);

    // An infinite far plane clusters up to 1000 units, its last slice open beyond
    const auto infinite = gridOf (0.1f, 0.0f);
    EXPECT_FLOAT_EQ (infinite.farPlane, 1000.0f);
    EXPECT_TRUE (infinite.infiniteFar);

    // Orthographic cameras may start at the camera: the slices stay finite
    camera.projection = CameraNode::Projection::orthographic;
    const auto orthographic = gridOf (0.0f, 50.0f);
    EXPECT_GT (orthographic.nearPlane, 0.0f);
    EXPECT_GT (LightClusters::getSliceForDepth (orthographic, 25.0f), 0);
    EXPECT_LT (LightClusters::getSliceForDepth (orthographic, 25.0f), orthographic.slices - 1);
}

TEST_F (LightClustersTests, PackedGridMatchesTheClusterBlock)
{
    grid.infiniteFar = true;

    float packedGrid[4] = {}, packedDepth[4] = {};
    LightClusters::packGrid (grid, 12, 3, packedGrid, packedDepth);

    EXPECT_EQ (packedGrid[0], 16.0f);
    EXPECT_EQ (packedGrid[1], 9.0f);
    EXPECT_EQ (packedGrid[2], 24.0f);
    EXPECT_EQ (packedGrid[3], 12.0f);
    EXPECT_EQ (packedDepth[0], grid.nearPlane);
    EXPECT_EQ (packedDepth[1], grid.farPlane);
    EXPECT_EQ (packedDepth[2], 3.0f);
    EXPECT_EQ (packedDepth[3], 1.0f);
}

TEST_F (LightClustersTests, ClusterIndexIsRowMajorPerSlice)
{
    const Point<float> viewport (1600.0f, 900.0f);
    EXPECT_EQ (LightClusters::getClusterIndex (grid, { 0.0f, 0.0f }, viewport, 0.1f), 0);
    EXPECT_EQ (LightClusters::getClusterIndex (grid, { 1599.0f, 0.0f }, viewport, 0.1f), 15);
    EXPECT_EQ (LightClusters::getClusterIndex (grid, { 0.0f, 899.0f }, viewport, 0.1f), 16 * 8);
    EXPECT_EQ (LightClusters::getClusterIndex (grid, { 0.0f, 0.0f }, viewport, 999.0f), 16 * 9 * 23);
    EXPECT_EQ (LightClusters::getClusterIndex (grid, { 1600.0f, 900.0f }, viewport, 0.1f), 16 * 9 - 1);
}

TEST_F (LightClustersTests, ClusterBoundsTileTheFrustum)
{
    // A cluster ends at the far depth of its slice, where the next slice of the same tile starts
    const auto a = LightClusters::getClusterBounds (grid, 3, 4, 10, inverseProjection);
    const auto b = LightClusters::getClusterBounds (grid, 3, 4, 11, inverseProjection);

    EXPECT_LT (a.getMin().getZ(), 0.0f); // in front of the camera, -Z
    EXPECT_NEAR (-a.getMin().getZ(), LightClusters::getDepthForSlice (grid, 11), 1.0e-3f * LightClusters::getDepthForSlice (grid, 11));
    EXPECT_NEAR (a.getMin().getZ(), b.getMax().getZ(), 1.0e-3f);
}

TEST_F (LightClustersTests, TopLeftTileIsAboveAndLeftOfTheViewAxis)
{
    // Tile y = 0 is the top row of pixels (NDC y = +1), where gl_FragCoord.y is 0
    const auto topLeft = LightClusters::getClusterBounds (grid, 0, 0, 10, inverseProjection);
    EXPECT_GT (topLeft.getMin().getY(), 0.0f);
    EXPECT_LT (topLeft.getMax().getX(), 0.0f);

    const auto bottomRight = LightClusters::getClusterBounds (grid, grid.tilesX - 1, grid.tilesY - 1, 10, inverseProjection);
    EXPECT_LT (bottomRight.getMax().getY(), 0.0f);
    EXPECT_GT (bottomRight.getMin().getX(), 0.0f);
}

TEST_F (LightClustersTests, ClusterBoundsHoldForInfiniteAndOrthographicCameras)
{
    CameraNode camera;
    camera.zFar = 0.0f;
    const auto infinite = LightClusters::getClusterBounds (grid, 3, 4, 10, camera.getProjectionMatrix (16.0f / 9.0f).inverted());
    const auto finite = LightClusters::getClusterBounds (grid, 3, 4, 10, inverseProjection);

    for (int axis = 0; axis < 3; ++axis)
    {
        const auto get = [axis] (const Vector3<float>& v)
        {
            return axis == 0 ? v.getX() : (axis == 1 ? v.getY() : v.getZ());
        };
        EXPECT_NEAR (get (infinite.getMin()), get (finite.getMin()), 1.0e-3f) << axis;
        EXPECT_NEAR (get (infinite.getMax()), get (finite.getMax()), 1.0e-3f) << axis;
    }

    camera.projection = CameraNode::Projection::orthographic;
    camera.zFar = 1000.0f;
    camera.xMag = 8.0f;
    camera.yMag = 4.5f;
    const auto orthographic = LightClusters::getClusterBounds (grid, 3, 4, 10, camera.getProjectionMatrix (16.0f / 9.0f).inverted());
    EXPECT_NEAR (orthographic.getMin().getX(), -8.0f + 16.0f * 3.0f / 16.0f, 1.0e-3f);
    EXPECT_NEAR (orthographic.getMax().getX(), -8.0f + 16.0f * 4.0f / 16.0f, 1.0e-3f);
    EXPECT_NEAR (orthographic.getMax().getY(), 4.5f - 9.0f * 4.0f / 9.0f, 1.0e-3f);
    EXPECT_NEAR (-orthographic.getMax().getZ(), LightClusters::getDepthForSlice (grid, 10), 1.0e-3f);
}

TEST_F (LightClustersTests, InfiniteFarPlaneOpensTheLastSlice)
{
    // Past the far plane of the grid: only an infinite camera rasterizes fragments there
    const GpuLight lights[] = { pointAt ({ 0.0f, 0.0f, -1500.0f }, 5.0f) };
    const auto lastSlice = LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 1500.0f);
    ASSERT_EQ (lastSlice / (grid.tilesX * grid.tilesY), grid.slices - 1);

    EXPECT_TRUE (LightClusters::assign (grid, lights, 0, view, inverseProjection)[static_cast<size_t> (lastSlice)].empty());

    grid.infiniteFar = true;
    EXPECT_EQ (LightClusters::assign (grid, lights, 0, view, inverseProjection)[static_cast<size_t> (lastSlice)], (std::vector<uint32> { 0 }));

    // Only the last slice opens, and the bounds stay finite numbers
    const auto open = LightClusters::getClusterBounds (grid, 3, 4, grid.slices - 1, inverseProjection);
    EXPECT_EQ (open.getMin().getZ(), -std::numeric_limits<float>::max());
    EXPECT_EQ (open.getMin().getX(), -std::numeric_limits<float>::max()); // tile 3 is left of the axis
    EXPECT_LT (open.getMax().getX(), 0.0f);
    EXPECT_FALSE (std::isnan (open.getMax().getY()));
    EXPECT_NEAR (-open.getMax().getZ(), LightClusters::getDepthForSlice (grid, grid.slices - 1), 1.0e-4f * LightClusters::getDepthForSlice (grid, grid.slices - 1));

    const auto closed = LightClusters::getClusterBounds (grid, 3, 4, grid.slices - 2, inverseProjection);
    EXPECT_GT (closed.getMin().getZ(), -grid.farPlane);
}

TEST_F (LightClustersTests, LightOnlyReachesNearbyClusters)
{
    const GpuLight lights[] = { pointAt ({ 0.0f, 0.0f, -5.0f }, 0.5f) };
    const auto assignment = LightClusters::assign (grid, lights, 0, view, inverseProjection);

    ASSERT_EQ (static_cast<int> (assignment.size()), grid.getNumClusters());

    int reached = 0;
    for (const auto& list : assignment)
        reached += list.empty() ? 0 : 1;

    EXPECT_GT (reached, 0);
    EXPECT_LT (reached, 40); // a 1-unit sphere at 5 units covers a handful of clusters, not the screen

    // The cluster at the screen center and depth 5 holds it
    const auto center = LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 5.0f);
    EXPECT_EQ (assignment[static_cast<size_t> (center)], (std::vector<uint32> { 0 }));
}

TEST_F (LightClustersTests, ClusterOverflowKeepsLowestIndices)
{
    std::vector<GpuLight> lights;
    for (int i = 0; i < 100; ++i)
        lights.push_back (pointAt ({ 0.0f, 0.0f, -5.0f }, 1.0f));

    const auto assignment = LightClusters::assign (grid, lights, 0, view, inverseProjection);
    const auto center = LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 5.0f);
    const auto& list = assignment[static_cast<size_t> (center)];

    ASSERT_EQ (static_cast<int> (list.size()), LightClusters::maxLightsPerCluster);
    for (int i = 0; i < LightClusters::maxLightsPerCluster; ++i)
        EXPECT_EQ (list[static_cast<size_t> (i)], static_cast<uint32> (i));
}

TEST_F (LightClustersTests, DirectionalLightsAreNotClustered)
{
    GpuLight sun {};
    sun.directionRange[1] = -1.0f;
    const GpuLight lights[] = { sun, pointAt ({ 0.0f, 0.0f, -5.0f }, 0.5f) };

    const auto assignment = LightClusters::assign (grid, lights, 1, view, inverseProjection);
    for (const auto& list : assignment)
        for (auto index : list)
            EXPECT_EQ (index, 1u);

    // Not even when a caller starts the local lights at the directional one
    const auto bounds = LightClusters::getClusterBounds (grid, 8, 4, 10, inverseProjection);
    EXPECT_FALSE (LightClusters::intersects (sun, view, bounds));
}

TEST_F (LightClustersTests, NegativeRangeIsAnEffectiveRange)
{
    const GpuLight lights[] = { pointAt ({ 0.0f, 0.0f, -5.0f }, -0.5f) }; // effective-only range, Task 4
    const auto assignment = LightClusters::assign (grid, lights, 0, view, inverseProjection);
    const auto center = LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 5.0f);
    EXPECT_EQ (assignment[static_cast<size_t> (center)].size(), 1u);
}

TEST_F (LightClustersTests, DarkLightsAreCulled)
{
    // A black or zero intensity light packs a range of -0.0
    const auto center = LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 5.0f);
    const auto bounds = LightClusters::getClusterBounds (grid, center % grid.tilesX, (center / grid.tilesX) % grid.tilesY, center / (grid.tilesX * grid.tilesY), inverseProjection);

    EXPECT_FALSE (LightClusters::intersects (pointAt ({ 0.0f, 0.0f, -5.0f }, -0.0f), view, bounds));
    EXPECT_FALSE (LightClusters::intersects (pointAt ({ 0.0f, 0.0f, -5.0f }, 0.0f), view, bounds));
    EXPECT_TRUE (LightClusters::intersects (pointAt ({ 0.0f, 0.0f, -5.0f }, 0.1f), view, bounds));
}

TEST_F (LightClustersTests, SpotLightsOnlyReachClustersAroundTheirCone)
{
    // A narrow spot 3 units left of the axis, pointing further left: its range sphere reaches the
    // center cluster, but its cone does not
    const auto center = LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 5.0f);
    const auto bounds = LightClusters::getClusterBounds (grid, center % grid.tilesX, (center / grid.tilesX) % grid.tilesY, center / (grid.tilesX * grid.tilesY), inverseProjection);

    EXPECT_FALSE (LightClusters::intersects (spotAt ({ -3.0f, 0.0f, -5.0f }, { -1.0f, 0.0f, 0.0f }, 4.0f, 0.3f), view, bounds));
    EXPECT_TRUE (LightClusters::intersects (spotAt ({ -3.0f, 0.0f, -5.0f }, { 1.0f, 0.0f, 0.0f }, 4.0f, 0.3f), view, bounds));
    EXPECT_TRUE (LightClusters::intersects (pointAt ({ -3.0f, 0.0f, -5.0f }, 4.0f), view, bounds));
}

TEST_F (LightClustersTests, SpotBoundIsConservativeAtEveryConeAngle)
{
    // Every point inside a spot's cone volume must find the light in its cluster, wide cones included
    const Point<float> viewport (1600.0f, 900.0f);
    const Vector3<float> apex (0.0f, 0.0f, -20.0f);
    const float range = 6.0f;
    const Vector3<float> directions[] = { { 1.0f, 0.0f, 0.0f }, Vector3<float> (0.6f, 0.48f, 0.64f).normalized() };
    const float degrees[] = { 10.0f, 30.0f, 45.0f, 60.0f, 85.0f, 89.0f };

    for (const auto& direction : directions)
    {
        // An orthonormal basis around the cone axis
        const auto u = direction.crossProduct (std::abs (direction.getY()) < 0.9f ? Vector3<float> (0.0f, 1.0f, 0.0f) : Vector3<float> (1.0f, 0.0f, 0.0f)).normalized();
        const auto v = direction.crossProduct (u);

        for (const auto angle : degrees)
        {
            const auto outer = degreesToRadians (angle);
            const GpuLight lights[] = { spotAt (apex, direction, range, outer) };
            const auto assignment = LightClusters::assign (grid, lights, 0, view, inverseProjection);

            int missed = 0;
            int missedPoints = 0;
            for (int d = 1; d <= 10; ++d)
            {
                for (const auto fraction : { 0.0f, 0.25f, 0.5f, 0.75f, 0.98f })
                {
                    for (int a = 0; a < 8; ++a)
                    {
                        const auto phi = outer * fraction;
                        const auto alpha = MathConstants<float>::twoPi * static_cast<float> (a) / 8.0f;
                        const auto offset = direction * std::cos (phi) + (u * std::cos (alpha) + v * std::sin (alpha)) * std::sin (phi);
                        const auto point = apex + offset * (range * 0.098f * static_cast<float> (d));

                        const auto ndc = projection.transformPoint (point);
                        const Point<float> pixel ((ndc.getX() + 1.0f) * 0.5f * viewport.getX(), (1.0f - ndc.getY()) * 0.5f * viewport.getY());
                        const auto cluster = LightClusters::getClusterIndex (grid, pixel, viewport, -point.getZ());

                        missed += assignment[static_cast<size_t> (cluster)].empty() ? 1 : 0;
                        missedPoints += LightClusters::intersects (lights[0], view, BoundingBox (point, point)) ? 0 : 1;
                    }
                }
            }

            EXPECT_EQ (missed, 0) << "outer angle " << angle << " degrees";
            EXPECT_EQ (missedPoints, 0) << "outer angle " << angle << " degrees";
        }
    }
}

TEST_F (LightClustersTests, LightsAreTakenToViewSpace)
{
    // The camera 10 units back along +Z sees the light at the origin at depth 10, in the center
    const auto cameraView = Matrix4::lookAt ({ 0.0f, 0.0f, 10.0f }, {}, { 0.0f, 1.0f, 0.0f });
    const GpuLight lights[] = { pointAt ({}, 0.5f) };
    const auto assignment = LightClusters::assign (grid, lights, 0, cameraView, inverseProjection);

    const auto center = LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 10.0f);
    EXPECT_EQ (assignment[static_cast<size_t> (center)].size(), 1u);

    const auto atFive = LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 5.0f);
    EXPECT_TRUE (assignment[static_cast<size_t> (atFive)].empty());
}

TEST_F (LightClustersTests, BundleDeclaresTheClusterBlockAndBuffers)
{
    static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightClusters.inc"
    };

    auto bundle = ShaderBundle::loadFromData (data, sizeof (data));
    ASSERT_TRUE (bundle.wasOk()) << bundle.getErrorMessage();

    const auto* compute = bundle.getReference().findShader (ShaderStage::compute, ShaderLanguage::spirv);
    ASSERT_NE (compute, nullptr);

    const auto& reflection = compute->reflection;
    EXPECT_EQ (reflection.workgroupSize.x, 64u);
    EXPECT_EQ (reflection.workgroupSize.y, 1u);

    ASSERT_EQ (reflection.uniformBuffers.size(), 1u);
    EXPECT_EQ (reflection.uniformBuffers[0].set, 0u);
    EXPECT_EQ (reflection.uniformBuffers[0].binding, 0u);
    EXPECT_EQ (reflection.uniformBuffers[0].blockSize, 160u);

    std::map<String, uint32_t> offsets;
    for (const auto& member : reflection.uniformBuffers[0].members)
        offsets[member.name] = member.offset;

    EXPECT_EQ (offsets["inverseProjection"], 64u);
    EXPECT_EQ (offsets["grid"], 128u);
    EXPECT_EQ (offsets["depth"], 144u);

    std::vector<uint32_t> storageBindings;
    for (const auto& buffer : reflection.storageBuffers)
    {
        EXPECT_EQ (buffer.set, 0u);
        storageBindings.push_back (buffer.binding);
    }

    std::sort (storageBindings.begin(), storageBindings.end());
    EXPECT_EQ (storageBindings, (std::vector<uint32_t> { 1, 2 }));
}
