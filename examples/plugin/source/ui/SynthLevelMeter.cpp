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
#include "SynthLevelMeter.h"
#include "SynthTheme.h"

#include <yup_audio_basics/yup_audio_basics.h>

SynthLevelMeter::SynthLevelMeter()
{
    setOpaque (false);
}

void SynthLevelMeter::setLevels (float left, float right)
{
    const std::array<float, 2> peaks { left, right };
    auto changed = false;

    for (std::size_t channel = 0; channel < peaks.size(); ++channel)
    {
        const auto decibels = yup::jmax (floorDecibels, yup::Decibels::gainToDecibels (peaks[channel], floorDecibels));
        const auto shown = yup::jmax (decibels, shownDecibels[channel] - fallPerCall);

        changed = changed || shown != shownDecibels[channel];
        shownDecibels[channel] = shown;
    }

    if (changed)
        repaint();
}

void SynthLevelMeter::paint (yup::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto barWidth = (bounds.getWidth() - 2.0f) * 0.5f;

    for (const auto decibels : shownDecibels)
    {
        const auto bar = bounds.removeFromLeft (barWidth);
        bounds.removeFromLeft (2.0f);

        SynthTheme::paintWell (g, bar, 2.0f);

        const auto proportion = (decibels - floorDecibels) / -floorDecibels;
        const auto height = (bar.getHeight() - 2.0f) * yup::jlimit (0.0f, 1.0f, proportion);

        g.setFillColor (decibels > -1.0f ? SynthTheme::accent().brighter (0.3f) : SynthTheme::accent());
        g.fillRoundedRect (bar.reduced (1.0f).withTrimmedTop (bar.getHeight() - 2.0f - height), 1.0f);
    }
}
