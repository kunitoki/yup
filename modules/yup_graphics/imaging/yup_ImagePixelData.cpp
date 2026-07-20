/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
void ImagePixelData::setPixelColor (int x, int y, Color color)
{
    setPixel (x, y, color.getARGB());
}

Color ImagePixelData::getPixelColor (int x, int y) const
{
    return Color (getPixel (x, y));
}

void ImagePixelData::fillColor (Color color)
{
    fill (color.getARGB());
}

//==============================================================================

std::vector<uint8> ImagePixelData::toRGBA (bool premultiplyAlpha) const
{
    const auto numPixels = static_cast<int> (static_cast<size_t> (width) * static_cast<size_t> (height));
    std::vector<uint8> result (static_cast<size_t> (numPixels) * 4);

    const auto* src = pixelBuffer.get();

    switch (format)
    {
        case PixelFormat::Grayscale:
            ColorVectorOperations::convertGrayscaleToRGBA (src, result.data(), numPixels);
            break;

        case PixelFormat::RGB:
            ColorVectorOperations::convertRGBToRGBA (src, result.data(), numPixels);
            break;

        case PixelFormat::RGBA:
            std::memcpy (result.data(), src, result.size());
            if (premultiplyAlpha)
                ColorVectorOperations::premultiplyRGBA (result.data(), numPixels);
            break;
    }

    return result;
}

} // namespace yup
