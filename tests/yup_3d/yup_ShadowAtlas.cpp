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

class ShadowAtlasTests : public ::testing::Test
{
protected:
    static bool overlaps (const ShadowAtlas::Tile& a, const ShadowAtlas::Tile& b)
    {
        return a.x < b.x + b.size && b.x < a.x + a.size && a.y < b.y + b.size && b.y < a.y + a.size;
    }

    static void expectValidLayout (const ShadowAtlas& atlas)
    {
        const auto tiles = atlas.getTiles();
        const auto size = atlas.getAtlasSize();

        for (size_t i = 0; i < tiles.size(); ++i)
        {
            EXPECT_GE (tiles[i].x, 0);
            EXPECT_GE (tiles[i].y, 0);
            EXPECT_LE (tiles[i].x + tiles[i].size, size);
            EXPECT_LE (tiles[i].y + tiles[i].size, size);

            for (size_t j = i + 1; j < tiles.size(); ++j)
                EXPECT_FALSE (overlaps (tiles[i], tiles[j])) << i << " vs " << j;
        }
    }

    static constexpr float yFov = 0.8f;
    static constexpr int viewportHeight = 1080;

    static void makeShadowed (LightNode& light, float outerConeDegrees = 45.0f)
    {
        light.range = 5.0f;
        light.outerConeAngle = degreesToRadians (outerConeDegrees);
        light.castsShadows = true;
    }

    static std::optional<ShadowAtlas::Request> requestFor (LightNode::Type type, float outerConeDegrees, float distance)
    {
        const ShadowAtlas atlas (4096, 64, 1024);
        LightNode light (type);
        makeShadowed (light, outerConeDegrees);
        return atlas.makeRequest (light, 0, distance, yFov, viewportHeight);
    }
};

TEST_F (ShadowAtlasTests, DesiredTileSizeIsAClampedPowerOfTwo)
{
    EXPECT_EQ (ShadowAtlas::getDesiredTileSize (10.0f, 64, 1024), 64);
    EXPECT_EQ (ShadowAtlas::getDesiredTileSize (100.0f, 64, 1024), 256);
    EXPECT_EQ (ShadowAtlas::getDesiredTileSize (129.0f, 64, 1024), 512);
    EXPECT_EQ (ShadowAtlas::getDesiredTileSize (5000.0f, 64, 1024), 1024);
}

TEST_F (ShadowAtlasTests, DesiredTileSizeHandlesDegenerateRadii)
{
    EXPECT_EQ (ShadowAtlas::getDesiredTileSize (0.0f, 64, 1024), 64);
    EXPECT_EQ (ShadowAtlas::getDesiredTileSize (-3.0f, 64, 1024), 64);
    EXPECT_EQ (ShadowAtlas::getDesiredTileSize (std::numeric_limits<float>::quiet_NaN(), 64, 1024), 64);
    EXPECT_EQ (ShadowAtlas::getDesiredTileSize (1.0e9f, 64, 1024), 1024);
    EXPECT_EQ (ShadowAtlas::getDesiredTileSize (std::numeric_limits<float>::infinity(), 64, 1024), 1024);
}

TEST_F (ShadowAtlasTests, ProjectedRadiusShrinksWithDistance)
{
    const auto nearRadius = ShadowAtlas::getProjectedRadius (1.0f, 2.0f, 0.8f, 1080);
    const auto farRadius = ShadowAtlas::getProjectedRadius (1.0f, 20.0f, 0.8f, 1080);
    EXPECT_GT (nearRadius, farRadius);
    EXPECT_NEAR (nearRadius / farRadius, 10.0f, 0.5f);
    EXPECT_GT (ShadowAtlas::getProjectedRadius (5.0f, 1.0f, 0.8f, 1080), 1.0e6f); // inside the sphere
}

TEST_F (ShadowAtlasTests, SpotGetsOneTileAndPointGetsSixOfTheSameSize)
{
    ShadowAtlas atlas (4096, 64, 1024);
    const ShadowAtlas::Request requests[] = { { 0, 1, 512 }, { 1, 6, 256 } };
    atlas.allocate (requests);

    const auto* spot = atlas.findAllocation (0);
    const auto* point = atlas.findAllocation (1);
    ASSERT_NE (spot, nullptr);
    ASSERT_NE (point, nullptr);

    EXPECT_EQ (spot->faces, 1);
    EXPECT_EQ (spot->size, 512);
    EXPECT_EQ (point->faces, 6);
    EXPECT_EQ (point->size, 256);

    for (int f = 0; f < 6; ++f)
        EXPECT_EQ (atlas.getTiles()[(size_t) (point->firstTile + f)].size, 256);
}

