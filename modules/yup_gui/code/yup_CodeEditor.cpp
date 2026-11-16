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

/* Moves line markers along with an edit: markers after the replaced block shift by the line
   delta, markers on lines that no longer exist collapse onto the last line of the new block.
*/
void shiftLineMarkers (std::vector<int>& markers, const CodeDocument::Change& change)
{
    if (markers.empty() || change.getLineDelta() == 0)
        return;

    const int blockEnd = change.firstLine + change.numLinesRemoved;
    const int lastNewLine = change.firstLine + jmax (0, change.numLinesInserted - 1);

    for (auto& line : markers)
    {
        if (line >= blockEnd)
            line += change.getLineDelta();
        else if (line > lastNewLine)
            line = lastNewLine;
    }

    std::sort (markers.begin(), markers.end());
    markers.erase (std::unique (markers.begin(), markers.end()), markers.end());
}

/* Moves diagnostics along with an edit, with the same rules as line markers. Error lines
   collapsing onto the same line are merged so they keep behaving like a set of lines.
*/
void shiftDiagnostics (std::vector<CodeEditor::Diagnostic>& diagnostics, const CodeDocument::Change& change)
{
    if (diagnostics.empty() || change.getLineDelta() == 0)
        return;

    const int blockEnd = change.firstLine + change.numLinesRemoved;
    const int lastNewLine = change.firstLine + jmax (0, change.numLinesInserted - 1);

    const auto shift = [&] (int& line)
    {
        if (line >= blockEnd)
            line += change.getLineDelta();
        else if (line > lastNewLine)
            line = lastNewLine;
    };

    for (auto& diagnostic : diagnostics)
    {
        shift (diagnostic.line);
        shift (diagnostic.endLine);
        diagnostic.endLine = jmax (diagnostic.line, diagnostic.endLine);
    }

    std::vector<int> errorLines;
    diagnostics.erase (std::remove_if (diagnostics.begin(), diagnostics.end(), [&] (const CodeEditor::Diagnostic& diagnostic)
    {
        if (! diagnostic.highlightLine)
            return false;

        if (std::find (errorLines.begin(), errorLines.end(), diagnostic.line) != errorLines.end())
            return true;

        errorLines.push_back (diagnostic.line);
        return false;
    }),
                       diagnostics.end());
}

/* Moves semantic tokens after the replaced block by the line delta and drops the ones on
   the replaced lines, whose content they no longer describe.
*/
void shiftSemanticTokens (std::vector<CodeEditor::SemanticToken>& tokens, const CodeDocument::Change& change)
{
    if (tokens.empty())
        return;

    const int blockEnd = change.firstLine + change.numLinesRemoved;

    tokens.erase (std::remove_if (tokens.begin(), tokens.end(), [&] (const CodeEditor::SemanticToken& token)
    {
        return token.line >= change.firstLine && token.line < blockEnd;
    }),
                  tokens.end());

    for (auto& token : tokens)
    {
        if (token.line >= blockEnd)
            token.line += change.getLineDelta();
    }
}

/* Splits the tokens of a line around a range and inserts a token with the given scope there. */
void overlayToken (std::vector<CodeTokeniser::Token>& tokens, int start, int end, const Identifier& scope)
{
    std::vector<CodeTokeniser::Token> result;
    result.reserve (tokens.size() + 2);

    for (const auto& token : tokens)
    {
        if (token.end <= start || token.start >= end)
        {
            result.push_back (token);
            continue;
        }

        if (token.start < start)
            result.push_back ({ token.start, start, token.type, token.scope });

        if (token.end > end)
            result.push_back ({ end, token.end, token.type, token.scope });
    }

    result.push_back ({ start, end, SyntaxDefinition::scopeToTokenType (scope), scope });

    std::sort (result.begin(), result.end(), [] (const CodeTokeniser::Token& a, const CodeTokeniser::Token& b)
    {
        return a.start < b.start;
    });

    tokens = std::move (result);
}

/* String indexing is linear on UTF-8, so lines scanned character by character are decoded once. */
std::vector<yup_wchar> toCodepoints (const String& text)
{
    std::vector<yup_wchar> result;

    for (auto p = text.getCharPointer(); ! p.isEmpty();)
        result.push_back (p.getAndAdvance());

    return result;
}

} // namespace

//==============================================================================

class CodeEditor::DocumentListener : public CodeDocument::Listener
{
public:
    explicit DocumentListener (CodeEditor& editor)
        : editor (editor)
    {
    }

    void codeDocumentChanged (CodeDocument&, const CodeDocument::Change& change) override
    {
        shiftLineMarkers (editor.breakpointLines, change);
        shiftDiagnostics (editor.diagnostics, change);
        shiftSemanticTokens (editor.semanticTokens, change);

        ++editor.documentChangeCount;
        editor.needsStyledTextUpdate = true;
        editor.searchMatchesNeedUpdate = editor.searchText.isNotEmpty();
        editor.hideHover();
        editor.repaint();
    }

private:
    CodeEditor& editor;
};

//==============================================================================

/* Records the caret and selection around an edit. Undo actions run in reverse order, so this
   action only stores the state to restore: the editor applies it once the whole transaction
   has been undone or redone and the text is back in place.
*/
class CodeEditor::SelectionAction : public UndoableAction
{
public:
    SelectionAction (CodeEditor& editor, int caretBefore, Range<int> selectionBefore, int caretAfter, Range<int> selectionAfter)
        : editor (&editor)
        , before (caretBefore, selectionBefore)
        , after (caretAfter, selectionAfter)
    {
    }

    bool isValid() const override
    {
        return editor != nullptr;
    }

    bool perform (UndoableActionState stateToPerform) override
    {
        auto* codeEditor = static_cast<CodeEditor*> (editor.get());
        if (codeEditor == nullptr)
            return false;

        codeEditor->pendingSelectionRestore = (stateToPerform == UndoableActionState::Undo) ? before : after;
        return true;
    }

private:
    WeakReference<Component> editor;
    std::pair<int, Range<int>> before;
    std::pair<int, Range<int>> after;
};

//==============================================================================

class CodeEditor::Minimap : public Component
{
public:
    explicit Minimap (CodeEditor& editor)
        : editor (editor)
    {
        setMouseCursor (MouseCursor::Hand);
    }

    void paint (Graphics& g) override
    {
        if (editor.document == nullptr)
            return;

        const auto bounds = getLocalBounds();
        const float minimapWidth = bounds.getWidth();
        const auto& scheme = editor.getScheme();

        g.setFillColor (scheme.getColor (CodeEditorScheme::ColorId::minimapBackground).value_or (Color (0xff202020)));
        g.fillRect (bounds);

        const float lineHeight = editor.getLineHeight();
        const int numLines = editor.document->getNumLines();
        const float totalHeight = jmax (1.0f, static_cast<float> (numLines) * lineHeight);
        const float scale = bounds.getHeight() / totalHeight;

        g.setFillColor (scheme.getColor (CodeEditorScheme::ColorId::minimapForeground).value_or (Color (0x66ffffff)));

        const float charWidth = editor.font.getHeight() * 0.6f;

        for (int line = 0; scale > 0.0f && line < numLines;)
        {
            const float y = line * lineHeight * scale;
            if (y >= bounds.getBottom())
                break;

            const int rowStartLine = line;
            int maxLineLength = editor.document->getLine (line).length();
            ++line;

            while (line < numLines && (line - rowStartLine) * lineHeight * scale < 1.0f)
            {
                maxLineLength = jmax (maxLineLength, editor.document->getLine (line).length());
                ++line;
            }

            const float barHeight = jmax (1.0f, (line - rowStartLine) * lineHeight * scale);
            const float barWidth = jmin (minimapWidth, (maxLineLength + 1) * charWidth * scale);

            g.fillRect (Rectangle<float> (0.0f, y, barWidth, barHeight));
        }

        const float viewY = editor.scrollOffset.getY() * scale;
        const float viewH = jmin (bounds.getHeight() - viewY, editor.getTextArea().getHeight() * scale);

        g.setFillColor (scheme.getColor (CodeEditorScheme::ColorId::minimapViewport).value_or (Color (0x3300a0ff)));
        g.fillRect (Rectangle<float> (0.0f, viewY, minimapWidth, jmax (2.0f, viewH)));
    }

    void mouseDown (const MouseEvent& event) override
    {
        if (editor.getWantsKeyboardFocus())
            editor.takeKeyboardFocus();
        scrollToPosition (event.getPosition().to<float>());
    }

    void mouseDrag (const MouseEvent& event) override
    {
        scrollToPosition (event.getPosition().to<float>());
    }

private:
    void scrollToPosition (const Point<float>& position)
    {
        if (editor.document == nullptr)
            return;

        editor.updateStyledTextIfNeeded();

        const auto textArea = editor.getTextArea();
        const float totalHeight = jmax (1.0f, static_cast<float> (editor.document->getNumLines()) * editor.getLineHeight());
        const float scale = textArea.getHeight() / totalHeight;

        const float clampedY = jlimit (getLocalBounds().getY(), getLocalBounds().getBottom(), position.getY());

        editor.scrollOffset.setY ((clampedY - textArea.getY()) / scale - textArea.getHeight() * 0.5f);
        editor.clampScrollOffsetIfNeeded();
        editor.repaint();
    }

    CodeEditor& editor;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Minimap)
};

//==============================================================================

class CodeEditor::HoverPopup : public Component
{
public:
    explicit HoverPopup (CodeEditor& editor)
        : editor (editor)
    {
    }

    void setText (const String& newText)
    {
        const auto textColor = editor.getScheme().getColor (CodeEditorScheme::ColorId::text).value_or (Color (0xffd4d4d4));

        {
            auto modifier = styledText.startUpdate();
            modifier.clear();
            modifier.setMaxSize ({ maxTextWidth, -1.0f });
            modifier.setWrap (StyledText::wrap);
            modifier.setHorizontalAlign (StyledText::left);
            modifier.setVerticalAlign (StyledText::top);
            modifier.appendText (newText, textColor, editor.font);
        }

        const auto textBounds = styledText.getComputedTextBounds();
        setSize (textBounds.getWidth() + padding * 2.0f, textBounds.getHeight() + padding * 2.0f);
    }

    bool hitTest (float, float) override
    {
        return false;
    }

    void paint (Graphics& g) override
    {
        const auto& scheme = editor.getScheme();
        const auto bounds = getLocalBounds();

        g.setFillColor (scheme.getColor (CodeEditorScheme::ColorId::gutterBackground).value_or (Color (0xff252526)));
        g.fillRect (bounds);

        g.setStrokeColor (scheme.getColor (CodeEditorScheme::ColorId::gutterText).value_or (Color (0xff858585)).withAlpha (0.4f));
        g.setStrokeWidth (1.0f);
        g.strokeRect (bounds.reduced (0.5f));

        g.fillFittedText (styledText, bounds.reduced (padding));
    }

private:
    static constexpr float maxTextWidth = 480.0f;
    static constexpr float padding = 6.0f;

    CodeEditor& editor;
    StyledText styledText;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HoverPopup)
};

//==============================================================================

class CodeEditor::CompletionPopup : public Component
{
public:
    static constexpr int maxVisibleRows = 8;

    explicit CompletionPopup (CodeEditor& editor)
        : editor (editor)
    {
        setWantsKeyboardFocus (false);
    }

    float getRowHeight() const
    {
        return jmax (16.0f, editor.getLineHeight() + 4.0f);
    }

    void update()
    {
        const int numItems = static_cast<int> (editor.visibleCompletions.size());
        const int numRows = jmin (numItems, maxVisibleRows);

        firstVisibleRow = jlimit (jmax (0, editor.selectedCompletion - numRows + 1), editor.selectedCompletion, firstVisibleRow);
        firstVisibleRow = jlimit (0, jmax (0, numItems - numRows), firstVisibleRow);

        setSize (jmin (360.0f, editor.getWidth()), numRows * getRowHeight() + 2.0f);
        repaint();
    }

    void paint (Graphics& g) override
    {
        const auto& scheme = editor.getScheme();
        const auto bounds = getLocalBounds();
        const float rowHeight = getRowHeight();

        g.setFillColor (scheme.getColor (CodeEditorScheme::ColorId::gutterBackground).value_or (Color (0xff252526)));
        g.fillRect (bounds);

        const auto textColor = scheme.getColor (CodeEditorScheme::ColorId::text).value_or (Color (0xffd4d4d4));
        const auto detailColor = scheme.getColor (CodeEditorScheme::ColorId::gutterText).value_or (Color (0xff858585));

        const int numItems = static_cast<int> (editor.visibleCompletions.size());
        const int lastRow = jmin (numItems, firstVisibleRow + maxVisibleRows);

        for (int index = firstVisibleRow; index < lastRow; ++index)
        {
            const auto& item = editor.visibleCompletions[static_cast<size_t> (index)];
            const auto row = Rectangle<float> (1.0f, 1.0f + (index - firstVisibleRow) * rowHeight, bounds.getWidth() - 2.0f, rowHeight);

            if (index == editor.selectedCompletion)
            {
                g.setFillColor (scheme.getColor (CodeEditorScheme::ColorId::selection).value_or (Color (0x33264692)));
                g.fillRect (row);
            }

            const auto textArea = row.reduced (6.0f, 0.0f);

            g.setFillColor (textColor);
            g.fillFittedText (item.label, editor.font, textArea, Justification::centerLeft);

            if (item.detail.isNotEmpty())
            {
                g.setFillColor (detailColor);
                g.fillFittedText (item.detail, editor.font.withHeight (editor.font.getHeight() * 0.85f), textArea, Justification::centerRight);
            }
        }

        g.setStrokeColor (detailColor.withAlpha (0.4f));
        g.setStrokeWidth (1.0f);
        g.strokeRect (bounds.reduced (0.5f));
    }

    void mouseDown (const MouseEvent& event) override
    {
        const int index = firstVisibleRow + static_cast<int> ((event.getPosition().getY() - 1.0f) / getRowHeight());
        if (! isPositiveAndBelow (index, static_cast<int> (editor.visibleCompletions.size())))
            return;

        editor.setSelectedCompletionIndex (index);
        editor.acceptCompletion();
    }

    bool mouseWheel (const MouseEvent&, const MouseWheelData& wheelData) override
    {
        const int numItems = static_cast<int> (editor.visibleCompletions.size());
        const int step = wheelData.getDeltaY() > 0.0f ? -1 : 1;

        firstVisibleRow = jlimit (0, jmax (0, numItems - maxVisibleRows), firstVisibleRow + step);
        repaint();
        return true;
    }

private:
    CodeEditor& editor;
    int firstVisibleRow = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompletionPopup)
};

