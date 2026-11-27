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
#include <yup_gui_markup/yup_gui_markup.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{
const String kToggleButtonText = "Toggle Me";
const String kToggleEmptyText = "";
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

    toggleButton->setButtonText (kToggleButtonText);
    EXPECT_EQ (kToggleButtonText, toggleButton->getButtonText());

    toggleButton->setButtonText (kToggleEmptyText);
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
    toggleButton->setButtonText (kToggleButtonText);
    toggleButton->setToggleState (true, dontSendNotification);

    EXPECT_EQ (kToggleButtonText, toggleButton->getButtonText());
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
    toggleButton->setButtonText (kToggleButtonText);

    for (int i = 0; i < 5; ++i)
    {
        bool state = (i % 2) == 0;
        toggleButton->setToggleState (state, dontSendNotification);
        EXPECT_EQ (state, toggleButton->getToggleState());
        EXPECT_EQ (kToggleButtonText, toggleButton->getButtonText());
    }
}

TEST_F (ToggleButtonTest, EmptyStringHandling)
{
    toggleButton->setButtonText (kToggleButtonText);
    EXPECT_EQ (kToggleButtonText, toggleButton->getButtonText());

    toggleButton->setButtonText (String());
    EXPECT_TRUE (toggleButton->getButtonText().isEmpty());

    toggleButton->setButtonText ("");
    EXPECT_TRUE (toggleButton->getButtonText().isEmpty());
}

