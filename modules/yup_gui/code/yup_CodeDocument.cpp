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

namespace yup
{

//==============================================================================

namespace
{

void splitIntoLines (StringRef text, std::vector<String>& result)
{
    result.clear();

    auto ptr = text.text;

    for (;;)
    {
        const auto startOfLine = ptr;
        auto endOfLine = ptr;
        bool reachedEnd = false;

        for (;;)
        {
            endOfLine = ptr;
            const auto c = ptr.getAndAdvance();

            if (c == 0)
            {
                reachedEnd = true;
                break;
            }

            if (c == '\n')
                break;
        }

        result.push_back (String (startOfLine, endOfLine));

        if (reachedEnd)
            break;
    }
}

} // namespace

//==============================================================================

CodeDocument::CodeDocument()
    : undoManager (new UndoManager())
    , ownsUndoManager (true)
{
    lines.emplace_back();
}

CodeDocument::CodeDocument (UndoManager::Ptr undoManagerToUse)
    : undoManager (std::move (undoManagerToUse))
    , ownsUndoManager (false)
{
    lines.emplace_back();
}

CodeDocument::~CodeDocument()
{
    if (ownsUndoManager)
        undoManager->clear();
}

//==============================================================================

String CodeDocument::getLine (int lineNumber) const
{
    return lines[static_cast<size_t> (clampLine (lineNumber))];
}

String CodeDocument::getText() const
{
    String result;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        result << lines[i];

        if (i + 1 < lines.size())
            result << newLineChars;
    }

    return result;
}

int CodeDocument::getNumCharacters() const
{
    ensureOffsetsBuilt();

    if (lines.empty())
        return 0;

    return lineStartOffsets.back()
         + static_cast<int> (lines.back().length());
}

void CodeDocument::setText (StringRef newText, NotificationType notification)
{
    // A wholesale replace invalidates every stored edit position (they refer to the
    // previous content), so the undo history must be dropped — undoing after a
    // setText would otherwise delete characters at stale positions and corrupt the
    // new document.
    undoManager->clear();

    splitIntoLines (newText, lines);
    offsetsDirty = true;

    if (notification != dontSendNotification)
    {
        const int lastChangedLine = static_cast<int> (lines.size()) - 1;
        listeners.call ([this, lastChangedLine] (Listener& l)
        {
            l.codeDocumentChanged (*this, 0, lastChangedLine);
        });
    }
}

//==============================================================================

int CodeDocument::getLineStartOffset (int lineNumber) const
{
    ensureOffsetsBuilt();

    const int line = clampLine (lineNumber);
    return lineStartOffsets[static_cast<size_t> (line)];
}

CodeDocument::Position CodeDocument::indexToPosition (int characterIndex) const
{
    ensureOffsetsBuilt();

    characterIndex = jlimit (0, getNumCharacters(), characterIndex);

    if (lineStartOffsets.empty())
        return { *this, 0, 0 };

    auto it = std::upper_bound (lineStartOffsets.begin(), lineStartOffsets.end(), characterIndex);
    const int line = jmax (0, static_cast<int> (std::distance (lineStartOffsets.begin(), it)) - 1);

    return Position (*this, line, characterIndex - lineStartOffsets[static_cast<size_t> (line)]);
}

int CodeDocument::positionToIndex (const Position& position) const
{
    ensureOffsetsBuilt();

    const int line = clampLine (position.getLineNumber());
    const int column = clampIndexInLine (line, position.getColumnNumber());

    return lineStartOffsets[static_cast<size_t> (line)] + column;
}

//==============================================================================

//==============================================================================

class CodeDocument::UndoableEdit : public UndoableAction
{
public:
    UndoableEdit (CodeDocument& document,
                  CodeDocument::Position start,
                  CodeDocument::Position end,
                  String oldText,
                  String newText)
        : document (&document)
        , startLine (start.getLineNumber())
        , startIndex (start.getColumnNumber())
        , endLine (end.getLineNumber())
        , endIndex (end.getColumnNumber())
        , oldText (std::move (oldText))
        , newText (std::move (newText))
    {
    }

    bool isValid() const override
    {
        return document != nullptr;
    }

    bool perform (UndoableActionState stateToPerform) override
    {
        if (document == nullptr)
            return false;

        if (stateToPerform == UndoableActionState::Redo)
        {
            const auto newEnd = document->applyEdit (position (startLine, startIndex),
                                                     position (endLine, endIndex),
                                                     newText);

            newEndLine = newEnd.getLineNumber();
            newEndIndex = newEnd.getColumnNumber();
        }
        else
        {
            document->applyEdit (position (startLine, startIndex),
                                 position (newEndLine, newEndIndex),
                                 oldText);
        }

        return true;
    }

    CodeDocument::Position getEndPosition() const
    {
        if (document != nullptr)
            return position (newEndLine, newEndIndex);

        return position (startLine, startIndex);
    }

private:
    CodeDocument::Position position (int line, int column) const
    {
        return CodeDocument::Position (*document, line, column);
    }

    CodeDocument* document;
    int startLine = 0;
    int startIndex = 0;
    int endLine = 0;
    int endIndex = 0;
    int newEndLine = 0;
    int newEndIndex = 0;
    String oldText;
    String newText;
};

CodeDocument::Position CodeDocument::replaceRange (Position start, Position end, StringRef newText)
{
    const auto oldText = getTextInRange (start, end);

    auto action = new UndoableEdit (*this, start, end, oldText, String (newText));
    if (undoManager->perform (action))
        return action->getEndPosition();

    return start; // undo manager disabled: the document was not modified
}

