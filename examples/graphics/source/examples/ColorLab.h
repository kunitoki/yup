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

#include <array>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

//==============================================================================

class ColorLabDemo : public yup::Component
{
public:
    ColorLabDemo()
        : yup::Component ("ColorLabDemo")
    {
        modeCombo = std::make_unique<yup::ComboBox> ("colorLabMode");
        modeCombo->addItem ("Blend Spaces", static_cast<int> (Mode::BlendSpaces));
        modeCombo->addItem ("Gradient Editor", static_cast<int> (Mode::GradientEditor));
        modeCombo->addItem ("Color Picker", static_cast<int> (Mode::Picker));
        modeCombo->setSelectedId (static_cast<int> (Mode::BlendSpaces));
        modeCombo->onSelectedItemChanged = [this]
        {
            setMode (static_cast<Mode> (modeCombo->getSelectedId()));
        };
        addAndMakeVisible (modeCombo.get());

        gradientTypeCombo = std::make_unique<yup::ComboBox> ("gradientType");
        gradientTypeCombo->addItem ("Linear", 1);
        gradientTypeCombo->addItem ("Radial", 2);
        gradientTypeCombo->setSelectedId (1);
        gradientTypeCombo->onSelectedItemChanged = [this]
        {
            isGradientRadial = gradientTypeCombo->getSelectedId() == 2;
            repaint();
        };
        addAndMakeVisible (gradientTypeCombo.get());

        pickerSpaceCombo = std::make_unique<yup::ComboBox> ("pickerSpace");
        pickerSpaceCombo->addItem ("RGBA", static_cast<int> (PickerSpace::Rgba));
        pickerSpaceCombo->addItem ("HSL", static_cast<int> (PickerSpace::Hsl));
        pickerSpaceCombo->addItem ("HSV", static_cast<int> (PickerSpace::Hsv));
        pickerSpaceCombo->addItem ("HSLuv", static_cast<int> (PickerSpace::Hsluv));
        pickerSpaceCombo->setSelectedId (static_cast<int> (PickerSpace::Hsluv));
        pickerSpaceCombo->onSelectedItemChanged = [this]
        {
            pickerSpace = static_cast<PickerSpace> (pickerSpaceCombo->getSelectedId());
            updatePickerSlidersFromColor();
            updateChannelLabels();
            repaint();
        };
        addAndMakeVisible (pickerSpaceCombo.get());

        for (size_t i = 0; i < channelSliders.size(); ++i)
        {
            channelSliders[i] = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
            channelSliders[i]->setRange (0.0, 1.0, 0.001);
            channelSliders[i]->setValue (0.5);
            channelSliders[i]->onValueChanged = [this, index = i] (double value)
            {
                handleChannelSliderChange (index, static_cast<float> (value));
            };
            addAndMakeVisible (channelSliders[i].get());

            channelLabels[i] = std::make_unique<yup::Label> ("channelLabel");
            channelLabels[i]->setText ("", yup::dontSendNotification);
            addAndMakeVisible (channelLabels[i].get());
        }

        for (size_t i = 0; i < blendStartSliders.size(); ++i)
        {
            blendStartSliders[i] = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
            blendStartSliders[i]->setRange (0.0, 1.0, 0.001);
            blendStartSliders[i]->onValueChanged = [this, index = i] (double value)
            {
                blendStartValues[index] = static_cast<float> (value);
                updateBlendColorsFromSliders();
            };
            addAndMakeVisible (blendStartSliders[i].get());

            blendStartLabels[i] = std::make_unique<yup::Label> ("blendStartLabel");
            addAndMakeVisible (blendStartLabels[i].get());
        }

        for (size_t i = 0; i < blendEndSliders.size(); ++i)
        {
            blendEndSliders[i] = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
            blendEndSliders[i]->setRange (0.0, 1.0, 0.001);
            blendEndSliders[i]->onValueChanged = [this, index = i] (double value)
            {
                blendEndValues[index] = static_cast<float> (value);
                updateBlendColorsFromSliders();
            };
            addAndMakeVisible (blendEndSliders[i].get());

            blendEndLabels[i] = std::make_unique<yup::Label> ("blendEndLabel");
            addAndMakeVisible (blendEndLabels[i].get());
        }

        addStopButton = std::make_unique<yup::TextButton> ("+");
        addStopButton->onClick = [this]
        {
            addGradientStop();
        };
        addAndMakeVisible (addStopButton.get());

        removeStopButton = std::make_unique<yup::TextButton> ("-");
        removeStopButton->onClick = [this]
        {
            removeSelectedStop();
        };
        addAndMakeVisible (removeStopButton.get());

        for (size_t i = 0; i < stopColorSliders.size(); ++i)
        {
            stopColorSliders[i] = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
            stopColorSliders[i]->setRange (0.0, 1.0, 0.001);
            stopColorSliders[i]->onValueChanged = [this, index = i] (double value)
            {
                stopColorValues[index] = static_cast<float> (value);
                updateSelectedStopColor();
            };
            addAndMakeVisible (stopColorSliders[i].get());

            stopColorLabels[i] = std::make_unique<yup::Label> ("stopColorLabel");
            addAndMakeVisible (stopColorLabels[i].get());
        }

        pickerColor = yup::Color::fromHSLuv (0.1f, 0.8f, 0.55f);
        updatePickerSlidersFromColor();
        updateChannelLabels();

        gradientStops.push_back ({ 0.0f, gradientStartColor });
        gradientStops.push_back ({ 1.0f, gradientEndColor });
        selectedStopIndex = 0;
        updateGradientStopSlidersFromSelection();
        updateBlendSlidersFromColors();
        updateControlVisibility();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().to<float>();
        headerArea = bounds.removeFromTop (80.0f);
        contentArea = bounds.reduced (20.0f);

        auto comboArea = headerArea;
        comboArea = comboArea.removeFromRight (180.0f).reduced (10.0f);
        modeCombo->setBounds (comboArea.removeFromTop (26.0f));

        comboArea.removeFromTop (6.0f);
        auto secondComboArea = comboArea.removeFromTop (26.0f);
        gradientTypeCombo->setBounds (secondComboArea);
        pickerSpaceCombo->setBounds (secondComboArea);

        {
            auto blendArea = getBlendControlsArea();
            layoutColorSliders (blendArea.removeFromTop (blendArea.getHeight() * 0.5f),
                                blendStartLabels,
                                blendStartSliders);
            blendArea.removeFromTop (12.0f);
            layoutColorSliders (blendArea, blendEndLabels, blendEndSliders);
        }

        {
            auto stopArea = getGradientControlsArea();
            auto buttonRow = stopArea.removeFromTop (26.0f);
            addStopButton->setBounds (buttonRow.removeFromLeft (50.0f));
            buttonRow.removeFromLeft (10.0f);
            removeStopButton->setBounds (buttonRow.removeFromLeft (50.0f));
            stopArea.removeFromTop (12.0f);
            layoutColorSliders (stopArea, stopColorLabels, stopColorSliders);
        }

        auto sliderArea = getPickerControlsArea();
        layoutColorSliders (sliderArea, channelLabels, channelSliders);
    }

