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
class ResizableDividerTests : public ::testing::Test
{
protected:
    using Length = ResizableDivider::Length;

    void SetUp() override
    {
        // 406 wide with a 6 thick bar leaves a usable extent of 400.
        parent.setBounds (0.0f, 0.0f, 406.0f, 100.0f);
        parent.addAndMakeVisible (divider);
        layout();
    }

    void layout()
    {
        divider.setBounds (divider.getEffectivePosition(), 0.0f, divider.getThickness(), parent.getHeight());
    }

    void layoutOnChange()
    {
        divider.onPositionChanged = [this] (float)
        {
            layout();
        };
    }

    MouseEvent eventAtParentX (float parentX) const
    {
        return MouseEvent (MouseEvent::leftButton, KeyModifiers(), { parentX - divider.getX(), 50.0f }, const_cast<ResizableDivider*> (&divider));
    }

    void drag (float fromParentX, float toParentX)
    {
        divider.mouseDown (eventAtParentX (fromParentX));
        divider.mouseDrag (eventAtParentX (toParentX));
        divider.mouseUp (eventAtParentX (toParentX));
    }

    bool pressKey (int keyCode, KeyModifiers modifiers = {})
    {
        return divider.keyDown (KeyPress (keyCode, modifiers), {});
    }

    Component parent;
    ResizableDivider divider;
};

//==============================================================================
TEST_F (ResizableDividerTests, DefaultsToCenteredVerticalDivider)
{
    EXPECT_EQ (ResizableDivider::Orientation::vertical, divider.getOrientation());
    EXPECT_EQ (MouseCursor::ResizeLeftRight, divider.getMouseCursor().getType());
    EXPECT_FLOAT_EQ (400.0f, divider.getUsableExtent());
    EXPECT_FLOAT_EQ (200.0f, divider.getPosition());
    EXPECT_FLOAT_EQ (0.5f, divider.getNormalizedPosition());
    EXPECT_FALSE (divider.isOpaque());
}

TEST_F (ResizableDividerTests, HorizontalUsesUpDownCursorAndParentHeight)
{
    divider.setOrientation (ResizableDivider::Orientation::horizontal);

    EXPECT_EQ (MouseCursor::ResizeUpDown, divider.getMouseCursor().getType());
    EXPECT_FLOAT_EQ (94.0f, divider.getUsableExtent());
    EXPECT_FLOAT_EQ (47.0f, divider.getPosition());
}

TEST_F (ResizableDividerTests, ExplicitAvailableExtentOverridesParent)
{
    divider.setAvailableExtent (206.0f);
    EXPECT_FLOAT_EQ (100.0f, divider.getPosition());

    divider.setAvailableExtent (std::nullopt);
    EXPECT_FLOAT_EQ (200.0f, divider.getPosition());
}

TEST_F (ResizableDividerTests, PercentPositionResolvesAgainstUsableExtent)
{
    divider.setPosition (Length::percent (25.0f));
    EXPECT_FLOAT_EQ (100.0f, divider.getPosition());

    divider.setPosition (Length::percent (100.0f));
    EXPECT_FLOAT_EQ (400.0f, divider.getPosition());

    divider.setNormalizedPosition (0.75f);
    EXPECT_FLOAT_EQ (300.0f, divider.getPosition());
}

TEST_F (ResizableDividerTests, PixelAndPercentConstraintsClampPosition)
{
    divider.setMinPosition (Length::pixels (50.0f));
    divider.setMaxPosition (Length::percent (75.0f));

    divider.setPosition (10.0f);
    EXPECT_FLOAT_EQ (50.0f, divider.getPosition());

    divider.setPosition (390.0f);
    EXPECT_FLOAT_EQ (300.0f, divider.getPosition());

    divider.setMinTrailingSize (Length::pixels (150.0f));
    EXPECT_FLOAT_EQ (250.0f, divider.getResolvedMaxPosition());
    EXPECT_FLOAT_EQ (250.0f, divider.getPosition());
}

TEST_F (ResizableDividerTests, MinimumWinsOverConflictingMaximum)
{
    divider.setMinPosition (Length::pixels (300.0f));
    divider.setMaxPosition (Length::pixels (100.0f));

    EXPECT_FLOAT_EQ (300.0f, divider.getResolvedMinPosition());
    EXPECT_FLOAT_EQ (300.0f, divider.getResolvedMaxPosition());
    EXPECT_FLOAT_EQ (300.0f, divider.getPosition());
}

