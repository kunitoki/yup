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

#pragma once

#include <yup_graphics/yup_graphics.h>

//==============================================================================
/** The outlines of the "NPC" logo letters, fitted in an area.

    Every letter is a closed shape that never crosses itself, the P with its counter as a second
    contour: stroke the path to draw the letters in outline, or fill it for solid letters.

    @param area The area the three letters fill, with gaps between them.
*/
inline yup::Path createNpcLogoPath (yup::Rectangle<float> area)
{
    const auto height = area.getHeight();
    const auto gap = height * 0.25f;
    const auto width = (area.getWidth() - gap * 2.0f) / 3.0f;
    const auto stem = height * 0.24f;
    const auto top = area.getY();
    const auto bottom = area.getBottom();

    yup::Path path;

    // N: two stems joined by a diagonal band
    auto x = area.getX();
    const auto band = height * 0.36f;
    path.moveTo (x, bottom)
        .lineTo (x, top)
        .lineTo (x + stem, top)
        .lineTo (x + width - stem, bottom - band)
        .lineTo (x + width - stem, top)
        .lineTo (x + width, top)
        .lineTo (x + width, bottom)
        .lineTo (x + width - stem, bottom)
        .lineTo (x + stem, top + band)
        .lineTo (x + stem, bottom)
        .close();

    // P: a stem with a rounded bowl, and the counter of the bowl
    x += width + gap;
    const auto bowl = top + height * 0.62f;
    const auto shoulder = x + width * 0.58f;
    path.moveTo (x, bottom)
        .lineTo (x, top)
        .lineTo (shoulder, top)
        .cubicTo (x + width, top, x + width, bowl, shoulder, bowl)
        .lineTo (x + stem, bowl)
        .lineTo (x + stem, bottom)
        .close();

    path.moveTo (x + stem, top + stem)
        .lineTo (shoulder, top + stem)
        .cubicTo (x + width - stem, top + stem, x + width - stem, bowl - stem, shoulder, bowl - stem)
        .lineTo (x + stem, bowl - stem)
        .close();

    // C: square ends with rounded corners on the left
    x += width + gap;
    const auto corner = height * 0.42f;
    path.moveTo (x + width, top)
        .lineTo (x + corner, top)
        .cubicTo (x, top, x, top, x, top + corner)
        .lineTo (x, bottom - corner)
        .cubicTo (x, bottom, x, bottom, x + corner, bottom)
        .lineTo (x + width, bottom)
        .lineTo (x + width, bottom - stem)
        .lineTo (x + corner, bottom - stem)
        .cubicTo (x + stem, bottom - stem, x + stem, bottom - stem, x + stem, bottom - corner)
        .lineTo (x + stem, top + corner)
        .cubicTo (x + stem, top + stem, x + stem, top + stem, x + corner, top + stem)
        .lineTo (x + width, top + stem)
        .close();

    return path;
}
