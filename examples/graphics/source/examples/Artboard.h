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

/** A simple rectangular marker that tracks a named Rive node.

    Its bounds are driven by the artboard node it is attached to: layout nodes
    report their laid-out size, shapes their real geometry, other nodes a unit
    rect. Depending on the attachment mode it either fills those bounds or keeps
    a fixed size while following the node's origin.
*/
class NodeMarker : public yup::Component
{
public:
    NodeMarker()
    {
        setOpaque (false);
    }

    void paint (yup::Graphics& g) override
    {
        const float markerSizeX = getWidth() - 4.0f;
        const float markerSizeY = getHeight() - 4.0f;

        auto rect = yup::Rectangle<float> (getWidth() * 0.5f - markerSizeX * 0.5f,
                                           getHeight() * 0.5f - markerSizeY * 0.5f,
                                           markerSizeX, markerSizeY);

        g.setFillColor (yup::Colors::darkorange.withAlpha (0.5f));
        g.fillRoundedRect (rect, 3.0f);

        g.setStrokeColor (yup::Colors::white);
        g.setStrokeWidth (2.0f);
        g.strokeRoundedRect (rect, 4.0f);
    }
};

//==============================================================================

class ArtboardDemoBase : public yup::Component
{
public:
    ArtboardDemoBase (yup::String rivePath, yup::String trackedNodeName, int defaultFitComboId = 2, bool fillArtboardCell = false)
        : rivePath (std::move (rivePath))
        , trackedNodeName (std::move (trackedNodeName))
        , defaultFitComboId (defaultFitComboId)
        , fillArtboardCell (fillArtboardCell)
    {
        setWantsKeyboardFocus (true);
        setupControls();
    }

    bool loadArtboard()
    {
        if (! artboards.isEmpty())
            return true;

        auto factory = getNativeComponent()->getFactory();
        if (factory == nullptr)
            return false;

        auto artboardFile = yup::ArtboardFile::load (getAssetPath (rivePath), *factory);
        if (! artboardFile)
            return false;

        loadedArtboardFile = artboardFile.getValue();

        // Setup artboards
        for (int i = 0; i < totalRows * totalColumns; ++i)
        {
            auto art = artboards.add (std::make_unique<yup::Artboard> (yup::String ("art") + yup::String (i)));
            addAndMakeVisible (art);

            art->setFile (loadedArtboardFile);
            art->setFitting (getSelectedFitting());
            art->setJustification (getSelectedJustification (alignmentCombo));

            art->advanceAndApply (i * art->durationSeconds());

            attachTrackedComponent (*art);
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

        auto labelHeight = 20;
        auto comboHeight = 24;
        auto labelWidth = 70;
        auto comboWidth = 170;
        auto spacing = 12;

        // Row 1: how the artboard is fitted into its bounds.
        auto controls = bounds.removeFromTop (30);
        fitLabel.setBounds (controls.removeFromLeft (labelWidth).withHeight (labelHeight));
        fitCombo.setBounds (controls.removeFromLeft (comboWidth).withHeight (comboHeight));
        controls.removeFromLeft (spacing);
        alignmentLabel.setBounds (controls.removeFromLeft (labelWidth).withHeight (labelHeight));
        alignmentCombo.setBounds (controls.removeFromLeft (comboWidth).withHeight (comboHeight));

        // Row 2: how the marker tracks the node (pivot = which component point is
        // anchored, anchor = which node point it is anchored to).
        controls = bounds.removeFromTop (30);
        markerLabel.setBounds (controls.removeFromLeft (labelWidth).withHeight (labelHeight));
        markerModeCombo.setBounds (controls.removeFromLeft (comboWidth).withHeight (comboHeight));
        controls.removeFromLeft (spacing);
        applyTransformToggle.setBounds (controls.removeFromLeft (110).withHeight (comboHeight));
        controls.removeFromLeft (spacing);
        pivotLabel.setBounds (controls.removeFromLeft (labelWidth).withHeight (labelHeight));
        pivotCombo.setBounds (controls.removeFromLeft (comboWidth).withHeight (comboHeight));
        controls.removeFromLeft (spacing);
        anchorLabel.setBounds (controls.removeFromLeft (labelWidth).withHeight (labelHeight));
        anchorCombo.setBounds (controls.removeFromLeft (comboWidth).withHeight (comboHeight));

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
                artboards.getUnchecked (j * totalRows + i)->setBounds (fillArtboardCell ? col : col.largestFittingSquare());
            }
        }
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

protected:
    // Builds the component attached to the tracked node of each artboard. Override
    // to attach something other than a plain marker rectangle.
    virtual std::unique_ptr<yup::Component> createTrackedComponent()
    {
        return std::make_unique<NodeMarker>();
    }

