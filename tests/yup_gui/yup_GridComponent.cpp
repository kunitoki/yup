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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

class GridComponentTests : public ::testing::Test
{
protected:
    Component first { "first" };
    Component second { "second" };
};

TEST_F (GridComponentTests, ParsesTrackLists)
{
    const auto tracks = GridComponent::parseTracks ("1fr 80px 25% auto");

    ASSERT_EQ (tracks.size(), 4);
    EXPECT_TRUE (tracks.getReference (0).isFractional());
    EXPECT_EQ (tracks.getReference (1).maximum.type, Grid::TrackInfo::SizeType::pixels);
    EXPECT_FLOAT_EQ (tracks.getReference (1).maximum.value, 80.0f);
    EXPECT_EQ (tracks.getReference (2).maximum.type, Grid::TrackInfo::SizeType::percent);
    EXPECT_EQ (tracks.getReference (3).maximum.type, Grid::TrackInfo::SizeType::autoSize);
}

TEST_F (GridComponentTests, ParsesATrackCount)
{
    EXPECT_EQ (GridComponent::parseTracks (3).size(), 3);
    EXPECT_EQ (GridComponent::parseTracks ("2").size(), 2);
}

TEST_F (GridComponentTests, PlacesChildrenInTheirCells)
{
    GridComponent grid;
    grid.addAndMakeVisible (first);
    grid.addAndMakeVisible (second);

    grid.setColumns ("1fr 1fr");
    grid.setRows (1);
    second.setProperty (Identifier ("gridColumn"), 1);

    grid.setBounds (0, 0, 100, 40);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (0.0f, 0.0f, 50.0f, 40.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (50.0f, 0.0f, 50.0f, 40.0f));
}

TEST_F (GridComponentTests, PlacesChildrenInNamedAreas)
{
    GridComponent grid;
    grid.addAndMakeVisible (first);
    grid.addAndMakeVisible (second);

    grid.setColumns ("1fr 1fr");
    grid.setRows ("1fr 1fr");
    grid.setProperty (Identifier ("areas"), "head head, side main");
    first.setProperty (Identifier ("gridArea"), "head");
    second.setProperty (Identifier ("gridArea"), "main");

    grid.setBounds (0, 0, 100, 40);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (0.0f, 0.0f, 100.0f, 20.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (50.0f, 20.0f, 50.0f, 20.0f));
}
