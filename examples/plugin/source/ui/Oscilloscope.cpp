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

#include "Oscilloscope.h"
#include "SynthTheme.h"

Oscilloscope::Oscilloscope()
    : Component ("Oscilloscope")
{
}

void Oscilloscope::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();

    g.setFillColor (SynthTheme::displayBackground());
    g.fillRoundedRect (bounds, 4.0f);

    g.setStrokeColor (SynthTheme::panelBorder());
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);
    g.strokeLine (bounds.getX(), bounds.getCenterY(), bounds.getRight(), bounds.getCenterY());

    if (renderData.empty())
        return;

    const auto pointCount = yup::jmin (512, static_cast<int> (renderData.size()));
    const auto xSize = bounds.getWidth() / static_cast<float> (yup::jmax (1, pointCount - 1));

    path.clear();
    path.reserveSpace (pointCount);
    path.moveTo (bounds.getX(), bounds.getCenterY() - renderData[0] * bounds.getHeight() * 0.45f);

    for (int i = 1; i < pointCount; ++i)
    {
        const auto sample = static_cast<std::size_t> (i) * (renderData.size() - 1) / static_cast<std::size_t> (pointCount - 1);
        path.lineTo (bounds.getX() + static_cast<float> (i) * xSize,
                     bounds.getCenterY() - renderData[sample] * bounds.getHeight() * 0.45f);
    }

    g.setStrokeColor (SynthTheme::accent().withAlpha (0.5f));
    g.setStrokeWidth (4.0f);
    g.setFeather (8.0f);
    g.strokePath (path);

    g.setFeather (4.0f);
    g.strokePath (path);

    g.setFeather (0.0f);
    g.setStrokeColor (SynthTheme::accent());
    g.setStrokeWidth (1.5f);
    g.strokePath (path);
}
