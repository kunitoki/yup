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

class ScrollBar;

//==============================================================================
/** A syntax-highlighting source code editor.

    The CodeEditor component displays and edits a `CodeDocument` with syntax
    highlighting driven by a `SyntaxDefinition` (see `setSyntaxDefinition`),
    an optional line-number gutter, undo/redo, clipboard support, and a
    configurable monospace font. It shares the caret/selection/hit-testing
    machinery of `StyledText`, so wrapped or scrolled text is positioned
    correctly.

    Highlighting is computed incrementally by the internal `CodeTokeniser`:
    only the lines affected by an edit are re-tokenized, and the colored text
    layout is rebuilt only when the document, font or definition changes.

    Example usage:
    @code
    CodeDocument document;
    document.setText ("int main() { return 0; }");

    CodeEditor editor (document);
    editor.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));
    addAndMakeVisible (editor);
    @endcode

    @see CodeDocument, CodeTokeniser, SyntaxDefinition
*/
class YUP_API CodeEditor : public Component
    , public TextInputTarget
{
public:
    //==============================================================================
    /** Creates an editor that owns its internal CodeDocument. */
    CodeEditor();

    /** Creates an editor bound to an external CodeDocument.

        The document must outlive the editor.

        @param document The document to edit.
    */
    explicit CodeEditor (CodeDocument& document);

    /** Destructor. */
    ~CodeEditor() override;

    //==============================================================================
    /** Returns the document being edited. */
    CodeDocument* getDocument() const noexcept { return document; }

    /** Binds the editor to a different document (must outlive the editor). */
    void setDocument (CodeDocument& newDocument);

    //==============================================================================
    /** Sets the syntax definition used for highlighting.

        @param definition The definition to use (must outlive the editor).
    */
    void setSyntaxDefinition (const SyntaxDefinition& definition);

    /** Sets a built-in syntax definition by language name ("cpp", "glsl", "python", "xml", "ydsp").

        @param languageName The language name.
    */
    void setSyntaxDefinition (StringRef languageName);

    /** Sets the built-in syntax definition matching a file extension, if any.

        @param fileExtension The file extension without a leading dot.
        @returns True if a matching definition was found and set.
    */
    bool setSyntaxDefinitionForExtension (StringRef fileExtension);

    /** Returns the active syntax definition. */
    const SyntaxDefinition& getSyntaxDefinition() const;

    //==============================================================================
    /** Sets the color scheme used to render the editor.

        The scheme provides every color the editor needs to paint itself, including
        the per-token syntax colors, so switching scheme changes the whole look of
        the editor consistently.

        @param newScheme The scheme to use (copied).
    */
    void setScheme (const CodeEditorScheme& newScheme);

    /** Returns the active color scheme. */
    const CodeEditorScheme& getScheme() const noexcept;

    //==============================================================================
    /** Returns the font used to draw the text. */
    const Font& getFont() const;

    /** Sets the font used to draw the text.

        @param newFont The new font.
    */
    void setFont (Font newFont);

    //==============================================================================
    /** Returns the editor's full text. */
    String getText() const;

    /** Replaces the whole document text (not undoable). */
    void setText (StringRef newText);

    /** Returns the currently selected text. */
    String getSelectedText() const;

    //==============================================================================
    /** Returns true if the editor is read-only. */
    bool isReadOnly() const noexcept { return readOnly; }

    /** Sets whether the editor is read-only.

        @param shouldBeReadOnly True to make the editor read-only.
    */
    void setReadOnly (bool shouldBeReadOnly);

    //==============================================================================
    /** Returns the caret position as a global character index. */
    int getCaretPosition() const noexcept { return caretPosition; }

    /** Sets the caret position (global character index), clearing the selection. */
    void setCaretPosition (int newPosition);

    /** Returns the selection range, or an empty range when nothing is selected. */
    Range<int> getSelection() const;

    /** Sets the selection range (global character indices). */
    void setSelection (const Range<int>& newSelection);

    /** Selects all text. */
    void selectAll();

    /** Returns true if any text is selected. */
    bool hasSelection() const;

    //==============================================================================
    /** Inserts text at the caret, replacing any selection (undoable). */
    void insertText (StringRef textToInsert);

    /** Copies the selected text to the clipboard. */
    void copy();

    /** Cuts the selected text to the clipboard. */
    void cut();

    /** Pastes text from the clipboard at the caret. */
    void paste();

    /** Undoes the most recent edit, if any. */
    bool undo();

    /** Redoes the most recently undone edit, if any. */
    bool redo();

    /** Returns true if an undo is available. */
    bool canUndo() const;

    /** Returns true if a redo is available. */
    bool canRedo() const;

    //==============================================================================
    /** Returns true if the line-number gutter is visible. */
    bool isLineNumbersVisible() const noexcept { return lineNumbersVisible; }

    /** Sets whether the line-number gutter is visible. */
    void setLineNumbersVisible (bool shouldBeVisible);

    /** Returns the gutter width in pixels (0 when hidden). */
    float getGutterWidth() const;

    /** Returns the number of spaces a tab expands to. */
    int getTabWidth() const;

    /** Sets the number of spaces a tab expands to. */
    void setTabWidth (int newTabWidth);

    /** Returns the number of pixels scrolled from the top-left of the document. */
    Point<float> getScrollOffset() const noexcept { return scrollOffset; }

    /** Sets the scroll offset, clamped to the document bounds. */
    void setScrollOffset (Point<float> newOffset);

    /** Scrolls so that a line is visible. */
    void scrollToLine (int lineNumber);

    //==============================================================================
    /** Returns all occurrences of a search string in the document.

        @param searchText    The text to find.
        @param caseSensitive Whether the search is case sensitive.
        @param wholeWord     Whether matches must be whole words.
        @returns The ranges of all matches (empty if none).
    */
    std::vector<Range<int>> findAll (StringRef searchText, bool caseSensitive = false, bool wholeWord = false) const;

    /** Finds the next occurrence after the caret and selects it.

        @param searchText    The text to find.
        @param caseSensitive Whether the search is case sensitive.
        @param wrapAround    Whether to continue from the start when the end is reached.
        @returns True if a match was found and selected.
    */
    bool findNext (StringRef searchText, bool caseSensitive = false, bool wrapAround = true);

    /** Finds the previous occurrence before the caret and selects it.

        @param searchText    The text to find.
        @param caseSensitive Whether the search is case sensitive.
        @param wrapAround    Whether to continue from the end when the start is reached.
        @returns True if a match was found and selected.
    */
    bool findPrevious (StringRef searchText, bool caseSensitive = false, bool wrapAround = true);

    /** Replaces the current selection if it matches, then selects the next match.

        @param searchText    The text to replace.
        @param replacement   The replacement text.
        @param caseSensitive Whether the search is case sensitive.
        @returns The number of replacements performed (0 or 1).
    */
    int replaceNext (StringRef searchText, StringRef replacement, bool caseSensitive = false);

    /** Replaces every occurrence of a search string in the document.

        @param searchText    The text to replace.
        @param replacement   The replacement text.
        @param caseSensitive Whether the search is case sensitive.
        @returns The number of replacements performed.
    */
    int replaceAll (StringRef searchText, StringRef replacement, bool caseSensitive = false);

    /** Highlights all matches of a search string while painting.

        @param searchText    The text to highlight (empty clears the highlights).
        @param caseSensitive Whether the search is case sensitive.
    */
    void highlightSearchMatches (StringRef searchText, bool caseSensitive = false);

    /** Clears the search match highlights. */
    void clearSearchHighlights();

    //==============================================================================
    /** Returns the matching bracket range for the bracket at (or next to) a position.

        Returns a range covering the opening and closing bracket, or std::nullopt when
        the position is not on a bracket or the bracket has no match.

        @param position The global character position to inspect.
        @returns The matching bracket range, if any.
    */
    std::optional<Range<int>> getBracketMatch (int position) const;

    //==============================================================================
    /** Sets or clears a breakpoint on a line.

        @param lineNumber       The 0-based line.
        @param shouldBeEnabled  Whether the breakpoint should be set.
    */
    void setBreakpoint (int lineNumber, bool shouldBeEnabled);

    /** Returns true if a breakpoint is set on a line. */
    bool getBreakpoint (int lineNumber) const;

    /** Removes all breakpoints. */
    void clearBreakpoints();

    //==============================================================================
    /** Returns true if the minimap overview is visible. */
    bool isMinimapVisible() const noexcept { return minimapVisible; }

    /** Sets whether the minimap overview is visible. */
    void setMinimapVisible (bool shouldBeVisible);

    //==============================================================================
    /** @internal Returns the rectangle of the text area (used by the theme for painting). */
    Rectangle<float> getTextArea() const;

    /** @internal Returns the height of a single line of text. */
    float getLineHeight() const;

    /** @internal Returns the caret bounds in view coordinates (used by the theme for painting). */
    Rectangle<float> getCaretBounds() const;

    /** @internal Returns the current selection rectangles in view coordinates. */
    std::vector<Rectangle<float>> getSelectedTextAreas() const;

    /** @internal Returns the search-match highlight rectangles in view coordinates. */
    std::vector<Rectangle<float>> getSearchMatchAreas() const;

    /** @internal Returns the styled text used for rendering. */
    const StyledText& getStyledText() const noexcept;

    /** @internal Returns the first line of the currently shaped text window. */
    int getWindowFirstLine() const noexcept;

    /** @internal Returns true if the caret is currently visible. */
    bool isCaretVisible() const noexcept;

    /** @internal Returns the lines that have a breakpoint set. */
    const std::vector<int>& getBreakpointLines() const noexcept;

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
    void enablementChanged() override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDoubleClick (const MouseEvent& event) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void textInput (const String& text) override;

    //==============================================================================
    /** @internal Returns the rectangle where text input is being edited. */
    Rectangle<float> getTextInputRect() const override;

private:
    //==============================================================================
    friend class CodeDocument::Listener;

    class DocumentListener;
    class Minimap;

    void initialiseEditor();
    void updateStyledTextIfNeeded();
    void rebuildStyledText();
    void clampScrollOffsetIfNeeded();
    void updateScrollBar();
    float getScrollBarWidth() const noexcept;

    void updateCaretPosition();
    void ensureCaretVisible();
    void blinkCaret();
    void stopCaretBlinking();
    void startCaretBlinking();
    Rectangle<float> getCaretBoundsInDocument() const;

    int getGlyphIndexAtPosition (const Point<float>& position) const;

    Range<int> computeShapingWindow() const;
    bool windowNeedsExpanding() const;
    void shapeWindow (Range<int> window);

    std::vector<Rectangle<float>> getSelectionRectanglesInDocument (int startIndex, int endIndex) const;

    void handleBackspace();
    void handleDelete();

    void moveCaretLeft (bool extendSelection);
    void moveCaretRight (bool extendSelection);
    void moveCaretUp (bool extendSelection);
    void moveCaretDown (bool extendSelection);
    void moveCaretTo (int newPosition, bool extendSelection);
    void moveCaretToStartOfLine (bool extendSelection);
    void moveCaretToEndOfLine (bool extendSelection);
    void moveCaretToStart (bool extendSelection);
    void moveCaretToEnd (bool extendSelection);
    void moveCaretToWordStart (bool extendSelection);
    void moveCaretToWordEnd (bool extendSelection);

    bool isWordSeparator (yup_wchar character) const;
    int findWordStart (int position) const;
    int findWordEnd (int position) const;
    void deleteWordBackward();
    void deleteWordForward();

    int getCurrentLineNumber() const;
    String getLineIndent (int lineNumber) const;
    bool canEdit() const;

    void toggleBreakpointAt (const Point<float>& position);
    float getMinimapWidth() const;

    //==============================================================================
    CodeDocument* document = nullptr;
    std::unique_ptr<CodeDocument> internalDocument;
    std::unique_ptr<DocumentListener> documentListener;
    std::unique_ptr<Minimap> minimap;
    std::unique_ptr<ScrollBar> scrollBar;

    CodeTokeniser tokeniser;
    CodeEditorScheme scheme;
    StyledText styledText;
    Font font;
    float fontSize = 14.0f;

    int caretPosition = 0;
    Range<int> selection;

    bool readOnly = false;
    bool lineNumbersVisible = true;
    bool isDragging = false;
    bool caretVisible = true;
    bool caretBlinking = false;
    bool needsStyledTextUpdate = true;

    int tabWidth = 4;

    Point<float> scrollOffset;

    int windowFirstLine = 0;
    int windowLastLine = -1;
    int windowStartIndex = 0;
    int windowEndIndex = 0;

    mutable float cachedLineHeight = -1.0f;

    TimedCallback caretTimer;

    //==============================================================================
    String searchText;
    bool searchCaseSensitive = false;
    std::vector<Range<int>> searchMatches;
    std::vector<int> breakpointLines;
    bool minimapVisible = true;

    mutable String cachedDocumentText;
    mutable bool cachedDocumentTextDirty = true;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CodeEditor)
};

} // namespace yup
