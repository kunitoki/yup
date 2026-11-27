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
#include <yup_graphics/yup_graphics.h>

#include <gtest/gtest.h>

using namespace yup;

namespace yup
{

template <>
class ComponentTestHelper<CodeEditor>
{
public:
    static void triggerMouseWheel (Component& component, const MouseEvent& event, const MouseWheelData& wheelData)
    {
        component.internalMouseWheel (event, wheelData);
    }
};

} // namespace yup

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

TEST (CodeEditorTest, TellsADrawingWhereItsTextIsAndPaintsItselfAsOnePart)
{
    CodeEditor editor;
    editor.setBounds (0.0f, 0.0f, 320.0f, 200.0f);

    // What an editor shows is one part, painted as the theme paints it: a drawing frames it
    const auto* content = editor.getTypeInfo().findPart (Identifier ("content"));
    ASSERT_NE (content, nullptr);
    EXPECT_FALSE (content->isPlaceable);
    EXPECT_EQ (content->getArea (editor), editor.getLocalBounds());

    EXPECT_EQ (VariantConverter<Rectangle<float>>::fromVar (editor.getProperty (Identifier ("textBounds"))), editor.getTextBounds());
    EXPECT_FLOAT_EQ (static_cast<float> (editor.getProperty (Identifier ("gutterWidth"))), editor.getGutterWidth());
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

TEST (CodeEditorTest, KeyDownClaimsEditingKeysButNotEscapeOrUnknownShortcuts)
{
    CodeDocument document;
    CodeEditor editor (document);

    EXPECT_TRUE (editor.keyDown (KeyPress ('a'), {}));
    EXPECT_TRUE (editor.keyDown (KeyPress (KeyPress::enterKey), {}));
    EXPECT_TRUE (editor.keyDown (KeyPress (KeyPress::textZKey, KeyModifiers::commandMask), {}));

    EXPECT_FALSE (editor.keyDown (KeyPress (KeyPress::escapeKey), {}));
    EXPECT_FALSE (editor.keyDown (KeyPress (KeyPress::textQKey, KeyModifiers::commandMask), {}));
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

TEST (CodeEditorTest, TabIndentsFullySelectedLine)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abc\ndef");
    editor.setSelection (Range<int> (0, 3));
    editor.keyDown (KeyPress (KeyPress::tabKey), {});

    EXPECT_EQ (String ("    abc\ndef"), editor.getText());
    EXPECT_EQ (Range<int> (0, 7), editor.getSelection());
}

TEST (CodeEditorTest, TabIndentsAllLinesTouchedBySelection)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abc\ndef\n\nghi\njkl");
    editor.setSelection (Range<int> (0, 10)); // "abc" .. "g" of "ghi"
    editor.keyDown (KeyPress (KeyPress::tabKey), {});

    EXPECT_EQ (String ("    abc\n    def\n\n    ghi\njkl"), editor.getText());
    EXPECT_EQ (Range<int> (0, 22), editor.getSelection());
}

TEST (CodeEditorTest, TabSkipsLineWhereSelectionEndsAtColumnZero)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abc\ndef\nghi");
    editor.setSelection (Range<int> (0, 8));
    editor.keyDown (KeyPress (KeyPress::tabKey), {});

    EXPECT_EQ (String ("    abc\n    def\nghi"), editor.getText());
    EXPECT_EQ (Range<int> (0, 16), editor.getSelection());
}

TEST (CodeEditorTest, ShiftTabOutdentsSelectedLines)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("      abc\n\tdef\n  ghi\njkl");
    editor.setSelection (Range<int> (2, 18));
    editor.keyDown (KeyPress (KeyPress::tabKey, KeyModifiers (KeyModifiers::shiftMask)), {});

    EXPECT_EQ (String ("  abc\ndef\nghi\njkl"), editor.getText());
    EXPECT_EQ (Range<int> (0, 11), editor.getSelection());
}

TEST (CodeEditorTest, ShiftTabWithoutSelectionOutdentsCaretLine)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("x\n        abc");
    editor.setCaretPosition (12);
    editor.keyDown (KeyPress (KeyPress::tabKey, KeyModifiers (KeyModifiers::shiftMask)), {});

    EXPECT_EQ (String ("x\n    abc"), editor.getText());
    EXPECT_EQ (8, editor.getCaretPosition());
    EXPECT_FALSE (editor.hasSelection());
}

TEST (CodeEditorTest, IndentIsUndoneInOneStepRestoringSelection)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abc\ndef");
    editor.setSelection (Range<int> (1, 6));
    editor.keyDown (KeyPress (KeyPress::tabKey), {});

    ASSERT_EQ (String ("    abc\n    def"), editor.getText());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("abc\ndef"), editor.getText());
    EXPECT_EQ (Range<int> (1, 6), editor.getSelection());

    EXPECT_TRUE (editor.redo());
    EXPECT_EQ (String ("    abc\n    def"), editor.getText());
    EXPECT_EQ (Range<int> (5, 14), editor.getSelection());
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

TEST (CodeEditorTest, UndoRestoresSelectionReplacedByTyping)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("hello world");
    editor.setSelection (Range<int> (6, 11));
    editor.insertText ("there");

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("hello world"), editor.getText());
    EXPECT_EQ (Range<int> (6, 11), editor.getSelection());
    EXPECT_EQ (11, editor.getCaretPosition());

    EXPECT_TRUE (editor.redo());
    EXPECT_EQ (String ("hello there"), editor.getText());
    EXPECT_EQ (11, editor.getCaretPosition());
    EXPECT_FALSE (editor.hasSelection());
}

TEST (CodeEditorTest, UndoRestoresCaretBeforeDeletion)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abc\ndef");
    editor.setCaretPosition (2);

    editor.keyDown (KeyPress (KeyPress::deleteKey), {});
    ASSERT_EQ (String ("ab\ndef"), editor.getText());

    editor.setCaretPosition (6);

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("abc\ndef"), editor.getText());
    EXPECT_EQ (2, editor.getCaretPosition());
}

TEST (CodeEditorTest, UndoKeepsSelectionAnchorDirection)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abcdef");
    editor.setCaretPosition (4);
    editor.keyDown (KeyPress (KeyPress::leftKey, KeyModifiers (KeyModifiers::shiftMask)), {});
    editor.keyDown (KeyPress (KeyPress::leftKey, KeyModifiers (KeyModifiers::shiftMask)), {});
    ASSERT_EQ (Range<int> (2, 4), editor.getSelection());
    ASSERT_EQ (2, editor.getCaretPosition());

    editor.insertText ("X");

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (Range<int> (2, 4), editor.getSelection());
    EXPECT_EQ (2, editor.getCaretPosition());
}

TEST (CodeEditorTest, TypingIsUndoneWordByWord)
{
    CodeDocument document;
    CodeEditor editor (document);

    for (const auto character : String ("hello world"))
        editor.textInput (String::charToString (character));

    ASSERT_EQ (String ("hello world"), editor.getText());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("hello"), editor.getText());
    EXPECT_EQ (5, editor.getCaretPosition());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String(), editor.getText());
}

TEST (CodeEditorTest, CaretMoveStartsNewUndoStep)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.textInput ("a");
    editor.textInput ("b");
    editor.keyDown (KeyPress (KeyPress::leftKey), {});
    editor.textInput ("X");

    ASSERT_EQ (String ("aXb"), editor.getText());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("ab"), editor.getText());
    EXPECT_EQ (1, editor.getCaretPosition());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String(), editor.getText());
}

TEST (CodeEditorTest, ConsecutiveBackspacesAreUndoneTogether)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("abcd");
    editor.setCaretPosition (4);

    for (int i = 0; i < 3; ++i)
        editor.keyDown (KeyPress (KeyPress::backspaceKey), {});

    ASSERT_EQ (String ("a"), editor.getText());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("abcd"), editor.getText());
    EXPECT_EQ (4, editor.getCaretPosition());
    EXPECT_FALSE (editor.canUndo());
}

TEST (CodeEditorTest, CommandsAreSeparateUndoSteps)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.textInput ("a");
    editor.insertText ("bc");
    editor.textInput ("d");
    editor.keyDown (KeyPress (KeyPress::enterKey), {});

    ASSERT_EQ (String ("abcd\n"), editor.getText());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("abcd"), editor.getText());
    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("abc"), editor.getText());
    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("a"), editor.getText());
    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String(), editor.getText());
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

TEST (CodeEditorTest, BracketMatchSpansLinesInBothDirections)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("{\n  [x]\n}");

    EXPECT_EQ (Range<int> (0, 8), editor.getBracketMatch (0).value_or (Range<int>()));
    EXPECT_EQ (Range<int> (0, 8), editor.getBracketMatch (8).value_or (Range<int>()));
    EXPECT_EQ (Range<int> (4, 6), editor.getBracketMatch (6).value_or (Range<int>()));
}

TEST (CodeEditorTest, BracketMatchUsesDefinitionPairs)
{
    SyntaxDefinition definition;
    ASSERT_TRUE (definition.loadFromData (R"({ "name": "Angles", "brackets": ["<>"] })").wasOk());

    CodeDocument document;
    CodeEditor editor (document);
    editor.setSyntaxDefinition (definition);
    editor.setText ("<a(b)>");

    EXPECT_EQ (Range<int> (0, 5), editor.getBracketMatch (0).value_or (Range<int>()));
    EXPECT_FALSE (editor.getBracketMatch (2).has_value());
}

