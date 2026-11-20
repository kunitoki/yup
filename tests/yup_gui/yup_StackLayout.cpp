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

class StackLayoutTests : public ::testing::Test
{
protected:
    Component first { "first" };
    Component second { "second" };
};

TEST_F (StackLayoutTests, StretchesItemsOverTheWholeArea)
{
    StackLayout stack;
    stack.items.add (StackLayout::Item (first));
    stack.items.add (StackLayout::Item (second));
    stack.performLayout (Rectangle<float> (10.0f, 20.0f, 100.0f, 50.0f));

    EXPECT_EQ (first.getBounds(), Rectangle<float> (10.0f, 20.0f, 100.0f, 50.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (10.0f, 20.0f, 100.0f, 50.0f));
}

TEST_F (StackLayoutTests, AppliesPaddingAndMargins)
{
    StackLayout stack;
    stack.setPadding (10.0f);
    stack.items.add (StackLayout::Item (first).withMargin (5.0f));
    stack.performLayout (Rectangle<int> (0, 0, 100, 100));

    EXPECT_EQ (first.getBounds(), Rectangle<float> (15.0f, 15.0f, 70.0f, 70.0f));
}

TEST_F (StackLayoutTests, AlignsSizedItems)
{
    StackLayout stack;
    stack.items.add (StackLayout::Item (first).withSize (20.0f, 10.0f).withAlign (StackLayout::Align::end, StackLayout::Align::start));
    stack.items.add (StackLayout::Item (second).withSize (20.0f, 10.0f).withAlign (StackLayout::Align::center, StackLayout::Align::end));
    stack.performLayout (Rectangle<float> (0.0f, 0.0f, 100.0f, 50.0f));

    EXPECT_EQ (first.getBounds(), Rectangle<float> (80.0f, 0.0f, 20.0f, 10.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (40.0f, 40.0f, 20.0f, 10.0f));
}

TEST_F (StackLayoutTests, ItemsInheritTheContainerAlignment)
{
    StackLayout stack;
    stack.alignX = StackLayout::Align::center;
    stack.alignY = StackLayout::Align::center;
    stack.items.add (StackLayout::Item (first).withSize (20.0f, 10.0f));
    stack.items.add (StackLayout::Item (second).withHeight (10.0f).withAlign (StackLayout::Align::stretch, StackLayout::Align::automatic));
    stack.performLayout (Rectangle<float> (0.0f, 0.0f, 100.0f, 50.0f));

    EXPECT_EQ (first.getBounds(), Rectangle<float> (40.0f, 20.0f, 20.0f, 10.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (0.0f, 20.0f, 100.0f, 10.0f));
}

TEST_F (StackLayoutTests, ClampsItemsToTheAvailableArea)
{
    StackLayout stack;
    stack.items.add (StackLayout::Item (first).withSize (500.0f, 500.0f).withMargin (10.0f));
    stack.items.add (StackLayout::Item (second).withMargin (100.0f));
    stack.performLayout (Rectangle<float> (0.0f, 0.0f, 100.0f, 50.0f));

    EXPECT_EQ (first.getBounds(), Rectangle<float> (10.0f, 10.0f, 80.0f, 30.0f));
    EXPECT_EQ (second.getWidth(), 0.0f);
    EXPECT_EQ (second.getHeight(), 0.0f);
}
