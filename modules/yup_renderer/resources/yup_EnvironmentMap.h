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

    The radiance function is sampled once on the CPU into a latitude-longitude image, four
    samples per texel, and projected into nine spherical harmonics coefficients for the
    diffuse light. On the GPU, the renderer turns the image into a cube whose faces are a quarter
    of its width, then GGX-prefilters it into a cube with one mip per roughness, from mirror-like
    at mip 0 to fully rough at the last one, and keeps it. The environment itself never changes
    after construction, so any number of renderers, on any threads, can share it.

    @code
    // A blue sky over a dark ground
    auto sky = [] (const yup::Vector3<float>& direction)
    {
        return direction.getY() > 0.0f ? yup::Vector3<float> (0.4f, 0.6f, 1.0f) : yup::Vector3<float> (0.05f, 0.05f, 0.05f);
    };

    scene->getRoot()->attach<yup::EnvironmentNode> (new yup::EnvironmentMap (sky));
    @endcode

    @see EnvironmentNode
*/
class YUP_API EnvironmentMap : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<EnvironmentMap>;

    /** Returns the linear RGB radiance seen along a unit direction. */
    using RadianceFunction = std::function<Vector3<float> (const Vector3<float>& direction)>;

    static constexpr int defaultWidth = 512;   ///< The width of the sampled latitude-longitude image; cube faces are a quarter of it.
    static constexpr int defaultNumLevels = 7; ///< The number of roughness levels (cube mips).

    //==============================================================================
    /** Samples an environment.

        @param radiance   The radiance function, called four times for every texel of the image.
        @param width      The width of the latitude-longitude image, a power of two of at least 8.
                          Its height is half, and the cube faces are a quarter of it.
        @param numLevels  The number of roughness levels, each a cube mip half the size of the
                          previous one, limited by the face size.
    */
    explicit EnvironmentMap (const RadianceFunction& radiance, int width = defaultWidth, int numLevels = defaultNumLevels);

    /** Destructor. */
    ~EnvironmentMap() override;

    //==============================================================================
    /** Returns the number of roughness levels, the mips of the GPU cube. */
    int getNumLevels() const noexcept { return numLevels; }

    /** Returns the width and height of the GPU cube's faces at mip 0, in texels. */
    int getFaceSize() const noexcept { return width / 4; }

    /** Returns the perceptual roughness a level is filtered for, from 0 to 1. */
    static float getRoughnessForLevel (int level, int numLevels) noexcept;

    //==============================================================================
    /** Returns the nine irradiance coefficients of the diffuse light, computed on the CPU, in
        the order 1, y, z, x, xy, yz, 3z^2 - 1, xz, x^2 - y^2. Evaluate them with
        SphericalHarmonics::evaluateIrradiance().
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
    /** Returns the width of the latitude-longitude image, in texels; its height is half. */
    int getWidth() const noexcept { return width; }

    /** Returns the latitude-longitude image as linear RGBA half floats, row by row from the zenith. */
    const std::vector<uint16>& getPixels() const noexcept { return pixels; }

private:
    int width = defaultWidth;
    int numLevels = defaultNumLevels;
    std::vector<uint16> pixels; // The latitude-longitude image as half floats, kept to build again
    std::array<Vector3<float>, 9> irradiance;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvironmentMap)
};

} // namespace yup