TEST (CodeEditorTest, CrlfDocumentKeepsIndicesAndLineEndings)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("foo\r\nbar foo");

    const auto matches = editor.findAll ("foo");
    ASSERT_EQ (2u, matches.size());
    EXPECT_EQ (Range<int> (8, 11), matches[1]);

    editor.setSelection ({ 2, 6 });
    EXPECT_EQ (String ("o\nba"), editor.getSelectedText());

    editor.setCaretPosition (11);
    editor.insertText ("!");
    EXPECT_EQ (String ("foo\r\nbar foo!"), editor.getText());
}

TEST (CodeEditorTest, FindAllMatchesAcrossLinesWhenSearchContainsNewLine)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("a;\nb;\na;\nb;");

    const auto matches = editor.findAll ("a;\nb");
    ASSERT_EQ (2u, matches.size());
    EXPECT_EQ (Range<int> (6, 10), matches[1]);
}

// ==============================================================================
// Line commands
// ==============================================================================

TEST (CodeEditorTest, ToggleLineCommentCommentsAndUncommentsAtMinimumIndent)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setSyntaxDefinition ("cpp");

    editor.setText ("  a;\n\n    b;");
    editor.setSelection ({ 0, 11 });
    editor.toggleLineComment();

    EXPECT_EQ (String ("  // a;\n\n  //   b;"), editor.getText());
    EXPECT_EQ (Range<int> (0, 17), editor.getSelection());

    editor.keyDown (KeyPress (KeyPress::slashKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (String ("  a;\n\n    b;"), editor.getText());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("  // a;\n\n  //   b;"), editor.getText());
}

TEST (CodeEditorTest, ToggleLineCommentUncommentsOnlyWhenAllLinesAreCommented)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setSyntaxDefinition ("python");

    editor.setText ("# a\nb");
    editor.selectAll();
    editor.toggleLineComment();

    EXPECT_EQ (String ("# # a\n# b"), editor.getText());
}

TEST (CodeEditorTest, ToggleBlockCommentWrapsAndUnwrapsSelection)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setSyntaxDefinition ("cpp");

    editor.setText ("f (x, y);");
    editor.setSelection ({ 3, 4 });
    editor.toggleBlockComment();

    EXPECT_EQ (String ("f (/* x */, y);"), editor.getText());
    EXPECT_EQ (Range<int> (3, 10), editor.getSelection());

    editor.toggleBlockComment();
    EXPECT_EQ (String ("f (x, y);"), editor.getText());
    EXPECT_EQ (Range<int> (3, 4), editor.getSelection());
}

TEST (CodeEditorTest, ToggleBlockCommentWithoutSelectionUsesLineText)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setSyntaxDefinition ("cpp");

    editor.setText ("    call();");
    editor.setCaretPosition (6);
    editor.keyDown (KeyPress (KeyPress::textAKey, KeyModifiers (KeyModifiers::altMask | KeyModifiers::shiftMask)), {});

    EXPECT_EQ (String ("    /* call(); */"), editor.getText());
}

TEST (CodeEditorTest, MoveSelectedLinesUpAndDown)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("a\nbb\ncc\nd");
    editor.setSelection ({ 3, 6 });
    editor.keyDown (KeyPress (KeyPress::upKey, KeyModifiers (KeyModifiers::altMask)), {});

    EXPECT_EQ (String ("bb\ncc\na\nd"), editor.getText());
    EXPECT_EQ (Range<int> (1, 4), editor.getSelection());

    editor.moveSelectedLines (true);
    EXPECT_EQ (String ("bb\ncc\na\nd"), editor.getText());

    editor.keyDown (KeyPress (KeyPress::downKey, KeyModifiers (KeyModifiers::altMask)), {});
    editor.moveSelectedLines (false);

    EXPECT_EQ (String ("a\nd\nbb\ncc"), editor.getText());
    EXPECT_EQ (Range<int> (5, 8), editor.getSelection());

    editor.moveSelectedLines (false);
    EXPECT_EQ (String ("a\nd\nbb\ncc"), editor.getText());
}

TEST (CodeEditorTest, DuplicateSelectedLines)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("ab\ncd");
    editor.setCaretPosition (1);
    editor.keyDown (KeyPress (KeyPress::downKey, KeyModifiers (KeyModifiers::altMask | KeyModifiers::shiftMask)), {});

    EXPECT_EQ (String ("ab\nab\ncd"), editor.getText());
    EXPECT_EQ (4, editor.getCaretPosition());

    editor.duplicateSelectedLines (false);
    EXPECT_EQ (String ("ab\nab\nab\ncd"), editor.getText());
    EXPECT_EQ (4, editor.getCaretPosition());
}

TEST (CodeEditorTest, DeleteSelectedLines)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("one\ntwo\nthree");
    editor.setCaretPosition (6);
    editor.keyDown (KeyPress (KeyPress::textKKey, KeyModifiers (KeyModifiers::controlMask | KeyModifiers::shiftMask)), {});

    EXPECT_EQ (String ("one\nthree"), editor.getText());
    EXPECT_EQ (6, editor.getCaretPosition());

    editor.setCaretPosition (8);
    editor.deleteSelectedLines();
    EXPECT_EQ (String ("one"), editor.getText());
    EXPECT_EQ (3, editor.getCaretPosition());

    editor.deleteSelectedLines();
    EXPECT_EQ (String(), editor.getText());
}

TEST (CodeEditorTest, InsertLineAboveAndBelowKeepsIndent)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("  ab");
    editor.setCaretPosition (3);
    editor.keyDown (KeyPress (KeyPress::enterKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String ("  ab\n  "), editor.getText());
    EXPECT_EQ (7, editor.getCaretPosition());

    editor.setCaretPosition (3);
    editor.keyDown (KeyPress (KeyPress::enterKey, KeyModifiers (KeyModifiers::controlMask | KeyModifiers::shiftMask)), {});

    EXPECT_EQ (String ("  \n  ab\n  "), editor.getText());
    EXPECT_EQ (2, editor.getCaretPosition());
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

TEST (CodeEditorTest, TypingOpenerInsertsClosingPair)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.textInput ("f");
    editor.textInput ("(");

    EXPECT_EQ (String ("f()"), editor.getText());
    EXPECT_EQ (2, editor.getCaretPosition());

    editor.textInput ("x");
    editor.textInput (")");

    EXPECT_EQ (String ("f(x)"), editor.getText());
    EXPECT_EQ (4, editor.getCaretPosition());
}

TEST (CodeEditorTest, AutoClosingIsSkippedBeforeWordsAndForQuotesAfterWords)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("x");
    editor.setCaretPosition (0);
    editor.textInput ("(");
    EXPECT_EQ (String ("(x"), editor.getText());

    editor.setSyntaxDefinition ("python");
    editor.setText ("don");
    editor.setCaretPosition (3);
    editor.textInput ("'");
    EXPECT_EQ (String ("don'"), editor.getText());
}

TEST (CodeEditorTest, AutoClosingCanBeDisabled)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setAutoClosingEnabled (false);

    editor.textInput ("[");

    EXPECT_FALSE (editor.isAutoClosingEnabled());
    EXPECT_EQ (String ("["), editor.getText());
}

TEST (CodeEditorTest, TypingOpenerWrapsSelection)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setSyntaxDefinition ("python");
    editor.setText ("a value");
    editor.setSelection ({ 2, 7 });
    editor.textInput ("\"");

    EXPECT_EQ (String ("a \"value\""), editor.getText());
    EXPECT_EQ (Range<int> (3, 8), editor.getSelection());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("a value"), editor.getText());
}

TEST (CodeEditorTest, BackspaceRemovesEmptyPair)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("f()");
    editor.setCaretPosition (2);
    editor.keyDown (KeyPress (KeyPress::backspaceKey), {});

    EXPECT_EQ (String ("f"), editor.getText());
    EXPECT_EQ (1, editor.getCaretPosition());
}

TEST (CodeEditorTest, EnterBetweenBracketsIndentsAndSplitsPair)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("  if (x) {}");
    editor.setCaretPosition (10);
    editor.keyDown (KeyPress (KeyPress::enterKey), {});

    EXPECT_EQ (String ("  if (x) {\n      \n  }"), editor.getText());
    EXPECT_EQ (17, editor.getCaretPosition());
}

TEST (CodeEditorTest, EnterAfterIncreaseIndentPatternIndents)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setSyntaxDefinition ("python");

    editor.setText ("def f():  # note");
    editor.setCaretPosition (16);
    editor.keyDown (KeyPress (KeyPress::enterKey), {});

    EXPECT_EQ (String ("def f():  # note\n    "), editor.getText());
}

TEST (CodeEditorTest, IndentUsingTabsInsertsTabs)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setIndentUsingTabs (true);

    editor.setText ("{");
    editor.setCaretPosition (1);
    editor.keyDown (KeyPress (KeyPress::enterKey), {});
    editor.keyDown (KeyPress (KeyPress::tabKey), {});

    EXPECT_TRUE (editor.isIndentUsingTabs());
    EXPECT_EQ (String ("{\n\t\t"), editor.getText());
}

TEST (CodeEditorTest, TypingCloserOnBlankLineAlignsWithOpener)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setAutoClosingEnabled (false);

    editor.setText ("  {\n      ");
    editor.setCaretPosition (10);
    editor.textInput ("}");

    EXPECT_EQ (String ("  {\n  }"), editor.getText());
    EXPECT_EQ (7, editor.getCaretPosition());
}

