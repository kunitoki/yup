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

#include "WaveformEditor.h"
#include "SynthTheme.h"

WaveformEditor::WaveformEditor (const SynthOscillatorParameters& parametersToShow,
                                SynthPartials& partialsToEdit,
                                const SynthOscillatorResources& sharedResources,
                                std::shared_ptr<SynthWaveformShader> sharedShader)
    : parameters (parametersToShow)
    , partials (partialsToEdit)
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

void WaveformEditor::setEditingPartials (bool shouldEdit)
{
    editingPartials = shouldEdit;
    repaint();
}

void WaveformEditor::refresh()
{
    auto values = parameters.read();

    if (partials.usesCustomSeries())
    {
        partials.copyHarmonicsInto (displaySeries, 1.0f);

        // Measured here, on the message thread, so the audio thread never has to work out
        // how loud an edited spectrum turned out to be.
        partials.updateHarmonicScale();
    }
    else
    {
        displaySeries.copyFrom (resources.getFrame (values.waveform));
    }

    // Derived exactly as the voices derive theirs, so the preview cannot drift from
    // what is played.
    partials.fillValues (values);
    preview.invalidate();
    preview.update (values, partials, resources);
    reconstruct (preview.getSeries());

    // Normalize whatever is actually drawn. The shaper preserves the coefficient sum
    // rather than the peak, and a sum bounds a peak from well above - three times over
    // for a sawtooth - so scaling the derived waveform by the source's peak would draw
    // it clean outside the display. During a stroke it follows the stroke's own peak
    // instead, so the curve stays under the pointer.
    const auto peak = measurePeak (displaySamples);

    if (peak > 1.0e-6f)
    {
        const auto scale = (lastDrawnIndex.has_value() ? measurePeak (drawnCycle) : 1.0f) / peak;

        for (auto& sample : displaySamples)
            sample *= scale;
    }

    repaint();
}

void WaveformEditor::reconstruct (const yup::FourierSeries<double>& series) noexcept
{
    for (int index = 0; index < displayResolution; ++index)
    {
        const auto phase = static_cast<double> (index) / static_cast<double> (displayResolution - 1);

        displaySamples[static_cast<std::size_t> (index)] =
            evaluateFourierSeries (series, phase, SynthExample::displayHarmonics);
    }
}

void WaveformEditor::commitPendingEdits()
{
    if (! pendingEdit)
        return;

    pendingEdit = false;
    partials.bumpGeneration();
}

void WaveformEditor::revertToPreset()
{
    partials.setUsesCustomSeries (false);

    publishEdit();
}

//==============================================================================
void WaveformEditor::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto landscape = editingPartials ? nullptr : renderLandscape (g);

    if (landscape != nullptr)
    {
        const auto state = g.saveState();

        // The clip follows the border's centerline so the texture's antialiased edge stays under it.
        yup::Path clip;
        clip.addRoundedRectangle (bounds.reduced (0.5f), cornerRadius);
        g.setClipPath (clip);
        g.drawTexture (landscape, bounds);
    }
    else
    {
        g.setFillColor (SynthTheme::wellColor (*this));
        g.fillRoundedRect (bounds, cornerRadius);
    }

    g.setStrokeColor (SynthTheme::panelBorder (*this));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), cornerRadius);

    if (editingPartials)
        paintPartials (g, bounds.reduced (contentInset));
    else if (landscape == nullptr)
        paintWaveform (g, bounds.reduced (contentInset));
    else
        paintDrawnCycle (g, bounds.reduced (contentInset));
}

void WaveformEditor::refreshDisplay (double lastFrameTimeSeconds)
{
    if (editingPartials || ! isShowing())
        return;

    animationTime += static_cast<float> (lastFrameTimeSeconds);
    repaint();
}

void WaveformEditor::mouseDown (const yup::MouseEvent& event)
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