TEST_F (ResizableDividerTests, ProportionalPolicyKeepsRatio)
{
    divider.setPosition (100.0f);
    parent.setSize ({ 806.0f, 100.0f });

    EXPECT_FLOAT_EQ (200.0f, divider.getPosition());
    EXPECT_FLOAT_EQ (0.25f, divider.getNormalizedPosition());
}

TEST_F (ResizableDividerTests, PreserveLeadingPolicyKeepsLeadingSize)
{
    divider.setResizePolicy (ResizableDivider::ResizePolicy::preserveLeading);
    divider.setPosition (100.0f);
    parent.setSize ({ 806.0f, 100.0f });

    EXPECT_FLOAT_EQ (100.0f, divider.getPosition());
}

TEST_F (ResizableDividerTests, PreserveTrailingPolicyKeepsTrailingSize)
{
    divider.setResizePolicy (ResizableDivider::ResizePolicy::preserveTrailing);
    divider.setPosition (100.0f);
    parent.setSize ({ 806.0f, 100.0f });

    EXPECT_FLOAT_EQ (500.0f, divider.getPosition());
}

TEST_F (ResizableDividerTests, SwitchingPolicyDoesNotMoveDivider)
{
    divider.setPosition (100.0f);
    parent.setSize ({ 806.0f, 100.0f });
    divider.setResizePolicy (ResizableDivider::ResizePolicy::preserveLeading);

    EXPECT_FLOAT_EQ (200.0f, divider.getPosition());
}

TEST_F (ResizableDividerTests, ShrinkingAndExpandingParentDoesNotDrift)
{
    divider.setResizePolicy (ResizableDivider::ResizePolicy::preserveLeading);
    divider.setPosition (300.0f);

    parent.setSize ({ 106.0f, 100.0f });
    EXPECT_FLOAT_EQ (100.0f, divider.getPosition());

    parent.setSize ({ 406.0f, 100.0f });
    EXPECT_FLOAT_EQ (300.0f, divider.getPosition());
}

TEST_F (ResizableDividerTests, PositionSetBeforeExtentIsKnownIsResolvedLater)
{
    Component other;
    other.setBounds (0.0f, 0.0f, 406.0f, 100.0f);

    ResizableDivider late;
    late.setResizePolicy (ResizableDivider::ResizePolicy::preserveLeading);
    late.setPosition (Length::pixels (120.0f));
    EXPECT_FLOAT_EQ (0.0f, late.getPosition());

    other.addAndMakeVisible (late);
    EXPECT_FLOAT_EQ (120.0f, late.getPosition());

    other.setSize ({ 806.0f, 100.0f });
    EXPECT_FLOAT_EQ (120.0f, late.getPosition());
}

TEST_F (ResizableDividerTests, ParentResizeNotifiesPositionChange)
{
    float reported = -1.0f;
    divider.onPositionChanged = [&] (float position)
    {
        reported = position;
    };

    parent.setSize ({ 806.0f, 100.0f });
    EXPECT_FLOAT_EQ (400.0f, reported);
}

TEST_F (ResizableDividerTests, SettingSamePositionDoesNotNotify)
{
    int calls = 0;
    divider.onPositionChanged = [&] (float)
    {
        ++calls;
    };

    divider.setPosition (200.0f);
    EXPECT_EQ (0, calls);

    divider.setPosition (210.0f);
    EXPECT_EQ (1, calls);
}

//==============================================================================
TEST_F (ResizableDividerTests, DragFollowsPointerInParentSpaceWhileMoved)
{
    layoutOnChange();

    std::vector<float> deltas;
    divider.onDragDelta = [&] (float delta)
    {
        deltas.push_back (delta);
    };

    divider.mouseDown (eventAtParentX (203.0f));
    divider.mouseDrag (eventAtParentX (253.0f));
    EXPECT_FLOAT_EQ (250.0f, divider.getPosition());
    EXPECT_FLOAT_EQ (250.0f, divider.getX());

    divider.mouseDrag (eventAtParentX (263.0f));
    EXPECT_FLOAT_EQ (260.0f, divider.getPosition());
    divider.mouseUp (eventAtParentX (263.0f));

    ASSERT_EQ (2u, deltas.size());
    EXPECT_FLOAT_EQ (50.0f, deltas[0]);
    EXPECT_FLOAT_EQ (10.0f, deltas[1]);
}