    std::shared_ptr<yup::ArtboardFile> loadedArtboardFile;

private:
    void attachedToNative() override
    {
        if (auto topLevelComponent = getTopLevelComponent(); topLevelComponent != nullptr && topLevelComponent->isOnDesktop())
            loadArtboard();
    }

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
        fitCombo.setSelectedId (defaultFitComboId);
        fitCombo.onSelectedItemChanged = [this]
        {
            updateArtboardsFitting();
        };
        addAndMakeVisible (fitCombo);

        alignmentLabel.setText ("Align", yup::dontSendNotification);
        alignmentLabel.setFont (labelFont);
        addAndMakeVisible (alignmentLabel);

        addJustificationItems (alignmentCombo, 5);
        alignmentCombo.onSelectedItemChanged = [this]
        {
            updateArtboardsFitting();
        };
        addAndMakeVisible (alignmentCombo);

        markerLabel.setText ("Marker", yup::dontSendNotification);
        markerLabel.setFont (labelFont);
        addAndMakeVisible (markerLabel);

        markerModeCombo.addItem ("Fill node", 1);
        markerModeCombo.addItem ("Track position", 2);
        markerModeCombo.setSelectedId (1);
        markerModeCombo.onSelectedItemChanged = [this]
        {
            updateMarkerControlsEnabled();
            updateMarkers();
        };
        addAndMakeVisible (markerModeCombo);

        applyTransformToggle.setButtonText ("Apply transform");
        applyTransformToggle.setToggleState (false, yup::dontSendNotification);
        applyTransformToggle.onClick = [this]
        {
            updateMarkers();
        };
        addAndMakeVisible (applyTransformToggle);

        pivotLabel.setText ("Pivot", yup::dontSendNotification);
        pivotLabel.setFont (labelFont);
        addAndMakeVisible (pivotLabel);

        addJustificationItems (pivotCombo, 1);
        pivotCombo.onSelectedItemChanged = [this]
        {
            updateMarkers();
        };
        addAndMakeVisible (pivotCombo);

        anchorLabel.setText ("Anchor", yup::dontSendNotification);
        anchorLabel.setFont (labelFont);
        addAndMakeVisible (anchorLabel);

        addJustificationItems (anchorCombo, 1);
        anchorCombo.onSelectedItemChanged = [this]
        {
            updateMarkers();
        };
        addAndMakeVisible (anchorCombo);

