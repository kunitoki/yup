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

constexpr float findBarRowHeight = 28.0f;
constexpr float findBarPadding = 4.0f;

} // namespace

//==============================================================================

CodeFindBar::CodeFindBar (CodeEditor& editorToSearch)
    : editor (editorToSearch)
{
    setWantsKeyboardFocus (false);

    findField.onTextChange = [this]
    {
        searchChanged();
    };

    lineField.onTextChange = [this]
    {
        updateStatus();
    };

    caseButton.setButtonText ("Aa");
    wordButton.setButtonText ("W");
    regexButton.setButtonText (".*");

    for (auto* button : { &caseButton, &wordButton, &regexButton })
    {
        button->onClick = [this]
        {
            searchChanged();
        };
    }

    previousButton.setButtonText ("<");
    previousButton.onClick = [this]
    {
        findPrevious();
    };

    nextButton.setButtonText (">");
    nextButton.onClick = [this]
    {
        findNext();
    };

    closeButton.setButtonText ("x");
    closeButton.onClick = [this]
    {
        hide();
    };

    replaceButton.setButtonText ("Replace");
    replaceButton.onClick = [this]
    {
        replaceNext();
    };

    replaceAllButton.setButtonText ("All");
    replaceAllButton.onClick = [this]
    {
        replaceAll();
    };

    for (auto* component : std::initializer_list<Component*> { &findField, &replaceField, &lineField, &caseButton, &wordButton, &regexButton, &statusLabel, &previousButton, &nextButton, &closeButton, &replaceButton, &replaceAllButton })
        addChildComponent (component);

    updateVisibleComponents();
}

CodeFindBar::~CodeFindBar() = default;

//==============================================================================

void CodeFindBar::show (Mode newMode)
{
    mode = newMode;

    if (mode == Mode::goToLine)
    {
        lineField.setText ({}, dontSendNotification);
        editor.clearSearchHighlights();
    }
    else
    {
        searchStartPosition = editor.getSelection().getStart();

        const auto selectedText = editor.getSelectedText();
        if (selectedText.isNotEmpty() && ! selectedText.containsChar ('\n'))
            findField.setText (selectedText, dontSendNotification);
    }

    updateVisibleComponents();
    setVisible (true);
    resized();

    if (mode == Mode::goToLine)
    {
        lineField.takeKeyboardFocus();
        updateStatus();
    }
    else
    {
        findField.takeKeyboardFocus();
        findField.selectAll();
        searchChanged();
    }
}

void CodeFindBar::hide()
{
    setVisible (false);
    editor.clearSearchHighlights();
    editor.takeKeyboardFocus();
}

float CodeFindBar::getPreferredHeight() const noexcept
{
    const int numRows = mode == Mode::replace ? 2 : 1;
    return numRows * findBarRowHeight + findBarPadding * 2.0f;
}

//==============================================================================

String CodeFindBar::getSearchText() const
{
    return findField.getText();
}

void CodeFindBar::setSearchText (const String& newText)
{
    findField.setText (newText, dontSendNotification);
    searchChanged();
}

String CodeFindBar::getReplaceText() const
{
    return replaceField.getText();
}

void CodeFindBar::setReplaceText (const String& newText)
{
    replaceField.setText (newText, dontSendNotification);
}

CodeEditor::SearchOptions CodeFindBar::getSearchOptions() const
{
    CodeEditor::SearchOptions options;
    options.caseSensitive = caseButton.getToggleState();
    options.wholeWord = wordButton.getToggleState();
    options.regex = regexButton.getToggleState();
    return options;
}

void CodeFindBar::setSearchOptions (const CodeEditor::SearchOptions& newOptions)
{
    caseButton.setToggleState (newOptions.caseSensitive, dontSendNotification);
    wordButton.setToggleState (newOptions.wholeWord, dontSendNotification);
    regexButton.setToggleState (newOptions.regex, dontSendNotification);
    searchChanged();
}

String CodeFindBar::getLineText() const
{
    return lineField.getText();
}

void CodeFindBar::setLineText (const String& newText)
{
    lineField.setText (newText, dontSendNotification);
    updateStatus();
}

