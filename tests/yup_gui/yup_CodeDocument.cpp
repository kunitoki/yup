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

class RecordingListener : public CodeDocument::Listener
{
public:
    void codeDocumentChanged (CodeDocument&, int firstChangedLine, int lastChangedLine) override
    {
        changes.emplace_back (firstChangedLine, lastChangedLine);
    }

    std::vector<std::pair<int, int>> changes;
};

} // namespace

// ==============================================================================
// Basic state
// ==============================================================================

TEST (CodeDocumentTests, NewDocumentHasOneEmptyLine)
{
    CodeDocument document;

    EXPECT_EQ (1, document.getNumLines());
    EXPECT_TRUE (document.getLine (0).isEmpty());
    EXPECT_TRUE (document.getText().isEmpty());
    EXPECT_EQ (0, document.getNumCharacters());
}

TEST (CodeDocumentTests, SetTextSplitsLines)
{
    CodeDocument document;
    document.setText ("one\ntwo\nthree");

    EXPECT_EQ (3, document.getNumLines());
    EXPECT_EQ (String ("one"), document.getLine (0));
    EXPECT_EQ (String ("two"), document.getLine (1));
    EXPECT_EQ (String ("three"), document.getLine (2));
    EXPECT_EQ (String ("one\ntwo\nthree"), document.getText());
    EXPECT_EQ (13, document.getNumCharacters());
}

TEST (CodeDocumentTests, SetTextWithTrailingNewlineProducesEmptyLastLine)
{
    CodeDocument document;
    document.setText ("a\nb\n");

    EXPECT_EQ (3, document.getNumLines());
    EXPECT_TRUE (document.getLine (2).isEmpty());
    EXPECT_EQ (String ("a\nb\n"), document.getText());
}

TEST (CodeDocumentTests, SetTextWithConsecutiveNewlinesProducesEmptyInteriorLine)
{
    CodeDocument document;
    document.setText ("a\n\nb");

    EXPECT_EQ (3, document.getNumLines());
    EXPECT_EQ (String ("a"), document.getLine (0));
    EXPECT_TRUE (document.getLine (1).isEmpty());
    EXPECT_EQ (String ("b"), document.getLine (2));
}

TEST (CodeDocumentTests, SetTextWithMultiByteCharactersSplitsOnCodepoints)
{
    CodeDocument document;

    // "\xc3\xa9" = U+00E9 (e acute), "\xc3\xb1" = U+00F1 (n tilde); each is a single codepoint
    // encoded as two UTF-8 bytes, so a byte-indexed split (rather than a codepoint-aware one)
    // would corrupt this text.
    document.setText (String::fromUTF8 ("\xc3\xa9\n\xc3\xb1"));

    EXPECT_EQ (2, document.getNumLines());
    EXPECT_EQ (1, document.getLine (0).length());
    EXPECT_EQ (1, document.getLine (1).length());
    EXPECT_EQ (String::fromUTF8 ("\xc3\xa9"), document.getLine (0));
    EXPECT_EQ (String::fromUTF8 ("\xc3\xb1"), document.getLine (1));
}

// ==============================================================================
// Position mapping
// ==============================================================================

TEST (CodeDocumentTests, IndexToPositionAndBack)
{
    CodeDocument document;
    document.setText ("ab\ncd");

    EXPECT_EQ (CodeDocument::Position (document, 0, 0), document.indexToPosition (0));
    EXPECT_EQ (CodeDocument::Position (document, 0, 2), document.indexToPosition (2));
    EXPECT_EQ (CodeDocument::Position (document, 1, 0), document.indexToPosition (3));
    EXPECT_EQ (CodeDocument::Position (document, 1, 2), document.indexToPosition (5));

    EXPECT_EQ (0, document.positionToIndex (CodeDocument::Position (document, 0, 0)));
    EXPECT_EQ (2, document.positionToIndex (CodeDocument::Position (document, 0, 2)));
    EXPECT_EQ (3, document.positionToIndex (CodeDocument::Position (document, 1, 0)));
    EXPECT_EQ (5, document.positionToIndex (CodeDocument::Position (document, 1, 2)));
}

