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

#include "SynthEnvelopePanel.h"
#include "SynthTheme.h"

SynthEnvelopePanel::SynthEnvelopePanel (const SynthEnvelopeParameters& parametersToEdit, SynthCurveState& curvesToEdit, int envelopeIndex)
    : curves (curvesToEdit)
    , index (envelopeIndex)
    , timeScaleKnob ("TIME", parametersToEdit.timeScale, SynthTheme::captionFont (12.0f))
    , velocityKnob ("VELOCITY", parametersToEdit.velocity, SynthTheme::captionFont (12.0f))
    , depthKnob ("DEPTH", parametersToEdit.depth, SynthTheme::captionFont (12.0f))
{
    setOpaque (false);

    display.onCurveEdited = [this] (const SynthCurveState::Curve& curve) { curves.setEnvelope (index, curve); };
    addAndMakeVisible (display);

    for (auto* knob : { &timeScaleKnob, &velocityKnob, &depthKnob })
        addAndMakeVisible (*knob);

    syncFromParameters();
}

void SynthEnvelopePanel::syncFromParameters()
{
    for (auto* knob : { &timeScaleKnob, &velocityKnob, &depthKnob })
        knob->syncFromParameter();

    if (const auto generation = curves.getGeneration(); generation != shownGeneration)
    {
        shownGeneration = generation;
        display.setCurve (curves.getEnvelope (index));
    }
}

void SynthEnvelopePanel::resized()
{
    auto bounds = getLocalBounds();
    auto knobs = bounds.removeFromBottom (knobRowHeight);
    bounds.removeFromBottom (SynthTheme::controlGap);
    display.setBounds (bounds);

    const auto width = knobs.getWidth() / 3.0f;

    for (auto* knob : { &timeScaleKnob, &velocityKnob, &depthKnob })
        knob->setBounds (knobs.removeFromLeft (width).reduced (0.0f, 6.0f));
}
