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

#include <yup_3d/yup_3d.h>

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
        viewData.update (scene, 64, 64, false, 1, 0);
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
#include <yup_3d/shaders/yup_LightClusters.inc>
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

#if YUP_APPLE
class LightClustersMetalTests : public LightClustersTests
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";

        if (RenderCapabilities::selectTier (RenderCapabilities::queryDevice (*device), RenderSettings::TierPreference::full) != RenderCapabilities::Tier::full)
            GTEST_SKIP() << "The device cannot run the full tier";
    }

    /** Uploads the lights through the frame resources, culls them on the GPU and reads back every cluster slot. */
    std::vector<uint32> cullOnGpu (Span<const GpuLight> lights, int firstLocalLight, const Matrix4& cullView, const Matrix4& cullInverseProjection)
    {
        FrameResources resources (device);
        resources.beginFrame (1600, 900);

        const auto lightBytes = lights.size() * sizeof (GpuLight);
        auto lightBuffer = resources.acquireStorageBuffer (FrameResources::StorageSlot::lights, lightBytes);
        auto clusterBuffer = resources.acquireStorageBuffer (FrameResources::StorageSlot::clusterLights, LightClusters::getClusterBufferSize (grid));
        if (lightBuffer == nullptr || clusterBuffer == nullptr || ! device->updateBuffer (lightBuffer, lights.data(), lightBytes))
            return {};

        // Poisoned, so a slot the pass never writes shows up as an overflowing count
        std::vector<uint32> result (clusterBuffer->getSizeInBytes() / sizeof (uint32), 0xffffffffu);
        if (! device->updateBuffer (clusterBuffer, result.data(), clusterBuffer->getSizeInBytes()))
            return {};

        LightClusters clusters;
        auto frame = GpuFrame::begin (device);
        const auto dispatched = clusters.dispatch (frame, device, grid, lightBuffer, static_cast<int> (lights.size()), firstLocalLight, cullView, cullInverseProjection, clusterBuffer);
        EXPECT_TRUE (dispatched);
        EXPECT_TRUE (frame.submit());
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        const auto read = device->readBuffer (clusterBuffer, result.data(), result.size() * sizeof (uint32));
        resources.endFrame();
        return dispatched && read ? result : std::vector<uint32> {};
    }

    /** True when the light's verdict on the cluster flips within 1e-4 of its range (preflight R17). */
    bool isAmbiguous (const GpuLight& light, const Matrix4& cullView, const BoundingBox& bounds) const
    {
        auto wider = light;
        auto narrower = light;
        wider.directionRange[3] *= 1.0f + 1.0e-4f;
        narrower.directionRange[3] *= 1.0f - 1.0e-4f;
        return LightClusters::intersects (wider, cullView, bounds) != LightClusters::intersects (narrower, cullView, bounds);
    }

    GpuDevice::Ptr device;
};

