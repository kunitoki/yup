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
/** The shared lookup array of the shading passes, set 0 binding 7 (lookupLuts).

    A size x size x numLayers rgba16float texture2DArray, generated on the CPU from LtcTables and
    uploaded, so it is the same on every tier and device:

    - layer 0: the inverse LTC matrices of the area lights (m00, m02, m11, m20, with m22 = 1);
    - layer 1: the GGX lobe magnitude x, its Fresnel part y, the sheen albedo z (computeSheenAlbedo())
      and 1.

    Layer 1 is also the split sum of the environment specular (Karis 2013): the single-scattering
    reflectance is f0 (x - y) + f90 y. Both layers share the LtcTables layout: x is the view angle,
    sqrt (1 - n.v), and y the roughness, sqrt (alpha), sampled at the texel centers
    ((u * (size - 1) + 0.5) / size).

    The textures belong to the caller, which creates them once per device and releases them with it.

    @internal
*/
class YUP_API LookupTables
{
public:
    static constexpr int size = LtcTables::size; ///< The width and height of each layer.
    static constexpr int numLayers = 2;          ///< The LTC matrices, then the lobe magnitudes.

    /** The perceptual sheen roughness the sheen albedo and the sheen lobe are floored at, about 0.079:
        below it the Charlie distribution narrows towards a singular one. It is the center of row 5,
        so the shaders read that row alone at the floor.
    */
    static constexpr float minSheenRoughness = 5.0f / static_cast<float> (LtcTables::size - 1);

    /** The directional albedo E of the KHR_materials_sheen lobe of a white sheen, for every cell of
        the tables, row by row (size * size values, the roughness row times size plus the view angle
        column, as layer 1 z holds them). E (n.v, alpha) is the integral over the light directions of
        D V (n.l), with the Charlie distribution (Estevez and Kulla 2017, "Production Friendly
        Microfacet Sheen BRDF") of alpha = roughness^2 and the visibility of Neubelt and Pettineo
        2013 ("Crafting a Next-Gen Material Pipeline for The Order: 1886"), 1 / (4 (n.l + n.v - n.l n.v)).

        The rows are the perceptual sheen roughness, floored at minSheenRoughness, and the columns
        sqrt (1 - n.v), with n.v floored at 1e-4 as the shaders do. That visibility is not energy
        conserving at grazing angles, where a smooth sheen reflects more than it receives: the albedo
        is limited to 1, so the layers under the sheen are dimmed at most to black.

        Integrated over the half vectors, where D depends on alpha and n.h alone: about 1.1 to 1.6 ms in
        Release and 11 ms in Debug (measured on an Apple M-series machine).
    */
    static std::vector<float> computeSheenAlbedo();

    /** Creates the lookup array for a device.

        Returns a new texture on every call, or nullptr if the device is null or the upload fails.
    */
    static GpuTexture::Ptr createTexture (const GpuDevice::Ptr& device);

    /** Creates a 1 x 1 lookup array of zeros, with numLayers layers, which stands in for the
        tables when they cannot be created: area lights and the environment then lose their
        specular light, and keep their diffuse light.

        Returns nullptr if the device is null or the upload fails.
    */
    static GpuTexture::Ptr createBlackTexture (const GpuDevice::Ptr& device);
};

} // namespace yup
