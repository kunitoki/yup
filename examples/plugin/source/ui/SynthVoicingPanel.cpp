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
#include "SynthVoicingPanel.h"
#include "SynthTheme.h"

SynthVoicingPanel::SynthVoicingPanel (const SynthParameters& parametersToEdit)
    : parameters (parametersToEdit)
    , modeGesture (parametersToEdit.playMode)
    , qualitySpinner (parametersToEdit.quality)
    , unisonSpinner (parametersToEdit.unisonVoices)
    , glideKnob ("GLIDE", parametersToEdit.glide, SynthTheme::captionFont (12.0f))
    , detuneKnob ("DETUNE", parametersToEdit.unisonDetune, SynthTheme::captionFont (12.0f))
    , spreadKnob ("SPREAD", parametersToEdit.unisonSpread, SynthTheme::captionFont (12.0f))
{
    setOpaque (false);

    for (auto* control : std::initializer_list<yup::Component*> { &qualitySpinner, &unisonSpinner, &glideKnob, &detuneKnob, &spreadKnob })
        addAndMakeVisible (*control);

    syncFromParameters();
}

SynthPlayMode SynthVoicingPanel::getMode() const
{
    return static_cast<SynthPlayMode> (yup::jlimit (0, 2, yup::roundToInt (parameters.playMode->getValue())));
}

void SynthVoicingPanel::setMode (SynthPlayMode mode)
{
    modeGesture.setNormalized (parameters.playMode->convertToNormalizedValue (static_cast<float> (mode)));
    syncFromParameters();
}

void SynthVoicingPanel::syncFromParameters()
{
    qualitySpinner.syncFromParameter();
    unisonSpinner.syncFromParameter();

    for (auto* knob : { &glideKnob, &detuneKnob, &spreadKnob })
        knob->syncFromParameter();

    const auto mode = getMode();
    glideKnob.setEnabled (mode != SynthPlayMode::poly);

    if (mode == shownMode)
        return;

    shownMode = mode;
    repaint();
}

void SynthVoicingPanel::resized()
{
    auto bounds = getLocalBounds().reduced (10.0f, 6.0f);

    unisonArea = bounds.removeFromRight (unisonWidth);
    bounds.removeFromRight (SynthTheme::controlGap * 2.0f);
    voicingArea = bounds;

    auto unisonTitle = unisonArea.withHeight (titleHeight);
    unisonSpinner.setBounds (unisonTitle.removeFromRight (42.0f).reduced (0.0f, 3.0f));

    auto unisonBody = unisonArea.withTrimmedTop (titleHeight + 4.0f);
    detuneKnob.setBounds (unisonBody.removeFromLeft (unisonBody.getWidth() * 0.5f));
    spreadKnob.setBounds (unisonBody);

    auto body = voicingArea.withTrimmedTop (titleHeight + 4.0f);
    glideKnob.setBounds (body.removeFromRight (56.0f));

    auto checks = body.removeFromLeft (checkRowWidth);
    const auto rowHeight = checks.getHeight() * 0.5f;
    monoBox = checks.removeFromTop (rowHeight).withWidth (checkSize).withSizeKeepingCenter (checkSize, checkSize);
    legatoBox = checks.withWidth (checkSize).withSizeKeepingCenter (checkSize, checkSize);

    body.removeFromLeft (SynthTheme::controlGap);
    qualityCaption = body.removeFromTop (rowHeight).withTrimmedRight (body.getWidth() * 0.45f);
    qualitySpinner.setBounds (qualityCaption.withX (qualityCaption.getRight()).withWidth (body.getWidth() * 0.45f).reduced (0.0f, 5.0f));
}

void SynthVoicingPanel::paint (yup::Graphics& g)
{
    SynthTheme::paintPanel (g, getLocalBounds());

    const auto titleFont = SynthTheme::captionFont (13.5f);

    for (const auto& [area, title] : { std::pair { voicingArea, "VOICING" }, std::pair { unisonArea, "UNISON" } })
    {
        const auto line = area.withHeight (titleHeight);
        g.setFillColor (SynthTheme::textPrimary());
        g.fillFittedText (title, titleFont, line, yup::Justification::centerLeft);
        SynthTheme::paintSeam (g, { line.getX(), line.getBottom() + 1.0f }, { line.getRight(), line.getBottom() + 1.0f });
    }

    const auto seamX = (voicingArea.getRight() + unisonArea.getX()) * 0.5f;
    SynthTheme::paintSeam (g, { seamX, voicingArea.getY() + 4.0f }, { seamX, voicingArea.getBottom() - 4.0f });

    const auto captionFont = SynthTheme::captionFont (12.0f);
    const auto mono = shownMode != SynthPlayMode::poly;
    const auto legato = shownMode == SynthPlayMode::legato;

    for (const auto& [box, text, on, enabled] : { std::tuple { monoBox, "MONO", mono, true }, std::tuple { legatoBox, "LEGATO", legato, mono } })
    {
        SynthTheme::paintWell (g, box, 2.0f);

        if (on)
        {
            g.setFillColor (SynthTheme::accent());
            g.fillRoundedRect (box.reduced (3.0f), 1.0f);
        }

        g.setFillColor (enabled ? SynthTheme::textPrimary() : SynthTheme::textSecondary());
        g.fillFittedText (text, captionFont, { box.getRight() + 7.0f, box.getY() - 4.0f, checkRowWidth - checkSize - 7.0f, box.getHeight() + 8.0f }, yup::Justification::centerLeft);
    }

    g.setFillColor (SynthTheme::textSecondary());
    g.fillFittedText ("QUALITY", captionFont, qualityCaption, yup::Justification::centerLeft);
}

void SynthVoicingPanel::mouseDown (const yup::MouseEvent& event)
{
    const auto position = event.getPosition();
    const auto row = [&position] (yup::Rectangle<float> box) { return box.withWidth (checkRowWidth).enlarged (0.0f, 6.0f).contains (position); };
    const auto mode = getMode();

    if (row (monoBox))
        setMode (mode == SynthPlayMode::poly ? SynthPlayMode::mono : SynthPlayMode::poly);
    else if (row (legatoBox) && mode != SynthPlayMode::poly)
        setMode (mode == SynthPlayMode::legato ? SynthPlayMode::mono : SynthPlayMode::legato);
}
