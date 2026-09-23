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
#include <optional>
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
/** Renders a raymarched ridge landscape shaped by one period of a waveform.

    The dominant displacement of the landscape is the waveform itself, so the ridges
    follow the shape the oscillator plays, with a few faint octaves on top as shimmer.

    Compiling the GLSL costs tens of milliseconds, so one instance is shared by every
    waveform display and it compiles once. A failed compile is remembered rather than
    retried, and render() then returns nullptr so the caller can draw without it.
*/
class SynthWaveformShader
{
public:
    /** The number of waveform samples render() expects. */
    static constexpr int sampleCount = 256;

    /** Renders the landscape into a target owned by the caller.

        @param context  The context the display paints with, providing the GPU device
        @param target   The caller's target, recreated here whenever the size changes
        @param width    The width of the landscape, in logical units
        @param height   The height of the landscape, in logical units
        @param samples  One period of the waveform, sampleCount values in the range -1 to 1
        @param time     The animation time, in seconds

        @returns The rendered landscape, or nullptr when no GPU path is available.
    */
    yup::GpuTexture::Ptr render (yup::GraphicsContext& context,
                                 yup::GpuTarget::Ptr& target,
                                 int width,
                                 int height,
                                 const std::vector<float>& samples,
                                 float time)
    {
        jassert (samples.size() == static_cast<std::size_t> (sampleCount));

        if (width < 2 || height < 2 || ! ensurePipeline (context))
            return nullptr;

        if (target == nullptr || target->getWidth() != width || target->getHeight() != height)
            target = yup::GpuTarget::create (device, width, height);

        if (target == nullptr)
            return nullptr;

        const Params params { time, static_cast<float> (width), static_cast<float> (height), 0.0f };

        auto frame = yup::GpuFrame::begin (device);
        if (! frame.isValid())
            return nullptr;

        auto pass = target->beginRenderPass (frame, { true, SynthTheme::displayBackground });
        if (! pass.isValid())
            return nullptr;

        pass.setPipeline (pipeline);
        pass.setUniformBuffer (0, 0, &params, sizeof (params));
        pass.setUniformBuffer (0, 1, samples.data(), samples.size() * sizeof (float));

        if (! pass.draw (3) || ! pass.finish() || ! frame.submit())
            return nullptr;

        return target->asTexture();
    }

private:
    struct alignas (16) Params
    {
        float time;
        float width;
        float height;
        float pad;
    };

    bool ensurePipeline (yup::GraphicsContext& context)
    {
        if (pipeline != nullptr)
            return true;

        if (compileAttempted || ! context.isGpuAvailable())
            return false;

        compileAttempted = true;
        device = context.getGpuDevice();

        yup::GpuPipelineOptions options;
        options.colorTargets.emplace_back().blendEnabled = false;

        auto result = yup::GpuPipeline::compileFromGlsl (device, vertexSource, yup::String::fromUTF8 (fragmentSource), options);
        if (result.failed())
        {
            yup::Logger::outputDebugString ("SynthWaveformShader: shader compile failed: " + result.getErrorMessage());
            return false;
        }

        pipeline = result.getValue();
        return true;
    }

    static constexpr char vertexSource[] = R"glsl(#version 450
void main() {
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)glsl";

