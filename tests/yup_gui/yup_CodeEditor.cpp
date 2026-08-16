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

class CodeEditorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();

        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

} // namespace

// ==============================================================================
// Construction and document binding
// ==============================================================================

TEST (CodeEditorTest, DefaultConstructorOwnsInternalDocument)
{
    CodeEditor editor;

    ASSERT_NE (nullptr, editor.getDocument());
    EXPECT_TRUE (editor.getText().isEmpty());
}

TEST (CodeEditorTest, BindsToExternalDocument)
{
    CodeDocument document;
    document.setText ("hello");

    CodeEditor editor (document);

    EXPECT_EQ (&document, editor.getDocument());
    EXPECT_EQ (String ("hello"), editor.getText());
}

TEST (CodeEditorTest, SetDocumentSwitchesBinding)
{
    CodeDocument first;
    first.setText ("first");

    CodeDocument second;
    second.setText ("second");

    CodeEditor editor (first);
    EXPECT_EQ (String ("first"), editor.getText());

    editor.setDocument (second);
    EXPECT_EQ (&second, editor.getDocument());
    EXPECT_EQ (String ("second"), editor.getText());
}

// ==============================================================================
// Text editing
// ==============================================================================

TEST (CodeEditorTest, SetTextAndGetText)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("int main() {}");

    EXPECT_EQ (String ("int main() {}"), editor.getText());
    EXPECT_EQ (String ("int main() {}"), document.getText());
}

TEST (CodeEditorTest, InsertTextAtCaret)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("hello world");
    editor.setCaretPosition (5);
    editor.insertText (",");

    EXPECT_EQ (String ("hello, world"), editor.getText());
    EXPECT_EQ (6, editor.getCaretPosition());
}

TEST (CodeEditorTest, InsertTextReplacesSelection)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("hello world");
    editor.setSelection (Range<int> (0, 5));
    editor.insertText ("bye");

    EXPECT_EQ (String ("bye world"), editor.getText());
    EXPECT_EQ (3, editor.getCaretPosition());
    EXPECT_FALSE (editor.hasSelection());
}

TEST (CodeEditorTest, KeyDownInsertsCharacters)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("a");
    editor.setCaretPosition (1);
    editor.keyDown (KeyPress (KeyPress::enterKey), {});
    editor.textInput ("b");

    EXPECT_EQ (String ("a\nb"), editor.getText());
}

TEST (CodeEditorTest, TabInsertsSpaces)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("x");
    editor.setCaretPosition (1);
    editor.keyDown (KeyPress (KeyPress::tabKey), {});

    EXPECT_EQ (String ("x    "), editor.getText());

    editor.setTabWidth (2);
    editor.setText ("x");
    editor.setCaretPosition (1);
    editor.keyDown (KeyPress (KeyPress::tabKey), {});

    EXPECT_EQ (String ("x  "), editor.getText());
}

// ==============================================================================
// Caret movement and selection
// ==============================================================================

TEST (CodeEditorTest, CaretMovementWithArrowKeys)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abc");
    editor.setCaretPosition (2);

    editor.keyDown (KeyPress (KeyPress::rightKey), {});
    EXPECT_EQ (3, editor.getCaretPosition());

    editor.keyDown (KeyPress (KeyPress::leftKey), {});
    editor.keyDown (KeyPress (KeyPress::leftKey), {});
    EXPECT_EQ (1, editor.getCaretPosition());
}

TEST (CodeEditorTest, CaretMovementWithUpAndDownKeys)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("one\ntwo\nthree");
    editor.setCaretPosition (5); // "t|wo" - column 1 of line 1.

    editor.keyDown (KeyPress (KeyPress::upKey), {});
    EXPECT_EQ (1, editor.getCaretPosition()); // "o|ne" - same column, line 0.

    editor.keyDown (KeyPress (KeyPress::downKey), {});
    editor.keyDown (KeyPress (KeyPress::downKey), {});
    EXPECT_EQ (9, editor.getCaretPosition()); // "t|hree" - same column, line 2.

    // Moving up from the first line jumps to the very start of the document; moving
    // down from the last line jumps to the very end, same as the previous glyph-based
    // navigation did at the document's boundaries.
    editor.setCaretPosition (1);
    editor.keyDown (KeyPress (KeyPress::upKey), {});
    EXPECT_EQ (0, editor.getCaretPosition());

    editor.setCaretPosition (9);
    editor.keyDown (KeyPress (KeyPress::downKey), {});
    EXPECT_EQ (document.getNumCharacters(), editor.getCaretPosition());
}

