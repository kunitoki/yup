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
const String kTestButtonText = "Toggle Me";
const String kEmptyText = "";
} // namespace

class ToggleButtonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        toggleButton = std::make_unique<ToggleButton> ("testToggleButton");
        toggleButton->setBounds (0, 0, 100, 30);
    }

    std::unique_ptr<ToggleButton> toggleButton;
};

TEST_F (ToggleButtonTest, ConstructorInitializesCorrectly)
{
    EXPECT_FALSE (toggleButton->getToggleState());
    EXPECT_TRUE (toggleButton->getButtonText().isEmpty());
    EXPECT_EQ (String ("testToggleButton"), toggleButton->getComponentID());
}

TEST_F (ToggleButtonTest, DefaultConstructorWorks)
{
    auto defaultButton = std::make_unique<ToggleButton>();

    EXPECT_FALSE (defaultButton->getToggleState());
    EXPECT_TRUE (defaultButton->getButtonText().isEmpty());
    EXPECT_TRUE (defaultButton->getComponentID().isEmpty());
}

TEST_F (ToggleButtonTest, SetToggleStateChangesState)
{
    EXPECT_FALSE (toggleButton->getToggleState());

    toggleButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (toggleButton->getToggleState());

    toggleButton->setToggleState (false, dontSendNotification);
    EXPECT_FALSE (toggleButton->getToggleState());
}

TEST_F (ToggleButtonTest, SetToggleStateWithSameValueIsIdempotent)
{
    toggleButton->setToggleState (false, dontSendNotification);
    EXPECT_FALSE (toggleButton->getToggleState());

    toggleButton->setToggleState (false, dontSendNotification);
    EXPECT_FALSE (toggleButton->getToggleState());

    toggleButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (toggleButton->getToggleState());

    toggleButton->setToggleState (true, dontSendNotification);
    EXPECT_TRUE (toggleButton->getToggleState());
}

TEST_F (ToggleButtonTest, ButtonTextGetterAndSetter)
{
    EXPECT_TRUE (toggleButton->getButtonText().isEmpty());

    toggleButton->setButtonText (kTestButtonText);
    EXPECT_EQ (kTestButtonText, toggleButton->getButtonText());

    toggleButton->setButtonText (kEmptyText);
    EXPECT_TRUE (toggleButton->getButtonText().isEmpty());
}

TEST_F (ToggleButtonTest, ButtonTextWithSpecialCharacters)
{
    const String specialText = "Special\nText\t&<>";
    toggleButton->setButtonText (specialText);
    EXPECT_EQ (specialText, toggleButton->getButtonText());
}

TEST_F (ToggleButtonTest, ButtonTextWithUnicode)
{
    const String unicodeText = L"Üñíçødé Téxt 🚀";
    toggleButton->setButtonText (unicodeText);
    EXPECT_EQ (unicodeText, toggleButton->getButtonText());
}

TEST_F (ToggleButtonTest, LongButtonText)
{
    String longText;
    for (int i = 0; i < 1000; ++i)
        longText += "A";

    toggleButton->setButtonText (longText);
    EXPECT_EQ (longText, toggleButton->getButtonText());
}

TEST_F (ToggleButtonTest, ToggleStateIndependentOfText)
{
    toggleButton->setButtonText (kTestButtonText);
    toggleButton->setToggleState (true, dontSendNotification);

    EXPECT_EQ (kTestButtonText, toggleButton->getButtonText());
    EXPECT_TRUE (toggleButton->getToggleState());

    toggleButton->setButtonText ("New Text");
    EXPECT_TRUE (toggleButton->getToggleState()); // State should remain

    toggleButton->setToggleState (false, dontSendNotification);
    EXPECT_EQ (String ("New Text"), toggleButton->getButtonText()); // Text should remain
}

TEST_F (ToggleButtonTest, MultipleToggleOperations)
{
    bool expectedState = false;

    for (int i = 0; i < 10; ++i)
    {
        expectedState = ! expectedState;
        toggleButton->setToggleState (expectedState, dontSendNotification);
        EXPECT_EQ (expectedState, toggleButton->getToggleState());
    }
}

TEST_F (ToggleButtonTest, ComponentIdIsSet)
{
    auto newButton = std::make_unique<ToggleButton> ("uniqueToggleButtonId");
    EXPECT_EQ (String ("uniqueToggleButtonId"), newButton->getComponentID());
}

TEST_F (ToggleButtonTest, BoundsAndSizeWork)
{
    Rectangle<int> bounds (10, 20, 80, 25);
    toggleButton->setBounds (bounds);

    EXPECT_EQ (bounds.to<float>(), toggleButton->getBounds());
    EXPECT_EQ (80, toggleButton->getWidth());
    EXPECT_EQ (25, toggleButton->getHeight());
}

TEST_F (ToggleButtonTest, IsButtonType)
{
    // ToggleButton inherits from Button
    Button* baseButton = toggleButton.get();
    EXPECT_NE (nullptr, baseButton);
}

TEST_F (ToggleButtonTest, StateChangeWithNotification)
{
    // Test that setToggleState with sendNotification doesn't crash
    // (We can't easily test the actual notification without a listener)
    toggleButton->setToggleState (true, sendNotification);
    EXPECT_TRUE (toggleButton->getToggleState());

    toggleButton->setToggleState (false, sendNotification);
    EXPECT_FALSE (toggleButton->getToggleState());
}

TEST_F (ToggleButtonTest, StateAfterMultipleTextChanges)
{
    toggleButton->setToggleState (true, dontSendNotification);

    for (int i = 0; i < 5; ++i)
    {
        String text = "Text " + String (i);
        toggleButton->setButtonText (text);
        EXPECT_TRUE (toggleButton->getToggleState());
        EXPECT_EQ (text, toggleButton->getButtonText());
    }
}

TEST_F (ToggleButtonTest, TextAfterMultipleStateChanges)
{
    toggleButton->setButtonText (kTestButtonText);

    for (int i = 0; i < 5; ++i)
    {
        bool state = (i % 2) == 0;
        toggleButton->setToggleState (state, dontSendNotification);
        EXPECT_EQ (state, toggleButton->getToggleState());
        EXPECT_EQ (kTestButtonText, toggleButton->getButtonText());
    }
}

TEST_F (ToggleButtonTest, EmptyStringHandling)
{
    toggleButton->setButtonText (kTestButtonText);
    EXPECT_EQ (kTestButtonText, toggleButton->getButtonText());

    toggleButton->setButtonText (String());
    EXPECT_TRUE (toggleButton->getButtonText().isEmpty());

    toggleButton->setButtonText ("");
    EXPECT_TRUE (toggleButton->getButtonText().isEmpty());
}