    // Shadertoy's y-up pixel space, since RHI targets read top-left-origin everywhere.
    static constexpr char fragmentSource[] = R"glsl(#version 450
layout(set = 0, binding = 0) uniform Params
{
    float time;
    float width;
    float height;
    float pad;
} u;

layout(set = 0, binding = 1) uniform Samples
{
    vec4 samples[64];
} waveform;

layout(location = 0) out vec4 fragColor;

const vec3 accent = vec3(0.447, 0.918, 0.824);
const float focal = 2.8;
const vec3 background = vec3(0.055, 0.067, 0.078);

float fetchSample(int index)
{
    return waveform.samples[index >> 2][index & 3];
}

float wave(float phase)
{
    float position = phase * 255.0;
    int i0 = int(position);
    int i1 = min(i0 + 1, 255);
    return mix(fetchSample(i0), fetchSample(i1), position - float(i0));
}

void main()
{
    vec2 resolution = vec2(u.width, u.height);
    vec2 I = vec2(gl_FragCoord.x, u.height - gl_FragCoord.y);

    // The ridge on the far wall, four units away, spans one period across the width.
    vec3 direction = normalize(vec3(I + I - resolution, -u.height * focal));
    float frequency = u.height * focal / (8.0 * u.width);

    float scroll = 0.5 + u.time * 0.01;
    float hue = u.time * 0.15;

    vec3 color = vec3(0.0);
    float z = 0.0;

    for (int i = 0; i < 90; ++i)
    {
        vec3 p = z * direction + vec3(0.0, 1.0, 1.0);

        float r = max(-p.y, 0.0);
        p.y += r + r;

        p.y -= wave(fract(p.x * frequency + scroll));

        for (float octave = 2.0; octave < 30.0; octave += octave)
            p.y += 0.12 * cos(p.x * octave + 0.6 * u.time * cos(octave) + z) / octave;

        float plane = p.z + 3.0;
        float d = (0.1 * r + abs(p.y - 1.0) / (1.0 + r + r + r * r) + max(plane, -plane * 0.1)) / 8.0;
        z += d;

        float phase = z * 0.5 + hue;
        vec3 tone = accent * (cos(phase) + 1.3) + vec3(0.0, 0.15, 0.08) * cos(phase + 2.0);
        color += tone / max(d * z, 1.0e-4);
    }

    fragColor = vec4(max(tanh(color / 900.0), background), 1.0);
}
)glsl";

    yup::GpuDevice::Ptr device;
    yup::GpuPipeline::Ptr pipeline;
    bool compileAttempted = false;
};