TEST_F (ShadowAtlasTests, TilesNeverOverlapAndStayInside)
{
    ShadowAtlas atlas (2048, 64, 512);

    std::vector<ShadowAtlas::Request> requests;
    for (int i = 0; i < 40; ++i)
        requests.push_back ({ i, (i % 3 == 0) ? 6 : 1, 64 << (i % 4) });

    atlas.allocate (requests);
    EXPECT_FALSE (atlas.getTiles().empty());
    expectValidLayout (atlas);
}

TEST_F (ShadowAtlasTests, TilesAreAlignedToTheirSize)
{
    ShadowAtlas atlas (2048, 64, 512);

    std::vector<ShadowAtlas::Request> requests;
    for (int i = 0; i < 40; ++i)
        requests.push_back ({ i, (i % 3 == 0) ? 6 : 1, 64 << (i % 4) });

    atlas.allocate (requests);
    ASSERT_FALSE (atlas.getTiles().empty());

    for (const auto& tile : atlas.getTiles())
    {
        EXPECT_EQ (tile.x % tile.size, 0) << tile.x << " " << tile.size;
        EXPECT_EQ (tile.y % tile.size, 0) << tile.y << " " << tile.size;
    }
}

TEST_F (ShadowAtlasTests, RepeatedAllocationsReuseTheWholeAtlas)
{
    ShadowAtlas atlas (1024, 64, 1024);

    for (int round = 0; round < 12; ++round)
    {
        std::vector<ShadowAtlas::Request> requests;
        for (int i = 0; i < 3 + round % 5; ++i)
            requests.push_back ({ i, ((i + round) % 3 == 0) ? 6 : 1, 64 << ((i * 7 + round) % 4) });

        atlas.allocate (requests);
        expectValidLayout (atlas);

        // Whatever the previous round left, the whole atlas is free again
        const ShadowAtlas::Request whole[] = { { 99, 1, 1024 } };
        atlas.allocate (whole);
        ASSERT_NE (atlas.findAllocation (99), nullptr) << round;
        EXPECT_EQ (atlas.findAllocation (99)->size, 1024) << round;
    }

    // Sixteen 256 tiles fill it exactly after the churn
    std::vector<ShadowAtlas::Request> exact;
    for (int i = 0; i < 16; ++i)
        exact.push_back ({ i, 1, 256 });

    atlas.allocate (exact);
    ASSERT_EQ (atlas.getAllocations().size(), 16u);
    for (const auto& allocation : atlas.getAllocations())
        EXPECT_EQ (allocation.size, 256);
    expectValidLayout (atlas);
}

TEST_F (ShadowAtlasTests, OverflowHalvesThenDrops)
{
    ShadowAtlas atlas (1024, 128, 1024);

    // Four 512 tiles fill the atlas exactly: light 4 finds no room down to the 128 minimum and
    // drops, and so does the 6-face point light
    const ShadowAtlas::Request requests[] = { { 0, 1, 512 }, { 1, 1, 512 }, { 2, 1, 512 }, { 3, 1, 512 }, { 4, 1, 512 }, { 5, 6, 512 } };
    atlas.allocate (requests);

    ASSERT_NE (atlas.findAllocation (3), nullptr);
    EXPECT_EQ (atlas.findAllocation (3)->size, 512);
    EXPECT_EQ (atlas.findAllocation (4), nullptr);
    EXPECT_EQ (atlas.findAllocation (5), nullptr);

    // With one 512 quadrant left, two point lights halve to 128 (two 256 nodes left after the
    // first, ten 128 slots) and the third finds only four 128 slots, so it drops
    const ShadowAtlas::Request points[] = { { 0, 1, 512 }, { 1, 1, 512 }, { 2, 1, 512 }, { 3, 6, 512 }, { 4, 6, 512 }, { 5, 6, 512 } };
    atlas.allocate (points);

    ASSERT_NE (atlas.findAllocation (3), nullptr);
    ASSERT_NE (atlas.findAllocation (4), nullptr);
    EXPECT_EQ (atlas.findAllocation (3)->size, 128);
    EXPECT_EQ (atlas.findAllocation (4)->size, 128);
    EXPECT_EQ (atlas.findAllocation (5), nullptr);
    expectValidLayout (atlas);
}

