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

//==============================================================================
class ResizableCornerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        parent.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
        parent.addAndMakeVisible (sibling);
        parent.addAndMakeVisible (corner);
    }

    void drag (const Point<float>& delta)
    {
        corner.mouseDown (MouseEvent (MouseEvent::leftButton, {}, { 0.0f, 0.0f }, &corner));
        corner.mouseDrag (MouseEvent (MouseEvent::leftButton, {}, delta, &corner));
    }

    Component parent;
    Component sibling;
    ResizableCorner corner;
};

TEST_F (ResizableCornerTests, SitsInParentBottomRightCorner)
{
    EXPECT_EQ (Rectangle<float> (184.0f, 84.0f, 16.0f, 16.0f), corner.getBounds());

    parent.setSize ({ 300.0f, 150.0f });
    EXPECT_EQ (Rectangle<float> (284.0f, 134.0f, 16.0f, 16.0f), corner.getBounds());

    corner.setCornerSize (24.0f);
    EXPECT_EQ (Rectangle<float> (276.0f, 126.0f, 24.0f, 24.0f), corner.getBounds());
}

TEST_F (ResizableCornerTests, StaysAboveSiblingsAddedLater)
{
    Component lateSibling;
    parent.addAndMakeVisible (lateSibling);
    parent.setSize ({ 210.0f, 100.0f });

    EXPECT_EQ (parent.getNumChildComponents() - 1, parent.getIndexOfChildComponent (&corner));
}

TEST_F (ResizableCornerTests, DragResizesParent)
{
    drag ({ 20.0f, 10.0f });

    EXPECT_EQ (Size<float> (220.0f, 110.0f), parent.getSize());
}

TEST_F (ResizableCornerTests, DragAppliesConstraint)
{
    corner.constrainSize = [] (Size<float> size)
    {
        return Size<float> (size.getWidth(), size.getWidth() * 0.5f);
    };

    drag ({ 40.0f, 0.0f });

    EXPECT_EQ (Size<float> (240.0f, 120.0f), parent.getSize());
}

TEST_F (ResizableCornerTests, DragNeverShrinksParentBelowCorner)
{
    drag ({ -500.0f, -500.0f });

    EXPECT_EQ (Size<float> (16.0f, 16.0f), parent.getSize());
}
