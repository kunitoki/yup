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

namespace yup
{

//==============================================================================

const Identifier TextEditor::Colors::backgroundColorId = "textEditorBackground";
const Identifier TextEditor::Colors::textColorId = "textEditorText";
const Identifier TextEditor::Colors::caretColorId = "textEditorCaret";
const Identifier TextEditor::Colors::selectionColorId = "textEditorSelection";
const Identifier TextEditor::Colors::outlineColorId = "textEditorOutline";
const Identifier TextEditor::Colors::focusedOutlineColorId = "textEditorFocusedOutline";

//==============================================================================

TextEditor::TextEditor (StringRef componentID)
    : Component (componentID)
    , caretTimer ([this] { caretVisible = !caretVisible; repaint(); })
{
    setWantsKeyboardFocus (true);
    setMouseCursor (MouseCursor::Text);

    // Start with empty text
    setText ("", dontSendNotification);
}

//==============================================================================

String TextEditor::getText() const
{
    return text;
}

void TextEditor::setText (String newText, NotificationType notification)
{
    if (text != newText)
    {
        text = newText;
        caretPosition = jmin (caretPosition, text.length());
        selectionStart = selectionEnd = caretPosition;
        needsUpdate = true;
        repaint();

        if (notification == sendNotification)
        {
            if (onTextChange)
                onTextChange();
        }
    }
}

//==============================================================================

void TextEditor::setMultiLine (bool shouldBeMultiLine)
{
    if (multiLine != shouldBeMultiLine)
    {
        multiLine = shouldBeMultiLine;
        needsUpdate = true;
        repaint();
    }
}

//==============================================================================

void TextEditor::setReadOnly (bool shouldBeReadOnly)
{
    if (readOnly != shouldBeReadOnly)
    {
        readOnly = shouldBeReadOnly;
        setMouseCursor (readOnly ? MouseCursor::Default : MouseCursor::Text);
    }
}

//==============================================================================

void TextEditor::setCaretPosition (int newPosition)
{
    newPosition = jlimit (0, text.length(), newPosition);
    if (caretPosition != newPosition)
    {
        caretPosition = newPosition;
        selectionStart = selectionEnd = caretPosition;
        updateCaretPosition();
        repaint();
    }
}

//==============================================================================

Range<int> TextEditor::getSelection() const
{
    int start = jmin (selectionStart, selectionEnd);
    int end = jmax (selectionStart, selectionEnd);
    return Range<int> (start, end - start);
}

void TextEditor::setSelection (const Range<int>& newSelection)
{
    int start = jlimit (0, text.length(), newSelection.getStart());
    int end = jlimit (0, text.length(), newSelection.getEnd());

    selectionStart = start;
    selectionEnd = end;
    caretPosition = end;

    updateCaretPosition();
    repaint();
}

void TextEditor::selectAll()
{
    setSelection (Range<int> (0, text.length()));
}

bool TextEditor::hasSelection() const
{
    return selectionStart != selectionEnd;
}

//==============================================================================

String TextEditor::getSelectedText() const
{
    if (!hasSelection())
        return {};

    int start = jmin (selectionStart, selectionEnd);
    int end = jmax (selectionStart, selectionEnd);
    return text.substring (start, end);
}

void TextEditor::deleteSelectedText()
{
    if (!hasSelection() || readOnly)
        return;

    int start = jmin (selectionStart, selectionEnd);
    int end = jmax (selectionStart, selectionEnd);

    text = text.substring (0, start) + text.substring (end);
    caretPosition = selectionStart = selectionEnd = start;
    needsUpdate = true;

    if (onTextChange)
        onTextChange();

    updateCaretPosition();
    repaint();
}

//==============================================================================

void TextEditor::insertText (const String& textToInsert)
{
    if (readOnly)
        return;

    deleteSelectedText();

    String filteredText = textToInsert;
    if (!multiLine)
    {
        // Remove line breaks for single-line editor
        filteredText = filteredText.replaceCharacters ("\r\n", "  ");
    }

    text = text.substring (0, caretPosition) + filteredText + text.substring (caretPosition);
    caretPosition += filteredText.length();
    selectionStart = selectionEnd = caretPosition;
    needsUpdate = true;

    if (onTextChange)
        onTextChange();

    updateCaretPosition();
    repaint();
}

//==============================================================================

void TextEditor::copy()
{
    if (hasSelection())
    {
        SystemClipboard::copyTextToClipboard (getSelectedText());
    }
}

void TextEditor::cut()
{
    if (hasSelection() && !readOnly)
    {
        copy();
        deleteSelectedText();
    }
}

void TextEditor::paste()
{
    if (!readOnly)
    {
        String textToInsert = SystemClipboard::getTextFromClipboard();
        if (textToInsert.isNotEmpty())
            insertText(textToInsert);
    }
}

//==============================================================================

std::optional<Font> TextEditor::getFont() const
{
    return font;
}

void TextEditor::setFont (Font newFont)
{
    font = newFont;
    needsUpdate = true;
    repaint();
}

void TextEditor::resetFont()
{
    font.reset();
    needsUpdate = true;
    repaint();
}

//==============================================================================

void TextEditor::paint (Graphics& g)
{
    auto bounds = getLocalBounds();
    auto textBounds = getTextBounds();

    // Update styled text if needed
    if (needsUpdate)
    {
        updateStyledText();
        needsUpdate = false;
    }

    // Draw background
    auto backgroundColor = findColor (Colors::backgroundColorId).value_or (yup::Colors::white);
    g.setFillColor (backgroundColor);
    g.fillRoundedRect (bounds, 4.0f);

    // Draw outline
    auto outlineColor = hasKeyboardFocus()
        ? findColor (Colors::focusedOutlineColorId).value_or (yup::Colors::blue)
        : findColor (Colors::outlineColorId).value_or (yup::Colors::gray);
    g.setStrokeColor (outlineColor);
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (bounds, 4.0f);

    // Draw selection background
    if (hasSelection())
    {
        auto selectionColor = findColor (Colors::selectionColorId).value_or (yup::Colors::lightblue.withAlpha (0.6f));
        g.setFillColor (selectionColor);

        int start = jmin (selectionStart, selectionEnd);
        int end = jmax (selectionStart, selectionEnd);

        // Get all selection rectangles for proper multiline selection rendering
        auto selectionRects = styledText.getSelectionRectangles(start, end);
        for (const auto& rect : selectionRects)
        {
            // Adjust each rectangle for scroll offset and text bounds
            auto adjustedRect = rect.translated(textBounds.getTopLeft() - scrollOffset);
            g.fillRect(adjustedRect);
        }
    }

    // Draw text with scroll offset
    g.setFillColor (outlineColor);
    auto scrolledTextBounds = textBounds.translated(-scrollOffset.getX(), -scrollOffset.getY());
    g.fillFittedText (styledText, scrolledTextBounds);

    // Draw caret
    if (hasKeyboardFocus() && caretVisible)
    {
        auto caretColor = findColor (Colors::caretColorId).value_or (yup::Colors::black);
        g.setFillColor (caretColor);
        auto caretBounds = getCaretBounds();
        g.fillRect (caretBounds);
    }
}

void TextEditor::resized()
{
    needsUpdate = true;
}

//==============================================================================

void TextEditor::focusGained()
{
    startCaretBlinking();
    repaint();
}

void TextEditor::focusLost()
{
    stopCaretBlinking();
    repaint();
}

//==============================================================================

void TextEditor::mouseDown (const MouseEvent& event)
{
    if (!hasKeyboardFocus())
        takeKeyboardFocus();

    auto position = event.getPosition().to<float>();
    int newCaretPos = getGlyphIndexAtPosition (position);

    if (event.getModifiers().isShiftDown())
    {
        // Extend selection
        selectionEnd = newCaretPos;
        caretPosition = newCaretPos;
    }
    else
    {
        // Start new selection
        caretPosition = selectionStart = selectionEnd = newCaretPos;
    }

    isDragging = true;
    updateCaretPosition();
    repaint();
}

//==============================================================================

void TextEditor::mouseDrag (const MouseEvent& event)
{
    if (isDragging)
    {
        auto position = event.getPosition().to<float>();
        int newCaretPos = getGlyphIndexAtPosition (position);

        selectionEnd = newCaretPos;
        caretPosition = newCaretPos;
        updateCaretPosition();
        repaint();
    }
}

//==============================================================================

void TextEditor::mouseUp (const MouseEvent& event)
{
    isDragging = false;
}

//==============================================================================

void TextEditor::keyDown (const KeyPress& key, const Point<float>& position)
{
    bool shiftDown = key.getModifiers().isShiftDown();
    bool ctrlDown = key.getModifiers().isControlDown() || key.getModifiers().isCommandDown();

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
            moveCaretUp(shiftDown);
    }
    else if (key.getKey() == KeyPress::downKey)
    {
        if (ctrlDown)
            moveCaretToEnd (shiftDown);
        else
            moveCaretDown(shiftDown);
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
    else if (key.getKey() == KeyPress::enterKey || key.getKey() == KeyPress::tabKey)
    {
        if (multiLine && key.getKey() == KeyPress::enterKey)
        {
            insertText ("\n");
        }
        else if (key.getKey() == KeyPress::tabKey)
        {
            insertText ("\t");
        }
    }
    else if (ctrlDown)
    {
        // Handle Ctrl shortcuts
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
    }

    updateCaretPosition();
    repaint();
}

void TextEditor::textInput (const String& inputText)
{
    if (!readOnly && inputText.isNotEmpty())
    {
        insertText (inputText);
    }
}

//==============================================================================

void TextEditor::updateStyledText()
{
    styledText.clear();

    if (text.isNotEmpty())
    {
        auto textColor = findColor (Colors::textColorId).value_or (yup::Colors::black);
        auto currentFont = font.value_or (ApplicationTheme::getGlobalTheme()->getDefaultFont());

        styledText.setMaxSize (getTextBounds().getSize());
        styledText.setHorizontalAlign (StyledText::left);
        styledText.setVerticalAlign (StyledText::top);
        styledText.setWrap (multiLine ? StyledText::wrap : StyledText::noWrap);
        styledText.setOverflow (StyledText::visible);

        // Calculate font size based on text editor height (with some padding)
        float fontSize = 14.0f; // jmax(10.0f, jmin(24.0f, getTextBounds().getHeight() * 0.6f));
        styledText.appendText (text, currentFont, fontSize);
        styledText.update();
    }
}

//==============================================================================

void TextEditor::updateCaretPosition()
{
    caretVisible = true;
    if (hasKeyboardFocus())
    {
        startCaretBlinking();
    }
    ensureCaretVisible();
}

//==============================================================================

void TextEditor::ensureCaretVisible()
{
    auto textBounds = getTextBounds();
    auto caretBounds = const_cast<StyledText&>(styledText).getCaretBounds(caretPosition);

    if (caretBounds.isEmpty())
        return;

    // Check if caret is outside visible area and adjust scroll offset
    bool needsRepaint = false;

    // Calculate visible area in StyledText coordinate space
    float visibleLeft = scrollOffset.getX();
    float visibleTop = scrollOffset.getY();
    float visibleRight = visibleLeft + textBounds.getWidth();
    float visibleBottom = visibleTop + textBounds.getHeight();

    // Add some padding for better user experience
    const float horizontalPadding = 10.0f;
    const float verticalPadding = 5.0f;

    // Horizontal scrolling
    if (caretBounds.getRight() + horizontalPadding > visibleRight)
    {
        scrollOffset.setX(caretBounds.getRight() + horizontalPadding - textBounds.getWidth());
        needsRepaint = true;
    }
    else if (caretBounds.getX() - horizontalPadding < visibleLeft)
    {
        scrollOffset.setX(jmax(0.0f, caretBounds.getX() - horizontalPadding));
        needsRepaint = true;
    }

    // Vertical scrolling
    if (caretBounds.getBottom() + verticalPadding > visibleBottom)
    {
        scrollOffset.setY(caretBounds.getBottom() + verticalPadding - textBounds.getHeight());
        needsRepaint = true;
    }
    else if (caretBounds.getY() - verticalPadding < visibleTop)
    {
        scrollOffset.setY(jmax(0.0f, caretBounds.getY() - verticalPadding));
        needsRepaint = true;
    }

    // Ensure scroll offset doesn't go negative
    scrollOffset.setX(jmax(0.0f, scrollOffset.getX()));
    scrollOffset.setY(jmax(0.0f, scrollOffset.getY()));

    // Limit scrolling to the actual text bounds
    auto textSize = const_cast<StyledText&>(styledText).getComputedTextBounds();
    scrollOffset.setX(jmin(scrollOffset.getX(), jmax(0.0f, textSize.getWidth() - textBounds.getWidth())));
    scrollOffset.setY(jmin(scrollOffset.getY(), jmax(0.0f, textSize.getHeight() - textBounds.getHeight())));

    if (needsRepaint)
        repaint();
}

//==============================================================================

int TextEditor::getGlyphIndexAtPosition (const Point<float>& position) const
{
    // Adjust position relative to text bounds and scroll offset
    auto textBounds = getTextBounds();
    auto relativePos = position - textBounds.getTopLeft() + scrollOffset;

    // Use StyledText's positioning functionality
    return const_cast<StyledText&> (styledText).getGlyphIndexAtPosition (relativePos);
}

//==============================================================================

Rectangle<float> TextEditor::getCaretBounds() const
{
    auto textBounds = getTextBounds();
    auto caretBounds = const_cast<StyledText&> (styledText).getCaretBounds (caretPosition);

    // Adjust bounds to be relative to the text editor's bounds with scroll offset applied
    return caretBounds.translated (textBounds.getTopLeft() - scrollOffset);
}


//==============================================================================

void TextEditor::moveCaretUp(bool extendSelection)
{
    if (multiLine)
    {
        // Get current caret bounds to maintain horizontal position
        auto currentCaretBounds = getCaretBounds();
        if (currentCaretBounds.isEmpty())
        {
            moveCaretToStart(extendSelection);
            return;
        }

        // Calculate target position above current line
        auto targetX = currentCaretBounds.getCenterX();
        auto targetY = currentCaretBounds.getY() - 5.0f; // Move up by a small amount to get to previous line

        // Convert back to text coordinate space
        auto textBounds = getTextBounds();
        auto relativeTargetPos = Point<float>(targetX, targetY) - textBounds.getTopLeft() + scrollOffset;

        // Get character index at target position
        int newPosition = styledText.getGlyphIndexAtPosition(relativeTargetPos);

        // Ensure we moved to a different position
        if (newPosition == caretPosition)
        {
            // If we didn't move, try to find the previous line manually
            newPosition = findPreviousLinePosition(caretPosition);
        }

        caretPosition = jlimit(0, text.length(), newPosition);

        if (!extendSelection)
            selectionStart = selectionEnd = caretPosition;
        else
            selectionEnd = caretPosition;
    }
    else
    {
        moveCaretToStart(extendSelection);
    }
}

void TextEditor::moveCaretDown(bool extendSelection)
{
    if (multiLine)
    {
        // Get current caret bounds to maintain horizontal position
        auto currentCaretBounds = getCaretBounds();
        if (currentCaretBounds.isEmpty())
        {
            moveCaretToEnd(extendSelection);
            return;
        }

        // Calculate target position below current line
        auto targetX = currentCaretBounds.getCenterX();
        auto targetY = currentCaretBounds.getBottom() + 5.0f; // Move down by a small amount to get to next line

        // Convert back to text coordinate space
        auto textBounds = getTextBounds();
        auto relativeTargetPos = Point<float>(targetX, targetY) - textBounds.getTopLeft() + scrollOffset;

        // Get character index at target position
        int newPosition = styledText.getGlyphIndexAtPosition(relativeTargetPos);

        // Ensure we moved to a different position
        if (newPosition == caretPosition)
        {
            // If we didn't move, try to find the next line manually
            newPosition = findNextLinePosition(caretPosition);
        }

        caretPosition = jlimit(0, text.length(), newPosition);

        if (!extendSelection)
            selectionStart = selectionEnd = caretPosition;
        else
            selectionEnd = caretPosition;
    }
    else
    {
        moveCaretToEnd(extendSelection);
    }
}

void TextEditor::moveCaretLeft (bool extendSelection)
{
    if (caretPosition > 0)
    {
        caretPosition--;

        if (!extendSelection)
            selectionStart = selectionEnd = caretPosition;
        else
            selectionEnd = caretPosition;
    }
}

void TextEditor::moveCaretRight (bool extendSelection)
{
    if (caretPosition < text.length())
    {
        caretPosition++;

        if (!extendSelection)
            selectionStart = selectionEnd = caretPosition;
        else
            selectionEnd = caretPosition;
    }
}

void TextEditor::moveCaretToStartOfLine (bool extendSelection)
{
    int newPosition = findLineStart(caretPosition);
    caretPosition = newPosition;

    if (!extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretToEndOfLine (bool extendSelection)
{
    int newPosition = findLineEnd(caretPosition);
    caretPosition = newPosition;

    if (!extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretToStart (bool extendSelection)
{
    caretPosition = 0;

    if (!extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretToEnd (bool extendSelection)
{
    caretPosition = text.length();

    if (!extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

//==============================================================================

void TextEditor::handleBackspace()
{
    if (readOnly)
        return;

    if (hasSelection())
    {
        deleteSelectedText();
    }
    else if (caretPosition > 0)
    {
        text = text.substring (0, caretPosition - 1) + text.substring (caretPosition);
        caretPosition--;
        selectionStart = selectionEnd = caretPosition;
        needsUpdate = true;

        if (onTextChange)
            onTextChange();

        updateCaretPosition();
        repaint();
    }
}

void TextEditor::handleDelete()
{
    if (readOnly)
        return;

    if (hasSelection())
    {
        deleteSelectedText();
    }
    else if (caretPosition < text.length())
    {
        text = text.substring (0, caretPosition) + text.substring (caretPosition + 1);
        needsUpdate = true;

        if (onTextChange)
            onTextChange();

        updateCaretPosition();
        repaint();
    }
}

//==============================================================================

void TextEditor::startCaretBlinking()
{
    caretVisible = true;
    caretTimer.startTimer (500); // Blink every 500ms
}

void TextEditor::stopCaretBlinking()
{
    caretTimer.stopTimer();
    caretVisible = false;
}

//==============================================================================

Rectangle<float> TextEditor::getTextBounds() const
{
    return getLocalBounds().reduced(4.0f);
}

//==============================================================================

int TextEditor::findLineStart(int position) const
{
    if (!multiLine)
        return 0;

    int pos = jlimit(0, text.length(), position);
    while (pos > 0 && text[pos - 1] != '\n')
        pos--;
    return pos;
}

int TextEditor::findLineEnd(int position) const
{
    if (!multiLine)
        return text.length();

    int pos = jlimit(0, text.length(), position);
    while (pos < text.length() && text[pos] != '\n')
        pos++;
    return pos;
}

int TextEditor::findPreviousLinePosition(int position) const
{
    if (!multiLine)
        return 0;

    int currentLineStart = findLineStart(position);
    if (currentLineStart == 0)
        return 0;

    // Find the start of the previous line
    int prevLineEnd = currentLineStart - 1; // Skip the newline
    int prevLineStart = findLineStart(prevLineEnd);

    // Try to maintain horizontal position
    int currentColumn = position - currentLineStart;
    int prevLineLength = prevLineEnd - prevLineStart;

    return prevLineStart + jmin(currentColumn, prevLineLength);
}

int TextEditor::findNextLinePosition(int position) const
{
    if (!multiLine)
        return text.length();

    int currentLineStart = findLineStart(position);
    int currentLineEnd = findLineEnd(position);

    if (currentLineEnd >= text.length())
        return text.length();

    // Find the start of the next line
    int nextLineStart = currentLineEnd + 1; // Skip the newline
    if (nextLineStart > text.length())
        return text.length();

    // Try to maintain horizontal position
    int currentColumn = position - currentLineStart;
    int nextLineEnd = findLineEnd(nextLineStart);
    int nextLineLength = nextLineEnd - nextLineStart;

    return nextLineStart + jmin(currentColumn, nextLineLength);
}

int TextEditor::findWordStart(int position) const
{
    int pos = jlimit(0, text.length(), position);

    // Skip any whitespace backwards
    while (pos > 0 && (text[pos - 1] == ' ' || text[pos - 1] == '\t' || text[pos - 1] == '\n'))
        pos--;

    // Find the start of the current word
    while (pos > 0 && !isWordSeparator(text[pos - 1]))
        pos--;

    return pos;
}

int TextEditor::findWordEnd(int position) const
{
    int pos = jlimit(0, text.length(), position);

    // Skip any whitespace forward
    while (pos < text.length() && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n'))
        pos++;

    // Find the end of the current word
    while (pos < text.length() && !isWordSeparator(text[pos]))
        pos++;

    return pos;
}

bool TextEditor::isWordSeparator(yup_wchar character) const
{
    return character == ' ' || character == '\t' || character == '\n' ||
           character == '.' || character == ',' || character == ';' || character == ':' ||
           character == '!' || character == '?' || character == '(' || character == ')' ||
           character == '[' || character == ']' || character == '{' || character == '}' ||
           character == '"' || character == '\'' || character == '/' || character == '\\' ||
           character == '|' || character == '&' || character == '*' || character == '+' ||
           character == '-' || character == '=' || character == '<' || character == '>' ||
           character == '@' || character == '#' || character == '$' || character == '%' ||
           character == '^' || character == '~' || character == '`';
}

void TextEditor::moveCaretToWordStart(bool extendSelection)
{
    int newPosition = findWordStart(caretPosition);
    caretPosition = newPosition;

    if (!extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretToWordEnd(bool extendSelection)
{
    int newPosition = findWordEnd(caretPosition);
    caretPosition = newPosition;

    if (!extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::deleteWordBackward()
{
    if (readOnly)
        return;

    if (hasSelection())
    {
        deleteSelectedText();
    }
    else
    {
        int wordStart = findWordStart(caretPosition);
        if (wordStart < caretPosition)
        {
            text = text.substring(0, wordStart) + text.substring(caretPosition);
            caretPosition = selectionStart = selectionEnd = wordStart;
            needsUpdate = true;

            if (onTextChange)
                onTextChange();

            updateCaretPosition();
            repaint();
        }
    }
}

void TextEditor::deleteWordForward()
{
    if (readOnly)
        return;

    if (hasSelection())
    {
        deleteSelectedText();
    }
    else
    {
        int wordEnd = findWordEnd(caretPosition);
        if (wordEnd > caretPosition)
        {
            text = text.substring(0, caretPosition) + text.substring(wordEnd);
            needsUpdate = true;

            if (onTextChange)
                onTextChange();

            updateCaretPosition();
            repaint();
        }
    }
}

} // namespace yup