TEST_F (LightClustersMetalTests, ClusterCullingMatchesCpuReference)
{
    // The camera of a TAA frame: the jittered projection is the one rasterized
    Scene scene;
    ViewData viewData;
    viewData.update (scene, 1600, 900, true, 8, 3);
    const auto& cullView = viewData.getView();
    const auto cullInverseProjection = viewData.getJitteredProjection().inverted();

    // Point lights with a few spots, a sun and dark lights, in a 10 unit box in front of the camera,
    // packed as the renderer packs them: the sun first, node ranges positive, effective ranges negative
    Random random (1234);
    const auto between = [&random] (float low, float high)
    {
        return low + (high - low) * random.nextFloat();
    };

    std::vector<std::unique_ptr<LightNode>> nodes;
    std::vector<SceneDrawList::Light> sceneLights;

    for (int i = 0; i < 200; ++i)
    {
        auto& node = *nodes.emplace_back (std::make_unique<LightNode> (i % 10 == 9 ? LightNode::Type::spot : LightNode::Type::point));
        node.color = { between (0.2f, 1.0f), between (0.2f, 1.0f), between (0.2f, 1.0f) };

        const auto range = between (0.2f, 2.0f);
        if (i % 2 == 0)
            node.range = range;
        else
            node.intensity = range * range * GpuSceneData::lightCutoff / jmax (node.color[0], node.color[1], node.color[2]);

        node.outerConeAngle = between (0.2f, 1.2f);
        node.innerConeAngle = node.outerConeAngle * between (0.0f, 0.9f);

        const Vector3<float> position (between (-5.0f, 5.0f), between (-5.0f, 5.0f), between (-10.0f, 0.0f));
        const Vector3<float> direction (between (-1.0f, 1.0f), between (-1.0f, 1.0f), between (-1.0f, 1.0f));
        sceneLights.push_back ({ &node, position, direction.normalized() });
    }

    auto& sun = *nodes.emplace_back (std::make_unique<LightNode> (LightNode::Type::directional));
    sceneLights.insert (sceneLights.begin() + 50, { &sun, {}, Vector3<float> (0.0f, -1.0f, 0.0f) });

    auto& dark = *nodes.emplace_back (std::make_unique<LightNode> (LightNode::Type::point));
    dark.intensity = 0.0f;
    sceneLights.insert (sceneLights.begin() + 100, { &dark, { 0.0f, 0.0f, -5.0f }, Vector3<float> (0.0f, 0.0f, -1.0f) });

    // Dark despite a node range: culled by the CPU and the GPU alike
    auto& rangedDark = *nodes.emplace_back (std::make_unique<LightNode> (LightNode::Type::point));
    rangedDark.range = 2.0f;
    rangedDark.intensity = 0.0f;
    sceneLights.insert (sceneLights.begin() + 120, { &rangedDark, { 0.5f, 0.0f, -4.0f }, Vector3<float> (0.0f, 0.0f, -1.0f) });

    auto& rangedBlack = *nodes.emplace_back (std::make_unique<LightNode> (LightNode::Type::spot));
    rangedBlack.range = 2.0f;
    rangedBlack.color = { 0.0f, 0.0f, 0.0f };
    sceneLights.insert (sceneLights.begin() + 140, { &rangedBlack, { -0.5f, 0.0f, -4.0f }, Vector3<float> (0.0f, 0.0f, -1.0f) });

    std::vector<uint32> order;
    const auto lights = GpuSceneData::packLights (sceneLights, {}, order);
    const auto firstLocalLight = 1;
    ASSERT_EQ (lights[0].positionType[3], 0.0f);
    ASSERT_EQ (std::count_if (lights.begin(), lights.end(), [] (const GpuLight& light)
                              {
        return light.directionRange[3] == 0.0f;
               }),
               4); // the sun and the three dark lights

    const auto expected = LightClusters::assign (grid, lights, firstLocalLight, cullView, cullInverseProjection);
    const auto gpu = cullOnGpu (lights, firstLocalLight, cullView, cullInverseProjection);
    ASSERT_EQ (gpu.size() * sizeof (uint32) >= LightClusters::getClusterBufferSize (grid), true);

    // Truncation would let one ambiguous pair shift which lights a cluster keeps
    size_t assigned = 0;
    for (const auto& list : expected)
    {
        ASSERT_LT (list.size(), static_cast<size_t> (LightClusters::maxLightsPerCluster));
        assigned += list.size();
    }

    EXPECT_GT (assigned, lights.size()); // not vacuous: lights reach several clusters

    // The cap counts against the pairs that can differ at all: listed on the CPU, or ambiguous
    int candidatePairs = 0;
    int ambiguousMismatches = 0;

    for (int slice = 0; slice < grid.slices; ++slice)
    {
        for (int y = 0; y < grid.tilesY; ++y)
        {
            for (int x = 0; x < grid.tilesX; ++x)
            {
                const auto cluster = static_cast<size_t> (x + grid.tilesX * (y + grid.tilesY * slice));
                const auto count = gpu[cluster * 64];
                ASSERT_LE (count, static_cast<uint32> (LightClusters::maxLightsPerCluster)) << "cluster " << cluster;

                const std::vector<uint32> list (gpu.begin() + static_cast<std::ptrdiff_t> (cluster * 64 + 1),
                                                gpu.begin() + static_cast<std::ptrdiff_t> (cluster * 64 + 1 + count));
                EXPECT_TRUE (std::adjacent_find (list.begin(), list.end(), std::greater_equal<uint32>()) == list.end()) << "cluster " << cluster;

                const auto bounds = LightClusters::getClusterBounds (grid, x, y, slice, cullInverseProjection);
                const auto& reference = expected[cluster];

                for (auto index = static_cast<uint32> (firstLocalLight); index < lights.size(); ++index)
                {
                    const auto onCpu = std::binary_search (reference.begin(), reference.end(), index);
                    const auto onGpu = std::binary_search (list.begin(), list.end(), index);
                    const auto ambiguous = isAmbiguous (lights[index], cullView, bounds);
                    candidatePairs += onCpu || ambiguous ? 1 : 0;

                    if (onCpu == onGpu)
                        continue;

                    if (ambiguous)
                        ++ambiguousMismatches;
                    else
                        ADD_FAILURE() << "cluster " << cluster << " light " << index << (onCpu ? " missing on the GPU" : " extra on the GPU");
                }
            }
        }
    }

    EXPECT_GT (candidatePairs, 0);
    EXPECT_LE (ambiguousMismatches * 100, candidatePairs);
}

