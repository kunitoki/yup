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
/** Defines how a transparency layer's mask turns into the layer's visibility.

    @see Graphics::TransparencyLayer::addMask
*/
enum class LayerMaskMode : uint8
{
    Alpha,            ///< The layer shows where the mask is opaque.
    InvertedAlpha,    ///< The layer shows where the mask is transparent.
    Luminance,        ///< The layer shows where the mask is bright.
    InvertedLuminance ///< The layer shows where the mask is dark.
};

} // namespace yup