TEST_F (ResizableDividerTests, DragReportsStateAndCallbacks)
{
    int started = 0;
    int ended = 0;
    divider.onDragStarted = [&]
    {
        ++started;
    };
    divider.onDragEnded = [&]
    {
        ++ended;
    };

    divider.mouseDown (eventAtParentX (203.0f));
    EXPECT_TRUE (divider.isDragging());
    EXPECT_TRUE (static_cast<bool> (divider.getProperty ("dragging")));

    divider.mouseUp (eventAtParentX (203.0f));
    EXPECT_FALSE (divider.isDragging());
    EXPECT_FALSE (static_cast<bool> (divider.getProperty ("dragging")));

    EXPECT_EQ (1, started);
    EXPECT_EQ (1, ended);
}

TEST_F (ResizableDividerTests, DragIsClampedToConstraints)
{
    divider.setMinPosition (Length::pixels (100.0f));

    drag (203.0f, -500.0f);
    EXPECT_FLOAT_EQ (100.0f, divider.getPosition());
    EXPECT_FALSE (divider.isCollapsed());
}

TEST_F (ResizableDividerTests, DragSnapsToNearbySnapPosition)
{
    divider.addSnapPosition (Length::percent (50.0f));
    divider.setPosition (100.0f);
    layout();

    drag (103.0f, 199.0f);
    EXPECT_FLOAT_EQ (200.0f, divider.getPosition());

    divider.setPosition (100.0f);
    layout();

    drag (103.0f, 193.0f);
    EXPECT_FLOAT_EQ (190.0f, divider.getPosition());

    divider.clearSnapPositions();
    EXPECT_TRUE (divider.getSnapPositions().empty());
}

TEST_F (ResizableDividerTests, DragPastMinimumCollapsesLeadingAndDragBackExpands)
{
    std::vector<ResizableDivider::CollapseSide> changes;
    divider.onCollapsedChanged = [&] (ResizableDivider::CollapseSide side)
    {
        changes.push_back (side);
    };

    divider.setCanCollapseLeading (true);
    divider.setMinPosition (Length::pixels (100.0f));
    divider.setPosition (150.0f);
    layout();

    divider.mouseDown (eventAtParentX (153.0f));
    divider.mouseDrag (eventAtParentX (53.0f));
    EXPECT_EQ (ResizableDivider::CollapseSide::leading, divider.getCollapsedSide());
    EXPECT_FLOAT_EQ (0.0f, divider.getEffectivePosition());
    EXPECT_TRUE (static_cast<bool> (divider.getProperty ("collapsed")));

    divider.mouseDrag (eventAtParentX (133.0f));
    divider.mouseUp (eventAtParentX (133.0f));
    EXPECT_FALSE (divider.isCollapsed());
    EXPECT_FLOAT_EQ (130.0f, divider.getEffectivePosition());

    ASSERT_EQ (2u, changes.size());
    EXPECT_EQ (ResizableDivider::CollapseSide::leading, changes[0]);
    EXPECT_EQ (ResizableDivider::CollapseSide::none, changes[1]);
}

TEST_F (ResizableDividerTests, DragPastMaximumCollapsesTrailing)
{
    divider.setCanCollapseTrailing (true);
    divider.setMaxPosition (Length::pixels (300.0f));

    drag (203.0f, 403.0f);
    EXPECT_EQ (ResizableDivider::CollapseSide::trailing, divider.getCollapsedSide());
    EXPECT_FLOAT_EQ (400.0f, divider.getEffectivePosition());
}

TEST_F (ResizableDividerTests, ExpandingRestoresPositionBeforeCollapse)
{
    divider.setPosition (150.0f);
    divider.setCollapsedSide (ResizableDivider::CollapseSide::trailing);
    EXPECT_FLOAT_EQ (400.0f, divider.getEffectivePosition());
    EXPECT_FLOAT_EQ (150.0f, divider.getPosition());

    divider.setCollapsedSide (ResizableDivider::CollapseSide::none);
    EXPECT_FLOAT_EQ (150.0f, divider.getEffectivePosition());
}

TEST_F (ResizableDividerTests, DoubleClickResetsToDefaultPosition)
{
    int doubleClicks = 0;
    divider.onDoubleClick = [&]
    {
        ++doubleClicks;
    };

    divider.setDefaultPosition (Length::percent (25.0f));
    divider.setPosition (300.0f);
    divider.setCollapsedSide (ResizableDivider::CollapseSide::leading);

    divider.mouseDoubleClick (eventAtParentX (3.0f));
    EXPECT_FALSE (divider.isCollapsed());
    EXPECT_FLOAT_EQ (100.0f, divider.getPosition());
    EXPECT_EQ (1, doubleClicks);
}

TEST_F (ResizableDividerTests, DoubleClickWithoutDefaultKeepsPosition)
{
    divider.setPosition (300.0f);
    divider.mouseDoubleClick (eventAtParentX (303.0f));

    EXPECT_FLOAT_EQ (300.0f, divider.getPosition());
}