        updateMarkerControlsEnabled();
    }

    std::optional<yup::Fitting> getSelectedFitting() const
    {
        switch (fitCombo.getSelectedId())
        {
            case 1:
                return yup::Fitting::fill;
            case 2:
                return yup::Fitting::scaleToFit;
            case 3:
                return yup::Fitting::scaleToFill;
            case 4:
                return yup::Fitting::fitWidth;
            case 5:
                return yup::Fitting::fitHeight;
            case 6:
                return yup::Fitting::none;
            case 7:
                return yup::Fitting::centerInside;
            case 8:
                return std::nullopt;
        }

        return yup::Fitting::scaleToFit;
    }

    void updateArtboardsFitting()
    {
        const auto newFitting = getSelectedFitting();
        const auto newJustification = getSelectedJustification (alignmentCombo);

        for (auto* artboard : artboards)
        {
            artboard->setFitting (newFitting);
            artboard->setJustification (newJustification);
        }
    }

    // Attaches a tracked component to the tracked node of the given artboard.
    void attachTrackedComponent (yup::Artboard& art)
    {
        auto component = trackedComponents.add (createTrackedComponent());
        art.addAndMakeVisible (component);

        const auto options = getMarkerOptions();

        if (options.mode == yup::Artboard::NodeAttachmentOptions::Mode::trackPosition)
            component->setSize (44.0f, 44.0f);

        component->setVisible (art.attachComponentToNode (trackedNodeName, component, options));
    }

    // Populates a combo with the nine Justification points (ids 1..9) and selects
    // the entry with the given id.
    static void addJustificationItems (yup::ComboBox& combo, int defaultId)
    {
        combo.addItem ("Top Left", 1);
        combo.addItem ("Top Center", 2);
        combo.addItem ("Top Right", 3);
        combo.addItem ("Center Left", 4);
        combo.addItem ("Center", 5);
        combo.addItem ("Center Right", 6);
        combo.addItem ("Bottom Left", 7);
        combo.addItem ("Bottom Center", 8);
        combo.addItem ("Bottom Right", 9);
        combo.setSelectedId (defaultId);
    }

    // Returns the Justification currently selected in a combo filled by
    // addJustificationItems.
    yup::Justification getSelectedJustification (const yup::ComboBox& combo) const
    {
        switch (combo.getSelectedId())
        {
            case 2:
                return yup::Justification::centerTop;
            case 3:
                return yup::Justification::topRight;
            case 4:
                return yup::Justification::centerLeft;
            case 5:
                return yup::Justification::center;
            case 6:
                return yup::Justification::centerRight;
            case 7:
                return yup::Justification::bottomLeft;
            case 8:
                return yup::Justification::centerBottom;
            case 9:
                return yup::Justification::bottomRight;
            default:
                return yup::Justification::topLeft;
        }
    }

    // Pivot and anchor only drive trackPosition mode; disable them in fillNode mode.
    void updateMarkerControlsEnabled()
    {
        const auto isTracking = markerModeCombo.getSelectedId() == 2;
        pivotLabel.setEnabled (isTracking);
        pivotCombo.setEnabled (isTracking);
        anchorLabel.setEnabled (isTracking);
        anchorCombo.setEnabled (isTracking);
    }

    yup::Artboard::NodeAttachmentOptions getMarkerOptions() const
    {
        yup::Artboard::NodeAttachmentOptions options;
        options.mode = markerModeCombo.getSelectedId() == 2
                           ? yup::Artboard::NodeAttachmentOptions::Mode::trackPosition
                           : yup::Artboard::NodeAttachmentOptions::Mode::fillNode;
        options.applyTransform = applyTransformToggle.getToggleState();
        options.pivot = getSelectedJustification (pivotCombo);
        options.anchor = getSelectedJustification (anchorCombo);
        return options;
    }

    // Re-attaches every tracked component with the currently selected attachment mode.
    void updateMarkers()
    {
        const auto options = getMarkerOptions();

        for (int i = 0; i < artboards.size() && i < trackedComponents.size(); ++i)
        {
            auto* art = artboards.getUnchecked (i);
            auto* component = trackedComponents.getUnchecked (i);

            if (component == nullptr || art == nullptr)
                continue;

            if (options.mode == yup::Artboard::NodeAttachmentOptions::Mode::trackPosition)
                component->setSize (44.0f, 44.0f);

            component->setVisible (art->attachComponentToNode (trackedNodeName, component, options));
        }
    }

    yup::String rivePath;
    yup::String trackedNodeName;
    int defaultFitComboId;
    bool fillArtboardCell;

    yup::OwnedArray<yup::Component> trackedComponents;
    yup::OwnedArray<yup::Artboard> artboards;
    yup::Label fitLabel;
    yup::ComboBox fitCombo;
    yup::Label alignmentLabel;
    yup::ComboBox alignmentCombo;
    yup::Label markerLabel;
    yup::ComboBox markerModeCombo;
    yup::ToggleButton applyTransformToggle;
    yup::Label pivotLabel;
    yup::ComboBox pivotCombo;
    yup::Label anchorLabel;
    yup::ComboBox anchorCombo;
    int totalRows = 1;
    int totalColumns = 1;
};

//==============================================================================

class ArtboardDemo : public ArtboardDemoBase
{
public:
    ArtboardDemo()
        : ArtboardDemoBase ("data/alien.riv", "Mouth")
    {
    }
};

//==============================================================================

class ArtboardLayoutDemo : public ArtboardDemoBase
{
public:
    ArtboardLayoutDemo()
        : ArtboardDemoBase ("data/layout-ui.riv", "keyboard_slot", 8, true)
    {
    }

private:
    std::unique_ptr<yup::Component> createTrackedComponent() override
    {
        auto keyboardArtboard = std::make_unique<yup::Artboard> ("keyboardArtboard");
        keyboardArtboard->setFile (loadedArtboardFile, "Keyboard");
        keyboardArtboard->setFitting (yup::Fitting::fill);
        return keyboardArtboard;
    }
};