//==============================================================================

CodeEditor::CodeEditor()
    : internalDocument (std::make_unique<CodeDocument>())
    , documentListener (std::make_unique<DocumentListener> (*this))
    , caretTimer (bindFront (&CodeEditor::blinkCaret, this))
{
    document = internalDocument.get();
    initialiseEditor();
}

CodeEditor::CodeEditor (CodeDocument& document)
    : document (&document)
    , documentListener (std::make_unique<DocumentListener> (*this))
    , caretTimer (bindFront (&CodeEditor::blinkCaret, this))
{
    initialiseEditor();
}

CodeEditor::~CodeEditor()
{
    if (document != nullptr)
        document->removeListener (documentListener.get());
}

//==============================================================================

void CodeEditor::initialiseEditor()
{
    setWantsKeyboardFocus (true);
    setMouseCursor (MouseCursor::Text);

    minimap = std::make_unique<Minimap> (*this);

    hoverTimer.onTimer = [this]
    {
        hoverTimer.stopTimer();
        showHover (hoverIndex);
    };

    if (minimapVisible)
        addAndMakeVisible (minimap.get());

    scrollBar = std::make_unique<ScrollBar> (ScrollBar::Orientation::vertical);
    scrollBar->setVisibilityMode (ScrollBar::VisibilityMode::autoHide);
    scrollBar->setScrollBarWidth (12.0f);
    scrollBar->onScrollPositionChanged = [this] (double newPosition)
    {
        scrollOffset.setY (static_cast<float> (newPosition));
        clampScrollOffsetIfNeeded();
        repaint();
    };
    addAndMakeVisible (scrollBar.get());

    horizontalScrollBar = std::make_unique<ScrollBar> (ScrollBar::Orientation::horizontal);
    horizontalScrollBar->setVisibilityMode (ScrollBar::VisibilityMode::autoHide);
    horizontalScrollBar->setScrollBarWidth (12.0f);
    horizontalScrollBar->onScrollPositionChanged = [this] (double newPosition)
    {
        scrollOffset.setX (static_cast<float> (newPosition));
        clampScrollOffsetIfNeeded();
        repaint();
    };
    addAndMakeVisible (horizontalScrollBar.get());

    if (auto theme = ApplicationTheme::getGlobalTheme())
    {
        auto monospaceFont = theme->getDefaultMonospaceFont();
        if (monospaceFont.isEmpty())
            monospaceFont = theme->getDefaultFont();

        if (! monospaceFont.isEmpty())
            font = monospaceFont.withHeight (fontSize);
    }

    if (document != nullptr)
    {
        document->addListener (documentListener.get());
        needsStyledTextUpdate = true;
    }
}

//==============================================================================

void CodeEditor::setDocument (CodeDocument& newDocument)
{
    if (document == &newDocument)
        return;

    if (document != nullptr)
        document->removeListener (documentListener.get());

    document = &newDocument;
    document->addListener (documentListener.get());

    caretPosition = 0;
    selection = {};
    scrollOffset = {};
    diagnostics.clear();
    semanticTokens.clear();
    needsStyledTextUpdate = true;

    repaint();
}

//==============================================================================

void CodeEditor::setSyntaxDefinition (const SyntaxDefinition& definition)
{
    tokeniser.setSyntaxDefinition (definition);
    needsStyledTextUpdate = true;
    repaint();
}

void CodeEditor::setSyntaxDefinition (StringRef languageName)
{
    setSyntaxDefinition (SyntaxDefinition::getBuiltIn (languageName));
}

bool CodeEditor::setSyntaxDefinitionForExtension (StringRef fileExtension)
{
    if (const auto* definition = SyntaxDefinition::getBuiltInForExtension (fileExtension))
    {
        setSyntaxDefinition (*definition);
        return true;
    }

    return false;
}

const SyntaxDefinition& CodeEditor::getSyntaxDefinition() const
{
    return tokeniser.getSyntaxDefinition();
}

//==============================================================================

void CodeEditor::setScheme (const CodeEditorScheme& newScheme)
{
    scheme = newScheme;
    needsStyledTextUpdate = true;
    repaint();
}

const CodeEditorScheme& CodeEditor::getScheme() const noexcept
{
    return scheme;
}

//==============================================================================

const Font& CodeEditor::getFont() const
{
    return font;
}

void CodeEditor::setFont (Font newFont)
{
    font = std::move (newFont);
    fontSize = font.getHeight();
    cachedLineHeight = -1.0f;
    needsStyledTextUpdate = true;
    repaint();
}

//==============================================================================

String CodeEditor::getText() const
{
    return document != nullptr ? document->getText() : String();
}

void CodeEditor::setText (StringRef newText)
{
    if (document == nullptr)
        return;

    document->setText (newText, sendNotification);

    caretPosition = 0;
    selection = {};
    scrollOffset = {};

    updateCaretPosition();
}

String CodeEditor::getSelectedText() const
{
    if (! hasSelection())
        return {};

    return document->getTextBetween (selection.getStart(), selection.getEnd());
}

//==============================================================================

bool CodeEditor::canEdit() const
{
    return document != nullptr && ! readOnly && isEnabled();
}

void CodeEditor::setReadOnly (bool shouldBeReadOnly)
{
    if (readOnly != shouldBeReadOnly)
    {
        readOnly = shouldBeReadOnly;
        repaint();
    }
}

//==============================================================================

void CodeEditor::setCaretPosition (int newPosition)
{
    if (document == nullptr)
        return;

    caretPosition = jlimit (0, document->getNumCharacters(), newPosition);
    selection = Range<int>::emptyRange (caretPosition);
    preferredColumn.reset();

    updateCaretPosition();
}

Range<int> CodeEditor::getSelection() const
{
    return selection;
}

void CodeEditor::setSelection (const Range<int>& newSelection)
{
    if (document == nullptr)
        return;

    const int numChars = document->getNumCharacters();

    selection = Range (jlimit (0, numChars, newSelection.getStart()),
                       jlimit (0, numChars, newSelection.getEnd()));

    caretPosition = selection.getEnd();
    preferredColumn.reset();
    updateCaretPosition();
}

void CodeEditor::selectAll()
{
    if (document == nullptr)
        return;

    selection = Range (0, document->getNumCharacters());
    caretPosition = selection.getEnd();
    updateCaretPosition();
}

bool CodeEditor::hasSelection() const
{
    return ! selection.isEmpty();
}

//==============================================================================

void CodeEditor::insertText (StringRef textToInsert)
{
    if (! canEdit() || textToInsert.isEmpty())
        return;

    const String text (textToInsert);
    auto kind = EditKind::command;

    if (! hasSelection() && text.length() == 1 && text[0] != '\n')
    {
        kind = EditKind::typing;

        const auto caret = document->indexToPosition (caretPosition);
        const auto lineText = document->getLine (caret.getLineNumber());
        const bool endsWord = caret.getColumnNumber() > 0
                           && CharacterFunctions::isWhitespace (text[0])
                           && ! CharacterFunctions::isWhitespace (lineText[caret.getColumnNumber() - 1]);

        if (endsWord)
            lastEditKind = EditKind::command;
    }

    performUndoableEdit ([&]
    {
        const auto endPosition = document->replaceRange (document->indexToPosition (selection.getStart()),
                                                         document->indexToPosition (selection.getEnd()),
                                                         textToInsert);

        caretPosition = document->positionToIndex (endPosition);
        selection = Range<int>::emptyRange (caretPosition);
    },
                         kind);

    updateCaretPosition();
}

void CodeEditor::copy()
{
    const auto selectedText = getSelectedText();
    if (selectedText.isNotEmpty())
        SystemClipboard::copyTextToClipboard (selectedText);
}

void CodeEditor::cut()
{
    if (! canEdit())
        return;

    copy();

    if (hasSelection())
    {
        performUndoableEdit ([this]
        {
            document->removeRange (document->indexToPosition (selection.getStart()),
                                   document->indexToPosition (selection.getEnd()));

            caretPosition = selection.getStart();
            selection = Range<int>::emptyRange (caretPosition);
        },
                             EditKind::command);

        updateCaretPosition();
    }
}

void CodeEditor::paste()
{
    if (! canEdit())
        return;

    const auto clipboardText = SystemClipboard::getTextFromClipboard();
    if (clipboardText.isNotEmpty())
        insertText (clipboardText);
}

bool CodeEditor::undo()
{
    if (document == nullptr)
        return false;

    pendingSelectionRestore.reset();

    const bool result = document->undo();
    if (result && pendingSelectionRestore.has_value())
        restoreSelection (pendingSelectionRestore->first, pendingSelectionRestore->second);
    else
        restoreSelection (caretPosition, Range<int>::emptyRange (caretPosition));

    pendingSelectionRestore.reset();
    return result;
}

bool CodeEditor::redo()
{
    if (document == nullptr)
        return false;

    pendingSelectionRestore.reset();

    const bool result = document->redo();
    if (result && pendingSelectionRestore.has_value())
        restoreSelection (pendingSelectionRestore->first, pendingSelectionRestore->second);
    else
        restoreSelection (caretPosition, Range<int>::emptyRange (caretPosition));

    pendingSelectionRestore.reset();
    return result;
}

void CodeEditor::performUndoableEdit (const std::function<void()>& edit, EditKind kind)
{
    auto undoManager = document->getUndoManager();

    const bool continuesGroup = kind != EditKind::command
                             && kind == lastEditKind
                             && ! hasSelection()
                             && caretPosition == lastEditCaretPosition
                             && documentChangeCount == lastEditChangeCount;

    if (! continuesGroup)
        undoManager->beginNewTransaction();

    const auto caretBefore = caretPosition;
    const auto selectionBefore = selection;
    const auto changeCountBefore = documentChangeCount;

    edit();
    preferredColumn.reset();

    lastEditKind = kind;
    lastEditCaretPosition = caretPosition;
    lastEditChangeCount = documentChangeCount;

    if (documentChangeCount == changeCountBefore)
        return;

    undoManager->perform (new SelectionAction (*this, caretBefore, selectionBefore, caretPosition, selection));
    pendingSelectionRestore.reset();

    if (kind == EditKind::command)
        undoManager->beginNewTransaction();
}

void CodeEditor::restoreSelection (int newCaretPosition, Range<int> newSelection)
{
    const int numChars = document->getNumCharacters();

    selection = Range (jlimit (0, numChars, newSelection.getStart()),
                       jlimit (0, numChars, newSelection.getEnd()));

    caretPosition = jlimit (selection.getStart(), selection.getEnd(), newCaretPosition);
    updateCaretPosition();
}

//==============================================================================

Range<int> CodeEditor::getSelectedLines() const
{
    const auto start = document->indexToPosition (selection.getStart());
    const auto end = document->indexToPosition (selection.getEnd());

    int lastLine = end.getLineNumber();
    if (lastLine > start.getLineNumber() && end.getColumnNumber() == 0)
        --lastLine;

    return { start.getLineNumber(), lastLine + 1 };
}

void CodeEditor::indentSelection()
{
    changeIndentOfSelectedLines (false);
}

void CodeEditor::outdentSelection()
{
    changeIndentOfSelectedLines (true);
}

void CodeEditor::changeIndentOfSelectedLines (bool outdent)
{
    if (! canEdit())
        return;

    const auto lines = getSelectedLines();
    const auto startPosition = document->indexToPosition (selection.getStart());
    const auto endPosition = document->indexToPosition (selection.getEnd());
    const bool caretAtEnd = caretPosition == selection.getEnd();
    const bool keepSelectionLineStart = hasSelection();
    const auto indentText = getIndentUnit();

    performUndoableEdit ([&]
    {
        std::vector<int> columnDeltas (static_cast<size_t> (lines.getLength()), 0);

        for (int line = lines.getStart(); line < lines.getEnd(); ++line)
        {
            const auto lineText = document->getLine (line);
            auto& delta = columnDeltas[static_cast<size_t> (line - lines.getStart())];

            if (outdent)
            {
                int numToRemove = 0;
                if (lineText[0] == '\t')
                    numToRemove = 1;
                else
                    while (numToRemove < tabWidth && lineText[numToRemove] == ' ')
                        ++numToRemove;

                if (numToRemove > 0)
                    document->removeRange (CodeDocument::Position (*document, line, 0), CodeDocument::Position (*document, line, numToRemove));

                delta = -numToRemove;
            }
            else if (lineText.isNotEmpty())
            {
                document->insertText (CodeDocument::Position (*document, line, 0), indentText);
                delta = indentText.length();
            }
        }

        const auto mapPosition = [&] (const CodeDocument::Position& position, bool keepLineStart)
        {
            const int line = position.getLineNumber();
            int column = position.getColumnNumber();

            if (lines.contains (line) && ! (keepLineStart && column == 0))
                column = jmax (0, column + columnDeltas[static_cast<size_t> (line - lines.getStart())]);

            return document->positionToIndex (CodeDocument::Position (*document, line, column));
        };

        const int newStart = mapPosition (startPosition, keepSelectionLineStart);
        const int newEnd = mapPosition (endPosition, false);

        selection = Range (newStart, newEnd);
        caretPosition = caretAtEnd ? newEnd : newStart;
    },
                         EditKind::command);

    updateCaretPosition();
}

void CodeEditor::applyLineEdits (const std::vector<LineEdit>& edits)
{
    if (edits.empty())
        return;

    // Edits are in ascending document order and expressed in original columns.
    const auto mapIndex = [&] (int index, bool isSelectionStart)
    {
        const auto position = document->indexToPosition (index);
        const int line = position.getLineNumber();
        const int column = position.getColumnNumber();
        int shift = 0;

        for (const auto& edit : edits)
        {
            if (edit.line != line || column < edit.column)
                continue;

            if (isSelectionStart && column == edit.column && edit.numToRemove == 0)
                continue;

            shift += edit.insertion.length() - jmin (edit.numToRemove, column - edit.column);
        }

        return std::make_pair (line, column + shift);
    };

    const bool caretAtStart = hasSelection() && caretPosition == selection.getStart();
    const auto newStart = mapIndex (selection.getStart(), hasSelection());
    const auto newEnd = mapIndex (selection.getEnd(), false);

    performUndoableEdit ([&]
    {
        for (auto it = edits.rbegin(); it != edits.rend(); ++it)
        {
            document->replaceRange (CodeDocument::Position (*document, it->line, it->column),
                                    CodeDocument::Position (*document, it->line, it->column + it->numToRemove),
                                    it->insertion);
        }

        const int start = document->positionToIndex (CodeDocument::Position (*document, newStart.first, newStart.second));
        const int end = document->positionToIndex (CodeDocument::Position (*document, newEnd.first, newEnd.second));

        selection = Range<int> (start, jmax (start, end));
        caretPosition = caretAtStart ? selection.getStart() : selection.getEnd();
    },
                         EditKind::command);

    updateCaretPosition();
}