TEST (CodeEditorTest, CaretMovementWithUpAndDownKeysClampsToShorterLine)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("longer line\nx");
    editor.setCaretPosition (9); // Column 9 of line 0.

    editor.keyDown (KeyPress (KeyPress::downKey), {});
    EXPECT_EQ (13, editor.getCaretPosition()); // Clamped to the end of the shorter "x" line.
}

TEST (CodeEditorTest, HomeAndEndKeys)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("line one\nline two");
    editor.setCaretPosition (7);

    editor.keyDown (KeyPress (KeyPress::homeKey), {});
    EXPECT_EQ (0, editor.getCaretPosition());

    editor.keyDown (KeyPress (KeyPress::endKey), {});
    EXPECT_EQ (8, editor.getCaretPosition());
}

TEST (CodeEditorTest, ShiftArrowExtendsSelection)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("hello world");
    editor.setCaretPosition (0);

    editor.keyDown (KeyPress (KeyPress::rightKey, KeyModifiers (KeyModifiers::shiftMask)), {});
    editor.keyDown (KeyPress (KeyPress::rightKey, KeyModifiers (KeyModifiers::shiftMask)), {});
    editor.keyDown (KeyPress (KeyPress::rightKey, KeyModifiers (KeyModifiers::shiftMask)), {});

    EXPECT_TRUE (editor.hasSelection());
    EXPECT_EQ (Range<int> (0, 3), editor.getSelection());
    EXPECT_EQ (String ("hel"), editor.getSelectedText());
}

TEST (CodeEditorTest, SelectAll)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("select me");
    editor.selectAll();

    EXPECT_TRUE (editor.hasSelection());
    EXPECT_EQ (String ("select me"), editor.getSelectedText());
}

// ==============================================================================
// Backspace / delete
// ==============================================================================

TEST (CodeEditorTest, BackspaceDeletesPreviousCharacter)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abc");
    editor.setCaretPosition (2);

    editor.keyDown (KeyPress (KeyPress::backspaceKey), {});

    EXPECT_EQ (String ("ac"), editor.getText());
    EXPECT_EQ (1, editor.getCaretPosition());
}

TEST (CodeEditorTest, DeleteRemovesNextCharacter)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abc");
    editor.setCaretPosition (1);

    editor.keyDown (KeyPress (KeyPress::deleteKey), {});

    EXPECT_EQ (String ("ac"), editor.getText());
    EXPECT_EQ (1, editor.getCaretPosition());
}

TEST (CodeEditorTest, BackspaceWithSelectionRemovesSelection)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("hello world");
    editor.setSelection (Range<int> (0, 5));

    editor.keyDown (KeyPress (KeyPress::backspaceKey), {});

    EXPECT_EQ (String (" world"), editor.getText());
    EXPECT_EQ (0, editor.getCaretPosition());
}

// ==============================================================================
// Undo / redo
// ==============================================================================

TEST (CodeEditorTest, UndoAndRedo)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("hello");
    editor.setCaretPosition (5);
    editor.insertText (" world");

    EXPECT_EQ (String ("hello world"), editor.getText());
    EXPECT_TRUE (editor.canUndo());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("hello"), editor.getText());

    EXPECT_TRUE (editor.redo());
    EXPECT_EQ (String ("hello world"), editor.getText());
}

TEST (CodeEditorTest, ControlZUndoes)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("a");
    editor.setCaretPosition (1);
    editor.insertText ("b");

    editor.keyDown (KeyPress (KeyPress::textZKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String ("a"), editor.getText());
}

// ==============================================================================
// Read-only
// ==============================================================================

TEST (CodeEditorTest, ReadOnlyPreventsEditing)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("original");
    editor.setReadOnly (true);
    EXPECT_TRUE (editor.isReadOnly());

    editor.setCaretPosition (8);
    editor.insertText ("!");
    editor.keyDown (KeyPress (KeyPress::enterKey), {});
    editor.keyDown (KeyPress (KeyPress::backspaceKey), {});

    EXPECT_EQ (String ("original"), editor.getText());
}