TEST (CodeDocumentTests, PositionGetAndSetByGlobalIndex)
{
    CodeDocument document;
    document.setText ("hello world");

    CodeDocument::Position position (document, 0, 0);
    position.setPosition (6);
    EXPECT_EQ (0, position.getLineNumber());
    EXPECT_EQ (6, position.getColumnNumber());
    EXPECT_EQ (6, position.getPosition());
}

TEST (CodeDocumentTests, LineStartOffsets)
{
    CodeDocument document;
    document.setText ("ab\ncd\ne");

    EXPECT_EQ (0, document.getLineStartOffset (0));
    EXPECT_EQ (3, document.getLineStartOffset (1));
    EXPECT_EQ (6, document.getLineStartOffset (2));
}

// ==============================================================================
// Editing
// ==============================================================================

TEST (CodeDocumentTests, ReplaceRangeWithinSingleLine)
{
    CodeDocument document;
    document.setText ("hello world");

    document.replaceRange (CodeDocument::Position (document, 0, 0),
                           CodeDocument::Position (document, 0, 5),
                           "hi");

    EXPECT_EQ (String ("hi world"), document.getText());
}

TEST (CodeDocumentTests, ReplaceRangeAcrossLines)
{
    CodeDocument document;
    document.setText ("one\ntwo\nthree");

    document.replaceRange (CodeDocument::Position (document, 0, 3),
                           CodeDocument::Position (document, 2, 0),
                           " ");

    EXPECT_EQ (String ("one three"), document.getText());
    EXPECT_EQ (1, document.getNumLines());
}

TEST (CodeDocumentTests, ReplaceRangeWithMultiLineText)
{
    CodeDocument document;
    document.setText ("a\nb\nc");

    document.replaceRange (CodeDocument::Position (document, 0, 1),
                           CodeDocument::Position (document, 2, 0),
                           "\nX\nY\n");

    EXPECT_EQ (String ("a\nX\nY\nc"), document.getText());
    EXPECT_EQ (4, document.getNumLines());
}

TEST (CodeDocumentTests, InsertText)
{
    CodeDocument document;
    document.setText ("hello");

    document.insertText (CodeDocument::Position (document, 0, 0), "X");
    EXPECT_EQ (String ("Xhello"), document.getText());

    document.insertText (CodeDocument::Position (document, 0, 6), "!");
    EXPECT_EQ (String ("Xhello!"), document.getText());
}

TEST (CodeDocumentTests, RemoveRange)
{
    CodeDocument document;
    document.setText ("hello world");

    document.removeRange (CodeDocument::Position (document, 0, 6),
                          CodeDocument::Position (document, 0, 11));

    EXPECT_EQ (String ("hello "), document.getText());
}

// ==============================================================================
// Undo / redo
// ==============================================================================

TEST (CodeDocumentTests, UndoAndRedoRestoreText)
{
    CodeDocument document;
    document.setText ("hello world");

    document.replaceRange (CodeDocument::Position (document, 0, 6),
                           CodeDocument::Position (document, 0, 11),
                           "yup");

    EXPECT_EQ (String ("hello yup"), document.getText());
    EXPECT_TRUE (document.canUndo());

    EXPECT_TRUE (document.undo());
    EXPECT_EQ (String ("hello world"), document.getText());
    EXPECT_TRUE (document.canRedo());

    EXPECT_TRUE (document.redo());
    EXPECT_EQ (String ("hello yup"), document.getText());
}