TEST (CodeEditorTest, SmartHomeTogglesBetweenIndentAndLineStart)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setText ("    code");
    editor.setCaretPosition (8);

    editor.keyDown (KeyPress (KeyPress::homeKey), {});
    EXPECT_EQ (4, editor.getCaretPosition());

    editor.keyDown (KeyPress (KeyPress::homeKey), {});
    EXPECT_EQ (0, editor.getCaretPosition());

    editor.keyDown (KeyPress (KeyPress::homeKey), {});
    EXPECT_EQ (4, editor.getCaretPosition());
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

TEST (CodeEditorTest, MarkersFollowInsertedAndRemovedLines)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a\nb\nc\nd");

    editor.setBreakpoint (2, true);
    editor.setErrorLine (3, true);
    editor.setBreakpoint (0, true);

    document.insertText (CodeDocument::Position (document, 1, 0), "x\ny\n");
    EXPECT_TRUE (editor.getBreakpoint (0));
    EXPECT_TRUE (editor.getBreakpoint (4));
    EXPECT_FALSE (editor.getBreakpoint (2));
    EXPECT_TRUE (editor.isErrorLine (5));

    document.removeRange (CodeDocument::Position (document, 1, 0), CodeDocument::Position (document, 3, 0));
    EXPECT_TRUE (editor.getBreakpoint (2));
    EXPECT_TRUE (editor.isErrorLine (3));

    // Joining the marked line into the previous one collapses the marker onto it
    document.removeRange (CodeDocument::Position (document, 2, 1), CodeDocument::Position (document, 3, 1));
    EXPECT_TRUE (editor.isErrorLine (2));
    EXPECT_EQ (1u, editor.getErrorLines().size());
}

TEST (CodeEditorTest, GutterBreakpointsCanBeToggled)
{
    CodeEditor editor;

    EXPECT_FALSE (editor.areGutterBreakpointsEnabled());

    editor.setGutterBreakpointsEnabled (true);
    EXPECT_TRUE (editor.areGutterBreakpointsEnabled());

    editor.setGutterBreakpointsEnabled (false);
    EXPECT_FALSE (editor.areGutterBreakpointsEnabled());
}

TEST (CodeEditorTest, BreakpointsCanBeSetWithGutterBreakpointsDisabled)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo");

    editor.setBreakpoint (1, true);
    EXPECT_TRUE (editor.getBreakpoint (1));
}

TEST (CodeEditorTest, ErrorLineLifecycle)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");

    EXPECT_FALSE (editor.isErrorLine (1));
    EXPECT_TRUE (editor.getErrorLines().empty());

    editor.setErrorLine (1, true);
    EXPECT_TRUE (editor.isErrorLine (1));
    EXPECT_FALSE (editor.isErrorLine (0));

    editor.setErrorLine (1, true);
    EXPECT_EQ (1u, editor.getErrorLines().size());

    editor.setErrorLine (1, false);
    EXPECT_FALSE (editor.isErrorLine (1));

    editor.setErrorLine (0, true);
    editor.setErrorLine (2, true);
    editor.clearErrorLines();
    EXPECT_FALSE (editor.isErrorLine (0));
    EXPECT_FALSE (editor.isErrorLine (2));
    EXPECT_TRUE (editor.getErrorLines().empty());
}

TEST (CodeEditorTest, ErrorLineOnOutOfRangeLineClamps)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo");

    editor.setErrorLine (99, true);
    EXPECT_TRUE (editor.isErrorLine (1));

    editor.setErrorLine (-1, true);
    EXPECT_TRUE (editor.isErrorLine (0));
}

TEST (CodeEditorTest, ErrorLinesAreIndependentOfBreakpoints)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo");

    editor.setErrorLine (1, true);
    editor.setBreakpoint (0, true);

    editor.clearBreakpoints();
    EXPECT_TRUE (editor.isErrorLine (1));

    editor.setBreakpoint (0, true);
    editor.clearErrorLines();
    EXPECT_TRUE (editor.getBreakpoint (0));
}

TEST (CodeEditorTest, SetDocumentClearsErrorLines)
{
    CodeDocument first;
    CodeDocument second;
    first.setText ("one\ntwo");
    second.setText ("three\nfour");

    CodeEditor editor (first);
    editor.setErrorLine (1, true);

    editor.setDocument (second);
    EXPECT_FALSE (editor.isErrorLine (1));
    EXPECT_TRUE (editor.getErrorLines().empty());
}

TEST (CodeEditorTest, DiagnosticRangeIsClampedAndExpandsEmptyRanges)
{
    CodeEditor editor;
    editor.setText ("int value = 1;\nfoo");

    CodeEditor::Diagnostic diagnostic;
    diagnostic.line = 0;
    diagnostic.column = 4;
    diagnostic.endLine = 0;
    diagnostic.endColumn = 9;
    EXPECT_EQ (Range<int> (4, 9), editor.getDiagnosticRange (diagnostic));

    // Empty ranges cover the word at their start
    diagnostic.endColumn = 4;
    EXPECT_EQ (Range<int> (4, 9), editor.getDiagnosticRange (diagnostic));

    // ...or the character there when it is not a word
    diagnostic.column = diagnostic.endColumn = 10;
    EXPECT_EQ (Range<int> (10, 11), editor.getDiagnosticRange (diagnostic));

    // ...or the previous character at the end of a line
    diagnostic.line = diagnostic.endLine = 1;
    diagnostic.column = diagnostic.endColumn = 3;
    EXPECT_EQ (Range<int> (17, 18), editor.getDiagnosticRange (diagnostic));

    // Out of range positions are clamped
    diagnostic.line = 0;
    diagnostic.column = 100;
    diagnostic.endLine = 50;
    diagnostic.endColumn = 50;
    EXPECT_EQ (Range<int> (14, 18), editor.getDiagnosticRange (diagnostic));
}

TEST (CodeEditorTest, DiagnosticsAtPositionAreSortedBySeverity)
{
    CodeEditor editor;
    editor.setText ("alpha beta");

    CodeEditor::Diagnostic warning;
    warning.column = 0;
    warning.endColumn = 10;
    warning.severity = CodeEditor::Diagnostic::Severity::warning;
    warning.message = "long line";

    CodeEditor::Diagnostic error;
    error.column = 6;
    error.endColumn = 10;
    error.message = "unknown beta";

    editor.setDiagnostics ({ warning, error });

    const auto atBeta = editor.getDiagnosticsAt (7);
    ASSERT_EQ (2u, atBeta.size());
    EXPECT_EQ (String ("unknown beta"), atBeta[0].message);
    EXPECT_EQ (String ("long line"), atBeta[1].message);

    EXPECT_EQ (1u, editor.getDiagnosticsAt (2).size());
    EXPECT_EQ (2u, editor.getDiagnosticsAt (10).size());

    editor.clearDiagnostics();
    EXPECT_TRUE (editor.getDiagnostics().empty());
}

TEST (CodeEditorTest, DiagnosticsFollowInsertedAndRemovedLines)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a\nb\nc");

    CodeEditor::Diagnostic diagnostic;
    diagnostic.line = diagnostic.endLine = 2;
    diagnostic.endColumn = 1;
    editor.addDiagnostic (diagnostic);

    document.insertText (CodeDocument::Position (document, 0, 0), "x\ny\n");
    ASSERT_EQ (1u, editor.getDiagnostics().size());
    EXPECT_EQ (4, editor.getDiagnostics()[0].line);
    EXPECT_EQ (4, editor.getDiagnostics()[0].endLine);

    document.removeRange (CodeDocument::Position (document, 0, 0), CodeDocument::Position (document, 2, 0));
    EXPECT_EQ (2, editor.getDiagnostics()[0].line);
}

TEST (CodeEditorTest, ErrorLinesAreDiagnosticsAndClearedWithThem)
{
    CodeEditor editor;
    editor.setText ("one\ntwo");

    editor.setErrorLine (1, true);
    ASSERT_EQ (1u, editor.getDiagnostics().size());
    EXPECT_TRUE (editor.getDiagnostics()[0].highlightLine);

    CodeEditor::Diagnostic diagnostic;
    diagnostic.endColumn = 3;
    editor.addDiagnostic (diagnostic);

    editor.clearErrorLines();
    ASSERT_EQ (1u, editor.getDiagnostics().size());
    EXPECT_FALSE (editor.getDiagnostics()[0].highlightLine);

    editor.setErrorLine (0, true);
    editor.clearDiagnostics();
    EXPECT_FALSE (editor.isErrorLine (0));
}

TEST (CodeEditorTest, HoverTextCombinesDiagnosticsAndProvider)
{
    CodeEditor editor;
    editor.setText ("alpha beta");

    CodeEditor::Diagnostic diagnostic;
    diagnostic.column = 6;
    diagnostic.endColumn = 10;
    diagnostic.message = "unknown name";
    diagnostic.source = "lint";
    editor.addDiagnostic (diagnostic);

    EXPECT_EQ (String ("lint: unknown name"), editor.getHoverText (7));
    EXPECT_TRUE (editor.getHoverText (2).isEmpty());
    EXPECT_TRUE (editor.getHoverText (10).isEmpty());

    editor.setHoverProvider ([] (const CodeEditor& e, int index)
    {
        return index < 5 ? String ("word ") + e.getText().substring (0, 5) : String();
    });

    EXPECT_EQ (String ("word alpha"), editor.getHoverText (2));
    EXPECT_EQ (String ("lint: unknown name"), editor.getHoverText (7));

    editor.setHoverProvider ([] (const CodeEditor&, int)
    {
        return String ("doc");
    });

    EXPECT_EQ (String ("lint: unknown name\n\ndoc"), editor.getHoverText (7));
}

