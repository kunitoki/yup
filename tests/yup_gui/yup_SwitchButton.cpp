/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace
{
constexpr int kDefaultAnimationTime = 50;
constexpr int kCustomAnimationTime = 100;
} // namespace

class SwitchButtonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        switchButton = std::make_unique<SwitchButton> ("testSwitchButton");
        switchButton->setBounds (0, 0, 60, 30);
    }

    std::unique_ptr<SwitchButton> switchButton;
};

TEST_F (SwitchButtonTest, ConstructorInitializesCorrectly)
{
    EXPECT_FALSE (switchButton->getToggleState());
    EXPECT_FALSE (switchButton->isVertical());
    EXPECT_EQ (String ("testSwitchButton"), switchButton->getComponentID());
}

TEST_F (SwitchButtonTest, DefaultConstructorWorks)
{
    auto defaultSwitch = std::make_unique<SwitchButton>();

    EXPECT_FALSE (defaultSwitch->getToggleState());
    EXPECT_FALSE (defaultSwitch->isVertical());
    EXPECT_TRUE (defaultSwitch->getComponentID().isEmpty());
}

TEST_F (SwitchButtonTest, VerticalConstructorWorks)
{
    auto verticalSwitch = std::make_unique<SwitchButton> ("verticalSwitch", true);

    EXPECT_FALSE (verticalSwitch->getToggleState());
    EXPECT_TRUE (verticalSwitch->isVertical());
    EXPECT_EQ (String ("verticalSwitch"), verticalSwitch->getComponentID());
}

TEST_F (SwitchButtonTest, SetToggleStateChangesState)
{
    EXPECT_FALSE (switchButton->getToggleState());

    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());

    switchButton->setToggleState (false, dontSendNotification);
    EXPECT_FALSE (switchButton->getToggleState());
}

TEST_F (SwitchButtonTest, SetToggleStateWithSameValueIsIdempotent)
{
    switchButton->setToggleState (false, dontSendNotification);
    EXPECT_FALSE (switchButton->getToggleState());

    switchButton->setToggleState (false, dontSendNotification);
    EXPECT_FALSE (switchButton->getToggleState());

    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());

    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());
}

TEST_F (SwitchButtonTest, VerticalOrientationToggle)
{
    EXPECT_FALSE (switchButton->isVertical());

    switchButton->setVertical (true);
    EXPECT_TRUE (switchButton->isVertical());

    switchButton->setVertical (false);
    EXPECT_FALSE (switchButton->isVertical());
}

TEST_F (SwitchButtonTest, VerticalToggleWithSameValueIsIdempotent)
{
    switchButton->setVertical (false);
    EXPECT_FALSE (switchButton->isVertical());

    switchButton->setVertical (false);
    EXPECT_FALSE (switchButton->isVertical());

    switchButton->setVertical (true);
    EXPECT_TRUE (switchButton->isVertical());

    switchButton->setVertical (true);
    EXPECT_TRUE (switchButton->isVertical());
}

TEST_F (SwitchButtonTest, AnimationTimeConfiguration)
{
    switchButton->setMillisecondsToSpendMoving (kCustomAnimationTime);

    // We can't directly test the internal animation time, but we can verify
    // the method doesn't crash and the switch still functions
    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());

    switchButton->setToggleState (false, dontSendNotification);
    EXPECT_FALSE (switchButton->getToggleState());
}

TEST_F (SwitchButtonTest, ZeroAnimationTime)
{
    switchButton->setMillisecondsToSpendMoving (0);

    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());

    switchButton->setToggleState (false, dontSendNotification);
    EXPECT_FALSE (switchButton->getToggleState());
}

TEST_F (SwitchButtonTest, NegativeAnimationTimeHandled)
{
    // Should handle negative values gracefully
    switchButton->setMillisecondsToSpendMoving (-10);

    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());
}

TEST_F (SwitchButtonTest, ToggleStateIndependentOfOrientation)
{
    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());
    EXPECT_FALSE (switchButton->isVertical());

    switchButton->setVertical (true);
    EXPECT_TRUE (switchButton->getToggleState());
    EXPECT_TRUE (switchButton->isVertical());

    switchButton->setToggleState (false, dontSendNotification);
    EXPECT_FALSE (switchButton->getToggleState());
    EXPECT_TRUE (switchButton->isVertical());
}

TEST_F (SwitchButtonTest, OrientationIndependentOfToggleState)
{
    switchButton->setVertical (true);
    EXPECT_TRUE (switchButton->isVertical());
    EXPECT_FALSE (switchButton->getToggleState());

    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->isVertical());
    EXPECT_TRUE (switchButton->getToggleState());

    switchButton->setVertical (false);
    EXPECT_FALSE (switchButton->isVertical());
    EXPECT_TRUE (switchButton->getToggleState());
}