TEST (CodeDocumentTests, UndoAcrossMultipleEdits)
{
    CodeDocument document;
    document.setText ("a\nb\nc");

    document.replaceRange (CodeDocument::Position (document, 1, 0),
                           CodeDocument::Position (document, 1, 1),
                           "B");

    // Consecutive edits coalesce into a single undo transaction unless a new one
    // is begun, so open a new transaction to make each edit undoable separately.
    document.getUndoManager()->beginNewTransaction();

    document.replaceRange (CodeDocument::Position (document, 2, 0),
                           CodeDocument::Position (document, 2, 1),
                           "C");

    EXPECT_EQ (String ("a\nB\nC"), document.getText());

    document.undo();
    EXPECT_EQ (String ("a\nB\nc"), document.getText());

    document.undo();
    EXPECT_EQ (String ("a\nb\nc"), document.getText());

    EXPECT_FALSE (document.canUndo());
}

TEST (CodeDocumentTests, UndoRestoresMultiLineReplacement)
{
    CodeDocument document;
    document.setText ("one\ntwo\nthree");

    document.replaceRange (CodeDocument::Position (document, 0, 3),
                           CodeDocument::Position (document, 2, 5),
                           "X");

    EXPECT_EQ (String ("oneX"), document.getText());

    document.undo();
    EXPECT_EQ (String ("one\ntwo\nthree"), document.getText());
}

// ==============================================================================
// Listener notifications
// ==============================================================================

TEST (CodeDocumentTests, ListenerNotifiedOnEdit)
{
    CodeDocument document;
    RecordingListener listener;
    document.addListener (&listener);

    document.replaceRange (CodeDocument::Position (document, 0, 0),
                           CodeDocument::Position (document, 0, 0),
                           "hello");

    ASSERT_EQ (1u, listener.changes.size());
    EXPECT_EQ (0, listener.changes[0].first);
    EXPECT_EQ (0, listener.changes[0].second);

    document.removeListener (&listener);
}

TEST (CodeDocumentTests, ListenerReportsSpanningLineRange)
{
    CodeDocument document;
    document.setText ("a\nb\nc\nd");
    RecordingListener listener;
    document.addListener (&listener);

    document.replaceRange (CodeDocument::Position (document, 1, 0),
                           CodeDocument::Position (document, 2, 1),
                           "X\nY\nZ");

    ASSERT_FALSE (listener.changes.empty());
    EXPECT_EQ (1, listener.changes.back().first);
    EXPECT_EQ (3, listener.changes.back().second);

    document.removeListener (&listener);
}

// ==============================================================================
// Returned caret position after edits
// ==============================================================================

TEST (CodeDocumentTests, ReplaceRangeReturnsCaretAfterSingleLineInsert)
{
    CodeDocument document;
    document.setText ("abc\ndef");

    const auto endPosition = document.insertText (CodeDocument::Position (document, 1, 0), "X");

    EXPECT_EQ (CodeDocument::Position (document, 1, 1), endPosition);
    EXPECT_EQ (String ("abc\nXdef"), document.getText());
}

TEST (CodeDocumentTests, NewlineInsertAtLineStartPlacesCaretOnLineBelowNewline)
{
    CodeDocument document;
    document.setText ("abc\ndef");

    // applyEdit always returns the true end of the inserted text so undo can
    // reconstruct the correct replacement range.  Enter-at-line-start inserts
    // "\n" whose sole segment after split is "", so the caret lands at the
    // beginning of the line that follows the inserted newline (line 2, "def"),
    // not on the empty line (line 1) that was created.  If the editor wants to
    // place the caret on the new empty line it must do so in its key handler.
    const auto endPosition = document.insertText (CodeDocument::Position (document, 1, 0), "\n");

    EXPECT_EQ (CodeDocument::Position (document, 2, 0), endPosition);
    EXPECT_EQ (String ("abc\n\ndef"), document.getText());
}

TEST (CodeDocumentTests, NewlineInsertAtEndOfLineKeepsCaretOnTrailingEmptyLine)
{
    CodeDocument document;
    document.setText ("abc");

    const auto endPosition = document.insertText (CodeDocument::Position (document, 0, 3), "\n");

    EXPECT_EQ (CodeDocument::Position (document, 1, 0), endPosition);
    EXPECT_EQ (String ("abc\n"), document.getText());
}

