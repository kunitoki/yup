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

SynthEnvelopePanel::SynthEnvelopePanel (const yup::String& panelTitle,
                                        const SynthEnvelopeParameters& parametersToEdit,
                                        const SynthCurveState& curvesToShow,
                                        int envelopeIndex,
                                        const yup::Font& font)
    : curves (curvesToShow)
    , index (envelopeIndex)
    , timeScaleKnob ("TIME", parametersToEdit.timeScale, font)
    , velocityKnob ("VELOCITY", parametersToEdit.velocity, font)
    , depthKnob ("DEPTH", parametersToEdit.depth, font)
{
    setupPanelTitle (titleLabel, panelTitle, font);
    addAndMakeVisible (titleLabel);

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
    auto bounds = getLocalBounds().reduced (panelInset);

    titleLabel.setBounds (bounds.removeFromTop (headerHeight));
    bounds.removeFromTop (spacing);

    auto knobs = bounds.removeFromRight (knobWidth * 3.0f);
    bounds.removeFromRight (spacing);
    display.setBounds (bounds);

    layoutControlsInRow (knobs, { &timeScaleKnob, &velocityKnob, &depthKnob });
}

void SynthEnvelopePanel::paint (yup::Graphics& g)
{
    paintSynthPanel (g, getLocalBounds());
}
