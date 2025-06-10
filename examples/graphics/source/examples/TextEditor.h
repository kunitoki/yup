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

class TextEditorDemo : public Component
{
public:
    TextEditorDemo()
        : Component("TextEditorDemo")
    {
        // Create the editors
        singleLineEditor = std::make_unique<TextEditor>("singleLineEditor");
        multiLineEditor = std::make_unique<TextEditor>("multiLineEditor");
        readOnlyEditor = std::make_unique<TextEditor>("readOnlyEditor");
        focused = std::make_unique<TextEditor>("focused");

        // Configure the editors
        singleLineEditor->setText("Single line editor");
        singleLineEditor->setMultiLine(false);

        multiLineEditor->setText("Multi-line editor\nSupports multiple lines\nTry typing here!");
        multiLineEditor->setMultiLine(true);

        readOnlyEditor->setText("This is read-only text that cannot be edited");
        readOnlyEditor->setReadOnly(true);

        // Create buttons with componentID as text
        selectAllButton = std::make_unique<TextButton>("Select All");
        selectAllButton->onClick = [this]()
        {
            if (auto* activeEditor = getActiveEditor())
                activeEditor->selectAll();
        };

        copyButton = std::make_unique<TextButton>("Copy");
        copyButton->onClick = [this]()
        {
            if (auto* activeEditor = getActiveEditor())
                activeEditor->copy();
        };

        pasteButton = std::make_unique<TextButton>("Paste");
        pasteButton->onClick = [this]()
        {
            if (auto* activeEditor = getActiveEditor())
                activeEditor->paste();
        };

        clearButton = std::make_unique<TextButton>("Clear");
        clearButton->onClick = [this]()
        {
            if (auto* activeEditor = getActiveEditor())
                activeEditor->setText("");
        };

        focused->setText("");

        // Create labels
        titleLabel = std::make_unique<Label>("titleLabel");
        singleLineLabel = std::make_unique<Label>("singleLineLabel");
        multiLineLabel = std::make_unique<Label>("multiLineLabel");
        readOnlyLabel = std::make_unique<Label>("readOnlyLabel");

        titleLabel->setText("TextEditor Widget Example");
        singleLineLabel->setText("Single Line Editor:");
        multiLineLabel->setText("Multi Line Editor:");
        readOnlyLabel->setText("Read Only Editor:");

        // Add all components
        addAndMakeVisible(*titleLabel);
        addAndMakeVisible(*singleLineLabel);
        addAndMakeVisible(*singleLineEditor);
        addAndMakeVisible(*multiLineLabel);
        addAndMakeVisible(*multiLineEditor);
        addAndMakeVisible(*readOnlyLabel);
        addAndMakeVisible(*readOnlyEditor);
        addAndMakeVisible(*selectAllButton);
        addAndMakeVisible(*copyButton);
        addAndMakeVisible(*pasteButton);
        addAndMakeVisible(*clearButton);
        addAndMakeVisible(*focused);

        setSize({ 800, 600 });
    }

    void paint(Graphics& g) override
    {
        // Background
        g.setFillColor(yup::Colors::lightgray);
        g.fillAll();

        // Header separator
        g.setStrokeColor(yup::Colors::darkgray);
        g.setStrokeWidth(2.0f);
        g.strokeLine(10.0f, 60.0f, getWidth() - 10.0f, 60.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);

        // Title
        titleLabel->setBounds(area.removeFromTop(40));

        area.removeFromTop(10); // Spacer

        // Single line editor
        singleLineLabel->setBounds(area.removeFromTop(25));
        singleLineEditor->setBounds(area.removeFromTop(30));

        area.removeFromTop(15); // Spacer

        // Multi line editor
        multiLineLabel->setBounds(area.removeFromTop(25));
        multiLineEditor->setBounds(area.removeFromTop(120));

        area.removeFromTop(15); // Spacer

        // Read only editor
        readOnlyLabel->setBounds(area.removeFromTop(25));
        readOnlyEditor->setBounds(area.removeFromTop(60));

        area.removeFromTop(20); // Spacer

        // Buttons
        auto buttonArea = area.removeFromTop(40);
        auto buttonWidth = buttonArea.getWidth() / 4 - 10;

        selectAllButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(10);
        copyButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(10);
        pasteButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(10);
        clearButton->setBounds(buttonArea);

        area.removeFromTop(10); // Spacer

        // Hidden focused editor for testing
        focused->setBounds(area.removeFromTop(30));
    }

private:
    TextEditor* getActiveEditor()
    {
        if (singleLineEditor->hasKeyboardFocus())
            return singleLineEditor.get();

        if (multiLineEditor->hasKeyboardFocus())
            return multiLineEditor.get();

        if (readOnlyEditor->hasKeyboardFocus())
            return readOnlyEditor.get();

        return nullptr;
    }

    std::unique_ptr<TextEditor> singleLineEditor;
    std::unique_ptr<TextEditor> multiLineEditor;
    std::unique_ptr<TextEditor> readOnlyEditor;
    std::unique_ptr<TextEditor> focused;

    std::unique_ptr<TextButton> selectAllButton;
    std::unique_ptr<TextButton> copyButton;
    std::unique_ptr<TextButton> pasteButton;
    std::unique_ptr<TextButton> clearButton;

    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> singleLineLabel;
    std::unique_ptr<Label> multiLineLabel;
    std::unique_ptr<Label> readOnlyLabel;
};

} // namespace yup
