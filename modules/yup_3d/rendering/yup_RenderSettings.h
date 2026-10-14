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

    /** Shadow cascades of the shadow casting directional light, from 1 to 4. More cascades keep
        shadows sharp further away, at the cost of one depth pass each. 0 picks the tier default:
        4 on full, RenderCapabilities::defaultRasterCascades on raster. See RenderCapabilities::getCascadeCount().
    */
    int sunCascades = 0;

    /** Texels per side of each sun shadow cascade. Clamped to [256, 4096] on both tiers, see
        RenderCapabilities::getShadowResolution().

        It also sizes the shadow atlas shared by the point and spot lights: this many texels per side
        on raster, twice as many on full, rounded down to a power of two and at most 4096.
    */
    int shadowResolution = 2048;

    /** Soft shadows whose penumbra widens with the size of the light (LightNode::radius) and the
        distance between the caster and the receiver. Unset picks the tier default: on for full, off
        for raster, where true opts in to the same technique at a lower cost.
    */
    std::optional<bool> softShadows;

    /** Short screen-space shadows at the contact of objects, for the lights that ask for them (see
        LightNode::contactShadows). Full tier only: raster cannot provide them and ignores the setting,
        which is not an error.
    */
    bool contactShadows = true;

    /** Ambient occlusion: darkens the indirect light in creases and contacts. Unset picks the tier
        default: on for full, off for raster, where true opts in when the device can draw the depth
        and normal prepass it needs (see RenderCapabilities::Feature::ambientOcclusion). An
        aoDistance of 0 or less, or one that is not finite, turns it off.
    */
    std::optional<bool> ambientOcclusion;

    /** The world-space distance within which surfaces occlude each other for ambient occlusion. A
        distance of 0 or less, or one that is not finite, turns ambient occlusion off.
    */
    float aoDistance = 0.2f;

    /** Screen-space reflections: smooth surfaces reflect what the camera sees, taken from the
        previous frame, and fall back to the reflection probes and the environment where the
        reflection is not on screen. Full tier only, and only with temporal anti-aliasing (taaSamples
        above 1), whose history they read (see RenderCapabilities::Feature::screenSpaceReflections).
        Unset picks the tier default: on for full. The raster tier ignores it, which is not an error.
    */
    std::optional<bool> screenSpaceReflections;

    bool operator== (const RenderSettings&) const = default;
};

} // namespace yup
