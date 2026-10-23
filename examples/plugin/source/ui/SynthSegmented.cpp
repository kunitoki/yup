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

#include "SynthSegmented.h"
#include "SynthTheme.h"

SynthSegmented::SynthSegmented (std::vector<Segment> segmentsToShow, bool isVertical)
    : segments (std::move (segmentsToShow))
    , vertical (isVertical)
{
    setOpaque (false);
}

yup::Path SynthSegmented::makePolyline (std::initializer_list<yup::Point<float>> points)
{
    yup::Path path;
    auto first = true;

    for (const auto& point : points)
    {
        if (first)
            path.moveTo (point.getX(), point.getY());
        else
            path.lineTo (point.getX(), point.getY());

        first = false;
    }

    return path;
}

bool SynthSegmented::setSelectedIndex (int index)
{
    if (index == selectedIndex)
        return false;

    selectedIndex = index;
    repaint();
    return true;
}

yup::Rectangle<float> SynthSegmented::getSegmentBounds (int index) const
{
    const auto inner = getLocalBounds().reduced (2.0f);
    const auto count = static_cast<float> (yup::jmax (1, static_cast<int> (segments.size())));

    if (vertical)
    {
        const auto height = inner.getHeight() / count;
        return inner.withY (inner.getY() + height * static_cast<float> (index)).withHeight (height);
    }

    const auto width = inner.getWidth() / count;
    return inner.withX (inner.getX() + width * static_cast<float> (index)).withWidth (width);
}

void SynthSegmented::paint (yup::Graphics& g)
{
    SynthTheme::paintWell (g, *this, getLocalBounds());

    for (int index = 0; index < static_cast<int> (segments.size()); ++index)
    {
        const auto& segment = segments[static_cast<std::size_t> (index)];
        const auto bounds = getSegmentBounds (index);
        const auto isSelected = index == selectedIndex;

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

void SynthSegmented::mouseDown (const yup::MouseEvent& event)
{
    for (int index = 0; index < static_cast<int> (segments.size()); ++index)
    {
        if (! getSegmentBounds (index).contains (event.getPosition()))
            continue;

        setSelectedIndex (index);

        if (onSelect != nullptr)
            onSelect (index);

        return;
    }
}