//==============================================================================
/** The waveform of one oscillator, either drawn or edited a partial at a time.

    In drawing mode the component shows one period of the reconstructed series, and a
    drag draws that period freehand: the stroke is analyzed back into partials, phase
    included, which become the oscillator's custom series. In editing mode it shows the
    magnitude of each harmonic as a bar that can be dragged.

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
                    const SynthOscillatorResources& sharedResources,
                    std::shared_ptr<SynthWaveformShader> sharedShader)
        : settings (settingsToEdit)
        , resources (sharedResources)
        , shader (std::move (sharedShader))
    {
        preview.prepare();
        displaySeries.resize (SynthExample::maxHarmonics);
        displaySamples.assign (displayResolution, 0.0f);
        drawnCycle.assign (displayResolution, 0.0f);
        drawnSeries.resize (SynthExample::editableHarmonics);

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
        // it clean outside the display. During a stroke it follows the stroke's own peak
        // instead, so the curve stays under the pointer.
        const auto peak = measurePeak();

        if (peak > 1.0e-6f)
        {
            const auto scale = (lastDrawnIndex.has_value() ? measurePeak (drawnCycle) : 1.0f) / peak;

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
        return measurePeak (displaySamples);
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

        publishEdit();
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds();
        const auto landscape = editingPartials ? nullptr : renderLandscape (g);

        if (landscape != nullptr)
        {
            const auto state = g.saveState();

            // setClipPath works in top-level coordinates, unlike the drawing calls.
            yup::Path clip;
            clip.addRoundedRectangle (getBoundsRelativeToTopLevelComponent(), cornerRadius);
            g.setClipPath (clip);
            g.drawTexture (landscape, bounds);
        }
        else
        {
            g.setFillColor (SynthTheme::displayBackground);
            g.fillRoundedRect (bounds, cornerRadius);
        }

        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (bounds.reduced (0.5f), cornerRadius);

        if (editingPartials)
            paintPartials (g, bounds.reduced (contentInset));
        else if (landscape == nullptr)
            paintWaveform (g, bounds.reduced (contentInset));
        else
            paintDrawnCycle (g, bounds.reduced (contentInset));
    }

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        if (editingPartials || ! isShowing())
            return;

        animationTime += static_cast<float> (lastFrameTimeSeconds);
        repaint();
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        if (editingPartials)
        {
            applyEdit (event);
            return;
        }

        seedDrawnCycle();
        lastDrawnIndex.reset();
        applyDraw (event);
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        if (editingPartials)
            applyEdit (event);
        else
            applyDraw (event);
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        if (! lastDrawnIndex.has_value())
            return;

        lastDrawnIndex.reset();
        refresh();
    }

private:
    //==============================================================================
    /** Renders the shader landscape behind the waveform, or returns nullptr without a GPU. */
    yup::GpuTexture::Ptr renderLandscape (yup::Graphics& g)
    {
        return shader->render (g.getGraphicsContext(),
                               landscapeTarget,
                               yup::roundToInt (getWidth()),
                               yup::roundToInt (getHeight()),
                               displaySamples,
                               animationTime);
    }

    /** Returns the largest magnitude in a buffer of samples. */
    static float measurePeak (const std::vector<float>& samples) noexcept
    {
        auto peak = 0.0f;

        for (auto sample : samples)
            peak = yup::jmax (peak, std::abs (sample));

        return peak;
    }

    /** Draws the raw stroke while one is in progress, so the pointer always has it underneath. */
    void paintDrawnCycle (yup::Graphics& g, yup::Rectangle<float> bounds)
    {
        if (! lastDrawnIndex.has_value())
            return;

        drawnPath.clear();
        drawnPath.reserveSpace (displayResolution);

        for (int index = 0; index < displayResolution; ++index)
        {
            const auto x = bounds.getX() + bounds.getWidth() * static_cast<float> (index)
                                             / static_cast<float> (displayResolution);

            const auto y = bounds.getCenterY() - drawnCycle[static_cast<std::size_t> (index)] * bounds.getHeight() * 0.45f;

            if (index == 0)
                drawnPath.moveTo (x, y);
            else
                drawnPath.lineTo (x, y);
        }

        g.setStrokeColor (SynthTheme::accent.withAlpha (0.25f));
        g.setStrokeWidth (1.0f);
        g.strokePath (drawnPath);
    }

    /** Draws one period of the reconstructed series. */
    void paintWaveform (yup::Graphics& g, yup::Rectangle<float> bounds)
    {
        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeLine (bounds.getX(), bounds.getCenterY(), bounds.getRight(), bounds.getCenterY());

        paintDrawnCycle (g, bounds);

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
    /** Editing a preset copies its partials in first, so the edit starts from the shape on screen. */
    void beginCustomEdit()
    {
        if (settings.usesCustomSeries.load())
            return;

        settings.seedHarmonicsFrom (displaySeries);
        settings.usesCustomSeries.store (true);
    }

    /** Marks the edit for the next commit and brings the display and the panel along. */
    void publishEdit()
    {
        pendingEdit = true;

        refresh();

        if (onPartialsChanged != nullptr)
            onPartialsChanged();
    }

    /** Starts a stroke from the source waveform, normalized as the display shows it. */
    void seedDrawnCycle()
    {
        for (int index = 0; index < displayResolution; ++index)
        {
            const auto phase = static_cast<double> (index) / static_cast<double> (displayResolution);

            drawnCycle[static_cast<std::size_t> (index)] =
                evaluateFourierSeries (displaySeries, phase, SynthExample::displayHarmonics);
        }

        const auto peak = measurePeak (drawnCycle);

        if (peak <= 1.0e-6f)
            return;

        for (auto& sample : drawnCycle)
            sample /= peak;
    }

    /** Draws the stroke into the cycle and analyzes the cycle into the custom series. */
    void applyDraw (const yup::MouseEvent& event)
    {
        const auto bounds = getLocalBounds().reduced (contentInset);

        if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
            return;

        beginCustomEdit();

        const auto position = event.getPosition();
        const auto index = yup::jlimit (0,
                                        displayResolution - 1,
                                        static_cast<int> (std::floor ((position.getX() - bounds.getX()) / bounds.getWidth()
                                                                      * static_cast<float> (displayResolution))));

        const auto value = yup::jlimit (-1.0f, 1.0f, (bounds.getCenterY() - position.getY()) / (bounds.getHeight() * 0.45f));

        // Fills every sample the pointer skipped since the last event, so a fast drag leaves no gaps.
        const auto first = lastDrawnIndex.value_or (index);
        const auto firstValue = drawnCycle[static_cast<std::size_t> (first)];
        const auto steps = std::abs (index - first);
        const auto direction = index >= first ? 1 : -1;

        for (int step = 1; step < steps; ++step)
        {
            const auto amount = static_cast<float> (step) / static_cast<float> (steps);

            drawnCycle[static_cast<std::size_t> (first + step * direction)] = firstValue + (value - firstValue) * amount;
        }

        drawnCycle[static_cast<std::size_t> (index)] = value;
        lastDrawnIndex = index;

        // The custom series has no DC term, so the drawn cycle's offset is simply dropped.
        drawnSeries.setFromCycle (yup::Span<const float> (drawnCycle));
        settings.seedHarmonicsFrom (drawnSeries);

        publishEdit();
    }

    /** Turns a mouse position into the magnitude of one harmonic. */
    void applyEdit (const yup::MouseEvent& event)
    {
        const auto bounds = getLocalBounds().reduced (contentInset);

        if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
            return;

        beginCustomEdit();

        const auto position = event.getPosition();
        const auto barWidth = bounds.getWidth() / static_cast<float> (SynthExample::editableHarmonics);
        const auto index = yup::jlimit (0,
                                        SynthExample::editableHarmonics - 1,
                                        static_cast<int> ((position.getX() - bounds.getX()) / barWidth));

        const auto magnitude = yup::jlimit (0.0f, 1.0f, (bounds.getBottom() - position.getY()) / bounds.getHeight());

        settings.setHarmonicMagnitude (index, magnitude);

        publishEdit();
    }

    //==============================================================================
    static constexpr int displayResolution = 256;
    static constexpr float contentInset = 6.0f;
    static constexpr float cornerRadius = 4.0f;
    static constexpr float barGap = 1.0f;

    static_assert (displayResolution == SynthWaveformShader::sampleCount);

    SynthOscillatorSettings& settings;
    const SynthOscillatorResources& resources;
    std::shared_ptr<SynthWaveformShader> shader;
    yup::GpuTarget::Ptr landscapeTarget;
    float animationTime = 0.0f;

    SynthSpectrumDerivation preview;
    yup::FourierSeries<double> displaySeries;
    std::vector<float> displaySamples;
    yup::Path path;

    std::vector<float> drawnCycle;
    yup::Path drawnPath;
    yup::FourierSeries<double> drawnSeries;
    std::optional<int> lastDrawnIndex;

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
/** Shape, rate, phase and retrigger of one LFO, with its display.

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

        retriggerButton.setButtonText ("RETRIG");
        retriggerButton.setColor (yup::ToggleButton::Style::backgroundColorId, SynthTheme::displayBackground);
        retriggerButton.setColor (yup::ToggleButton::Style::backgroundToggledColorId, SynthTheme::accentDim);
        retriggerButton.setColor (yup::ToggleButton::Style::textColorId, SynthTheme::textSecondary);
        retriggerButton.setColor (yup::ToggleButton::Style::textToggledColorId, SynthTheme::textPrimary);
        retriggerButton.setColor (yup::ToggleButton::Style::borderColorId, SynthTheme::panelBorder);
        retriggerButton.setColor (yup::ToggleButton::Style::borderToggledColorId, SynthTheme::accent);
        retriggerButton.onClick = [this] { settings.retrigger = retriggerButton.getToggleState(); };
        addAndMakeVisible (retriggerButton);

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
        retriggerButton.setToggleState (settings.retrigger.load(), yup::dontSendNotification);

        refreshDisplay();
    }

    /** Moves the display marker to the engine's phase. */
    void setPhase (float phase) { display.setPhase (phase); }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (panelInset);

        auto header = bounds.removeFromTop (headerHeight);
        retriggerButton.setBounds (header.removeFromRight (buttonWidth));
        titleLabel.setBounds (header);
        bounds.removeFromTop (spacing);

        // Everything sits in one row so the panel stays short: display, shape, rate, phase.
        auto knobs = bounds.removeFromRight (knobWidth * 2.0f);
        bounds.removeFromRight (spacing);
        auto choice = bounds.removeFromRight (choiceWidth);
        bounds.removeFromRight (spacing);
        display.setBounds (bounds);

        shapeChoice.setBounds (choice.withSizeKeepingCenter (choice.getWidth(), yup::jmin (choice.getHeight(), choiceHeight)));
        layoutControlsInRow (knobs, { &rateKnob, &phaseKnob });
    }

    void paint (yup::Graphics& g) override
    {
        paintSynthPanel (g, getLocalBounds());
    }

private:
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 18.0f;
    static constexpr float choiceHeight = 36.0f;
    static constexpr float choiceWidth = 90.0f;
    static constexpr float knobWidth = 58.0f;
    static constexpr float buttonWidth = 68.0f;
    static constexpr float spacing = 6.0f;

    void refreshDisplay() { display.setValues (settings.read()); }

    SynthLFOSettings& settings;

    yup::Label titleLabel;
    yup::ToggleButton retriggerButton;
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
                          std::shared_ptr<SynthWaveformShader> waveformShader,
                          const yup::Font& font)
        : settings (settingsToEdit)
        , editor (settingsToEdit, resources, std::move (waveformShader))
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
