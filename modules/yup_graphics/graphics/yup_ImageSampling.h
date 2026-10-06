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

namespace yup
{

//==============================================================================
/** Defines how an image paint repeats outside of the image along one axis. */
enum class ImageWrap : uint8
{
    Clamp,  ///< The edge pixels extend outward.
    Repeat, ///< The image tiles.
    Mirror  ///< The image tiles, flipping every other copy.
};

/** Defines how an image paint is sampled when it is scaled. */
enum class ImageFilter : uint8
{
    Linear, ///< Smooth interpolation between pixels.
    Nearest ///< The nearest pixel, keeping hard edges (pixel art).
};

//==============================================================================
/** Describes how an image paint is sampled.

    @see Graphics::setFillImage, Graphics::setStrokeImage
*/
struct YUP_API ImageSampling
{
    /** How the image repeats horizontally. */
    ImageWrap wrapX = ImageWrap::Clamp;

    /** How the image repeats vertically. */
    ImageWrap wrapY = ImageWrap::Clamp;

    /** How the image is filtered when scaled. */
    ImageFilter filter = ImageFilter::Linear;
};

} // namespace yup