//==============================================================================

bool CodeFindBar::findNext()
{
    const bool found = editor.findNext (getSearchText(), getSearchOptions());
    updateStatus();
    return found;
}

bool CodeFindBar::findPrevious()
{
    const bool found = editor.findPrevious (getSearchText(), getSearchOptions());
    updateStatus();
    return found;
}

int CodeFindBar::replaceNext()
{
    const auto options = getSearchOptions();

    // Select the next match first so the first press replaces something
    if (! editor.hasSelection())
        editor.findNext (getSearchText(), options);

    const int numReplaced = editor.replaceNext (getSearchText(), getReplaceText(), options);
    if (numReplaced == 0)
        editor.findNext (getSearchText(), options);

    updateStatus();
    return numReplaced;
}

int CodeFindBar::replaceAll()
{
    const int numReplaced = editor.replaceAll (getSearchText(), getReplaceText(), getSearchOptions());
    updateStatus();
    return numReplaced;
}

bool CodeFindBar::goToLine()
{
    const auto text = getLineText().trim();
    if (text.isEmpty() || ! text.containsOnly ("0123456789:"))
        return false;

    const int line = text.upToFirstOccurrenceOf (":", false, false).getIntValue();
    const int column = text.containsChar (':') ? text.fromFirstOccurrenceOf (":", false, false).getIntValue() : 1;

    if (line <= 0)
        return false;

    editor.goToLine (line - 1, jmax (0, column - 1));

    setVisible (false);
    editor.takeKeyboardFocus();
    return true;
}

//==============================================================================

String CodeFindBar::getStatusText() const
{
    if (mode == Mode::goToLine)
    {
        const int numLines = editor.getDocument() != nullptr ? editor.getDocument()->getNumLines() : 1;
        return "Line 1 - " + String (numLines);
    }

    const auto text = getSearchText();
    if (text.isEmpty())
        return {};

    const auto options = getSearchOptions();
    if (options.regex && CodeRegex::compile (text, options.caseSensitive).failed())
        return "Invalid pattern";

    const auto& matches = editor.getSearchMatches();
    if (matches.empty())
        return "No results";

    const auto selected = std::find (matches.begin(), matches.end(), editor.getSelection());
    if (selected != matches.end())
        return String (static_cast<int> (selected - matches.begin()) + 1) + " of " + String (static_cast<int> (matches.size()));

    return String (static_cast<int> (matches.size())) + " results";
}

void CodeFindBar::searchChanged()
{
    const auto text = getSearchText();
    const auto options = getSearchOptions();

    editor.highlightSearchMatches (text, options);

    if (text.isNotEmpty())
    {
        editor.setCaretPosition (searchStartPosition);
        editor.findNext (text, options);
    }

    updateStatus();
}

void CodeFindBar::updateStatus()
{
    statusLabel.setText (getStatusText(), dontSendNotification);
}

void CodeFindBar::updateVisibleComponents()
{
    const bool searching = mode != Mode::goToLine;
    const bool replacing = mode == Mode::replace;

    for (auto* component : std::initializer_list<Component*> { &findField, &caseButton, &wordButton, &regexButton, &previousButton, &nextButton })
        component->setVisible (searching);

    for (auto* component : std::initializer_list<Component*> { &replaceField, &replaceButton, &replaceAllButton })
        component->setVisible (replacing);

    lineField.setVisible (! searching);
    statusLabel.setVisible (true);
    closeButton.setVisible (true);
}

//==============================================================================

void CodeFindBar::paint (Graphics& g)
{
    // A style of the bar paints it in place of the colors of the scheme, as it does for any component
    if (auto style = ApplicationTheme::findComponentStyle (*this))
    {
        if (auto theme = ApplicationTheme::getGlobalTheme())
            style->paint (g, *theme, *this);

        return;
    }

    const auto& scheme = editor.getScheme();
    const auto bounds = getLocalBounds();

    g.setFillColor (scheme.getColor (CodeEditorScheme::ColorId::gutterBackground).value_or (Color (0xff252526)));
    g.fillRect (bounds);

    g.setStrokeColor (scheme.getColor (CodeEditorScheme::ColorId::gutterText).value_or (Color (0xff858585)).withAlpha (0.4f));
    g.setStrokeWidth (1.0f);
    g.strokeRect (bounds.reduced (0.5f));
}