TEST (CodeDocumentTests, NewlineAndIndentInsertPlacesCaretAfterIndent)
{
    CodeDocument document;
    document.setText ("abc\ndef");

    // Enter with auto-indent: "\n    " ends with non-newline content, so the caret
    // lands at the end of the indentation on the new line.
    const auto endPosition = document.insertText (CodeDocument::Position (document, 1, 0), "\n    ");

    EXPECT_EQ (CodeDocument::Position (document, 2, 4), endPosition);
    EXPECT_EQ (String ("abc\n\n    def"), document.getText());
}

// ==============================================================================
// External undo manager
// ==============================================================================

TEST (CodeDocumentTests, ExternalUndoManagerIsUsedAndShared)
{
    UndoManager::Ptr sharedManager (new UndoManager());

    CodeDocument first (sharedManager);
    CodeDocument second (sharedManager);

    EXPECT_EQ (sharedManager, first.getUndoManager());
    EXPECT_EQ (sharedManager, second.getUndoManager());

    first.setText ("abc");
    first.insertText (CodeDocument::Position (first, 0, 0), "x");
    EXPECT_EQ (String ("xabc"), first.getText());
    EXPECT_TRUE (first.canUndo());

    // The shared manager exposes the same history to the other document.
    EXPECT_TRUE (second.canUndo());
    EXPECT_TRUE (second.undo());
    EXPECT_EQ (String ("abc"), first.getText());
}

TEST (CodeDocumentTests, ExternalUndoManagerIsNotClearedOnDestruction)
{
    UndoManager::Ptr sharedManager (new UndoManager());

    {
        CodeDocument document (sharedManager);
        document.setText ("abc");
        document.insertText (CodeDocument::Position (document, 0, 0), "x");
        EXPECT_TRUE (sharedManager->canUndo());
    }

    // The externally owned manager keeps its history (not cleared by the document).
    EXPECT_TRUE (sharedManager->canUndo());
    EXPECT_FALSE (sharedManager->canRedo());
}

// ==============================================================================
// Degenerate inputs and clamping
// ==============================================================================

TEST (CodeDocumentTests, SetTextEmptyProducesSingleEmptyLine)
{
    CodeDocument document;
    document.setText ("");

    EXPECT_EQ (1, document.getNumLines());
    EXPECT_TRUE (document.getLine (0).isEmpty());
    EXPECT_EQ (0, document.getNumCharacters());
    EXPECT_TRUE (document.getText().isEmpty());
}

TEST (CodeDocumentTests, SetTextWithOnlyNewlinesProducesEmptyLines)
{
    CodeDocument document;
    document.setText ("\n");

    EXPECT_EQ (2, document.getNumLines());
    EXPECT_TRUE (document.getLine (0).isEmpty());
    EXPECT_TRUE (document.getLine (1).isEmpty());
    EXPECT_EQ (1, document.getNumCharacters());

    document.setText ("\n\n");
    EXPECT_EQ (3, document.getNumLines());
    EXPECT_EQ (2, document.getNumCharacters());
}

TEST (CodeDocumentTests, SetTextWithDefaultConstructedStringRefIsSafe)
{
    CodeDocument document;
    document.setText (StringRef());

    EXPECT_EQ (1, document.getNumLines());
    EXPECT_TRUE (document.getText().isEmpty());
}

TEST (CodeDocumentTests, GetLineOutOfRangeClampsToLastLine)
{
    CodeDocument document;
    document.setText ("one\ntwo");

    EXPECT_EQ (String ("two"), document.getLine (2));
    EXPECT_EQ (String ("two"), document.getLine (100));
    EXPECT_EQ (String ("one"), document.getLine (-1));
}

TEST (CodeDocumentTests, GetLineStartOffsetOutOfRangeClampsToLastLine)
{
    CodeDocument document;
    document.setText ("ab\ncd\ne");

    EXPECT_EQ (6, document.getLineStartOffset (2));
    EXPECT_EQ (6, document.getLineStartOffset (42));
    EXPECT_EQ (0, document.getLineStartOffset (-3));
}