TEST (CodeEditorTest, HoverPopupShowsAndHides)
{
    CodeEditor editor;
    editor.setText ("alpha beta");
    editor.setSize (400, 200);

    editor.showHover (2);
    EXPECT_FALSE (editor.isHoverVisible());

    editor.setHoverProvider ([] (const CodeEditor&, int)
    {
        return String ("info");
    });

    editor.showHover (2);
    EXPECT_TRUE (editor.isHoverVisible());

    editor.keyDown (KeyPress (KeyPress::rightKey), {});
    EXPECT_FALSE (editor.isHoverVisible());

    editor.showHover (2);
    editor.insertText ("x");
    EXPECT_FALSE (editor.isHoverVisible());

    editor.showHover (2);
    editor.hideHover();
    EXPECT_FALSE (editor.isHoverVisible());
}

namespace
{

CodeEditor::CompletionProvider makeTestCompletionProvider()
{
    return [] (const CodeEditor&, int)
    {
        return std::vector<CodeEditor::CompletionItem> {
            { "println", {}, "fn" },
            { "print", "print()", "fn" },
            { "sprint", {}, {} },
            { "value", {}, {} }
        };
    };
}

} // namespace

TEST (CodeEditorTest, CompletionsOpenWithCtrlSpaceAndFilterByWordBeforeCaret)
{
    CodeEditor editor;
    editor.setSize (400, 200);
    editor.setText ("pri");
    editor.setCaretPosition (3);

    EXPECT_TRUE (editor.keyDown (KeyPress (KeyPress::spaceKey, KeyModifiers::controlMask), {}));
    EXPECT_FALSE (editor.isCompletionVisible());

    editor.setCompletionProvider (makeTestCompletionProvider());
    editor.keyDown (KeyPress (KeyPress::spaceKey, KeyModifiers::controlMask), {});

    ASSERT_TRUE (editor.isCompletionVisible());
    ASSERT_EQ (3u, editor.getVisibleCompletions().size());
    EXPECT_EQ (String ("println"), editor.getVisibleCompletions()[0].label);
    EXPECT_EQ (String ("print"), editor.getVisibleCompletions()[1].label);
    EXPECT_EQ (String ("sprint"), editor.getVisibleCompletions()[2].label);

    editor.textInput ("n");
    ASSERT_EQ (3u, editor.getVisibleCompletions().size());

    editor.textInput ("t");
    editor.textInput ("l");
    ASSERT_EQ (1u, editor.getVisibleCompletions().size());
    EXPECT_EQ (String ("println"), editor.getVisibleCompletions()[0].label);

    editor.textInput ("z");
    EXPECT_FALSE (editor.isCompletionVisible());
}

TEST (CodeEditorTest, CompletionSelectionWrapsAndEnterAccepts)
{
    CodeEditor editor;
    editor.setSize (400, 200);
    editor.setCompletionProvider (makeTestCompletionProvider());
    editor.setText ("x = pr");
    editor.setCaretPosition (6);
    editor.showCompletions();

    ASSERT_TRUE (editor.isCompletionVisible());
    EXPECT_EQ (0, editor.getSelectedCompletionIndex());

    EXPECT_TRUE (editor.keyDown (KeyPress (KeyPress::upKey), {}));
    EXPECT_EQ (2, editor.getSelectedCompletionIndex());

    EXPECT_TRUE (editor.keyDown (KeyPress (KeyPress::downKey), {}));
    EXPECT_EQ (0, editor.getSelectedCompletionIndex());

    EXPECT_TRUE (editor.keyDown (KeyPress (KeyPress::downKey), {}));
    EXPECT_TRUE (editor.keyDown (KeyPress (KeyPress::enterKey), {}));

    EXPECT_FALSE (editor.isCompletionVisible());
    EXPECT_EQ (String ("x = print()"), editor.getText());
    EXPECT_EQ (11, editor.getCaretPosition());

    editor.undo();
    EXPECT_EQ (String ("x = pr"), editor.getText());
}

TEST (CodeEditorTest, CompletionTriggerCharacterOpensPopup)
{
    CodeEditor editor;
    editor.setSize (400, 200);
    editor.setCompletionProvider (makeTestCompletionProvider());
    editor.setText ("object");
    editor.setCaretPosition (6);

    editor.textInput (".");
    EXPECT_FALSE (editor.isCompletionVisible());

    editor.setCompletionTriggerCharacters (".");
    editor.textInput (".");
    ASSERT_TRUE (editor.isCompletionVisible());
    EXPECT_EQ (4u, editor.getVisibleCompletions().size());

    editor.textInput ("v");
    ASSERT_EQ (1u, editor.getVisibleCompletions().size());

    EXPECT_TRUE (editor.keyDown (KeyPress (KeyPress::tabKey), {}));
    EXPECT_EQ (String ("object..value"), editor.getText());
}

TEST (CodeEditorTest, CompletionClosesOnEscapeCaretMovesAndBackspacePastWordStart)
{
    CodeEditor editor;
    editor.setSize (400, 200);
    editor.setCompletionProvider (makeTestCompletionProvider());
    editor.setText ("a p");
    editor.setCaretPosition (3);

    editor.showCompletions();
    ASSERT_TRUE (editor.isCompletionVisible());
    EXPECT_TRUE (editor.keyDown (KeyPress (KeyPress::escapeKey), {}));
    EXPECT_FALSE (editor.isCompletionVisible());
    EXPECT_EQ (String ("a p"), editor.getText());

    editor.showCompletions();
    editor.keyDown (KeyPress (KeyPress::leftKey), {});
    EXPECT_FALSE (editor.isCompletionVisible());
    EXPECT_EQ (2, editor.getCaretPosition());

    editor.setCaretPosition (3);
    editor.showCompletions();
    editor.keyDown (KeyPress (KeyPress::backspaceKey), {});
    EXPECT_TRUE (editor.isCompletionVisible());

    editor.keyDown (KeyPress (KeyPress::backspaceKey), {});
    EXPECT_FALSE (editor.isCompletionVisible());
}

TEST (CodeEditorTest, CompletionIsNotShownWhenReadOnly)
{
    CodeEditor editor;
    editor.setCompletionProvider (makeTestCompletionProvider());
    editor.setReadOnly (true);

    editor.showCompletions();
    EXPECT_FALSE (editor.isCompletionVisible());
    EXPECT_FALSE (editor.acceptCompletion());
}

TEST (CodeEditorTest, SemanticTokensOverlayLineTokens)
{
    CodeEditor editor;
    editor.setText ("int foo = bar;\nnext");
    editor.setSemanticTokens ({
        { 1, 0, 4, Identifier ("variable") },
        { 0, 4, 3, Identifier ("entity.name.function") },
        { 0, 10, 99, Identifier ("variable.parameter") }
    });

    ASSERT_EQ (3u, editor.getSemanticTokens().size());
    EXPECT_EQ (0, editor.getSemanticTokens()[0].line);

    const auto tokens = editor.getLineTokens (0);
    ASSERT_EQ (4u, tokens.size());
    EXPECT_EQ (0, tokens[0].start);
    EXPECT_EQ (4, tokens[0].end);
    EXPECT_EQ (4, tokens[1].start);
    EXPECT_EQ (7, tokens[1].end);
    EXPECT_EQ (Identifier ("entity.name.function"), tokens[1].scope);
    EXPECT_EQ (7, tokens[2].start);
    EXPECT_EQ (10, tokens[2].end);
    EXPECT_EQ (10, tokens[3].start);
    EXPECT_EQ (14, tokens[3].end);
    EXPECT_EQ (Identifier ("variable.parameter"), tokens[3].scope);

    EXPECT_TRUE (editor.getLineTokens (5).empty());
}

TEST (CodeEditorTest, SemanticTokensSplitSyntaxTokens)
{
    CodeEditor editor;
    editor.setSyntaxDefinition ("cpp");
    editor.setText ("int value;");
    editor.setSemanticTokens ({ { 0, 4, 5, Identifier ("identifier.member") } });

    const auto tokens = editor.getLineTokens (0);
    ASSERT_FALSE (tokens.empty());
    EXPECT_EQ (0, tokens.front().start);
    EXPECT_EQ (10, tokens.back().end);

    for (size_t i = 1; i < tokens.size(); ++i)
        EXPECT_EQ (tokens[i - 1].end, tokens[i].start);

    const auto found = std::find_if (tokens.begin(), tokens.end(), [] (const CodeTokeniser::Token& token)
    {
        return token.start == 4;
    });

    ASSERT_NE (found, tokens.end());
    EXPECT_EQ (5, found->end - found->start);
    EXPECT_EQ (Identifier ("identifier.member"), found->scope);
    EXPECT_EQ (SyntaxDefinition::TokenType::identifier, found->type);
}

