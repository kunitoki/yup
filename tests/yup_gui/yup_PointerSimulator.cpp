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
class PointerSimulatorTests : public ::testing::Test
{
protected:
    /** Records the pointer events it receives, in order. */
    class Recorder : public Component
    {
    public:
        void mouseEnter (const MouseEvent&) override { events.add ("enter"); }

        void mouseExit (const MouseEvent&) override { events.add ("exit"); }

        void mouseMove (const MouseEvent& event) override
        {
            events.add ("move");
            lastPosition = event.getPosition();
        }

        void mouseDown (const MouseEvent& event) override
        {
            events.add ("down");
            lastPosition = event.getPosition();
        }

        void mouseDrag (const MouseEvent& event) override
        {
            events.add ("drag");
            lastPosition = event.getPosition();
        }

        void mouseUp (const MouseEvent&) override { events.add ("up"); }

        void mouseDoubleClick (const MouseEvent&) override { events.add ("doubleClick"); }

        bool mouseWheel (const MouseEvent&, const MouseWheelData& wheel) override
        {
            events.add ("wheel");
            lastWheel = wheel.getDeltaY();
            return true;
        }

        StringArray events;
        Point<float> lastPosition;
        float lastWheel = 0.0f;
    };

    void SetUp() override
    {
        root.setVisible (true);
        root.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
        left.setBounds (10.0f, 10.0f, 80.0f, 80.0f);
        right.setBounds (110.0f, 10.0f, 80.0f, 80.0f);

        root.addAndMakeVisible (left);
        root.addAndMakeVisible (right);
    }

    Recorder root;
    Recorder left;
    Recorder right;
    PointerSimulator pointer { root };
};

//==============================================================================
TEST_F (PointerSimulatorTests, MovingEntersAndLeavesWhatIsUnderThePointer)
{
    EXPECT_EQ (pointer.moveTo ({ 20.0f, 20.0f }), &left);
    EXPECT_EQ (left.events, StringArray ({ "enter", "move" }));
    EXPECT_EQ (left.lastPosition, Point<float> (10.0f, 10.0f));
    EXPECT_TRUE (left.isHovered());
    EXPECT_TRUE (root.isHovered());

    EXPECT_EQ (pointer.moveTo ({ 120.0f, 20.0f }), &right);
    EXPECT_EQ (left.events, StringArray ({ "enter", "move", "exit" }));
    EXPECT_EQ (right.events, StringArray ({ "enter", "move" }));
    EXPECT_FALSE (left.isHovered());
    EXPECT_TRUE (right.isHovered());
    EXPECT_EQ (pointer.getComponentUnderPointer(), &right);

    pointer.leave();
    EXPECT_EQ (right.events, StringArray ({ "enter", "move", "exit" }));
    EXPECT_FALSE (right.isHovered());
    EXPECT_FALSE (root.isHovered());
    EXPECT_EQ (pointer.getComponentUnderPointer(), nullptr);
}

TEST_F (PointerSimulatorTests, AClickPressesAndReleasesWhereItIs)
{
    EXPECT_EQ (pointer.click ({ 120.0f, 20.0f }), &right);
    EXPECT_EQ (right.events, StringArray ({ "enter", "move", "down", "up" }));
    EXPECT_EQ (right.lastPosition, Point<float> (10.0f, 10.0f));
    EXPECT_FALSE (right.isPressed());
    EXPECT_TRUE (right.isHovered());
    EXPECT_TRUE (left.events.isEmpty());
}

TEST_F (PointerSimulatorTests, WhatIsPressedKeepsThePointerUntilItIsReleased)
{
    pointer.moveTo ({ 20.0f, 20.0f });
    EXPECT_EQ (pointer.press(), &left);
    EXPECT_TRUE (left.isPressed());
    EXPECT_EQ (pointer.getPressedComponent(), &left);

    // The drag goes to the component pressed, in its coordinates, even over another one
    EXPECT_EQ (pointer.moveTo ({ 120.0f, 20.0f }), &left);
    EXPECT_EQ (left.events, StringArray ({ "enter", "move", "down", "drag" }));
    EXPECT_EQ (left.lastPosition, Point<float> (110.0f, 10.0f));
    EXPECT_TRUE (right.events.isEmpty());

    EXPECT_EQ (pointer.release(), &left);
    EXPECT_FALSE (left.isPressed());
    EXPECT_EQ (pointer.getPressedComponent(), nullptr);

    // Released, the pointer is over what it was dragged to
    EXPECT_EQ (left.events, StringArray ({ "enter", "move", "down", "drag", "up", "exit" }));
    EXPECT_EQ (right.events, StringArray ({ "enter" }));
}