void CodeEditor::replaceLineBlock (Range<int> lines, const String& newText, int newCaret, Range<int> newSelection)
{
    const int lastLine = lines.getEnd() - 1;

    performUndoableEdit ([&]
    {
        document->replaceRange (CodeDocument::Position (*document, lines.getStart(), 0),
                                CodeDocument::Position (*document, lastLine, document->getLine (lastLine).length()),
                                newText);

        selection = newSelection;
        caretPosition = newCaret;
    },
                         EditKind::command);

    updateCaretPosition();
}

void CodeEditor::toggleLineComment()
{
    if (! canEdit())
        return;

    const auto& definition = getSyntaxDefinition();
    const auto& prefix = definition.getLineCommentPrefix();

    if (prefix.isEmpty())
    {
        if (definition.getBlockComment().has_value())
            toggleBlockComment();

        return;
    }

    const auto lines = getSelectedLines();

    std::vector<int> nonBlankLines;
    bool allCommented = true;
    int minIndent = std::numeric_limits<int>::max();

    for (int line = lines.getStart(); line < lines.getEnd(); ++line)
    {
        const auto text = document->getLine (line);
        const int indent = getLineIndent (line).length();

        if (indent == text.length())
            continue;

        nonBlankLines.push_back (line);
        minIndent = jmin (minIndent, indent);
        allCommented = allCommented && text.substring (indent).startsWith (prefix);
    }

    std::vector<LineEdit> edits;

    for (const int line : nonBlankLines)
    {
        if (allCommented)
        {
            const int indent = getLineIndent (line).length();
            const bool hasSpace = document->getLine (line).substring (indent + prefix.length()).startsWithChar (' ');
            edits.push_back ({ line, indent, prefix.length() + (hasSpace ? 1 : 0), {} });
        }
        else
        {
            edits.push_back ({ line, minIndent, 0, prefix + " " });
        }
    }

    applyLineEdits (edits);
}

void CodeEditor::toggleBlockComment()
{
    if (! canEdit())
        return;

    const auto& definition = getSyntaxDefinition();
    const auto& delimiters = definition.getBlockComment();

    if (! delimiters.has_value() || delimiters->start.isEmpty() || delimiters->end.isEmpty())
    {
        if (definition.getLineCommentPrefix().isNotEmpty())
            toggleLineComment();

        return;
    }

    auto range = selection;

    if (! hasSelection())
    {
        const int line = getCurrentLineNumber();
        const int lineStart = document->getLineStartOffset (line);
        range = { lineStart + getLineIndent (line).length(), lineStart + document->getLine (line).length() };
    }

    const auto text = document->getTextBetween (range.getStart(), range.getEnd());
    const auto trimmed = text.trim();
    const auto& open = delimiters->start;
    const auto& close = delimiters->end;

    const auto editAt = [this] (int index, int numToRemove, const String& insertion)
    {
        const auto position = document->indexToPosition (index);
        return LineEdit { position.getLineNumber(), position.getColumnNumber(), numToRemove, insertion };
    };

    if (trimmed.length() >= open.length() + close.length() && trimmed.startsWith (open) && trimmed.endsWith (close))
    {
        const auto inner = trimmed.substring (open.length(), trimmed.length() - close.length());
        const int numOpen = open.length() + (inner.startsWithChar (' ') ? 1 : 0);
        const int numClose = close.length() + (inner.length() > 1 && inner.endsWithChar (' ') ? 1 : 0);

        const int openStart = range.getStart() + text.length() - text.trimStart().length();
        const int closeEnd = range.getEnd() - (text.length() - text.trimEnd().length());

        applyLineEdits ({ editAt (openStart, numOpen, {}), editAt (closeEnd - numClose, numClose, {}) });
    }
    else
    {
        applyLineEdits ({ editAt (range.getStart(), 0, open + " "), editAt (range.getEnd(), 0, " " + close) });
    }
}

void CodeEditor::moveSelectedLines (bool moveUp)
{
    if (! canEdit())
        return;

    const auto lines = getSelectedLines();
    if (moveUp ? lines.getStart() == 0 : lines.getEnd() >= document->getNumLines())
        return;

    const int firstLineStart = document->getLineStartOffset (lines.getStart());
    const int lastLine = lines.getEnd() - 1;
    const auto block = document->getTextBetween (firstLineStart, document->getLineStartOffset (lastLine) + document->getLine (lastLine).length());

    if (moveUp)
    {
        const auto neighbor = document->getLine (lines.getStart() - 1);
        const int delta = -(neighbor.length() + 1);
        replaceLineBlock ({ lines.getStart() - 1, lines.getEnd() }, block + "\n" + neighbor, caretPosition + delta, selection + delta);
    }
    else
    {
        const auto neighbor = document->getLine (lines.getEnd());
        const int delta = neighbor.length() + 1;
        replaceLineBlock ({ lines.getStart(), lines.getEnd() + 1 }, neighbor + "\n" + block, caretPosition + delta, selection + delta);
    }
}

void CodeEditor::duplicateSelectedLines (bool selectCopy)
{
    if (! canEdit())
        return;

    const auto lines = getSelectedLines();
    const int lastLine = lines.getEnd() - 1;
    const auto block = document->getTextBetween (document->getLineStartOffset (lines.getStart()),
                                                 document->getLineStartOffset (lastLine) + document->getLine (lastLine).length());

    const int delta = selectCopy ? block.length() + 1 : 0;
    replaceLineBlock (lines, block + "\n" + block, caretPosition + delta, selection + delta);
}

void CodeEditor::deleteSelectedLines()
{
    if (! canEdit())
        return;

    const auto lines = getSelectedLines();
    const int lastLine = lines.getEnd() - 1;
    const int caretColumn = document->indexToPosition (caretPosition).getColumnNumber();

    const auto lineEnd = [this] (int line)
    {
        return CodeDocument::Position (*document, line, document->getLine (line).length());
    };

    performUndoableEdit ([&]
    {
        if (lines.getEnd() < document->getNumLines())
            document->removeRange (CodeDocument::Position (*document, lines.getStart(), 0), CodeDocument::Position (*document, lines.getEnd(), 0));
        else if (lines.getStart() > 0)
            document->removeRange (lineEnd (lines.getStart() - 1), lineEnd (lastLine));
        else
            document->removeRange (CodeDocument::Position (*document, 0, 0), lineEnd (lastLine));

        const int line = jmin (lines.getStart(), document->getNumLines() - 1);
        const int column = jmin (caretColumn, document->getLine (line).length());

        caretPosition = document->getLineStartOffset (line) + column;
        selection = Range<int>::emptyRange (caretPosition);
    },
                         EditKind::command);

    updateCaretPosition();
}

void CodeEditor::insertLine (bool above)
{
    if (! canEdit())
        return;

    const int line = getCurrentLineNumber();
    const auto indent = getLineIndent (line);

    performUndoableEdit ([&]
    {
        if (above)
        {
            document->insertText (CodeDocument::Position (*document, line, 0), indent + "\n");
            caretPosition = document->getLineStartOffset (line) + indent.length();
        }
        else
        {
            document->insertText (CodeDocument::Position (*document, line, document->getLine (line).length()), "\n" + indent);
            caretPosition = document->getLineStartOffset (line + 1) + indent.length();
        }

        selection = Range<int>::emptyRange (caretPosition);
    },
                         EditKind::command);

    updateCaretPosition();
}

bool CodeEditor::canUndo() const
{
    return document != nullptr && document->canUndo();
}

bool CodeEditor::canRedo() const
{
    return document != nullptr && document->canRedo();
}

//==============================================================================

void CodeEditor::setLineNumbersVisible (bool shouldBeVisible)
{
    if (lineNumbersVisible != shouldBeVisible)
    {
        lineNumbersVisible = shouldBeVisible;
        resized();
        repaint();
    }
}

float CodeEditor::getGutterWidth() const
{
    if (! lineNumbersVisible || document == nullptr)
        return 0.0f;

    const int numLines = document->getNumLines();
    const int digits = jmax (1, String (numLines).length());
    const float charWidth = font.getHeight() * 0.6f;

    return 8.0f + digits * charWidth + 8.0f;
}

int CodeEditor::getTabWidth() const
{
    return tabWidth;
}

void CodeEditor::setIndentUsingTabs (bool shouldUseTabs)
{
    indentUsingTabs = shouldUseTabs;
}

void CodeEditor::setAutoClosingEnabled (bool shouldAutoClose)
{
    autoClosingEnabled = shouldAutoClose;
}

void CodeEditor::setOccurrenceHighlightingEnabled (bool shouldHighlight)
{
    occurrenceHighlighting = shouldHighlight;
    repaint();
}

void CodeEditor::setIndentGuidesVisible (bool shouldBeVisible)
{
    indentGuidesVisible = shouldBeVisible;
    repaint();
}

void CodeEditor::setWhitespaceVisible (bool shouldBeVisible)
{
    whitespaceVisible = shouldBeVisible;
    repaint();
}

String CodeEditor::getIndentUnit() const
{
    return indentUsingTabs ? String ("\t") : String::repeatedString (" ", tabWidth);
}

void CodeEditor::setTabWidth (int newTabWidth)
{
    tabWidth = jmax (1, newTabWidth);
}

void CodeEditor::setScrollOffset (Point<float> newOffset)
{
    scrollOffset = newOffset;
    clampScrollOffsetIfNeeded();
    repaint();
}

void CodeEditor::scrollToLine (int lineNumber)
{
    if (document == nullptr)
        return;

    const auto textArea = getTextArea();
    const float lineHeight = getLineHeight();

    scrollOffset.setY (jmax (0.0f, lineNumber * lineHeight - textArea.getHeight() * 0.5f));
    clampScrollOffsetIfNeeded();
    repaint();
}

//==============================================================================

void CodeEditor::updateStyledTextIfNeeded()
{
    if (document == nullptr)
        return;

    if (needsStyledTextUpdate || windowNeedsExpanding())
    {
        rebuildStyledText();
        needsStyledTextUpdate = false;
    }
}

Range<int> CodeEditor::computeShapingWindow() const
{
    const int numLines = document->getNumLines();
    if (numLines <= 0)
        return { 0, 0 };

    const auto textArea = getTextArea();
    if (textArea.getHeight() <= 0.0f)
        return { 0, numLines - 1 };

    const float lineHeight = getLineHeight();
    const int firstLine = jmax (0, static_cast<int> (scrollOffset.getY() / lineHeight));
    const int lastLine = jmin (numLines - 1, static_cast<int> ((scrollOffset.getY() + textArea.getHeight()) / lineHeight) + 1);

    return { firstLine, jmax (firstLine, lastLine) };
}

bool CodeEditor::windowNeedsExpanding() const
{
    if (windowLastLine < windowFirstLine)
        return true; // Never shaped yet.

    const auto desired = computeShapingWindow();
    return desired.getStart() < windowFirstLine || desired.getEnd() > windowLastLine;
}

void CodeEditor::rebuildStyledText()
{
    if (document == nullptr)
        return;

    shapeWindow (computeShapingWindow());

    const auto recalibrated = computeShapingWindow();
    if (recalibrated.getStart() != windowFirstLine || recalibrated.getEnd() != windowLastLine)
        shapeWindow (recalibrated);
}

void CodeEditor::shapeWindow (Range<int> window)
{
    windowFirstLine = window.getStart();
    windowLastLine = window.getEnd();
    windowStartIndex = document->getLineStartOffset (windowFirstLine);
    windowEndIndex = document->getLineStartOffset (windowLastLine) + document->getLine (windowLastLine).length();

    auto modifier = styledText.startUpdate();
    modifier.clear();
    modifier.setMaxSize ({ -1.0f, -1.0f });
    modifier.setWrap (StyledText::noWrap);
    modifier.setHorizontalAlign (StyledText::left);
    modifier.setVerticalAlign (StyledText::top);

    const auto textColor = scheme.getColor (CodeEditorScheme::ColorId::text).value_or (Color (0xffd4d4d4));

    std::unordered_map<Identifier, Color> scopeColors;

    auto colorForToken = [this, &textColor, &scopeColors] (const CodeTokeniser::Token& token) -> Color
    {
        if (token.scope.isNull())
            return scheme.getColor (token.type).value_or (textColor);

        auto found = scopeColors.find (token.scope);
        if (found == scopeColors.end())
            found = scopeColors.emplace (token.scope, scheme.getColorForScope (token.scope).value_or (textColor)).first;

        return found->second;
    };

    for (int line = windowFirstLine; line <= windowLastLine; ++line)
    {
        const auto lineText = document->getLine (line);
        const auto tokens = getLineTokens (line);

        auto appendRun = [&] (int start, int end, Color color)
        {
            start = jlimit (0, lineText.length(), start);
            end = jlimit (start, lineText.length(), end);

            const auto runText = lineText.substring (start, end);
            if (runText.isNotEmpty())
                modifier.appendText (runText, color, font);
        };

        const bool tokensTileLine = ! tokens.empty()
                                 && tokens.front().start == 0
                                 && tokens.back().end == lineText.length();

        if (tokensTileLine)
        {
            int runStart = 0;
            auto runColor = colorForToken (tokens[0]);

            for (size_t i = 1; i < tokens.size(); ++i)
            {
                const auto tokenColor = colorForToken (tokens[i]);
                if (tokenColor != runColor)
                {
                    appendRun (tokens[runStart].start, tokens[i - 1].end, runColor);
                    runStart = static_cast<int> (i);
                    runColor = tokenColor;
                }
            }

            appendRun (tokens[runStart].start, tokens.back().end, runColor);
        }
        else if (lineText.isNotEmpty())
        {
            modifier.appendText (lineText, textColor, font);
        }

        if (line < windowLastLine)
            modifier.appendText ("\n", textColor, font);
    }
}

//==============================================================================

