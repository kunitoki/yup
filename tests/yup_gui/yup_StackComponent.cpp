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

class StackComponentTests : public ::testing::Test
{
protected:
    Component first { "first" };
    Component second { "second" };
};

TEST_F (StackComponentTests, StretchesTheChildrenOverTheContentArea)
{
    StackComponent stack;
    stack.addAndMakeVisible (first);
    stack.addAndMakeVisible (second);

    stack.setPadding (EdgeSpacing<float> (10.0f));
    stack.setBounds (0, 0, 100, 60);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (10.0f, 10.0f, 80.0f, 40.0f));
    EXPECT_EQ (second.getBounds(), first.getBounds());
}

TEST_F (StackComponentTests, AlignsASizedChild)
{
    StackComponent stack;
    stack.addAndMakeVisible (first);

    first.setProperty (Identifier ("preferredWidth"), 20.0);
    first.setProperty (Identifier ("preferredHeight"), 10.0);
    first.setProperty (Identifier ("justifySelf"), "end");
    first.setProperty (Identifier ("alignSelf"), "center");

    stack.setBounds (0, 0, 100, 50);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (80.0f, 20.0f, 20.0f, 10.0f));
}

TEST_F (StackComponentTests, FitsToItsLargestChild)
{
    StackComponent stack;
    stack.addAndMakeVisible (first);
    stack.addAndMakeVisible (second);

    first.setProperty (Identifier ("preferredWidth"), 60.0);
    first.setProperty (Identifier ("preferredHeight"), 10.0);
    second.setProperty (Identifier ("preferredWidth"), 20.0);
    second.setProperty (Identifier ("preferredHeight"), 30.0);

    stack.setPadding (EdgeSpacing<float> (5.0f));
    stack.setFitContent (true);

    EXPECT_FLOAT_EQ (stack.findPropertyAs<float> (Identifier ("preferredWidth")), 70.0f);
    EXPECT_FLOAT_EQ (stack.findPropertyAs<float> (Identifier ("preferredHeight")), 40.0f);
}
