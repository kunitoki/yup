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
/** The font shown by the previews, with the drawing options shared by all of them. */
struct FontPreviewSettings
{
    yup::Font font;
    float size = 32.0f;
    float strokeWidth = 0.0f;
    float feather = 0.0f;
};

//==============================================================================
/** Paints an editable sample text with the preview font, filled and optionally stroked. */
class FontSamplePage final : public yup::Component
{
public:
    explicit FontSamplePage (const FontPreviewSettings& settingsToUse)
        : yup::Component ("fontSamplePage")
        , settings (settingsToUse)
    {
        setOpaque (false);

        sampleEditor.setMultiLine (true);
        sampleEditor.setText ("The quick brown fox jumps over the lazy dog. 0123456789\n"
                              "Sphinx of black quartz, judge my vow! fi fl ffi 1/2 3/4",
                              yup::dontSendNotification);
        sampleEditor.onTextChange = [this]
        {
            updateText();
        };
        addAndMakeVisible (sampleEditor);
    }

    void updateText()
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (textBounds.getSize());
        modifier.setHorizontalAlign (yup::StyledText::left);
        modifier.setVerticalAlign (yup::StyledText::top);
        modifier.setOverflow (yup::StyledText::visible);
        modifier.setWrap (yup::StyledText::wrap);
        modifier.clear();
        modifier.appendText (sampleEditor.getText(), settings.font.withHeight (settings.size));

        repaint();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (12.0f);
        sampleEditor.setBounds (bounds.removeFromTop (56.0f));
        textBounds = bounds.withTrimmedTop (12.0f);

        updateText();
    }

    void paint (yup::Graphics& g) override
    {
        const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

        if (settings.strokeWidth > 0.0f)
        {
            g.setStrokeColor (palette.getColor (yup::ThemePalette::Role::accent));
            g.setStrokeWidth (settings.strokeWidth);
            g.setStrokeCap (yup::StrokeCap::Round);
            g.setStrokeJoin (yup::StrokeJoin::Round);
            g.setFeather (settings.feather);
            g.strokeFittedText (styledText, textBounds);
        }

        g.setFeather (0.0f);
        g.setFillColor (palette.getColor (yup::ThemePalette::Role::text));
        g.fillFittedText (styledText, textBounds);
    }

private:
    const FontPreviewSettings& settings;
    yup::TextEditor sampleEditor { "sampleEditor" };
    yup::StyledText styledText;
    yup::Rectangle<float> textBounds;
};

//==============================================================================
/** Paints a pangram at increasing sizes. */
class FontWaterfallPage final : public yup::Component
{
public:
    explicit FontWaterfallPage (const FontPreviewSettings& settingsToUse)
        : yup::Component ("fontWaterfallPage")
        , settings (settingsToUse)
    {
        setOpaque (false);
    }

    void paint (yup::Graphics& g) override
    {
        const auto theme = yup::ApplicationTheme::getGlobalTheme();
        const auto& palette = theme->getPalette();
        const auto captionFont = theme->getDefaultFont().withHeight (11.0f);

        auto bounds = getLocalBounds().reduced (12.0f);

        const auto state = g.saveState();
        g.setClipPath (bounds);

        for (const auto size : { 10.0f, 12.0f, 14.0f, 18.0f, 24.0f, 32.0f, 48.0f, 64.0f, 96.0f })
        {
            auto row = bounds.removeFromTop (size * 1.35f + 4.0f);
            if (row.getBottom() > getHeight())
                break;

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::textMuted));
            g.fillFittedText (yup::String (yup::roundToInt (size)), captionFont, row.removeFromLeft (32.0f), yup::Justification::centerLeft);

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::text));

            yup::StyledText line;
            {
                auto modifier = line.startUpdate();
                modifier.setMaxSize (row.getSize());
                modifier.setWrap (yup::StyledText::noWrap);
                modifier.setVerticalAlign (yup::StyledText::middle);
                modifier.appendText ("The quick brown fox jumps over the lazy dog", settings.font.withHeight (size));
            }

            g.fillFittedText (line, row);
        }
    }

private:
    const FontPreviewSettings& settings;
};

