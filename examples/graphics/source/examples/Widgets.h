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

        // Image Button whose clickable area follows the logo's opaque pixels
        imageButton = std::make_unique<ImageHitTestButton>();
        imageButton->onClick = [this]
        {
            updateStatus ("Image Button clicked on an opaque pixel!");
        };
        addAndMakeVisible (imageButton.get());

        imageButtonLabel = std::make_unique<yup::Label> ("imageButtonLabel");
        imageButtonLabel->setText ("Image Button: only the logo's opaque pixels are clickable",
                                   yup::dontSendNotification);
        addAndMakeVisible (imageButtonLabel.get());

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
        slider->onValueChanged = [this] (double value)
        {
            updateStatus ("Slider value: " + yup::String (value, 1));
        };
        addAndMakeVisible (slider.get());

        // TextEditor
        textEditor = std::make_unique<yup::TextEditor> ("textEditor");
        textEditor->setText ("Type some text here...", yup::dontSendNotification);
        textEditor->setMultiLine (true);
        addAndMakeVisible (textEditor.get());

        // Progress Bar (normal mode - linked to slider)
        progressBar = std::make_unique<yup::ProgressBar> ("progressBar");
        progressBar->setProgress (0.5, yup::dontSendNotification);
        progressBar->onProgressChanged = [this] (double value)
        {
            if (value >= 0.0)
                updateStatus ("Progress: " + yup::String (value * 100.0, 0) + "%");
        };
        addAndMakeVisible (progressBar.get());

        // Progress Bar Label
        progressBarLabel = std::make_unique<yup::Label> ("progressBarLabel");
        progressBarLabel->setText ("Progress Bar (linked to slider):", yup::dontSendNotification);
        addAndMakeVisible (progressBarLabel.get());

        // Indeterminate Progress Bar
        indeterminateProgressBar = std::make_unique<yup::ProgressBar> ("indeterminateProgressBar");
        indeterminateProgressBar->setProgress (-1.0, yup::dontSendNotification);
        addAndMakeVisible (indeterminateProgressBar.get());

        // Indeterminate Progress Bar Label
        indeterminateLabel = std::make_unique<yup::Label> ("indeterminateLabel");
        indeterminateLabel->setText ("Indeterminate Progress Bar:", yup::dontSendNotification);
        addAndMakeVisible (indeterminateLabel.get());

        // Update slider to control progress bar
        slider->onValueChanged = [this] (double value)
        {
            updateStatus ("Slider value: " + yup::String (value, 1));
            progressBar->setProgress (value / 100.0, yup::dontSendNotification);
        };
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
        y += componentHeight + spacing * 2;

        // Input widgets
        auto inputWidth = (bounds.getWidth() - 3 * margin) / 2;

        comboBox->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (inputWidth), static_cast<float> (componentHeight)));
        y += componentHeight + spacing;

        textEditor->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), 100.0f));
        y += 110;

        // Slider, and the image button sharing the row the square slider leaves half empty
        auto sliderSize = static_cast<int> (inputWidth / 2);
        slider->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (sliderSize), static_cast<float> (sliderSize)));

        auto imageButtonSize = 110;
        auto imageButtonX = margin + sliderSize + spacing * 2;
        imageButton->setBounds (yup::Rectangle<float> (static_cast<float> (imageButtonX), static_cast<float> (y), static_cast<float> (imageButtonSize), static_cast<float> (imageButtonSize)));
        imageButtonLabel->setBounds (yup::Rectangle<float> (static_cast<float> (imageButtonX), static_cast<float> (y + imageButtonSize + spacing), static_cast<float> (bounds.getWidth() - imageButtonX - margin), 20.0f));

        y += sliderSize + spacing * 2;

        // Progress Bar (normal mode)
        progressBarLabel->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), 20.0f));
        y += 25;

        progressBar->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), static_cast<float> (componentHeight)));
        y += componentHeight + spacing * 2;

        // Indeterminate Progress Bar
        indeterminateLabel->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), 20.0f));
        y += 25;

        indeterminateProgressBar->setBounds (yup::Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), static_cast<float> (componentHeight)));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

    //==============================================================================
    /** A button whose clickable area is the logo's opaque pixels rather than its bounds.

        hitTest() is what decides which component a mouse event reaches, so sampling the
        image's alpha there makes the transparent parts of the PNG fall through to whatever
        is behind the button. The hover highlight goes through the same test, so dragging
        the pointer across a transparent region inside the button's bounds drops it - which
        is the easiest way to see the irregular hit area.
    */
    class ImageHitTestButton final : public yup::Button
    {
    public:
        ImageHitTestButton()
            : yup::Button ("imageButton")
        {
            logo = loadLogo();
        }

        bool hitTest (float x, float y) override
        {
            if (! logo.isValid())
                return yup::Button::hitTest (x, y);

            const auto pixel = componentToImage ({ x, y });
            const auto pixelX = static_cast<int> (pixel.getX());
            const auto pixelY = static_cast<int> (pixel.getY());

            // Letterboxing leaves bands of the component the image does not cover, and
            // getPixelColor() throws rather than clamping, so the bounds test has to come first.
            if (! yup::isPositiveAndBelow (pixelX, logo.getWidth())
                || ! yup::isPositiveAndBelow (pixelY, logo.getHeight()))
                return false;

            return logo.getPixelColor (pixelX, pixelY).getAlpha() >= minimumHitAlpha;
        }

        void paintButton (yup::Graphics& g) override
        {
            const auto imageArea = getImageArea();

            if (isButtonOver())
            {
                g.setFillColor (yup::Colors::white.withAlpha (isButtonDown() ? 0.35f : 0.15f));
                g.fillRoundedRect (imageArea, 8.0f);
            }

            if (logo.isValid())
                g.drawImage (logo, imageArea);
        }

    private:
        // The logo is anti-aliased, so a bare test for full transparency would leave a fringe
        // of barely visible pixels clickable.
        static constexpr yup::uint8 minimumHitAlpha = 128;

        yup::Rectangle<float> getImageArea() const
        {
            if (! logo.isValid())
                return getLocalBounds();

            const auto bounds = getLocalBounds();
            const auto scale = yup::jmin (bounds.getWidth() / static_cast<float> (logo.getWidth()),
                                          bounds.getHeight() / static_cast<float> (logo.getHeight()));

            return bounds.withSizeKeepingCenter (logo.getWidth() * scale, logo.getHeight() * scale);
        }

        yup::Point<float> componentToImage (const yup::Point<float>& localPoint) const
        {
            const auto imageArea = getImageArea();
            const auto scale = imageArea.getWidth() / static_cast<float> (logo.getWidth());

            return (localPoint - imageArea.getPosition()) / scale;
        }

        static yup::Image loadLogo()
        {
            const auto file = getAssetPath ("data/logo.png");

            if (! file.existsAsFile())
                return {};

            yup::ImageFormatManager formatManager;
            formatManager.registerDefaultFormats();

            auto reader = formatManager.createReaderFor (file);

            return reader != nullptr ? reader->readImage() : yup::Image();
        }

        yup::Image logo;
    };

    //==============================================================================
    // Custom ComboBox to handle selection changes
    class CustomComboBox : public yup::ComboBox
    {
    public:
        CustomComboBox (const yup::String& componentID, WidgetsDemo* parent)
            : yup::ComboBox (componentID)
            , parentWidget (parent)
        {
        }

        void selectedItemChanged() override
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
    std::unique_ptr<ImageHitTestButton> imageButton;
    std::unique_ptr<yup::Label> imageButtonLabel;
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
    std::unique_ptr<yup::ProgressBar> progressBar;
    std::unique_ptr<yup::Label> progressBarLabel;
    std::unique_ptr<yup::ProgressBar> indeterminateProgressBar;
    std::unique_ptr<yup::Label> indeterminateLabel;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WidgetsDemo)
};
