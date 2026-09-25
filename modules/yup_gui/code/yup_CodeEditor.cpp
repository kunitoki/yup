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

class CodeEditor::DocumentListener : public CodeDocument::Listener
{
public:
    explicit DocumentListener (CodeEditor& editor)
        : editor (editor)
    {
    }

    void codeDocumentChanged (CodeDocument&, int, int) override
    {
        editor.needsStyledTextUpdate = true;
        editor.cachedDocumentTextDirty = true;
        editor.repaint();
    }

private:
    CodeEditor& editor;
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

    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override
    {
        editor.mouseWheel (event.withRelativePositionTo (&editor), wheelData);
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
    needsStyledTextUpdate = true;
    cachedDocumentTextDirty = true;

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

    return getText().substring (selection.getStart(), selection.getEnd());
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

    const auto endPosition = document->replaceRange (document->indexToPosition (selection.getStart()),
                                                     document->indexToPosition (selection.getEnd()),
                                                     textToInsert);

    caretPosition = document->positionToIndex (endPosition);
    selection = Range<int>::emptyRange (caretPosition);

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
        document->removeRange (document->indexToPosition (selection.getStart()),
                               document->indexToPosition (selection.getEnd()));

        caretPosition = selection.getStart();
        selection = Range<int>::emptyRange (caretPosition);

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

    const bool result = document->undo();
    if (result)
    {
        caretPosition = jlimit (0, document->getNumCharacters(), caretPosition);
        selection = Range<int>::emptyRange (caretPosition);
    }
    updateCaretPosition();
    return result;
}

bool CodeEditor::redo()
{
    if (document == nullptr)
        return false;

    const bool result = document->redo();
    if (result)
    {
        caretPosition = jlimit (0, document->getNumCharacters(), caretPosition);
        selection = Range<int>::emptyRange (caretPosition);
    }
    updateCaretPosition();
    return result;
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

    auto colorForToken = [this, &textColor] (SyntaxDefinition::TokenType type) -> Color
    {
        return scheme.getColor (type).value_or (textColor);
    };

    for (int line = windowFirstLine; line <= windowLastLine; ++line)
    {
        const auto lineText = document->getLine (line);
        const auto tokens = tokeniser.getTokens (*document, line);

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
            auto runColor = colorForToken (tokens[0].type);

            for (size_t i = 1; i < tokens.size(); ++i)
            {
                const auto tokenColor = colorForToken (tokens[i].type);
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
                             bounds.getHeight());
}

float CodeEditor::getScrollBarWidth() const noexcept
{
    return scrollBar != nullptr && scrollBar->isVisible() ? scrollBar->getScrollBarWidth() : 0.0f;
}

void CodeEditor::updateScrollBar()
{
    if (scrollBar == nullptr || document == nullptr)
        return;

    const auto textArea = getTextArea();
    const float documentHeight = document->getNumLines() * getLineHeight();

    scrollBar->setRangeLimits (0.0, documentHeight);
    scrollBar->setCurrentRange (scrollOffset.getY(), scrollOffset.getY() + textArea.getHeight());
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

std::vector<Rectangle<float>> CodeEditor::getSelectedTextAreas() const
{
    if (! hasSelection())
        return {};

    const auto textArea = getTextArea();
    auto rectangles = getSelectionRectanglesInDocument (selection.getStart(), selection.getEnd());

    for (auto& rectangle : rectangles)
        rectangle = rectangle.translated (textArea.getTopLeft() - scrollOffset);

    return rectangles;
}

std::vector<Rectangle<float>> CodeEditor::getSearchMatchAreas() const
{
    if (searchMatches.empty() || document == nullptr)
        return {};

    const auto textArea = getTextArea();

    std::vector<Rectangle<float>> rectangles;
    for (const auto& match : searchMatches)
    {
        for (const auto& rectangle : getSelectionRectanglesInDocument (match.getStart(), match.getEnd()))
            rectangles.push_back (rectangle.translated (textArea.getTopLeft() - scrollOffset));
    }

    return rectangles;
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

int CodeEditor::getGlyphIndexAtPosition (const Point<float>& position) const
{
    const auto textArea = getTextArea();
    const auto relativePosition = position - textArea.getTopLeft() + scrollOffset;
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
    if (getWantsKeyboardFocus())
        takeKeyboardFocus();

    if (document == nullptr)
        return;

    updateStyledTextIfNeeded();

    const auto position = event.getPosition().to<float>();
    const auto bounds = getLocalBounds();

    if (lineNumbersVisible && position.getX() <= bounds.getX() + 4.0f + getGutterWidth())
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

    const int wordStart = findWordStart (clickPos);
    const int wordEnd = findWordEnd (clickPos);

    selection = Range (wordStart, wordEnd);
    caretPosition = wordEnd;

    updateCaretPosition();
    repaint();
}

void CodeEditor::mouseWheel (const MouseEvent&, const MouseWheelData& wheelData)
{
    const float wheelSpeed = 30.0f;

    scrollOffset.setX (scrollOffset.getX() + wheelData.getDeltaX() * wheelSpeed);
    scrollOffset.setY (scrollOffset.getY() - wheelData.getDeltaY() * wheelSpeed);

    clampScrollOffsetIfNeeded();
    repaint();
}

//==============================================================================

void CodeEditor::keyDown (const KeyPress& key, const Point<float>&)
{
    if (! isEnabled())
        return;

    const bool shiftDown = key.getModifiers().isShiftDown();
    const bool ctrlDown = key.getModifiers().isControlDown() || key.getModifiers().isCommandDown();

    if (key.getKey() == KeyPress::leftKey)
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
        if (ctrlDown)
            moveCaretToStart (shiftDown);
        else
            moveCaretUp (shiftDown);
    }
    else if (key.getKey() == KeyPress::downKey)
    {
        if (ctrlDown)
            moveCaretToEnd (shiftDown);
        else
            moveCaretDown (shiftDown);
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
        if (canEdit())
        {
            const auto indent = getLineIndent (getCurrentLineNumber());
            insertText ("\n" + indent);
        }
    }
    else if (key.getKey() == KeyPress::tabKey)
    {
        if (canEdit())
            insertText (String::repeatedString (" ", tabWidth));
    }
    else if (ctrlDown)
    {
        if (key.getKey() == KeyPress::textAKey)
        {
            selectAll();
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
    }
}

void CodeEditor::textInput (const String& text)
{
    if (canEdit() && text.isNotEmpty())
        insertText (text);
}

//==============================================================================

void CodeEditor::handleBackspace()
{
    if (! canEdit())
        return;

    if (hasSelection())
    {
        document->removeRange (document->indexToPosition (selection.getStart()),
                               document->indexToPosition (selection.getEnd()));

        caretPosition = selection.getStart();
        selection = Range<int>::emptyRange (caretPosition);
    }
    else if (caretPosition > 0)
    {
        const int start = caretPosition - 1;
        document->removeRange (document->indexToPosition (start),
                               document->indexToPosition (caretPosition));

        caretPosition = start;
        selection = Range<int>::emptyRange (caretPosition);
    }

    updateCaretPosition();
}

void CodeEditor::handleDelete()
{
    if (! canEdit())
        return;

    if (hasSelection())
    {
        document->removeRange (document->indexToPosition (selection.getStart()),
                               document->indexToPosition (selection.getEnd()));

        caretPosition = selection.getStart();
        selection = Range<int>::emptyRange (caretPosition);
    }
    else if (caretPosition < document->getNumCharacters())
    {
        document->removeRange (document->indexToPosition (caretPosition),
                               document->indexToPosition (caretPosition + 1));
    }

    updateCaretPosition();
}

//==============================================================================

void CodeEditor::moveCaretTo (int newPosition, bool extendSelection)
{
    const int anchor = extendSelection && caretPosition == selection.getEnd() ? selection.getStart() : selection.getEnd();

    caretPosition = newPosition;

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
    if (document == nullptr)
        return;

    const auto position = document->indexToPosition (caretPosition);
    const int currentLine = position.getLineNumber();
    const int newPosition = currentLine > 0
                              ? document->positionToIndex ({ *document, currentLine - 1, position.getColumnNumber() })
                              : 0;

    moveCaretTo (newPosition, extendSelection);
}

void CodeEditor::moveCaretDown (bool extendSelection)
{
    if (document == nullptr)
        return;

    const auto position = document->indexToPosition (caretPosition);
    const int currentLine = position.getLineNumber();
    const int newPosition = currentLine < document->getNumLines() - 1
                              ? document->positionToIndex ({ *document, currentLine + 1, position.getColumnNumber() })
                              : document->getNumCharacters();

    moveCaretTo (newPosition, extendSelection);
}

void CodeEditor::moveCaretToStartOfLine (bool extendSelection)
{
    if (document == nullptr)
        return;

    const int newPosition = document->positionToIndex ({ *document,
                                                         document->indexToPosition (caretPosition).getLineNumber(),
                                                         0 });

    moveCaretTo (newPosition, extendSelection);
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
    return CharacterFunctions::isWhitespace (character)
        || character == '(' || character == ')'
        || character == '[' || character == ']'
        || character == '{' || character == '}'
        || character == '.' || character == ','
        || character == ';' || character == ':'
        || character == '<' || character == '>'
        || character == '"' || character == '\''
        || character == '\\' || character == '/'
        || character == '+' || character == '-'
        || character == '*' || character == '%'
        || character == '=' || character == '!'
        || character == '&' || character == '|'
        || character == '^' || character == '~';
}

int CodeEditor::findWordStart (int position) const
{
    if (document == nullptr)
        return 0;

    const auto text = getText();
    position = jlimit (0, text.length(), position);

    if (position > 0 && isWordSeparator (text[position - 1]))
        return position;

    while (position > 0 && ! isWordSeparator (text[position - 1]))
        --position;

    return position;
}

int CodeEditor::findWordEnd (int position) const
{
    if (document == nullptr)
        return 0;

    const auto text = getText();
    position = jlimit (0, text.length(), position);

    while (position < text.length() && ! isWordSeparator (text[position]))
        ++position;

    return position;
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
        document->removeRange (document->indexToPosition (wordStart),
                               document->indexToPosition (caretPosition));

        caretPosition = wordStart;
        selection = Range<int>::emptyRange (caretPosition);

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
        document->removeRange (document->indexToPosition (caretPosition),
                               document->indexToPosition (wordEnd));

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

std::vector<Range<int>> CodeEditor::findAll (StringRef searchTextToFind, bool caseSensitive, bool wholeWord) const
{
    if (document == nullptr || searchTextToFind.isEmpty())
        return {};

    const auto text = getText();
    const String search (searchTextToFind);

    std::vector<Range<int>> results;

    int index = 0;
    while (index < text.length())
    {
        const int found = caseSensitive ? text.indexOf (index, search)
                                        : text.indexOfIgnoreCase (index, search);
        if (found < 0)
            break;

        const bool startBoundary = found == 0 || isWordSeparator (text[found - 1]);
        const bool endBoundary = found + search.length() >= text.length()
                              || isWordSeparator (text[found + search.length()]);

        if (! wholeWord || (startBoundary && endBoundary))
            results.emplace_back (found, found + search.length());

        index = found + search.length();
    }

    return results;
}

bool CodeEditor::findNext (StringRef searchTextToFind, bool caseSensitive, bool wrapAround)
{
    const auto matches = findAll (searchTextToFind, caseSensitive, false);
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

bool CodeEditor::findPrevious (StringRef searchTextToFind, bool caseSensitive, bool wrapAround)
{
    const auto matches = findAll (searchTextToFind, caseSensitive, false);
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

int CodeEditor::replaceNext (StringRef searchTextToFind, StringRef replacement, bool caseSensitive)
{
    if (! canEdit() || ! hasSelection())
        return 0;

    const String search (searchTextToFind);
    const bool matches = caseSensitive ? getSelectedText() == search
                                       : getSelectedText().equalsIgnoreCase (search);
    if (! matches)
        return 0;

    const auto endPos = document->replaceRange (document->indexToPosition (selection.getStart()),
                                                document->indexToPosition (selection.getEnd()),
                                                replacement);

    caretPosition = document->positionToIndex (endPos);
    selection = Range<int>::emptyRange (caretPosition);

    findNext (searchTextToFind, caseSensitive);

    return 1;
}

int CodeEditor::replaceAll (StringRef searchTextToFind, StringRef replacement, bool caseSensitive)
{
    if (! canEdit() || searchTextToFind.isEmpty())
        return 0;

    const auto matches = findAll (searchTextToFind, caseSensitive, false);
    if (matches.empty())
        return 0;

    document->getUndoManager()->beginNewTransaction();

    for (auto it = matches.rbegin(); it != matches.rend(); ++it)
    {
        document->replaceRange (document->indexToPosition (it->getStart()),
                                document->indexToPosition (it->getEnd()),
                                replacement);
    }

    return static_cast<int> (matches.size());
}

void CodeEditor::highlightSearchMatches (StringRef searchTextToFind, bool caseSensitive)
{
    searchText = searchTextToFind;
    searchCaseSensitive = caseSensitive;
    searchMatches = findAll (searchText, searchCaseSensitive, false);

    repaint();
}

void CodeEditor::clearSearchHighlights()
{
    searchText.clear();
    searchCaseSensitive = false;
    searchMatches.clear();

    repaint();
}

//==============================================================================
// Bracket matching
//==============================================================================

std::optional<Range<int>> CodeEditor::getBracketMatch (int position) const
{
    if (document == nullptr)
        return std::nullopt;

    if (cachedDocumentTextDirty)
    {
        cachedDocumentText = document->getText();
        cachedDocumentTextDirty = false;
    }

    const auto& text = cachedDocumentText;
    if (text.isEmpty())
        return std::nullopt;

    position = jlimit (0, text.length() - 1, position);

    auto findMatch = [&text] (int start, yup_wchar open, yup_wchar close, int direction) -> int
    {
        int depth = 1;
        int index = start + direction;

        while (index >= 0 && index < text.length())
        {
            const auto character = text[index];

            if (direction > 0)
            {
                if (character == open)
                    ++depth;
                else if (character == close && --depth == 0)
                    return index;
            }
            else
            {
                if (character == close)
                    ++depth;
                else if (character == open && --depth == 0)
                    return index;
            }

            index += direction;
        }

        return -1;
    };

    for (const int probe : { position, position - 1 })
    {
        if (probe < 0 || probe >= text.length())
            continue;

        const auto character = text[probe];
        if (character != '(' && character != ')' && character != '[' && character != ']' && character != '{' && character != '}')
            continue;

        if (character == '(')
        {
            const int match = findMatch (probe, '(', ')', 1);
            if (match >= 0)
                return Range<int> (probe, match);
        }

        if (character == ')')
        {
            const int match = findMatch (probe, '(', ')', -1);
            if (match >= 0)
                return Range<int> (match, probe);
        }

        if (character == '[')
        {
            const int match = findMatch (probe, '[', ']', 1);
            if (match >= 0)
                return Range<int> (probe, match);
        }

        if (character == ']')
        {
            const int match = findMatch (probe, '[', ']', -1);
            if (match >= 0)
                return Range<int> (match, probe);
        }

        if (character == '{')
        {
            const int match = findMatch (probe, '{', '}', 1);
            if (match >= 0)
                return Range<int> (probe, match);
        }

        if (character == '}')
        {
            const int match = findMatch (probe, '{', '}', -1);
            if (match >= 0)
                return Range<int> (match, probe);
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
