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
#include "SynthUndoHistory.h"

SynthEnvelopePanel::SynthEnvelopePanel (const SynthEnvelopeParameters& parametersToEdit, SynthCurveState& curvesToEdit, int envelopeIndex)
    : parameters (parametersToEdit)
    , curves (curvesToEdit)
    , index (envelopeIndex)
    , syncLed (parametersToEdit.tempoSync, "Sync")
    , handle (static_cast<SynthModulationSource> (static_cast<int> (SynthModulationSource::env1) + envelopeIndex))
    , timeScaleKnob ("TIME", parametersToEdit.timeScale, SynthTheme::captionFont (12.0f))
    , velocityKnob ("VELOCITY", parametersToEdit.velocity, SynthTheme::captionFont (12.0f))
    , depthKnob ("DEPTH", parametersToEdit.depth, SynthTheme::captionFont (12.0f))
{
    setCachedToTexture (true);

    setOpaque (false);

    display.setAmplitude (index == 0);
    display.onCurveEdited = [this] (const SynthCurveState::Curve& curve) { curves.setEnvelope (index, curve); };
    display.onEditFinished = [this] (const SynthCurveState::Curve& before)
    {
        // Read back from the state, which may have adjusted the curve, so undo restores what played.
        const auto after = curves.getEnvelope (index);

        if (after == before)
            return;

        SynthUndoHistory::record<SynthCurveState::Curve> (*this, before, after, [&curves = curves, envelope = index] (const SynthCurveState::Curve& curve)
        {
            curves.setEnvelope (envelope, curve);
        });
    };
    addAndMakeVisible (display);

    for (auto* knob : { &timeScaleKnob, &velocityKnob, &depthKnob })
        addAndMakeVisible (*knob);

    snapBox.addItemList (SynthTimeGrid::getSnapNames (false), 1);
    snapBox.setSelectedId (defaultSnap + 1, yup::dontSendNotification);
    snapBox.onSelectedItemChanged = [this] { display.setSnap (snapBox.getSelectedId() - 1); };
    display.setSnap (defaultSnap);

    addAndMakeVisible (snapBox);

    syncFromParameters();
}

void SynthEnvelopePanel::setTransport (const SynthTransport& newTransport)
{
    if (newTransport == transport)
        return;

    transport = newTransport;
    updateTiming();
}

void SynthEnvelopePanel::updateTiming()
{
    const auto values = parameters.read();

    // The two snapping lists differ, so a choice the new one lacks turns snapping to its default.
    if (values.tempoSync != shownMusical)
    {
        shownMusical = values.tempoSync;
        const auto choice = snapBox.getSelectedId();
        const auto names = SynthTimeGrid::getSnapNames (shownMusical);

        snapBox.clear();
        snapBox.addItemList (names, 1);
        snapBox.setSelectedId (choice >= 1 && choice <= names.size() ? choice : defaultSnap + 1, yup::dontSendNotification);
        display.setSnap (snapBox.getSelectedId() - 1);
    }

    display.setTiming ({ values.tempoSync, values.timeScale, transport.numerator, transport.denominator });
}

void SynthEnvelopePanel::syncFromParameters()
{
    for (auto* knob : { &timeScaleKnob, &velocityKnob, &depthKnob })
        knob->syncFromParameter();

    syncLed.syncFromParameter();
    updateTiming();

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

    auto options = knobs.removeFromRight (optionsWidth).reduced (0.0f, 8.0f);
    options.removeFromTop (options.getHeight() * 0.5f);
    snapBox.setBounds (options);
    knobs.removeFromRight (SynthTheme::controlGap);

    const auto width = knobs.getWidth() / 3.0f;

    for (auto* knob : { &timeScaleKnob, &velocityKnob, &depthKnob })
        knob->setBounds (knobs.removeFromLeft (width).reduced (0.0f, 6.0f));
}