CodeDocument::Position CodeDocument::insertText (Position position, StringRef newText)
{
    return replaceRange (position, position, newText);
}

void CodeDocument::removeRange (Position start, Position end)
{
    replaceRange (start, end, {});
}

//==============================================================================

bool CodeDocument::undo()
{
    return undoManager->undo();
}

bool CodeDocument::redo()
{
    return undoManager->redo();
}

bool CodeDocument::canUndo() const
{
    return undoManager->canUndo();
}

bool CodeDocument::canRedo() const
{
    return undoManager->canRedo();
}

//==============================================================================

void CodeDocument::addListener (Listener* listenerToAdd)
{
    listeners.add (listenerToAdd);
}

void CodeDocument::removeListener (Listener* listenerToRemove)
{
    listeners.remove (listenerToRemove);
}

//==============================================================================

CodeDocument::Position CodeDocument::applyEdit (const Position& start, const Position& end, StringRef newText)
{
    const int startLine = clampLine (start.getLineNumber());
    const int startIndex = clampIndexInLine (startLine, start.getColumnNumber());
    const int endLine = clampLine (end.getLineNumber());
    const int endIndex = clampIndexInLine (endLine, end.getColumnNumber());

    if (startLine > endLine || (startLine == endLine && startIndex > endIndex))
        return Position (*this, startLine, startIndex);

    std::vector<String> newLines;
    if (! newText.isEmpty())
        splitIntoLines (newText, newLines);

    const String head = lines[static_cast<size_t> (startLine)].substring (0, startIndex);
    const String tail = lines[static_cast<size_t> (endLine)].substring (endIndex);

    std::vector<String> replacement;
    replacement.reserve (newLines.size() > 0 ? newLines.size() : 1);

    if (newLines.empty())
    {
        replacement.push_back (head + tail);
    }
    else
    {
        for (size_t i = 0; i < newLines.size(); ++i)
        {
            String line = newLines[i];

            if (i == 0)
                line = head + line;

            if (i + 1 == newLines.size())
                line = line + tail;

            replacement.push_back (line);
        }
    }

    lines.erase (lines.begin() + startLine, lines.begin() + endLine + 1);
    lines.insert (lines.begin() + startLine, replacement.begin(), replacement.end());

    offsetsDirty = true;

    const int lastChangedLine = startLine + static_cast<int> (replacement.size()) - 1;
    listeners.call ([this, startLine, lastChangedLine] (Listener& l)
    {
        l.codeDocumentChanged (*this, startLine, lastChangedLine);
    });

    if (newLines.empty())
        return Position (*this, startLine, startIndex);

    if (newLines.size() == 1)
        return Position (*this, startLine, startIndex + static_cast<int> (newLines[0].length()));

    return Position (*this, startLine + static_cast<int> (newLines.size()) - 1, static_cast<int> (newLines.back().length()));
}

String CodeDocument::getTextInRange (const Position& start, const Position& end) const
{
    const int startLine = clampLine (start.getLineNumber());
    const int startIndex = clampIndexInLine (startLine, start.getColumnNumber());
    const int endLine = clampLine (end.getLineNumber());
    const int endIndex = clampIndexInLine (endLine, end.getColumnNumber());

    if (startLine > endLine || (startLine == endLine && startIndex > endIndex))
        return {};

    String result;
    for (int line = startLine; line <= endLine; ++line)
    {
        const int begin = (line == startLine) ? startIndex : 0;
        const int finish = (line == endLine) ? endIndex : static_cast<int> (lines[static_cast<size_t> (line)].length());

        if (begin < finish)
            result << lines[static_cast<size_t> (line)].substring (begin, finish);

        if (line < endLine)
            result << newLineChars;
    }

    return result;
}

int CodeDocument::clampLine (int lineNumber) const noexcept
{
    return jlimit (0, static_cast<int> (lines.size()) - 1, lineNumber);
}

int CodeDocument::clampIndexInLine (int lineNumber, int column) const noexcept
{
    if (lineNumber < 0 || lineNumber >= static_cast<int> (lines.size()))
        return 0;

    return jlimit (0, static_cast<int> (lines[static_cast<size_t> (lineNumber)].length()), column);
}

void CodeDocument::ensureOffsetsBuilt() const
{
    if (! offsetsDirty)
        return;

    lineStartOffsets.clear();
    lineStartOffsets.reserve (lines.size());

    int offset = 0;
    for (const auto& line : lines)
    {
        lineStartOffsets.push_back (offset);
        offset += static_cast<int> (line.length()) + static_cast<int> (newLineChars.length());
    }

    offsetsDirty = false;
}

//==============================================================================

CodeDocument::Position::Position (const CodeDocument& owner, int line, int column)
    : owner (&owner)
    , line (line)
    , column (column)
{
}

int CodeDocument::Position::getPosition() const
{
    return owner != nullptr ? owner->positionToIndex (*this) : 0;
}

void CodeDocument::Position::setPosition (int newPosition)
{
    if (owner != nullptr)
        *this = owner->indexToPosition (newPosition);
}

bool CodeDocument::Position::operator== (const Position& other) const noexcept
{
    return line == other.line && column == other.column;
}

bool CodeDocument::Position::operator!= (const Position& other) const noexcept
{
    return ! operator== (other);
}

} // namespace yup