TEST (CodeEditorTest, SemanticTokensFollowLineEditsAndDropEditedLines)
{
    CodeEditor editor;
    editor.setText ("aaa\nbbb\nccc");
    editor.setSemanticTokens ({
        { 0, 0, 3, Identifier ("variable") },
        { 1, 0, 3, Identifier ("variable") },
        { 2, 0, 3, Identifier ("variable") }
    });

    editor.setCaretPosition (4);
    editor.insertText ("x\ny");

    ASSERT_EQ (2u, editor.getSemanticTokens().size());
    EXPECT_EQ (0, editor.getSemanticTokens()[0].line);
    EXPECT_EQ (3, editor.getSemanticTokens()[1].line);

    editor.clearSemanticTokens();
    EXPECT_TRUE (editor.getSemanticTokens().empty());

    editor.setSemanticTokens ({ { 0, 0, 3, Identifier ("variable") } });
    CodeDocument other;
    editor.setDocument (other);
    EXPECT_TRUE (editor.getSemanticTokens().empty());
}

TEST (CodeEditorTest, DiagnosticGeometryCoversUnderlinesAndGutter)
{
    CodeEditor editor;
    editor.setText ("int value = 1;\nint other = 2;");
    editor.setSize (400, 200);

    CodeEditor::Diagnostic hint;
    hint.column = 4;
    hint.endColumn = 9;
    hint.severity = CodeEditor::Diagnostic::Severity::hint;

    CodeEditor::Diagnostic error;
    error.line = error.endLine = 1;
    error.column = 4;
    error.endColumn = 9;

    editor.setDiagnostics ({ error, hint });
    editor.setErrorLine (1, true);

    const auto underlines = editor.getDiagnosticUnderlineAreas();
    ASSERT_EQ (2u, underlines.size());
    EXPECT_EQ (CodeEditor::Diagnostic::Severity::hint, underlines[0].severity);
    EXPECT_EQ (CodeEditor::Diagnostic::Severity::error, underlines[1].severity);
    EXPECT_GT (underlines[1].area.getY(), underlines[0].area.getY());

    // Hints and error lines have no gutter marker
    const auto gutter = editor.getDiagnosticGutterMarkers();
    ASSERT_EQ (1u, gutter.size());
    EXPECT_EQ (CodeEditor::Diagnostic::Severity::error, gutter[0].severity);
    EXPECT_LT (gutter[0].area.getRight(), editor.getTextBounds().getX());

    editor.setLineNumbersVisible (false);
    EXPECT_TRUE (editor.getDiagnosticGutterMarkers().empty());
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

TEST (CodeEditorTest, FindAllWithRegexMatchesPatternPerLine)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a1 b22\nC333");

    CodeEditor::SearchOptions options;
    options.regex = true;
    options.caseSensitive = true;

    const auto matches = editor.findAll ("[a-z]\\d+", options);
    ASSERT_EQ (2u, matches.size());
    EXPECT_EQ (Range<int> (0, 2), matches[0]);
    EXPECT_EQ (Range<int> (3, 6), matches[1]);

    options.caseSensitive = false;
    EXPECT_EQ (3u, editor.findAll ("[a-z]\\d+", options).size());
}

TEST (CodeEditorTest, FindAllWithInvalidRegexReturnsNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abc");

    CodeEditor::SearchOptions options;
    options.regex = true;

    EXPECT_TRUE (editor.findAll ("(abc", options).empty());
    EXPECT_FALSE (editor.findNext ("(abc", options));
}

TEST (CodeEditorTest, FindAllWithRegexSkipsEmptyMatches)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("ab");

    CodeEditor::SearchOptions options;
    options.regex = true;

    EXPECT_EQ (1u, editor.findAll ("x*b", options).size());
    EXPECT_TRUE (editor.findAll ("x*", options).empty());
}

TEST (CodeEditorTest, RegexWholeWordUsesWordBoundaries)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("cat concat cats");

    CodeEditor::SearchOptions options;
    options.regex = true;
    options.wholeWord = true;

    const auto matches = editor.findAll ("ca[a-z]", options);
    ASSERT_EQ (1u, matches.size());
    EXPECT_EQ (Range<int> (0, 3), matches[0]);
}

TEST (CodeEditorTest, RegexReplaceAllExpandsCaptureGroups)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo = 1\nbar = 22");

    CodeEditor::SearchOptions options;
    options.regex = true;

    EXPECT_EQ (2, editor.replaceAll ("(\\w+) = (\\d+)", "$2 => $1 ($0) $$", options));
    EXPECT_EQ (String ("1 => foo (foo = 1) $\n22 => bar (bar = 22) $"), editor.getText());

    EXPECT_TRUE (editor.undo());
    EXPECT_EQ (String ("foo = 1\nbar = 22"), editor.getText());
}

TEST (CodeEditorTest, RegexReplaceNextReplacesSelectedMatchOnly)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("x1 x2");

    CodeEditor::SearchOptions options;
    options.regex = true;

    editor.setSelection (Range<int> (0, 1));
    EXPECT_EQ (0, editor.replaceNext ("x(\\d)", "y$1", options));

    editor.setSelection (Range<int> (0, 2));
    EXPECT_EQ (1, editor.replaceNext ("x(\\d)", "y$1", options));
    EXPECT_EQ (String ("y1 x2"), editor.getText());
    EXPECT_EQ (Range<int> (3, 5), editor.getSelection());
}

TEST (CodeEditorTest, SearchHighlightsFollowDocumentEdits)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo bar");

    editor.highlightSearchMatches ("foo");
    EXPECT_EQ (1u, editor.getSearchMatches().size());

    editor.setCaretPosition (7);
    editor.insertText (" foo");
    EXPECT_EQ (2u, editor.getSearchMatches().size());

    editor.clearSearchHighlights();
    EXPECT_TRUE (editor.getSearchMatches().empty());
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

// ==============================================================================
// Font initialisation
// ==============================================================================

TEST (CodeEditorTest, InitialisesWithFallbackFontWhenThemeHasNoMonospaceFont)
{
    auto oldTheme = ApplicationTheme::getGlobalTheme();
    ApplicationTheme::Ptr bareTheme = new ApplicationTheme();
    ApplicationTheme::setGlobalTheme (bareTheme);

    // A bare theme has neither a default monospace font nor a default font, so
    // construction falls through both fallbacks without ever assigning a font.
    CodeEditor editor;
    EXPECT_TRUE (editor.getFont().isEmpty());

    ApplicationTheme::setGlobalTheme (oldTheme.get());
}

// ==============================================================================
// Simple accessors
// ==============================================================================

TEST (CodeEditorTest, GetTabWidthReturnsConfiguredValue)
{
    CodeDocument document;
    CodeEditor editor (document);

    EXPECT_EQ (4, editor.getTabWidth());

    editor.setTabWidth (2);
    EXPECT_EQ (2, editor.getTabWidth());
}

TEST (CodeEditorTest, CanRedoReflectsRedoAvailability)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a");
    editor.setCaretPosition (1);
    editor.insertText ("b");

    EXPECT_FALSE (editor.canRedo());

    editor.undo();
    EXPECT_TRUE (editor.canRedo());
}

TEST (CodeEditorTest, IsCaretVisibleDefaultsToTrue)
{
    CodeEditor editor;
    EXPECT_TRUE (editor.isCaretVisible());
}

TEST (CodeEditorTest, GetBreakpointLinesReturnsSetLines)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");

    editor.setBreakpoint (0, true);
    editor.setBreakpoint (2, true);

    EXPECT_EQ (2u, editor.getBreakpointLines().size());
}

TEST (CodeEditorTest, GetTextInputRectMatchesCaretBounds)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello");
    editor.setCaretPosition (3);

    EXPECT_EQ (editor.getCaretBounds(), editor.getTextInputRect());
}

TEST (CodeEditorTest, GetLineHeightFallsBackToFontHeightBeforeShaping)
{
    CodeEditor editor;
    EXPECT_GT (editor.getLineHeight(), 0.0f);
}

TEST (CodeEditorTest, GetStyledTextReturnsNonEmptyForNonEmptyDocument)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setSize (400, 200);
    editor.setText ("hello");
    editor.setCaretPosition (0);

    EXPECT_FALSE (editor.getStyledText().getOrderedLines().empty());
}

TEST (CodeEditorTest, ScrollToLineUpdatesScrollOffsetAndShapingWindow)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setSize (400, 200);

    String text;
    for (int i = 0; i < 100; ++i)
        text += "line " + String (i) + "\n";
    editor.setText (text);

    editor.scrollToLine (50);

    EXPECT_GT (editor.getScrollOffset().getY(), 0.0f);
    EXPECT_GT (editor.getWindowFirstLine(), 0);
}

// ==============================================================================
// Caret bounds edge cases
// ==============================================================================

TEST (CodeEditorTest, GetCaretBoundsFallsBackToSyntheticPositionWhenNotShaped)
{
    CodeDocument document;
    document.setText ("hello world");

    // The document has content before the editor ever shapes it, so the very
    // first query for caret bounds must synthesize a position instead of
    // relying on StyledText's (still empty) glyph layout.
    CodeEditor editor (document);

    const auto bounds = editor.getCaretBounds();
    EXPECT_GT (bounds.getHeight(), 0.0f);
}

TEST (CodeEditorTest, GetCaretBoundsReturnsTranslatedBoundsAfterShaping)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSize (400, 200);
    editor.setCaretPosition (5);

    const auto bounds = editor.getCaretBounds();
    EXPECT_GT (bounds.getHeight(), 0.0f);
}

