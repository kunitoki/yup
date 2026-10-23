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
#include "ParameterSegmented.h"
#include "SynthTheme.h"

ParameterSegmented::ParameterSegmented (yup::AudioParameter::Ptr choiceParameter, std::vector<Segment> segmentsToShow)
    : gesture (std::move (choiceParameter))
    , segments (std::move (segmentsToShow))
{
    setOpaque (false);
    syncFromParameter();
}

int ParameterSegmented::getSelectedIndex() const
{
    return yup::roundToInt (gesture.getParameter()->getValue());
}

bool ParameterSegmented::syncFromParameter()
{
    const auto index = getSelectedIndex();

    if (index == shownIndex)
        return false;

    shownIndex = index;
    repaint();
    return true;
}

yup::Rectangle<float> ParameterSegmented::getSegmentBounds (int index) const
{
    const auto inner = getLocalBounds().reduced (2.0f);
    const auto width = inner.getWidth() / static_cast<float> (yup::jmax (1, static_cast<int> (segments.size())));

    return inner.withX (inner.getX() + width * static_cast<float> (index)).withWidth (width);
}

void ParameterSegmented::paint (yup::Graphics& g)
{
    SynthTheme::paintWell (g, *this, getLocalBounds());

    const auto selected = getSelectedIndex();

    for (int index = 0; index < static_cast<int> (segments.size()); ++index)
    {
        const auto& segment = segments[static_cast<std::size_t> (index)];
        const auto bounds = getSegmentBounds (index);
        const auto isSelected = index == selected;

        if (isSelected)
            SynthTheme::paintRaised (g, *this, bounds, 2.0f);

        const auto color = ! isEnabled() ? SynthTheme::textSecondary (*this).withAlpha (0.5f)
                                         : (isSelected ? SynthTheme::textPrimary (*this) : SynthTheme::textSecondary (*this));

        if (! segment.icon.isEmpty())
        {
            const auto box = bounds.withSizeKeepingCenter (yup::jmin (bounds.getWidth(), bounds.getHeight()) * 0.6f, bounds.getHeight() * 0.6f);
            const auto transform = yup::AffineTransform::scaling (box.getWidth(), box.getHeight()).translated (box.getX(), box.getY());

            g.setStrokeColor (color);
            g.setStrokeWidth (1.4f);
            g.setStrokeCap (yup::StrokeCap::Round);
            g.strokePath (segment.icon.transformed (transform));
            continue;
        }

        g.setFillColor (color);
        g.fillFittedText (segment.text.toUpperCase(), SynthTheme::captionFont (yup::jmin (12.0f, bounds.getHeight() * 0.7f)), bounds, yup::Justification::center);
    }
}

void ParameterSegmented::mouseDown (const yup::MouseEvent& event)
{
    for (int index = 0; index < static_cast<int> (segments.size()); ++index)
    {
        if (! getSegmentBounds (index).contains (event.getPosition()))
            continue;

        const auto& parameter = gesture.getParameter();
        gesture.setNormalized (parameter->convertToNormalizedValue (static_cast<float> (index)));
        syncFromParameter();
        return;
    }
}
