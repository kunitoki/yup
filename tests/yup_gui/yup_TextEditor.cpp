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

#include <gtest/gtest.h>

#include <yup_gui/yup_gui.h>

using namespace yup;

namespace
{
constexpr auto kSingleText = "Hello World";
constexpr auto kMultilineText = "Line 1\nLine 2\nLine 3";
} // namespace

class TextEditorTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        editor = std::make_unique<TextEditor> ("testEditor");
    }

    void TearDown() override
    {
        editor.reset();
    }

    std::unique_ptr<TextEditor> editor;
};

TEST_F (TextEditorTests, ConstructorInitializesCorrectly)
{
    EXPECT_TRUE (editor->getText().isEmpty());
    EXPECT_EQ (0, editor->getCaretPosition());
    EXPECT_FALSE (editor->hasSelection());
    EXPECT_FALSE (editor->isMultiLine());
    EXPECT_FALSE (editor->isReadOnly());
}

TEST_F (TextEditorTests, SetTextUpdatesContent)
{
    editor->setText (kSingleText);
    EXPECT_EQ (String (kSingleText), editor->getText());
    EXPECT_EQ (0, editor->getCaretPosition());
}

TEST_F (TextEditorTests, CaretPositionHandling)
{
    editor->setText (kSingleText);

    editor->setCaretPosition (5);
    EXPECT_EQ (5, editor->getCaretPosition());

    // Test bounds checking
    editor->setCaretPosition (-1);
    EXPECT_EQ (0, editor->getCaretPosition());

    editor->setCaretPosition (1000);
    EXPECT_EQ (editor->getText().length(), editor->getCaretPosition());
}

TEST_F (TextEditorTests, SelectionHandling)
{
    editor->setText (kSingleText);

    // Test setting selection
    editor->setSelection (Range<int> (2, 7));
    EXPECT_TRUE (editor->hasSelection());
    EXPECT_EQ (String ("llo W"), editor->getSelectedText());

    // Test select all
    editor->selectAll();
    EXPECT_EQ (String (kSingleText), editor->getSelectedText());

    // Test clearing selection
    editor->setCaretPosition (3);
    EXPECT_FALSE (editor->hasSelection());
}

TEST_F (TextEditorTests, TextInsertion)
{
    editor->setText ("Hello");
    editor->setCaretPosition (5);
    editor->insertText (" World");

    EXPECT_EQ (String ("Hello World"), editor->getText());
    EXPECT_EQ (11, editor->getCaretPosition());
}

TEST_F (TextEditorTests, TextDeletion)
{
    editor->setText (kSingleText);
    editor->setSelection (Range<int> (6, 11)); // Select "World"
    editor->deleteSelectedText();

    EXPECT_EQ (String ("Hello "), editor->getText());
    EXPECT_EQ (6, editor->getCaretPosition());
    EXPECT_FALSE (editor->hasSelection());
}

TEST_F (TextEditorTests, MultiLineMode)
{
    editor->setMultiLine (true);
    EXPECT_TRUE (editor->isMultiLine());

    editor->setText (kMultilineText);
    EXPECT_EQ (String (kMultilineText), editor->getText());
}

TEST_F (TextEditorTests, ReadOnlyMode)
{
    editor->setText (kSingleText);
    editor->setReadOnly (true);
    EXPECT_TRUE (editor->isReadOnly());

    // Text insertion should be ignored
    editor->insertText (" Extra");
    EXPECT_EQ (String (kSingleText), editor->getText());

    // Selection deletion should be ignored
    editor->selectAll();
    editor->deleteSelectedText();
    EXPECT_EQ (String (kSingleText), editor->getText());
}

TEST_F (TextEditorTests, TextInputIgnoredWhenReadOnly)
{
    editor->setText ("Hello");
    editor->setCaretPosition (5);
    editor->setReadOnly (true);

    editor->textInput (" World");

    EXPECT_EQ (String ("Hello"), editor->getText());
}

TEST_F (TextEditorTests, TextInputIgnoredWhenDisabled)
{
    editor->setText ("Hello");
    editor->setCaretPosition (5);
    editor->setEnabled (false);

    editor->textInput (" World");

    EXPECT_EQ (String ("Hello"), editor->getText());
}

