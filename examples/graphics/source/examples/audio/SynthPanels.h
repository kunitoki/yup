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

#include "SynthEngine.h"

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

//==============================================================================
/** The palette the synthesiser panels share.

    The example draws its own chrome rather than leaning on the theme, so that the
    oscillator, envelope and display panels read as one instrument.
*/
namespace SynthTheme
{
inline constexpr yup::Color windowBackground { 0xff16191d };
inline constexpr yup::Color panelBackground { 0xff21262c };
inline constexpr yup::Color panelBorder { 0xff2e353d };
inline constexpr yup::Color displayBackground { 0xff0e1114 };
inline constexpr yup::Color accent { 0xff72ead2 };
inline constexpr yup::Color accentDim { 0xff287f78 };
inline constexpr yup::Color textPrimary { 0xffe6ebf0 };
inline constexpr yup::Color textSecondary { 0xff8b96a0 };

constexpr float panelCorner = 6.0f;
} // namespace SynthTheme

/** @internal Paints the rounded frame every panel of the instrument sits in. */
inline void paintSynthPanel (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    g.setFillColor (SynthTheme::panelBackground);
    g.fillRoundedRect (bounds, SynthTheme::panelCorner);

    g.setStrokeColor (SynthTheme::panelBorder);
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), SynthTheme::panelCorner);
}

//==============================================================================
/** Text for a knob's value while it is being dragged. */
namespace SynthFormat
{
inline yup::String plain (double value, int decimals) { return yup::String (value, decimals); }
inline yup::String hertz (double value) { return value >= 1000.0 ? yup::String (value / 1000.0, 2) + " kHz" : yup::String (value, value < 100.0 ? 2 : 0) + " Hz"; }
inline yup::String percent (double value) { return yup::String (static_cast<int> (std::round (value * 100.0))) + " %"; }
inline yup::String cents (double value) { return yup::String (static_cast<int> (std::round (value))) + " ct"; }
inline yup::String octaves (double value) { return yup::String (static_cast<int> (value)) + " oct"; }
inline yup::String milliseconds (double seconds) { return seconds >= 1.0 ? yup::String (seconds, 2) + " s" : yup::String (static_cast<int> (std::round (seconds * 1000.0))) + " ms"; }
inline yup::String ratio (double value) { return yup::String (value, 2) + " x"; }
inline yup::String count (double value) { return yup::String (static_cast<int> (value)); }
inline yup::String bipolar (double value) { return yup::String (value >= 0.0 ? "+" : "") + yup::String (static_cast<int> (std::round (value * 100.0))) + " %"; }
} // namespace SynthFormat

//==============================================================================
/** A rotary knob with its caption underneath, which shows the value while dragging. */
class KnobControl : public yup::Component
{
public:
    KnobControl (const yup::String& caption,
                 double minimum,
                 double maximum,
                 double interval,
                 double defaultValue,
                 const yup::Font& font)
        : slider (yup::Slider::RotaryVerticalDrag)
    {
        setOpaque (false); // the knob and its caption paint themselves, the row draws nothing

        slider.setRange (minimum, maximum, interval);
        slider.setDefaultValue (defaultValue);
        slider.setValue (defaultValue, yup::dontSendNotification);
        slider.setColor (yup::Slider::Style::backgroundColorId, SynthTheme::displayBackground);
        slider.setColor (yup::Slider::Style::trackColorId, SynthTheme::accent);
        slider.setColor (yup::Slider::Style::thumbColorId, SynthTheme::textPrimary);
        slider.setColor (yup::Slider::Style::thumbOverColorId, SynthTheme::accent);
        slider.setColor (yup::Slider::Style::thumbDownColorId, SynthTheme::accent);
        slider.onValueChanged = [this] (double value)
        {
            if (onChange != nullptr)
                onChange (value);

            if (showingValue)
                label.setText (formatCurrentValue(), yup::dontSendNotification);
        };
        slider.onDragStart = [this] (const yup::MouseEvent&)
        {
            showingValue = true;
            label.setText (formatCurrentValue(), yup::dontSendNotification);
        };
        slider.onDragEnd = [this] (const yup::MouseEvent&)
        {
            showingValue = false;
            label.setText (captionText, yup::dontSendNotification);
        };
        addAndMakeVisible (slider);

        captionText = caption;
        label.setText (caption, yup::dontSendNotification);
        label.setFont (font);
        label.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary);
        addAndMakeVisible (label);
    }

    /** Called with the new knob value. */
    std::function<void (double)> onChange;

    /** Turns the value into the text shown while dragging; decimals from the interval when unset. */
    std::function<yup::String (double)> formatValue;

    yup::Slider& getSlider() noexcept { return slider; }

    void resized() override
    {
        auto bounds = getLocalBounds();

        label.setBounds (bounds.removeFromBottom (captionHeight));

        const auto size = yup::jmin (bounds.getWidth(), bounds.getHeight());

        slider.setBounds (bounds.withSizeKeepingCenter (size, size));
    }

private:
    static constexpr float captionHeight = 13.0f;

    yup::String formatCurrentValue() const
    {
        const auto value = slider.getValue();

        if (formatValue != nullptr)
            return formatValue (value);

        const auto interval = slider.getInterval();
        const auto decimals = interval >= 1.0 ? 0 : interval >= 0.1 ? 1 : interval >= 0.01 ? 2 : 3;

        return SynthFormat::plain (value, decimals);
    }

    yup::Slider slider;
    yup::Label label;
    yup::String captionText;
    bool showingValue = false;
};

//==============================================================================
/** A combo box with its caption above it. */
class ChoiceControl : public yup::Component
{
public:
    ChoiceControl (const yup::String& caption, const yup::StringArray& items, const yup::Font& font)
    {
        setOpaque (false); // the caption and combo box paint themselves, the row draws nothing

        label.setText (caption, yup::dontSendNotification);
        label.setFont (font);
        label.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary);
        addAndMakeVisible (label);