TEST_F (ShadowAtlasTests, HalvesWhenTheDesiredSizeNoLongerFits)
{
    ShadowAtlas atlas (1024, 64, 1024);
    const ShadowAtlas::Request requests[] = { { 0, 1, 512 }, { 1, 1, 512 }, { 2, 1, 512 }, { 3, 6, 512 } };
    atlas.allocate (requests);

    // One 512 quadrant is left: six faces fit at 128 (6 * 128^2 <= 512^2), not at 256
    const auto* point = atlas.findAllocation (3);
    ASSERT_NE (point, nullptr);
    EXPECT_EQ (point->size, 128);
}

TEST_F (ShadowAtlasTests, LargerRequestsAreServedFirstThenByLightIndex)
{
    ShadowAtlas atlas (512, 64, 512);
    const ShadowAtlas::Request requests[] = { { 7, 1, 256 }, { 2, 1, 512 } };
    atlas.allocate (requests);

    // The 512 request takes the whole atlas, the 256 one drops even though it came first
    ASSERT_NE (atlas.findAllocation (2), nullptr);
    EXPECT_EQ (atlas.findAllocation (7), nullptr);
}

TEST_F (ShadowAtlasTests, EqualSizesAreServedByLightIndex)
{
    ShadowAtlas atlas (512, 64, 512);

    // Both resolve to 512 once clamped: the lower light index wins, whatever was asked for
    const ShadowAtlas::Request requests[] = { { 5, 1, 1024 }, { 3, 1, 512 } };
    atlas.allocate (requests);

    ASSERT_NE (atlas.findAllocation (3), nullptr);
    EXPECT_EQ (atlas.findAllocation (3)->size, 512);
    EXPECT_EQ (atlas.findAllocation (5), nullptr);
}

TEST_F (ShadowAtlasTests, DesiredSizesAreClampedToTheTileRange)
{
    ShadowAtlas atlas (1024, 64, 256);
    const ShadowAtlas::Request requests[] = { { 0, 1, 1024 }, { 1, 1, 16 } };
    atlas.allocate (requests);

    ASSERT_NE (atlas.findAllocation (0), nullptr);
    ASSERT_NE (atlas.findAllocation (1), nullptr);
    EXPECT_EQ (atlas.findAllocation (0)->size, 256);
    EXPECT_EQ (atlas.findAllocation (1)->size, 64);
}

TEST_F (ShadowAtlasTests, AllocationIsDeterministic)
{
    std::vector<ShadowAtlas::Request> requests;
    for (int i = 0; i < 20; ++i)
        requests.push_back ({ i, (i % 2 == 0) ? 6 : 1, 64 << (i % 3) });

    ShadowAtlas a (2048, 64, 512);
    ShadowAtlas b (2048, 64, 512);
    a.allocate (requests);
    b.allocate (requests);

    ASSERT_EQ (a.getTiles().size(), b.getTiles().size());
    for (size_t i = 0; i < a.getTiles().size(); ++i)
    {
        EXPECT_EQ (a.getTiles()[i].x, b.getTiles()[i].x);
        EXPECT_EQ (a.getTiles()[i].y, b.getTiles()[i].y);
    }
}

