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

const Identifier TextEditor::Style::backgroundColorId = "textEditorBackground";
const Identifier TextEditor::Style::textColorId = "textEditorText";
const Identifier TextEditor::Style::caretColorId = "textEditorCaret";
const Identifier TextEditor::Style::selectionColorId = "textEditorSelection";
const Identifier TextEditor::Style::outlineColorId = "textEditorOutline";
const Identifier TextEditor::Style::focusedOutlineColorId = "textEditorFocusedOutline";

//==============================================================================

TextEditor::TextEditor (StringRef componentID)
    : Component (componentID)
    , caretTimer (bindFront (&TextEditor::blinkCaret, this))
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
    newText = newText.removeCharacters ("\r");

    if (text != newText)
    {
        text = newText;
        caretPosition = 0;
        selectionStart = selectionEnd = 0;
        needsUpdate = true;

        sendChangeNotification (notification, [this]
        {
            if (onTextChange)
                onTextChange();
        });

        updateCaretPosition();
        repaint();
    }
}

//==============================================================================

void TextEditor::insertText (const String& textToInsert, NotificationType notification)
{
    if (readOnly)
        return;

    deleteSelectedText (dontSendNotification);

    String filteredText = textToInsert.removeCharacters ("\r");
    if (! multiLine)
        filteredText = filteredText.replaceCharacters ("\n", " ");

    text = text.substring (0, caretPosition) + filteredText + text.substring (caretPosition);
    caretPosition += filteredText.length();
    selectionStart = selectionEnd = caretPosition;
    needsUpdate = true;

    sendChangeNotification (notification, [this]
    {
        if (onTextChange)
            onTextChange();
    });

    updateCaretPosition();
    repaint();
}

//==============================================================================