TEST_F (TextEditorTests, KeyDownIgnoredWhenDisabled)
{
    editor->setText ("Hello");
    editor->setCaretPosition (5);
    editor->setEnabled (false);

    editor->keyDown (KeyPress (KeyPress::backspaceKey), {});

    EXPECT_EQ (String ("Hello"), editor->getText());
    EXPECT_EQ (5, editor->getCaretPosition());
}

TEST_F (TextEditorTests, FontHandling)
{
    // Test default font
    EXPECT_FALSE (editor->getFont().has_value());

    // Test setting custom font
    Font customFont;
    editor->setFont (customFont);
    EXPECT_TRUE (editor->getFont().has_value());

    // Test resetting font
    editor->resetFont();
    EXPECT_FALSE (editor->getFont().has_value());
}

TEST (TextEditorStaticTests, ColorIdentifiersExist)
{
    // Verify that color identifiers are properly defined
    EXPECT_FALSE (TextEditor::Style::backgroundColorId.toString().isEmpty());
    EXPECT_FALSE (TextEditor::Style::textColorId.toString().isEmpty());
    EXPECT_FALSE (TextEditor::Style::caretColorId.toString().isEmpty());
    EXPECT_FALSE (TextEditor::Style::selectionColorId.toString().isEmpty());
    EXPECT_FALSE (TextEditor::Style::outlineColorId.toString().isEmpty());
    EXPECT_FALSE (TextEditor::Style::focusedOutlineColorId.toString().isEmpty());
}

TEST_F (TextEditorTests, SetTextResetsCaret)
{
    editor->setText ("Hello");
    editor->setCaretPosition (5);
    editor->setText ("New text");
    EXPECT_EQ (0, editor->getCaretPosition());
    EXPECT_FALSE (editor->hasSelection());
}

TEST_F (TextEditorTests, GetSelectionRoundTripsWithSetSelection)
{
    editor->setText (kSingleText);
    editor->setSelection (Range<int> (2, 7));
    EXPECT_EQ (String ("llo W"), editor->getSelectedText());

    auto saved = editor->getSelection();
    editor->setCaretPosition (0);
    editor->setSelection (saved);

    EXPECT_EQ (String ("llo W"), editor->getSelectedText());
    EXPECT_EQ (2, editor->getSelection().getStart());
    EXPECT_EQ (7, editor->getSelection().getEnd());
}

TEST_F (TextEditorTests, InsertTextNotificationCallback)
{
    editor->setText ("Hello");
    editor->setCaretPosition (5);

    int callCount = 0;
    editor->onTextChange = [&callCount]
    {
        ++callCount;
    };

    editor->insertText (" World", sendNotification);
    EXPECT_EQ (1, callCount);

    editor->insertText ("!", dontSendNotification);
    EXPECT_EQ (1, callCount);
}

TEST_F (TextEditorTests, DeleteSelectedTextNotificationCallback)
{
    editor->setText (kSingleText, dontSendNotification);
    editor->selectAll();

    int callCount = 0;
    editor->onTextChange = [&callCount]
    {
        ++callCount;
    };

    editor->deleteSelectedText (sendNotification);
    EXPECT_EQ (1, callCount);

    editor->setText (kSingleText, dontSendNotification);
    editor->selectAll();

    editor->deleteSelectedText (dontSendNotification);
    EXPECT_EQ (1, callCount);
}

TEST_F (TextEditorTests, MoveCaretLeftRight)
{
    editor->setText (kSingleText);
    editor->setCaretPosition (5);

    editor->moveCaretLeft();
    EXPECT_EQ (4, editor->getCaretPosition());
    EXPECT_FALSE (editor->hasSelection());

    editor->moveCaretRight();
    EXPECT_EQ (5, editor->getCaretPosition());

    editor->moveCaretLeft (true);
    EXPECT_EQ (4, editor->getCaretPosition());
    EXPECT_TRUE (editor->hasSelection());
    EXPECT_EQ (String ("o"), editor->getSelectedText());
}

