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

/** An editor over a label that is two components deep in a panel, under a pointer. */
class InPlaceTextEditorTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);

        panel.setVisible (true);
        panel.setBounds (0.0f, 0.0f, 300.0f, 200.0f);

        group.setBounds (20.0f, 30.0f, 200.0f, 100.0f);
        panel.addAndMakeVisible (group);

        label = std::make_unique<Label>();
        label->setBounds (10.0f, 40.0f, 80.0f, 20.0f);
        group.addAndMakeVisible (*label);

        other.setBounds (0.0f, 150.0f, 300.0f, 50.0f);
        panel.addAndMakeVisible (other);

        editor.onCommit = [this] (const String& text)
        {
            committed.add (text);
        };
    }

    void TearDown() override
    {
        editor.close();

        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
    Component panel, group, other;
    std::unique_ptr<Label> label;
    PointerSimulator pointer { panel };
    InPlaceTextEditor editor;
    StringArray committed;
};

TEST_F (InPlaceTextEditorTests, OpensOverItsAnchorInTheInterfaceWithItsTextSelected)
{
    EXPECT_FALSE (editor.isOpen());
    EXPECT_EQ (editor.getTextEditor(), nullptr);

    editor.open (*label, "0.5");

    ASSERT_TRUE (editor.isOpen());
    EXPECT_EQ (editor.getAnchor(), label.get());

    auto* text = editor.getTextEditor();
    ASSERT_NE (text, nullptr);

    // In the component the popups of the anchor go to, over the anchor, above what was there
    EXPECT_EQ (text->getParentComponent(), &panel);
    EXPECT_EQ (panel.getChildComponent (panel.getNumChildComponents() - 1), text);
    EXPECT_EQ (text->getBounds(), Rectangle<float> (30.0f, 70.0f, 80.0f, 20.0f));
    EXPECT_TRUE (text->isVisible());
    EXPECT_TRUE (text->isExcludedFromParentLayout());

    EXPECT_FALSE (text->isMultiLine());
    EXPECT_EQ (text->getText(), "0.5");
    EXPECT_EQ (text->getSelectedText(), "0.5");
    EXPECT_TRUE (text->hasStyleClass ("edit"));
}

TEST_F (InPlaceTextEditorTests, OpensInTheParentItIsGiven)
{
    editor.open (*label, "0.5", &group);

    ASSERT_NE (editor.getTextEditor(), nullptr);
    EXPECT_EQ (editor.getTextEditor()->getParentComponent(), &group);
    EXPECT_EQ (editor.getTextEditor()->getBounds(), label->getBounds());
}

TEST_F (InPlaceTextEditorTests, FollowsItsAnchorAndWhatHoldsIt)
{
    editor.open (*label, "0.5");

    label->setBounds (20.0f, 10.0f, 60.0f, 24.0f);
    EXPECT_EQ (editor.getTextEditor()->getBounds(), Rectangle<float> (40.0f, 40.0f, 60.0f, 24.0f));

    group.setBounds (100.0f, 0.0f, 200.0f, 100.0f);
    EXPECT_EQ (editor.getTextEditor()->getBounds(), Rectangle<float> (120.0f, 10.0f, 60.0f, 24.0f));
}

TEST_F (InPlaceTextEditorTests, ReturnCommitsTheTextThatWasTyped)
{
    editor.open (*label, "0.5");
    editor.getTextEditor()->setText ("0.75");

    EXPECT_TRUE (editor.getTextEditor()->keyDown (KeyPress (KeyPress::enterKey), {}));

    EXPECT_FALSE (editor.isOpen());
    EXPECT_EQ (editor.getAnchor(), nullptr);
    EXPECT_EQ (editor.getTextEditor()->getParentComponent(), nullptr);
    EXPECT_EQ (committed.joinIntoString (", "), "0.75");
}

TEST_F (InPlaceTextEditorTests, ATextLeftAsItOpenedIsNotCommitted)
{
    editor.open (*label, "0.5");

    EXPECT_TRUE (editor.getTextEditor()->keyDown (KeyPress (KeyPress::enterKey), {}));

    EXPECT_FALSE (editor.isOpen());
    EXPECT_TRUE (committed.isEmpty());
}

TEST_F (InPlaceTextEditorTests, EscapeDropsWhatWasTyped)
{
    editor.open (*label, "0.5");
    editor.getTextEditor()->setText ("0.75");

    EXPECT_TRUE (editor.getTextEditor()->keyDown (KeyPress (KeyPress::escapeKey), {}));

    EXPECT_FALSE (editor.isOpen());
    EXPECT_TRUE (committed.isEmpty());
}

TEST_F (InPlaceTextEditorTests, APressElsewhereAndTheLossOfTheFocusCommit)
{
    editor.open (*label, "0.5");
    editor.getTextEditor()->setText ("0.75");

    // A press on the editor is one of the edit
    pointer.click ({ 40.0f, 80.0f });
    EXPECT_TRUE (editor.isOpen());
    EXPECT_TRUE (committed.isEmpty());

    pointer.click ({ 150.0f, 175.0f });
    EXPECT_FALSE (editor.isOpen());
    EXPECT_EQ (committed.joinIntoString (", "), "0.75");

    editor.open (*label, "0.75");
    editor.getTextEditor()->setText ("1");
    editor.getTextEditor()->focusLost();

    EXPECT_FALSE (editor.isOpen());
    EXPECT_EQ (committed.joinIntoString (", "), "0.75, 1");

    // Once closed, neither commits again
    editor.getTextEditor()->focusLost();
    pointer.click ({ 150.0f, 175.0f });
    EXPECT_EQ (committed.size(), 2);
}

TEST_F (InPlaceTextEditorTests, ClosesWhenItsAnchorGoesAway)
{
    editor.open (*label, "0.5");
    editor.getTextEditor()->setText ("0.75");

    label.reset();

    EXPECT_FALSE (editor.isOpen());
    EXPECT_EQ (editor.getTextEditor()->getParentComponent(), nullptr);
    EXPECT_TRUE (committed.isEmpty());
}

TEST_F (InPlaceTextEditorTests, OpeningAgainMovesItWithoutCommitting)
{
    Label second;
    second.setBounds (200.0f, 0.0f, 50.0f, 20.0f);
    panel.addAndMakeVisible (second);

    editor.open (*label, "0.5");
    editor.getTextEditor()->setText ("0.75");
    editor.open (second, "12");

    EXPECT_EQ (editor.getAnchor(), &second);
    EXPECT_EQ (editor.getTextEditor()->getText(), "12");
    EXPECT_EQ (editor.getTextEditor()->getBounds(), second.getBounds());
    EXPECT_TRUE (committed.isEmpty());

    // The callback of a commit can open it again
    editor.onCommit = [this] (const String& text)
    {
        committed.add (text);
        editor.open (*label, text);
    };

    editor.getTextEditor()->setText ("13");
    editor.commit();

    EXPECT_EQ (committed.joinIntoString (", "), "13");
    EXPECT_EQ (editor.getAnchor(), label.get());
    EXPECT_EQ (editor.getTextEditor()->getText(), "13");

    editor.close();
}
