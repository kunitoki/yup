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
class WidgetsDemo : public Component
{
public:
    WidgetsDemo()
    {
        auto theme = ApplicationTheme::getGlobalTheme();
        exampleFont = theme->getDefaultFont();

        setupWidgets();
        setupLayout();
    }

private:
    void setupWidgets()
    {
        // Text Button (uses componentID as text)
        textButton = std::make_unique<TextButton> ("Text Button");
        textButton->onClick = [this]
        {
            updateStatus ("Text Button clicked!");
        };
        addAndMakeVisible (textButton.get());

        // Toggle Button
        toggleButton = std::make_unique<ToggleButton> ("toggleButton");
        toggleButton->setButtonText ("Toggle Button");
        toggleButton->onClick = [this] { updateStatus ("Toggle Button: " + String (toggleButton->getToggleState() ? "ON" : "OFF")); };
        addAndMakeVisible (toggleButton.get());

        // Switch Button
        switchButton = std::make_unique<SwitchButton> ("switchButton");
        switchButton->onClick = [this] { updateStatus ("Switch Button: " + String (switchButton->getToggleState() ? "ON" : "OFF")); };
        addAndMakeVisible (switchButton.get());

        // Image Button
        //imageButton = std::make_unique<ImageButton> ("imageButton");
        // Note: You would set images here with setImages()
        //addAndMakeVisible (imageButton.get());

        // Labels
        titleLabel = std::make_unique<Label> ("titleLabel");
        titleLabel->setText ("YUP Widget Examples", dontSendNotification);
        titleLabel->setFont (exampleFont);
        addAndMakeVisible (titleLabel.get());

        statusLabel = std::make_unique<Label> ("statusLabel");
        statusLabel->setText ("Click widgets to see status updates...", dontSendNotification);
        addAndMakeVisible (statusLabel.get());

        // ComboBox with custom callback
        comboBox = std::make_unique<CustomComboBox> ("comboBox", this);
        comboBox->addItem ("Option 1", 1);
        comboBox->addItem ("Option 2", 2);
        comboBox->addItem ("Option 3", 3);
        comboBox->setSelectedId (1);
        addAndMakeVisible (comboBox.get());

        // Viewport with content
        /*
        viewport = std::make_unique<Viewport> ("viewport");
        viewportContent = std::make_unique<Component> ("viewportContent");
        viewportContent->setSize (Size<float> (800.0f, 600.0f)); // Larger than viewport

        contentLabel = std::make_unique<Label> ("contentLabel");
        contentLabel->setText ("This is scrollable content\nYou can scroll to see more text...", dontSendNotification);
        contentLabel->setBounds (Rectangle<float> (20.0f, 20.0f, 360.0f, 200.0f));
        viewportContent->addAndMakeVisible (contentLabel.get());

        viewport->setViewedComponent (viewportContent.release(), false);
        addAndMakeVisible (viewport.get());
        */

        // Slider
        slider = std::make_unique<Slider> ("slider");
        slider->setRange (Range<float> (0.0f, 100.0f));
        slider->setValue (50.0);
        slider->onValueChanged = [this] (float value)
        {
            updateStatus ("Slider value: " + String (value, 1));
        };
        addAndMakeVisible (slider.get());

        // TextEditor
        textEditor = std::make_unique<TextEditor> ("textEditor");
        textEditor->setText ("Type some text here...", dontSendNotification);
        textEditor->setMultiLine (true);
        addAndMakeVisible (textEditor.get());
    }

    void setupLayout()
    {
        // Layout will be handled in resized()
    }

    void updateStatus (const String& message)
    {
        statusLabel->setText (message, dontSendNotification);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto margin = 20;
        auto componentHeight = 30;
        auto spacing = 10;

        int y = margin;

        // Title
        titleLabel->setBounds (Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), 40.0f));
        y += 50;

        // Status
        statusLabel->setBounds (Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), static_cast<float> (componentHeight)));
        y += componentHeight + spacing * 2;

        // Buttons row
        auto buttonWidth = 120;
        textButton->setBounds (Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (buttonWidth), static_cast<float> (componentHeight)));
        toggleButton->setBounds (Rectangle<float> (static_cast<float>(margin + buttonWidth + spacing), static_cast<float>(y), static_cast<float>(buttonWidth), static_cast<float>(componentHeight)));
        switchButton->setBounds (Rectangle<float> (static_cast<float>(margin + 2 * (buttonWidth + spacing)), static_cast<float>(y), 80.0f, static_cast<float>(componentHeight)));
        //imageButton->setBounds (Rectangle<float> (static_cast<float>(margin + 3 * (buttonWidth + spacing)), static_cast<float>(y), static_cast<float>(buttonWidth), static_cast<float>(componentHeight)));
        y += componentHeight + spacing * 2;

        // Input widgets
        auto inputWidth = (bounds.getWidth() - 3 * margin) / 2;

        comboBox->setBounds (Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (inputWidth), static_cast<float> (componentHeight)));
        y += componentHeight + spacing;

        textEditor->setBounds (Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), 100.0f));
        y += 110;

        // Viewport
        //viewport->setBounds (Rectangle<float> (static_cast<float>(margin), static_cast<float>(y), static_cast<float>(inputWidth), 150.0f));

        // Slider
        slider->setBounds (Rectangle<float> (static_cast<float> (margin + inputWidth + spacing), static_cast<float> (y), static_cast<float> (inputWidth), static_cast<float> (componentHeight)));
    }

    // Custom ComboBox to handle selection changes
    class CustomComboBox : public ComboBox
    {
    public:
        CustomComboBox (const String& componentID, WidgetsDemo* parent)
            : ComboBox (componentID)
            , parentWidget (parent)
        {
        }

        void comboBoxChanged() override
        {
            if (parentWidget)
                parentWidget->updateStatus ("ComboBox selected: " + getItemText (getSelectedItemIndex()));
        }

    private:
        WidgetsDemo* parentWidget;
    };

private:
    Font exampleFont;
    std::unique_ptr<TextButton> textButton;
    std::unique_ptr<ToggleButton> toggleButton;
    std::unique_ptr<SwitchButton> switchButton;
    //std::unique_ptr<ImageButton> imageButton;
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> statusLabel;
    std::unique_ptr<CustomComboBox> comboBox;
    /*
    std::unique_ptr<Viewport> viewport;
    std::unique_ptr<Component> viewportContent;
    */
    std::unique_ptr<Label> contentLabel;
    std::unique_ptr<Slider> slider;
    std::unique_ptr<TextEditor> textEditor;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WidgetsDemo)
};

} // namespace yup
