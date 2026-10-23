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

SynthLFOPanel::SynthLFOPanel (const SynthLFOParameters& parametersToEdit, const SynthCurveState& curvesToShow, int lfoIndex)
    : parameters (parametersToEdit)
    , curves (curvesToShow)
    , index (lfoIndex)
    , shownValues (parametersToEdit.read())
    , shapeSelector (parametersToEdit.shape, makeShapeSegments())
    , retriggerChoice ({}, parametersToEdit.retrigger, SynthTheme::valueFont (13.5f))
    , rateKnob ("RATE", parametersToEdit.rate, SynthTheme::captionFont (12.0f))
    , phaseKnob ("PHASE", parametersToEdit.phase, SynthTheme::captionFont (12.0f))
    , smoothKnob ("SMOOTH", parametersToEdit.smoothing, SynthTheme::captionFont (12.0f))
    , depthKnob ("DEPTH", parametersToEdit.depth, SynthTheme::captionFont (12.0f))
{
    setCachedToTexture (true);

    setOpaque (false);

    display.setValues (shownValues);

    for (auto* control : std::initializer_list<yup::Component*> { &shapeSelector, &retriggerChoice, &display })
        addAndMakeVisible (*control);

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
             { "Smooth Random", smoothRandom } };
}

void SynthLFOPanel::syncFromParameters()
{
    shapeSelector.syncFromParameter();
    retriggerChoice.syncFromParameter();

    if (const auto values = parameters.read(); values != shownValues)
    {
        shownValues = values;
        display.setValues (values);
    }

    rateKnob.setParameter (shownValues.tempoSync ? parameters.division : parameters.rate);

    for (auto* knob : { &rateKnob, &phaseKnob, &smoothKnob, &depthKnob })
        knob->syncFromParameter();

    if (const auto generation = curves.getGeneration(); generation != shownGeneration)
    {
        shownGeneration = generation;
        display.setCurve (curves.getLFO (index));
    }
}

void SynthLFOPanel::resized()
{
    const auto gap = SynthTheme::controlGap;
    auto bounds = getLocalBounds();

    auto line = bounds.removeFromTop (lineHeight);
    retriggerChoice.setBounds (line.removeFromRight (retriggerWidth));
    line.removeFromRight (gap);
    shapeSelector.setBounds (line.withWidth (yup::jmin (line.getWidth(), 7.0f * 26.0f)));
    bounds.removeFromTop (gap);

    auto knobs = bounds.removeFromBottom (knobRowHeight);
    bounds.removeFromBottom (gap);
    display.setBounds (bounds);

    rateKnob.setBounds (knobs.removeFromLeft (knobs.getWidth() * 0.31f));
    const auto width = knobs.getWidth() / 3.0f;

    for (auto* knob : { &phaseKnob, &smoothKnob, &depthKnob })
        knob->setBounds (knobs.removeFromLeft (width).reduced (0.0f, 6.0f));
}
