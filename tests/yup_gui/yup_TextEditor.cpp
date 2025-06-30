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
constexpr auto kTestText = "Hello World";
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
    editor->setText (kTestText);
    EXPECT_EQ (String (kTestText), editor->getText());
    EXPECT_EQ (0, editor->getCaretPosition());
}

TEST_F (TextEditorTests, CaretPositionHandling)
{
    editor->setText (kTestText);

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
    editor->setText (kTestText);

    // Test setting selection
    editor->setSelection (Range<int> (2, 7));
    EXPECT_TRUE (editor->hasSelection());
    EXPECT_EQ (String ("llo W"), editor->getSelectedText());

    // Test select all
    editor->selectAll();
    EXPECT_EQ (String (kTestText), editor->getSelectedText());

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
    editor->setText (kTestText);
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
    editor->setText (kTestText);
    editor->setReadOnly (true);
    EXPECT_TRUE (editor->isReadOnly());

    // Text insertion should be ignored
    editor->insertText (" Extra");
    EXPECT_EQ (String (kTestText), editor->getText());

    // Selection deletion should be ignored
    editor->selectAll();
    editor->deleteSelectedText();
    EXPECT_EQ (String (kTestText), editor->getText());
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