TEST_F (ShadowAtlasTests, AllocatingTheSameRequestsTwiceGivesTheSameLayout)
{
    std::vector<ShadowAtlas::Request> requests;
    for (int i = 0; i < 20; ++i)
        requests.push_back ({ i, (i % 2 == 0) ? 6 : 1, 64 << (i % 3) });

    ShadowAtlas atlas (2048, 64, 512);
    atlas.allocate (requests);
    const std::vector<ShadowAtlas::Tile> firstTiles (atlas.getTiles().begin(), atlas.getTiles().end());
    const std::vector<ShadowAtlas::Allocation> firstAllocations (atlas.getAllocations().begin(), atlas.getAllocations().end());

    atlas.allocate (requests);

    ASSERT_EQ (atlas.getTiles().size(), firstTiles.size());
    for (size_t i = 0; i < firstTiles.size(); ++i)
    {
        EXPECT_EQ (atlas.getTiles()[i].x, firstTiles[i].x);
        EXPECT_EQ (atlas.getTiles()[i].y, firstTiles[i].y);
        EXPECT_EQ (atlas.getTiles()[i].size, firstTiles[i].size);
    }

    ASSERT_EQ (atlas.getAllocations().size(), firstAllocations.size());
    for (size_t i = 0; i < firstAllocations.size(); ++i)
    {
        EXPECT_EQ (atlas.getAllocations()[i].lightIndex, firstAllocations[i].lightIndex);
        EXPECT_EQ (atlas.getAllocations()[i].firstTile, firstAllocations[i].firstTile);
        EXPECT_EQ (atlas.getAllocations()[i].faces, firstAllocations[i].faces);
        EXPECT_EQ (atlas.getAllocations()[i].size, firstAllocations[i].size);
    }
}

TEST_F (ShadowAtlasTests, ReallocateReplacesPreviousAllocations)
{
    ShadowAtlas atlas (1024, 64, 1024);
    const ShadowAtlas::Request first[] = { { 0, 1, 1024 } };
    atlas.allocate (first);
    const ShadowAtlas::Request second[] = { { 1, 1, 1024 } };
    atlas.allocate (second);

    EXPECT_EQ (atlas.findAllocation (0), nullptr);
    ASSERT_NE (atlas.findAllocation (1), nullptr);
}

TEST_F (ShadowAtlasTests, GuardAngleGrowsAsTilesShrink)
{
    EXPECT_GT (ShadowAtlas::getGuardAngle (64), ShadowAtlas::getGuardAngle (1024));
    EXPECT_NEAR (ShadowAtlas::getGuardAngle (64), 2.0f * std::atan (1.0625f) - MathConstants<float>::halfPi, 1.0e-6f);
    EXPECT_GT (ShadowAtlas::getGuardAngle (1024), 0.0f);
}

TEST_F (ShadowAtlasTests, PointLightRequestsSixFacesSizedByItsEffectiveRange)
{
    const ShadowAtlas atlas (4096, 64, 1024);
    LightNode light (LightNode::Type::point);
    makeShadowed (light);

    const auto request = atlas.makeRequest (light, 3, 50.0f, yFov, viewportHeight);
    ASSERT_TRUE (request.has_value());
    EXPECT_EQ (request->lightIndex, 3);
    EXPECT_EQ (request->faces, 6);

    const auto radius = ShadowAtlas::getProjectedRadius (GpuSceneData::getEffectiveRange (light), 50.0f, yFov, viewportHeight);
    EXPECT_EQ (request->desiredSize, ShadowAtlas::getDesiredTileSize (radius, 64, 1024));
    EXPECT_EQ (request->desiredSize, 256); // 5 m at 50 m: about 128 px of radius
}

TEST_F (ShadowAtlasTests, InfiniteRangeUsesTheIntensityCutoff)
{
    const ShadowAtlas atlas (4096, 64, 1024);
    LightNode light (LightNode::Type::point);
    makeShadowed (light);
    light.range = 0.0f;
    light.intensity = 400.0f;

    const auto request = atlas.makeRequest (light, 0, 200.0f, yFov, viewportHeight);
    ASSERT_TRUE (request.has_value());

    const auto radius = ShadowAtlas::getProjectedRadius (GpuSceneData::getEffectiveRange (light), 200.0f, yFov, viewportHeight);
    EXPECT_EQ (request->desiredSize, ShadowAtlas::getDesiredTileSize (radius, 64, 1024));
}

TEST_F (ShadowAtlasTests, CameraInsideTheInfluenceGetsTheLargestTile)
{
    const auto request = requestFor (LightNode::Type::spot, 45.0f, 1.0f);
    ASSERT_TRUE (request.has_value());
    EXPECT_EQ (request->desiredSize, 1024);
}