// ==============================================================================
// Syntax definitions
// ==============================================================================

TEST (CodeEditorTest, SetSyntaxDefinitionByName)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setSyntaxDefinition ("cpp");
    EXPECT_EQ (String ("C++"), editor.getSyntaxDefinition().getName());
}

TEST (CodeEditorTest, SetSyntaxDefinitionForExtension)
{
    CodeDocument document;
    CodeEditor editor (document);

    EXPECT_TRUE (editor.setSyntaxDefinitionForExtension ("py"));
    EXPECT_EQ (String ("Python"), editor.getSyntaxDefinition().getName());

    EXPECT_FALSE (editor.setSyntaxDefinitionForExtension ("unknown"));
}

TEST (CodeEditorTest, SetSyntaxDefinitionObject)
{
    CodeDocument document;
    CodeEditor editor (document);

    auto& definition = SyntaxDefinition::getBuiltIn ("glsl");
    editor.setSyntaxDefinition (definition);

    EXPECT_EQ (&definition, &editor.getSyntaxDefinition());
}

// ==============================================================================
// Gutter and font
// ==============================================================================

TEST (CodeEditorTest, GutterVisibilityControlsWidth)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("one\ntwo\nthree");

    EXPECT_TRUE (editor.isLineNumbersVisible());
    EXPECT_GT (editor.getGutterWidth(), 0.0f);

    editor.setLineNumbersVisible (false);
    EXPECT_FALSE (editor.isLineNumbersVisible());
    EXPECT_EQ (0.0f, editor.getGutterWidth());
}

TEST (CodeEditorTest, SetAndGetFont)
{
    CodeDocument document;
    CodeEditor editor (document);

    Font font;
    font.setHeight (18.0f);
    editor.setFont (font);

    EXPECT_EQ (18.0f, editor.getFont().getHeight());
}

// ==============================================================================
// Clipboard
// ==============================================================================

TEST (CodeEditorTest, CutRemovesSelection)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("hello world");
    editor.setSelection (Range<int> (0, 5));
    editor.cut();

    EXPECT_EQ (String (" world"), editor.getText());
    EXPECT_EQ (0, editor.getCaretPosition());
}

TEST (CodeEditorTest, UndoAfterMultipleEdits)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("a\nb\nc");
    editor.setCaretPosition (1);
    editor.insertText ("B");

    EXPECT_EQ (String ("aB\nb\nc"), editor.getText());

    editor.undo();
    EXPECT_EQ (String ("a\nb\nc"), editor.getText());
}

// ==============================================================================
// Find / replace
// ==============================================================================

TEST (CodeEditorTest, FindAllReturnsMatches)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("foo bar foo baz foo");

    const auto matches = editor.findAll ("foo");
    ASSERT_EQ (3u, matches.size());
    EXPECT_EQ (Range<int> (0, 3), matches[0]);
    EXPECT_EQ (Range<int> (8, 11), matches[1]);
    EXPECT_EQ (Range<int> (16, 19), matches[2]);
}

TEST (CodeEditorTest, FindAllRespectsCaseSensitivity)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("Foo foo FOO");

    EXPECT_EQ (1u, editor.findAll ("foo", true).size());
    EXPECT_EQ (3u, editor.findAll ("foo", false).size());
}

TEST (CodeEditorTest, FindNextSelectsMatch)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("a b a b a");
    editor.setCaretPosition (0);

    EXPECT_TRUE (editor.findNext ("b"));
    EXPECT_EQ (Range<int> (2, 3), editor.getSelection());

    EXPECT_TRUE (editor.findNext ("b"));
    EXPECT_EQ (Range<int> (6, 7), editor.getSelection());
}

TEST (CodeEditorTest, FindNextWrapsAround)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("x y x");
    editor.setCaretPosition (5);

    EXPECT_TRUE (editor.findNext ("x"));
    EXPECT_EQ (Range<int> (4, 5), editor.getSelection());

    EXPECT_TRUE (editor.findNext ("x"));
    EXPECT_EQ (Range<int> (0, 1), editor.getSelection());
}

