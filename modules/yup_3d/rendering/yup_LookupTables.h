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
    - layer 1: the GGX lobe magnitude x, its Fresnel part y, the sheen albedo (0 for now) and 1.

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