TEST_F (LightClustersMetalTests, InfiniteFarPlaneOpensTheLastSliceOnTheGpu)
{
    grid.infiniteFar = true;
    const GpuLight lights[] = { pointAt ({ 0.0f, 0.0f, -1500.0f }, 5.0f), pointAt ({ 0.0f, 0.0f, -5.0f }, 0.5f) };

    const auto expected = LightClusters::assign (grid, lights, 0, view, inverseProjection);
    const auto gpu = cullOnGpu (lights, 0, view, inverseProjection);
    ASSERT_EQ (gpu.size() * sizeof (uint32) >= LightClusters::getClusterBufferSize (grid), true);

    for (size_t cluster = 0; cluster < expected.size(); ++cluster)
    {
        const std::vector<uint32> list (gpu.begin() + static_cast<std::ptrdiff_t> (cluster * 64 + 1),
                                        gpu.begin() + static_cast<std::ptrdiff_t> (cluster * 64 + 1 + jmin (gpu[cluster * 64], 63u)));
        EXPECT_EQ (list, expected[cluster]) << "cluster " << cluster;
    }

    const auto lastSlice = static_cast<size_t> (LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 1500.0f));
    EXPECT_EQ (gpu[lastSlice * 64], 1u);
    EXPECT_EQ (gpu[lastSlice * 64 + 1], 0u);
}

TEST_F (LightClustersMetalTests, ClusterOverflowKeepsLowestIndicesOnTheGpu)
{
    std::vector<GpuLight> lights;
    for (int i = 0; i < 100; ++i)
        lights.push_back (pointAt ({ 0.0f, 0.0f, -5.0f }, 1.0f));

    const auto gpu = cullOnGpu (lights, 0, view, inverseProjection);
    ASSERT_EQ (gpu.size() * sizeof (uint32) >= LightClusters::getClusterBufferSize (grid), true);

    const auto center = static_cast<size_t> (LightClusters::getClusterIndex (grid, { 800.0f, 450.0f }, { 1600.0f, 900.0f }, 5.0f));
    ASSERT_EQ (gpu[center * 64], static_cast<uint32> (LightClusters::maxLightsPerCluster));

    for (uint32 i = 0; i < static_cast<uint32> (LightClusters::maxLightsPerCluster); ++i)
        EXPECT_EQ (gpu[center * 64 + 1 + i], i);
}
#endif
