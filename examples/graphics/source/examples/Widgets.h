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

#pragma once

//==============================================================================

class WidgetsDemo : public yup::Component
{
public:
    WidgetsDemo()
    {
        auto theme = yup::ApplicationTheme::getGlobalTheme();
        exampleFont = theme->getDefaultFont();

        setupWidgets();
        setupLayout();
    }

private:
    void setupWidgets()
    {
        // Text Button (uses componentID as text)
        textButton = std::make_unique<yup::TextButton> ("Text Button");
        textButton->onClick = [this]
        {
            updateStatus ("Text Button clicked!");
        };
        addAndMakeVisible (textButton.get());

        // Toggle Button
        toggleButton = std::make_unique<yup::ToggleButton> ("toggleButton");
        toggleButton->setButtonText ("Toggle Button");
        toggleButton->onClick = [this]
        {
            updateStatus ("Toggle Button: " + yup::String (toggleButton->getToggleState() ? "ON" : "OFF"));
        };
        addAndMakeVisible (toggleButton.get());

        // Switch Button
        switchButton = std::make_unique<yup::SwitchButton> ("switchButton");
        switchButton->onClick = [this]
        {
            updateStatus ("Switch Button: " + yup::String (switchButton->getToggleState() ? "ON" : "OFF"));
        };
        addAndMakeVisible (switchButton.get());

        // Image Button
        //imageButton = std::make_unique<ImageButton> ("imageButton");
        // Note: You would set images here with setImages()
        //addAndMakeVisible (imageButton.get());

        // Labels
        titleLabel = std::make_unique<yup::Label> ("titleLabel");
        titleLabel->setText ("YUP Widget Examples", yup::dontSendNotification);
        titleLabel->setFont (exampleFont);
        addAndMakeVisible (titleLabel.get());

        statusLabel = std::make_unique<yup::Label> ("statusLabel");
        statusLabel->setText ("Click widgets to see status updates...", yup::dontSendNotification);
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
        viewport = std::make_unique<yup::Viewport> ("viewport");
        viewportContent = std::make_unique<yup::Component> ("viewportContent");
        viewportContent->setSize (yup::Size<float> (800.0f, 600.0f)); // Larger than viewport

        contentLabel = std::make_unique<yup::Label> ("contentLabel");
        contentLabel->setText ("This is scrollable content\nYou can scroll to see more text...", yup::dontSendNotification);
        contentLabel->setBounds (yup::Rectangle<float> (20.0f, 20.0f, 360.0f, 200.0f));
        viewportContent->addAndMakeVisible (contentLabel.get());

        viewport->setViewedComponent (viewportContent.release(), false);
        addAndMakeVisible (viewport.get());
        */

        // Slider
        slider = std::make_unique<yup::Slider> (yup::Slider::Rotary, "slider");
        slider->setRange (yup::Range<double> (0.0, 100.0));
        slider->setValue (50.0);
        slider->onValueChanged = [this] (float value)
        {
            updateStatus ("Slider value: " + yup::String (value, 1));
        };
        addAndMakeVisible (slider.get());

        // TextEditor
        textEditor = std::make_unique<yup::TextEditor> ("textEditor");
        textEditor->setText ("Type some text here...", yup::dontSendNotification);
        textEditor->setMultiLine (true);
        addAndMakeVisible (textEditor.get());
    }

    void setupLayout()
    {
        // Layout will be handled in resized()
    }

    void updateStatus (const yup::String& message)
    {
        statusLabel->setText (message, yup::dontSendNotification);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto margin = 20;
        auto componentHeight = 30;
        auto spacing = 10;

        int y = margin;

        // Title
        titleLabel->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), 40.0f));
        y += 50;

        // Status
        statusLabel->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), static_cast<float> (componentHeight)));
        y += componentHeight + spacing * 2;

        // Buttons row
        auto buttonWidth = 120;
        textButton->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (buttonWidth), static_cast<float> (componentHeight)));
        toggleButton->setBounds (yup::Rectangle<float> (static_cast<float> (margin + buttonWidth + spacing), static_cast<float> (y), static_cast<float> (buttonWidth), static_cast<float> (componentHeight)));
        switchButton->setBounds (yup::Rectangle<float> (static_cast<float> (margin + 2 * (buttonWidth + spacing)), static_cast<float> (y), 80.0f, static_cast<float> (componentHeight)));
        //imageButton->setBounds (yup::Rectangle<float> (static_cast<float>(margin + 3 * (buttonWidth + spacing)), static_cast<float>(y), static_cast<float>(buttonWidth), static_cast<float>(componentHeight)));
        y += componentHeight + spacing * 2;

        // Input widgets
        auto inputWidth = (bounds.getWidth() - 3 * margin) / 2;

        comboBox->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (inputWidth), static_cast<float> (componentHeight)));
        y += componentHeight + spacing;

        textEditor->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), 100.0f));
        y += 110;

        // Viewport
        //viewport->setBounds (yup::Rectangle<float> (static_cast<float>(margin), static_cast<float>(y), static_cast<float>(inputWidth), 150.0f));

        // Slider
        slider->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (inputWidth / 2), static_cast<float> (inputWidth / 2)));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

    // Custom ComboBox to handle selection changes
    class CustomComboBox : public yup::ComboBox
    {
    public:
        CustomComboBox (const yup::String& componentID, WidgetsDemo* parent)
            : yup::ComboBox (componentID)
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
    yup::Font exampleFont;
    std::unique_ptr<yup::TextButton> textButton;
    std::unique_ptr<yup::ToggleButton> toggleButton;
    std::unique_ptr<yup::SwitchButton> switchButton;
    //std::unique_ptr<yup::ImageButton> imageButton;
    std::unique_ptr<yup::Label> titleLabel;
    std::unique_ptr<yup::Label> statusLabel;
    std::unique_ptr<CustomComboBox> comboBox;
    /*
    std::unique_ptr<yup::Viewport> viewport;
    std::unique_ptr<yup::Component> viewportContent;
    */
    std::unique_ptr<yup::Label> contentLabel;
    std::unique_ptr<yup::Slider> slider;
    std::unique_ptr<yup::TextEditor> textEditor;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WidgetsDemo)
};