TEST (CodeDocumentTests, IndexToPositionClampsOutOfRangeIndexes)
{
    CodeDocument document;
    document.setText ("ab\ncd");

    EXPECT_EQ (CodeDocument::Position (document, 0, 0), document.indexToPosition (-10));
    EXPECT_EQ (CodeDocument::Position (document, 1, 2), document.indexToPosition (100));
}

TEST (CodeDocumentTests, PositionToIndexClampsOutOfRangePositions)
{
    CodeDocument document;
    document.setText ("ab\ncd");

    EXPECT_EQ (0, document.positionToIndex (CodeDocument::Position (document, -1, 0)));
    EXPECT_EQ (0, document.positionToIndex (CodeDocument::Position (document, 0, -5)));
    EXPECT_EQ (3, document.positionToIndex (CodeDocument::Position (document, 99, 0)));
    EXPECT_EQ (5, document.positionToIndex (CodeDocument::Position (document, 1, 99)));
}

TEST (CodeDocumentTests, UnboundPositionIsSafeAndInert)
{
    CodeDocument::Position position;

    EXPECT_EQ (0, position.getLineNumber());
    EXPECT_EQ (0, position.getColumnNumber());
    EXPECT_EQ (0, position.getPosition());

    position.setPosition (42); // no owner: silently ignored
    EXPECT_EQ (0, position.getPosition());

    EXPECT_TRUE (position == CodeDocument::Position());
}

TEST (CodeDocumentTests, PositionsFromDifferentDocumentsCompareByLineAndIndex)
{
    CodeDocument first;
    CodeDocument second;

    EXPECT_TRUE (CodeDocument::Position (first, 1, 2) == CodeDocument::Position (second, 1, 2));
    EXPECT_TRUE (CodeDocument::Position (first, 1, 2) != CodeDocument::Position (second, 1, 3));
}

TEST (CodeDocumentTests, ReplaceRangeWithInvertedRangeIsNoOp)
{
    CodeDocument document;
    document.setText ("a\nb\nc");

    const auto endPosition = document.replaceRange (CodeDocument::Position (document, 2, 0),
                                                    CodeDocument::Position (document, 1, 0),
                                                    "X");

    EXPECT_EQ (String ("a\nb\nc"), document.getText());
    EXPECT_EQ (CodeDocument::Position (document, 2, 0), endPosition);

    // The no-op edit still records an undo step; undoing it is a harmless no-op.
    EXPECT_TRUE (document.canUndo());
    EXPECT_TRUE (document.undo());
    EXPECT_EQ (String ("a\nb\nc"), document.getText());
}

TEST (CodeDocumentTests, ReplaceRangeWithEmptyRangeAndEmptyTextIsNoOp)
{
    CodeDocument document;
    document.setText ("abc");

    document.replaceRange (CodeDocument::Position (document, 1, 1),
                           CodeDocument::Position (document, 1, 1),
                           "");

    EXPECT_EQ (String ("abc"), document.getText());

    // Minor quirk: a no-op edit still pushes an undo step (undoing it is a no-op).
    EXPECT_TRUE (document.canUndo());
    EXPECT_TRUE (document.undo());
    EXPECT_EQ (String ("abc"), document.getText());
}

TEST (CodeDocumentTests, UndoAfterSetTextDoesNotCorruptDocument)
{
    // setText() replaces the whole document, which invalidates every stored edit
    // position. The undo history must be dropped so undoing afterwards cannot
    // delete characters at stale positions in the new content.
    CodeDocument document;

    document.insertText (CodeDocument::Position (document, 0, 0), "x");

    document.setText ("abcdef");

    EXPECT_FALSE (document.canUndo());
    EXPECT_FALSE (document.undo());
    EXPECT_EQ (String ("abcdef"), document.getText());
}

