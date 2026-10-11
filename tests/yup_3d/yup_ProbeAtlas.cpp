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

class ProbeAtlasTests : public ::testing::Test
{
protected:
    static bool overlaps (const Rectangle<int>& a, const Rectangle<int>& b)
    {
        return a.getX() < b.getRight() && b.getX() < a.getRight() && a.getY() < b.getBottom() && b.getY() < a.getBottom();
    }
};

TEST_F (ProbeAtlasTests, BlocksNeverOverlap)
{
    ProbeAtlas atlas (256, 1024);
    Random random (7);
    std::vector<Rectangle<int>> blocks;

    for (int i = 0; i < 60; ++i)
    {
        auto block = atlas.allocate (1 + random.nextInt (40), 1 + random.nextInt (40));
        if (! block)
            break;

        for (const auto& other : blocks)
            EXPECT_FALSE (overlaps (*block, other));

        EXPECT_GE (block->getX(), 0);
        EXPECT_LE (block->getRight(), 256);
        EXPECT_LE (block->getBottom(), atlas.getRequiredHeight());
        blocks.push_back (*block);
    }

    EXPECT_GT (blocks.size(), 20u);
}

TEST_F (ProbeAtlasTests, AlignedTilesSitOnTheirGrid)
{
    ProbeAtlas atlas (1024, 1024);

    for (int i = 0; i < 12; ++i)
    {
        auto tile = atlas.allocate (128, 128, 128);
        ASSERT_TRUE (tile.has_value());
        EXPECT_EQ (tile->getX() % 128, 0);
        EXPECT_EQ (tile->getY() % 128, 0);
    }
}

TEST_F (ProbeAtlasTests, AlignedTilesStayOnTheirGridAfterUnalignedBlocks)
{
    ProbeAtlas atlas (256, 1024);
    std::vector<Rectangle<int>> blocks;

    const auto row = atlas.allocate (9, 1);
    ASSERT_TRUE (row.has_value());
    blocks.push_back (*row);

    const auto unaligned = atlas.allocate (30, 30);
    ASSERT_TRUE (unaligned.has_value());
    atlas.release (*unaligned);

    for (int i = 0; i < 4; ++i)
    {
        const auto tile = atlas.allocate (30, 30, 32);
        ASSERT_TRUE (tile.has_value());
        EXPECT_EQ (tile->getX() % 32, 0);
        EXPECT_EQ (tile->getY() % 32, 0);

        for (const auto& other : blocks)
            EXPECT_FALSE (overlaps (*tile, other));

        blocks.push_back (*tile);
    }

    EXPECT_EQ (atlas.allocate (30, 30), unaligned);
}

TEST_F (ProbeAtlasTests, ReleasedBlockIsReused)
{
    ProbeAtlas atlas (256, 256);
    const auto first = atlas.allocate (64, 64, 64);
    ASSERT_TRUE (first.has_value());
    atlas.allocate (64, 64, 64);
    atlas.release (*first);

    const auto again = atlas.allocate (64, 64, 64);
    ASSERT_TRUE (again.has_value());
    EXPECT_EQ (*again, *first);
}

TEST_F (ProbeAtlasTests, FullAtlasRefuses)
{
    ProbeAtlas atlas (128, 128);
    ASSERT_TRUE (atlas.allocate (128, 128).has_value());
    EXPECT_FALSE (atlas.allocate (1, 1).has_value());
    EXPECT_FALSE (atlas.allocate (256, 1).has_value());
}

TEST_F (ProbeAtlasTests, HeightGrowsInPowersOfTwoAndBumpsTheGeneration)
{
    ProbeAtlas atlas (128, 1024);
    EXPECT_EQ (atlas.getRequiredHeight(), 1);

    const auto generation = atlas.getGeneration();
    atlas.allocate (128, 100);
    EXPECT_EQ (atlas.getRequiredHeight(), 128);
    EXPECT_GT (atlas.getGeneration(), generation);

    atlas.allocate (128, 100);
    EXPECT_EQ (atlas.getRequiredHeight(), 256);
}

TEST_F (ProbeAtlasTests, ClearEmptiesTheAtlas)
{
    ProbeAtlas atlas (128, 128);
    atlas.allocate (128, 128);

    const auto generation = atlas.getGeneration();
    atlas.clear();
    EXPECT_EQ (atlas.getRequiredHeight(), 1);
    EXPECT_GT (atlas.getGeneration(), generation);
    EXPECT_TRUE (atlas.allocate (128, 128).has_value());
}

TEST_F (ProbeAtlasTests, RejectsEmptyBlocksAndInvalidAlignment)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "Empty blocks and a non positive alignment trigger an assertion when assertions are enabled or logged";
#endif

    ProbeAtlas atlas (128, 128);
    EXPECT_FALSE (atlas.allocate (0, 16).has_value());
    EXPECT_FALSE (atlas.allocate (16, -1).has_value());
    EXPECT_FALSE (atlas.allocate (16, 16, 0).has_value());
    EXPECT_EQ (atlas.getRequiredHeight(), 1);
}