//==============================================================================
TEST_F (ResizableDividerTests, KeyboardNudgesAlongAxis)
{
    divider.setMinPosition (Length::pixels (20.0f));
    divider.setMaxPosition (Length::pixels (380.0f));

    EXPECT_TRUE (pressKey (KeyPress::rightKey));
    EXPECT_FLOAT_EQ (210.0f, divider.getPosition());

    EXPECT_TRUE (pressKey (KeyPress::leftKey, KeyModifiers (KeyModifiers::shiftMask)));
    EXPECT_FLOAT_EQ (160.0f, divider.getPosition());

    EXPECT_TRUE (pressKey (KeyPress::homeKey));
    EXPECT_FLOAT_EQ (20.0f, divider.getPosition());

    EXPECT_TRUE (pressKey (KeyPress::endKey));
    EXPECT_FLOAT_EQ (380.0f, divider.getPosition());

    EXPECT_FALSE (pressKey (KeyPress::upKey));
}

TEST_F (ResizableDividerTests, HorizontalKeyboardUsesUpAndDown)
{
    divider.setOrientation (ResizableDivider::Orientation::horizontal);
    divider.setKeyboardStep (4.0f);

    EXPECT_TRUE (pressKey (KeyPress::downKey));
    EXPECT_FLOAT_EQ (51.0f, divider.getPosition());

    EXPECT_TRUE (pressKey (KeyPress::upKey));
    EXPECT_FLOAT_EQ (47.0f, divider.getPosition());

    EXPECT_FALSE (pressKey (KeyPress::rightKey));
}

//==============================================================================
TEST_F (ResizableDividerTests, LayoutComponentsSplitsVerticalArea)
{
    Component leading, trailing;
    divider.setPosition (100.0f);

    divider.layoutComponents ({ 10.0f, 20.0f, 206.0f, 50.0f }, &leading, &trailing);

    EXPECT_FLOAT_EQ (206.0f, divider.getAvailableExtent());
    EXPECT_EQ (Rectangle<float> (10.0f, 20.0f, 100.0f, 50.0f), leading.getBounds());
    EXPECT_EQ (Rectangle<float> (110.0f, 20.0f, 6.0f, 50.0f), divider.getBounds());
    EXPECT_EQ (Rectangle<float> (116.0f, 20.0f, 100.0f, 50.0f), trailing.getBounds());
}

TEST_F (ResizableDividerTests, LayoutComponentsSplitsHorizontalAreaAndHonorsCollapse)
{
    Component leading, trailing;
    divider.setOrientation (ResizableDivider::Orientation::horizontal);
    divider.setCollapsedSide (ResizableDivider::CollapseSide::leading);

    divider.layoutComponents ({ 0.0f, 10.0f, 80.0f, 106.0f }, &leading, nullptr);
    divider.layoutComponents ({ 0.0f, 10.0f, 80.0f, 106.0f }, nullptr, &trailing);

    EXPECT_EQ (Rectangle<float> (0.0f, 10.0f, 80.0f, 0.0f), leading.getBounds());
    EXPECT_EQ (Rectangle<float> (0.0f, 10.0f, 80.0f, 6.0f), divider.getBounds());
    EXPECT_EQ (Rectangle<float> (0.0f, 16.0f, 80.0f, 100.0f), trailing.getBounds());
}

TEST_F (ResizableDividerTests, ReflectedPropertiesRoundTrip)
{
    divider.setProperty ("position", 120.0);
    EXPECT_FLOAT_EQ (120.0f, divider.getPosition());
    EXPECT_FLOAT_EQ (120.0f, static_cast<float> (divider.getProperty ("position")));

    divider.setProperty ("normalized", 0.25);
    EXPECT_FLOAT_EQ (100.0f, divider.getPosition());

    divider.setProperty ("gripStyle", "dots");
    EXPECT_EQ (ResizableDivider::GripStyle::dots, divider.getGripStyle());

    divider.setProperty ("resizePolicy", "preserveTrailing");
    EXPECT_EQ (ResizableDivider::ResizePolicy::preserveTrailing, divider.getResizePolicy());

    divider.setProperty ("thickness", 10.0);
    EXPECT_FLOAT_EQ (10.0f, divider.getThickness());

    divider.setProperty ("orientation", "horizontal");
    EXPECT_EQ (ResizableDivider::Orientation::horizontal, divider.getOrientation());
    EXPECT_EQ ("horizontal", divider.getProperty ("orientation").toString());
}
