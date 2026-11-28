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
class JoystickComponentTests : public ::testing::Test
{
protected:
    using Axis = JoystickComponent::Axis;

    void SetUp() override
    {
        joystick.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    }

    MouseEvent eventAt (float x, float y) const
    {
        return MouseEvent (MouseEvent::leftButton, KeyModifiers(), { x, y }, const_cast<JoystickComponent*> (&joystick));
    }

    void drag (Point<float> from, Point<float> to)
    {
        joystick.mouseDown (eventAt (from.getX(), from.getY()));
        joystick.mouseDrag (eventAt (to.getX(), to.getY()));
        joystick.mouseUp (eventAt (to.getX(), to.getY()));
    }

    bool pressKey (int keyCode, KeyModifiers modifiers = {})
    {
        return joystick.keyDown (KeyPress (keyCode, modifiers), {});
    }

    JoystickComponent joystick;
};

//==============================================================================
TEST_F (JoystickComponentTests, DefaultsToCenteredUnitRanges)
{
    EXPECT_DOUBLE_EQ (0.5, joystick.getValue (Axis::x));
    EXPECT_DOUBLE_EQ (0.5, joystick.getValue (Axis::y));
    EXPECT_EQ (Point<float> (100.0f, 50.0f), joystick.getThumbPosition());
    EXPECT_EQ (JoystickComponent::GrabMode::anywhere, joystick.getGrabMode());
    EXPECT_FALSE (joystick.isOpaque());
}

TEST_F (JoystickComponentTests, ValuesAreSnappedToTheRange)
{
    joystick.setRange (Axis::x, { -10.0, 10.0, 5.0 });

    joystick.setValue (Axis::x, 3.0);
    EXPECT_DOUBLE_EQ (5.0, joystick.getValue (Axis::x));

    joystick.setValue (Axis::x, 100.0);
    EXPECT_DOUBLE_EQ (10.0, joystick.getValue (Axis::x));
}

TEST_F (JoystickComponentTests, ChangingTheRangeConstrainsTheValues)
{
    joystick.setValues (0.9, 0.9);
    joystick.setRange (Axis::y, { 0.0, 0.5 });

    EXPECT_DOUBLE_EQ (0.5, joystick.getValue (Axis::y));
    EXPECT_DOUBLE_EQ (0.5, joystick.getDefaultValue (Axis::y));
    EXPECT_DOUBLE_EQ (0.9, joystick.getValue (Axis::x));
}

TEST_F (JoystickComponentTests, YAxisGrowsToTheTop)
{
    joystick.setValues (0.0, 1.0);
    EXPECT_EQ (Point<float> (0.0f, 0.0f), joystick.getThumbPosition());

    drag ({ 100.0f, 50.0f }, { 150.0f, 75.0f });
    EXPECT_DOUBLE_EQ (0.75, joystick.getValue (Axis::x));
    EXPECT_DOUBLE_EQ (0.25, joystick.getValue (Axis::y));
}

TEST_F (JoystickComponentTests, SkewedRangeMapsThroughNormalizedPosition)
{
    joystick.setRange (Axis::x, { 20.0, 20000.0, 0.0, 0.25 });
    joystick.setValue (Axis::x, joystick.getRange (Axis::x).convertFrom0to1 (0.5));

    EXPECT_NEAR (0.5, joystick.getNormalizedValue (Axis::x), 1.0e-9);
    EXPECT_NEAR (100.0f, joystick.getThumbPosition().getX(), 1.0e-3f);
}

TEST_F (JoystickComponentTests, LogarithmicAxisGivesEachDecadeTheSameRoom)
{
    joystick.setLogarithmic (Axis::x, true);
    joystick.setRange (Axis::x, { 20.0, 20000.0 });

    joystick.setValue (Axis::x, 200.0);
    EXPECT_NEAR (1.0 / 3.0, joystick.getNormalizedValue (Axis::x), 1.0e-9);

    drag ({ 100.0f, 50.0f }, { 200.0f * 2.0f / 3.0f, 50.0f });
    EXPECT_NEAR (2000.0, joystick.getValue (Axis::x), 1.0e-2);

    joystick.setLogarithmic (Axis::x, false);
    EXPECT_NEAR (2000.0, joystick.getValue (Axis::x), 1.0e-2);
    EXPECT_NEAR (1980.0 / 19980.0, joystick.getNormalizedValue (Axis::x), 1.0e-6);
}

TEST_F (JoystickComponentTests, LogarithmicAxisStaysLinearWithoutPositiveRange)
{
    joystick.setRange (Axis::x, { -1.0, 1.0 });
    joystick.setLogarithmic (Axis::x, true);
    joystick.setValue (Axis::x, 0.0);

    EXPECT_TRUE (joystick.isLogarithmic (Axis::x));
    EXPECT_NEAR (0.5, joystick.getNormalizedValue (Axis::x), 1.0e-9);
}