    void paint (yup::Graphics& g) override
    {
        drawBackground (g, getLocalBounds().to<float>());

        auto titleArea = headerArea;
        titleArea.removeFromRight (200.0f);
        drawHeader (g, titleArea.reduced (8.0f, 10.0f));

        switch (currentMode)
        {
            case Mode::BlendSpaces:
                drawBlendSpaces (g, contentArea);
                break;
            case Mode::GradientEditor:
                drawGradientEditor (g, contentArea);
                break;
            case Mode::Picker:
                drawPicker (g, contentArea);
                break;
        }
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        handlePointer (event.getPosition(), event.getModifiers(), true);
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        handlePointer (event.getPosition(), event.getModifiers(), false);
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        dragHandle = DragHandle::None;
        draggingStopIndex = invalidStopIndex;
    }

private:
    enum class Mode
    {
        BlendSpaces = 1,
        GradientEditor,
        Picker
    };

    enum class PickerSpace
    {
        Rgba = 1,
        Hsl,
        Hsv,
        Hsluv
    };

    enum class DragHandle
    {
        None,
        Start,
        End,
        Stop
    };

    struct GradientStopData
    {
        float t = 0.0f;
        yup::Color color;
        yup::Point<float> customPositionNorm;
        bool hasCustomPosition = false;
    };

    enum class BlendMode
    {
        Srgb,
        Linear,
        Spectral
    };

    void setMode (Mode mode)
    {
        if (currentMode == mode)
            return;

        currentMode = mode;
        updateControlVisibility();
        repaint();
    }

    void updateControlVisibility()
    {
        const bool gradientMode = currentMode == Mode::GradientEditor;
        const bool pickerMode = currentMode == Mode::Picker;
        const bool blendMode = currentMode == Mode::BlendSpaces;

        gradientTypeCombo->setVisible (gradientMode);
        pickerSpaceCombo->setVisible (pickerMode);

        for (size_t i = 0; i < blendStartSliders.size(); ++i)
        {
            blendStartSliders[i]->setVisible (blendMode);
            blendStartLabels[i]->setVisible (blendMode);
            blendEndSliders[i]->setVisible (blendMode);
            blendEndLabels[i]->setVisible (blendMode);
        }

        addStopButton->setVisible (gradientMode);
        removeStopButton->setVisible (gradientMode);
        for (size_t i = 0; i < stopColorSliders.size(); ++i)
        {
            stopColorSliders[i]->setVisible (gradientMode);
            stopColorLabels[i]->setVisible (gradientMode);
        }

        for (size_t i = 0; i < channelSliders.size(); ++i)
        {
            channelSliders[i]->setVisible (pickerMode);
            channelLabels[i]->setVisible (pickerMode);
        }
    }

    yup::Rectangle<float> getPickerControlsArea() const
    {
        auto area = contentArea;
        area.removeFromLeft (area.getWidth() * 0.55f);
        return area.reduced (12.0f);
    }

    yup::Rectangle<float> getBlendControlsArea() const
    {
        auto area = contentArea;
        area.removeFromLeft (area.getWidth() * 0.55f);
        return area.reduced (12.0f);
    }

    yup::Rectangle<float> getGradientControlsArea() const
    {
        auto area = contentArea;
        area.removeFromLeft (area.getWidth() * 0.6f);
        return area.reduced (12.0f);
    }