TEST_F (PointerSimulatorTests, ADragMovesInStepsBetweenAPressAndARelease)
{
    EXPECT_EQ (pointer.drag ({ 20.0f, 20.0f }, { 60.0f, 60.0f }, 4), &left);
    EXPECT_EQ (left.events, StringArray ({ "enter", "move", "down", "drag", "drag", "drag", "drag", "up" }));
    EXPECT_EQ (left.lastPosition, Point<float> (50.0f, 50.0f));
}

TEST_F (PointerSimulatorTests, ADoubleClickIsToldBeforeItsSecondRelease)
{
    EXPECT_EQ (pointer.doubleClick ({ 20.0f, 20.0f }), &left);
    EXPECT_EQ (left.events, StringArray ({ "enter", "move", "down", "up", "down", "doubleClick", "up" }));
}

TEST_F (PointerSimulatorTests, TheWheelGoesToWhatIsUnderThePointer)
{
    EXPECT_EQ (pointer.wheel ({ 120.0f, 20.0f }, { 0.0f, 1.5f }), &right);
    EXPECT_EQ (right.events, StringArray ({ "enter", "wheel" }));
    EXPECT_FLOAT_EQ (right.lastWheel, 1.5f);
}

TEST_F (PointerSimulatorTests, NothingIsSentWhereThereIsNoComponent)
{
    EXPECT_EQ (pointer.click ({ 500.0f, 500.0f }), nullptr);
    EXPECT_EQ (pointer.wheel ({ 500.0f, 500.0f }, { 0.0f, 1.0f }), nullptr);
    EXPECT_TRUE (root.events.isEmpty());

    // The root takes what none of its children is under
    EXPECT_EQ (pointer.click ({ 100.0f, 50.0f }), &root);
}

TEST_F (PointerSimulatorTests, ADisabledComponentIsHoveredButNotPressed)
{
    left.setEnabled (false);

    pointer.moveTo ({ 20.0f, 20.0f });
    EXPECT_EQ (pointer.press(), &left);
    EXPECT_TRUE (left.events.isEmpty());
    EXPECT_TRUE (left.isHovered());
    EXPECT_FALSE (left.isPressed());
}

TEST_F (PointerSimulatorTests, AComponentDeletedWhilePressedIsForgotten)
{
    auto doomed = std::make_unique<Recorder>();
    doomed->setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    root.addAndMakeVisible (*doomed);

    pointer.moveTo ({ 20.0f, 20.0f });
    EXPECT_EQ (pointer.press(), doomed.get());

    doomed.reset();

    EXPECT_EQ (pointer.getPressedComponent(), nullptr);
    EXPECT_EQ (pointer.release(), nullptr);
    EXPECT_EQ (pointer.moveTo ({ 20.0f, 20.0f }), &left);
}

TEST_F (PointerSimulatorTests, WidgetsAnswerToASimulatedPointer)
{
    ToggleButton toggle;
    Slider slider (Slider::LinearHorizontal);
    int numClicks = 0;

    toggle.setBounds (0.0f, 0.0f, 100.0f, 30.0f);
    slider.setBounds (0.0f, 40.0f, 200.0f, 30.0f);
    slider.setRange (0.0, 1.0);
    slider.setValue (0.0);

    Component panel;
    panel.setVisible (true);
    panel.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    panel.addAndMakeVisible (toggle);
    panel.addAndMakeVisible (slider);

    toggle.onClick = [&numClicks]
    {
        ++numClicks;
    };

    PointerSimulator widgets (panel);

    EXPECT_EQ (widgets.click ({ 50.0f, 15.0f }), &toggle);
    EXPECT_EQ (numClicks, 1);
    EXPECT_TRUE (toggle.getToggleState());

    EXPECT_EQ (widgets.drag ({ 20.0f, 55.0f }, { 180.0f, 55.0f }), &slider);
    EXPECT_GT (slider.getValue(), 0.5);

    const auto dragged = slider.getValue();
    EXPECT_EQ (widgets.wheel ({ 100.0f, 55.0f }, { 0.0f, -1.0f }), &slider);
    EXPECT_NE (slider.getValue(), dragged);
}
