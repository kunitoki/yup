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

#pragma once

namespace yup
{

//==============================================================================
/** A line-based text document with undo/redo support and incremental change notifications.

    The document stores its text as a list of lines (without trailing newlines) and
    provides an `UndoManager`-backed editing API. Listeners are notified with the
    inclusive range of lines affected by each edit, which allows dependent consumers
    (such as syntax tokenizers or line-number gutters) to invalidate only what changed.

    Example usage:
    @code
    CodeDocument document;
    document.setText ("int main() {\\n    return 0;\\n}\\n");

    CodeDocument::Position start (&document, 0, 0);
    CodeDocument::Position end (&document, 2, 1);
    document.replaceRange (start, end, "int main() { return 0; }");
    document.undo();
    @endcode

    @see CodeTokeniser
*/
class YUP_API CodeDocument
{
public:
    //==============================================================================
    /** Represents a position in a CodeDocument as a line number and a column within that line.

        Positions are clamped to the document bounds when queried, so they stay valid
        across edits that shrink or grow the document.
    */
    class YUP_API Position
    {
    public:
        /** Creates an unbound position. */
        Position() = default;

        /** Creates a position owned by the given document.

            @param owner       The document the position refers to.
            @param line        The line number (0-based).
            @param column      The column number within the line (0-based).
        */
        Position (const CodeDocument& owner, int line, int column);

        /** Returns the 0-based line number of this position. */
        int getLineNumber() const noexcept { return line; }

        /** Returns the 0-based column number within the line. */
        int getColumnNumber() const noexcept { return column; }

        /** Returns the global character index of this position (newlines included). */
        int getPosition() const;

        /** Sets this position from a global character index, clamping to the document bounds.

            @param newPosition The global character index.
        */
        void setPosition (int newPosition);

        /** Returns true if the positions refer to the same line and column. */
        bool operator== (const Position& other) const noexcept;

        /** Returns true if the positions refer to different lines or columns. */
        bool operator!= (const Position& other) const noexcept;

    private:
        friend class CodeDocument;

        const CodeDocument* owner = nullptr;
        int line = 0;
        int column = 0;
    };

    //==============================================================================
    /** Receives callbacks when the document content changes.

        @see addListener, removeListener
    */
    class YUP_API Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** Called when text is inserted, removed, replaced, undone or redone.

            @param document        The document that changed.
            @param firstChangedLine The first line (inclusive) affected by the change.
            @param lastChangedLine  The last line (inclusive) affected by the change.
        */
        virtual void codeDocumentChanged (CodeDocument& document, int firstChangedLine, int lastChangedLine) = 0;
    };

    //==============================================================================
    /** Creates an empty document with its own internal undo manager. */
    CodeDocument();

    /** Creates an empty document that uses an externally provided undo manager.

        The passed-in manager is shared and its history is NOT cleared when this
        document is destroyed — the caller owns its lifetime and is responsible
        for clearing it once no live document (or pending action) references it.

        @param undoManagerToUse The undo manager to use.
    */
    explicit CodeDocument (UndoManager::Ptr undoManagerToUse);

    /** Destructor. */
    ~CodeDocument();

    //==============================================================================
    /** Returns the number of lines in the document.

        An empty document always contains at least one (empty) line.

        @returns The number of lines.
    */
    int getNumLines() const noexcept { return static_cast<int> (lines.size()); }

    /** Returns the text of a line without its trailing newline.

        @param lineNumber The 0-based line index.
        @returns The line's text (clamped to the last line if the line is out of range).
    */
    String getLine (int lineNumber) const;

    /** Returns the full text of the document, lines joined with newlines. */
    String getText() const;

    /** Returns the total number of characters in the document, newlines included. */
    int getNumCharacters() const;

    /** Replaces the whole document with new text.

        @param newText     The new text.
        @param notification Whether to notify listeners.
    */
    void setText (StringRef newText, NotificationType notification = sendNotification);

    //==============================================================================
    /** Returns the global character offset at which a line starts.

        @param lineNumber The 0-based line index.
        @returns The global offset (clamped to the last line's offset for an out-of-range line).
    */
    int getLineStartOffset (int lineNumber) const;

    /** Converts a global character index into a line-based position.

        @param characterIndex The global character index (newlines included).
        @returns The position, clamped to the document bounds.
    */
    Position indexToPosition (int characterIndex) const;

    /** Converts a line-based position into a global character index.

        @param position The position.
        @returns The global character index, clamped to the document bounds.
    */
    int positionToIndex (const Position& position) const;

    //==============================================================================
    /** Replaces a range of text with new text, recording an undo step.

        @param start      The start of the range to replace.
        @param end        The end of the range to replace.
        @param newText    The replacement text.
        @return The position right after the inserted text (the caret location).
    */
    Position replaceRange (Position start, Position end, StringRef newText);

    /** Inserts text at a position, recording an undo step.

        @param position The position to insert at.
        @param newText  The text to insert.
        @return The position right after the inserted text (the caret location).
    */
    Position insertText (Position position, StringRef newText);

    /** Removes a range of text, recording an undo step.

        @param start The start of the range to remove.
        @param end   The end of the range to remove.
    */
    void removeRange (Position start, Position end);

    //==============================================================================
    /** Undoes the most recent edit, if any. */
    bool undo();

    /** Redoes the most recently undone edit, if any. */
    bool redo();

    /** Returns true if an undo is available. */
    bool canUndo() const;

    /** Returns true if a redo is available. */
    bool canRedo() const;

    /** Returns the undo manager used by this document.

        Callers can use it to group edits into single transactions via
        `UndoManager::beginNewTransaction()`.
    */
    UndoManager::Ptr getUndoManager() const { return undoManager; }

    //==============================================================================
    /** Adds a listener to be notified of document changes. */
    void addListener (Listener* listenerToAdd);

    /** Removes a previously added listener. */
    void removeListener (Listener* listenerToRemove);

private:
    friend class Position;

    class UndoableEdit;

    Position applyEdit (const Position& start, const Position& end, StringRef newText);
    String getTextInRange (const Position& start, const Position& end) const;
    int clampLine (int lineNumber) const noexcept;
    int clampIndexInLine (int lineNumber, int column) const noexcept;
    void ensureOffsetsBuilt() const;

    std::vector<String> lines;
    String newLineChars = "\n";

    mutable std::vector<int> lineStartOffsets;
    mutable bool offsetsDirty = true;

    ListenerList<Listener> listeners;
    UndoManager::Ptr undoManager;
    bool ownsUndoManager = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CodeDocument)
};

} // namespace yup
