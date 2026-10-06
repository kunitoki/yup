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

//==============================================================================

class ThemesDemo : public yup::Component
{
public:
    ThemesDemo()
        : Component ("ThemesDemo")
    {
        setupPalettePanel();
        setupSliders();
        setupGallery();

        const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();
        paletteEditor.setText (toHexList (palette.getSourceColors()), yup::dontSendNotification);
        modeTabs.setSelectedTab (palette.getMode() == yup::ThemePalette::Mode::light ? "light" : "dark", yup::dontSendNotification);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (margin);

        auto left = bounds.removeFromLeft (bounds.getWidth() * 0.42f);
        bounds.removeFromLeft (margin);

        // Palette panel
        left.removeFromTop (titleHeight);
        presetCombo.setBounds (left.removeFromTop (rowHeight));
        left.removeFromTop (spacing);
        paletteEditor.setBounds (left.removeFromTop (150.0f));
        left.removeFromTop (spacing);

        auto buttons = left.removeFromTop (rowHeight);
        pasteButton.setBounds (buttons.removeFromLeft (80.0f));
        buttons.removeFromLeft (spacing);
        applyButton.setBounds (buttons.removeFromLeft (80.0f));
        buttons.removeFromLeft (spacing * 2.0f);
        modeTabs.setBounds (buttons.removeFromLeft (140.0f));

        left.removeFromTop (spacing);
        errorLabel.setBounds (left.removeFromTop (rowHeight));
        left.removeFromTop (spacing);
        swatchArea = left;

        // Sliders
        bounds.removeFromTop (titleHeight);
        auto slidersRow = bounds.removeFromTop (yup::jmin (bounds.getHeight() * 0.45f, 200.0f));
        const auto sliderWidth = slidersRow.getWidth() / 3.0f;

        for (auto* slider : { &stockSlider, &ledRingSlider, &pieSlider })
        {
            auto column = slidersRow.removeFromLeft (sliderWidth).reduced (spacing, 0.0f);
            captionFor (*slider).setBounds (column.removeFromBottom (rowHeight));
            slider->setBounds (column.withSizeKeepingCenter (yup::jmin (column.getWidth(), column.getHeight()),
                                                              yup::jmin (column.getWidth(), column.getHeight())));
        }

        // Widget gallery
        bounds.removeFromTop (spacing);
        bounds.removeFromTop (titleHeight);
        galleryArea = bounds;

        auto galleryLeft = bounds.removeFromLeft (bounds.getWidth() * 0.5f).reduced (spacing, 0.0f);
        for (yup::Component* component : { static_cast<yup::Component*> (&galleryButton),
                                           static_cast<yup::Component*> (&galleryToggle),
                                           static_cast<yup::Component*> (&galleryCombo),
                                           static_cast<yup::Component*> (&galleryEditor),
                                           static_cast<yup::Component*> (&galleryLabel) })
        {
            component->setBounds (galleryLeft.removeFromTop (rowHeight));
            galleryLeft.removeFromTop (spacing);
        }

        auto switchRow = galleryLeft.removeFromTop (rowHeight);
        gallerySwitch.setBounds (switchRow.removeFromLeft (56.0f));
        galleryLeft.removeFromTop (spacing);
        galleryProgress.setBounds (galleryLeft.removeFromTop (12.0f));
        galleryLeft.removeFromTop (spacing);
        galleryTabs.setBounds (galleryLeft.removeFromTop (rowHeight + 6.0f));

        galleryList.setBounds (bounds.reduced (spacing, 0.0f).withTrimmedBottom (spacing));
    }