std::vector<CodeTokeniser::Token> CodeEditor::getLineTokens (int lineIndex)
{
    if (document == nullptr || ! isPositiveAndBelow (lineIndex, document->getNumLines()))
        return {};

    const auto lexical = tokeniser.getTokens (*document, lineIndex);
    std::vector<CodeTokeniser::Token> tokens (lexical.begin(), lexical.end());

    auto first = std::lower_bound (semanticTokens.begin(), semanticTokens.end(), lineIndex, [] (const SemanticToken& token, int line)
    {
        return token.line < line;
    });

    if (first == semanticTokens.end() || first->line != lineIndex)
        return tokens;

    const int lineLength = document->getLine (lineIndex).length();
    const bool tokensTileLine = ! tokens.empty() && tokens.front().start == 0 && tokens.back().end == lineLength;

    if (! tokensTileLine)
    {
        tokens.clear();

        if (lineLength > 0)
            tokens.push_back ({ 0, lineLength, SyntaxDefinition::TokenType::other, {} });
    }

    for (; first != semanticTokens.end() && first->line == lineIndex; ++first)
    {
        const int start = jlimit (0, lineLength, first->column);
        const int end = jlimit (start, lineLength, first->column + first->length);

        if (start < end && first->scope.isValid())
            overlayToken (tokens, start, end, first->scope);
    }

    return tokens;
}

//==============================================================================

void CodeEditor::updateCaretPosition()
{
    caretVisible = true;
    if (hasKeyboardFocus() && ! caretBlinking)
        startCaretBlinking();

    ensureCaretVisible();

    if (isTextInputActive())
        updateTextInputRect();

    repaint();
}

void CodeEditor::ensureCaretVisible()
{
    updateStyledTextIfNeeded();

    if (document == nullptr)
        return;

    const auto textArea = getTextArea();
    const auto caretBounds = getCaretBoundsInDocument();

    bool needsRepaint = false;

    if (caretBounds.isEmpty())
    {
        scrollOffset.setY (jmax (0.0f, scrollOffset.getY()));
        return;
    }

    const float visibleLeft = scrollOffset.getX();
    const float visibleTop = scrollOffset.getY();
    const float visibleRight = visibleLeft + textArea.getWidth();
    const float visibleBottom = visibleTop + textArea.getHeight();

    const float horizontalPadding = 10.0f;
    const float verticalPadding = 5.0f;

    if (caretBounds.getRight() + horizontalPadding > visibleRight)
    {
        scrollOffset.setX (caretBounds.getRight() + horizontalPadding - textArea.getWidth());
        needsRepaint = true;
    }
    else if (caretBounds.getX() - horizontalPadding < visibleLeft)
    {
        scrollOffset.setX (jmax (0.0f, caretBounds.getX() - horizontalPadding));
        needsRepaint = true;
    }

    if (caretBounds.getBottom() + verticalPadding > visibleBottom)
    {
        scrollOffset.setY (caretBounds.getBottom() + verticalPadding - textArea.getHeight());
        needsRepaint = true;
    }
    else if (caretBounds.getY() - verticalPadding < visibleTop)
    {
        scrollOffset.setY (jmax (0.0f, caretBounds.getY() - verticalPadding));
        needsRepaint = true;
    }

    clampScrollOffsetIfNeeded();

    if (needsRepaint)
        repaint();
}

void CodeEditor::clampScrollOffsetIfNeeded()
{
    updateStyledTextIfNeeded();

    if (document == nullptr)
        return;

    const auto textArea = getTextArea();
    const auto textSize = styledText.getComputedTextBounds();
    const float documentHeight = document->getNumLines() * getLineHeight();

    scrollOffset.setX (jmax (0.0f, jmin (scrollOffset.getX(), jmax (0.0f, textSize.getWidth() - textArea.getWidth()))));
    scrollOffset.setY (jmax (0.0f, jmin (scrollOffset.getY(), jmax (0.0f, documentHeight - textArea.getHeight()))));

    updateScrollBar();
}

void CodeEditor::blinkCaret()
{
    caretVisible = ! caretVisible;
    repaint();
}

void CodeEditor::startCaretBlinking()
{
    caretBlinking = true;
    caretTimer.startTimer (500);
}

void CodeEditor::stopCaretBlinking()
{
    caretBlinking = false;
    caretTimer.stopTimer();
    caretVisible = true;
}

//==============================================================================

Rectangle<float> CodeEditor::getTextArea() const
{
    const auto bounds = getLocalBounds().reduced (4.0f);
    const float gutterWidth = getGutterWidth();
    const float minimapWidth = getMinimapWidth();
    const float scrollBarWidth = getScrollBarWidth();

    return Rectangle<float> (bounds.getX() + gutterWidth,
                             bounds.getY(),
                             bounds.getWidth() - gutterWidth - minimapWidth - scrollBarWidth,
                             bounds.getHeight() - getHorizontalScrollBarHeight());
}

float CodeEditor::getScrollBarWidth() const noexcept
{
    return scrollBar != nullptr && scrollBar->isVisible() ? scrollBar->getScrollBarWidth() : 0.0f;
}

float CodeEditor::getHorizontalScrollBarHeight() const noexcept
{
    return horizontalScrollBar != nullptr && horizontalScrollBar->isVisible() ? horizontalScrollBar->getScrollBarWidth() : 0.0f;
}

void CodeEditor::updateScrollBar()
{
    if (scrollBar == nullptr || document == nullptr)
        return;

    const auto textArea = getTextArea();
    const float documentHeight = document->getNumLines() * getLineHeight();

    scrollBar->setRangeLimits (0.0, documentHeight);
    scrollBar->setCurrentRange (scrollOffset.getY(), scrollOffset.getY() + textArea.getHeight());

    if (horizontalScrollBar != nullptr)
    {
        const float contentWidth = styledText.getComputedTextBounds().getWidth();

        horizontalScrollBar->setRangeLimits (0.0, jmax (contentWidth, textArea.getWidth()));
        horizontalScrollBar->setCurrentRange (scrollOffset.getX(), scrollOffset.getX() + textArea.getWidth());
    }
}

float CodeEditor::getMinimapWidth() const
{
    return minimapVisible ? 60.0f : 0.0f;
}

float CodeEditor::getLineHeight() const
{
    if (cachedLineHeight > 0.0f)
        return cachedLineHeight;

    const auto& orderedLines = styledText.getOrderedLines();
    if (! needsStyledTextUpdate && ! orderedLines.empty())
    {
        const auto& glyphLine = orderedLines[0].glyphLine();
        cachedLineHeight = glyphLine.bottom - glyphLine.top;
        return cachedLineHeight;
    }

    return font.getHeight() * 1.2f;
}

Rectangle<float> CodeEditor::getCaretBoundsInDocument() const
{
    if (caretPosition >= windowStartIndex && caretPosition <= windowEndIndex)
    {
        const auto caretBounds = styledText.getCaretBounds (caretPosition - windowStartIndex);
        if (! caretBounds.isEmpty())
            return caretBounds.translated (0.0f, windowFirstLine * getLineHeight());
    }

    if (styledText.getOrderedLines().empty())
        return {};

    const auto position = document->indexToPosition (caretPosition);
    const float lineHeight = getLineHeight();
    const float charWidth = font.getHeight() * 0.6f;

    return Rectangle<float> (position.getColumnNumber() * charWidth,
                             position.getLineNumber() * lineHeight,
                             1.0f,
                             lineHeight);
}

Rectangle<float> CodeEditor::getCaretBounds() const
{
    if (document == nullptr)
        return {};

    const auto textArea = getTextArea();
    const auto caretBounds = getCaretBoundsInDocument();

    if (caretBounds.isEmpty())
    {
        const auto position = document->indexToPosition (caretPosition);
        const float lineHeight = getLineHeight();
        const float charWidth = font.getHeight() * 0.6f;

        return Rectangle<float> (textArea.getX() - scrollOffset.getX() + position.getColumnNumber() * charWidth,
                                 textArea.getY() - scrollOffset.getY() + position.getLineNumber() * lineHeight,
                                 1.0f,
                                 lineHeight);
    }

    return caretBounds.translated (textArea.getTopLeft() - scrollOffset);
}

std::vector<Rectangle<float>> CodeEditor::getSelectionRectanglesInDocument (int startIndex, int endIndex) const
{
    const int clampedStart = jmax (startIndex, windowStartIndex);
    const int clampedEnd = jmin (endIndex, windowEndIndex);

    if (clampedStart >= clampedEnd)
        return {};

    auto rectangles = styledText.getSelectionRectangles (clampedStart - windowStartIndex, clampedEnd - windowStartIndex);

    const float yOffset = windowFirstLine * getLineHeight();
    for (auto& rectangle : rectangles)
        rectangle = rectangle.translated (0.0f, yOffset);

    return rectangles;
}

std::vector<Rectangle<float>> CodeEditor::getViewRectangles (int startIndex, int endIndex) const
{
    const auto offset = getTextArea().getTopLeft() - scrollOffset;

    auto rectangles = getSelectionRectanglesInDocument (startIndex, endIndex);
    for (auto& rectangle : rectangles)
        rectangle = rectangle.translated (offset);

    return rectangles;
}

std::vector<Rectangle<float>> CodeEditor::getSelectedTextAreas() const
{
    if (! hasSelection())
        return {};

    return getViewRectangles (selection.getStart(), selection.getEnd());
}

std::vector<Rectangle<float>> CodeEditor::getSearchMatchAreas() const
{
    if (document == nullptr)
        return {};

    std::vector<Rectangle<float>> rectangles;
    for (const auto& match : getSearchMatches())
    {
        for (const auto& rectangle : getViewRectangles (match.getStart(), match.getEnd()))
            rectangles.push_back (rectangle);
    }

    return rectangles;
}

std::vector<Rectangle<float>> CodeEditor::getBracketMatchAreas() const
{
    const auto match = getBracketMatch (caretPosition);
    if (! match.has_value())
        return {};

    auto rectangles = getViewRectangles (match->getStart(), match->getStart() + 1);
    for (const auto& rectangle : getViewRectangles (match->getEnd(), match->getEnd() + 1))
        rectangles.push_back (rectangle);

    return rectangles;
}

std::vector<Range<int>> CodeEditor::getWordOccurrences() const
{
    if (document == nullptr || ! occurrenceHighlighting)
        return {};

    Range<int> wordRange (findWordStart (caretPosition), findWordEnd (caretPosition));

    if (hasSelection())
    {
        wordRange = selection;

        if (findWordStart (selection.getEnd()) != selection.getStart() || findWordEnd (selection.getStart()) != selection.getEnd())
            return {};
    }

    if (wordRange.isEmpty())
        return {};

    const auto wordStart = document->indexToPosition (wordRange.getStart());
    const auto wordLine = toCodepoints (document->getLine (wordStart.getLineNumber()));
    const auto wordBegin = wordLine.begin() + wordStart.getColumnNumber();
    const std::vector<yup_wchar> word (wordBegin, wordBegin + wordRange.getLength());

    const auto visibleLines = computeShapingWindow();
    std::vector<Range<int>> occurrences;

    for (int line = visibleLines.getStart(); line <= visibleLines.getEnd(); ++line)
    {
        const auto text = toCodepoints (document->getLine (line));
        if (text.size() < word.size())
            continue;

        const int lineStart = document->positionToIndex (CodeDocument::Position (*document, line, 0));

        for (size_t column = 0; column + word.size() <= text.size(); ++column)
        {
            if (column > 0 && ! isWordSeparator (text[column - 1]))
                continue;

            const size_t end = column + word.size();
            if (end < text.size() && ! isWordSeparator (text[end]))
                continue;

            if (std::equal (word.begin(), word.end(), text.begin() + static_cast<std::ptrdiff_t> (column)))
            {
                const int start = lineStart + static_cast<int> (column);
                occurrences.emplace_back (start, start + static_cast<int> (word.size()));
            }
        }
    }

    return occurrences;
}

std::vector<Rectangle<float>> CodeEditor::getWordOccurrenceAreas() const
{
    const auto occurrences = getWordOccurrences();
    if (occurrences.size() < 2)
        return {};

    std::vector<Rectangle<float>> rectangles;
    for (const auto& occurrence : occurrences)
    {
        for (const auto& rectangle : getViewRectangles (occurrence.getStart(), occurrence.getEnd()))
            rectangles.push_back (rectangle);
    }

    return rectangles;
}

float CodeEditor::getCharacterWidth() const
{
    if (document != nullptr)
    {
        const int lastLine = jmin (windowLastLine, document->getNumLines() - 1);

        for (int line = windowFirstLine; line <= lastLine; ++line)
        {
            const auto text = toCodepoints (document->getLine (line));
            const auto found = std::find_if (text.begin(), text.end(), [] (yup_wchar c)
            {
                return c != '\t' && c != '\n' && c != '\r';
            });

            if (found == text.end())
                continue;

            const int index = document->positionToIndex (CodeDocument::Position (*document, line, 0))
                            + static_cast<int> (found - text.begin());

            const auto rectangles = getSelectionRectanglesInDocument (index, index + 1);
            if (! rectangles.empty() && rectangles.front().getWidth() > 0.0f)
                return rectangles.front().getWidth();
        }
    }

    return font.getHeight() * 0.6f;
}

int CodeEditor::getIndentColumns (const String& lineText) const
{
    int columns = 0;

    for (const auto character : toCodepoints (lineText))
    {
        if (character == ' ')
            ++columns;
        else if (character == '\t')
            columns += tabWidth - columns % tabWidth;
        else if (character == '\n' || character == '\r')
            break;
        else
            return columns;
    }

    return -1;
}

std::vector<Rectangle<float>> CodeEditor::getIndentGuideAreas() const
{
    if (document == nullptr || ! indentGuidesVisible)
        return {};

    const int numLines = document->getNumLines();
    const auto visibleLines = computeShapingWindow();
    const auto textArea = getTextArea();
    const float lineHeight = getLineHeight();
    const float charWidth = getCharacterWidth();

    const auto findNonBlankIndent = [this, numLines] (int line, int step)
    {
        constexpr int maxLinesToSearch = 100;

        for (int i = 0; i < maxLinesToSearch && line >= 0 && line < numLines; ++i, line += step)
        {
            const int indent = getIndentColumns (document->getLine (line));
            if (indent >= 0)
                return indent;
        }

        return 0;
    };

    std::vector<Rectangle<float>> guides;

    for (int line = visibleLines.getStart(); line <= visibleLines.getEnd() && line < numLines; ++line)
    {
        int indent = getIndentColumns (document->getLine (line));
        if (indent < 0)
            indent = jmin (findNonBlankIndent (line - 1, -1), findNonBlankIndent (line + 1, 1));

        const float y = textArea.getY() + line * lineHeight - scrollOffset.getY();

        for (int column = 0; column < indent; column += tabWidth)
            guides.emplace_back (textArea.getX() - scrollOffset.getX() + column * charWidth, y, 1.0f, lineHeight);
    }

    return guides;
}

