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

#include "SynthLFOPanel.h"
#include "SynthTheme.h"
#include "SynthUndoHistory.h"

SynthLFOPanel::SynthLFOPanel (const SynthLFOParameters& parametersToEdit,
                              SynthCurveState& curvesToEdit,
                              int lfoIndex,
                              std::shared_ptr<SynthWaveformShader> landscapeShader)
    : parameters (parametersToEdit)
    , curves (curvesToEdit)
    , index (lfoIndex)
    , shownValues (parametersToEdit.read())
    , shapeSelector (parametersToEdit.shape, makeShapeSegments())
    , retriggerChoice ({}, parametersToEdit.retrigger, SynthTheme::valueFont (13.5f))
    , display (landscapeShader)
    , editor (landscapeShader)
    , invertLed (parametersToEdit.inverted, "Invert")
    , handle (static_cast<SynthModulationSource> (static_cast<int> (SynthModulationSource::lfo1) + lfoIndex))
    , rateKnob ("RATE", parametersToEdit.rate, SynthTheme::captionFont (12.0f))
    , phaseKnob ("PHASE", parametersToEdit.phase, SynthTheme::captionFont (12.0f))
    , smoothKnob ("SMOOTH", parametersToEdit.smoothing, SynthTheme::captionFont (12.0f))
    , depthKnob ("DEPTH", parametersToEdit.depth, SynthTheme::captionFont (12.0f))
{
    setCachedToTexture (true);

    setOpaque (false);

    editor.setCycle (true);
    editor.onCurveEdited = [this] (const SynthCurveState::Curve& curve) { curves.setLFO (index, curve); };
    editor.onEditFinished = [this] (const SynthCurveState::Curve& before)
    {
        const auto after = curves.getLFO (index);

        if (after == before)
            return;

        SynthUndoHistory::record<SynthCurveState::Curve> (*this, before, after, [&curves = curves, lfo = index] (const SynthCurveState::Curve& curve)
        {
            curves.setLFO (lfo, curve);
        });
    };

    snapBox.addItemList (SynthTimeGrid::getSnapNames (false), 1);
    snapBox.setSelectedId (defaultSnap + 1, yup::dontSendNotification);
    snapBox.onSelectedItemChanged = [this] { editor.setSnap (snapBox.getSelectedId() - 1); };
    editor.setSnap (defaultSnap);

    for (auto* control : std::initializer_list<yup::Component*> { &shapeSelector, &retriggerChoice, &display, &invertLed, &snapBox })
        addAndMakeVisible (*control);

    addChildComponent (editor);
    showValues();

    for (auto* knob : { &rateKnob, &phaseKnob, &smoothKnob, &depthKnob })
        addAndMakeVisible (*knob);

    rateKnob.setCaptionShowsValue (true);
    syncFromParameters();
}

std::vector<ParameterSegmented::Segment> SynthLFOPanel::makeShapeSegments()
{
    const auto polyline = &SynthSegmented::makePolyline;

    yup::Path sine;
    sine.moveTo (0.0f, 0.5f);
    sine.cubicTo (0.2f, -0.15f, 0.3f, -0.15f, 0.5f, 0.5f);
    sine.cubicTo (0.7f, 1.15f, 0.8f, 1.15f, 1.0f, 0.5f);

    yup::Path smoothRandom;
    smoothRandom.moveTo (0.0f, 0.6f);
    smoothRandom.cubicTo (0.15f, 0.1f, 0.3f, 0.2f, 0.42f, 0.55f);
    smoothRandom.cubicTo (0.55f, 0.95f, 0.75f, 0.9f, 0.82f, 0.4f);
    smoothRandom.cubicTo (0.88f, 0.1f, 0.95f, 0.2f, 1.0f, 0.3f);

    // In the order of getSynthLFOShapeNames().
    return { { "Sine", sine },
             { "Triangle", polyline ({ { 0.0f, 0.5f }, { 0.25f, 0.0f }, { 0.75f, 1.0f }, { 1.0f, 0.5f } }) },
             { "Saw Up", polyline ({ { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } }) },
             { "Square", polyline ({ { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 1.0f } }) },
             { "S&H", polyline ({ { 0.0f, 0.7f }, { 0.25f, 0.7f }, { 0.25f, 0.1f }, { 0.5f, 0.1f }, { 0.5f, 0.9f }, { 0.75f, 0.9f }, { 0.75f, 0.4f }, { 1.0f, 0.4f } }) },
             { "Saw Down", polyline ({ { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f } }) },
             { "Smooth Random", smoothRandom },
             { "Custom", polyline ({ { 0.0f, 1.0f }, { 0.2f, 0.2f }, { 0.45f, 0.6f }, { 0.7f, 0.0f }, { 1.0f, 1.0f } }) } };
}