    void paint (yup::Graphics& g) override
    {
        auto theme = yup::ApplicationTheme::getGlobalTheme();
        const auto& palette = theme->getPalette();
        const auto titleFont = theme->getDefaultFont().withHeight (16.0f);
        const auto& font = theme->getDefaultFont();

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::background));
        g.fillAll();

        auto bounds = getLocalBounds().reduced (margin);
        auto left = bounds.removeFromLeft (bounds.getWidth() * 0.42f);
        bounds.removeFromLeft (margin);

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::text));
        g.fillFittedText ("Palette", titleFont, left.removeFromTop (titleHeight), yup::Justification::centerLeft);
        g.fillFittedText ("Stock and custom slider styles", titleFont, bounds.removeFromTop (titleHeight), yup::Justification::centerLeft);
        g.fillFittedText ("Widgets", titleFont, galleryArea.translated (0.0f, -titleHeight).withHeight (titleHeight), yup::Justification::centerLeft);

        // Source colors
        auto swatches = swatchArea;
        g.setFillColor (palette.getColor (yup::ThemePalette::Role::textMuted));
        g.fillFittedText ("Source colors", font, swatches.removeFromTop (rowHeight), yup::Justification::centerLeft);

        const auto& sources = palette.getSourceColors();
        auto sourceRow = swatches.removeFromTop (swatchSize);
        const auto sourceWidth = yup::jmin (swatchSize * 2.0f, sourceRow.getWidth() / static_cast<float> (yup::jmax<std::size_t> (1, sources.size())));

        for (const auto& color : sources)
        {
            g.setFillColor (color);
            g.fillRoundedRect (sourceRow.removeFromLeft (sourceWidth).reduced (2.0f), 4.0f);
        }

        // Derived roles
        swatches.removeFromTop (spacing);
        g.setFillColor (palette.getColor (yup::ThemePalette::Role::textMuted));
        g.fillFittedText ("Derived roles", font, swatches.removeFromTop (rowHeight), yup::Justification::centerLeft);

        const auto columnWidth = swatches.getWidth() * 0.5f;
        for (int i = 0; i < static_cast<int> (yup::ThemePalette::Role::numRoles); ++i)
        {
            auto cell = yup::Rectangle<float> (swatches.getX() + (i % 2) * columnWidth,
                                               swatches.getY() + (i / 2) * (swatchSize + spacing),
                                               columnWidth,
                                               swatchSize);

            const auto chip = cell.removeFromLeft (swatchSize).reduced (2.0f);
            g.setFillColor (palette.getColor (static_cast<yup::ThemePalette::Role> (i)));
            g.fillRoundedRect (chip, 4.0f);
            g.setStrokeColor (palette.getColor (yup::ThemePalette::Role::outline));
            g.setStrokeWidth (1.0f);
            g.strokeRoundedRect (chip, 4.0f);

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::text));
            g.fillFittedText (roleNames[i], font, cell.withTrimmedLeft (spacing), yup::Justification::centerLeft);
        }
    }