TEST (CodeEditorTest, ReplaceAllReplacesEveryMatch)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("foo bar foo");
    const int replaced = editor.replaceAll ("foo", "baz");

    EXPECT_EQ (2, replaced);
    EXPECT_EQ (String ("baz bar baz"), editor.getText());

    // Undo restores the whole batch.
    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("foo bar foo"), editor.getText());
}

TEST (CodeEditorTest, ReplaceNextReplacesCurrentMatch)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("foo foo");
    editor.findNext ("foo");
    EXPECT_EQ (1, editor.replaceNext ("foo", "bar"));
    EXPECT_EQ (String ("bar foo"), editor.getText());
}

TEST (CodeEditorTest, HighlightAndClearSearchMatches)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("a a a");
    editor.highlightSearchMatches ("a");
    editor.clearSearchHighlights();
}

// ==============================================================================
// Bracket matching
// ==============================================================================

TEST (CodeEditorTest, BracketMatchFindsClosingBracket)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("foo(bar)");

    const auto match = editor.getBracketMatch (3);
    ASSERT_TRUE (match.has_value());
    EXPECT_EQ (Range<int> (3, 7), *match);
}

TEST (CodeEditorTest, BracketMatchHandlesNesting)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("f(a, g(b, c), d)");

    const auto match = editor.getBracketMatch (1);
    ASSERT_TRUE (match.has_value());
    EXPECT_EQ (Range<int> (1, 15), *match);
}

TEST (CodeEditorTest, BracketMatchReturnsNulloptForNonBracket)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("plain text");

    EXPECT_FALSE (editor.getBracketMatch (3).has_value());
}

// ==============================================================================
// Auto-indent
// ==============================================================================

TEST (CodeEditorTest, EnterIndentsNewLine)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("    if (x)");
    editor.setCaretPosition (10);
    editor.keyDown (KeyPress (KeyPress::enterKey), {});

    EXPECT_EQ (String ("    if (x)\n    "), editor.getText());
}

// ==============================================================================
// Breakpoints and minimap
// ==============================================================================

TEST (CodeEditorTest, BreakpointLifecycle)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("one\ntwo\nthree");

    EXPECT_FALSE (editor.getBreakpoint (1));

    editor.setBreakpoint (1, true);
    EXPECT_TRUE (editor.getBreakpoint (1));
    EXPECT_FALSE (editor.getBreakpoint (0));

    editor.setBreakpoint (1, false);
    EXPECT_FALSE (editor.getBreakpoint (1));

    editor.setBreakpoint (0, true);
    editor.setBreakpoint (2, true);
    editor.clearBreakpoints();
    EXPECT_FALSE (editor.getBreakpoint (0));
    EXPECT_FALSE (editor.getBreakpoint (2));
}

TEST (CodeEditorTest, MinimapVisibility)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("one\ntwo");

    EXPECT_TRUE (editor.isMinimapVisible());
    editor.setMinimapVisible (false);
    EXPECT_FALSE (editor.isMinimapVisible());
}

// ==============================================================================
// Selection and caret edge cases
// ==============================================================================

TEST (CodeEditorTest, GetSelectionReturnsCaretRangeWhenNothingSelected)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello");

    editor.setCaretPosition (3);
    EXPECT_FALSE (editor.hasSelection());
    EXPECT_EQ (Range<int> (3, 3), editor.getSelection());
    EXPECT_TRUE (editor.getSelectedText().isEmpty());
}

TEST (CodeEditorTest, SetSelectionClampsToDocumentBounds)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");

    // Range<int> normalizes start/end at construction, so inverted ranges can't be
    // expressed; setSelection still clamps out-of-bounds positions to the document.
    editor.setSelection (Range<int> (-3, 999));
    EXPECT_TRUE (editor.hasSelection());
    EXPECT_EQ (Range<int> (0, 11), editor.getSelection());
    EXPECT_EQ (String ("hello world"), editor.getSelectedText());

    editor.setSelection (Range<int> (5, 999));
    EXPECT_EQ (Range<int> (5, 11), editor.getSelection());
    EXPECT_EQ (String (" world"), editor.getSelectedText());
}

TEST (CodeEditorTest, SetCaretPositionClampsToDocumentBounds)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello");

    editor.setCaretPosition (-5);
    EXPECT_EQ (0, editor.getCaretPosition());

    editor.setCaretPosition (999);
    EXPECT_EQ (5, editor.getCaretPosition());
}