//==============================================================================
/** Shows every character of the preview font in a scrolling grid. */
class FontGlyphsPage final : public yup::Component
{
public:
    explicit FontGlyphsPage (const FontPreviewSettings& settingsToUse)
        : yup::Component ("fontGlyphsPage")
        , grid (settingsToUse, viewport)
    {
        setOpaque (false);

        viewport.setViewedComponent (&grid);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);
    }

    void setCodepoints (std::vector<yup::yup_wchar> newCodepoints)
    {
        grid.codepoints = std::move (newCodepoints);
        viewport.setViewPosition ({});
        resized();
    }

    void resized() override
    {
        viewport.setBounds (getLocalBounds().reduced (12.0f, 8.0f));

        const auto width = yup::jmax (GlyphGrid::cellSize, viewport.getWidth() - 14.0f);
        const auto rows = (int (grid.codepoints.size()) + grid.getNumColumns (width) - 1) / grid.getNumColumns (width);
        grid.setSize (width, (float) rows * GlyphGrid::cellSize);
        grid.repaint();
    }

private:
    struct GlyphGrid final : public yup::Component
    {
        static constexpr float cellSize = 64.0f;

        GlyphGrid (const FontPreviewSettings& settingsToUse, const yup::Viewport& viewportToUse)
            : yup::Component ("glyphGrid")
            , settings (settingsToUse)
            , viewport (viewportToUse)
        {
            setOpaque (false);
        }

        int getNumColumns (float width) const
        {
            return yup::jmax (1, int (width / cellSize));
        }

        void paint (yup::Graphics& g) override
        {
            const auto theme = yup::ApplicationTheme::getGlobalTheme();
            const auto& palette = theme->getPalette();
            const auto glyphFont = settings.font.withHeight (cellSize * 0.45f);
            const auto codeFont = theme->getDefaultFont().withHeight (9.0f);
            const auto columns = getNumColumns (getWidth());

            // Only the rows in view are painted: fonts can have tens of thousands of characters
            const auto visible = viewport.getVisibleArea();
            const auto firstRow = yup::jmax (0, int (visible.getY() / cellSize));
            const auto lastRow = int ((visible.getBottom()) / cellSize) + 1;

            for (int row = firstRow; row <= lastRow; ++row)
            {
                for (int column = 0; column < columns; ++column)
                {
                    const auto index = (size_t) (row * columns + column);
                    if (index >= codepoints.size())
                        return;

                    const auto cell = yup::Rectangle<float> (column * cellSize, row * cellSize, cellSize, cellSize).reduced (2.0f);

                    g.setFillColor (palette.getColor (yup::ThemePalette::Role::surface));
                    g.fillRoundedRect (cell, 4.0f);

                    auto area = cell;
                    const auto codeArea = area.removeFromBottom (14.0f);

                    g.setFillColor (palette.getColor (yup::ThemePalette::Role::text));
                    g.fillFittedText (yup::String::charToString (codepoints[index]), glyphFont, area, yup::Justification::center);

                    g.setFillColor (palette.getColor (yup::ThemePalette::Role::textMuted));
                    g.fillFittedText (yup::String::toHexString ((int) codepoints[index]).toUpperCase().paddedLeft ('0', 4), codeFont, codeArea, yup::Justification::center);
                }
            }
        }

        const FontPreviewSettings& settings;
        const yup::Viewport& viewport;
        std::vector<yup::yup_wchar> codepoints;
    };

    yup::Viewport viewport { "glyphsViewport" };
    GlyphGrid grid;
};

