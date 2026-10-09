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
ShadowAtlas::ShadowAtlas (int atlasSizeIn, int minTileSizeIn, int maxTileSizeIn)
    : atlasSize (atlasSizeIn)
    , minTileSize (minTileSizeIn)
    , maxTileSize (jmin (maxTileSizeIn, atlasSizeIn))
{
    jassert (isPowerOfTwo (atlasSize) && isPowerOfTwo (minTileSize) && isPowerOfTwo (maxTileSize));
    jassert (minTileSize <= maxTileSize);

    minTileSize = jlimit (1, maxTileSize, minTileSize);
    freeNodes.resize (static_cast<size_t> (getLevel (minTileSize) + 1));
}

ShadowAtlas::~ShadowAtlas() = default;

//==============================================================================
void ShadowAtlas::allocate (Span<const Request> requests)
{
    allocations.clear();
    tiles.clear();

    for (auto& level : freeNodes)
        level.clear();

    freeNodes.front().push_back ({ 0, 0, atlasSize });

    std::vector<Request> ordered (requests.begin(), requests.end());
    for (auto& request : ordered)
    {
        jassert (request.faces > 0 && isPowerOfTwo (request.desiredSize));
        request.desiredSize = jlimit (minTileSize, maxTileSize, nextPowerOfTwo (jmin (request.desiredSize, maxTileSize)));
    }

    std::sort (ordered.begin(), ordered.end(), [] (const Request& a, const Request& b)
    {
        return a.desiredSize != b.desiredSize ? a.desiredSize > b.desiredSize : a.lightIndex < b.lightIndex;
    });

    for (const auto& request : ordered)
    {
        if (request.faces <= 0)
            continue;

        for (auto size = request.desiredSize; size >= minTileSize; size /= 2)
        {
            if (getCapacity (size) < request.faces)
                continue;

            allocations.push_back ({ request.lightIndex, static_cast<int> (tiles.size()), request.faces, size });

            for (int face = 0; face < request.faces; ++face)
                tiles.push_back (takeNode (getLevel (size)));

            break;
        }
    }
}

Span<const ShadowAtlas::Allocation> ShadowAtlas::getAllocations() const noexcept
{
    return allocations;
}

Span<const ShadowAtlas::Tile> ShadowAtlas::getTiles() const noexcept
{
    return tiles;
}

int ShadowAtlas::getAtlasSize() const noexcept
{
    return atlasSize;
}

const ShadowAtlas::Allocation* ShadowAtlas::findAllocation (int lightIndex) const noexcept
{
    const auto found = std::find_if (allocations.begin(), allocations.end(), [lightIndex] (const Allocation& allocation)
    {
        return allocation.lightIndex == lightIndex;
    });

    return found != allocations.end() ? &*found : nullptr;
}

//==============================================================================
std::optional<ShadowAtlas::Request> ShadowAtlas::makeRequest (const LightNode& light, int lightIndex, float distanceToCamera, float yFov, int viewportHeight) const noexcept
{
    if (light.type == LightNode::Type::directional || ! light.castsShadows)
        return std::nullopt;

    const auto range = GpuSceneData::getEffectiveRange (light);
    if (! (range > 0.0f))
        return std::nullopt;

    const auto wideSpot = 2.0f * light.outerConeAngle + getGuardAngle (minTileSize) > degreesToRadians (120.0f);

    Request request;
    request.lightIndex = lightIndex;
    request.faces = (light.type == LightNode::Type::point || wideSpot) ? 6 : 1;
    request.desiredSize = getDesiredTileSize (getProjectedRadius (range, distanceToCamera, yFov, viewportHeight), minTileSize, maxTileSize);
    return request;
}

//==============================================================================
int ShadowAtlas::getDesiredTileSize (float projectedRadiusPx, int minTile, int maxTile) noexcept
{
    if (! (projectedRadiusPx > 0.0f))
        return minTile;

    const auto pixels = jmin (std::ceil (2.0f * projectedRadiusPx), static_cast<float> (maxTile));
    return jlimit (minTile, maxTile, nextPowerOfTwo (static_cast<int> (pixels)));
}

float ShadowAtlas::getProjectedRadius (float sphereRadius, float distanceToCamera, float yFov, int viewportHeight) noexcept
{
    if (! (yFov > 0.0f) || viewportHeight <= 0)
        return 0.0f;

    if (distanceToCamera <= sphereRadius)
        return 1.0e9f;

    return sphereRadius / (distanceToCamera * std::tan (yFov * 0.5f)) * static_cast<float> (viewportHeight) * 0.5f;
}

float ShadowAtlas::getGuardAngle (int tileSize) noexcept
{
    return 2.0f * std::atan (1.0f + 4.0f / static_cast<float> (tileSize)) - MathConstants<float>::halfPi;
}

//==============================================================================
int ShadowAtlas::getLevel (int size) const noexcept
{
    int level = 0;
    for (auto nodeSize = atlasSize; nodeSize > size; nodeSize /= 2)
        ++level;

    return level;
}

int ShadowAtlas::getCapacity (int size) const noexcept
{
    int capacity = 0;
    for (int level = 0, nodeSize = atlasSize; nodeSize >= size; ++level, nodeSize /= 2)
        capacity += static_cast<int> (freeNodes[static_cast<size_t> (level)].size()) * (nodeSize / size) * (nodeSize / size);

    return capacity;
}

ShadowAtlas::Tile ShadowAtlas::takeNode (int level)
{
    auto& nodes = freeNodes[static_cast<size_t> (level)];
    if (! nodes.empty())
    {
        const auto node = nodes.back();
        nodes.pop_back();
        return node;
    }

    // Split the smallest larger free node: the children go in reverse Morton order, so the
    // top-left one is taken first
    jassert (level > 0);
    const auto parent = takeNode (level - 1);
    const auto half = parent.size / 2;

    nodes.push_back ({ parent.x + half, parent.y + half, half });
    nodes.push_back ({ parent.x, parent.y + half, half });
    nodes.push_back ({ parent.x + half, parent.y, half });
    return { parent.x, parent.y, half };
}

} // namespace yup