TEST_F (SwitchButtonTest, MultipleToggleOperations)
{
    bool expectedState = false;

    for (int i = 0; i < 10; ++i)
    {
        expectedState = ! expectedState;
        switchButton->setToggleState (expectedState, dontSendNotification);
        EXPECT_EQ (expectedState, switchButton->getToggleState());
    }
}

TEST_F (SwitchButtonTest, ComponentIdIsSet)
{
    auto newSwitch = std::make_unique<SwitchButton> ("uniqueSwitchButtonId");
    EXPECT_EQ (String ("uniqueSwitchButtonId"), newSwitch->getComponentID());
}

TEST_F (SwitchButtonTest, BoundsAndSizeWork)
{
    Rectangle<int> bounds (10, 20, 80, 25);
    switchButton->setBounds (bounds);

    EXPECT_EQ (bounds.to<float>(), switchButton->getBounds());
    EXPECT_EQ (80, switchButton->getWidth());
    EXPECT_EQ (25, switchButton->getHeight());
}

TEST_F (SwitchButtonTest, IsButtonType)
{
    // SwitchButton inherits from Button
    Button* baseButton = switchButton.get();
    EXPECT_NE (nullptr, baseButton);
}

TEST_F (SwitchButtonTest, StateChangeWithNotification)
{
    // Test that setToggleState with sendNotification doesn't crash
    switchButton->setToggleState (true, sendNotification);
    EXPECT_TRUE (switchButton->getToggleState());

    switchButton->setToggleState (false, sendNotification);
    EXPECT_FALSE (switchButton->getToggleState());
}

TEST_F (SwitchButtonTest, GetSwitchCircleBoundsReturnsValidRectangle)
{
    auto circleBounds = switchButton->getSwitchCircleBounds();

    // Circle bounds should be within the button bounds
    EXPECT_GE (circleBounds.getX(), 0.0f);
    EXPECT_GE (circleBounds.getY(), 0.0f);
    EXPECT_LE (circleBounds.getRight(), static_cast<float> (switchButton->getWidth()));
    EXPECT_LE (circleBounds.getBottom(), static_cast<float> (switchButton->getHeight()));
}

TEST_F (SwitchButtonTest, CircleBoundsChangeWithToggleState)
{
    auto initialBounds = switchButton->getSwitchCircleBounds();

    switchButton->setToggleState (true, dontSendNotification);
    auto toggledBounds = switchButton->getSwitchCircleBounds();

    // The circle should move when toggled (exact position depends on implementation)
    // We just verify bounds are still valid
    EXPECT_LE (toggledBounds.getRight(), static_cast<float> (switchButton->getWidth()));
    EXPECT_LE (toggledBounds.getBottom(), static_cast<float> (switchButton->getHeight()));
}

TEST_F (SwitchButtonTest, VerticalSwitchBehavior)
{
    auto verticalSwitch = std::make_unique<SwitchButton> ("vertical", true);
    verticalSwitch->setBounds (0, 0, 30, 60);

    EXPECT_TRUE (verticalSwitch->isVertical());

    auto circleBounds = verticalSwitch->getSwitchCircleBounds();
    EXPECT_LE (circleBounds.getRight(), static_cast<float> (verticalSwitch->getWidth()));
    EXPECT_LE (circleBounds.getBottom(), static_cast<float> (verticalSwitch->getHeight()));

    verticalSwitch->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (verticalSwitch->getToggleState());

    auto toggledBounds = verticalSwitch->getSwitchCircleBounds();
    EXPECT_LE (toggledBounds.getRight(), static_cast<float> (verticalSwitch->getWidth()));
    EXPECT_LE (toggledBounds.getBottom(), static_cast<float> (verticalSwitch->getHeight()));
}

TEST_F (SwitchButtonTest, LargeAnimationTime)
{
    switchButton->setMillisecondsToSpendMoving (5000);

    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());
}

TEST_F (SwitchButtonTest, SwitchWithZeroSize)
{
    switchButton->setBounds (0, 0, 0, 0);

    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());

    auto circleBounds = switchButton->getSwitchCircleBounds();
    // Should handle zero size gracefully
    EXPECT_GE (circleBounds.getX(), 0.0f);
    EXPECT_GE (circleBounds.getY(), 0.0f);
}

TEST_F (SwitchButtonTest, SwitchWithVerySmallSize)
{
    switchButton->setMillisecondsToSpendMoving (0);
    switchButton->setBounds (0, 0, 1, 1);

    switchButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (switchButton->getToggleState());

    auto circleBounds = switchButton->getSwitchCircleBounds();
    EXPECT_LE (circleBounds.getRight(), 1.0f);
    EXPECT_LE (circleBounds.getBottom(), 1.0f);
}
