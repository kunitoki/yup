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
class CodeFindBar;

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

    /** Sets a built-in syntax definition by language name ("cpp", "glsl", "python", "xml", "ydsp", "yui").

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

    /** Selects a whole line including its line break (the last line up to the end of the document).

        A double-click followed by a third click selects the clicked line the same way.

        @param lineNumber The zero-based line, clamped to the document.
    */
    void selectLine (int lineNumber);

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

    /** Indents the lines touched by the selection (or the caret line) by one tab width (undoable).

        Empty lines are left untouched. The selection is kept and follows the moved text.
    */
    void indentSelection();

    /** Removes one level of indentation from the lines touched by the selection (or the caret line).

        Up to one tab width of leading spaces, or a single leading tab, is removed from each line.
        The selection is kept and follows the moved text.
    */
    void outdentSelection();

    /** Comments or uncomments the lines touched by the selection (or the caret line) (undoable).

        When every non-blank touched line starts with the definition's line comment prefix the
        prefix (and one following space) is removed, otherwise the prefix and a space are inserted
        at the smallest indentation of the non-blank lines. Without a line comment prefix this
        falls back to toggleBlockComment(). Bound to Ctrl+/ (Cmd+/ on macOS).
    */
    void toggleLineComment();

    /** Wraps the selection in the definition's block comment delimiters, or removes them (undoable).

        Without a selection the text of the caret line after its indentation is used. The
        delimiters (and one inner space on each side) are removed when the selected text, ignoring
        surrounding whitespace, already starts and ends with them. Without block comment
        delimiters this falls back to toggleLineComment(). Bound to Shift+Alt+A.
    */
    void toggleBlockComment();

    /** Moves the lines touched by the selection (or the caret line) one line up or down (undoable).

        The selection moves with the lines. Nothing happens at the start or end of the document.
        Bound to Alt+Up and Alt+Down.

        @param moveUp True to move the lines up, false to move them down.
    */
    void moveSelectedLines (bool moveUp);

    /** Duplicates the lines touched by the selection (or the caret line) (undoable).

        The copy is inserted below the original lines. Bound to Shift+Alt+Down (selecting the
        copy) and Shift+Alt+Up (keeping the selection on the original lines).

        @param selectCopy True to move the selection onto the copy.
    */
    void duplicateSelectedLines (bool selectCopy = true);

    /** Deletes the lines touched by the selection (or the caret line) (undoable).

        The caret is placed on the line that follows the deleted ones. Bound to Ctrl+Shift+K.
    */
    void deleteSelectedLines();

    /** Inserts an empty line, indented like the caret line, below or above it and moves the caret
        there (undoable). Bound to Ctrl+Enter and Ctrl+Shift+Enter.

        @param above True to insert above the caret line, false to insert below it.
    */
    void insertLine (bool above);

    //==============================================================================
    /** Undoes the most recent edit, if any.

        The caret and selection are restored to what they were before the undone edit.
    */
    bool undo();

    /** Redoes the most recently undone edit, if any.

        The caret and selection are restored to what they were after the redone edit.
    */
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

    /** Returns true if indentation inserts tab characters instead of spaces. */
    bool isIndentUsingTabs() const noexcept { return indentUsingTabs; }

    /** Sets how Tab, block indent and auto-indent insert one indent level.

        @param shouldUseTabs True to insert a tab character, false (the default) to insert
                             getTabWidth() spaces.
    */
    void setIndentUsingTabs (bool shouldUseTabs);

    /** Returns true if typed brackets and quotes are closed automatically. */
    bool isAutoClosingEnabled() const noexcept { return autoClosingEnabled; }

    /** Enables or disables auto-closing of the syntax definition's auto-closing pairs.

        When enabled (the default), typing an opening character inserts its closing one
        after the caret if the next character is whitespace, a closing character or the
        end of the line; typing the closing character next to it steps over it; typing an
        opening character with a selection wraps the selection; and Backspace between an
        empty pair removes both characters.

        @param shouldAutoClose True to enable auto-closing.

        @see SyntaxDefinition::getAutoClosingPairs
    */
    void setAutoClosingEnabled (bool shouldAutoClose);

    /** Returns true if other occurrences of the word at the caret are highlighted. */
    bool isOccurrenceHighlightingEnabled() const noexcept { return occurrenceHighlighting; }

    /** Enables or disables the highlight of other occurrences of the word at the caret.

        When enabled (the default), every whole-word occurrence of the word under the caret,
        or of the selected text when the selection is exactly one word, is highlighted in the
        visible lines with the CodeEditorScheme::ColorId::wordOccurrence color.

        @param shouldHighlight True to enable occurrence highlighting.

        @see getWordOccurrences
    */
    void setOccurrenceHighlightingEnabled (bool shouldHighlight);

    /** Returns true if indent guides are painted. */
    bool isIndentGuidesVisible() const noexcept { return indentGuidesVisible; }

    /** Shows or hides the vertical indent guides.

        A guide is painted at every indentation level of a line (one level is getTabWidth()
        columns). Blank lines take the smaller indent of the closest non-blank lines above and
        below, so guides run continuously through a block. Enabled by default.

        @param shouldBeVisible True to paint indent guides.
    */
    void setIndentGuidesVisible (bool shouldBeVisible);

    /** Returns true if whitespace characters are rendered. */
    bool isWhitespaceVisible() const noexcept { return whitespaceVisible; }

    /** Shows or hides whitespace markers.

        When enabled, spaces are rendered as centered dots and tabs as arrows, using the
        CodeEditorScheme::ColorId::whitespace color. Disabled by default.

        @param shouldBeVisible True to render whitespace.
    */
    void setWhitespaceVisible (bool shouldBeVisible);

    /** Returns the number of pixels scrolled from the top-left of the document. */
    Point<float> getScrollOffset() const noexcept { return scrollOffset; }

    /** Sets the scroll offset, clamped to the document bounds. */
    void setScrollOffset (Point<float> newOffset);

    /** Scrolls so that a line is visible. */
    void scrollToLine (int lineNumber);

    //==============================================================================
    /** Options controlling how find and replace match the search text. */
    struct SearchOptions
    {
        /** Whether letter case must match. */
        bool caseSensitive = false;

        /** Whether matches must start and end at word boundaries. */
        bool wholeWord = false;

        /** Whether the search text is an ECMAScript regular expression.

            Regular expressions match within a single line. In replacements, `$0` inserts
            the whole match, `$1` to `$9` a capture group and `$$` a dollar sign.
        */
        bool regex = false;
    };

    /** Returns all matches of a search text in the document.

        @param searchText The text or regular expression to find.
        @param options    How the text is matched.
        @returns The ranges of all matches in document order (empty if none or if the
                 regular expression is invalid).
    */
    std::vector<Range<int>> findAll (StringRef searchText, const SearchOptions& options) const;

    /** Returns all occurrences of a search string in the document.

        @param searchText    The text to find.
        @param caseSensitive Whether the search is case sensitive.
        @param wholeWord     Whether matches must be whole words.
        @returns The ranges of all matches (empty if none).
    */
    std::vector<Range<int>> findAll (StringRef searchText, bool caseSensitive = false, bool wholeWord = false) const;

    /** Finds the next match after the selection and selects it.

        @param searchText The text or regular expression to find.
        @param options    How the text is matched.
        @param wrapAround Whether to continue from the start when the end is reached.
        @returns True if a match was found and selected.
    */
    bool findNext (StringRef searchText, const SearchOptions& options, bool wrapAround = true);

    /** Finds the next occurrence after the caret and selects it.

        @param searchText    The text to find.
        @param caseSensitive Whether the search is case sensitive.
        @param wrapAround    Whether to continue from the start when the end is reached.
        @returns True if a match was found and selected.
    */
    bool findNext (StringRef searchText, bool caseSensitive = false, bool wrapAround = true);

    /** Finds the previous match before the selection and selects it.

        @param searchText The text or regular expression to find.
        @param options    How the text is matched.
        @param wrapAround Whether to continue from the end when the start is reached.
        @returns True if a match was found and selected.
    */
    bool findPrevious (StringRef searchText, const SearchOptions& options, bool wrapAround = true);

    /** Finds the previous occurrence before the caret and selects it.

        @param searchText    The text to find.
        @param caseSensitive Whether the search is case sensitive.
        @param wrapAround    Whether to continue from the end when the start is reached.
        @returns True if a match was found and selected.
    */
    bool findPrevious (StringRef searchText, bool caseSensitive = false, bool wrapAround = true);

    /** Replaces the selection if it is exactly a match, then selects the next match.

        @param searchText  The text or regular expression to replace.
        @param replacement The replacement text, with capture references for regular expressions.
        @param options     How the text is matched.
        @returns The number of replacements performed (0 or 1).
    */
    int replaceNext (StringRef searchText, StringRef replacement, const SearchOptions& options);

    /** Replaces the current selection if it matches, then selects the next match.

        @param searchText    The text to replace.
        @param replacement   The replacement text.
        @param caseSensitive Whether the search is case sensitive.
        @returns The number of replacements performed (0 or 1).
    */
    int replaceNext (StringRef searchText, StringRef replacement, bool caseSensitive = false);

    /** Replaces every match in the document as a single undo step.

        @param searchText  The text or regular expression to replace.
        @param replacement The replacement text, with capture references for regular expressions.
        @param options     How the text is matched.
        @returns The number of replacements performed.
    */
    int replaceAll (StringRef searchText, StringRef replacement, const SearchOptions& options);

    /** Replaces every occurrence of a search string in the document.

        @param searchText    The text to replace.
        @param replacement   The replacement text.
        @param caseSensitive Whether the search is case sensitive.
        @returns The number of replacements performed.
    */
    int replaceAll (StringRef searchText, StringRef replacement, bool caseSensitive = false);

    /** Highlights all matches of a search text while painting.

        The highlights follow document edits until they are cleared.

        @param searchText The text or regular expression to highlight (empty clears the highlights).
        @param options    How the text is matched.
    */
    void highlightSearchMatches (StringRef searchText, const SearchOptions& options);

    /** Highlights all matches of a search string while painting.

        @param searchText    The text to highlight (empty clears the highlights).
        @param caseSensitive Whether the search is case sensitive.
    */
    void highlightSearchMatches (StringRef searchText, bool caseSensitive = false);

    /** Returns the ranges currently highlighted by highlightSearchMatches. */
    const std::vector<Range<int>>& getSearchMatches() const;

    /** Clears the search match highlights. */
    void clearSearchHighlights();

    //==============================================================================
    /** Shows the find bar (Ctrl+F), or the find and replace bar (Ctrl+H).

        A single-line selection becomes the search text. The replace field is not shown
        when the editor is read-only.

        @see CodeFindBar
    */
    void showFind (bool withReplace = false);

    /** Shows the go-to-line bar (Ctrl+G). */
    void showGoToLine();

    /** Hides the find bar if it is showing. */
    void hideFindBar();

    /** Returns the find bar, or nullptr if it was never shown. */
    CodeFindBar* getFindBar() const noexcept { return findBar.get(); }

    /** Moves the caret to a line and column (both 0-based, clamped to the document) and
        scrolls the line into the center of the view.
    */
    void goToLine (int lineNumber, int column = 0);

    //==============================================================================
    /** Returns the matching bracket range for the bracket at (or next to) a position.

        The bracket pairs come from the syntax definition (SyntaxDefinition::getBracketPairs).
        Returns a range covering the opening and closing bracket, or std::nullopt when
        the position is not on a bracket or the bracket has no match.

        @param position The global character position to inspect.
        @returns The matching bracket range, if any.
    */
    std::optional<Range<int>> getBracketMatch (int position) const;

    /** Returns the occurrences of the word at the caret in the visible lines.

        The word is the selected text when the selection is exactly one word, otherwise the
        word under the caret. Only whole-word matches are returned, including the word itself.
        Returns an empty vector when there is no word or occurrence highlighting is disabled.

        @returns The global character ranges of the occurrences, in document order.

        @see setOccurrenceHighlightingEnabled
    */
    std::vector<Range<int>> getWordOccurrences() const;

    //==============================================================================
    /** Sets or clears a breakpoint on a line.

        Breakpoints move with the text when lines are inserted or removed above them.

        @param lineNumber       The 0-based line.
        @param shouldBeEnabled  Whether the breakpoint should be set.
    */
    void setBreakpoint (int lineNumber, bool shouldBeEnabled);

    /** Returns true if a breakpoint is set on a line. */
    bool getBreakpoint (int lineNumber) const;

    /** Removes all breakpoints. */
    void clearBreakpoints();

    /** Returns true if clicking the line-number gutter toggles breakpoints. */
    bool areGutterBreakpointsEnabled() const noexcept { return gutterBreakpointsEnabled; }

    /** Sets whether clicking the line-number gutter toggles breakpoints.

        Off by default, in which case a gutter click places the caret like a click in the text.
        Breakpoints set with setBreakpoint() are shown either way.

        @param shouldBeEnabled  Whether gutter clicks should toggle breakpoints.
    */
    void setGutterBreakpointsEnabled (bool shouldBeEnabled) noexcept { gutterBreakpointsEnabled = shouldBeEnabled; }

    //==============================================================================
    /** A problem reported for a range of the document, for example by a compiler or a linter.

        Positions are 0-based lines and columns (characters), like the Language Server
        Protocol. They are clamped to the document when used, so a diagnostic that went
        stale after an edit never points outside the text.
    */
    struct Diagnostic
    {
        /** How serious the problem is, which selects its color. */
        enum class Severity
        {
            error,       /**< CodeEditorScheme::ColorId::diagnosticError. */
            warning,     /**< CodeEditorScheme::ColorId::diagnosticWarning. */
            information, /**< CodeEditorScheme::ColorId::diagnosticInformation. */
            hint         /**< CodeEditorScheme::ColorId::diagnosticHint, underline only. */
        };

        int line = 0;      /**< The first line of the range. */
        int column = 0;    /**< The first column on the first line. */
        int endLine = 0;   /**< The last line of the range. */
        int endColumn = 0; /**< The column after the range on the last line. An empty range marks the word starting at it. */

        Severity severity = Severity::error; /**< The severity. */
        String message;                      /**< The text shown when hovering the range. */
        String source;                       /**< Optional origin, for example the compiler name. */

        /** When true the whole line is filled with CodeEditorScheme::ColorId::errorLine
            instead of being underlined (this is how setErrorLine is expressed).
        */
        bool highlightLine = false;
    };

    /** Replaces all diagnostics, including the ones added by setErrorLine.

        Diagnostics are underlined with a squiggle in their severity color and the gutter
        shows a marker on their first line. They move with the text when lines are inserted
        or removed above them; setDocument() removes them all.
    */
    void setDiagnostics (std::vector<Diagnostic> newDiagnostics);

    /** Adds one diagnostic. */
    void addDiagnostic (const Diagnostic& diagnostic);

    /** Returns all diagnostics, with their positions updated by the edits made since they were set. */
    const std::vector<Diagnostic>& getDiagnostics() const noexcept { return diagnostics; }

    /** Removes all diagnostics, including the error lines. */
    void clearDiagnostics();

    /** Returns the diagnostics whose range contains a character index, most severe first.

        The end of a range counts as inside, so the caret right after an underlined word
        still reports it.
    */
    std::vector<Diagnostic> getDiagnosticsAt (int characterIndex) const;

    /** Returns the character range a diagnostic covers in the current document. */
    Range<int> getDiagnosticRange (const Diagnostic& diagnostic) const;

    //==============================================================================
    /** A range of a single line colored with a scope computed by a language service. */
    struct SemanticToken
    {
        int line = 0;     /**< The 0-based line. */
        int column = 0;   /**< The 0-based character offset within the line. */
        int length = 0;   /**< The number of characters covered. */
        Identifier scope; /**< The dotted scope (e.g. "type.class"), resolved by CodeEditorScheme::getColorForScope. */
    };

    /** Replaces the semantic tokens.

        Semantic tokens are drawn over the tokens of the SyntaxDefinition, so a language
        service can color what a lexer cannot tell apart (types, parameters, macros...).
        Overlapping tokens are applied in order, the later one winning. Tokens move with
        the text when lines are inserted or removed above them and are dropped from the
        lines that are edited, until the next call; setDocument() removes them all.
    */
    void setSemanticTokens (std::vector<SemanticToken> newTokens);

    /** Returns the semantic tokens, sorted by line (keeping their order within a line) and updated by the edits made since they were set. */
    const std::vector<SemanticToken>& getSemanticTokens() const noexcept { return semanticTokens; }

    /** Removes all semantic tokens. */
    void clearSemanticTokens();

    /** Returns the tokens used to color a line: the syntax tokens with the semantic tokens applied.

        @param lineIndex The 0-based line.
        @returns The tokens covering the line, or an empty vector for an out of range or uncolored line.
    */
    std::vector<CodeTokeniser::Token> getLineTokens (int lineIndex);

    //==============================================================================
    /** Returns the hover text for a character index, or an empty string for none.

        The text may contain newlines; long lines are wrapped in the hover popup.
    */
    using HoverProvider = std::function<String (const CodeEditor& editor, int characterIndex)>;

    /** Sets the provider queried when the mouse rests over a character.

        The popup shows the messages of the diagnostics under the mouse followed by the
        provider text. Pass nullptr to only show diagnostics.
    */
    void setHoverProvider (HoverProvider newProvider);

    /** Returns the text the hover popup shows for a character: the diagnostic messages
        covering it (as "source: message", most severe first) and the provider text,
        separated by blank lines.
    */
    String getHoverText (int characterIndex) const;

    /** Shows the hover popup for a character right away, below it (or above when there is
        no room). Does nothing but hide the popup when getHoverText() is empty.
    */
    void showHover (int characterIndex);

    /** Hides the hover popup. Typing, clicking, scrolling and edits hide it too. */
    void hideHover();

    /** Returns true if the hover popup is showing. */
    bool isHoverVisible() const noexcept;

    /** Returns how long the mouse must rest over a character before the hover popup shows. */
    int getHoverDelay() const noexcept { return hoverDelayMs; }

    /** Sets how long the mouse must rest over a character before the hover popup shows. */
    void setHoverDelay (int milliseconds) noexcept { hoverDelayMs = jmax (0, milliseconds); }

    //==============================================================================
    /** One entry of the completion popup. */
    struct CompletionItem
    {
        String label;      /**< The text shown and matched against the typed word. */
        String insertText; /**< The text inserted when accepted, the label when empty. */
        String detail;     /**< Optional dimmed text shown on the right, for example a type. */
    };

    /** Returns the completions available at a caret position.

        The provider is queried once when the popup opens; typing more of the word then
        filters the returned items without querying again.
    */
    using CompletionProvider = std::function<std::vector<CompletionItem> (const CodeEditor& editor, int caretPosition)>;

    /** Sets the completion provider. Pass nullptr to disable completion. */
    void setCompletionProvider (CompletionProvider newProvider);

    /** Sets the characters that open the completion popup when typed, for example ".:>". */
    void setCompletionTriggerCharacters (const String& characters) { completionTriggers = characters; }

    /** Returns the characters that open the completion popup when typed. */
    const String& getCompletionTriggerCharacters() const noexcept { return completionTriggers; }

    /** Opens the completion popup at the caret (Ctrl+Space).

        The word before the caret filters the items: labels containing its characters in
        order match, case insensitive, and labels starting with it come first. The popup
        closes when nothing matches. While it is open, Up / Down / Page Up / Page Down
        select, Enter or Tab accept, Escape, clicks and caret moves close it.
    */
    void showCompletions();

    /** Closes the completion popup. */
    void hideCompletions();

    /** Returns true if the completion popup is showing. */
    bool isCompletionVisible() const noexcept;

    /** Returns the items currently listed in the popup, filtered and sorted. */
    const std::vector<CompletionItem>& getVisibleCompletions() const noexcept { return visibleCompletions; }

    /** Returns the index of the selected item in getVisibleCompletions(). */
    int getSelectedCompletionIndex() const noexcept { return selectedCompletion; }

    /** Selects an item of getVisibleCompletions(), clamped. */
    void setSelectedCompletionIndex (int newIndex);

    /** Replaces the word before the caret with the selected item and closes the popup.

        @returns False if the popup was not showing.
    */
    bool acceptCompletion();

    //==============================================================================
    /** Highlights a line as an error, or removes its highlight.

        Error lines are diagnostics with Diagnostic::highlightLine set: they are filled
        across the whole text width with the scheme's CodeEditorScheme::ColorId::errorLine
        color. They move with the text when lines are inserted or removed above them;
        setDocument() removes them all.

        @param lineNumber  The 0-based line, clamped to the document.
        @param isError     Whether the line should be highlighted.
    */
    void setErrorLine (int lineNumber, bool isError);

    /** Returns true if a line is highlighted as an error. */
    bool isErrorLine (int lineNumber) const;

    /** Removes all error line highlights. */
    void clearErrorLines();

    //==============================================================================
    /** Returns true if the minimap overview is visible. */
    bool isMinimapVisible() const noexcept { return minimapVisible; }

    /** Sets whether the minimap overview is visible. */
    void setMinimapVisible (bool shouldBeVisible);

    //==============================================================================
    struct Style
    {
        static const Identifier contentPartId; /**< Name of the part that is the editor as the theme paints it: `part content { }` in a drawing */
    };

    //==============================================================================
    /** Returns the static type info describing the CodeEditor type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal Returns the rectangle of the text area (used by the theme for painting). */
    Rectangle<float> getTextBounds() const;

    /** @internal Returns the height of a single line of text. */
    float getLineHeight() const;

    /** @internal Returns the caret bounds in view coordinates (used by the theme for painting). */
    Rectangle<float> getCaretBounds() const;

    /** @internal Returns the current selection rectangles in view coordinates. */
    std::vector<Rectangle<float>> getSelectedTextAreas() const;

    /** @internal Returns the search-match highlight rectangles in view coordinates. */
    std::vector<Rectangle<float>> getSearchMatchAreas() const;

    /** @internal Returns the rectangles of the bracket pair at the caret in view coordinates. */
    std::vector<Rectangle<float>> getBracketMatchAreas() const;

    /** @internal Returns the rectangles of the word occurrences in view coordinates. */
    std::vector<Rectangle<float>> getWordOccurrenceAreas() const;

    /** @internal Returns one thin rectangle per visible indent guide in view coordinates. */
    std::vector<Rectangle<float>> getIndentGuideAreas() const;

    /** @internal A rendered whitespace character. */
    struct WhitespaceMarker
    {
        Rectangle<float> area; /**< The character cell in view coordinates. */
        bool isTab = false;    /**< True for a tab, false for a space. */
    };

    /** @internal Returns the whitespace markers of the visible lines (empty when hidden). */
    std::vector<WhitespaceMarker> getWhitespaceMarkers() const;

    /** @internal Returns the styled text used for rendering. */
    const StyledText& getStyledText() const noexcept;

    /** @internal Returns the first line of the currently shaped text window. */
    int getWindowFirstLine() const noexcept;

    /** @internal Returns true if the caret is currently visible. */
    bool isCaretVisible() const noexcept;

    /** @internal Returns the lines that have a breakpoint set. */
    const std::vector<int>& getBreakpointLines() const noexcept;

    /** @internal Returns the lines highlighted as errors, sorted. */
    std::vector<int> getErrorLines() const;

    /** @internal A diagnostic underline or gutter marker in view coordinates. */
    struct DiagnosticMarker
    {
        Rectangle<float> area;
        Diagnostic::Severity severity = Diagnostic::Severity::error;
    };

    /** @internal Returns the text areas to underline, least severe first so errors paint on top. */
    std::vector<DiagnosticMarker> getDiagnosticUnderlineAreas() const;

    /** @internal Returns one gutter marker per visible line with diagnostics, using the most severe one. */
    std::vector<DiagnosticMarker> getDiagnosticGutterMarkers() const;

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
    bool mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;
    /** @internal */
    void mouseMove (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    bool keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void textInput (const String& text) override;

    //==============================================================================
    /** @internal Returns the rectangle where text input is being edited. */
    Rectangle<float> getTextInputRect() const override;

private:
    //==============================================================================
    friend class CodeDocument::Listener;

    class CompletionPopup;
    class DocumentListener;
    class HoverPopup;
    class Minimap;
    class SelectionAction;

    enum class EditKind
    {
        command,
        typing,
        deleting
    };

    void performUndoableEdit (const std::function<void()>& edit, EditKind kind);
    void restoreSelection (int newCaretPosition, Range<int> newSelection);
    Range<int> getSelectedLines() const;
    void changeIndentOfSelectedLines (bool outdent);

    struct LineEdit
    {
        int line = 0;
        int column = 0;
        int numToRemove = 0;
        String insertion;
    };

    void applyLineEdits (const std::vector<LineEdit>& edits);
    void replaceLineBlock (Range<int> lines, const String& newText, int newCaret, Range<int> newSelection);

    void initialiseEditor();
    void updateStyledTextIfNeeded();
    void rebuildStyledText();
    void clampScrollOffsetIfNeeded();
    void updateScrollBar();
    float getScrollBarWidth() const noexcept;
    float getHorizontalScrollBarHeight() const noexcept;

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
    std::vector<Rectangle<float>> getViewRectangles (int startIndex, int endIndex) const;
    float getCharacterWidth() const;
    int getIndentColumns (const String& lineText) const;

    void handleBackspace();
    void handleDelete();
    void insertNewLine();
    bool handleTypedCharacter (yup_wchar character);
    String getIndentUnit() const;

    void moveCaretLeft (bool extendSelection);
    void moveCaretRight (bool extendSelection);
    void moveCaretUp (bool extendSelection);
    void moveCaretDown (bool extendSelection);
    void moveCaretTo (int newPosition, bool extendSelection);
    void moveCaretVertically (int numLines, bool extendSelection);
    int getLinesPerPage() const;
    void moveCaretToStartOfLine (bool extendSelection);
    void moveCaretToEndOfLine (bool extendSelection);
    void moveCaretToStart (bool extendSelection);
    void moveCaretToEnd (bool extendSelection);
    void moveCaretToWordStart (bool extendSelection);
    void moveCaretToWordEnd (bool extendSelection);

    bool isWordSeparator (yup_wchar character) const;
    const std::vector<std::pair<yup_wchar, yup_wchar>>& getBracketPairs() const;
    std::optional<int> findMatchingBracket (const CodeDocument::Position& start, yup_wchar open, yup_wchar close, int direction) const;
    int findWordStart (int position) const;

    struct SearchMatch
    {
        Range<int> range;
        StringArray groups;
    };

    std::vector<SearchMatch> findMatches (StringRef searchText, const SearchOptions& options) const;
    static String expandReplacement (const String& replacement, const StringArray& groups);
    int findWordEnd (int position) const;
    void deleteWordBackward();
    void deleteWordForward();

    int getCurrentLineNumber() const;
    String getLineIndent (int lineNumber) const;
    bool canEdit() const;

    void toggleBreakpointAt (const Point<float>& position);
    float getMinimapWidth() const;
    CodeFindBar& getOrCreateFindBar();
    int getCharacterIndexAt (const Point<float>& position);
    void updateCompletions();
    bool handleCompletionKey (const KeyPress& key);
    void layoutFindBar();

    //==============================================================================
    CodeDocument* document = nullptr;
    std::unique_ptr<CodeDocument> internalDocument;
    std::unique_ptr<DocumentListener> documentListener;
    std::unique_ptr<Minimap> minimap;
    std::unique_ptr<ScrollBar> scrollBar;
    std::unique_ptr<ScrollBar> horizontalScrollBar;
    std::unique_ptr<CodeFindBar> findBar;
    std::unique_ptr<HoverPopup> hoverPopup;
    HoverProvider hoverProvider;
    TimedCallback hoverTimer;
    int hoverIndex = -1;
    int hoverDelayMs = 500;
    std::unique_ptr<CompletionPopup> completionPopup;
    CompletionProvider completionProvider;
    String completionTriggers;
    std::vector<CompletionItem> completionItems;
    std::vector<CompletionItem> visibleCompletions;
    int selectedCompletion = 0;
    int completionStart = 0;

    CodeTokeniser tokeniser;
    CodeEditorScheme scheme;
    StyledText styledText;
    Font font;
    float fontSize = 14.0f;

    int caretPosition = 0;
    Range<int> selection;
    std::optional<std::pair<int, Range<int>>> pendingSelectionRestore;

    bool readOnly = false;
    bool lineNumbersVisible = true;
    bool isDragging = false;
    bool caretVisible = true;
    bool caretBlinking = false;
    bool needsStyledTextUpdate = true;

    int tabWidth = 4;
    std::optional<int> preferredColumn;
    std::optional<uint32> lastDoubleClickTime;
    Point<float> lastDoubleClickPosition;
    static constexpr uint32 tripleClickTimeoutMs = 600;
    bool indentUsingTabs = false;
    bool autoClosingEnabled = true;
    bool occurrenceHighlighting = true;
    bool indentGuidesVisible = true;
    bool whitespaceVisible = false;
    int documentChangeCount = 0;

    EditKind lastEditKind = EditKind::command;
    int lastEditCaretPosition = -1;
    int lastEditChangeCount = -1;

    Point<float> scrollOffset;

    int windowFirstLine = 0;
    int windowLastLine = -1;
    int windowStartIndex = 0;
    int windowEndIndex = 0;

    mutable float cachedLineHeight = -1.0f;

    TimedCallback caretTimer;

    //==============================================================================
    String searchText;
    SearchOptions searchOptions;
    mutable std::vector<Range<int>> searchMatches;
    mutable bool searchMatchesNeedUpdate = false;
    std::vector<int> breakpointLines;
    std::vector<Diagnostic> diagnostics;
    std::vector<SemanticToken> semanticTokens;
    bool gutterBreakpointsEnabled = false;
    bool minimapVisible = true;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CodeEditor)
};

} // namespace yup