std::vector<CodeEditor::WhitespaceMarker> CodeEditor::getWhitespaceMarkers() const
{
    if (document == nullptr || ! whitespaceVisible)
        return {};

    const auto visibleLines = computeShapingWindow();
    std::vector<WhitespaceMarker> markers;

    for (int line = visibleLines.getStart(); line <= visibleLines.getEnd() && line < document->getNumLines(); ++line)
    {
        const auto text = toCodepoints (document->getLine (line));
        const int lineStart = document->positionToIndex (CodeDocument::Position (*document, line, 0));

        for (size_t column = 0; column < text.size(); ++column)
        {
            const auto character = text[column];
            if (character != ' ' && character != '\t')
                continue;

            const int index = lineStart + static_cast<int> (column);
            for (const auto& area : getViewRectangles (index, index + 1))
                markers.push_back ({ area, character == '\t' });
        }
    }

    return markers;
}

const StyledText& CodeEditor::getStyledText() const noexcept
{
    return styledText;
}

int CodeEditor::getWindowFirstLine() const noexcept
{
    return windowFirstLine;
}

bool CodeEditor::isCaretVisible() const noexcept
{
    return caretVisible;
}

const std::vector<int>& CodeEditor::getBreakpointLines() const noexcept
{
    return breakpointLines;
}

std::vector<int> CodeEditor::getErrorLines() const
{
    std::vector<int> lines;

    for (const auto& diagnostic : diagnostics)
    {
        if (diagnostic.highlightLine)
            lines.push_back (diagnostic.line);
    }

    std::sort (lines.begin(), lines.end());
    lines.erase (std::unique (lines.begin(), lines.end()), lines.end());
    return lines;
}

std::vector<CodeEditor::DiagnosticMarker> CodeEditor::getDiagnosticUnderlineAreas() const
{
    if (document == nullptr || diagnostics.empty())
        return {};

    std::vector<DiagnosticMarker> markers;

    for (const auto& diagnostic : diagnostics)
    {
        if (diagnostic.highlightLine)
            continue;

        const auto range = getDiagnosticRange (diagnostic);
        for (const auto& rectangle : getViewRectangles (range.getStart(), range.getEnd()))
            markers.push_back ({ rectangle, diagnostic.severity });
    }

    std::stable_sort (markers.begin(), markers.end(), [] (const DiagnosticMarker& a, const DiagnosticMarker& b)
    {
        return a.severity > b.severity;
    });

    return markers;
}

std::vector<CodeEditor::DiagnosticMarker> CodeEditor::getDiagnosticGutterMarkers() const
{
    if (document == nullptr || diagnostics.empty() || ! lineNumbersVisible)
        return {};

    const auto textArea = getTextArea();
    const float lineHeight = getLineHeight();
    if (lineHeight <= 0.0f)
        return {};

    const int firstVisibleLine = static_cast<int> (scrollOffset.getY() / lineHeight);
    const int lastVisibleLine = static_cast<int> ((scrollOffset.getY() + textArea.getHeight()) / lineHeight);

    std::map<int, Diagnostic::Severity> severities;

    for (const auto& diagnostic : diagnostics)
    {
        if (diagnostic.highlightLine || diagnostic.severity == Diagnostic::Severity::hint)
            continue;

        const int line = jlimit (0, jmax (0, document->getNumLines() - 1), diagnostic.line);
        if (line < firstVisibleLine || line > lastVisibleLine)
            continue;

        const auto [it, inserted] = severities.emplace (line, diagnostic.severity);
        if (! inserted)
            it->second = jmin (it->second, diagnostic.severity);
    }

    std::vector<DiagnosticMarker> markers;

    for (const auto& [line, severity] : severities)
    {
        const float y = textArea.getY() + line * lineHeight - scrollOffset.getY();
        markers.push_back ({ Rectangle<float> (textArea.getX() - 4.0f, y, 3.0f, lineHeight), severity });
    }

    return markers;
}

int CodeEditor::getGlyphIndexAtPosition (const Point<float>& position) const
{
    const auto textArea = getTextArea();
    const auto relativePosition = position - textArea.getTopLeft() + scrollOffset;

    if (document != nullptr && relativePosition.getY() >= 0.0f)
    {
        // Empty lines (the trailing one in particular) produce no glyphs, so resolve them by line.
        const int line = static_cast<int> (relativePosition.getY() / getLineHeight());
        if (line >= document->getNumLines())
            return document->getNumCharacters();

        if (document->getLine (line).isEmpty())
            return document->getLineStartOffset (line);
    }

    const auto windowRelativePosition = relativePosition.translated (0.0f, -(windowFirstLine * getLineHeight()));

    return styledText.getGlyphIndexAtPosition (windowRelativePosition) + windowStartIndex;
}

Rectangle<float> CodeEditor::getTextInputRect() const
{
    return getCaretBounds();
}

//==============================================================================