TEST (CodeEditorTest, GetCaretBoundsFallsBackWhenCaretOutsideShapedWindow)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setSize (400, 100); // Small viewport shapes only the first few lines.

    String text;
    for (int i = 0; i < 200; ++i)
        text += "line " + String (i) + "\n";
    editor.setText (text);

    editor.setCaretPosition (text.length() - 1); // Far below the currently shaped window.

    const auto bounds = editor.getCaretBounds();
    EXPECT_GT (bounds.getHeight(), 0.0f);
}

TEST (CodeEditorTest, EnsureCaretVisibleHandlesEmptyDocumentGracefully)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setSize (400, 200);

    editor.setCaretPosition (0); // Document is empty; caret bounds are empty.

    EXPECT_EQ (0, editor.getCaretPosition());
}

TEST (CodeEditorTest, GetSelectedTextAreasReturnsRectanglesForSelection)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSize (400, 200);
    editor.setSelection (Range<int> (0, 5));

    EXPECT_FALSE (editor.getSelectedTextAreas().empty());
}

TEST (CodeEditorTest, GetSearchMatchAreasReturnsRectanglesForMatches)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo bar foo");
    editor.setSize (400, 200);
    editor.highlightSearchMatches ("foo");

    EXPECT_FALSE (editor.getSearchMatchAreas().empty());
}

TEST (CodeEditorTest, WordOccurrencesMatchWholeWordsOfWordAtCaret)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo bar foo foobar\nfoo");
    editor.setSize (400, 200);
    editor.setCaretPosition (1);

    const auto occurrences = editor.getWordOccurrences();

    ASSERT_EQ (3u, occurrences.size());
    EXPECT_EQ (Range<int> (0, 3), occurrences[0]);
    EXPECT_EQ (Range<int> (8, 11), occurrences[1]);
    EXPECT_EQ (Range<int> (19, 22), occurrences[2]);
    EXPECT_EQ (3u, editor.getWordOccurrenceAreas().size());
}

TEST (CodeEditorTest, WordOccurrencesUseSelectionOnlyWhenItIsOneWord)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo bar foo");
    editor.setSize (400, 200);

    editor.setSelection (Range<int> (0, 2));
    EXPECT_TRUE (editor.getWordOccurrences().empty());

    editor.setSelection (Range<int> (0, 7));
    EXPECT_TRUE (editor.getWordOccurrences().empty());

    editor.setSelection (Range<int> (8, 11));
    EXPECT_EQ (2u, editor.getWordOccurrences().size());
}

TEST (CodeEditorTest, WordOccurrenceAreasAreEmptyForUniqueWordOrWhenDisabled)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo bar foo");
    editor.setSize (400, 200);

    editor.setCaretPosition (5);
    EXPECT_EQ (1u, editor.getWordOccurrences().size());
    EXPECT_TRUE (editor.getWordOccurrenceAreas().empty());

    editor.setCaretPosition (1);
    editor.setOccurrenceHighlightingEnabled (false);
    EXPECT_TRUE (editor.getWordOccurrences().empty());
    EXPECT_TRUE (editor.getWordOccurrenceAreas().empty());
}

TEST (CodeEditorTest, BracketMatchAreasCoverBothBrackets)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("f(x)");
    editor.setSize (400, 200);

    editor.setCaretPosition (1);
    const auto areas = editor.getBracketMatchAreas();

    ASSERT_EQ (2u, areas.size());
    EXPECT_LT (areas[0].getX(), areas[1].getX());

    editor.setCaretPosition (0);
    EXPECT_TRUE (editor.getBracketMatchAreas().empty());
}

TEST (CodeEditorTest, IndentGuidesFollowIndentLevelsAndBlankLines)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a\n    b\n\n        c\nd");
    editor.setSize (400, 200);

    const auto guides = editor.getIndentGuideAreas();

    ASSERT_EQ (4u, guides.size());
    EXPECT_FLOAT_EQ (guides[0].getX(), guides[1].getX());
    EXPECT_FLOAT_EQ (guides[1].getX(), guides[2].getX());
    EXPECT_LT (guides[2].getX(), guides[3].getX());
    EXPECT_LT (guides[0].getY(), guides[1].getY());

    editor.setIndentGuidesVisible (false);
    EXPECT_TRUE (editor.getIndentGuideAreas().empty());
}

TEST (CodeEditorTest, WhitespaceMarkersAreRenderedOnlyWhenEnabled)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a b\tc");
    editor.setSize (400, 200);

    EXPECT_TRUE (editor.getWhitespaceMarkers().empty());

    editor.setWhitespaceVisible (true);
    const auto markers = editor.getWhitespaceMarkers();

    ASSERT_EQ (2u, markers.size());
    EXPECT_FALSE (markers[0].isTab);
    EXPECT_TRUE (markers[1].isTab);
}

TEST (CodeEditorTest, HorizontalScrollBarAppearsForLongLines)
{
    CodeDocument shortDocument;
    CodeEditor shortEditor (shortDocument);
    shortEditor.setText ("short");
    shortEditor.setSize (200, 200);
    shortEditor.setScrollOffset ({ 0.0f, 0.0f });

    CodeDocument longDocument;
    CodeEditor longEditor (longDocument);
    longEditor.setText (String::repeatedString ("long ", 100));
    longEditor.setSize (200, 200);
    longEditor.setScrollOffset ({ 0.0f, 0.0f });

    EXPECT_LT (longEditor.getTextBounds().getHeight(), shortEditor.getTextBounds().getHeight());

    longEditor.setScrollOffset ({ 100.0f, 0.0f });
    EXPECT_FLOAT_EQ (100.0f, longEditor.getScrollOffset().getX());
}

// ==============================================================================
// Syntax highlighting shaping
// ==============================================================================

TEST (CodeEditorTest, SyntaxHighlightingBuildsMultiColorRunsForTokens)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setSyntaxDefinition ("cpp");
    editor.setText ("int main() {}");

    EXPECT_FALSE (editor.getStyledText().getOrderedLines().empty());
}

// ==============================================================================
// Find / replace edge cases (additional)
// ==============================================================================

TEST (CodeEditorTest, FindPreviousWithNoMatchesReturnsFalse)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abc");

    EXPECT_FALSE (editor.findPrevious ("zzz"));
    EXPECT_FALSE (editor.hasSelection());
}

TEST (CodeEditorTest, FindPreviousWithoutWrapReturnsFalseAtStart)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a b a b a");
    editor.setCaretPosition (0);

    EXPECT_FALSE (editor.findPrevious ("b", false, false));
    EXPECT_FALSE (editor.hasSelection());
}

TEST (CodeEditorTest, ReplaceNextWithNoSelectionReturnsZero)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("foo bar");

    EXPECT_EQ (0, editor.replaceNext ("foo", "X"));
    EXPECT_EQ (String ("foo bar"), editor.getText());
}

TEST (CodeEditorTest, ReplaceAllWithNoMatchesReturnsZero)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abc");

    EXPECT_EQ (0, editor.replaceAll ("zzz", "X"));
    EXPECT_EQ (String ("abc"), editor.getText());
}

// ==============================================================================
// Bracket matching edge cases (additional)
// ==============================================================================

TEST (CodeEditorTest, BracketMatchOnNestedClosingParenSearchesBackward)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("(a(b)c)");

    const auto match = editor.getBracketMatch (6); // Outer closing paren.
    ASSERT_TRUE (match.has_value());
    EXPECT_EQ (Range<int> (0, 6), *match);
}

TEST (CodeEditorTest, BracketMatchOnClosingSquareBracketSearchesBackward)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("{ [a] }");

    const auto match = editor.getBracketMatch (4); // Closing ']'.
    ASSERT_TRUE (match.has_value());
    EXPECT_EQ (Range<int> (2, 4), *match);
}

TEST (CodeEditorTest, BracketMatchOnClosingCurlyBraceSearchesBackward)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("{ [a] }");

    const auto match = editor.getBracketMatch (6); // Closing '}'.
    ASSERT_TRUE (match.has_value());
    EXPECT_EQ (Range<int> (0, 6), *match);
}

TEST (CodeEditorTest, BracketMatchAtStartOfDocumentSkipsOutOfRangeProbe)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("x(y)");

    EXPECT_FALSE (editor.getBracketMatch (0).has_value());
}

// ==============================================================================
// Minimap visibility (additional)
// ==============================================================================

TEST (CodeEditorTest, SetMinimapVisibleTrueReAddsMinimap)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setMinimapVisible (false);
    EXPECT_FALSE (editor.isMinimapVisible());

    editor.setMinimapVisible (true);
    EXPECT_TRUE (editor.isMinimapVisible());
}

// ==============================================================================
// Clipboard edge cases (additional)
// ==============================================================================

TEST (CodeEditorTest, CutInReadOnlyModeDoesNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello");
    editor.setReadOnly (true);
    editor.setSelection (Range<int> (0, 5));

    editor.cut();

    EXPECT_EQ (String ("hello"), editor.getText());
}

TEST (CodeEditorTest, PasteInsertsClipboardText)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("world");
    editor.setCaretPosition (0);

    SystemClipboard::copyTextToClipboard ("hello ");
    editor.paste();

    EXPECT_EQ (String ("hello world"), editor.getText());
}

TEST (CodeEditorTest, PasteInReadOnlyModeDoesNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello");
    editor.setReadOnly (true);

    SystemClipboard::copyTextToClipboard ("world");
    editor.paste();

    EXPECT_EQ (String ("hello"), editor.getText());
}