TEST (CodeEditorTest, InsertTextEmptyIsNoOp)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello");
    editor.setCaretPosition (2);

    editor.insertText ("");
    EXPECT_EQ (String ("hello"), editor.getText());
    EXPECT_EQ (2, editor.getCaretPosition());
}

TEST (CodeEditorTest, CopyWithNoSelectionDoesNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello");

    editor.copy(); // must not crash or change anything
    EXPECT_EQ (String ("hello"), editor.getText());
}

TEST (CodeEditorTest, UndoAndRedoWithEmptyHistoryReturnFalse)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello");

    EXPECT_FALSE (editor.undo());
    EXPECT_FALSE (editor.redo());
}

// ==============================================================================
// Find / replace edge cases
// ==============================================================================

TEST (CodeEditorTest, FindAllWholeWord)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo foobar bar foo");

    const auto matches = editor.findAll ("foo", false, true);
    ASSERT_EQ (2u, matches.size());
    EXPECT_EQ (Range<int> (0, 3), matches[0]);
    EXPECT_EQ (Range<int> (15, 18), matches[1]);
}

TEST (CodeEditorTest, FindAllEmptySearchReturnsNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abc");

    EXPECT_TRUE (editor.findAll ("").empty());
}

TEST (CodeEditorTest, FindNextWithNoMatchesReturnsFalse)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abc");

    EXPECT_FALSE (editor.findNext ("zzz"));
    EXPECT_FALSE (editor.hasSelection());
}

TEST (CodeEditorTest, FindNextWithoutWrapReturnsFalseAtEnd)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("x y x");
    editor.setCaretPosition (5);

    EXPECT_TRUE (editor.findNext ("x", false, false));
    EXPECT_EQ (Range<int> (4, 5), editor.getSelection());

    EXPECT_FALSE (editor.findNext ("x", false, false));
    EXPECT_FALSE (editor.hasSelection());
    EXPECT_EQ (Range<int> (5, 5), editor.getSelection());
}

TEST (CodeEditorTest, FindPreviousFindsPriorMatch)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a b a b a");
    editor.setCaretPosition (7);

    EXPECT_TRUE (editor.findPrevious ("b"));
    EXPECT_EQ (Range<int> (6, 7), editor.getSelection());

    EXPECT_TRUE (editor.findPrevious ("b"));
    EXPECT_EQ (Range<int> (2, 3), editor.getSelection());
}

TEST (CodeEditorTest, FindPreviousWrapsAround)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a b a");
    editor.setCaretPosition (0);

    EXPECT_TRUE (editor.findPrevious ("a"));
    EXPECT_EQ (Range<int> (4, 5), editor.getSelection());
}

TEST (CodeEditorTest, ReplaceNextWithNonMatchingSelectionReturnsZero)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo bar");
    editor.setSelection (Range<int> (4, 7)); // "bar"

    EXPECT_EQ (0, editor.replaceNext ("foo", "X"));
    EXPECT_EQ (String ("foo bar"), editor.getText());
    EXPECT_TRUE (editor.hasSelection());
}

TEST (CodeEditorTest, ReplaceAllEmptySearchReturnsZero)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abc");

    EXPECT_EQ (0, editor.replaceAll ("", "X"));
    EXPECT_EQ (String ("abc"), editor.getText());
}

TEST (CodeEditorTest, ReplaceAllWithEmptyReplacementDeletesMatches)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("aXbXc");

    EXPECT_EQ (2, editor.replaceAll ("X", ""));
    EXPECT_EQ (String ("abc"), editor.getText());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("aXbXc"), editor.getText());
}

// ==============================================================================
// Bracket matching edge cases
// ==============================================================================

TEST (CodeEditorTest, BracketMatchOnClosingBracket)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo(bar)");

    const auto match = editor.getBracketMatch (7);
    ASSERT_TRUE (match.has_value());
    EXPECT_EQ (Range<int> (3, 7), *match);
}

TEST (CodeEditorTest, BracketMatchOnCurlyAndSquareBrackets)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("{ [a] }");

    const auto curly = editor.getBracketMatch (0);
    ASSERT_TRUE (curly.has_value());
    EXPECT_EQ (Range<int> (0, 6), *curly);

    const auto square = editor.getBracketMatch (2);
    ASSERT_TRUE (square.has_value());
    EXPECT_EQ (Range<int> (2, 4), *square);
}