void CodeEditor::paint (Graphics& g)
{
    updateStyledTextIfNeeded();

    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void CodeEditor::resized()
{
    const auto bounds = getLocalBounds();

    updateScrollBar();

    const float minimapWidth = getMinimapWidth();

    if (minimap != nullptr && minimapVisible)
    {
        minimap->setBounds (bounds.getRight() - minimapWidth,
                            bounds.getY(),
                            minimapWidth,
                            bounds.getHeight());
    }

    if (scrollBar != nullptr)
    {
        const float width = scrollBar->getScrollBarWidth();

        scrollBar->setBounds (bounds.getRight() - minimapWidth - width,
                              bounds.getY(),
                              width,
                              bounds.getHeight());
    }

    if (horizontalScrollBar != nullptr)
    {
        const float height = horizontalScrollBar->getScrollBarWidth();
        const float left = bounds.getX() + getGutterWidth();

        horizontalScrollBar->setBounds (left,
                                        bounds.getBottom() - height,
                                        bounds.getRight() - minimapWidth - getScrollBarWidth() - left,
                                        height);
    }

    layoutFindBar();
    ensureCaretVisible();
}

//==============================================================================

void CodeEditor::focusGained()
{
    if (isEnabled())
    {
        startCaretBlinking();

        if (! readOnly)
            requestTextInput();
    }
    else
    {
        stopCaretBlinking();
        relinquishTextInput();
    }

    repaint();
}

void CodeEditor::focusLost()
{
    hideCompletions();
    isDragging = false;
    stopCaretBlinking();
    relinquishTextInput();
    repaint();
}

void CodeEditor::enablementChanged()
{
    repaint();
}

//==============================================================================

void CodeEditor::mouseDown (const MouseEvent& event)
{
    hideHover();
    hideCompletions();

    if (getWantsKeyboardFocus())
        takeKeyboardFocus();

    if (document == nullptr)
        return;

    updateStyledTextIfNeeded();

    const auto position = event.getPosition().to<float>();
    const auto bounds = getLocalBounds();

    if (gutterBreakpointsEnabled && lineNumbersVisible && position.getX() <= bounds.getX() + 4.0f + getGutterWidth())
    {
        toggleBreakpointAt (position);
        return;
    }

    const int newCaretPos = getGlyphIndexAtPosition (position);

    moveCaretTo (newCaretPos, event.getModifiers().isShiftDown());

    isDragging = true;
    repaint();
}

void CodeEditor::mouseDrag (const MouseEvent& event)
{
    if (! isDragging || document == nullptr)
        return;

    updateStyledTextIfNeeded();

    moveCaretTo (getGlyphIndexAtPosition (event.getPosition().to<float>()), true);

    repaint();
}

void CodeEditor::mouseUp (const MouseEvent&)
{
    isDragging = false;
}

void CodeEditor::mouseDoubleClick (const MouseEvent& event)
{
    if (getWantsKeyboardFocus())
        takeKeyboardFocus();

    if (document == nullptr)
        return;

    updateStyledTextIfNeeded();

    const auto position = event.getPosition().to<float>();
    const int clickPos = getGlyphIndexAtPosition (position);

    const auto now = Time::getMillisecondCounter();
    const bool isTripleClick = lastDoubleClickTime.has_value()
                            && now - *lastDoubleClickTime < tripleClickTimeoutMs
                            && position.distanceTo (lastDoubleClickPosition) < 4.0f;

    lastDoubleClickTime = isTripleClick ? std::nullopt : std::optional<uint32> (now);
    lastDoubleClickPosition = position;

    if (isTripleClick)
    {
        selectLine (document->indexToPosition (clickPos).getLineNumber());
    }
    else
    {
        selection = Range (findWordStart (clickPos), findWordEnd (clickPos));
        caretPosition = selection.getEnd();
        preferredColumn.reset();

        updateCaretPosition();
    }

    repaint();
}

void CodeEditor::selectLine (int lineNumber)
{
    if (document == nullptr)
        return;

    lineNumber = jlimit (0, document->getNumLines() - 1, lineNumber);

    const int start = document->getLineStartOffset (lineNumber);
    const int end = lineNumber + 1 < document->getNumLines() ? document->getLineStartOffset (lineNumber + 1) : document->getNumCharacters();

    setSelection ({ start, end });
}

bool CodeEditor::mouseWheel (const MouseEvent&, const MouseWheelData& wheelData)
{
    hideHover();
    hideCompletions();

    const auto previousOffset = scrollOffset;
    const float wheelSpeed = 30.0f;

    scrollOffset.setX (scrollOffset.getX() + wheelData.getDeltaX() * wheelSpeed);
    scrollOffset.setY (scrollOffset.getY() - wheelData.getDeltaY() * wheelSpeed);

    clampScrollOffsetIfNeeded();
    repaint();
    return scrollOffset != previousOffset;
}

//==============================================================================

bool CodeEditor::keyDown (const KeyPress& key, const Point<float>&)
{
    if (! isEnabled())
        return false;

    const bool shiftDown = key.getModifiers().isShiftDown();
    const bool ctrlDown = key.getModifiers().isControlDown() || key.getModifiers().isCommandDown();
    const bool altDown = key.getModifiers().isAltDown();

    hideHover();

    if (handleCompletionKey (key))
        return true;

    if (ctrlDown && key.getKey() == KeyPress::spaceKey)
    {
        showCompletions();
    }
    else if (ctrlDown && ! altDown && (key.getKey() == KeyPress::textFKey || key.getKey() == KeyPress::textHKey))
    {
        showFind (key.getKey() == KeyPress::textHKey);
    }
    else if (ctrlDown && ! altDown && key.getKey() == KeyPress::textGKey)
    {
        showGoToLine();
    }
    else if (key.getKey() == KeyPress::f3Key)
    {
        if (findBar == nullptr || findBar->getSearchText().isEmpty())
            showFind();
        else if (shiftDown)
            findPrevious (findBar->getSearchText(), findBar->getSearchOptions());
        else
            findNext (findBar->getSearchText(), findBar->getSearchOptions());
    }
    else if (key.getKey() == KeyPress::escapeKey && findBar != nullptr && findBar->isVisible())
    {
        hideFindBar();
    }
    else if (key.getKey() == KeyPress::leftKey)
    {
        if (ctrlDown)
            moveCaretToWordStart (shiftDown);
        else
            moveCaretLeft (shiftDown);
    }
    else if (key.getKey() == KeyPress::rightKey)
    {
        if (ctrlDown)
            moveCaretToWordEnd (shiftDown);
        else
            moveCaretRight (shiftDown);
    }
    else if (key.getKey() == KeyPress::upKey)
    {
        if (altDown && shiftDown)
            duplicateSelectedLines (false);
        else if (altDown)
            moveSelectedLines (true);
        else if (ctrlDown)
            moveCaretToStart (shiftDown);
        else
            moveCaretUp (shiftDown);
    }
    else if (key.getKey() == KeyPress::downKey)
    {
        if (altDown && shiftDown)
            duplicateSelectedLines (true);
        else if (altDown)
            moveSelectedLines (false);
        else if (ctrlDown)
            moveCaretToEnd (shiftDown);
        else
            moveCaretDown (shiftDown);
    }
    else if (key.getKey() == KeyPress::pageUpKey || key.getKey() == KeyPress::pageDownKey)
    {
        const int numLines = key.getKey() == KeyPress::pageUpKey ? -getLinesPerPage() : getLinesPerPage();

        setScrollOffset ({ scrollOffset.getX(), scrollOffset.getY() + static_cast<float> (numLines) * getLineHeight() });
        moveCaretVertically (numLines, shiftDown);
    }
    else if (key.getKey() == KeyPress::homeKey)
    {
        if (ctrlDown)
            moveCaretToStart (shiftDown);
        else
            moveCaretToStartOfLine (shiftDown);
    }
    else if (key.getKey() == KeyPress::endKey)
    {
        if (ctrlDown)
            moveCaretToEnd (shiftDown);
        else
            moveCaretToEndOfLine (shiftDown);
    }
    else if (key.getKey() == KeyPress::backspaceKey)
    {
        if (ctrlDown)
            deleteWordBackward();
        else
            handleBackspace();

        if (isCompletionVisible())
            updateCompletions();
    }
    else if (key.getKey() == KeyPress::deleteKey)
    {
        if (ctrlDown)
            deleteWordForward();
        else
            handleDelete();
    }
    else if (key.getKey() == KeyPress::enterKey)
    {
        if (ctrlDown)
            insertLine (shiftDown);
        else
            insertNewLine();
    }
    else if (key.getKey() == KeyPress::tabKey)
    {
        if (canEdit())
        {
            const auto selectedLines = getSelectedLines();
            const auto firstLineStart = document->getLineStartOffset (selectedLines.getStart());
            const bool coversWholeLine = hasSelection()
                                      && selection.getStart() == firstLineStart
                                      && selection.getEnd() >= firstLineStart + document->getLine (selectedLines.getStart()).length();

            if (shiftDown)
                outdentSelection();
            else if (selectedLines.getLength() > 1 || coversWholeLine)
                indentSelection();
            else
                insertText (getIndentUnit());
        }
    }
    else if (altDown && shiftDown && key.getKey() == KeyPress::textAKey)
    {
        toggleBlockComment();
    }
    else if (ctrlDown)
    {
        if (key.getKey() == KeyPress::textAKey)
        {
            selectAll();
        }
        else if (key.getKey() == KeyPress::slashKey)
        {
            toggleLineComment();
        }
        else if (shiftDown && key.getKey() == KeyPress::textKKey)
        {
            deleteSelectedLines();
        }
        else if (key.getKey() == KeyPress::textCKey)
        {
            copy();
        }
        else if (key.getKey() == KeyPress::textXKey)
        {
            cut();
        }
        else if (key.getKey() == KeyPress::textVKey)
        {
            paste();
        }
        else if (key.getKey() == KeyPress::textZKey)
        {
            if (shiftDown)
                redo();
            else
                undo();
        }
        else if (key.getKey() == KeyPress::textYKey)
        {
            redo();
        }
        else
        {
            return false;
        }
    }
    else
    {
        // Typed characters arrive through textInput(), so their presses belong here too;
        // only Escape is left to the components around.
        return key.getKey() != KeyPress::escapeKey;
    }

    return true;
}

void CodeEditor::textInput (const String& text)
{
    if (! canEdit() || text.isEmpty())
        return;

    if (text.length() != 1 || ! handleTypedCharacter (text[0]))
        insertText (text);

    if (text.length() == 1 && completionTriggers.containsChar (text[0]))
        showCompletions();
    else if (text.length() == 1 && ! isWordSeparator (text[0]) && isCompletionVisible())
        updateCompletions();
    else
        hideCompletions();
}

bool CodeEditor::handleTypedCharacter (yup_wchar character)
{
    const auto caret = document->indexToPosition (caretPosition);
    const auto lineText = toCodepoints (document->getLine (caret.getLineNumber()));
    const int column = caret.getColumnNumber();

    const auto charAt = [&] (int index) -> yup_wchar
    {
        return isPositiveAndBelow (index, static_cast<int> (lineText.size())) ? lineText[static_cast<size_t> (index)] : 0;
    };

    const auto& autoClosingPairs = getSyntaxDefinition().getAutoClosingPairs();
    const auto isAutoCloser = [&] (yup_wchar c)
    {
        return std::any_of (autoClosingPairs.begin(), autoClosingPairs.end(), [c] (const auto& pair) { return pair.second == c; });
    };

    if (autoClosingEnabled)
    {
        for (const auto& [open, close] : autoClosingPairs)
        {
            if (character != open || ! hasSelection())
                continue;

            const auto selected = getSelectedText();
            const auto wrapped = String::charToString (open) + selected + String::charToString (close);
            const int start = selection.getStart();

            performUndoableEdit ([&]
            {
                document->replaceRange (document->indexToPosition (selection.getStart()),
                                        document->indexToPosition (selection.getEnd()),
                                        wrapped);

                selection = Range<int> (start + 1, start + 1 + selection.getLength());
                caretPosition = selection.getEnd();
            },
                                 EditKind::command);

            updateCaretPosition();
            return true;
        }

        if (! hasSelection() && charAt (column) == character && isAutoCloser (character))
        {
            moveCaretTo (caretPosition + 1, false);
            return true;
        }

        for (const auto& [open, close] : autoClosingPairs)
        {
            if (character != open || hasSelection())
                continue;

            const auto next = charAt (column);
            const auto previous = charAt (column - 1);
            const bool nextAllowsClosing = next == 0 || CharacterFunctions::isWhitespace (next) || isAutoCloser (next);
            const bool previousIsWord = previous != 0 && ! CharacterFunctions::isWhitespace (previous) && ! isWordSeparator (previous);

            if (! nextAllowsClosing || (open == close && previousIsWord))
                break;

            const auto pairText = String::charToString (open) + String::charToString (close);

            performUndoableEdit ([&]
            {
                document->insertText (caret, pairText);
                caretPosition += 1;
                selection = Range<int>::emptyRange (caretPosition);
            },
                                 EditKind::typing);

            updateCaretPosition();
            return true;
        }
    }

    if (hasSelection() || column == 0)
        return false;

    const bool onlyWhitespaceBefore = std::all_of (lineText.begin(), lineText.begin() + jmin (column, static_cast<int> (lineText.size())), [] (yup_wchar c)
    {
        return CharacterFunctions::isWhitespace (c);
    });

    if (! onlyWhitespaceBefore)
        return false;

    for (const auto& [open, close] : getBracketPairs())
    {
        if (character != close || open == close)
            continue;

        const auto opener = findMatchingBracket (caret, open, close, -1);
        if (! opener.has_value())
            return false;

        const auto indent = getLineIndent (document->indexToPosition (*opener).getLineNumber());
        const CodeDocument::Position lineStart (*document, caret.getLineNumber(), 0);

        performUndoableEdit ([&]
        {
            const auto end = document->replaceRange (lineStart, caret, indent + String::charToString (character));
            caretPosition = document->positionToIndex (end);
            selection = Range<int>::emptyRange (caretPosition);
        },
                             EditKind::typing);

        updateCaretPosition();
        return true;
    }

    return false;
}

void CodeEditor::insertNewLine()
{
    if (! canEdit())
        return;

    const auto start = document->indexToPosition (selection.getStart());
    const auto end = document->indexToPosition (selection.getEnd());
    const auto lineText = toCodepoints (document->getLine (start.getLineNumber()));
    const auto endLineText = toCodepoints (document->getLine (end.getLineNumber()));
    const auto indent = getLineIndent (start.getLineNumber());

    int lastIndex = jmin (start.getColumnNumber(), static_cast<int> (lineText.size())) - 1;
    while (lastIndex >= 0 && CharacterFunctions::isWhitespace (lineText[static_cast<size_t> (lastIndex)]))
        --lastIndex;

    const yup_wchar lastBefore = lastIndex >= 0 ? lineText[static_cast<size_t> (lastIndex)] : 0;
    const yup_wchar nextAfter = isPositiveAndBelow (end.getColumnNumber(), static_cast<int> (endLineText.size()))
                                  ? endLineText[static_cast<size_t> (end.getColumnNumber())]
                                  : 0;

    bool increaseIndent = false;
    bool betweenPair = false;

    for (const auto& [open, close] : getBracketPairs())
    {
        if (lastBefore == open && open != close)
        {
            increaseIndent = true;
            betweenPair = nextAfter == close;
        }
    }

    if (const auto* pattern = getSyntaxDefinition().getIncreaseIndentPattern(); ! increaseIndent && pattern != nullptr)
    {
        const CodeRegex::Subject subject (document->getLine (start.getLineNumber()).substring (0, start.getColumnNumber()));
        increaseIndent = pattern->findNext (subject, 0).has_value();
    }

    const auto innerIndent = increaseIndent ? indent + getIndentUnit() : indent;
    const auto text = "\n" + innerIndent + (betweenPair ? "\n" + indent : String());

    performUndoableEdit ([&]
    {
        document->replaceRange (start, end, text);
        caretPosition = selection.getStart() + 1 + innerIndent.length();
        selection = Range<int>::emptyRange (caretPosition);
    },
                         EditKind::command);

    updateCaretPosition();
}

//==============================================================================

void CodeEditor::handleBackspace()
{
    if (! canEdit())
        return;

    if (hasSelection())
    {
        performUndoableEdit ([this]
        {
            document->removeRange (document->indexToPosition (selection.getStart()),
                                   document->indexToPosition (selection.getEnd()));

            caretPosition = selection.getStart();
            selection = Range<int>::emptyRange (caretPosition);
        },
                             EditKind::command);
    }
    else if (caretPosition > 0)
    {
        int numAfter = 0;

        if (autoClosingEnabled)
        {
            const auto caret = document->indexToPosition (caretPosition);
            const auto lineText = toCodepoints (document->getLine (caret.getLineNumber()));
            const auto column = static_cast<size_t> (caret.getColumnNumber());

            if (column > 0 && column < lineText.size())
            {
                for (const auto& [open, close] : getSyntaxDefinition().getAutoClosingPairs())
                    if (lineText[column - 1] == open && lineText[column] == close)
                        numAfter = 1;
            }
        }

        performUndoableEdit ([this, numAfter]
        {
            const int start = caretPosition - 1;
            document->removeRange (document->indexToPosition (start),
                                   document->indexToPosition (caretPosition + numAfter));

            caretPosition = start;
            selection = Range<int>::emptyRange (caretPosition);
        },
                             EditKind::deleting);
    }

    updateCaretPosition();
}

void CodeEditor::handleDelete()
{
    if (! canEdit())
        return;

    if (hasSelection())
    {
        performUndoableEdit ([this]
        {
            document->removeRange (document->indexToPosition (selection.getStart()),
                                   document->indexToPosition (selection.getEnd()));

            caretPosition = selection.getStart();
            selection = Range<int>::emptyRange (caretPosition);
        },
                             EditKind::command);
    }
    else if (caretPosition < document->getNumCharacters())
    {
        performUndoableEdit ([this]
        {
            document->removeRange (document->indexToPosition (caretPosition),
                                   document->indexToPosition (caretPosition + 1));
        },
                             EditKind::deleting);
    }

    updateCaretPosition();
}

//==============================================================================

void CodeEditor::moveCaretTo (int newPosition, bool extendSelection)
{
    const int anchor = extendSelection && caretPosition == selection.getEnd() ? selection.getStart() : selection.getEnd();

    caretPosition = newPosition;
    preferredColumn.reset();

    if (extendSelection)
        selection = Range<int>::between (anchor, caretPosition);
    else
        selection = Range<int>::emptyRange (caretPosition);

    updateCaretPosition();
}

void CodeEditor::moveCaretLeft (bool extendSelection)
{
    if (caretPosition > 0)
        moveCaretTo (caretPosition - 1, extendSelection);
}

void CodeEditor::moveCaretRight (bool extendSelection)
{
    if (document != nullptr && caretPosition < document->getNumCharacters())
        moveCaretTo (caretPosition + 1, extendSelection);
}

void CodeEditor::moveCaretUp (bool extendSelection)
{
    moveCaretVertically (-1, extendSelection);
}

void CodeEditor::moveCaretDown (bool extendSelection)
{
    moveCaretVertically (1, extendSelection);
}

void CodeEditor::moveCaretVertically (int numLines, bool extendSelection)
{
    if (document == nullptr)
        return;

    const auto position = document->indexToPosition (caretPosition);
    const int column = preferredColumn.value_or (position.getColumnNumber());
    const int targetLine = position.getLineNumber() + numLines;

    if (targetLine < 0)
    {
        moveCaretTo (0, extendSelection);
        return;
    }

    if (targetLine >= document->getNumLines())
    {
        moveCaretTo (document->getNumCharacters(), extendSelection);
        return;
    }

    const int targetColumn = jmin (column, document->getLine (targetLine).length());
    moveCaretTo (document->getLineStartOffset (targetLine) + targetColumn, extendSelection);
    preferredColumn = column;
}

int CodeEditor::getLinesPerPage() const
{
    const auto lineHeight = getLineHeight();
    if (lineHeight <= 0.0f)
        return 1;

    return jmax (1, static_cast<int> (getTextArea().getHeight() / lineHeight) - 1);
}

void CodeEditor::moveCaretToStartOfLine (bool extendSelection)
{
    if (document == nullptr)
        return;

    const auto position = document->indexToPosition (caretPosition);
    const int lineStart = document->getLineStartOffset (position.getLineNumber());
    const int firstNonWhitespace = lineStart + getLineIndent (position.getLineNumber()).length();

    moveCaretTo (caretPosition == firstNonWhitespace ? lineStart : firstNonWhitespace, extendSelection);
}

void CodeEditor::moveCaretToEndOfLine (bool extendSelection)
{
    if (document == nullptr)
        return;

    const auto position = document->indexToPosition (caretPosition);
    const int newPosition = document->positionToIndex ({ *document,
                                                         position.getLineNumber(),
                                                         document->getLine (position.getLineNumber()).length() });

    moveCaretTo (newPosition, extendSelection);
}

void CodeEditor::moveCaretToStart (bool extendSelection)
{
    moveCaretTo (0, extendSelection);
}

void CodeEditor::moveCaretToEnd (bool extendSelection)
{
    if (document == nullptr)
        return;

    moveCaretTo (document->getNumCharacters(), extendSelection);
}

//==============================================================================

bool CodeEditor::isWordSeparator (yup_wchar character) const
{
    return getSyntaxDefinition().isWordSeparator (character);
}

const std::vector<std::pair<yup_wchar, yup_wchar>>& CodeEditor::getBracketPairs() const
{
    return getSyntaxDefinition().getBracketPairs();
}

int CodeEditor::findWordStart (int position) const
{
    if (document == nullptr)
        return 0;

    const auto start = document->indexToPosition (position);
    const auto line = toCodepoints (document->getLine (start.getLineNumber()));
    int column = jmin (start.getColumnNumber(), static_cast<int> (line.size()));

    if (column > 0 && isWordSeparator (line[static_cast<size_t> (column - 1)]))
        return document->positionToIndex (CodeDocument::Position (*document, start.getLineNumber(), column));

    while (column > 0 && ! isWordSeparator (line[static_cast<size_t> (column - 1)]))
        --column;

    return document->positionToIndex (CodeDocument::Position (*document, start.getLineNumber(), column));
}

int CodeEditor::findWordEnd (int position) const
{
    if (document == nullptr)
        return 0;

    const auto start = document->indexToPosition (position);
    const auto line = toCodepoints (document->getLine (start.getLineNumber()));
    int column = jmin (start.getColumnNumber(), static_cast<int> (line.size()));

    while (column < static_cast<int> (line.size()) && ! isWordSeparator (line[static_cast<size_t> (column)]))
        ++column;

    return document->positionToIndex (CodeDocument::Position (*document, start.getLineNumber(), column));
}

void CodeEditor::moveCaretToWordStart (bool extendSelection)
{
    moveCaretTo (findWordStart (caretPosition), extendSelection);
}

void CodeEditor::moveCaretToWordEnd (bool extendSelection)
{
    moveCaretTo (findWordEnd (caretPosition), extendSelection);
}

void CodeEditor::deleteWordBackward()
{
    if (! canEdit())
        return;

    if (hasSelection())
    {
        handleBackspace();
        return;
    }

    const int wordStart = findWordStart (caretPosition);

    if (wordStart < caretPosition)
    {
        performUndoableEdit ([&]
        {
            document->removeRange (document->indexToPosition (wordStart),
                                   document->indexToPosition (caretPosition));

            caretPosition = wordStart;
            selection = Range<int>::emptyRange (caretPosition);
        },
                             EditKind::command);

        updateCaretPosition();
    }
}

void CodeEditor::deleteWordForward()
{
    if (! canEdit())
        return;

    if (hasSelection())
    {
        handleDelete();
        return;
    }

    const int wordEnd = findWordEnd (caretPosition);

    if (wordEnd > caretPosition)
    {
        performUndoableEdit ([&]
        {
            document->removeRange (document->indexToPosition (caretPosition),
                                   document->indexToPosition (wordEnd));
        },
                             EditKind::command);

        updateCaretPosition();
    }
}

int CodeEditor::getCurrentLineNumber() const
{
    return document != nullptr ? document->indexToPosition (caretPosition).getLineNumber() : 0;
}

//==============================================================================
// Find / replace
//==============================================================================

std::vector<CodeEditor::SearchMatch> CodeEditor::findMatches (StringRef searchTextToFind, const SearchOptions& options) const
{
    if (document == nullptr || searchTextToFind.isEmpty())
        return {};

    const String search (searchTextToFind);

    CodeRegex regex;
    if (options.regex)
    {
        auto compiled = CodeRegex::compile (search, options.caseSensitive);
        if (compiled.failed())
            return {};

        regex = compiled.getValue();
    }

    std::vector<SearchMatch> results;

    const auto addIfAccepted = [&] (const std::vector<yup_wchar>& characters, int offset, int start, int end, StringArray groups)
    {
        const int textLength = static_cast<int> (characters.size());
        const bool startBoundary = start == 0 || isWordSeparator (characters[static_cast<size_t> (start - 1)]);
        const bool endBoundary = end >= textLength || isWordSeparator (characters[static_cast<size_t> (end)]);

        if (! options.wholeWord || (startBoundary && endBoundary))
            results.push_back ({ Range<int> (offset + start, offset + end), std::move (groups) });
    };

    const auto searchIn = [&] (const String& text, int offset)
    {
        const auto characters = toCodepoints (text);
        const int textLength = static_cast<int> (characters.size());

        if (options.regex)
        {
            const CodeRegex::Subject subject (text);

            int index = 0;
            while (index <= textLength)
            {
                const auto match = regex.findNext (subject, index);
                if (! match.has_value())
                    break;

                const auto range = match->groups.front();
                if (! range.isEmpty())
                    addIfAccepted (characters, offset, range.getStart(), range.getEnd(), match->texts);

                index = range.isEmpty() ? range.getEnd() + 1 : range.getEnd();
            }

            return;
        }

        const int searchLength = search.length();

        int index = 0;
        while (index < textLength)
        {
            const int found = options.caseSensitive ? text.indexOf (index, search)
                                                    : text.indexOfIgnoreCase (index, search);
            if (found < 0)
                break;

            addIfAccepted (characters, offset, found, found + searchLength, {});
            index = found + searchLength;
        }
    };

    // Line breaks are word separators, so a needle without one is matched line by line
    if (! options.regex && search.containsChar ('\n'))
    {
        searchIn (document->getTextBetween (0, document->getNumCharacters()), 0);
    }
    else
    {
        for (int line = 0; line < document->getNumLines(); ++line)
            searchIn (document->getLine (line), document->getLineStartOffset (line));
    }

    return results;
}

String CodeEditor::expandReplacement (const String& replacement, const StringArray& groups)
{
    if (! replacement.containsChar ('$'))
        return replacement;

    const auto characters = toCodepoints (replacement);
    String result;

    for (size_t i = 0; i < characters.size(); ++i)
    {
        const auto character = characters[i];
        const auto next = i + 1 < characters.size() ? characters[i + 1] : yup_wchar (0);

        if (character == '$' && next == '$')
        {
            result << '$';
            ++i;
        }
        else if (character == '$' && next >= '0' && next <= '9')
        {
            const int group = static_cast<int> (next - '0');
            if (group < groups.size())
                result << groups[group];

            ++i;
        }
        else
        {
            result << String::charToString (character);
        }
    }

    return result;
}

std::vector<Range<int>> CodeEditor::findAll (StringRef searchTextToFind, const SearchOptions& options) const
{
    std::vector<Range<int>> ranges;

    for (const auto& match : findMatches (searchTextToFind, options))
        ranges.push_back (match.range);

    return ranges;
}

std::vector<Range<int>> CodeEditor::findAll (StringRef searchTextToFind, bool caseSensitive, bool wholeWord) const
{
    return findAll (searchTextToFind, SearchOptions { caseSensitive, wholeWord, false });
}

bool CodeEditor::findNext (StringRef searchTextToFind, const SearchOptions& options, bool wrapAround)
{
    const auto matches = findAll (searchTextToFind, options);
    if (matches.empty())
        return false;

    const int from = selection.getEnd();

    for (const auto& match : matches)
    {
        if (match.getStart() >= from || (! hasSelection() && match.getEnd() == from))
        {
            setSelection (match);
            return true;
        }
    }

    if (wrapAround)
    {
        setSelection (matches.front());
        return true;
    }

    selection = Range<int>::emptyRange (caretPosition);
    repaint();

    return false;
}

bool CodeEditor::findNext (StringRef searchTextToFind, bool caseSensitive, bool wrapAround)
{
    return findNext (searchTextToFind, SearchOptions { caseSensitive, false, false }, wrapAround);
}

bool CodeEditor::findPrevious (StringRef searchTextToFind, const SearchOptions& options, bool wrapAround)
{
    const auto matches = findAll (searchTextToFind, options);
    if (matches.empty())
        return false;

    const int from = selection.getStart();

    for (auto it = matches.rbegin(); it != matches.rend(); ++it)
    {
        if (it->getEnd() <= from)
        {
            setSelection (*it);
            return true;
        }
    }

    if (wrapAround)
    {
        setSelection (matches.back());
        return true;
    }

    selection = Range<int>::emptyRange (caretPosition);
    repaint();

    return false;
}

bool CodeEditor::findPrevious (StringRef searchTextToFind, bool caseSensitive, bool wrapAround)
{
    return findPrevious (searchTextToFind, SearchOptions { caseSensitive, false, false }, wrapAround);
}

int CodeEditor::replaceNext (StringRef searchTextToFind, StringRef replacement, const SearchOptions& options)
{
    if (! canEdit() || ! hasSelection())
        return 0;

    const auto matches = findMatches (searchTextToFind, options);
    const auto selected = std::find_if (matches.begin(), matches.end(), [this] (const SearchMatch& match)
    {
        return match.range == selection;
    });

    if (selected == matches.end())
        return 0;

    const auto replacementText = options.regex ? expandReplacement (replacement, selected->groups) : String (replacement);

    performUndoableEdit ([&]
    {
        const auto endPos = document->replaceRange (document->indexToPosition (selection.getStart()),
                                                    document->indexToPosition (selection.getEnd()),
                                                    replacementText);

        caretPosition = document->positionToIndex (endPos);
        selection = Range<int>::emptyRange (caretPosition);
    },
                         EditKind::command);

    findNext (searchTextToFind, options);

    return 1;
}

int CodeEditor::replaceNext (StringRef searchTextToFind, StringRef replacement, bool caseSensitive)
{
    return replaceNext (searchTextToFind, replacement, SearchOptions { caseSensitive, false, false });
}

int CodeEditor::replaceAll (StringRef searchTextToFind, StringRef replacement, const SearchOptions& options)
{
    if (! canEdit())
        return 0;

    const auto matches = findMatches (searchTextToFind, options);
    if (matches.empty())
        return 0;

    performUndoableEdit ([&]
    {
        for (auto it = matches.rbegin(); it != matches.rend(); ++it)
        {
            document->replaceRange (document->indexToPosition (it->range.getStart()),
                                    document->indexToPosition (it->range.getEnd()),
                                    options.regex ? expandReplacement (replacement, it->groups) : String (replacement));
        }

        caretPosition = jlimit (0, document->getNumCharacters(), caretPosition);
        selection = Range<int>::emptyRange (caretPosition);
    },
                         EditKind::command);

    updateCaretPosition();
    return static_cast<int> (matches.size());
}

int CodeEditor::replaceAll (StringRef searchTextToFind, StringRef replacement, bool caseSensitive)
{
    return replaceAll (searchTextToFind, replacement, SearchOptions { caseSensitive, false, false });
}

void CodeEditor::highlightSearchMatches (StringRef searchTextToFind, const SearchOptions& options)
{
    searchText = searchTextToFind;
    searchOptions = options;
    searchMatchesNeedUpdate = true;

    repaint();
}

void CodeEditor::highlightSearchMatches (StringRef searchTextToFind, bool caseSensitive)
{
    highlightSearchMatches (searchTextToFind, SearchOptions { caseSensitive, false, false });
}

const std::vector<Range<int>>& CodeEditor::getSearchMatches() const
{
    if (searchMatchesNeedUpdate)
    {
        searchMatches = findAll (searchText, searchOptions);
        searchMatchesNeedUpdate = false;
    }

    return searchMatches;
}

void CodeEditor::clearSearchHighlights()
{
    searchText.clear();
    searchOptions = {};
    searchMatches.clear();
    searchMatchesNeedUpdate = false;

    repaint();
}

//==============================================================================
// Hover

void CodeEditor::setHoverProvider (HoverProvider newProvider)
{
    hoverProvider = std::move (newProvider);
    hideHover();
}

String CodeEditor::getHoverText (int characterIndex) const
{
    StringArray parts;

    for (const auto& diagnostic : getDiagnosticsAt (characterIndex))
    {
        if (diagnostic.message.isEmpty() || characterIndex >= getDiagnosticRange (diagnostic).getEnd())
            continue;

        parts.add (diagnostic.source.isNotEmpty() ? diagnostic.source + ": " + diagnostic.message : diagnostic.message);
    }

    if (hoverProvider != nullptr)
    {
        const auto text = hoverProvider (*this, characterIndex);
        if (text.isNotEmpty())
            parts.add (text);
    }

    return parts.joinIntoString ("\n\n");
}

void CodeEditor::showHover (int characterIndex)
{
    const auto text = characterIndex >= 0 ? getHoverText (characterIndex) : String();
    if (text.isEmpty())
    {
        hideHover();
        return;
    }

    if (hoverPopup == nullptr)
    {
        hoverPopup = std::make_unique<HoverPopup> (*this);
        addChildComponent (*hoverPopup);
    }

    updateStyledTextIfNeeded();
    hoverPopup->setText (text);

    const auto characterAreas = getViewRectangles (characterIndex, characterIndex + 1);
    const auto anchor = characterAreas.empty() ? getCaretBounds() : characterAreas.front();
    const auto bounds = getLocalBounds();
    const auto size = hoverPopup->getSize();

    float y = anchor.getBottom() + 2.0f;
    if (y + size.getHeight() > bounds.getBottom() && anchor.getY() - size.getHeight() - 2.0f >= bounds.getY())
        y = anchor.getY() - size.getHeight() - 2.0f;

    const float x = jlimit (bounds.getX(), jmax (bounds.getX(), bounds.getRight() - size.getWidth()), anchor.getX());

    hoverPopup->setTopLeft ({ x, y });
    hoverPopup->setVisible (true);
    hoverPopup->toFront (false);
}

void CodeEditor::hideHover()
{
    hoverTimer.stopTimer();

    if (hoverPopup != nullptr && hoverPopup->isVisible())
        hoverPopup->setVisible (false);
}

bool CodeEditor::isHoverVisible() const noexcept
{
    return hoverPopup != nullptr && hoverPopup->isVisible();
}

int CodeEditor::getCharacterIndexAt (const Point<float>& position)
{
    if (document == nullptr || ! getTextArea().contains (position))
        return -1;

    updateStyledTextIfNeeded();

    const int index = getGlyphIndexAtPosition (position);

    // The glyph index is the nearest caret position, the character under the mouse is either side
    for (const int candidate : { index, index - 1 })
    {
        for (const auto& area : getViewRectangles (candidate, candidate + 1))
        {
            if (area.contains (position))
                return candidate;
        }
    }

    return -1;
}

void CodeEditor::mouseMove (const MouseEvent& event)
{
    const int index = getCharacterIndexAt (event.getPosition().to<float>());
    if (index == hoverIndex)
        return;

    hideHover();
    hoverIndex = index;

    if (index >= 0 && (hoverProvider != nullptr || ! diagnostics.empty()))
        hoverTimer.startTimer (jmax (1, hoverDelayMs));
}

void CodeEditor::mouseExit (const MouseEvent&)
{
    hideHover();
    hoverIndex = -1;
}

//==============================================================================
// Completion

void CodeEditor::setCompletionProvider (CompletionProvider newProvider)
{
    completionProvider = std::move (newProvider);
    hideCompletions();
}

void CodeEditor::showCompletions()
{
    if (document == nullptr || completionProvider == nullptr || ! canEdit())
    {
        hideCompletions();
        return;
    }

    completionStart = findWordStart (caretPosition);
    completionItems = completionProvider (*this, caretPosition);
    selectedCompletion = 0;

    updateCompletions();
}

void CodeEditor::hideCompletions()
{
    completionItems.clear();
    visibleCompletions.clear();
    selectedCompletion = 0;

    if (completionPopup != nullptr && completionPopup->isVisible())
        completionPopup->setVisible (false);
}

bool CodeEditor::isCompletionVisible() const noexcept
{
    return completionPopup != nullptr && completionPopup->isVisible();
}

void CodeEditor::setSelectedCompletionIndex (int newIndex)
{
    if (visibleCompletions.empty())
        return;

    selectedCompletion = jlimit (0, static_cast<int> (visibleCompletions.size()) - 1, newIndex);

    if (completionPopup != nullptr)
        completionPopup->update();
}

bool CodeEditor::acceptCompletion()
{
    if (! isCompletionVisible() || ! isPositiveAndBelow (selectedCompletion, static_cast<int> (visibleCompletions.size())))
        return false;

    const auto& item = visibleCompletions[static_cast<size_t> (selectedCompletion)];
    const auto text = item.insertText.isNotEmpty() ? item.insertText : item.label;
    const int start = completionStart;

    hideCompletions();

    setSelection ({ start, caretPosition });
    if (hasSelection() || text.isNotEmpty())
        insertText (text);

    return true;
}

void CodeEditor::updateCompletions()
{
    const auto caretLine = document != nullptr ? document->indexToPosition (caretPosition).getLineNumber() : -1;

    if (document == nullptr
        || completionItems.empty()
        || hasSelection()
        || caretPosition < completionStart
        || document->indexToPosition (completionStart).getLineNumber() != caretLine)
    {
        hideCompletions();
        return;
    }

    const auto prefix = document->getTextBetween (completionStart, caretPosition).toLowerCase();

    // Labels starting with the prefix rank before labels only containing its characters in order
    const auto matchRank = [&prefix] (const String& label)
    {
        const auto lowerLabel = label.toLowerCase();
        if (lowerLabel.startsWith (prefix))
            return 2;

        int searchFrom = 0;
        for (auto p = prefix.getCharPointer(); ! p.isEmpty();)
        {
            searchFrom = lowerLabel.indexOfChar (searchFrom, p.getAndAdvance());
            if (searchFrom < 0)
                return 0;

            ++searchFrom;
        }

        return 1;
    };

    std::vector<std::pair<int, const CompletionItem*>> ranked;
    for (const auto& item : completionItems)
    {
        if (const int rank = matchRank (item.label); rank > 0)
            ranked.emplace_back (rank, &item);
    }

    std::stable_sort (ranked.begin(), ranked.end(), [] (const auto& a, const auto& b)
    {
        return a.first > b.first;
    });

    std::vector<CompletionItem> filtered;
    filtered.reserve (ranked.size());
    for (const auto& entry : ranked)
        filtered.push_back (*entry.second);

    if (filtered.empty())
    {
        hideCompletions();
        return;
    }

    visibleCompletions = std::move (filtered);
    selectedCompletion = 0;

    if (completionPopup == nullptr)
    {
        completionPopup = std::make_unique<CompletionPopup> (*this);
        addChildComponent (*completionPopup);
    }

    completionPopup->update();

    updateStyledTextIfNeeded();

    const auto caret = getCaretBounds();
    const auto bounds = getLocalBounds();
    const auto size = completionPopup->getSize();

    float y = caret.getBottom() + 2.0f;
    if (y + size.getHeight() > bounds.getBottom() && caret.getY() - size.getHeight() - 2.0f >= bounds.getY())
        y = caret.getY() - size.getHeight() - 2.0f;

    const float x = jlimit (bounds.getX(), jmax (bounds.getX(), bounds.getRight() - size.getWidth()), caret.getX());

    completionPopup->setTopLeft ({ x, y });
    completionPopup->setVisible (true);
    completionPopup->toFront (false);
}

bool CodeEditor::handleCompletionKey (const KeyPress& key)
{
    if (! isCompletionVisible())
        return false;

    const auto modifiers = key.getModifiers();
    const bool plain = ! (modifiers.isShiftDown() || modifiers.isControlDown() || modifiers.isCommandDown() || modifiers.isAltDown());
    const int numItems = static_cast<int> (visibleCompletions.size());
    const int k = key.getKey();

    if (plain && k == KeyPress::upKey)
        setSelectedCompletionIndex (selectedCompletion > 0 ? selectedCompletion - 1 : numItems - 1);
    else if (plain && k == KeyPress::downKey)
        setSelectedCompletionIndex (selectedCompletion < numItems - 1 ? selectedCompletion + 1 : 0);
    else if (plain && k == KeyPress::pageUpKey)
        setSelectedCompletionIndex (selectedCompletion - CompletionPopup::maxVisibleRows);
    else if (plain && k == KeyPress::pageDownKey)
        setSelectedCompletionIndex (selectedCompletion + CompletionPopup::maxVisibleRows);
    else if (plain && (k == KeyPress::enterKey || k == KeyPress::tabKey))
        acceptCompletion();
    else if (k == KeyPress::escapeKey)
        hideCompletions();
    else
    {
        // Moving the caret or running a command closes the popup, typing keeps filtering it
        const bool movesCaret = k == KeyPress::leftKey || k == KeyPress::rightKey || k == KeyPress::upKey || k == KeyPress::downKey
                             || k == KeyPress::homeKey || k == KeyPress::endKey || k == KeyPress::pageUpKey || k == KeyPress::pageDownKey
                             || k == KeyPress::enterKey || k == KeyPress::tabKey;

        if (movesCaret || ((modifiers.isControlDown() || modifiers.isCommandDown()) && k != KeyPress::spaceKey))
            hideCompletions();

        return false;
    }

    return true;
}

//==============================================================================
// Find bar and go to line

CodeFindBar& CodeEditor::getOrCreateFindBar()
{
    if (findBar == nullptr)
    {
        findBar = std::make_unique<CodeFindBar> (*this);
        addChildComponent (*findBar);
    }

    return *findBar;
}

void CodeEditor::layoutFindBar()
{
    if (findBar == nullptr)
        return;

    const auto textArea = getTextArea();
    const float width = jmin (textArea.getWidth(), 460.0f);

    findBar->setBounds (textArea.getRight() - width, textArea.getY(), width, findBar->getPreferredHeight());
}

void CodeEditor::showFind (bool withReplace)
{
    auto& bar = getOrCreateFindBar();
    bar.show (withReplace && ! isReadOnly() ? CodeFindBar::Mode::replace : CodeFindBar::Mode::find);
    layoutFindBar();
}

void CodeEditor::showGoToLine()
{
    getOrCreateFindBar().show (CodeFindBar::Mode::goToLine);
    layoutFindBar();
}

void CodeEditor::hideFindBar()
{
    if (findBar != nullptr && findBar->isVisible())
        findBar->hide();
}

void CodeEditor::goToLine (int lineNumber, int column)
{
    if (document == nullptr)
        return;

    const int line = jlimit (0, jmax (0, document->getNumLines() - 1), lineNumber);
    const int lineLength = document->getLine (line).length();
    const int position = document->getLineStartOffset (line) + jlimit (0, lineLength, column);

    setCaretPosition (position);
    scrollToLine (line);
}

//==============================================================================
// Bracket matching
//==============================================================================

std::optional<Range<int>> CodeEditor::getBracketMatch (int position) const
{
    if (document == nullptr)
        return std::nullopt;

    const int numChars = document->getNumCharacters();
    if (numChars == 0)
        return std::nullopt;

    position = jlimit (0, numChars - 1, position);

    for (const int probe : { position, position - 1 })
    {
        if (probe < 0 || probe >= numChars)
            continue;

        const auto probePosition = document->indexToPosition (probe);
        const auto line = toCodepoints (document->getLine (probePosition.getLineNumber()));

        if (probePosition.getColumnNumber() >= static_cast<int> (line.size()))
            continue;

        const auto character = line[static_cast<size_t> (probePosition.getColumnNumber())];

        for (const auto& [open, close] : getBracketPairs())
        {
            if (character != open && character != close)
                continue;

            const int direction = character == open ? 1 : -1;
            if (const auto match = findMatchingBracket (probePosition, open, close, direction))
                return direction > 0 ? Range<int> (probe, *match) : Range<int> (*match, probe);
        }
    }

    return std::nullopt;
}

std::optional<int> CodeEditor::findMatchingBracket (const CodeDocument::Position& start, yup_wchar open, yup_wchar close, int direction) const
{
    const auto nested = direction > 0 ? open : close;
    const auto closing = direction > 0 ? close : open;
    int depth = 1;

    for (int lineNumber = start.getLineNumber(); lineNumber >= 0 && lineNumber < document->getNumLines(); lineNumber += direction)
    {
        const auto line = toCodepoints (document->getLine (lineNumber));
        const int numColumns = static_cast<int> (line.size());

        int column = direction > 0 ? 0 : numColumns - 1;
        if (lineNumber == start.getLineNumber())
            column = start.getColumnNumber() + direction;

        for (; column >= 0 && column < numColumns; column += direction)
        {
            const auto character = line[static_cast<size_t> (column)];

            if (character == nested)
                ++depth;
            else if (character == closing && --depth == 0)
                return document->getLineStartOffset (lineNumber) + column;
        }
    }

    return std::nullopt;
}

//==============================================================================
// Breakpoints
//==============================================================================

void CodeEditor::setBreakpoint (int lineNumber, bool shouldBeEnabled)
{
    if (document == nullptr)
        return;

    lineNumber = jlimit (0, document->getNumLines() - 1, lineNumber);

    const auto it = std::find (breakpointLines.begin(), breakpointLines.end(), lineNumber);
    const bool alreadySet = it != breakpointLines.end();

    if (shouldBeEnabled && ! alreadySet)
        breakpointLines.push_back (lineNumber);

    if (! shouldBeEnabled && alreadySet)
        breakpointLines.erase (it);

    repaint();
}

bool CodeEditor::getBreakpoint (int lineNumber) const
{
    return std::find (breakpointLines.begin(), breakpointLines.end(), lineNumber) != breakpointLines.end();
}

void CodeEditor::clearBreakpoints()
{
    breakpointLines.clear();
    repaint();
}

void CodeEditor::setErrorLine (int lineNumber, bool isError)
{
    if (document == nullptr)
        return;

    lineNumber = jlimit (0, document->getNumLines() - 1, lineNumber);

    const auto isErrorOnLine = [lineNumber] (const Diagnostic& diagnostic)
    {
        return diagnostic.highlightLine && diagnostic.line == lineNumber;
    };

    const bool alreadySet = std::any_of (diagnostics.begin(), diagnostics.end(), isErrorOnLine);

    if (isError && ! alreadySet)
    {
        Diagnostic diagnostic;
        diagnostic.line = lineNumber;
        diagnostic.endLine = lineNumber;
        diagnostic.endColumn = document->getLine (lineNumber).length();
        diagnostic.highlightLine = true;
        diagnostics.push_back (diagnostic);
    }

    if (! isError && alreadySet)
        diagnostics.erase (std::remove_if (diagnostics.begin(), diagnostics.end(), isErrorOnLine), diagnostics.end());

    repaint();
}

bool CodeEditor::isErrorLine (int lineNumber) const
{
    return std::any_of (diagnostics.begin(), diagnostics.end(), [lineNumber] (const Diagnostic& diagnostic)
    {
        return diagnostic.highlightLine && diagnostic.line == lineNumber;
    });
}

void CodeEditor::clearErrorLines()
{
    diagnostics.erase (std::remove_if (diagnostics.begin(), diagnostics.end(), [] (const Diagnostic& diagnostic)
    {
        return diagnostic.highlightLine;
    }),
                       diagnostics.end());

    repaint();
}

//==============================================================================

void CodeEditor::setDiagnostics (std::vector<Diagnostic> newDiagnostics)
{
    diagnostics = std::move (newDiagnostics);
    repaint();
}

void CodeEditor::addDiagnostic (const Diagnostic& diagnostic)
{
    diagnostics.push_back (diagnostic);
    repaint();
}

void CodeEditor::clearDiagnostics()
{
    diagnostics.clear();
    repaint();
}

void CodeEditor::setSemanticTokens (std::vector<SemanticToken> newTokens)
{
    std::stable_sort (newTokens.begin(), newTokens.end(), [] (const SemanticToken& a, const SemanticToken& b)
    {
        return a.line < b.line;
    });

    semanticTokens = std::move (newTokens);
    needsStyledTextUpdate = true;
    repaint();
}

void CodeEditor::clearSemanticTokens()
{
    semanticTokens.clear();
    needsStyledTextUpdate = true;
    repaint();
}

std::vector<CodeEditor::Diagnostic> CodeEditor::getDiagnosticsAt (int characterIndex) const
{
    std::vector<Diagnostic> result;

    for (const auto& diagnostic : diagnostics)
    {
        const auto range = getDiagnosticRange (diagnostic);
        if (characterIndex >= range.getStart() && characterIndex <= range.getEnd())
            result.push_back (diagnostic);
    }

    std::stable_sort (result.begin(), result.end(), [] (const Diagnostic& a, const Diagnostic& b)
    {
        return a.severity < b.severity;
    });

    return result;
}

Range<int> CodeEditor::getDiagnosticRange (const Diagnostic& diagnostic) const
{
    if (document == nullptr || document->getNumLines() == 0)
        return {};

    const int lastLine = document->getNumLines() - 1;

    const auto toIndex = [&] (int line, int column)
    {
        line = jlimit (0, lastLine, line);
        return document->getLineStartOffset (line) + jlimit (0, document->getLine (line).length(), column);
    };

    const int start = toIndex (diagnostic.line, diagnostic.column);
    const int end = jmax (start, toIndex (diagnostic.endLine, diagnostic.endColumn));

    if (end > start)
        return { start, end };

    // An empty range marks the word at its start, or the character around it
    const int line = jlimit (0, lastLine, diagnostic.line);
    const auto lineText = document->getLine (line);
    const int lineStart = document->getLineStartOffset (line);

    int wordEnd = start - lineStart;
    while (wordEnd < lineText.length() && (CharacterFunctions::isLetterOrDigit (lineText[wordEnd]) || lineText[wordEnd] == '_'))
        ++wordEnd;

    if (wordEnd > start - lineStart)
        return { start, lineStart + wordEnd };

    if (start - lineStart < lineText.length())
        return { start, start + 1 };

    if (start > lineStart)
        return { start - 1, start };

    return { start, start };
}

void CodeEditor::toggleBreakpointAt (const Point<float>& position)
{
    if (document == nullptr)
        return;

    const float lineHeight = getLineHeight();
    const int line = static_cast<int> ((position.getY() - getTextArea().getY() + scrollOffset.getY()) / lineHeight);

    if (line >= 0 && line < document->getNumLines())
        setBreakpoint (line, ! getBreakpoint (line));
}

//==============================================================================
// Misc
//==============================================================================

String CodeEditor::getLineIndent (int lineNumber) const
{
    if (document == nullptr)
        return {};

    const auto lineText = document->getLine (lineNumber);

    int index = 0;
    while (index < lineText.length() && (lineText[index] == ' ' || lineText[index] == '\t'))
        ++index;

    return lineText.substring (0, index);
}

void CodeEditor::setMinimapVisible (bool shouldBeVisible)
{
    if (minimapVisible != shouldBeVisible)
    {
        minimapVisible = shouldBeVisible;

        if (minimap != nullptr)
        {
            if (shouldBeVisible)
                addAndMakeVisible (minimap.get());
            else
                removeChildComponent (minimap.get());
        }

        resized();
        repaint();
    }
}

} // namespace yup