/* TODO: moveCaretToWordEnd/moveCaretToWordStart is private
TEST_F (TextEditorTests, WordNavigation)
{
    editor->setText ("Hello World");
    editor->setCaretPosition (6);

    editor->moveCaretToWordEnd();
    EXPECT_EQ (11, editor->getCaretPosition());

    editor->moveCaretToWordStart();
    EXPECT_EQ (6, editor->getCaretPosition());

    editor->moveCaretToWordStart (true);
    EXPECT_TRUE (editor->hasSelection());
}
*/

TEST_F (TextEditorTests, BackspaceDeleteBehavior)
{
    editor->setText ("Hello");
    editor->setCaretPosition (5);

    editor->keyDown (KeyPress (KeyPress::backspaceKey), {});
    EXPECT_EQ (String ("Hell"), editor->getText());
    EXPECT_EQ (4, editor->getCaretPosition());

    editor->setCaretPosition (0);
    editor->keyDown (KeyPress (KeyPress::deleteKey), {});
    EXPECT_EQ (String ("ell"), editor->getText());
    EXPECT_EQ (0, editor->getCaretPosition());
}

TEST_F (TextEditorTests, WordDeletionBackwardForward)
{
    editor->setText ("Hello World");
    editor->setCaretPosition (11);

    editor->keyDown (KeyPress (KeyPress::backspaceKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (String ("Hello "), editor->getText());

    editor->setCaretPosition (0);
    editor->keyDown (KeyPress (KeyPress::deleteKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (String (""), editor->getText());
}

TEST_F (TextEditorTests, TabKeyIgnoredInSingleLineMode)
{
    editor->setText ("Hello");
    editor->setCaretPosition (5);
    EXPECT_FALSE (editor->isMultiLine());

    editor->keyDown (KeyPress (KeyPress::tabKey), {});
    EXPECT_EQ (String ("Hello"), editor->getText());
}

TEST_F (TextEditorTests, TabKeyInsertsInMultiLineMode)
{
    editor->setMultiLine (true);
    editor->setText ("Hello");
    editor->setCaretPosition (5);

    editor->keyDown (KeyPress (KeyPress::tabKey), {});
    EXPECT_EQ (String ("Hello\t"), editor->getText());
}

TEST_F (TextEditorTests, MoveCaretUpDownMultiLine)
{
    editor->setMultiLine (true);
    editor->setText (kMultilineText);
    editor->setCaretPosition (0);

    editor->moveCaretToEnd();
    EXPECT_EQ (editor->getText().length(), editor->getCaretPosition());

    editor->moveCaretToStart();
    EXPECT_EQ (0, editor->getCaretPosition());
}

TEST_F (TextEditorTests, MoveCaretDownUsesWrappedVisualLines)
{
    editor->setBounds (0.0f, 0.0f, 20.0f, 200.0f);
    editor->setMultiLine (true);
    editor->setText ("abcdefghijklmnopqrstuvwxyz");
    editor->setCaretPosition (0);

    const auto firstLineCaretBounds = editor->getCaretBounds();
    ASSERT_FALSE (firstLineCaretBounds.isEmpty());

    editor->moveCaretDown();

    const auto secondLineCaretBounds = editor->getCaretBounds();
    ASSERT_FALSE (secondLineCaretBounds.isEmpty());

    EXPECT_GT (secondLineCaretBounds.getY(), firstLineCaretBounds.getY());
    EXPECT_GT (editor->getCaretPosition(), 0);
    EXPECT_LT (editor->getCaretPosition(), editor->getText().length());
}

/* TODO: moveCaretToWordEnd/moveCaretToWordStart is private
TEST_F (TextEditorTests, DoubleClickSelectsWord)
{
    editor->setText ("Hello World");
    editor->setCaretPosition (6);

    editor->moveCaretToWordEnd();
    int wordEnd = editor->getCaretPosition();

    editor->setCaretPosition (6);
    editor->moveCaretToWordStart();
    int wordStart = editor->getCaretPosition();

    editor->setSelection (Range<int> (wordStart, wordEnd));
    EXPECT_EQ (String ("World"), editor->getSelectedText());
}
*/