TEST (CodeEditorTest, BracketMatchReturnsNulloptForUnmatchedBracket)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo(bar");

    EXPECT_FALSE (editor.getBracketMatch (3).has_value());
}

TEST (CodeEditorTest, BracketMatchReturnsNulloptForEmptyDocument)
{
    CodeDocument document;
    CodeEditor editor (document);

    EXPECT_FALSE (editor.getBracketMatch (0).has_value());
}

TEST (CodeEditorTest, BracketMatchCacheIsInvalidatedByEdits)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo(bar)");

    ASSERT_TRUE (editor.getBracketMatch (3).has_value());

    editor.setText ("foo bar");
    EXPECT_FALSE (editor.getBracketMatch (3).has_value());
}

// ==============================================================================
// Document binding and scroll
// ==============================================================================

TEST (CodeEditorTest, SetDocumentResetsCaretSelectionAndScroll)
{
    CodeDocument first;
    first.setText ("one");
    CodeDocument second;
    second.setText ("two");

    CodeEditor editor (first);
    editor.setCaretPosition (3);
    editor.setSelection (Range<int> (0, 2));
    editor.setScrollOffset (Point<float> (50.0f, 40.0f));

    editor.setDocument (second);

    EXPECT_EQ (&second, editor.getDocument());
    EXPECT_EQ (0, editor.getCaretPosition());
    EXPECT_FALSE (editor.hasSelection());
    EXPECT_EQ (Point<float> (0.0f, 0.0f), editor.getScrollOffset());
}

TEST (CodeEditorTest, ExternalDocumentEditsAreReflected)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abc");

    document.insertText (CodeDocument::Position (document, 0, 3), "def");
    EXPECT_EQ (String ("abcdef"), editor.getText());
}

// ==============================================================================
// Tab / indent / word-deletion edges
// ==============================================================================

TEST (CodeEditorTest, SetTabWidthClampsToAtLeastOne)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("x");
    editor.setCaretPosition (1);

    editor.setTabWidth (0);
    editor.keyDown (KeyPress (KeyPress::tabKey), {});

    EXPECT_EQ (String ("x "), editor.getText());
}

TEST (CodeEditorTest, EnterReindentsFromTabIndentedLine)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("\tif (x)");
    editor.setCaretPosition (7);
    editor.keyDown (KeyPress (KeyPress::enterKey), {});

    EXPECT_EQ (String ("\tif (x)\n\t"), editor.getText());
}

TEST (CodeEditorTest, DeleteAtEndOfDocumentDoesNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abc");
    editor.setCaretPosition (3);

    editor.keyDown (KeyPress (KeyPress::deleteKey), {});
    EXPECT_EQ (String ("abc"), editor.getText());
}

TEST (CodeEditorTest, BackspaceAtStartOfDocumentDoesNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abc");
    editor.setCaretPosition (0);

    editor.keyDown (KeyPress (KeyPress::backspaceKey), {});
    EXPECT_EQ (String ("abc"), editor.getText());
}

TEST (CodeEditorTest, WordDeletionWithControlBackspace)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setCaretPosition (11);

    editor.keyDown (KeyPress (KeyPress::backspaceKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (String ("hello "), editor.getText());
    EXPECT_EQ (6, editor.getCaretPosition());
}

TEST (CodeEditorTest, ControlDeleteRemovesNextWord)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setCaretPosition (0);

    editor.keyDown (KeyPress (KeyPress::deleteKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (String (" world"), editor.getText());
}

// ==============================================================================
// Read-only text input and breakpoint clamping
// ==============================================================================

TEST (CodeEditorTest, ReadOnlyBlocksTextInput)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a");
    editor.setReadOnly (true);

    editor.textInput ("b");
    EXPECT_EQ (String ("a"), editor.getText());
}

TEST (CodeEditorTest, BreakpointOnOutOfRangeLineClamps)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo");

    editor.setBreakpoint (99, true);
    EXPECT_TRUE (editor.getBreakpoint (1));
    EXPECT_FALSE (editor.getBreakpoint (0));

    editor.setBreakpoint (-1, false); // clamps to line 0, which has no breakpoint
    EXPECT_FALSE (editor.getBreakpoint (0));
}