    template <typename LabelArray, typename SliderArray>
    void layoutColorSliders (yup::Rectangle<float> area, LabelArray& labels, SliderArray& sliders)
    {
        const float labelWidth = 96.0f;
        const float rowHeight = 26.0f;
        const auto labelFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);
        for (size_t i = 0; i < sliders.size(); ++i)
        {
            auto row = area.removeFromTop (rowHeight);
            labels[i]->setFont (labelFont);
            labels[i]->setJustification (yup::Justification::left);
            labels[i]->setBounds (row.removeFromLeft (labelWidth));
            sliders[i]->setBounds (row);
            area.removeFromTop (6.0f);
        }
    }

    static double toLinear (double value)
    {
        if (value <= 0.04045)
            return value / 12.92;

        return std::pow ((value + 0.055) / 1.055, 2.4);
    }

    static double fromLinear (double value)
    {
        if (value <= 0.0031308)
            return 12.92 * value;

        return 1.055 * std::pow (value, 1.0 / 2.4) - 0.055;
    }

    static uint8 toByte (double value)
    {
        return static_cast<uint8> (yup::roundToInt (yup::jlimit (0.0, 1.0, value) * 255.0));
    }

    static yup::Color colorFromNormalized (double r, double g, double b, double a)
    {
        return yup::Color (toByte (a), toByte (r), toByte (g), toByte (b));
    }

    void drawBackground (yup::Graphics& g, const yup::Rectangle<float>& area)
    {
        g.setFillColor (yup::Color::fromHSLuv (0.62f, 0.35f, 0.12f));
        g.fillAll();
    }

    void drawHeader (yup::Graphics& g, yup::Rectangle<float> area)
    {
        const auto titleFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (26.0f);
        const auto subtitleFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (13.0f);

        yup::StyledText title;
        {
            auto modifier = title.startUpdate();
            modifier.setMaxSize (area.getSize());
            modifier.setHorizontalAlign (yup::StyledText::left);
            modifier.setWrap (yup::StyledText::noWrap);
            modifier.appendText ("Color Lab", titleFont);
        }

        yup::StyledText subtitle;
        {
            auto modifier = subtitle.startUpdate();
            modifier.setMaxSize (area.getSize());
            modifier.setHorizontalAlign (yup::StyledText::left);
            modifier.setWrap (yup::StyledText::noWrap);
            modifier.appendText ("Perceptual spaces, spectral mixing, editable gradients, and live picking.", subtitleFont);
        }

        g.setFillColor (yup::Colors::white.withAlpha (0.9f));
        g.fillFittedText (title, area.removeFromTop (34.0f));
        g.setFillColor (yup::Colors::white.withAlpha (0.6f));
        g.fillFittedText (subtitle, area.removeFromTop (18.0f));
    }

    void drawPanel (yup::Graphics& g, const yup::Rectangle<float>& area)
    {
        const auto panelBase = yup::Color::fromHSLuv (0.63f, 0.15f, 0.16f).withAlpha (0.92f);
        const auto panelEdge = yup::Color::fromHSLuv (0.63f, 0.25f, 0.1f).withAlpha (0.92f);
        yup::ColorGradient panel (panelBase, area.getX(), area.getY(), panelEdge, area.getRight(), area.getBottom(), yup::ColorGradient::Linear);

        g.setFillColorGradient (panel);
        g.fillRoundedRect (area, 16.0f);
        g.setStrokeColor (yup::Colors::white.withAlpha (0.08f));
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (area, 16.0f);
    }

    void drawSectionTitle (yup::Graphics& g, const yup::String& title, yup::Rectangle<float> area)
    {
        yup::StyledText text;
        {
            auto modifier = text.startUpdate();
            modifier.setMaxSize (area.getSize());
            modifier.setHorizontalAlign (yup::StyledText::left);
            modifier.appendText (title, yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (14.0f));
        }

        g.setFillColor (yup::Colors::white.withAlpha (0.75f));
        g.fillFittedText (text, area.removeFromTop (18.0f));
    }

    void drawBlendSpaces (yup::Graphics& g, yup::Rectangle<float> area)
    {
        auto leftArea = area.withTrimmedRight (area.getWidth() * 0.45f).reduced (8.0f);
        auto rightArea = area.removeFromRight (area.getWidth() * 0.45f).reduced (8.0f);

        drawPanel (g, leftArea);
        drawPanel (g, rightArea);

        auto content = leftArea.reduced (24.0f);
        drawSectionTitle (g, "Normal vs Linear vs Spectral", content);
        content.removeFromTop (28.0f);

        auto rowHeight = (content.getHeight() - 20.0f) / 3.0f;
        auto row1 = content.removeFromTop (rowHeight);
        auto row2 = content.removeFromTop (rowHeight);
        auto row3 = content.removeFromTop (rowHeight);

        drawBlendRamp (g, row1, "sRGB Blend", BlendMode::Srgb);
        drawBlendRamp (g, row2, "Linear RGB", BlendMode::Linear);
        drawBlendRamp (g, row3, "Spectral Mix", BlendMode::Spectral);
    }

    void drawGradientEditor (yup::Graphics& g, yup::Rectangle<float> area)
    {
        auto leftArea = area.withTrimmedRight (area.getWidth() * 0.4f).reduced (8.0f);
        auto rightArea = area.removeFromRight (area.getWidth() * 0.4f).reduced (8.0f);

        drawPanel (g, leftArea);
        drawPanel (g, rightArea);

        auto content = leftArea.reduced (24.0f);
        drawSectionTitle (g, "Gradient Editor", content);
        content.removeFromTop (28.0f);

        gradientEditorArea = content.withTrimmedBottom (28.0f);

        const auto start = pointFromNormalized (gradientEditorArea, gradientStartNorm);
        const auto end = pointFromNormalized (gradientEditorArea, gradientEndNorm);
        const auto stops = buildGradientStops (gradientEditorArea, start, end);
        auto gradient = yup::ColorGradient (isGradientRadial ? yup::ColorGradient::Radial : yup::ColorGradient::Linear, stops);
        g.setFillColorGradient (gradient);
        g.fillRoundedRect (gradientEditorArea, 16.0f);

        g.setStrokeColor (yup::Colors::white.withAlpha (0.2f));
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (gradientEditorArea, 16.0f);

        if (isGradientRadial)
        {
            const float radius = start.distanceTo (end);
            g.setStrokeColor (yup::Colors::white.withAlpha (0.35f));
            g.setStrokeWidth (1.0f);
            g.strokeEllipse (yup::Rectangle<float> (start.getX() - radius, start.getY() - radius, radius * 2.0f, radius * 2.0f));
        }
        else
        {
            g.setStrokeColor (yup::Colors::white.withAlpha (0.35f));
            g.setStrokeWidth (1.0f);
            g.strokeLine (start, end);
        }

        drawHandle (g, start, "A");
        drawHandle (g, end, "B");
        drawStopHandles (g, gradientEditorArea, start, end);

        auto hintArea = content.removeFromBottom (22.0f);
        g.setFillColor (yup::Colors::white.withAlpha (0.6f));
        const auto infoFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);
        g.fillFittedText ("Drag points; hold Shift to constrain stops to the line.", infoFont, hintArea, yup::Justification::left);
    }

    void drawPicker (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawPanel (g, area.reduced (8.0f));
        auto content = area.reduced (24.0f);
        drawSectionTitle (g, "Color Picker", content);
        content.removeFromTop (12.0f);

        auto pickerArea = content;
        pickerArea.removeFromRight (content.getWidth() * 0.36f);
        pickerArea = pickerArea.reduced (10.0f);
        const float wheelSize = yup::jmin (pickerArea.getWidth(), pickerArea.getHeight());
        pickerWheelArea = pickerArea.withSize (wheelSize, wheelSize).withCenter (pickerArea.getCenterX(), pickerArea.getCenterY());

        drawPickerWheel (g, pickerWheelArea);
        drawPickerDetails (g, content.removeFromRight (content.getWidth() * 0.36f).reduced (12.0f));
    }

    void drawPickerWheel (yup::Graphics& g, const yup::Rectangle<float>& area)
    {
        const auto center = yup::Point<float> (area.getCenterX(), area.getCenterY());
        const float radius = area.getWidth() * 0.5f;
        const auto arcBounds = yup::Rectangle<float> (center.getX() - radius, center.getY() - radius, radius * 2.0f, radius * 2.0f);

        const int segments = 180;
        const float step = 1.0f / static_cast<float> (segments);
        const float overlap = step * 0.35f;

        for (int i = 0; i < segments; ++i)
        {
            const float hue = static_cast<float> (i) * step;
            const float hueMid = hue + step * 0.5f;
            const float angle1 = (hue - overlap) * 2.0f * yup::MathConstants<float>::pi;
            const float angle2 = (hue + step + overlap) * 2.0f * yup::MathConstants<float>::pi;

            const auto edgePoint = yup::Point<float> (center.getX() + std::cos (angle2) * radius,
                                                      center.getY() + std::sin (angle2) * radius);

            const auto centerColor = yup::Color::fromHSV (hueMid, 0.0f, pickerLuminance).withAlpha (pickerAlpha);
            const auto edgeColor = yup::Color::fromHSV (hueMid, 1.0f, pickerLuminance).withAlpha (pickerAlpha);

            yup::ColorGradient radial (centerColor, center.getX(), center.getY(), edgeColor, edgePoint.getX(), edgePoint.getY(), yup::ColorGradient::Radial);
            g.setFillColorGradient (radial);

            yup::Path wedge;
            wedge.moveTo (center);
            wedge.lineTo (center.getX() + std::cos (angle1) * radius, center.getY() + std::sin (angle1) * radius);
            wedge.addArc (arcBounds, angle1, angle2, false);
            wedge.close();
            g.fillPath (wedge);
        }

        g.setStrokeColor (yup::Colors::white.withAlpha (0.25f));
        g.setStrokeWidth (1.0f);
        g.strokeEllipse (area);

        const auto marker = wheelMarkerPosition (area);
        g.setFillColor (yup::Colors::white);
        g.fillEllipse (yup::Rectangle<float> (marker.getX() - 4.5f, marker.getY() - 4.5f, 9.0f, 9.0f));
        g.setStrokeColor (yup::Colors::black.withAlpha (0.4f));
        g.setStrokeWidth (1.0f);
        g.strokeEllipse (yup::Rectangle<float> (marker.getX() - 4.5f, marker.getY() - 4.5f, 9.0f, 9.0f));
    }

    void drawPickerDetails (yup::Graphics& g, yup::Rectangle<float> area)
    {
        area.removeFromTop (100.0f);

        auto swatch = area.removeFromTop (50.0f);
        g.setFillColor (pickerColor);
        g.fillRoundedRect (swatch, 14.0f);
        g.setStrokeColor (yup::Colors::white.withAlpha (0.2f));
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (swatch, 14.0f);

        area.removeFromTop (12.0f);
        g.setFillColor (yup::Colors::white.withAlpha (0.65f));
        const auto infoFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);
        g.fillFittedText ("Space: " + pickerSpaceCombo->getText(), infoFont, area.removeFromTop (18.0f), yup::Justification::left);
    }

    std::vector<yup::ColorGradient::ColorStop> buildGradientStops (const yup::Rectangle<float>& area,
                                                                   const yup::Point<float>& start,
                                                                   const yup::Point<float>& end) const
    {
        std::vector<yup::ColorGradient::ColorStop> stops;
        stops.reserve (gradientStops.size());

        const auto line = yup::Line<float> (start, end);
        for (size_t i = 0; i < gradientStops.size(); ++i)
        {
            const auto point = stopPositionForIndex (i, area, line);
            stops.emplace_back (gradientStops[i].color, point, gradientStops[i].t);
        }

        std::sort (stops.begin(), stops.end(), [] (const auto& a, const auto& b)
        {
            return a.delta < b.delta;
        });

        return stops;
    }

    void drawStopHandles (yup::Graphics& g,
                          const yup::Rectangle<float>& area,
                          const yup::Point<float>& start,
                          const yup::Point<float>& end)
    {
        if (gradientStops.empty())
            return;

        const auto line = yup::Line<float> (start, end);
        for (size_t i = 0; i < gradientStops.size(); ++i)
        {
            const auto point = stopPositionForIndex (i, area, line);
            const bool isSelected = i == selectedStopIndex;
            const float radius = isSelected ? 7.0f : 5.0f;
            g.setFillColor (gradientStops[i].color.withAlpha (1.0f));
            g.fillEllipse (yup::Rectangle<float> (point.getX() - radius, point.getY() - radius, radius * 2.0f, radius * 2.0f));
            g.setStrokeColor (isSelected ? yup::Colors::white : yup::Colors::black.withAlpha (0.4f));
            g.setStrokeWidth (1.0f);
            g.strokeEllipse (yup::Rectangle<float> (point.getX() - radius, point.getY() - radius, radius * 2.0f, radius * 2.0f));

            if (isSelected)
            {
                const auto labelFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (11.0f);
                const auto label = formatValue (gradientStops[i].t, 2);
                const auto labelArea = yup::Rectangle<float> (point.getX() + 16.0f, point.getY() - 10.0f, 48.0f, 18.0f);
                g.setFillColor (yup::Colors::white.withAlpha (0.8f));
                g.fillFittedText (label, labelFont, labelArea, yup::Justification::left);
            }
        }
    }

    void drawBlendRamp (yup::Graphics& g, yup::Rectangle<float> area, const yup::String& label, BlendMode mode)
    {
        auto labelArea = area.removeFromTop (18.0f);
        g.setFillColor (yup::Colors::white.withAlpha (0.7f));
        const auto labelFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);
        g.fillFittedText (label, labelFont, labelArea, yup::Justification::left);

        const auto& colorA = blendStartColor;
        const auto& colorB = blendEndColor;

        const int steps = 10;
        const float gap = 6.0f;
        const float swatchWidth = (area.getWidth() - gap * (steps - 1)) / static_cast<float> (steps);
        const float swatchHeight = area.getHeight() - 6.0f;

        for (int i = 0; i < steps; ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (steps - 1);
            auto swatch = yup::Rectangle<float> (area.getX() + i * (swatchWidth + gap), area.getY(), swatchWidth, swatchHeight);
            g.setFillColor (blendColors (colorA, colorB, t, mode));
            g.fillRoundedRect (swatch, 8.0f);
        }
    }

    void drawHsluvPalette (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "HSLuv Palette", area);
        area.removeFromTop (10.0f);

        const int columns = 6;
        const int rows = 2;
        const float gap = 8.0f;
        const float swatchWidth = (area.getWidth() - gap * (columns - 1)) / static_cast<float> (columns);
        const float swatchHeight = (area.getHeight() - gap * (rows - 1)) / static_cast<float> (rows);

        for (int row = 0; row < rows; ++row)
        {
            const float luminance = row == 0 ? 0.58f : 0.35f;
            for (int col = 0; col < columns; ++col)
            {
                const int index = row * columns + col;
                const float hue = static_cast<float> (index) / static_cast<float> (columns * rows);
                const float saturation = row == 0 ? 0.85f : 0.6f;

                auto swatch = yup::Rectangle<float> (
                    area.getX() + col * (swatchWidth + gap),
                    area.getY() + row * (swatchHeight + gap),
                    swatchWidth,
                    swatchHeight);

                g.setFillColor (yup::Color::fromHSLuv (hue, saturation, luminance));
                g.fillRoundedRect (swatch, 10.0f);
                g.setStrokeColor (yup::Colors::black.withAlpha (0.2f));
                g.setStrokeWidth (1.0f);
                g.strokeRoundedRect (swatch, 10.0f);
            }
        }
    }

    void drawSpectralMix (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Spectral Mixing Ramp", area);
        area.removeFromTop (10.0f);

        const auto leftColor = yup::Color::fromHSLuv (0.03f, 0.92f, 0.58f);
        const auto rightColor = yup::Color::fromHSLuv (0.65f, 0.85f, 0.55f);
        const int steps = 9;
        const float gap = 8.0f;
        const float swatchWidth = (area.getWidth() - gap * (steps - 1)) / static_cast<float> (steps);
        const float swatchHeight = area.getHeight() * 0.55f;

        for (int i = 0; i < steps; ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (steps - 1);
            auto swatch = yup::Rectangle<float> (area.getX() + i * (swatchWidth + gap), area.getY(), swatchWidth, swatchHeight);
            g.setFillColor (leftColor.mixedWith (rightColor, t));
            g.fillRoundedRect (swatch, 9.0f);
        }

        auto bar = area.withTrimmedTop (swatchHeight + 14.0f);
        auto gradient = yup::ColorGradient::fromLinearColors (leftColor, bar.getX(), bar.getY(), rightColor, bar.getRight(), bar.getY(), 7);
        g.setFillColorGradient (gradient);
        g.fillRoundedRect (bar, 8.0f);
        g.setStrokeColor (yup::Colors::white.withAlpha (0.15f));
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (bar, 8.0f);
    }

    void drawGradientGallery (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Linear + Radial Gradients", area);
        area.removeFromTop (10.0f);

        auto top = area.removeFromTop (area.getHeight() * 0.5f);
        auto bottom = area;

        auto linear = yup::ColorGradient::fromLinearColors (
            yup::Color::fromHSLuv (0.88f, 0.7f, 0.62f),
            top.getX(),
            top.getY(),
            yup::Color::fromHSLuv (0.2f, 0.9f, 0.55f),
            top.getRight(),
            top.getBottom(),
            6);
        g.setFillColorGradient (linear);
        g.fillRoundedRect (top, 12.0f);

        auto radial = yup::ColorGradient::fromLinearColors (
            yup::Color::fromHSLuv (0.55f, 0.7f, 0.58f),
            bottom.getX(),
            bottom.getY(),
            yup::Color::fromHSLuv (0.95f, 0.2f, 0.2f),
            bottom.getRight(),
            bottom.getBottom(),
            5,
            yup::ColorGradient::Radial);
        g.setFillColorGradient (radial);
        g.fillRoundedRect (bottom, 12.0f);
    }

    yup::Color blendColors (yup::Color a, yup::Color b, float t, BlendMode mode) const
    {
        if (mode == BlendMode::Spectral)
            return a.mixedWith (b, t);

        const double ra = a.getRedFloat();
        const double ga = a.getGreenFloat();
        const double ba = a.getBlueFloat();
        const double rb = b.getRedFloat();
        const double gb = b.getGreenFloat();
        const double bb = b.getBlueFloat();

        if (mode == BlendMode::Srgb)
        {
            const double r = ra + (rb - ra) * t;
            const double g = ga + (gb - ga) * t;
            const double bl = ba + (bb - ba) * t;
            const double alpha = a.getAlphaFloat() + (b.getAlphaFloat() - a.getAlphaFloat()) * t;
            return colorFromNormalized (r, g, bl, alpha);
        }

        const double r = fromLinear (toLinear (ra) + (toLinear (rb) - toLinear (ra)) * t);
        const double g = fromLinear (toLinear (ga) + (toLinear (gb) - toLinear (ga)) * t);
        const double bl = fromLinear (toLinear (ba) + (toLinear (bb) - toLinear (ba)) * t);
        const double alpha = a.getAlphaFloat() + (b.getAlphaFloat() - a.getAlphaFloat()) * t;
        return colorFromNormalized (r, g, bl, alpha);
    }

    yup::Point<float> pointFromNormalized (const yup::Rectangle<float>& area, const yup::Point<float>& norm) const
    {
        return { area.getX() + norm.getX() * area.getWidth(), area.getY() + norm.getY() * area.getHeight() };
    }

    yup::Point<float> normalizedFromPoint (const yup::Rectangle<float>& area, const yup::Point<float>& point) const
    {
        const float x = yup::jlimit (0.0f, 1.0f, (point.getX() - area.getX()) / area.getWidth());
        const float y = yup::jlimit (0.0f, 1.0f, (point.getY() - area.getY()) / area.getHeight());
        return { x, y };
    }

    void drawHandle (yup::Graphics& g, const yup::Point<float>& point, const yup::String& label)
    {
        g.setFillColor (yup::Colors::white.withAlpha (0.85f));
        g.fillEllipse (yup::Rectangle<float> (point.getX() - 6.0f, point.getY() - 6.0f, 12.0f, 12.0f));
        g.setStrokeColor (yup::Colors::black.withAlpha (0.4f));
        g.setStrokeWidth (1.0f);
        g.strokeEllipse (yup::Rectangle<float> (point.getX() - 6.0f, point.getY() - 6.0f, 12.0f, 12.0f));
        g.setFillColor (yup::Colors::black.withAlpha (0.6f));

        const auto labelFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (11.0f);
        g.fillFittedText (label, labelFont, yup::Rectangle<float> (point.getX() + 8.0f, point.getY() - 10.0f, 18.0f, 18.0f), yup::Justification::left);
    }

    yup::Point<float> wheelMarkerPosition (const yup::Rectangle<float>& area) const
    {
        const float angle = pickerHue * 2.0f * yup::MathConstants<float>::pi;
        const float radius = pickerSaturation * area.getWidth() * 0.5f;
        const float cx = area.getCenterX() + std::cos (angle) * radius;
        const float cy = area.getCenterY() + std::sin (angle) * radius;
        return { cx, cy };
    }

    size_t hitTestStop (const yup::Point<float>& position,
                        const yup::Rectangle<float>& area,
                        const yup::Point<float>& start,
                        const yup::Point<float>& end) const
    {
        if (gradientStops.empty())
            return invalidStopIndex;

        const auto line = yup::Line<float> (start, end);
        for (size_t i = 0; i < gradientStops.size(); ++i)
        {
            const auto point = stopPositionForIndex (i, area, line);
            if (position.distanceTo (point) < 12.0f)
                return i;
        }

        return invalidStopIndex;
    }

    yup::Point<float> stopPositionForIndex (size_t index,
                                            const yup::Rectangle<float>& area,
                                            const yup::Line<float>& line) const
    {
        if (index >= gradientStops.size())
            return line.getStart();

        const auto& stop = gradientStops[index];
        if (stop.hasCustomPosition)
            return pointFromNormalized (area, stop.customPositionNorm);

        return line.pointAlong (stop.t);
    }

    float projectPositionToT (const yup::Point<float>& position,
                              const yup::Point<float>& start,
                              const yup::Point<float>& end) const
    {
        const float dx = end.getX() - start.getX();
        const float dy = end.getY() - start.getY();
        const float lenSquared = dx * dx + dy * dy;
        if (lenSquared <= 0.000001f)
            return 0.0f;

        const float t = ((position.getX() - start.getX()) * dx + (position.getY() - start.getY()) * dy) / lenSquared;
        return yup::jlimit (0.0f, 1.0f, t);
    }

    yup::String formatValue (float value, int decimals = 2) const
    {
        return yup::String (value, decimals);
    }

    void updateCustomStopDeltas()
    {
        if (gradientStops.size() < 3)
            return;

        const auto start = pointFromNormalized (gradientEditorArea, gradientStartNorm);
        const auto end = pointFromNormalized (gradientEditorArea, gradientEndNorm);

        for (size_t i = 1; i + 1 < gradientStops.size(); ++i)
        {
            if (! gradientStops[i].hasCustomPosition)
                continue;

            const auto position = pointFromNormalized (gradientEditorArea, gradientStops[i].customPositionNorm);
            const float minT = gradientStops[i - 1].t + 0.001f;
            const float maxT = gradientStops[i + 1].t - 0.001f;
            gradientStops[i].t = yup::jlimit (minT, maxT, projectPositionToT (position, start, end));
        }
    }

    void selectStop (size_t index)
    {
        if (index >= gradientStops.size())
            return;

        selectedStopIndex = index;
        updateGradientStopSlidersFromSelection();
        repaint();
    }

    void handlePointer (const yup::Point<float>& position, const yup::KeyModifiers& modifiers, bool isDown)
    {
        if (currentMode == Mode::GradientEditor)
        {
            if (! gradientEditorArea.contains (position))
                return;

            if (isDown)
            {
                const auto start = pointFromNormalized (gradientEditorArea, gradientStartNorm);
                const auto end = pointFromNormalized (gradientEditorArea, gradientEndNorm);
                const auto hitIndex = hitTestStop (position, gradientEditorArea, start, end);

                if (position.distanceTo (start) < 14.0f)
                {
                    dragHandle = DragHandle::Start;
                    selectStop (0);
                    draggingStopIndex = invalidStopIndex;
                }
                else if (position.distanceTo (end) < 14.0f)
                {
                    dragHandle = DragHandle::End;
                    selectStop (gradientStops.size() - 1);
                    draggingStopIndex = invalidStopIndex;
                }
                else if (hitIndex != invalidStopIndex)
                {
                    dragHandle = DragHandle::Stop;
                    draggingStopIndex = hitIndex;
                    selectStop (hitIndex);
                }
                else
                {
                    dragHandle = DragHandle::None;
                    draggingStopIndex = invalidStopIndex;
                }
            }

            if (dragHandle != DragHandle::None)
            {
                if (dragHandle == DragHandle::Start)
                {
                    gradientStartNorm = normalizedFromPoint (gradientEditorArea, position);
                    updateCustomStopDeltas();
                }
                else if (dragHandle == DragHandle::End)
                {
                    gradientEndNorm = normalizedFromPoint (gradientEditorArea, position);
                    updateCustomStopDeltas();
                }
                else if (dragHandle == DragHandle::Stop && draggingStopIndex != invalidStopIndex)
                {
                    if (draggingStopIndex > 0 && draggingStopIndex + 1 < gradientStops.size())
                    {
                        const auto start = pointFromNormalized (gradientEditorArea, gradientStartNorm);
                        const auto end = pointFromNormalized (gradientEditorArea, gradientEndNorm);
                        const auto normalized = normalizedFromPoint (gradientEditorArea, position);
                        const auto clampedPosition = pointFromNormalized (gradientEditorArea, normalized);
                        const bool constrainToLine = modifiers.isShiftDown();

                        auto& stop = gradientStops[draggingStopIndex];
                        if (constrainToLine)
                        {
                            stop.hasCustomPosition = false;
                        }
                        else
                        {
                            stop.customPositionNorm = normalized;
                            stop.hasCustomPosition = true;
                        }

                        const float minT = gradientStops[draggingStopIndex - 1].t + 0.001f;
                        const float maxT = gradientStops[draggingStopIndex + 1].t - 0.001f;
                        stop.t = yup::jlimit (minT, maxT, projectPositionToT (clampedPosition, start, end));
                    }
                }

                repaint();
            }
        }
        else if (currentMode == Mode::Picker)
        {
            if (! pickerWheelArea.contains (position))
                return;

            const auto center = yup::Point<float> (pickerWheelArea.getCenterX(), pickerWheelArea.getCenterY());
            const float dx = position.getX() - center.getX();
            const float dy = position.getY() - center.getY();
            const float radius = pickerWheelArea.getWidth() * 0.5f;
            const float distance = std::sqrt (dx * dx + dy * dy);
            pickerSaturation = yup::jlimit (0.0f, 1.0f, distance / radius);
            pickerHue = std::fmod ((std::atan2 (dy, dx) / (2.0f * yup::MathConstants<float>::pi) + 1.0f), 1.0f);
            pickerColor = yup::Color::fromHSV (pickerHue, pickerSaturation, pickerLuminance).withAlpha (pickerAlpha);
            updatePickerSlidersFromColor();
            repaint();
        }
    }

    void handleChannelSliderChange (size_t index, float value)
    {
        if (updatingSliders)
            return;

        channelValues[index] = value;
        updateColorFromPickerSliders();
    }

    void updateColorFromPickerSliders()
    {
        updatingSliders = true;

        switch (pickerSpace)
        {
            case PickerSpace::Rgba:
                pickerColor = colorFromNormalized (channelValues[0], channelValues[1], channelValues[2], channelValues[3]);
                break;
            case PickerSpace::Hsl:
                pickerColor = yup::Color::fromHSL (channelValues[0], channelValues[1], channelValues[2], channelValues[3]);
                break;
            case PickerSpace::Hsv:
                pickerColor = yup::Color::fromHSV (channelValues[0], channelValues[1], channelValues[2], channelValues[3]);
                break;
            case PickerSpace::Hsluv:
                pickerColor = yup::Color::fromHSLuv (channelValues[0], channelValues[1], channelValues[2], channelValues[3]);
                break;
        }

        updatePickerDerivedValues();
        updateChannelLabels();
        updatingSliders = false;
        repaint();
    }

    void updatePickerDerivedValues()
    {
        const auto hsv = pickerColor.toHSV();
        pickerHue = std::get<0> (hsv);
        pickerSaturation = std::get<1> (hsv);
        pickerLuminance = std::get<2> (hsv);
        pickerAlpha = pickerColor.getAlphaFloat();
    }

    void updatePickerSlidersFromColor()
    {
        updatingSliders = true;

        updatePickerDerivedValues();

        switch (pickerSpace)
        {
            case PickerSpace::Rgba:
                channelValues = { pickerColor.getRedFloat(), pickerColor.getGreenFloat(), pickerColor.getBlueFloat(), pickerColor.getAlphaFloat() };
                break;
            case PickerSpace::Hsl:
            {
                const auto hsl = pickerColor.toHSL();
                channelValues = { std::get<0> (hsl), std::get<1> (hsl), std::get<2> (hsl), pickerColor.getAlphaFloat() };
                break;
            }
            case PickerSpace::Hsv:
            {
                const auto hsv = pickerColor.toHSV();
                channelValues = { std::get<0> (hsv), std::get<1> (hsv), std::get<2> (hsv), pickerColor.getAlphaFloat() };
                break;
            }
            case PickerSpace::Hsluv:
            {
                const auto hsluv = pickerColor.toHSLuv();
                channelValues = { std::get<0> (hsluv), std::get<1> (hsluv), std::get<2> (hsluv), pickerColor.getAlphaFloat() };
                break;
            }
        }

        for (size_t i = 0; i < channelSliders.size(); ++i)
            channelSliders[i]->setValue (channelValues[i]);

        updateChannelLabels();
        updatingSliders = false;
    }

    void updateChannelLabels()
    {
        switch (pickerSpace)
        {
            case PickerSpace::Rgba:
                channelLabels[0]->setText ("R " + yup::String (yup::roundToInt (channelValues[0] * 255.0f)), yup::dontSendNotification);
                channelLabels[1]->setText ("G " + yup::String (yup::roundToInt (channelValues[1] * 255.0f)), yup::dontSendNotification);
                channelLabels[2]->setText ("B " + yup::String (yup::roundToInt (channelValues[2] * 255.0f)), yup::dontSendNotification);
                channelLabels[3]->setText ("A " + formatValue (channelValues[3]), yup::dontSendNotification);
                break;
            case PickerSpace::Hsl:
                channelLabels[0]->setText ("H " + formatValue (channelValues[0]), yup::dontSendNotification);
                channelLabels[1]->setText ("S " + formatValue (channelValues[1]), yup::dontSendNotification);
                channelLabels[2]->setText ("L " + formatValue (channelValues[2]), yup::dontSendNotification);
                channelLabels[3]->setText ("A " + formatValue (channelValues[3]), yup::dontSendNotification);
                break;
            case PickerSpace::Hsv:
                channelLabels[0]->setText ("H " + formatValue (channelValues[0]), yup::dontSendNotification);
                channelLabels[1]->setText ("S " + formatValue (channelValues[1]), yup::dontSendNotification);
                channelLabels[2]->setText ("V " + formatValue (channelValues[2]), yup::dontSendNotification);
                channelLabels[3]->setText ("A " + formatValue (channelValues[3]), yup::dontSendNotification);
                break;
            case PickerSpace::Hsluv:
                channelLabels[0]->setText ("H " + formatValue (channelValues[0]), yup::dontSendNotification);
                channelLabels[1]->setText ("S " + formatValue (channelValues[1]), yup::dontSendNotification);
                channelLabels[2]->setText ("L " + formatValue (channelValues[2]), yup::dontSendNotification);
                channelLabels[3]->setText ("A " + formatValue (channelValues[3]), yup::dontSendNotification);
                break;
        }
    }

    void updateBlendSlidersFromColors()
    {
        updatingBlendSliders = true;
        blendStartValues = { blendStartColor.getRedFloat(), blendStartColor.getGreenFloat(), blendStartColor.getBlueFloat(), blendStartColor.getAlphaFloat() };
        blendEndValues = { blendEndColor.getRedFloat(), blendEndColor.getGreenFloat(), blendEndColor.getBlueFloat(), blendEndColor.getAlphaFloat() };

        for (size_t i = 0; i < blendStartSliders.size(); ++i)
        {
            blendStartSliders[i]->setValue (blendStartValues[i]);
            blendEndSliders[i]->setValue (blendEndValues[i]);
        }

        updateBlendLabels();
        updatingBlendSliders = false;
    }

    void updateBlendColorsFromSliders()
    {
        if (updatingBlendSliders)
            return;

        blendStartColor = colorFromNormalized (blendStartValues[0], blendStartValues[1], blendStartValues[2], blendStartValues[3]);
        blendEndColor = colorFromNormalized (blendEndValues[0], blendEndValues[1], blendEndValues[2], blendEndValues[3]);
        updateBlendLabels();
        repaint();
    }

    void updateBlendLabels()
    {
        for (size_t i = 0; i < blendStartLabels.size(); ++i)
        {
            const yup::String startPrefix = i == 0 ? "Start R " : i == 1 ? "Start G "
                                                            : i == 2     ? "Start B "
                                                                         : "Start A ";
            const yup::String endPrefix = i == 0 ? "End R " : i == 1 ? "End G "
                                                        : i == 2     ? "End B "
                                                                     : "End A ";

            if (i < 3)
            {
                blendStartLabels[i]->setText (startPrefix + yup::String (yup::roundToInt (blendStartValues[i] * 255.0f)), yup::dontSendNotification);
                blendEndLabels[i]->setText (endPrefix + yup::String (yup::roundToInt (blendEndValues[i] * 255.0f)), yup::dontSendNotification);
            }
            else
            {
                blendStartLabels[i]->setText (startPrefix + yup::String (blendStartValues[i], 2), yup::dontSendNotification);
                blendEndLabels[i]->setText (endPrefix + yup::String (blendEndValues[i], 2), yup::dontSendNotification);
            }
        }
    }

    void updateGradientStopSlidersFromSelection()
    {
        if (gradientStops.empty())
            return;

        if (selectedStopIndex >= gradientStops.size())
            selectedStopIndex = gradientStops.size() - 1;

        if (removeStopButton)
            removeStopButton->setEnabled (gradientStops.size() > 2);

        updatingStopSliders = true;
        const auto& color = gradientStops[selectedStopIndex].color;
        stopColorValues = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat(), color.getAlphaFloat() };

        for (size_t i = 0; i < stopColorSliders.size(); ++i)
            stopColorSliders[i]->setValue (stopColorValues[i]);

        updateStopLabels();
        updatingStopSliders = false;
    }

    void updateSelectedStopColor()
    {
        if (updatingStopSliders || gradientStops.empty())
            return;

        gradientStops[selectedStopIndex].color = colorFromNormalized (stopColorValues[0], stopColorValues[1], stopColorValues[2], stopColorValues[3]);
        updateGradientEndpointsFromStops();
        updateStopLabels();
        repaint();
    }

    void updateStopLabels()
    {
        for (size_t i = 0; i < stopColorLabels.size(); ++i)
        {
            const yup::String prefix = i == 0 ? "Stop R " : i == 1 ? "Stop G "
                                                      : i == 2     ? "Stop B "
                                                                   : "Stop A ";
            if (i < 3)
                stopColorLabels[i]->setText (prefix + yup::String (yup::roundToInt (stopColorValues[i] * 255.0f)), yup::dontSendNotification);
            else
                stopColorLabels[i]->setText (prefix + yup::String (stopColorValues[i], 2), yup::dontSendNotification);
        }
    }

    void updateGradientEndpointsFromStops()
    {
        if (gradientStops.size() < 2)
            return;

        gradientStartColor = gradientStops.front().color;
        gradientEndColor = gradientStops.back().color;
    }

    void addGradientStop()
    {
        if (gradientStops.empty())
            return;

        size_t insertIndex = 0;
        float t = 0.5f;
        const auto size = gradientStops.size();

        if (selectedStopIndex < size - 1)
        {
            const auto& a = gradientStops[selectedStopIndex];
            const auto& b = gradientStops[selectedStopIndex + 1];
            t = (a.t + b.t) * 0.5f;
            insertIndex = selectedStopIndex + 1;
        }
        else if (selectedStopIndex > 0)
        {
            const auto& a = gradientStops[selectedStopIndex - 1];
            const auto& b = gradientStops[selectedStopIndex];
            t = (a.t + b.t) * 0.5f;
            insertIndex = selectedStopIndex;
        }

        GradientStopData newStop;
        newStop.t = t;
        newStop.color = gradientStops[selectedStopIndex].color;

        gradientStops.insert (gradientStops.begin() + static_cast<long> (insertIndex), newStop);
        sortGradientStops();
        selectedStopIndex = findStopIndexByT (t);
        updateGradientStopSlidersFromSelection();
        repaint();
    }

    void removeSelectedStop()
    {
        if (gradientStops.size() <= 2 || selectedStopIndex >= gradientStops.size())
            return;

        gradientStops.erase (gradientStops.begin() + static_cast<long> (selectedStopIndex));
        if (selectedStopIndex >= gradientStops.size())
            selectedStopIndex = gradientStops.size() - 1;

        updateGradientEndpointsFromStops();
        updateGradientStopSlidersFromSelection();
        repaint();
    }

    void sortGradientStops()
    {
        std::sort (gradientStops.begin(), gradientStops.end(), [] (const auto& a, const auto& b)
        {
            return a.t < b.t;
        });

        if (! gradientStops.empty())
        {
            gradientStops.front().t = 0.0f;
            gradientStops.front().hasCustomPosition = false;
            gradientStops.back().t = 1.0f;
            gradientStops.back().hasCustomPosition = false;
        }

        updateGradientEndpointsFromStops();
    }

    size_t findStopIndexByT (float t) const
    {
        for (size_t i = 0; i < gradientStops.size(); ++i)
        {
            if (std::abs (gradientStops[i].t - t) < 0.0001f)
                return i;
        }

        return gradientStops.empty() ? 0 : gradientStops.size() - 1;
    }

    static constexpr size_t invalidStopIndex = std::numeric_limits<size_t>::max();

    Mode currentMode = Mode::BlendSpaces;
    PickerSpace pickerSpace = PickerSpace::Hsluv;
    DragHandle dragHandle = DragHandle::None;
    size_t draggingStopIndex = invalidStopIndex;

    yup::Rectangle<float> headerArea;
    yup::Rectangle<float> contentArea;
    yup::Rectangle<float> gradientEditorArea;
    yup::Rectangle<float> pickerWheelArea;

    std::unique_ptr<yup::ComboBox> modeCombo;
    std::unique_ptr<yup::ComboBox> gradientTypeCombo;
    std::unique_ptr<yup::ComboBox> pickerSpaceCombo;
    std::array<std::unique_ptr<yup::Slider>, 4> channelSliders;
    std::array<std::unique_ptr<yup::Label>, 4> channelLabels;
    std::array<std::unique_ptr<yup::Slider>, 4> blendStartSliders;
    std::array<std::unique_ptr<yup::Label>, 4> blendStartLabels;
    std::array<std::unique_ptr<yup::Slider>, 4> blendEndSliders;
    std::array<std::unique_ptr<yup::Label>, 4> blendEndLabels;
    std::unique_ptr<yup::TextButton> addStopButton;
    std::unique_ptr<yup::TextButton> removeStopButton;
    std::array<std::unique_ptr<yup::Slider>, 4> stopColorSliders;
    std::array<std::unique_ptr<yup::Label>, 4> stopColorLabels;

    bool updatingSliders = false;
    std::array<float, 4> channelValues { 0.5f, 0.5f, 0.5f, 1.0f };
    bool updatingBlendSliders = false;
    std::array<float, 4> blendStartValues { 1.0f, 0.2f, 0.2f, 1.0f };
    std::array<float, 4> blendEndValues { 0.2f, 0.4f, 1.0f, 1.0f };
    bool updatingStopSliders = false;
    std::array<float, 4> stopColorValues { 1.0f, 0.2f, 0.2f, 1.0f };

    yup::Color pickerColor;
    float pickerHue = 0.1f;
    float pickerSaturation = 0.8f;
    float pickerLuminance = 0.55f;
    float pickerAlpha = 1.0f;

    yup::Color blendStartColor = yup::Color::fromHSLuv (0.08f, 0.85f, 0.6f);
    yup::Color blendEndColor = yup::Color::fromHSLuv (0.7f, 0.8f, 0.45f);
    yup::Color gradientStartColor = blendStartColor;
    yup::Color gradientEndColor = blendEndColor;
    bool isGradientRadial = false;
    yup::Point<float> gradientStartNorm { 0.2f, 0.3f };
    yup::Point<float> gradientEndNorm { 0.8f, 0.7f };
    std::vector<GradientStopData> gradientStops;
    size_t selectedStopIndex = 0;
};