void WaveformEditor::mouseDrag (const yup::MouseEvent& event)
{
    if (editingPartials)
        applyEdit (event);
    else
        applyDraw (event);
}

void WaveformEditor::mouseUp (const yup::MouseEvent&)
{
    if (! lastDrawnIndex.has_value())
        return;

    lastDrawnIndex.reset();
    refresh();
}

//==============================================================================
/** Renders the shader landscape behind the waveform, or returns nullptr without a GPU. */
yup::GpuTexture::Ptr WaveformEditor::renderLandscape (yup::Graphics& g)
{
    return shader->render (g.getGraphicsContext(),
                           landscapeTarget,
                           yup::roundToInt (getWidth()),
                           yup::roundToInt (getHeight()),
                           displaySamples,
                           animationTime,
                           SynthTheme::accent (*this),
                           SynthTheme::wellColor (*this));
}

/** Returns the largest magnitude in a buffer of samples. */
float WaveformEditor::measurePeak (const std::vector<float>& samples) noexcept
{
    auto peak = 0.0f;

    for (auto sample : samples)
        peak = yup::jmax (peak, std::abs (sample));

    return peak;
}

/** Draws the raw stroke while one is in progress, so the pointer always has it underneath. */
void WaveformEditor::paintDrawnCycle (yup::Graphics& g, yup::Rectangle<float> bounds)
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

    g.setStrokeColor (SynthTheme::accent (*this).withAlpha (0.25f));
    g.setStrokeWidth (1.0f);
    g.strokePath (drawnPath);
}

/** Draws one period of the reconstructed series. */
void WaveformEditor::paintWaveform (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    g.setStrokeColor (SynthTheme::panelBorder (*this));
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

    g.setStrokeColor (SynthTheme::accent (*this).withAlpha (0.35f));
    g.setStrokeWidth (4.0f);
    g.setFeather (6.0f);
    g.strokePath (path);

    g.setFeather (0.0f);
    g.setStrokeColor (SynthTheme::accent (*this));
    g.setStrokeWidth (1.5f);
    g.strokePath (path);
}

/** Draws the editable magnitude of every harmonic. */
void WaveformEditor::paintPartials (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    const auto barWidth = bounds.getWidth() / static_cast<float> (SynthExample::editableHarmonics);

    for (int index = 0; index < SynthExample::editableHarmonics; ++index)
    {
        const auto magnitude = yup::jlimit (0.0f, 1.0f, static_cast<float> (displaySeries.getMagnitude (index + 1)));
        const auto height = yup::jmax (1.0f, magnitude * bounds.getHeight());
        const auto x = bounds.getX() + barWidth * static_cast<float> (index);

        const yup::Rectangle<float> bar { x + barGap, bounds.getBottom() - height, yup::jmax (1.0f, barWidth - barGap * 2.0f), height };

        g.setFillColor (magnitude > 0.0f ? SynthTheme::accent (*this) : SynthTheme::panelBorder (*this));
        g.fillRect (bar);
    }
}

//==============================================================================
/** Editing a preset copies its partials in first, so the edit starts from the shape on screen. */
void WaveformEditor::beginCustomEdit()
{
    if (partials.usesCustomSeries())
        return;

    partials.seedHarmonicsFrom (displaySeries);
    partials.setUsesCustomSeries (true);
}

/** Marks the edit for the next commit and brings the display along. */
void WaveformEditor::publishEdit()
{
    pendingEdit = true;

    refresh();
}

/** Starts a stroke from the source waveform, normalized as the display shows it. */
void WaveformEditor::seedDrawnCycle()
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
void WaveformEditor::applyDraw (const yup::MouseEvent& event)
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
    partials.seedHarmonicsFrom (drawnSeries);

    publishEdit();
}

/** Turns a mouse position into the magnitude of one harmonic. */
void WaveformEditor::applyEdit (const yup::MouseEvent& event)
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

    partials.setHarmonicMagnitude (index, magnitude);

    publishEdit();
}
