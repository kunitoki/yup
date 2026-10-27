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
#include "SynthSliderField.h"
#include "SynthTheme.h"

SynthSliderField::SynthSliderField (const yup::String& captionText, yup::AudioParameter::Ptr parameterToControl, Look lookToUse, bool isBipolar)
    : gesture (*this, std::move (parameterToControl))
    , caption (captionText)
    , look (lookToUse)
    , bipolar (isBipolar)
{
    setOpaque (false);
    setMouseCursor (yup::MouseCursor::Hand);
    syncFromParameter();
}

void SynthSliderField::setModulationArcs (std::vector<SynthKnob::ModulationArc> newArcs)
{
    if (newArcs == modulationArcs)
        return;

    modulationArcs = std::move (newArcs);
    repaint();
}

void SynthSliderField::showModulation (std::vector<SynthKnob::ModulationArc> arcs, std::optional<float> modulatedValue)
{
    setModulationArcs (std::move (arcs));
    setModulatedValue (modulatedValue);
}

void SynthSliderField::setModulatedValue (std::optional<float> newModulatedValue)
{
    if (newModulatedValue.has_value() == modulatedValue.has_value()
        && (! newModulatedValue.has_value() || std::abs (*newModulatedValue - *modulatedValue) < 1.0e-4f))
        return;

    modulatedValue = newModulatedValue;
    repaint();
}

bool SynthSliderField::syncFromParameter()
{
    const auto value = gesture.getParameter()->getNormalizedValue();

    if (std::abs (value - shownValue) < 1.0e-6f)
        return false;

    shownValue = value;
    repaint();
    return true;
}

yup::Rectangle<float> SynthSliderField::getTrackBounds() const
{
    auto bounds = getLocalBounds();

    if (look == Look::bar)
        return bounds.withTrimmedLeft (captionWidth).withTrimmedRight (valueWidth).withSizeKeepingCenter (bounds.getWidth() - captionWidth - valueWidth, 4.0f);

    bounds = bounds.reduced (9.0f, 0.0f);
    return bounds.withSizeKeepingCenter (bounds.getWidth(), 4.0f).withY (bounds.getCenterY() + bounds.getHeight() * 0.18f);
}

void SynthSliderField::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto track = getTrackBounds();
    const auto& parameter = gesture.getParameter();
    const auto accent = isEnabled() ? SynthTheme::accent (*this) : SynthTheme::textSecondary (*this);
    const auto value = parameter->getNormalizedValue();

    if (look == Look::boxed)
    {
        SynthTheme::paintWell (g, *this, bounds);

        const auto text = bounds.reduced (9.0f, 0.0f).withHeight (track.getY() - 2.0f - bounds.getY());
        g.setFillColor (SynthTheme::textSecondary (*this));
        g.fillFittedText (caption.toUpperCase(), SynthTheme::captionFont (11.5f), text, yup::Justification::bottomLeft);
        g.setFillColor (SynthTheme::textPrimary (*this));
        g.fillFittedText (parameter->toString(), SynthTheme::valueFont (13.0f), text, yup::Justification::bottomRight);
    }
    else
    {
        g.setFillColor (SynthTheme::textSecondary (*this));
        g.fillFittedText (caption.toUpperCase(), SynthTheme::captionFont (11.5f), bounds.withWidth (captionWidth), yup::Justification::centerLeft);
        g.setFillColor (accent);
        g.fillFittedText (parameter->toString(), SynthTheme::valueFont (12.0f), bounds.withTrimmedLeft (bounds.getWidth() - valueWidth), yup::Justification::centerRight);
    }

    g.setFillColor (SynthTheme::wellColor (*this).mixedWith (SynthTheme::panelColor (*this), 0.35f, yup::ColorSpace::SRGB));
    g.fillRoundedRect (track, 2.0f);

    const auto positionOf = [&track] (float normalized) { return track.getX() + track.getWidth() * yup::jlimit (0.0f, 1.0f, normalized); };
    const auto origin = positionOf (bipolar ? 0.5f : 0.0f);
    const auto current = positionOf (value);

    g.setFillColor (accent);
    g.fillRoundedRect (track.withX (yup::jmin (origin, current)).withWidth (std::abs (current - origin)), 2.0f);

    if (bipolar)
    {
        g.setFillColor (SynthTheme::textSecondary (*this));
        g.fillRect (origin - 0.5f, track.getY() - 2.0f, 1.0f, track.getHeight() + 4.0f);
    }

    auto lane = track.getBottom() + 2.0f;

    for (const auto& arc : modulationArcs)
    {
        const auto from = arc.bipolar ? positionOf (value - arc.depth) : current;
        const auto to = positionOf (value + arc.depth);
        g.setFillColor (arc.color);
        g.fillRect (yup::jmin (from, to), lane, std::abs (to - from), 1.5f);
        lane += 2.5f;
    }

    if (modulatedValue.has_value())
    {
        g.setFillColor (SynthTheme::textPrimary (*this));
        g.fillRoundedRect (positionOf (*modulatedValue) - 1.5f, track.getY() - 3.0f, 3.0f, track.getHeight() + 6.0f, 1.5f);
    }
}

void SynthSliderField::mouseDown (const yup::MouseEvent&)
{
    gesture.begin();
}

void SynthSliderField::mouseDrag (const yup::MouseEvent& event)
{
    if (! gesture.isActive())
        return;

    const auto width = yup::jmax (1.0f, getTrackBounds().getWidth());
    const auto travel = event.getPosition().getX() - event.getLastMouseDownPosition().getX();
    const auto scale = event.getModifiers().isShiftDown() ? 0.1f : 1.0f;

    gesture.setNormalized (gesture.getStartValue() + travel / width * scale);
    syncFromParameter();
}

void SynthSliderField::mouseUp (const yup::MouseEvent&)
{
    gesture.end();
}

void SynthSliderField::mouseDoubleClick (const yup::MouseEvent&)
{
    gesture.resetToDefault();
    syncFromParameter();
}