// ==============================================================================
// Scheme
// ==============================================================================

TEST (CodeEditorTest, DefaultSchemeIsApplied)
{
    CodeEditor editor;

    const auto background = editor.getScheme().getColor (CodeEditorScheme::ColorId::background);
    ASSERT_TRUE (background.has_value());
    EXPECT_EQ (Color (0xff1e1e1e), *background);
}

TEST (CodeEditorTest, SetAndGetScheme)
{
    CodeEditor editor;

    editor.setScheme (CodeEditorScheme::getBuiltIn ("monokai"));

    const auto background = editor.getScheme().getColor (CodeEditorScheme::ColorId::background);
    ASSERT_TRUE (background.has_value());
    EXPECT_EQ (Color (0xff272822), *background);
}

TEST (CodeEditorTest, SettingSchemeUpdatesTokenColors)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("int main() {}");

    editor.setScheme (CodeEditorScheme::getBuiltIn ("solarizedDark"));

    const auto keywordColor = editor.getScheme().getColor (SyntaxDefinition::TokenType::keyword);
    ASSERT_TRUE (keywordColor.has_value());
    EXPECT_EQ (Color (0xff859900), *keywordColor);
}

// ==============================================================================
// Scrollbar
// ==============================================================================

namespace
{

ScrollBar* findVerticalScrollBar (CodeEditor& editor)
{
    for (int i = 0; i < editor.getNumChildComponents(); ++i)
    {
        if (auto* scrollBar = dynamic_cast<ScrollBar*> (editor.getChildComponent (i)))
            return scrollBar;
    }

    return nullptr;
}

} // namespace

TEST (CodeEditorTest, ScrollBarIsHiddenWhenContentFits)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");
    editor.setSize (400, 400);

    auto* scrollBar = findVerticalScrollBar (editor);
    ASSERT_NE (nullptr, scrollBar);
    EXPECT_FALSE (scrollBar->isVisible());
}

TEST (CodeEditorTest, ScrollBarAppearsWhenLinesOverflow)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 200; ++i)
        text += "line " + String (i) + "\n";

    editor.setText (text);
    editor.setSize (400, 200);

    auto* scrollBar = findVerticalScrollBar (editor);
    ASSERT_NE (nullptr, scrollBar);
    EXPECT_TRUE (scrollBar->isVisible());
}

TEST (CodeEditorTest, ScrollBarTracksScrollOffset)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 200; ++i)
        text += "line " + String (i) + "\n";

    editor.setText (text);
    editor.setSize (400, 200);

    auto* scrollBar = findVerticalScrollBar (editor);
    ASSERT_NE (nullptr, scrollBar);
    ASSERT_TRUE (scrollBar->isVisible());

    editor.setScrollOffset (Point<float> (0.0f, 100.0f));

    EXPECT_GT (scrollBar->getCurrentRangeStart(), 0.0);
}

TEST (CodeEditorTest, ScrollBarScrollingMovesEditorScrollOffset)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 200; ++i)
        text += "line " + String (i) + "\n";

    editor.setText (text);
    editor.setSize (400, 200);

    auto* scrollBar = findVerticalScrollBar (editor);
    ASSERT_NE (nullptr, scrollBar);
    ASSERT_TRUE (scrollBar->isVisible());

    scrollBar->setCurrentRangeStart (50.0, sendNotification);

    EXPECT_FLOAT_EQ (50.0f, editor.getScrollOffset().getY());
}

TEST (CodeEditorTest, ScrollBarIsLeftOfMinimap)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 200; ++i)
        text += "line " + String (i) + "\n";

    editor.setText (text);
    editor.setSize (400, 200);

    auto* scrollBar = findVerticalScrollBar (editor);
    ASSERT_NE (nullptr, scrollBar);
    ASSERT_TRUE (scrollBar->isVisible());

    // The minimap (60px) keeps the far-right edge; the scrollbar must sit to its
    // left, flush against the minimap's left edge at most.
    EXPECT_LE (scrollBar->getBounds().getRight(), editor.getWidth() - 60.0f);
    EXPECT_LT (scrollBar->getBounds().getX(), editor.getWidth() - 60.0f);
}
