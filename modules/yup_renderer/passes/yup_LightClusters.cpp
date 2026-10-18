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

namespace yup
{

//==============================================================================
namespace
{

// Shader bundles built with the module, see shaderBundles in yup_renderer.h
constexpr uint8_t lightClustersShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightClusters.inc"
};

// std140 mirror of ClusterBlock in yup_LightClusters.comp
struct LightClustersBlock
{
    float view[16];
    float inverseProjection[16];
    float grid[4];  // tilesX, tilesY, slices, number of lights
    float depth[4]; // near, far, firstLocalLight, 1 when the last slice is open (infinite far plane)
};

static_assert (sizeof (LightClustersBlock) == 160);

} // namespace

//==============================================================================
LightClusters::LightClusters() = default;

LightClusters::~LightClusters() = default;

//==============================================================================
LightClusters::Grid LightClusters::getGrid (const ViewData& view) noexcept
{
    const auto& clipPlanes = view.getBlock().clipPlanes;

    Grid grid;
    grid.nearPlane = jmax (clipPlanes[0], 1.0e-4f);
    grid.infiniteFar = clipPlanes[1] <= 0.0f;
    grid.farPlane = grid.infiniteFar ? ViewData::infiniteFarDistance : clipPlanes[1];
    return grid;
}

int LightClusters::getSliceForDepth (const Grid& grid, float viewDepth) noexcept
{
    if (! (viewDepth > grid.nearPlane))
        return 0;

    const auto slice = std::floor (std::log (viewDepth / grid.nearPlane) * static_cast<float> (grid.slices) / std::log (grid.farPlane / grid.nearPlane));
    return static_cast<int> (jmin (slice, static_cast<float> (grid.slices - 1)));
}

float LightClusters::getDepthForSlice (const Grid& grid, int slice) noexcept
{
    return grid.nearPlane * std::pow (grid.farPlane / grid.nearPlane, static_cast<float> (slice) / static_cast<float> (grid.slices));
}

int LightClusters::getClusterIndex (const Grid& grid, Point<float> pixel, Point<float> viewportSize, float viewDepth) noexcept
{
    const auto tile = [] (float position, float size, int tiles)
    {
        return static_cast<int> (jlimit (0.0f, static_cast<float> (tiles - 1), std::floor (position * static_cast<float> (tiles) / size)));
    };

    const auto x = tile (pixel.getX(), viewportSize.getX(), grid.tilesX);
    const auto y = tile (pixel.getY(), viewportSize.getY(), grid.tilesY);
    return x + grid.tilesX * (y + grid.tilesY * getSliceForDepth (grid, viewDepth));
}

BoundingBox LightClusters::getClusterBounds (const Grid& grid, int x, int y, int slice, const Matrix4& inverseProjection) noexcept
{
    const float depths[] = { getDepthForSlice (grid, slice), getDepthForSlice (grid, slice + 1) };
    const auto openFar = grid.infiniteFar && slice == grid.slices - 1;

    BoundingBox bounds;
    bool openMin[3] = {}, openMax[3] = {};

    for (int corner = 0; corner < 4; ++corner)
    {
        // Tile row 0 is the top of the screen, NDC y = +1
        const auto ndcX = -1.0f + 2.0f * static_cast<float> (x + (corner & 1)) / static_cast<float> (grid.tilesX);
        const auto ndcY = 1.0f - 2.0f * static_cast<float> (y + (corner >> 1)) / static_cast<float> (grid.tilesY);

        // Two finite points of the corner ray: NDC depth 1 is at infinity with an infinite far plane
        const auto a = inverseProjection.transformPoint ({ ndcX, ndcY, 0.0f });
        const auto b = inverseProjection.transformPoint ({ ndcX, ndcY, 0.5f });

        const auto rayAt = [&a, &b] (float depth)
        {
            return a + (b - a) * ((-depth - a.getZ()) / (b.getZ() - a.getZ()));
        };

        bounds.expand (rayAt (depths[0]));

        if (! openFar)
        {
            bounds.expand (rayAt (depths[1]));
            continue;
        }

        // The ray runs to infinity: open the bounds on each axis it moves along, never scaling by infinity
        const auto direction = (b - a) / (a.getZ() - b.getZ());
        const float components[] = { direction.getX(), direction.getY(), direction.getZ() };

        for (int axis = 0; axis < 3; ++axis)
        {
            openMin[axis] = openMin[axis] || components[axis] < 0.0f;
            openMax[axis] = openMax[axis] || components[axis] > 0.0f;
        }
    }

    if (! openFar)
        return bounds;

    constexpr auto open = std::numeric_limits<float>::max();
    const auto& low = bounds.getMin();
    const auto& high = bounds.getMax();

    return { { openMin[0] ? -open : low.getX(), openMin[1] ? -open : low.getY(), openMin[2] ? -open : low.getZ() },
             { openMax[0] ? open : high.getX(), openMax[1] ? open : high.getY(), openMax[2] ? open : high.getZ() } };
}