void SynthLFOPanel::setPhase (float phase)
{
    display.setPhase (phase);

    if (! isCustom())
        return;

    if (phase < 0.0f)
    {
        editor.setPlayhead (-1.0f);
        return;
    }

    // The editor's playhead counts points: find the segment the cycle position is in.
    const auto curve = curves.getLFO (index);
    const auto position = phase + shownValues.phase - std::floor (phase + shownValues.phase);
    auto pointPosition = static_cast<float> (curve.getNumPoints() - 1);

    for (int point = 1; point < curve.getNumPoints(); ++point)
    {
        const auto from = curve.getPoint (point - 1).time;
        const auto to = curve.getPoint (point).time;

        if (position < to)
        {
            pointPosition = static_cast<float> (point - 1) + (to > from ? (position - from) / (to - from) : 0.0f);
            break;
        }
    }

    editor.setPlayhead (pointPosition);
}

void SynthLFOPanel::showValues()
{
    display.setValues (shownValues);
    editor.setInverted (shownValues.inverted);
    display.setVisible (! isCustom());
    editor.setVisible (isCustom());
    snapBox.setEnabled (isCustom());
    updateTiming();
}

void SynthLFOPanel::setTransport (const SynthTransport& newTransport)
{
    if (newTransport == transport)
        return;

    transport = newTransport;
    display.setTempo (transport.bpm);
    updateTiming();
}

void SynthLFOPanel::updateTiming()
{
    // The two snapping lists differ, so a choice the new one lacks turns snapping to its default.
    if (shownValues.tempoSync != shownMusical)
    {
        shownMusical = shownValues.tempoSync;
        const auto choice = snapBox.getSelectedId();
        const auto names = SynthTimeGrid::getSnapNames (shownMusical);

        snapBox.clear();
        snapBox.addItemList (names, 1);
        snapBox.setSelectedId (choice >= 1 && choice <= names.size() ? choice : defaultSnap + 1, yup::dontSendNotification);
        editor.setSnap (snapBox.getSelectedId() - 1);
    }

    // One cycle lasts the division in beats when synced, or one period in seconds.
    const auto& divisions = getSynthLFODivisions();
    const auto division = divisions[static_cast<std::size_t> (yup::jlimit (0, static_cast<int> (divisions.size()) - 1, shownValues.division))];
    const auto cycleLength = shownValues.tempoSync ? division.beats : 1.0 / yup::jmax (0.01, static_cast<double> (shownValues.rate));
    const SynthTimeGrid::Timing timing { shownValues.tempoSync, cycleLength, transport.numerator, transport.denominator };

    display.setTiming (timing);
    editor.setTiming (timing);
}

void SynthLFOPanel::syncFromParameters()
{
    shapeSelector.syncFromParameter();
    retriggerChoice.syncFromParameter();

    invertLed.syncFromParameter();

    if (const auto values = parameters.read(); values != shownValues)
    {
        shownValues = values;
        showValues();
    }

    rateKnob.setParameter (shownValues.tempoSync ? parameters.division : parameters.rate);

    for (auto* knob : { &rateKnob, &phaseKnob, &smoothKnob, &depthKnob })
        knob->syncFromParameter();

    if (const auto generation = curves.getGeneration(); generation != shownGeneration)
    {
        shownGeneration = generation;
        display.setCurve (curves.getLFO (index));
        editor.setCurve (curves.getLFO (index));
    }
}

void SynthLFOPanel::resized()
{
    const auto gap = SynthTheme::controlGap;
    auto bounds = getLocalBounds();

    auto line = bounds.removeFromTop (lineHeight);
    retriggerChoice.setBounds (line.removeFromRight (retriggerWidth));
    line.removeFromRight (gap);
    shapeSelector.setBounds (line.withWidth (yup::jmin (line.getWidth(), static_cast<float> (getSynthLFOShapeNames().size()) * 26.0f)));
    bounds.removeFromTop (gap);

    auto knobs = bounds.removeFromBottom (knobRowHeight);
    bounds.removeFromBottom (gap);
    display.setBounds (bounds);
    editor.setBounds (bounds);

    auto options = knobs.removeFromRight (optionsWidth).reduced (0.0f, 8.0f);
    invertLed.setBounds (options.removeFromTop (options.getHeight() * 0.5f));
    snapBox.setBounds (options);
    knobs.removeFromRight (gap);

    rateKnob.setBounds (knobs.removeFromLeft (knobs.getWidth() * 0.31f));
    const auto width = knobs.getWidth() / 3.0f;

    for (auto* knob : { &phaseKnob, &smoothKnob, &depthKnob })
        knob->setBounds (knobs.removeFromLeft (width).reduced (0.0f, 6.0f));
}
