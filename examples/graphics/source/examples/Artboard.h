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

class ArtboardDemo : public yup::Component
{
public:
    ArtboardDemo()
    {
        setWantsKeyboardFocus (true);
        setupControls();
    }

    bool loadArtboard()
    {
        auto factory = getNativeComponent()->getFactory();
        if (factory == nullptr)
            return false;

        auto artboardFile = yup::ArtboardFile::load (getAssetPath (YUP_EXAMPLE_GRAPHICS_RIVE_FILE), *factory);
        if (! artboardFile)
            return false;

        // Setup artboards
        for (int i = 0; i < totalRows * totalColumns; ++i)
        {
            auto art = artboards.add (std::make_unique<yup::Artboard> (yup::String ("art") + yup::String (i)));
            addAndMakeVisible (art);

            art->setFile (artboardFile.getValue());
            art->setLayout (getSelectedLayout());
            art->setAlignment (getSelectedAlignment());

            art->advanceAndApply (i * art->durationSeconds());
        }

        resized();

        return true;
    }

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        repaint();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10, 20);
        auto controls = bounds.removeFromTop (30);

        auto labelHeight = 20;
        auto comboHeight = 24;
        auto labelWidth = 70;
        auto comboWidth = 170;
        auto spacing = 12;

        fitLabel.setBounds (controls.removeFromLeft (labelWidth).withHeight (labelHeight));
        fitCombo.setBounds (controls.removeFromLeft (comboWidth).withHeight (comboHeight));
        controls.removeFromLeft (spacing);
        alignmentLabel.setBounds (controls.removeFromLeft (labelWidth).withHeight (labelHeight));
        alignmentCombo.setBounds (controls.removeFromLeft (comboWidth).withHeight (comboHeight));

        if (artboards.size() != totalRows * totalColumns)
            return;

        //for (int i = 0; i < totalRows * totalColumns; ++i)
        //    artboards.getUnchecked (i)->setBounds (getLocalBounds().reduced (100.0f));

        bounds.removeFromTop (10);
        auto width = bounds.getWidth() / totalColumns;
        auto height = bounds.getHeight() / totalRows;

        for (int i = 0; i < totalRows; ++i)
        {
            auto row = bounds.removeFromTop (height);
            for (int j = 0; j < totalColumns; ++j)
            {
                auto col = row.removeFromLeft (width);
                artboards.getUnchecked (j * totalRows + i)->setBounds (col.largestFittingSquare());
            }
        }
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

private:
    void visibilityChanged() override
    {
        if (isVisible())
        {
            if (auto topLevelComponent = getTopLevelComponent(); topLevelComponent != nullptr && topLevelComponent->isOnDesktop())
                loadArtboard();
        }
    }

    void setupControls()
    {
        auto labelFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

        fitLabel.setText ("Fit", yup::dontSendNotification);
        fitLabel.setFont (labelFont);
        addAndMakeVisible (fitLabel);

        fitCombo.addItem ("Fill", 1);
        fitCombo.addItem ("Contain", 2);
        fitCombo.addItem ("Cover", 3);
        fitCombo.addItem ("Fit Width", 4);
        fitCombo.addItem ("Fit Height", 5);
        fitCombo.addItem ("None", 6);
        fitCombo.addItem ("Scale Down", 7);
        fitCombo.addItem ("Layout", 8);
        fitCombo.setSelectedId (2);
        fitCombo.onSelectedItemChanged = [this]
        {
            updateArtboardsLayout();
        };
        addAndMakeVisible (fitCombo);

        alignmentLabel.setText ("Align", yup::dontSendNotification);
        alignmentLabel.setFont (labelFont);
        addAndMakeVisible (alignmentLabel);

        alignmentCombo.addItem ("Top Left", 1);
        alignmentCombo.addItem ("Top Center", 2);
        alignmentCombo.addItem ("Top Right", 3);
        alignmentCombo.addItem ("Center Left", 4);
        alignmentCombo.addItem ("Center", 5);
        alignmentCombo.addItem ("Center Right", 6);
        alignmentCombo.addItem ("Bottom Left", 7);
        alignmentCombo.addItem ("Bottom Center", 8);
        alignmentCombo.addItem ("Bottom Right", 9);
        alignmentCombo.setSelectedId (5);
        alignmentCombo.onSelectedItemChanged = [this]
        {
            updateArtboardsLayout();
        };
        addAndMakeVisible (alignmentCombo);
    }

    yup::Artboard::Layout getSelectedLayout() const
    {
        switch (fitCombo.getSelectedId())
        {
            case 1:
                return yup::Artboard::Layout::fill;
            case 2:
                return yup::Artboard::Layout::contain;
            case 3:
                return yup::Artboard::Layout::cover;
            case 4:
                return yup::Artboard::Layout::fitWidth;
            case 5:
                return yup::Artboard::Layout::fitHeight;
            case 6:
                return yup::Artboard::Layout::none;
            case 7:
                return yup::Artboard::Layout::scaleDown;
            case 8:
                return yup::Artboard::Layout::layout;
        }

        return yup::Artboard::Layout::contain;
    }

    yup::Artboard::Alignment getSelectedAlignment() const
    {
        switch (alignmentCombo.getSelectedId())
        {
            case 1:
                return yup::Artboard::Alignment::topLeft;
            case 2:
                return yup::Artboard::Alignment::topCenter;
            case 3:
                return yup::Artboard::Alignment::topRight;
            case 4:
                return yup::Artboard::Alignment::centerLeft;
            case 5:
                return yup::Artboard::Alignment::center;
            case 6:
                return yup::Artboard::Alignment::centerRight;
            case 7:
                return yup::Artboard::Alignment::bottomLeft;
            case 8:
                return yup::Artboard::Alignment::bottomCenter;
            case 9:
                return yup::Artboard::Alignment::bottomRight;
        }

        return yup::Artboard::Alignment::center;
    }

    void updateArtboardsLayout()
    {
        auto newLayout = getSelectedLayout();
        auto newAlignment = getSelectedAlignment();

        for (auto* artboard : artboards)
        {
            artboard->setLayout (newLayout);
            artboard->setAlignment (newAlignment);
        }
    }

    yup::OwnedArray<yup::Artboard> artboards;
    yup::Label fitLabel;
    yup::ComboBox fitCombo;
    yup::Label alignmentLabel;
    yup::ComboBox alignmentCombo;
    int totalRows = 1;
    int totalColumns = 1;
};
