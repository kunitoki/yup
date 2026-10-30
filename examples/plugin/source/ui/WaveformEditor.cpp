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
#include "SynthUndoHistory.h"

//==============================================================================
WaveformEditor::WaveformEditor (const SynthOscillatorParameters& parametersToShow,
                                SynthPartials& partialsToEdit,
                                const SynthOscillatorResources& sharedResources,
                                std::shared_ptr<SynthWaveformShader> sharedShader)
    : parameters (parametersToShow)
    , partials (partialsToEdit)
    , resources (sharedResources)
    , shader (std::move (sharedShader))
{
    captionFont = SynthTheme::captionFont (11.5f);

    setMouseCursor (yup::MouseCursor::Crosshair);
    setWantedGestures (yup::GestureType::pinch);
    setWantsKeyboardFocus (true);
    
    clampPartialsView();
    preview.prepare();

    displaySeries.resize (SynthExample::maxHarmonics);
    displaySamples.assign (displayResolution, 0.0f);
    drawnCycle.assign (displayResolution, 0.0f);
    drawnSeries.resize (SynthExample::displayHarmonics);
    rotationBase.resize (SynthExample::editableHarmonics);
    rotated.resize (SynthExample::editableHarmonics);

    refresh();
}

void WaveformEditor::setEditingPartials (bool shouldEdit)
{
    editingPartials = shouldEdit;
    landscapeDirty = true;
    repaint();
}

void WaveformEditor::setPartialsTool (PartialsTool tool)
{
    partialsTool = tool;
    repaint();
}

void WaveformEditor::setGridDivisions (int divisions)
{
    gridDivisions = yup::jmax (0, divisions);
    repaint();
}

void WaveformEditor::applyOperation (PartialsOperation operation)
{
    const auto before = partials.getSnapshot();

    beginCustomEdit();

    auto edited = partials.getSnapshot();
    applyPartialsOperation (edited, operation, random);
    partials.restore (edited);

    publishEdit();
    recordEdit (before);
}

void WaveformEditor::refresh (SynthParameterReader reader)
{
    shownValues = parameters.read (reader);

    rebuild();
}

void WaveformEditor::rebuild()
{
    auto& values = shownValues;

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

    landscapeDirty = true;
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

void WaveformEditor::revertToPreset (bool joinsLastStep)
{
    const auto before = partials.getSnapshot();

    partials.setUsesCustomSeries (false);

    publishEdit();
    recordEdit (before, joinsLastStep);
}

//==============================================================================
void WaveformEditor::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto landscape = editingPartials ? nullptr : getLandscape (g);

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
    {
        paintPartials (g, bounds.reduced (contentInset));
    }
    else if (landscape == nullptr)
    {
        paintWaveform (g, bounds.reduced (contentInset));
    }
    else
    {
        // The landscape already shows the played waveform, following the stroke as it goes.
        paintGrid (g, bounds.reduced (contentInset));
        paintDrawnCycle (g, bounds.reduced (contentInset));
    }
}

void WaveformEditor::refreshDisplay (double lastFrameTimeSeconds)
{
    if (editingPartials || ! isShowing() || ! animating || isDrawing())
    {
        sinceLastFrame = 0.0;
        return;
    }

    sinceLastFrame += lastFrameTimeSeconds;

    if (sinceLastFrame < animationInterval)
        return;

    animationTime += static_cast<float> (sinceLastFrame);
    sinceLastFrame = 0.0;
    landscapeDirty = true;
    repaint();
}

void WaveformEditor::resized()
{
    landscapeDirty = true;
}

void WaveformEditor::mouseDown (const yup::MouseEvent& event)
{
    gestureStart = partials.getSnapshot();

    if (editingPartials)
    {
        draggingOverview = event.getPosition().getY() > getPartialsArea().getBottom();

        if (draggingOverview)
            centerPartialsAt (event.getPosition().getX());
        else
            applyEdit (event);

        return;
    }

    if (event.getModifiers().isShiftDown())
    {
        beginCustomEdit();
        partials.copyHarmonicsInto (rotationBase, 1.0f);
        rotationStartX = event.getPosition().getX();
        return;
    }

    seedDrawnCycle();
    strokeBase = drawnCycle;
    pressIndex = cycleIndexAt (event.getPosition().getX());
    pressValue = cycleValueAt (event.getPosition().getY());
    strokeSeed = random.nextInt64();
    lastDrawnIndex.reset();
    applyDraw (event);
}

void WaveformEditor::mouseDrag (const yup::MouseEvent& event)
{
    if (editingPartials && draggingOverview)
        centerPartialsAt (event.getPosition().getX());
    else if (editingPartials)
        applyEdit (event);
    else if (rotationStartX.has_value())
        applyRotation (event);
    else
        applyDraw (event);
}