TEST (CodeDocumentTests, DisabledUndoManagerIgnoresEdits)
{
    UndoManager::Ptr manager (new UndoManager());
    manager->setEnabled (false);

    CodeDocument document (manager);
    document.setText ("abc");

    const auto endPosition = document.insertText (CodeDocument::Position (document, 1, 0), "X");

    EXPECT_EQ (String ("abc"), document.getText());
    EXPECT_EQ (CodeDocument::Position (document, 1, 0), endPosition);
    EXPECT_FALSE (document.canUndo());
}

// ==============================================================================
// Listener edge cases
// ==============================================================================

TEST (CodeDocumentTests, MultipleListenersAllNotified)
{
    CodeDocument document;
    document.setText ("a\nb");

    RecordingListener first;
    RecordingListener second;
    document.addListener (&first);
    document.addListener (&second);

    document.insertText (CodeDocument::Position (document, 0, 1), "X");

    EXPECT_EQ (1u, first.changes.size());
    EXPECT_EQ (1u, second.changes.size());

    document.removeListener (&first);
    document.removeListener (&second);
}

TEST (CodeDocumentTests, SetTextWithoutNotificationDoesNotNotifyListeners)
{
    CodeDocument document;
    RecordingListener listener;
    document.addListener (&listener);

    document.setText ("one\ntwo", dontSendNotification);

    EXPECT_TRUE (listener.changes.empty());

    document.removeListener (&listener);
}

TEST (CodeDocumentTests, SetTextWithNotificationNotifiesWholeRange)
{
    CodeDocument document;
    document.setText ("one");
    RecordingListener listener;
    document.addListener (&listener);

    document.setText ("a\nb\nc", sendNotification);

    ASSERT_EQ (1u, listener.changes.size());
    EXPECT_EQ (0, listener.changes[0].first);
    EXPECT_EQ (2, listener.changes[0].second);

    document.removeListener (&listener);
}

// ==============================================================================
// Content invariants
// ==============================================================================

TEST (CodeDocumentTests, CRLFLineEndingsKeepCarriageReturnInLineText)
{
    // Lines are split on '\n' only, so '\r' remains part of the line text. This
    // matches the current implementation; consumers must handle it.
    CodeDocument document;
    document.setText ("a\r\nb");

    EXPECT_EQ (2, document.getNumLines());
    EXPECT_EQ (String ("a\r"), document.getLine (0));
    EXPECT_EQ (String ("b"), document.getLine (1));
    EXPECT_EQ (String ("a\r\nb"), document.getText());
    EXPECT_EQ (4, document.getNumCharacters());
}

TEST (CodeDocumentTests, GetNumCharactersMatchesTextLength)
{
    for (const char* text : { "abc", "a\nb\nc", "\n", "a\n\nb", "", "hello world\n", "\n\n\n" })
    {
        CodeDocument document;
        document.setText (text);

        EXPECT_EQ (document.getText().length(), document.getNumCharacters()) << text;
    }
}

// ==============================================================================
// Undo / redo stability
// ==============================================================================

TEST (CodeDocumentTests, RepeatedUndoRedoCyclesAreStable)
{
    CodeDocument document;
    document.setText ("abc\ndef");

    auto undo = document.getUndoManager();
    ASSERT_TRUE (undo);

    undo->beginNewTransaction();
    document.insertText (CodeDocument::Position (document, 1, 0), "X");
    undo->beginNewTransaction();
    document.insertText (CodeDocument::Position (document, 1, 1), "Y");

    const String finalText = document.getText();
    EXPECT_EQ (String ("abc\nXYdef"), finalText);

    for (int i = 0; i < 3; ++i)
    {
        EXPECT_TRUE (document.undo());
        EXPECT_TRUE (document.undo());
        EXPECT_EQ (String ("abc\ndef"), document.getText());

        EXPECT_TRUE (document.redo());
        EXPECT_TRUE (document.redo());
        EXPECT_EQ (finalText, document.getText());
    }
}