void CodeFindBar::resized()
{
    auto row = getLocalBounds().reduced (findBarPadding).withHeight (findBarRowHeight);
    const auto buttonWidth = findBarRowHeight;

    if (mode == Mode::goToLine)
    {
        closeButton.setBounds (row.removeFromRight (buttonWidth));
        statusLabel.setBounds (row.removeFromRight (110.0f).reduced (4.0f, 0.0f));
        lineField.setBounds (row);
        return;
    }

    auto secondRow = row.translated (0.0f, findBarRowHeight);

    closeButton.setBounds (row.removeFromRight (buttonWidth));
    nextButton.setBounds (row.removeFromRight (buttonWidth));
    previousButton.setBounds (row.removeFromRight (buttonWidth));
    statusLabel.setBounds (row.removeFromRight (80.0f).reduced (4.0f, 0.0f));
    regexButton.setBounds (row.removeFromRight (44.0f));
    wordButton.setBounds (row.removeFromRight (44.0f));
    caseButton.setBounds (row.removeFromRight (44.0f));
    findField.setBounds (row);

    replaceField.setBounds (secondRow.withWidth (findField.getWidth()));

    auto replaceButtons = secondRow.withTrimmedLeft (findField.getWidth());
    replaceButton.setBounds (replaceButtons.removeFromLeft (buttonWidth * 3.0f));
    replaceAllButton.setBounds (replaceButtons.removeFromLeft (buttonWidth * 2.0f));
}

bool CodeFindBar::keyDown (const KeyPress& key, const Point<float>&)
{
    const bool shiftDown = key.getModifiers().isShiftDown();
    const bool ctrlDown = key.getModifiers().isControlDown() || key.getModifiers().isCommandDown();
    const bool altDown = key.getModifiers().isAltDown();

    if (key.getKey() == KeyPress::escapeKey)
    {
        hide();
    }
    else if (key.getKey() == KeyPress::enterKey)
    {
        if (mode == Mode::goToLine)
            goToLine();
        else if (replaceField.hasKeyboardFocus() && ctrlDown && altDown)
            replaceAll();
        else if (replaceField.hasKeyboardFocus())
            replaceNext();
        else if (shiftDown)
            findPrevious();
        else
            findNext();
    }
    else if (key.getKey() == KeyPress::f3Key && mode != Mode::goToLine)
    {
        if (shiftDown)
            findPrevious();
        else
            findNext();
    }
    else if (key.getKey() == KeyPress::tabKey && mode == Mode::replace)
    {
        if (findField.hasKeyboardFocus())
            replaceField.takeKeyboardFocus();
        else
            findField.takeKeyboardFocus();
    }
    else if (ctrlDown && key.getKey() == KeyPress::textFKey)
    {
        show (Mode::find);
    }
    else if (ctrlDown && key.getKey() == KeyPress::textHKey && ! editor.isReadOnly())
    {
        show (Mode::replace);
    }
    else if (ctrlDown && key.getKey() == KeyPress::textGKey)
    {
        show (Mode::goToLine);
    }
    else if (altDown && mode != Mode::goToLine && key.getKey() == KeyPress::textCKey)
    {
        caseButton.setToggleState (! caseButton.getToggleState(), dontSendNotification);
        searchChanged();
    }
    else if (altDown && mode != Mode::goToLine && key.getKey() == KeyPress::textWKey)
    {
        wordButton.setToggleState (! wordButton.getToggleState(), dontSendNotification);
        searchChanged();
    }
    else if (altDown && mode != Mode::goToLine && key.getKey() == KeyPress::textRKey)
    {
        regexButton.setToggleState (! regexButton.getToggleState(), dontSendNotification);
        searchChanged();
    }

    // Keys left over by the fields must not reach the editor behind the bar
    return true;
}

} // namespace yup
