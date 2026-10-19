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

#include <yup_renderer/yup_renderer.h>

using namespace yup;

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
    viewData.update (scene, SceneSettings::resolve (scene), 1600, 900, true, 8, 3);
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
