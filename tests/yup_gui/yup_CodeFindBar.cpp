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

namespace
{

class CodeFindBarTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();

        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);

        editor = std::make_unique<CodeEditor>();
        editor->setSize (600, 400);
    }

    void TearDown() override
    {
        editor.reset();

        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    CodeFindBar& showFind (const String& text, bool withReplace = false)
    {
        editor->setText (text);
        editor->setCaretPosition (0);
        editor->showFind (withReplace);

        auto* bar = editor->getFindBar();
        EXPECT_NE (nullptr, bar);
        return *bar;
    }

    std::unique_ptr<CodeEditor> editor;
    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

} // namespace

// ==============================================================================
// Showing and hiding
// ==============================================================================

TEST_F (CodeFindBarTest, IsCreatedOnDemand)
{
    EXPECT_EQ (nullptr, editor->getFindBar());

    editor->showFind();

    ASSERT_NE (nullptr, editor->getFindBar());
    EXPECT_TRUE (editor->getFindBar()->isVisible());
    EXPECT_EQ (CodeFindBar::Mode::find, editor->getFindBar()->getMode());
}

TEST_F (CodeFindBarTest, KeyboardShortcutsOpenTheBarInEachMode)
{
    editor->setText ("one two");

    EXPECT_TRUE (editor->keyDown (KeyPress (KeyPress::textFKey, KeyModifiers::commandMask), {}));
    ASSERT_NE (nullptr, editor->getFindBar());
    EXPECT_EQ (CodeFindBar::Mode::find, editor->getFindBar()->getMode());

    EXPECT_TRUE (editor->keyDown (KeyPress (KeyPress::textHKey, KeyModifiers::commandMask), {}));
    EXPECT_EQ (CodeFindBar::Mode::replace, editor->getFindBar()->getMode());

    EXPECT_TRUE (editor->keyDown (KeyPress (KeyPress::textGKey, KeyModifiers::commandMask), {}));
    EXPECT_EQ (CodeFindBar::Mode::goToLine, editor->getFindBar()->getMode());
}

TEST_F (CodeFindBarTest, ReplaceModeIsNotShownWhenReadOnly)
{
    editor->setReadOnly (true);
    editor->showFind (true);

    ASSERT_NE (nullptr, editor->getFindBar());
    EXPECT_EQ (CodeFindBar::Mode::find, editor->getFindBar()->getMode());
}

TEST_F (CodeFindBarTest, SingleLineSelectionSeedsTheSearchText)
{
    editor->setText ("alpha beta\ngamma");
    editor->setSelection ({ 6, 10 });
    editor->showFind();

    EXPECT_EQ (String ("beta"), editor->getFindBar()->getSearchText());
    EXPECT_EQ (Range<int> (6, 10), editor->getSelection());
}

TEST_F (CodeFindBarTest, MultiLineSelectionDoesNotSeedTheSearchText)
{
    editor->setText ("alpha beta\ngamma");
    editor->setSelection ({ 6, 13 });
    editor->showFind();

    EXPECT_TRUE (editor->getFindBar()->getSearchText().isEmpty());
}

TEST_F (CodeFindBarTest, EscapeHidesTheBarAndClearsHighlights)
{
    auto& bar = showFind ("foo bar foo");
    bar.setSearchText ("foo");
    EXPECT_EQ (2u, editor->getSearchMatches().size());

    EXPECT_TRUE (bar.keyDown (KeyPress (KeyPress::escapeKey), {}));

    EXPECT_FALSE (bar.isVisible());
    EXPECT_TRUE (editor->getSearchMatches().empty());
}

TEST_F (CodeFindBarTest, EditorEscapeHidesVisibleBar)
{
    editor->showFind();

    EXPECT_TRUE (editor->keyDown (KeyPress (KeyPress::escapeKey), {}));
    EXPECT_FALSE (editor->getFindBar()->isVisible());

    EXPECT_FALSE (editor->keyDown (KeyPress (KeyPress::escapeKey), {}));
}

TEST_F (CodeFindBarTest, BarSwallowsUnhandledKeys)
{
    auto& bar = showFind ("abc");

    EXPECT_TRUE (bar.keyDown (KeyPress (KeyPress::backspaceKey), {}));
    EXPECT_EQ (String ("abc"), editor->getText());
}

// ==============================================================================
// Searching
// ==============================================================================

TEST_F (CodeFindBarTest, TypingSelectsFirstMatchAndReportsCount)
{
    auto& bar = showFind ("foo bar foo baz foo");
    bar.setSearchText ("foo");

    EXPECT_EQ (Range<int> (0, 3), editor->getSelection());
    EXPECT_EQ (3u, editor->getSearchMatches().size());
    EXPECT_EQ (String ("1 of 3"), bar.getStatusText());
}

TEST_F (CodeFindBarTest, SearchStartsFromCaretPosition)
{
    editor->setText ("foo bar foo");
    editor->setCaretPosition (5);
    editor->showFind();

    editor->getFindBar()->setSearchText ("foo");

    EXPECT_EQ (Range<int> (8, 11), editor->getSelection());
    EXPECT_EQ (String ("2 of 2"), editor->getFindBar()->getStatusText());
}

