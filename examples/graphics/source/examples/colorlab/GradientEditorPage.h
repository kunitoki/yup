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

namespace colorlab
{

//==============================================================================
/**
    Edits a linear or radial gradient: drag its end points A and B, drag the stops, and
    select a stop to change its color. Stops leave the A-B line when dragged away from it,
    hold Shift to keep them on it.
*/
class GradientEditorPage final : public yup::Component
{
public:
    GradientEditorPage()
        : yup::Component ("gradientEditorPage")
    {
        setOpaque (false);

        typeCombo.addItem ("Linear", 1);
        typeCombo.addItem ("Radial", 2);
        typeCombo.setSelectedId (1, yup::dontSendNotification);
        typeCombo.onSelectedItemChanged = [this]
        {
            repaint();
        };
        addAndMakeVisible (typeCombo);

        addStopButton.setButtonText ("Add stop");
        addStopButton.onClick = [this]
        {
            addStop();
        };
        addAndMakeVisible (addStopButton);

        removeStopButton.setButtonText ("Remove stop");
        removeStopButton.onClick = [this]
        {
            removeSelectedStop();
        };
        addAndMakeVisible (removeStopButton);

        stopEditor.onColorChanged = [this] (yup::Color newColor)
        {
            stops[selectedStop].color = newColor;
            repaint();
        };
        addAndMakeVisible (stopEditor);

        stops.push_back ({ 0.0f, yup::Color::fromHSLuv (0.08f, 0.85f, 0.6f) });
        stops.push_back ({ 1.0f, yup::Color::fromHSLuv (0.7f, 0.8f, 0.45f) });
        selectStop (0);
    }

    void paint (yup::Graphics& g) override
    {
        drawPanel (g, canvasPanel);

        if (canvas.isEmpty())
            return;

        const auto start = getStartPoint();
        const auto end = getEndPoint();
        const auto isRadial = typeCombo.getSelectedId() == 2;

        std::vector<yup::ColorGradient::ColorStop> gradientStops;
        for (size_t i = 0; i < stops.size(); ++i)
            gradientStops.emplace_back (stops[i].color, getStopPosition (i), stops[i].t);

        g.setFillColorGradient (yup::ColorGradient (isRadial ? yup::ColorGradient::Radial : yup::ColorGradient::Linear, gradientStops));
        g.fillRoundedRect (canvas, 10.0f);

        g.setStrokeColor (yup::Colors::white.withAlpha (0.4f));
        g.setStrokeWidth (1.0f);
        if (isRadial)
        {
            const auto radius = start.distanceTo (end);
            g.strokeEllipse (yup::Rectangle<float> (0.0f, 0.0f, radius * 2.0f, radius * 2.0f).withCenter (start));
        }
        else
        {
            g.strokeLine (start, end);
        }

        drawHandle (g, start, "A");
        drawHandle (g, end, "B");

        for (size_t i = 0; i < stops.size(); ++i)
        {
            const auto radius = i == selectedStop ? 7.0f : 5.0f;
            const auto area = yup::Rectangle<float> (0.0f, 0.0f, radius * 2.0f, radius * 2.0f).withCenter (getStopPosition (i));

            g.setFillColor (stops[i].color.withAlpha (1.0f));
            g.fillEllipse (area);
            g.setStrokeColor (i == selectedStop ? yup::Colors::white : yup::Colors::black.withAlpha (0.4f));
            g.setStrokeWidth (i == selectedStop ? 2.0f : 1.0f);
            g.strokeEllipse (area);
        }

        g.setFillColor (paletteColor (yup::ThemePalette::Role::textMuted));
        g.fillFittedText ("Drag A, B and the stops. Hold Shift to keep a stop on the A-B line.",
                          themeFont (12.0f),
                          canvasPanel.reduced (16.0f, 8.0f).removeFromBottom (20.0f),
                          yup::Justification::centerLeft);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (12.0f);

        auto controls = bounds.removeFromRight (yup::jmin (300.0f, bounds.getWidth() * 0.45f));
        typeCombo.setBounds (controls.removeFromTop (28.0f));
        controls.removeFromTop (8.0f);

        auto buttons = controls.removeFromTop (28.0f);
        addStopButton.setBounds (buttons.removeFromLeft (buttons.getWidth() * 0.5f).withTrimmedRight (4.0f));
        removeStopButton.setBounds (buttons.withTrimmedLeft (4.0f));
        controls.removeFromTop (12.0f);

        stopEditor.setBounds (controls.removeFromTop (ColorChannelEditor::preferredHeight));

        canvasPanel = bounds.withTrimmedRight (16.0f);
        canvas = canvasPanel.reduced (16.0f).withTrimmedBottom (24.0f);
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        const auto position = event.getPosition();
        dragging = Handle::none;

        if (! canvas.contains (position))
            return;

        if (position.distanceTo (getStartPoint()) < 14.0f)
        {
            dragging = Handle::start;
            selectStop (0);
        }
        else if (position.distanceTo (getEndPoint()) < 14.0f)
        {
            dragging = Handle::end;
            selectStop (stops.size() - 1);
        }
        else
        {
            for (size_t i = 0; i < stops.size(); ++i)
            {
                if (position.distanceTo (getStopPosition (i)) < 12.0f)
                {
                    dragging = Handle::stop;
                    selectStop (i);
                    break;
                }
            }
        }
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        const auto normalized = toNormalized (event.getPosition());

        switch (dragging)
        {
            case Handle::none:
                return;

            case Handle::start:
                startNorm = normalized;
                updateCustomStops();
                break;

            case Handle::end:
                endNorm = normalized;
                updateCustomStops();
                break;

            case Handle::stop:
            {
                // The first and last stops are bound to A and B
                if (selectedStop == 0 || selectedStop + 1 == stops.size())
                    return;

                auto& stop = stops[selectedStop];
                stop.hasCustomPosition = ! event.getModifiers().isShiftDown();
                stop.customPositionNorm = normalized;
                stop.t = clampedT (selectedStop, projectToLine (fromNormalized (normalized)));
                break;
            }
        }

        repaint();
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        dragging = Handle::none;
    }

private:
    enum class Handle
    {
        none,
        start,
        end,
        stop
    };