//==============================================================================
/**
    Browses fonts: the theme fonts and the fonts installed on the system.

    - The family list filters as you type, and the style box lists the styles of the
      selected family, loaded with Font::loadSystemFont().
    - Variable fonts get one knob per axis, and the common OpenType features the font has
      can be toggled.
    - The previews show an editable sample, a waterfall of sizes and every character.
*/
class FontsDemo final
    : public yup::Component
    , public LoadingDemo
{
public:
    FontsDemo()
        : yup::Component ("FontsDemo")
    {
        setTitle ("Fonts");

        const auto theme = yup::ApplicationTheme::getGlobalTheme();
        allFamilies.push_back ({ "Theme default", theme->getDefaultFont() });
        allFamilies.push_back ({ "Theme monospace", theme->getDefaultMonospaceFont() });

        searchEditor.onTextChange = [this]
        {
            updateFilter();
        };
        addAndMakeVisible (searchEditor);

        familyModel.owner = this;
        familyList.setModel (&familyModel);
        familyList.setRowSize (28.0f);
        addAndMakeVisible (familyList);

        styleCombo.onSelectedItemChanged = [this]
        {
            loadSelectedStyle();
        };
        addAndMakeVisible (styleCombo);

        addControl (sizeSlider, sizeLabel, 6.0, 120.0, settings.size);
        addControl (strokeSlider, strokeLabel, 0.0, 8.0, settings.strokeWidth);
        addControl (featherSlider, featherLabel, 0.0, 10.0, settings.feather);

        addAndMakeVisible (infoLabel);

        previewTabs.addTab ("sample", "Sample", samplePage);
        previewTabs.addTab ("waterfall", "Waterfall", waterfallPage);
        previewTabs.addTab ("glyphs", "Glyphs", glyphsPage);
        previewTabs.getTabBar().setVariant (yup::TabBar::Variant::underline);
        addAndMakeVisible (previewTabs);

        // Scanning the system fonts can take a moment, so it runs behind the loading bar
        addLoadingStep ([this]
        {
            for (const auto& family : yup::Font::findSystemFontFamilies())
                allFamilies.push_back ({ family, {} });

            updateFilter();
            familyList.selectRow (0);
        });
    }

    void paint (yup::Graphics& g) override
    {
        const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::background));
        g.fillAll();

        pumpLoading();

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::surface));
        g.fillRect (sidebarBounds);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        sidebarBounds = bounds.removeFromLeft (yup::jmin (240.0f, bounds.getWidth() * 0.35f));
        auto sidebar = sidebarBounds.reduced (8.0f);
        searchEditor.setBounds (sidebar.removeFromTop (28.0f));
        familyList.setBounds (sidebar.withTrimmedTop (8.0f));

        bounds = bounds.reduced (12.0f, 8.0f);

        auto topRow = bounds.removeFromTop (28.0f);
        styleCombo.setBounds (topRow.removeFromLeft (yup::jmin (200.0f, topRow.getWidth() * 0.4f)));
        infoLabel.setBounds (topRow.withTrimmedLeft (12.0f));

        bounds.removeFromTop (8.0f);

        auto sliderRow = bounds.removeFromTop (40.0f);
        const auto sliderWidth = sliderRow.getWidth() / 3.0f;
        for (auto [slider, label] : { std::pair (&sizeSlider, &sizeLabel), std::pair (&strokeSlider, &strokeLabel), std::pair (&featherSlider, &featherLabel) })
        {
            auto cell = sliderRow.removeFromLeft (sliderWidth).reduced (4.0f, 0.0f);
            label->setBounds (cell.removeFromTop (16.0f));
            slider->setBounds (cell);
        }

        if (! axisSliders.isEmpty())
        {
            auto axisRow = bounds.removeFromTop (76.0f);
            const auto axisWidth = yup::jmin (80.0f, axisRow.getWidth() / (float) axisSliders.size());

            for (int index = 0; index < axisSliders.size(); ++index)
            {
                auto cell = axisRow.removeFromLeft (axisWidth);
                axisLabels[index]->setBounds (cell.removeFromBottom (16.0f));

                const auto knobSize = yup::jmin (cell.getWidth(), cell.getHeight());
                axisSliders[index]->setBounds (cell.withSizeKeepingCenter (knobSize, knobSize));
            }
        }

        if (! featureToggles.isEmpty())
        {
            bounds.removeFromTop (4.0f);

            constexpr float toggleWidth = 64.0f;
            const auto perRow = yup::jmax (1, int (bounds.getWidth() / toggleWidth));
            auto featureRow = bounds.removeFromTop (24.0f);

            for (int index = 0; index < featureToggles.size(); ++index)
            {
                if (index > 0 && index % perRow == 0)
                    featureRow = bounds.removeFromTop (24.0f);

                featureToggles[index]->setBounds (featureRow.removeFromLeft (toggleWidth));
            }
        }

        previewTabs.setBounds (bounds.withTrimmedTop (8.0f));
    }

