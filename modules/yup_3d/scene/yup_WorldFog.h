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
/** Exponential height fog filling the whole scene.

    The extinction at a height y is density * exp (-heightFalloff * (y - baseHeight)): uniform when
    heightFalloff is 0, thinning upwards when it is positive and thickening upwards when it is
    negative. Where the fog gets denser, far below the base (or far above it for a negative
    heightFalloff), the extinction stops growing at density * exp (40), so it stays finite.

    The fog is applied over the rendered scene by a pass of its own, from the camera to the first
    surface or to the background. It is lit by the ambient light plus the environment's mean light,
    when the scene has one, and by the sun, as single scattering with no shadowing: light reaches the
    fog everywhere, even behind objects that would shadow it.

    @see Scene::setWorldFog
*/
struct YUP_API WorldFog
{
    bool enabled = false;                            ///< True to render the fog.
    std::array<float, 3> color { 1.0f, 1.0f, 1.0f }; ///< Scattering albedo, linear RGB.
    float density = 0.02f;                           ///< Extinction per world unit at baseHeight.
    float heightFalloff = 0.0f;                      ///< Exponential falloff per world unit above baseHeight: 0 for uniform fog, negative to thicken upwards.
    float baseHeight = 0.0f;                         ///< The height, in world units, where the extinction equals density.
    float anisotropy = 0.0f;                         ///< Henyey-Greenstein g, from -0.95 (back scattering) to 0.95 (forward).

    /** Returns the extinction per world unit at a height, 0 when disabled. */
    float getDensity (float height) const noexcept;

    /** Returns the optical depth (integrated extinction) along a straight segment, exact for any slope.

        exp (-getOpticalDepth (a, b)) is the transmittance from a to b. It is the integral of
        getDensity() along the segment, including the part far below the base where the extinction
        stops growing. It is 0 when disabled.
    */
    float getOpticalDepth (const Vector3<float>& from, const Vector3<float>& to) const noexcept;

    bool operator== (const WorldFog&) const = default;
};

} // namespace yup