        comboBox.addItemList (items, 1);
        comboBox.setTextWhenNothingSelected ("-");
        comboBox.setColor (yup::ComboBox::Style::backgroundColorId, SynthTheme::displayBackground);
        comboBox.setColor (yup::ComboBox::Style::textColorId, SynthTheme::textPrimary);
        comboBox.setColor (yup::ComboBox::Style::borderColorId, SynthTheme::panelBorder);
        comboBox.setColor (yup::ComboBox::Style::arrowColorId, SynthTheme::accent);
        comboBox.onSelectedItemChanged = [this]
        {
            if (onChange != nullptr)
                onChange (comboBox.getSelectedId());
        };
        addAndMakeVisible (comboBox);
    }

    /** Called with the 1-based identifier of the newly selected item. */
    std::function<void (int)> onChange;

    yup::ComboBox& getComboBox() noexcept { return comboBox; }

    void resized() override
    {
        auto bounds = getLocalBounds();

        label.setBounds (bounds.removeFromTop (captionHeight));
        comboBox.setBounds (bounds);
    }

private:
    static constexpr float captionHeight = 13.0f;

    yup::Label label;
    yup::ComboBox comboBox;
};

//==============================================================================
/** Sums a Fourier series at one point of its period.

    yup::FourierSeries stores coefficients rather than samples, so the waveform display
    reconstructs them on demand. Only the message thread calls this.

    @param series      The coefficients to sum
    @param phase       The position in the period, normalized to 0 to 1
    @param maxHarmonic The highest harmonic to include

    @returns The value of the series at that phase.
*/
inline float evaluateFourierSeries (const yup::FourierSeries<double>& series, double phase, int maxHarmonic) noexcept
{
    const auto count = yup::jmin (maxHarmonic, series.getNumHarmonics());
    const auto theta = yup::MathConstants<double>::twoPi * phase;

    auto value = series.getDC();

    for (int harmonic = 1; harmonic <= count; ++harmonic)
    {
        const auto angle = theta * harmonic;

        value += series.getCosine (harmonic) * std::cos (angle)
               + series.getSine (harmonic) * std::sin (angle);
    }

    return static_cast<float> (value);
}

//==============================================================================
/** The waveform of one oscillator, either drawn or edited a partial at a time.

    In drawing mode the component reconstructs the series and shows one period of it.
    In editing mode it shows the magnitude of each harmonic as a bar that can be
    dragged, which is what actually defines the waveform the oscillator renders.

    Dragging writes straight into the settings, but the generation counter the audio
    thread watches is only bumped by commitPendingEdits(), once per user interface
    frame. Without that, one drag would queue an inverse FFT per mouse event on every
    sounding voice.

    @see SynthOscillatorSettings
*/
class WaveformEditor : public yup::Component
{
public:
    WaveformEditor (SynthOscillatorSettings& settingsToEdit,
                    const SynthOscillatorResources& sharedResources)
        : settings (settingsToEdit)
        , resources (sharedResources)
    {
        preview.prepare();
        displaySeries.resize (SynthExample::maxHarmonics);
        displaySamples.assign (displayResolution, 0.0f);

        refresh();
    }

    /** Called whenever a drag changed the partials, so the panel can follow along. */
    std::function<void()> onPartialsChanged;

    /** Switches between drawing the waveform and editing its partials. */
    void setEditingPartials (bool shouldEdit)
    {
        editingPartials = shouldEdit;
        repaint();
    }

    bool isEditingPartials() const noexcept { return editingPartials; }

    /** Rereads the series from the settings, following a preset or randomize change. */
    void refresh()
    {
        const auto usesCustomSeries = settings.usesCustomSeries.load();

        if (usesCustomSeries)
            settings.copyHarmonicsInto (displaySeries, 1.0f);
        else
            displaySeries.copyFrom (resources.getFrame (static_cast<yup::Waveform> (settings.waveform.load())));

        reconstruct (displaySeries);

        // The reconstruction is measured here, on the message thread, so the audio thread
        // never has to work out how loud an edited spectrum turned out to be. This is the
        // source's peak, which is a different quantity from the drawn waveform's below.
        if (usesCustomSeries)
        {
            const auto sourcePeak = measurePeak();

            if (sourcePeak > 1.0e-6f)
                settings.harmonicScale.store (1.0f / sourcePeak);
        }

        // Derived exactly as the voices derive theirs, so the preview cannot drift from
        // what is played.
        preview.invalidate();
        preview.update (settings.read(), settings, resources);
        reconstruct (preview.getSeries());

        // Normalize whatever is actually drawn. The shaper preserves the coefficient sum
        // rather than the peak, and a sum bounds a peak from well above - three times over
        // for a sawtooth - so scaling the derived waveform by the source's peak would draw
        // it clean outside the display.
        const auto peak = measurePeak();

        if (peak > 1.0e-6f)
        {
            const auto scale = 1.0f / peak;

            for (auto& sample : displaySamples)
                sample *= scale;
        }

        repaint();
    }

    /** Sums a series into the display buffer at the display's resolution. */
    void reconstruct (const yup::FourierSeries<double>& series) noexcept
    {
        for (int index = 0; index < displayResolution; ++index)
        {
            const auto phase = static_cast<double> (index) / static_cast<double> (displayResolution - 1);

            displaySamples[static_cast<std::size_t> (index)] =
                evaluateFourierSeries (series, phase, SynthExample::displayHarmonics);
        }
    }

    /** Returns the largest magnitude currently in the display buffer. */
    float measurePeak() const noexcept
    {
        auto peak = 0.0f;

        for (auto sample : displaySamples)
            peak = yup::jmax (peak, std::abs (sample));

        return peak;
    }