void WaveformEditor::mouseUp (const yup::MouseEvent&)
{
    rotationStartX.reset();
    draggingOverview = false;

    if (lastDrawnIndex.has_value())
    {
        lastDrawnIndex.reset();
        rebuild();
    }

    recordEdit (gestureStart);
}

bool WaveformEditor::mouseWheel (const yup::MouseEvent& event, const yup::MouseWheelData& wheelData)
{
    if (! editingPartials)
        return false;

    const auto modifiers = event.getModifiers();
    const auto previousViewStart = partialsViewStart;
    const auto previousViewCount = partialsViewCount;

    if (modifiers.isCommandDown() || modifiers.isControlDown())
    {
        zoomPartials (std::exp2 (wheelData.getDeltaY() * 0.5f), event.getPosition().getX());
        return partialsViewStart != previousViewStart || partialsViewCount != previousViewCount;
    }

    // A vertical trackpad swipe carries a little horizontal motion too, so the larger axis wins.
    const auto delta = std::abs (wheelData.getDeltaX()) > std::abs (wheelData.getDeltaY()) ? wheelData.getDeltaX() : wheelData.getDeltaY();

    partialsViewStart -= delta * partialsViewCount * 0.1f;
    clampPartialsView();
    repaint();
    return partialsViewStart != previousViewStart || partialsViewCount != previousViewCount;
}

void WaveformEditor::gesturePinch (const yup::GestureEvent& event)
{
    if (editingPartials)
        zoomPartials (event.getScaleDelta(), event.getPosition().getX());
}

//==============================================================================
/** Returns the landscape, rendered again only when it moved or its waveform changed. */
yup::GpuTexture::Ptr WaveformEditor::getLandscape (yup::Graphics& g)
{
    if (landscapeDirty || cachedLandscape == nullptr)
    {
        cachedLandscape = renderLandscape (g);
        landscapeDirty = false;
    }

    return cachedLandscape;
}

/** Renders the shader landscape behind the waveform, or returns nullptr without a GPU. */
yup::GpuTexture::Ptr WaveformEditor::renderLandscape (yup::Graphics& g)
{
    return shader->render (g.getGraphicsContext(),
                           landscapeTarget,
                           yup::roundToInt (getWidth()),
                           yup::roundToInt (getHeight()),
                           displaySamples,
                           animationTime,
                           { SynthTheme::accent (*this), SynthTheme::wellColor (*this), ! SynthTheme::isDark (*this) });
}

/** True while a stroke or a rotation is in progress. */
bool WaveformEditor::isDrawing() const noexcept
{
    return lastDrawnIndex.has_value() || rotationStartX.has_value();
}

/** Returns the sample of the drawn cycle under a horizontal position. */
int WaveformEditor::cycleIndexAt (float x) const noexcept
{
    const auto bounds = getLocalBounds().reduced (contentInset);

    if (bounds.getWidth() <= 0.0f)
        return 0;

    return yup::jlimit (0, displayResolution - 1, static_cast<int> (std::floor ((x - bounds.getX()) / bounds.getWidth() * static_cast<float> (displayResolution))));
}

/** Returns the level of the drawn cycle at a vertical position. */
float WaveformEditor::cycleValueAt (float y) const noexcept
{
    const auto bounds = getLocalBounds().reduced (contentInset);

    if (bounds.getHeight() <= 0.0f)
        return 0.0f;

    return yup::jlimit (-1.0f, 1.0f, (bounds.getCenterY() - y) / (bounds.getHeight() * 0.45f));
}

/** Returns where the bars go in editing mode, above the overview strip. */
yup::Rectangle<float> WaveformEditor::getPartialsArea() const noexcept
{
    return getLocalBounds().reduced (contentInset).withTrimmedBottom (overviewHeight + overviewGap);
}

int WaveformEditor::getVisiblePartialCount() const noexcept
{
    return yup::jlimit (1, SynthExample::editableHarmonics, yup::roundToInt (partialsViewCount));
}

int WaveformEditor::getFirstVisiblePartial() const noexcept
{
    return yup::jlimit (0, SynthExample::editableHarmonics - getVisiblePartialCount(), yup::roundToInt (partialsViewStart));
}

