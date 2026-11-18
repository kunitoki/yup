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

#include "Figures.h"

//==============================================================================

void addImagingFigures (Figures& f)
{
    using namespace yup;

    // An image written pixel by pixel, magnified with each filter
    f.paint ("imaging/pixels_pixels", { 480.0f, 170.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        Image image { 16, 8 };

        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                image.setPixelColor (x, y, Color (static_cast<uint8> (x * 16), static_cast<uint8> (60 + y * 24), 220));

        image.setPixelColor (10, 2, Colors::white);

        labelled (g, area, { "ImageFilter::Nearest", "ImageFilter::Linear" }, 2, [&] (int index, Rectangle<float> cell)
        {
            const auto target = cell.withSizeKeepingCenter (192.0f, 96.0f);
            const auto filter = index == 0 ? ImageFilter::Nearest : ImageFilter::Linear;

            g.setFillImage (image, AffineTransform::scaling (12.0f).translated (target.getX(), target.getY()), { ImageWrap::Clamp, ImageWrap::Clamp, filter });
            g.fillRect (target);
        });
    });
}