void TextEditor::setMultiLine (bool shouldBeMultiLine)
{
    if (multiLine != shouldBeMultiLine)
    {
        multiLine = shouldBeMultiLine;
        needsUpdate = true;
        updateTextInputRectIfActive();
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

        if (canUseTextInput() && hasKeyboardFocus())
            requestTextInput();
        else
            relinquishTextInput();
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
    return Range<int> (start, end);
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
    if (! hasSelection())
        return {};

    int start = jmin (selectionStart, selectionEnd);
    int end = jmax (selectionStart, selectionEnd);
    return text.substring (start, end);
}

void TextEditor::deleteSelectedText (NotificationType notification)
{
    if (! hasSelection() || readOnly)
        return;

    int start = jmin (selectionStart, selectionEnd);
    int end = jmax (selectionStart, selectionEnd);

    text = text.substring (0, start) + text.substring (end);
    caretPosition = selectionStart = selectionEnd = start;
    needsUpdate = true;

    sendChangeNotification (notification, [this]
    {
        if (onTextChange)
            onTextChange();
    });

    updateCaretPosition();
    repaint();
}

std::vector<Rectangle<float>> TextEditor::getSelectedTextAreas() const
{
    const int start = jmin (selectionStart, selectionEnd);
    const int end = jmax (selectionStart, selectionEnd);

    return const_cast<StyledText&> (styledText).getSelectionRectangles (start, end);
}

//==============================================================================

void TextEditor::copy()
{
    if (hasSelection())
        SystemClipboard::copyTextToClipboard (getSelectedText());
}

void TextEditor::cut()
{
    if (hasSelection() && ! readOnly)
    {
        copy();
        deleteSelectedText();
    }
}

void TextEditor::paste()
{
    if (! readOnly)
    {
        String textToInsert = SystemClipboard::getTextFromClipboard();
        if (textToInsert.isNotEmpty())
            insertText (textToInsert);
    }
}

//==============================================================================

std::optional<Font> TextEditor::getFont() const
{
    return font;
}

void TextEditor::setFont (Font newFont)
{
    if (! font || *font != newFont)
    {
        font = newFont;
        needsUpdate = true;

        updateTextInputRectIfActive();
        repaint();
    }
}

void TextEditor::resetFont()
{
    if (font)
    {
        font.reset();
        needsUpdate = true;

        updateTextInputRectIfActive();
        repaint();
    }
}

std::optional<float> TextEditor::getFontSize() const
{
    return fontSize;
}

void TextEditor::setFontSize (float newFontSize)
{
    if (! fontSize || ! approximatelyEqual (*fontSize, newFontSize))
    {
        fontSize = newFontSize;
        needsUpdate = true;

        updateTextInputRectIfActive();
        repaint();
    }
}

void TextEditor::resetFontSize()
{
    if (fontSize)
    {
        fontSize.reset();
        needsUpdate = true;

        updateTextInputRectIfActive();
        repaint();
    }
}

//==============================================================================

void TextEditor::paint (Graphics& g)
{
    updateStyledTextIfNeeded();

    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void TextEditor::resized()
{
    needsUpdate = true;
    clampScrollOffset();
}

//==============================================================================

void TextEditor::focusGained()
{
    if (isEnabled())
    {
        startCaretBlinking();

        if (canUseTextInput())
            requestTextInput();
    }
    else
    {
        stopCaretBlinking();
        relinquishTextInput();
    }

    repaint();
}

void TextEditor::focusLost()
{
    isDragging = false;
    stopCaretBlinking();
    relinquishTextInput();
    repaint();
}

//==============================================================================

void TextEditor::mouseDown (const MouseEvent& event)
{
    if (getWantsKeyboardFocus())
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
    if (! isDragging)
        return;

    auto position = event.getPosition().to<float>();
    int newCaretPos = getGlyphIndexAtPosition (position);

    selectionEnd = newCaretPos;
    caretPosition = newCaretPos;

    updateCaretPosition();
    repaint();
}

//==============================================================================

void TextEditor::mouseUp (const MouseEvent& event)
{
    isDragging = false;
}

//==============================================================================

void TextEditor::mouseDoubleClick (const MouseEvent& event)
{
    auto position = event.getPosition().to<float>();
    int clickPos = getGlyphIndexAtPosition (position);

    if (clickPos < 0 || clickPos >= text.length())
    {
        selectAll();
        return;
    }

    yup_wchar ch = text[clickPos];
    bool isWhitespace = CharacterFunctions::isWhitespace (ch);

    int wordStart = clickPos;
    int wordEnd = clickPos + 1;

    if (isWhitespace)
    {
        while (wordStart > 0 && (CharacterFunctions::isWhitespace (text[wordStart - 1])))
            wordStart--;

        while (wordEnd < text.length() && (CharacterFunctions::isWhitespace (text[wordEnd])))
            wordEnd++;
    }
    else if (! isWordSeparator (ch))
    {
        while (wordStart > 0 && ! isWordSeparator (text[wordStart - 1]))
            wordStart--;

        while (wordEnd < text.length() && ! isWordSeparator (text[wordEnd]))
            wordEnd++;
    }

    selectionStart = wordStart;
    selectionEnd = wordEnd;
    caretPosition = wordEnd;

    updateCaretPosition();
    repaint();
}

//==============================================================================

void TextEditor::keyDown (const KeyPress& key, const Point<float>& position)
{
    if (! isEnabled())
        return;

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
    else if (key.getKey() == KeyPress::enterKey || key.getKey() == KeyPress::tabKey)
    {
        if (multiLine && key.getKey() == KeyPress::enterKey)
        {
            insertText ("\n");
        }
        else if (multiLine && key.getKey() == KeyPress::tabKey)
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
    if (canUseTextInput())
    {
        const auto filtered = inputText.removeCharacters ("\r");
        if (filtered.isNotEmpty())
            insertText (filtered);
    }
}

//==============================================================================

void TextEditor::updateStyledTextIfNeeded()
{
    if (! needsUpdate)
        return;

    auto modifier = styledText.startUpdate();
    modifier.clear();

    if (text.isNotEmpty())
    {
        auto textColor = findColor (Style::textColorId).value_or (Colors::black);
        auto currentFont = font.value_or (ApplicationTheme::getGlobalTheme()->getDefaultFont());

        modifier.setMaxSize (getTextBounds().getSize());
        modifier.setHorizontalAlign (StyledText::left);
        modifier.setVerticalAlign (StyledText::top);
        modifier.setWrap (multiLine ? StyledText::wrap : StyledText::noWrap);
        modifier.setOverflow (StyledText::visible);

        modifier.appendText (text, currentFont.withHeight (fontSize.value_or (14.0f)));
    }

    needsUpdate = false;
}

//==============================================================================

void TextEditor::updateCaretPosition()
{
    caretVisible = true;
    if (hasKeyboardFocus() && ! caretBlinking)
        startCaretBlinking();

    ensureCaretVisible();
    updateTextInputRectIfActive();
}

//==============================================================================

void TextEditor::ensureCaretVisible()
{
    updateStyledTextIfNeeded();

    auto textBounds = getTextBounds();
    auto caretBounds = styledText.getCaretBounds (caretPosition);

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
        scrollOffset.setX (caretBounds.getRight() + horizontalPadding - textBounds.getWidth());
        needsRepaint = true;
    }
    else if (caretBounds.getX() - horizontalPadding < visibleLeft)
    {
        scrollOffset.setX (jmax (0.0f, caretBounds.getX() - horizontalPadding));
        needsRepaint = true;
    }

    // Vertical scrolling
    if (caretBounds.getBottom() + verticalPadding > visibleBottom)
    {
        scrollOffset.setY (caretBounds.getBottom() + verticalPadding - textBounds.getHeight());
        needsRepaint = true;
    }
    else if (caretBounds.getY() - verticalPadding < visibleTop)
    {
        scrollOffset.setY (jmax (0.0f, caretBounds.getY() - verticalPadding));
        needsRepaint = true;
    }

    clampScrollOffset();

    if (needsRepaint)
        repaint();
}

//==============================================================================

void TextEditor::blinkCaret()
{
    caretVisible = ! caretVisible;

    repaint();
}

//==============================================================================

int TextEditor::getGlyphIndexAtPosition (const Point<float>& position) const
{
    // Adjust position relative to text bounds and scroll offset
    auto textBounds = getTextBounds();
    auto relativePos = position - textBounds.getTopLeft() + scrollOffset;

    // Use StyledText's positioning functionality
    return styledText.getGlyphIndexAtPosition (relativePos);
}

//==============================================================================

Rectangle<float> TextEditor::getCaretBounds() const
{
    auto textBounds = getTextBounds();
    auto caretBounds = styledText.getCaretBounds (caretPosition);

    // Adjust bounds to be relative to the text editor's bounds with scroll offset applied
    return caretBounds.translated (textBounds.getTopLeft() - scrollOffset);
}

//==============================================================================

void TextEditor::moveCaretUp (bool extendSelection)
{
    if (multiLine)
        caretPosition = findVisualLinePosition (caretPosition, false);
    else
        caretPosition = 0;

    if (! extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretDown (bool extendSelection)
{
    if (multiLine)
        caretPosition = findVisualLinePosition (caretPosition, true);
    else
        caretPosition = text.length();

    if (! extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretLeft (bool extendSelection)
{
    if (caretPosition > 0)
    {
        caretPosition--;

        if (! extendSelection)
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

        if (! extendSelection)
            selectionStart = selectionEnd = caretPosition;
        else
            selectionEnd = caretPosition;
    }
}

void TextEditor::moveCaretToStartOfLine (bool extendSelection)
{
    int newPosition = findLineStart (caretPosition);
    caretPosition = newPosition;

    if (! extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretToEndOfLine (bool extendSelection)
{
    int newPosition = findLineEnd (caretPosition);
    caretPosition = newPosition;

    if (! extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretToStart (bool extendSelection)
{
    caretPosition = 0;

    if (! extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretToEnd (bool extendSelection)
{
    caretPosition = text.length();

    if (! extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

//==============================================================================

Rectangle<float> TextEditor::getTextInputRect() const
{
    const_cast<TextEditor*> (this)->updateStyledTextIfNeeded();

    auto caretBounds = getCaretBounds();
    auto screenPos = localToScreen (caretBounds.getTopLeft());

    return Rectangle<float> (screenPos.getX(), screenPos.getY(), caretBounds.getWidth(), caretBounds.getHeight());
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

        sendChangeNotification (sendNotification, [this]
        {
            if (onTextChange)
                onTextChange();
        });

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

        sendChangeNotification (sendNotification, [this]
        {
            if (onTextChange)
                onTextChange();
        });

        updateCaretPosition();
        repaint();
    }
}

//==============================================================================

void TextEditor::startCaretBlinking()
{
    caretVisible = true;
    caretBlinking = true;
    caretTimer.startTimer (500);
}

void TextEditor::stopCaretBlinking()
{
    caretTimer.stopTimer();
    caretVisible = false;
    caretBlinking = false;
}

//==============================================================================

void TextEditor::clampScrollOffset()
{
    updateStyledTextIfNeeded();

    auto textBounds = getTextBounds();
    auto textSize = styledText.getComputedTextBounds();

    scrollOffset.setX (jmax (0.0f, jmin (scrollOffset.getX(), jmax (0.0f, textSize.getWidth() - textBounds.getWidth()))));
    scrollOffset.setY (jmax (0.0f, jmin (scrollOffset.getY(), jmax (0.0f, textSize.getHeight() - textBounds.getHeight()))));

    updateTextInputRectIfActive();
}

//==============================================================================

void TextEditor::mouseWheel (const MouseEvent& /*event*/, const MouseWheelData& wheelData)
{
    const float wheelSpeed = 30.0f;
    scrollOffset.setX (scrollOffset.getX() - wheelData.getDeltaX() * wheelSpeed);
    scrollOffset.setY (scrollOffset.getY() - wheelData.getDeltaY() * wheelSpeed);
    clampScrollOffset();
    repaint();
}

//==============================================================================

void TextEditor::enablementChanged()
{
    if (isEnabled())
    {
        setMouseCursor (readOnly ? MouseCursor::Default : MouseCursor::Text);
        if (hasKeyboardFocus())
        {
            startCaretBlinking();

            if (canUseTextInput())
                requestTextInput();
        }
    }
    else
    {
        isDragging = false;
        stopCaretBlinking();
        relinquishTextInput();
        setMouseCursor (MouseCursor::Default);
    }

    repaint();
}

bool TextEditor::canUseTextInput() const noexcept
{
    return isEnabled() && ! readOnly;
}

void TextEditor::updateTextInputRectIfActive()
{
    if (isTextInputActive())
        updateTextInputRect();
}

//==============================================================================

Rectangle<float> TextEditor::getTextBounds() const
{
    return getLocalBounds().reduced (4.0f);
}

//==============================================================================

int TextEditor::findLineStart (int position) const
{
    if (! multiLine)
        return 0;

    int pos = jlimit (0, text.length(), position);

    while (pos > 0 && text[pos - 1] != '\n')
        pos--;

    return pos;
}

int TextEditor::findLineEnd (int position) const
{
    if (! multiLine)
        return text.length();

    int pos = jlimit (0, text.length(), position);

    while (pos < text.length() && text[pos] != '\n')
        pos++;

    return pos;
}

int TextEditor::findPreviousLinePosition (int position) const
{
    if (! multiLine)
        return 0;

    int currentLineStart = findLineStart (position);
    if (currentLineStart == 0)
        return 0;

    // Find the start of the previous line
    int prevLineEnd = currentLineStart - 1; // Skip the newline
    int prevLineStart = findLineStart (prevLineEnd);

    // Try to maintain horizontal position
    int currentColumn = position - currentLineStart;
    int prevLineLength = prevLineEnd - prevLineStart;

    return prevLineStart + jmin (currentColumn, prevLineLength);
}

int TextEditor::findNextLinePosition (int position) const
{
    if (! multiLine)
        return text.length();

    int currentLineStart = findLineStart (position);
    int currentLineEnd = findLineEnd (position);

    if (currentLineEnd >= text.length())
        return text.length();

    // Find the start of the next line
    int nextLineStart = currentLineEnd + 1; // Skip the newline
    if (nextLineStart > text.length())
        return text.length();

    // Try to maintain horizontal position
    int currentColumn = position - currentLineStart;
    int nextLineEnd = findLineEnd (nextLineStart);
    int nextLineLength = nextLineEnd - nextLineStart;

    return nextLineStart + jmin (currentColumn, nextLineLength);
}

int TextEditor::findVisualLinePosition (int position, bool moveDown) const
{
    if (! multiLine)
        return moveDown ? text.length() : 0;

    const_cast<TextEditor*> (this)->updateStyledTextIfNeeded();
    return styledText.getGlyphIndexOnAdjacentLine (position, moveDown);
}

int TextEditor::findWordStart (int position) const
{
    int pos = jlimit (0, text.length(), position);

    // Skip any whitespace backwards
    while (pos > 0 && CharacterFunctions::isWhitespace (text[pos - 1]))
        pos--;

    // Find the start of the current word
    while (pos > 0 && ! isWordSeparator (text[pos - 1]))
        pos--;

    return pos;
}

int TextEditor::findWordEnd (int position) const
{
    int pos = jlimit (0, text.length(), position);

    // Skip any whitespace forward
    while (pos < text.length() && CharacterFunctions::isWhitespace (text[pos]))
        pos++;

    // Find the end of the current word
    while (pos < text.length() && ! isWordSeparator (text[pos]))
        pos++;

    return pos;
}

bool TextEditor::isWordSeparator (yup_wchar character) const
{
    return character == ' ' || character == '\t' || character == '\n' || character == '.' || character == ',' || character == ';' || character == ':' || character == '!' || character == '?' || character == '(' || character == ')' || character == '[' || character == ']' || character == '{' || character == '}' || character == '"' || character == '\'' || character == '/' || character == '\\' || character == '|' || character == '&' || character == '*' || character == '+' || character == '-' || character == '=' || character == '<' || character == '>' || character == '@' || character == '#' || character == '$' || character == '%' || character == '^' || character == '~' || character == '`';
}

void TextEditor::moveCaretToWordStart (bool extendSelection)
{
    int newPosition = findWordStart (caretPosition);
    caretPosition = newPosition;

    if (! extendSelection)
        selectionStart = selectionEnd = caretPosition;
    else
        selectionEnd = caretPosition;
}

void TextEditor::moveCaretToWordEnd (bool extendSelection)
{
    int newPosition = findWordEnd (caretPosition);
    caretPosition = newPosition;

    if (! extendSelection)
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
        int wordStart = findWordStart (caretPosition);
        if (wordStart < caretPosition)
        {
            text = text.substring (0, wordStart) + text.substring (caretPosition);
            caretPosition = selectionStart = selectionEnd = wordStart;
            needsUpdate = true;

            sendChangeNotification (sendNotification, [this]
            {
                if (onTextChange)
                    onTextChange();
            });

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
        int wordEnd = findWordEnd (caretPosition);
        while (wordEnd < text.length() && CharacterFunctions::isWhitespace (text[wordEnd]))
            wordEnd++;

        if (wordEnd > caretPosition)
        {
            text = text.substring (0, caretPosition) + text.substring (wordEnd);
            needsUpdate = true;

            sendChangeNotification (sendNotification, [this]
            {
                if (onTextChange)
                    onTextChange();
            });

            updateCaretPosition();
            repaint();
        }
    }
}

} // namespace yup