    /** Publishes a pending drag to the audio thread, coalescing a frame's worth of edits. */
    void commitPendingEdits()
    {
        if (! pendingEdit)
            return;

        pendingEdit = false;
        settings.harmonicGeneration.fetch_add (1);
    }

    /** Drops the edited partials and returns to the selected waveform preset. */
    void revertToPreset()
    {
        settings.usesCustomSeries.store (false);
        pendingEdit = true;

        refresh();

        if (onPartialsChanged != nullptr)
            onPartialsChanged();
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds();

        g.setFillColor (SynthTheme::displayBackground);
        g.fillRoundedRect (bounds, 4.0f);

        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);

        if (editingPartials)
            paintPartials (g, bounds.reduced (contentInset));
        else
            paintWaveform (g, bounds.reduced (contentInset));
    }

    void mouseDown (const yup::MouseEvent& event) override { applyEdit (event); }

    void mouseDrag (const yup::MouseEvent& event) override { applyEdit (event); }

private:
    //==============================================================================
    /** Draws one period of the reconstructed series. */
    void paintWaveform (yup::Graphics& g, yup::Rectangle<float> bounds)
    {
        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeLine (bounds.getX(), bounds.getCenterY(), bounds.getRight(), bounds.getCenterY());

        path.clear();
        path.reserveSpace (displayResolution);

        for (int index = 0; index < displayResolution; ++index)
        {
            const auto x = bounds.getX() + bounds.getWidth() * static_cast<float> (index)
                                             / static_cast<float> (displayResolution - 1);

            const auto y = bounds.getCenterY() - displaySamples[static_cast<std::size_t> (index)] * bounds.getHeight() * 0.45f;

            if (index == 0)
                path.moveTo (x, y);
            else
                path.lineTo (x, y);
        }

        g.setStrokeColor (SynthTheme::accent.withAlpha (0.35f));
        g.setStrokeWidth (4.0f);
        g.setFeather (6.0f);
        g.strokePath (path);

        g.setFeather (0.0f);
        g.setStrokeColor (SynthTheme::accent);
        g.setStrokeWidth (1.5f);
        g.strokePath (path);
    }

    /** Draws the editable magnitude of every harmonic. */
    void paintPartials (yup::Graphics& g, yup::Rectangle<float> bounds)
    {
        const auto barWidth = bounds.getWidth() / static_cast<float> (SynthExample::editableHarmonics);

        for (int index = 0; index < SynthExample::editableHarmonics; ++index)
        {
            const auto magnitude = yup::jlimit (0.0f, 1.0f, static_cast<float> (displaySeries.getMagnitude (index + 1)));
            const auto height = yup::jmax (1.0f, magnitude * bounds.getHeight());
            const auto x = bounds.getX() + barWidth * static_cast<float> (index);

            const yup::Rectangle<float> bar { x + barGap, bounds.getBottom() - height, yup::jmax (1.0f, barWidth - barGap * 2.0f), height };

            g.setFillColor (magnitude > 0.0f ? SynthTheme::accent : SynthTheme::panelBorder);
            g.fillRect (bar);
        }
    }

    //==============================================================================
    /** Turns a mouse position into the magnitude of one harmonic. */
    void applyEdit (const yup::MouseEvent& event)
    {
        if (! editingPartials)
            return;

        const auto bounds = getLocalBounds().reduced (contentInset);

        if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
            return;

        // Editing a preset copies its partials in first, so the drag starts from the
        // shape that is on screen instead of from silence.
        if (! settings.usesCustomSeries.load())
        {
            settings.seedHarmonicsFrom (displaySeries);
            settings.usesCustomSeries.store (true);
        }

        const auto position = event.getPosition();
        const auto barWidth = bounds.getWidth() / static_cast<float> (SynthExample::editableHarmonics);
        const auto index = yup::jlimit (0,
                                        SynthExample::editableHarmonics - 1,
                                        static_cast<int> ((position.getX() - bounds.getX()) / barWidth));

        const auto magnitude = yup::jlimit (0.0f, 1.0f, (bounds.getBottom() - position.getY()) / bounds.getHeight());

        settings.harmonics[static_cast<std::size_t> (index)].store (magnitude);
        pendingEdit = true;

        refresh();

        if (onPartialsChanged != nullptr)
            onPartialsChanged();
    }

    //==============================================================================
    static constexpr int displayResolution = 256;
    static constexpr float contentInset = 6.0f;
    static constexpr float barGap = 1.0f;

    SynthOscillatorSettings& settings;
    const SynthOscillatorResources& resources;

    SynthSpectrumDerivation preview;
    yup::FourierSeries<double> displaySeries;
    std::vector<float> displaySamples;
    yup::Path path;

    bool editingPartials = false;
    bool pendingEdit = false;
};

//==============================================================================
/** Draws the most recent block of rendered audio as a waveform. */
class Oscilloscope : public yup::Component
{
public:
    Oscilloscope()
        : Component ("Oscilloscope")
    {
    }

    /** Copies the samples to display. Called from the message thread. */
    void setRenderData (const std::vector<float>& data)
    {
        renderData = data;
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds();

        g.setFillColor (SynthTheme::displayBackground);
        g.fillRoundedRect (bounds, 4.0f);

        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);
        g.strokeLine (bounds.getX(), bounds.getCenterY(), bounds.getRight(), bounds.getCenterY());

        if (renderData.empty())
            return;

        const auto pointCount = yup::jmin (512, static_cast<int> (renderData.size()));
        const auto xSize = bounds.getWidth() / static_cast<float> (yup::jmax (1, pointCount - 1));

        path.clear();
        path.reserveSpace (pointCount);
        path.moveTo (bounds.getX(), bounds.getCenterY() - renderData[0] * bounds.getHeight() * 0.45f);