// ==============================================================================
// Ctrl-modified caret navigation
// ==============================================================================

TEST (CodeEditorTest, ControlLeftMovesToWordStart)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setCaretPosition (11);

    editor.keyDown (KeyPress (KeyPress::leftKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (6, editor.getCaretPosition());
}

TEST (CodeEditorTest, ControlRightMovesToWordEnd)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setCaretPosition (0);

    editor.keyDown (KeyPress (KeyPress::rightKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (5, editor.getCaretPosition());
}

TEST (CodeEditorTest, ControlUpMovesToDocumentStart)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");
    editor.setCaretPosition (9);

    editor.keyDown (KeyPress (KeyPress::upKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (0, editor.getCaretPosition());
}

TEST (CodeEditorTest, ControlDownMovesToDocumentEnd)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");
    editor.setCaretPosition (0);

    editor.keyDown (KeyPress (KeyPress::downKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (document.getNumCharacters(), editor.getCaretPosition());
}

TEST (CodeEditorTest, ControlHomeMovesToDocumentStart)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");
    editor.setCaretPosition (9);

    editor.keyDown (KeyPress (KeyPress::homeKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (0, editor.getCaretPosition());
}

TEST (CodeEditorTest, ControlEndMovesToDocumentEnd)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");
    editor.setCaretPosition (0);

    editor.keyDown (KeyPress (KeyPress::endKey, KeyModifiers (KeyModifiers::controlMask)), {});
    EXPECT_EQ (document.getNumCharacters(), editor.getCaretPosition());
}

// ==============================================================================
// Ctrl-modified clipboard / undo shortcuts
// ==============================================================================

TEST (CodeEditorTest, ControlAShortcutSelectsAll)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("select all of this");

    editor.keyDown (KeyPress (KeyPress::textAKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String ("select all of this"), editor.getSelectedText());
}

TEST (CodeEditorTest, ControlCShortcutCopiesSelection)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("copy me");
    editor.setSelection (Range<int> (0, 4));

    editor.keyDown (KeyPress (KeyPress::textCKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String ("copy"), SystemClipboard::getTextFromClipboard());
}

TEST (CodeEditorTest, ControlXShortcutCutsSelection)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("cut this");
    editor.setSelection (Range<int> (0, 3));

    editor.keyDown (KeyPress (KeyPress::textXKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String (" this"), editor.getText());
    EXPECT_EQ (String ("cut"), SystemClipboard::getTextFromClipboard());
}

TEST (CodeEditorTest, ControlVShortcutPastesClipboard)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("world");
    editor.setCaretPosition (0);

    SystemClipboard::copyTextToClipboard ("hello ");
    editor.keyDown (KeyPress (KeyPress::textVKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String ("hello world"), editor.getText());
}

TEST (CodeEditorTest, ControlShiftZShortcutRedoes)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a");
    editor.setCaretPosition (1);
    editor.insertText ("b");
    editor.undo();

    editor.keyDown (KeyPress (KeyPress::textZKey, KeyModifiers (KeyModifiers::controlMask | KeyModifiers::shiftMask)), {});

    EXPECT_EQ (String ("ab"), editor.getText());
}

TEST (CodeEditorTest, ControlYShortcutRedoes)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("a");
    editor.setCaretPosition (1);
    editor.insertText ("b");
    editor.undo();

    editor.keyDown (KeyPress (KeyPress::textYKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String ("ab"), editor.getText());
}

TEST (CodeEditorTest, KeyDownWhenDisabledIsIgnored)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello");
    editor.setEnabled (false);

    editor.keyDown (KeyPress (KeyPress::backspaceKey), {});

    EXPECT_EQ (String ("hello"), editor.getText());
}

// ==============================================================================
// Delete / word-deletion edge cases (additional)
// ==============================================================================

TEST (CodeEditorTest, DeleteWithSelectionRemovesSelection)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSelection (Range<int> (0, 5));

    editor.keyDown (KeyPress (KeyPress::deleteKey), {});

    EXPECT_EQ (String (" world"), editor.getText());
    EXPECT_EQ (0, editor.getCaretPosition());
}

TEST (CodeEditorTest, ControlBackspaceWithSelectionDeletesSelection)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSelection (Range<int> (0, 5));

    editor.keyDown (KeyPress (KeyPress::backspaceKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String (" world"), editor.getText());
}

TEST (CodeEditorTest, ControlBackspaceInReadOnlyModeDoesNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setReadOnly (true);
    editor.setCaretPosition (11);

    editor.keyDown (KeyPress (KeyPress::backspaceKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String ("hello world"), editor.getText());
}

TEST (CodeEditorTest, ControlDeleteWithSelectionDeletesSelection)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSelection (Range<int> (6, 11));

    editor.keyDown (KeyPress (KeyPress::deleteKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String ("hello "), editor.getText());
}

TEST (CodeEditorTest, ControlDeleteInReadOnlyModeDoesNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setReadOnly (true);

    editor.keyDown (KeyPress (KeyPress::deleteKey, KeyModifiers (KeyModifiers::controlMask)), {});

    EXPECT_EQ (String ("hello world"), editor.getText());
}

// ==============================================================================
// Text input rect
// ==============================================================================

TEST (CodeEditorTest, SetCaretPositionUpdatesTextInputRectWhenActive)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.focusGained(); // Activates text input.

    editor.setCaretPosition (5); // Should call updateTextInputRect() without crashing.

    EXPECT_TRUE (editor.isTextInputActive());
}

// ==============================================================================
// Focus and enablement
// ==============================================================================

TEST (CodeEditorTest, FocusGainedStartsCaretBlinkingAndRequestsTextInput)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.focusGained();

    EXPECT_TRUE (editor.isCaretVisible());
    EXPECT_TRUE (editor.isTextInputActive());
}

TEST (CodeEditorTest, FocusGainedInReadOnlyModeDoesNotRequestTextInput)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setReadOnly (true);

    editor.focusGained();

    EXPECT_FALSE (editor.isTextInputActive());
}

TEST (CodeEditorTest, FocusGainedWhenDisabledStopsBlinkingAndRelinquishesInput)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.focusGained(); // Become active first so relinquishing has an effect.
    editor.setEnabled (false);

    editor.focusGained(); // isEnabled() is false -> takes the else branch.

    EXPECT_FALSE (editor.isTextInputActive());
}

TEST (CodeEditorTest, FocusLostStopsBlinkingAndRelinquishesTextInput)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.focusGained();

    editor.focusLost();

    EXPECT_FALSE (editor.isTextInputActive());
}

TEST (CodeEditorTest, EnablementChangedRunsWithoutCrashingWhenTogglingEnabledState)
{
    CodeDocument document;
    CodeEditor editor (document);

    editor.setEnabled (false);
    EXPECT_FALSE (editor.isEnabled());

    editor.setEnabled (true);
    EXPECT_TRUE (editor.isEnabled());
}

// ==============================================================================
// Mouse interaction
// ==============================================================================

TEST (CodeEditorTest, MouseDownInGutterTogglesBreakpoint)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");
    editor.setSize (400, 200);
    editor.setGutterBreakpointsEnabled (true);

    const float lineHeight = editor.getLineHeight();
    const float clickY = editor.getTextBounds().getY() + lineHeight * 1.5f; // Line index 1.

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (2.0f, clickY));
    editor.mouseDown (event);

    EXPECT_TRUE (editor.getBreakpoint (1));
}

TEST (CodeEditorTest, MouseDownInGutterIgnoresBreakpointsByDefault)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");
    editor.setSize (400, 200);

    EXPECT_FALSE (editor.areGutterBreakpointsEnabled());

    const float clickY = editor.getTextBounds().getY() + editor.getLineHeight() * 1.5f; // Line index 1.

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (2.0f, clickY));
    editor.mouseDown (event);

    EXPECT_FALSE (editor.getBreakpoint (1));
    EXPECT_TRUE (editor.getBreakpointLines().empty());
}

TEST (CodeEditorTest, MouseDownInTextAreaMovesCaretAndClearsSelection)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSize (400, 200);
    editor.setLineNumbersVisible (false); // Simplify: whole width is text area.

    const auto textArea = editor.getTextBounds();
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (textArea.getX() + 5.0f, textArea.getY() + 5.0f));
    editor.mouseDown (event);

    EXPECT_FALSE (editor.hasSelection());
}

TEST (CodeEditorTest, MouseDownOnEmptyLastLineMovesCaretThere)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello\nworld\n");
    editor.setSize (400, 200);
    editor.setLineNumbersVisible (false);

    const auto textArea = editor.getTextBounds();
    const auto lineHeight = editor.getLineHeight();

    editor.mouseDown (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (textArea.getX() + 30.0f, textArea.getY() + lineHeight * 2.5f)));
    EXPECT_EQ (12, editor.getCaretPosition());

    editor.setCaretPosition (0);
    editor.mouseDown (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (textArea.getX() + 30.0f, textArea.getY() + lineHeight * 6.0f)));
    EXPECT_EQ (12, editor.getCaretPosition());
}

TEST (CodeEditorTest, MouseDownOnEmptyMiddleLineMovesCaretThere)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello\n\nworld");
    editor.setSize (400, 200);
    editor.setLineNumbersVisible (false);

    const auto textArea = editor.getTextBounds();
    editor.mouseDown (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (textArea.getX() + 30.0f, textArea.getY() + editor.getLineHeight() * 1.5f)));

    EXPECT_EQ (6, editor.getCaretPosition());
}