private:
    //==============================================================================
    class GalleryListModel : public yup::ListBoxModel
    {
    public:
        int getNumRows() override { return 24; }

        yup::String getRowText (int rowIndex) override { return "List item " + yup::String (rowIndex + 1); }
    };

    //==============================================================================
    void setupPalettePanel()
    {
        presets = {
            { "YUP Default", toHexList (yup::ThemePalette().getSourceColors()) },
            { "Frosted", R"(<palette>
  <color name="Pitch Black" hex="100b00" r="16" g="11" b="0" />
  <color name="Yellow Green" hex="85cb33" r="133" g="203" b="51" />
  <color name="Frosted Mint" hex="efffc8" r="239" g="255" b="200" />
  <color name="Ash Grey" hex="a5cbc3" r="165" g="203" b="195" />
  <color name="Dark Khaki" hex="3b341f" r="59" g="52" b="31" />
</palette>)" },
            { "Dusk (coolors URL)", "https://coolors.co/1a1423-372549-774c60-b75d69-eacdc2" },
            { "Oceanic (CSS hex)", "#0b132b, #1c2541, #3a506b, #5bc0be, #f0f3f5" },
            { "Nord (plain hex)", "2e3440 3b4252 88c0d0 d8dee9 eceff4" }
        };

        presetCombo.setTextWhenNothingSelected ("Choose a preset...");
        for (int i = 0; i < static_cast<int> (presets.size()); ++i)
            presetCombo.addItem (presets[static_cast<std::size_t> (i)].first, i + 1);

        presetCombo.onSelectedItemChanged = [this]
        {
            const auto index = presetCombo.getSelectedId() - 1;
            if (index < 0 || index >= static_cast<int> (presets.size()))
                return;

            paletteEditor.setText (presets[static_cast<std::size_t> (index)].second, yup::dontSendNotification);
            applyPalette();
        };
        addAndMakeVisible (presetCombo);

        paletteEditor.setMultiLine (true);
        addAndMakeVisible (paletteEditor);

        pasteButton.setButtonText ("Paste");
        pasteButton.onClick = [this]
        {
            paletteEditor.setText (yup::SystemClipboard::getTextFromClipboard(), yup::dontSendNotification);
            applyPalette();
        };
        addAndMakeVisible (pasteButton);

        applyButton.setButtonText ("Apply");
        applyButton.onClick = [this]
        {
            applyPalette();
        };
        addAndMakeVisible (applyButton);

        modeTabs.setLayout (yup::TabBar::Layout::fill);
        modeTabs.addTab ("dark", "Dark");
        modeTabs.addTab ("light", "Light");
        modeTabs.onSelectionChanged = [this] (const yup::Identifier&)
        {
            applyPalette();
        };
        addAndMakeVisible (modeTabs);

        errorLabel.setColor (yup::Label::Style::textFillColorId, yup::Colors::orangered);
        addAndMakeVisible (errorLabel);
    }

    void setupSliders()
    {
        ledRingSlider.setStyle (yup::ComponentStyle::createStyle<yup::Slider> (paintLedRingSlider));
        pieSlider.setStyle (yup::ComponentStyle::createStyle<yup::Slider> (paintPieSlider));

        for (auto* slider : { &stockSlider, &ledRingSlider, &pieSlider })
        {
            slider->setRange (0.0, 1.0);
            slider->setValue (0.65);
            addAndMakeVisible (*slider);
        }

        stockCaption.setText ("Stock", yup::dontSendNotification);
        ledRingCaption.setText ("Segmented LED ring", yup::dontSendNotification);
        pieCaption.setText ("Filled pie", yup::dontSendNotification);

        for (auto* caption : { &stockCaption, &ledRingCaption, &pieCaption })
            addAndMakeVisible (*caption);
    }

    void setupGallery()
    {
        galleryButton.setButtonText ("Text button");
        galleryToggle.setButtonText ("Toggle button");
        galleryToggle.setToggleState (true, yup::dontSendNotification);
        gallerySwitch.setToggleState (true, yup::dontSendNotification);

        galleryCombo.addItem ("Combo box", 1);
        galleryCombo.addItem ("Second item", 2);
        galleryCombo.addItem ("Third item", 3);
        galleryCombo.setSelectedId (1, yup::dontSendNotification);

        galleryEditor.setText ("Text editor", yup::dontSendNotification);
        galleryLabel.setText ("Label text at the theme height", yup::dontSendNotification);
        galleryProgress.setProgress (0.6, yup::dontSendNotification);

        galleryList.setModel (&galleryListModel);
        galleryList.setRowSize (28.0f);
        galleryList.selectRow (2, false, yup::dontSendNotification);

        for (const auto* name : { "Share", "Privacy", "Publishing", "Domain" })
            galleryTabs.addTab (yup::String (name).toLowerCase(), name);

        for (yup::Component* component : { static_cast<yup::Component*> (&galleryButton),
                                           static_cast<yup::Component*> (&galleryToggle),
                                           static_cast<yup::Component*> (&gallerySwitch),
                                           static_cast<yup::Component*> (&galleryCombo),
                                           static_cast<yup::Component*> (&galleryEditor),
                                           static_cast<yup::Component*> (&galleryLabel),
                                           static_cast<yup::Component*> (&galleryProgress),
                                           static_cast<yup::Component*> (&galleryTabs),
                                           static_cast<yup::Component*> (&galleryList) })
        {
            addAndMakeVisible (component);
        }
    }

    //==============================================================================
    void applyPalette()
    {
        const auto mode = modeTabs.getSelectedTabId() == yup::Identifier ("light") ? yup::ThemePalette::Mode::light : yup::ThemePalette::Mode::dark;
        auto result = yup::ThemePalette::fromString (paletteEditor.getText(), mode);

        if (result.failed())
        {
            errorLabel.setText (result.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        errorLabel.setText ({}, yup::dontSendNotification);
        yup::ApplicationTheme::getGlobalTheme()->setPalette (result.getReference());

        if (auto* topLevel = getTopLevelComponent())
            topLevel->repaint();
        else
            repaint();
    }

    yup::Label& captionFor (const yup::Slider& slider)
    {
        if (&slider == &ledRingSlider)
            return ledRingCaption;

        return &slider == &pieSlider ? pieCaption : stockCaption;
    }

    static yup::String toHexList (const std::vector<yup::Color>& colors)
    {
        yup::StringArray hexColors;
        for (const auto& color : colors)
            hexColors.add (color.toString().dropLastCharacters (2));

        return hexColors.joinIntoString (", ");
    }

    //==============================================================================
    static void paintLedRingSlider (yup::Graphics& g, const yup::ApplicationTheme& theme, const yup::Slider& slider)
    {
        using Role = yup::ThemePalette::Role;

        const auto& palette = theme.getPalette();
        const auto bounds = slider.getLocalBounds().reduced (4.0f);
        const auto size = yup::jmin (bounds.getWidth(), bounds.getHeight());
        const auto center = bounds.getCenter();
        const auto value = static_cast<float> (slider.getValueNormalised());
        const auto litSegments = yup::roundToInt (value * numLedSegments);

        g.setStrokeCap (yup::StrokeCap::Round);
        g.setStrokeWidth (size * 0.05f);

        for (int i = 0; i < numLedSegments; ++i)
        {
            const auto angle = rotaryStartAngle + rotarySweep * (static_cast<float> (i) + 0.5f) / static_cast<float> (numLedSegments);
            const auto isLit = i < litSegments;

            g.setStrokeColor (isLit ? palette.getColor (Role::accent) : palette.getColor (Role::surfaceRaised));
            g.strokeLine (center.getPointOnCircumference (size * 0.34f, angle),
                          center.getPointOnCircumference (size * 0.46f, angle));
        }

        g.setFillColor (slider.isMouseOver() ? palette.getColor (Role::accent) : palette.getColor (Role::text));
        g.fillFittedText (yup::String (yup::roundToInt (value * 100.0f)),
                          theme.getDefaultFont().withHeight (size * 0.2f),
                          bounds.withSizeKeepingCenter (size * 0.5f, size * 0.3f),
                          yup::Justification::center);
    }

    static void paintPieSlider (yup::Graphics& g, const yup::ApplicationTheme& theme, const yup::Slider& slider)
    {
        using Role = yup::ThemePalette::Role;

        const auto& palette = theme.getPalette();
        const auto bounds = slider.getLocalBounds().reduced (4.0f);
        const auto size = yup::jmin (bounds.getWidth(), bounds.getHeight());
        const auto center = bounds.getCenter();
        const auto radius = size * 0.45f;
        const auto valueAngle = rotaryStartAngle + rotarySweep * static_cast<float> (slider.getValueNormalised());
        const auto accent = palette.getColor (Role::accent);

        g.setFillColor (palette.getColor (Role::surface));
        g.fillEllipse (bounds.withSizeKeepingCenter (radius * 2.0f, radius * 2.0f));

        yup::Path pie;
        pie.moveTo (center);
        pie.addCenteredArc (center, radius, radius, 0.0f, rotaryStartAngle, valueAngle, false);
        pie.close();

        g.setFillColor (accent.withAlpha (slider.isMouseOver() ? 0.55f : 0.4f));
        g.fillPath (pie);

        g.setStrokeCap (yup::StrokeCap::Round);
        g.setStrokeColor (accent);
        g.setStrokeWidth (2.0f);
        g.strokeLine (center, center.getPointOnCircumference (radius, valueAngle));

        g.setStrokeColor (palette.getColor (Role::outline));
        g.setStrokeWidth (1.0f);
        g.strokeEllipse (bounds.withSizeKeepingCenter (radius * 2.0f, radius * 2.0f));
    }

    //==============================================================================
    static constexpr float margin = 10.0f;
    static constexpr float spacing = 6.0f;
    static constexpr float rowHeight = 30.0f;
    static constexpr float titleHeight = 28.0f;
    static constexpr float swatchSize = 26.0f;
    static constexpr int numLedSegments = 21;
    static constexpr float rotaryStartAngle = yup::degreesToRadians (135.0f);
    static constexpr float rotarySweep = yup::degreesToRadians (270.0f);

    static constexpr const char* roleNames[] = { "background", "surface", "surfaceRaised", "outline",
                                                 "text", "textMuted", "accent", "onAccent" };

    std::vector<std::pair<yup::String, yup::String>> presets;

    yup::ComboBox presetCombo;
    yup::TextEditor paletteEditor;
    yup::TextButton pasteButton;
    yup::TextButton applyButton;
    yup::TabBar modeTabs;
    yup::Label errorLabel;
    yup::Rectangle<float> swatchArea;

    yup::Slider stockSlider { yup::Slider::RotaryVerticalDrag };
    yup::Slider ledRingSlider { yup::Slider::RotaryVerticalDrag };
    yup::Slider pieSlider { yup::Slider::RotaryVerticalDrag };
    yup::Label stockCaption;
    yup::Label ledRingCaption;
    yup::Label pieCaption;

    GalleryListModel galleryListModel;
    yup::Rectangle<float> galleryArea;
    yup::TextButton galleryButton;
    yup::ToggleButton galleryToggle;
    yup::SwitchButton gallerySwitch;
    yup::ComboBox galleryCombo;
    yup::TextEditor galleryEditor;
    yup::Label galleryLabel;
    yup::ProgressBar galleryProgress;
    yup::TabBar galleryTabs;
    yup::ListBox galleryList;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThemesDemo)
};
