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
/** A component that allows editing text with cursor movement and selection.

    The TextEditor component provides a complete text editing interface including:
    - Single line and multiline text editing
    - Cursor movement using arrow keys and mouse clicks
    - Text selection using mouse drag or shift+arrow keys
    - Copy, cut, and paste operations
    - Customizable styling through ApplicationTheme

    Example usage:
    @code
    auto editor = std::make_unique<TextEditor>("myEditor");
    editor->setText("Hello World");
    editor->setMultiLine(true);
    editor->setBounds(10, 10, 200, 100);
    addAndMakeVisible(*editor);
    @endcode

    @see Component, StyledText
*/
class YUP_API TextEditor : public Component
{
public:
    //==============================================================================
    /** Creates a text editor with an optional component ID. */
    TextEditor (StringRef componentID = {});

    //==============================================================================
    /** Returns the editor's current text.

        @returns The text currently displayed in the editor
    */
    String getText() const;

    /** Changes the editor's text.

        @param newText          The new text to display
        @param notification     Whether to trigger a change notification
    */
    void setText (String newText, NotificationType notification = sendNotification);

    //==============================================================================
    /** Inserts text at the current caret position.

        @param textToInsert     The text to insert
        @param notification     Whether to trigger a change notification
    */
    void insertText (const String& textToInsert, NotificationType notification = sendNotification);

    //==============================================================================
    /** Returns whether this editor supports multiple lines.

        @returns True if multiline editing is enabled
    */
    bool isMultiLine() const noexcept { return multiLine; }

    /** Sets whether this editor should support multiple lines.

        @param shouldBeMultiLine    True to enable multiline editing
    */
    void setMultiLine (bool shouldBeMultiLine);

    //==============================================================================
    /** Returns whether the editor is read-only.

        @returns True if the editor is read-only
    */
    bool isReadOnly() const noexcept { return readOnly; }

    /** Sets whether the editor should be read-only.

        @param shouldBeReadOnly     True to make the editor read-only
    */
    void setReadOnly (bool shouldBeReadOnly);

    //==============================================================================
    /** Returns the current caret position in the text.

        @returns The character index of the caret position
    */
    int getCaretPosition() const noexcept { return caretPosition; }

    /** Sets the caret position.

        @param newPosition  The new character index for the caret
    */
    void setCaretPosition (int newPosition);

    /** Returns whether the caret is visible. */
    bool isCaretVisible() const noexcept { return caretVisible; }

    /** Moves the caret up.

        @param extendSelection  Whether to extend the selection to the caret position
    */
    void moveCaretUp (bool extendSelection = false);

    /** Moves the caret down.

        @param extendSelection  Whether to extend the selection to the caret position
    */
    void moveCaretDown (bool extendSelection = false);

    /** Moves the caret left.

        @param extendSelection  Whether to extend the selection to the caret position
    */
    void moveCaretLeft (bool extendSelection = false);

    /** Moves the caret right.

        @param extendSelection  Whether to extend the selection to the caret position
    */
    void moveCaretRight (bool extendSelection = false);

    /** Moves the caret to the start of the line.

        @param extendSelection  Whether to extend the selection to the caret position
    */
    void moveCaretToStartOfLine (bool extendSelection = false);

    /** Moves the caret to the end of the line.

        @param extendSelection  Whether to extend the selection to the caret position
    */
    void moveCaretToEndOfLine (bool extendSelection = false);

    /** Moves the caret to the start of the text.

        @param extendSelection  Whether to extend the selection to the caret position
    */
    void moveCaretToStart (bool extendSelection = false);

    /** Moves the caret to the end of the text.

        @param extendSelection  Whether to extend the selection to the caret position
    */
    void moveCaretToEnd (bool extendSelection = false);

    //==============================================================================
    /** Gets the current selection range.

        @returns A Range indicating the selected text (start, length)
    */
    Range<int> getSelection() const;