TEST (CodeEditorTest, MouseDragExtendsSelectionWhileDragging)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSize (400, 200);
    editor.setLineNumbersVisible (false);

    const auto textArea = editor.getTextBounds();
    MouseEvent down (MouseEvent::leftButton, KeyModifiers(), Point<float> (textArea.getX(), textArea.getY() + 5.0f));
    editor.mouseDown (down);

    MouseEvent drag (MouseEvent::leftButton, KeyModifiers(), Point<float> (textArea.getX() + 50.0f, textArea.getY() + 5.0f));
    editor.mouseDrag (drag);

    EXPECT_TRUE (editor.hasSelection());
}

TEST (CodeEditorTest, MouseDragWithoutPriorMouseDownDoesNothing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSize (400, 200);

    MouseEvent drag (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 10.0f));
    editor.mouseDrag (drag); // isDragging is false; should be a no-op.

    EXPECT_FALSE (editor.hasSelection());
}

TEST (CodeEditorTest, MouseUpStopsDragging)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSize (400, 200);

    MouseEvent down (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 10.0f));
    editor.mouseDown (down);

    editor.mouseUp (MouseEvent());

    const auto selectionBeforeDrag = editor.getSelection();
    MouseEvent drag (MouseEvent::leftButton, KeyModifiers(), Point<float> (200.0f, 10.0f));
    editor.mouseDrag (drag);

    EXPECT_EQ (selectionBeforeDrag, editor.getSelection());
}

TEST (CodeEditorTest, MouseDoubleClickSelectsWord)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world");
    editor.setSize (400, 200);
    editor.setLineNumbersVisible (false);

    const auto textArea = editor.getTextBounds();
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (textArea.getX() + 2.0f, textArea.getY() + 5.0f));
    editor.mouseDoubleClick (event);

    EXPECT_TRUE (editor.hasSelection());
}

TEST (CodeEditorTest, ThirdClickAfterDoubleClickSelectsLine)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("hello world\nnext");
    editor.setSize (400, 200);
    editor.setLineNumbersVisible (false);

    const auto textArea = editor.getTextBounds();
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (textArea.getX() + 2.0f, textArea.getY() + 5.0f));

    editor.mouseDoubleClick (event);
    EXPECT_EQ (Range<int> (0, 5), editor.getSelection());

    editor.mouseDoubleClick (event);
    EXPECT_EQ (Range<int> (0, 12), editor.getSelection());
}

TEST (CodeEditorTest, SelectLineIncludesLineBreak)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("ab\ncd");

    editor.selectLine (0);
    EXPECT_EQ (Range<int> (0, 3), editor.getSelection());

    editor.selectLine (5);
    EXPECT_EQ (Range<int> (3, 5), editor.getSelection());
}

TEST (CodeEditorTest, VerticalMovesKeepPreferredColumn)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("abcdef\nab\nabcdef");
    editor.setCaretPosition (5);

    editor.keyDown (KeyPress (KeyPress::downKey), {});
    EXPECT_EQ (9, editor.getCaretPosition());

    editor.keyDown (KeyPress (KeyPress::downKey), {});
    EXPECT_EQ (15, editor.getCaretPosition());

    editor.keyDown (KeyPress (KeyPress::leftKey), {});
    editor.keyDown (KeyPress (KeyPress::upKey), {});
    editor.keyDown (KeyPress (KeyPress::upKey), {});
    EXPECT_EQ (4, editor.getCaretPosition());
}

TEST (CodeEditorTest, PageDownAndUpMoveByVisibleLines)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 100; ++i)
        text << "line " << i << "\n";

    editor.setText (text);
    editor.setSize (400, 200);

    editor.keyDown (KeyPress (KeyPress::pageDownKey), {});
    const int lineAfterPageDown = document.indexToPosition (editor.getCaretPosition()).getLineNumber();

    EXPECT_GT (lineAfterPageDown, 1);
    EXPECT_GT (editor.getScrollOffset().getY(), 0.0f);

    editor.keyDown (KeyPress (KeyPress::pageDownKey, KeyModifiers (KeyModifiers::shiftMask)), {});
    EXPECT_TRUE (editor.hasSelection());

    editor.keyDown (KeyPress (KeyPress::pageUpKey), {});
    editor.keyDown (KeyPress (KeyPress::pageUpKey), {});
    EXPECT_EQ (0, editor.getCaretPosition());
}

TEST (CodeEditorTest, MouseWheelScrollsContent)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 200; ++i)
        text += "line " + String (i) + "\n";
    editor.setText (text);
    editor.setSize (400, 200);

    MouseWheelData wheelData (0.0f, -5.0f);
    editor.mouseWheel (MouseEvent(), wheelData);

    EXPECT_GT (editor.getScrollOffset().getY(), 0.0f);
}

// ==============================================================================
// Minimap child component (private nested class, dispatched via the base
// Component pointer since it cannot be named outside CodeEditor).
// ==============================================================================

namespace
{

Component* findMinimapComponent (CodeEditor& editor)
{
    for (int i = 0; i < editor.getNumChildComponents(); ++i)
    {
        auto* child = editor.getChildComponent (i);
        if (dynamic_cast<ScrollBar*> (child) == nullptr)
            return child;
    }

    return nullptr;
}

} // namespace

TEST (CodeEditorTest, MinimapPaintsWithoutCrashing)
{
    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");
    editor.setSize (400, 300);

    auto* minimapComponent = findMinimapComponent (editor);
    ASSERT_NE (nullptr, minimapComponent);

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (nullptr, context);
    auto renderer = context->makeRenderer (200, 200);
    ASSERT_NE (nullptr, renderer);
    Graphics g (*context, *renderer);

    minimapComponent->paint (g);
}

TEST (CodeEditorTest, MinimapPaintsWithManyLinesCompressedIntoBars)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 500; ++i)
        text += "line " + String (i) + "\n";
    editor.setText (text);
    editor.setSize (400, 60); // Small height forces many lines per minimap bar.

    auto* minimapComponent = findMinimapComponent (editor);
    ASSERT_NE (nullptr, minimapComponent);

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (nullptr, context);
    auto renderer = context->makeRenderer (200, 200);
    ASSERT_NE (nullptr, renderer);
    Graphics g (*context, *renderer);

    minimapComponent->paint (g);
}

TEST (CodeEditorTest, MinimapMouseDownScrollsToPosition)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 200; ++i)
        text += "line " + String (i) + "\n";
    editor.setText (text);
    editor.setSize (400, 200);

    auto* minimapComponent = findMinimapComponent (editor);
    ASSERT_NE (nullptr, minimapComponent);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 150.0f));
    minimapComponent->mouseDown (event);

    EXPECT_GE (editor.getScrollOffset().getY(), 0.0f);
}

TEST (CodeEditorTest, MinimapMouseDragScrollsToPosition)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 200; ++i)
        text += "line " + String (i) + "\n";
    editor.setText (text);
    editor.setSize (400, 200);

    auto* minimapComponent = findMinimapComponent (editor);
    ASSERT_NE (nullptr, minimapComponent);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 180.0f));
    minimapComponent->mouseDrag (event);

    EXPECT_GE (editor.getScrollOffset().getY(), 0.0f);
}

TEST (CodeEditorTest, MinimapMouseWheelForwardsToEditor)
{
    CodeDocument document;
    CodeEditor editor (document);

    String text;
    for (int i = 0; i < 200; ++i)
        text += "line " + String (i) + "\n";
    editor.setText (text);
    editor.setSize (400, 200);
    editor.setVisible (true);

    auto* minimapComponent = findMinimapComponent (editor);
    ASSERT_NE (nullptr, minimapComponent);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 10.0f));
    MouseWheelData wheelData (0.0f, -3.0f);
    ComponentTestHelper<CodeEditor>::triggerMouseWheel (*minimapComponent, event, wheelData);

    EXPECT_GT (editor.getScrollOffset().getY(), 0.0f);
}

// ==============================================================================
// Painting
// ==============================================================================

TEST (CodeEditorTest, PaintUsesRegisteredThemeStyle)
{
    auto oldTheme = ApplicationTheme::getGlobalTheme();
    ApplicationTheme::setGlobalTheme (createThemeVersion1());

    CodeDocument document;
    CodeEditor editor (document);
    editor.setSyntaxDefinition ("cpp");
    editor.setText ("int main() {}");
    editor.setSize (400, 300);

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (nullptr, context);
    auto renderer = context->makeRenderer (400, 300);
    ASSERT_NE (nullptr, renderer);
    Graphics g (*context, *renderer);

    editor.paint (g);

    ApplicationTheme::setGlobalTheme (oldTheme.get());
}

TEST (CodeEditorTest, PaintsErrorLines)
{
    auto oldTheme = ApplicationTheme::getGlobalTheme();
    ApplicationTheme::setGlobalTheme (createThemeVersion1());

    CodeDocument document;
    CodeEditor editor (document);
    editor.setText ("one\ntwo\nthree");
    editor.setSize (400, 300);
    editor.setErrorLine (1, true);

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (nullptr, context);
    auto renderer = context->makeRenderer (400, 300);
    ASSERT_NE (nullptr, renderer);
    Graphics g (*context, *renderer);

    editor.paint (g);

    ApplicationTheme::setGlobalTheme (oldTheme.get());
}
