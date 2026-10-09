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

namespace yup
{

//==============================================================================
/** The light surrounding a scene, for reflections and soft diffuse light.

    The environment is baked once on the CPU from a radiance function into a latitude-longitude
    image with one level per roughness, from mirror-like at level 0 to fully rough at the last
    level, and into nine spherical harmonics coefficients for the diffuse light.

    Level 0 samples the function four times per texel. Every other level is the previous one
    halved and blurred on the sphere by a gaussian matching the reflections of its roughness,
    which keeps the energy of small bright lights like a sun.

    @code
    // A blue sky over a dark ground
    auto sky = [] (const yup::Vector3<float>& direction)
    {
        return direction.getY() > 0.0f ? yup::Vector3<float> (0.4f, 0.6f, 1.0f) : yup::Vector3<float> (0.05f, 0.05f, 0.05f);
    };

    scene->setEnvironment (new yup::EnvironmentMap (sky));
    @endcode

    @see Scene::setEnvironment
*/
class YUP_API EnvironmentMap : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<EnvironmentMap>;

    /** Returns the linear RGB radiance seen along a unit direction. */
    using RadianceFunction = std::function<Vector3<float> (const Vector3<float>& direction)>;

    static constexpr int defaultWidth = 512;   ///< The width of level 0, its height is half.
    static constexpr int defaultNumLevels = 7; ///< The number of roughness levels.

    //==============================================================================
    /** Bakes an environment.

        @param radiance   The radiance function, called four times for every texel of level 0.
        @param width      The width of level 0, a power of two of at least 8. Its height is half.
        @param numLevels  The number of levels, each half the size of the previous one.
    */
    explicit EnvironmentMap (const RadianceFunction& radiance, int width = defaultWidth, int numLevels = defaultNumLevels);

    /** Destructor. */
    ~EnvironmentMap() override;

    //==============================================================================
    /** Returns the number of roughness levels. */
    int getNumLevels() const noexcept { return static_cast<int> (levels.size()); }

    /** Returns the width of a level in texels. */
    int getLevelWidth (int level) const noexcept;

    /** Returns the height of a level in texels. */
    int getLevelHeight (int level) const noexcept;

    /** Returns the linear RGBA pixels of a level, four floats per texel, rows from the zenith down. */
    Span<const float> getLevelPixels (int level) const noexcept;

    /** Returns the perceptual roughness a level is baked for, from 0 to 1. */
    static float getRoughnessForLevel (int level, int numLevels) noexcept;

    /** Returns the gaussian blur, in radians, that approximates the reflections of a roughness. */
    static float getBlurForRoughness (float roughness) noexcept;

    //==============================================================================
    /** Returns the diffuse light received around a unit normal: the cosine weighted average
        radiance, to be multiplied by the diffuse color.
    */
    Vector3<float> getIrradiance (const Vector3<float>& normal) const noexcept;

    /** Returns the nine coefficients getIrradiance() evaluates, in the order 1, y, z, x, xy, yz,
        3z^2 - 1, xz, x^2 - y^2.
    */
    const std::array<Vector3<float>, 9>& getIrradianceCoefficients() const noexcept { return irradiance; }

    //==============================================================================
    /** Returns the texture coordinates of a unit direction: u around the vertical axis, starting
        towards +Z, and v from the zenith (0) to the nadir (1).
    */
    static Point<float> getTextureCoordinates (const Vector3<float>& direction) noexcept;

    /** Returns the unit direction at some texture coordinates, the inverse of getTextureCoordinates(). */
    static Vector3<float> getDirection (Point<float> textureCoordinates) noexcept;

    //==============================================================================
    /** Returns the GPU texture for a device, with every level as a mip level, uploading it if needed. */
    GpuTexture::Ptr getGpuTexture (const GpuDevice::Ptr& device);

    /** Returns the GPU sampler for a device: trilinear, repeating around and clamped at the poles. */
    GpuSampler::Ptr getGpuSampler (const GpuDevice::Ptr& device);

private:
    void useDevice (const GpuDevice::Ptr& device);

    int width = defaultWidth;
    std::vector<std::vector<float>> levels;
    std::array<Vector3<float>, 9> irradiance;

    GpuDevice::Ptr cachedDevice;
    GpuTexture::Ptr gpuTexture;
    GpuSampler::Ptr gpuSampler;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvironmentMap)
};

} // namespace yup
