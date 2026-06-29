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

namespace yup
{

//==============================================================================
// AnimationGradient

void AnimationGradient::addColorStop (float pos, Color color)
{
    ColorStop stop;
    stop.position.withStaticValue (pos);
    stop.color.withStaticValue (color);
    colorStops.push_back (std::move (stop));
}

ColorGradient AnimationGradient::toColorGradient (float frameNo) const
{
    const Point<float> start = startPoint.getValueAt (frameNo);
    const Point<float> end = endPoint.getValueAt (frameNo);

    if (colorStops.empty())
        return ColorGradient (Color(), start, Color(), end, gradientType == GradientType::Radial ? ColorGradient::Type::Radial : ColorGradient::Type::Linear);

    const Color c0 = colorStops.front().color.getValueAt (frameNo);
    const Color c1 = colorStops.back().color.getValueAt (frameNo);

    ColorGradient gradient (c0, start, c1, end, gradientType == GradientType::Radial ? ColorGradient::Type::Radial : ColorGradient::Type::Linear);

    for (size_t i = 1; i + 1 < colorStops.size(); ++i)
    {
        const float delta = colorStops[i].position.getValueAt (frameNo);
        const Color col = colorStops[i].color.getValueAt (frameNo);
        gradient.addColorStop (col, delta);
    }

    return gradient;
}

//==============================================================================
// FillPaint

Color FillPaint::colorAt (float frameNo) const
{
    return color.getValueAt (frameNo);
}

float FillPaint::opacityAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, opacity.getValueAt (frameNo) / 100.0f);
}

//==============================================================================
// StrokePaint

Color StrokePaint::colorAt (float frameNo) const
{
    return color.getValueAt (frameNo);
}

float StrokePaint::opacityAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, opacity.getValueAt (frameNo) / 100.0f);
}

float StrokePaint::widthAt (float frameNo) const
{
    return width.getValueAt (frameNo);
}

StrokeType StrokePaint::strokeTypeAt (float frameNo) const
{
    return StrokeType (widthAt (frameNo), join, cap);
}

} // namespace yup