TEST_F (ShadowAtlasTests, NarrowSpotRequestsOneFace)
{
    const auto request = requestFor (LightNode::Type::spot, 45.0f, 10.0f);
    ASSERT_TRUE (request.has_value());
    EXPECT_EQ (request->faces, 1);

    // 2 * 58 degrees plus the guard band of a 64 texel tile (about 3.5 degrees) stays below 120
    const auto justBelow = requestFor (LightNode::Type::spot, 58.0f, 10.0f);
    ASSERT_TRUE (justBelow.has_value());
    EXPECT_EQ (justBelow->faces, 1);
}

TEST_F (ShadowAtlasTests, WideSpotIsShadowedAsAPointLight)
{
    for (const auto degrees : { 58.5f, 60.0f, 89.0f })
    {
        const auto request = requestFor (LightNode::Type::spot, degrees, 10.0f);
        ASSERT_TRUE (request.has_value());
        EXPECT_EQ (request->faces, 6) << degrees;
    }
}

TEST_F (ShadowAtlasTests, NoRequestForLightsWithoutAnAtlasShadow)
{
    const ShadowAtlas atlas (4096, 64, 1024);

    LightNode directional (LightNode::Type::directional);
    makeShadowed (directional);
    EXPECT_FALSE (atlas.makeRequest (directional, 0, 10.0f, yFov, viewportHeight).has_value());

    LightNode unshadowed (LightNode::Type::point);
    makeShadowed (unshadowed);
    unshadowed.castsShadows = false;
    EXPECT_FALSE (atlas.makeRequest (unshadowed, 0, 10.0f, yFov, viewportHeight).has_value());

    LightNode dark (LightNode::Type::spot);
    makeShadowed (dark);
    dark.intensity = 0.0f;
    EXPECT_FALSE (atlas.makeRequest (dark, 0, 10.0f, yFov, viewportHeight).has_value());

    LightNode black (LightNode::Type::point);
    makeShadowed (black);
    black.color = { 0.0f, 0.0f, 0.0f };
    EXPECT_FALSE (atlas.makeRequest (black, 0, 10.0f, yFov, viewportHeight).has_value());
}

TEST_F (ShadowAtlasTests, DegenerateCameraInputsGiveTheSmallestTile)
{
    const ShadowAtlas atlas (4096, 64, 1024);
    LightNode light (LightNode::Type::point);
    makeShadowed (light);

    for (const auto fov : { -0.8f, 0.0f })
    {
        const auto request = atlas.makeRequest (light, 0, 10.0f, fov, viewportHeight);
        ASSERT_TRUE (request.has_value());
        EXPECT_EQ (request->desiredSize, 64) << fov;
    }

    const auto noViewport = atlas.makeRequest (light, 0, 10.0f, yFov, 0);
    ASSERT_TRUE (noViewport.has_value());
    EXPECT_EQ (noViewport->desiredSize, 64);

    const auto nanDistance = atlas.makeRequest (light, 0, std::numeric_limits<float>::quiet_NaN(), yFov, viewportHeight);
    ASSERT_TRUE (nanDistance.has_value());
    EXPECT_EQ (nanDistance->desiredSize, 64);
}

TEST_F (ShadowAtlasTests, OrthographicCameraSizesByTheViewHeight)
{
    // 5 m of influence radius in a view 2 * 50 m high spans 54 of 1080 px: no distance makes it nearer
    EXPECT_FLOAT_EQ (ShadowAtlas::getOrthographicProjectedRadius (5.0f, 50.0f, viewportHeight), 54.0f);
    EXPECT_FLOAT_EQ (ShadowAtlas::getOrthographicProjectedRadius (5.0f, 0.0f, viewportHeight), 0.0f);

    const ShadowAtlas atlas (4096, 64, 1024);
    LightNode light (LightNode::Type::point);
    makeShadowed (light);

    for (const auto distance : { 1.0f, 1000.0f })
    {
        const auto wide = atlas.makeRequest (light, 0, distance, yFov, viewportHeight, 50.0f);
        const auto narrow = atlas.makeRequest (light, 0, distance, yFov, viewportHeight, 2.5f);
        ASSERT_TRUE (wide.has_value() && narrow.has_value());
        EXPECT_EQ (wide->desiredSize, 128) << distance;
        EXPECT_EQ (narrow->desiredSize, 1024) << distance;
    }
}
