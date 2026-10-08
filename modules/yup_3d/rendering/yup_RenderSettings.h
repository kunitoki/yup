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
/** User-facing switches for how a Scene is rendered.

    Features that the device or the active tier cannot provide are ignored; ask
    RenderCapabilities what is actually in effect.
*/
struct YUP_API RenderSettings
{
    /** Which rendering path to use. */
    enum class TierPreference
    {
        automatic, ///< Full on capable desktop GPUs, raster elsewhere.
        raster,    ///< Force the raster path.
        full       ///< Request the full path (still raster when the device cannot run it).
    };

    TierPreference tier = TierPreference::automatic; ///< automatic picks full on capable desktop GPUs, raster elsewhere.
    int taaSamples = 16;                             ///< Full tier: jittered frames accumulated before the image is final.

    bool bloom = false;           ///< Glow around highlights. Raster: needs an MSAA rgba16float target.
    float bloomThreshold = 1.0f;  ///< Exposed radiance above which pixels glow.
    float bloomIntensity = 0.05f; ///< How much of the blurred glow is added.
    float bloomRadius = 0.85f;    ///< 0 to 1: how far the glow spreads.

    bool operator== (const RenderSettings&) const = default;
};

} // namespace yup