/** Scales the window of partials shown, keeping the partial under a horizontal position in place. */
void WaveformEditor::zoomPartials (float factor, float x)
{
    const auto area = getPartialsArea();

    if (area.getWidth() <= 0.0f || ! (factor > 0.0f))
        return;

    const auto fraction = yup::jlimit (0.0f, 1.0f, (x - area.getX()) / area.getWidth());
    const auto anchor = partialsViewStart + fraction * partialsViewCount;

    partialsViewCount /= factor;
    clampPartialsView();
    partialsViewStart = anchor - fraction * partialsViewCount;
    clampPartialsView();
    repaint();
}

/** Moves the window of partials shown so it is centered where the overview strip was pressed. */
void WaveformEditor::centerPartialsAt (float x)
{
    const auto bounds = getLocalBounds().reduced (contentInset);

    if (bounds.getWidth() <= 0.0f)
        return;

    partialsViewStart = (x - bounds.getX()) / bounds.getWidth() * static_cast<float> (SynthExample::editableHarmonics) - partialsViewCount * 0.5f;
    clampPartialsView();
    repaint();
}

void WaveformEditor::clampPartialsView() noexcept
{
    const auto total = static_cast<float> (SynthExample::editableHarmonics);

    partialsViewCount = yup::jlimit (yup::jmin (total, static_cast<float> (minimumVisiblePartials)), total, partialsViewCount);
    partialsViewStart = yup::jlimit (0.0f, total - partialsViewCount, partialsViewStart);
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

    strokeGlowing (g, drawnPath);
}

/** Strokes a curve in the accent color over a soft glow of itself. */
void WaveformEditor::strokeGlowing (yup::Graphics& g, const yup::Path& pathToStroke)
{
    g.setStrokeColor (SynthTheme::accent (*this).withAlpha (0.35f));
    g.setStrokeWidth (4.0f);
    g.setFeather (6.0f);
    g.strokePath (pathToStroke);

    g.setFeather (0.0f);
    g.setStrokeColor (SynthTheme::accent (*this));
    g.setStrokeWidth (1.5f);
    g.strokePath (pathToStroke);
}

/** Marks the cells the span tools snap to. */
void WaveformEditor::paintGrid (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    if (gridDivisions <= 0)
        return;

    g.setStrokeColor (SynthTheme::panelBorder (*this).withAlpha (0.6f));
    g.setStrokeWidth (1.0f);

    for (int cell = 1; cell < gridDivisions; ++cell)
    {
        const auto x = bounds.getX() + bounds.getWidth() * static_cast<float> (cell) / static_cast<float> (gridDivisions);
        g.strokeLine (x, bounds.getY(), x, bounds.getBottom());
    }
}

/** Draws one period of the reconstructed series. */
void WaveformEditor::paintWaveform (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    g.setStrokeColor (SynthTheme::panelBorder (*this));
    g.setStrokeWidth (1.0f);
    g.strokeLine (bounds.getX(), bounds.getCenterY(), bounds.getRight(), bounds.getCenterY());

    paintGrid (g, bounds);

    if (lastDrawnIndex.has_value())
    {
        paintDrawnCycle (g, bounds);
        return;
    }

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

    strokeGlowing (g, path);
}

/** Draws the editable magnitude of every harmonic in the window shown. */
void WaveformEditor::paintPartials (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    paintPartialsOverview (g, bounds);

    const auto area = getPartialsArea();

    const auto first = getFirstVisiblePartial();
    const auto count = getVisiblePartialCount();
    const auto barWidth = area.getWidth() / static_cast<float> (count);
    const auto showsPhase = partialsTool == PartialsTool::phase;

    if (showsPhase)
    {
        g.setStrokeColor (SynthTheme::panelBorder (*this));
        g.setStrokeWidth (1.0f);
        g.strokeLine (area.getX(), area.getCenterY(), area.getRight(), area.getCenterY());
    }

    for (int index = first; index < first + count; ++index)
    {
        const auto magnitude = yup::jlimit (0.0f, 1.0f, static_cast<float> (displaySeries.getMagnitude (index + 1)));
        const auto height = yup::jmax (1.0f, magnitude * area.getHeight());
        const auto x = area.getX() + barWidth * static_cast<float> (index - first) + barGap;
        const auto width = yup::jmax (1.0f, barWidth - barGap * 2.0f);

        const yup::Rectangle<float> bar { x, area.getBottom() - height, width, height };

        g.setFillColor (magnitude > 0.0f ? SynthTheme::accent (*this).withAlpha (showsPhase ? 0.25f : 1.0f) : SynthTheme::panelBorder (*this));
        g.fillRect (bar);

        if (! showsPhase || magnitude <= 0.0f)
            continue;

        // The phase from -pi to pi, drawn up or down from the middle; a sine sits at zero.
        const auto phase = std::atan2 (displaySeries.getCosine (index + 1), displaySeries.getSine (index + 1));
        const auto offset = static_cast<float> (phase / yup::MathConstants<double>::pi) * area.getHeight() * 0.5f;

        g.setFillColor (SynthTheme::accent (*this));
        g.fillRect (yup::Rectangle<float> { x, area.getCenterY() - yup::jmax (0.0f, offset), width, yup::jmax (1.0f, std::abs (offset)) });
    }
}