    struct Stop
    {
        float t = 0.0f;
        yup::Color color;
        yup::Point<float> customPositionNorm;
        bool hasCustomPosition = false;
    };

    yup::Point<float> fromNormalized (yup::Point<float> norm) const
    {
        return { canvas.getX() + norm.getX() * canvas.getWidth(), canvas.getY() + norm.getY() * canvas.getHeight() };
    }

    yup::Point<float> toNormalized (yup::Point<float> point) const
    {
        return { yup::jlimit (0.0f, 1.0f, (point.getX() - canvas.getX()) / canvas.getWidth()),
                 yup::jlimit (0.0f, 1.0f, (point.getY() - canvas.getY()) / canvas.getHeight()) };
    }

    yup::Point<float> getStartPoint() const { return fromNormalized (startNorm); }

    yup::Point<float> getEndPoint() const { return fromNormalized (endNorm); }

    yup::Point<float> getStopPosition (size_t index) const
    {
        const auto& stop = stops[index];
        if (stop.hasCustomPosition)
            return fromNormalized (stop.customPositionNorm);

        return yup::Line<float> (getStartPoint(), getEndPoint()).pointAlong (stop.t);
    }

    float projectToLine (yup::Point<float> position) const
    {
        const auto start = getStartPoint();
        const auto direction = getEndPoint() - start;
        const auto lengthSquared = direction.getX() * direction.getX() + direction.getY() * direction.getY();
        if (lengthSquared <= 0.000001f)
            return 0.0f;

        const auto offset = position - start;
        return yup::jlimit (0.0f, 1.0f, (offset.getX() * direction.getX() + offset.getY() * direction.getY()) / lengthSquared);
    }

    /** Keeps a stop between its neighbors, so the stops stay sorted. */
    float clampedT (size_t index, float t) const
    {
        return yup::jlimit (stops[index - 1].t + 0.001f, stops[index + 1].t - 0.001f, t);
    }

    void updateCustomStops()
    {
        for (size_t i = 1; i + 1 < stops.size(); ++i)
        {
            if (stops[i].hasCustomPosition)
                stops[i].t = clampedT (i, projectToLine (fromNormalized (stops[i].customPositionNorm)));
        }
    }

    void selectStop (size_t index)
    {
        selectedStop = yup::jmin (index, stops.size() - 1);
        stopEditor.setColor (stops[selectedStop].color);
        removeStopButton.setEnabled (stops.size() > 2 && selectedStop > 0 && selectedStop + 1 < stops.size());
        repaint();
    }

    void addStop()
    {
        // Halfway to the next stop, or to the previous one from the last stop
        const auto next = selectedStop + 1 < stops.size() ? selectedStop + 1 : selectedStop;
        const auto previous = next - 1;

        Stop stop;
        stop.t = (stops[previous].t + stops[next].t) * 0.5f;
        stop.color = stops[previous].color.mixedWith (stops[next].color, 0.5f);

        stops.insert (stops.begin() + (std::ptrdiff_t) next, stop);
        selectStop (next);
    }

    void removeSelectedStop()
    {
        if (stops.size() <= 2 || selectedStop == 0 || selectedStop + 1 >= stops.size())
            return;

        stops.erase (stops.begin() + (std::ptrdiff_t) selectedStop);
        selectStop (selectedStop - 1);
    }

    void drawHandle (yup::Graphics& g, yup::Point<float> point, const yup::String& label)
    {
        const auto area = yup::Rectangle<float> (0.0f, 0.0f, 12.0f, 12.0f).withCenter (point);

        g.setFillColor (yup::Colors::white.withAlpha (0.9f));
        g.fillEllipse (area);
        g.setStrokeColor (yup::Colors::black.withAlpha (0.4f));
        g.setStrokeWidth (1.0f);
        g.strokeEllipse (area);

        g.setFillColor (yup::Colors::white);
        g.fillFittedText (label, themeFont (11.0f), area.withX (area.getRight() + 2.0f).withWidth (18.0f), yup::Justification::centerLeft);
    }

    std::vector<Stop> stops;
    size_t selectedStop = 0;
    Handle dragging = Handle::none;
    yup::Point<float> startNorm { 0.2f, 0.3f };
    yup::Point<float> endNorm { 0.8f, 0.7f };
    yup::Rectangle<float> canvasPanel, canvas;

    yup::ComboBox typeCombo { "gradientType" };
    yup::TextButton addStopButton { "addStop" };
    yup::TextButton removeStopButton { "removeStop" };
    ColorChannelEditor stopEditor { "Selected stop" };
};

} // namespace colorlab