        for (int i = 1; i < pointCount; ++i)
        {
            const auto sample = static_cast<std::size_t> (i) * (renderData.size() - 1) / static_cast<std::size_t> (pointCount - 1);
            path.lineTo (bounds.getX() + static_cast<float> (i) * xSize,
                         bounds.getCenterY() - renderData[sample] * bounds.getHeight() * 0.45f);
        }

        filledPath = path.createStrokePolygon (4.0f);

        g.setFillColor (SynthTheme::accent.withAlpha (0.5f));
        g.setFeather (8.0f);
        g.fillPath (filledPath);

        g.setFeather (4.0f);
        g.fillPath (filledPath);

        g.setFeather (0.0f);
        g.setStrokeColor (SynthTheme::accent);
        g.setStrokeWidth (1.5f);
        g.strokePath (path);
    }

private:
    std::vector<float> renderData;
    yup::Path path;
    yup::Path filledPath;
};

//==============================================================================
/** @internal Spreads controls evenly across a row. */
inline void layoutControlsInRow (yup::Rectangle<float> area, const std::vector<yup::Component*>& controls)
{
    if (controls.empty())
        return;

    const auto width = area.getWidth() / static_cast<float> (controls.size());

    for (auto* control : controls)
        control->setBounds (area.removeFromLeft (width).reduced (3.0f, 0.0f));
}

//==============================================================================
/** Draws the shape the amplitude envelope traces, with a point at every breakpoint. */
class EnvelopeDisplay : public yup::Component
{
public:
    /** Updates the drawn shape. Called from the message thread. */
    void setValues (const SynthEnvelopeValues& newValues)
    {
        values = newValues;
        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds();

        g.setFillColor (SynthTheme::displayBackground);
        g.fillRoundedRect (bounds, 4.0f);

        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);

        const auto area = bounds.reduced (8.0f);

        if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
            return;

        // The sustain stage has no duration of its own, so it is given a fixed share of
        // the width and the timed stages share what is left.
        const auto totalSeconds = yup::jmax (1.0e-4f, values.delay + values.attack + values.hold + values.decay + values.release);
        const auto timedWidth = area.getWidth() * (1.0f - sustainShare);
        const auto secondsToPixels = timedWidth / totalSeconds;

        const auto levelToY = [area] (float level)
        {
            return area.getBottom() - yup::jlimit (0.0f, 1.0f, level) * area.getHeight();
        };

        constexpr int numPoints = 7;

        const yup::Point<float> points[numPoints] = {
            { area.getX(), levelToY (0.0f) },
            { area.getX() + values.delay * secondsToPixels, levelToY (0.0f) },
            { area.getX() + (values.delay + values.attack) * secondsToPixels, levelToY (1.0f) },
            { area.getX() + (values.delay + values.attack + values.hold) * secondsToPixels, levelToY (1.0f) },
            { area.getX() + (values.delay + values.attack + values.hold + values.decay) * secondsToPixels, levelToY (values.sustain) },
            { area.getX() + (values.delay + values.attack + values.hold + values.decay) * secondsToPixels + area.getWidth() * sustainShare, levelToY (values.sustain) },
            { area.getRight(), levelToY (0.0f) }
        };

        path.clear();
        path.reserveSpace (numPoints + 2);
        path.moveTo (points[0]);

        for (int index = 1; index < numPoints; ++index)
            path.lineTo (points[index]);

        g.setStrokeColor (SynthTheme::accent);
        g.setStrokeWidth (1.5f);
        g.strokePath (path);

        for (int index = 1; index < numPoints - 1; ++index)
        {
            g.setFillColor (SynthTheme::accent);
            g.fillEllipse (yup::Rectangle<float> (points[index].getX() - pointRadius,
                                                  points[index].getY() - pointRadius,
                                                  pointRadius * 2.0f,
                                                  pointRadius * 2.0f));
        }
    }

private:
    static constexpr float sustainShare = 0.22f;
    static constexpr float pointRadius = 3.0f;

    SynthEnvelopeValues values;
    yup::Path path;
};