/** Draws where the window of partials shown sits among all of them, and which ones it holds. */
void WaveformEditor::paintPartialsOverview (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    const auto total = static_cast<float> (SynthExample::editableHarmonics);
    const auto first = getFirstVisiblePartial();
    const auto count = getVisiblePartialCount();
    const yup::Rectangle<float> strip { bounds.getX(), bounds.getBottom() - overviewHeight, bounds.getWidth(), overviewHeight };

    g.setFillColor (SynthTheme::panelBorder (*this));
    g.fillRect (strip);

    g.setFillColor (SynthTheme::accent (*this));
    g.fillRect (yup::Rectangle<float> { strip.getX() + strip.getWidth() * static_cast<float> (first) / total,
                                        strip.getY(),
                                        strip.getWidth() * static_cast<float> (count) / total,
                                        overviewHeight });

    if (count >= SynthExample::editableHarmonics)
        return;

    g.setFillColor (SynthTheme::textSecondary (*this));
    g.fillFittedText (yup::String (first + 1) + "-" + yup::String (first + count) + " / " + yup::String (SynthExample::editableHarmonics),
                      captionFont,
                      getPartialsArea().reduced (2.0f),
                      yup::Justification::topRight);
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

    rebuild();
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

    const auto index = cycleIndexAt (event.getPosition().getX());
    const auto value = cycleValueAt (event.getPosition().getY());

    if (drawTool == WaveformDrawTool::brush)
    {
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
    }
    else
    {
        // A span tool redraws the whole span from the cycle the stroke started on.
        drawnCycle = strokeBase;
        stampWaveformShape (yup::Span<float> (drawnCycle), drawTool, pressIndex, pressValue, index, value, gridDivisions, strokeSeed);
    }

    lastDrawnIndex = index;

    // The custom series has no DC term, so the drawn cycle's offset is simply dropped.
    drawnSeries.setFromCycle (yup::Span<const float> (drawnCycle));
    partials.seedHarmonicsFrom (drawnSeries);

    publishEdit();
}

/** Rotates the period by the horizontal distance dragged, from where the drag started. */
void WaveformEditor::applyRotation (const yup::MouseEvent& event)
{
    const auto width = getLocalBounds().reduced (contentInset).getWidth();

    if (width <= 0.0f || ! rotationStartX.has_value())
        return;

    rotated.copyFrom (rotationBase);
    rotated.timeShift ((event.getPosition().getX() - *rotationStartX) / width);
    partials.seedHarmonicsFrom (rotated);

    publishEdit();
}

/** Keeps a finished stroke or operation as one step of the undo history. */
void WaveformEditor::recordEdit (const SynthPartials::Snapshot& before, bool joinsLastStep)
{
    const auto after = partials.getSnapshot();

    if (after == before)
        return;

    // The history outlives the editor, so undoing works on the partials alone; the editor
    // shows the result once it sees the generation move.
    SynthUndoHistory::record<SynthPartials::Snapshot> (*this, before, after, [&partials = partials] (const SynthPartials::Snapshot& snapshot)
    {
        partials.restore (snapshot);
        partials.updateHarmonicScale();
        partials.bumpGeneration();
    }, joinsLastStep);
}

/** Turns a mouse position into the magnitude, or the phase, of one harmonic. */
void WaveformEditor::applyEdit (const yup::MouseEvent& event)
{
    const auto bounds = getPartialsArea();

    if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
        return;

    beginCustomEdit();

    const auto position = event.getPosition();
    const auto count = getVisiblePartialCount();
    const auto barWidth = bounds.getWidth() / static_cast<float> (count);
    const auto index = getFirstVisiblePartial()
                     + yup::jlimit (0, count - 1, static_cast<int> ((position.getX() - bounds.getX()) / barWidth));

    if (partialsTool == PartialsTool::phase)
    {
        const auto phase = yup::jlimit (-1.0f, 1.0f, (bounds.getCenterY() - position.getY()) / (bounds.getHeight() * 0.5f));
        partials.setHarmonicPhase (index, phase * yup::MathConstants<float>::pi);
    }
    else
    {
        partials.setHarmonicMagnitude (index, yup::jlimit (0.0f, 1.0f, (bounds.getBottom() - position.getY()) / bounds.getHeight()));
    }

    publishEdit();
}