TEST_F (JoystickComponentTests, AnywhereModeJumpsToThePointerAndClamps)
{
    joystick.mouseDown (eventAt (20.0f, 10.0f));
    EXPECT_TRUE (joystick.isDragging());
    EXPECT_DOUBLE_EQ (0.1, joystick.getValue (Axis::x));
    EXPECT_DOUBLE_EQ (0.9, joystick.getValue (Axis::y));

    joystick.mouseDrag (eventAt (-50.0f, 500.0f));
    EXPECT_DOUBLE_EQ (0.0, joystick.getValue (Axis::x));
    EXPECT_DOUBLE_EQ (0.0, joystick.getValue (Axis::y));

    joystick.mouseUp (eventAt (-50.0f, 500.0f));
    EXPECT_FALSE (joystick.isDragging());
}

TEST_F (JoystickComponentTests, ThumbModeHitsOnlyTheThumbAndDragsRelatively)
{
    joystick.setGrabMode (JoystickComponent::GrabMode::thumb);

    EXPECT_TRUE (joystick.hitTest (104.0f, 52.0f));
    EXPECT_FALSE (joystick.hitTest (20.0f, 20.0f));

    drag ({ 104.0f, 50.0f }, { 124.0f, 50.0f });
    EXPECT_DOUBLE_EQ (0.6, joystick.getValue (Axis::x));
    EXPECT_DOUBLE_EQ (0.5, joystick.getValue (Axis::y));
}

TEST_F (JoystickComponentTests, TheThumbIsAsLargeAsItsSizeAroundItsPosition)
{
    EXPECT_EQ (Rectangle<float> (92.0f, 42.0f, 16.0f, 16.0f), joystick.getThumbBounds());

    // It follows the values, around an edge of the joystick too
    joystick.setThumbSize (24.0f);
    joystick.setValues (0.25, 1.0);
    EXPECT_EQ (Rectangle<float> (38.0f, -12.0f, 24.0f, 24.0f), joystick.getThumbBounds());
    EXPECT_EQ (joystick.getThumbPosition(), joystick.getThumbBounds().getCenter());

    // Grabbed by the thumb, the joystick takes a press across it and none beside it
    joystick.setGrabMode (JoystickComponent::GrabMode::thumb);
    EXPECT_TRUE (joystick.hitTest (61.0f, 0.0f));
    EXPECT_FALSE (joystick.hitTest (64.0f, 0.0f));
}

TEST_F (JoystickComponentTests, SnapBackAndDoubleClickRestoreDefaults)
{
    joystick.setDefaultValue (Axis::x, 0.2);
    joystick.setSnapBack (true);

    drag ({ 0.0f, 0.0f }, { 180.0f, 90.0f });
    EXPECT_DOUBLE_EQ (0.2, joystick.getValue (Axis::x));
    EXPECT_DOUBLE_EQ (0.5, joystick.getValue (Axis::y));

    joystick.setSnapBack (false);
    joystick.setValues (1.0, 1.0);
    joystick.mouseDoubleClick (eventAt (0.0f, 0.0f));
    EXPECT_DOUBLE_EQ (0.2, joystick.getValue (Axis::x));
    EXPECT_DOUBLE_EQ (0.5, joystick.getValue (Axis::y));
}

TEST_F (JoystickComponentTests, ArrowKeysNudgeByIntervalOrPercent)
{
    joystick.setRange (Axis::x, { 0.0, 10.0, 1.0 });
    joystick.setValue (Axis::x, 5.0);

    EXPECT_TRUE (pressKey (KeyPress::rightKey));
    EXPECT_DOUBLE_EQ (6.0, joystick.getValue (Axis::x));

    EXPECT_TRUE (pressKey (KeyPress::upKey, KeyModifiers (KeyModifiers::shiftMask)));
    EXPECT_NEAR (0.6, joystick.getValue (Axis::y), 1.0e-9);

    EXPECT_FALSE (pressKey (KeyPress::homeKey));
}

TEST_F (JoystickComponentTests, NotifiesOnceWithBothValues)
{
    int calls = 0;
    double lastX = 0.0, lastY = 0.0;
    joystick.onValueChanged = [&] (double x, double y)
    {
        ++calls;
        lastX = x;
        lastY = y;
    };

    joystick.setValues (0.25, 0.75);
    EXPECT_EQ (1, calls);
    EXPECT_DOUBLE_EQ (0.25, lastX);
    EXPECT_DOUBLE_EQ (0.75, lastY);

    joystick.setValues (0.25, 0.75);
    joystick.setValue (Axis::x, 0.1, dontSendNotification);
    EXPECT_EQ (1, calls);
}

TEST_F (JoystickComponentTests, DragSignalsBracketTheGesture)
{
    StringArray events;
    joystick.onDragStarted = [&] { events.add ("start"); };
    joystick.onValueChanged = [&] (double, double) { events.add ("value"); };
    joystick.onDragEnded = [&] { events.add ("end"); };

    drag ({ 10.0f, 10.0f }, { 30.0f, 30.0f });

    EXPECT_EQ (StringArray ({ "start", "value", "value", "end" }), events);
}