//==============================================================================
/** The editing surface of one envelope.

    @see SynthEnvelopeSettings
*/
class SynthEnvelopePanel : public yup::Component
{
public:
    SynthEnvelopePanel (const yup::String& panelTitle, SynthEnvelopeSettings& settingsToEdit, const yup::Font& font)
        : settings (settingsToEdit)
        , delayKnob ("DELAY", 0.0, 2.0, 0.001, 0.0, font)
        , attackKnob ("ATTACK", 0.001, 4.0, 0.001, 0.005, font)
        , holdKnob ("HOLD", 0.0, 2.0, 0.001, 0.0, font)
        , decayKnob ("DECAY", 0.001, 4.0, 0.001, 0.35, font)
        , sustainKnob ("SUSTAIN", 0.0, 1.0, 0.001, 0.7, font)
        , releaseKnob ("RELEASE", 0.001, 8.0, 0.001, 0.35, font)
    {
        titleLabel.setText (panelTitle, yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (12.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        addAndMakeVisible (display);

        for (auto* knob : { &delayKnob, &attackKnob, &holdKnob, &decayKnob, &sustainKnob, &releaseKnob })
            addAndMakeVisible (*knob);

        for (auto* knob : { &delayKnob, &attackKnob, &holdKnob, &decayKnob, &releaseKnob })
            knob->formatValue = SynthFormat::milliseconds;

        sustainKnob.formatValue = SynthFormat::percent;

        delayKnob.onChange = [this] (double value) { settings.delay = static_cast<float> (value); refreshDisplay(); };
        attackKnob.onChange = [this] (double value) { settings.attack = static_cast<float> (value); refreshDisplay(); };
        holdKnob.onChange = [this] (double value) { settings.hold = static_cast<float> (value); refreshDisplay(); };
        decayKnob.onChange = [this] (double value) { settings.decay = static_cast<float> (value); refreshDisplay(); };
        sustainKnob.onChange = [this] (double value) { settings.sustain = static_cast<float> (value); refreshDisplay(); };
        releaseKnob.onChange = [this] (double value) { settings.release = static_cast<float> (value); refreshDisplay(); };

        refresh();
    }

    /** Reads the settings back into the knobs and the drawn shape. */
    void refresh()
    {
        delayKnob.getSlider().setValue (settings.delay.load(), yup::dontSendNotification);
        attackKnob.getSlider().setValue (settings.attack.load(), yup::dontSendNotification);
        holdKnob.getSlider().setValue (settings.hold.load(), yup::dontSendNotification);
        decayKnob.getSlider().setValue (settings.decay.load(), yup::dontSendNotification);
        sustainKnob.getSlider().setValue (settings.sustain.load(), yup::dontSendNotification);
        releaseKnob.getSlider().setValue (settings.release.load(), yup::dontSendNotification);

        refreshDisplay();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (panelInset);

        titleLabel.setBounds (bounds.removeFromTop (headerHeight));
        bounds.removeFromTop (spacing);

        auto knobArea = bounds.removeFromBottom (knobRowHeight);
        bounds.removeFromBottom (spacing);

        display.setBounds (bounds);

        layoutControlsInRow (knobArea, { &delayKnob, &attackKnob, &holdKnob, &decayKnob, &sustainKnob, &releaseKnob });
    }

    void paint (yup::Graphics& g) override
    {
        paintSynthPanel (g, getLocalBounds());
    }

private:
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 16.0f;
    static constexpr float knobRowHeight = 58.0f;
    static constexpr float spacing = 6.0f;

    void refreshDisplay() { display.setValues (settings.read()); }

    SynthEnvelopeSettings& settings;

    yup::Label titleLabel;
    EnvelopeDisplay display;

    KnobControl delayKnob;
    KnobControl attackKnob;
    KnobControl holdKnob;
    KnobControl decayKnob;
    KnobControl sustainKnob;
    KnobControl releaseKnob;
};

//==============================================================================
/** Draws one cycle of an LFO shape with a marker at the engine's current phase. */
class LFODisplay : public yup::Component
{
public:
    /** Updates the drawn shape. Called from the message thread. */
    void setValues (const SynthLFOValues& newValues)
    {
        values = newValues;
        repaint();
    }

    /** Moves the marker. Called every frame from the message thread. */
    void setPhase (float newPhase)
    {
        if (std::abs (newPhase - phase) < 1.0e-3f)
            return;

        phase = newPhase;
        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds();

        g.setFillColor (SynthTheme::displayBackground);
        g.fillRoundedRect (bounds, 4.0f);

        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);

        const auto area = bounds.reduced (8.0f);

        if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
            return;

        g.strokeLine (area.getX(), area.getCenterY(), area.getRight(), area.getCenterY());

        // One period at one sample per point: the shape is read the way the engine reads it.
        yup::LFO<float> shape;
        shape.prepare (static_cast<double> (points));
        shape.setFrequency (1.0f);
        shape.setShape (values.shape);
        shape.setPhaseOffset (values.phase);

        path.clear();
        path.reserveSpace (points + 1);

        for (int index = 0; index <= points; ++index)
        {
            const auto x = area.getX() + area.getWidth() * static_cast<float> (index) / static_cast<float> (points);
            const auto y = area.getCenterY() - shape.processSample() * area.getHeight() * 0.45f;

            if (index == 0)
                path.moveTo (x, y);
            else
                path.lineTo (x, y);
        }

        g.setStrokeColor (SynthTheme::accent);
        g.setStrokeWidth (1.5f);
        g.strokePath (path);

        shape.reset (phase);

        const auto markerX = area.getX() + area.getWidth() * phase;
        const auto markerY = area.getCenterY() - shape.getValue() * area.getHeight() * 0.45f;

        g.setFillColor (SynthTheme::textPrimary);
        g.fillEllipse (yup::Rectangle<float> (markerX - 3.0f, markerY - 3.0f, 6.0f, 6.0f));
    }

private:
    static constexpr int points = 128;

    SynthLFOValues values;
    float phase = 0.0f;
    yup::Path path;
};

//==============================================================================
/** Shape, rate and phase of one LFO, with its display.

    @see SynthLFOSettings
*/
class SynthLFOPanel : public yup::Component
{
public:
    SynthLFOPanel (const yup::String& panelTitle, SynthLFOSettings& settingsToEdit, const yup::Font& font)
        : settings (settingsToEdit)
        , shapeChoice ("SHAPE", getSynthLFOShapeNames(), font)
        , rateKnob ("RATE", 0.01, 20.0, 0.01, 1.0, font)
        , phaseKnob ("PHASE", 0.0, 1.0, 0.001, 0.0, font)
    {
        titleLabel.setText (panelTitle, yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (12.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        addAndMakeVisible (display);
        addAndMakeVisible (shapeChoice);
        addAndMakeVisible (rateKnob);
        addAndMakeVisible (phaseKnob);

        rateKnob.getSlider().setSkewFactorFromMidpoint (1.0);
        rateKnob.formatValue = SynthFormat::hertz;
        phaseKnob.formatValue = SynthFormat::percent;

        shapeChoice.onChange = [this] (int id) { settings.shape = id - 1; refreshDisplay(); };
        rateKnob.onChange = [this] (double value) { settings.rate = static_cast<float> (value); refreshDisplay(); };
        phaseKnob.onChange = [this] (double value) { settings.phase = static_cast<float> (value); refreshDisplay(); };

        refresh();
    }

    /** Reads the settings back into the widgets and the drawn shape. */
    void refresh()
    {
        shapeChoice.getComboBox().setSelectedId (settings.shape.load() + 1, yup::dontSendNotification);
        rateKnob.getSlider().setValue (settings.rate.load(), yup::dontSendNotification);
        phaseKnob.getSlider().setValue (settings.phase.load(), yup::dontSendNotification);

        refreshDisplay();
    }

    /** Moves the display marker to the engine's phase. */
    void setPhase (float phase) { display.setPhase (phase); }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (panelInset);

        titleLabel.setBounds (bounds.removeFromTop (headerHeight));
        bounds.removeFromTop (spacing);

        auto controls = bounds.removeFromRight (controlWidth);
        bounds.removeFromRight (spacing);
        display.setBounds (bounds);

        shapeChoice.setBounds (controls.removeFromTop (choiceHeight));
        controls.removeFromTop (spacing);
        layoutControlsInRow (controls, { &rateKnob, &phaseKnob });
    }

    void paint (yup::Graphics& g) override
    {
        paintSynthPanel (g, getLocalBounds());
    }

private:
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 16.0f;
    static constexpr float choiceHeight = 36.0f;
    static constexpr float controlWidth = 130.0f;
    static constexpr float spacing = 6.0f;

    void refreshDisplay() { display.setValues (settings.read()); }

    SynthLFOSettings& settings;

    yup::Label titleLabel;
    LFODisplay display;
    ChoiceControl shapeChoice;
    KnobControl rateKnob;
    KnobControl phaseKnob;
};

//==============================================================================
/** Type, cutoff, resonance, drive and keytracking of the per-voice filter.

    @see SynthFilterSettings
*/
class SynthFilterPanel : public yup::Component
{
public:
    SynthFilterPanel (SynthFilterSettings& settingsToEdit, const yup::Font& font)
        : settings (settingsToEdit)
        , typeChoice ("TYPE", getSynthFilterTypeNames(), font)
        , cutoffKnob ("CUTOFF", 20.0, 20000.0, 1.0, 8000.0, font)
        , resonanceKnob ("RESO", 0.0, 1.0, 0.001, 0.2, font)
        , driveKnob ("DRIVE", 0.0, 1.0, 0.001, 0.0, font)
        , keytrackKnob ("KEYTRACK", 0.0, 1.0, 0.001, 0.0, font)
    {
        titleLabel.setText ("FILTER", yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (12.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        addAndMakeVisible (typeChoice);

        for (auto* knob : { &cutoffKnob, &resonanceKnob, &driveKnob, &keytrackKnob })
            addAndMakeVisible (*knob);

        cutoffKnob.getSlider().setSkewFactorFromMidpoint (1000.0);
        cutoffKnob.formatValue = SynthFormat::hertz;
        resonanceKnob.formatValue = SynthFormat::percent;
        driveKnob.formatValue = SynthFormat::percent;
        keytrackKnob.formatValue = SynthFormat::percent;

        typeChoice.onChange = [this] (int id) { settings.type = id - 1; };
        cutoffKnob.onChange = [this] (double value) { settings.cutoff = static_cast<float> (value); };
        resonanceKnob.onChange = [this] (double value) { settings.resonance = static_cast<float> (value); };
        driveKnob.onChange = [this] (double value) { settings.drive = static_cast<float> (value); };
        keytrackKnob.onChange = [this] (double value) { settings.keytrack = static_cast<float> (value); };

        refresh();
    }

    /** Reads the settings back into the widgets. */
    void refresh()
    {
        typeChoice.getComboBox().setSelectedId (settings.type.load() + 1, yup::dontSendNotification);
        cutoffKnob.getSlider().setValue (settings.cutoff.load(), yup::dontSendNotification);
        resonanceKnob.getSlider().setValue (settings.resonance.load(), yup::dontSendNotification);
        driveKnob.getSlider().setValue (settings.drive.load(), yup::dontSendNotification);
        keytrackKnob.getSlider().setValue (settings.keytrack.load(), yup::dontSendNotification);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (panelInset);

        titleLabel.setBounds (bounds.removeFromTop (headerHeight));
        bounds.removeFromTop (spacing);

        typeChoice.setBounds (bounds.removeFromTop (choiceHeight));
        bounds.removeFromTop (spacing);

        layoutControlsInRow (bounds, { &cutoffKnob, &resonanceKnob, &driveKnob, &keytrackKnob });
    }

    void paint (yup::Graphics& g) override
    {
        paintSynthPanel (g, getLocalBounds());
    }

private:
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 16.0f;
    static constexpr float choiceHeight = 36.0f;
    static constexpr float spacing = 6.0f;

    SynthFilterSettings& settings;

    yup::Label titleLabel;
    ChoiceControl typeChoice;
    KnobControl cutoffKnob;
    KnobControl resonanceKnob;
    KnobControl driveKnob;
    KnobControl keytrackKnob;
};

//==============================================================================
/** The editing surface of one oscillator, writing straight into the voice settings.

    Every widget is wired to a single atomic setting, and refresh() copies the
    settings back into the widgets for changes coming from somewhere else, such as
    the randomize button.

    @see SynthOscillatorSettings, WaveformEditor
*/
class SynthOscillatorPanel : public yup::Component
{
public:
    SynthOscillatorPanel (const yup::String& panelTitle,
                          SynthOscillatorSettings& settingsToEdit,
                          const SynthOscillatorResources& resources,
                          const yup::Font& font)
        : settings (settingsToEdit)
        , editor (settingsToEdit, resources)
        , waveformChoice ("WAVEFORM", getSynthWaveformNames(), font)
        , syncModeChoice ("SYNC", getSynthSyncModeNames(), font)
        , levelKnob ("LEVEL", 0.0, 1.0, 0.001, 0.5, font)
        , octaveKnob ("OCTAVE", -3.0, 3.0, 1.0, 0.0, font)
        , detuneKnob ("CENTS", -100.0, 100.0, 1.0, 0.0, font)
        , ridgesKnob ("RIDGES", 0.25, 8.0, 0.01, 1.5, font)
        , colorKnob ("COLOR", 0.0, 1.0, 0.001, 0.0, font)
        , dispersionKnob ("DISPERSION", 0.01, 0.99, 0.001, 0.5, font)
        , squeezeKnob ("SQUEEZE", 0.0, 0.5, 0.001, 0.0, font)
        , squashKnob ("SQUASH", 0.1, 4.0, 0.01, 1.0, font)
        , tiltKnob ("TILT", -4.0, 4.0, 0.01, 0.0, font)
        , oddEvenKnob ("ODD/EVEN", 0.0, 1.0, 0.001, 0.5, font)
        , formantKnob ("FORMANT", -4.0, 4.0, 0.01, 0.0, font)
        , formantPositionKnob ("F.POS", 0.0, 7.0, 0.01, 2.0, font)
        , scatterKnob ("SCATTER", 0.0, 1.0, 0.001, 0.0, font)
        , syncRatioKnob ("SYNC RATIO", 1.0, 8.0, 0.01, 1.5, font)
        , unisonKnob ("UNISON", 1.0, static_cast<double> (SynthExample::maxUnisonVoices), 1.0, 1.0, font)
        , unisonDetuneKnob ("U.DETUNE", 0.0, 1.0, 0.001, 0.2, font)
        , spreadKnob ("SPREAD", 0.0, 1.0, 0.001, 0.6, font)
    {
        titleLabel.setText (panelTitle, yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (12.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        partialsButton.setButtonText ("PARTIALS");
        partialsButton.setColor (yup::ToggleButton::Style::backgroundColorId, SynthTheme::displayBackground);
        partialsButton.setColor (yup::ToggleButton::Style::backgroundToggledColorId, SynthTheme::accentDim);
        partialsButton.setColor (yup::ToggleButton::Style::textColorId, SynthTheme::textSecondary);
        partialsButton.setColor (yup::ToggleButton::Style::textToggledColorId, SynthTheme::textPrimary);
        partialsButton.setColor (yup::ToggleButton::Style::borderColorId, SynthTheme::panelBorder);
        partialsButton.setColor (yup::ToggleButton::Style::borderToggledColorId, SynthTheme::accent);
        partialsButton.onClick = [this] { editor.setEditingPartials (partialsButton.getToggleState()); };
        addAndMakeVisible (partialsButton);

        resetButton.setColor (yup::TextButton::Style::backgroundColorId, SynthTheme::displayBackground);
        resetButton.setColor (yup::TextButton::Style::textColorId, SynthTheme::textSecondary);
        resetButton.setColor (yup::TextButton::Style::outlineColorId, SynthTheme::panelBorder);
        resetButton.onClick = [this] { editor.revertToPreset(); };
        addAndMakeVisible (resetButton);

        addAndMakeVisible (editor);

        for (auto* choice : { &waveformChoice, &syncModeChoice })
            addAndMakeVisible (*choice);

        for (auto* knob : { &levelKnob, &octaveKnob, &detuneKnob, &ridgesKnob, &colorKnob, &dispersionKnob,
                            &squeezeKnob, &squashKnob, &tiltKnob, &oddEvenKnob, &formantKnob,
                            &formantPositionKnob, &scatterKnob, &syncRatioKnob,
                            &unisonKnob, &unisonDetuneKnob, &spreadKnob })
            addAndMakeVisible (*knob);

        for (auto* knob : { &levelKnob, &colorKnob, &squeezeKnob, &oddEvenKnob, &scatterKnob, &unisonDetuneKnob, &spreadKnob })
            knob->formatValue = SynthFormat::percent;

        octaveKnob.formatValue = SynthFormat::octaves;
        detuneKnob.formatValue = SynthFormat::cents;
        ridgesKnob.formatValue = SynthFormat::ratio;
        syncRatioKnob.formatValue = SynthFormat::ratio;
        unisonKnob.formatValue = SynthFormat::count;

        // Picking a preset drops any edited partials, otherwise the oscillator would keep
        // playing the edited shape while the combo box claims something else.
        waveformChoice.onChange = [this] (int id)
        {
            settings.waveform = id - 1;
            editor.revertToPreset();
        };

        syncModeChoice.onChange = [this] (int id)
        {
            settings.syncMode = id - 1;
            updateSyncAvailability();
            editor.refresh();
        };

        levelKnob.onChange = [this] (double value) { settings.level = static_cast<float> (value); };
        octaveKnob.onChange = [this] (double value) { settings.octave = static_cast<int> (value); };
        detuneKnob.onChange = [this] (double value) { settings.detuneSemitones = static_cast<float> (value * 0.01); };
        ridgesKnob.onChange = [this] (double value) { settings.ridgeSpacing = static_cast<float> (value); editor.refresh(); };
        colorKnob.onChange = [this] (double value) { settings.color = static_cast<float> (value); editor.refresh(); };
        dispersionKnob.onChange = [this] (double value) { settings.dispersion = static_cast<float> (value); editor.refresh(); };
        squeezeKnob.onChange = [this] (double value) { settings.squeeze = static_cast<float> (value); editor.refresh(); };
        squashKnob.onChange = [this] (double value) { settings.squash = static_cast<float> (value); editor.refresh(); };
        tiltKnob.onChange = [this] (double value) { settings.tilt = static_cast<float> (value); editor.refresh(); };
        oddEvenKnob.onChange = [this] (double value) { settings.oddEven = static_cast<float> (value); editor.refresh(); };
        formantKnob.onChange = [this] (double value) { settings.formant = static_cast<float> (value); editor.refresh(); };
        formantPositionKnob.onChange = [this] (double value) { settings.formantPosition = static_cast<float> (value); editor.refresh(); };
        scatterKnob.onChange = [this] (double value) { settings.scatter = static_cast<float> (value); editor.refresh(); };
        syncRatioKnob.onChange = [this] (double value) { settings.syncRatio = static_cast<float> (value); editor.refresh(); };
        unisonKnob.onChange = [this] (double value) { settings.unisonVoices = static_cast<int> (value); };
        unisonDetuneKnob.onChange = [this] (double value) { settings.unisonDetune = static_cast<float> (value); };
        spreadKnob.onChange = [this] (double value) { settings.unisonSpread = static_cast<float> (value); };

        refresh();
    }

    /** Reads the settings back into the widgets. */
    void refresh()
    {
        waveformChoice.getComboBox().setSelectedId (settings.waveform.load() + 1, yup::dontSendNotification);
        syncModeChoice.getComboBox().setSelectedId (settings.syncMode.load() + 1, yup::dontSendNotification);

        levelKnob.getSlider().setValue (settings.level.load(), yup::dontSendNotification);
        octaveKnob.getSlider().setValue (settings.octave.load(), yup::dontSendNotification);
        detuneKnob.getSlider().setValue (settings.detuneSemitones.load() * 100.0, yup::dontSendNotification);
        ridgesKnob.getSlider().setValue (settings.ridgeSpacing.load(), yup::dontSendNotification);
        colorKnob.getSlider().setValue (settings.color.load(), yup::dontSendNotification);
        dispersionKnob.getSlider().setValue (settings.dispersion.load(), yup::dontSendNotification);
        squeezeKnob.getSlider().setValue (settings.squeeze.load(), yup::dontSendNotification);
        squashKnob.getSlider().setValue (settings.squash.load(), yup::dontSendNotification);
        tiltKnob.getSlider().setValue (settings.tilt.load(), yup::dontSendNotification);
        oddEvenKnob.getSlider().setValue (settings.oddEven.load(), yup::dontSendNotification);
        formantKnob.getSlider().setValue (settings.formant.load(), yup::dontSendNotification);
        formantPositionKnob.getSlider().setValue (settings.formantPosition.load(), yup::dontSendNotification);
        scatterKnob.getSlider().setValue (settings.scatter.load(), yup::dontSendNotification);
        syncRatioKnob.getSlider().setValue (settings.syncRatio.load(), yup::dontSendNotification);
        unisonKnob.getSlider().setValue (settings.unisonVoices.load(), yup::dontSendNotification);
        unisonDetuneKnob.getSlider().setValue (settings.unisonDetune.load(), yup::dontSendNotification);
        spreadKnob.getSlider().setValue (settings.unisonSpread.load(), yup::dontSendNotification);

        updateSyncAvailability();

        editor.refresh();
    }

    /** Publishes a frame's worth of partial edits to the audio thread. */
    void commitPendingEdits() { editor.commitPendingEdits(); }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (panelInset);

        auto header = bounds.removeFromTop (headerHeight);
        resetButton.setBounds (header.removeFromRight (buttonWidth));
        header.removeFromRight (spacing);
        partialsButton.setBounds (header.removeFromRight (buttonWidth));
        titleLabel.setBounds (header);

        bounds.removeFromTop (spacing);

        auto knobArea = bounds.removeFromBottom (knobRowHeight * 3.0f + spacing * 2.0f);
        bounds.removeFromBottom (spacing);

        auto choiceArea = bounds.removeFromBottom (choiceRowHeight);
        bounds.removeFromBottom (spacing);

        editor.setBounds (bounds);

        layoutControlsInRow (choiceArea, { &waveformChoice, &syncModeChoice });

        layoutControlsInRow (knobArea.removeFromTop (knobRowHeight),
                             { &levelKnob, &octaveKnob, &detuneKnob, &ridgesKnob, &colorKnob, &dispersionKnob });

        knobArea.removeFromTop (spacing);

        layoutControlsInRow (knobArea.removeFromTop (knobRowHeight),
                             { &squeezeKnob, &squashKnob, &tiltKnob, &oddEvenKnob, &formantKnob, &formantPositionKnob });

        knobArea.removeFromTop (spacing);

        layoutControlsInRow (knobArea,
                             { &scatterKnob, &syncRatioKnob, &unisonKnob, &unisonDetuneKnob, &spreadKnob });
    }

    void paint (yup::Graphics& g) override
    {
        paintSynthPanel (g, getLocalBounds());
    }

private:
    //==============================================================================
    /** Greys out the ratio knob while no sync mode reads it. */
    void updateSyncAvailability()
    {
        syncRatioKnob.setEnabled (static_cast<yup::SyncMode> (settings.syncMode.load()) != yup::SyncMode::none);
    }

    //==============================================================================
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 18.0f;
    static constexpr float choiceRowHeight = 36.0f;
    static constexpr float knobRowHeight = 58.0f;
    static constexpr float buttonWidth = 68.0f;
    static constexpr float spacing = 6.0f;

    SynthOscillatorSettings& settings;

    yup::Label titleLabel;
    yup::ToggleButton partialsButton;
    yup::TextButton resetButton { "RESET" };
    WaveformEditor editor;

    ChoiceControl waveformChoice;
    ChoiceControl syncModeChoice;

    KnobControl levelKnob;
    KnobControl octaveKnob;
    KnobControl detuneKnob;
    KnobControl ridgesKnob;
    KnobControl colorKnob;
    KnobControl dispersionKnob;
    KnobControl squeezeKnob;
    KnobControl squashKnob;
    KnobControl tiltKnob;
    KnobControl oddEvenKnob;
    KnobControl formantKnob;
    KnobControl formantPositionKnob;
    KnobControl scatterKnob;
    KnobControl syncRatioKnob;
    KnobControl unisonKnob;
    KnobControl unisonDetuneKnob;
    KnobControl spreadKnob;
};
