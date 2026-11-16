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
/** The find, replace and go-to-line bar shown at the top of a CodeEditor.

    The bar is owned by its editor and created on demand by CodeEditor::showFind and
    CodeEditor::showGoToLine. It highlights the matches of the search text while typing,
    selects the next match from where the search started and shows the match count.

    Keys while the bar has focus:
    - Enter / Shift+Enter (or F3 / Shift+F3): next / previous match.
    - Enter in the replace field: replace the current match; Ctrl+Alt+Enter: replace all.
    - Alt+C, Alt+W, Alt+R: toggle case sensitive, whole word and regular expression.
    - Tab: switch between the find and replace fields.
    - Escape: close the bar and return focus to the editor.

    In go-to-line mode the field accepts `line` or `line:column` (1-based).

    @see CodeEditor
*/
class YUP_API CodeFindBar : public Component
{
public:
    //==============================================================================
    /** What the bar is showing. */
    enum class Mode
    {
        find,     /**< The find field with options and navigation. */
        replace,  /**< Find plus the replace field and replace buttons. */
        goToLine  /**< A line number field. */
    };

    //==============================================================================
    /** Creates a bar searching the given editor. */
    explicit CodeFindBar (CodeEditor& editor);

    /** Destructor. */
    ~CodeFindBar() override;

    //==============================================================================
    /** Shows the bar in a mode and focuses its first field.

        In find and replace mode a single-line selection of the editor becomes the
        search text, and the search starts from the selection.
    */
    void show (Mode newMode);

    /** Hides the bar, clears the search highlights and gives the focus back to the editor. */
    void hide();

    /** Returns the current mode. */
    Mode getMode() const noexcept { return mode; }

    /** Returns the height the bar needs in its current mode. */
    float getPreferredHeight() const noexcept;

    //==============================================================================
    /** Returns the search text. */
    String getSearchText() const;

    /** Sets the search text and updates the highlights and the selected match. */
    void setSearchText (const String& newText);

    /** Returns the replacement text. */
    String getReplaceText() const;

    /** Sets the replacement text. */
    void setReplaceText (const String& newText);

    /** Returns the search options selected in the bar. */
    CodeEditor::SearchOptions getSearchOptions() const;

    /** Sets the search options and updates the highlights. */
    void setSearchOptions (const CodeEditor::SearchOptions& newOptions);

    //==============================================================================
    /** Selects the next match. Returns true if one was found. */
    bool findNext();

    /** Selects the previous match. Returns true if one was found. */
    bool findPrevious();

    /** Replaces the selected match and selects the next one. Returns the number of replacements. */
    int replaceNext();

    /** Replaces every match. Returns the number of replacements. */
    int replaceAll();

    /** Moves the caret to the line (and optional column) typed in go-to-line mode.

        @returns True if the text was a valid line number and the bar was closed.
    */
    bool goToLine();

    /** Returns the text of the go-to-line field. */
    String getLineText() const;

    /** Sets the text of the go-to-line field. */
    void setLineText (const String& newText);

    /** Returns the status shown next to the fields, for example "2 of 5" or "No results". */
    String getStatusText() const;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    bool keyDown (const KeyPress& key, const Point<float>& position) override;

private:
    void searchChanged();
    void updateStatus();
    void updateVisibleComponents();

    CodeEditor& editor;
    Mode mode = Mode::find;
    int searchStartPosition = 0;

    TextEditor findField;
    TextEditor replaceField;
    TextEditor lineField;
    ToggleButton caseButton;
    ToggleButton wordButton;
    ToggleButton regexButton;
    Label statusLabel;
    TextButton previousButton;
    TextButton nextButton;
    TextButton closeButton;
    TextButton replaceButton;
    TextButton replaceAllButton;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CodeFindBar)
};

} // namespace yup