    /** Sets the selection range.

        @param newSelection     The new selection range
    */
    void setSelection (const Range<int>& newSelection);

    /** Selects all text in the editor. */
    void selectAll();

    /** Returns true if any text is currently selected. */
    bool hasSelection() const;

    /** Returns the selection rectangles, usable for knowing where the selection is. */
    std::vector<Rectangle<float>> getSelectedTextAreas() const;

    //==============================================================================
    /** Returns the currently selected text.

        @returns The selected text as a String
    */
    String getSelectedText() const;

    /** Deletes the currently selected text.

        @param notification     Whether to trigger a change notification
    */
    void deleteSelectedText (NotificationType notification = sendNotification);

    //==============================================================================
    /** Copies the selected text to the clipboard. */
    void copy();

    /** Cuts the selected text to the clipboard. */
    void cut();

    /** Pastes text from the clipboard at the current caret position. */
    void paste();

    //==============================================================================
    /** Sets a callback function to be called when the text changes.

        @param callback     The function to call when text changes
    */
    std::function<void()> onTextChange;

    //==============================================================================
    /** Returns the current font.

        @returns The font used to display the text
    */
    std::optional<Font> getFont() const;

    /** Sets the font to use.

        @param newFont    The new font to use
    */
    void setFont (Font newFont);

    /** Reset the font to the theme one. */
    void resetFont();

    //==============================================================================
    /** Returns the current font size. */
    std::optional<float> getFontSize() const;

    /** Sets the font size.

        @param newFontSize    The new font size to use
    */
    void setFontSize (float newFontSize);

    /** Reset the font size to the theme one. */
    void resetFontSize();

    //==============================================================================
    /** Returns the bounds of the text. */
    Rectangle<float> getTextBounds() const;

    /** Returns the bounds of the caret. */
    Rectangle<float> getCaretBounds() const;

    /** Get the scroll offset. */
    Point<float> getScrollOffset() const noexcept { return scrollOffset; }

    //==============================================================================
    /** Color identifiers used by the text editor. */
    struct Colors
    {
        static const Identifier backgroundColorId;
        static const Identifier textColorId;
        static const Identifier caretColorId;
        static const Identifier selectionColorId;
        static const Identifier outlineColorId;
        static const Identifier focusedOutlineColorId;
    };

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void focusGained() override;
    /** @internal */
    void focusLost() override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDoubleClick (const MouseEvent& event) override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void textInput (const String& text) override;
    /** @internal */
    StyledText& getStyledText() const noexcept { return const_cast<StyledText&> (styledText); }

private:
    //==============================================================================
    void updateStyledTextIfNeeded();
    void updateCaretPosition();
    void ensureCaretVisible();
    void blinkCaret();
    int getGlyphIndexAtPosition (const Point<float>& position) const;
    void handleBackspace();
    void handleDelete();
    void startCaretBlinking();
    void stopCaretBlinking();
    int findLineStart (int position) const;
    int findLineEnd (int position) const;
    int findPreviousLinePosition (int position) const;
    int findNextLinePosition (int position) const;

    // Word navigation methods
    int findWordStart (int position) const;
    int findWordEnd (int position) const;
    bool isWordSeparator (yup_wchar character) const;
    void moveCaretToWordStart (bool extendSelection = false);
    void moveCaretToWordEnd (bool extendSelection = false);
    void deleteWordBackward();
    void deleteWordForward();

    //==============================================================================
    String text;
    StyledText styledText;
    std::optional<Font> font;
    std::optional<float> fontSize;

    int caretPosition = 0;
    int selectionStart = 0;
    int selectionEnd = 0;

    bool multiLine = false;
    bool readOnly = false;
    bool isDragging = false;
    bool caretVisible = true;
    bool needsUpdate = true;

    Point<float> scrollOffset;

    TimedCallback caretTimer;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextEditor)
};

} // namespace yup