TEST_F (ToggleButtonTest, SetToggleStateUpdatesCheckedBindingsLinksAndStyles)
{
    ApplicationTheme::Ptr oldTheme = ApplicationTheme::getGlobalTheme();
    ApplicationTheme::Ptr theme = new ApplicationTheme();
    ApplicationTheme::setGlobalTheme (theme);

    DataTree params ("params");
    params.beginTransaction().setProperty ("bypass", false);
    DataTreeBindingProvider provider (params);

    YuiDocument document;
    document.setModel ("params", provider);

    const auto result = document.loadFromString (R"yui(
        style {
            ToggleButton:checked { opacity: 0.5; }
            SwitchButton:checked { opacity: 0.25; }
        }
        Column {
            ToggleButton { id: toggle; checked <=> params.bypass; }
            SwitchButton { id: sw; }
            Component { id: panel; enabled: toggle.checked; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* toggle = document.findById<ToggleButton> ("toggle");
    auto* sw = document.findById<SwitchButton> ("sw");
    auto* panel = document.findById ("panel");
    ASSERT_NE (toggle, nullptr);
    ASSERT_NE (sw, nullptr);
    ASSERT_NE (panel, nullptr);

    EXPECT_FALSE (panel->isEnabled());
    EXPECT_FLOAT_EQ (toggle->getOpacity(), 1.0f);

    toggle->setToggleState (true, dontSendNotification);
    document.getRoot()->getStyleContext()->restyleNow();

    EXPECT_TRUE (panel->isEnabled());
    EXPECT_TRUE (static_cast<bool> (params.getProperty ("bypass")));
    EXPECT_NEAR (toggle->getOpacity(), 0.5f, 0.01f);

    sw->setToggleState (true, dontSendNotification);
    document.getRoot()->getStyleContext()->restyleNow();
    EXPECT_NEAR (sw->getOpacity(), 0.25f, 0.01f);

    document.clear();
    ApplicationTheme::setGlobalTheme (oldTheme.get());
}

TEST_F (ToggleButtonTest, OnlyOneButtonOfARadioGroupIsOn)
{
    Component panel;
    Component otherPanel;
    ToggleButton first, second, third, ungrouped, elsewhere;
    SwitchButton switched;

    for (auto* button : { &first, &second, &third })
    {
        button->setRadioGroupId (1);
        panel.addAndMakeVisible (*button);
    }

    switched.setRadioGroupId (1);
    panel.addAndMakeVisible (switched);
    panel.addAndMakeVisible (ungrouped);

    elsewhere.setRadioGroupId (1);
    otherPanel.addAndMakeVisible (elsewhere);

    ungrouped.setToggleState (true);
    elsewhere.setToggleState (true);
    EXPECT_EQ (first.getRadioGroupId(), 1);
    EXPECT_EQ (ungrouped.getRadioGroupId(), 0);

    first.setToggleState (true);
    EXPECT_TRUE (first.getToggleState());

    // Turning one on turns off the others of its group, whatever kind of button they are
    second.setToggleState (true);
    EXPECT_FALSE (first.getToggleState());
    EXPECT_TRUE (second.getToggleState());

    switched.setToggleState (true);
    EXPECT_FALSE (second.getToggleState());
    EXPECT_TRUE (switched.getToggleState());

    third.setToggleState (true);
    EXPECT_FALSE (switched.getToggleState());

    // A button in no group, in another group or in another parent is left as it is
    EXPECT_TRUE (ungrouped.getToggleState());
    EXPECT_TRUE (elsewhere.getToggleState());

    second.setRadioGroupId (2);
    second.setToggleState (true);
    EXPECT_TRUE (third.getToggleState());

    // A button on that joins a group turns off the one that was on in it
    second.setRadioGroupId (1);
    EXPECT_TRUE (second.getToggleState());
    EXPECT_FALSE (third.getToggleState());
}

TEST_F (ToggleButtonTest, AClickLeavesTheButtonOfARadioGroupOn)
{
    Component panel;
    ToggleButton first, second;
    SwitchButton switched;

    panel.setVisible (true);
    panel.setBounds (0.0f, 0.0f, 300.0f, 30.0f);
    first.setBounds (0.0f, 0.0f, 100.0f, 30.0f);
    second.setBounds (100.0f, 0.0f, 100.0f, 30.0f);
    switched.setBounds (200.0f, 0.0f, 100.0f, 30.0f);

    for (auto* button : std::initializer_list<Button*> { &first, &second, &switched })
    {
        button->setRadioGroupId (7);
        panel.addAndMakeVisible (*button);
    }

    PointerSimulator pointer (panel);

    pointer.click ({ 50.0f, 15.0f });
    EXPECT_TRUE (first.getToggleState());

    pointer.click ({ 50.0f, 15.0f });
    EXPECT_TRUE (first.getToggleState());

    pointer.click ({ 250.0f, 15.0f });
    EXPECT_FALSE (first.getToggleState());
    EXPECT_TRUE (switched.getToggleState());

    pointer.click ({ 250.0f, 15.0f });
    EXPECT_TRUE (switched.getToggleState());

    // It can still be turned off by the program, and a button in no group by a click
    switched.setToggleState (false);
    EXPECT_FALSE (switched.getToggleState());

    second.setRadioGroupId (0);
    pointer.click ({ 150.0f, 15.0f });
    pointer.click ({ 150.0f, 15.0f });
    EXPECT_FALSE (second.getToggleState());
}

TEST_F (ToggleButtonTest, AClickRunsOnClickOnce)
{
    Component panel;
    SwitchButton switched;
    int toggleClicks = 0;
    int switchClicks = 0;

    panel.setVisible (true);
    panel.setBounds (0.0f, 0.0f, 200.0f, 30.0f);
    switched.setBounds (100.0f, 0.0f, 100.0f, 30.0f);
    panel.addAndMakeVisible (*toggleButton);
    panel.addAndMakeVisible (switched);

    toggleButton->onClick = [&toggleClicks]
    {
        ++toggleClicks;
    };

    switched.onClick = [&switchClicks]
    {
        ++switchClicks;
    };

    PointerSimulator pointer (panel);

    pointer.click ({ 50.0f, 15.0f });
    EXPECT_EQ (toggleClicks, 1);
    EXPECT_TRUE (toggleButton->getToggleState());

    pointer.click ({ 150.0f, 15.0f });
    EXPECT_EQ (switchClicks, 1);
    EXPECT_TRUE (switched.getToggleState());

    // The button of a radio group that is on stays on, and still tells it was clicked
    toggleButton->setRadioGroupId (3);
    pointer.click ({ 50.0f, 15.0f });
    EXPECT_EQ (toggleClicks, 2);
    EXPECT_TRUE (toggleButton->getToggleState());

    // A disabled button keeps its state
    switched.setEnabled (false);
    pointer.click ({ 150.0f, 15.0f });
    EXPECT_EQ (switchClicks, 1);
    EXPECT_TRUE (switched.getToggleState());
}

TEST_F (ToggleButtonTest, TheRadioGroupIsAPropertyThatNotifies)
{
    struct Recorder : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override { names.add (name.toString()); }

        StringArray names;
    } recorder;

    toggleButton->addComponentListener (&recorder);
    toggleButton->setProperty ("radioGroup", 3);

    EXPECT_EQ (toggleButton->getRadioGroupId(), 3);
    EXPECT_EQ (static_cast<int> (toggleButton->getProperty ("radioGroup")), 3);
    EXPECT_TRUE (recorder.names.contains ("radioGroup"));

    toggleButton->removeComponentListener (&recorder);
}

TEST_F (ToggleButtonTest, AButtonWithAValueIsOnWhileItsSelectionIsThatValue)
{
    struct Recorder : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override { names.add (name.toString()); }

        StringArray names;
    } recorder;

    toggleButton->addComponentListener (&recorder);
    toggleButton->setProperty ("value", 2);
    EXPECT_FALSE (toggleButton->getToggleState());

    // A number equal to the value selects it, whatever kind of number it is
    toggleButton->setProperty ("selection", 2.0);
    EXPECT_TRUE (toggleButton->getToggleState());
    EXPECT_EQ (recorder.names.joinIntoString (" "), "value selection checked");

    toggleButton->setSelection (3);
    EXPECT_FALSE (toggleButton->getToggleState());

    // The button follows its value too
    toggleButton->setValue (3);
    EXPECT_TRUE (toggleButton->getToggleState());

    // A selection of nothing is no value, not even 0
    toggleButton->setValue (0);
    toggleButton->setSelection (0);
    EXPECT_TRUE (toggleButton->getToggleState());

    toggleButton->setSelection (var());
    EXPECT_FALSE (toggleButton->getToggleState());

    // A switch does the same, here with a text
    SwitchButton switched;
    switched.setValue ("saw");
    switched.setSelection ("saw");
    EXPECT_TRUE (switched.getToggleState());

    switched.setSelection ("square");
    EXPECT_FALSE (switched.getToggleState());

    // Without a value the selection is not looked at
    ToggleButton plain;
    plain.setToggleState (true);
    plain.setSelection (1);
    EXPECT_TRUE (plain.getToggleState());

    // And a button that has no checked state is given none
    TextButton momentary;
    momentary.setValue (1);
    momentary.setSelection (1);
    EXPECT_FALSE (momentary.hasProperty ("checked"));

    toggleButton->removeComponentListener (&recorder);
}

TEST_F (ToggleButtonTest, AClickSelectsTheValueOfTheButtonOrItsOffValue)
{
    Component panel;
    SwitchButton switched;
    int toggleClicks = 0;

    panel.setVisible (true);
    panel.setBounds (0.0f, 0.0f, 200.0f, 30.0f);
    switched.setBounds (100.0f, 0.0f, 100.0f, 30.0f);
    panel.addAndMakeVisible (*toggleButton);
    panel.addAndMakeVisible (switched);

    toggleButton->setValue (5);
    switched.setValue ("saw");
    switched.setOffValue ("none");
    EXPECT_EQ (switched.getProperty ("offValue").toString(), "none");

    toggleButton->onClick = [&toggleClicks]
    {
        ++toggleClicks;
    };

    PointerSimulator pointer (panel);

    // A click selects the value of the button, which turns it on
    pointer.click ({ 50.0f, 15.0f });
    EXPECT_EQ (static_cast<int> (toggleButton->getSelection()), 5);
    EXPECT_TRUE (toggleButton->getToggleState());
    EXPECT_EQ (toggleClicks, 1);

    // With no off value the button that is on stays on, and still tells it was clicked
    pointer.click ({ 50.0f, 15.0f });
    EXPECT_TRUE (toggleButton->getToggleState());
    EXPECT_EQ (toggleClicks, 2);

    // With one, a click on the button that is on selects it
    pointer.click ({ 150.0f, 15.0f });
    EXPECT_EQ (switched.getSelection().toString(), "saw");
    EXPECT_TRUE (switched.getToggleState());

    pointer.click ({ 150.0f, 15.0f });
    EXPECT_EQ (switched.getSelection().toString(), "none");
    EXPECT_FALSE (switched.getToggleState());

    // The selection decides: given another one, the button goes off without a click
    toggleButton->setSelection (6);
    EXPECT_FALSE (toggleButton->getToggleState());
}