private:
    //==============================================================================
    struct FamilyEntry
    {
        yup::String name;
        yup::Font themeFont; // Set for the theme fonts, empty for system families
    };

    struct FamilyListModel final : public yup::ListBoxModel
    {
        int getNumRows() override
        {
            return (int) owner->visibleFamilies.size();
        }

        yup::String getRowText (int rowIndex) override
        {
            return yup::isPositiveAndBelow (rowIndex, getNumRows()) ? owner->visibleFamilies[(size_t) rowIndex]->name : yup::String();
        }

        void selectedRowsChanged (const yup::Array<int>& selectedRows) override
        {
            if (! selectedRows.isEmpty())
                owner->selectFamily (selectedRows[0]);
        }

        FontsDemo* owner = nullptr;
    };

    /** The common OpenType features, with their default state. */
    struct KnownFeature
    {
        const char* tag;
        const char* name;
        bool onByDefault;
    };

    static constexpr KnownFeature knownFeatures[] = {
        { "kern", "Kerning", true },
        { "liga", "Ligatures", true },
        { "calt", "Contextual", true },
        { "dlig", "Discretionary", false },
        { "smcp", "Small caps", false },
        { "onum", "Old style", false },
        { "lnum", "Lining", false },
        { "tnum", "Tabular", false },
        { "frac", "Fractions", false },
        { "zero", "Slashed 0", false },
        { "case", "Case", false },
        { "salt", "Alternates", false },
        { "ss01", "Set 1", false },
        { "ss02", "Set 2", false },
        { "ss03", "Set 3", false },
    };

    //==============================================================================
    void addControl (yup::Slider& slider, yup::Label& label, double minValue, double maxValue, float& valueToSet)
    {
        slider.setRange (minValue, maxValue);
        slider.setDefaultValue (valueToSet);
        slider.setValue (valueToSet);
        slider.onValueChanged = [this, &slider, &label, &valueToSet] (double value)
        {
            valueToSet = (float) value;
            label.setText (slider.getComponentID() + ": " + yup::String (value, 1), yup::dontSendNotification);
            updatePreviews();
        };
        addAndMakeVisible (slider);

        label.setText (slider.getComponentID() + ": " + yup::String (valueToSet, 1), yup::dontSendNotification);
        addAndMakeVisible (label);
    }

    void updateFilter()
    {
        const auto search = searchEditor.getText().trim();
        const auto* selected = yup::isPositiveAndBelow (familyList.getSelectedRow(), (int) visibleFamilies.size())
                                 ? visibleFamilies[(size_t) familyList.getSelectedRow()]
                                 : nullptr;

        visibleFamilies.clear();
        for (const auto& entry : allFamilies)
        {
            if (search.isEmpty() || entry.name.containsIgnoreCase (search))
                visibleFamilies.push_back (&entry);
        }

        familyList.deselectAllRows (yup::dontSendNotification);
        familyList.updateContent();

        const auto found = std::find (visibleFamilies.begin(), visibleFamilies.end(), selected);
        if (found != visibleFamilies.end())
            familyList.selectRow ((int) std::distance (visibleFamilies.begin(), found), true, yup::dontSendNotification);
    }

    void selectFamily (int row)
    {
        if (! yup::isPositiveAndBelow (row, (int) visibleFamilies.size()))
            return;

        const auto& entry = *visibleFamilies[(size_t) row];
        selectedFamily = entry.name;
        selectedIsThemeFont = ! entry.themeFont.isEmpty();

        styleCombo.clear();

        if (selectedIsThemeFont)
        {
            styleCombo.addItem (entry.themeFont.getStyleName(), 1);
            styleCombo.setSelectedItemIndex (0, yup::dontSendNotification);
            styleCombo.setEnabled (false);
            setFont (entry.themeFont);
            return;
        }

        const auto styles = yup::Font::findSystemFontStyles (entry.name);
        styleCombo.addItemList (styles, 1);
        styleCombo.setSelectedItemIndex (yup::jmax (0, styles.indexOf ("Regular", true)), yup::dontSendNotification);
        styleCombo.setEnabled (styles.size() > 1);
        loadSelectedStyle();
    }

    void loadSelectedStyle()
    {
        if (selectedIsThemeFont || selectedFamily.isEmpty())
            return;

        auto result = yup::Font::loadSystemFont (selectedFamily, styleCombo.getText());
        if (result.failed())
        {
            infoLabel.setText (result.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        setFont (result.getValue());
    }

    void setFont (const yup::Font& newFont)
    {
        baseFont = newFont;

        rebuildAxisControls();
        rebuildFeatureToggles();

        auto codepoints = baseFont.getCodepoints();

        infoLabel.setText (baseFont.getFamilyName() + " " + baseFont.getStyleName()
                               + " - " + yup::String ((int) codepoints.size()) + " characters"
                               + (baseFont.getNumAxis() > 0 ? " - " + yup::String (baseFont.getNumAxis()) + " axes" : yup::String()),
                           yup::dontSendNotification);

        updatePreviews();
        glyphsPage.setCodepoints (std::move (codepoints));
        resized();
    }

    void rebuildAxisControls()
    {
        axisSliders.clear();
        axisLabels.clear();

        for (int index = 0; index < baseFont.getNumAxis(); ++index)
        {
            const auto axis = baseFont.getAxisDescription (index);
            if (! axis)
                continue;

            auto label = axisLabels.add (std::make_unique<yup::Label> (axis->tagName + "Label"));
            label->setText (axis->tagName, yup::dontSendNotification);
            label->setJustification (yup::Justification::center);
            addAndMakeVisible (label);

            auto slider = axisSliders.add (std::make_unique<yup::Slider> (yup::Slider::Rotary, axis->tagName));
            slider->setRange (axis->minimumValue, axis->maximumValue);
            slider->setDefaultValue (axis->defaultValue);
            slider->setValue (baseFont.getAxisValue (index), yup::dontSendNotification);
            slider->onValueChanged = [this, index, label] (double value)
            {
                baseFont.setAxisValue (index, (float) value);
                label->setText (yup::String (value, 0), yup::dontSendNotification);
                updatePreviews();
            };
            addAndMakeVisible (slider);
        }
    }

    void rebuildFeatureToggles()
    {
        featureToggles.clear();

        const auto fontFeatures = baseFont.getFeatureTags();

        for (const auto& feature : knownFeatures)
        {
            if (! fontFeatures.contains (feature.tag))
                continue;

            auto toggle = featureToggles.add (std::make_unique<yup::ToggleButton> (feature.tag));
            toggle->setButtonText (feature.tag);
            toggle->setToggleState (feature.onByDefault, yup::dontSendNotification);
            toggle->onClick = [this]
            {
                updatePreviews();
            };
            addAndMakeVisible (toggle);
        }
    }

    void updatePreviews()
    {
        auto font = baseFont;
        for (const auto* toggle : featureToggles)
            font = font.withFeature (yup::Font::Feature (yup::StringRef (toggle->getComponentID()), toggle->getToggleState() ? 1u : 0u));

        settings.font = font;

        samplePage.updateText();
        waterfallPage.repaint();
        glyphsPage.repaint();
    }

    //==============================================================================
    std::vector<FamilyEntry> allFamilies;
    std::vector<const FamilyEntry*> visibleFamilies;
    yup::String selectedFamily;
    bool selectedIsThemeFont = false;
    yup::Font baseFont;
    FontPreviewSettings settings;

    yup::Rectangle<float> sidebarBounds;
    yup::TextEditor searchEditor { "fontSearch" };
    FamilyListModel familyModel;
    yup::ListBox familyList { "fontFamilies" };

    yup::ComboBox styleCombo { "fontStyle" };
    yup::Label infoLabel { "fontInfo" };
    yup::Slider sizeSlider { yup::Slider::LinearHorizontal, "Size" };
    yup::Slider strokeSlider { yup::Slider::LinearHorizontal, "Stroke" };
    yup::Slider featherSlider { yup::Slider::LinearHorizontal, "Feather" };
    yup::Label sizeLabel { "sizeLabel" };
    yup::Label strokeLabel { "strokeLabel" };
    yup::Label featherLabel { "featherLabel" };
    yup::OwnedArray<yup::Slider> axisSliders;
    yup::OwnedArray<yup::Label> axisLabels;
    yup::OwnedArray<yup::ToggleButton> featureToggles;

    FontSamplePage samplePage { settings };
    FontWaterfallPage waterfallPage { settings };
    FontGlyphsPage glyphsPage { settings };
    yup::TabComponent previewTabs { "fontPreviews" };
};