TEST_F (CodeFindBarTest, EnterAndShiftEnterNavigateMatches)
{
    auto& bar = showFind ("foo bar foo baz foo");
    bar.setSearchText ("foo");

    EXPECT_TRUE (bar.keyDown (KeyPress (KeyPress::enterKey), {}));
    EXPECT_EQ (Range<int> (8, 11), editor->getSelection());

    EXPECT_TRUE (bar.keyDown (KeyPress (KeyPress::f3Key), {}));
    EXPECT_EQ (Range<int> (16, 19), editor->getSelection());

    EXPECT_TRUE (bar.keyDown (KeyPress (KeyPress::enterKey), {}));
    EXPECT_EQ (Range<int> (0, 3), editor->getSelection());

    EXPECT_TRUE (bar.keyDown (KeyPress (KeyPress::enterKey, KeyModifiers::shiftMask), {}));
    EXPECT_EQ (Range<int> (16, 19), editor->getSelection());
    EXPECT_EQ (String ("3 of 3"), bar.getStatusText());
}

TEST_F (CodeFindBarTest, EditorF3UsesLastSearchOfTheBar)
{
    auto& bar = showFind ("foo bar foo");
    bar.setSearchText ("foo");
    bar.hide();

    EXPECT_TRUE (editor->keyDown (KeyPress (KeyPress::f3Key), {}));
    EXPECT_EQ (Range<int> (8, 11), editor->getSelection());
}

TEST_F (CodeFindBarTest, StatusReportsNoResultsAndInvalidPattern)
{
    auto& bar = showFind ("foo bar");

    EXPECT_TRUE (bar.getStatusText().isEmpty());

    bar.setSearchText ("qux");
    EXPECT_EQ (String ("No results"), bar.getStatusText());

    CodeEditor::SearchOptions options;
    options.regex = true;
    bar.setSearchOptions (options);
    bar.setSearchText ("(foo");
    EXPECT_EQ (String ("Invalid pattern"), bar.getStatusText());
}

TEST_F (CodeFindBarTest, OptionsAreAppliedAndToggledWithAltKeys)
{
    auto& bar = showFind ("Foo foo food");
    bar.setSearchText ("foo");
    EXPECT_EQ (3u, editor->getSearchMatches().size());

    EXPECT_TRUE (bar.keyDown (KeyPress (KeyPress::textCKey, KeyModifiers::altMask), {}));
    EXPECT_TRUE (bar.getSearchOptions().caseSensitive);
    EXPECT_EQ (2u, editor->getSearchMatches().size());

    EXPECT_TRUE (bar.keyDown (KeyPress (KeyPress::textWKey, KeyModifiers::altMask), {}));
    EXPECT_TRUE (bar.getSearchOptions().wholeWord);
    EXPECT_EQ (1u, editor->getSearchMatches().size());

    EXPECT_TRUE (bar.keyDown (KeyPress (KeyPress::textRKey, KeyModifiers::altMask), {}));
    EXPECT_TRUE (bar.getSearchOptions().regex);
}

// ==============================================================================
// Replacing
// ==============================================================================

TEST_F (CodeFindBarTest, ReplaceNextReplacesCurrentMatchAndSelectsNext)
{
    auto& bar = showFind ("foo bar foo", true);
    bar.setSearchText ("foo");
    bar.setReplaceText ("x");

    EXPECT_EQ (1, bar.replaceNext());
    EXPECT_EQ (String ("x bar foo"), editor->getText());
    EXPECT_EQ (Range<int> (6, 9), editor->getSelection());

    EXPECT_EQ (1, bar.replaceNext());
    EXPECT_EQ (String ("x bar x"), editor->getText());
    EXPECT_EQ (String ("No results"), bar.getStatusText());
}

TEST_F (CodeFindBarTest, ReplaceAllReplacesEveryMatch)
{
    auto& bar = showFind ("a1 b2 c3", true);

    CodeEditor::SearchOptions options;
    options.regex = true;
    bar.setSearchOptions (options);
    bar.setSearchText ("([a-z])(\\d)");
    bar.setReplaceText ("$2$1");

    EXPECT_EQ (3, bar.replaceAll());
    EXPECT_EQ (String ("1a 2b 3c"), editor->getText());
}

// ==============================================================================
// Go to line
// ==============================================================================

TEST_F (CodeFindBarTest, GoToLineMovesCaretAndClosesTheBar)
{
    editor->setText ("one\ntwo\nthree");
    editor->showGoToLine();

    auto* bar = editor->getFindBar();
    ASSERT_NE (nullptr, bar);
    EXPECT_EQ (String ("Line 1 - 3"), bar->getStatusText());

    bar->setLineText ("3:2");
    EXPECT_TRUE (bar->keyDown (KeyPress (KeyPress::enterKey), {}));

    EXPECT_EQ (9, editor->getCaretPosition());
    EXPECT_FALSE (bar->isVisible());
}

TEST_F (CodeFindBarTest, GoToLineRejectsInvalidInput)
{
    editor->setText ("one\ntwo");
    editor->setCaretPosition (1);
    editor->showGoToLine();

    auto* bar = editor->getFindBar();

    for (const auto* text : { "", "abc", "0", "-1", "2x" })
    {
        bar->setLineText (text);
        EXPECT_FALSE (bar->goToLine()) << text;
        EXPECT_EQ (1, editor->getCaretPosition());
        EXPECT_TRUE (bar->isVisible());
    }
}

TEST_F (CodeFindBarTest, EditorGoToLineClampsLineAndColumn)
{
    editor->setText ("one\ntwo\nthree");

    editor->goToLine (1, 2);
    EXPECT_EQ (6, editor->getCaretPosition());

    editor->goToLine (10, 0);
    EXPECT_EQ (8, editor->getCaretPosition());

    editor->goToLine (0, 99);
    EXPECT_EQ (3, editor->getCaretPosition());

    editor->goToLine (-5, -5);
    EXPECT_EQ (0, editor->getCaretPosition());
}