bool LightClusters::intersects (const GpuLight& light, const Matrix4& view, const BoundingBox& viewBounds) noexcept
{
    if (light.positionType[3] < 0.5f)
        return false;

    // The packed cone offset is -cosOuter * scale
    const auto isSpot = light.positionType[3] > 1.5f;
    const auto cosOuter = isSpot ? -light.spotRadiusShadow[1] / light.spotRadiusShadow[0] : 0.0f;

    return GpuSceneData::lightReachesBox (viewBounds,
                                          view.transformPoint ({ light.positionType[0], light.positionType[1], light.positionType[2] }),
                                          view.transformVector ({ light.directionRange[0], light.directionRange[1], light.directionRange[2] }),
                                          std::abs (light.directionRange[3]),
                                          isSpot,
                                          cosOuter);
}

std::vector<std::vector<uint32>> LightClusters::assign (const Grid& grid, Span<const GpuLight> lights, int firstLocalLight, const Matrix4& view, const Matrix4& inverseProjection)
{
    std::vector<std::vector<uint32>> clusters (static_cast<size_t> (grid.getNumClusters()));

    for (int slice = 0; slice < grid.slices; ++slice)
    {
        for (int y = 0; y < grid.tilesY; ++y)
        {
            for (int x = 0; x < grid.tilesX; ++x)
            {
                const auto bounds = getClusterBounds (grid, x, y, slice, inverseProjection);
                auto& list = clusters[static_cast<size_t> (x + grid.tilesX * (y + grid.tilesY * slice))];

                for (auto index = static_cast<size_t> (jmax (0, firstLocalLight)); index < lights.size() && list.size() < static_cast<size_t> (maxLightsPerCluster); ++index)
                {
                    if (intersects (lights[index], view, bounds))
                        list.push_back (static_cast<uint32> (index));
                }
            }
        }
    }

    return clusters;
}

void LightClusters::packGrid (const Grid& grid, int numLights, int firstLocalLight, float (&gridOut)[4], float (&depthOut)[4]) noexcept
{
    gridOut[0] = static_cast<float> (grid.tilesX);
    gridOut[1] = static_cast<float> (grid.tilesY);
    gridOut[2] = static_cast<float> (grid.slices);
    gridOut[3] = static_cast<float> (numLights);
    depthOut[0] = grid.nearPlane;
    depthOut[1] = grid.farPlane;
    depthOut[2] = static_cast<float> (firstLocalLight);
    depthOut[3] = grid.infiniteFar ? 1.0f : 0.0f;
}

//==============================================================================
Result LightClusters::prepare (const GpuDevice::Ptr& device)
{
    if (preparation)
        return *preparation;

    auto bundle = ShaderBundle::loadFromData (lightClustersShaderBundle, sizeof (lightClustersShaderBundle));
    if (bundle.failed())
        return *(preparation = Result::fail ("Unable to load the light clusters shader bundle: " + bundle.getErrorMessage()));

    auto compiled = GpuComputePipeline::compileFromBundle (device, bundle.getReference());
    if (compiled.failed())
        return *(preparation = Result::fail ("Unable to compile the light clusters pipeline: " + compiled.getErrorMessage()));

    pipeline = compiled.getValue();
    return *(preparation = Result::ok());
}

bool LightClusters::dispatch (GpuFrame& frame, const GpuDevice::Ptr& device, const Grid& grid, GpuBuffer::Ptr lights, int numLights, int firstLocalLight, const Matrix4& view, const Matrix4& inverseProjection, GpuBuffer::Ptr clusterLights)
{
    if (lights == nullptr || clusterLights == nullptr || prepare (device).failed())
        return false;

    if (clusterLights->getSizeInBytes() < getClusterBufferSize (grid)
        || static_cast<size_t> (jmax (0, numLights)) > lights->getSizeInBytes() / sizeof (GpuLight))
    {
        jassertfalse; // The buffers are too small for the grid or the number of lights
        return false;
    }

    LightClustersBlock block {};
    std::copy_n (view.getData(), 16, block.view);
    std::copy_n (inverseProjection.getData(), 16, block.inverseProjection);
    packGrid (grid, numLights, firstLocalLight, block.grid, block.depth);

    auto pass = GpuComputePass::begin (frame);
    if (! pass.isValid())
        return false;

    pass.setPipeline (pipeline);
    pass.setUniformBuffer (0, 0, &block, sizeof (block));
    pass.setStorageBuffer (0, 1, std::move (lights));
    pass.setStorageBuffer (0, 2, std::move (clusterLights));

    return pass.dispatch (static_cast<uint32_t> ((grid.getNumClusters() + 63) / 64), 1, 1);
}

} // namespace yup
